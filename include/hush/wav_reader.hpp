#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace hush {

// Fully decoded interleaved audio. Loading performs file I/O and allocates memory;
// use only on an import/control thread, never in the realtime audio callback.
struct WavAudioFile {
    std::uint32_t sample_rate{0};
    std::uint16_t channels{0};
    std::vector<float> interleaved_samples;

    [[nodiscard]] std::uint64_t frame_count() const noexcept {
        return channels == 0U ? 0U :
            static_cast<std::uint64_t>(interleaved_samples.size() / channels);
    }
};

struct WavReadOptions {
    // Prevent accidental loading of huge/untrusted files into memory.
    std::uint64_t max_data_bytes{512ULL * 1024ULL * 1024ULL};
};

// Reads RIFF/WAVE PCM16 or IEEE float32 files, including files with ancillary
// chunks before/after the data chunk. Does not resample or remix channels.
[[nodiscard]] bool read_wav(const std::filesystem::path& path,
                            WavAudioFile& out,
                            std::string* error = nullptr,
                            WavReadOptions options = {});

} // namespace hush
