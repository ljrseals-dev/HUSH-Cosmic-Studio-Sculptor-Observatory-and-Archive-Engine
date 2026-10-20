#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hush::studio03a {

struct TimelineClip {
    std::string id;
    std::string track_id;
    std::uint64_t start_frame{};
    std::uint64_t length_frames{};
    std::uint64_t source_offset_frames{};
    bool muted{false};
};

class ArrangementModel {
public:
    [[nodiscard]] bool replace(std::vector<TimelineClip> clips);
    [[nodiscard]] bool move_clip(std::string_view id, std::uint64_t new_start_frame) noexcept;
    [[nodiscard]] bool resize_clip(std::string_view id, std::uint64_t new_length_frames) noexcept;
    [[nodiscard]] const std::vector<TimelineClip>& clips() const noexcept { return clips_; }
    [[nodiscard]] std::uint64_t duration_frames() const noexcept;
private:
    std::vector<TimelineClip> clips_;
};

struct PluginParameter {
    std::uint32_t id{};
    std::string name;
    float minimum{};
    float maximum{1.0F};
    float default_value{};
    bool automatable{true};
};

class PluginEditorModel {
public:
    [[nodiscard]] bool configure(std::string plugin_id, std::vector<PluginParameter> parameters);
    [[nodiscard]] bool set_value(std::uint32_t parameter_id, float value) noexcept;
    [[nodiscard]] bool value(std::uint32_t parameter_id, float& out) const noexcept;
    [[nodiscard]] const std::string& plugin_id() const noexcept { return plugin_id_; }
    [[nodiscard]] const std::vector<PluginParameter>& parameters() const noexcept { return parameters_; }
private:
    std::string plugin_id_;
    std::vector<PluginParameter> parameters_;
    std::vector<float> values_;
};

enum class MidiEventKind : std::uint8_t { NoteOff, NoteOn, ControlChange, PitchBend, ProgramChange };
struct MidiEvent {
    MidiEventKind kind{};
    std::uint8_t channel{};
    std::uint8_t data1{};
    std::uint8_t data2{};
};

class MidiByteParser {
public:
    // Feed MIDI 1.0 bytes. Returns true exactly when one complete channel event is emitted.
    [[nodiscard]] bool feed(std::uint8_t byte, MidiEvent& event) noexcept;
    void reset() noexcept;
private:
    std::uint8_t status_{};
    std::uint8_t data_[2]{};
    std::uint8_t count_{};
    std::uint8_t needed_{};
};

enum class VoiceActionKind : std::uint8_t { Play, Stop, Record, ToggleMute, SetTempo };
struct VoiceAction {
    VoiceActionKind kind{};
    std::string target;
    double number{};
};
// Deterministic text-command parser. It does not capture audio or perform speech recognition.
[[nodiscard]] bool parse_voice_command(std::string_view text, VoiceAction& out) noexcept;

struct StereoCaptureBlock {
    static constexpr std::size_t kMaxFrames = 1024;
    std::uint32_t frames{};
    std::uint64_t first_sample{};
    std::array<float, kMaxFrames * 2> interleaved{};
};

// Single-producer (audio callback), single-consumer (recording writer) bounded queue.
// No allocations, locks, file I/O, or waiting occur in try_push().
class RecordingBlockQueue {
public:
    static constexpr std::size_t kCapacity = 8;
    [[nodiscard]] bool try_push(const float* interleaved, std::uint32_t frames,
                                std::uint64_t first_sample) noexcept;
    [[nodiscard]] bool try_pop(StereoCaptureBlock& out) noexcept;
    [[nodiscard]] std::uint64_t dropped_blocks() const noexcept {
        return dropped_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t queued_blocks() const noexcept;
private:
    std::array<StereoCaptureBlock, kCapacity> blocks_{};
    alignas(64) std::atomic<std::uint64_t> write_index_{0};
    alignas(64) std::atomic<std::uint64_t> read_index_{0};
    std::atomic<std::uint64_t> dropped_{0};
};

} // namespace hush::studio03a
