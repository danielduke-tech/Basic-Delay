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
    
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "delayTime", 1},
        "Delay Time",
        juce::NormalisableRange<float> (1.0f, 2000.0f, 1.0f),
        400.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "feedback", 1},
        "FeedBack",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f),
        0.35f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1},
        "Dry/Wet Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputGain", 1},
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f),
        0.0f));
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
    juce::ignoreUnused (samplesPerBlock);
    
    const int numChannels = juce::jmax (1, getTotalNumInputChannels(), getTotalNumOutputChannels());
    // Maximum delay time is 2000 ms, which is 2 secondds.
    const int maxDelayInSamples = static_cast<int> (sampleRate * 2.0);
    
    delayBuffer.setSize (numChannels, maxDelayInSamples);
    delayBuffer.clear();
    
    delayWritePosition = 0;
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
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

void BasicDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
    
    auto* delayTimeParam = apvts.getRawParameterValue ("delayTime");
    auto* feedbackParam = apvts.getRawParameterValue ("feedback");
    auto* mixParam = apvts.getRawParameterValue ("mix");
    auto* outputGainParam = apvts.getRawParameterValue ("outputGain");
    auto* stereoWidthParm = apvts.getRawParameterValue("stereoWidth");

    if (delayTimeParam == nullptr || feedbackParam == nullptr || mixParam == nullptr || outputGainParam == nullptr || stereoWidthParm == nullptr)
        return;

    const float delayTimeMs = *apvts.getRawParameterValue ("delayTime");
    const float feedback = *apvts.getRawParameterValue ("feedback");
    const float mix = *apvts.getRawParameterValue ("mix");
    const float outputGainDb = *apvts.getRawParameterValue ("outputGain");
    const float outputGain = juce::Decibels::decibelsToGain (outputGainDb);
    const float stereoWidth = stereoWidthParm->load();
    
    const int delayBufferSize = delayBuffer.getNumSamples();

    if (delayBufferSize <= 1)
        return;

    const int numDelayChannels = delayBuffer.getNumChannels();
    const int channelsToProcess = juce::jmin (numInputChannels, numDelayChannels);

    if (channelsToProcess <= 0)
        return;

    if (delayWritePosition >= delayBufferSize)
        delayWritePosition = 0;

    
    const float maxWidthOffsetMs = 20.0f;
    const float widthOffsetMs = stereoWidth * maxWidthOffsetMs;
    
   

    for (int sample = 0; sample < numSamples; ++sample)
    {

        for (int channel = 0; channel < channelsToProcess; ++channel)
        {
            float channelDelayMs = delayTimeMs;
            
            if (channel == 0)
                channelDelayMs -= widthOffsetMs;
            else if (channel == 1)
                channelDelayMs += widthOffsetMs;
            
            channelDelayMs = juce::jlimit(1.0f, 2000.0f, channelDelayMs);
            
            const int requestedDelaySamples = static_cast<int>((channelDelayMs / 1000.0f) * getSampleRate());
            const int delayTimeSamples = juce::jlimit(1, delayBufferSize - 1, requestedDelaySamples);
            
            const int readPosition = (delayWritePosition + delayBufferSize - delayTimeSamples) % delayBufferSize;
            
            
            const float inputSample = buffer.getSample (channel, sample);
            const float delayedSample = delayBuffer.getSample (channel, readPosition);

            // Mix between the dry input and the delayed signal.
            const float outputSample = (inputSample * (1.0f - mix)) + (delayedSample * mix);

            buffer.setSample (channel, sample, outputSample * outputGain);

            // Feedback writes input + feedback * delayedSample back into the delay buffer.
            const float valueToStore = inputSample + (feedback * delayedSample);

            delayBuffer.setSample (channel, delayWritePosition, valueToStore);
        }

        ++delayWritePosition;

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
