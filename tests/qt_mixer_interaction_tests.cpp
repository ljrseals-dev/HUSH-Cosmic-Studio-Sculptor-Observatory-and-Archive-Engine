#define HUSH_GUI_TESTING 1
#include "../src/mixer_gui.cpp"

#include <QCheckBox>
#include <QMetaObject>
#include <QPushButton>
#include <QSlider>
#include "hush/recovery_journal.hpp"
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

    // Exercise the GUI lifecycle hook against a temporary local recovery journal.
    // This is a software integration test; it does not claim power-loss durability.
    const auto journal_base = std::filesystem::temp_directory_path() /
                              "hush-qt-autosave-lifecycle-test.json";
    const std::filesystem::path journal_path(journal_base.string() + ".hush-recovery.journal");
    std::error_code cleanup_error;
    std::filesystem::remove(journal_path, cleanup_error);
    window.test_configure_autosave(QString::fromStdString(journal_base.string()));
    gain = slider(window, "gain-track-1");
    assert(gain != nullptr);
    gain->setValue(75);
    assert(QMetaObject::invokeMethod(gain, "sliderReleased", Qt::DirectConnection));
    assert(window.test_autosave_dirty());
    window.test_poll_autosave();
    assert(!window.test_autosave_dirty());

    hush::Project recovered;
    hush::Error recovery_error;
    assert(hush::RecoveryJournal(journal_path).recover_latest(recovered, recovery_error));
    assert(std::abs(recovered.tracks[0].gain - 0.75F) < 1.0e-5F);
    std::filesystem::remove(journal_path, cleanup_error);
    return 0;
}
