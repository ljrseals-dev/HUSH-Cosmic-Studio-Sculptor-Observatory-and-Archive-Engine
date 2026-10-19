#pragma once
#include "hush/bus_graph.hpp"
#include "hush/mixer.hpp"
#include <string>
#include <vector>

namespace hush {

// One bus' direct input, before its fader. All channels must have equal frame counts.
struct BusAudioInput {
    std::string bus_id;
    StereoBuffer audio;
    float fader_gain{1.0F};
};

// Both tap points are exposed so downstream sends can choose pre- or post-fader audio.
struct RenderedBus {
    std::string bus_id;
    StereoBuffer pre_fader;
    StereoBuffer post_fader;
};

struct BusRenderError {
    std::string code;
    std::string entity_id;
    std::string message;
};

// Offline/reference renderer. Validates inputs and allocates buffers; NOT real-time safe.
// Build the graph and call this on a non-audio thread until a preallocated engine is implemented.
bool render_bus_graph(const BusGraph& graph,
                      const std::vector<BusAudioInput>& inputs,
                      std::vector<RenderedBus>& outputs,
                      BusRenderError& error);

} // namespace hush
