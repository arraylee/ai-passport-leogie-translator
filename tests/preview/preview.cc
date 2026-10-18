#include <lvgl.h>
#include <src/misc/lv_text_private.h>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include "application.h"
#include "leogie_display.h"

class PreviewDisplay : public LeogieDisplay {
public:
    lv_obj_t* Notification() { return notification_label_; }
};

static std::vector<uint8_t> framebuffer(240 * 320 * 3);

static void verify_labels() {
    auto* screen = lv_screen_active();
    lv_obj_update_layout(screen);
    for (uint32_t i = 0; i < lv_obj_get_child_count(screen); ++i) {
        auto* object = lv_obj_get_child(screen, i);
        if (!lv_obj_check_type(object, &lv_label_class))
            continue;
        lv_text_attributes_t a;
        lv_text_attributes_init(&a);
        a.max_width = lv_obj_get_content_width(object);
        a.line_space = lv_obj_get_style_text_line_space(object, LV_PART_MAIN);
        const auto* f = lv_obj_get_style_text_font(object, LV_PART_MAIN);
        lv_point_t size;
        lv_text_get_size_attributes(&size, lv_label_get_text(object), f, &a);
        if (lv_label_get_long_mode(object) == LV_LABEL_LONG_WRAP &&
            size.y > lv_obj_get_height(object)) {
            fprintf(stderr, "CLIPPED %s: %d > %d\n", lv_label_get_text(object), size.y,
                    lv_obj_get_height(object));
            abort();
        }
        assert(lv_obj_get_y(object) + lv_obj_get_height(object) <= 320);
    }
}
static void capture(const std::filesystem::path& path, lv_display_t* display) {
    verify_labels();
    lv_refr_now(display);
    std::ofstream out(path, std::ios::binary);
    out << "P6\n240 320\n255\n";
    for (size_t i = 0; i < framebuffer.size(); i += 3) {
        out.put(framebuffer[i + 2]);
        out.put(framebuffer[i + 1]);
        out.put(framebuffer[i]);
    }
}
int main(int argc, char** argv) {
    assert(argc == 3);
    // No external font partition: production UI must render on a fresh install.
    std::filesystem::path output(argv[2]);
    std::filesystem::create_directories(output);
    lv_init();
    auto* display = lv_display_create(240, 320);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB888);
    lv_display_set_buffers(display, framebuffer.data(), nullptr, framebuffer.size(),
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(
        display, [](lv_display_t* d, const lv_area_t*, uint8_t*) { lv_display_flush_ready(d); });
    PreviewDisplay ui;
    ui.SetupUI();
    ui.UpdateStatusBar();
    capture(output / "idle.ppm", display);
    Application::GetInstance().state = kDeviceStateActivating;
    ui.SetStatus("激活设备");
    ui.SetChatMessage("user", "打开 xiaozhi.me 绑定设备");
    ui.SetChatMessage("assistant", "123456");
    capture(output / "activation.ppm", display);
    ui.ClearChatMessages();
    Application::GetInstance().state = kDeviceStateListening;
    ui.SetStatus("listening");
    capture(output / "listening.ppm", display);
    // STT arrives before tts/start; the device state is still Listening here.
    ui.SetStatus("翻译中");
    lv_obj_t* status = nullptr;
    for (uint32_t i = 0; i < lv_obj_get_child_count(lv_screen_active()); ++i) {
        auto* object = lv_obj_get_child(lv_screen_active(), i);
        if (lv_obj_check_type(object, &lv_label_class) && lv_obj_get_y(object) == 7 &&
            lv_obj_get_x(object) == 12) {
            status = object;
            break;
        }
    }
    assert(status);
    assert(std::string(lv_label_get_text(status)) == "翻译中");
    capture(output / "awaiting-translation.ppm", display);
    Application::GetInstance().state = kDeviceStateSpeaking;
    ui.SetStatus("thinking");
    ui.SetChatMessage("direction", "zh");
    ui.SetChatMessage("user", "请问火车站在哪里？");
    capture(output / "thinking.ppm", display);
    ui.SetChatMessage("assistant", "Where is the train station?");
    ui.SetStatus("正在播报");
    capture(output / "speaking.ppm", display);
    ui.SetStatus("speaking");
    capture(output / "speaking-deferred.ppm", display);
    ui.SetChatMessage("user", "请帮我买两张明天早上八点到上海的火车票，我们不需要头等座。");
    ui.SetChatMessage("assistant",
                      "Please get me two train tickets to Shanghai for eight tomorrow morning. We "
                      "do not need first class.");
    capture(output / "long-1.ppm", display);
    for (int i = 0; i < 21; i++) {
        lv_tick_inc(250);
        lv_timer_handler();
    }
    capture(output / "long-2.ppm", display);
    ui.SetChatMessage("direction", "en");
    ui.SetChatMessage("user", "I do not eat peanuts.");
    ui.SetChatMessage("assistant", "我不吃花生。");
    capture(output / "english.ppm", display);
    ui.SetStatus("请重试");
    ui.SetChatMessage("system", "网络已断开，请按一下重新开始。");
    capture(output / "error.ppm", display);
    ui.ClearChatMessages();
    Application::GetInstance().state = kDeviceStateWifiConfiguring;
    ui.SetStatus("配网模式");
    ui.SetChatMessage("system", "手机连接 Xiaozhi 热点，打开 192.168.4.1 配网。");
    capture(output / "wifi.ppm", display);
    ui.SetChatMessage("user", "我想去 San Francisco，带上 2 bottles of water。");
    ui.SetChatMessage("assistant", "supercalifragilisticexpialidocious1234567890abcdefghijk");
    for (int page = 0; page < 8; ++page) {
        capture(output / ("mixed-long-word-" + std::to_string(page) + ".ppm"), display);
        for (int tick = 0; tick < 20; ++tick) {
            lv_tick_inc(250);
            lv_timer_handler();
        }
    }
    auto* notice = ui.Notification();
    assert(lv_label_get_long_mode(notice) == LV_LABEL_LONG_SCROLL_CIRCULAR);
    assert(lv_obj_get_x(notice) + lv_obj_get_width(notice) <= 176);
    lv_label_set_text(notice, "连接成功: 12345678901234567890123456789012");
    lv_obj_remove_flag(notice, LV_OBJ_FLAG_HIDDEN);
    capture(output / "notification.ppm", display);
    using Phase = leogie::Countdown::Phase;
    ui.ShowCountdown(Phase::Setting, 10);
    capture(output / "timer-setting.ppm", display);
    ui.ShowCountdown(Phase::Setting, 120);
    capture(output / "timer-max.ppm", display);
    ui.ShowCountdown(Phase::Running, 10);
    ui.SetStatus("网络已断开"); // asynchronous status cannot replace countdown
    capture(output / "timer-running.ppm", display);
    ui.ShowCountdown(Phase::Running, 1);
    capture(output / "timer-last-minute.ppm", display);
    ui.ShowCountdown(Phase::Ringing, 0);
    capture(output / "timer-ringing.ppm", display);
    ui.ShowCountdown(Phase::Closed, 0);
    capture(output / "timer-exit.ppm", display);
    puts("Actual LVGL screens render without clipped labels at 240x320.");
}
