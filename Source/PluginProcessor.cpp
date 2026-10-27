#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace {
juce::AudioProcessorValueTreeState::ParameterLayout layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"pitch",1}, "Pitch", juce::NormalisableRange<float>{-12,12,0.1f}, 0,
        juce::AudioParameterFloatAttributes{}.withLabel("st")));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"formant",1}, "Formant", juce::NormalisableRange<float>{-12,12,0.1f}, 0,
        juce::AudioParameterFloatAttributes{}.withLabel("st")));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix",1}, "Mix", juce::NormalisableRange<float>{0,100,0.1f}, 100,
        juce::AudioParameterFloatAttributes{}.withLabel("%")));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1}, "Bypass", false));
    return l;
}
}
BabyVoiceProcessor::BabyVoiceProcessor()
    : AudioProcessor(BusesProperties().withInput("Vocal In",juce::AudioChannelSet::stereo(),true)
        .withOutput("Vocal Out",juce::AudioChannelSet::stereo(),true)),
      parameters(*this,nullptr,"BabyVoiceState",layout()),
      pitch(parameters.getRawParameterValue("pitch")), formant(parameters.getRawParameterValue("formant")),
      mix(parameters.getRawParameterValue("mix")), bypass(parameters.getRawParameterValue("bypass")) {}
bool BabyVoiceProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet()
        && (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono() || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
void BabyVoiceProcessor::prepareToPlay(double sr, int) {
    dsp.prepare(sr,getTotalNumInputChannels(),pitch->load(),formant->load(),mix->load()*0.01f,bypass->load()>0.5f);
    setLatencySamples(dsp.latency());
}
void BabyVoiceProcessor::render(juce::AudioBuffer<float>& b, bool hostBypass) {
    juce::ScopedNoDenormals noDenormals;
    dsp.process(b.getArrayOfWritePointers(), b.getNumSamples(),pitch->load(),formant->load(),mix->load()*0.01f,hostBypass || bypass->load()>0.5f);
}
void BabyVoiceProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) { render(b,false); }
void BabyVoiceProcessor::processBlockBypassed(juce::AudioBuffer<float>& b, juce::MidiBuffer&) { render(b,true); }
juce::AudioProcessorParameter* BabyVoiceProcessor::getBypassParameter() const { return parameters.getParameter("bypass"); }
void BabyVoiceProcessor::setCurrentProgram(int i) {
    if (i<0 || i>=getNumPrograms()) return;
    const auto& p = babyPresets[static_cast<size_t>(i)];
    const char* ids[]{"pitch","formant","mix","bypass"};
    const float values[]{p.pitch,p.formant,p.mix,0};
    for (int k=0;k<4;++k) {
        auto* param = parameters.getParameter(ids[k]);
        param->beginChangeGesture(); param->setValueNotifyingHost(param->convertTo0to1(values[k])); param->endChangeGesture();
    }
}
int BabyVoiceProcessor::getCurrentProgram() { return juce::jmax(0, matchingPresetIndex()); }
int BabyVoiceProcessor::matchingPresetIndex() const {
    for (size_t i=0;i<babyPresets.size();++i) {
        const auto& p=babyPresets[i];
        if (std::abs(pitch->load()-p.pitch)<0.05f && std::abs(formant->load()-p.formant)<0.05f && std::abs(mix->load()-p.mix)<0.05f)
            return static_cast<int>(i);
    }
    return -1; // Custom settings: no factory program matches.
}
const juce::String BabyVoiceProcessor::getProgramName(int i) {
    return i>=0 && i<getNumPrograms() ? babyPresets[static_cast<size_t>(i)].name : "Custom";
}
void BabyVoiceProcessor::resetParameters() { setCurrentProgram(0); }
void BabyVoiceProcessor::getStateInformation(juce::MemoryBlock& d) {
    if (auto xml=parameters.copyState().createXml()) copyXmlToBinary(*xml,d);
}
void BabyVoiceProcessor::setStateInformation(const void* data,int bytes) {
    if (auto xml=getXmlFromBinary(data,bytes))
        if (xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* BabyVoiceProcessor::createEditor() { return new BabyVoiceEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BabyVoiceProcessor(); }
