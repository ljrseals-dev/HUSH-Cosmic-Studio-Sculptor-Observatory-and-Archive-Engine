#include "hush/operation_log.hpp"
#include "hush/project.hpp"
#include "hush/realtime_track_mixer.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>
#include <memory>
#include <vector>

#ifdef HUSH_GUI_HAS_ALSA
#include "hush/alsa_audio_backend.hpp"
#endif

namespace {

#ifdef HUSH_GUI_HAS_ALSA
// Optional, explicitly user-started ALSA monitor. All storage and mixer state are
// prepared before start(); callback() performs no allocation, locking, logging or I/O.
// Current ALSA adapter exposes one stereo capture stream: L/R feed tracks 0/1.
// Further project tracks have no live source until track-source routing is implemented.
class LiveMixer final {
public:
    static constexpr std::size_t kMaxFrames = 4096;

    bool start(const hush::Project& project, std::string& error) {
        stop();
        if (project.tracks.empty()) {
            error = "The project has no tracks.";
            return false;
        }
        auto mixer = std::make_unique<hush::RealtimeTrackMixer>();
        if (!mixer->configure(project.tracks.size(), kMaxFrames)) {
            error = "Could not configure the realtime track mixer.";
            return false;
        }

        std::vector<std::vector<float>> buffers(project.tracks.size(),
                                                std::vector<float>(kMaxFrames, 0.0F));
        std::vector<const float*> pointers(project.tracks.size(), nullptr);
        for (std::size_t i = 0; i < buffers.size(); ++i) pointers[i] = buffers[i].data();

        mixer_ = std::move(mixer);
        track_buffers_ = std::move(buffers);
        track_pointers_ = std::move(pointers);
        if (!enqueue_project_controls(project)) {
            stop();
            error = "The bounded realtime command queue rejected initial controls.";
            return false;
        }

        hush::AlsaAudioBackend::Config config;
        config.sample_rate = 48000;
        config.period_frames = 128;
        if (!backend_.start(config, &LiveMixer::callback, this, error)) {
            stop();
            return false;
        }
        error.clear();
        return true;
    }

    void stop() noexcept {
        backend_.stop(); // Joins the callback thread before callback-owned state is freed.
        mixer_.reset();
        track_buffers_.clear();
        track_pointers_.clear();
    }

    [[nodiscard]] bool running() const noexcept { return backend_.running(); }

    bool enqueue_project_controls(const hush::Project& project) noexcept {
        if (!mixer_ || project.tracks.size() != track_buffers_.size()) return false;
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            const auto& t = project.tracks[i];
            if (!mixer_->enqueue_control(i, t.gain, t.pan, t.mute, t.solo)) return false;
        }
        return true;
    }

private:
    static void callback(const float* input, float* output, std::size_t frames,
                         void* user_data) noexcept {
        auto* self = static_cast<LiveMixer*>(user_data);
        if (!self || !output) return;
        if (!input || frames > kMaxFrames || !self->mixer_) {
            std::fill_n(output, frames * 2U, 0.0F);
            return;
        }

        for (auto& track : self->track_buffers_) std::fill_n(track.data(), frames, 0.0F);
        if (!self->track_buffers_.empty()) {
            for (std::size_t frame = 0; frame < frames; ++frame)
                self->track_buffers_[0][frame] = input[frame * 2U];
        }
        if (self->track_buffers_.size() > 1U) {
            for (std::size_t frame = 0; frame < frames; ++frame)
                self->track_buffers_[1][frame] = input[frame * 2U + 1U];
        }

        if (!self->mixer_->process(self->track_pointers_.data(),
                                   self->track_pointers_.size(),
                                   self->output_left_.data(), self->output_right_.data(),
                                   frames)) {
            std::fill_n(output, frames * 2U, 0.0F);
            return;
        }
        for (std::size_t frame = 0; frame < frames; ++frame) {
            output[frame * 2U] = self->output_left_[frame];
            output[frame * 2U + 1U] = self->output_right_[frame];
        }
    }

public:
    // Called before backend start; these buffers never resize during processing.
    bool prepare_output() {
        output_left_.assign(kMaxFrames, 0.0F);
        output_right_.assign(kMaxFrames, 0.0F);
        return true;
    }

private:
    hush::AlsaAudioBackend backend_;
    std::unique_ptr<hush::RealtimeTrackMixer> mixer_;
    std::vector<std::vector<float>> track_buffers_;
    std::vector<const float*> track_pointers_;
    std::vector<float> output_left_{kMaxFrames, 0.0F};
    std::vector<float> output_right_{kMaxFrames, 0.0F};
};
#endif

} // namespace

class MixerWindow final : public QMainWindow {
public:
    MixerWindow() {
        setWindowTitle("HUSH Cosmic Studio — Mixer");
        resize(1100, 640);
        auto* root = new QWidget(this);
        auto* layout = new QVBoxLayout(root);
        auto* bar = new QHBoxLayout;
        auto* open = new QPushButton("Open Project…");
        auto* save = new QPushButton("Save Project");
        auto* undo = new QPushButton("Undo");
        auto* redo = new QPushButton("Redo");
#ifdef HUSH_GUI_HAS_ALSA
        live_button_ = new QPushButton("Start Live ALSA");
#else
        live_button_ = new QPushButton("Live Audio (build with ALSA)");
        live_button_->setEnabled(false);
        live_button_->setToolTip("Configure with -DHUSH_ENABLE_ALSA=ON to enable the optional live monitor.");
#endif
        status_ = new QLabel("Offline mixer · live audio stopped");
        bar->addWidget(open);
        bar->addWidget(save);
        bar->addWidget(undo);
        bar->addWidget(redo);
        bar->addWidget(live_button_);
        bar->addStretch();
        bar->addWidget(status_);
        layout->addLayout(bar);

        table_ = new QTableWidget(0, 6, root);
        table_->setHorizontalHeaderLabels({"Track", "Gain", "Pan", "Mute", "Solo", "Stable ID"});
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
        table_->verticalHeader()->hide();
        table_->setAlternatingRowColors(true);
        layout->addWidget(table_);
        setCentralWidget(root);

        connect(open, &QPushButton::clicked, this, [this] { openProject(); });
        connect(save, &QPushButton::clicked, this, [this] { saveProject(); });
        connect(undo, &QPushButton::clicked, this, [this] {
            if (log_.undo(project_)) {
                sync_live_controls();
                refresh();
                status_->setText("Undo applied");
            }
        });
        connect(redo, &QPushButton::clicked, this, [this] {
            if (log_.redo(project_)) {
                sync_live_controls();
                refresh();
                status_->setText("Redo applied");
            }
        });
#ifdef HUSH_GUI_HAS_ALSA
        connect(live_button_, &QPushButton::clicked, this, [this] { toggle_live(); });
#endif

        project_.id = "hush-demo";
        project_.tracks = {{"track-1", "Input L", 1.0F, 0.0F, false, false},
                           {"track-2", "Input R", 0.8F, 0.0F, false, false}};
        refresh();
        status_->setText("Demo session · unsaved · offline");
    }

    ~MixerWindow() override {
#ifdef HUSH_GUI_HAS_ALSA
        live_.stop();
#endif
    }

private:
    hush::Project project_;
    hush::OperationLog log_;
    QTableWidget* table_{};
    QLabel* status_{};
    QPushButton* live_button_{};
#ifdef HUSH_GUI_HAS_ALSA
    LiveMixer live_;
#endif

    void openProject() {
        const auto path = QFileDialog::getOpenFileName(this, "Open HUSH Project", {}, "HUSH Project (*.json)");
        if (path.isEmpty()) return;
#ifdef HUSH_GUI_HAS_ALSA
        live_.stop();
        live_button_->setText("Start Live ALSA");
#endif
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
        status_->setText("Loaded · " + QFileInfo(path).fileName() + " · live audio stopped");
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
        if (before != after && log_.apply_and_append(project_, {id, kind, before, after})) {
            if (!sync_live_controls()) {
                status_->setText("Project edited; live command queue full — retry or stop audio");
            } else {
                status_->setText("Edited · undo history " + QString::number(log_.cursor()));
            }
        }
    }

    bool sync_live_controls() {
#ifdef HUSH_GUI_HAS_ALSA
        if (live_.running() && !live_.enqueue_project_controls(project_)) return false;
#endif
        return true;
    }

#ifdef HUSH_GUI_HAS_ALSA
    void toggle_live() {
        if (live_.running()) {
            live_.stop();
            live_button_->setText("Start Live ALSA");
            status_->setText("Live audio stopped");
            refresh();
            return;
        }
        if (!live_.prepare_output()) {
            status_->setText("Could not allocate live audio buffers");
            return;
        }
        std::string error;
        if (!live_.start(project_, error)) {
            status_->setText("ALSA start failed: " + QString::fromStdString(error));
            return;
        }
        live_button_->setText("Stop Live ALSA");
        status_->setText("Live ALSA · capture L/R → tracks 1–2; later tracks have no live source");
        refresh();
    }
#endif

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
            const auto track_id = t.id;
            connect(gain, &QSlider::sliderReleased, this,
                    [this, gain, track_id] { change(track_id, hush::OperationKind::SetGain, gain->value() / 100.0F); });
            connect(pan, &QSlider::sliderReleased, this,
                    [this, pan, track_id] { change(track_id, hush::OperationKind::SetPan, pan->value() / 100.0F); });
            connect(mute, &QCheckBox::clicked, this,
                    [this, mute, track_id] { change(track_id, hush::OperationKind::SetMute, mute->isChecked() ? 1.0F : 0.0F); });
            connect(solo, &QCheckBox::clicked, this,
                    [this, solo, track_id] { change(track_id, hush::OperationKind::SetSolo, solo->isChecked() ? 1.0F : 0.0F); });
#ifdef HUSH_GUI_HAS_ALSA
            // This backend currently exposes only two capture channels as live sources.
            if (live_.running() && i >= 2U) {
                gain->setEnabled(false);
                pan->setEnabled(false);
                mute->setEnabled(false);
                solo->setEnabled(false);
            }
#endif
        }
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MixerWindow window;
    window.show();
    return app.exec();
}
