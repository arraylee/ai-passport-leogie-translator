#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>

namespace leogie {
// Serialize capture admission with cancellation, never the blocking mic read or
// Opus encoding. Callbacks may take the audio queue lock; never reverse that order.
class CaptureGeneration {
public:
    uint32_t snapshot() const { return epoch_.load(); }

    template <class Flush>
    void Change(bool enable, Flush flush) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto next = (epoch_.load() + 2u) & ~1u;
        epoch_.store(next | (enable ? 1u : 0u));
        flush();
    }

    template <class Action>
    bool Admit(uint32_t epoch, Action action) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!(epoch & 1u) || epoch != epoch_.load())
            return false;
        action();
        return true;
    }

private:
    std::atomic<uint32_t> epoch_{0};
    std::mutex mutex_;
};
}  // namespace leogie
