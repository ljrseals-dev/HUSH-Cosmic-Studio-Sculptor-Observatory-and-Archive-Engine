#include "hush/studio_03a.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace hush::studio03a {

bool ArrangementModel::replace(std::vector<TimelineClip> clips) {
    for (const auto& clip : clips) {
        if (clip.id.empty() || clip.track_id.empty() || clip.length_frames == 0 ||
            clip.start_frame > std::numeric_limits<std::uint64_t>::max() - clip.length_frames)
            return false;
    }
    for (std::size_t i = 0; i < clips.size(); ++i)
        for (std::size_t j = i + 1; j < clips.size(); ++j)
            if (clips[i].id == clips[j].id) return false;
    clips_ = std::move(clips);
    return true;
}

bool ArrangementModel::move_clip(std::string_view id, std::uint64_t start) noexcept {
    for (auto& clip : clips_) {
        if (clip.id != id) continue;
        if (start > std::numeric_limits<std::uint64_t>::max() - clip.length_frames) return false;
        clip.start_frame = start;
        return true;
    }
    return false;
}

bool ArrangementModel::resize_clip(std::string_view id, std::uint64_t length) noexcept {
    if (length == 0) return false;
    for (auto& clip : clips_) {
        if (clip.id != id) continue;
        if (clip.start_frame > std::numeric_limits<std::uint64_t>::max() - length) return false;
        clip.length_frames = length;
        return true;
    }
    return false;
}

std::uint64_t ArrangementModel::duration_frames() const noexcept {
    std::uint64_t end = 0;
    for (const auto& clip : clips_) {
        if (clip.start_frame <= std::numeric_limits<std::uint64_t>::max() - clip.length_frames)
            end = std::max(end, clip.start_frame + clip.length_frames);
    }
    return end;
}

bool PluginEditorModel::configure(std::string id, std::vector<PluginParameter> params) {
    if (id.empty()) return false;
    for (std::size_t i = 0; i < params.size(); ++i) {
        const auto& p = params[i];
        if (p.name.empty() || !std::isfinite(p.minimum) || !std::isfinite(p.maximum) ||
            !std::isfinite(p.default_value) || p.minimum > p.maximum ||
            p.default_value < p.minimum || p.default_value > p.maximum) return false;
        for (std::size_t j = i + 1; j < params.size(); ++j)
            if (p.id == params[j].id) return false;
    }
    std::vector<float> values;
    values.reserve(params.size());
    for (const auto& p : params) values.push_back(p.default_value);
    plugin_id_ = std::move(id);
    parameters_ = std::move(params);
    values_ = std::move(values);
    return true;
}

bool PluginEditorModel::set_value(std::uint32_t id, float value) noexcept {
    if (!std::isfinite(value)) return false;
    for (std::size_t i = 0; i < parameters_.size(); ++i) {
        const auto& p = parameters_[i];
        if (p.id != id) continue;
        if (value < p.minimum || value > p.maximum) return false;
        values_[i] = value;
        return true;
    }
    return false;
}

bool PluginEditorModel::value(std::uint32_t id, float& out) const noexcept {
    for (std::size_t i = 0; i < parameters_.size(); ++i) {
        if (parameters_[i].id != id) continue;
        out = values_[i];
        return true;
    }
    return false;
}

void MidiByteParser::reset() noexcept {
    status_ = 0;
    data_[0] = data_[1] = count_ = needed_ = 0;
}

bool MidiByteParser::feed(std::uint8_t byte, MidiEvent& event) noexcept {
    if (byte >= 0xF8U) return false; // Realtime bytes do not disturb running status.
    if ((byte & 0x80U) != 0U) {
        if (byte >= 0xF0U) { reset(); return false; } // SysEx/system-common not in this milestone.
        status_ = byte;
        count_ = 0;
        const auto command = static_cast<std::uint8_t>(status_ & 0xF0U);
        needed_ = (command == 0xCU || command == 0xDU) ? 1U : 2U;
        return false;
    }
    if (status_ == 0U || needed_ == 0U) return false;
    data_[count_++] = static_cast<std::uint8_t>(byte & 0x7FU);
    if (count_ < needed_) return false;
    const auto command = static_cast<std::uint8_t>(status_ & 0xF0U);
    event.channel = static_cast<std::uint8_t>(status_ & 0x0FU);
    event.data1 = data_[0];
    event.data2 = needed_ == 2U ? data_[1] : 0U;
    switch (command) {
    case 0x80U: event.kind = MidiEventKind::NoteOff; break;
    case 0x90U: event.kind = data_[1] == 0U ? MidiEventKind::NoteOff : MidiEventKind::NoteOn; break;
    case 0xB0U: event.kind = MidiEventKind::ControlChange; break;
    case 0xE0U: event.kind = MidiEventKind::PitchBend; break;
    case 0xC0U: event.kind = MidiEventKind::ProgramChange; break;
    default: count_ = 0; return false;
    }
    count_ = 0; // Retain status for MIDI running-status messages.
    return true;
}

bool parse_voice_command(std::string_view text, VoiceAction& out) noexcept {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r' || text.back() == '\n')) text.remove_suffix(1);
    if (text == "play") { out = {VoiceActionKind::Play, {}, 0.0}; return true; }
    if (text == "stop") { out = {VoiceActionKind::Stop, {}, 0.0}; return true; }
    if (text == "record") { out = {VoiceActionKind::Record, {}, 0.0}; return true; }
    constexpr std::string_view mute_prefix = "mute track ";
    if (text.starts_with(mute_prefix) && text.size() > mute_prefix.size()) {
        out = {VoiceActionKind::ToggleMute, std::string(text.substr(mute_prefix.size())), 0.0};
        return true;
    }
    constexpr std::string_view tempo_prefix = "set tempo ";
    if (text.starts_with(tempo_prefix)) {
        const auto number = text.substr(tempo_prefix.size());
        double bpm = 0.0;
        bool dot = false;
        bool digit = false;
        for (const char c : number) {
            if (c >= '0' && c <= '9') {
                digit = true;
                bpm = bpm * 10.0 + static_cast<double>(c - '0');
                if (dot) bpm = bpm; // decimal is validated below by integer-only policy
            } else if (c == '.' && !dot) { dot = true; }
            else return false;
        }
        if (!digit || dot || bpm < 20.0 || bpm > 300.0) return false;
        out = {VoiceActionKind::SetTempo, {}, bpm};
        return true;
    }
    return false;
}

bool RecordingBlockQueue::try_push(const float* input, std::uint32_t frames,
                                   std::uint64_t first_sample) noexcept {
    if (input == nullptr || frames == 0U || frames > StereoCaptureBlock::kMaxFrames) {
        dropped_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    const auto write = write_index_.load(std::memory_order_relaxed);
    const auto read = read_index_.load(std::memory_order_acquire);
    if (write - read >= kCapacity) {
        dropped_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    auto& block = blocks_[static_cast<std::size_t>(write % kCapacity)];
    block.frames = frames;
    block.first_sample = first_sample;
    std::copy_n(input, static_cast<std::size_t>(frames) * 2U, block.interleaved.begin());
    write_index_.store(write + 1U, std::memory_order_release);
    return true;
}

bool RecordingBlockQueue::try_pop(StereoCaptureBlock& out) noexcept {
    const auto read = read_index_.load(std::memory_order_relaxed);
    const auto write = write_index_.load(std::memory_order_acquire);
    if (read == write) return false;
    out = blocks_[static_cast<std::size_t>(read % kCapacity)];
    read_index_.store(read + 1U, std::memory_order_release);
    return true;
}

std::uint64_t RecordingBlockQueue::queued_blocks() const noexcept {
    return write_index_.load(std::memory_order_acquire) -
           read_index_.load(std::memory_order_acquire);
}

} // namespace hush::studio03a
