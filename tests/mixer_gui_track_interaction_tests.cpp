#define HUSH_GUI_NO_MAIN
#define HUSH_GUI_TESTING 1
#include "../src/mixer_gui.cpp"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <cassert>

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);
    MixerWindow window;

    auto* add = window.findChild<QPushButton*>("addTrackButton");
    auto* remove = window.findChild<QPushButton*>("removeTrackButton");
    auto* undo = window.findChild<QPushButton*>("undoButton");
    auto* redo = window.findChild<QPushButton*>("redoButton");
    auto* count = window.findChild<QLabel*>("trackCountLabel");
    assert(add && remove && undo && redo && count);
    assert(count->text().startsWith("2 / "));

    add->click();
    assert(count->text().startsWith("3 / "));
    undo->click();
    assert(count->text().startsWith("2 / "));
    redo->click();
    assert(count->text().startsWith("3 / "));

    remove->click();
    assert(count->text().startsWith("2 / "));
    undo->click();
    assert(count->text().startsWith("3 / "));
    redo->click();
    assert(count->text().startsWith("2 / "));

    window.set_track_count_for_test(hush::kMaxTracks);
    assert(count->text().startsWith(QString::number(
        static_cast<qulonglong>(hush::kMaxTracks)) + " / "));
    assert(!add->isEnabled());
    assert(add->toolTip().contains("limit reached"));
    add->click();
    assert(count->text().startsWith(QString::number(
        static_cast<qulonglong>(hush::kMaxTracks)) + " / "));

    window.set_track_count_for_test(0U);
    assert(count->text().startsWith("0 / "));
    assert(!remove->isEnabled());
    return 0;
}
