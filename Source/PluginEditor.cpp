/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BasicDelayAudioProcessorEditor::BasicDelayAudioProcessorEditor (BasicDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (650, 300);
    
    delayTimeLabel.setText ("Delay Time", juce::dontSendNotification);
    feedbackLabel.setText ("Feedback", juce::dontSendNotification);
    mixLabel.setText ("Dry/Wet Mix", juce::dontSendNotification);
    outputGainLabel.setText ("Output Gain", juce::dontSendNotification);
    stereoWidthLabel.setText("Stereo Width", juce::dontSendNotification);
    
    addAndMakeVisible (delayTimeLabel);
    addAndMakeVisible (feedbackLabel);
    addAndMakeVisible (mixLabel);
    addAndMakeVisible (outputGainLabel);
    addAndMakeVisible (stereoWidthLabel);
    
    delayTimeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    stereoWidthSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    
    delayTimeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
    feedbackSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
    stereoWidthSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 30);
    
    
    addAndMakeVisible (delayTimeSlider);
    addAndMakeVisible (feedbackSlider);
    addAndMakeVisible (mixSlider);
    addAndMakeVisible (outputGainSlider);
    addAndMakeVisible (stereoWidthSlider);
    
    delayTimeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayTime", delayTimeSlider);
    feedbackAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "feedback", feedbackSlider);
    mixAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "mix", mixSlider);
    outputGainAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "outputGain", outputGainSlider);
    stereoWidthAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "stereoWidth", stereoWidthSlider);
}

BasicDelayAudioProcessorEditor::~BasicDelayAudioProcessorEditor()
{
}

//==============================================================================
void BasicDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (30, 40, 42));

    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("Basic Delay", getLocalBounds().removeFromTop (50), juce::Justification::centred, 1);
}

void BasicDelayAudioProcessorEditor::resized()
{
    const int sliderWidth = 110;
    const int sliderHeight = 120;
    const int labelHeight = 24;
    const int top = 80;
    const int gap = 10;
    
    delayTimeLabel.setBounds (30, top, sliderWidth, labelHeight);
    delayTimeSlider.setBounds (30, top + labelHeight + gap, sliderWidth, sliderHeight);
    
    feedbackLabel.setBounds (145, top, sliderWidth, labelHeight);
    feedbackSlider.setBounds (145, top + labelHeight + gap, sliderWidth, sliderHeight);
    
    mixLabel.setBounds (260, top, sliderWidth, labelHeight);
    mixSlider.setBounds (260, top + labelHeight + gap, sliderWidth, sliderHeight);
    
    outputGainLabel.setBounds (375, top, sliderWidth, labelHeight);
    outputGainSlider.setBounds (375, top + labelHeight + gap, sliderWidth, sliderHeight);
    
    stereoWidthLabel.setBounds(490, top, sliderWidth, labelHeight);
    stereoWidthSlider.setBounds(490, top + labelHeight + gap, sliderWidth, sliderHeight);
}
