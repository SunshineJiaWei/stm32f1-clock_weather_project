#include "stm32f10x.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "esp_at.h"


static const char *wifi_ssid = "X100s";
static const char *wifi_passward = "llljjjwww";
static const char *weather_url = "https://api.seniverse.com/v3/weather/now.json?key=S8l98VrOAK5fFI3YX&location=meizhou&language=en&unit=c";
static const char *city = "meizhou";


static void board_lowlevel_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
}

int main(void)
{
    board_lowlevel_init();

    if(!esp_at_init()) while(1);
    if(!esp_at_wifi_init()) while(1);
    if(!esp_at_wifi_connect(wifi_ssid, wifi_passward)) while(1);

    while (1)
    {

    }
}
