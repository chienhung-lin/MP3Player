/*
    bspSD.h

    Board support for controlling the SD on the Adafruit shields via NUCLEO-F401RE MCU

    Developed for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy wrote/arranged it
*/

#include "stm32l4xx.h"

#ifndef __BSPSD_H
#define __BSPSD_H

#define SD_ADAFRUIT_CS_GPIO               GPIOA
#define SD_ADAFRUIT_CS_GPIO_Pin           LL_GPIO_PIN_3


#define SD_ADAFRUIT_CS_ASSERT()        LL_GPIO_ResetOutputPin(SD_ADAFRUIT_CS_GPIO, SD_ADAFRUIT_CS_GPIO_Pin);
#define SD_ADAFRUIT_CS_DEASSERT()      LL_GPIO_SetOutputPin(SD_ADAFRUIT_CS_GPIO, SD_ADAFRUIT_CS_GPIO_Pin);

#define SD_ADAFRUIT_DC_LOW()        LL_GPIO_ResetOutputPin(SD_ADAFRUIT_DC_GPIO, SD_ADAFRUIT_DC_GPIO_Pin);
#define SD_ADAFRUIT_DC_HIGH()       LL_GPIO_SetOutputPin(SD_ADAFRUIT_DC_GPIO, SD_ADAFRUIT_DC_GPIO_Pin);

#define SD_SPI_DEVICE_ID  PJDF_DEVICE_ID_SPI1

//#define SD_SPI_DATARATE  LL_SPI_BAUDRATEPRESCALER_DIV2  // Tune to find optimal value SD controller will work with. OK with 16mhz HCLK
#define SD_SPI_DATARATE  LL_SPI_BAUDRATEPRESCALER_DIV4  // Tune to find optimal value SD controller will work with. OK with 80MHz HCLK

void BspSDInitAdafruit();

#endif