/*
  ==============================================================================

    MainComponent.cpp
	Team 1

	This function is the main body of our program.
	It consists of an audio thread regulary reading presenting a buffer which we fill with data from one our more circular buffers.
	The data in this bufferToFill is then send to the speaker.
	There are 5 pre-initalised threads, each reading a unique UDP port.
		These threads write received data to it's own circular buffer/fifo.
	The UI thread listens for button clicks to decide which circualr buffer should be used by the audio thread.

	- globals.h						contains some interesting settings but be warned that little to no error checking is done so the program may crash if these values are changed.
	- LockFreeQueue.h				is a class that manages circular buffers.
	- MultiChannelFilter.cpp/h		does the singal processing.
	- Resampler.h					is a class that manages upsanpling and down sampling.
	- TimeAligner.cpp/h				tries to align two vectors.
	- UDPThread.h					is a class that listens for UDP datagrams and writes it content to a fifo, see LockFreeQueu.h
	- VAD.cpp/h						does voice acitivation detection.
	- WAVWriterReader.h				is class that writes to and reads from WAV files.
	- Wiener.cpp/h					is not implemented at the time of this writing
	- Eigen							is a C++ template library for linear algebra taken from https://gitlab.com/libeigen/eigen

	A note on the saving file functionality:
		Saving multiple audio files was required for this project.
		On of the use cases is to take the incoming audio streams, save them to a wav file and import these in Matlab for multi-channel noise reduction.
		To make the Matlab part easier a global 'start recording' function is implemented to have some synchronization between all these audio files.
		Otherwise there would be 2 problems:
			- When starting the program no audio is received so the wave files would start of as DC
			- When adding multiple secondary devices in a row, 
				the time it takes to add a new device is time that needs to be removed in Matlab as to have arrays of the same size whit somewhat aligned audio.

	note: FIFO, lock free queue and circular buffer are used interchangebally

  ==============================================================================
*/

#include "MainComponent.h"


//==============================================================================
MainComponent::MainComponent()
{
	//INIT INTERFACE ::::
	//init left area of interface ::::
	//Settings for hte port textfield
	addAndMakeVisible(textfieldInputPort);
	textfieldInputPort.setTextToShowWhenEmpty("NEW PORT", Colours::black);
	textfieldInputPort.setColour(TextEditor::backgroundColourId, Colours::whitesmoke);
	textfieldInputPort.applyColourToAllText(Colours::black);
	textfieldInputPort.setInputRestrictions(4, "0123456789");	//only allow 4 characters, only allow numbers as characters
	Justification* newJust = new Justification(Justification::centred);
	textfieldInputPort.setJustification(*newJust);

	//Settings for the add new device button
	addAndMakeVisible(addDevice);
	addDevice.setButtonText("+");
	addDevice.setColour(TextButton::buttonColourId, Colours::blue);
	addDevice.addListener(this);

	//Settings for the noise reduction button
	addAndMakeVisible(noiseReduction);
	noiseReduction.setButtonText("NR");
	noiseReduction.setClickingTogglesState(true);
	noiseReduction.setColour(TextButton::textColourOffId, Colours::grey);
	noiseReduction.setColour(TextButton::textColourOnId, Colours::white);
	noiseReduction.addListener(this);

	//Settings for the select all devices button
	addAndMakeVisible(selecAllButton);
	selecAllButton.setButtonText("all");
	selecAllButton.setClickingTogglesState(true);
	selecAllButton.setColour(TextButton::textColourOffId, Colours::grey);
	selecAllButton.setColour(TextButton::textColourOnId, Colours::white);
	selecAllButton.addListener(this);

	//Settings for the record button
	addAndMakeVisible(recordButton);
	recordButton.setButtonText("REC");
	recordButton.setClickingTogglesState(true);
	recordButton.setColour(TextButton::textColourOffId, Colours::grey);
	recordButton.setColour(TextButton::textColourOnId, Colours::red);
	recordButton.addListener(this);

	//Settings for frequency slider	-	deprecated
	cutoffFreqSlider.setRange(1, 10000);
	cutoffFreqSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
	cutoffFreqSlider.setValue(100);
	//addAndMakeVisible(cutoffFreqSlider);

	//Settings for the save file button
	addAndMakeVisible(saveFile);
	saveFile.setButtonText("save");
	saveFile.setColour(TextButton::buttonColourId, Colours::green);
	saveFile.setColour(TextButton::textColourOffId, Colours::white);
	saveFile.setColour(TextButton::textColourOnId, Colours::white);
	saveFile.addListener(this);

	//Settings for the ComboBox/dropdown menu
	addAndMakeVisible(selectDataComboBox);
	selectDataComboBox.setText("CHOOSE OUTPUT");
	selectDataComboBox.setEditableText(false);
	selectDataComboBox.addItem("1st sec device", 1);
	selectDataComboBox.addItem("2nd sec device", 2);
	selectDataComboBox.addItem("3rd sec device", 3);
	selectDataComboBox.addItem("4th sec device", 4);
	selectDataComboBox.addItem("5th sec device", 5);
	selectDataComboBox.addItem("part2_pd", 6);
	selectDataComboBox.addItem("part2e_pd1", 7);
	selectDataComboBox.addItem("part2e_pd2", 8);
	selectDataComboBox.addItem("part2e_pdmix", 9);
	selectDataComboBox.addItem("part3b_pd_enh", 10);
	//TODO: add more

	//init right area of interface ::::
	//Settings for the header
	header.setColour(Label::backgroundColourId, Colour(0xff00407A));	//KU Leuven dark blue
	header.setText("Secondary Devices", NotificationType::dontSendNotification);
	juce::Array<IPAddress> addresses;
	IPAddress::findAllAddresses(addresses);	//try to find this devices IPv4 address
	#if JUCE_WINDOWS
		header.setText(addresses.getLast().toString(), NotificationType::dontSendNotification);
	#else
		header.setText(addresses.getFirst().toString(), NotificationType::dontSendNotification);
	#endif
	header.setColour(Label::textColourId, Colours::white);
	header.setJustificationType(Justification::centred);
	addAndMakeVisible(header);

	//SET UI WINDOW SIZE ::::
	setSize(800, 600);

	//PRE INITLIALISE TO GAIN PERFORMANCE ::::
	//PRE INIT intermediate buffers ::::
	firstIntermediateBuffer = new LockFreeQueue("first");	//a circular buffer with a read and a write pointer
	secondIntermediateBuffer = new LockFreeQueue("second");
	thirdIntermediateBuffer = new LockFreeQueue("third");
	fourthIntermediateBuffer = new LockFreeQueue("fourth");
	fifthIntermediateBuffer = new LockFreeQueue("fifth");

	//PRE INIT UDP threads ::::
	boolHolder = new BoolHolder(&isRecording);
	firstUDPThread = new UDPThread("first", 6665, firstIntermediateBuffer, &firstAllData, boolHolder);	//a thread with a UDP socket and a fifo
	secondUDPThread = new UDPThread("second", 6666, secondIntermediateBuffer, &secondAllData, boolHolder);
	thirdUDPThread = new UDPThread("third", 6667, thirdIntermediateBuffer, &thirdAllData, boolHolder);
	fourthUDPThread = new UDPThread("fourth", 6668, fourthIntermediateBuffer, &fourhtAllData, boolHolder);
	fifthUDPThread = new UDPThread("fifth", 6669, fifthIntermediateBuffer, &fifthAllData, boolHolder);
	arrayOfThreads.add(firstUDPThread);
	arrayOfThreads.add(secondUDPThread);
	arrayOfThreads.add(thirdUDPThread);
	arrayOfThreads.add(fourthUDPThread);
	arrayOfThreads.add(fifthUDPThread);
	

	//FOR FILTERING	::::
	timeAligner = new TimeAligner(9);	//TODO: make global var
	wienerFirst = new Wiener(SAMPLE_RATE, OUTPUT_BUFFER_SIZE, 128);
	wienerSecond = new Wiener(SAMPLE_RATE, OUTPUT_BUFFER_SIZE, 128);
	wienerMulti = new Wiener(SAMPLE_RATE, SP_BUFFER_SIZE, 128);

	//INIT ADUIO HARDWARE ::::
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
        setAudioChannels (0, 2);
    }   
	AudioDeviceManager::AudioDeviceSetup currentAudioSetup;
	deviceManager.getAudioDeviceSetup(currentAudioSetup);
	currentAudioSetup.bufferSize = OUTPUT_BUFFER_SIZE;			//SET AUDIO OUTPUT BUFFER
	currentAudioSetup.sampleRate = SAMPLE_RATE;
	deviceManager.setAudioDeviceSetup(currentAudioSetup, true);
	DBG(deviceManager.getCurrentAudioDevice()->getCurrentBufferSizeSamples());
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
	firstUDPThread->signalThreadShouldExit();	//stop threads
	secondUDPThread->signalThreadShouldExit();
	thirdUDPThread->signalThreadShouldExit();
	fourthUDPThread->signalThreadShouldExit();
	fifthUDPThread->signalThreadShouldExit();
}

//====================================================================================================
//====================================================================================================

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{   

}

/*
	Every time the hardware needs another audio block this function is called.
	.bufferToFill' should then be filled with the contents that should be played back.
	Always fake read the other fifos to mantain some synchronization
*/
void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
	numSelecetdSources = arrayOfSelectedSecondaryDevices.size();	//how many devices are selected/how many buttons are pressed

	switch (numSelecetdSources)
	{
	case 0:		//no device selected :::: 
		//play DC and fake read all
		bufferToFill.buffer->clear();	
		for (int i = 0; i < devicesConnected; i++)
		{
			arrayOfThreads.operator[](i)->getFIFO()->fakeReadFromFIFO(OUTPUT_BUFFER_SIZE);
		}
		break;
	case 1:		// 1 device selected ::::
	{
		//copy the contents of the selected FIFO to bufferToFill and fake read from the other FIFOs
		selectedSource = arrayOfSelectedSecondaryDevices.getFirst();
		for (int i = 0; i < devicesConnected; i++)
		{
			selectedFiFo = arrayOfThreads.operator[](i)->getFIFO();
			if ( i == selectedSource ) //if i corresponds to selected source: copy content to bufferToFill
			{
				selectedFiFo->readFromFIFO(bufferToFill.buffer->getWritePointer(LEFT_CHANNEL, bufferToFill.startSample), OUTPUT_BUFFER_SIZE);
				FloatVectorOperations::copy(bufferToFill.buffer->getWritePointer(RIGHT_CHANNEL, bufferToFill.startSample), bufferToFill.buffer->getReadPointer(LEFT_CHANNEL, bufferToFill.startSample), OUTPUT_BUFFER_SIZE);	//copy left channel to right channel
				if (isRecording)
				{
					allOutputDataPart2.insert(std::end(allOutputDataPart2), bufferToFill.buffer->getReadPointer(LEFT_CHANNEL, bufferToFill.startSample), bufferToFill.buffer->getReadPointer(LEFT_CHANNEL, bufferToFill.startSample) + OUTPUT_BUFFER_SIZE);	//store current time frame
				}
				continue; 
			}
			selectedFiFo->fakeReadFromFIFO(OUTPUT_BUFFER_SIZE);	//else fake read
		}
		break;
	}
	default:	//multiple devices selected ::::
	{
		if (doNoiseReduction == true)
		{
			//NOISE REDUCTION ::::

			//combined channels
			std::vector<std::vector<float>> inputChannels;	//this will contain multiple channels
			int numOfChannels = 0;

			//get indexes of selected sources
			int firstSelectedSource = arrayOfSelectedSecondaryDevices.operator[](0);
			int secondSelectedSource = arrayOfSelectedSecondaryDevices.operator[](1);	//only the first two selected devices will be used

			//first channel
			std::vector<float> firstSelectedChannel(OUTPUT_BUFFER_SIZE, 0);

			selectedFiFo = arrayOfThreads.operator[](firstSelectedSource)->getFIFO();
			selectedFiFo->readFromFIFO(&firstSelectedChannel, OUTPUT_BUFFER_SIZE);
			
			//downsample first channel
			std::vector<float> firstSelectedChannelDownSampled(SP_BUFFER_SIZE, 0);
			Resampler::downSample(firstSelectedChannel.data(), OUTPUT_BUFFER_SIZE, firstSelectedChannelDownSampled.data(), SP_BUFFER_SIZE);

			//add to input channel
			inputChannels.push_back(firstSelectedChannelDownSampled);

			//second channel
			std::vector<float> secondSelectedChannel(OUTPUT_BUFFER_SIZE, 0);
			selectedFiFo = arrayOfThreads.operator[](secondSelectedSource)->getFIFO();
			selectedFiFo->readFromFIFO(&secondSelectedChannel, OUTPUT_BUFFER_SIZE);

			//downsample second channel
			std::vector<float> secondSelectedChannelDownSampled(SP_BUFFER_SIZE, 0);
			Resampler::downSample(secondSelectedChannel.data(), OUTPUT_BUFFER_SIZE, secondSelectedChannelDownSampled.data(), SP_BUFFER_SIZE);

			//add to input channel
			inputChannels.push_back(secondSelectedChannelDownSampled);

			//initialise
			if (isFirstTime)
			{
				
				//wienerSecond->initializeFilter(&secondSelectedChannel, 3);
				//wienerFirst->initializeFilter(&firstSelectedChannel, 3);
				MCF = new MultiChannelFilter(2, 1, 0.7, 0.9, SAMPLE_RATE/(OUTPUT_BUFFER_SIZE/SP_BUFFER_SIZE), inputChannels);	
				isFirstTime = true;
				
			}

			//filter
			std::vector<float> output = MCF->Filter2(inputChannels);
			/*std::vector<float> output1 = wienerFirst->filterSignal(&firstSelectedChannel);
			std::vector<float> output2 = wienerSecond->filterSignal(&secondSelectedChannel);*/
	/*		std::vector<float> output(OUTPUT_BUFFER_SIZE, 0);
			FloatVectorOperations::copy(output.data(), output1.data(), OUTPUT_BUFFER_SIZE);
			FloatVectorOperations::addWithMultiply(output.data(), output2.data(), 0.5, OUTPUT_BUFFER_SIZE);*/

			//dsp::Matrix<float>* SPP = MCF->getSPP();
			//std::vector<float> output1 = wienerFirst->filterMultichannel(&output, SPP);

			//upsample output
			std::vector<float> outputUpsampled(OUTPUT_BUFFER_SIZE, 0);
			Resampler::upsample(output.data(), SP_BUFFER_SIZE, outputUpsampled.data(), OUTPUT_BUFFER_SIZE);

			if (isRecording)
			{
				allOutputDataPart3b_enh.insert(std::end(allOutputDataPart3b_enh), std::begin(outputUpsampled), std::end(outputUpsampled));
			}

			//write to bufferToFill
			FloatVectorOperations::copy(bufferToFill.buffer->getWritePointer(LEFT_CHANNEL, bufferToFill.startSample), outputUpsampled.data(), OUTPUT_BUFFER_SIZE);
			//FloatVectorOperations::copy(bufferToFill.buffer->getWritePointer(LEFT_CHANNEL, bufferToFill.startSample), output.data(), OUTPUT_BUFFER_SIZE);


		}
		else
		{
			//TIME ALIGNMENT ::::

			//get indexes of selected sources
			int firstSelectedSource = arrayOfSelectedSecondaryDevices.operator[](0);
			int secondSelectedSource = arrayOfSelectedSecondaryDevices.operator[](1);	//only the first two selected devices will be used

			//get data of first selected source
			std::vector<float> referenceVector(OUTPUT_BUFFER_SIZE);
			arrayOfThreads.operator[](firstSelectedSource)->getFIFO()->readFromFIFO(&referenceVector, OUTPUT_BUFFER_SIZE);
			if (isRecording)
			{
				allOutputDataPart2e_1.insert(std::end(allOutputDataPart2e_1), referenceVector.begin(), referenceVector.end());	//store current time frame
			}

			//get data of second selected source
			std::vector<float> vectorToAllign(OUTPUT_BUFFER_SIZE);
			arrayOfThreads.operator[](1)->getFIFO()->readFromFIFO(&vectorToAllign, OUTPUT_BUFFER_SIZE);	//TODO: make dynamic
			if (isRecording)
			{
				allOutputDataPart2e_2.insert(std::end(allOutputDataPart2e_2), vectorToAllign.begin(), vectorToAllign.end());	//store current time frame
			}

			//align
			std::vector<float> alignedVector = timeAligner->alignWithReferenceFast(referenceVector.data(), vectorToAllign.data(), OUTPUT_BUFFER_SIZE);

			////add aligned vector and reference vector together
			std::vector<float> output(OUTPUT_BUFFER_SIZE, 0);
			FloatVectorOperations::copy(output.data(), referenceVector.data(), OUTPUT_BUFFER_SIZE);	//copy reference to output
			FloatVectorOperations::add(output.data(), alignedVector.data(), OUTPUT_BUFFER_SIZE);	//add aligned data to output

			////align
			//std::vector<float> output = timeAligner->alignWithReferenceAndMix(referenceVector.data(), vectorToAllign.data());	//alligns two signals and mixes them together

			if (isRecording)
			{
				allOutputDataPart2e_mix.insert(std::end(allOutputDataPart2e_mix), output.begin(), output.end());	//store current time frame
			}

			//write to bufferToFill
			FloatVectorOperations::copy(bufferToFill.buffer->getWritePointer(LEFT_CHANNEL, bufferToFill.startSample), output.data(), OUTPUT_BUFFER_SIZE);
			FloatVectorOperations::copy(bufferToFill.buffer->getWritePointer(RIGHT_CHANNEL, bufferToFill.startSample), bufferToFill.buffer->getReadPointer(LEFT_CHANNEL, bufferToFill.startSample), OUTPUT_BUFFER_SIZE);	//copy left channel to right channel
		}
		break;
	}
	}
	
}

void MainComponent::releaseResources()
{

}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

}

/*
	This function sets the size of every component in the GUI.
*/
void MainComponent::resized()
{    
	//INIT INTERFACE ::::
	//init left area
	leftArea = getLocalBounds();
	leftArea.removeFromTop(margins);
	leftArea.removeFromLeft(margins + 5);
	leftArea.removeFromBottom(margins);
	leftArea.removeFromRight(getWidth() - leftAreaWidth);
	textfieldInputPort.setBounds(leftArea.removeFromTop(textButtonHeight));
	addDevice.setBounds(leftArea.removeFromTop(textButtonHeight));
	noiseReduction.setBounds(leftArea.removeFromTop(textButtonHeight));	
    cutoffFreqSlider.setBounds(leftArea.removeFromTop(cutoffFreqSliderHeight));
	saveFile.setBounds(leftArea.removeFromBottom(textButtonHeight));
	selectDataComboBox.setBounds(leftArea.removeFromBottom(textButtonHeight));
	recordButton.setBounds(leftArea.removeFromBottom(textButtonHeight));

	//init right area
	rightArea = getLocalBounds();
	rightArea.removeFromLeft(leftAreaWidth + 5 + 5 + margins);
	header.setBounds(rightArea.removeFromTop(textButtonHeight));
	selecAllButton.setBounds(rightArea.removeFromTop(textButtonHeight));

	resizeButtons();
}

/*
	This function dynamically sets the size for every dynamicly created button in the GUI.
*/
void MainComponent::resizeButtons()
{
	for each (TextButton * button in arrayOfButtonsForEachSender)
	{
		button->setBounds(rightArea.removeFromTop(textButtonHeight));
	}
}


/*
	Listener for button clicked.
	Unfortunally there doesn't seem to be a way to have a seperate callback for each button.
	As of writing this the order is:
		- add device
		- select all
		- noiser reduction
		- save file
		- record audio
		- select a certain secondary device
*/
void MainComponent::buttonClicked(Button* button)
{
	if (button == &addDevice)
	{
		//ADD new device ::::
		if (devicesConnected >= 5) return;
		//check input
		if (textfieldInputPort.getTotalNumChars() != 4) return;	//invalid

		//extract port number
		String portInput = textfieldInputPort.getText();
		textfieldInputPort.clear();
		int  newPortNumber = std::stoi(portInput.toStdString());
		//start new UDP thread
		UDPThread* dumdum = arrayOfThreads.operator[](devicesConnected);
		dumdum->setPort(newPortNumber);
		dumdum->createSocket();
		dumdum->startThread();	//TODO: allign buffers.


		auto newButton = new TextButton(portInput);
		newButton->setClickingTogglesState(true);
		newButton->setColour(TextButton::textColourOnId, Colours::white);
		newButton->setColour(TextButton::textColourOffId, Colours::grey);
		newButton->addListener(this);
		addAndMakeVisible(newButton);
		arrayOfButtonsForEachSender.add(newButton);
		resized();

		devicesConnected++;

	}
	else if (button == &selecAllButton)
	{
		//Select/deselect all devices ::::
		if (selecAllButton.getToggleState() == true)
		{
			for each (TextButton * button in arrayOfButtonsForEachSender)
			{
				button->setToggleState(true, NotificationType::dontSendNotification);
				int indexOfButtonToggled = arrayOfButtonsForEachSender.indexOf((TextButton*)button);
				if (arrayOfSelectedSecondaryDevices.contains(indexOfButtonToggled) == false)
				{
					arrayOfSelectedSecondaryDevices.add(indexOfButtonToggled);
				}
			}
		}
		else
		{
			for each (TextButton * button in arrayOfButtonsForEachSender)
			{
				button->setToggleState(false, NotificationType::dontSendNotification);
				int indexOfButtonToggled = arrayOfButtonsForEachSender.indexOf((TextButton*)button);
				if (arrayOfSelectedSecondaryDevices.contains(indexOfButtonToggled) == true)
				{
					int index = arrayOfSelectedSecondaryDevices.indexOf(indexOfButtonToggled);
					arrayOfSelectedSecondaryDevices.remove(index);
				}
			}
		}
	}
	else if (button == &noiseReduction)
	{
		//Do noise reduction ::::
		doNoiseReduction = !doNoiseReduction;
	
	}
	else if (button == &saveFile)
	{
		//Save the selected file, files are selected via the drop down menu in the GUI
		FileChooser chooser("Save file",
			File::getSpecialLocation(File::userDesktopDirectory),
			"*.wav");
		if (chooser.browseForFileToSave(true) == true)
		{
			String filePath = chooser.getResult().getFullPathName();
			int selected = selectDataComboBox.getSelectedId();
			switch (selected)
			{
			case 0:
				//no file selected
				break;
			case 1:
				writer.writeWAV(firstAllData, filePath);
				break;
			case 2:
				writer.writeWAV(secondAllData, filePath);
				break;
			case 3:
				writer.writeWAV(thirdAllData, filePath);
				break;
			case 4:
				writer.writeWAV(fourhtAllData, filePath);
				break;
			case 5:
				writer.writeWAV(fifthAllData, filePath);
				break;
			case 6:
				writer.writeWAV(allOutputDataPart2, filePath);
				break;
			case 7:
				writer.writeWAV(allOutputDataPart2e_1, filePath);
				break;
			case 8:
				writer.writeWAV(allOutputDataPart2e_2, filePath);
				break;
			case 9:
				writer.writeWAV(allOutputDataPart2e_mix, filePath);
				break;
			case 10:
				writer.writeWAV(allOutputDataPart3b_enh, filePath);
				break;
				//TODO: add more
			default:
				break;
			}
		}
		else
		{
			DBG("ERROR opening file chooser");
		}
	}
	else if ( button == &recordButton )
	{
		//Toggle wether to append data to the recording buffers in real time
		//The advantage of having a global toggle for recording audio is that threads which aren' started at the same do start recording at +- the same time.
		//Otherwise the program would start and audio would be recording 1: even before devices are connected, 2: when adding multiple devices in a row there would be an offset in the different audio streams
		if (isRecording)
		{
			isRecording = false;
		}
		else
		{
			isRecording = true;
		}
	}
	else
	{
		// check which of the buttons corresponding to secondary devicesis toggled.
		int indexOfButtonToggled = arrayOfButtonsForEachSender.indexOf((TextButton*)button);
		if (button->getToggleState() == true)
		{
			arrayOfSelectedSecondaryDevices.add(indexOfButtonToggled);
		}
		else
		{
			int index = arrayOfSelectedSecondaryDevices.indexOf(indexOfButtonToggled);
			arrayOfSelectedSecondaryDevices.remove(index);
		}
	}
}

