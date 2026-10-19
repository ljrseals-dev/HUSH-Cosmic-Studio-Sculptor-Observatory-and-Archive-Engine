#include "hush/project_wav_export.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace hush {
namespace {

bool fail(ProjectWavExportError& error, const char* code,
          const std::string& entity_id, const char* message) {
    error = {code, entity_id, message};
    return false;
}

} // namespace

bool render_project_bus_to_wav(
    const Project& project,
    const std::vector<BusAudioInput>& bus_inputs,
    const std::string& output_bus_id,
    const std::filesystem::path& output_path,
    std::uint32_t sample_rate,
    WavEncoding encoding,
    ProjectWavExportError& error) {
    error = {};
    if (project.schema_version != 1U)
        return fail(error, "project_schema_unsupported", project.id,
                    "Only schema-v1 projects can be exported.");
    if (project.buses.empty())
        return fail(error, "project_buses_missing", project.id,
                    "Project has no bus graph to render.");
    if (output_bus_id.empty())
        return fail(error, "output_bus_missing", "", "An output bus ID is required.");

    BusGraph graph;
    RoutingError routing_error;
    if (!graph.configure(project.buses, project.routes, routing_error))
        return fail(error, routing_error.code.c_str(), routing_error.entity_id,
                    routing_error.message.c_str());

    bool output_exists = false;
    for (const auto& bus : project.buses) {
        if (bus.id == output_bus_id) {
            output_exists = true;
            break;
        }
    }
    if (!output_exists)
        return fail(error, "output_bus_unknown", output_bus_id,
                    "Selected output bus does not exist in the project.");

    std::vector<RenderedBus> rendered;
    BusRenderError render_error;
    if (!render_bus_graph(graph, bus_inputs, rendered, render_error))
        return fail(error, render_error.code.c_str(), render_error.entity_id,
                    render_error.message.c_str());

    const RenderedBus* selected = nullptr;
    for (const auto& bus : rendered) {
        if (bus.bus_id == output_bus_id) {
            selected = &bus;
            break;
        }
    }
    if (selected == nullptr)
        return fail(error, "render_output_missing", output_bus_id,
                    "Renderer did not produce the selected output bus.");

    if (selected->post_fader.left.size() != selected->post_fader.right.size())
        return fail(error, "render_channel_length_mismatch", output_bus_id,
                    "Rendered output channels have different frame counts.");
    if (selected->post_fader.left.size() >
        std::numeric_limits<std::size_t>::max() / 2U)
        return fail(error, "render_too_large", output_bus_id,
                    "Rendered stereo output is too large to interleave.");

    std::vector<float> interleaved;
    interleaved.reserve(selected->post_fader.left.size() * 2U);
    for (std::size_t frame = 0; frame < selected->post_fader.left.size(); ++frame) {
        interleaved.push_back(selected->post_fader.left[frame]);
        interleaved.push_back(selected->post_fader.right[frame]);
    }

    std::string write_error;
    if (!write_wav(output_path, interleaved, sample_rate, 2U, encoding, &write_error))
        return fail(error, "wav_write_failed", output_path.string(), write_error.c_str());

    error = {};
    return true;
}

} // namespace hush
