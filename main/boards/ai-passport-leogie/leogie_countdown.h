#pragma once
#include <algorithm>
#include <cstdint>

namespace leogie {
// Owned by the application task. Clock is monotonic microseconds since boot.
class Countdown {
public:
    enum class Phase { Closed, Setting, Running, Ringing };
    Phase phase() const { return phase_; }
    bool active() const { return phase_ != Phase::Closed; }
    int minutes() const { return minutes_; }
    void Open() {
        if (!active())
            phase_ = Phase::Setting;
    }
    void Adjust(int delta) {
        if (phase_ == Phase::Setting)
            minutes_ = std::clamp(minutes_ + delta, 1, 120);
    }
    void Start(int64_t now) {
        if (phase_ != Phase::Setting)
            return;
        deadline_ = now + static_cast<int64_t>(minutes_) * 60000000;
        phase_ = Phase::Running;
    }
    bool Tick(int64_t now) {
        if (phase_ != Phase::Running || now < deadline_)
            return false;
        phase_ = Phase::Ringing;
        return true;
    }
    int RemainingSeconds(int64_t now) const {
        if (phase_ == Phase::Setting)
            return minutes_ * 60;
        if (phase_ != Phase::Running)
            return 0;
        return static_cast<int>(std::max<int64_t>(0, deadline_ - now + 999999) / 1000000);
    }
    int RemainingMinutes(int64_t now) const { return (RemainingSeconds(now) + 59) / 60; }
    void Cancel() {
        phase_ = Phase::Closed;
        deadline_ = 0;
    }

private:
    Phase phase_ = Phase::Closed;
    int minutes_ = 10;
    int64_t deadline_ = 0;
};
}  // namespace leogie
