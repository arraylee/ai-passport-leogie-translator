#pragma once
#include <atomic>
#include <cstdint>
#include <string>

namespace leogie {
// Mutations belong to the main task. Receive tasks may read this single atomic.
class Session {
public:
    uint32_t Toggle() { return generation_.fetch_add(1) + 1; }
    void Stop() {
        if (active())
            generation_.fetch_add(1);
    }
    bool active() const { return (generation_.load() & 1u) != 0; }
    uint32_t epoch() const { return generation_.load(); }
    bool Accepts(uint32_t epoch) const { return (epoch & 1u) && generation_.load() == epoch; }

private:
    std::atomic<uint32_t> generation_{0};
};

enum class CloudTransport { None, Mqtt, Websocket };

inline CloudTransport SelectCloudTransport(bool mqtt, bool websocket) {
    return mqtt        ? CloudTransport::Mqtt
           : websocket ? CloudTransport::Websocket
                       : CloudTransport::None;
}

// Direction is a display hint; the cloud role performs translation itself.
inline std::string SourceDirection(const std::string& text) {
    unsigned han = 0, words = 0;
    bool in_word = false;
    for (size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        const bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        if (letter && !in_word)
            ++words;
        in_word = letter;
        if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) {
            const auto b = static_cast<unsigned char>(text[i + 1]);
            const auto d = static_cast<unsigned char>(text[i + 2]);
            if ((b & 0xC0) == 0x80 && (d & 0xC0) == 0x80) {
                const unsigned point = ((c & 15) << 12) | ((b & 63) << 6) | (d & 63);
                if (point >= 0x3400 && point <= 0x9FFF)
                    ++han;
                i += 2;
            }
        }
    }
    if (!han && !words)
        return {};
    return !han || (words >= 3 && words > han) ? "en" : "zh";
}
}  // namespace leogie
