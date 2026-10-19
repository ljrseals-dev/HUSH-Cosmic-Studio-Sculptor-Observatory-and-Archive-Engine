#pragma once

#include "hush/realtime_bus_engine.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace hush {

// Control-thread mapping from stable project track IDs to preconfigured bus indices.
// Configure before publishing the bridge to the GUI. The GUI is the single producer;
// the audio callback remains the single consumer through RealtimeBusEngine's SPSC queue.
// This first gate intentionally forwards gain only: pan/mute/solo need matching DSP
// semantics and are not falsely reported as live until that engine work is complete.
struct TrackBusBinding {
    std::string track_id;
    std::size_t bus_index{};
};

class MixerEngineBridge {
public:
    bool configure(RealtimeBusEngine& engine,
                   std::vector<TrackBusBinding> bindings,
                   std::string& error);

    // GUI/control thread only. No graph mutation and no blocking.
    [[nodiscard]] bool enqueue_gain(std::string_view track_id, float gain) noexcept;
    [[nodiscard]] bool ready() const noexcept { return engine_ != nullptr; }

private:
    RealtimeBusEngine* engine_{nullptr};
    std::vector<TrackBusBinding> bindings_;
};

} // namespace hush
