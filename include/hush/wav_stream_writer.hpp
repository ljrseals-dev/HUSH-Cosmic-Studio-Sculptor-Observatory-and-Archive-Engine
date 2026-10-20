#pragma once
#include "hush/wav_writer.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
namespace hush {
// Incremental WAV writer for a non-realtime worker thread. Never call from an audio callback.
class WavStreamWriter {
public:
    WavStreamWriter() = default;
    ~WavStreamWriter();
    WavStreamWriter(const WavStreamWriter&) = delete;
    WavStreamWriter& operator=(const WavStreamWriter&) = delete;
    [[nodiscard]] bool start(const std::filesystem::path&, std::uint32_t, std::uint16_t,
                             WavEncoding = WavEncoding::Float32, std::string* error = nullptr);
    [[nodiscard]] bool append(std::span<const float>, std::string* error = nullptr);
    [[nodiscard]] bool finalize(std::string* error = nullptr);
    [[nodiscard]] bool is_open() const noexcept { return out_.is_open(); }
    [[nodiscard]] std::uint64_t frames_written() const noexcept { return frames_written_; }
private:
    bool write_header(std::uint32_t, std::string*);
    std::ofstream out_;
    std::filesystem::path path_;
    std::uint32_t sample_rate_{0};
    std::uint16_t channels_{0};
    WavEncoding encoding_{WavEncoding::Float32};
    std::uint64_t data_bytes_{0}, frames_written_{0};
};
} // namespace hush
