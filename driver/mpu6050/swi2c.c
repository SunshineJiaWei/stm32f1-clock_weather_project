#include "stm32f10x.h"
#include "swi2c.h"
#include "delay.h"


#define I2C_SCL_PORT    GPIOB
#define I2C_SCL_PIN     GPIO_Pin_6
#define I2C_SDA_PORT    GPIOB
#define I2C_SDA_PIN     GPIO_Pin_7

#define I2C_SCL_HIGH()  GPIO_SetBits(I2C_SCL_PORT, I2C_SCL_PIN)
#define I2C_SCL_LOW()   GPIO_ResetBits(I2C_SCL_PORT, I2C_SCL_PIN)
#define I2C_SDA_HIGH()  GPIO_SetBits(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_SDA_LOW()   GPIO_ResetBits(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_READ_SDA()  GPIO_ReadInputDataBit(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_SDA_READ()  GPIO_ReadInputDataBit(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_DELAY()     delay_us(5)


static void swi2c_start()
{
    I2C_SDA_HIGH();
    I2C_SCL_HIGH();
    I2C_DELAY();
    I2C_SDA_LOW();
    I2C_DELAY();
    I2C_SCL_LOW();
}

static void swi2c_stop()
{
    I2C_SDA_LOW();
    I2C_DELAY();
    I2C_SCL_HIGH();
    I2C_DELAY();
    I2C_SDA_HIGH();
    I2C_DELAY();
}

static bool swi2c_write_byte(uint8_t data)
{
    // 发送一个字节数据
   for(uint8_t i = 0;i < 8;i ++)
    {
        // MSB，高位先行
        if((data << i) & 0x80)
        {
            I2C_SDA_HIGH();
        }
        else
        {
            I2C_SDA_LOW();
        }
        I2C_DELAY();
        I2C_SCL_HIGH(); //
        I2C_DELAY();
        I2C_SCL_LOW();
    }

    I2C_SDA_HIGH(); // 释放总线，等待ACK
    I2C_DELAY();
    I2C_SCL_HIGH();// 读取ACK
    I2C_DELAY();
    if(I2C_SDA_READ())
    {
        // ACK=1，发送失败，从机未接收到数据
        I2C_SCL_LOW();
        return false;
    }
    I2C_SCL_LOW();
    I2C_DELAY();

    return true;
}

static uint8_t swi2c_read_byte(bool ack)
{
    uint8_t data = 0;

    I2C_SDA_HIGH(); // 释放总线，等待从机发送数据

    // 接收一个字节数据
    for(uint8_t i = 0;i < 8;i ++)
    {
        I2C_SCL_HIGH(); // 拉高读取数据
        I2C_DELAY();
        if(I2C_SDA_READ())
        {
            data |= (0x80 >> i);
        }
        I2C_SCL_LOW();
        I2C_DELAY();
    }

    if(ack) I2C_SDA_LOW();
    else I2C_SDA_HIGH();
    I2C_DELAY();
    I2C_SCL_HIGH(); // 主机读取应答位

    I2C_DELAY();
    I2C_SCL_LOW();
    I2C_SDA_HIGH();
    I2C_DELAY();

    return data;
}

static void swi2c_io_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_SCL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_SDA_PORT, &GPIO_InitStructure);
}

void swi2c_init(void)
{
    swi2c_io_init();
}

bool swi2c_write(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    // 发送开始信号
    swi2c_start();

    // 发送从机地址+写操作0
    if(!swi2c_write_byte(addr << 1))
    {
        swi2c_stop();
        return false;
    }

    // 发送寄存器地址
    swi2c_write_byte(reg);

    for(uint16_t i = 0;i < length;i ++)
    {
        swi2c_write_byte(data[i]);
    }

    swi2c_stop();

    return true;
}

bool swi2c_read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    // 发送开始信号
    swi2c_start();

    // 发送从机地址
    if(!swi2c_write_byte(addr << 1))
    {
        swi2c_stop();
        return false;
    }

    // 发送寄存器地址
    swi2c_write_byte(reg);

    // 再次发送开始信号
    swi2c_start();
    // 发送从机地址+读操作1
    swi2c_write_byte((addr << 1) | 0x01);

    for(uint16_t i = 0;i < length;i ++)
    {
        data[i] = swi2c_read_byte(i == length - 1 ? false : true);
    }

    swi2c_stop();

    return true;
}

