#include "stm32f10x.h"
#include "rtc.h"
#include <string.h>


// 假设时区偏移为 UTC+8
#define TIMEZONE_OFFSET 8


static bool date_validate(const rtc_date_t *date)
{
    if (date->year < 1970 || date->year > 2099)
        return false;
    if (date->month < 1 || date->month > 12)
        return false;
    if (date->day < 1 || date->day > 31)
        return false;
    if (date->hour > 23)
        return false;
    if (date->minute > 59)
        return false;
    if (date->second > 59)
        return false;

    return true;
}
uint32_t date_to_ts(const rtc_date_t *date)
{
    uint16_t year = date->year;
    uint8_t month = date->month;
    uint8_t day = date->day;
    uint8_t hour = date->hour;
    uint8_t minute = date->minute;
    uint8_t second = date->second;

    uint64_t ts = 0;

    /* 二月闰月，计算当作最后一个月份 */
    month -= 2;
    if ((int8_t)month <= 0)
    {
        month += 12;
        year -= 1;
    }

    /* 计算时间戳 */
    ts = (((year / 4 - year / 100 + year / 400 + 367 * month / 12 + day + year * 365 - 719499) * 24 +
          hour) * 60 + minute) * 60 + second;

    return ts;
}

void ts_to_date(uint32_t seconds, rtc_date_t *date)
{
    uint32_t leapyears = 0, yearhours = 0;
    const uint32_t mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const uint16_t ONE_YEAR_HOURS = 8760;

    memset(date, 0, sizeof(rtc_date_t));

    /* 秒 */
    date->second = seconds % 60;
    seconds /= 60;

    /* 分 */
    date->minute = seconds % 60;
    seconds /= 60;

    /* 年 */
    leapyears = seconds / (1461 * 24);
    date->year = (leapyears << 2) + 1970;
    seconds %= 1461 * 24;

    for (;;)
    {
        yearhours = ONE_YEAR_HOURS;

        /* 闰年加一天 */
        if ((date->year & 3) == 0)
            yearhours += 24;

        /* 剩余时间已经不足一年了跳出循环 */
        if (seconds < yearhours)
            break;

        date->year++;
        /* 减去一年的时间 */
        seconds -= yearhours;
    }

    /* 时 */
    date->hour = seconds % 24 + TIMEZONE_OFFSET;
    seconds /= 24;
    seconds++;

    /* 闰年 */
    if ((date->year & 3) == 0)
    {
        if (seconds > 60)
        {
            seconds--;
        }
        else
        {
            if (seconds == 60)
            {
                date->month = 2;
                date->day = 29;
                return;
            }
        }
    }

    /* 月 */
    for (date->month = 0; mdays[date->month] < seconds; date->month++)
        seconds -= mdays[date->month];
    date->month++;

    /* 日 */
    date->day = seconds;
}

void rtc_init(void)
{
    PWR_BackupAccessCmd(ENABLE);                        // 开启对后备区域的访问权限，后备区域（包括RTC和BKP寄存器）在复位后默认是写保护的，需要通过PWR的DBP位来解锁
    BKP_DeInit();                                       // 备份寄存器初始化

    RCC_LSEConfig(RCC_LSE_ON);                          // 启动外部低速时钟，32.768KHz
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET); // 等待LSE稳定

    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);             // 选择RTC时钟源
    RCC_RTCCLKCmd(ENABLE);                              // 使能RTC时钟
    RTC_WaitForSynchro();                               // 等待RTC同步，
    RTC_WaitForLastTask();                              // 等待上一次的RTC操作完成
    RTC_SetPrescaler(32767);                            // 设置RTC的预分频器，预分频后的计数频率为1Hz
    RTC_WaitForLastTask();                              // 等待上一次的RTC操作完成
}

void rtc_get_date(rtc_date_t* date)
{
    uint32_t ts = RTC_GetCounter();
    if(date)
    {
        ts_to_date(ts, date);
    }
}

void rtc_set_date(rtc_date_t* date)
{
    if(!date_validate(date)) return;

    uint32_t ts = date_to_ts(date);

    RTC_WaitForLastTask();
    RTC_SetCounter(ts);
    RTC_WaitForLastTask();
}

void rtc_set_timestamp(uint32_t timestamp)
{
    RTC_WaitForLastTask();
    RTC_SetCounter(timestamp);
    RTC_WaitForLastTask();
}

void rtc_get_timestamp(uint32_t* timestamp)
{
    if(timestamp)
    {
        RTC_WaitForLastTask();
        *timestamp = RTC_GetCounter();
        RTC_WaitForLastTask();
    }
}
