#include "hush/operation_log.hpp"
#include "hush/project.hpp"
#include "hush/realtime_track_mixer.hpp"
#include "hush/track_mixer_command_bridge.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace {
constexpr float kTolerance = 1.0e-5F;

void expect_near(float actual, float expected) {
    assert(std::abs(actual - expected) < kTolerance);
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

    // Startup snapshots are applied at the first processing block.
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 1.0F);
    expect_near(out_r[0], 1.0F);

    // Initial snapshot seeding rejects a topology mismatch before enqueueing
    // any partial state. The current engine output remains unchanged.
    Project mismatched = project;
    mismatched.tracks.pop_back();
    mismatched.tracks[0].gain = 0.1F;
    assert(!TrackMixerCommandBridge::enqueue_project(mismatched, engine));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 1.0F);
    expect_near(out_r[0], 1.0F);

    // GUI drag preview is sent before the project edit is committed. It must
    // change the next block without creating an undo entry or mutating project.
    OperationLog history;
    assert(TrackMixerCommandBridge::enqueue_preview(project, engine, "track-left", 0.25F, -1.0F));
    assert(project.tracks[0].gain == 1.0F);
    assert(history.size() == 0);
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);
    expect_near(out_r[0], 1.0F);

    // Pan drag preview follows the same path: the next block changes, but the
    // project model and undo history remain untouched until slider release.
    assert(TrackMixerCommandBridge::enqueue_preview(project, engine, "track-left", 0.25F, 0.0F));
    assert(project.tracks[0].pan == -1.0F);
    assert(history.size() == 0);
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F * 0.70710678F);
    expect_near(out_r[0], 1.0F + 0.25F * 0.70710678F);

    // Restore the uncommitted pan preview before testing the commit path.
    assert(TrackMixerCommandBridge::enqueue_preview(project, engine, "track-left", 0.25F, -1.0F));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);
    expect_near(out_r[0], 1.0F);

    // Slider release commits one undoable operation and syncs the committed
    // snapshot to the same bounded queue used by the GUI.
    assert(history.apply_and_append(project,
        {"track-left", OperationKind::SetGain, 1.0F, 0.25F}));
    assert(TrackMixerCommandBridge::enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);
    assert(history.size() == 1);
    assert(history.cursor() == 1);

    // Undo and redo each enqueue the resulting project snapshot; the next
    // engine block, not merely the UI model, must reflect each state.
    assert(history.undo(project));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 1.0F);

    assert(history.redo(project));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);

    // Mute and solo controls have the same next-block delivery contract.
    assert(history.apply_and_append(project,
        {"track-left", OperationKind::SetMute, 0.0F, 1.0F}));
    assert(enqueue_track(project, engine, "track-left"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.0F);
    expect_near(out_r[0], 1.0F);

    assert(history.apply_and_append(project,
        {"track-right", OperationKind::SetSolo, 0.0F, 1.0F}));
    assert(TrackMixerCommandBridge::enqueue_track(project, engine, "track-right"));
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.0F);
    expect_near(out_r[0], 1.0F);

    // Queue overflow must fail promptly; it must never block the UI thread.
    RealtimeTrackMixer full;
    assert(full.configure(1, 2));
    for (std::size_t i = 0; i < RealtimeTrackMixer::kQueueCapacity; ++i)
        assert(full.enqueue_control(0, 1.0F, 0.0F, false, false));
    assert(!full.enqueue_control(0, 0.5F, 0.0F, false, false));

    return 0;
}
