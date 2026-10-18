#include "boards/ai-passport-leogie/leogie_alarm.h"
#include <cassert>
#include "audio/demuxer/ogg_demuxer.h"
int main() {
    OggDemuxer demuxer;
    int packets = 0, milliseconds = 0;
    demuxer.OnPacket([&](const uint8_t*, int sample_rate, int frame_ms, size_t size) {
        assert(sample_rate == 24000);
        assert(frame_ms == 60);
        assert(size > 0 && size <= 250);
        ++packets;
        milliseconds += frame_ms;
    });
    demuxer.Process(leogie_alarm, leogie_alarm_size);
    assert(!demuxer.HasError());
    assert(demuxer.Finish());
    assert(packets > 0 && packets < 20);  // never fills decode queue on main task
    assert(milliseconds >= 600 && milliseconds <= 720);
}
