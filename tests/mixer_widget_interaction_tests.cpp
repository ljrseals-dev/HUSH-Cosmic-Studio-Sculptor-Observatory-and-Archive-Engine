#include "../src/mixer_gui.cpp"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QtTest/QtTest>

#include "hush/realtime_track_mixer.hpp"
#include "hush/track_mixer_command_bridge.hpp"

#include <array>
#include <cmath>
#include <string>

class TestLiveSink final : public hush::LiveTrackControlSink {
public:
    TestLiveSink() {
        configured = mixer.configure(2, 1);
        inputs = {left_input.data(), right_input.data()};
    }

    bool running() const noexcept override { return configured; }

    bool enqueue_track_control(const hush::Project& project,
                               const std::string& track_id) noexcept override {
        ++control_commands;
        return hush::TrackMixerCommandBridge::enqueue_track(project, mixer, track_id);
    }

    bool enqueue_track_preview(const hush::Project& project, const std::string& track_id,
                               float gain, float pan) noexcept override {
        ++preview_commands;
        return hush::TrackMixerCommandBridge::enqueue_preview(project, mixer, track_id, gain, pan);
    }

    bool process_one_block() noexcept {
        return mixer.process(inputs.data(), inputs.size(), output_left.data(),
                             output_right.data(), 1);
    }

    bool configured{false};
    int preview_commands{0};
    int control_commands{0};
    hush::RealtimeTrackMixer mixer;
    std::array<float, 1> left_input{1.0F};
    std::array<float, 1> right_input{0.0F};
    std::array<const float*, 2> inputs{};
    std::array<float, 1> output_left{};
    std::array<float, 1> output_right{};
};

class MixerWidgetInteractionTests final : public QObject {
    Q_OBJECT

private slots:
    void gainAndPanCommitUndoRedo() {
        TestLiveSink sink;
        QVERIFY(sink.configured);
        MixerWindow window(&sink);
        window.show();
        QCoreApplication::processEvents();

        auto* gain = window.findChild<QSlider*>("gain-track-1");
        auto* pan = window.findChild<QSlider*>("pan-track-1");
        auto* status = window.findChild<QLabel*>("hush-status");
        auto* undo = window.findChild<QPushButton*>("undo");
        auto* redo = window.findChild<QPushButton*>("redo");
        QVERIFY(gain);
        QVERIFY(pan);
        QVERIFY(status);
        QVERIFY(undo);
        QVERIFY(redo);

        gain->setValue(125);
        QVERIFY(sink.preview_commands > 0);
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0] - 1.25F) < 1.0e-5F);
        QVERIFY(std::abs(sink.output_right[0]) < 1.0e-5F);

        QVERIFY(QMetaObject::invokeMethod(gain, "sliderReleased", Qt::DirectConnection));
        QVERIFY(sink.control_commands > 0);
        QVERIFY(status->text().startsWith("Edited"));
        QCOMPARE(window.findChild<QSlider*>("gain-track-1")->value(), 125);
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0] - 1.25F) < 1.0e-5F);

        undo->click();
        QCOMPARE(window.findChild<QSlider*>("gain-track-1")->value(), 100);
        QVERIFY(status->text().startsWith("Undo applied"));
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0] - 1.0F) < 1.0e-5F);
        redo->click();
        QCOMPARE(window.findChild<QSlider*>("gain-track-1")->value(), 125);
        QVERIFY(status->text().startsWith("Redo applied"));
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0] - 1.25F) < 1.0e-5F);

        pan = window.findChild<QSlider*>("pan-track-1");
        QVERIFY(pan);
        pan->setValue(35);
        QVERIFY(QMetaObject::invokeMethod(pan, "sliderReleased", Qt::DirectConnection));
        QCOMPARE(window.findChild<QSlider*>("pan-track-1")->value(), 35);
        undo->click();
        QCOMPARE(window.findChild<QSlider*>("pan-track-1")->value(), -100);
    }

    void muteAndSoloClickUndoRedo() {
        MixerWindow window;
        window.show();
        QCoreApplication::processEvents();

        auto* mute = window.findChild<QCheckBox*>("mute-track-1");
        auto* solo = window.findChild<QCheckBox*>("solo-track-1");
        auto* status = window.findChild<QLabel*>("hush-status");
        auto* undo = window.findChild<QPushButton*>("undo");
        auto* redo = window.findChild<QPushButton*>("redo");
        QVERIFY(mute);
        QVERIFY(solo);
        QVERIFY(status);
        QVERIFY(undo);
        QVERIFY(redo);

        QTest::mouseClick(mute, Qt::LeftButton);
        QVERIFY(status->text().startsWith("Edited"));
        QVERIFY(window.findChild<QCheckBox*>("mute-track-1")->isChecked());
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0]) < 1.0e-5F);
        undo->click();
        QVERIFY(!window.findChild<QCheckBox*>("mute-track-1")->isChecked());
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0] - 1.0F) < 1.0e-5F);
        redo->click();
        QVERIFY(window.findChild<QCheckBox*>("mute-track-1")->isChecked());
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0]) < 1.0e-5F);

        undo->click();
        solo = window.findChild<QCheckBox*>("solo-track-1");
        QVERIFY(solo);
        QTest::mouseClick(solo, Qt::LeftButton);
        QVERIFY(window.findChild<QCheckBox*>("solo-track-1")->isChecked());
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0] - 1.0F) < 1.0e-5F);
        undo->click();
        QVERIFY(!window.findChild<QCheckBox*>("solo-track-1")->isChecked());
        QVERIFY(sink.process_one_block());
        QVERIFY(std::abs(sink.output_left[0] - 1.0F) < 1.0e-5F);
    }
};

QTEST_MAIN(MixerWidgetInteractionTests)
#include "mixer_widget_interaction_tests.moc"
