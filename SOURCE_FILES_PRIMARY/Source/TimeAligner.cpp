#include "TimeAligner.h"
#include <cmath>

TimeAligner::TimeAligner(int order)
{
	this->oneFrameLength = pow(2, order);
	this->numOfFrames = storeSize / oneFrameLength;
	this->workingState = FillingBuffer;
	int FFTorder = log2(storeSize); // half of the store size plus zero padding
	this->oneBlockFFT = new dsp::FFT(order + 1); //frames + zeropadding: double the length
	this->bigFrameFFT = new dsp::FFT(FFTorder); //8192=> 4096*2
	this->oneBlockWin = new dsp::WindowingFunction<float>(oneFrameLength + 1, dsp::WindowingFunction<float>::hann, false, 0.0f);  //one frame
	this->bigFrameWin = new dsp::WindowingFunction<float>(storeSize / 2 + 1, dsp::WindowingFunction<float>::hann, false, 0.0f);  //four frames
	this->output = new vector<float>(oneFrameLength, 0);

}

TimeAligner::~TimeAligner()
{}

//
///*This function will align secondChannel to the reference*/
//vector<float> TimeAligner::alignWithReference(float* reference, float* secondChannel, int length, int downSample, bool skipFrame)
//{
//	// if enable skip frames, 1 calculation for every 10 frames 
//	if (skipFrame)
//	{
//		if (updateTime == 0)//reupdate the delayNum
//		{
//			float y2LagMaxCros = 0;
//			int y2LagIndex = 0;
//
//			float y2LeadMaxCros = 0;
//			int y2LeadIndex = 0;
//
//			/*step 1: calculate the crosscorrelation assume: secondChannel lags*/
//			y2LagMaxCros = calculateCrossCorrelation(reference, secondChannel, length, downSample, &y2LagIndex);
//
//			/*step 2: calculate the crosscorrelation assume: secondChannel leads*/
//			y2LeadMaxCros = calculateCrossCorrelation(secondChannel, reference, length, downSample, &y2LeadIndex);
//
//			/*step 3: compare lead or lag*/
//			if (y2LagMaxCros > y2LeadMaxCros)
//			{
//				// update memory for the next 10 frames
//				oldCrossValue = y2LagMaxCros;
//				delayNumber = y2LagIndex;
//				updateTime++;
//
//				vector<float> output;
//				//lag : -----********* => *********00000
//				output = vector<float>(secondChannel + y2LagIndex, secondChannel + length - 1);
//				output.resize(length, 0);
//				return output;
//			}
//			else
//			{
//				//update memory for the next 10 frames
//				oldCrossValue = y2LeadMaxCros;
//				delayNumber = -y2LeadIndex;
//				updateTime++;
//
//				//lead: ********---- => 0000*******
//				/*1. one vector all zeros*/
//				vector<float> zeroHead = vector<float>(y2LeadIndex, 0);
//				/*2. another vector before*/
//				vector <float> overlap = vector<float>(secondChannel, secondChannel + length - y2LeadIndex - 1);
//				/*concatenate zero vector and before*/
//				zeroHead.insert(zeroHead.end(), overlap.begin(), overlap.end());
//				return zeroHead;
//			}
//		}
//		else
//		{
//		//use the memory delayIndex as the current delay index so no calculation
//			if (delayNumber > 0)//y2 lags
//			{
//				updateTime++;
//				updateTime = updateTime % 10; //reinitialize to 0 after 10 trials
//				vector<float> output;
//				//lag : -----********* => *********00000
//				output = vector<float>(secondChannel + delayNumber, secondChannel + length - 1);
//				output.resize(length, 0);
//				return output;
//			}
//			else//y2 leads
//			{
//				updateTime++;
//				updateTime = updateTime % 10; //reinitialize to 0 after 10 trials
//				//lead: ********---- => 0000*******
//				/*1. one vector all zeros*/
//				vector<float> zeroHead = vector<float>(-delayNumber, 0);
//				/*2. another vector before*/
//				vector <float> overlap = vector<float>(secondChannel, secondChannel + length + delayNumber - 1);
//				/*concatenate zero vector and before*/
//				zeroHead.insert(zeroHead.end(), overlap.begin(), overlap.end());
//				return zeroHead;
//			}
//		}
//	}
//	else // not skip calculation but use memory to save half of calculations
//	{
//		if (updateTime == 0)// the first time: full calculations to get a correct memory
//		{
//			float y2LagMaxCros = 0;
//			int y2LagIndex = 0;
//
//			float y2LeadMaxCros = 0;
//			int y2LeadIndex = 0;
//
//			/*step 1: calculate the crosscorrelation assume: secondChannel lags*/
//			y2LagMaxCros = calculateCrossCorrelation(reference, secondChannel, length, downSample, &y2LagIndex);
//
//			/*step 2: calculate the crosscorrelation assume: secondChannel leads*/
//			y2LeadMaxCros = calculateCrossCorrelation(secondChannel, reference, length, downSample, &y2LeadIndex);
//
//			/*step 3: compare lead or lag*/
//			if (y2LagMaxCros > y2LeadMaxCros)
//			{
//				// update memory for the next 10 frames
//				oldCrossValue = y2LagMaxCros;
//				delayNumber = y2LagIndex;
//				updateTime++;
//
//				vector<float> output;
//				//lag : -----********* => *********00000
//				output = vector<float>(secondChannel + y2LagIndex, secondChannel + length - 1);
//				output.resize(length, 0);
//				return output;
//			}
//			else
//			{
//				//update memory for the next 10 frames
//				oldCrossValue = y2LeadMaxCros;
//				delayNumber = -y2LeadIndex;
//				updateTime++;
//
//				//lead: ********---- => 0000*******
//				/*1. one vector all zeros*/
//				vector<float> zeroHead = vector<float>(y2LeadIndex, 0);
//				/*2. another vector before*/
//				vector <float> overlap = vector<float>(secondChannel, secondChannel + length - y2LeadIndex - 1);
//				/*concatenate zero vector and before*/
//				zeroHead.insert(zeroHead.end(), overlap.begin(), overlap.end());
//				return zeroHead;
//			}
//		}
//		else // use the memory to calculate only once
//		{
//			if (delayNumber > 0)// y2 lags
//			{
//				/*calculate once*/
//				float crox = 0;
//				int y2_index = 0;
//				for (int index = delayNumber; index < length; index + downSample)
//				{
//					crox = crox + *(reference + index) * *(secondChannel + y2_index);
//					y2_index = y2_index + downSample;
//				}
//				crox = crox / (length / downSample);
//
//				if (crox > oldCrossValue) // keep using this delayNumber but update crossValue
//				{
//					oldCrossValue = crox;
//					vector<float> output;
//					//lag : -----********* => *********00000
//					output = vector<float>(secondChannel + delayNumber, secondChannel + length - 1);
//					output.resize(length, 0);
//					return output;
//				}
//				else if (crox >= oldCrossValue * alignThreshold) //keep using the delay number but not update the oldCrossValue
//				{
//					vector<float> output;
//					//lag : -----********* => *********00000
//					output = vector<float>(secondChannel + delayNumber, secondChannel + length - 1);
//					output.resize(length, 0);
//					return output;
//				}
//				else // this time keeps using delayTime, but updateTime = 0=> next time recalculate
//				{
//					updateTime = 0;
//					vector<float> output;
//					//lag : -----********* => *********00000
//					output = vector<float>(secondChannel + delayNumber, secondChannel + length - 1);
//					output.resize(length, 0);
//					return output;
//				}
//
//			}
//			else // y2 leads
//			{
//				/*calculate once*/
//				float crox = 0;
//				int y2_index = 0;
//				for (int index = -delayNumber; index < length; index + downSample)
//				{
//					crox = crox + *(secondChannel + index) * *(reference + y2_index);
//					y2_index = y2_index + downSample;
//				}
//				crox = crox / (length / downSample);
//				if (crox > oldCrossValue)// keep using this delayNumber but update crossValue
//				{
//					oldCrossValue = crox;
//					//lead: ********---- => 0000*******
//					/*1. one vector all zeros*/
//					vector<float> zeroHead = vector<float>(-delayNumber, 0);
//					/*2. another vector before*/
//					vector <float> overlap = vector<float>(secondChannel, secondChannel + length + delayNumber - 1);
//					/*concatenate zero vector and before*/
//					zeroHead.insert(zeroHead.end(), overlap.begin(), overlap.end());
//					return zeroHead;
//				}
//				else if (crox >= oldCrossValue * alignThreshold)// keep using the delayTime but not update oldCrossValue
//				{
//					//lead: ********---- => 0000*******
//					/*1. one vector all zeros*/
//					vector<float> zeroHead = vector<float>(-delayNumber, 0);
//					/*2. another vector before*/
//					vector <float> overlap = vector<float>(secondChannel, secondChannel + length + delayNumber - 1);
//					/*concatenate zero vector and before*/
//					zeroHead.insert(zeroHead.end(), overlap.begin(), overlap.end());
//					return zeroHead;
//				}
//				else// keep using the delayTime this time but set updateTime = 0, so next time recalculate
//				{
//					updateTime = 0;
//					//lead: ********---- => 0000*******
//					/*1. one vector all zeros*/
//					vector<float> zeroHead = vector<float>(-delayNumber, 0);
//					/*2. another vector before*/
//					vector <float> overlap = vector<float>(secondChannel, secondChannel + length + delayNumber - 1);
//					/*concatenate zero vector and before*/
//					zeroHead.insert(zeroHead.end(), overlap.begin(), overlap.end());
//					return zeroHead;
//				}
//			}
//		}
//	}
//}
//
//
//float TimeAligner::calculateCrossCorrelation(float* reference, float* secondChannel, int length, int downSample, int* delayLength)
//{
//	float y2LagMaxCros = 0;
//	int y2LagIndex = 0;
//
//	/*calculate the crosscorrelation using downsample assume: secondChannel lags*/
//	for (int i = 0; i < length; i + downSample)
//	{
//		float crox = 0;
//		int y2_index = 0;
//		for (int index = i; index < length; index + downSample)
//		{
//			crox = crox + *(reference + index) * *(secondChannel + y2_index);
//			y2_index = y2_index + downSample;
//		}
//		crox = crox / (length/downSample);
//		if (crox > y2LagMaxCros)
//		{
//			y2LagMaxCros = crox;
//			y2LagIndex = i;
//		}
//	}
//
//	*delayLength = y2LagIndex*downSample;
//	return y2LagMaxCros;
//}
vector<float> TimeAligner::alignWithReferenceAndMix(float* reference, float* secondChannel) {
	//step 1: fill in the buffer
	vector<float> referenceInput(reference, reference + oneFrameLength);
	vector<float> secondInput(secondChannel, secondChannel + oneFrameLength);

	switch (workingState)
	{
	case FillingBuffer:
	{
		/*step 1: fill in the buffer*/
		referenceBuffer.push_back(referenceInput);
		secondChannelBuffer.push_back(secondInput);


		/*check if the buffer is full*/
		if (referenceBuffer.size() == numOfFrames)
		{
			workingState = Working;
		}
		break;
	}
	case Working:
	{
		//do time alignment first
		//step 1: loop each frame and calculate gccphat, save the maxNum, maxIndex, maxLag
		vector<float> bigRef = referenceBuffer[0];
		vector<float> bigSec = secondChannelBuffer[0];
		vector<float> calculateRef = referenceBuffer[0];


		int tempLag;
		float tempMaxC;
		for (int i = 0; i < numOfFrames; i++)
		{
			// append vectors
			if (i > 0)
			{
				bigRef.insert(bigRef.end(), referenceBuffer[i].begin(), referenceBuffer[i].end());
				bigSec.insert(bigSec.end(), secondChannelBuffer[i].begin(), secondChannelBuffer[i].end());
			}

			vector<float> calculateSec = secondChannelBuffer[i];
			//step 1: GCC-PHAT
			tempLag = gccPHAT(calculateRef, calculateSec, &tempMaxC);
			if (tempMaxC > maxCorre)
			{
				maxFrameIndex = i;
				maxLag = tempLag;
				maxCorre = tempMaxC;
			}
		}


		//step 4: mix
		if (maxLag > 0) //reference lead =>start from the first frame of the reference
		{
			//reference: 23456789
			//second:    12345678 => 2345678
			int lag = maxFrameIndex * oneFrameLength + maxLag;
			if (lag + oneFrameLength < bigSec.size())
			{
				vector<float> second = vector<float>(bigSec.begin() + lag, bigSec.begin() + lag + oneFrameLength);
				//mix them together
				std::transform(referenceBuffer[0].begin(), referenceBuffer[0].end(), second.begin(),
					(*output).begin(), std::plus<float>());

			}
			else
			{
				vector<float> second = secondChannelBuffer[numOfFrames - 1];
				//mix them together
				std::transform(referenceBuffer[0].begin(), referenceBuffer[0].end(), second.begin(),
					(*output).begin(), std::plus<float>());

			}
		}
		else if (maxLag < 0)//reference lag =>start from the first frame of the second channel
		{
			int lag = maxFrameIndex * oneFrameLength + (-maxLag);
			// reference: 000123456789 => 123456789
			// second:    123456789 
			if (lag + oneFrameLength < bigRef.size())
			{
				vector<float> Refe = vector<float>(bigRef.begin() + lag, bigRef.begin() + lag + oneFrameLength);
				//mix them together
				std::transform(secondChannelBuffer[0].begin(), secondChannelBuffer[0].end(), Refe.begin(),
					(*output).begin(), std::plus<float>());

			}
			else
			{
				vector<float> Refe = referenceBuffer[numOfFrames - 1];
				//mix them together
				std::transform(secondChannelBuffer[0].begin(), secondChannelBuffer[0].end(), Refe.begin(),
					(*output).begin(), std::plus<float>());

			}
		}
		else// no alignment needed
		{
			//mix them together
			std::transform(secondChannelBuffer[0].begin(), secondChannelBuffer[0].end(), referenceBuffer[0].begin(),
				(*output).begin(), std::plus<float>());

		}

		// step 5: delete the oldest data and insert new data
		referenceBuffer.erase(referenceBuffer.begin());
		referenceBuffer.push_back(referenceInput);

		secondChannelBuffer.erase(secondChannelBuffer.begin());
		secondChannelBuffer.push_back(secondInput);

		break;
	}


	}

	return *output;
}



vector<float> TimeAligner::alignWithReferenceFast(float* reference, float* secondChannel, int length) {

	int newlength = 2 * length - 1;
	vector<float> signalRef(reference, reference + length);
	vector<float> signalSec(secondChannel, secondChannel + length);

	vector<float> outputRef = signalRef;
	vector<float> outputSec = signalSec;

	vector<complex<float>> vReference;
	vector<complex<float>> vSecond;
	vector<complex<float>> zeros(length, 0);
	vector<complex<float>> XResult(length * 2, 0);
	vector<complex<float>> tresult(length * 2, 0);
	vector<complex<float>> XReference(length * 2, 0);
	vector<complex<float>> XSecond(length * 2, 0);

	oneBlockWin->multiplyWithWindowingTable(signalRef.data(), length);
	oneBlockWin->multiplyWithWindowingTable(signalSec.data(), length);
	/*step 1: zero padding*/
	vReference.insert(vReference.end(), signalRef.begin(), signalRef.end());
	vReference.insert(vReference.end(), zeros.begin(), zeros.end());

	vSecond.insert(vSecond.end(), signalSec.begin(), signalSec.end());
	vSecond.insert(vSecond.end(), zeros.begin(), zeros.end());
	/*step 2: fft*/
	oneBlockFFT->perform(vReference.data(), XReference.data(), false);
	oneBlockFFT->perform(vSecond.data(), XSecond.data(), false);
	/*step 3: vSecond time complex conjugate of vReference*/
	for (int i = 0; i <= newlength; i++)
	{
		complex<float> temp = XSecond[i] * std::conj(XReference[i]);
		float normalize = abs(temp);
		if (normalize == 0)
		{
			XResult[i] = 0;
		}
		else
		{
			XResult[i] = temp / normalize;
		}
	}

	/*step 4: ifft to get cross correlation*/
	oneBlockFFT->perform(XResult.data(), tresult.data(), true);
	/*step 5: reconstruct the vector*/
	vector<complex<float>> first(tresult.begin() + length + 1, tresult.end());
	vector<complex<float>> second(tresult.begin(), tresult.begin() + length);
	first.insert(first.end(), second.begin(), second.end());
	/*step 6: find the max crox*/
	float max = 0;
	int lag = 0;
	int index = 0;
	for (auto value : first) {
		if (value.real() > max)
		{
			max = value.real();
			lag = index;
		}
		index++;
	}

	lag = lag - length + 1;

	if (lag > 0) //reference lead
	{
		//reference: 2345678
		//second:    1234567 => 234567+8
		vector<float> Head = vector<float>(outputSec.begin() + lag, outputSec.end());
		vector<float> End = vector<float>(outputRef.begin() + length - lag, outputRef.end());
		Head.insert(Head.end(), End.begin(), End.end());
		return Head;
	}
	else if (lag < 0)//reference lag
	{
		// reference: 000*********
		// second:    ************ => 000********
				/*1. one vector all zeros*/
		vector<float> Head = vector<float>(outputRef.begin(), outputRef.begin() + (-lag));
		/*2. another vector before*/
		vector <float> overlap = vector<float>(outputSec.begin(), outputSec.begin() + length - (-lag));
		/*concatenate zero vector and before*/
		Head.insert(Head.end(), overlap.begin(), overlap.end());
		return Head;
	}
	else
	{
		vector<float> signal(secondChannel, secondChannel + length - 1);
		return signal;
	}
}

int TimeAligner::gccPHAT(vector<float> ref, vector<float> sec, float* maxC)
{
	float* reference = ref.data();
	float* secondChannel = sec.data();

	int calculateLength = oneFrameLength;

	int newlength = 2 * calculateLength - 1;

	oneBlockWin->multiplyWithWindowingTable(reference, calculateLength);
	oneBlockWin->multiplyWithWindowingTable(secondChannel, calculateLength);

	vector<complex<float>> vReference(reference, reference + calculateLength);
	vector<complex<float>> vSecond(secondChannel, secondChannel + calculateLength);

	vector<complex<float>> zeros(calculateLength, 0);
	vector<complex<float>> XResult(calculateLength * 2, 0);
	vector<complex<float>> tresult(calculateLength * 2, 0);
	vector<complex<float>> XReference(calculateLength * 2, 0);
	vector<complex<float>> XSecond(calculateLength * 2, 0);

	/*step 1: zero padding*/
	vReference.insert(vReference.end(), zeros.begin(), zeros.end());
	vSecond.insert(vSecond.end(), zeros.begin(), zeros.end());

	/*step 2: fft*/
	oneBlockFFT->perform(vReference.data(), XReference.data(), false);
	oneBlockFFT->perform(vSecond.data(), XSecond.data(), false);
	/*step 3: vSecond time complex conjugate of vReference*/
	for (int i = 0; i <= newlength; i++)
	{
		complex<float> temp = XSecond[i] * std::conj(XReference[i]);
		float normalize = abs(temp);

		if (normalize == 0)
		{
			XResult[i] = 0;
		}
		else
		{
			XResult[i] = temp / normalize;
		}

	}

	/*step 4: ifft to get cross correlation*/
	oneBlockFFT->perform(XResult.data(), tresult.data(), true);
	/*step 5: reconstruct the vector*/
	vector<complex<float>> first(tresult.begin() + calculateLength + 1, tresult.end());
	vector<complex<float>> second(tresult.begin(), tresult.begin() + calculateLength);
	first.insert(first.end(), second.begin(), second.end());
	/*step 6: find the max crox*/
	float max = 0;
	int lag = 0;
	int index = 0;
	for (auto value : first) {
		if (value.real() > max)
		{
			max = value.real();
			lag = index;
		}
		index++;
	}
	*maxC = max;
	lag = lag - calculateLength + 1;
	return lag;
}