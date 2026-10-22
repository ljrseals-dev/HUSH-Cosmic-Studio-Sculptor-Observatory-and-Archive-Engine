#define HUSH_GUI_TESTING 1
#include "../src/mixer_gui.cpp"

#include <QCheckBox>
#include <QMetaObject>
#include <QPushButton>
#include <QSlider>
#include <cassert>
#include <cmath>
#include <array>
#include <string>

namespace {
bool near(float actual, float expected) {
    return std::abs(actual - expected) < 1.0e-5F;
}

class FakeLiveSink final : public hush::LiveTrackControlSink {
public:
    FakeLiveSink() {
        configured = mixer.configure(2, 1);
        inputs = {left_input.data(), right_input.data()};
    }

    bool running() const noexcept override { return configured; }

    bool enqueue_track_control_values(const hush::Project& project, const std::string& track_id,
                                      float gain, float pan, bool mute, bool solo) noexcept override {
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            if (project.tracks[i].id == track_id)
                return mixer.enqueue_control(i, gain, pan, mute, solo);
        }
        return false;
    }

    bool process_one_block() noexcept {
        return mixer.process(inputs.data(), inputs.size(), output_left.data(), output_right.data(), 1);
    }

    bool configured{false};
    hush::RealtimeTrackMixer mixer;
    std::array<float, 1> left_input{1.0F};
    std::array<float, 1> right_input{0.0F};
    std::array<const float*, 2> inputs{};
    std::array<float, 1> output_left{};
    std::array<float, 1> output_right{};
};

QSlider* slider(MixerWindow& window, const QString& name) {
    return window.findChild<QSlider*>(name);
}
QCheckBox* check_box(MixerWindow& window, const QString& name) {
    return window.findChild<QCheckBox*>(name);
}
QPushButton* button(MixerWindow& window, const QString& name) {
    return window.findChild<QPushButton*>(name);
}
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    FakeLiveSink live_sink;
    assert(live_sink.configured);
    MixerWindow window(&live_sink);

    auto* gain = slider(window, "gain-track-1");
    assert(gain != nullptr);
    assert(near(window.test_project().tracks[0].gain, 1.0F));
    assert(window.test_history_cursor() == 0U);

    // Moving a slider previews a candidate value but does not mutate the model
    // until the actual QSlider::sliderReleased signal commits one undo operation.
    gain->setValue(50);
    assert(near(window.test_project().tracks[0].gain, 1.0F));
    assert(window.test_history_cursor() == 0U);
    assert(live_sink.process_one_block());
    assert(near(live_sink.output_left[0], 0.5F));
    assert(near(live_sink.output_right[0], 0.0F));
    assert(QMetaObject::invokeMethod(gain, "sliderReleased", Qt::DirectConnection));
    assert(near(window.test_project().tracks[0].gain, 0.5F));
    assert(window.test_history_cursor() == 1U);
    assert(live_sink.process_one_block());
    assert(near(live_sink.output_left[0], 0.5F));

    auto* undo = button(window, "undoButton");
    assert(undo != nullptr);
    undo->click();
    assert(near(window.test_project().tracks[0].gain, 1.0F));
    assert(window.test_history_cursor() == 0U);
    assert(live_sink.process_one_block());
    assert(near(live_sink.output_left[0], 1.0F));

    auto* redo = button(window, "redoButton");
    assert(redo != nullptr);
    redo->click();
    assert(near(window.test_project().tracks[0].gain, 0.5F));
    assert(window.test_history_cursor() == 1U);
    assert(live_sink.process_one_block());
    assert(near(live_sink.output_left[0], 0.5F));

    auto* pan = slider(window, "pan-track-1");
    assert(pan != nullptr);
    pan->setValue(0);
    assert(near(window.test_project().tracks[0].pan, -1.0F));
    assert(live_sink.process_one_block());
    assert(near(live_sink.output_left[0], 0.5F * 0.70710678F));
    assert(near(live_sink.output_right[0], 0.5F * 0.70710678F));
    assert(QMetaObject::invokeMethod(pan, "sliderReleased", Qt::DirectConnection));
    assert(near(window.test_project().tracks[0].pan, 0.0F));
    assert(window.test_history_cursor() == 2U);

    auto* mute = check_box(window, "mute-track-1");
    assert(mute != nullptr);
    mute->click();
    assert(window.test_project().tracks[0].mute);
    assert(window.test_history_cursor() == 3U);
    assert(live_sink.process_one_block());
    assert(near(live_sink.output_left[0], 0.0F));

    undo->click();
    assert(!window.test_project().tracks[0].mute);
    assert(live_sink.process_one_block());
    assert(near(live_sink.output_left[0], 0.5F * 0.70710678F));

    auto* solo = check_box(window, "solo-track-1");
    assert(solo != nullptr);
    live_sink.right_input[0] = 1.0F;
    solo->click();
    assert(window.test_project().tracks[0].solo);
    assert(window.test_history_cursor() == 3U);
    assert(live_sink.process_one_block());
    assert(near(live_sink.output_left[0], 0.5F * 0.70710678F));
    undo->click();
    assert(!window.test_project().tracks[0].solo);
    assert(live_sink.process_one_block());
    assert(live_sink.output_left[0] > 0.5F * 0.70710678F);
    return 0;
}
