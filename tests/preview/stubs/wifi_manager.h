#pragma once
class WifiManager {
public:
 static WifiManager& GetInstance() {static WifiManager wifi; return wifi;}
 bool IsConnected() const {return true;}
};
