#include "hush/spsc_parameter_queue.hpp"

#include <cassert>
#include <cstdint>

struct ParameterChange {
    std::uint32_t parameter_id{};
    float value{};
};

int main() {
    hush::SpscParameterQueue<ParameterChange, 3> queue;
    assert(queue.capacity() == 3);
    assert(queue.size_approx() == 0);

    ParameterChange received{};
    assert(!queue.try_pop(received));

    assert(queue.try_push({1, 0.25F}));
    assert(queue.try_push({2, 0.5F}));
    assert(queue.try_push({3, 0.75F}));
    assert(!queue.try_push({4, 1.0F}));
    assert(queue.size_approx() == 3);

    assert(queue.try_pop(received));
    assert(received.parameter_id == 1 && received.value == 0.25F);
    assert(queue.try_push({4, 1.0F})); // wrapped slot reuse

    assert(queue.try_pop(received));
    assert(received.parameter_id == 2 && received.value == 0.5F);
    assert(queue.try_pop(received));
    assert(received.parameter_id == 3 && received.value == 0.75F);
    assert(queue.try_pop(received));
    assert(received.parameter_id == 4 && received.value == 1.0F);
    assert(!queue.try_pop(received));
    assert(queue.size_approx() == 0);

    // Repeated single-threaded wrap-around catches index/modulo regressions.
    for (std::uint32_t i = 0; i < 10000; ++i) {
        assert(queue.try_push({i, static_cast<float>(i)}));
        assert(queue.try_pop(received));
        assert(received.parameter_id == i);
        assert(received.value == static_cast<float>(i));
    }
    return 0;
}
