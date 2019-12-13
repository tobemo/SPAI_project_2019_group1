/*
  ==============================================================================

    VAD.cpp
    Created: 21 Nov 2019 5:24:13pm
    Author:  Liuyin Yang

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "VAD.h"
#include <cmath>
#include <algorithm>
//==============================================================================
VAD::VAD(int signalLength, int fLength, int shiftLength, int order)
{
	/*initialize basic parameters*/
	this->frLeng = fLength;                           // frame length: recommended 128
	this->fshift = shiftLength;                       // shift length: recommended 64
	this->nFrames = floor(signalLength / fshift) - 1; // number of frames
	this->nFrequency = frLeng / 2 + 1;
	/*construct matrix and windowing function*/
	this->anWin = new dsp::WindowingFunction<float>(frLeng + 1, dsp::WindowingFunction<float>::hann, false, 0.0f);  //hanning window
	this->SPP = new dsp::Matrix<float>(nFrequency, nFrames);               // matrix of the speech presence probabilities for all frequencies at each time frame.
	this->noisePower = new dsp::Matrix<float>(nFrequency, nFrames);        // matrix with estimated noise power for each frame for all frequencies.
	this->myFFT = new dsp::FFT(order);                                     // initialize fft, order must be 2 times the fLength in order to perform the RealOnlyInverseTransform()
	/*for online memory*/
	this->noisePow = new vector<float>;
	this->PH1mean = new vector<float>(nFrequency, 0.5); // initialize this vector length = output of the fft length, value = 0.5
}

VAD::~VAD()
{
}

void VAD::init_noise_tracker_ideal_vad(vector<float>* x, int numberOfFrames)
{
	/*function   noise_psd_init =init_noise_tracker_ideal_vad(noisy,fr_size,fft_size,hop,sq_hann_window)
	init_noise_tracker_ideal_vad(noisy,frLen,frLen,fShift, anWin)
 noisy:          noisy signal fr_size:        frame size fft_size:       fft size hop:            hop size of frame
sq_hann_window: analysis window Output parameters:  noise_psd_init: initial noise PSD estimate
 
for I=1:5
    noisy_frame=sq_hann_window.*noisy((I-1)*hop+1:(I-1)*hop+fr_size);
    noisy_dft_frame_matrix(:,I)=fft(noisy_frame,fft_size);
end
noise_psd_init=mean(abs(noisy_dft_frame_matrix(1:fr_size/2+1,1:end)).^2,2);%%%compute the initialisation of the noise tracking algorithms.
return*/
	
	vector<float> result;
	for (int i = 1; i <= numberOfFrames; i++)
	{
		/*step 1: get the corresponding frame*/
		unique_ptr<vector<float>> temp;
		temp = std::make_unique<vector<float>>(vector<float>(x->begin() + (i-1)*fshift, x->begin() + (i-1)*fshift+frLeng));
		temp->resize(2 * (temp->size()), 0);

		/*step 3: apply the windowing function to this frame*/
		anWin->multiplyWithWindowingTable(temp->data(), frLeng);
		
		/*step 4: apply fft to the frame*/
		int testOnly = myFFT->getSize();
		myFFT->performFrequencyOnlyForwardTransform(temp->data()); //the size of input must match the order of this fft=> check frLeng!!
		
		/*step 5: get the first half length +1 and store in the result vector*/
		result.insert(end(result), temp->begin(), temp->begin() + nFrequency); 
	
	}
	/*step 6: store in the Matrix*/
	dsp::Matrix<float> noisy_dft = dsp::Matrix<float>(numberOfFrames, nFrequency, result.data()); //each row stores a fft result array

	/*Calculate the mean value for each frequency component*/
	for (int fbin = 0; fbin < frLeng/2+1; fbin++)
	{
		float sum = 0;
		for (int frame = 0; frame < numberOfFrames; frame++)
		{
			sum = sum + noisy_dft(frame, fbin)* noisy_dft(frame, fbin);
		}
		(*noisePow).push_back(sum / numberOfFrames);
	}

}
void VAD:: calc_SPP(vector<float>* x) {
	/*%
PH1mean  = 0.5;
alphaPH1mean = 0.9;
alphaPSD = 0.8;

%constants for a posteriori SPP
q          = 0.5; % a priori probability of speech presence:
priorFact  = q./(1-q);
xiOptDb    = 15; % optimal fixed a priori SNR for SPP estimation
xiOpt      = 10.^(xiOptDb./10);
logGLRFact = log(1./(1+xiOpt));
GLRexp     = xiOpt./(1+xiOpt);

for indFr = 1:nFrames  % All frequencies are kept in a vector
    indices       = (indFr-1)*fShift+1:(indFr-1)*fShift+frLen;
    noisy_frame   = anWin.*noisy(indices);
    noisyDftFrame = fft(noisy_frame,frLen);
    noisyDftFrame = noisyDftFrame(1:frLen/2+1);
	
    noisyPer = noisyDftFrame.*conj(noisyDftFrame);
    snrPost1 =  noisyPer./(noisePow);% a posteriori SNR based on old noise power estimate


    %% noise power estimation
	GLR     = priorFact .* exp(min(logGLRFact + GLRexp.*snrPost1,200));
	PH1     = GLR./(1+GLR); % a posteriori speech presence probability
	PH1mean  = alphaPH1mean * PH1mean + (1-alphaPH1mean) * PH1;
	stuckInd = PH1mean > 0.99;
	PH1(stuckInd) = min(PH1(stuckInd),0.99);
	estimate =  PH1 .* noisePow + (1-PH1) .* noisyPer ;
	noisePow = alphaPSD *noisePow+(1-alphaPSD)*estimate;
    
    SPP(:,indFr) = PH1;    
	noisePowMat(:,indFr) = noisePow;
end
return*/

	/*for loop=>each frame*/
	for (int i = 0; i < nFrames; i++)
	{
		/*if (i >= 395)
		{
			int t = 1;
		}*/
		/*step1: get the correct index*/
		int indexStart = i*fshift;
		int indexEnd = i*fshift + frLeng;
		/*step2: get the noisy frame*/
		unique_ptr<vector<float>> noisyFrame;
		noisyFrame = std::make_unique<vector<float>>(vector<float>(x->begin() + indexStart, x->begin() + indexEnd));
		noisyFrame->resize(2 * (noisyFrame->size()), 0);
		/*step 3: apply the windowing function to this frame*/
		anWin->multiplyWithWindowingTable(noisyFrame->data(), frLeng);
		/*step 4: get the power spectrum by doing fft*/
		myFFT->performFrequencyOnlyForwardTransform(noisyFrame->data()); //the size of input must match the order of this fft=> check frLeng!!
		std::transform(noisyFrame->begin(), noisyFrame->begin() + nFrequency, noisyFrame->begin(), [](float f)->float { return f * f; });
		/*step 5: get the first half length +1 */
		unique_ptr<vector<float>> result;
		result = std::make_unique<vector<float>>(vector<float>(noisyFrame->begin(), noisyFrame->begin() + nFrequency));

		/*step 7: noise power estimation*/
		/*GLR = priorFact .* exp(min(logGLRFact + GLRexp.*snrPost1,200));*/
		/*PH1 = GLR./(1+GLR); % a posteriori speech presence probability*/
		/*PH1mean  = alphaPH1mean * PH1mean + (1-alphaPH1mean) * PH1;*/

		for (int j = 0; j < nFrequency; j++)
		{
			/*step 6: get a posteriori SNR based on old noise power estimate*/
			float snrPost1 = (*result)[j] / (*noisePow)[j];
			float GLRResult = min(priorFact*exp(min(logGLRFact + GLRexp*snrPost1, static_cast<float>(200))),1000000.0f);//avoid overflow
			
			float PH1Result = GLRResult / (1 + GLRResult);
			(*PH1mean)[j] = alphaPH1mean * (*PH1mean)[j] + (1 - alphaPH1mean) * PH1Result;
			/*stuckInd = PH1mean > 0.99;
			PH1(stuckInd) = min(PH1(stuckInd),0.99);*/
			if ((*PH1mean)[j] > 0.99)
			{
				PH1Result = min(PH1Result,static_cast<float>(0.99));
			}
			
			/*estimate =  PH1 .* noisePow + (1-PH1) .* noisyPer ;
			noisePow = alphaPSD *noisePow+(1-alphaPSD)*estimate;*/
			float estimate = PH1Result * (*noisePow)[j] + (1 - PH1Result) * (*result)[j];
			(*noisePow)[j] = alphaPSD * (*noisePow)[j] + (1 - alphaPSD) * estimate;

			/* SPP(:,indFr) = PH1;    
			noisePowMat(:,indFr) = noisePow;*/
			(*SPP)(j, i) = PH1Result;
			(*noisePower)(j, i) = (*noisePow)[j];
		}

	}
}

dsp::Matrix<float>* VAD::getSPP() {
	return SPP;
}

dsp::Matrix<float>* VAD::getnoisePower() {
	return noisePower;
}

int VAD::getnFrames()
{
	return nFrames;
}

int VAD::getnFbins()
{
	return nFrequency;
}

