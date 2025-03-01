#include "stm32f10x.h"
#include "esp_usart.h"
#include <string.h>


static esp_usart_reveive_callback_t esp_usart_receive_callback;

static void esp_usart_io_init(void)
{
    // usart2 tx:PA2,rx:PA3
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void esp_usart_nvic_init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void esp_usart_lowlevel_init(void)
{
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &USART_InitStructure);
	
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART2, ENABLE);
}

void esp_usart_init(void)
{
    esp_usart_io_init();
    esp_usart_nvic_init();
    esp_usart_lowlevel_init();
}

void esp_usart_write_data(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    for(i = 0;i < len;i ++)
    {
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, data[i]); // 软件往DR寄存器写数据后，TXE置0
    }
}

void esp_usart_write_string(const char *str)
{
    uint16_t len = strlen(str);
    esp_usart_write_data((uint8_t *)str, len);
}

void usart_receive_register(esp_usart_reveive_callback_t callback)
{
    esp_usart_receive_callback = callback;
}

void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
    {
        uint8_t data = USART_ReceiveData(USART2);
        if(esp_usart_receive_callback)
        {
            esp_usart_receive_callback(data);
        }
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}
