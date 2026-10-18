#include "leogie_display.h"
#include <src/misc/lv_text_private.h>
#include <algorithm>
#include <cstring>
#include "application.h"
#include "audio_codec.h"
#include "board.h"
#include "leogie_art.h"
#include "wifi_manager.h"

LV_FONT_DECLARE(leogie_font20);
LV_FONT_DECLARE(leogie_digits48);

namespace {
constexpr uint32_t kInk = 0x183551;
constexpr uint32_t kCream = 0xfffbee;
constexpr int kTextWidth = 200;
void Visible(lv_obj_t* object, bool visible) {
    if (visible)
        lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
}
}  // namespace

lv_obj_t* LeogieDisplay::Shape(int x, int y, int w, int h, uint32_t color, int radius) {
    auto* object = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(object);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, w, h);
    lv_obj_set_style_bg_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(object, radius, 0);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    return object;
}

lv_obj_t* LeogieDisplay::Label(int x, int y, int w, int h, uint32_t color) {
    auto* label = lv_label_create(lv_screen_active());
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, w, h);
    lv_obj_set_style_text_font(label, &leogie_font20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_line_space(label, 2, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "");
    return label;
}

void LeogieDisplay::SetupUI() {
    if (setup_ui_called_)
        return;
    DisplayLockGuard lock(this);
    if (!lock)
        return;
    Display::SetupUI();
    auto* screen = lv_screen_active();
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xffe27a), 0);
    // Native shapes keep the illustrated world inexpensive on a no-PSRAM board.
    Shape(192, 48, 30, 30, 0xffcf48, LV_RADIUS_CIRCLE);
    Shape(-12, 83, 56, 21, kCream, 12);
    Shape(1, 72, 25, 27, kCream, LV_RADIUS_CIRCLE);
    Shape(210, 111, 47, 19, kCream, 12);
    Shape(219, 101, 25, 27, kCream, LV_RADIUS_CIRCLE);
    Shape(-25, 204, 290, 160, 0xb4df85, 100);
    Shape(-24, 186, 65, 87, 0x75cba1, 40);
    Shape(208, 197, 61, 91, 0x75cba1, 40);
    Shape(-15, 270, 45, 57, 0x45ae87, 28);
    Shape(214, 273, 47, 55, 0x45ae87, 28);
    for (const auto x : {8, 224}) {
        Shape(x, 273, 8, 8, kCream, LV_RADIUS_CIRCLE);
        Shape(x + 2, 275, 4, 4, 0xffcf48, LV_RADIUS_CIRCLE);
    }
    portrait_ = lv_image_create(screen);
    bubble_ = Shape(10, 207, 220, 78, kCream, 27);
    lv_obj_set_style_border_width(bubble_, 2, 0);
    lv_obj_set_style_border_color(bubble_, lv_color_hex(kInk), 0);
    status_label_ = Label(12, 7, 164, 29, kInk);
    notification_label_ = Label(12, 7, 164, 29, kInk);
    lv_label_set_long_mode(notification_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_bg_color(notification_label_, lv_color_hex(kCream), 0);
    lv_obj_set_style_bg_opa(notification_label_, LV_OPA_COVER, 0);
    Visible(notification_label_, false);
    network_ = Label(178, 7, 52, 29, kInk);
    lv_obj_set_style_text_align(network_, LV_TEXT_ALIGN_RIGHT, 0);
    direction_ = Label(20, 143, kTextWidth, 29, kInk);
    lv_obj_set_style_text_align(direction_, LV_TEXT_ALIGN_CENTER, 0);
    original_ = Label(20, 181, kTextWidth, 29, 0x516b7d);
    translated_ = Label(20, 214, kTextWidth, 60, kInk);
    hint_ = Label(12, 290, 216, 29, kInk);
    lv_obj_set_style_text_align(hint_, LV_TEXT_ALIGN_CENTER, 0);
    for (size_t i = 0; i < bars_.size(); ++i)
        bars_[i] = Shape(77 + i * 13, 216, 7, 20, 0x238bc7, 4);
    for (size_t i = 0; i < dots_.size(); ++i)
        dots_[i] = Shape(180 + i * 11, 122, 6, 6, 0x238bc7, LV_RADIUS_CIRCLE);
    timer_caption_ = Label(22, 159, 65, 29, kInk);
    lv_label_set_text(timer_caption_, "倒计时");
    timer_unit_ = Label(183, 159, 30, 29, kInk);
    lv_label_set_text(timer_unit_, "分");
    timer_digits_ = Label(86, 151, 90, 42, kInk);
    lv_obj_set_style_text_font(timer_digits_, &leogie_digits48, 0);
    lv_obj_set_style_text_align(timer_digits_, LV_TEXT_ALIGN_CENTER, 0);
    for (size_t i = 0; i < timer_keys_.size(); ++i) {
        timer_keys_[i] = Label(20, 203 + i * 28, 200, 29, kInk);
        lv_obj_set_style_text_align(timer_keys_[i], LV_TEXT_ALIGN_CENTER, 0);
    }
    lv_label_set_text(status_label_, "Leogie 翻译器");
    lv_label_set_text(hint_, "确定翻译 上键计时");
    RenderMode();
    animation_ = lv_timer_create(
        [](lv_timer_t* timer) {
            static_cast<LeogieDisplay*>(lv_timer_get_user_data(timer))->Tick();
        },
        250, this);
}

void LeogieDisplay::RenderMode() {
    const bool timer_open = timer_phase_ != leogie::Countdown::Phase::Closed;
    Visible(timer_digits_, timer_open);
    Visible(timer_caption_, timer_open);
    Visible(timer_unit_, timer_open);
    for (auto* label : timer_keys_)
        Visible(label, timer_open);
    if (timer_open) {
        RenderCountdown();
        return;
    }
    Visible(translated_, true);
    compact_ = !original_pages_.empty() || !translated_pages_.empty() || mode_ == Mode::Thinking ||
               mode_ == Mode::Speaking || mode_ == Mode::Info || mode_ == Mode::Error;
    unsigned pose = mode_ == Mode::Listening  ? 1
                    : mode_ == Mode::Speaking ? 3
                    : mode_ == Mode::Idle     ? 0
                                              : 2;
    lv_image_set_src(portrait_, compact_ ? &leogie_small[pose] : &leogie_large[pose]);
    lv_obj_set_pos(portrait_, compact_ ? 60 : 30, compact_ ? 36 : 38);
    const uint32_t sky = mode_ == Mode::Idle       ? 0xffe27a
                         : mode_ == Mode::Thinking ? 0xb8edd9
                         : mode_ == Mode::Error    ? 0xffd7bc
                                                   : 0x9bdcfa;
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(sky), 0);
    lv_obj_set_pos(bubble_, 10, compact_ ? 174 : 207);
    lv_obj_set_size(bubble_, 220, compact_ ? 111 : 78);
    Visible(direction_, compact_);
    Visible(original_, compact_);
    lv_obj_set_pos(translated_, 20, compact_ ? 214 : 218);
    lv_obj_set_style_text_align(translated_, compact_ ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER,
                                0);
    for (auto* bar : bars_)
        Visible(bar, mode_ == Mode::Listening && !compact_);
    for (auto* dot : dots_)
        Visible(dot, mode_ == Mode::Thinking || mode_ == Mode::Speaking);
    ShowPage();
}

void LeogieDisplay::SetStatus(const char* status) {
    if (!setup_ui_called_ || !status)
        return;
    auto state = Application::GetInstance().GetDeviceState();
    DisplayLockGuard lock(this);
    if (!lock)
        return;
    const char* label = status;
    if (state == kDeviceStateIdle) {
        label = "Leogie 翻译器";
        mode_ = Mode::Idle;
    } else if (state == kDeviceStateListening) {
        label = "我在听";
        mode_ = Mode::Listening;
    } else if (state == kDeviceStateConnecting) {
        label = "正在连接";
        mode_ = Mode::Thinking;
    } else if (state == kDeviceStateSpeaking) {
        label = mode_ == Mode::Speaking ? "正在播报" : "翻译中";
        if (mode_ != Mode::Speaking)
            mode_ = Mode::Thinking;
    } else {
        mode_ = Mode::Info;
    }
    // STT can arrive before tts/start changes the device state.
    if (!strcmp(status, "翻译中")) {
        label = status;
        mode_ = Mode::Thinking;
    } else if (!strcmp(status, "正在播报")) {
        label = status;
        mode_ = Mode::Speaking;
    } else if (!strcmp(status, "请重试")) {
        label = status;
        mode_ = Mode::Error;
    }
    lv_label_set_text(status_label_, label);
    Visible(status_label_, true);
    Visible(notification_label_, false);
    lv_label_set_text(hint_, state == kDeviceStateActivating        ? "请在小智控制台绑定"
                             : state == kDeviceStateWifiConfiguring ? "请先连接 Wi-Fi"
                             : state == kDeviceStateIdle            ? "确定翻译 上键计时"
                                                                    : "再按一下结束");
    RenderMode();
}

void LeogieDisplay::SetEmotion(const char* emotion) {
    if (!setup_ui_called_ || !emotion)
        return;
    DisplayLockGuard lock(this);
    if (!lock)
        return;
    if (!strcmp(emotion, "thinking"))
        mode_ = Mode::Thinking;
    else if (!strcmp(emotion, "speaking"))
        mode_ = Mode::Speaking;
    else
        return;
    RenderMode();
}

std::vector<std::string> LeogieDisplay::Pages(const std::string& text, unsigned lines) {
    std::vector<std::string> pages;
    std::string page;
    lv_text_attributes_t attributes;
    lv_text_attributes_init(&attributes);
    attributes.max_width = kTextWidth;
    attributes.line_space = 2;
    unsigned line = 0;
    for (size_t pos = 0; pos < text.size();) {
        const size_t length = lv_text_get_next_line(text.c_str() + pos, text.size() - pos,
                                                    &leogie_font20, nullptr, &attributes);
        if (length == 0 || length > text.size() - pos)
            break;
        std::string part = text.substr(pos, length);
        while (!part.empty() && (part.back() == '\n' || part.back() == '\r' || part.back() == ' '))
            part.pop_back();
        if (line)
            page += '\n';
        page += part;
        pos += length;
        if (++line == lines) {
            pages.push_back(page);
            page.clear();
            line = 0;
        }
    }
    if (!page.empty())
        pages.push_back(page);
    return pages;
}

void LeogieDisplay::SetChatMessage(const char* role, const char* text) {
    if (!setup_ui_called_ || !role || !text || strlen(text) > 4096)
        return;
    DisplayLockGuard lock(this);
    if (!lock)
        return;
    if (!strcmp(role, "direction")) {
        direction_text_ = !strcmp(text, "zh")   ? "中译英"
                          : !strcmp(text, "en") ? "英译中"
                                                : "中英互译";
    } else if (!strcmp(role, "user")) {
        original_pages_ = Pages(text, 1);
        translated_pages_.clear();
    } else if (!strcmp(role, "assistant")) {
        translated_pages_ = Pages(text, 2);
    } else if (*text) {
        original_pages_.clear();
        translated_pages_ = Pages(text, 2);
        direction_text_ = "Leogie 提示";
    } else {
        return;
    }
    page_ = page_ticks_ = 0;
    RenderMode();
}

void LeogieDisplay::ShowPage() {
    const size_t count = std::max(original_pages_.size(), translated_pages_.size());
    std::string heading = direction_text_;
    if (count > 1)
        heading += " " + std::to_string(page_ % count + 1) + "/" + std::to_string(count);
    lv_label_set_text(direction_, heading.c_str());
    lv_label_set_text(original_, original_pages_.empty()
                                     ? ""
                                     : original_pages_[page_ % original_pages_.size()].c_str());
    if (!translated_pages_.empty()) {
        lv_label_set_text(translated_, translated_pages_[page_ % translated_pages_.size()].c_str());
    } else if (!compact_) {
        lv_label_set_text(translated_, mode_ == Mode::Listening ? "\n说完稍等一下"
                                                                : "一起聊世界！\n中文 / English");
    } else {
        lv_label_set_text(translated_, mode_ == Mode::Listening ? "等你继续说…" : "请稍等…");
    }
}

void LeogieDisplay::ClearChatMessages() {
    if (!setup_ui_called_)
        return;
    DisplayLockGuard lock(this);
    if (!lock)
        return;
    original_pages_.clear();
    translated_pages_.clear();
    direction_text_ = "中英互译";
    page_ = page_ticks_ = 0;
    RenderMode();
}

void LeogieDisplay::Tick() {
    if (timer_phase_ != leogie::Countdown::Phase::Closed)
        return;
    ++ticks_;
    if (mode_ == Mode::Listening && !compact_) {
        for (size_t i = 0; i < bars_.size(); ++i) {
            const int height = 8 + ((ticks_ + i * 3) % 5) * 4;
            lv_obj_set_height(bars_[i], height);
            lv_obj_set_y(bars_[i], 234 - height);
        }
    }
    for (size_t i = 0; i < dots_.size(); ++i)
        lv_obj_set_style_bg_color(dots_[i],
                                  lv_color_hex((ticks_ / 2) % 3 == i ? 0x238bc7 : 0x72badd), 0);
    if (++page_ticks_ >= 20) {
        page_ticks_ = 0;
        if (original_pages_.size() > 1 || translated_pages_.size() > 1) {
            ++page_;
            ShowPage();
        }
    }
}

void LeogieDisplay::UpdateStatusBar(bool all) {
    if (!setup_ui_called_)
        return;
    int battery = -1;
    bool charging = false, discharging = false;
    Board::GetInstance().GetBatteryLevel(battery, charging, discharging);
    DisplayLockGuard lock(this);
    if (!lock)
        return;
    if (!WifiManager::GetInstance().IsConnected())
        lv_label_set_text(network_, "离线");
    else if (battery >= 0)
        lv_label_set_text_fmt(network_, "%d%%", battery);
    else
        lv_label_set_text(network_, "在线");
}

void LeogieDisplay::SetPowerSaveMode(bool on) {
    DisplayLockGuard lock(this);
    if (!lock || !animation_)
        return;
    if (on)
        lv_timer_pause(animation_);
    else
        lv_timer_resume(animation_);
}

void LeogieDisplay::ShowCountdown(leogie::Countdown::Phase phase, int minutes) {
    DisplayLockGuard lock(this);
    if (!lock || !setup_ui_called_)
        return;
    if (phase == timer_phase_ && minutes == timer_minutes_)
        return;
    timer_phase_ = phase;
    timer_minutes_ = minutes;
    if (phase == leogie::Countdown::Phase::Closed) {
        mode_ = Mode::Idle;
        original_pages_.clear();
        translated_pages_.clear();
        direction_text_ = "中英互译";
        lv_label_set_text(status_label_, "Leogie 翻译器");
        lv_label_set_text(hint_, "确定翻译 上键计时");
    }
    RenderMode();
}

void LeogieDisplay::RenderCountdown() {
    using Phase = leogie::Countdown::Phase;
    const bool setting = timer_phase_ == Phase::Setting;
    const bool ringing = timer_phase_ == Phase::Ringing;
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(ringing ? 0xffe27a : 0xb8edd9), 0);
    lv_image_set_src(portrait_, &leogie_small[ringing ? 3 : 0]);
    lv_obj_set_pos(portrait_, 60, 34);
    lv_obj_set_pos(bubble_, 10, 144);
    lv_obj_set_size(bubble_, 220, 144);
    Visible(direction_, false);
    Visible(original_, false);
    Visible(translated_, false);
    for (auto* bar : bars_)
        Visible(bar, false);
    for (auto* dot : dots_)
        Visible(dot, false);
    Visible(notification_label_, false);
    Visible(status_label_, true);
    lv_label_set_text(status_label_, ringing ? "时间到啦！" : setting ? "设个倒计时" : "倒计时中");
    lv_label_set_text_fmt(timer_digits_, "%d", timer_minutes_);
    if (setting) {
        lv_label_set_text(timer_keys_[0], "上键：加 1 分钟");
        lv_label_set_text(timer_keys_[1], "中键：减 1 分钟");
        lv_label_set_text(timer_keys_[2], "确定：开始计时");
    } else if (ringing) {
        lv_label_set_text(timer_keys_[0], "叮叮！时间到啦");
        lv_label_set_text(timer_keys_[1], "按任意键停止铃声");
        lv_label_set_text(timer_keys_[2], "");
    } else {
        lv_label_set_text(timer_keys_[0], "正在倒计时…");
        lv_label_set_text(timer_keys_[1], "到点会响铃");
        lv_label_set_text(timer_keys_[2], "");
    }
    lv_label_set_text(hint_, ringing   ? "按键返回翻译器"
                             : setting ? "长按确定返回"
                                       : "长按确定取消");
}
