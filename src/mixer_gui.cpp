#include "hush/operation_log.hpp"
#include "hush/autosave_controller.hpp"
#include "hush/project.hpp"
#include "hush/project_close_policy.hpp"
#include "hush/realtime_project_audio_pipeline.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QComboBox>
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
// Optional, explicitly user-started ALSA monitor. The project-aware pipeline and all
// scratch buffers are configured before start(); its callback performs no allocation,
// locking, logging or I/O. Stereo capture feeds source tracks 0/1; later tracks remain
// silent until a media/device source is connected, but their bus assignments are honored.
class LiveMixer final {
public:
    static constexpr std::size_t kMaxFrames = 4096;

    bool start(const hush::Project& project, const std::filesystem::path& project_directory,
               std::string& error) {
        stop();
        if (project.tracks.empty()) {
            error = "The project has no tracks.";
            return false;
        }

        auto pipeline = std::make_unique<hush::RealtimeProjectAudioPipeline>();
        const bool configured = project.clips.empty()
            ? pipeline->configure(project, kMaxFrames, error)
            : pipeline->configure_from_project_directory(project, project_directory, kMaxFrames, error);
        if (!configured) return false;
        pipeline_ = std::move(pipeline);

        hush::AlsaAudioBackend::Config config;
        config.sample_rate = 48000;
        config.period_frames = 128;
        if (!backend_.start(config, &hush::RealtimeProjectAudioPipeline::alsa_callback,
                            pipeline_.get(), error)) {
            stop();
            return false;
        }
        error.clear();
        return true;
    }

    void stop() noexcept {
        backend_.stop(); // Join the callback before releasing the pipeline and its buffers.
        pipeline_.reset();
    }

    [[nodiscard]] bool running() const noexcept { return backend_.running(); }
    [[nodiscard]] std::uint64_t xrun_count() const noexcept { return backend_.xrun_count(); }
    [[nodiscard]] std::uint64_t callback_count() const noexcept { return backend_.callback_count(); }
    [[nodiscard]] float input_peak() const noexcept {
        return pipeline_ ? pipeline_->input_peak() : 0.0F;
    }
    [[nodiscard]] float output_peak() const noexcept {
        return pipeline_ ? pipeline_->output_peak() : 0.0F;
    }
    [[nodiscard]] std::size_t bus_input_meter_count() const noexcept {
        return pipeline_ ? pipeline_->bus_input_meter_count() : 0U;
    }
    [[nodiscard]] hush::BusInputMeterSnapshot bus_input_meter(std::size_t index) const noexcept {
        return pipeline_ ? pipeline_->bus_input_meter(index) : hush::BusInputMeterSnapshot{};
    }
    [[nodiscard]] std::size_t bus_output_meter_count() const noexcept {
        return pipeline_ ? pipeline_->bus_output_meter_count() : 0U;
    }
    [[nodiscard]] hush::BusOutputMeterSnapshot bus_output_meter(std::size_t index) const noexcept {
        return pipeline_ ? pipeline_->bus_output_meter(index) : hush::BusOutputMeterSnapshot{};
    }

    bool enqueue_track_control_values(const hush::Project& project, const std::string& track_id,
                                      float gain, float pan, bool mute, bool solo) noexcept {
        if (!pipeline_) return false;
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            if (project.tracks[i].id == track_id)
                return pipeline_->enqueue_track_control(i, gain, pan, mute, solo);
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

    bool enqueue_bus_control_values(const hush::Project& project, const std::string& bus_id,
                                    float gain, bool mute, bool solo) noexcept {
        (void)project;
        if (!pipeline_) return false;
        return pipeline_->enqueue_bus_control(bus_id, gain, mute, solo);
    }

    bool replace_bus_processing(const hush::Project& project, std::string& error) {
        return pipeline_ && pipeline_->replace_project_bus_processing(project, error);
    }

    void set_transport_playing(bool playing) noexcept {
        if (pipeline_) pipeline_->set_transport_playing(playing);
    }
    [[nodiscard]] bool transport_playing() const noexcept {
        return pipeline_ && pipeline_->transport_playing();
    }
    void seek_transport(std::uint64_t frame) noexcept {
        if (pipeline_) pipeline_->seek_transport(frame);
    }

    // Pipeline buffers are allocated in start(), before the ALSA callback begins.
    bool prepare_output() const noexcept { return true; }

private:
    hush::AlsaAudioBackend backend_;
    std::unique_ptr<hush::RealtimeProjectAudioPipeline> pipeline_;
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
        transport_button_ = new QPushButton("Play Timeline");
        transport_button_->setObjectName("playTimelineButton");
#ifndef HUSH_GUI_HAS_ALSA
        transport_button_->setEnabled(false);
        transport_button_->setToolTip("Configure with -DHUSH_ENABLE_ALSA=ON to play local project clips.");
#endif
        bar->addWidget(transport_button_);
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

        bus_table_ = new QTableWidget(0, 5, root);
        bus_table_->setObjectName("busTable");
        bus_table_->setHorizontalHeaderLabels({"Bus", "Gain", "Mute", "Solo", "Stable ID"});
        bus_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        bus_table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
        bus_table_->verticalHeader()->hide();
        bus_table_->setMaximumHeight(180);
        layout->addWidget(bus_table_);

        insert_table_ = new QTableWidget(0, 5, root);
        insert_table_->setObjectName("insertTable");
        insert_table_->setHorizontalHeaderLabels({"Insert", "Bus", "Type", "Enabled", "Gain"});
        insert_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        insert_table_->setMaximumHeight(160);
        layout->addWidget(insert_table_);
        auto* insert_buttons = new QHBoxLayout;
        auto* add_gain_insert = new QPushButton("Add Gain Insert");
        add_gain_insert->setObjectName("addGainInsertButton");
        auto* add_soft_clip_insert = new QPushButton("Add Soft Clip Insert");
        add_soft_clip_insert->setObjectName("addSoftClipInsertButton");
        auto* remove_insert = new QPushButton("Remove Insert");
        remove_insert->setObjectName("removeInsertButton");
        insert_buttons->addWidget(add_gain_insert);
        insert_buttons->addWidget(add_soft_clip_insert);
        insert_buttons->addWidget(remove_insert);
        insert_buttons->addStretch();
        layout->addLayout(insert_buttons);

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
        bus_input_meter_label_ = new QLabel("BUS INPUTS · IDLE", root);
        bus_input_meter_label_->setObjectName("busInputMeters");
        bus_input_meter_label_->setMinimumHeight(24);
        layout->addWidget(bus_input_meter_label_);
        bus_output_meter_label_ = new QLabel("BUS OUTPUTS · IDLE", root);
        bus_output_meter_label_->setObjectName("busOutputMeters");
        bus_output_meter_label_->setMinimumHeight(24);
        layout->addWidget(bus_output_meter_label_);
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
        connect(add_gain_insert, &QPushButton::clicked, this, [this] { add_insert(hush::InsertKind::Gain); });
        connect(add_soft_clip_insert, &QPushButton::clicked, this, [this] { add_insert(hush::InsertKind::SoftClip); });
        connect(remove_insert, &QPushButton::clicked, this, [this] { remove_selected_insert(); });
        connect(undo, &QPushButton::clicked, this, [this] {
            const auto* operation = log_.cursor() > 0 ? log_.operation_at(log_.cursor() - 1) : nullptr;
            if (!operation) return;
            if (is_insert_operation(operation->kind)) {
                hush::Project candidate = project_;
                hush::OperationLog candidate_log = log_;
                if (!candidate_log.undo(candidate)) return;
                std::string error;
                if (!replace_live_bus_processing(candidate, error)) {
                    status_->setText("Undo not applied: bus insert graph replacement failed: " +
                                     QString::fromStdString(error));
                    return;
                }
                project_ = std::move(candidate);
                log_ = std::move(candidate_log);
                autosave_dirty_ = true;
                status_->setText("Undo applied");
                refresh();
                return;
            }
            if (is_bus_operation(operation->kind)) {
                auto values = bus_control_values(operation->entity_id);
                if (!values) return;
                apply_value_to_bus_controls(*operation, operation->before_value, *values);
                if (!enqueue_live_bus_control_values(operation->entity_id, *values)) {
                    status_->setText("Undo not applied: live bus command queue full; retry after audio drains");
                    return;
                }
                if (log_.undo(project_)) {
                    autosave_dirty_ = true;
                    status_->setText("Undo applied");
                    refresh();
                }
                return;
            }
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
            if (is_insert_operation(operation->kind)) {
                hush::Project candidate = project_;
                hush::OperationLog candidate_log = log_;
                if (!candidate_log.redo(candidate)) return;
                std::string error;
                if (!replace_live_bus_processing(candidate, error)) {
                    status_->setText("Redo not applied: bus insert graph replacement failed: " +
                                     QString::fromStdString(error));
                    return;
                }
                project_ = std::move(candidate);
                log_ = std::move(candidate_log);
                autosave_dirty_ = true;
                status_->setText("Redo applied");
                refresh();
                return;
            }
            if (is_bus_operation(operation->kind)) {
                auto values = bus_control_values(operation->entity_id);
                if (!values) return;
                apply_value_to_bus_controls(*operation, operation->after_value, *values);
                if (!enqueue_live_bus_control_values(operation->entity_id, *values)) {
                    status_->setText("Redo not applied: live bus command queue full; retry after audio drains");
                    return;
                }
                if (log_.redo(project_)) {
                    autosave_dirty_ = true;
                    status_->setText("Redo applied");
                    refresh();
                }
                return;
            }
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
        connect(transport_button_, &QPushButton::clicked, this, [this] { toggle_transport(); });
        connect(&status_timer_, &QTimer::timeout, this, [this] { poll_live_status(); poll_channel_light(); });
        status_timer_.start(250);
#endif

        connect(&autosave_timer_, &QTimer::timeout, this, [this] { poll_autosave(); });
        autosave_timer_.start(1000);

        project_.id = "hush-demo";
        project_.tracks = {{"track-1", "Input L", 1.0F, -1.0F, false, false},
                           {"track-2", "Input R", 0.8F, 1.0F, false, false}};
        project_.buses = {{"capture", "Capture"}, {"master", "Master"}};
        project_.routes = {{"capture", "master", hush::SendTap::PostFader, 1.0F}};
        log_.mark_dirty(); // The demo project has not been written to disk yet.
        refresh();
        status_->setText("Demo session · unsaved · offline");
    }

#ifdef HUSH_GUI_TESTING
    [[nodiscard]] const hush::Project& test_project() const noexcept { return project_; }
    [[nodiscard]] std::size_t test_history_cursor() const noexcept { return log_.cursor(); }
    bool test_load_project_from_path(const QString& path, bool accept_recovery) {
        return load_project_from_path(path, accept_recovery);
    }
    void test_poll_autosave() { poll_autosave(); }
    [[nodiscard]] QString test_status_text() const { return status_->text(); }
    [[nodiscard]] std::filesystem::path test_journal_path() const { return autosave_journal_path_; }
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
    std::filesystem::path autosave_journal_path_;
    std::unique_ptr<hush::AutosaveController> autosave_;
    QTimer autosave_timer_;
    bool autosave_dirty_{false};
    std::optional<hush::AutosaveController::Clock::time_point> autosave_retry_after_;
    QTableWidget* table_{};
    QTableWidget* bus_table_{};
    QTableWidget* insert_table_{};
    QLabel* status_{};
    QLabel* input_light_{};
    QLabel* process_light_{};
    QLabel* clip_light_{};
    QLabel* output_light_{};
    QProgressBar* input_meter_{};
    QProgressBar* output_meter_{};
    QLabel* bus_input_meter_label_{};
    QLabel* bus_output_meter_label_{};
    QPushButton* live_button_{};
    QPushButton* transport_button_{};
#ifdef HUSH_GUI_HAS_ALSA
    LiveMixer live_;
    QTimer status_timer_;
#endif

    void openProject() {
        if (!confirmProjectReplacement()) return;
        const auto path = QFileDialog::getOpenFileName(this, "Open HUSH Project", {}, "HUSH Project (*.json)");
        if (path.isEmpty()) return;
        (void)load_project_from_path(path, std::nullopt);
    }

    bool load_project_from_path(const QString& path, std::optional<bool> test_recovery_choice) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            if (!test_recovery_choice)
                QMessageBox::warning(this, "Open failed", file.errorString());
            return false;
        }
        hush::Project candidate;
        hush::Error error;
        if (!hush::from_json(QString::fromUtf8(file.readAll()).toStdString(), candidate, error)) {
            if (!test_recovery_choice)
                QMessageBox::warning(this, "Invalid project", QString::fromStdString(error.message));
            return false;
        }

        bool recovered_from_journal = false;
        const std::filesystem::path journal_path(path.toStdString() + ".hush-recovery.journal");
        std::error_code filesystem_error;
        const bool journal_exists = std::filesystem::exists(journal_path, filesystem_error);
        if (filesystem_error) {
            if (!test_recovery_choice)
                QMessageBox::warning(this, "Recovery check failed",
                                     QString::fromStdString(filesystem_error.message()));
        } else if (journal_exists) {
            hush::Project recovered = candidate;
            bool applied = false;
            hush::Error recovery_error;
            if (!hush::RecoveryJournal(journal_path).recover_if_different(
                    recovered, applied, recovery_error)) {
                if (!test_recovery_choice)
                    QMessageBox::warning(this, "Recovery unavailable",
                        "The saved project will open, but its recovery journal could not be read: " +
                        QString::fromStdString(recovery_error.message));
            } else if (applied) {
                const bool accept = test_recovery_choice.has_value()
                    ? *test_recovery_choice
                    : QMessageBox::question(
                          this, "Recover unsaved changes",
                          "A newer recovery snapshot exists for this project. Restore it?",
                          QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes;
                if (accept) {
                    candidate = std::move(recovered);
                    recovered_from_journal = true;
                } else {
                    // Respect an explicit refusal to recover this snapshot; otherwise
                    // the same stale journal could be offered again on the next launch.
                    std::error_code remove_error;
                    std::filesystem::remove(journal_path, remove_error);
                    if (remove_error && !test_recovery_choice)
                        QMessageBox::warning(this, "Recovery journal",
                            "Could not discard the declined recovery snapshot: " +
                            QString::fromStdString(remove_error.message()));
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
        return true;
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
        if (choice == hush::ProjectCloseChoice::Discard && autosave_) {
            std::error_code remove_error;
            std::filesystem::remove(autosave_journal_path_, remove_error);
            autosave_dirty_ = false;
            if (remove_error) {
                status_->setText("Could not discard recovery journal: " +
                                 QString::fromStdString(remove_error.message()));
            }
        }
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

    struct BusControlValues {
        float gain{1.0F};
        bool mute{false};
        bool solo{false};
    };

    static bool is_bus_operation(hush::OperationKind kind) noexcept {
        return kind == hush::OperationKind::SetBusGain ||
               kind == hush::OperationKind::SetBusMute ||
               kind == hush::OperationKind::SetBusSolo;
    }

    static bool is_insert_operation(hush::OperationKind kind) noexcept {
        return kind == hush::OperationKind::AddBusInsert ||
               kind == hush::OperationKind::RemoveBusInsert ||
               kind == hush::OperationKind::SetInsertGain ||
               kind == hush::OperationKind::SetInsertEnabled ||
               kind == hush::OperationKind::SetInsertKind;
    }

    bool replace_live_bus_processing(const hush::Project& candidate, std::string& error) {
#ifdef HUSH_GUI_HAS_ALSA
        if (live_.running() && !live_.replace_bus_processing(candidate, error)) return false;
#else
        (void)candidate;
        error.clear();
#endif
        return true;
    }

    bool commit_insert_operation(hush::Operation operation, hush::Project candidate,
                                 bool refresh_table) {
        hush::OperationLog candidate_log = log_;
        if (!candidate_log.apply_and_append(candidate, operation)) return false;
        std::string error;
        if (!replace_live_bus_processing(candidate, error)) {
            status_->setText("Insert edit not applied: graph replacement failed: " +
                             QString::fromStdString(error));
            return false;
        }
        project_ = std::move(candidate);
        log_ = std::move(candidate_log);
        autosave_dirty_ = true;
        status_->setText("Insert edited · undo history " + QString::number(log_.cursor()));
        if (refresh_table) refresh_insert_table();
        return true;
    }

    void add_insert(hush::InsertKind kind) {
        if (project_.buses.empty()) {
            status_->setText("Add a project bus before adding an insert");
            return;
        }
        int row = bus_table_->currentRow();
        if (row < 0 || static_cast<std::size_t>(row) >= project_.buses.size()) row = 0;
        const auto& bus_id = project_.buses[static_cast<std::size_t>(row)].id;
        std::size_t suffix = 1U;
        std::string id;
        do {
            id = bus_id + "-insert-" + std::to_string(suffix++);
        } while (std::any_of(project_.inserts.begin(), project_.inserts.end(),
                             [&](const hush::BusInsert& item) { return item.id == id; }));
        hush::BusInsert insert{id, bus_id, kind, true, 1.0F};
        hush::Operation operation{id, hush::OperationKind::AddBusInsert, 0.0F, 1.0F};
        operation.after_insert = insert;
        operation.after_index = project_.inserts.size();
        (void)commit_insert_operation(std::move(operation), project_, true);
    }

    void remove_selected_insert() {
        const int row = insert_table_->currentRow();
        if (row < 0 || static_cast<std::size_t>(row) >= project_.inserts.size()) {
            status_->setText("Select an insert to remove");
            return;
        }
        const auto& insert = project_.inserts[static_cast<std::size_t>(row)];
        hush::Operation operation{insert.id, hush::OperationKind::RemoveBusInsert, 1.0F, 0.0F};
        operation.before_insert = insert;
        operation.before_index = static_cast<std::size_t>(row);
        (void)commit_insert_operation(std::move(operation), project_, true);
    }

    void change_insert(const std::string& id, hush::OperationKind kind, float after) {
        const auto it = std::find_if(project_.inserts.begin(), project_.inserts.end(),
                                     [&](const hush::BusInsert& insert) { return insert.id == id; });
        if (it == project_.inserts.end()) return;
        float before = 0.0F;
        switch (kind) {
        case hush::OperationKind::SetInsertGain: before = it->gain; break;
        case hush::OperationKind::SetInsertEnabled: before = it->enabled ? 1.0F : 0.0F; break;
        case hush::OperationKind::SetInsertKind:
            before = it->kind == hush::InsertKind::SoftClip ? 1.0F : 0.0F;
            break;
        default: return;
        }
        if (before == after) return;
        hush::Operation operation{id, kind, before, after};
        (void)commit_insert_operation(std::move(operation), project_, false);
    }

    void refresh_insert_table() {
        insert_table_->setRowCount(static_cast<int>(project_.inserts.size()));
        for (std::size_t i = 0; i < project_.inserts.size(); ++i) {
            const auto& insert = project_.inserts[i];
            const int row = static_cast<int>(i);
            auto* id = new QTableWidgetItem(QString::fromStdString(insert.id));
            id->setFlags(id->flags() & ~Qt::ItemIsEditable);
            insert_table_->setItem(row, 0, id);
            auto* bus = new QTableWidgetItem(QString::fromStdString(insert.bus_id));
            bus->setFlags(bus->flags() & ~Qt::ItemIsEditable);
            insert_table_->setItem(row, 1, bus);
            auto* kind = new QComboBox;
            kind->setObjectName(QString("kind-insert-%1").arg(QString::fromStdString(insert.id)));
            kind->addItem("Gain");
            kind->addItem("Soft Clip");
            kind->setCurrentIndex(insert.kind == hush::InsertKind::SoftClip ? 1 : 0);
            insert_table_->setCellWidget(row, 2, kind);
            auto* enabled = new QCheckBox("Enabled");
            enabled->setObjectName(QString("enabled-insert-%1").arg(QString::fromStdString(insert.id)));
            enabled->setChecked(insert.enabled);
            insert_table_->setCellWidget(row, 3, enabled);
            auto* gain = new QSlider(Qt::Horizontal);
            gain->setObjectName(QString("gain-insert-%1").arg(QString::fromStdString(insert.id)));
            gain->setRange(0, 1600);
            gain->setValue(static_cast<int>(insert.gain * 100.0F));
            gain->setToolTip("Insert gain 0.00–16.00 (used by Gain insert)");
            insert_table_->setCellWidget(row, 4, gain);
            const auto insert_id = insert.id;
            connect(kind, qOverload<int>(&QComboBox::currentIndexChanged), this,
                    [this, insert_id](int index) {
                        change_insert(insert_id, hush::OperationKind::SetInsertKind,
                                      index == 0 ? 0.0F : 1.0F);
                    });
            connect(enabled, &QCheckBox::clicked, this,
                    [this, enabled, insert_id] {
                        change_insert(insert_id, hush::OperationKind::SetInsertEnabled,
                                      enabled->isChecked() ? 1.0F : 0.0F);
                    });
            connect(gain, &QSlider::sliderReleased, this,
                    [this, gain, insert_id] {
                        change_insert(insert_id, hush::OperationKind::SetInsertGain,
                                      gain->value() / 100.0F);
                    });
        }
    }

    std::optional<BusControlValues> bus_control_values(const std::string& id) const {
        const auto it = std::find_if(project_.buses.begin(), project_.buses.end(),
                                     [&](const hush::Bus& bus) { return bus.id == id; });
        if (it == project_.buses.end()) return std::nullopt;
        return BusControlValues{it->gain, it->mute, it->solo};
    }

    static void apply_value_to_bus_controls(const hush::Operation& operation, float value,
                                            BusControlValues& controls) {
        switch (operation.kind) {
        case hush::OperationKind::SetBusGain: controls.gain = value; break;
        case hush::OperationKind::SetBusMute: controls.mute = value != 0.0F; break;
        case hush::OperationKind::SetBusSolo: controls.solo = value != 0.0F; break;
        default: break;
        }
    }

    bool enqueue_live_bus_control_values(const std::string& id, const BusControlValues& values) {
#ifdef HUSH_GUI_HAS_ALSA
        if (live_.running() &&
            !live_.enqueue_bus_control_values(project_, id, values.gain, values.mute, values.solo))
            return false;
#else
        (void)id;
        (void)values;
#endif
        return true;
    }

    void preview_live_bus_gain(const std::string& id, float gain) {
        auto values = bus_control_values(id);
        if (!values) return;
        values->gain = gain;
        if (!enqueue_live_bus_control_values(id, *values))
            status_->setText("Live bus preview queue full — release slider or stop audio");
    }

    void change_bus(const std::string& id, hush::OperationKind kind, float after) {
        const auto it = std::find_if(project_.buses.begin(), project_.buses.end(),
                                     [&](const hush::Bus& bus) { return bus.id == id; });
        if (it == project_.buses.end()) return;
        float before = 0.0F;
        switch (kind) {
        case hush::OperationKind::SetBusGain: before = it->gain; break;
        case hush::OperationKind::SetBusMute: before = it->mute ? 1.0F : 0.0F; break;
        case hush::OperationKind::SetBusSolo: before = it->solo ? 1.0F : 0.0F; break;
        default: return;
        }
        if (before == after) return;
        BusControlValues values{it->gain, it->mute, it->solo};
        const hush::Operation operation{id, kind, before, after};
        apply_value_to_bus_controls(operation, after, values);
        if (!enqueue_live_bus_control_values(id, values)) {
            status_->setText("Bus edit not applied: live command queue full; retry after audio drains");
            QTimer::singleShot(0, this, [this] { refresh_bus_table(); });
            return;
        }
        if (log_.apply_and_append(project_, operation)) {
            autosave_dirty_ = true;
            status_->setText("Bus edited · undo history " + QString::number(log_.cursor()));
        }
    }

    void refresh_bus_table() {
        bus_table_->setRowCount(static_cast<int>(project_.buses.size()));
        for (std::size_t i = 0; i < project_.buses.size(); ++i) {
            const auto& bus = project_.buses[i];
            const int row = static_cast<int>(i);
            auto* name = new QTableWidgetItem(QString::fromStdString(bus.name));
            name->setFlags(name->flags() & ~Qt::ItemIsEditable);
            bus_table_->setItem(row, 0, name);
            auto* gain = new QSlider(Qt::Horizontal);
            gain->setObjectName(QString("gain-bus-%1").arg(QString::fromStdString(bus.id)));
            gain->setRange(0, 1600);
            gain->setValue(static_cast<int>(bus.gain * 100.0F));
            gain->setToolTip("Bus gain 0.00–16.00");
            bus_table_->setCellWidget(row, 1, gain);
            auto* mute = new QCheckBox("Mute");
            mute->setObjectName(QString("mute-bus-%1").arg(QString::fromStdString(bus.id)));
            mute->setChecked(bus.mute);
            bus_table_->setCellWidget(row, 2, mute);
            auto* solo = new QCheckBox("Solo");
            solo->setObjectName(QString("solo-bus-%1").arg(QString::fromStdString(bus.id)));
            solo->setChecked(bus.solo);
            bus_table_->setCellWidget(row, 3, solo);
            auto* id = new QTableWidgetItem(QString::fromStdString(bus.id));
            id->setFlags(id->flags() & ~Qt::ItemIsEditable);
            bus_table_->setItem(row, 4, id);
            const auto bus_id = bus.id;
            connect(gain, &QSlider::valueChanged, this,
                    [this, gain, bus_id](int) { preview_live_bus_gain(bus_id, gain->value() / 100.0F); });
            connect(gain, &QSlider::sliderReleased, this,
                    [this, gain, bus_id] { change_bus(bus_id, hush::OperationKind::SetBusGain, gain->value() / 100.0F); });
            connect(mute, &QCheckBox::clicked, this,
                    [this, mute, bus_id] { change_bus(bus_id, hush::OperationKind::SetBusMute, mute->isChecked() ? 1.0F : 0.0F); });
            connect(solo, &QCheckBox::clicked, this,
                    [this, solo, bus_id] { change_bus(bus_id, hush::OperationKind::SetBusSolo, solo->isChecked() ? 1.0F : 0.0F); });
        }
    }

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
        default: break;
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
        default: return;
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
        if (!running) {
            bus_input_meter_label_->setText("BUS INPUTS · IDLE");
            bus_output_meter_label_->setText("BUS OUTPUTS · IDLE");
        } else {
            QString summary = "BUS INPUTS · peak L/R · RMS L/R: ";
            const auto count = live_.bus_input_meter_count();
            for (std::size_t bus = 0; bus < count; ++bus) {
                const auto meter = live_.bus_input_meter(bus);
                if (bus != 0U) summary += " | ";
                summary += "B" + QString::number(static_cast<qulonglong>(bus)) + ": ";
                summary += QString::number(meter.peak_left, 'f', 2) + "/" +
                           QString::number(meter.peak_right, 'f', 2) + " · ";
                summary += QString::number(meter.rms_left, 'f', 2) + "/" +
                           QString::number(meter.rms_right, 'f', 2);
            }
            bus_input_meter_label_->setText(count == 0U ? "BUS INPUTS · NO METERS" : summary);

            QString output_summary = "BUS OUTPUTS · post-fader peak L/R · RMS L/R: ";
            const auto output_count = live_.bus_output_meter_count();
            for (std::size_t bus = 0; bus < output_count; ++bus) {
                const auto meter = live_.bus_output_meter(bus);
                if (bus != 0U) output_summary += " | ";
                output_summary += "B" + QString::number(static_cast<qulonglong>(bus)) + ": ";
                output_summary += QString::number(meter.peak_left, 'f', 2) + "/" +
                                  QString::number(meter.peak_right, 'f', 2) + " · ";
                output_summary += QString::number(meter.rms_left, 'f', 2) + "/" +
                                  QString::number(meter.rms_right, 'f', 2);
            }
            bus_output_meter_label_->setText(output_count == 0U ? "BUS OUTPUTS · NO METERS" : output_summary);
        }
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

    void toggle_transport() {
        if (!live_.running() || project_.clips.empty()) {
            status_->setText("Start Live ALSA with a project containing clips before timeline playback");
            return;
        }
        const bool playing = !live_.transport_playing();
        live_.set_transport_playing(playing);
        transport_button_->setText(playing ? "Pause Timeline" : "Play Timeline");
        status_->setText(playing ? "Timeline playback active" : "Timeline playback paused");
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
        const auto project_directory = project_path_.isEmpty()
            ? std::filesystem::path{}
            : std::filesystem::path(QFileInfo(project_path_).absolutePath().toStdString());
        if (!live_.start(project_, project_directory, error)) {
            status_->setText("ALSA start failed: " + QString::fromStdString(error));
            return;
        }
        live_button_->setText("Stop Live ALSA");
        status_->setText("Live ALSA · tracks 1–2 → project buses; later tracks have no live source");
        refresh();
    }
#endif

    void configure_autosave(const QString& project_path) {
        if (autosave_ && autosave_project_path_ == project_path) return;
        autosave_project_path_ = project_path;
        autosave_journal_path_ = std::filesystem::path(
            project_path.toStdString() + ".hush-recovery.journal");
        autosave_ = std::make_unique<hush::AutosaveController>(
            hush::RecoveryJournal(autosave_journal_path_));
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
        refresh_bus_table();
        refresh_insert_table();
#ifdef HUSH_GUI_HAS_ALSA
        const bool can_play_timeline = live_.running() && !project_.clips.empty();
        transport_button_->setEnabled(can_play_timeline);
        transport_button_->setText(live_.transport_playing() ? "Pause Timeline" : "Play Timeline");
#else
        transport_button_->setEnabled(false);
#endif
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
