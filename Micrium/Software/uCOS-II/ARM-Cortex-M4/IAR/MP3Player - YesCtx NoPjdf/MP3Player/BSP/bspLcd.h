/*
    bspLcd.h

    Board support for controlling the ILI9341 LCD on the Adafruit '2.8" TFT LCD w/Cap Touch' via NUCLEO-F401RE MCU

    Developed for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy wrote/arranged it
*/

#include "stm32l4xx.h"

#ifndef __BSPLCD_H
#define __BSPLCD_H

#define LCD_ILI9341_CS_GPIO               GPIOA
#define LCD_ILI9341_CS_GPIO_Pin           LL_GPIO_PIN_2

#define LCD_ILI9341_DC_GPIO               GPIOA
#define LCD_ILI9341_DC_GPIO_Pin           LL_GPIO_PIN_15


#define LCD_ILI9341_CS_ASSERT()        LL_GPIO_ResetOutputPin(LCD_ILI9341_CS_GPIO, LCD_ILI9341_CS_GPIO_Pin);
#define LCD_ILI9341_CS_DEASSERT()      LL_GPIO_SetOutputPin(LCD_ILI9341_CS_GPIO, LCD_ILI9341_CS_GPIO_Pin);

#define LCD_ILI9341_DC_LOW()        LL_GPIO_ResetOutputPin(LCD_ILI9341_DC_GPIO, LCD_ILI9341_DC_GPIO_Pin);
#define LCD_ILI9341_DC_HIGH()       LL_GPIO_SetOutputPin(LCD_ILI9341_DC_GPIO, LCD_ILI9341_DC_GPIO_Pin);

#define LCD_SPI_DEVICE_ID  PJDF_DEVICE_ID_SPI1

#define LCD_SPI_DATARATE  LL_SPI_BAUDRATEPRESCALER_DIV2  // Tune to find optimal value LCD controller will work with. OK with 16MHz and 80MHzHCLK

void BspLcdInitILI9341();



#endif