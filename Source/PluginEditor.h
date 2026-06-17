/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class BasicDelayAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    BasicDelayAudioProcessorEditor (BasicDelayAudioProcessor&);
    ~BasicDelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    juce::Slider delayTimeSlider;
    juce::Slider feedbackSlider;
    juce::Slider mixSlider;
    juce::Slider outputGainSlider;
    juce::Slider stereoWidthSlider;
    
    juce::Label delayTimeLabel;
    juce::Label feedbackLabel;
    juce::Label mixLabel;
    juce::Label outputGainLabel;
    juce::Label stereoWidthLabel;
    
    std::unique_ptr<SliderAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> feedbackAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> stereoWidthAttachment;
    
    BasicDelayAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicDelayAudioProcessorEditor)
};
