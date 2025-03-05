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
#include "mpu6050.h"
#include "stimage.h"


static const char *wifi_ssid = "X100s";
static const char *wifi_passward = "llljjjwww";
static const char *weather_url = "https://api.seniverse.com/v3/weather/now.json?key=S8l98VrOAK5fFI3YX&location=meizhou&language=en&unit=c";


static uint32_t runms;
static uint32_t disp_height;
static bool sntp_ok = false;
static bool weather_ok = false;
static bool temper_ok = false;

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
    board_lowlevel_init();

    mpu6050_init();

    rtc_init();

    timer_init(1000);
	timer_elapsed_register(timer_elapsed_callback);
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
            snprintf(str, sizeof(str), "%02d-%02d-%02d", date.year % 100, date.month, date.day);
            st7735_write_string(0, 0, str, &st_font_ascii_8x16, ST7735_WHITE, ST7735_BLACK);
            snprintf(str, sizeof(str), "%02d%s%02d", date.hour, date.second % 2 ? " " : ":", date.minute);
            st7735_write_string(0, 80, str, &font_time_24x48, ST7735_CYAN, ST7735_BLACK);
        }

        // 每1h联网同步时间
        if(!sntp_ok || runms % (60 * 60 * 1000) == 0)
        {
            uint32_t ts;
            sntp_ok = esp_at_get_time(&ts);
            rtc_set_timestamp(ts);
        }

        // 每10min联网更新天气
        if(!weather_ok || runms % (10 * 60 * 1000) == 0)
        {
            const char *rsp;
            weather_ok = esp_at_get_http(weather_url, &rsp, NULL, 10000);
            weather_t weather;
            weather_parse(rsp, &weather);

            const st_image_t *img = NULL;
            if (strcmp(weather.weather, "Cloudy") == 0)
            {
                img = &icon_weather_duoyun;
            }
            else if (strcmp(weather.weather, "Wind") == 0)
            {
                img = &icon_weather_feng;
            }
            else if (strcmp(weather.weather, "Clear") == 0)
            {
                img = &icon_weather_qing;
            }
            else if (strcmp(weather.weather, "Snow") == 0)
            {
                img = &icon_weather_xue;
            }
            else if (strcmp(weather.weather, "Overcast") == 0)
            {
                img = &icon_weather_yin;
            }
            else if (strcmp(weather.weather, "Rain") == 0)
            {
                img = &icon_weather_yu;
            }

            if (img != NULL)
            { /* 如果有匹配的天气，则显示天气图标 */
                st7735_draw_image(0, 16, img->width, img->height, img->data);
				snprintf(str, sizeof(str), "%s", weather.weather);
				st7735_write_string(0, 64, str, &st_font_ascii_8x16, ST7735_YELLOW, ST7735_BLACK);
            }
            else
            { /* 否则直接显示天气文字 */
                snprintf(str, sizeof(str), "%s", weather.weather);
                st7735_write_string(0, 16, str, &st_font_ascii_8x16, ST7735_YELLOW, ST7735_BLACK);
            }

            snprintf(str, sizeof(str), "%s", weather.temperature);
            st7735_write_string(74, 0, str, &font_temper_16x32, ST7735_CYAN, ST7735_BLACK);
            st7735_write_char(106, 0, 'C', &self_font_temper_16x32, ST7735_CYAN, ST7735_BLACK);
        }

		/* 环境温度从mpu6050读取 ^_^ */
        // 每10s更新环境温度
        if(!temper_ok || runms % (10 * 1000) == 0)
        {
            float temper = mpu6050_read_temper();
			if(temper) temper_ok = true;
		    snprintf(str, sizeof(str), "%5.1f", temper);
		    st7735_write_string(74, 40, str, &st_font_ascii_8x16, ST7735_GREEN, ST7735_BLACK);

            st7735_write_char(114, 40, 'C', &st_font_temper_8x16, ST7735_GREEN, ST7735_BLACK);
        }
    }
}
