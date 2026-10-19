#include "hush/operation_log.hpp"
#include "hush/project.hpp"
#include "hush/realtime_track_mixer.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace {
constexpr float kTolerance = 1.0e-5F;

void expect_near(float actual, float expected) {
    assert(std::abs(actual - expected) < kTolerance);
}

// Mirrors the GUI's track-id lookup and bounded command submission. The audio
// thread observes these snapshots only when process() begins the next block.
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

    // Startup snapshots are applied at the first processing block.
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 1.0F);
    expect_near(out_r[0], 1.0F);

    // GUI drag preview is sent before the project edit is committed. It must
    // change the next block without creating an undo entry or mutating project.
    OperationLog history;
    assert(enqueue_preview(project, engine, "track-left", 0.25F, -1.0F));
    assert(project.tracks[0].gain == 1.0F);
    assert(history.size() == 0);
    assert(engine.process(inputs, 2, out_l, out_r, 2));
    expect_near(out_l[0], 0.25F);
    expect_near(out_r[0], 1.0F);

    // Slider release commits one undoable operation and syncs the committed
    // snapshot to the same bounded queue used by the GUI.
    assert(history.apply_and_append(project,
        {"track-left", OperationKind::SetGain, 1.0F, 0.25F}));
    assert(enqueue_track(project, engine, "track-left"));
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
    assert(enqueue_track(project, engine, "track-right"));
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
