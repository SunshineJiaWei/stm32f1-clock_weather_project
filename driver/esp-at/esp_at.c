#include "stm32f10x.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_at.h"
#include "esp_usart.h"
#include "delay.h"


#define RX_BUFFER_SIZE      4096

#define RX_RESULT_OK        0
#define RX_RESULT_ERROR     1
#define RX_RESULT_FAIL      2

static uint8_t rxdata_buffer[RX_BUFFER_SIZE]; // 缓存esp32返回的数据
static uint16_t rxdata_len;                   // 缓存数据长度
static uint8_t rxresult;                      // 记录esp32返回的结果
static bool rxready;                          // 是否接收数据

static void on_usart_reveive(uint8_t data)
{
    // 没有数据请求，不接受数据
    if(!rxready) return;

    // 防止缓存区溢出
    if(rxdata_len < RX_BUFFER_SIZE)
    {
        rxdata_buffer[rxdata_len++] = data;
    }
    else
    {
        rxresult = RX_RESULT_FAIL;
        rxready = false;
        return;
    }

    // 数据接收完毕
    if(data == '\n')
    {
        if(rxdata_len >= 2 && rxdata_buffer[rxdata_len - 2] == '\r')
        {
            if(rxdata_len >= 4 && rxdata_buffer[rxdata_len - 4] == 'O' && rxdata_buffer[rxdata_len - 3] == 'K')
            {
                rxresult = RX_RESULT_OK;
                rxready = false;
            }
            else if(rxdata_len >= 7 && rxdata_buffer[rxdata_len - 7] == 'E' &&
                rxdata_buffer[rxdata_len - 6] == 'R' && rxdata_buffer[rxdata_len - 5] == 'R' &&
                rxdata_buffer[rxdata_len - 4] == 'O' && rxdata_buffer[rxdata_len - 3] == 'R')
            {
                rxresult = RX_RESULT_ERROR;
                rxready = false;
            }
        }

    }
}

bool esp_at_send_command(const char *cmd, const char **rsp, uint32_t timeout)
{
    rxready = true;
    rxresult = RX_RESULT_FAIL;

    esp_usart_write_string(cmd);
    esp_usart_write_string("\r\n");

    // 等待数据接收完毕
    while(rxready && timeout --)
    {
        delay_ms(1);
    }
    rxready = false;

    if(rsp) *rsp = (const char *)rxdata_buffer;

    return rxresult == RX_RESULT_OK;

}

bool esp_at_reset(void)
{
    // 恢复出厂设置esp32, 芯片到手时可能保存了其他数据，让芯片更可控
    if(!esp_at_send_command("AT+RESTORE", NULL, 1000))
    {
        return false;
    }
    delay_ms(2000);

    // 关闭回显
    if(!esp_at_send_command("ATE0", NULL, 1000))
    {
        return false;
    }

    // 关闭存储
    if(!esp_at_send_command("AT+SYSSTORE=0", NULL, 1000))
    {
        return false;
    }

    return true;
}

bool esp_at_init(void)
{
    esp_usart_init();
    usart_receive_register(on_usart_reveive);
    return esp_at_reset();
}

bool esp_at_wifi_init(void)
{
    // 设置为station模式
    if(!esp_at_send_command("AT+CWMODE=1", NULL, 1000))
    {
        return false;
    }
    return true;
}

bool esp_at_wifi_connect(const char* ssid, const char* pwd)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    if(!esp_at_send_command(cmd, NULL, 10000))
    {
        return false;
    }
    return true;
}

bool esp_at_sntp_init(void)
{
    // 设置sntp模式,设 置 中 国 时 区 (UTC+08:00)
    if(!esp_at_send_command("AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"", NULL, 1000))
    {
        return false;
    }

    // 查询SNTP时间
    if(!esp_at_send_command("AT+CIPSNTPTIME?", NULL, 1000))
    {
        return false;
    }

    return true;
}

bool esp_at_http_get(const char* url, const char** rsp, uint32_t *length, uint32_t timeout)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+HTTPCGET=\"%s\"", url);
    if(!esp_at_send_command(cmd, rsp, timeout))
    {
        return false;
    }

    return true;
}

bool esp_at_time_get(uint32_t *timestamp)
{
    const char *esp;

    if(!esp_at_send_command("AT+SYSTIMESTAMP?", &esp, 1000))
    {
        return false;
    }

    char *sts =strstr(esp, "+SYSTIMESTAMP:");// strstr()在一个字符串中查找子字符串的第一次出现位置
    sts += strlen("+SYSTIMESTAMP:"); // 指向时间戳

    *timestamp = atoi(sts); // atoi()把字符串转换为一个整数

    return true;
}


