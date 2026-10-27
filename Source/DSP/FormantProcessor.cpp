#include "FormantProcessor.h"
#include <algorithm>
namespace baby {
void FormantProcessor::prepare(double sr, float initial) noexcept {
    value = target = initial;
    coefficient = -1.0 / (0.03 * sr);
}
void FormantProcessor::setTarget(float s) noexcept { target = std::clamp(s, -12.f, 12.f); }
float FormantProcessor::advance(int n) noexcept {
    value = target + (value - target) * static_cast<float>(std::exp(coefficient * n));
    if (std::abs(value - target) < 1e-5f) value = target;
    return value;
}
}
