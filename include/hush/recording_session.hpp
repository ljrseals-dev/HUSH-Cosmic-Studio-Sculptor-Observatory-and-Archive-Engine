#pragma once
#include "hush/recording_ring_buffer.hpp"
#include "hush/wav_stream_writer.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace hush {
// Construct before audio starts. capture() is callback-safe; start/drain/stop do I/O or allocate.
class RecordingSession {
public:
 RecordingSession(std::uint16_t channels,std::size_t ring_capacity_frames,std::size_t worker_chunk_frames);
 RecordingSession(const RecordingSession&)=delete;RecordingSession& operator=(const RecordingSession&)=delete;
 [[nodiscard]] bool start(const std::filesystem::path&,std::uint32_t,WavEncoding=WavEncoding::Float32,std::string* error=nullptr);
 [[nodiscard]] bool capture(std::span<const float>)noexcept;
 [[nodiscard]] bool drain(std::string* error=nullptr);
 [[nodiscard]] bool stop(std::string* error=nullptr);
 [[nodiscard]] bool is_recording()const noexcept{return recording_;}
 [[nodiscard]] std::uint64_t dropped_frames()const noexcept{return ring_.dropped_frames();}
 [[nodiscard]] std::uint64_t frames_written()const noexcept{return writer_.frames_written();}
 [[nodiscard]] std::uint16_t channels()const noexcept{return ring_.channels();}
private: RecordingRingBuffer ring_;std::vector<float> worker_buffer_;WavStreamWriter writer_;bool recording_{false};
};
} // namespace hush
