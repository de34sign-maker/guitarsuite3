#pragma once
#include "PluginProcessor.h"
#include "Widgets.h"

class RogerThatEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int baseW = 620, baseH = 400;     // design size; the UI is scaled from this

    explicit RogerThatEditor(RogerThatProcessor&);
    ~RogerThatEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

    void setUiScale(float s);                          // 1.0 / 1.25 / 1.5 / 2.0, stored in the plugin state
    float getUiScale() const { return uiScale; }

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
    float uiScale = 1.f;

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
