#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout GuitarSuiteProcessor::createLayout()
{
    APVTS::ParameterLayout l;
    auto pct = juce::NormalisableRange<float>(0.f, 100.f, 0.1f);
    // Defaults = the user's preset: Comp 100, Retro BP 2042/31, Garage Fuzz 70, Box 9, Mix 30
    l.add(std::make_unique<juce::AudioParameterBool> (juce::ParameterID{"comp_on", 1}, "Compressor", true));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"comp_amount", 1}, "Compressor Amount", pct, 100.f));
    l.add(std::make_unique<juce::AudioParameterBool> (juce::ParameterID{"filt_on", 1}, "Filter (Retro BP 2042 Hz / Res 31)", true));
    l.add(std::make_unique<juce::AudioParameterBool> (juce::ParameterID{"dist_on", 1}, "Distortion (Garage Fuzz)", true));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"dist_amount", 1}, "Distortion Amount", pct, 70.f));
    l.add(std::make_unique<juce::AudioParameterBool> (juce::ParameterID{"box_on", 1}, "Box Modeler (Model 9)", true));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix", pct, 30.f));
    return l;
}

GuitarSuiteProcessor::GuitarSuiteProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
    pCompOn = apvts.getRawParameterValue("comp_on");   pCompAmt = apvts.getRawParameterValue("comp_amount");
    pFiltOn = apvts.getRawParameterValue("filt_on");
    pDistOn = apvts.getRawParameterValue("dist_on");   pDistAmt = apvts.getRawParameterValue("dist_amount");
    pBoxOn  = apvts.getRawParameterValue("box_on");
    pMix    = apvts.getRawParameterValue("mix");
}

void GuitarSuiteProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& c : chains) c.prepare(sampleRate);
    setLatencySamples(gs::GuitarSuite::latencySamples);
}

bool GuitarSuiteProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet(); const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void GuitarSuiteProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    gs::Params p;
    p.compOn = *pCompOn > 0.5f; p.compAmount = *pCompAmt;
    p.filtOn = *pFiltOn > 0.5f;
    p.distOn = *pDistOn > 0.5f; p.distAmount = *pDistAmt;
    p.boxOn  = *pBoxOn  > 0.5f;
    p.mix    = *pMix;

    const int nCh = juce::jmin(buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int ch = 0; ch < nCh; ++ch)
    {
        chains[ch].setParams(p);
        float* d = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i) d[i] = chains[ch].process(d[i]);
    }
}

juce::AudioProcessorEditor* GuitarSuiteProcessor::createEditor() { return new GuitarSuiteEditor(*this); }

void GuitarSuiteProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, destData);
}
void GuitarSuiteProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GuitarSuiteProcessor(); }
