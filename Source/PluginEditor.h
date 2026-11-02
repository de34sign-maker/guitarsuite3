#pragma once
#include "PluginProcessor.h"

class GuitarSuiteEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GuitarSuiteEditor(GuitarSuiteProcessor&);
    ~GuitarSuiteEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Row {
        juce::ToggleButton on; juce::Slider amount; juce::Label title, fixed;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amtAtt;
    };
    void setupRow(Row&, const juce::String& title, const juce::String& onId, const juce::String& amtId, const juce::String& fixedText);

    GuitarSuiteProcessor& proc;
    Row comp, filt, dist, box;
    juce::Slider mix; juce::Label mixLabel, srWarn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarSuiteEditor)
};
