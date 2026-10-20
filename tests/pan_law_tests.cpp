#include "hush/mixer.hpp"

#include <cassert>
#include <cmath>
#include <limits>

namespace {
bool near(float a, float b, float epsilon = 1.0e-5F) {
    return std::abs(a - b) <= epsilon;
}
}

int main() {
    using namespace hush;
    const auto left = pan_gains(-1.0F, PanLaw::EqualPowerMinus3dB);
    const auto center = pan_gains(0.0F, PanLaw::EqualPowerMinus3dB);
    const auto right = pan_gains(1.0F, PanLaw::EqualPowerMinus3dB);
    assert(near(left.left, 1.0F) && near(left.right, 0.0F));
    assert(near(center.left, std::sqrt(0.5F)) && near(center.right, std::sqrt(0.5F)));
    assert(near(right.left, 0.0F) && near(right.right, 1.0F));

    const auto legacy_left = pan_gains(-1.0F, PanLaw::LegacyMinus4_5dB);
    const auto legacy_center = pan_gains(0.0F, PanLaw::LegacyMinus4_5dB);
    const auto legacy_right = pan_gains(1.0F, PanLaw::LegacyMinus4_5dB);
    assert(near(legacy_left.left, 1.0F) && near(legacy_left.right, 0.0F));
    assert(near(legacy_center.left, 0.5946035575F));
    assert(near(legacy_center.right, 0.5946035575F));
    assert(near(legacy_right.left, 0.0F) && near(legacy_right.right, 1.0F));

    // Continuity at center and bounded behavior for invalid/out-of-range controls.
    const auto legacy_before = pan_gains(-1.0e-6F, PanLaw::LegacyMinus4_5dB);
    const auto legacy_after = pan_gains(1.0e-6F, PanLaw::LegacyMinus4_5dB);
    assert(near(legacy_before.left, legacy_after.left, 2.0e-6F));
    assert(near(legacy_before.right, legacy_after.right, 2.0e-6F));
    assert(pan_gains(-10.0F, PanLaw::EqualPowerMinus3dB).left == left.left);
    assert(pan_gains(10.0F, PanLaw::EqualPowerMinus3dB).right == right.right);
    const auto nonfinite = pan_gains(std::numeric_limits<float>::quiet_NaN(),
                                     PanLaw::EqualPowerMinus3dB);
    assert(near(nonfinite.left, center.left) && near(nonfinite.right, center.right));

    Project project;
    project.tracks = {{"hot", "Hot", 2.0F, 0.0F, false, false}};
    const auto mixed = mix_tracks(project, {{1.0F}});
    assert(mixed.left.size() == 1U && mixed.right.size() == 1U);
    assert(mixed.left[0] > 1.0F && mixed.right[0] > 1.0F); // float path does not clamp
    return 0;
}
