#include "hush/autosave_controller.hpp"
#include "hush/mixer.hpp"
#include "hush/operation_log.hpp"
#include "hush/recovery_journal.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
bool injected_failure = false;
bool injected_writer(const std::filesystem::path& path, const hush::Project& project, hush::Error& error) {
    if (injected_failure) {
        error = {"journal_write_failed", project.id, "injected write failure", "error"};
        return false;
    }
    return hush::RecoveryJournal(path).append_snapshot(project, error);
}
}

int main() {
    using namespace hush;
    Project p; p.id="session-A"; p.tracks={{"track-1","Lead",0.75F,0.0F,false,false},{"track-2","Pad",0.5F,-0.5F,false,false}};
    p.buses={{"bus-master","Master"},{"bus-music","Music"}};
    p.routes={{"bus-music","bus-master",SendTap::PostFader,0.8F}};
    const auto json=to_json(p); Project loaded; Error err;
    assert(from_json(json,loaded,err)); assert(loaded==p);
    Project bad; assert(!from_json(R"({"schema_version":2})",bad,err)); assert(err.code=="schema_version_unsupported");

    const std::string legacy=R"({"schema_version":1,"id":"old","pan_law":"equal_power_minus_3_db","tracks":[]})";
    assert(from_json(legacy,loaded,err)); assert(loaded.buses.empty() && loaded.routes.empty());

    // Schema-v1 sessions remain valid through the configured ceiling; the next
    // track is rejected with a stable, user-readable error code.
    Project at_ceiling;
    at_ceiling.id = "track-capacity-boundary";
    at_ceiling.tracks.reserve(kMaxTracks);
    for (std::size_t i = 0; i < kMaxTracks; ++i) {
        at_ceiling.tracks.push_back({"track-" + std::to_string(i),
                                     "Track " + std::to_string(i), 1.0F, 0.0F, false, false});
    }
    Project ceiling_loaded;
    assert(from_json(to_json(at_ceiling), ceiling_loaded, err));
    assert(ceiling_loaded.tracks.size() == kMaxTracks);
    at_ceiling.tracks.push_back({"track-over", "Over ceiling", 1.0F, 0.0F, false, false});
    assert(!from_json(to_json(at_ceiling), ceiling_loaded, err));
    assert(err.code == "track_count_exceeded");

    const std::string cyclic=R"({"schema_version":1,"id":"cycle","pan_law":"equal_power_minus_3_db","tracks":[],"buses":[{"id":"a","name":"A"},{"id":"b","name":"B"}],"routes":[{"source_bus_id":"a","destination_bus_id":"b","tap":"post_fader","gain":1},{"source_bus_id":"b","destination_bus_id":"a","tap":"pre_fader","gain":1}]})";
    assert(!from_json(cyclic,loaded,err)); assert(err.code=="bus_route_cycle");

    OperationLog log; const Project before=p;
    assert(!log.dirty());
    log.append({"track-1",OperationKind::SetGain,0.75F,0.25F});
    assert(log.dirty());
    log.mark_saved(); assert(!log.dirty());
    assert(log.operation_at(0) != nullptr);
    assert(log.operation_at(0)->entity_id == "track-1");
    assert(log.operation_at(1) == nullptr);
    p.tracks[0].gain=0.25F; assert(log.undo(p)); assert(p.tracks[0].gain==0.75F); assert(log.dirty());
    assert(log.redo(p)); assert(p.tracks[0].gain==0.25F); assert(log.dirty());
    assert(log.undo(p)); assert(p==before); assert(log.dirty());

    // Structural track operations participate in the same undo/redo history.
    Project structural_project;
    structural_project.id = "structural-track-edit";
    OperationLog structural_log;
    const Track added_track{"track-added", "Added Track", 1.0F, 0.0F, false, false};
    Operation add;
    add.entity_id = added_track.id;
    add.kind = OperationKind::AddTrack;
    add.track_index = 0U;
    add.track_snapshot = added_track;
    assert(structural_log.apply_and_append(structural_project, add));
    assert(structural_project.tracks.size() == 1U);
    assert(structural_log.undo(structural_project));
    assert(structural_project.tracks.empty());
    assert(structural_log.redo(structural_project));
    assert(structural_project.tracks.size() == 1U && structural_project.tracks[0] == added_track);

    Operation remove;
    remove.entity_id = added_track.id;
    remove.kind = OperationKind::RemoveTrack;
    remove.track_index = 0U;
    remove.track_snapshot = added_track;
    assert(structural_log.apply_and_append(structural_project, remove));
    assert(structural_project.tracks.empty());
    assert(structural_log.undo(structural_project));
    assert(structural_project.tracks.size() == 1U && structural_project.tracks[0] == added_track);
    assert(structural_log.redo(structural_project));
    assert(structural_project.tracks.empty());

    std::vector<std::vector<float>> audio{{1.0F,0.5F},{1.0F,0.5F}};
    p.tracks[0].solo=true;
    const auto mixed=mix_tracks(p,audio);
    assert(mixed.left.size()==2 && mixed.right.size()==2);
    assert(std::abs(mixed.left[0]-std::sqrt(0.5F)*0.75F)<1.0e-5F);
    assert(std::abs(mixed.right[0]-std::sqrt(0.5F)*0.75F)<1.0e-5F);

    const auto journal_path=std::filesystem::temp_directory_path()/"hush-recovery-contract.journal";
    std::error_code cleanup_error;
    std::filesystem::remove(journal_path,cleanup_error);
    RecoveryJournal journal(journal_path);
    Project first=p; first.id="recover-1";
    Project second=p; second.id="recover-2"; second.buses.push_back({"bus-vox","Vocals"});
    assert(journal.append_snapshot(first,err));
    assert(journal.append_snapshot(second,err));
    { std::ofstream tail(journal_path,std::ios::binary|std::ios::app); tail << "HUSHJ1 200\npartial"; }
    Project recovered;
    assert(journal.recover_latest(recovered,err));
    assert(recovered==second);
    bool recovery_applied = false;
    Project canonical = second;
    assert(journal.recover_if_different(canonical, recovery_applied, err));
    assert(!recovery_applied && canonical == second);
    canonical.id = "older-canonical";
    assert(journal.recover_if_different(canonical, recovery_applied, err));
    assert(recovery_applied && canonical == second);
    std::filesystem::remove(journal_path,cleanup_error);

    // Autosave schedule: first poll saves, early poll does not, interval and stop do.
    const auto autosave_path=std::filesystem::temp_directory_path()/"hush-autosave-contract.journal";
    std::filesystem::remove(autosave_path,cleanup_error);
    AutosaveController autosave(RecoveryJournal(autosave_path),std::chrono::milliseconds(100));
    const auto t0=AutosaveController::Clock::time_point{};
    Project autosave_project; autosave_project.id="autosave-first";
    assert(autosave.poll(autosave_project,t0,false,err)==AutosaveController::Outcome::Saved);
    autosave_project.id="autosave-too-soon";
    assert(autosave.poll(autosave_project,t0+std::chrono::milliseconds(50),false,err)==AutosaveController::Outcome::NotDue);
    autosave_project.id="autosave-periodic";
    assert(autosave.poll(autosave_project,t0+std::chrono::milliseconds(100),true,err)==AutosaveController::Outcome::Saved);
    autosave_project.id="autosave-stop";
    assert(autosave.poll(autosave_project,t0+std::chrono::milliseconds(110),false,err)==AutosaveController::Outcome::Saved);
    assert(RecoveryJournal(autosave_path).recover_latest(recovered,err));
    assert(recovered.id=="autosave-stop");
    bool dirty = false;
    assert(autosave.poll_if_dirty(autosave_project, t0 + std::chrono::milliseconds(200), dirty, false, err) == AutosaveController::Outcome::NotDue);
    assert(!dirty);
    dirty = true;
    assert(autosave.poll_if_dirty(autosave_project, t0 + std::chrono::milliseconds(250), dirty, false, err) == AutosaveController::Outcome::Saved);
    assert(!dirty && autosave.last_save_error() == SaveError::None);

    const auto failure_path = std::filesystem::temp_directory_path() / "hush-autosave-failure-contract.journal";
    std::filesystem::remove(failure_path, cleanup_error);
    AutosaveController failing(RecoveryJournal(failure_path, &injected_writer), std::chrono::milliseconds(1));
    injected_failure = true;
    dirty = true;
    assert(failing.poll_if_dirty(autosave_project, t0, dirty, false, err) == AutosaveController::Outcome::Failed);
    assert(dirty && failing.last_save_error() == SaveError::JournalWriteFailed);
    assert(failing.last_save_message() == "injected write failure");
    failing.report_project_write_failure("project write denied");
    assert(failing.last_save_error() == SaveError::ProjectWriteFailed);
    assert(failing.last_save_message() == "project write denied");
    injected_failure = false;
    assert(failing.save_now(autosave_project, err) == AutosaveController::Outcome::Saved);
    assert(failing.last_save_error() == SaveError::None);
    std::filesystem::remove(failure_path, cleanup_error);
    std::filesystem::remove(autosave_path,cleanup_error);
    return 0;
}
