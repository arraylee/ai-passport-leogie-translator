#pragma once
#include <array>
#include <string>
#include <vector>
#include "display/lcd_display.h"
#include "leogie_countdown.h"

class LeogieDisplay : public SpiLcdDisplay {
public:
    using SpiLcdDisplay::SpiLcdDisplay;
    void SetupUI() override;
    void SetStatus(const char* status) override;
    void SetEmotion(const char* emotion) override;
    void SetChatMessage(const char* role, const char* text) override;
    void ClearChatMessages() override;
    void UpdateStatusBar(bool all = false) override;
    void SetTheme(Theme* theme) override {};
    void SetPowerSaveMode(bool on) override;
    void ShowCountdown(leogie::Countdown::Phase phase, int minutes);

private:
    leogie::Countdown::Phase timer_phase_ = leogie::Countdown::Phase::Closed;
    int timer_minutes_ = -1;
    lv_obj_t* timer_digits_ = nullptr;
    lv_obj_t* timer_caption_ = nullptr;
    lv_obj_t* timer_unit_ = nullptr;
    std::array<lv_obj_t*, 3> timer_keys_{};
    void RenderCountdown();
    enum class Mode { Idle, Listening, Thinking, Speaking, Info, Error };
    Mode mode_ = Mode::Idle;
    lv_obj_t* portrait_ = nullptr;
    lv_obj_t* bubble_ = nullptr;
    lv_obj_t* original_ = nullptr;
    lv_obj_t* translated_ = nullptr;
    lv_obj_t* hint_ = nullptr;
    lv_obj_t* network_ = nullptr;
    lv_obj_t* direction_ = nullptr;
    std::array<lv_obj_t*, 7> bars_{};
    std::array<lv_obj_t*, 3> dots_{};
    bool compact_ = false;
    lv_timer_t* animation_ = nullptr;
    std::string direction_text_ = "中英互译";
    std::vector<std::string> original_pages_, translated_pages_;
    unsigned ticks_ = 0, page_ticks_ = 0, page_ = 0;
    lv_obj_t* Label(int x, int y, int w, int h, uint32_t color);
    lv_obj_t* Shape(int x, int y, int w, int h, uint32_t color, int radius);
    std::vector<std::string> Pages(const std::string& text, unsigned lines);
    void RenderMode();  // Caller owns the LVGL lock.
    void Tick();        // Called by LVGL while its lock is already held.
    void ShowPage();
};
