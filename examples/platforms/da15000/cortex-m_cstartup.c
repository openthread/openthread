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
 * @file     cortex-m_cstartup.c
 * ARM Cortex-M generic cstartup.
 *
 * Setup RAM as needed based on GCC linker script sections.
 */

#include <stdint.h>
//#include <tool.h>
#define __NAKED                    __attribute__((naked))
#define __USED                     __attribute__((used))
#define __NO_RETURN                __attribute__((noreturn))
// forward declaration
extern void main(void);
extern void __cpu_startup(void);
extern void __gcc_program_start(void);

extern uint32_t _sdata; // Start of .data region in RAM
extern uint32_t _edata; // End of .data region in RAM
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __init_array_start;
extern uint32_t __init_array_end;
extern uint32_t __rwdata_start__; // End of .text region in FLASH

/********************************************************************/
void cstartup_rwdata()
{

    // Get the addresses for the .data section (initialized data section)
    uint32_t *data_rom = (uint32_t *) &__rwdata_start__;
    uint32_t *data_ram = (uint32_t *) &_sdata;
    uint32_t *data_ram_end = (uint32_t *) &_edata;

    // Copy initialized data from ROM to RAM
    while (data_ram < data_ram_end)
    {
        *data_ram++ = *data_rom++;
    }
}

void cstartup_bss()
{

    /* Clear the zero-initialized data section */
    uint32_t *bss_start = (uint32_t *) &__bss_start__;
    uint32_t *bss_end   = (uint32_t *) &__bss_end__;

    while (bss_start < bss_end)
    {
        *bss_start++ = 0;
    }
}

void cstartup(void)
{
    cstartup_rwdata();
    cstartup_bss();
}

/********************************************************************/

/**
 * ARM Cortex-M Start
 *
 * This function calls all of the needed startup routines and then
 * branches to the main process.
 *
 * This contains the very first instructions that are run on boot.
 */
void __USED __NAKED __NO_RETURN __gcc_program_start(void)
{
    // early platform initialization - configure clocks, etc.
    __cpu_startup();

    // C/C++ runtime initialization (BSS, Data, relocation, etc.)
    cstartup();

    // TBD we should pass parameters here.
    main();

    // No actions to perform after this so wait forever
    while (1) {}
}
