/*
    bspSD.c

    Board support for controlling the SD on the Adafruit shields via NUCLEO-F401RE MCU

    Developed for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy wrote/arranged it
*/

#include "bsp.h"



// Initializes GPIO pins for the SD device.
void BspSDInitAdafruit()
{
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_GPIO_InitTypeDef GPIO_InitStruct;
    
    /*-------- Configure CS ChipSelect Pin --------*/ 
 
    GPIO_InitStruct.Pin = SD_ADAFRUIT_CS_GPIO_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
     
    LL_GPIO_Init(SD_ADAFRUIT_CS_GPIO, &GPIO_InitStruct);
    SD_ADAFRUIT_CS_DEASSERT();
}