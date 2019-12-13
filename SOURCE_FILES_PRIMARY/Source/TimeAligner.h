#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include <vector>
#include <memory>
using namespace std;
enum State : int16_t { FillingBuffer, Working };

class TimeAligner
{

public:
	TimeAligner(int order);//2^order = length
	~TimeAligner();
	// alignWithReference will return a religned version of the second channel(to the reference)
	// downSample: 1: no downsample, 2: pick 1 out from 2 samples, 3: pick 1 out from 3 samples, etc..
	// skipFrame: true: only do 1 calculation every 10 frames; false: do calculation for every frame 
	// vector<float> alignWithReference(float* reference, float* secondChannel, int length, int downSample, bool skipFrame);
	// float calculateCrossCorrelation(float* reference, float* secondChannel, int length, int downSample, int* delayLength);
	vector<float> alignWithReferenceFast(float* reference, float* secondChannel, int length);
	int gccPHAT(vector<float> ref, vector<float> sec, float* maxC);
	vector<float> alignWithReferenceAndMix(float* reference, float* secondChannel);

private:
	float maxCorre = 0;
	int maxFrameIndex = 0;
	int maxLag = 0;

	int oneFrameLength;
	int numOfFrames;
	const int storeSize = 8192;

	State workingState;
	vector<vector<float>> referenceBuffer; // This buffer acts as a circular buffer that stores previous input signals
	vector<vector<float>> secondChannelBuffer;

	vector<float>* output;


	dsp::FFT* oneBlockFFT;
	dsp::FFT* bigFrameFFT;
	dsp::WindowingFunction<float>* oneBlockWin;
	dsp::WindowingFunction<float>* bigFrameWin;
};

