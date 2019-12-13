#pragma once
#include <iostream>
#include "../JuceLibraryCode/JuceHeader.h"
#include "../eigen-master/Eigen/Dense"
#include "VAD.h"

using namespace Eigen;
using namespace std;
using namespace dsp;



class MultiChannelFilter
{
public:
	MultiChannelFilter(int nr_channels, int refChannel, float SPP_threshold, float forgettingFactor, int Fs, std::vector<std::vector<float>>& inputSignal);
	~MultiChannelFilter();

	std::vector<MatrixXcf> stft(std::vector < std::vector<float>>& y_TD);
	std::vector<float> istft(MatrixXcf& X);
	dsp::Matrix < std::complex<float>> Filter(std::vector<std::vector<float>> inputSignal);
	std::vector<float> Filter2(std::vector<std::vector<float>> inputSignal);
	dsp::Matrix<float>* getSPP();

private:
	VAD* vad;
	dsp::Matrix<float>* SPP;

	//STFT variables
	int nfft = 128;
	int noverlap = 2;
	int overlap_start = 64;
	bool twoside = false;
	int fftorder = 7;
	int fs;
	int N_half = nfft / 2 + 1;
	int halfFs;
	std::vector<MatrixXcf> y_STFT;
	MatrixXcf Y;
	std::vector<float> real;
	std::vector<float> frame;
	std::vector<float> out_TD;
	dsp::Complex<float> out_fft[512];
	dsp::Complex<float> temp[512];
	int frame_start;
	int frame_end;


	//filter variables
	float SPP_thr;
	int M;
	float lambda;
	int N_frames;
	int N_freqs;
	int ref;
	float alpha_n = 0.9;
	float alpha_s = 0.92;
	float Xi_min = exp(-9);
	int nrSamples;
	int isFirstTime = 1;

	std::vector<MatrixXcf> Rnn_prev;
	std::vector<MatrixXcf> Ryy_prev;

	MatrixXcf output;

	VectorXcf y;
	MatrixXcf y2;

	MatrixXcf Rnn_inv;
	MatrixXcf Rnn;
	MatrixXcf Ryy;
	MatrixXcf Rxx;

	VectorXcf rtf;
	VectorXf  eref;
	VectorXcf filter;

	MatrixXf sig_n;
	MatrixXf sig_s;
	MatrixXf G_sc_stft;
	MatrixXcf S_sc_stft;

	dsp::FFT* fft;
	dsp::WindowingFunction<float>* fwindow;
};
