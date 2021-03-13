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

//#define MP3_SPI_DATARATE LL_SPI_BAUDRATEPRESCALER_DIV8  // Tune to find optimal value MP3 decoder will work with. Works with 16MHz HCLK
#define MP3_SPI_DATARATE LL_SPI_BAUDRATEPRESCALER_DIV32  // Tune to find optimal value MP3 decoder will work with. Works with 80MHz HCLK

/*
 * opcode
 */
#define WRITE_OPCODE 0x02
#define READ_OPCODE 0x03

/*
 * register map
 */
#define MODE_REG 0x00
#define CLOCKF_REG 0x03
#define VOL_REG 0x0B

/*
 * clock frequency register
 */
#define SM_RESET_OFFSET (2)
#define SM_SDINEW_OFFSET (11)

#define SM_RESET (1 << SM_RESET_OFFSET)
#define SM_SDINEW (1 << SM_SDINEW_OFFSET)

/*
 * clock frequency register
 */
#define CLKF_MULTI_MASK (0x07)
#define CLKF_MULTI_OFFSET (13)
#define CLKF_MULTI(val) ((val & CLKF_MULTI_MASK) << CLKF_MULTI_OFFSET)
#define CLKF_ADD_MASK (0x03)
#define CLKF_ADD_OFFSET (11)
#define CLKF_ADD(val) ((val & CLKF_ADD_MASK) << CLKF_ADD_OFFSET)
#define CLKF_FREQ_MASK (0x03FF)
#define CLKF_FREQ_OFFSET (0)
#define CLKF_FREQ(val) ((val & CLKF_FREQ_MASK) << CLKF_FREQ_OFFSET)

#define MULTI_0_0 (0)
#define MULTI_2_0 (1)
#define MULTI_2_5 (2)
#define MULTI_3_0 (3)
#define MULTI_3_5 (4)
#define MULTI_4_0 (5)
#define MULTI_4_5 (6)
#define MULTI_5_0 (7)

#define ADD_0_0 (0)
#define ADD_1_0 (1)
#define ADD_1_5 (2)
#define ADD_2_0 (3)

#define FREQ_NO_CHANGE (0)

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