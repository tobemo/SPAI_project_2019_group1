/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "LockFreeQueue.h"
#include "Resampler.h"
#include "UDPThread.h"
#include "TimeAligner.h"
#include "WAVWriterReader.h"
#include "BoolHolder.h"
#include "MultiChannelFilter.h"
#include "globals.h"
#include "Wiener.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

//Struct which contains all information that will be recieved

class MainComponent   : public AudioAppComponent,
						public Button::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;
	void resizeButtons();
	virtual void buttonClicked(Button*) override;

private:
	//GLOBAL VARIABLES ::::
		int devicesConnected = 0;
		juce::Array<LockFreeQueue> arrayOfFIFOs;
		juce::Array<UDPThread*> arrayOfThreads;
		juce::Array<int> arrayOfSelectedSecondaryDevices;
		bool isFirstTime = true;
		bool doNoiseReduction = false;
		bool isRecording = false;
		BoolHolder* boolHolder;

	//SECONDARY DEVICES ::::	
		//fifos
		LockFreeQueue* firstIntermediateBuffer;
		LockFreeQueue* secondIntermediateBuffer;
		LockFreeQueue* thirdIntermediateBuffer;
		LockFreeQueue* fourthIntermediateBuffer;
		LockFreeQueue* fifthIntermediateBuffer;
		//udp threads
		UDPThread* firstUDPThread;
		UDPThread* secondUDPThread;
		UDPThread* thirdUDPThread;
		UDPThread* fourthUDPThread;
		UDPThread* fifthUDPThread;
		//all data
		std::vector<float> firstAllData;
		std::vector<float> secondAllData;
		std::vector<float> thirdAllData;
		std::vector<float> fourhtAllData;
		std::vector<float> fifthAllData;

	//GETNEXTAUDIOBLOCK ::::
		int numSelecetdSources;
		int selectedSource;
		LockFreeQueue* selectedFiFo;
		float sumBuffer[OUTPUT_BUFFER_SIZE] = { 0 };	//buffer used to sum different audio sources
		float referenceBuffer[OUTPUT_BUFFER_SIZE] = { 0 };
		float toAlignBuffer[OUTPUT_BUFFER_SIZE] = { 0 };

	//INTERFACE ::::
		int margins = 2;
		//left area
		TextEditor textfieldInputPort;
		TextButton addDevice;
		TextButton noiseReduction;
		TextButton selecAllButton;
		TextButton recordButton;
		TextButton saveFile;
		Slider cutoffFreqSlider;
		ComboBox selectDataComboBox;
	
		Rectangle<int> leftArea;
		int leftAreaWidth = 100;
		int textButtonHeight = 50;
		int widthCutoffFreqSlider = 30;
		int cutoffFreqSliderHeight = 200;
		//right area
		Label header;
		juce::Array<TextButton*> arrayOfButtonsForEachSender;
		Rectangle<int> rightArea;
		int sourceButtonHeight = 40;

	//SP ::::
		TimeAligner* timeAligner;
		MultiChannelFilter* MCF;
		Wiener* wienerFirst;
		Wiener* wienerSecond;
		Wiener* wienerMulti;

	//RECORDING ALL AUDIO
		WAVWriterReader writer;
		std::vector<float> allOutputDataPart2;
		std::vector<float> allOutputDataPart2e_1;
		std::vector<float> allOutputDataPart2e_2;
		std::vector<float> allOutputDataPart2e_mix;
		std::vector<float> allOutputDataPart3b_enh;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)

};


