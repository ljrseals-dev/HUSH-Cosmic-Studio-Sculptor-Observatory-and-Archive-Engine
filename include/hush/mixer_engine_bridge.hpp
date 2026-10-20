#pragma once

#include "hush/realtime_bus_engine.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace hush {

// Control-thread mapping from stable project track IDs to preconfigured bus indices.
// Configure once on the control thread before exposing the bridge to the GUI.
// The GUI/control thread is the sole producer; RealtimeBusEngine's audio callback
// remains the sole consumer. The realtime thread never reads this mapping.
struct TrackBusBinding {
    std::string track_id;
    std::size_t bus_index{};
};

class MixerEngineBridge {
public:
    // Must be called before the bridge is shared with the GUI. Allocations and
    // validation happen here, never in enqueue_gain() or the audio callback.
    bool configure(RealtimeBusEngine& engine,
                   std::vector<TrackBusBinding> bindings,
                   std::string& error);

    // Control/GUI thread only. Returns false for an unknown track, invalid gain,
    // or a full realtime parameter queue. Does not mutate the bus graph.
    [[nodiscard]] bool enqueue_gain(std::string_view track_id, float gain) noexcept;
    [[nodiscard]] bool ready() const noexcept { return engine_ != nullptr; }

private:
    RealtimeBusEngine* engine_{nullptr};
    std::vector<TrackBusBinding> bindings_;
};

} // namespace hush
