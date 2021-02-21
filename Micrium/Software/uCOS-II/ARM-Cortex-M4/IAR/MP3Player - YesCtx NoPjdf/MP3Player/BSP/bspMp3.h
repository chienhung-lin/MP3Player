/*
    bspMp3.h

    Board support for controlling Adafruit Music Maker VS1053 MP3 decoder shield via NUCLEO-F401RE MCU

    Developed for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy wrote/arranged it
*/

#include "stm32l4xx.h"

#ifndef __BSPMP3_H
#define __BSPMP3_H

#define MP3_VS1053_MCS_GPIO               GPIOA
#define MP3_VS1053_MCS_GPIO_Pin           LL_GPIO_PIN_4

#define MP3_VS1053_DCS_GPIO               GPIOB
#define MP3_VS1053_DCS_GPIO_Pin           LL_GPIO_PIN_1

#define MP3_VS1053_DREQ_GPIO               GPIOB
#define MP3_VS1053_DREQ_GPIO_Pin           LL_GPIO_PIN_0

#define MP3_VS1053_MCS_ASSERT()       LL_GPIO_ResetOutputPin(MP3_VS1053_MCS_GPIO, MP3_VS1053_MCS_GPIO_Pin);
#define MP3_VS1053_MCS_DEASSERT()      LL_GPIO_SetOutputPin(MP3_VS1053_MCS_GPIO, MP3_VS1053_MCS_GPIO_Pin);

#define MP3_VS1053_DCS_ASSERT()       LL_GPIO_ResetOutputPin(MP3_VS1053_DCS_GPIO, MP3_VS1053_DCS_GPIO_Pin);
#define MP3_VS1053_DCS_DEASSERT()      LL_GPIO_SetOutputPin(MP3_VS1053_DCS_GPIO, MP3_VS1053_DCS_GPIO_Pin);


#define MP3_DECODER_BUF_SIZE       32    // number of bytes to stream at one time to the decoder

#define MP3_SPI_DEVICE_ID  PJDF_DEVICE_ID_SPI1

#define MP3_SPI_DATARATE LL_SPI_BAUDRATEPRESCALER_DIV8  // Tune to find optimal value MP3 decoder will work with. Works with 16MHz HCLK
//#define MP3_SPI_DATARATE LL_SPI_BAUDRATEPRESCALER_DIV32  // Tune to find optimal value MP3 decoder will work with. Works with 80MHz HCLK

// some command strings to send to the VS1053 MP3 decoder:
extern const INT8U BspMp3SineWave[];
extern const INT8U BspMp3Deact[];
extern const INT8U BspMp3TestMode[];
extern const INT8U BspMp3PlayMode[];
extern const INT8U BspMp3SoftReset[];
extern const INT8U BspMp3SetClockF[];
extern const INT8U BspMp3SetVol1010[];
extern const INT8U BspMp3SetVol6060[];
extern const INT8U BspMp3ReadVol[];

// Lengths of the above commands
extern const INT8U BspMp3SineWaveLen;
extern const INT8U BspMp3DeactLen;
extern const INT8U BspMp3TestModeLen;
extern const INT8U BspMp3PlayModeLen;
extern const INT8U BspMp3SoftResetLen;
extern const INT8U BspMp3SetClockFLen;
extern const INT8U BspMp3SetVol1010Len;
extern const INT8U BspMp3SetVol6060Len;
extern const INT8U BspMp3ReadVolLen;


void BspMp3InitVS1053();

#endif