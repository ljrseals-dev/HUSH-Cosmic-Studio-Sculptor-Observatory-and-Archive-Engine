#pragma once

#include "hush/clip_renderer.hpp"
#include "hush/bus_renderer.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace hush {

struct ClipBusRenderError {
    std::string code;
    std::string entity_id;
    std::string message;
};

// Offline/control-thread bridge: decoded clip placements -> canonical track
// mixer -> bus direct inputs. Empty Track::bus_id routes to the first project
// bus for compatibility with older schema-v1 projects. This allocates and must
// never be called from the realtime callback.
[[nodiscard]] bool render_project_clips_to_bus_inputs(
    const Project& project,
    const std::vector<DecodedMediaBuffer>& decoded_sources,
    std::size_t output_frames,
    std::vector<BusAudioInput>& bus_inputs,
    ClipBusRenderError& error);

} // namespace hush
