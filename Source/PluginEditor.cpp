#include "PluginEditor.h"

void GuitarSuiteEditor::setupRow(Row& r, const juce::String& title, const juce::String& onId,
                                 const juce::String& amtId, const juce::String& fixedText)
{
    r.title.setText(title, juce::dontSendNotification);
    r.title.setFont(juce::Font(juce::FontOptions(15.f, juce::Font::bold)));
    addAndMakeVisible(r.title);

    r.on.setButtonText("On");
    addAndMakeVisible(r.on);
    r.onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, onId, r.on);

    if (amtId.isNotEmpty())
    {
        r.amount.setSliderStyle(juce::Slider::LinearHorizontal);
        r.amount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
        r.amount.setTextValueSuffix(" %");
        addAndMakeVisible(r.amount);
        r.amtAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, amtId, r.amount);
    }
    else
    {
        r.fixed.setText(fixedText, juce::dontSendNotification);
        r.fixed.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(r.fixed);
    }
}

GuitarSuiteEditor::GuitarSuiteEditor(GuitarSuiteProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    setupRow(comp, "Compressor", "comp_on", "comp_amount", "");
    setupRow(filt, "Filter", "filt_on", "", "Retro  /  Bandpass  /  2042 Hz  /  Resonance 34 %");
    setupRow(dist, "Distortion", "dist_on", "dist_amount", "");
    setupRow(box,  "Box Modeler", "box_on", "", "Model 9");

    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setFont(juce::Font(juce::FontOptions(15.f, juce::Font::bold)));
    addAndMakeVisible(mixLabel);
    mix.setSliderStyle(juce::Slider::LinearHorizontal);
    mix.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
    mix.setTextValueSuffix(" %");
    addAndMakeVisible(mix);
    mixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, "mix", mix);

    srWarn.setJustificationType(juce::Justification::centred);
    srWarn.setColour(juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible(srWarn);

    setSize(460, 400);
    startTimerHz(2);
}

void GuitarSuiteEditor::timerCallback()
{
    const double sr = proc.currentSampleRate;
    srWarn.setText(std::abs(sr - kDataSampleRate) < 1.0 ? juce::String()
                   : "Measured at 44.1 kHz, recalculated for " + juce::String(sr / 1000.0, 1) + " kHz session",
                   juce::dontSendNotification);
}

void GuitarSuiteEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2b2b2b));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(18.f, juce::Font::bold)));
    g.drawText(juce::String::fromUTF8("Guitar Suite 3.0 \xe2\x80\x94 Audition clone"), 0, 8, getWidth(), 24, juce::Justification::centred);
    g.setColour(juce::Colour(0xff3c3c3c));
    for (int i = 0; i < 5; ++i) g.fillRoundedRectangle(10.f, 44.f + i * 64.f, (float)getWidth() - 20.f, 58.f, 6.f);
}

void GuitarSuiteEditor::resized()
{
    auto place = [this](Row& r, int y) {
        r.title.setBounds(20, y, 140, 24);
        r.on.setBounds(20, y + 26, 60, 24);
        if (r.amtAtt) r.amount.setBounds(90, y + 26, getWidth() - 110, 24);
        else          r.fixed.setBounds(90, y + 26, getWidth() - 110, 24);
    };
    place(comp, 48); place(filt, 112); place(dist, 176); place(box, 240);
    mixLabel.setBounds(20, 304, 140, 24);
    mix.setBounds(90, 330, getWidth() - 110, 24);
    srWarn.setBounds(10, 368, getWidth() - 20, 24);
}
