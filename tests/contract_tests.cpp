#include "hush/autosave_controller.hpp"
#include "hush/mixer.hpp"
#include "hush/operation_log.hpp"
#include "hush/recovery_journal.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
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
    Project p; p.id="session-\\"A\\"\\\\folder";
    p.tracks={{"track-1","Lead \\"main\\"\\\\take",0.75F,0.0F,false,false},
              {"track-2","Pad",0.5F,-0.5F,false,false}};
    p.buses={{"bus-\\"master\\"\\\\out","Master \\"bus\\""},{"bus-music","Music"}};
    p.routes={{"bus-music","bus-\\"master\\"\\\\out",SendTap::PostFader,0.8F}};
    p.media_sources={{"source-1","audio\\\\take\\"one\\".wav",48000U,2U,1000U}};
    p.clips={{"clip-\\"one\\"\\\\v1","track-1","source-1",0U,0U,128U,1.0F}};
    p.tempo_map={{0.0,120.0},{16.125,89.123456789123}};
    p.automation_lanes={{"automation-\\\"gain\\\"\\\\lane","track-1.gain\\\"quoted\\\\target",ProjectAutomationInterpolation::Linear,
                         {{0.0,0.5F},{8.0,0.8F},{16.0,0.25F}}},
                        {"automation-mute","track-2.mute",ProjectAutomationInterpolation::Step,
                         {{0.0,0.0F},{4.0,1.0F}}}};
    const auto json=to_json(p); Project loaded; Error err;
    if (!from_json(json,loaded,err)) {
        std::cerr << "Project round-trip failed: " << err.code << ": " << err.message << '\\n';
        return 1;
    }
    assert(loaded==p);
    const std::string malformed_track_number=R"({"schema_version":1,"id":"bad-number","pan_law":"equal_power_minus_3_db","tracks":[{"id":"t","name":"T","gain":0.5.7,"pan":0,"mute":false,"solo":false}]})";
    assert(!from_json(malformed_track_number,loaded,err));
    assert(err.code=="track_number_invalid");
        const std::string invalid_tempo=R"({"schema_version":1,"id":"bad-tempo","pan_law":"equal_power_minus_3_db","tracks":[],"tempo_map":[{"beat":1,"bpm":120}]})";
    assert(!from_json(invalid_tempo,loaded,err)); assert(err.code=="tempo_point_invalid");
    const std::string malformed_tempo_number=R"({"schema_version":1,"id":"bad-number","pan_law":"equal_power_minus_3_db","tracks":[],"tempo_map":[{"beat":0,"bpm":120.5.6}]})";
    assert(!from_json(malformed_tempo_number,loaded,err));
    assert(err.code=="tempo_point_number_invalid");
    const std::string invalid_lane=R"({"schema_version":1,"id":"bad-lane","pan_law":"equal_power_minus_3_db","tracks":[],"automation_lanes":[{"id":"x","target":"track.gain","interpolation":"linear","points":[{"beat":0,"value":0.5},{"beat":0,"value":0.7}]}]})";
    assert(!from_json(invalid_lane,loaded,err)); assert(err.code=="automation_point_invalid");
    const std::string malformed_automation_number=R"({"schema_version":1,"id":"bad-number","pan_law":"equal_power_minus_3_db","tracks":[],"automation_lanes":[{"id":"x","target":"track.gain","interpolation":"linear","points":[{"beat":0,"value":0.5.7}]}]})";
    assert(!from_json(malformed_automation_number,loaded,err));
    assert(err.code=="automation_point_number_invalid");
    Project bad; assert(!from_json(R"({"schema_version":2})",bad,err)); assert(err.code=="schema_version_unsupported");

    const std::string legacy=R"({"schema_version":1,"id":"old","pan_law":"equal_power_minus_3_db","tracks":[]})";
    assert(from_json(legacy,loaded,err)); assert(loaded.buses.empty() && loaded.routes.empty());

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
