#pragma once
enum DeviceState {kDeviceStateIdle,kDeviceStateListening,kDeviceStateConnecting,kDeviceStateSpeaking,kDeviceStateActivating,kDeviceStateWifiConfiguring};
class Application {
public:
 DeviceState state=kDeviceStateIdle;
 static Application& GetInstance() {static Application app; return app;}
 DeviceState GetDeviceState() const {return state;}
};
