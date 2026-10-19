#include "hush/mixer_engine_bridge.hpp"

#include <algorithm>
#include <utility>

namespace hush {

bool MixerEngineBridge::configure(RealtimeBusEngine& engine,
                                  std::vector<TrackBusBinding> bindings,
                                  std::string& error) {
    if (engine.bus_count() == 0) {
        error = "RealtimeBusEngine must be configured before the mixer bridge";
        return false;
    }
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (bindings[i].track_id.empty()) {
            error = "Track IDs in the bridge mapping must not be empty";
            return false;
        }
        if (bindings[i].bus_index >= engine.bus_count()) {
            error = "Track mapping references a bus index outside the configured engine";
            return false;
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (bindings[j].track_id == bindings[i].track_id) {
                error = "Track IDs in the bridge mapping must be unique";
                return false;
            }
        }
    }
    bindings_ = std::move(bindings);
    engine_ = &engine;
    error.clear();
    return true;
}

bool MixerEngineBridge::enqueue_gain(std::string_view track_id, float gain) noexcept {
    if (engine_ == nullptr) return false;
    const auto found = std::find_if(bindings_.begin(), bindings_.end(),
        [track_id](const TrackBusBinding& binding) {
            return binding.track_id == track_id;
        });
    if (found == bindings_.end()) return false;
    return engine_->enqueue_fader_change(found->bus_index, gain);
}

} // namespace hush
