/*
  ==============================================================================

    WAVWriterReader.h

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "globals.h"

class WAVWriterReader
{
public:
	WAVWriterReader() {};

	/*
		Takes a vector of audio data and writes it to a location pointed to by file.
	*/
	void writeWAV(std::vector<float> samples, File file) {
		float* h1 = samples.data();
		float** h2 = &h1;
		AudioBuffer<float> buffer(1, samples.size());
		buffer.setDataToReferTo(h2, 1, samples.size());
		File tempFile(file);
		tempFile.create();
		const StringPairArray& metadataValues("");
		AudioBuffer<float> fileBuffer;
		WavAudioFormat wav;
		ScopedPointer <OutputStream> outStream(tempFile.createOutputStream());
		if (outStream != nullptr) {
			ScopedPointer <AudioFormatWriter> writer(wav.createWriterFor(outStream, SAMPLE_RATE, 1, (int)32, metadataValues, 0));

			if (writer != nullptr) {
				outStream.release();
				writer->writeFromAudioSampleBuffer(buffer, 0, samples.size());
			}
			writer = nullptr;
		}
	}

	/*
		Reads a WAV file into a container of type Matrix.
	*/
	dsp::Matrix<float> readWAV() {
		FileChooser chooser("Select a Wave file shorter than 2 seconds to play...", {}, "*.wav");

		if (chooser.browseForFileToOpen()) {
			AudioFormatManager formatManager;
			AudioBuffer<float> fileBuffer;
			formatManager.registerBasicFormats();
			auto file = chooser.getResult();
			std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(file));

			if (reader.get() != nullptr) {
				auto duration = reader->lengthInSamples / reader->sampleRate;
				fileBuffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
				reader->read(&fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

				std::vector<float> f;
				for (int i = 0; i < fileBuffer.getNumChannels(); i++) {
					f.insert(end(f), fileBuffer.getReadPointer(i, 0), fileBuffer.getReadPointer(i, 0) + 160000);
				}

				dsp::Matrix<float> samples = dsp::Matrix<float>(fileBuffer.getNumChannels(), fileBuffer.getNumSamples(), f.data());
				return samples;
			}
		}
	}
};