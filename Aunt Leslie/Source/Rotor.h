/*
  ==============================================================================

    Rotor.h
    Created: 14 Apr 2026 2:39:01pm
    Author:  Andrew Wyld

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <cstring>

#define LEFT 0
#define RIGHT 1
#define TREBLE_HORN_A 0
#define TREBLE_HORN_B 1

#define CHORALE_RPM 50.f
#define TREMOLO_RPM 400.f
#define TREBLE_HORN_RADIUS_M 0.204f
// #define BASS_WEDGE_RADIUS 0.204f ?
#define SPEED_OF_SOUND_M_S 343.f

#define CUTOFF_BASE 12000.f
#define CUTOFF_VARIATION 6000.f

const float M_TAU = 2.f * M_PI;

class Rotor
{
public:

    constexpr static float CHORALE_HZ = CHORALE_RPM / 60.f;
    constexpr static float CHORALE_PERIOD_MS = 1000.f / CHORALE_HZ;

    constexpr static float TREMOLO_HZ = TREMOLO_RPM / 60.f;
    constexpr static float TREMOLO_PERIOD_MS = 1000.f / TREMOLO_HZ;
    
    constexpr static float MAX_TREBLE_HORN_EXCURSION_S = TREBLE_HORN_RADIUS_M / SPEED_OF_SOUND_M_S;
    // max delay should be about 4.162 times longer than the excursion
    constexpr static float INTRINSIC_BUFFER_DELAY_S = MAX_TREBLE_HORN_EXCURSION_S * 5.f;
    
    constexpr static float MIC_DISTANCE = 2.f; // as a factor of the horn radius

    void initializeLines(int channelsIn, double sampleRate);
    void releaseResources();
    void processBlock(int sampleCount, int outChannels, float** outputs, int inChannels, const float** inputs);

private:
    float max_treble_horn_excursion_samples = 0.f;
    // float max_bass_wedge_excursion_samples = 0.f
    
    float chorale_frequency_per_sample = 0.f;
    float tremolo_frequency_per_sample = 0.f;
    
    float filterSampleTime = 0.f;
    float filterGain = 1.f;

    int channelsIn = 0;
    int delayLineLength = 0; // must be initialized to INTRINSIC_BUFFER_DELAY * sample rate

    float **delayLines = nullptr; // must be initialized to new float[delayLineLength]
    int delayLineRecordHeadPosition = 0;
    
    float theta(int idx);
    
    float getReadHeadOffset(int hornIdx, int stereoChannel, float theta, int writeHead);
    float getFilterCoefficient(int hornIdx, int stereoChannel, float theta);
    float filter(int hornIdx, int stereoChannel, float theta, float input);

    long lastSample = 0; // current time
    
    bool running[2][2] = {{false, false}, {false, false}};

    float x_n_minus_one[2][2] = {{0.f, 0.f}, {0.f, 0.f}};
    
};
