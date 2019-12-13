/*
  ==============================================================================
    VAD.h
    Created: 21 Nov 2019 5:24:13pm
    Author:  Liuyin Yang
	Voice Activity Detection:
	used for online noise cancellation
	1. use init_noise_tracker_ideal_vad() to initialize few frames of the signal first
	2. use calc_SPP() later to calculate
	3. after calculation, use the following lines to get access to spp
			dsp::Matrix<float>* SPP = testVAD.getSPP();
			float* rawSPP = SPP->getRawDataPointer();
			now rawSPP is a vector contains all elements of SPP(in a row order)
	####IMPORTANT####
	After initialized VAD, you can't change the signal length any more
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <vector>
using namespace std;
//==============================================================================

class VAD    
{
public:
    VAD(int signalLength, int fLength, int shiftLength, int order);
    ~VAD();

    void init_noise_tracker_ideal_vad(vector<float>* x, int numberOfFrames);
    void calc_SPP(vector<float>* x);

    dsp::Matrix<float>* getSPP();
    dsp::Matrix<float>* getnoisePower();

	int getnFrames();
	int getnFbins();
	
private:
	int frLeng;				 // the length of each frame
	int fshift;				 // specify the overlapping size
	int nFrames;             // total frames
	int nFrequency;          // useful frequency components = frLeng/2 + 1

	/*constants for a posteriori SPP*/
	float alphaPH1mean = 0.9;
	float alphaPSD = 0.8;
	float q = 0.5;                     // a priori probability of speech presence :
	float priorFact = q / (1 - q);
	float xiOptDb = 15;                // optimal fixed a priori SNR for SPP estimation
	float xiOpt = pow(10, xiOptDb / 10);
	float logGLRFact = log(1 / (1 + xiOpt));
	float GLRexp = xiOpt / (1 + xiOpt);

	//vector<float>* noisy;    // input noisy signal
	vector<float>* noisePow;   // previous noise power
	vector<float>* PH1mean; 

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VAD)
	dsp::Matrix<float>* SPP;
    dsp::Matrix<float>* noisePower;    //why it is a gloabal vector is because we need to reuse the last frame's noisePower
	dsp::WindowingFunction<float>* anWin;
	dsp::FFT* myFFT;

};
