#include "PluginEditor.h"

using namespace rt;

RogerThatEditor::RogerThatEditor(RogerThatProcessor& p)
    : AudioProcessorEditor(&p), proc(p), scope(p.scope)
{
    addAndMakeVisible(scope);

    for (auto* k : { &compKnob, &fuzzKnob, &mixKnob }) addAndMakeVisible(k);
    for (auto* b : { (juce::Button*) &compOn, (juce::Button*) &fuzzOn, (juce::Button*) &filtOn, (juce::Button*) &boxOn }) addAndMakeVisible(b);

    mixLabel.setText("MIX", juce::dontSendNotification);
    mixLabel.setFont(Fonts::inter(11.f, 0.14f));
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, theme::muted);
    addAndMakeVisible(mixLabel);

    brand.onClick = [this] { about.open(); };
    addAndMakeVisible(brand);
    addChildComponent(about);   // hidden until the badge is clicked

    compAmtAtt = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "comp_amount", compKnob);
    fuzzAmtAtt = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "dist_amount", fuzzKnob);
    mixAtt     = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "mix",         mixKnob);
    compOnAtt  = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "comp_on", compOn);
    fuzzOnAtt  = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "dist_on", fuzzOn);
    filtOnAtt  = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "filt_on", filtOn);
    boxOnAtt   = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "box_on",  boxOn);

    setSize(620, 400);
    setResizable(false, false);
    startTimerHz(30);
}

void RogerThatEditor::paint(juce::Graphics& g)
{
    g.fillAll(theme::bg);
    drawPanelShadow(g, scope.getBounds().toFloat(), 0.f);
}

void RogerThatEditor::resized()
{
    scope.setBounds(0, 0, getWidth(), 212);
    brand.setBounds(getWidth() - 16 - BrandButton::widthPx, getHeight() - 10 - BrandButton::heightPx, BrandButton::widthPx, BrandButton::heightPx);
    about.setBounds(getLocalBounds());

    // controls row, in signal order: COMP · FILTER · FUZZ · BOX · MIX
    const int knobY = 242, labelY = 344, labelH = 26;
    const int kw = Knob::sizePx;
    const int xComp = 40, xFuzz = 236, xMix = 432;

    compKnob.setBounds(xComp, knobY, kw, kw);
    fuzzKnob.setBounds(xFuzz, knobY, kw, kw);
    mixKnob .setBounds(xMix,  knobY, kw, kw);

    compOn.setBounds(xComp + 10, labelY, kw - 20, labelH);
    fuzzOn.setBounds(xFuzz + 10, labelY, kw - 20, labelH);
    mixLabel.setBounds(xMix + 10, labelY, kw - 20, labelH);

    const int knobCY = knobY + kw / 2;
    const int pw = PillToggle::widthPx, ph = PillToggle::heightPx;
    filtOn.setBounds((xComp + kw + xFuzz) / 2 - pw / 2, knobCY - ph / 2, pw, ph);
    boxOn .setBounds((xFuzz + kw + xMix) / 2 - pw / 2, knobCY - ph / 2, pw, ph);
}

void RogerThatEditor::timerCallback()
{
    scope.repaint();
    // a knob whose block is bypassed is dimmed
    const float compA = compOn.getToggleState() ? 1.f : 0.45f;
    const float fuzzA = fuzzOn.getToggleState() ? 1.f : 0.45f;
    if (compKnob.getAlpha() != compA) compKnob.setAlpha(compA);
    if (fuzzKnob.getAlpha() != fuzzA) fuzzKnob.setAlpha(fuzzA);
}
