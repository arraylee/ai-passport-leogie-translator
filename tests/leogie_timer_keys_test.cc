#include <cassert>
#include "boards/ai-passport-leogie/leogie_release_key.h"
int main() {
    leogie::ReleaseKey key;
    // Each debounced release is one action, even in the multi-click interval.
    assert(key.Release());
    assert(key.Release());
    assert(key.Release());
    key.LongPress();
    assert(!key.Release());  // long-confirm cancel cannot also start translation
    assert(key.Release());
    key.LongPress();
    key.LongPress();
    assert(!key.Release());  // repeated long notification still consumes one release
    assert(key.Release());
}
