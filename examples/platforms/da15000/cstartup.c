/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file     cstartup.c
 * cstartup for Dialog da15100.
 * Called by __BOOT_STARTUP to initialize the processor on boot.
 * The caller is aliased in tool.h to a compiler dependant value:
 *   IAR: __low_level_init
 *   GCC: __gcc_program_start
 *
 * For Cortex-M cores, called from cpu/arm/cortex-m/boot/cstartup.c.
 */

#include "tool.h"
#include "cpu.h"
#include "global_io.h"

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

