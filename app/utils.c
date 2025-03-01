#include <stdint.h>
#include "stm32f10x.h"


void delay(uint32_t ticks)
{
    for (uint32_t t = 0; t < ticks; t++)
    {
        for (uint32_t n = 0; n < 7200; n++)
        {
            __NOP();
        }
    }
}

