#include "hush/alsa_audio_backend.hpp"
#include "hush/bus_graph.hpp"
#include "hush/realtime_bus_engine_host.hpp"
#include "hush/realtime_bus_engine_host_audio_adapter.hpp"

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
    RealtimeBusEngineHost engine_host;
    ProcessorSetupError setup_error;
    if (!engine_host.replace_graph(graph, max_frames, "master", setup_error)) {
        std::cerr << "Realtime engine setup failed: " << setup_error.code
                  << ": " << setup_error.message << '\n';
        return 1;
    }

    RealtimeBusEngineHostAudioAdapter adapter;
    std::string adapter_error;
    if (!adapter.configure(engine_host, 0, graph.buses().size(), max_frames, adapter_error)) {
        std::cerr << "Audio adapter setup failed: " << adapter_error << '\n';
        return 1;
    }

    AlsaAudioBackend backend;
    AlsaAudioBackend::Config config;
    config.sample_rate = 48000;
    config.period_frames = 128;
    std::string backend_error;
    if (!backend.start(config, &RealtimeBusAudioAdapter::alsa_callback,
                      &adapter, backend_error)) {
        std::cerr << "ALSA startup failed: " << backend_error << '\n';
        return 1;
    }

    std::cout << "HUSH ALSA realtime host running at "
              << backend.actual_sample_rate() << " Hz, "
              << backend.actual_period_frames() << " frames/period.\n"
              << "Capture is routed to the Master bus. Press Ctrl-C to stop.\n";

    while (keep_running != 0 && backend.running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    backend.stop();
    std::cout << "Stopped. Callbacks: " << backend.callback_count()
              << ", observed recoveries/short transfers: "
              << backend.xrun_count() << '\n';
    return 0;
}
