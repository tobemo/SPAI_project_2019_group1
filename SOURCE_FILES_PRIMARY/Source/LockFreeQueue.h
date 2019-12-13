/*
  ==============================================================================

    LockFreeQueue.h

	This class is a wrapper around some Juce:AbstractFifo functions.
	The AbstractFifo class handles the read and write pointer of a lock free circular buffer.
	You use your own array and tell it how much you want to read or write and it tells you where to start reading and or writing.
	It also ensures that the read pointer doesn't overtake the write pointer and vice versa.

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "globals.h"

class LockFreeQueue
{

public:
	/*
		The cosntructor creates a new circular buffer.
		It ensures that the data array has the space it needs and initializes it to 0.
	*/
	LockFreeQueue(String name)
	{
		nameOfFIFO = name;
		lockFreeFifo = new AbstractFifo(FIFOBUFFER_SIZE);

		//clear
		FloatVectorOperations::clear(data, FIFOBUFFER_SIZE);
	}

	/*
		Wrapper around the AbstractFIFO functions 'prepareToWirte' and 'finishedWrite'.
		This function writes data from a given buffer to the circular buffer of this object.
	*/
	void writeToFIFO(const float* sourceBuffer, int numToWrite)
	{
		//ask where to start writing 'numToWrite' samples
		lockFreeFifo->prepareToWrite(numToWrite, writeStart1, writeBlockSize1, writeStart2, writeBlockSize2);

		if ( writeBlockSize1 > 0 ) 
		{
			FloatVectorOperations::copy(data + writeStart1, sourceBuffer, writeBlockSize1); 
		}	//copy source data to local data array
		if ( writeBlockSize2 > 0 ) 
		{ 
			FloatVectorOperations::copy(data + writeStart2, sourceBuffer + writeBlockSize1, writeBlockSize2); 
		}	//copy source data to local data array

		//move write pointer
		lockFreeFifo->finishedWrite(writeBlockSize1 + writeBlockSize2);
	}

	/*
		Wrapper around the AbstractFIFO functions 'prepareToRead' and 'finishedRead'.
		This function reads data from the circular buffer of this object to a given buffer.
	*/
	void readFromFIFO(float* destinationBuffer, int numToRead)
	{
		//ask where to start reading 'numToRead' samples
		lockFreeFifo->prepareToRead(numToRead, readStart1, readBlockSize1, readStart2, readBlockSize2);

		if ( readBlockSize1 > 0 ) 
		{ 
			FloatVectorOperations::copy(destinationBuffer, data + readStart1, readBlockSize1); 
		}
		if ( readBlockSize2 > 0 ) 
		{ 
			FloatVectorOperations::copy(destinationBuffer + readBlockSize1, data + readStart2, readBlockSize2); 
		}

		//move read pointer
		lockFreeFifo->finishedRead(readBlockSize1 + readBlockSize2);
	}
	void readFromFIFO(std::vector<float>* destinationVector, int numToRead)
	{
		lockFreeFifo->prepareToRead(numToRead, readStart1, readBlockSize1, readStart2, readBlockSize2);

		if (readBlockSize1 > 0)
		{
			FloatVectorOperations::copy(destinationVector->data(), data + readStart1, readBlockSize1);
		}
		if (readBlockSize2 > 0)
		{
			FloatVectorOperations::copy(destinationVector->data() + readBlockSize1, data + readStart2, readBlockSize2);
		}

		lockFreeFifo->finishedRead(readBlockSize1 + readBlockSize2);
	}

	/*
		Move the read pointer 'numToRead times without reading anything.
	*/
	void fakeReadFromFIFO(int numToRead)
	{
		lockFreeFifo->prepareToRead(numToRead, fakeReadStart1, fakeReadBlockSize1, fakeReadStart2, fakeReadBlockSize2);
		//don't read aything
		lockFreeFifo->finishedRead(fakeReadBlockSize1 + fakeReadBlockSize2);
	}

	/*
		Move the write pointer 'numToWrite times without writing anything.
	*/
	void fakeWriteToFIFO(int numToWrite)
	{
		lockFreeFifo->prepareToWrite(numToWrite, fakeWriteStart1, fakeWriteStart2, fakeWriteBlockSize1, fakeWriteBlockSize2);
		//don't write anything
		lockFreeFifo->finishedWrite(fakeWriteBlockSize1 + fakeWriteBlockSize2);
	}

	/*
		Copy the content of the whole FIFO which is the data array.
	*/
	void copyData(float* destinationBuffer)
	{
		FloatVectorOperations::copy(destinationBuffer, data, FIFOBUFFER_SIZE);
	}

	/*
		Returns the data array.
	*/
	float* getData()
	{
		return data;
	}

	/*
		Returns the index right after the last read. 
		Combined with a pointer to the data array this can be used to index the samples that are going to be read next time 'readFromFIFO' is called.
	*/
	int getReadPointer(int numToRead)
	{
		lockFreeFifo->prepareToRead(numToRead, readPointerStart1, readPointerStart2, readPointerNlockSize1, readPointerNlockSize2);
		if (readPointerNlockSize1 > 0)
		{
			readPointer = readPointerStart1;
		}
		if (readPointerNlockSize2 > 0)
		{
			readPointer = readPointerStart2;
		}
		return readPointer;
	}

	bool isFull()
	{
		if (lockFreeFifo->getFreeSpace() == 0)
		{
			return true;
		}

		return false;
	}

	String setName(String newName)
	{
		nameOfFIFO = newName;
	}

	String getName()
	{
		return nameOfFIFO;
	}

	private:
		ScopedPointer<AbstractFifo> lockFreeFifo;		//the pointer to this circular buffer
		float data[FIFOBUFFER_SIZE] = {0};				//the actual data of this circular buffer
		int readPointer = 0;	//NOT threadsafe
		String nameOfFIFO = "";

		int writeStart1, writeStart2, writeBlockSize1, writeBlockSize2;								//for function writeToFIFO
		int readStart1, readStart2, readBlockSize1, readBlockSize2;									//for function readFromFIFO
		int fakeReadStart1, fakeReadStart2, fakeReadBlockSize1, fakeReadBlockSize2;					//for function fakeReadFromFIFO
		int fakeWriteStart1, fakeWriteStart2, fakeWriteBlockSize1, fakeWriteBlockSize2;				//for function fakeWriteToFIFO
		int readPointerStart1, readPointerStart2, readPointerNlockSize1, readPointerNlockSize2;		//for function getReadPointer
};
