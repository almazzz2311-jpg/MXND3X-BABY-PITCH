#include "PitchProcessor.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace baby {
void PitchProcessor::prepare(double sr, int channels, float pitch, float formant, float mix, bool bypass) {
    if (channels < 1 || channels > 2 || sr < 8000) throw std::invalid_argument("Unsupported audio layout");
    channelCount = channels;
    // Default quality: ~120ms analysis window, not the cheaper short-window mode.
    engine.presetDefault(channels, static_cast<float>(sr), true);
    engine.reset();
    engine.setFormantBase(0); // Library's adaptive fundamental estimator.
    delay = engine.inputLatency() + engine.outputLatency();
    for (auto& d : dry) d.assign(static_cast<size_t>(delay + 1), 0.f);
    writeIndex = 0;
    pitchControl.prepare(sr, pitch);
    formantControl.prepare(sr, formant);
    wetAmount = bypass ? 0.f : std::clamp(mix, 0.f, 1.f);
    mixCoefficient = std::exp(-1.0 / (sr * 0.02));
    engine.setTransposeSemitones(pitch);
    engine.setFormantSemitones(formant, true);
}
void PitchProcessor::reset() noexcept {
    if (!channelCount) return;
    engine.reset();
    for (auto& d : dry) std::fill(d.begin(), d.end(), 0.f);
    writeIndex = 0;
}
void PitchProcessor::process(float* const* audio, int samples, float pitch, float formant, float mix, bool bypass) noexcept {
    if (!channelCount) return;
    pitchControl.setTarget(pitch);
    formantControl.setTarget(formant);
    const float targetWet = bypass ? 0.f : std::clamp(mix, 0.f, 1.f);
    const float* ins[2] = {input[0].data(), input[1].data()};
    float* outs[2] = {wet[0].data(), wet[1].data()};
    for (int offset = 0; offset < samples; offset += quantum) {
        const int n = std::min(quantum, samples - offset);
        for (int c = 0; c < channelCount; ++c)
            std::copy_n(audio[c] + offset, n, input[c].data());
        engine.setTransposeSemitones(pitchControl.advance(n));
        // compensatePitch=true: formant=0 preserves the source envelope even when pitch changes.
        engine.setFormantSemitones(formantControl.advance(n), true);
        engine.process(ins, n, outs, n);
        for (int i = 0; i < n; ++i) {
            wetAmount = targetWet + (wetAmount - targetWet) * static_cast<float>(mixCoefficient);
            if (std::abs(wetAmount - targetWet) < 1e-6f) wetAmount = targetWet;
            const int read = (writeIndex + 1) % (delay + 1);
            for (int c = 0; c < channelCount; ++c) {
                dry[c][static_cast<size_t>(writeIndex)] = input[c][i];
                const float original = dry[c][static_cast<size_t>(read)];
                const float processed = wet[c][i];
                // Linear crossfade preserves unity gain for correlated dry/wet at neutral settings.
                // No hard limiter: float audio retains headroom for the host mixer.
                audio[c][offset + i] = original + wetAmount * (processed - original);
            }
            writeIndex = read;
        }
    }
}
}
