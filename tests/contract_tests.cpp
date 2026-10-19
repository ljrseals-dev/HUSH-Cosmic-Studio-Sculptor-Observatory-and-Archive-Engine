#include "hush/mixer.hpp"
#include "hush/operation_log.hpp"
#include "hush/recovery_journal.hpp"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>

int main() {
    using namespace hush;
    Project p; p.id="session-A"; p.tracks={{"track-1","Lead",0.75F,0.0F,false,false},{"track-2","Pad",0.5F,-0.5F,false,false}};
    p.buses={{"bus-master","Master"},{"bus-music","Music"}};
    p.routes={{"bus-music","bus-master",SendTap::PostFader,0.8F}};
    const auto json=to_json(p); Project loaded; Error err;
    assert(from_json(json,loaded,err)); assert(loaded==p);
    Project bad; assert(!from_json(R"({"schema_version":2})",bad,err)); assert(err.code=="schema_version_unsupported");

    // Older schema-v1 project files without bus/route arrays remain readable.
    const std::string legacy=R"({"schema_version":1,"id":"old","pan_law":"equal_power_minus_3_db","tracks":[]})";
    assert(from_json(legacy,loaded,err)); assert(loaded.buses.empty() && loaded.routes.empty());

    // A cycle in persisted routing is rejected at the project boundary.
    const std::string cyclic=R"({"schema_version":1,"id":"cycle","pan_law":"equal_power_minus_3_db","tracks":[],"buses":[{"id":"a","name":"A"},{"id":"b","name":"B"}],"routes":[{"source_bus_id":"a","destination_bus_id":"b","tap":"post_fader","gain":1},{"source_bus_id":"b","destination_bus_id":"a","tap":"pre_fader","gain":1}]})";
    assert(!from_json(cyclic,loaded,err)); assert(err.code=="bus_route_cycle");

    OperationLog log; const Project before=p;
    log.append({"track-1",OperationKind::SetGain,0.75F,0.25F});
    p.tracks[0].gain=0.25F; assert(log.undo(p)); assert(p.tracks[0].gain==0.75F);
    assert(log.redo(p)); assert(p.tracks[0].gain==0.25F);
    assert(log.undo(p)); assert(p==before);

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
    // Simulate a process dying during the next record. Recovery must retain the last
    // complete record instead of accepting a partial JSON payload.
    { std::ofstream tail(journal_path,std::ios::binary|std::ios::app); tail << "HUSHJ1 200\npartial"; }
    Project recovered;
    assert(journal.recover_latest(recovered,err));
    assert(recovered==second);
    std::filesystem::remove(journal_path,cleanup_error);
    return 0;
}
