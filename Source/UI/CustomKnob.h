#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
class BabyLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    BabyLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
};
class CustomKnob final : public juce::Component, private juce::Timer {
public:
    CustomKnob(const juce::String& title, bool percent);
    juce::Slider slider;
    void refreshText();
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    juce::String name;
    bool isPercent;
    float hover = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomKnob)
};
