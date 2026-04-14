/*
  ==============================================================================

   A U N T   L E S L I E
   a fully electronic Leslie speaker simulation
   (c) 2026 Andrew Wyld

   processor code
 
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AuntLeslieAudioProcessor::AuntLeslieAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    rotor = new Rotor();
}

AuntLeslieAudioProcessor::~AuntLeslieAudioProcessor()
{
    releaseResources();
    delete rotor;
}

//==============================================================================
const juce::String AuntLeslieAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AuntLeslieAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AuntLeslieAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AuntLeslieAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AuntLeslieAudioProcessor::getTailLengthSeconds() const
{
    return rotor->INTRINSIC_BUFFER_DELAY_S;
}

int AuntLeslieAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AuntLeslieAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AuntLeslieAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AuntLeslieAudioProcessor::getProgramName (int index)
{
    return {};
}

void AuntLeslieAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AuntLeslieAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::Logger::writeToLog("prepareToPlay: sampleRate = " + juce::String(sampleRate) + ", samplesPerBlock = " + juce::String(samplesPerBlock));
    
    // ya never know
    releaseResources();

    rotor->initializeLines(getTotalNumInputChannels(), sampleRate);
}

void AuntLeslieAudioProcessor::releaseResources()
{
    rotor->releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AuntLeslieAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void AuntLeslieAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto sampleCount = buffer.getNumSamples();

    // we will assume two ins, two outs
    // TODO ensure this!
    float* outputs[] = {buffer.getWritePointer(LEFT), buffer.getWritePointer(RIGHT)};
    const float* inputs[] = {buffer.getReadPointer(TREBLE_HORN_A), buffer.getReadPointer(TREBLE_HORN_B)};
    
    rotor->processBlock(sampleCount, totalNumOutputChannels, outputs, totalNumInputChannels, inputs);
}

//==============================================================================
bool AuntLeslieAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AuntLeslieAudioProcessor::createEditor()
{
    return new AuntLeslieAudioProcessorEditor (*this);
}

//==============================================================================
void AuntLeslieAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AuntLeslieAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AuntLeslieAudioProcessor();
}
