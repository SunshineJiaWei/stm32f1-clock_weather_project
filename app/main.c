#include "stm32f10x.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "esp_at.h"
#include "rtc.h"
#include "st7735.h"


static const char *wifi_ssid = "X100s";
static const char *wifi_passward = "llljjjwww";
static const char *weather_url = "https://api.seniverse.com/v3/weather/now.json?key=S8l98VrOAK5fFI3YX&location=meizhou&language=en&unit=c";
static const char *city = "meizhou";


int main(void)
{
    board_lowlevel_init();
    st7735_init();
    st7735_fill_screen(ST7735_GREEN);

//   if(!esp_at_init()) while(1);
//   if(!esp_at_wifi_init()) while(1);
//   if(!esp_at_wifi_connect(wifi_ssid, wifi_passward)) while(1);

    while (1)
    {
        st7735_write_char(0, 0, 'a', &st_font_8x16, ST7735_BLACK, ST7735_GREEN);
        st7735_write_string(0, 16, "hello world", &st_font_8x16, ST7735_BLACK, ST7735_GREEN);
    }
}
