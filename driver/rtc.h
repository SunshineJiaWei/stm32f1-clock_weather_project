#ifndef __RTC_H
#define __RTC_H


#include <stdint.h>
#include <stdbool.h>


typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} rtc_date_t;


void rtc_init(void);
void rtc_get_date(rtc_date_t* date);
void rtc_set_date(rtc_date_t* date);
void rtc_set_timestamp(uint32_t timestamp);
void rtc_get_timestamp(uint32_t* timestamp);


#endif      /*__RTC_H */
