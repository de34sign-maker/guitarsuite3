#pragma once
#include "PluginProcessor.h"
#include "Widgets.h"

class RogerThatEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int baseW = 620, baseH = 400;     // design size (all layout is done in these units)
    static constexpr float uiScale = 1.2f;             // the whole UI is drawn 20 % larger than the design

    explicit RogerThatEditor(RogerThatProcessor&);
    ~RogerThatEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // everything lives on this fixed 620x400 surface; the surface is scaled as a whole
    struct Surface : juce::Component
    {
        void paint(juce::Graphics&) override;
        juce::Rectangle<float> panelShadowRect;
    };

    using APVTS = juce::AudioProcessorValueTreeState;
    RogerThatProcessor& proc;

    Surface surface;
    rt::ScopeView scope;
    rt::Knob compKnob, fuzzKnob, mixKnob;
    rt::UnderlineToggle compOn { "COMP" }, fuzzOn { "FUZZ" };
    rt::PillToggle filtOn { "FILTER" }, boxOn { "BOX" };
    juce::Label mixLabel;
    rt::BrandButton brand;
    rt::AboutPanel about;

    std::unique_ptr<APVTS::SliderAttachment> compAmtAtt, fuzzAmtAtt, mixAtt;
    std::unique_ptr<APVTS::ButtonAttachment> compOnAtt, fuzzOnAtt, filtOnAtt, boxOnAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RogerThatEditor)
};
