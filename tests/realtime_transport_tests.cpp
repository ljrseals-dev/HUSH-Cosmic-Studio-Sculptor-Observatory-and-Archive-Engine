#include "hush/realtime_transport.hpp"

#include <cassert>
#include <cstdint>

int main() {
    hush::RealtimeTransport<4> transport;
    assert(transport.begin_block(128) == 0);
    assert(!transport.is_playing());
    assert(transport.position_samples() == 0);

    assert(transport.enqueue_play());
    assert(transport.begin_block(128) == 0);
    assert(transport.is_playing());
    assert(transport.position_samples() == 128);
    assert(transport.begin_block(64) == 128);
    assert(transport.position_samples() == 192);

    assert(transport.enqueue_seek(48000));
    assert(transport.begin_block(256) == 48000);
    assert(transport.position_samples() == 48256);

    assert(transport.enqueue_stop());
    assert(transport.begin_block(128) == 48256);
    assert(!transport.is_playing());
    assert(transport.position_samples() == 48256);

    assert(transport.enqueue_play());
    assert(transport.begin_block(1) == 48256);
    assert(transport.position_samples() == 48257);

    // Queue overflow is explicit and never blocks the producer.
    hush::RealtimeTransport<2> small;
    assert(small.enqueue_play());
    assert(small.enqueue_stop());
    assert(!small.enqueue_seek(99));
    return 0;
}
