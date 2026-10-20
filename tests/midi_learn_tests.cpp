#include "hush/midi_learn.hpp"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace hush;

    MidiLearnMap map;
    const MidiLearnBinding bindings[] = {
        {0, 7, 1001, 0.0F, 1.0F, false},
        {1, 10, 1002, -1.0F, 1.0F, true},
        {15, 127, 1003, 20.0F, 20000.0F, false},
    };
    assert(map.set_bindings(bindings, 3));
    assert(map.size() == 3);

    MappedParameterValue output{};
    assert(map.map_cc({0, 7, 0}, output));
    assert(output.parameter_id == 1001 && output.value == 0.0F);
    assert(map.map_cc({0, 7, 127}, output));
    assert(output.parameter_id == 1001 && output.value == 1.0F);
    assert(map.map_cc({0, 7, 64}, output));
    assert(std::abs(output.value - (64.0F / 127.0F)) < 1.0e-6F);

    assert(map.map_cc({1, 10, 0}, output));
    assert(output.parameter_id == 1002 && output.value == 1.0F);
    assert(map.map_cc({1, 10, 127}, output));
    assert(output.parameter_id == 1002 && output.value == -1.0F);
    assert(!map.map_cc({2, 10, 64}, output));
    assert(!map.map_cc({16, 10, 64}, output));
    assert(!map.map_cc({0, 128, 64}, output));
    assert(!map.map_cc({0, 7, 128}, output));

    const MidiLearnBinding duplicate[] = {
        {0, 1, 10, 0.0F, 1.0F, false},
        {0, 1, 11, 0.0F, 1.0F, false},
    };
    assert(!map.set_bindings(duplicate, 2));
    assert(map.size() == 3); // invalid update does not destroy prior map
    assert(map.map_cc({0, 7, 127}, output) && output.parameter_id == 1001);

    const MidiLearnBinding invalid_range[] = {{0, 1, 12, 2.0F, 1.0F, false}};
    assert(!map.set_bindings(invalid_range, 1));
    const MidiLearnBinding invalid_id[] = {{0, 1, 0, 0.0F, 1.0F, false}};
    assert(!map.set_bindings(invalid_id, 1));
    const MidiLearnBinding invalid_nan[] = {
        {0, 1, 12, std::numeric_limits<float>::quiet_NaN(), 1.0F, false}
    };
    assert(!map.set_bindings(invalid_nan, 1));

    assert(map.set_bindings(nullptr, 0));
    assert(map.size() == 0);
    assert(!map.map_cc({0, 7, 127}, output));
    return 0;
}
