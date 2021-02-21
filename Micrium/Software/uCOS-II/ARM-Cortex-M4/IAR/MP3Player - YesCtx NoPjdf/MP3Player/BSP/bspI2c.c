/*
    bspI2c.c

    Board support for controlling I2C interfaces

    Adapted for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy adapted it
    2020/8 Nick Strathy adapted it for ST32L475VGT6
  
*/

#include "bsp.h"

#define MAX_TIMEOUT_ITERATIONS  10000

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void BspI2C1_init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  LL_I2C_InitTypeDef I2C_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  /**I2C1 GPIO Configuration
  PB8   ------> I2C1_SCL
  PB9   ------> I2C1_SDA
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8|LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  /** I2C Initialization
  */
  LL_I2C_EnableAutoEndMode(I2C1);
  LL_I2C_DisableOwnAddress2(I2C1);
  LL_I2C_DisableGeneralCall(I2C1);
  LL_I2C_EnableClockStretching(I2C1);
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x00303D5B;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 0;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

// Perform a wait with timeout on the given I2C1 flag. If timeout occurs, reset I2C1.
// IsActive is a pointer to a LL_I2C_IsActiveFlag_X() function.
// value is 0 or 1 and represents the desired active state to wait for. 0 means inactive, 1 means active.
void BspI2c_WaitWithTimeoutReset(uint32_t (*IsActive)(I2C_TypeDef *I2Cx), uint32_t value) {
  int32_t timeoutCount = MAX_TIMEOUT_ITERATIONS;
  
  if (value) {
    while (!(*IsActive)(I2C1) && --timeoutCount);
  } else {
    while ((*IsActive)(I2C1) && --timeoutCount);
  }

  if (timeoutCount <= 0) 
    BspI2C1_init();
}



/* This function issues a start condition and 
 * transmits the slave address + R/W bit
 * 
 * Parameters:
 * 		I2Cx --> the I2C peripheral e.g. I2C1
 * 		address --> the 7 bit slave address
 * 		direction --> the transmission direction can be:
 * 						LL_I2C_GENERATE_START_WRITE for Master transmitter mode
 * 						LL_I2C_GENERATE_START_READ for Master receiver mode
 */
void I2C_start(I2C_TypeDef* I2Cx, uint8_t address, uint32_t direction, uint8_t nBytes){
  BspI2c_WaitWithTimeoutReset(LL_I2C_IsActiveFlag_BUSY, 0);
  LL_I2C_HandleTransfer(I2Cx, address, LL_I2C_ADDRSLAVE_7BIT, (uint32_t)nBytes, LL_I2C_MODE_AUTOEND, direction);  
}

/* This function transmits one byte to the slave device
 * Parameters:
 *		I2Cx --> the I2C peripheral e.g. I2C1 
 *		data --> the data byte to be transmitted
 */
void I2C_write(I2C_TypeDef* I2Cx, uint8_t data)
{
  // Wait for transmit reg to be empty
  BspI2c_WaitWithTimeoutReset(LL_I2C_IsActiveFlag_TXE, 1);
  LL_I2C_TransmitData8(I2Cx, data);
  BspI2c_WaitWithTimeoutReset(LL_I2C_IsActiveFlag_TXE, 1);
}

/* This function reads one byte from the slave device.
 * I2C_start must be called first to set up the read. If the NBYTE register 
 * of the specified I2C device becomes 0, a NACK is sent to the slave after
 * the read, otherwise, ACK is sent.
 */
uint8_t I2C_read_ack(I2C_TypeDef* I2Cx){
        LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);
	BspI2c_WaitWithTimeoutReset(LL_I2C_IsActiveFlag_RXNE, 1);
	// read data from I2C data register and return data byte
	uint8_t data = LL_I2C_ReceiveData8(I2Cx);
	return data;
}

/* This function reads one byte from the slave device
 * and doesn't acknowledge the received data 
 * after that a STOP condition is transmitted
 */
uint8_t I2C_read_nack(I2C_TypeDef* I2Cx){
	// disable acknowledge of received data
	// nack also generates stop condition after last byte received
	// see reference manual for more info
        LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
	// wait until one byte has been received
	BspI2c_WaitWithTimeoutReset(LL_I2C_IsActiveFlag_RXNE, 1);
	// read data from I2C data register and return data byte
	uint8_t data = LL_I2C_ReceiveData8(I2Cx);
	return data;
}

/* This function issues a stop condition and therefore
 * releases the bus
 */
void I2C_stop(I2C_TypeDef* I2Cx){
  LL_I2C_GenerateStopCondition(I2Cx);
  BspI2c_WaitWithTimeoutReset(LL_I2C_IsActiveFlag_STOP, 1);
}

