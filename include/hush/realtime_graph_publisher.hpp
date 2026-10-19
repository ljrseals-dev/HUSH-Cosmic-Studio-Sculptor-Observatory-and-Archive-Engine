#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace hush {

// Single-control-thread / single-audio-reader publication primitive for immutable
// graph snapshots. Build a replacement off-thread, publish it, then reclaim retired
// snapshots on the control thread. Never call reclaim_retired() from the audio thread.
template <class Graph>
class RealtimeGraphPublisher {
public:
    class AudioReadGuard {
    public:
        AudioReadGuard() = default;
        explicit AudioReadGuard(RealtimeGraphPublisher* owner) noexcept : owner_(owner) {
            owner_->reader_active_.store(true, std::memory_order_seq_cst);
            graph_ = owner_->active_.load(std::memory_order_seq_cst);
        }
        AudioReadGuard(const AudioReadGuard&) = delete;
        AudioReadGuard& operator=(const AudioReadGuard&) = delete;
        AudioReadGuard(AudioReadGuard&& other) noexcept
            : owner_(std::exchange(other.owner_, nullptr)), graph_(std::exchange(other.graph_, nullptr)) {}
        AudioReadGuard& operator=(AudioReadGuard&&) = delete;
        ~AudioReadGuard() {
            if (owner_) owner_->reader_active_.store(false, std::memory_order_seq_cst);
        }
        [[nodiscard]] const Graph* get() const noexcept { return graph_; }
        [[nodiscard]] const Graph& operator*() const noexcept { return *graph_; }
        [[nodiscard]] const Graph* operator->() const noexcept { return graph_; }
        [[nodiscard]] explicit operator bool() const noexcept { return graph_ != nullptr; }
    private:
        RealtimeGraphPublisher* owner_{nullptr};
        const Graph* graph_{nullptr};
    };

    RealtimeGraphPublisher() = default;
    RealtimeGraphPublisher(const RealtimeGraphPublisher&) = delete;
    RealtimeGraphPublisher& operator=(const RealtimeGraphPublisher&) = delete;

    // Control thread only. The graph must be fully constructed and immutable after publish.
    void publish(std::unique_ptr<Graph> replacement) {
        if (!replacement) return;
        if (active_owner_) retired_.reserve(retired_.size() + 1U);
        Graph* next = replacement.get();
        (void)active_.exchange(next, std::memory_order_seq_cst);
        if (active_owner_) retired_.push_back(std::move(active_owner_));
        active_owner_ = std::move(replacement);
        reclaim_retired();
    }

    // Audio thread only. A single callback reader is supported; nested/concurrent guards
    // are invalid. Guard lifetime must not extend beyond the callback.
    [[nodiscard]] AudioReadGuard acquire_for_audio() noexcept {
        return AudioReadGuard(this);
    }

    // Control thread only. Safe to call periodically after publication.
    void reclaim_retired() noexcept {
        if (!reader_active_.load(std::memory_order_seq_cst)) retired_.clear();
    }

    [[nodiscard]] std::size_t retired_count_for_control() const noexcept {
        return retired_.size();
    }

private:
    std::atomic<Graph*> active_{nullptr};
    std::atomic<bool> reader_active_{false};
    std::unique_ptr<Graph> active_owner_;
    std::vector<std::unique_ptr<Graph>> retired_;
};

} // namespace hush
