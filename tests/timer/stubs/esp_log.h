#pragma once
#include <cstdio>
#define ESP_LOGE(tag, format, ...) std::fprintf(stderr, "%s: " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) ((void)0)
#define ESP_LOGD(tag, format, ...) ((void)0)
