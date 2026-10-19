#pragma once

#include "hush/bus_renderer.hpp"
#include "hush/project.hpp"
#include "hush/wav_writer.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace hush {

struct ProjectWavExportError {
    std::string code;
    std::string entity_id;
    std::string message;
};

// Render a schema-v1 project's configured bus graph and export the selected bus
// as interleaved stereo WAV. Bus inputs are already decoded/prepared audio buffers.
// This is an offline control-thread API: it allocates memory and performs file I/O.
[[nodiscard]] bool render_project_bus_to_wav(
    const Project& project,
    const std::vector<BusAudioInput>& bus_inputs,
    const std::string& output_bus_id,
    const std::filesystem::path& output_path,
    std::uint32_t sample_rate,
    WavEncoding encoding,
    ProjectWavExportError& error);

} // namespace hush
