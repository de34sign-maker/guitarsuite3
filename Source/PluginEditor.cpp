#include "PluginEditor.h"

using namespace rt;

static const juce::Identifier kUiScale("ui_scale");

RogerThatEditor::RogerThatEditor(RogerThatProcessor& p)
    : AudioProcessorEditor(&p), proc(p), scope(p.scope)
{
    addAndMakeVisible(surface);

    surface.addAndMakeVisible(scope);
    for (auto* k : { &compKnob, &fuzzKnob, &mixKnob }) surface.addAndMakeVisible(k);
    for (auto* b : { (juce::Button*) &compOn, (juce::Button*) &fuzzOn, (juce::Button*) &filtOn, (juce::Button*) &boxOn }) surface.addAndMakeVisible(b);

    mixLabel.setText("MIX", juce::dontSendNotification);
    mixLabel.setFont(Fonts::inter(11.f, 0.14f));
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, theme::muted);
    surface.addAndMakeVisible(mixLabel);

    brand.onClick = [this] { about.open(); };
    surface.addAndMakeVisible(brand);
    surface.addChildComponent(about);   // hidden until the badge is clicked
    about.onScaleChosen = [this](float s) { setUiScale(s); };

    compAmtAtt = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "comp_amount", compKnob);
    fuzzAmtAtt = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "dist_amount", fuzzKnob);
    mixAtt     = std::make_unique<APVTS::SliderAttachment>(proc.apvts, "mix",         mixKnob);
    compOnAtt  = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "comp_on", compOn);
    fuzzOnAtt  = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "dist_on", fuzzOn);
    filtOnAtt  = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "filt_on", filtOn);
    boxOnAtt   = std::make_unique<APVTS::ButtonAttachment>(proc.apvts, "box_on",  boxOn);

    // fixed design layout on the surface
    surface.setBounds(0, 0, baseW, baseH);
    scope.setBounds(0, 0, baseW, 212);
    surface.panelShadowRect = scope.getBounds().toFloat();

    // controls row, centred: COMP · FILTER · FUZZ · BOX · MIX (discs and pills are 72 wide, 40 between)
    const int knobY = 242, labelY = 344, labelH = 26;
    const int kw = Knob::sizePx;
    const int xComp = 36, xFuzz = 260, xMix = 484;
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

    // "↗ vst34" in the top-right corner of the display, opposite the IN / FX legend
    brand.setBounds(baseW - 16 - BrandButton::widthPx, 8, BrandButton::widthPx, BrandButton::heightPx);
    about.setBounds(0, 0, baseW, baseH);

    setResizable(false, false);
    const float saved = (float) (double) proc.apvts.state.getProperty(kUiScale, 1.0);
    uiScale = juce::jlimit(1.f, 2.f, saved);
    about.setScale(uiScale);
    setSize(juce::roundToInt(baseW * uiScale), juce::roundToInt(baseH * uiScale));
    startTimerHz(30);
}

void RogerThatEditor::setUiScale(float s)
{
    s = juce::jlimit(1.f, 2.f, s);
    if (std::abs(s - uiScale) < 0.001f) return;
    uiScale = s;
    proc.apvts.state.setProperty(kUiScale, (double) s, nullptr);   // saved with the project / preset
    about.setScale(s);
    setSize(juce::roundToInt(baseW * s), juce::roundToInt(baseH * s));
}

void RogerThatEditor::paint(juce::Graphics& g)
{
    g.fillAll(theme::bg);
}

void RogerThatEditor::Surface::paint(juce::Graphics& g)
{
    g.fillAll(theme::bg);
    drawPanelShadow(g, panelShadowRect, 0.f);
}

void RogerThatEditor::resized()
{
    // the surface keeps its design size; the whole thing is scaled to fill the editor
    const float s = (float) getWidth() / (float) baseW;
    surface.setTransform(juce::AffineTransform::scale(s));
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
