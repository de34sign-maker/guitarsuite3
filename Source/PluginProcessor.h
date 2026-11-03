#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "../GuitarSuiteDSP.h"

// Lock-free history of peak levels for the waveform display: one column every ~4 ms.
// Audio thread writes, UI thread reads; no locks.
struct ScopeHistory
{
    static constexpr int size = 2048;              // columns kept (power of two)
    static constexpr double columnSeconds = 0.004; // time per column

    std::array<std::atomic<float>, size> in {};    // peak |input| in the column
    std::array<std::atomic<float>, size> fx {};    // peak |wet path x mix| in the column
    std::atomic<int> writeIndex { 0 };             // next column to write

    void push(float inPeak, float fxPeak) noexcept
    {
        const int w = writeIndex.load(std::memory_order_relaxed);
        in[(size_t) w].store(inPeak, std::memory_order_relaxed);
        fx[(size_t) w].store(fxPeak, std::memory_order_relaxed);
        writeIndex.store((w + 1) & (size - 1), std::memory_order_release);
    }
};

class RogerThatProcessor : public juce::AudioProcessor
{
public:
    RogerThatProcessor();
    ~RogerThatProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RogerThat"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "User preset"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    double currentSampleRate = 44100.0;
    ScopeHistory scope;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    gs::GuitarSuite chains[2];

    std::atomic<float>* pCompOn;  std::atomic<float>* pCompAmt;
    std::atomic<float>* pFiltOn;
    std::atomic<float>* pDistOn;  std::atomic<float>* pDistAmt;
    std::atomic<float>* pBoxOn;
    std::atomic<float>* pMix;

    // scope accumulation (audio thread only)
    int   scopeSamplesPerColumn = 176;
    int   scopeCount = 0;
    float scopeInPeak = 0.f, scopeFxPeak = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RogerThatProcessor)
};
