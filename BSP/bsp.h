/*
    bsp.h
    Catch-all include file of board support interfaces for inclusion by applications

    Developed for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy wrote/arranged it
*/

#ifndef __BSP_H
#define __BSP_H

// Add interfaces to the specific hardware for use by application code
#include <stdio.h>
#include <string.h>

#include <os_cpu.h>
#include <os_cfg.h>
#include <app_cfg.h>
#include <ucos_ii.h>

#include "discoveryboard.h"
#include "hw_init.h"
#include "bspI2c.h"
#include "bspLcd.h"
#include "bspLed.h"
#include "bspMp3.h"
#include "bspSD.h"
#include "bspSpi.h"
#include "bspUart.h"
#include "print.h"
#include "pjdf.h"

//get external reference to the print buffer
PRINT_BUFFER();

void SetLED(BOOLEAN On);

#endif /* __BSP_H */