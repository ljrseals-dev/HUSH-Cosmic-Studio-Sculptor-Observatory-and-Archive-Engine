#include "../src/mixer_gui.cpp"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QtTest/QtTest>

class MixerWidgetInteractionTests final : public QObject {
    Q_OBJECT

private slots:
    void gainAndPanCommitUndoRedo() {
        MixerWindow window;
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
        QVERIFY(QMetaObject::invokeMethod(gain, "sliderReleased", Qt::DirectConnection));
        QVERIFY(status->text().startsWith("Edited"));
        QCOMPARE(window.findChild<QSlider*>("gain-track-1")->value(), 125);

        undo->click();
        QCOMPARE(window.findChild<QSlider*>("gain-track-1")->value(), 100);
        QVERIFY(status->text().startsWith("Undo applied"));
        redo->click();
        QCOMPARE(window.findChild<QSlider*>("gain-track-1")->value(), 125);
        QVERIFY(status->text().startsWith("Redo applied"));

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
        undo->click();
        QVERIFY(!window.findChild<QCheckBox*>("mute-track-1")->isChecked());
        redo->click();
        QVERIFY(window.findChild<QCheckBox*>("mute-track-1")->isChecked());

        solo = window.findChild<QCheckBox*>("solo-track-1");
        QVERIFY(solo);
        QTest::mouseClick(solo, Qt::LeftButton);
        QVERIFY(window.findChild<QCheckBox*>("solo-track-1")->isChecked());
        undo->click();
        QVERIFY(!window.findChild<QCheckBox*>("solo-track-1")->isChecked());
    }
};

QTEST_MAIN(MixerWidgetInteractionTests)
#include "mixer_widget_interaction_tests.moc"
