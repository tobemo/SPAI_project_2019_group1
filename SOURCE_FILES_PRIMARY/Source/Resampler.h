/*
  ==============================================================================

    Resampler.h
    
	This class can upsample and down sample a given buffer into another provided buffer.
	Upsampling is done by copying samples a number of times.
	Downsampling is done by discarding samples at certain intervals.
  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "globals.h"
class Resampler
{
public:
	Resampler( )
	{

	}

	static void upsample(float* inputBuffer, int inputBufferSize, float* outputBuffer, int outputBufferSize)
	{
		jassert(outputBufferSize > inputBufferSize);
		jassert( (outputBufferSize % inputBufferSize) == 0 );
		int ratio = outputBufferSize / inputBufferSize;
		for (int i = 0; i < inputBufferSize; i++)
		{
			for (int j = 0; j < ratio; j++)
			{
				outputBuffer[i*ratio + j] = inputBuffer[i];
			}
		}
	}

	static void downSample(float* inputBuffer, int inputBufferSize, float* outputBuffer, int outputBufferSize)
	{
		jassert(inputBufferSize > outputBufferSize);
		jassert( (inputBufferSize % outputBufferSize) == 0 );
		int ratio = inputBufferSize / outputBufferSize;
		for (int i = 0, j = 0; i < outputBufferSize; i++, j+=ratio)
		{
			outputBuffer[i] = inputBuffer[j];
		}
	}
private:
};
