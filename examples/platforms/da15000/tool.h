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
 * @file     core/tool.h
 * TOOL abstraction -- common coding convention for multiple toolchains:
 *      GCC, IAR, ARM, SDCC (8051)
 *
 */

#ifndef _TOOL_H_
#define _TOOL_H_

#ifdef __cplusplus
extern "C" {
#endif

// =========== SOME COMMON TYPES ===========

typedef void (*irq_handler_t)(void);

// =========== HW SELECTION : START ===========

#if defined(__GNUC__)
#define __TOOL_VERSION__           __VERSION__
#define __TOOL_BUILD__             __VERSION__
#define __TOOL_FPU__               __VFP_FP__
#define __TOOL_LITTLE_ENDIAN__     __IEEE_LITTLE_ENDIAN
#define __TOOL_INSTRUCTION_SET__   __THUMB_INTERWORK__

#define __ASM                      __asm
#define __RAM                      __attribute__((section(“RAMFUNC”)))
#define __IRQ                      __attribute__((interrupt(“IRQ”)))
#define __FIQ                      __attribute__((interrupt(“FIQ”)))

#define __SWI                      __attribute__((interrupt(“SWI”)))
#define __NAKED                    __attribute__((naked))
#define __NESTED                   __attribute__((nesting))
#define __NO_RETURN                __attribute__((noreturn))
#define __NO_INIT                  __attribute__((section(“no_init”)))

#define _WEAK_                     __attribute__((weak))
#define __WEAK                     __attribute__((weak))
#define __USED                     __attribute__((used))
#define __PACKED                   __attribute__((packed))
#define __NET_ORDER

#define __FINAL                    final

extern void         __gcc_program_start(void);
extern unsigned int __stack_start__[];

#define __BOOT_VECTOR              __attribute__ ((section(".isr_vector")))
#define __BOOT_STACK               &__stack_start__
#define __BOOT_STARTUP             __gcc_program_start
//#define __BOOT_STARTUP             _start

#elif defined(__ICCARM__) || defined(__ICC8051__)

#include "intrinsics.h"

#define __TOOL_VERSION__           __VER__
#define __TOOL_BUILD__             __VER__
#define __TOOL_FPU__               __ARMVFP__
#define __TOOL_LITTLE_ENDIAN__     __LITTLE_ENDIAN__
#define __TOOL_INSTRUCTION_SET__   __CPU_MODE__

#define __ASM                      __asm
#define __RAM                      __ramfunc
#define __IRQ                      __irq
#define __FIQ                      __fiq

#define __SWI                      __swi
#define __NAKED                    __task
#define __NESTED                   __nested
#define __NO_RETURN                __noreturn
#define __NO_INIT                  __no_init

#define _WEAK_                     __weak
#define __WEAK                     __weak
#define __USED                     __root
#define __PACKED                   __packed
#define __NET_ORDER                __big_endian

#define __FINAL                    final

extern unsigned long    __BOOT_STACK_ADDRESS[];
extern void             __iar_program_start(void);

#define __BOOT_VECTOR
#define __BOOT_STACK               __BOOT_STACK_ADDRESS
#define __BOOT_STARTUP             __iar_program_start


#elif defined(__SDCC)

#define __TOOL_VERSION__           __SDCC
#define __TOOL_BUILD__             __SDCC_REVISION
#define __TOOL_FPU__               __SDCC_FLOAT_REENT
#define __TOOL_LITTLE_ENDIAN__     1
#define __TOOL_INSTRUCTION_SET__   __SDCC_mcs51

#define __ASM                      __asm
#define __RAM                      __idata
#define __IRQ                      __interrupt
#define __FIQ                      __critical __interrupt

#define _WEAK_
#define __USED
#define __PACKED
#define __NET_ORDER

#define __BOOT_VECTOR

#define __FINAL



#elif defined(__CC_ARM)

#elif defined(__CODEWARRIOR)

#define __BOOT_VECTOR             __declspec(vectortable)

#else

#error "Error: No valid Toolchain specified"

#endif

// =========== HW SELECTION : END ===========

#ifdef __cplusplus
}
#endif

#endif // _TOOL_H_
