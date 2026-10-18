#pragma once
#include <cstdint>
#include <cstddef>
using esp_partition_mmap_handle_t = unsigned;
using esp_partition_subtype_t = int;
struct esp_partition_t {size_t size;};
constexpr int ESP_PARTITION_TYPE_DATA=1, ESP_PARTITION_MMAP_DATA=1, ESP_OK=0;
const esp_partition_t* esp_partition_find_first(int,int,const char*);
int esp_partition_mmap(const esp_partition_t*,size_t,size_t,int,const void**,esp_partition_mmap_handle_t*);
void esp_partition_munmap(esp_partition_mmap_handle_t);
