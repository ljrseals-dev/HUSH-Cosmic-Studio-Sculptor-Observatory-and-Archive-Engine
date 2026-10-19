#include "hush/mixer.hpp"
#include "hush/operation_log.hpp"
#include <cassert>
#include <cmath>
int main() {
    using namespace hush;
    Project p; p.id="session-A"; p.tracks={{"track-1","Lead",0.75F,0.0F,false,false},{"track-2","Pad",0.5F,-0.5F,false,false}};
    const auto json=to_json(p); Project loaded; Error err;
    assert(from_json(json,loaded,err)); assert(loaded==p);
    Project bad; assert(!from_json(R"({"schema_version":2})",bad,err)); assert(err.code=="schema_version_unsupported");

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
    return 0;
}
