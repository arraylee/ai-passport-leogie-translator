#include <cassert>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include "translator/capture_generation.h"

int main() {
    leogie::CaptureGeneration capture;
    std::vector<int> pending_pcm, sent;
    auto reset = [&] {
        pending_pcm.clear();
        sent.clear();
    };
    assert(!capture.Admit(capture.snapshot(), [] {}));
    capture.Change(true, reset);
    const auto cancelled = capture.snapshot();
    capture.Admit(cancelled, [&] { pending_pcm.push_back(1); });

    std::mutex barrier;
    std::condition_variable cv;
    bool encoding = false, resume = false;
    std::thread encoder([&] {
        capture.Admit(cancelled, [&] { pending_pcm.clear(); });
        {
            std::unique_lock<std::mutex> lock(barrier);
            encoding = true;
            cv.notify_all();
            cv.wait(lock, [&] { return resume; });
        }
        // The production encoder also runs unlocked and admits its result later.
        assert(!capture.Admit(cancelled, [&] { sent.push_back(1); }));
    });
    {
        std::unique_lock<std::mutex> lock(barrier);
        cv.wait(lock, [&] { return encoding; });
    }
    capture.Admit(cancelled, [&] { pending_pcm.push_back(2); });
    capture.Change(false, reset);
    assert(pending_pcm.empty() && sent.empty());
    capture.Change(true, reset);
    const auto restarted = capture.snapshot();
    assert(restarted != cancelled);
    // A microphone read begun before stop cannot feed the new engine buffer.
    assert(!capture.Admit(cancelled, [&] { pending_pcm.push_back(3); }));
    {
        std::lock_guard<std::mutex> lock(barrier);
        resume = true;
    }
    cv.notify_all();
    encoder.join();
    assert(sent.empty());
    assert(capture.Admit(restarted, [&] { sent.push_back(4); }));
    assert(sent == std::vector<int>{4});
    capture.Change(false, reset);
    assert(!capture.Admit(restarted, [&] { sent.push_back(5); }));
    assert(sent.empty());
}
