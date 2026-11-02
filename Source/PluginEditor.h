#pragma once
#include "PluginProcessor.h"
#include "Widgets.h"

class RogerThatEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RogerThatEditor(RogerThatProcessor&);
    ~RogerThatEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using APVTS = juce::AudioProcessorValueTreeState;
    RogerThatProcessor& proc;

    rt::ScopeView scope;
    rt::Knob compKnob, fuzzKnob, mixKnob;
    rt::UnderlineToggle compOn { "COMP" }, fuzzOn { "FUZZ" };
    rt::PillToggle filtOn { "FILTER" }, boxOn { "BOX" };
    juce::Label mixLabel;

    std::unique_ptr<APVTS::SliderAttachment> compAmtAtt, fuzzAmtAtt, mixAtt;
    std::unique_ptr<APVTS::ButtonAttachment> compOnAtt, fuzzOnAtt, filtOnAtt, boxOnAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RogerThatEditor)
};
