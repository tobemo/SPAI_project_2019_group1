#include "Wiener.h"

Wiener::Wiener(float f_s, int signalLength, int fft_num)
{
	fs = f_s;
	Lsignal = signalLength;
	N_fft = fft_num;
	R_fft = N_fft / 2;
	N_half = floor(N_fft / 2) + 1;
	anWin = new dsp::WindowingFunction<float>(N_fft, dsp::WindowingFunction<float>::hann);
	myVAD = new VAD(signalLength, N_fft, R_fft, 7); // the order needs to exactly fit the length!!!!! now 128
	//mySTFT = new STFT();                 // needs to update later

	/*step 2: construct results matrixes*/
	int numRow = myVAD->getnFbins();
	int numCol = myVAD->getnFrames();

	sig_n = new dsp::Matrix<float>(numRow, numCol);
	sig_s = new dsp::Matrix<float>(numRow, numCol);
	G_sc_stft = new dsp::Matrix<float>(numRow, numCol);
	S_sc_stft = new dsp::Matrix<complex<float>>(numRow, numCol);
}

Wiener::~Wiener()
{
}

void Wiener::initializeFilter(vector<float>* x, int numberOfFrames)
{
	myVAD->init_noise_tracker_ideal_vad(x, numberOfFrames);
}

vector<float> Wiener::filterSignal(vector<float>* x)
{
	/*step 1: calculate stft*/
	/*step 1.1: convert vector to matrix*/
	dsp::Matrix<float> oneChannelInput = dsp::Matrix<float>(1, Lsignal, x->data());
	vector <dsp::Matrix<dsp::Complex<float>>> stftResult = stft(oneChannelInput, fs, "hann", N_fft, 2, 0);
	dsp::Matrix<dsp::Complex<float>> stft = stftResult.at(0);

	/*step 2: calculate the Speech Presence Probability*/
	myVAD->calc_SPP(x);
	dsp::Matrix<float>* spp = myVAD->getSPP();
	//vector<float> testSPP(spp->getRawDataPointer(), spp->getRawDataPointer() + 162434);
	/*step 3: filter each frequency bin in each frame*/
	int numFrame = myVAD->getnFrames();
	int numFbins = myVAD->getnFbins();


	for (int i = 1; i < numFrame; i++)
	{
		for (int fbin = 0; fbin < numFbins; fbin++)
		{
			if ((*spp)(fbin, i) < sppThreshold) //noise only
			{
				//sig_n(k, l) = alpha_n * sig_n(k, l - 1) + (1 - alpha_n) * abs(y_STFT(k, l, 1)) ^ 2;
				(*sig_n)(fbin, i) = alpha_n * ((*sig_n)(fbin, i - 1)) + (1 - alpha_n) * abs(stft(fbin, i)) * abs(stft(fbin, i)); //the first frame only
			}
			else
			{
				(*sig_n)(fbin, i) = (*sig_n)(fbin, i - 1);
			}


			(*sig_s)(fbin, i) = max(alpha_s * abs((*S_sc_stft)(fbin, i - 1)) * abs((*S_sc_stft)(fbin, i - 1)) + (1 - alpha_s) * (abs(stft(fbin, i)) * abs(stft(fbin, i)) - (*sig_n)(fbin, i)), Xi_min * (*sig_n)(fbin, i));
			(*G_sc_stft)(fbin, i) = (*sig_s)(fbin, i) / ((*sig_s)(fbin, i) + (*sig_n)(fbin, i));
			(*S_sc_stft)(fbin, i) = (*G_sc_stft)(fbin, i) * stft(fbin, i);
			//S_sc_stft contains filtered frequency components
		}
	}
	/*#######################################################################################################################*/
	/*step 5: transform to the time domain*/
//	vector<float> test2(G_sc_stft->getRawDataPointer(), G_sc_stft->getRawDataPointer() + 162434);
	//vector<complex<float>> test(S_sc_stft->getRawDataPointer(), S_sc_stft->getRawDataPointer() + 162434);
	vector<float> filteredSignal = istft(*S_sc_stft, 7, N_fft, 2, 0);
	return filteredSignal;
}


/*this function works for the multichannel post filter*/
vector<float> Wiener::filterMultichannel(dsp::Matrix<complex<float>>* Multi_S_sc_stft, dsp::Matrix<float>* multi_spp) {

	/*step 1: filter each frequency bin in each frame*/
	int numFrame = multi_spp->getNumColumns();
	int numFbins = multi_spp->getNumRows();

	for (int i = 1; i < numFrame; i++)
	{
		for (int fbin = 0; fbin < numFbins; fbin++)
		{
			if ((*multi_spp)(fbin, i) < sppThreshold) //noise only
			{
				//sig_n(k, l) = alpha_n * sig_n(k, l - 1) + (1 - alpha_n) * abs(y_STFT(k, l, 1)) ^ 2;
				(*sig_n)(fbin, i) = alpha_n * ((*sig_n)(fbin, i - 1)) + (1 - alpha_n) * abs((*Multi_S_sc_stft)(fbin, i)) * abs((*Multi_S_sc_stft)(fbin, i)); //the first frame only
			}
			else
			{
				(*sig_n)(fbin, i) = (*sig_n)(fbin, i - 1);
			}

			(*sig_s)(fbin, i) = max(alpha_s * abs((*S_sc_stft)(fbin, i - 1)) * abs((*S_sc_stft)(fbin, i - 1)) + (1 - alpha_s) * (abs((*Multi_S_sc_stft)(fbin, i)) * abs((*Multi_S_sc_stft)(fbin, i)) - (*sig_n)(fbin, i)), Xi_min * (*sig_n)(fbin, i));
			(*G_sc_stft)(fbin, i) = (*sig_s)(fbin, i) / ((*sig_s)(fbin, i) + (*sig_n)(fbin, i));
			(*S_sc_stft)(fbin, i) = (*G_sc_stft)(fbin, i) * (*Multi_S_sc_stft)(fbin, i);
			//S_sc_stft contains filtered frequency components
		}
	}
	/*#######################################################################################################################*/
	/*step 5: transform to the time domain*/
	/*s_sc_TD = calc_ISTFT(S_sc_stft, win, N_fft, N_fft/R_fft, 'onesided');*/
	//copy(mySTFT->calculateISTFT(&S_sc_stft), mySTFT->calculateISTFT(&S_sc_stft)+Lsignal, x->begin())
	//x->assign() = mySTFT->calculateISTFT(S_sc_stft);
	vector<float> filteredSignal = istft(*S_sc_stft, 7, N_fft, 2, 0);
	return filteredSignal;

}



std::vector<dsp::Matrix<std::complex<float>>> Wiener::stft(dsp::Matrix<float> x, int fs, String window, int nfft, int noverlap, bool twoside) {
	//VARIABLES
	const int N_half = nfft / 2 + 1;
	int halfFS = fs / 2;
	std::vector<dsp::Matrix<std::complex<float>>> ret;

	//INIT
	const int L = floor((x.getNumColumns() - nfft + (nfft / noverlap)) / (nfft / noverlap));
	const int M = x.getNumRows();
	dsp::Matrix<std::complex<float>> X = dsp::Matrix<std::complex<float>>::Matrix(N_half, L);
	if (twoside == 1) {
		X = dsp::Matrix<std::complex<float>>::Matrix(nfft, L);
	}

	//OLA PROCESSING
	int block_start = floor(nfft / noverlap);
	int frame_length = nfft;
	std::vector<float> x_frame;
	dsp::Complex<float> X_frame[512];
	dsp::Complex<float> temp[512];
	dsp::FFT fft(7);
	for (int m = 0; m < M; m++) {
		for (int l = 0; l < L; l++) {

			int local_start = l * block_start;
			int local_end = local_start + nfft;
			for (local_start; local_start < local_end; local_start++) {
				x_frame.push_back(x(m, local_start));
			}
			//float* temp = x_frame.data();
			///Block fft
			dsp::WindowingFunction<float> fwindow(frame_length + 1, dsp::WindowingFunction<float>::hann, false, 0.0f); //Na window x_frame dubbele van in matlab
			fwindow.multiplyWithWindowingTable(&*x_frame.begin(), frame_length);
			for (int i = 0; i < x_frame.size(); i++) {
				temp[i] = x_frame[i];
			}


			fft.perform(temp, X_frame, false);

			if (twoside == 0) {
				int n = nfft / 2 + 1;
				for (int i = 0; i < n; i++) {
					X(i, l) = X_frame[i];
				}
			}
			else if (twoside == 1) {
				for (int i = 0; i < nfft; i++) {
					X(i, l) = X_frame[i];
				}
				int o = 0;
			}
			x_frame.clear();
		}
		ret.push_back(X);
	}
	return ret;
}

std::vector<float> Wiener::istft(dsp::Matrix<std::complex<float>> X, int fftorder, int nfft, int noverlap, bool twoside) {
	//INIT
	int overlap_start = floor(nfft / noverlap);
	int L = X.getNumColumns();
	int R = X.getNumRows();

	const int samples = (nfft / noverlap) * (L - 1) + nfft;
	std::vector<float> x(samples, 0);
	dsp::FFT fft(fftorder);
	dsp::WindowingFunction<float> fwindow(nfft + 1, dsp::WindowingFunction<float>::hann, false, 0.0f);

	//OLA PROCESSING
	dsp::Complex<float> x_frame[512];
	dsp::Complex<float> temp[512];
	std::vector<float*> x_temp;

	///One or twosided
	if (twoside == 0) {
		int n1 = R - 1;
		int n2 = 2 * (R - 1);

		///For each timeframe
		for (int l = 0; l < L; l++) {

			///Perform inverse fft for each timeframe
			for (int i = 0; i < n1; i++) {
				temp[i] = X(i, l);
				temp[n1 + i] = conj(X(n1 - i, l));
			}
			fft.perform(temp, x_frame, true);
			std::vector<float>* tempf = new std::vector<float>;

			///Take the real part + window
			for (int i = 0; i < n2; i++) {
				(*tempf).push_back((float)x_frame[i]._Val[0]);
			}
			fwindow.multiplyWithWindowingTable((*tempf).data(), nfft);

			///Add everthing together
			int frame_start = l * overlap_start;
			int frame_end = frame_start + nfft;
			int i = 0;
			for (frame_start; frame_start < frame_end; frame_start++) {
				x[frame_start] = x[frame_start] + (*tempf)[i];
				i++;
			}
		}
	}
	else if (twoside == 1) {
		for (int l = 0; l < L; l++) {

			for (int i = 0; i < R; i++) {
				temp[i] = X(i, l);
			}

			fft.perform(temp, x_frame, true);
			std::vector<float>* tempf = new std::vector<float>;
			for (int i = 0; i < R; i++) {
				(*tempf).push_back((float)x_frame[i]._Val[0]);
			}
			fwindow.multiplyWithWindowingTable((*tempf).data(), nfft);

			int frame_start = l * overlap_start;
			int frame_end = frame_start + nfft;
			int i = 0;
			for (frame_start; frame_start < frame_end; frame_start++) {
				x[frame_start] = x[frame_start] + (*tempf)[i];
				i++;
			}
		}
	}
	return x;
}



