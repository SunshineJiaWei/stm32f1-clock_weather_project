#ifndef __LCD_SPI_H
#define __LCD_SPI_H


#include <stdint.h>
#include <stdbool.h>


typedef void (*lcd_spi_send_finish_callback_t)(void);


void lcd_spi_init(void);
void lcd_spi_write(uint8_t *data, uint16_t len);
void lcd_spi_write_dma(uint8_t *data, uint16_t len);

void lcd_spi_send_finish_register(lcd_spi_send_finish_callback_t callback);


#endif
