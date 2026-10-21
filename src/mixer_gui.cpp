#include "hush/operation_log.hpp"
#include "hush/autosave_controller.hpp"
#include "hush/project.hpp"
#include "hush/project_close_policy.hpp"
#include "hush/realtime_track_mixer.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
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
    [[nodiscard]] std::uint64_t xrun_count() const noexcept { return backend_.xrun_count(); }
    [[nodiscard]] std::uint64_t callback_count() const noexcept { return backend_.callback_count(); }
    [[nodiscard]] float input_peak() const noexcept { return input_peak_.load(std::memory_order_relaxed); }
    [[nodiscard]] float output_peak() const noexcept { return output_peak_.load(std::memory_order_relaxed); }

    bool enqueue_project_controls(const hush::Project& project) noexcept {
        if (!mixer_ || project.tracks.size() != track_buffers_.size()) return false;
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            const auto& t = project.tracks[i];
            if (!mixer_->enqueue_control(i, t.gain, t.pan, t.mute, t.solo)) return false;
        }
        return true;
    }

    // After startup, enqueue only the track touched by the operation. Sending the
    // whole project for every slider release needlessly consumes the bounded queue.
    bool enqueue_track_control_values(const hush::Project& project, const std::string& track_id,
                                      float gain, float pan, bool mute, bool solo) noexcept {
        if (!mixer_ || project.tracks.size() != track_buffers_.size()) return false;
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            if (project.tracks[i].id == track_id)
                return mixer_->enqueue_control(i, gain, pan, mute, solo);
        }
        return false;
    }

    bool enqueue_track_control(const hush::Project& project, const std::string& track_id) noexcept {
        for (const auto& track : project.tracks) {
            if (track.id == track_id)
                return enqueue_track_control_values(project, track_id, track.gain, track.pan,
                                                    track.mute, track.solo);
        }
        return false;
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

        float input_peak = 0.0F;
        for (std::size_t i = 0; i < frames * 2U; ++i)
            input_peak = std::max(input_peak, std::abs(input[i]));
        self->input_peak_.store(input_peak, std::memory_order_relaxed);

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
        float output_peak = 0.0F;
        for (std::size_t frame = 0; frame < frames; ++frame) {
            output[frame * 2U] = self->output_left_[frame];
            output[frame * 2U + 1U] = self->output_right_[frame];
            output_peak = std::max(output_peak, std::abs(self->output_left_[frame]));
            output_peak = std::max(output_peak, std::abs(self->output_right_[frame]));
        }
        self->output_peak_.store(output_peak, std::memory_order_relaxed);
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
    std::atomic<float> input_peak_{0.0F};
    std::atomic<float> output_peak_{0.0F};
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
        open->setObjectName("openProjectButton");
        auto* save = new QPushButton("Save Project");
        save->setObjectName("saveProjectButton");
        auto* undo = new QPushButton("Undo");
        undo->setObjectName("undoButton");
        auto* redo = new QPushButton("Redo");
        redo->setObjectName("redoButton");
#ifdef HUSH_GUI_HAS_ALSA
        live_button_ = new QPushButton("Start Live ALSA");
        live_button_->setObjectName("liveAudioButton");
#else
        live_button_ = new QPushButton("Live Audio (build with ALSA)");
        live_button_->setObjectName("liveAudioButton");
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
        table_->setObjectName("trackTable");
        table_->setHorizontalHeaderLabels({"Track", "Gain", "Pan", "Mute", "Solo", "Stable ID"});
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
        table_->verticalHeader()->hide();
        table_->setAlternatingRowColors(true);
        layout->addWidget(table_);

        // Channel Light: meters are refreshed on the GUI thread from atomic peaks
        // measured in the actual ALSA callback. The callback never touches Qt.
        auto* light_panel = new QWidget(root);
        light_panel->setObjectName("channelLightPanel");
        auto* light_layout = new QHBoxLayout(light_panel);
        auto* light_title = new QLabel("CHANNEL LIGHT", light_panel);
        light_title->setObjectName("channelLightTitle");
        light_layout->addWidget(light_title);
        input_light_ = new QLabel("INPUT · IDLE", light_panel);
        process_light_ = new QLabel("PROCESSING · IDLE", light_panel);
        clip_light_ = new QLabel("CLIP · CLEAR", light_panel);
        output_light_ = new QLabel("OUTPUT · IDLE", light_panel);
        for (auto* label : {input_light_, process_light_, clip_light_, output_light_}) {
            label->setMinimumWidth(125);
            label->setAlignment(Qt::AlignCenter);
            label->setMargin(7);
            light_layout->addWidget(label);
        }
        input_meter_ = new QProgressBar(light_panel);
        input_meter_->setObjectName("channelInputMeter");
        input_meter_->setRange(0, 1000);
        input_meter_->setTextVisible(false);
        output_meter_ = new QProgressBar(light_panel);
        output_meter_->setObjectName("channelOutputMeter");
        output_meter_->setRange(0, 1000);
        output_meter_->setTextVisible(false);
        light_layout->addWidget(input_meter_, 1);
        light_layout->addWidget(output_meter_, 1);
        layout->addWidget(light_panel);
        setStyleSheet(
            "QMainWindow,QWidget { background:#111522; color:#E6ECF7; }"
            "QTableWidget { background:#171D2C; alternate-background-color:#1D2537; gridline-color:#303A50; }"
            "QHeaderView::section { background:#222B40; color:#DCE7FF; padding:6px; border:0; }"
            "QPushButton { background:#222B40; border:1px solid #3C4965; border-radius:5px; padding:6px 10px; }"
            "QPushButton:hover { border-color:#48D9FF; }"
            "QSlider::groove:horizontal { height:5px; background:#303A50; }"
            "QSlider::handle:horizontal { background:#48D9FF; width:13px; margin:-5px 0; border-radius:6px; }"
            "QCheckBox { spacing:6px; }"
            "QLabel#channelLightTitle { color:#B9A1FF; font-weight:bold; letter-spacing:2px; }"
            "QProgressBar { background:#20283A; border:1px solid #35415B; border-radius:4px; height:12px; }"
            "QProgressBar::chunk { background:#48D9FF; border-radius:3px; }");
        setCentralWidget(root);

        connect(open, &QPushButton::clicked, this, [this] { openProject(); });
        connect(save, &QPushButton::clicked, this, [this] { saveProject(); });
        connect(undo, &QPushButton::clicked, this, [this] {
            const auto* operation = log_.cursor() > 0 ? log_.operation_at(log_.cursor() - 1) : nullptr;
            if (!operation) return;
            auto values = live_control_values(operation->entity_id);
            if (!values) return;
            apply_value_to_controls(*operation, operation->before_value, *values);
            if (!enqueue_live_control_values(operation->entity_id, *values)) {
                status_->setText("Undo not applied: live command queue full; retry after audio drains");
                return;
            }
            if (log_.undo(project_)) {
                autosave_dirty_ = true;
                status_->setText("Undo applied");
                refresh();
            }
        });
        connect(redo, &QPushButton::clicked, this, [this] {
            const auto* operation = log_.operation_at(log_.cursor());
            if (!operation) return;
            auto values = live_control_values(operation->entity_id);
            if (!values) return;
            apply_value_to_controls(*operation, operation->after_value, *values);
            if (!enqueue_live_control_values(operation->entity_id, *values)) {
                status_->setText("Redo not applied: live command queue full; retry after audio drains");
                return;
            }
            if (log_.redo(project_)) {
                autosave_dirty_ = true;
                status_->setText("Redo applied");
                refresh();
            }
        });
#ifdef HUSH_GUI_HAS_ALSA
        connect(live_button_, &QPushButton::clicked, this, [this] { toggle_live(); });
        connect(&status_timer_, &QTimer::timeout, this, [this] { poll_live_status(); poll_channel_light(); });
        status_timer_.start(250);
#endif

        connect(&autosave_timer_, &QTimer::timeout, this, [this] { poll_autosave(); });
        autosave_timer_.start(1000);

        project_.id = "hush-demo";
        project_.tracks = {{"track-1", "Input L", 1.0F, -1.0F, false, false},
                           {"track-2", "Input R", 0.8F, 1.0F, false, false}};
        log_.mark_dirty(); // The demo project has not been written to disk yet.
        refresh();
        status_->setText("Demo session · unsaved · offline");
    }

#ifdef HUSH_GUI_TESTING
    [[nodiscard]] const hush::Project& test_project() const noexcept { return project_; }
    [[nodiscard]] std::size_t test_history_cursor() const noexcept { return log_.cursor(); }
#endif

    ~MixerWindow() override {
#ifdef HUSH_GUI_HAS_ALSA
        live_.stop();
#endif
    }

private:
    hush::Project project_;
    hush::OperationLog log_;
    QString project_path_;
    QString autosave_project_path_;
    std::unique_ptr<hush::AutosaveController> autosave_;
    QTimer autosave_timer_;
    bool autosave_dirty_{false};
    std::optional<hush::AutosaveController::Clock::time_point> autosave_retry_after_;
    QTableWidget* table_{};
    QLabel* status_{};
    QLabel* input_light_{};
    QLabel* process_light_{};
    QLabel* clip_light_{};
    QLabel* output_light_{};
    QProgressBar* input_meter_{};
    QProgressBar* output_meter_{};
    QPushButton* live_button_{};
#ifdef HUSH_GUI_HAS_ALSA
    LiveMixer live_;
    QTimer status_timer_;
#endif

    void openProject() {
        if (!confirmProjectReplacement()) return;
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

        bool recovered_from_journal = false;
        const std::filesystem::path journal_path(path.toStdString() + ".hush-recovery.journal");
        std::error_code filesystem_error;
        const bool journal_exists = std::filesystem::exists(journal_path, filesystem_error);
        if (filesystem_error) {
            QMessageBox::warning(this, "Recovery check failed",
                                 QString::fromStdString(filesystem_error.message()));
        } else if (journal_exists) {
            hush::Project recovered = candidate;
            bool applied = false;
            hush::Error recovery_error;
            if (!hush::RecoveryJournal(journal_path).recover_if_different(
                    recovered, applied, recovery_error)) {
                QMessageBox::warning(this, "Recovery unavailable",
                    "The saved project will open, but its recovery journal could not be read: " +
                    QString::fromStdString(recovery_error.message));
            } else if (applied) {
                const auto choice = QMessageBox::question(
                    this, "Recover unsaved changes",
                    "A newer recovery snapshot exists for this project. Restore it?",
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                if (choice == QMessageBox::Yes) {
                    candidate = std::move(recovered);
                    recovered_from_journal = true;
                }
            }
        }
#ifdef HUSH_GUI_HAS_ALSA
        live_.stop();
        live_button_->setText("Start Live ALSA");
#endif
        project_ = std::move(candidate);
        project_path_ = path;
        configure_autosave(path);
        log_ = hush::OperationLog{};
        if (recovered_from_journal) {
            log_.mark_dirty();
            autosave_dirty_ = true;
            status_->setText("Recovered journal snapshot · review and save changes");
        } else {
            log_.mark_saved();
            autosave_dirty_ = false;
            status_->setText("Loaded · " + QFileInfo(path).fileName() + " · live audio stopped");
        }
        refresh();
    }

    bool saveProject() {
        QString path = project_path_;
        if (path.isEmpty()) {
            path = QFileDialog::getSaveFileName(
                this, "Save HUSH Project", "session.json", "HUSH Project (*.json)");
        }
        if (path.isEmpty()) return false;

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(this, "Save failed", file.errorString());
            return false;
        }
        const auto json = hush::to_json(project_);
        if (file.write(json.data(), static_cast<qint64>(json.size())) !=
            static_cast<qint64>(json.size())) {
            QMessageBox::warning(this, "Save failed", file.errorString());
            return false;
        }
        if (!file.flush()) {
            QMessageBox::warning(this, "Save failed", file.errorString());
            return false;
        }

        project_path_ = path;
        configure_autosave(path);
        log_.mark_saved();
        hush::Error journal_error;
        if (autosave_->save_now(project_, journal_error) == hush::AutosaveController::Outcome::Failed) {
            autosave_dirty_ = true;
            autosave_retry_after_ = hush::AutosaveController::Clock::now() + std::chrono::seconds(30);
            status_->setText("Project saved; recovery journal failed: " +
                             QString::fromStdString(autosave_->last_save_message()));
        } else {
            autosave_dirty_ = false;
            autosave_retry_after_.reset();
            status_->setText("Saved · " + QFileInfo(path).fileName() + " · recovery snapshot saved");
        }
        return true;
    }

    bool confirmProjectReplacement() {
        if (!log_.dirty()) return true;

        QMessageBox prompt(QMessageBox::Warning, "Unsaved project changes",
                           "Save changes to the current project before continuing?",
                           QMessageBox::NoButton, this);
        auto* save = prompt.addButton("Save", QMessageBox::AcceptRole);
        auto* discard = prompt.addButton("Discard", QMessageBox::DestructiveRole);
        prompt.addButton(QMessageBox::Cancel);
        prompt.setDefaultButton(save);
        prompt.exec();

        hush::ProjectCloseChoice choice = hush::ProjectCloseChoice::Cancel;
        if (prompt.clickedButton() == save) choice = hush::ProjectCloseChoice::Save;
        else if (prompt.clickedButton() == discard) choice = hush::ProjectCloseChoice::Discard;

        const bool saved = choice == hush::ProjectCloseChoice::Save && saveProject();
        const bool allow = hush::should_close_project(log_.dirty(), choice, saved);
        if (!allow && choice == hush::ProjectCloseChoice::Save)
            status_->setText("Project remains open: save did not complete");
        return allow;
    }

    struct LiveControlValues {
        float gain{1.0F};
        float pan{0.0F};
        bool mute{false};
        bool solo{false};
    };

    std::optional<LiveControlValues> live_control_values(const std::string& id) const {
        const auto it = std::find_if(project_.tracks.begin(), project_.tracks.end(),
                                     [&](const hush::Track& t) { return t.id == id; });
        if (it == project_.tracks.end()) return std::nullopt;
        return LiveControlValues{it->gain, it->pan, it->mute, it->solo};
    }

    static void apply_value_to_controls(const hush::Operation& operation, float value,
                                        LiveControlValues& controls) {
        switch (operation.kind) {
        case hush::OperationKind::SetGain: controls.gain = value; break;
        case hush::OperationKind::SetPan: controls.pan = value; break;
        case hush::OperationKind::SetMute: controls.mute = value != 0.0F; break;
        case hush::OperationKind::SetSolo: controls.solo = value != 0.0F; break;
        }
    }

    bool enqueue_live_control_values(const std::string& id, const LiveControlValues& values) {
#ifdef HUSH_GUI_HAS_ALSA
        if (live_.running() &&
            !live_.enqueue_track_control_values(project_, id, values.gain, values.pan,
                                                values.mute, values.solo)) return false;
#else
        (void)id;
        (void)values;
#endif
        return true;
    }

    // During a fader/pan drag, preview the candidate values in the live engine
    // without mutating project state or adding undo entries. sliderReleased commits
    // the final value through change(), creating one undoable operation.
    void preview_live_sliders(const std::string& id, float gain, float pan) {
        auto values = live_control_values(id);
        if (!values) return;
        values->gain = gain;
        values->pan = pan;
        if (!enqueue_live_control_values(id, *values))
            status_->setText("Live preview queue full — release slider or stop audio");
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
        if (before == after) return;
        auto values = LiveControlValues{it->gain, it->pan, it->mute, it->solo};
        hush::Operation operation{id, kind, before, after};
        apply_value_to_controls(operation, after, values);
        // Reserve the live-engine command before committing project state/history. If
        // the bounded queue is full, neither the model nor undo history advances.
        if (!enqueue_live_control_values(id, values)) {
            status_->setText("Edit not applied: live command queue full; retry after audio drains");
            QTimer::singleShot(0, this, [this] { refresh(); });
            return;
        }
        if (log_.apply_and_append(project_, std::move(operation))) {
            autosave_dirty_ = true;
            status_->setText("Edited · undo history " + QString::number(log_.cursor()));
        }
    }

#ifdef HUSH_GUI_HAS_ALSA
    void poll_channel_light() {
        const bool running = live_.running();
        const float in = running ? live_.input_peak() : 0.0F;
        const float out = running ? live_.output_peak() : 0.0F;
        const auto level = [](float value) {
            if (!std::isfinite(value) || value <= 0.0F) return 0;
            return static_cast<int>(std::clamp(value, 0.0F, 1.0F) * 1000.0F);
        };
        input_meter_->setValue(level(in));
        output_meter_->setValue(level(out));
        input_light_->setText(running && in > 0.0001F ? "INPUT · SIGNAL" : "INPUT · IDLE");
        process_light_->setText(running ? "PROCESSING · ACTIVE" : "PROCESSING · IDLE");
        clip_light_->setText((in >= 1.0F || out >= 1.0F) ? "CLIP · CLIPPING" : "CLIP · CLEAR");
        output_light_->setText(running && out > 0.0001F ? "OUTPUT · SIGNAL" : "OUTPUT · IDLE");
        input_light_->setStyleSheet(running && in > 0.0001F ? "background:#123C4B;color:#58E4FF;border-radius:5px;" : "background:#20283A;color:#8290AA;border-radius:5px;");
        process_light_->setStyleSheet(running ? "background:#30264B;color:#C9B5FF;border-radius:5px;" : "background:#20283A;color:#8290AA;border-radius:5px;");
        clip_light_->setStyleSheet((in >= 1.0F || out >= 1.0F) ? "background:#5A202D;color:#FF8797;border-radius:5px;" : "background:#193C32;color:#8AF0BC;border-radius:5px;");
        output_light_->setStyleSheet(running && out > 0.0001F ? "background:#193C32;color:#8AF0BC;border-radius:5px;" : "background:#20283A;color:#8290AA;border-radius:5px;");
    }

    void poll_live_status() {
        if (live_.running()) {
            status_->setText("Live ALSA · callbacks " + QString::number(live_.callback_count()) +
                             " · xruns/recoveries " + QString::number(live_.xrun_count()));
            return;
        }
        if (live_button_->text() == "Stop Live ALSA") {
            const auto callbacks = live_.callback_count();
            const auto xruns = live_.xrun_count();
            live_.stop();
            live_button_->setText("Start Live ALSA");
            refresh();
            status_->setText("ALSA stopped · callbacks " + QString::number(callbacks) +
                             " · xruns/recoveries " + QString::number(xruns));
        }
    }

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

    void configure_autosave(const QString& project_path) {
        if (autosave_ && autosave_project_path_ == project_path) return;
        autosave_project_path_ = project_path;
        const std::filesystem::path journal_path(project_path.toStdString() + ".hush-recovery.journal");
        autosave_ = std::make_unique<hush::AutosaveController>(hush::RecoveryJournal(journal_path));
        autosave_dirty_ = false;
        autosave_retry_after_.reset();
    }

    void poll_autosave() {
        if (!autosave_ || !autosave_dirty_) return;
        const auto now = hush::AutosaveController::Clock::now();
        if (autosave_retry_after_ && now < *autosave_retry_after_) return;

        bool transport_running = false;
#ifdef HUSH_GUI_HAS_ALSA
        transport_running = live_.running();
#endif
        hush::Error error;
        const auto outcome = autosave_->poll_if_dirty(
            project_, now, autosave_dirty_, transport_running, error);
        if (outcome == hush::AutosaveController::Outcome::Failed) {
            autosave_retry_after_ = now + std::chrono::seconds(30);
            status_->setText("Recovery journal write failed; retrying: " +
                             QString::fromStdString(autosave_->last_save_message()));
        } else if (outcome == hush::AutosaveController::Outcome::Saved) {
            autosave_retry_after_.reset();
            status_->setText("Recovery snapshot saved · project file may still be unsaved");
        }
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
            gain->setObjectName(QString("gain-%1").arg(QString::fromStdString(t.id)));
            gain->setRange(0, 200);
            gain->setValue(static_cast<int>(t.gain * 100.0F));
            gain->setToolTip("Gain 0.00–2.00");
            table_->setCellWidget(row, 1, gain);
            auto* pan = new QSlider(Qt::Horizontal);
            pan->setObjectName(QString("pan-%1").arg(QString::fromStdString(t.id)));
            pan->setRange(-100, 100);
            pan->setValue(static_cast<int>(t.pan * 100.0F));
            pan->setToolTip("Pan −1.00 to +1.00");
            table_->setCellWidget(row, 2, pan);
            auto* mute = new QCheckBox("Mute");
            mute->setObjectName(QString("mute-%1").arg(QString::fromStdString(t.id)));
            mute->setChecked(t.mute);
            table_->setCellWidget(row, 3, mute);
            auto* solo = new QCheckBox("Solo");
            solo->setObjectName(QString("solo-%1").arg(QString::fromStdString(t.id)));
            solo->setChecked(t.solo);
            table_->setCellWidget(row, 4, solo);
            auto* id = new QTableWidgetItem(QString::fromStdString(t.id));
            id->setFlags(id->flags() & ~Qt::ItemIsEditable);
            table_->setItem(row, 5, id);
            const auto track_id = t.id;
            // Live audio follows the slider during a drag. The release handlers below
            // commit only the final value to the project and undo history.
            connect(gain, &QSlider::valueChanged, this,
                    [this, gain, pan, track_id](int) {
                        preview_live_sliders(track_id, gain->value() / 100.0F,
                                             pan->value() / 100.0F);
                    });
            connect(pan, &QSlider::valueChanged, this,
                    [this, gain, pan, track_id](int) {
                        preview_live_sliders(track_id, gain->value() / 100.0F,
                                             pan->value() / 100.0F);
                    });
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
protected:
    void closeEvent(QCloseEvent* event) override {
        if (confirmProjectReplacement()) event->accept();
        else event->ignore();
    }
};

#ifndef HUSH_GUI_TESTING
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MixerWindow window;
    window.show();
    return app.exec();
}
#endif
