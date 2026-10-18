#include <cassert>
#include "application.h"
#include "board.h"
void message(std::shared_ptr<Protocol> protocol,const char* value){auto root=cJSON_Parse(value);assert(root);protocol->json(root);cJSON_Delete(root);}
void packet(std::shared_ptr<Protocol> protocol,uint8_t value){protocol->audio(std::make_unique<AudioStreamPacket>(AudioStreamPacket{{value}}));}
int main(){
 for(auto transport:{leogie::CloudTransport::Mqtt,leogie::CloudTransport::Websocket}){
  Application app;app.translator_transport_=transport;
  app.ToggleTranslator();app.Drain();assert(app.state==kDeviceStateListening);
  auto first=app.protocol_;
  // Real cloud packets can arrive before the main task has processed tts/start.
  message(first,R"({"type":"tts","state":"start"})");
  packet(first,42);
  app.Drain();
  assert(app.state==kDeviceStateSpeaking);
  assert(app.audio_service_.received==std::vector<uint8_t>{42});
  assert(app.audio_service_.resets==1);
  // Standard STT has no custom source_language field.
  message(first,R"({"type":"stt","text":"火车站在哪里？"})");app.Drain();
  assert(Board::GetInstance().display.direction=="zh");
  message(first,R"({"type":"tts","state":"stop"})");app.Drain();assert(app.state==kDeviceStateListening);
  // Already queued and late callbacks from the old connection cannot resume it.
  message(first,R"({"type":"tts","state":"start"})");packet(first,43);
  app.ToggleTranslator();assert(!app.translator_session_.active());
  app.ToggleTranslator();app.Drain();assert(app.state==kDeviceStateListening);
  packet(first,44);message(first,R"({"type":"tts","state":"start"})");app.Drain();
  assert(app.state==kDeviceStateListening && app.audio_service_.received.empty());
  message(app.protocol_,R"({"type":"tts","state":"start"})");packet(app.protocol_,45);app.Drain();
  assert(app.audio_service_.received==std::vector<uint8_t>{45});
 }
 Application unbound;unbound.translator_transport_=leogie::CloudTransport::None;
 unbound.ToggleTranslator();assert(!unbound.translator_session_.active());assert(!unbound.protocol_);
}
