/*
    bspLed.c

    Board support for controlling LEDs

    Developed for University of Washington embedded systems programming certificate
    
    2020/8 Nick Strathy wrote/arranged it for STM332L475
*/

#include "bsp.h"

void LedInit()
{
  /* Enable the LED2 Clock */
  LED2_GPIO_CLK_ENABLE();

  /* Configure IO in output push-pull mode to drive external LED2 */
  LL_GPIO_SetPinMode(LED2_GPIO_PORT, LED2_PIN, LL_GPIO_MODE_OUTPUT);
}

void SetLED(BOOLEAN On)
{
    if (On) {
        LL_GPIO_SetOutputPin(GPIO_PORT_LD2, GPIO_PIN_LD2);
      } else {
        /* The high 16 bits of BSRR reset the pin */
        LL_GPIO_ResetOutputPin(GPIO_PORT_LD2, GPIO_PIN_LD2);
     }
}
