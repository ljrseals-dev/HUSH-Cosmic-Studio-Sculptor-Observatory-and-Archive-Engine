#include "hush/alsa_audio_backend.hpp"
#include "hush/bus_graph.hpp"
#include "hush/published_realtime_bus_audio_adapter.hpp"
#include "hush/published_realtime_bus_engine.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

namespace {
volatile std::sig_atomic_t keep_running = 1;

void handle_signal(int) {
    keep_running = 0;
}
} // namespace

int main() {
    using namespace hush;

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    RoutingError routing_error;
    BusGraph graph;
    if (!graph.configure(
            {{"capture", "Capture"}, {"master", "Master"}},
            {{"capture", "master", SendTap::PostFader, 1.0F}},
            routing_error)) {
        std::cerr << "Bus graph setup failed: " << routing_error.code
                  << ": " << routing_error.message << '\n';
        return 1;
    }

    constexpr std::size_t max_frames = 4096;
    PublishedRealtimeBusEngine engine;
    ProcessorSetupError setup_error;
    if (!engine.publish_graph(graph, max_frames, setup_error)) {
        std::cerr << "Realtime engine setup failed: " << setup_error.code
                  << ": " << setup_error.message << '\n';
        return 1;
    }

    PublishedRealtimeBusAudioAdapter adapter;
    std::string adapter_error;
    if (!adapter.configure(engine, graph.buses().size(), 0, 1, max_frames,
                           adapter_error)) {
        std::cerr << "Audio adapter setup failed: " << adapter_error << '\n';
        return 1;
    }

    AlsaAudioBackend backend;
    AlsaAudioBackend::Config config;
    config.sample_rate = 48000;
    config.period_frames = 128;
    std::string backend_error;
    if (!backend.start(config, &PublishedRealtimeBusAudioAdapter::alsa_callback,
                      &adapter, backend_error)) {
        std::cerr << "ALSA startup failed: " << backend_error << '\n';
        return 1;
    }

    std::cout << "HUSH ALSA realtime host running at "
              << backend.actual_sample_rate() << " Hz, "
              << backend.actual_period_frames() << " frames/period.\n"
              << "Capture is routed to the Master bus through the guarded published engine. "
              << "Press Ctrl-C to stop.\n";

    while (keep_running != 0 && backend.running()) {
        engine.reclaim_retired();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    backend.stop();
    engine.reclaim_retired();
    std::cout << "Stopped. Callbacks: " << backend.callback_count()
              << ", xruns: " << backend.xrun_count()
              << ", recoveries: " << backend.recovery_count()
              << ", short transfers: " << backend.short_transfer_count()
              << '\n';
    return 0;
}
