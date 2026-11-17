#include "hush/take_coordinator.hpp"
#include <cassert>
int main() {
    using namespace hush;
    Project p; p.id = "session-A";
    p.clips = {{"clip-a","track-a","source-a",0,0,128},
               {"clip-b","track-b","source-b",0,0,128},
               {"clip-c","track-a","source-c",128,0,128}};
    Error error;
    assert(assign_recording_take(p, {"clip-a","clip-b"}, error));
    assert(error.code.empty());
    assert(p.clips[0].take_id == "session-A/take/1");
    assert(p.clips[1].take_id == p.clips[0].take_id);
    assert(p.clips[0].take_number == 1U && p.clips[1].take_number == 1U);
    assert(take_color_index(p.clips[0].take_id) == take_color_index(p.clips[1].take_id));
    assert(assign_recording_take(p, {"clip-c"}, error));
    assert(p.clips[2].take_id == "session-A/take/2");
    Project invalid; invalid.id = "session-B";
    invalid.clips = {{"clip-x","track-x","source-x",0,0,64},{"clip-y","track-y","source-y",0,0,64}};
    assert(!assign_recording_take(invalid, {"clip-x","missing"}, error));
    assert(error.code == "take_clip_unknown" && invalid.clips[0].take_id.empty());
    assert(!assign_recording_take(invalid, {"clip-x","clip-x"}, error));
    assert(error.code == "take_clip_duplicate");
    assert(!assign_recording_take(invalid, {}, error));
    assert(error.code == "take_clip_list_empty");
    return 0;
}
