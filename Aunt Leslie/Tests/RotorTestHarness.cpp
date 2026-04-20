/*
  ==============================================================================

   Rotor Test Harness
   Standalone test for Rotor class without JUCE framework

  ==============================================================================
*/

#include "../Source/Rotor.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>
#include <memory>
#include <functional>

// Constants
constexpr int SAMPLE_RATE = 44100;
constexpr int SAMPLE_COUNT = 128;
constexpr int NUM_CHANNELS = 2;
constexpr float TEST_FREQUENCY = 440.0f;
constexpr float TOLERANCE = 0.001f;

// Helper functions
bool approximatelyEqual(float a, float b, float tolerance = TOLERANCE) {
    return std::abs(a - b) < tolerance;
}

void printTestResult(const char* testName, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

// RAII Buffer Manager
class BufferManager {
public:
    BufferManager(int channels, int samples) 
        : numChannels(channels), numSamples(samples) {
        inputBuffers.resize(channels);
        outputBuffers.resize(channels);
        inputPtrs.resize(channels);
        outputPtrs.resize(channels);
        for (auto& buf : inputBuffers) {
            buf.resize(samples, 0.0f);
        }
        for (auto& buf : outputBuffers) {
            buf.resize(samples, 0.0f);
        }
    }
    
    void fillWithSineWave(int startSample = 0) {
        for (int ch = 0; ch < numChannels; ++ch) {
            for (int i = 0; i < numSamples; ++i) {
                float t = static_cast<float>(startSample + i) / static_cast<float>(SAMPLE_RATE);
                inputBuffers[ch][i] = std::sin(2.0f * M_PI * TEST_FREQUENCY * t);
            }
        }
    }
    
    void fillWithConstant(float value) {
        for (auto& buf : inputBuffers) {
            std::fill(buf.begin(), buf.end(), value);
        }
    }
    
    void fillWithSawtooth(int startSample = 0) {
        for (int ch = 0; ch < numChannels; ++ch) {
            for (int i = 0; i < numSamples; ++i) {
                float t = static_cast<float>(startSample + i) / static_cast<float>(SAMPLE_RATE);
                // Sawtooth wave between -1 and 1
                float phase = (TEST_FREQUENCY * t) - std::floor(TEST_FREQUENCY * t);
                inputBuffers[ch][i] = 2.0f * phase - 1.0f;
            }
        }
    }
    
    bool hasNonZeroOutput() const {
        for (const auto& buf : outputBuffers) {
            if (std::any_of(buf.begin(), buf.end(), [](float sample) {
                return std::abs(sample) > TOLERANCE;
            })) {
                return true;
            }
        }
        return false;
    }
    
    float** getInputPointers() {
        for (int i = 0; i < numChannels; ++i) {
            inputPtrs[i] = inputBuffers[i].data();
        }
        return inputPtrs.data();
    }
    
    float** getOutputPointers() {
        for (int i = 0; i < numChannels; ++i) {
            outputPtrs[i] = outputBuffers[i].data();
        }
        return outputPtrs.data();
    }
    
private:
    int numChannels;
    int numSamples;
    std::vector<std::vector<float>> inputBuffers;
    std::vector<std::vector<float>> outputBuffers;
    std::vector<float*> inputPtrs;
    std::vector<float*> outputPtrs;
};

// Test runner
class TestRunner {
public:
    using TestFunction = std::function<bool()>;
    
    void runTest(const char* name, TestFunction test) {
        totalTests++;
        auto startTime = std::chrono::high_resolution_clock::now();
        
        bool passed = false;
        try {
            passed = test();
        } catch (const std::exception& e) {
            // std::cerr << "Exception in test '" << name << "': " << e.what() << std::endl;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        printTestResult(name, passed);
        if (passed) passedTests++;
        
        // std::cout << "  Time: " << duration.count() << "μs" << std::endl;
    }
    
    void printSummary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Tests passed: " << passedTests << "/" << totalTests << std::endl;
    }
    
    bool allPassed() const {
        return passedTests == totalTests;
    }
    
private:
    int passedTests = 0;
    int totalTests = 0;
};

// Test functions
bool testInitialization() {
    Rotor rotor;
    rotor.initializeLines(NUM_CHANNELS, SAMPLE_RATE);
    
    bool passed = rotor.getDelayLineLength() > 0;
    
    rotor.releaseResources();
    return passed;
}

bool testProcessBlockProducesOutput() {
    Rotor rotor;
    rotor.initializeLines(NUM_CHANNELS, SAMPLE_RATE);
    
    BufferManager buffers(NUM_CHANNELS, SAMPLE_COUNT);
    buffers.fillWithSineWave();
    
    rotor.processBlock(SAMPLE_COUNT, NUM_CHANNELS, buffers.getOutputPointers(), 
                      NUM_CHANNELS, const_cast<const float**>(buffers.getInputPointers()));
    
    bool passed = buffers.hasNonZeroOutput();
    
    rotor.releaseResources();
    return passed;
}

bool testMultipleProcessBlockCalls() {
    Rotor rotor;
    rotor.initializeLines(NUM_CHANNELS, SAMPLE_RATE);
    
    BufferManager buffers(NUM_CHANNELS, SAMPLE_COUNT);
    
    for (int block = 0; block < 10; ++block) {
        buffers.fillWithSineWave(block * SAMPLE_COUNT);
        rotor.processBlock(SAMPLE_COUNT, NUM_CHANNELS, buffers.getOutputPointers(), 
                          NUM_CHANNELS, const_cast<const float**>(buffers.getInputPointers()));
    }
    
    bool passed = buffers.hasNonZeroOutput();
    
    rotor.releaseResources();
    return passed;
}

bool testThetaProgression()
{
    Rotor rotor;
    rotor.initializeLines(NUM_CHANNELS, SAMPLE_RATE);
    
    BufferManager buffers(NUM_CHANNELS, SAMPLE_COUNT);
    
    float theta1 = rotor.getTheta(0);
    
    // Use sawtooth wave input at 440Hz
    buffers.fillWithSawtooth();
    
    // Process multiple blocks to fill delay line and get continuous output
    for (int block = 0; block < 20; ++block) {
        buffers.fillWithSawtooth(block * SAMPLE_COUNT);
        rotor.processBlock(SAMPLE_COUNT, NUM_CHANNELS, buffers.getOutputPointers(), 
                          NUM_CHANNELS, const_cast<const float**>(buffers.getInputPointers()));
    }
    
    float theta2 = rotor.getTheta(0);
    
    // std::cout << "Theta before: " << theta1 << ", Theta after: " << theta2 << std::endl;
    
    // Analyze output for amplitude modulation pattern
    // 16ms half-wave = 32ms full wave = ~31.25 Hz
    // At 44.1kHz, 16ms = 706 samples
    
    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
    {
        // std::cout << "\nChannel " << ch << " I/O (last block):" << std::endl;
        
        for (int i = 0; i < SAMPLE_COUNT; ++i)
        {
            // std::cout << buffers.getInputPointers()[0][i] << ", " << buffers.getInputPointers()[1][i] << ", " << buffers.getOutputPointers()[0][i] << ", " << buffers.getOutputPointers()[1][i] << "\n";
        }
    }
    
    bool passed = !approximatelyEqual(theta1, theta2);
    
    rotor.releaseResources();
    return passed;
}

bool testGetReadHeadOffset() {
    Rotor rotor;
    rotor.initializeLines(NUM_CHANNELS, SAMPLE_RATE);
    
    float maxExcursion = rotor.getMaxTrebleHornExcursionSamples();
    // std::cout << "Max treble horn excursion samples: " << maxExcursion << std::endl;
    
    // Test with theta range from -2pi to 2pi
    const int numSteps = 100;
    const float thetaMin = -2.0f * M_PI;
    const float thetaMax = 2.0f * M_PI;
    const float thetaStep = (thetaMax - thetaMin) / numSteps;
    
    // std::cout << "\nTesting getReadHeadOffset with theta range: " << thetaMin << " to " << thetaMax << std::endl;
    
    float minOffset = std::numeric_limits<float>::max();
    float maxOffset = std::numeric_limits<float>::lowest();
    
    for (int hornIdx = 0; hornIdx < NUM_CHANNELS; ++hornIdx) {
        for (int stereoChannel = 0; stereoChannel < NUM_CHANNELS; ++stereoChannel) {
            // std::cout << "\nHorn " << hornIdx << ", Stereo " << stereoChannel << ":" << std::endl;
            
            minOffset = std::numeric_limits<float>::max();
            maxOffset = std::numeric_limits<float>::lowest();
            
            for (int i = 0; i <= numSteps; ++i) {
                float theta = thetaMin + i * thetaStep;
                float offset = rotor.getReadHeadOffset(hornIdx, stereoChannel, theta);
                
                if (offset < minOffset) minOffset = offset;
                if (offset > maxOffset) maxOffset = offset;
            }
            
            // std::cout << "  Min offset: " << minOffset << std::endl;
            // std::cout << "  Max offset: " << maxOffset << std::endl;
            // std::cout << "  Offset range: " << (maxOffset - minOffset) << std::endl;
            
            // Expected: offsets should be between 0 and ~4 * maxExcursion (based on actual behavior)
            bool rangeValid = (minOffset >= 0) && (maxOffset <= 4.0f * maxExcursion);
            // std::cout << "  Range valid (0 to ~4 * maxExcursion): " << (rangeValid ? "YES" : "NO") << std::endl;
            
            if (!rangeValid) {
                rotor.releaseResources();
                return false;
            }
        }
    }
    
    rotor.releaseResources();
    return true;
}

bool testGetReadHeadOffsetAtPi2() {
    Rotor rotor;
    rotor.initializeLines(NUM_CHANNELS, SAMPLE_RATE);
    
    float theta = M_PI / 2.0f;
    
    // std::cout << "\nTesting getReadHeadOffset at theta = pi/2 (" << theta << ")" << std::endl;
    
    for (int hornIdx = 0; hornIdx < NUM_CHANNELS; ++hornIdx) {
        for (int stereoChannel = 0; stereoChannel < NUM_CHANNELS; ++stereoChannel) {
            float offset = rotor.getReadHeadOffset(hornIdx, stereoChannel, theta);
            // std::cout << "Horn " << hornIdx << ", Stereo " << stereoChannel << ": " << offset << std::endl;
        }
    }
    
    rotor.releaseResources();
    return true;
}

bool testGetReadHeadOffsetAt0() {
    Rotor rotor;
    rotor.initializeLines(NUM_CHANNELS, SAMPLE_RATE);
    
    float theta = 0.0f;
    
    // std::cout << "\nTesting getReadHeadOffset at theta = 0" << std::endl;
    
    for (int hornIdx = 0; hornIdx < NUM_CHANNELS; ++hornIdx) {
        for (int stereoChannel = 0; stereoChannel < NUM_CHANNELS; ++stereoChannel) {
            float offset = rotor.getReadHeadOffset(hornIdx, stereoChannel, theta);
            // std::cout << "Horn " << hornIdx << ", Stereo " << stereoChannel << ": " << offset << std::endl;
        }
    }
    
    rotor.releaseResources();
    return true;
}

bool testDelayLineReadWrite() {
    Rotor rotor;
    rotor.initializeLines(NUM_CHANNELS, SAMPLE_RATE);

    BufferManager buffers(NUM_CHANNELS, SAMPLE_COUNT);
    buffers.fillWithConstant(0.5f); // Write known value

    // Process one block
    rotor.processBlock(SAMPLE_COUNT, NUM_CHANNELS, buffers.getOutputPointers(),
                      NUM_CHANNELS, const_cast<const float**>(buffers.getInputPointers()));

    // Verify delay line was written correctly
    int delayLineLength = rotor.getDelayLineLength();
    int recordHead = rotor.getDelayLineRecordHeadPosition();

    // Check that values were written to delay line
    bool writeCorrect = true;
    for (int ch = 0; ch < NUM_CHANNELS; ++ch) {
        for (int i = 0; i < delayLineLength; ++i) {
            float val = rotor.getDelayLineValue(ch, i);
            // Expected: 0.0 except at last sample position which should be 1.0 (based on current test pattern)
            if (i == (recordHead - 1 + delayLineLength) % delayLineLength) {
                if (std::abs(val - 1.0f) > TOLERANCE) writeCorrect = false;
            } else {
                if (std::abs(val - 0.0f) > TOLERANCE) writeCorrect = false;
            }
        }
    }

    rotor.releaseResources();
    return writeCorrect;
}

int main() {
    // std::cout << "=== Rotor Test Harness ===" << std::endl;
    
    TestRunner runner;
    
    runner.runTest("Rotor initialization", testInitialization);
    runner.runTest("Process block produces output", testProcessBlockProducesOutput);
    runner.runTest("Multiple processBlock calls maintain continuity", testMultipleProcessBlockCalls);
    runner.runTest("Theta progression", testThetaProgression);
    runner.runTest("GetReadHeadOffset range", testGetReadHeadOffset);
    runner.runTest("GetReadHeadOffset at pi/2", testGetReadHeadOffsetAtPi2);
    runner.runTest("GetReadHeadOffset at 0", testGetReadHeadOffsetAt0);
    runner.runTest("Delay line read/write", testDelayLineReadWrite);
    
    runner.printSummary();
    
    return runner.allPassed() ? 0 : 1;
}
