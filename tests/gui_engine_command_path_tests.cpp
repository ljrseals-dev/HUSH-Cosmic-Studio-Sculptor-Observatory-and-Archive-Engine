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

// Mirrors the production GUI's stable track-ID lookup and bounded command
// submission. The real OperationLog and RealtimeTrackMixer are exercised.
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

// Match the GUI's transactional rule: reserve the command before mutating
// canonical project state or undo history.
bool commit_live_gain(hush::Project& project, hush::OperationLog& history,
                      hush::RealtimeTrackMixer& engine, const std::string& track_id,
                      float after) {
    for (std::size_t i = 0; i < project.tracks.size(); ++i) {
        const auto& track = project.tracks[i];
        if (track.id != track_id) continue;
        if (!engine.enqueue_control(i, after, track.pan, track.mute, track.solo)) return false;
        return history.apply_and_append(project,
            {track_id, hush::OperationKind::SetGain, track.gain, after});
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

    // Slider preview reaches the next block without changing canonical state
    // or adding an undo entry.
    OperationLog history;
    assert(enqueue_preview(project, engine, "track-left", 0.25F, -1.0F));
    assert(project.tracks[0].gain == 1.0F);
    assert(history.size() == 0);
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);
    expect_near(out_r[0], 1.0F);

    // Slider release commits one undoable operation through the same queue.
    assert(commit_live_gain(project, history, engine, "track-left", 0.25F));
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

    // Pan changes are applied at the next processing boundary.
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

    // Queue-full rejection must leave the model and history untouched.
    Project rejected;
    rejected.id = "queue-full-transaction";
    rejected.tracks = {{"only-track", "Only", 1.0F, 0.0F, false, false}};
    OperationLog rejected_history;
    RealtimeTrackMixer full;
    assert(full.configure(1, 2));
    for (std::size_t i = 0; i < RealtimeTrackMixer::kQueueCapacity; ++i) {
        assert(full.enqueue_control(0, 1.0F, 0.0F, false, false));
    }
    assert(!commit_live_gain(rejected, rejected_history, full, "only-track", 0.5F));
    assert(rejected.tracks[0].gain == 1.0F);
    assert(rejected_history.size() == 0 && rejected_history.cursor() == 0);

    // Stable-ID lookup rejects a missing track without queueing a command.
    assert(!enqueue_track(project, engine, "no-such-track"));
    return 0;
}
