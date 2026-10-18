#pragma once
class Board {
public:
 static Board& GetInstance() {static Board board; return board;}
 bool GetBatteryLevel(int& battery,bool& charging,bool& discharging) {battery=100;charging=false;discharging=true;return true;}
};
