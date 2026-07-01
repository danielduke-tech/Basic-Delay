/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BasicDelayAudioProcessor::BasicDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
                    apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}
juce::AudioProcessorValueTreeState::ParameterLayout
BasicDelayAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    // Delay Time controls how far back in the circular buffer is needed
    // Range is 1 ms to 2000 ms, with a default of 400 ms
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayTime", 1},
        "Delay Time",
        juce::NormalisableRange<float> (1.0f, 2000.0f, 1.0f),
        400.0f));
    
    // Feedback controls how much of the delayed signal is written back into the delay buffer.
    // It is limited to 0.95 instead of 1.0 to reduce the risk of unstable runaway feedback.
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1},
        "FeedBack",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f),
        0.35f));
    
    // Dry/Wet Mix controls the balance between the original input and the delayed signal.
    //0.0 is fully dry, 1.0 is fully wet, and 0.5 is an equal blend.
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1},
        "Dry/Wet Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f));
    
    //Output Gain is stored in decibels for the user, then converted to linear gain in processBlock.
    //0 dB means no volume change.
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputGain", 1},
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f),
        0.0f));
    
    // Stereo Width offsets the left and right delay times slightly,
    // 0.0mkeeps both channels the same, while 1.0 gives the widesr left/right timing difference.
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"steroWidth", 1},
        "Stereo Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.25f));
    return { parameters.begin(), parameters.end() };
}
BasicDelayAudioProcessor::~BasicDelayAudioProcessor()
{
}

//==============================================================================
const juce::String BasicDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BasicDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BasicDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BasicDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BasicDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BasicDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BasicDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BasicDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BasicDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void BasicDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BasicDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
   // samplePerBlock is supplied by the host, but this delay setup only needs sampleRate.
    juce::ignoreUnused (samplesPerBlock);
    
    //Use whuchever is larger: input channels or output channels.
    // THis makes sure the delay buffer has enough channels for the current host layout.
    const int numChannels = juce::jmax (1, getTotalNumInputChannels(), getTotalNumOutputChannels());
    
    // Maximum delay time is 2000 ms, which is 2 secondds.
    // Convert seconds to samples using:
    // samples = seconds * samleRate
    const int maxDelayInSamples = static_cast<int> (sampleRate * 2.0);
    
    // Allocate the delay buffer.
    // Each channel gets its own delay memory, so stereo audio has separate left/right buffers.
    delayBuffer.setSize (numChannels, maxDelayInSamples);
    
    // Clear the buffer so the delay starts with silence instead of random old memory.
    delayBuffer.clear();
    
    // Start writing at the beginning of the circular buffer.
    delayWritePosition = 0;
}

void BasicDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BasicDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif
    // This function is called repeatedly by the host to process of audio. 
    // All delay DSP happenes herw, samplemby sample, using a circular buffer

void BasicDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    // This plugin does not use MIDI, so we explicity mark it as unused.
    juce::ignoreUnused (midiMessages);

    // Prevents very tiny floating-point values from slowing down the CPU during audio processing.
    juce::ScopedNoDenormals noDenormals;

    // Get the current audio block information from the host.
    // numSamples is how many samples are in this block.
    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // If there are more outputs than inputs, clear the unused output channels.
    //This prevents unwanted old audio data from being heard
    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
    
    // Get safe pointers to the plugin parameters.
    // If a parameter ID is spelled wrong, the pointer could be null, so it is checked before it is used.
    auto* delayTimeParam = apvts.getRawParameterValue ("delayTime");
    auto* feedbackParam = apvts.getRawParameterValue ("feedback");
    auto* mixParam = apvts.getRawParameterValue ("mix");
    auto* outputGainParam = apvts.getRawParameterValue ("outputGain");
    auto* stereoWidthParm = apvts.getRawParameterValue("stereoWidth");

    if (delayTimeParam == nullptr || feedbackParam == nullptr || mixParam == nullptr || outputGainParam == nullptr || stereoWidthParm == nullptr)
        return;
 
    // Read the current parameter values.
    // Delay time is in milliseconds.
    // Feebback and mix use 0.0 to 1.0 ranges.
    // Output gain is stored in decibels, so it must be converted before multiplyinh audio samples.
    const float delayTimeMs = *apvts.getRawParameterValue ("delayTime");
    const float feedback = *apvts.getRawParameterValue ("feedback");
    const float mix = *apvts.getRawParameterValue ("mix");
    const float outputGainDb = *apvts.getRawParameterValue ("outputGain");
    const float outputGain = juce::Decibels::decibelsToGain (outputGainDb);
    const float stereoWidth = stereoWidthParm->load();
    
    // The delay buffer stores past samples.
    // Its length in samples determines the maximum possible delay time.
    const int delayBufferSize = delayBuffer.getNumSamples();

    if (delayBufferSize <= 1)
        return;

    // Only process channels that exist in both the audio buffer and the delay buffer.
    // This protects the plugin if the host gives an unexpected channel layout.
    const int numDelayChannels = delayBuffer.getNumChannels();
    const int channelsToProcess = juce::jmin (numInputChannels, numDelayChannels);

    if (channelsToProcess <= 0)
        return;

    // Make sure the write pointer is inside the circular buffer before processing.
    if (delayWritePosition >= delayBufferSize)
        delayWritePosition = 0;

    
    // Stereo width is created by offsetting the delay time different for left and right.
    // At full width, the left delay is 20 ms shorter and the right ddelay is 20 ms longer.
    const float maxWidthOffsetMs = 20.0f;
    const float widthOffsetMs = stereoWidth * maxWidthOffsetMs;
    
   

    for (int sample = 0; sample < numSamples; ++sample)
    {

        for (int channel = 0; channel < channelsToProcess; ++channel)
        {
            // Start with the main delay time, then adjust it per channel for stereo width.
            float channelDelayMs = delayTimeMs;
            
            if (channel == 0)
                channelDelayMs -= widthOffsetMs;
            else if (channel == 1)
                channelDelayMs += widthOffsetMs;
            
            // Keep the delay time inside the supported range.
            channelDelayMs = juce::jlimit(1.0f, 2000.0f, channelDelayMs);
            
            // Converts milliseconds to samples:
            // seconds = milliseconds / 1000
            // samples =  seconds * sampleRate
            const int requestedDelaySamples = static_cast<int>((channelDelayMs / 1000.0f) * getSampleRate());
            
            // Keep the delay length inside the actual delay buffer size.
            const int delayTimeSamples = juce::jlimit(1, delayBufferSize - 1, requestedDelaySamples);
            
            // Circular buffer read position:
            // read from delayTimeSamples behind the current write position.
            // Adding delayBufferSize prevents negative positions, and % wraps around the buffer.
            const int readPosition = (delayWritePosition + delayBufferSize - delayTimeSamples) % delayBufferSize;
            
            
            const float inputSample = buffer.getSample (channel, sample);
            const float delayedSample = delayBuffer.getSample (channel, readPosition);

            // Dry/wet mix:
            // mix = 0.0 gives only dry input.
            // mix = 1.0 gives only delayed signal.
            const float outputSample = (inputSample * (1.0f - mix)) + (delayedSample * mix);

            // Apply final output gain after the dry/wet mix.
            buffer.setSample (channel, sample, outputSample * outputGain);

            // Feedback writes input + feedback * delayedSample back into the delay buffer.
            // This feeds part of the old delayed sound back into the buffer to create repeats.
            const float valueToStore = inputSample + (feedback * delayedSample);

            delayBuffer.setSample (channel, delayWritePosition, valueToStore);
        }

        //Move the circular buffer write pointer forward by one sample.
        ++delayWritePosition;

        // Wrap the write pointer back to the start when it reaches the end.
        if (delayWritePosition >= delayBufferSize)
            delayWritePosition = 0;
    }
}

//==============================================================================
bool BasicDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BasicDelayAudioProcessor::createEditor()
{
    return new BasicDelayAudioProcessorEditor (*this);
}

//==============================================================================
void BasicDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BasicDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BasicDelayAudioProcessor();
}
