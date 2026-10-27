#pragma once
#include "PluginProcessor.h"
#include "UI/CustomKnob.h"
class BabyVoiceEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit BabyVoiceEditor(BabyVoiceProcessor&);
    ~BabyVoiceEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    BabyVoiceProcessor& processor;
    BabyLookAndFeel theme;
    juce::Component canvas;
    CustomKnob pitch{"PITCH",false},formant{"FORMANT",false},mix{"MIX",true};
    juce::TextButton bypass{"BYPASS"},reset{"RESET"};
    juce::ComboBox presets;
    juce::Image brand, character, title;
    juce::TooltipWindow tooltips{this,600};
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchLink,formantLink,mixLink;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassLink;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BabyVoiceEditor)
};
