#include <stdarg.h>

#include "bsp.h"
#include "print.h"

#define BUFSIZE 128

#ifdef __cplusplus
extern "C" {
#endif

static char buf[BUFSIZE];

/* Reference: 12.8 Fault handlers, The definitive Guide to ARM Cortex M3&4*/
void HardFault_Handler_C(uint32_t *stack_frame, uint32_t lr_value)
{
    PrintWithBuf(buf, BUFSIZE, "===HardFault!===\n");
    PrintWithBuf(buf, BUFSIZE, "EXC_RETURN: 0x%08X\n", lr_value);
    PrintWithBuf(buf, BUFSIZE, "PC: 0x%08X\n", stack_frame[6]);
    PrintWithBuf(buf, BUFSIZE, "LR: 0x%08X\n", stack_frame[5]);
    PrintWithBuf(buf, BUFSIZE, "xPSR: 0x%08X\n", stack_frame[7]);
    PrintWithBuf(buf, BUFSIZE, "SP: 0x%08X\n", stack_frame);
    PrintWithBuf(buf, BUFSIZE, "R0: 0x%08X\n", stack_frame[0]);
    PrintWithBuf(buf, BUFSIZE, "R1: 0x%08X\n", stack_frame[1]);
    PrintWithBuf(buf, BUFSIZE, "R2: 0x%08X\n", stack_frame[2]);
    PrintWithBuf(buf, BUFSIZE, "R3: 0x%08X\n", stack_frame[3]);
    PrintWithBuf(buf, BUFSIZE, "R12: 0x%08X\n", stack_frame[4]);
    for (;;) ;
}

#ifdef __cplusplus
}   
#endif