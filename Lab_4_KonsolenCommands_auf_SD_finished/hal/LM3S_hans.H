/******************************************************************************
 * @file:    LM3Sxxx.h
 * @purpose: Header file for Luminary Micro LM3Sxxx microcontrollers 
 *
 * @version: V1.1
 * @date:    28. June 2011
 *----------------------------------------------------------------------------
 *
 * Copyright (C) 2008-2011 ARM Limited. All rights reserved.
 *
 * ARM Limited (ARM) is supplying this software for use with Cortex-M0 
 * processor based microcontrollers.  This file can be freely distributed 
 * within development tools that are supporting such ARM based processors. 
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/

#ifndef __LM3SXXX_H__
#define __LM3SXXX_H__

/*----------------------------------------------------------------------------
  Note:
        set INC path to "Stellarisware library folder"
                        "./Keil/ARM/RV31/LIB/Luminary"
 *----------------------------------------------------------------------------*/ 

#include "inc/hw_adc.h"
//#include "inc/hw_comp.h"
#include "inc/hw_flash.h"
#include "inc/hw_gpio.h"
#include "inc/hw_i2c.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
//#include "inc/hw_pwm.h"
//#include "inc/hw_qei.h"
#include "inc/hw_ssi.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
//#include "inc/hw_watchdog.h"

#include "driverlib/adc.h"
//#include "driverlib/comp.h"
#include "driverlib/cpu.h"
#include "driverlib/debug.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
//#include "driverlib/mpu.h"
#include "driverlib/pin_map.h"
//#include "driverlib/pwm.h"
//#include "driverlib/qei.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
//#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
//#include "driverlib/watchdog.h"

#endif // __LM3SXXX_H__
