#ifndef __ST_FONTS_H
#define __ST_FONTS_H


#include <stdint.h>
#include <stdbool.h>

// 定义字库类型
typedef struct {
    const uint16_t width;
    const uint16_t height;
    const uint8_t *data;
    const uint32_t count;
} st_fonts_t;


extern st_fonts_t st_font_ascii_8x16;
extern st_fonts_t font_time_25x25;


#endif      /*__ST_FONTS_H*/
