/*
    bspSpi.c

    Board support for controlling SPI interfaces on NUCLEO-F401RE MCU

    Developed for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy wrote/arranged it
*/

#include "bsp.h"

// BspSPI1Init
// Initializes the SPI1 memory mapped register block and enables it for use
// as a master SPI device.
void BspSPI1Init()
{
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  /* Configure SPI1 communication */
  LL_SPI_SetBaudRatePrescaler(SPI1, LL_SPI_BAUDRATEPRESCALER_DIV256);
  LL_SPI_SetTransferDirection(SPI1,LL_SPI_FULL_DUPLEX);
  LL_SPI_SetClockPhase(SPI1, LL_SPI_PHASE_1EDGE);
  LL_SPI_SetClockPolarity(SPI1, LL_SPI_POLARITY_LOW);
  LL_SPI_SetTransferBitOrder(SPI1, LL_SPI_MSB_FIRST);
  LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_8BIT);
  LL_SPI_SetNSSMode(SPI1, LL_SPI_NSS_SOFT);
  LL_SPI_SetRxFIFOThreshold(SPI1, LL_SPI_RX_FIFO_TH_QUARTER);
  LL_SPI_SetMode(SPI1, LL_SPI_MODE_MASTER);

    LL_GPIO_InitTypeDef GPIO_InitStruct;
    
    /*-------- Configure SCK, MISO, MOSI --------*/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
     
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /*--------- Configure alternate GPIO functions to SPI1 ------*/
  LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_5, LL_GPIO_AF_5);
  LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_6, LL_GPIO_AF_5);
  LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF_5);

  LL_SPI_Enable(SPI1);
}


// SPI_SendBuffer
// Sends the given data to the given SPI device
void SPI_SendBuffer(SPI_TypeDef *spi, uint8_t *buffer, uint16_t bufLength)
{    
    for (int i = 0; i < bufLength; i++) {
        while(!LL_SPI_IsActiveFlag_TXE(spi)); 
        LL_SPI_TransmitData8(spi, buffer[i]);
        while(!LL_SPI_IsActiveFlag_RXNE(spi));
        LL_SPI_ReceiveData8(spi);
    }
 }

// SPI_GetBuffer
// Sends the given data to the given SPI device.
// buffer: on entry contains the command to retrieve data from the spi device.
//    On exit the buffer is OVERWRITTEN with the data output by the device
//    in response to the command.
void SPI_GetBuffer(SPI_TypeDef *spi, uint8_t *buffer, uint16_t bufLength)
{
    int iread = 0;

    for (int i = 0; i < bufLength; i++) {
        while(!LL_SPI_IsActiveFlag_TXE(spi)); 
        LL_SPI_TransmitData8(spi, buffer[i]);
        while(!LL_SPI_IsActiveFlag_RXNE(spi));
        buffer[iread++] =LL_SPI_ReceiveData8(spi);
    }
 }


// Set a value to control the data rate of the given SPI interface
void SPI_SetDataRate(SPI_TypeDef *spi, uint16_t value)
{
  LL_SPI_SetBaudRatePrescaler(spi, value);
}

