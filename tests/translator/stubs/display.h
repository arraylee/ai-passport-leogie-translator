#pragma once
#include <string>
class Display {public:
 std::string direction;
 void SetStatus(const char*){} void SetEmotion(const char*){}
 void SetChatMessage(const char* role,const char* value){if(std::string(role)=="direction")direction=value;}
};
