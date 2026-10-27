// Minimal Linux VST3 host smoke test. Loads the produced module, not linked processor code.
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <dlfcn.h>
#include <iostream>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace Steinberg;
using namespace Steinberg::Vst;
void require(bool b,const char* m){if(!b)throw std::runtime_error(m);}
class Host final : public IHostApplication {
public:
    tresult PLUGIN_API queryInterface(const TUID id,void** out) override {
        if(FUnknownPrivate::iidEqual(id,FUnknown::iid) || FUnknownPrivate::iidEqual(id,IHostApplication::iid)){
            *out=static_cast<IHostApplication*>(this);addRef();return kResultOk;
        }*out=nullptr;return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override {return ++refs;}
    uint32 PLUGIN_API release() override {return --refs;}
    tresult PLUGIN_API getName(String128 name) override {
        const char16_t text[]=u"MXND3X Test Host";std::memcpy(name,text,sizeof(text));return kResultOk;
    }
    tresult PLUGIN_API createInstance(TUID,TUID,void** object) override {*object=nullptr;return kNotImplemented;}
private:uint32 refs=1;
};
int main(int argc,char** argv){try{
    require(argc==2,"Pass VST3 shared object path");void* module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!module)throw std::runtime_error(dlerror());
    auto enter=reinterpret_cast<bool(*)(void*)>(dlsym(module,"ModuleEntry"));
    auto leave=reinterpret_cast<bool(*)()>(dlsym(module,"ModuleExit"));
    auto getFactory=reinterpret_cast<IPluginFactory*(*)()>(dlsym(module,"GetPluginFactory"));
    require(enter&&leave&&getFactory,"VST3 entry points missing");require(enter(module),"ModuleEntry failed");
    auto* factory=getFactory();require(factory!=nullptr,"Factory missing");PClassInfo info{};bool found=false;
    for(int32 i=0;i<factory->countClasses();++i){require(factory->getClassInfo(i,&info)==kResultOk,"Class info failed");if(std::strcmp(info.category,kVstAudioEffectClass)==0){found=true;break;}}
    require(found,"No audio effect class");std::cout<<"Loaded VST3: "<<info.name<<"\n";
    IComponent* component=nullptr;require(factory->createInstance(info.cid,IComponent::iid,reinterpret_cast<void**>(&component))==kResultOk,"Component instantiation failed");
    Host host;require(component->initialize(&host)==kResultOk,"Component initialize failed");
    IAudioProcessor* audio=nullptr;require(component->queryInterface(IAudioProcessor::iid,reinterpret_cast<void**>(&audio))==kResultOk,"No audio processor interface");
    SpeakerArrangement stereo=SpeakerArr::kStereo;require(audio->setBusArrangements(&stereo,1,&stereo,1)==kResultOk,"Stereo bus setup failed");
    require(component->activateBus(kAudio,kInput,0,true)==kResultOk,"Input activation failed");require(component->activateBus(kAudio,kOutput,0,true)==kResultOk,"Output activation failed");
    ProcessSetup setup{};setup.processMode=kRealtime;setup.symbolicSampleSize=kSample32;setup.maxSamplesPerBlock=257;setup.sampleRate=48000;
    require(audio->setupProcessing(setup)==kResultOk,"Processing setup failed");
    require(component->setActive(true)==kResultOk,"Activation failed");const auto latency=audio->getLatencySamples();std::cout<<"VST3 latency="<<latency<<"\n";require(latency==7200,"Unexpected VST3 latency");require(audio->setProcessing(true)==kResultOk,"Processing activation failed");
    std::array<float,257> inLeft{},inRight{},outLeft{},outRight{};
    float* inputs[]{inLeft.data(),inRight.data()};float* outputs[]{outLeft.data(),outRight.data()};
    AudioBusBuffers inBus{},outBus{};inBus.numChannels=outBus.numChannels=2;inBus.channelBuffers32=inputs;outBus.channelBuffers32=outputs;
    ProcessData data{};data.processMode=kRealtime;data.symbolicSampleSize=kSample32;data.numSamples=257;data.numInputs=data.numOutputs=1;data.inputs=&inBus;data.outputs=&outBus;
    double energy=0,error=0;size_t samples=0;
    for(int block=0;block<400;++block){
        for(int i=0;i<257;++i){const auto n=block*257+i;inLeft[static_cast<size_t>(i)]=.1f*float(std::sin(6.283185307179586*220*n/48000.));inRight[static_cast<size_t>(i)]=0;}
        require(audio->process(data)==kResultOk,"VST3 audio process failed");
        for(int i=0;i<257;++i){float x=outLeft[static_cast<size_t>(i)];require(std::isfinite(x),"Nonfinite VST3 output");require(std::abs(outRight[static_cast<size_t>(i)])<1e-6,"VST3 stereo crosstalk");
            int n=block*257+i;if(n>int(latency)+12000){double expected=.1*std::sin(6.283185307179586*220*(n-int(latency))/48000.);error+=(x-expected)*(x-expected);energy+=x*x;++samples;}}
    }
    require(energy/samples>.001,"VST3 output silent");require(std::sqrt(error/samples)<.003,"Neutral VST3 output is not latency aligned");
    require(audio->setProcessing(false)==kResultOk,"Stop processing failed");require(component->setActive(false)==kResultOk,"Deactivate failed");audio->release();require(component->terminate()==kResultOk,"Terminate failed");component->release();factory->release();require(leave(),"ModuleExit failed");dlclose(module);
    std::cout<<"PASS VST3 load, factory, buses, real-time process, latency, stereo isolation, teardown; neutral RMS error="<<std::sqrt(error/samples)<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
