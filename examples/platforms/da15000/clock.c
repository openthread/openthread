/**
 * clock init for Dialog da15100.
 *
 * @file     clock.c
 * @author   Martin Turon
 * @date     July 23, 2015
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *   
 */

#include "tool.h"
#include "cpu.h"

#include "hw_cpm.h"
#include "hw_watchdog.h"

volatile uint8_t xtal16_settled = 0;

extern void XTAL16RDY_Handler(void);

/* The highest interrupt priority that can be used by any interrupt service
   routine that makes calls to interrupt safe FreeRTOS API functions.
   DO NOT CALL INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT
   THAT HAS A HIGHER PRIORITY THAN THIS! (higher priorities are lower
   numeric values on an ARM Cortex-M). */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 1

void switch_to_rc16(void)
{
    if (!hw_cpm_check_rc16_status()) {          // RC16 is disabled
        hw_cpm_enable_rc16();                   // Enable RC16
        hw_cpm_short_delay();                   // Wait until it is stable
    }

    // fast --> slow clock switch
    hw_cpm_set_sysclk(SYS_CLK_IS_RC16);         // Set RC16 as sys_clk
    //cm_adjust_otp_access_timings();           // Adjust OTP timings
}


