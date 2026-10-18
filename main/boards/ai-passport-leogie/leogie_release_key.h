#pragma once
namespace leogie {
// One instance per key, used exclusively by the button task. Actions happen on
// debounced release, so rapid taps do not wait for a multi-click classifier.
class ReleaseKey {
public:
    void LongPress() { long_handled_ = true; }
    bool Release() {
        const bool short_press = !long_handled_;
        long_handled_ = false;
        return short_press;
    }

private:
    bool long_handled_ = false;
};
}  // namespace leogie
