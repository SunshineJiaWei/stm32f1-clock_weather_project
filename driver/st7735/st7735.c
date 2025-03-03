#include "stm32f10x.h"
#include "lcd_spi.h"
#include "st7735.h"
#include "delay.h"
#include "stfonts.h"
#include <stddef.h>


#define RES_PORT    GPIOB
#define RES_PIN     GPIO_Pin_0
#define DC_PORT     GPIOB
#define DC_PIN      GPIO_Pin_1
#define CS_PORT     GPIOA
#define CS_PIN      GPIO_Pin_4
#define BLK_PORT    GPIOB
#define BLK_PIN     GPIO_Pin_10

#define GRAM_BUFFER_SIZE 4096

// ST7735 Commands
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_GAMSET  0x26
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define CMD_DELAY      0xFF
#define CMD_EOF        0xFF


static volatile bool lcd_spi_async_done;
static uint8_t gram_buffer[GRAM_BUFFER_SIZE];

static const uint8_t init_cmd_list[] =
{
    0x11,  0,
    CMD_DELAY, 12,
    0xB1,  3,  0x01, 0x2C, 0x2D,
    0xB2,  3,  0x01, 0x2C, 0x2D,
    0xB3,  6,  0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,
    0xB4,  1,  0x07,
    0xC0,  3,  0xA2, 0x02, 0x84,
    0xC1,  1,  0xC5,
    0xC2,  2,  0x0A, 0x00,
    0xC3,  2,  0x8A, 0x2A,
    0xC4,  2,  0x8A, 0xEE,
    0xC5,  1,  0x0E,
    0x36,  1,  0xC8,
    0xE0,  16, 0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20, 0x22, 0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10,
    0xE1,  16, 0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29, 0x2E, 0x30, 0x30, 0x39, 0x3F, 0x00, 0x07, 0x03, 0x10,
    0x2A,  4,  0x00, 0x00, 0x00, 0x7F,
    0x2B,  4,  0x00, 0x00, 0x00, 0x9F,
    0xF6,  1,  0x00,
    0x3A,  1,  0x05,
    0x29,  0,

    CMD_DELAY, CMD_EOF,
};

static void spi_on_async_finish(void)
{
    lcd_spi_async_done = true;
}

static void st7735_pin_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // RES
    GPIO_InitStructure.GPIO_Pin = RES_PIN;
    GPIO_Init(RES_PORT, &GPIO_InitStructure);

    // DC
    GPIO_InitStructure.GPIO_Pin = DC_PIN;
    GPIO_Init(DC_PORT, &GPIO_InitStructure);

    // CS
    GPIO_InitStructure.GPIO_Pin = CS_PIN;
    GPIO_Init(CS_PORT, &GPIO_InitStructure);

    // BLK
    GPIO_InitStructure.GPIO_Pin = BLK_PIN;
    GPIO_Init(BLK_PORT, &GPIO_InitStructure);
    GPIO_WriteBit(BLK_PORT, BLK_PIN, Bit_RESET);  // 关闭背光
}

void st7735_reset(void)
{
    GPIO_WriteBit(RES_PORT, RES_PIN, Bit_RESET);
    delay_ms(2);
    GPIO_WriteBit(RES_PORT, RES_PIN, Bit_SET);
    delay_ms(120);
}

static void st7735_write_cmd(uint8_t cmd)
{
    GPIO_WriteBit(DC_PORT, DC_PIN, Bit_RESET);
    lcd_spi_write(&cmd, 1);
}

static void st7735_write_data(uint8_t *data, uint16_t len)
{
    GPIO_WriteBit(DC_PORT, DC_PIN, Bit_SET);
    lcd_spi_async_done = false;
    lcd_spi_write_dma(data, len);
    while(!lcd_spi_async_done);
}

// 初始化LCD，对照ST7735手册
static void st7735_exec_cmds(const uint8_t *cmd_list)
{
    while (1)
    {
        uint8_t cmd = *cmd_list++;// 从命令列表中读取当前命令
        uint8_t num = *cmd_list++;// 读取参数数量或延时时间
        if (cmd == CMD_DELAY)
        {
            if (num == CMD_EOF)	  // 若参数是终止符，退出循环
                break;
            else
                delay_ms(num * 10);
        }
        else
        {
            st7735_write_cmd(cmd);// 发送命令到显示屏
            if (num > 0) {
                st7735_write_data((uint8_t *)cmd_list, num);// 发送参数数据
            }
            cmd_list += num; // 移动指针到下一个命令位置
        }
    }
}

void st7735_select(void)
{
    GPIO_WriteBit(CS_PORT, CS_PIN, Bit_RESET);
}

void st7735_unselect(void)
{
    GPIO_WriteBit(CS_PORT, CS_PIN, Bit_SET);
}

void st7735_init(void)
{
    lcd_spi_init();

    lcd_spi_send_finish_register(spi_on_async_finish);

    st7735_pin_init();                  // 启动

    st7735_reset();                     // 复位

    st7735_select();                    // 选择LCD
    st7735_exec_cmds(init_cmd_list);	// 初始化LCD配置参数
    st7735_unselect();

    GPIO_WriteBit(BLK_PORT, BLK_PIN, Bit_SET); // 打开背光
}

static void st7735_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    // 设置列地址
    st7735_write_cmd(ST7735_CASET);
    uint8_t data[] = {0x00, x0 + ST7735_XSTARTOFFSET, 0x00, x1 + ST7735_XSTARTOFFSET};
    st7735_write_data(data, sizeof(data));

    // 设置行地址
    st7735_write_cmd(ST7735_RASET);
    data[1] = y0 + ST7735_YSTARTOFFSET;
    data[3] = y1 + ST7735_YSTARTOFFSET;
    st7735_write_data(data, sizeof(data));

    // 写入ram
    st7735_write_cmd(ST7735_RAMWR);
}

void st7735_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if(x >= ST7735_WIDTH || y >= ST7735_HEIGHT) return; // 坐标越界判断
    if(x + w - 1 >= ST7735_WIDTH) w = ST7735_WIDTH - x; // 避免溢出
    if(y + h - 1 >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    st7735_select();
    st7735_set_window(x, y, x + w - 1, y + h - 1);      // 设置显示窗口

    uint8_t pixel[2] = {color >> 8, color & 0xFF};      // RGB565高低位分离

    for(uint16_t i = 0;i < w * h;i += GRAM_BUFFER_SIZE / 2) // 每次加上2048个像素
    {
        uint16_t pixel_len = w * h - i;
        if(pixel_len > GRAM_BUFFER_SIZE / 2) pixel_len = GRAM_BUFFER_SIZE / 2; // pixel_len最大为2048
        if(i == 0)
        {
			uint8_t *pbuffer = gram_buffer;
            for(uint16_t j = 0;j < pixel_len;j ++)
            {
                // 一个像素两个字节表示
                *pbuffer++ = pixel[0];
                *pbuffer++ = pixel[1];
            }
        }
        // 显存写入GRAM
        st7735_write_data(gram_buffer, pixel_len * 2);
    }
    st7735_unselect();
}

void st7735_fill_screen(uint16_t color)
{
    st7735_fill_rect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

static const uint8_t* st7735_find_font(const st_fonts_t *font, char ch)
{
    uint32_t bytes_per_line = (font->width + 7) / 8;

    for(uint32_t i = 0;i < font->count;i ++)
    {
        const uint8_t *pcode = font->data + i * (font->height * bytes_per_line + 1);
        if(*pcode == ch)
        {
            return pcode + 1;
        }
    }
	
    return NULL;
}

void st7735_write_char(uint16_t x, uint16_t y, char ch, st_fonts_t *font, uint16_t color, uint16_t bgcolor)
{
    st7735_select();

    st7735_set_window(x, y, x + font->width - 1, y + font->height - 1);

    uint8_t bytes_per_line = (font->width + 7) / 8; // 向上取整，判断每行显示多少个字节

    uint8_t *pbuffer = gram_buffer;
    const uint8_t *fcode = st7735_find_font(font, ch);
    for(uint8_t y = 0;y < font->height;y ++) // 遍历y轴/行
    {
       const uint8_t *pcode = fcode + y * bytes_per_line; // 定位到具体行的第几个字节
       for(uint8_t x = 0;x < font->width;x ++) // 遍历x轴/列,每行的像素点
       {
            uint8_t byte = pcode[x >> 3];   // 渲染每行的第几个字节，汉字有两个字节，相当于*(pcode + x / 8)
            if((byte << (x & 0x7)) & 0x80)  // 取出当前bit的值，0填充背景色，1填充前景色
            {
                *pbuffer++ = color >> 8;
                *pbuffer++ = 0xFF & color;
            }
            else
            {
                *pbuffer++ = bgcolor >> 8;
                *pbuffer++ = 0xFF & bgcolor;
            }
       }
    }

    st7735_write_data(gram_buffer, pbuffer - gram_buffer);

    st7735_unselect();
}

void st7735_write_string(uint16_t x, uint16_t y, const char *str, st_fonts_t *font, uint16_t color, uint16_t bgcolor)
{
    while(*str)
    {
        if(x + font->width >= ST7735_WIDTH)
        {
            x = 0;
            y += font->height;
            if(y + font->height >= ST7735_HEIGHT) break; // 超出屏幕范围

            if(*str == ' ')
            {
                str ++;
                continue;
            }
        }

        st7735_write_char(x, y, *str, font, color, bgcolor);
        x += font->width;
        str ++;
    }
}
