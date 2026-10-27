#pragma once
#include <cmath>
namespace baby {
// Spectral-envelope transport is performed inside the shared STFT engine.
// This control object smooths envelope transposition independently of pitch.
class FormantProcessor {
public:
    void prepare(double sampleRate, float initial = 0.f) noexcept;
    void setTarget(float semitones) noexcept;
    float advance(int samples) noexcept;
    float current() const noexcept { return value; }
private:
    float value = 0, target = 0;
    double coefficient = 0;
};
}
