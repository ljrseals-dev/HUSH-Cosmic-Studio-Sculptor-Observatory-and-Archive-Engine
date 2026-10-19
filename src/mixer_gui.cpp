#include "hush/operation_log.hpp"
#include "hush/project.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace {
enum class StudioTheme { PlainVanilla, Inkho };

QString themeName(StudioTheme theme) {
    return theme == StudioTheme::Inkho ? QStringLiteral("Inkho") : QStringLiteral("Plain Vanilla");
}

// Presentation only: themes never touch the project model or audio-processing path.
void applyTheme(QApplication& app, StudioTheme theme) {
    if (theme == StudioTheme::Inkho) {
        app.setStyleSheet(QStringLiteral(R"(
            QWidget { background: #090914; color: #e9e7ff; font-size: 10pt; }
            QMainWindow { background: #070711; }
            QLabel { color: #dcd9ff; }
            QPushButton, QComboBox {
                background: #19152b; color: #f4f0ff; border: 1px solid #6244a5;
                border-radius: 6px; padding: 6px 10px; min-height: 20px;
            }
            QPushButton:hover, QComboBox:hover { background: #292044; border-color: #b35dff; }
            QPushButton:pressed { background: #40205c; }
            QTableWidget {
                background: #0d0d1b; alternate-background-color: #141326;
                gridline-color: #29243f; border: 1px solid #373052;
                selection-background-color: #47286e; selection-color: #ffffff;
            }
            QHeaderView::section {
                background: #19152b; color: #c9b8ff; border: 0;
                border-bottom: 1px solid #7b48d7; padding: 8px; font-weight: 600;
            }
            QSlider::groove:horizontal { height: 5px; background: #302747; border-radius: 2px; }
            QSlider::sub-page:horizontal { background: #9b52ff; border-radius: 2px; }
            QSlider::handle:horizontal {
                background: #28e4ff; width: 13px; margin: -5px 0;
                border-radius: 6px; border: 1px solid #a4f5ff;
            }
            QCheckBox { spacing: 7px; }
            QCheckBox::indicator { width: 15px; height: 15px; }
            QCheckBox::indicator:unchecked { background: #171427; border: 1px solid #70648f; border-radius: 3px; }
            QCheckBox::indicator:checked { background: #a348ff; border: 1px solid #d2aaff; border-radius: 3px; }
        )"));
    } else {
        // Familiar desktop defaults with subtle neutral polish and no decorative glow.
        app.setStyleSheet(QStringLiteral(R"(
            QWidget { background: #f3f4f6; color: #20242b; font-size: 10pt; }
            QMainWindow { background: #f3f4f6; }
            QPushButton, QComboBox {
                background: #ffffff; color: #20242b; border: 1px solid #c8cdd4;
                border-radius: 4px; padding: 6px 10px; min-height: 20px;
            }
            QPushButton:hover, QComboBox:hover { background: #e8edf3; border-color: #8b98a8; }
            QPushButton:pressed { background: #dce3eb; }
            QTableWidget {
                background: #ffffff; alternate-background-color: #f6f8fa;
                gridline-color: #dce1e7; border: 1px solid #cbd1d8;
                selection-background-color: #d9e8fb; selection-color: #20242b;
            }
            QHeaderView::section {
                background: #e7ebf0; color: #303843; border: 0;
                border-bottom: 1px solid #c5ccd5; padding: 8px; font-weight: 600;
            }
            QSlider::groove:horizontal { height: 5px; background: #d2d8e0; border-radius: 2px; }
            QSlider::sub-page:horizontal { background: #64748b; border-radius: 2px; }
            QSlider::handle:horizontal {
                background: #ffffff; width: 13px; margin: -5px 0;
                border-radius: 6px; border: 1px solid #7d8998;
            }
            QCheckBox { spacing: 7px; }
        )"));
    }
}
} // namespace

class MixerWindow final : public QMainWindow {
public:
    explicit MixerWindow(QApplication& app) : app_(app) {
        setWindowTitle("HUSH Cosmic Studio — Mixer"); resize(1120, 660);
        auto* root = new QWidget(this);
        auto* layout = new QVBoxLayout(root);
        auto* bar = new QHBoxLayout;
        auto* open = new QPushButton("Open Project…");
        auto* save = new QPushButton("Save Project");
        auto* undo = new QPushButton("Undo");
        auto* redo = new QPushButton("Redo");
        auto* themeLabel = new QLabel("Theme");
        themePicker_ = new QComboBox;
        themePicker_->addItem("Plain Vanilla");
        themePicker_->addItem("Inkho");
        status_ = new QLabel("Offline mixer");
        bar->addWidget(open);
        bar->addWidget(save);
        bar->addWidget(undo);
        bar->addWidget(redo);
        bar->addSpacing(12);
        bar->addWidget(themeLabel);
        bar->addWidget(themePicker_);
        bar->addStretch();
        bar->addWidget(status_);
        layout->addLayout(bar);

        table_ = new QTableWidget(0, 6, root);
        table_->setHorizontalHeaderLabels({"Track", "Gain", "Pan", "Mute", "Solo", "Stable ID"});
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
        table_->verticalHeader()->hide();
        table_->setAlternatingRowColors(true);
        table_->setShowGrid(true);
        layout->addWidget(table_);
        setCentralWidget(root);

        connect(open, &QPushButton::clicked, this, [this] { openProject(); });
        connect(save, &QPushButton::clicked, this, [this] { saveProject(); });
        connect(undo, &QPushButton::clicked, this, [this] {
            if (log_.undo(project_)) { refresh(); status_->setText("Undo applied"); }
        });
        connect(redo, &QPushButton::clicked, this, [this] {
            if (log_.redo(project_)) { refresh(); status_->setText("Redo applied"); }
        });
        connect(themePicker_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
            const auto theme = index == 1 ? StudioTheme::Inkho : StudioTheme::PlainVanilla;
            applyTheme(app_, theme);
            QSettings settings;
            settings.setValue("appearance/theme", themeName(theme));
            status_->setText("Theme · " + themeName(theme));
        });

        project_.id = "hush-demo";
        project_.tracks = {{"track-1","Audio 1",1.0F,0.0F,false,false},
                           {"track-2","Audio 2",0.8F,-0.2F,false,false},
                           {"track-3","Reference",0.65F,0.2F,false,false}};

        // QSettings is local desktop configuration only; no network or telemetry.
        QSettings settings;
        const auto savedTheme = settings.value("appearance/theme", "Plain Vanilla").toString();
        const int themeIndex = savedTheme.compare("Inkho", Qt::CaseInsensitive) == 0 ? 1 : 0;
        themePicker_->setCurrentIndex(themeIndex);
        applyTheme(app_, themeIndex == 1 ? StudioTheme::Inkho : StudioTheme::PlainVanilla);
        refresh();
        status_->setText("Demo session · unsaved");
    }

private:
    QApplication& app_;
    hush::Project project_;
    hush::OperationLog log_;
    QTableWidget* table_{};
    QLabel* status_{};
    QComboBox* themePicker_{};

    void openProject() {
        const auto path = QFileDialog::getOpenFileName(this, "Open HUSH Project", {}, "HUSH Project (*.json)");
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Open failed", file.errorString());
            return;
        }
        hush::Project candidate;
        hush::Error error;
        if (!hush::from_json(QString::fromUtf8(file.readAll()).toStdString(), candidate, error)) {
            QMessageBox::warning(this, "Invalid project", QString::fromStdString(error.message));
            return;
        }
        project_ = std::move(candidate);
        log_ = hush::OperationLog{};
        refresh();
        status_->setText("Loaded · " + QFileInfo(path).fileName());
    }

    void saveProject() {
        const auto path = QFileDialog::getSaveFileName(this, "Save HUSH Project", "session.json", "HUSH Project (*.json)");
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, "Save failed", file.errorString());
            return;
        }
        const auto json = hush::to_json(project_);
        if (file.write(json.data(), static_cast<qint64>(json.size())) != static_cast<qint64>(json.size())) {
            QMessageBox::warning(this, "Save failed", file.errorString());
            return;
        }
        status_->setText("Saved · " + QFileInfo(path).fileName());
    }

    void change(const std::string& id, hush::OperationKind kind, float after) {
        const auto it = std::find_if(project_.tracks.begin(), project_.tracks.end(),
            [&](const hush::Track& t) { return t.id == id; });
        if (it == project_.tracks.end()) return;
        float before = 0.0F;
        switch (kind) {
        case hush::OperationKind::SetGain: before = it->gain; break;
        case hush::OperationKind::SetPan: before = it->pan; break;
        case hush::OperationKind::SetMute: before = it->mute ? 1.0F : 0.0F; break;
        case hush::OperationKind::SetSolo: before = it->solo ? 1.0F : 0.0F; break;
        }
        if (before != after && log_.apply_and_append(project_, {id, kind, before, after}))
            status_->setText("Edited · undo history " + QString::number(log_.cursor()));
    }

    void refresh() {
        table_->setRowCount(static_cast<int>(project_.tracks.size()));
        for (std::size_t i = 0; i < project_.tracks.size(); ++i) {
            const auto& t = project_.tracks[i];
            const int row = static_cast<int>(i);
            auto* name = new QTableWidgetItem(QString::fromStdString(t.name));
            name->setFlags(name->flags() & ~Qt::ItemIsEditable);
            table_->setItem(row, 0, name);

            auto* gain = new QSlider(Qt::Horizontal);
            gain->setRange(0, 200);
            gain->setValue(static_cast<int>(t.gain * 100.0F));
            gain->setToolTip("Gain 0.00–2.00");
            table_->setCellWidget(row, 1, gain);

            auto* pan = new QSlider(Qt::Horizontal);
            pan->setRange(-100, 100);
            pan->setValue(static_cast<int>(t.pan * 100.0F));
            pan->setToolTip("Pan −1.00 to +1.00");
            table_->setCellWidget(row, 2, pan);

            auto* mute = new QCheckBox("Mute");
            mute->setChecked(t.mute);
            table_->setCellWidget(row, 3, mute);
            auto* solo = new QCheckBox("Solo");
            solo->setChecked(t.solo);
            table_->setCellWidget(row, 4, solo);

            auto* id = new QTableWidgetItem(QString::fromStdString(t.id));
            id->setFlags(id->flags() & ~Qt::ItemIsEditable);
            table_->setItem(row, 5, id);

            const auto trackId = t.id;
            connect(gain, &QSlider::sliderReleased, this, [this, gain, trackId] {
                change(trackId, hush::OperationKind::SetGain, gain->value() / 100.0F);
            });
            connect(pan, &QSlider::sliderReleased, this, [this, pan, trackId] {
                change(trackId, hush::OperationKind::SetPan, pan->value() / 100.0F);
            });
            connect(mute, &QCheckBox::clicked, this, [this, mute, trackId] {
                change(trackId, hush::OperationKind::SetMute, mute->isChecked() ? 1.0F : 0.0F);
            });
            connect(solo, &QCheckBox::clicked, this, [this, solo, trackId] {
                change(trackId, hush::OperationKind::SetSolo, solo->isChecked() ? 1.0F : 0.0F);
            });
        }
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MixerWindow window(app);
    window.show();
    return app.exec();
}
