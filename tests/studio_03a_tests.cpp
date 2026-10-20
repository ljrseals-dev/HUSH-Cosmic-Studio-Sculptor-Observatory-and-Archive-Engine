#include "hush/studio_03a.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

int main() {
    using namespace hush::studio03a;

    ArrangementModel arrangement;
    assert(arrangement.replace({{"clip-a", "track-1", 100, 200, 0, false},
                                {"clip-b", "track-2", 500, 100, 0, false}}));
    assert(arrangement.duration_frames() == 600);
    assert(arrangement.move_clip("clip-a", 250));
    assert(arrangement.resize_clip("clip-b", 300));
    assert(arrangement.duration_frames() == 800);
    assert(!arrangement.replace({{"same", "t", 0, 1, 0, false},
                                 {"same", "t", 2, 1, 0, false}}));
    assert(arrangement.duration_frames() == 800); // Failed replacement is transactional.

    PluginEditorModel editor;
    assert(editor.configure("hush.eq", {{1, "Frequency", 20, 20000, 1000, true},
                                        {2, "Gain", -24, 24, 0, true}}));
    assert(editor.set_value(2, 3.5F));
    float gain = 0;
    assert(editor.value(2, gain) && std::abs(gain - 3.5F) < 1e-6F);
    assert(!editor.set_value(2, 100));
    assert(!editor.configure("bad", {{1, "a", 1, 0, 0, true}}));
    assert(editor.value(2, gain) && std::abs(gain - 3.5F) < 1e-6F);

    MidiByteParser midi;
    MidiEvent event{};
    assert(!midi.feed(0x92, event));
    assert(!midi.feed(60, event));
    assert(midi.feed(100, event));
    assert(event.kind == MidiEventKind::NoteOn && event.channel == 2 &&
           event.data1 == 60 && event.data2 == 100);
    assert(!midi.feed(61, event));
    assert(midi.feed(0, event)); // Running-status note-on with velocity zero is note-off.
    assert(event.kind == MidiEventKind::NoteOff && event.data1 == 61);
    assert(!midi.feed(0xF0, event)); // Unsupported system message resets parser.
    assert(!midi.feed(60, event));

    VoiceAction action{};
    assert(parse_voice_command(" play ", action) && action.kind == VoiceActionKind::Play);
    assert(parse_voice_command("mute track vocal-1", action) &&
           action.kind == VoiceActionKind::ToggleMute && action.target == "vocal-1");
    assert(parse_voice_command("set tempo 128", action) &&
           action.kind == VoiceActionKind::SetTempo && action.number == 128);
    assert(!parse_voice_command("set tempo 999", action));
    assert(!parse_voice_command("delete everything", action));

    RecordingBlockQueue queue;
    std::vector<float> samples(256, 0.25F);
    for (std::size_t i = 0; i < RecordingBlockQueue::kCapacity; ++i)
        assert(queue.try_push(samples.data(), 128, i * 128));
    assert(!queue.try_push(samples.data(), 128, 9999));
    assert(queue.dropped_blocks() == 1);
    StereoCaptureBlock block{};
    assert(queue.try_pop(block) && block.frames == 128 && block.first_sample == 0);
    assert(std::abs(block.interleaved[0] - 0.25F) < 1e-6F);
    assert(queue.queued_blocks() == RecordingBlockQueue::kCapacity - 1);
    assert(!queue.try_push(samples.data(), 0, 0));
    assert(queue.dropped_blocks() == 2);
    return 0;
}
