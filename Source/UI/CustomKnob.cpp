#include "CustomKnob.h"
namespace {
const juce::Colour ink{0xff15232e}, blue{0xff80cfff};
}
BabyLookAndFeel::BabyLookAndFeel() {
    setColour(juce::Slider::textBoxTextColourId,ink);
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xffe9f6ff));
    setColour(juce::ComboBox::backgroundColourId,juce::Colours::white);
    setColour(juce::ComboBox::textColourId,ink);
    setColour(juce::ComboBox::outlineColourId,ink);
    setColour(juce::ComboBox::arrowColourId,ink);
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xfff0f9ff));
    setColour(juce::PopupMenu::textColourId,ink);
    setColour(juce::TextButton::textColourOffId,ink);
    setColour(juce::TextButton::textColourOnId,ink);
}
void BabyLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider& s) {
    const auto r = juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(16);
    const float radius = juce::jmin(r.getWidth(),r.getHeight())*0.5f;
    const auto centre=r.getCentre();
    const auto dial=juce::Rectangle<float>(radius*2,radius*2).withCentre(centre);
    juce::Path track, active;
    track.addCentredArc(centre.x,centre.y,radius+9,radius+9,0,start,end,true);
    active.addCentredArc(centre.x,centre.y,radius+9,radius+9,0,start,start+pos*(end-start),true);
    g.setColour(juce::Colour(0xffd9edf9)); g.strokePath(track,juce::PathStrokeType(5));
    g.setColour(juce::Colour(0xff4eaef0)); g.strokePath(active,juce::PathStrokeType(5,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    g.setColour(ink.withAlpha(0.14f));g.fillEllipse(dial.translated(0,5));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd5f2ff),centre.x,dial.getY(),blue.darker(s.isMouseButtonDown()?0.15f:0.03f),centre.x,dial.getBottom(),false));
    g.fillEllipse(dial);g.setColour(ink);g.drawEllipse(dial,3.5f);
    g.setColour(juce::Colours::white.withAlpha(0.65f));g.drawEllipse(dial.reduced(8),1.5f);
    juce::Path marker; marker.addRoundedRectangle(-3,-radius+16,6,radius*0.44f,3);
    marker.applyTransform(juce::AffineTransform::rotation(start+pos*(end-start)).translated(centre.x,centre.y));
    g.setColour(ink);g.fillPath(marker);
    g.setColour(juce::Colours::white.withAlpha(0.72f));
    g.fillEllipse(centre.x-radius*0.48f,centre.y-radius*0.58f,radius*0.35f,radius*0.18f);
}
void BabyLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) {
    auto r=b.getLocalBounds().toFloat().reduced(2);
    g.setColour(ink.withAlpha(0.1f));g.fillRoundedRectangle(r.translated(0,2),12);
    g.setColour(b.getToggleState()?blue:(over?juce::Colour(0xffdef3ff):juce::Colours::white));
    g.fillRoundedRectangle(down?r.translated(0,1):r,12);
    g.setColour(ink);g.drawRoundedRectangle(r,12,2);
}
CustomKnob::CustomKnob(const juce::String& title,bool percent):name(title),isPercent(percent) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,110,30);
    slider.setRange(percent?0:-12,percent?100:12,0.1);
    slider.setDoubleClickReturnValue(true,percent?100:0);
    slider.setVelocityBasedMode(false);
    slider.setVelocityModeParameters(0.35,1,0.0,true,juce::ModifierKeys::shiftModifier);
    slider.setMouseDragSensitivity(240);
    slider.setTitle(title);
    slider.setDescription(percent?"Dry / wet blend. Double click: 100 percent.":"Semitones. Double click: zero. Shift-drag for fine adjustment.");
    refreshText();
    addAndMakeVisible(slider);
    startTimerHz(30);
}
void CustomKnob::refreshText(){
    const bool percent=isPercent;
    slider.textFromValueFunction=[percent](double v){return (v>0 && !percent?juce::String("+"):juce::String())+juce::String(v,1)+(percent?" %":" st");};
    slider.valueFromTextFunction=[](const juce::String& t){return t.retainCharacters("0123456789.-+").getDoubleValue();};
    slider.updateText();
}
void CustomKnob::resized(){slider.setBounds(14,42,getWidth()-28,getHeight()-58);}
void CustomKnob::timerCallback(){const float target=isMouseOver(true)?1.f:0.f;hover+=(target-hover)*0.2f;if(std::abs(target-hover)>0.001f)repaint();}
void CustomKnob::paint(juce::Graphics& g){
    auto r=getLocalBounds().toFloat().reduced(4);
    g.setColour(juce::Colours::white.interpolatedWith(juce::Colour(0xffeaf7ff),hover));g.fillRoundedRectangle(r,24);
    g.setColour(juce::Colour(0xffd5e9f5));g.drawRoundedRectangle(r,24,1.5f);
    g.setColour(ink);g.setFont(juce::FontOptions(19,juce::Font::bold));g.drawText(name,0,16,getWidth(),26,juce::Justification::centred);
}
