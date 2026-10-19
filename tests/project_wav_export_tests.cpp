#include "hush/project_wav_export.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
std::uint32_t u32(const std::vector<unsigned char>& b, std::size_t i) {
    return static_cast<std::uint32_t>(b.at(i)) |
           (static_cast<std::uint32_t>(b.at(i + 1U)) << 8U) |
           (static_cast<std::uint32_t>(b.at(i + 2U)) << 16U) |
           (static_cast<std::uint32_t>(b.at(i + 3U)) << 24U);
}
std::uint16_t u16(const std::vector<unsigned char>& b, std::size_t i) {
    return static_cast<std::uint16_t>(b.at(i)) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b.at(i + 1U)) << 8U);
}
std::vector<unsigned char> read_bytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
}

int main() {
    using namespace hush;
    const auto path = std::filesystem::temp_directory_path() / "hush_project_export_contract.wav";
    Project project;
    project.id = "export-contract";
    project.buses = {{"source", "Source"}, {"master", "Master"}};
    project.routes = {{"source", "master", SendTap::PostFader, 1.0F}};

    const std::vector<BusAudioInput> inputs{
        {"source", {{0.25F, -0.5F}, {0.5F, -0.25F}}, 0.5F},
        {"master", {{0.0F, 0.0F}, {0.0F, 0.0F}}, 1.0F}
    };
    ProjectWavExportError error;
    assert(render_project_bus_to_wav(project, inputs, "master", path, 48000U,
                                     WavEncoding::Float32, error));
    const auto bytes = read_bytes(path);
    assert(bytes.size() == 60U);
    assert(std::string(bytes.begin(), bytes.begin() + 4) == "RIFF");
    assert(std::string(bytes.begin() + 8, bytes.begin() + 12) == "WAVE");
    assert(u32(bytes, 24U) == 48000U);
    assert(u16(bytes, 22U) == 2U);
    assert(u16(bytes, 34U) == 32U);
    assert(u32(bytes, 40U) == 16U);
    // source post-fader = [0.125, -0.25] L and [0.25, -0.125] R; sent to master.
    const auto read_f32 = [&](std::size_t at) {
        const std::uint32_t bits = u32(bytes, at);
        float value{};
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    };
    assert(std::abs(read_f32(44U) - 0.125F) < 1.0e-6F);
    assert(std::abs(read_f32(48U) - 0.25F) < 1.0e-6F);
    assert(std::abs(read_f32(52U) + 0.25F) < 1.0e-6F);
    assert(std::abs(read_f32(56U) + 0.125F) < 1.0e-6F);

    assert(!render_project_bus_to_wav(project, inputs, "missing", path, 48000U,
                                      WavEncoding::Pcm16, error));
    assert(error.code == "output_bus_unknown");
    Project empty;
    assert(!render_project_bus_to_wav(empty, {}, "master", path, 48000U,
                                       WavEncoding::Pcm16, error));
    assert(error.code == "project_buses_missing");

    std::filesystem::remove(path);
    return 0;
}
