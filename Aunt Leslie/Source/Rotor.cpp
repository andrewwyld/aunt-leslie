/*
  ==============================================================================

    Rotor.cpp
    Created: 14 Apr 2026 2:39:01pm
    Author:  Andrew Wyld

  ==============================================================================
*/

#include "Rotor.h"
#include <iostream>
#include <cassert>

void Rotor::releaseResources()
{
    if (delayLines) {
        for (int i = 0; i < channelsIn; ++i) {
            if (delayLines[i]) {
                delete [] delayLines[i];
            }
        }
        delete [] delayLines;
        delayLines = nullptr;
    }
    running[TREBLE_HORN_A][LEFT] = false;
    running[TREBLE_HORN_A][RIGHT] = false;
    running[TREBLE_HORN_B][LEFT] = false;
    running[TREBLE_HORN_B][RIGHT] = false;
}

void Rotor::initializeLines(int channelsIn_, double sampleRate) {
    channelsIn = channelsIn_;
    delayLineLength = sampleRate * INTRINSIC_BUFFER_DELAY_S;
    
    delayLines = new float*[channelsIn];

    max_treble_horn_excursion_samples = std::ceil(sampleRate * MAX_TREBLE_HORN_EXCURSION_S);
    chorale_frequency_per_sample = CHORALE_HZ / sampleRate;
    tremolo_frequency_per_sample = TREMOLO_HZ / sampleRate;
    
    filterSampleTime = 1.f / sampleRate;
    
    for (int i = 0; i < channelsIn; ++ i) {
        delayLines[i] = new float[delayLineLength];
        std::memset(delayLines[i], 0, delayLineLength * sizeof(float));
    }
}

void Rotor::processBlock(int sampleCount, int totalNumOutputChannels, float **outputs, int totalNumInputChannels, const float **inputs)
{
    // delay line recording boundaries for all channels
    int delayLineRecordLength = std::fmin(sampleCount, delayLineLength);
    int delayLineInputStart = std::fmax(0, sampleCount - delayLineLength);
    
    // process variable delay
    
    // for either stereo output channel
    for (int stereoChannel = 0; stereoChannel < totalNumOutputChannels; ++stereoChannel)
    {

        // for every write position in the output arrays
        for (int writeHead = 0; writeHead < sampleCount; ++writeHead)
        {
            
            float sampleSum = 0.f;
            float _theta = theta(writeHead);

            // and for both treble horns
            for (int hornIdx = 0; hornIdx < totalNumInputChannels; ++hornIdx)
            {
                float readHeadOffset = getReadHeadOffset(hornIdx, stereoChannel, _theta);
                
                // std::cout << (hornIdx == 0 ? "A/": "B/") << (stereoChannel == 0 ? "L, ": "R, ") << _theta << ", " << readHeadOffset << "\n";
                
                // get the appropriate read head function value
                float readHead = (float) writeHead - readHeadOffset;
                
                int readHeadLo = std::floor(readHead);
                int readHeadHi = std::ceil(readHead);
                float proportion = readHead - readHeadLo;

                int delayLineReadHeadLo = (delayLineRecordHeadPosition + delayLineLength + readHeadLo) % delayLineLength;
                int delayLineReadHeadHi = (delayLineRecordHeadPosition + delayLineLength + readHeadHi) % delayLineLength;

                assert(proportion >= 0.0f);
                assert(proportion < 1.0f);
                
                float sampleLo = readHeadLo < 0 ? delayLines[hornIdx][delayLineReadHeadLo] : inputs[hornIdx][readHeadLo];
                float sampleHi = readHeadHi < 0 ? delayLines[hornIdx][delayLineReadHeadHi] : inputs[hornIdx][readHeadHi];
                
                // linear interpolation's good enough for you, right? me too
                float filterAnd = (1.f - proportion) * sampleLo + proportion * sampleHi;
                

                sampleSum += filterAnd; // filter(hornIdx, stereoChannel, _theta, filterAnd);
            }

            outputs[stereoChannel][writeHead] = sampleSum / 2.f;

            /*
            std::cout
            << (stereoChannel == 0 ? "L, ": "R, ")
            << writeHead << ", "
            << _theta << ", "
            << inputs[TREBLE_HORN_A][writeHead] << ", "
            << inputs[TREBLE_HORN_B][writeHead] << ", "
            << sampleSum << "\n";
             */
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
                 delayLines[inputIdx][i] = (readHead == sampleCount - 1 ? 1.f : 0.f); // inputs[inputIdx][readHead];
                 
                 // std::cout << "DL Write: ch=" << inputIdx << ", pos=" << i << ", val=" << inputs[inputIdx][readHead] << "\n";
        }
    }
    
    delayLineRecordHeadPosition += delayLineRecordLength;
    delayLineRecordHeadPosition %= delayLineLength;
    // std::cout << "delayLineRecordHeadPosition " << delayLineRecordHeadPosition << "\n";

    lastSample += sampleCount;
    // std::cout << "lastSample " << lastSample << "\n";
}

float Rotor::theta(int idx)
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
float Rotor::getReadHeadOffset(int hornIdx, int stereoChannel, float theta) const
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
float Rotor::getFilterCoefficient(int hornIdx, int stereoChannel, float theta)
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
float Rotor::filter(int hornIdx, int stereoChannel, float theta, float input) {
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
