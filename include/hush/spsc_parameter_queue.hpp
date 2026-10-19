#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace hush {

// Fixed-capacity single-producer/single-consumer queue.
// Producer and consumer roles must never be shared by multiple threads.
// No allocation or blocking occurs in push/pop after construction.
template <typename T, std::size_t Capacity>
class SpscParameterQueue {
    static_assert(Capacity > 0, "Queue capacity must be positive");
    static_assert(std::is_trivially_copyable_v<T>,
                  "Queue payload must be trivially copyable");

public:
    SpscParameterQueue() = default;
    SpscParameterQueue(const SpscParameterQueue&) = delete;
    SpscParameterQueue& operator=(const SpscParameterQueue&) = delete;

    // Called only by the producer thread. Returns false when the queue is full.
    [[nodiscard]] bool try_push(const T& value) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);
        if (head - tail >= Capacity) {
            return false;
        }
        slots_[head % Capacity] = value;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    // Called only by the consumer thread (typically the audio thread).
    // Returns false when the queue is empty.
    [[nodiscard]] bool try_pop(T& value) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire);
        if (tail == head) {
            return false;
        }
        value = slots_[tail % Capacity];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    // Approximate snapshot for diagnostics only; not a synchronization primitive.
    [[nodiscard]] std::size_t size_approx() const noexcept {
        const auto head = head_.load(std::memory_order_acquire);
        const auto tail = tail_.load(std::memory_order_acquire);
        return head - tail;
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

    // Check on the target platform before relying on strict lock-free behavior.
    [[nodiscard]] bool atomics_are_lock_free() const noexcept {
        return head_.is_lock_free() && tail_.is_lock_free();
    }

private:
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
    std::array<T, Capacity> slots_{};
};

} // namespace hush
