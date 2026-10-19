#include <QtTest/QtTest>

// Compile the actual application widget implementation with its GUI-test-only
// accessors. This exercises real Qt signal connections without requiring ALSA hardware.
#define HUSH_GUI_TESTING 1
#include "../src/mixer_gui.cpp"

class MixerGuiInteractionTests final : public QObject {
    Q_OBJECT
private slots:
    void gainPanAndUndoRedoUseOperationLog() {
        MixerWindow window;
        window.show();
        QCoreApplication::processEvents();

        auto* gain = window.findChild<QSlider*>(QStringLiteral("gain_track-1"));
        auto* pan = window.findChild<QSlider*>(QStringLiteral("pan_track-1"));
        QVERIFY(gain);
        QVERIFY(pan);

        gain->setSliderDown(true);
        gain->setValue(125);
        gain->setSliderDown(false);
        QCOMPARE(window.test_project().tracks.at(0).gain, 1.25F);
        QCOMPARE(window.test_operation_log().cursor(), std::size_t{1});

        pan->setSliderDown(true);
        pan->setValue(35);
        pan->setSliderDown(false);
        QCOMPARE(window.test_project().tracks.at(0).pan, 0.35F);
        QCOMPARE(window.test_operation_log().cursor(), std::size_t{2});

        auto* undo = window.findChild<QPushButton*>(QStringLiteral("undoButton"));
        auto* redo = window.findChild<QPushButton*>(QStringLiteral("redoButton"));
        QVERIFY(undo);
        QVERIFY(redo);
        QTest::mouseClick(undo, Qt::LeftButton);
        QCOMPARE(window.test_project().tracks.at(0).pan, 0.0F);
        QCOMPARE(window.test_operation_log().cursor(), std::size_t{1});
        QTest::mouseClick(redo, Qt::LeftButton);
        QCOMPARE(window.test_project().tracks.at(0).pan, 0.35F);
        QCOMPARE(window.test_operation_log().cursor(), std::size_t{2});
    }

    void muteAndSoloClicksUpdateProjectAndHistory() {
        MixerWindow window;
        window.show();
        QCoreApplication::processEvents();
        auto* mute = window.findChild<QCheckBox*>(QStringLiteral("mute_track-1"));
        auto* solo = window.findChild<QCheckBox*>(QStringLiteral("solo_track-1"));
        QVERIFY(mute);
        QVERIFY(solo);

        QTest::mouseClick(mute, Qt::LeftButton);
        QVERIFY(window.test_project().tracks.at(0).mute);
        QCOMPARE(window.test_operation_log().cursor(), std::size_t{1});
        QTest::mouseClick(solo, Qt::LeftButton);
        QVERIFY(window.test_project().tracks.at(0).solo);
        QCOMPARE(window.test_operation_log().cursor(), std::size_t{2});
    }

    void liveActionAvailabilityMatchesBuildConfiguration() {
        MixerWindow window;
        auto* live = window.findChild<QPushButton*>(QStringLiteral("liveAlsaButton"));
        QVERIFY(live);
#ifdef HUSH_GUI_HAS_ALSA
        QVERIFY(live->isEnabled());
#else
        QVERIFY(!live->isEnabled());
#endif
    }
};

QTEST_MAIN(MixerGuiInteractionTests)
#include "gui_interaction_tests.moc"
