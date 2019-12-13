# SPAI_project_2019_group1
SPAI_project_2019_group1

This git contains 4 code projecs.
- an Android Studio project that can record audio and send it over UDP
- a Juce project for IOS/macOS/Windows that can record audio and send it over UDP
- a Juce project for Windows/macOs that can receive audio of UDP, process it, and play it back
- a Matlab project that does noise reduction on audio files

To edit the Android Studio project, Android Studio is required.
To edit the Juce projects, Projucer and Microsoft Visual Studio/XCode are required.
To edit the Matlab project, Matlab is required.


// PRIMARY DEVICE ::::
A primary device receives audio over UDP, processes it, and plays it back.
A primary device can also store:
- all the audio it is receiving,
- the audio that is selected for playback,
- the audio that is used for timealignmet, multichannel filtering
- the audio that is obtained after timealignmet, multichannel filtering

In the GUI the user can see the IP address of the device, this IP adress can be used by the secondary devices.
IMPORTANT:
This IP address is not always the correct one.
To be 100% sure 'ipconfig' on Windows or 'ipconfig getifaddr en0' on macOS should be used.

To be able to receive data from a new secondary device a new port needs to be opened.
Write the desired port number in the top-left textfield and hit the + button.

To play the audio from a single secondary device, toggle the corresponding button.
To mix multiple devices, toggle multiple buttons.
To do noise reduction, toggle multiple buttons and toggle the NR button.
To record audio, press the record button.
To save an audio output, select the desired output in the drop down menu and hit save.
This can be done multiple times for different audio outputs.
IMPORTANT: 
It is best to stop recording before saving since, in theory, this operation is not thread safe.

Audio streams that can be saved are:
- every incoming audio stream (so the raw audio from every secnodary device)
- an audio stream that is the result of switching between different audio streams (so for example 5 seconds of the audio of device 2 is followed by 3 seconds of the audio of device 5)
- the audio streams before and after doing signal processing

IMPORTANT: only 5 connected devices are suported.
This could be adjusted in code but remember that every secondary device equals a new thread.

// SECONDARY DEVICE ::::
A secondary device records audio and sends it over UDP.
A secondary device can also store the audio it's sending.
In the GUI the user needs to set the IP and port to which it want to send it's data.
Make sure every secondary device is sending to a unique port and that portforwarding is allowed on the used network.
In IOS/Windows/macOs, set a file name in the UI, in android, set a file name in the code itself.

Once ready, press 'Start Recording' to record and send audio data.
IMPORTANT: 
If a device locks itself to save battery it is possible it is no longer recording and or sending audio.
Sometimes pressing stop and then start again solves this, sometimes the application needs to be restard.

// HOW TO BUILD
1. Make a new 'Audio Application' with Projucer (JUCE).
2. Add source filles to this project via the Projucer.
3. Choose the right 'Selected exporter' for the platform of choice, and open it in an IDE by clicking on 'Save and Open in IDE'.
4. Compile and run in IDE.
Note: this code requieres the eigen library. This library is not included in this repository. 

//Note on the offline noise reduction
- The Matlab file has comments describing all the different variables
- We have two different enhanced signals for part 3a, an MWF output which has some musical noise and a cepstral smoothing output which has no noise but the speech is less clear
- Both signals have high noise for a small duration in the beginning
