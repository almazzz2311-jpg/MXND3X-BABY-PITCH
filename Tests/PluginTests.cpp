#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>
#include <stdexcept>
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
void renderFile(const juce::File& input,const juce::File& output,int preset) {
    juce::AudioFormatManager formats;formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(input));
    check(reader!=nullptr,"Cannot read input audio");
    check(reader->numChannels>=1 && reader->numChannels<=2,"Use mono or stereo audio");
    check(reader->lengthInSamples>0 && reader->lengthInSamples<reader->sampleRate*600,"Use audio shorter than ten minutes");
    check(input!=output,"Input and output must be different files");
    check(!output.exists(),"Output already exists; choose a new filename");
    BabyVoiceProcessor processor;auto layout=processor.getBusesLayout();
    const int channels=static_cast<int>(reader->numChannels),length=static_cast<int>(reader->lengthInSamples);
    layout.inputBuses.set(0,channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo());layout.outputBuses=layout.inputBuses;
    check(processor.setBusesLayout(layout),"Unsupported file channel layout");
    check(preset>=0&&preset<6,"Preset index must be 0 through 5");processor.setCurrentProgram(preset);processor.prepareToPlay(reader->sampleRate,257);
    const int latency=processor.getLatencySamples();
    juce::AudioBuffer<float> source(channels,length),result(channels,length+latency);
    check(reader->read(&source,0,length,0,true,true),"Audio file read failed");result.clear();
    juce::AudioBuffer<float> block(channels,257);juce::MidiBuffer midi;
    for(int offset=0;offset<length+latency;offset+=257){
        const int n=juce::jmin(257,length+latency-offset);block.setSize(channels,n,false,false,true);block.clear();
        for(int c=0;c<channels;++c)if(offset<length)block.copyFrom(c,0,source,c,offset,juce::jmin(n,length-offset));
        processor.processBlock(block,midi);
        for(int c=0;c<channels;++c)result.copyFrom(c,offset,block,c,0,n);
    }
    double energy=0,difference=0;float peak=0;
    for(int c=0;c<channels;++c)for(int i=0;i<length;++i){float x=result.getSample(c,i+latency);check(std::isfinite(x),"Nonfinite rendered audio");peak=juce::jmax(peak,std::abs(x));energy+=x*x;float d=x-source.getSample(c,i);difference+=d*d;}
    check(energy>1e-10,"Rendered audio is silent");check(peak<=1.f,"Output exceeds PCM headroom; lower input gain first");
    std::unique_ptr<juce::OutputStream> stream=output.createOutputStream();check(stream!=nullptr,"Cannot create output audio");
    juce::WavAudioFormat wav;auto writer=wav.createWriterFor(stream,juce::AudioFormatWriterOptions{}.withSampleRate(reader->sampleRate).withNumChannels(channels).withBitsPerSample(24));
    check(writer!=nullptr,"Cannot create WAV writer");
    check(writer->writeFromAudioSampleBuffer(result,latency,length),"WAV writing failed");writer.reset();
    std::cout<<"PASS render: "<<length<<" frames, "<<reader->sampleRate<<" Hz, "<<channels<<" channels, latency trimmed="<<latency<<", peak="<<peak<<", RMS difference="<<std::sqrt(difference/(length*channels))<<"\n";
}
int checkNumberFields(juce::Component& c){
    int count=0;
    if(auto* label=dynamic_cast<juce::Label*>(&c))if(dynamic_cast<juce::Slider*>(c.getParentComponent())){
        check(label->getText().isNotEmpty(),"Empty knob value field");
        check(label->findColour(juce::Label::textColourId)!=label->findColour(juce::Label::backgroundColourId),"Invisible knob value text");++count;
    }
    for(auto* child:c.getChildren()) count+=checkNumberFields(*child);
    return count;
}
int main(int argc,char** argv){try{
    juce::ScopedJuceInitialiser_GUI init;
    if(argc==5 && juce::String(argv[1])=="--render"){renderFile(juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]),juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]),juce::String(argv[4]).getIntValue());return 0;}
    BabyVoiceProcessor p;
    for(int i=0;i<6;++i){p.setCurrentProgram(i);check(p.getCurrentProgram()==i,"Preset not applied");
        auto& s=babyPresets[static_cast<size_t>(i)];check(std::abs(p.parameters.getRawParameterValue("pitch")->load()-s.pitch)<.01f,"Preset pitch mismatch");
        check(std::abs(p.parameters.getRawParameterValue("formant")->load()-s.formant)<.01f,"Preset formant mismatch");
        check(std::abs(p.parameters.getRawParameterValue("mix")->load()-s.mix)<.01f,"Preset mix mismatch");}
    p.setCurrentProgram(5);p.parameters.getParameter("bypass")->setValueNotifyingHost(1);
    juce::MemoryBlock state;p.getStateInformation(state);p.resetParameters();p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    check(p.getCurrentProgram()==5,"State preset roundtrip failed");check(p.parameters.getRawParameterValue("bypass")->load()>0.5f,"Bypass state lost");
    const char invalid[]{1,2,3};p.setStateInformation(invalid,3);check(p.getCurrentProgram()==5,"Invalid state corrupted parameters");
    for(double sr:{44100.,48000.,88200.,96000.})for(int ch:{1,2}){
        auto layout=p.getBusesLayout();layout.inputBuses.set(0,ch==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo());layout.outputBuses=layout.inputBuses;
        check(p.setBusesLayout(layout),"Supported bus layout rejected");p.prepareToPlay(sr,127);check(p.getLatencySamples()>0,"Missing host latency");
        juce::AudioBuffer<float> b(ch,513);b.clear();juce::MidiBuffer midi;b.setSample(0,0,.5f);
        p.processBlock(b,midi);for(int c=0;c<ch;++c)for(int i=0;i<b.getNumSamples();++i)check(std::isfinite(b.getSample(c,i)),"Plugin nonfinite output");
        p.processBlockBypassed(b,midi);p.releaseResources();
    }
    if(argc>1 && (juce::String(argv[1])=="--gui" || juce::String(argv[1])=="--gui-desktop")){
        p.setCurrentProgram(1);
        for(int i=0;i<8;++i){std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());check(editor!=nullptr,"Editor creation failed");
            editor->setSize(i%2?720:1350,i%2?480:900);
            if(juce::String(argv[1])=="--gui-desktop"){editor->addToDesktop(0);editor->setVisible(true);}
            check(checkNumberFields(*editor)==3,"Missing numeric knob fields");
            auto image=editor->createComponentSnapshot(editor->getLocalBounds());check(image.isValid(),"Editor render failed");
            if(i==0){juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile("interface.png");auto stream=file.createOutputStream();if(stream){stream->setPosition(0);stream->truncate();}juce::PNGImageFormat png;check(stream&&png.writeImageToStream(image,*stream),"Screenshot failed");}
        }
        std::cout<<"PASS editor create, resize, render and destroy x8\n";
    }
    std::cout<<"PASS factory presets, parameter state roundtrip, invalid state, mono/stereo, 4 sample rates, host bypass\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
