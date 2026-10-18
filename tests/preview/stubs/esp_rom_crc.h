#pragma once
#include <zlib.h>
inline uint32_t esp_rom_crc32_le(uint32_t crc,const uint8_t* bytes,size_t size) {return crc32(crc,bytes,size);}
