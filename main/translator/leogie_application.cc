#include <esp_log.h>
#include <cstring>
#include <new>
#include "application.h"
#include "board.h"
#include "display.h"
#include "mqtt_protocol.h"
#include "websocket_protocol.h"

void Application::ToggleTranslator() {
    const auto state = GetDeviceState();
    if (state == kDeviceStateWifiConfiguring || state == kDeviceStateStarting ||
        state == kDeviceStateActivating)
        return;
    if (translator_session_.active()) {
        StopTranslator();
        return;
    }
    last_error_message_.clear();
    translator_audio_overflow_ = false;
    translator_sent_packets_ = 0;
    translator_send_failures_ = 0;
    translator_received_packets_ = 0;
    translator_session_.Toggle();
    SetDeviceState(kDeviceStateConnecting);
    if (!translator_connecting_)
        ConnectTranslator();
}

void Application::StopTranslator(const std::string& message) {
    ESP_LOGI("LeogieDiag", "stop reason=%s sent=%lu received=%u send_failures=%lu",
             message.empty() ? "user" : "error", (unsigned long)translator_sent_packets_,
             translator_received_packets_.load(), (unsigned long)translator_send_failures_);
    // Invalidate receive callbacks before touching queues or transport ownership.
    translator_session_.Stop();
    pending_listening_start_ = false;
    audio_service_.EnableVoiceProcessing(false);
    audio_service_.ResetDecoder();
    if (protocol_ && !translator_connecting_)
        protocol_->CloseAudioChannel();
    protocol_.reset();
    SetDeviceState(kDeviceStateIdle);
    if (!message.empty()) {
        last_error_message_ = message;
        Board::GetInstance().GetDisplay()->SetStatus("请重试");
        Board::GetInstance().GetDisplay()->SetChatMessage("system", message.c_str());
    }
}

void Application::ConnectTranslator() {
    if (!translator_session_.active() || translator_connecting_)
        return;
    const auto epoch = translator_session_.epoch();
    ESP_LOGI("LeogieDiag", "connect epoch=%lu transport=%d", (unsigned long)epoch,
             (int)translator_transport_);
    std::shared_ptr<Protocol> protocol;
    if (translator_transport_ == leogie::CloudTransport::Mqtt)
        protocol = std::make_shared<MqttProtocol>();
    else if (translator_transport_ == leogie::CloudTransport::Websocket)
        protocol = std::make_shared<WebsocketProtocol>();
    else {
        StopTranslator("请先在小智平台激活设备，再重启");
        return;
    }
    protocol_ = protocol;
    protocol->OnIncomingAudio([this, epoch](std::unique_ptr<AudioStreamPacket> packet) {
        if (!translator_session_.Accepts(epoch) || packet->payload.empty() ||
            packet->payload.size() > 1500)
            return;
        if (translator_received_packets_.fetch_add(1) == 0)
            ESP_LOGI("LeogieDiag", "first_audio epoch=%lu", (unsigned long)epoch);
        if (translator_pending_audio_.fetch_add(1) >= MAX_DECODE_PACKETS_IN_QUEUE) {
            translator_pending_audio_.fetch_sub(1);
            if (!translator_audio_overflow_.exchange(true)) {
                Schedule([this, epoch]() {
                    if (translator_session_.Accepts(epoch))
                        StopTranslator("网络音频拥堵，请重试");
                });
            }
            return;
        }
        auto pending = std::make_shared<std::unique_ptr<AudioStreamPacket>>(std::move(packet));
        Schedule([this, epoch, pending]() {
            translator_pending_audio_.fetch_sub(1);
            // Queue admission and StopTranslator run on the same main task.
            // No old packet can race past the stop/reset into playback.
            if (!translator_session_.Accepts(epoch) || GetDeviceState() != kDeviceStateSpeaking)
                return;
            if (!audio_service_.PushPacketToDecodeQueue(std::move(*pending)))
                StopTranslator("网络音频拥堵，请重试");
        });
    });
    protocol->OnIncomingJson([this, epoch](const cJSON* root) { TranslatorJson(root, epoch); });
    protocol->OnAudioChannelClosed([this, epoch]() {
        Schedule([this, epoch]() {
            if (translator_session_.Accepts(epoch))
                StopTranslator("连接已断开，按键重试");
        });
    });
    protocol->OnNetworkError([this, epoch](const std::string&) {
        Schedule([this, epoch]() {
            if (translator_session_.Accepts(epoch))
                StopTranslator("无法连接小智服务，请检查网络");
        });
    });
    Board::GetInstance().SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);
    struct Job {
        Application* app;
        std::shared_ptr<Protocol> protocol;
        uint32_t epoch;
    };
    auto* job = new (std::nothrow) Job{this, protocol, epoch};
    if (!job) {
        StopTranslator("内存不足，请重启");
        return;
    }
    translator_connecting_ = true;
    auto result = xTaskCreate(
        [](void* data) {
            auto* job = static_cast<Job*>(data);
            auto* app = job->app;
            auto protocol = job->protocol;
            const auto epoch = job->epoch;
            delete job;
            const bool connected = protocol->OpenAudioChannel();
            app->Schedule([app, protocol, epoch, connected]() {
                app->translator_connecting_ = false;
                if (!app->translator_session_.Accepts(epoch)) {
                    protocol->CloseAudioChannel();
                    if (app->translator_session_.active())
                        app->ConnectTranslator();
                    return;
                }
                if (!connected) {
                    app->StopTranslator("无法连接小智服务，请检查网络");
                    return;
                }
                app->SetListeningMode(kListeningModeAutoStop);
            });
            vTaskDelete(nullptr);
        },
        "leogie_connect", 6144, job, 3, nullptr);
    if (result != pdPASS) {
        delete job;
        translator_connecting_ = false;
        StopTranslator("内存不足，请重启");
    }
}

void Application::TranslatorJson(const cJSON* root, uint32_t epoch) {
    if (!translator_session_.Accepts(epoch))
        return;
    const auto* kind = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (!cJSON_IsString(kind))
        return;
    const std::string type(kind->valuestring);
    const auto* state = cJSON_GetObjectItemCaseSensitive(root, "state");
    // Metadata only: never print transcripts, model output, credentials or IDs.
    const auto* text_info = cJSON_GetObjectItemCaseSensitive(root, "text");
    const char* phase = cJSON_IsString(state) && (!strcmp(state->valuestring, "start") ||
                                                  !strcmp(state->valuestring, "stop") ||
                                                  !strcmp(state->valuestring, "sentence_start"))
                            ? state->valuestring
                            : "-";
    const char* event = (type == "stt" || type == "tts" || type == "error" || type == "alert")
                            ? type.c_str()
                            : "other";
    ESP_LOGI("LeogieDiag", "cloud_event=%s phase=%s text_bytes=%u", event, phase,
             cJSON_IsString(text_info) ? (unsigned)strlen(text_info->valuestring) : 0u);
    if (type == "tts" && cJSON_IsString(state)) {
        const std::string action(state->valuestring);
        if (action == "start") {
            Schedule([this, epoch]() {
                if (!translator_session_.Accepts(epoch) ||
                    GetDeviceState() != kDeviceStateListening)
                    return;
                // Cloud audio may follow tts/start immediately, without an ACK.
                // Reset here, in main-task order, before admitting queued audio.
                audio_service_.EnableVoiceProcessing(false);
                audio_service_.ResetDecoder();
                SetDeviceState(kDeviceStateSpeaking);
                Board::GetInstance().GetDisplay()->SetEmotion("thinking");
            });
        } else if (action == "stop") {
            Schedule([this, epoch]() {
                if (translator_session_.Accepts(epoch) && GetDeviceState() == kDeviceStateSpeaking)
                    SetDeviceState(kDeviceStateListening);
            });
        }
    }
    if (type == "stt" || type == "tts" || type == "error") {
        const auto* text = cJSON_GetObjectItemCaseSensitive(root, "text");
        if (!cJSON_IsString(text) || strlen(text->valuestring) > 4096)
            return;
        if (type == "tts" &&
            (!cJSON_IsString(state) || strcmp(state->valuestring, "sentence_start")))
            return;
        const auto source = leogie::SourceDirection(text->valuestring);
        Schedule([this, epoch, type, source, message = std::string(text->valuestring)]() {
            if (!translator_session_.Accepts(epoch))
                return;
            auto display = Board::GetInstance().GetDisplay();
            if (type == "stt")
                display->SetChatMessage("direction", source.c_str());
            display->SetChatMessage(type == "stt"   ? "user"
                                    : type == "tts" ? "assistant"
                                                    : "system",
                                    message.c_str());
            if (type == "stt") {
                display->SetStatus("翻译中");
                display->SetEmotion("thinking");
            }
            if (type == "tts") {
                display->SetStatus("正在播报");
                display->SetEmotion("speaking");
            }
        });
    }
}
