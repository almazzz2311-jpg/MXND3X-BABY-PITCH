#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/PitchProcessor.h"
#include <array>
struct BabyPreset { const char* name; float pitch, formant, mix; };
inline constexpr std::array<BabyPreset, 6> babyPresets{{
    {"Natural",0,0,100}, {"Baby Voice",7,5,100}, {"Hyperpop",12,7,100},
    {"Deep Voice",-7,-5,100}, {"Monster",-12,-9,100}, {"Soft Baby",4,3,70}
}};
class BabyVoiceProcessor final : public juce::AudioProcessor {
public:
    BabyVoiceProcessor();
    using juce::AudioProcessor::processBlock;
    using juce::AudioProcessor::processBlockBypassed;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override { dsp.reset(); }
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MXND3X BABY VOICE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.3; }
    int getNumPrograms() override { return static_cast<int>(babyPresets.size()); }
    int getCurrentProgram() override;
    int matchingPresetIndex() const;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    void resetParameters();
    juce::AudioProcessorValueTreeState parameters;
private:
    void render(juce::AudioBuffer<float>&, bool);
    baby::PitchProcessor dsp;
    std::atomic<float> *pitch, *formant, *mix, *bypass;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BabyVoiceProcessor)
};
