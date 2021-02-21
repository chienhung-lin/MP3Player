/*
    bspI2c.h

    Board support for controlling I2C interfaces on NUCLEO-F401RE MCU

    Developed for University of Washington embedded systems programming certificate
    
    2018/12 Nick Strathy wrote/arranged it
*/

#ifndef ___BSPI2C_H
#define ___BSPI2C_H

#include "stm32l4xx_ll_i2c.h"

#define PJDF_I2C1 I2C1 // Address of I2C1 memory mapped register block

void BspI2C1_init(void);
void BspI2c_WaitWithTimeoutReset(uint32_t (*IsActive)(I2C_TypeDef *I2Cx), uint32_t value);
uint8_t I2C_read_ack(I2C_TypeDef* I2Cx);
uint8_t I2C_read_nack(I2C_TypeDef* I2Cx);
void I2C_start(I2C_TypeDef* I2Cx, uint8_t address, uint32_t direction, uint8_t nBytes);
void I2C_stop(I2C_TypeDef* I2Cx);
void I2C_write(I2C_TypeDef* I2Cx, uint8_t data);


#endif
