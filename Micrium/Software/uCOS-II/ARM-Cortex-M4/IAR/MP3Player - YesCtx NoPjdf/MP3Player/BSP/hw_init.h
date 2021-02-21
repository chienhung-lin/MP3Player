/*
    hw_init.h
       
    2021/1 Nick Strathy wrote/arranged it
*/
#ifndef __HW_INIT_H
#define __HW_INIT_H


// Application interface to hardware

void Hw_init(void);
void SetSysTick(uint32_t ticksPerSec);

#endif /* __HW_INIT_H */