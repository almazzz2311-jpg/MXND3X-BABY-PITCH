#include "PluginEditor.h"
#include "BinaryData.h"
BabyVoiceEditor::BabyVoiceEditor(BabyVoiceProcessor& p):AudioProcessorEditor(p),processor(p) {
    setLookAndFeel(&theme);
    brand=juce::ImageCache::getFromMemory(BinaryData::brand_png,BinaryData::brand_pngSize);
    if (brand.isValid()) {
        character=brand.getClippedImage({32,35,765,894});
        title=brand.getClippedImage({790,260,870,570});
    }
    addAndMakeVisible(canvas);
    for(auto* c:std::initializer_list<juce::Component*>{&pitch,&formant,&mix,&bypass,&reset,&presets})canvas.addAndMakeVisible(c);
    bypass.setClickingTogglesState(true);
    bypass.setTooltip("Crossfade to latency-compensated original vocal");
    reset.setTooltip("Reset pitch, formant, mix and bypass to Natural");
    reset.onClick=[this]{processor.resetParameters();};
    for(size_t i=0;i<babyPresets.size();++i)presets.addItem(babyPresets[i].name,static_cast<int>(i)+1);
    presets.setTextWhenNothingSelected("Custom");
    presets.onChange=[this]{if(presets.getSelectedId()>0)processor.setCurrentProgram(presets.getSelectedId()-1);};
    pitchLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"pitch",pitch.slider);
    formantLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"formant",formant.slider);
    mixLink=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"mix",mix.slider);
    bypassLink=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"bypass",bypass);
    canvas.setLookAndFeel(&theme);
    pitch.refreshText();formant.refreshText();mix.refreshText();
    setResizable(true,true);
    setResizeLimits(720,480,1350,900);
    getConstrainer()->setFixedAspectRatio(1.5);
    setSize(900,600);
    timerCallback();startTimerHz(15);
}
BabyVoiceEditor::~BabyVoiceEditor(){stopTimer();canvas.setLookAndFeel(nullptr);setLookAndFeel(nullptr);}
void BabyVoiceEditor::timerCallback(){presets.setSelectedId(processor.matchingPresetIndex()+1,juce::dontSendNotification);}
void BabyVoiceEditor::resized(){
    const float scale=getWidth()/900.f;
    canvas.setBounds(0,0,900,600);canvas.setTransform(juce::AffineTransform::scale(scale));
    presets.setBounds(556,265,198,36);reset.setBounds(766,265,94,36);bypass.setBounds(40,265,112,36);
    pitch.setBounds(36,320,268,234);formant.setBounds(316,320,268,234);mix.setBounds(596,320,268,234);
}
void BabyVoiceEditor::paint(juce::Graphics& g){
    g.fillAll(juce::Colour(0xfff4faff));
    juce::Graphics::ScopedSaveState saved(g);g.addTransform(juce::AffineTransform::scale(getWidth()/900.f));
    g.setColour(juce::Colours::white);g.fillRoundedRectangle(16,16,868,238,24);
    if(character.isValid())g.drawImageWithin(character,48,22,210,224,juce::RectanglePlacement::centred);
    if(title.isValid())g.drawImageWithin(title,300,30,548,210,juce::RectanglePlacement::centred);
    juce::Path sparkle; sparkle.addStar({276.f,64.f},4,4.f,12.f);sparkle.addStar({854.f,230.f},4,3.f,9.f);
    g.setColour(juce::Colour(0xffb7e4ff));g.fillPath(sparkle);g.setColour(juce::Colour(0xff15232e));g.strokePath(sparkle,juce::PathStrokeType(2.f));
    g.setColour(juce::Colour(0xff6c8598));g.setFont(juce::FontOptions(12));
    g.drawText("INDEPENDENT PITCH + FORMANT",172,267,320,32,juce::Justification::centredLeft);
    g.drawText("MXND3X  /  VOCAL PLAYGROUND",40,562,360,22,juce::Justification::centredLeft);
    g.drawText("SHIFT + DRAG: FINE  /  DOUBLE CLICK: RESET",470,562,390,22,juce::Justification::centredRight);
}
