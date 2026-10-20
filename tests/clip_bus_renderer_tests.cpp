#include "hush/clip_bus_renderer.hpp"
#include "hush/bus_graph.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

int main() {
    using namespace hush;
    Project project;
    project.id = "clip-bus-routing";
    project.tracks = {
        {"left-track", "Left", 0.5F, -1.0F, false, false, "bus-left"},
        {"right-track", "Right", 1.0F, 1.0F, false, false, "bus-right"}
    };
    project.buses = {
        {"bus-left", "Left bus"},
        {"bus-right", "Right bus"},
        {"master", "Master"}
    };
    project.routes = {
        {"bus-left", "master", SendTap::PostFader, 1.0F},
        {"bus-right", "master", SendTap::PostFader, 1.0F}
    };
    project.media_sources = {
        {"left-source", "left.wav", 48000U, 1U, 4U},
        {"right-source", "right.wav", 48000U, 1U, 4U}
    };
    project.clips = {
        {"left-clip", "left-track", "left-source", 0, 0, 4, 1.0F, 0, 0},
        {"right-clip", "right-track", "right-source", 0, 0, 4, 1.0F, 0, 0}
    };
    const std::vector<DecodedMediaBuffer> decoded = {
        {"left-source", 1U, {1.0F, 2.0F, 3.0F, 4.0F}},
        {"right-source", 1U, {10.0F, 20.0F, 30.0F, 40.0F}}
    };

    // Track bus assignments are persisted in schema-v1 and round-trip.
    Project loaded;
    Error project_error;
    assert(from_json(to_json(project), loaded, project_error));
    assert(loaded == project);

    std::vector<BusAudioInput> inputs;
    ClipBusRenderError clip_error;
    assert(render_project_clips_to_bus_inputs(project, decoded, 4U, inputs, clip_error));
    assert(inputs.size() == 3U);
    assert(inputs[0].bus_id == "bus-left");
    assert(inputs[1].bus_id == "bus-right");
    for (std::size_t i = 0; i < 4U; ++i) {
        assert(std::abs(inputs[0].audio.left[i] - (static_cast<float>(i) + 1.0F) * 0.5F) < 1.0e-6F);
        assert(std::abs(inputs[0].audio.right[i]) < 1.0e-6F);
        assert(std::abs(inputs[1].audio.left[i]) < 1.0e-6F);
        assert(std::abs(inputs[1].audio.right[i] - (static_cast<float>(i) + 1.0F) * 10.0F) < 1.0e-6F);
    }

    BusGraph graph;
    RoutingError routing_error;
    assert(graph.configure(project.buses, project.routes, routing_error));
    std::vector<RenderedBus> rendered;
    BusRenderError bus_error;
    assert(render_bus_graph(graph, inputs, rendered, bus_error));
    const auto master = std::find_if(rendered.begin(), rendered.end(),
        [](const RenderedBus& bus) { return bus.bus_id == "master"; });
    assert(master != rendered.end());
    for (std::size_t i = 0; i < 4U; ++i) {
        assert(std::abs(master->pre_fader.left[i] - (static_cast<float>(i) + 1.0F) * 0.5F) < 1.0e-6F);
        assert(std::abs(master->pre_fader.right[i] - (static_cast<float>(i) + 1.0F) * 10.0F) < 1.0e-6F);
    }

    auto invalid = project;
    invalid.tracks[0].bus_id = "missing-bus";
    assert(!render_project_clips_to_bus_inputs(invalid, decoded, 4U, inputs, clip_error));
    assert(clip_error.code == "track_bus_missing");

    // Legacy schema-v1 projects with no bus_id retain first-bus routing.
    const std::string legacy = R"({"schema_version":1,"id":"legacy","pan_law":"equal_power_minus_3_db","tracks":[{"id":"t","name":"T","gain":1,"pan":0,"mute":false,"solo":false}],"buses":[{"id":"master","name":"Master"}],"routes":[]})";
    assert(from_json(legacy, loaded, project_error));
    assert(loaded.tracks.front().bus_id.empty());
    return 0;
}
