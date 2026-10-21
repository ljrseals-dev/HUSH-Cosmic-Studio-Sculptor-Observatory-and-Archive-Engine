#pragma once

#include "hush/bus_graph.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace hush {

struct BusInputView {
    const float* left{};
    const float* right{};
    float fader_gain{1.0F};
};

struct StereoConstView {
    const float* left{};
    const float* right{};
    std::size_t frames{};
};

enum class RealtimeProcessStatus {
    Ok,
    NotConfigured,
    InvalidInputCount,
    NullChannel,
    InvalidFaderGain,
    FrameCountExceedsCapacity
};

struct ProcessorSetupError {
    std::string code;
    std::string entity_id;
    std::string message;
};

// Control-thread-configured processor. configure() may allocate; process()
// does not allocate, lock, log, or perform I/O. One process() caller at a time.
class PreallocatedBusProcessor {
public:
    bool configure(const BusGraph& graph, std::size_t max_frames,
                   ProcessorSetupError& error,
                   const std::vector<BusInsert>& inserts = {});

    [[nodiscard]] RealtimeProcessStatus process(const BusInputView* inputs,
                                                std::size_t input_count,
                                                std::size_t frame_count) noexcept;

    [[nodiscard]] std::size_t bus_count() const noexcept { return buses_.size(); }
    [[nodiscard]] std::size_t max_frames() const noexcept { return max_frames_; }
    [[nodiscard]] const std::string& bus_id(std::size_t index) const;
    [[nodiscard]] StereoConstView pre_fader(std::size_t bus_index,
                                           std::size_t frames) const noexcept;
    [[nodiscard]] StereoConstView post_fader(std::size_t bus_index,
                                            std::size_t frames) const noexcept;

private:
    struct RuntimeInsert {
        InsertKind kind{InsertKind::Gain};
        bool enabled{true};
        float gain{1.0F};
    };
    struct RuntimeBus {
        std::string id;
        std::vector<RuntimeInsert> inserts;
        std::vector<float> pre_left;
        std::vector<float> pre_right;
        std::vector<float> post_left;
        std::vector<float> post_right;
    };
    struct RuntimeRoute {
        std::size_t source{};
        std::size_t destination{};
        SendTap tap{SendTap::PostFader};
        float gain{1.0F};
    };

    std::vector<RuntimeBus> buses_;
    std::vector<RuntimeRoute> routes_;
    std::vector<std::size_t> render_order_;
    std::size_t max_frames_{0};
    bool configured_{false};
};

} // namespace hush
