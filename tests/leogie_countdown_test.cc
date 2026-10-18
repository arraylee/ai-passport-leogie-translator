#include "boards/ai-passport-leogie/leogie_countdown.h"
#include <cassert>
using leogie::Countdown;
int main() {
    Countdown rounding;
    rounding.Open();
    rounding.Start(0);
    assert(rounding.RemainingMinutes(0) == 10);
    assert(rounding.RemainingMinutes(539999999) == 2);
    assert(rounding.RemainingMinutes(540000000) == 1);
    assert(rounding.RemainingMinutes(599999999) == 1);
    assert(rounding.RemainingMinutes(600000000) == 0);
    Countdown t;
    assert(!t.active());
    t.Open();
    assert(t.phase() == Countdown::Phase::Setting && t.minutes() == 10);
    for (int i = 0; i < 200; ++i)
        t.Adjust(-1);
    assert(t.minutes() == 1);
    for (int i = 0; i < 200; ++i)
        t.Adjust(1);
    assert(t.minutes() == 120);
    t.Cancel();
    t.Open();
    assert(t.minutes() == 120);  // remember selection in RAM only
    t.Adjust(-119);
    constexpr int64_t now = 987654321000LL;
    t.Start(now);
    assert(t.phase() == Countdown::Phase::Running);
    assert(t.RemainingSeconds(now) == 60);
    assert(t.RemainingMinutes(now) == 1);
    assert(t.RemainingSeconds(now + 1) == 60);
    assert(t.RemainingSeconds(now + 59000000) == 1);
    assert(t.RemainingMinutes(now + 59000000) == 1);
    t.Adjust(5);
    t.Start(now + 30000000);  // repeated confirm cannot restart
    assert(t.minutes() == 1);
    assert(!t.Tick(now + 59999999));
    assert(t.Tick(now + 60000000));
    assert(t.phase() == Countdown::Phase::Ringing);
    assert(t.RemainingSeconds(now + 60000000) == 0);
    assert(t.RemainingMinutes(now + 60000000) == 0);
    assert(!t.Tick(now + 61000000));  // only one expiry transition
    t.Cancel();
    assert(!t.active());
    assert(!t.Tick(now + 9000000000LL));
    t.Open();
    t.Start(now);
    t.Cancel();  // long-confirm running cancels, never rings in background
    assert(!t.Tick(now + 60000000));
    t.Open();
    t.Start(now + 9000000000LL);
    assert(t.RemainingSeconds(now + 9000000000LL) == 60);
    assert(!t.Tick(now + 9000000000LL));
    assert(t.Tick(now + 9999999999LL));  // delayed main-loop tick catches expiry
}
