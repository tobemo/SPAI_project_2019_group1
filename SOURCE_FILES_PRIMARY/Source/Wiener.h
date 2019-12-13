#pragma once
#include "VAD.h"

class Wiener
{
public:
	Wiener(float f_s, int signalLength, int fft_num);
	~Wiener();
	vector<float> filterSignal(vector<float>* x);
	vector<float> filterMultichannel(dsp::Matrix<complex<float>>* Multi_S_sc_stft, dsp::Matrix<float>* multi_spp);
	void initializeFilter(vector<float>* x, int numberOfFrames); //only once at the start time
	std::vector<dsp::Matrix<std::complex<float>>> stft(dsp::Matrix<float> x, int fs, String window, int nfft, int noverlap, bool twoside);
	std::vector<float> istft(dsp::Matrix<std::complex<float>> X, int fftorder, int nfft, int noverlap, bool twoside);

private:
	float fs;                               // sampling frequency
	int Lsignal;                            // signal length
	int N_fft;                              // number of FFT points
	int R_fft;                              // shifting(50 % overlap)
	dsp::WindowingFunction<float>* anWin;   // analysis window
	int N_half;                             // number of bins in onesided FFT

	/*filter*/
	float sppThreshold = 0.7;
	float alpha_n = 0.8;
	float alpha_s = 0.8;
	float Xi_min = exp(-6);

	VAD* myVAD;
	

	/*results*/
	dsp::Matrix<float>* sig_n;
	dsp::Matrix<float>* sig_s;
	dsp::Matrix<float>* G_sc_stft;
	dsp::Matrix<complex<float>>* S_sc_stft;

};

