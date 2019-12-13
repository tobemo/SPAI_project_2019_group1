
#include "MultiChannelFilter.h"
#include "../JuceLibraryCode/JuceHeader.h"

MultiChannelFilter::MultiChannelFilter(int nr_channels, int refChannel, float SPP_threshold, float forgettingFactor, int Fs, std::vector<std::vector<float>>& inputSignal)
{
	//General variables
	SPP_thr = SPP_threshold;
	lambda = forgettingFactor;
	nrSamples = inputSignal.at(0).size();
	M = nr_channels;
	fs = Fs;
	ref = refChannel;

	//FFT variables
	halfFs = Fs / 2;
	N_frames = floor((nrSamples - nfft + overlap_start) / overlap_start);
	N_freqs = overlap_start + 1;
	out_TD = std::vector<float>(nrSamples, 0);
	Y = MatrixXcf(N_half, N_frames);
	fft = new dsp::FFT(fftorder);
	fwindow = new dsp::WindowingFunction<float>(nfft + 1, dsp::WindowingFunction<float>::hann, false, 0.0f);

	//Multichannel variables
	output = MatrixXcf::Zero(N_freqs, N_frames);
	MatrixXcf z = MatrixXcf::Zero(M, M);
	Rnn_prev = std::vector<MatrixXcf>(N_freqs, z);
	Ryy_prev = std::vector<MatrixXcf>(N_freqs, z);
	y = VectorXcf(M);
	y2 = MatrixXcf(M, M);
	Rnn = MatrixXcf(M, M);
	Ryy = MatrixXcf(M, M);
	Rxx = MatrixXcf(M, M);
	Rnn_inv = MatrixXcf(M, M);
	rtf = VectorXcf::Zero(M);
	filter = VectorXcf::Ones(M) * 1 / M;
	eref = VectorXf::Zero(M);
	eref(ref - 1, 0) = 1;

	//VAD init
	vad = new VAD(nrSamples, nfft, overlap_start, fftorder);
	vad->init_noise_tracker_ideal_vad(&inputSignal.at(ref - 1), 5);
}

MultiChannelFilter::~MultiChannelFilter()
{
}

std::vector<MatrixXcf> MultiChannelFilter::stft(std::vector<std::vector<float>>& y_TD)
{

	for (int m = 0; m < M; m++)
	{
		for (int l = 0; l < N_frames; l++)
		{

			frame_start = l * overlap_start;
			frame_end = frame_start + nfft;

			for (frame_start; frame_start < frame_end; frame_start++)
			{
				frame.push_back(y_TD[m][frame_start]);
			}

			///Block fft
			fwindow->multiplyWithWindowingTable(&frame[0], nfft);

			for (int i = 0; i < frame.size(); i++)
			{
				temp[i] = frame[i];
			}

			fft->perform(temp, out_fft, false);

			for (int i = 0; i < N_half; i++)
			{
				Y(i, l) = out_fft[i];
			}

			frame.clear();
		}
		y_STFT.push_back(Y);
	}
	return y_STFT;

}

std::vector<float> MultiChannelFilter::istft(MatrixXcf& X)
{
	//INIT
	int L = X.cols();
	int R = X.rows();

	const int samples = nrSamples;
	std::vector<float> x(samples, 0);


	//OLA PROCESSING
	dsp::Complex<float> x_frame[512];
	dsp::Complex<float> temp[512];
	std::vector<float*> x_temp;

	///One or twosided
	int n1 = R - 1;
	int n2 = 2 * (R - 1);

	///For each timeframe
	for (int l = 0; l < L; l++) {

		///Perform inverse fft for each timeframe
		for (int i = 0; i < n1; i++) {
			temp[i] = X(i, l);
			temp[n1 + i] = conj(X(n1 - i, l));
		}
		fft->perform(temp, x_frame, true);
		std::vector<float>* tempf = new std::vector<float>;

		///Take the real part + window
		for (int i = 0; i < n2; i++) {
			(*tempf).push_back((float)x_frame[i]._Val[0]);
		}
		fwindow->multiplyWithWindowingTable((*tempf).data(), nfft);

		///Add everthing together
		int frame_start = l * overlap_start;
		int frame_end = frame_start + nfft;
		int i = 0;
		for (frame_start; frame_start < frame_end; frame_start++) {
			x[frame_start] = x[frame_start] + (*tempf)[i];
			i++;
		}
	}


	return x;
}

dsp::Matrix < std::complex<float>> MultiChannelFilter::Filter(std::vector<std::vector<float>> inputSignal)
{
	vad->calc_SPP(&inputSignal.at(ref - 1));
	SPP = vad->getSPP();

	//calculate STFT
	y_STFT.clear();
	std::vector<MatrixXcf> signal = stft(inputSignal);

	for (int l = isFirstTime; l < N_frames; l++) {
		for (int k = 0; k < N_freqs; k++) {


			//Get current frame from all channels
			for (int m = 0; m < M; m++) {
				y(m) = signal.at(m)(k, l);
			}
			y2 = y * y.adjoint(); //y2 = y_mc*y_mc'

			//Calculate noise only and speech + noise correlation matrices
			float s = (*SPP)(k, l);
			if (s < SPP_thr || l < 3)
			{
				Rnn = lambda * Rnn_prev.at(k) + (1 - lambda) * y2;
				Rnn_prev.at(k) = Rnn;
				Ryy = Ryy_prev.at(k);
			}
			else {
				Rnn = Rnn_prev.at(k);
				Ryy = lambda * Ryy_prev.at(k) + (1 - lambda) * y2;
				Ryy_prev.at(k) = Ryy;
			}

			/*
			//MWF filter
			Rxx = Ryy - Rnn;
			filter = Ryy.completeOrthogonalDecomposition().pseudoInverse() * Rxx * eref;
			output(k, l) = filter.adjoint() * y;
			*/

			//MVDR filter
			//Calculate RTF vector
			Rxx = Ryy - Rnn;
			MatrixXcf erefT = eref.transpose();
			MatrixXcf den(1, 1);
			den = (erefT * Rxx) * eref;
			rtf = (Rxx * eref) * den.completeOrthogonalDecomposition().pseudoInverse();

			//Calculate Filter
			Rnn_inv = Rnn.completeOrthogonalDecomposition().pseudoInverse();
			den = (rtf.adjoint() * Rnn_inv) * rtf;
			filter = Rnn_inv * rtf * den.completeOrthogonalDecomposition().pseudoInverse();

			//Apply filter and return new value
			output(k, l) = filter.adjoint() * y;

		}

	}

	isFirstTime = 0;
	MatrixXcf out  = output.transpose();
	dsp::Matrix<std::complex<float>> o = dsp::Matrix<std::complex<float>>(out.cols(), out.rows(), out.data());
	//std::vector<float> out = istft(output);
	return o;
}


std::vector<float> MultiChannelFilter::Filter2(std::vector<std::vector<float>> inputSignal)
{
	vad->calc_SPP(&inputSignal.at(ref - 1));
	SPP = vad->getSPP();

	//calculate STFT
	y_STFT.clear();
	std::vector<MatrixXcf> signal = stft(inputSignal);

	for (int l = isFirstTime; l < N_frames; l++) {
		for (int k = 0; k < N_freqs; k++) {


			//Get current frame from all channels
			for (int m = 0; m < M; m++) {
				y(m) = signal.at(m)(k, l);
			}
			y2 = y * y.adjoint(); //y2 = y_mc*y_mc'

			//Calculate noise only and speech + noise correlation matrices
			float s = (*SPP)(k, l);
			if (s < SPP_thr || l < 3)
			{
				Rnn = lambda * Rnn_prev.at(k) + (1 - lambda) * y2;
				Rnn_prev.at(k) = Rnn;
				Ryy = Ryy_prev.at(k);
			}
			else {
				Rnn = Rnn_prev.at(k);
				Ryy = lambda * Ryy_prev.at(k) + (1 - lambda) * y2;
				Ryy_prev.at(k) = Ryy;
			}

			/*
			//MWF filter
			Rxx = Ryy - Rnn;
			filter = Ryy.completeOrthogonalDecomposition().pseudoInverse() * Rxx * eref;
			output(k, l) = filter.adjoint() * y;
			*/

			//MVDR filter
			//Calculate RTF vector
			Rxx = Ryy - Rnn;
			MatrixXcf erefT = eref.transpose();
			MatrixXcf den(1, 1);
			den = (erefT * Rxx) * eref;
			rtf = (Rxx * eref) * den.completeOrthogonalDecomposition().pseudoInverse();

			//Calculate Filter
			Rnn_inv = Rnn.completeOrthogonalDecomposition().pseudoInverse();
			den = (rtf.adjoint() * Rnn_inv) * rtf;
			filter = Rnn_inv * rtf * den.completeOrthogonalDecomposition().pseudoInverse();

			//Apply filter and return new value
			output(k, l) = filter.adjoint() * y;

		}

	}

	isFirstTime = 0;
	std::vector<float> out = istft(output);
	return out;
}

dsp::Matrix<float>* MultiChannelFilter::getSPP()
{
	return SPP;
}



/*
for (int k = 0; k < N_freqs; k++)
{
	float s = *(SPP->getRawDataPointer() + 1 + k * N_frames);
	if (s < SPP_thr) //noise only
	{
		sig_n(k, l) = alpha_n * sig_n(k, l - 1) + (1 - alpha_n) * abs(output(k, l))* abs(output(k, l));
	}
	else
	{
		sig_n(k, l) = sig_n(k, l-1);
	}


	sig_s(k, l) = max(alpha_s * abs(S_sc_stft(k, l))* abs(S_sc_stft(k, l)) + (1 - alpha_s) * (abs(output(k, l)) * abs(output(k, l))- sig_n(k, l)), Xi_min * sig_n(k, l));
	G_sc_stft(k, l) = sig_s(k, l) / (sig_s(k,l) + sig_n(k, l));
	S_sc_stft(k, l) = G_sc_stft(k,l) * output(k, l);
}*/