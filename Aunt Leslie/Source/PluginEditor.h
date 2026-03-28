/*
  ==============================================================================

   A U N T   L E S L I E
   a fully electronic Leslie speaker simulation
   (c) 2026 Andrew Wyld

   editor header
 
  ==============================================================================
*/


#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AuntLeslieAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AuntLeslieAudioProcessorEditor (AuntLeslieAudioProcessor&);
    ~AuntLeslieAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AuntLeslieAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuntLeslieAudioProcessorEditor)
};
