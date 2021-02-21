/*
    bspLed.h

    Board support for controlling LEDs

    Developed for University of Washington embedded systems programming certificate
    
    2020/8 Nick Strathy wrote/arranged it for STM32L475
*/

#ifndef __BSPLED_H
#define __BSPLED_H

#define LED2_PIN                           LL_GPIO_PIN_5
#define LED2_GPIO_PORT                     GPIOA
#define LED2_GPIO_CLK_ENABLE()             LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA)

void LedInit();
void SetLED(BOOLEAN On);

#endif /* __BSPLED_H */