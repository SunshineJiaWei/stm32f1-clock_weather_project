#ifndef __ST_IMAGE_H
#define __ST_IMAGE_H


#include <stdint.h>


typedef struct {
    const uint16_t width;
    const uint16_t height;
    const uint8_t *data;
    const uint32_t count;
} st_image_t;

extern const st_image_t icon_weather_duoyun;
extern const st_image_t icon_weather_feng;
extern const st_image_t icon_weather_qing;
extern const st_image_t icon_weather_xue;
extern const st_image_t icon_weather_yin;
extern const st_image_t icon_weather_yu;

extern const st_image_t icon_temper;


#endif      /*__ST_IMAGE_H*/
