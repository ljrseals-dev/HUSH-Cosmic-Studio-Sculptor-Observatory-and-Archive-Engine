#include "hush/operation_log.hpp"
#include "hush/project.hpp"
#include "hush/realtime_track_mixer.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <string>

namespace {
constexpr float kTolerance = 1.0e-5F;

void expect_near(float actual, float expected) {
    assert(std::abs(actual - expected) < kTolerance);
}

// Mirrors the GUI's stable-track-ID lookup and bounded command submission.
// The production OperationLog and RealtimeTrackMixer are exercised below.
bool enqueue_track(const hush::Project& project, hush::RealtimeTrackMixer& engine,
                   const std::string& track_id) {
    for (std::size_t i = 0; i < project.tracks.size(); ++i) {
        const auto& track = project.tracks[i];
        if (track.id == track_id) {
            return engine.enqueue_control(i, track.gain, track.pan, track.mute, track.solo);
        }
    }
    return false;
}

bool enqueue_preview(const hush::Project& project, hush::RealtimeTrackMixer& engine,
                     const std::string& track_id, float gain, float pan) {
    for (std::size_t i = 0; i < project.tracks.size(); ++i) {
        const auto& track = project.tracks[i];
        if (track.id == track_id) {
            return engine.enqueue_control(i, gain, pan, track.mute, track.solo);
        }
    }
    return false;
}
} // namespace

int main() {
    using namespace hush;

    Project project;
    project.id = "gui-engine-contract";
    project.tracks = {
        {"track-left", "Left", 1.0F, -1.0F, false, false},
        {"track-right", "Right", 1.0F, 1.0F, false, false},
    };

    RealtimeTrackMixer engine;
    assert(engine.configure(project.tracks.size(), 4));
    for (std::size_t i = 0; i < project.tracks.size(); ++i) {
        const auto& t = project.tracks[i];
        assert(engine.enqueue_control(i, t.gain, t.pan, t.mute, t.solo));
    }

    const float left_input[] = {1.0F, 1.0F};
    const float right_input[] = {1.0F, 1.0F};
    const float* inputs[] = {left_input, right_input};
    float out_l[2]{};
    float out_r[2]{};

    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 1.0F);
    expect_near(out_r[0], 1.0F);

    // A slider preview reaches the next block without changing canonical
    // project state or adding an undo operation.
    OperationLog history;
    assert(enqueue_preview(project, engine, "track-left", 0.25F, -1.0F));
    assert(project.tracks[0].gain == 1.0F);
    assert(history.size() == 0);
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);
    expect_near(out_r[0], 1.0F);

    // Slider release commits one undoable operation and synchronizes its
    // canonical value through the same command queue.
    assert(history.apply_and_append(project,
        {"track-left", OperationKind::SetGain, 1.0F, 0.25F}));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);
    assert(history.size() == 1 && history.cursor() == 1);

    assert(history.undo(project));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 1.0F);

    assert(history.redo(project));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);

    // Pan edits also reach the processing block. Centering the left track
    // contributes equal-power signal to both output channels.
    assert(history.apply_and_append(project,
        {"track-left", OperationKind::SetPan, -1.0F, 0.0F}));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F * 0.70710678F);
    expect_near(out_r[0], 1.0F + 0.25F * 0.70710678F);

    assert(history.undo(project));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);
    expect_near(out_r[0], 1.0F);

    assert(history.redo(project));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F * 0.70710678F);
    expect_near(out_r[0], 1.0F + 0.25F * 0.70710678F);

    assert(history.apply_and_append(project,
        {"track-left", OperationKind::SetMute, 0.0F, 1.0F}));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.0F);
    expect_near(out_r[0], 1.0F);

    assert(history.apply_and_append(project,
        {"track-right", OperationKind::SetSolo, 0.0F, 1.0F}));
    assert(enqueue_track(project, engine, "track-right"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.0F);
    expect_near(out_r[0], 1.0F);

    // A full bounded queue rejects promptly instead of blocking or overwriting.
    RealtimeTrackMixer full;
    assert(full.configure(1, 2));
    for (std::size_t i = 0; i < RealtimeTrackMixer::kQueueCapacity; ++i)
        assert(full.enqueue_control(0, 1.0F, 0.0F, false, false));
    assert(!full.enqueue_control(0, 0.5F, 0.0F, false, false));

    return 0;
}
