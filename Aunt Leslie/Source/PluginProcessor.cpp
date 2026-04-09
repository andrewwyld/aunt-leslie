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
#include <cmath>

const float M_TAU = 2.f * M_PI;

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
    
    filterSampleTime = 1.f / sampleRate;
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
    running[TREBLE_HORN_A][LEFT] = false;
    running[TREBLE_HORN_A][RIGHT] = false;
    running[TREBLE_HORN_B][LEFT] = false;
    running[TREBLE_HORN_B][RIGHT] = false;
}

void AuntLeslieAudioProcessor::initializeLines(int channelsIn_, int delayLineLength_) {
    channelsIn = channelsIn_;
    delayLineLength = delayLineLength_;
    
    delayLines = new float*[channelsIn];

    for (int i = 0; i < channelsIn; ++ i) {
        delayLines[i] = new float[delayLineLength];
        std::memset(delayLines[i], 0, delayLineLength * sizeof(float));
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
    for (int stereoChannel = 0; stereoChannel < totalNumOutputChannels; ++stereoChannel)
    {
        float sampleSum = 0.f;

        // for every write position in the output arrays
        for (int writeHead = 0; writeHead < sampleCount; ++writeHead)
        {
            
            // and for both treble horns
            for (int hornIdx = 0; hornIdx < totalNumInputChannels; ++hornIdx)
            {
                float _theta = theta(writeHead);
                
                // get the appropriate read head function value
                float readHead = (float) writeHead - getReadHeadOffset(hornIdx, stereoChannel, _theta, writeHead);
                
                int readHeadLo = std::floor(readHead);
                int readHeadHi = std::ceil(readHead);
                float proportion = readHead - readHeadLo;
                
                float sampleLo = readHeadLo < 0 ? delayLines[hornIdx][(delayLineRecordHeadPosition + delayLineLength + readHeadLo) % delayLineLength] : inputs[hornIdx][readHeadLo];
                float sampleHi = readHeadHi < 0 ? delayLines[hornIdx][(delayLineRecordHeadPosition + delayLineLength + readHeadHi) % delayLineLength] : inputs[hornIdx][readHeadHi];
                
                // linear interpolation's good enough for you, right? me too
                float filterAnd = (1.f - proportion) * sampleLo + proportion * sampleHi;
                
                sampleSum += filter(hornIdx, stereoChannel, _theta, filterAnd);
            }

            outputs[stereoChannel][writeHead] = sampleSum / 2.f;
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

float AuntLeslieAudioProcessor::theta(int idx)
{
    // get current absolute sample time for continuity
    long time = lastSample + idx;

    return std::fmod(time * chorale_frequency_per_sample, M_TAU);

    // TODO
    // - add enum: MODE_CHORALE, MODE_TREMOLO, MODE_ACCELERATE, MODE_DECELERATE
    // - add square function for MODE_ACCELERATE, MODE_DECELERATE
    // - add variants for the bass and treble rotors (or separate functions?)
    // - add mode for tremolo_frequency_per_sample
}

// read head position based on sine shift
// TODO have shiftable sine rate
float AuntLeslieAudioProcessor::getReadHeadOffset(int hornIdx, int stereoChannel, float theta, int writeHead)
{
    float sign = 0.f;
    
    switch (hornIdx)
    {
        case TREBLE_HORN_A:
            sign = 1.f;
            break;
        case TREBLE_HORN_B:
            sign = -1.f;
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
    
    return max_treble_horn_excursion_samples
    * (1.f + std::sqrt(longitudinal * longitudinal + transverse * transverse));
}

/**
 For simplicity, we define

 $L = 2\pi T_s f_c$

 giving

 $x_{n + 1} = (1 - L)x_n + L K u_n$
*/
float AuntLeslieAudioProcessor::getFilterCoefficient(int hornIdx, int stereoChannel, float theta)
{
    float sign = 0.f;
    
    switch (hornIdx)
    {
        case TREBLE_HORN_A:
            sign = 1.f;
            break;
        case TREBLE_HORN_B:
            sign = -1.f;
            break;
    }

    float f_c = 0.f;
    
    switch (stereoChannel)
    {
        case LEFT:
            f_c = CUTOFF_BASE + sign * CUTOFF_VARIATION * std::sin(theta);
            break;
        case RIGHT:
            f_c = CUTOFF_BASE + sign * CUTOFF_VARIATION * std::cos(theta);
            break;
    }
    
    return M_TAU * filterSampleTime * f_c;
}

/**
 The filter will use the equation

 $x_{n + 1} = (1 - 2\pi T_s f_c)x_n + 2\pi T_s f_c K u_n$

 where $u_n$ is the input and $x_n$ is the output at sample $n$, $K$ is the filter gain (default 1), and $T_s$ is the filter sample time (eg 1/96000 seconds, for a 96kHz system).

 */
float AuntLeslieAudioProcessor::filter(int hornIdx, int stereoChannel, float theta, float input) {
    if (! running[hornIdx][stereoChannel])
    {
        running[hornIdx][stereoChannel] = true;
        x_n_minus_one[hornIdx][stereoChannel] = input;
        return input;
    }
    
    float L = getFilterCoefficient(hornIdx, stereoChannel, theta);
    
    // TODO maybe vary filterGain as a function of f_c?
    return (1.f - L) * x_n_minus_one[hornIdx][stereoChannel] + L * filterGain * input;
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
