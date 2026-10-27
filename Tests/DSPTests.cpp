#include "DSP/PitchProcessor.h"
#include <iostream>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <complex>
#include <numeric>
#include <random>
#include <fstream>
#include <cstdint>
static bool watchAllocations = false;
static size_t allocations = 0;
void* operator new(std::size_t n) { if(watchAllocations)++allocations; if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc(); }
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
constexpr double pi=3.14159265358979323846;
void check(bool v,const char* msg){if(!v)throw std::runtime_error(msg);}
using Audio=std::vector<std::vector<float>>;
Audio vocal(double sr,int channels,int count) {
    Audio a(static_cast<size_t>(channels),std::vector<float>(static_cast<size_t>(count)));
    for(int c=0;c<channels;++c)for(int i=0;i<count;++i){
        double value=0,t=i/sr;
        for(int h=1;h<70;++h){
            double f=h*120.;
            double envelope=std::exp(-0.5*std::pow((f-850)/150,2))+0.5*std::exp(-0.5*std::pow((f-2200)/250,2))+0.05;
            value+=envelope*std::sin(2*pi*f*t+0.03*c*h);
        }
        a[static_cast<size_t>(c)][static_cast<size_t>(i)]=static_cast<float>(value*0.035);
    }return a;
}
Audio render(const Audio& source,double sr,float pitch,float formant,float mix,bool bypass,int& latency,const std::vector<int>& blocks={257,1,64,1024,7,511}) {
    auto out=source;
    baby::PitchProcessor dsp;dsp.prepare(sr,static_cast<int>(out.size()),pitch,formant,mix,bypass);latency=dsp.latency();
    size_t offset=0,k=0;
    while(offset<out[0].size()){
        int n=std::min(blocks[k++%blocks.size()],static_cast<int>(out[0].size()-offset));
        float* ptr[2]={out[0].data()+offset,out.size()>1?out[1].data()+offset:nullptr};
        watchAllocations=true;dsp.process(ptr,n,pitch,formant,mix,bypass);watchAllocations=false;
        for(auto& channel:out)for(int i=0;i<n;++i)check(std::isfinite(channel[offset+static_cast<size_t>(i)]),"Nonfinite output");
        offset+=static_cast<size_t>(n);
    }return out;
}
double magnitude(const std::vector<float>& a,double sr,double f) {
    const size_t count=std::min<size_t>(16384,a.size()/2),start=a.size()-count;
    std::complex<double> sum{0,0},step=std::polar(1.,-2*pi*f/sr),phase{1,0};
    for(size_t i=0;i<count;++i){sum+=static_cast<double>(a[start+i])*(0.5-0.5*std::cos(2*pi*i/(count-1)))*phase;phase*=step;}
    return std::abs(sum)/count;
}
double peak(const std::vector<float>& a,double sr,double lo,double hi,double step){
    double best=0,freq=lo;
    for(double f=lo;f<=hi;f+=step){double m=magnitude(a,sr,f);if(m>best){best=m;freq=f;}}return freq;
}
void wav(const char* path,const std::vector<float>& a,int sr){
    std::ofstream f(path,std::ios::binary);auto u16=[&](uint16_t v){char b[]{char(v),char(v>>8)};f.write(b,2);};auto u32=[&](uint32_t v){u16(uint16_t(v));u16(uint16_t(v>>16));};
    f.write("RIFF",4);u32(uint32_t(36+a.size()*2));f.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(uint32_t(sr));u32(uint32_t(sr*2));u16(2);u16(16);f.write("data",4);u32(uint32_t(a.size()*2));
    for(float x:a)u16(static_cast<uint16_t>(static_cast<int16_t>(std::clamp(x,-1.f,1.f)*32767)));
}
int main(){try{
    for(double sr:{44100.,48000.,88200.,96000.})for(int ch:{1,2}){
        auto source=vocal(sr,ch,static_cast<int>(sr*0.55));int latency=0;
        auto dry=render(source,sr,7,5,0,false,latency);
        for(int c=0;c<ch;++c)for(size_t i=0;i<source[0].size();++i)
            check(std::abs(dry[static_cast<size_t>(c)][i]-(i>=static_cast<size_t>(latency)?source[static_cast<size_t>(c)][i-static_cast<size_t>(latency)]:0))<1e-7f,"Dry latency mismatch");
        auto bypass=render(source,sr,7,5,1,true,latency);check(bypass==dry,"Bypass is not delayed dry");
        auto wet=render(source,sr,7,5,1,false,latency),half=render(source,sr,7,5,.5f,false,latency);
        for(size_t c=0;c<source.size();++c)for(size_t i=0;i<source[0].size();++i)
            check(std::abs(half[c][i]-.5f*(wet[c][i]+dry[c][i]))<1e-6f,"Mix is not latency aligned");
        auto alt=render(source,sr,7,5,1,false,latency,{32});
        double e=0;for(size_t i=0;i<wet[0].size();++i)e=std::max(e,double(std::abs(alt[0][i]-wet[0][i])));
        check(e<1e-5,"Host block size changes output");
        std::cout<<"PASS "<<sr<<" Hz / "<<ch<<" channels: dry, bypass, mix, block sizes; latency="<<latency<<"\n";
    }
    constexpr double sr=48000;int latency=0;
    Audio sine(1,std::vector<float>(96000));for(size_t i=0;i<sine[0].size();++i)sine[0][i]=.15f*static_cast<float>(std::sin(2*pi*220*i/sr));
    for(float pitch:{-12.f,0.f,7.f,12.f}){
        auto a=render(sine,sr,pitch,0,1,false,latency);
        double expected=220*std::pow(2.,pitch/12.),actual=peak(a[0],sr,expected-5,expected+5,.25);
        std::cout<<"Pitch "<<pitch<<": "<<actual<<" Hz; expected "<<expected<<"\n";
        check(std::abs(actual-expected)<1,"Pitch shift frequency wrong");
        check(magnitude(a[0],sr,actual)>.01,"Pitch shift silent");
    }
    auto source=vocal(sr,1,96000);
    auto natural=render(source,sr,0,0,1,false,latency);
    auto formant=render(source,sr,0,7,1,false,latency);
    auto pitched=render(source,sr,7,0,1,false,latency);
    double npeak=peak(natural[0],sr,500,1600,5),fpeak=peak(formant[0],sr,500,1700,5),ppeak=peak(pitched[0],sr,500,1600,5);
    std::cout<<"Envelope dominant peak: natural="<<npeak<<", formant +7="<<fpeak<<", pitch +7 / formant 0="<<ppeak<<"\n";
    check(fpeak>npeak*1.25,"Formant control did not move envelope upward");
    check(std::abs(ppeak-npeak)<240,"Pitch shifted the formant envelope excessively");
    auto fund=peak(formant[0],sr,115,125,.25);check(std::abs(fund-120)<1,"Formant changed fundamental");
    // Full harmonic grid: envelope movement must not detune the source harmonics.
    check(magnitude(formant[0],sr,1200)>magnitude(formant[0],sr,1230)*5,"Formant moved harmonic grid");
    // Automation, first callback, arbitrary lengths, silence, and stereo channel isolation.
    baby::PitchProcessor dsp;dsp.prepare(sr,2);std::mt19937 rng(42);std::uniform_real_distribution<float> noise(-.1f,.1f);
    std::array<float,511> left{},right{};float* ptr[]{left.data(),right.data()};
    for(int block=0;block<400;++block){for(auto& x:left)x=block<250?noise(rng):0;right.fill(0);
        watchAllocations=true;dsp.process(ptr,511,block%2?12.f:-12.f,block%3?7.f:-9.f,block%5?.7f:0,block%13==0);watchAllocations=false;
        for(size_t i=0;i<left.size();++i){check(std::isfinite(left[i])&&std::abs(left[i])<4,"Automation unstable");check(std::abs(right[i])<1e-7f,"Stereo crosstalk");}
    }
    watchAllocations=true;dsp.reset();watchAllocations=false;
    left.fill(0);right.fill(0);
    for(int i=0;i<50;++i){watchAllocations=true;dsp.process(ptr,511,0,0,1,false);watchAllocations=false;for(float x:left)check(std::abs(x)<1e-7f,"Reset retained old audio");}
    std::cout<<"Audio-thread heap allocations: "<<allocations<<"\n";check(allocations==0,"Audio-thread allocation detected");
    auto baby=render(source,sr,7,5,1,false,latency);
    wav("vocal-source.wav",source[0],48000);wav("vocal-baby.wav",baby[0],48000);
    std::cout<<"ALL DSP CHECKS PASSED (synthetic voice; listening test on real vocal still required)\n";
    return 0;
}catch(const std::exception& e){watchAllocations=false;std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
