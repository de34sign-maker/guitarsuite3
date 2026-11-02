#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../GuitarSuiteDSP.h"

class GuitarSuiteProcessor : public juce::AudioProcessor
{
public:
    GuitarSuiteProcessor();
    ~GuitarSuiteProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GuitarSuite3"; }
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    gs::GuitarSuite chains[2];

    std::atomic<float>* pCompOn;  std::atomic<float>* pCompAmt;
    std::atomic<float>* pFiltOn;
    std::atomic<float>* pDistOn;  std::atomic<float>* pDistAmt;
    std::atomic<float>* pBoxOn;
    std::atomic<float>* pMix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarSuiteProcessor)
};
