#include <cassert>
#include <string>
#include "translator/session.h"

int main() {
    leogie::Session session;
    assert(!session.active());
    auto first = session.Toggle();
    assert(session.active() && session.Accepts(first));
    session.Toggle();
    assert(!session.active() && !session.Accepts(first));
    auto second = session.Toggle();
    assert(session.Accepts(second) && !session.Accepts(first));
    session.Stop();
    session.Stop();
    assert(!session.active());
    // Each real tap is retained, rather than coalesced into an event bit.
    for (int i = 0; i < 100; ++i) {
        session.Toggle();
        assert(session.active() == (i % 2 == 0));
    }
    assert(leogie::SelectCloudTransport(true, true) == leogie::CloudTransport::Mqtt);
    assert(leogie::SelectCloudTransport(false, true) == leogie::CloudTransport::Websocket);
    assert(leogie::SelectCloudTransport(false, false) == leogie::CloudTransport::None);
    assert(leogie::SourceDirection("火车站在哪里？") == "zh");
    assert(leogie::SourceDirection("Can I pay by card?") == "en");
    assert(leogie::SourceDirection("我要 two 张票") == "zh");
    assert(leogie::SourceDirection("Please go to 上海") == "en");
    assert(leogie::SourceDirection("123 ...").empty());
}
