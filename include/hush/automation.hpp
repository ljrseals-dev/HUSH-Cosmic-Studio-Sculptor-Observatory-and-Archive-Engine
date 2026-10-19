#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hush {

// Curves describe the segment starting at a point and ending at the next point.
enum class AutomationCurve : std::uint8_t { Step = 0, Linear = 1 };

struct AutomationPoint {
    std::uint64_t position_samples{};
    float value{};
    AutomationCurve curve_to_next{AutomationCurve::Linear};
};

// Immutable-by-convention copy for playback. Construct/update snapshots on a
// control thread, then publish them through the graph-lifetime mechanism before
// an audio callback reads them. evaluate() itself allocates nothing.
class AutomationSnapshot {
public:
    AutomationSnapshot() = default;
    explicit AutomationSnapshot(std::vector<AutomationPoint> points);

    [[nodiscard]] float evaluate(std::uint64_t position_samples) const noexcept;
    [[nodiscard]] bool empty() const noexcept { return points_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return points_.size(); }

private:
    std::vector<AutomationPoint> points_;
};

// Mutable editor-side representation. Mutations may allocate and must remain
// on a non-audio thread. A snapshot is a value copy suitable for immutable
// playback; safe publication/reclamation is deliberately owned by the graph
// publisher rather than hidden inside this class.
class AutomationLane {
public:
    [[nodiscard]] bool replace_points(std::vector<AutomationPoint> points);
    [[nodiscard]] bool upsert(AutomationPoint point);
    [[nodiscard]] AutomationSnapshot snapshot() const;
    [[nodiscard]] const std::vector<AutomationPoint>& points() const noexcept {
        return points_;
    }

private:
    static bool valid_point(const AutomationPoint& point) noexcept;
    static bool valid_sequence(const std::vector<AutomationPoint>& points) noexcept;
    std::vector<AutomationPoint> points_;
};

} // namespace hush
