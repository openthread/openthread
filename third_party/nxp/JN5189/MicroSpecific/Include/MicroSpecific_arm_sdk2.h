/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef  MICRO_SPECIFIC_INCLUDED
#define  MICRO_SPECIFIC_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "JN5189.h"

extern void (*isr_handlers[])(void);

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/** @{ Defined system call numbers */
#define SYSCALL_SEMIHOSTING   						0xAB

#define SEMIHOSTING_WRITE0      					0x04
#define SEMIHOSTING_READC       0x07
/** @} */

#define MICRO_INTERRUPT_EXCEPTION_OFFSET            16

// number of bits are defined by the hardware
#define MICRO_INTERRUPT_NUMBER_OF_PRIORITY_BITS     __NVIC_PRIO_BITS

// this macro depends on the setting of the priority group in the NVIC, setting G=3 in this case
#define MICRO_INTERRUPT_MAX_PRIORITY                ((1 << MICRO_INTERRUPT_NUMBER_OF_PRIORITY_BITS) - 1)
// half way
#define MICRO_INTERRUPT_MID_PRIORITY                (MICRO_INTERRUPT_MAX_PRIORITY/2)

// Priority levels in the arm are higher for lower values - B-Semi chips were the other way around
#define MICRO_INTERRUPT_ELEVATED_PRIORITY           (11)
#define MICRO_INTERRUPT_MEDIUM_PRIORITY             (12)

// priority/sub priority register 8-bits wide
// read/write priority
#define MICRO_INTERRUPT_WRITE_PRIORITY_VALUE(W)     ((W) << (8 - MICRO_INTERRUPT_NUMBER_OF_PRIORITY_BITS))
#define MICRO_INTERRUPT_READ_PRIORITY_VALUE(R)      ((R) >> (8 - MICRO_INTERRUPT_NUMBER_OF_PRIORITY_BITS))
// read/write sub-priority
#define MICRO_INTERRUPT_SUBPRIORITY_MASK            ((1  << (8 - MICRO_INTERRUPT_NUMBER_OF_PRIORITY_BITS)) -1)
#define MICRO_INTERRUPT_SUBPRIORITY_VALUE(S)        ((S) & (MICRO_INTERRUPT_SUBPRIORITY_MASK))

#define asm					    __asm__

#if 0 /* CJGTODO */
// JN5172 interrupt mappings

#define MICRO_ISR_NUM_TMR2                          0
#define MICRO_ISR_NUM_TMR3                          1
#define MICRO_ISR_NUM_TMR4                          2
#define MICRO_ISR_NUM_TMR5                          3
#define MICRO_ISR_NUM_TMR6                          4
#define MICRO_ISR_NUM_TMR7                          5
#define MICRO_ISR_NUM_SYSCTRL                       6
#define MICRO_ISR_NUM_BBC                           7       // MAC
#define MICRO_ISR_NUM_AES                           8
#define MICRO_ISR_NUM_PHY                           9
#define MICRO_ISR_NUM_UART0                         10
#define MICRO_ISR_NUM_UART1                         11
#define MICRO_ISR_NUM_SPIS                          12
#define MICRO_ISR_NUM_SPIM                          13
#define MICRO_ISR_NUM_I2C                           14
#define MICRO_ISR_NUM_TMR0                          15
#define MICRO_ISR_NUM_TMR1                          16
#define MICRO_ISR_NUM_TMR8                          17     // TIMER_ADC
#define MICRO_ISR_NUM_ANPER                         18
#define MICRO_ISR_NUM_WDOG                          19
// number of interrupts
#define MICRO_ISR_NUM                               20
// mask
#define MICRO_ISR_EN_MASK                           ((1 << MICRO_ISR_NUM) - 1)

/* NOTE: Following are based on values above. Not all are defined for all
   chips. So if an error is raised by the MICRO_ISR_MASK_* macros below, look
   to see if the corresponding MICRO_ISR_NUM_* macro is defined above. If it
   isn't, that peripheral is not present on the defined chip */
#define MICRO_ISR_MASK_SYSCTRL  (1 << MICRO_ISR_NUM_SYSCTRL)
#define MICRO_ISR_MASK_BBC      (1 << MICRO_ISR_NUM_BBC)
#define MICRO_ISR_MASK_AES      (1 << MICRO_ISR_NUM_AES)
#define MICRO_ISR_MASK_PHY      (1 << MICRO_ISR_NUM_PHY)
#define MICRO_ISR_MASK_UART0    (1 << MICRO_ISR_NUM_UART0)
#define MICRO_ISR_MASK_UART1    (1 << MICRO_ISR_NUM_UART1)
#define MICRO_ISR_MASK_TMR0     (1 << MICRO_ISR_NUM_TMR0)
#define MICRO_ISR_MASK_TMR1     (1 << MICRO_ISR_NUM_TMR1)
#define MICRO_ISR_MASK_TMR2     (1 << MICRO_ISR_NUM_TMR2)
#define MICRO_ISR_MASK_TMR3     (1 << MICRO_ISR_NUM_TMR3)
#define MICRO_ISR_MASK_TMR4     (1 << MICRO_ISR_NUM_TMR4)
#define MICRO_ISR_MASK_I2C      (1 << MICRO_ISR_NUM_I2C)
#define MICRO_ISR_MASK_SPIM     (1 << MICRO_ISR_NUM_SPIM)
#define MICRO_ISR_MASK_SPIS     (1 << MICRO_ISR_NUM_SPIS)
#define MICRO_ISR_MASK_INTPER   (1 << MICRO_ISR_NUM_INTPER)
#define MICRO_ISR_MASK_ANPER    (1 << MICRO_ISR_NUM_ANPER)
#define MICRO_ISR_MASK_WDOG     (1 << MICRO_ISR_NUM_WDOG)
#endif

/* Handy macros for controlling interrupts, PIC, interrupt levels */

#define MICRO_ENABLE_INTERRUPTS()                                           \
        asm volatile ("CPSIE I;" : : );

#define MICRO_DISABLE_INTERRUPTS()                                          \
        asm volatile ("CPSID I;" : : );

extern void vAHI_InterruptSetPriority(uint32 u32Mask, uint8 u8Level);
extern uint8 u8AHI_InterruptGetPriority(uint32 u32InterruptNumber);
extern void vAHI_InterruptDisable(uint32 u32EnableMask);
extern void vAHI_TickTimerIntEnable(bool_t bIntEnable);
extern void vAHI_InterruptSetActivePriorityLevel(uint8 u8Level);
extern uint8 u8AHI_InterruptReadActivePriorityLevel(void);

#define MICRO_ENABLE_TICK_TIMER_INTERRUPT();                                \
        vAHI_TickTimerIntEnable(TRUE);

// use same value as Jennic/BA devices
#define MICRO_SET_PIC_ENABLE(A)                                             \
    vAHI_InterruptSetPriority(A, 8);

#define MICRO_CLEAR_PIC_ENABLE(A)                                           \
    vAHI_InterruptDisable(A)

#define MICRO_SET_PIC_PRIORITY_LEVEL(A,B)                                   \
    vAHI_InterruptSetPriority(A, B);

#define MICRO_GET_PIC_PRIORITY_LEVEL(A)                                     \
    u8AHI_InterruptGetPriority(A);

/* MRS Move to register from status */
/* MSR Move to status register */
#define MICRO_SET_ACTIVE_INT_LEVEL(A)                                       \
   ({                                                                       \
       register uint32 __u32interruptLevelStore  = A;                       \
        asm volatile ("MSR BASEPRI, %[intlevelstore];"                      \
            :                                                               \
            :[intlevelstore] "r"(__u32interruptLevelStore)                  \
            );                                                              \
   })

#define MICRO_SET_ACTIVE_INT_LEVEL_MAX(A)                                   \
   ({                                                                       \
       register uint32 __u32interruptLevelStore  = A;                       \
        asm volatile ("MSR BASEPRI_MAX, %[intlevelstore];"                  \
            :                                                               \
            :[intlevelstore] "r"(__u32interruptLevelStore)                  \
            );                                                              \
   })

#define MICRO_GET_ACTIVE_INT_LEVEL()                                        \
    ({                                                                      \
        register uint32 __u32interruptActiveLevel;                          \
        asm volatile ("MRS %[activelevelstore], BASEPRI;"                   \
            :[activelevelstore] "=r"(__u32interruptActiveLevel)             \
            : );                                                            \
        __u32interruptActiveLevel;                                          \
    })

#define MICRO_SET_PRIMASK_LEVEL(A)                                          \
   ({                                                                       \
        register uint32 __u32primaskLevelStore  = A;                        \
        asm volatile ("MSR PRIMASK, %[primasklevelstore];"                  \
            :                                                               \
            :[primasklevelstore] "r"(__u32primaskLevelStore)                \
            );                                                              \
   })

#define MICRO_GET_PRIMASK_LEVEL()                                           \
   ({                                                                       \
        register uint32 __u32primaskLevelStore;                             \
        asm volatile ("MRS %[primasklevelstore], PRIMASK;"                  \
            :[primasklevelstore] "=r"(__u32primaskLevelStore)               \
            :                                                               \
            );                                                              \
        __u32primaskLevelStore;                                             \
   })

// read back PRIMASK status into u32Store variable then disable the interrupts
#define MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store)                         \
   ({                                                                       \
        asm volatile ("MRS %[primasklevelstore], PRIMASK;"                  \
                :[primasklevelstore] "=r"(u32Store)                         \
                :                                                           \
                );                                                          \
        asm volatile ("CPSID I;" : : );                                     \
   })

#define MICRO_RESTORE_INTERRUPTS(u32Store)                                  \
   ({                                                                       \
        asm volatile ("MSR PRIMASK, %[primasklevelstore];"                  \
                :                                                           \
                :[primasklevelstore] "r"(u32Store)                          \
                );                                                          \
   })

// using AAPCS the parameter (the stack frame) will map to r0
#define MICRO_GET_EXCEPTION_STACK_FRAME()                                   \
   {                                                                        \
        asm volatile("MRS R0, MSP");                                        \
   }

// macro using privilege/non�privilege model
#define MICRO_GET_EXCEPTION_STACK_FRAME_PNPM()                              \
   ({                                                                       \
        asm volatile("TST LR, #4");                                         \
        asm volatile("ITE EQ");                                             \
        asm volatile("MRSEQ R0, MSP");                                      \
        asm volatile("MRSNE R0, PSP");                                      \
    })

#define FF1(__input)                                                        \
     ({ register uint32 __reverse, __result, __return;                      \
        asm volatile ("RBIT %[reverse], %[input];"                          \
            : [reverse] "=r" (__reverse)                                    \
            : [input]  "r"  (__input)                                       \
                     );                                                     \
        asm volatile ("CLZ %[result], %[reverse];"                          \
            : [result] "=r" (__result)                                      \
            : [reverse]  "r"  (__reverse)                                   \
            );                                                              \
        __return = ((__result == 32) ? 0 : __result+1);                  \
        __return; })

#if 0 /* In chip_jn518x\inc/core_cmInstr.h */
/* Reverse byte order */
#define __REV(A)                                        \
     ({                                                 \
        register uint32 __reverse, __input = A;         \
        asm volatile ("REV %[reverse], %[input];"       \
            : [reverse] "=r" (__reverse)                \
            : [input]  "r"  (__input)                   \
                     );                                 \
        __reverse;                                      \
        })
#endif

#define MICRO_GET_LX()                                                      \
   ({                                                                       \
        register uint32 __u32lxRegister;                                    \
        asm volatile ("MOV %[lxRegister], R14;"                             \
            :[lxRegister] "=r"(__u32lxRegister)                             \
            :                                                               \
            );                                                              \
        __u32lxRegister;                                                    \
   })

#define MICRO_GET_STACK_LEVEL()                                             \
   ({                                                                       \
        register uint32 __u32stackRegister;                                 \
        asm volatile ("MOV %[stackRegister], SP;"                           \
            :[stackRegister] "=r"(__u32stackRegister)                       \
            :                                                               \
            );                                                              \
        __u32stackRegister;                                                 \
   })

/* Interrupt Handler registration - only useful if you're putting the handlers
 * in RAM */

/* Location of isr_handlers is no longer at a known location, but we can link
   to it directly instead */
#define MICRO_SET_INT_HANDLER(INT, FUNC);                                   \
    isr_handlers[(MICRO_INTERRUPT_EXCEPTION_OFFSET + INT)] = (void *)(FUNC);

#define MICRO_GET_INT_HANDLER(INT)                                          \
    (isr_handlers[(MICRO_INTERRUPT_EXCEPTION_OFFSET + INT)])

/* Nested interrupt control */
#define MICRO_INT_STORAGE         tsMicroIntStorage sIntStorage
#define MICRO_INT_ENABLE_ONLY(A)  vMicroIntEnableOnly(&sIntStorage, A)
#define MICRO_INT_RESTORE_STATE() vMicroIntRestoreState(&sIntStorage)

/* Exception Handlers */
#define MICRO_ESR_NUM_RESETISR                    1
#define MICRO_ESR_NUM_NMI                         2
#define MICRO_ESR_NUM_HARDFAULT                   3
#define MICRO_ESR_NUM_MEMMANAGE                   4
#define MICRO_ESR_NUM_BUSFAULT                    5
#define MICRO_ESR_NUM_USGFAULT                    6
// 4 reserved handlers here
#define MICRO_ESR_NUM_SVCALL                      11
#define MICRO_ESR_NUM_DEBUGMON                    12
// 1 reserved handler here
#define MICRO_ESR_NUM_PENDSV                      14
#define MICRO_ESR_NUM_SYSTICK                     15

/* Location of exception_handlers is no longer at a known location, but we can link
   to it directly instead - only useful if you're putting the handlers
 * in RAM */
#define MICRO_SET_EXCEPTION_HANDLER(EXCEPTION, FUNC)                        \
    isr_handlers[EXCEPTION] = (void *)(FUNC);

#define MICRO_GET_EXCEPTION_HANDLER(INT)                                    \
    (isr_handlers[EXCEPTION])

/* NOP instruction */
#define MICRO_NOP()                                                         \
    {                                                                       \
        asm volatile ("nop;");                                              \
    }

/* TRAP instruction */
#define MICRO_TRAP()                                                        \
    {                                                                       \
        asm volatile("BKPT 0;");                                            \
    }

#define MICRO_JUMP_TO_ADDRESS(ADDRESS)                                      \
   ({                                                                       \
       register uint32 __u32programAddressStore = ADDRESS | 0x1;            \
       asm volatile ("BLX %[programAddressStore];"                           \
           :                                                                \
           :[programAddressStore] "r"(__u32programAddressStore));           \
    })

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/* Nested interrupt control */
typedef struct
{
    uint8 u8Level;
} tsMicroIntStorage;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vAHI_InitialiseInterruptController(uint32 *pu32InterruptVectorTable);

/* Nested interrupt control */
PUBLIC void vMicroIntSetGlobalEnable(uint32 u32EnableMask);
PUBLIC void vMicroIntEnableOnly(tsMicroIntStorage *, uint32 u32EnableMask);
PUBLIC void vMicroIntRestoreState(tsMicroIntStorage *);
/* Default Exception Handler */
PUBLIC void vIntDefaultHandler(void);

PUBLIC void __attribute__((noinline)) vMicroSyscall(volatile uint32 u32SysCallNumber, ...);
PUBLIC void __attribute__((noinline)) vMicroSemihost(volatile uint32 u32SemihostNumber, ...);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* MICRO_SPECIFIC_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
