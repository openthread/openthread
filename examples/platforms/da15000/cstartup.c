/**
 * cstartup for Dialog da15100.
 *
 * @file     cstartup.c
 * @author   Martin Turon
 * @date     May 15, 2015
 *
 * Copyright (c) 2015 Nest Labs, Inc.  All Rights Reserved.
 */

#include "tool.h"
#include "cpu.h"
#include "global_io.h"




/**
 * Called by __BOOT_STARTUP to initialize the processor on boot.
 * The caller is aliased in tool.h to a compiler dependant value:
 *   IAR: __low_level_init
 *   GCC: __gcc_program_start
 *
 * For Cortex-M cores, called from cpu/arm/cortex-m/boot/cstartup.c.
 */
void __cpu_startup(void)
{
    // SetWord16(CLK_AMBA_REG, 0x00);              // set clocks (hclk and pclk ) 16MHz
    // SetWord16(SET_FREEZE_REG,FRZ_WDOG);         // stop watchdog
    // SetBits16(SYS_CTRL_REG,PAD_LATCH_EN,1);     // enable pads
    ENABLE_DEBUGGER;                               // enable debugger
    // SetBits16(PMU_CTRL_REG, PERIPH_SLEEP,0);    // exit peripheral power down
    // SetBits16(PMU_CTRL_REG, RETAIN_RAM, dg_configMEM_RETENTION_MODE);
    // SetBits16(AON_SPARE_REG, EN_BATSYS_RET, 1); // Power GPIOs during sleep
}

