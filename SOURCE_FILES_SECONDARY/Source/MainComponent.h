/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/
#define BUFFERSIZE 512

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent
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
    void writeWAV(std::vector<float> samples, File file);
    

private:
    Label titleLabel;
    Label ipInputLabel;
    Label ipLabel;
    Label portInputLabel;
    Label portLabel;
    Label resampleInputLabel;
    Label resampleLabel;
    Label filePathInputLabel;
    Label filePathLabel;
    TextButton startRecordingButton {"Start Recording"};
    TextButton stopRecordingButton {"Stop Recording"};
    String ipAddr = "";
    String pathName = "";
    int portNumber = 0;
    int resampleIndex = 0;
    bool startRecording = false;
    DatagramSocket writeSocket {false};
    File parentDir;
    File lastRecording;
    std::vector<float> toFileBuffer;
    //==============================================================================
    // Your private member variables go here...


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
