#ifndef __SWI2C_H
#define __SWI2C_H


#include <stdint.h>
#include <stdbool.h>


void swi2c_init(void);
bool swi2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length);
bool swi2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length);


#endif      /*__SWI2C_H*/
