#include "hush/mixer.hpp"
#include <algorithm>
#include <cmath>

namespace hush {
PanGains pan_gains(float pan, PanLaw law) noexcept {
    constexpr float pi = 3.14159265358979323846F;
    constexpr float legacy_center = 0.5946035575F; // -4.515 dB amplitude
    const float p = std::isfinite(pan) ? std::clamp(pan, -1.0F, 1.0F) : 0.0F;

    if (law == PanLaw::LegacyMinus4_5dB) {
        // Piecewise-linear balance with a continuous -4.5 dB center dip.
        // Hard-left=(1,0), center=(legacy_center,legacy_center), hard-right=(0,1).
        if (p < 0.0F) {
            return {legacy_center + (-p) * (1.0F - legacy_center),
                    legacy_center * (1.0F + p)};
        }
        return {legacy_center * (1.0F - p),
                legacy_center + p * (1.0F - legacy_center)};
    }

    const float angle = (p + 1.0F) * pi * 0.25F;
    return {std::cos(angle), std::sin(angle)};
}

StereoBuffer mix_tracks(const Project& p, const std::vector<std::vector<float>>& inputs) {
    std::size_t frames = 0;
    for (const auto& v : inputs) frames = std::max(frames, v.size());
    StereoBuffer out{std::vector<float>(frames, 0.0F), std::vector<float>(frames, 0.0F)};
    const bool any_solo = std::any_of(p.tracks.begin(), p.tracks.end(),
                                      [](const Track& t) { return t.solo; });
    for (std::size_t n = 0; n < p.tracks.size() && n < inputs.size(); ++n) {
        const auto& track = p.tracks[n];
        if (track.mute || (any_solo && !track.solo)) continue;
        const auto gains = pan_gains(track.pan, p.pan_law);
        for (std::size_t i = 0; i < inputs[n].size(); ++i) {
            const float sample = inputs[n][i] * track.gain;
            // Intentionally do not clamp: float32 mixer output may exceed [-1, 1].
            out.left[i] += sample * gains.left;
            out.right[i] += sample * gains.right;
        }
    }
    return out;
}
} // namespace hush
