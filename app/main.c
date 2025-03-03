#include "stm32f10x.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "esp_at.h"
#include "rtc.h"
#include "st7735.h"
#include "timer.h"
#include "delay.h"
#include "weather.h"


static const char *wifi_ssid = "X100s";
static const char *wifi_passward = "llljjjwww";
static const char *weather_url = "https://api.seniverse.com/v3/weather/now.json?key=S8l98VrOAK5fFI3YX&location=meizhou&language=en&unit=c";
static const char *city = "meizhou";


static uint32_t runms;
static uint32_t disp_height;
static bool sntp_ok = false;
static bool weather_ok = false;

static void timer_elapsed_callback()
{
    runms ++;
    if(runms >= 24 * 60 * 60 * 1000)
    {
        runms = 0;
    }
}

static void wifi_init(void)
{
    st7735_write_string(0, disp_height, "ESP32 Init...", &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += st_font_ascii_8x16.height;
    if(!esp_at_init())
    {
        st7735_write_string(0, 0, "ESP32 Fail...", &st_font_ascii_8x16, ST7735_RED, ST7735_BLACK);
        disp_height += st_font_ascii_8x16.height;
        while(1);
    }

    st7735_write_string(0, disp_height, "WIFI Init...", &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += st_font_ascii_8x16.height;
    if(!esp_at_wifi_init())
    {
        st7735_write_string(0, 16, "WIFI Fail...", &st_font_ascii_8x16, ST7735_RED, ST7735_BLACK);
        disp_height += st_font_ascii_8x16.height;
        while(1);
    }

    st7735_write_string(0, disp_height, "WIFI Connect...", &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += st_font_ascii_8x16.height;
    if(!esp_at_wifi_connect(wifi_ssid, wifi_passward))
    {
        st7735_write_string(0, 32, "Connect Fail...", &st_font_ascii_8x16, ST7735_RED, ST7735_BLACK);
        disp_height += st_font_ascii_8x16.height;
        while(1);
    }

    st7735_write_string(0, disp_height, "Sync Time...", &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += st_font_ascii_8x16.height;
    if(!esp_at_sntp_init())
    {
        st7735_write_string(0, 48, "SyncTimeFail...", &st_font_ascii_8x16, ST7735_RED, ST7735_BLACK);
        disp_height += st_font_ascii_8x16.height;
        while(1);
    }
}

int main(void)
{
	timer_elapsed_register(timer_elapsed_callback);

    board_lowlevel_init();

    rtc_init();

    timer_init(1000);
    timer_start();

    st7735_init();
    st7735_fill_screen(ST7735_BLACK);

    // 显示开机内容
    st7735_write_string(0, disp_height, "Initializing...", &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += st_font_ascii_8x16.height;
    delay_ms(500);

    st7735_write_string(0, disp_height, "Wait ESP32...", &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
    disp_height += st_font_ascii_8x16.height;
    delay_ms(1500);

	wifi_init();

    st7735_write_string(0, disp_height, "Ready", &st_font_ascii_8x16, ST7735_GREEN, ST7735_BLACK);
    disp_height += st_font_ascii_8x16.height;
    delay_ms(500);

    // 清屏
    st7735_fill_screen(ST7735_BLACK);

    char str[64];

    while (1)
    {
        // 每100ms更新时间信息
        if(runms % 100 == 0)
        {
            rtc_date_t date;
            rtc_get_date(&date);
            snprintf(str, sizeof(str), "%02d-%02d", date.month, date.day);
            st7735_write_string(0, 0, str, &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
            snprintf(str, sizeof(str), "%02d%s%02d", date.hour, date.second % 2 ? " " : ":", date.minute);
            st7735_write_string(32, 64, str, &font_time_25x25, ST7735_CYAN, ST7735_BLACK);
        }

        // 每1h联网同步时间
        if(!sntp_ok || runms % (60 * 60 * 1000) == 0)
        {
            uint32_t ts;
            sntp_ok = esp_at_time_get(&ts);
            rtc_set_timestamp(ts);
        }

        // 每10min联网更新天气
        if(!weather_ok || runms % (10 * 60 * 1000) == 0)
        {
            const char *rsp;
            weather_ok = esp_at_http_get(weather_url, &rsp, NULL, 10000);
            weather_t weather;
            weather_parse(rsp, &weather);

            snprintf(str, sizeof(str), "%s", weather.weather);
            st7735_write_string(0, 16, str, &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
            snprintf(str, sizeof(str), "%s", weather.temperature);
            st7735_write_string(0, 32, str, &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
        }

        // 每10s更新环境温度
        if(runms % (10 * 1000) == 0)
        {

        }

        // 每30s更新网络信息
        if(runms % (30 * 1000) == 0)
        {

        }

    }
}
