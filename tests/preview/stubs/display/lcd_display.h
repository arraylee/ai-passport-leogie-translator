#pragma once
#include <lvgl.h>
struct Theme {};
class Display {
public:
 virtual ~Display() = default;
 virtual void SetupUI() {setup_ui_called_=true;}
 virtual void SetStatus(const char*) {}
 virtual void SetEmotion(const char*) {}
 virtual void SetChatMessage(const char*,const char*) {}
 virtual void ClearChatMessages() {}
 virtual void UpdateStatusBar(bool=false) {}
 virtual void SetTheme(Theme*) {}
 virtual void SetPowerSaveMode(bool) {}
protected:
 bool setup_ui_called_=false;
 lv_obj_t* status_label_=nullptr;
 lv_obj_t* notification_label_=nullptr;
};
class SpiLcdDisplay : public Display {};
class DisplayLockGuard {
public:
 explicit DisplayLockGuard(Display*) {}
 explicit operator bool() const {return true;}
};
