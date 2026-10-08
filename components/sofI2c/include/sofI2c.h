#ifndef _I2C_MASTER_H
#define _I2C_MASTER_H
#include "driver/gpio.h"

// extern  bool  ack;

// #define  LT8918LConfig  0x01
// #define  LT8918L_ADDR   0x40

#define DEVICE_ADDRESS 0x15 // 7位地址
#define BIT_WRITE 0
#define BIT_READ 1

// 引脚定义
/*
 * SCL——PB6
 * SDA——PB7
 */

#define I2C_SCL_PIN GPIO_NUM_0
#define I2C_SDA_PIN GPIO_NUM_1
#define I2C_GPIO_PORT GPIOC
#define I2C_GPIO_CLK_ENABLE() __HAL_RCC_GPIOC_CLK_ENABLE()

#define I2C_SDA_IN()                                      \
    {                                                     \
        gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT); \
    } // PC6输入模式
#define I2C_SDA_OUT()                                      \
    {                                                      \
        gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_OUTPUT); \
    } // PC5开漏输出模式

// I2C的IO操作函数
// 带参宏，可以像内联函数一样使用
#define SCL_H gpio_set_level(I2C_SCL_PIN, 1)
#define SCL_L gpio_set_level(I2C_SCL_PIN, 0)
#define SDA_H gpio_set_level(I2C_SDA_PIN, 1)
#define SDA_L gpio_set_level(I2C_SDA_PIN, 0)
#define I2C_Read_SDA gpio_get_level(I2C_SDA_PIN)

uint8_t RcvByte(void);
uint8_t Read_I2C_Byte(uint8_t RegAddr);
void Delay1us(uint8_t uscount);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_Send_Byte(uint8_t ucByte);
void Ack_I2C(bool a);
bool I2C_write_byte(uint8_t sla, uint8_t suba, uint8_t *s, uint8_t no);
bool I2C_read_byte(uint8_t sla, uint8_t suba, uint8_t *s, uint8_t no);
bool Read_I2C_ByteN(uint8_t RegAddr, uint8_t *p_data, uint8_t N);
bool Write_I2C_Byte(uint8_t RegAddr, uint8_t d);
bool Write_I2C_ByteN(uint8_t RegAddr, uint8_t *d, uint8_t N);
void SW_I2C1_Init(void);
void write_addr(uint8_t addr);
#endif
