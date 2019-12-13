/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    addAndMakeVisible(titleLabel);
    titleLabel.setText("ConnexSound", dontSendNotification);
    titleLabel.setFont (Font (28.0f, Font::bold));
    
    addAndMakeVisible(ipInputLabel);
    addAndMakeVisible(ipLabel);
    ipLabel.setText("IP Address", dontSendNotification);
    ipLabel.setFont (Font (12.0f, Font::bold));
    ipInputLabel.setEditable(true);
    ipInputLabel.setColour (Label::backgroundColourId, Colours::lightgrey);
    ipInputLabel.onTextChange = [this] {
        ipAddr = ipInputLabel.getText();
    };
    
    addAndMakeVisible(portInputLabel);
    addAndMakeVisible(portLabel);
    portLabel.setText("Port Number", dontSendNotification);
    portLabel.setFont (Font (12.0f, Font::bold));
    portInputLabel.setEditable(true);
    portInputLabel.setColour (Label::backgroundColourId, Colours::lightgrey);
    portInputLabel.onTextChange = [this] {
        std::string text = (portInputLabel.getText()).toStdString();
        std::istringstream iss (text);
        iss >> portNumber;
    };
    
    addAndMakeVisible(resampleInputLabel);
    addAndMakeVisible(resampleLabel);
    resampleLabel.setText("Resample Index", dontSendNotification);
    resampleLabel.setFont (Font (12.0f, Font::bold));
    resampleInputLabel.setEditable(true);
    resampleInputLabel.setColour (Label::backgroundColourId, Colours::lightgrey);
    resampleInputLabel.onTextChange = [this] {
        
    };
    
    addAndMakeVisible(filePathInputLabel);
    addAndMakeVisible(filePathLabel);
    filePathLabel.setText("File Path", dontSendNotification);
    filePathLabel.setFont (Font (12.0f, Font::bold));
    filePathInputLabel.setEditable(true);
    filePathInputLabel.setColour (Label::backgroundColourId, Colours::lightgrey);
    filePathInputLabel.onTextChange = [this] {
        pathName = filePathInputLabel.getText();
    };
    
    addAndMakeVisible(startRecordingButton);
    startRecordingButton.onClick = [this] {
        if(ipAddr == ""){
            String error = "Invallid Ip Address";
            NativeMessageBox::showMessageBoxAsync (AlertWindow::WarningIcon, "Input error",
            error);
        }
        else if(portNumber <= 0){
            String error = "Invallid Port Number";
            NativeMessageBox::showMessageBoxAsync (AlertWindow::WarningIcon, "Input error",
            error);
        }
        else if(pathName == ""){
            String error = "Invallid Path Name";
            NativeMessageBox::showMessageBoxAsync (AlertWindow::WarningIcon, "Input error",
            error);
        }
        else {
            writeSocket.bindToPort(portNumber);
            #if (JUCE_ANDROID || JUCE_IOS)
             auto parentDir = File::getSpecialLocation (File::tempDirectory);
            #else
             auto parentDir = File::getSpecialLocation (File::userDocumentsDirectory);
            #endif
            lastRecording = parentDir.getNonexistentChildFile (pathName, ".wav");
            
            startRecording = true;
        }
        
    };
    addAndMakeVisible(stopRecordingButton);
    stopRecordingButton.onClick = [this] {
        startRecording = false;
        writeWAV(toFileBuffer, lastRecording);
        toFileBuffer.clear();
    };
    
    
    
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
        && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request (RuntimePermissions::recordAudio,
                                     [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    
    AudioDeviceManager::AudioDeviceSetup deviceSetup = AudioDeviceManager::AudioDeviceSetup();
    deviceSetup.bufferSize = BUFFERSIZE;
    deviceManager.initialise (2, 2, 0, true, "" , &deviceSetup);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    if(startRecording){
        auto * inBuff = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
        
        for(int i=0; i<bufferToFill.numSamples; i++){
            toFileBuffer.push_back(inBuff[i]);
        }
        
        writeSocket.write(ipAddr, portNumber, inBuff, sizeof(float)*bufferToFill.numSamples);
    }
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    int x = 10;
    int y = 10;
    int buttonWidth = 140;
    int buttonHeight = 60;
    int inputHeight = 50;
    titleLabel.setBounds(x, y, 200, 40);
    ipInputLabel.setBounds(x+100, getHeight()/2-45-inputHeight, 200, 30);
    portInputLabel.setBounds(x+100,getHeight()/2-inputHeight,200,30);
    resampleInputLabel.setBounds(x+100, getHeight()/2+45-inputHeight, 200, 30);
    filePathInputLabel.setBounds(x, getHeight()/2+100, getWidth()-2*x, 30);
    startRecordingButton.setBounds(x, getHeight()-y*8, buttonWidth, buttonHeight);
    stopRecordingButton.setBounds(getWidth()-buttonWidth-x, getHeight()-y*8, buttonWidth, buttonHeight);
    ipLabel.setBounds(x, getHeight()/2-45+4-inputHeight, 100, 20);
    portLabel.setBounds(x, getHeight()/2+4-inputHeight, 100, 20);
    resampleLabel.setBounds(x, getHeight()/2+45+4-inputHeight, 100, 20);
    filePathLabel.setBounds(x, getHeight()/2+75, getWidth()-2*x, 20);
    
}

void MainComponent::writeWAV(std::vector<float> samples, File file) {
    float *h1 = samples.data();
    float** h2 = &h1;
    AudioBuffer<float> buffer(1, samples.size());
    buffer.setDataToReferTo(h2, 1, samples.size());
    File tempFile = (file);
    tempFile.create();
    const StringPairArray& metadataValues("");
    AudioBuffer<float> fileBuffer;
    WavAudioFormat wav;
    ScopedPointer <OutputStream> outStream(tempFile.createOutputStream());
    if (outStream != nullptr){
        ScopedPointer <AudioFormatWriter> writer(wav.createWriterFor(outStream, 44100, 1, (int)32, metadataValues, 0));

        if (writer != nullptr){
            outStream.release();
            
            
            writer->writeFromAudioSampleBuffer(buffer, 0, samples.size());
        }
        writer = nullptr;
    }
    
    
    #if (JUCE_ANDROID || JUCE_IOS)
    SafePointer<MainComponent> safeThis (this);
          File fileToShare = lastRecording;

          ContentSharer::getInstance()->shareFiles (Array<URL> ({URL (fileToShare)}),
                                                    [safeThis, fileToShare] (bool success, const String& error)
                                                    {
                                                        if (fileToShare.existsAsFile())
                                                            fileToShare.deleteFile();

                                                        if (! success && error.isNotEmpty())
                                                        {
                                                            NativeMessageBox::showMessageBoxAsync (AlertWindow::WarningIcon,
                                                                                                   "Sharing Error",
                                                                                                   error);
                                                            std::cout << "Pathname   :"  << fileToShare.getFullPathName() << std::endl;
                                                        }
                                                    });
    #endif
}
