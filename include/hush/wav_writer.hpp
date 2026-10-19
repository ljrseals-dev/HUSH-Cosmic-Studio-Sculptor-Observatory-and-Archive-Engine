#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace hush {

enum class WavEncoding {
    Pcm16,
    Float32
};

// Writes interleaved audio to a standard RIFF/WAVE file.
// This is an offline/export-thread API; never call it from an audio callback.
[[nodiscard]] bool write_wav(const std::filesystem::path& path,
                             std::span<const float> interleaved_samples,
                             std::uint32_t sample_rate,
                             std::uint16_t channels,
                             WavEncoding encoding = WavEncoding::Pcm16,
                             std::string* error = nullptr);

} // namespace hush
