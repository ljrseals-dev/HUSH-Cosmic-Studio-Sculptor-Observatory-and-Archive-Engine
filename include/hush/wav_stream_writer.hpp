#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
namespace hush {
// Incremental IEEE-float WAV writer. Disk I/O is worker-thread-only.
class WavStreamWriter {
public:
 WavStreamWriter() = default;
 WavStreamWriter(const WavStreamWriter&) = delete;
 WavStreamWriter& operator=(const WavStreamWriter&) = delete;
 ~WavStreamWriter();
 [[nodiscard]] bool open(const std::filesystem::path&, std::uint32_t, std::uint16_t, std::string* = nullptr);
 [[nodiscard]] bool write_frames(const float*, std::uint64_t, std::string* = nullptr);
 [[nodiscard]] bool finalize(std::string* = nullptr);
 [[nodiscard]] std::uint64_t frames_written() const noexcept { return frames_written_; }
private:
 std::fstream file_;
 std::uint32_t sample_rate_{0};
 std::uint16_t channels_{0};
 std::uint64_t frames_written_{0};
 bool failed_{false};
};
}