#pragma once
#include <signalsmith-stretch/signalsmith-stretch.h>
#include "FormantProcessor.h"
#include <array>
#include <vector>
namespace baby {
class PitchProcessor {
public:
    void prepare(double sampleRate, int channels, float pitch = 0, float formant = 0, float mix = 1, bool bypass = false);
    // Input/output may alias. Unlimited host block sizes are subdivided without allocation.
    void process(float* const* audio, int samples, float pitch, float formant, float mix, bool bypass) noexcept;
    void reset() noexcept;
    int latency() const noexcept { return delay; }
private:
    static constexpr int quantum = 32;
    signalsmith::stretch::SignalsmithStretch<float> engine{0x4d584e44};
    FormantProcessor pitchControl, formantControl;
    std::array<std::array<float, quantum>, 2> input{}, wet{};
    std::array<std::vector<float>, 2> dry;
    int channelCount = 0, delay = 0, writeIndex = 0;
    float wetAmount = 1;
    double mixCoefficient = 0;
};
}
