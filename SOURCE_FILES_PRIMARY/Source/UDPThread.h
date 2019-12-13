/*
  ==============================================================================

    UDPThread.h

	This class is a wrapper around some Juce:DatagramSocket and Juce:Threads functions.
	The Thread class handles all functionalities conerning creating new threads.
	The Datagram socket handles everything to do with the User Datagram Protocol.
	This class creates a new background thread which creates a socket that listens for UDP packets.
	The content of these packets is written to a lock free FIFO, see LockFreeQueue.h

	Additionally, all incoming data is appended to vector. This vector can be written to a WAV file in MainComponent.cpp

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "globals.h"
#include "LockFreeQueue.h"
#include "BoolHolder.h"

struct outBuffer {
	float buffer[INPUT_BUFFER_SIZE];
	int windowIndex;
	int windowEnd;
	int flag;
};

//==============================================================================
/*
*/
class UDPThread    :	public Thread
{
public:
	/*
		The constructor takes a pointer to a LockFreeQueue object.
		Packets read will be written to this FIFO.
	*/
	UDPThread(String threadName, int port, LockFreeQueue* FIFO, std::vector<float>* data, BoolHolder* newHolder):Thread(threadName, 0)
	{
		portNumber = port;
		thisFIFO = FIFO;
		thisData = data;
		thisHolder = newHolder;
		socket = new DatagramSocket(false);

		Thread::setPriority(realtimeAudioPriority);
		//startThread();
	}
	~UDPThread()
	{
		stopThread(2000);
	}

	/*
		Set the global variable portNumber. This method should be called before 'createSocket'.
	*/
	void setPort(int newPort)
	{
		portNumber = newPort;
	}

	/*
		Create a socket and bind it to the global variable portNumber.
	*/
	void createSocket()
	{
		successBind = socket->bindToPort(portNumber);
		if (successBind) { DBG("Socket created succesfully."); }
		else { DBG("Failed to bind socket."); }
	}

	String getSenderIP()
	{
		return senderIp;
	}

	LockFreeQueue* getFIFO()
	{
		return thisFIFO;
	}

	/*
		Starts when .startThread() is called. 
		This continously listens on a certain port for INPUT_BUFFER_SIZE number of floats.
		These get written to the FIFO of this object.
	*/
	void run() override
	{
		while (threadShouldExit() == false) {
			//FloatVectorOperations::clear(inputBuffer, INPUT_BUFFER_SIZE);
			successRead = socket->read(inputBuffer, INPUT_BUFFER_SIZE * sizeof(float), true, senderIp, senderPortNumber);	//FALSE OR TRUE or sync to audiocallback??
			if (successRead == -1) { DBG("Failed to read socket."); }	//error

			thisFIFO->writeToFIFO(inputBuffer, INPUT_BUFFER_SIZE);
			if ((*thisHolder).GetState() == true)
			{
				thisData->insert(std::end(*thisData), std::begin(inputBuffer), std::end(inputBuffer));	//store current time frame
			}
		}
	}

private:
	//UDP
	int senderPortNumber;
	float inputBuffer[INPUT_BUFFER_SIZE] = { 0 };
	int successRead;
	bool successBind;
	String senderIp;
	DatagramSocket* socket;	
	int portNumber;

	//FIFO
	LockFreeQueue* thisFIFO;

	//All data
	std::vector<float>* thisData;
	BoolHolder* thisHolder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UDPThread)
};
