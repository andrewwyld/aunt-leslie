/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

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
}

AuntLeslieAudioProcessor::~AuntLeslieAudioProcessor()
{
    releaseResources();
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
    return INTRINSIC_BUFFER_DELAY_S;
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
    // ya never know
    releaseResources();

    initializeLines(getTotalNumInputChannels(), (int) sampleRate * INTRINSIC_BUFFER_DELAY_S);
    
    max_treble_horn_excursion_samples = sampleRate * MAX_TREBLE_HORN_EXCURSION_S;
    chorale_frequency_per_sample = CHORALE_HZ / sampleRate;
    tremolo_frequency_per_sample = TREMOLO_HZ / sampleRate;
}

void AuntLeslieAudioProcessor::releaseResources()
{
    if (delayLines) {
        for (int i = 0; i < channelsIn; ++i) {
            if (delayLines[i]) {
                delete [] delayLines[i];
            }
        }
        delete [] delayLines;
        delayLines = NULL;
    }
    
}

void AuntLeslieAudioProcessor::initializeLines(int channelsIn_, int delayLineLength_) {
    channelsIn = channelsIn_;
    delayLineLength = delayLineLength_;
    
    delayLines = new float*[channelsIn];

    for (int i = 0; i < channelsIn; ++ i) {
        delayLines[i] = new float[delayLineLength];
    }
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

    // delay line recording boundaries for all channels
    int delayLineRecordLength = std::min(sampleCount, delayLineLength);
    int delayLineInputStart = std::max(0, sampleCount - delayLineLength);

    // we will assume two ins, two outs
    // TODO ensure this!
    float* outputs[] = {buffer.getWritePointer(LEFT), buffer.getWritePointer(RIGHT)};
    const float* inputs[] = {buffer.getReadPointer(TREBLE_HORN_A), buffer.getReadPointer(TREBLE_HORN_B)};
    
    // process variable delay

    // for either stereo output channel
    for (int outputIdx = 0; outputIdx < totalNumOutputChannels; ++outputIdx)
    {
        float sampleSum = 0.f;

        // for every write position in the output arrays
        for (int writeHead = 0; writeHead < sampleCount; ++writeHead)
        {
            
            // and for both treble horns
            for (int inputIdx = 0; inputIdx < totalNumInputChannels; ++inputIdx)
            {
                // get the appropriate read head function value
                float readHead = getReadHead(inputIdx, outputIdx, writeHead);
                
                int readHeadLo = std::floor(readHead);
                int readHeadHi = std::ceil(readHead);
                float proportion = readHead - readHeadLo;
                
                float sampleLo = readHeadLo < 0 ? delayLines[inputIdx][(delayLineRecordHeadPosition + readHeadLo) % delayLineLength] : inputs[inputIdx][readHeadLo];
                float sampleHi = readHeadHi < 0 ? delayLines[inputIdx][(delayLineRecordHeadPosition + readHeadHi) % delayLineLength] : inputs[inputIdx][readHeadHi];
                
                // linear interpolation's good enough for you, right? me too
                float filterAnd = (1.f - proportion) * sampleLo + proportion * sampleHi;
                
                // TODO LP filtering happens before summing! (Am I going to need a secondary buffer?)
                sampleSum += filterAnd;
            }

            outputs[outputIdx][writeHead] = sampleSum / 2.f;
        }
    }

    for (int inputIdx = 0; inputIdx < totalNumInputChannels; ++inputIdx)
    {
        // record delay line for next pass
        for (
             int readHead = delayLineInputStart, i = delayLineRecordHeadPosition;
             readHead < sampleCount;
             ++readHead, ++i, i %= delayLineLength
             ) {
                 delayLines[inputIdx][i] = inputs[inputIdx][readHead];
        }
    }
    
    delayLineRecordHeadPosition += delayLineRecordLength;
    delayLineRecordHeadPosition %= delayLineLength;
    
    lastSample += sampleCount;
}

// read head position based on sine shift
// TODO have shiftable sine rate
float AuntLeslieAudioProcessor::getReadHead(int hornIdx, int stereoChannel, int writeHead)
{
    // get current absolute sample time for continuity
    long time = lastSample + writeHead;

    float theta = time * chorale_frequency_per_sample;
    
    int sign = 0;
    
    switch (hornIdx)
    {
        case TREBLE_HORN_A:
            sign = 1;
            break;
        case TREBLE_HORN_B:
            sign = -1;
            break;
    }
    
    float longitudinal = 0.f;
    float transverse = 0.f;
    
    switch (stereoChannel)
    {
        case LEFT:
            longitudinal = MIC_DISTANCE - sign * std::sin(theta);
            transverse = std::cos(theta);
            break;
        case RIGHT:
            longitudinal = MIC_DISTANCE - sign * std::cos(theta);
            transverse = std::sin(theta);
            break;
    }
    
    float offset = 1.f + std::sqrt(longitudinal * longitudinal + transverse * transverse);

    // multiply offset by horn excursion delay and subtract from write head position
    return writeHead - max_treble_horn_excursion_samples * offset;
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
