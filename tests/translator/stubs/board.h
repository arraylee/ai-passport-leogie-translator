#pragma once
#include "display.h"
enum class PowerSaveLevel{PERFORMANCE};
class Board {public:
 Display display;
 static Board& GetInstance(){static Board board;return board;}
 Display* GetDisplay(){return &display;}
 void SetPowerSaveLevel(PowerSaveLevel){}
};
