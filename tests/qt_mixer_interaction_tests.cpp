#define HUSH_GUI_TESTING 1
#include "../src/mixer_gui.cpp"

#include <QCheckBox>
#include <QMetaObject>
#include <QPushButton>
#include <QTemporaryDir>
#include <QSlider>
#include <cassert>
#include <cmath>
#include <filesystem>

namespace {
bool near(float actual, float expected) {
    return std::abs(actual - expected) < 1.0e-5F;
}

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
    MixerWindow window;

    auto* gain = slider(window, "gain-track-1");
    assert(gain != nullptr);
    assert(near(window.test_project().tracks[0].gain, 1.0F));
    assert(window.test_history_cursor() == 0U);

    // Moving a slider previews a candidate value but does not mutate the model
    // until the actual QSlider::sliderReleased signal commits one undo operation.
    gain->setValue(50);
    assert(near(window.test_project().tracks[0].gain, 1.0F));
    assert(window.test_history_cursor() == 0U);
    assert(QMetaObject::invokeMethod(gain, "sliderReleased", Qt::DirectConnection));
    assert(near(window.test_project().tracks[0].gain, 0.5F));
    assert(window.test_history_cursor() == 1U);

    auto* undo = button(window, "undoButton");
    assert(undo != nullptr);
    undo->click();
    assert(near(window.test_project().tracks[0].gain, 1.0F));
    assert(window.test_history_cursor() == 0U);

    auto* redo = button(window, "redoButton");
    assert(redo != nullptr);
    redo->click();
    assert(near(window.test_project().tracks[0].gain, 0.5F));
    assert(window.test_history_cursor() == 1U);

    auto* pan = slider(window, "pan-track-1");
    assert(pan != nullptr);
    pan->setValue(0);
    assert(near(window.test_project().tracks[0].pan, -1.0F));
    assert(QMetaObject::invokeMethod(pan, "sliderReleased", Qt::DirectConnection));
    assert(near(window.test_project().tracks[0].pan, 0.0F));
    assert(window.test_history_cursor() == 2U);

    auto* mute = check_box(window, "mute-track-1");
    assert(mute != nullptr);
    mute->click();
    assert(window.test_project().tracks[0].mute);
    assert(window.test_history_cursor() == 3U);

    auto* solo = check_box(window, "solo-track-1");
    assert(solo != nullptr);
    solo->click();
    assert(window.test_project().tracks[0].solo);
    assert(window.test_history_cursor() == 4U);

    // A valid local recovery snapshot is restored through the same startup
    // recovery path used by the application, without opening a modal dialog.
    QTemporaryDir restore_dir;
    assert(restore_dir.isValid());
    const std::filesystem::path restore_journal =
        (restore_dir.path() + QStringLiteral("/session-recovery.journal")).toStdString();
    hush::Project previous;
    previous.id = "previous-session";
    previous.tracks.push_back({"restored-track", "Restored", 0.42F, 0.25F, true, false});
    hush::Error recovery_error;
    assert(hush::RecoveryJournal(restore_journal).append_snapshot(previous, recovery_error));
    {
        MixerWindow recovery_window;
        recovery_window.test_initialize_recovery(restore_journal, true);
        assert(recovery_window.test_project().id == "previous-session");
        assert(recovery_window.test_project().tracks.size() == 1U);
        assert(near(recovery_window.test_project().tracks[0].gain, 0.42F));
        assert(recovery_window.test_project().tracks[0].mute);
    }

    // Declining recovery must append the current in-memory session, so the old
    // session is not silently offered again on the next launch.
    QTemporaryDir decline_dir;
    assert(decline_dir.isValid());
    const std::filesystem::path decline_journal =
        (decline_dir.path() + QStringLiteral("/session-recovery.journal")).toStdString();
    assert(hush::RecoveryJournal(decline_journal).append_snapshot(previous, recovery_error));
    {
        MixerWindow decline_window;
        decline_window.test_initialize_recovery(decline_journal, false);
        assert(decline_window.test_project().id == "hush-demo");
    }
    hush::Project latest;
    assert(hush::RecoveryJournal(decline_journal).recover_latest(latest, recovery_error));
    assert(latest.id == "hush-demo");
    assert(latest.tracks.size() == 2U);
    return 0;
}
