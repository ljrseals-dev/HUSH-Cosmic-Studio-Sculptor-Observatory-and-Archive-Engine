#define HUSH_GUI_TESTING 1
#include "../src/mixer_gui.cpp"

#include "hush/recovery_journal.hpp"

#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QMetaObject>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
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

bool write_project_file(const QString& path, const hush::Project& project) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const auto json = hush::to_json(project);
    return file.write(json.data(), static_cast<qint64>(json.size())) ==
               static_cast<qint64>(json.size()) &&
           file.flush();
}
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MixerWindow window;
    auto* bus_meters = window.findChild<QLabel*>("busInputMeters");
    assert(bus_meters != nullptr);
    assert(bus_meters->text().contains("BUS INPUTS"));
    auto* bus_outputs = window.findChild<QLabel*>("busOutputMeters");
    assert(bus_outputs != nullptr);
    assert(bus_outputs->text().contains("BUS OUTPUTS"));

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

    // Bus controls use the same bounded live command path and undo history.
    auto* bus_gain = slider(window, "gain-bus-capture");
    assert(bus_gain != nullptr);
    bus_gain->setValue(75);
    assert(near(window.test_project().buses[0].gain, 1.0F));
    assert(QMetaObject::invokeMethod(bus_gain, "sliderReleased", Qt::DirectConnection));
    assert(near(window.test_project().buses[0].gain, 0.75F));
    assert(window.test_history_cursor() == 5U);
    undo->click();
    assert(near(window.test_project().buses[0].gain, 1.0F));
    assert(window.test_history_cursor() == 4U);
    redo->click();
    assert(near(window.test_project().buses[0].gain, 0.75F));
    assert(window.test_history_cursor() == 5U);

    auto* bus_mute = check_box(window, "mute-bus-capture");
    assert(bus_mute != nullptr);
    bus_mute->click();
    assert(window.test_project().buses[0].mute);
    assert(window.test_history_cursor() == 6U);
    auto* bus_solo = check_box(window, "solo-bus-capture");
    assert(bus_solo != nullptr);
    bus_solo->click();
    assert(window.test_project().buses[0].solo);
    assert(window.test_history_cursor() == 7U);
    undo->click();
    assert(!window.test_project().buses[0].solo);
    assert(window.test_history_cursor() == 6U);

    // Exercise the same production project-load/recovery path without a modal prompt.
    // Accepting recovery restores the latest snapshot, marks the session dirty, and
    // lets the normal GUI autosave poll persist a fresh recovery record off-callback.
    QTemporaryDir temp;
    assert(temp.isValid());
    const QString accepted_path = temp.filePath("accepted.json");
    hush::Project saved;
    saved.id = "recovery-accepted";
    saved.tracks.push_back({"track-a", "Saved", 1.0F, 0.0F, false, false});
    assert(write_project_file(accepted_path, saved));
    auto recovered = saved;
    recovered.tracks[0].gain = 0.37F;
    const std::filesystem::path accepted_journal(
        (accepted_path + ".hush-recovery.journal").toStdString());
    hush::Error journal_error;
    assert(hush::RecoveryJournal(accepted_journal).append_snapshot(recovered, journal_error));

    assert(window.test_load_project_from_path(accepted_path, true));
    assert(near(window.test_project().tracks[0].gain, 0.37F));
    assert(window.test_status_text().contains("Recovered journal snapshot"));
    assert(std::filesystem::exists(accepted_journal));
    window.test_poll_autosave();
    hush::Project latest;
    assert(hush::RecoveryJournal(accepted_journal).recover_latest(latest, journal_error));
    assert(near(latest.tracks[0].gain, 0.37F));

    // Declining recovery must load the saved project and remove the declined journal
    // so the same stale recovery is not offered on the next launch.
    const QString declined_path = temp.filePath("declined.json");
    saved.id = "recovery-declined";
    saved.tracks[0].gain = 0.81F;
    assert(write_project_file(declined_path, saved));
    recovered = saved;
    recovered.tracks[0].gain = 0.22F;
    const std::filesystem::path declined_journal(
        (declined_path + ".hush-recovery.journal").toStdString());
    assert(hush::RecoveryJournal(declined_journal).append_snapshot(recovered, journal_error));
    assert(window.test_load_project_from_path(declined_path, false));
    assert(near(window.test_project().tracks[0].gain, 0.81F));
    assert(!std::filesystem::exists(declined_journal));
    assert(window.test_status_text().contains("Loaded"));

    return 0;
}
