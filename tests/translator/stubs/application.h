#pragma once
#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "cJSON.h"
#include "translator/session.h"
#define MAX_DECODE_PACKETS_IN_QUEUE 20
#define pdPASS 1
inline int xTaskCreate(void (*fn)(void*), const char*, int, void* data, int, void*) {fn(data); return pdPASS;}
inline void vTaskDelete(void*) {}
enum DeviceState { kDeviceStateIdle, kDeviceStateListening, kDeviceStateConnecting, kDeviceStateSpeaking, kDeviceStateWifiConfiguring, kDeviceStateStarting, kDeviceStateActivating };
enum ListeningMode { kListeningModeAutoStop };
struct AudioStreamPacket {std::vector<uint8_t> payload;};
class Protocol {
public:
 std::function<void(std::unique_ptr<AudioStreamPacket>)> audio;
 std::function<void(const cJSON*)> json;
 std::function<void()> closed;
 std::function<void(const std::string&)> error;
 virtual ~Protocol()=default;
 void OnIncomingAudio(decltype(audio) cb){audio=cb;}
 void OnIncomingJson(decltype(json) cb){json=cb;}
 void OnAudioChannelClosed(decltype(closed) cb){closed=cb;}
 void OnNetworkError(decltype(error) cb){error=cb;}
 bool OpenAudioChannel(){return true;}
 void CloseAudioChannel(){if(closed)closed();}
};
class AudioService {
public:
 bool enabled=false;
 unsigned resets=0;
 std::vector<uint8_t> received;
 void EnableVoiceProcessing(bool value){enabled=value;}
 void ResetDecoder(){++resets; received.clear();}
 bool PushPacketToDecodeQueue(std::unique_ptr<AudioStreamPacket> packet){received.push_back(packet->payload[0]);return true;}
};
class Application {
public:
 DeviceState state=kDeviceStateIdle;
 leogie::Session translator_session_;
 leogie::CloudTransport translator_transport_=leogie::CloudTransport::Websocket;
 bool translator_connecting_=false,pending_listening_start_=false;
 std::atomic<unsigned> translator_pending_audio_{0};
 std::atomic<bool> translator_audio_overflow_{false};
 uint32_t translator_sent_packets_=0,translator_send_failures_=0;
 std::atomic<unsigned> translator_received_packets_{0};
 std::string last_error_message_;
 std::shared_ptr<Protocol> protocol_;
 AudioService audio_service_;
 std::deque<std::function<void()>> tasks;
 DeviceState GetDeviceState()const{return state;}
 void SetDeviceState(DeviceState value){state=value;}
 void SetListeningMode(ListeningMode){state=kDeviceStateListening;}
 void Schedule(std::function<void()> fn){tasks.push_back(fn);}
 void Drain(){while(!tasks.empty()){auto fn=tasks.front();tasks.pop_front();fn();}}
 void ToggleTranslator(); void StopTranslator(const std::string& message="");
 void ConnectTranslator(); void TranslatorJson(const cJSON* root,uint32_t epoch);
};
