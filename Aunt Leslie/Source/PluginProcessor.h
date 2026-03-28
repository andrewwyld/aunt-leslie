/*
  ==============================================================================

   A U N T   L E S L I E
   a fully electronic Leslie speaker simulation
   (c) 2026 Andrew Wyld

   processor header

 ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#define LEFT 0
#define RIGHT 1
#define TREBLE_HORN_A 0
#define TREBLE_HORN_B 1

#define CHORALE_RPM 50.f
#define TREMOLO_RPM 400.f
#define TREBLE_HORN_RADIUS_M 0.204f
// #define BASS_WEDGE_RADIUS 0.204f ?
#define SPEED_OF_SOUND_M_S 343.f

//==============================================================================
/**
*/
class AuntLeslieAudioProcessor  : public juce::AudioProcessor
{
public:

    constexpr static float CHORALE_HZ = CHORALE_RPM / 60.f;
    constexpr static float CHORALE_PERIOD_MS = 1000.f / CHORALE_HZ;

    constexpr static float TREMOLO_HZ = TREMOLO_RPM / 60.f;
    constexpr static float TREMOLO_PERIOD_MS = 1000.f / TREMOLO_HZ;
    
    constexpr static float MAX_TREBLE_HORN_EXCURSION_S = SPEED_OF_SOUND_M_S / TREBLE_HORN_RADIUS_M;
    constexpr static float INTRINSIC_BUFFER_DELAY_S = MAX_TREBLE_HORN_EXCURSION_S * 3.f;
    
    constexpr static float MIC_DISTANCE = 2.f; // as a factor of the horn radius

    //==============================================================================
    AuntLeslieAudioProcessor();
    ~AuntLeslieAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuntLeslieAudioProcessor)
    
    float max_treble_horn_excursion_samples = 0.f;
    // float max_bass_wedge_excursion_samples = 0.f
    
    float chorale_frequency_per_sample = 0.f;
    float tremolo_frequency_per_sample = 0.f;

    int channelsIn = 0;
    int delayLineLength = 0; // must be initialized to INTRINSIC_BUFFER_DELAY * sample rate

    float **delayLines = NULL; // must be initialized to new float[delayLineLength]
    int delayLineRecordHeadPosition = 0;
    
    void initializeLines(int channelsIn, int delayLineLength);
    float getReadHead(int hornIdx, int stereoChannel, int writeHead);

    long lastSample = 0; // current time
};
