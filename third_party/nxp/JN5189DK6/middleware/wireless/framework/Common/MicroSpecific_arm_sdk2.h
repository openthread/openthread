/*****************************************************************************
 *
 * MODULE:              Definitions specific to a particular processor
 *
 * DESCRIPTION:
 * Definitions for a specific processor, i.e. functions that can only be
 * resolved by op codes
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2012, 2019. All rights reserved
 *
 ***************************************************************************/

#ifndef  MICRO_SPECIFIC_INCLUDED
#define  MICRO_SPECIFIC_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "fsl_device_registers.h"

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

/* Actual macros are instantiated in the respective CMSIS files */
#define MICRO_ENABLE_INTERRUPTS()             __enable_irq()

#define MICRO_DISABLE_INTERRUPTS()            __disable_irq()

#define MICRO_GET_PRIMASK_LEVEL()            __get_PRIMASK()
/* Former implementation : deprecated since CMSIS alternative exists
 * #define MICRO_GET_PRIMASK_LEVEL()                                 \
 *  ({                                                               \
 *       register uint32 __u32primaskLevelStore;                     \
 *       asm volatile ("MRS %[primasklevelstore], PRIMASK;"          \
 *           :[primasklevelstore] "=r"(__u32primaskLevelStore)       \
 *           :                                                       \
 *           );                                                      \
 *       __u32primaskLevelStore;                                     \
 *  })
 */
#define MICRO_SET_PRIMASK_LEVEL(A)           __set_PRIMASK(A)
/* Former implementation : deprecated since CMSIS alternative exists
 * #define MICRO_SET_PRIMASK_LEVEL(A)                                \
 *  ({                                                               \
 *       register uint32 __u32primaskLevelStore  = A;                \
 *       asm volatile ("MSR PRIMASK, %[primasklevelstore];"          \
 *           :                                                       \
 *           :[primasklevelstore] "r"(__u32primaskLevelStore)        \
 *           );                                                      \
 *  })
 */
#define MICRO_GET_ACTIVE_INT_LEVEL()         __get_BASEPRI()
/* Former implementation : deprecated since CMSIS alternative exists
 * #define MICRO_GET_ACTIVE_INT_LEVEL()                              \
 *   ({                                                              \
 *       register uint32 __u32interruptActiveLevel;                  \
 *       asm volatile ("MRS %[activelevelstore], BASEPRI;"           \
 *           :[activelevelstore] "=r"(__u32interruptActiveLevel)     \
 *           : );                                                    \
 *       __u32interruptActiveLevel;                                  \
 *   })
 */
#define MICRO_SET_ACTIVE_INT_LEVEL_MAX(A)    __set_BASEPRI_MAX(A)
/* Former implementation : deprecated since CMSIS alternative exists
 * #define MICRO_SET_ACTIVE_INT_LEVEL_MAX(A)                         \
 *  ({                                                               \
 *     register uint32 __u32interruptLevelStore  = A;                \
 *       asm volatile ("MSR BASEPRI_MAX, %[intlevelstore];"          \
 *           :                                                       \
 *           :[intlevelstore] "r"(__u32interruptLevelStore)          \
 *           );                                                       \
 *  })
 */
#define MICRO_SET_ACTIVE_INT_LEVEL(A)        __set_BASEPRI(A)
/* Former implementation : deprecated since CMSIS alternative exists
 * #define MICRO_SET_ACTIVE_INT_LEVEL(A)                             \
 *  ({                                                               \
 *      register uint32 __u32interruptLevelStore  = A;               \
 *       asm volatile ("MSR BASEPRI, %[intlevelstore];"              \
 *           :                                                       \
 *           :[intlevelstore] "r"(__u32interruptLevelStore)          \
 *           );                                                      \
 *  })
 */
/* Read back PRIMASK status into u32Store variable then disable 
 * the interrupts */
#define MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store)                  \
   ({                                                                \
        u32Store = __get_PRIMASK();                                  \
        __disable_irq();                                             \
   })
/* Former implementation : deprecated since combining CMSIS alternative is 
 * possible.
 * #define MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store)               \
 *  ({                                                               \
 *       asm volatile ("MRS %[primasklevelstore], PRIMASK;"          \
 *               :[primasklevelstore] "=r"(u32Store)                 \
 *               :                                                   \
 *               );                                                  \
 *       asm volatile ("CPSID I;" : : );                             \
 *  })
 */
#define MICRO_RESTORE_INTERRUPTS(u32Store)  __set_PRIMASK(u32Store)
/* Former implementation : deprecated since CMSIS equivalent exist.
 * #define MICRO_RESTORE_INTERRUPTS(u32Store)                        \
 *  ({                                                               \
 *       asm volatile ("MSR PRIMASK, %[primasklevelstore];"          \
 *               :                                                   \
 *               :[primasklevelstore] "r"(u32Store)                  \
 *               );                                                  \
 *  })
 */
#define MICRO_GET_EXCEPTION_STACK_FRAME()   __get_MSP()
/* Former implementation : deprecated since CMSIS equivalent exist.
 * using AAPCS the parameter (the stack frame) wouuld map to r0
 * #define MICRO_GET_EXCEPTION_STACK_FRAME()                         \
 *  {                                                                \
 *       asm volatile("MRS R0, MSP");                                \
 *  }
 */

#if defined(__IAR_SYSTEMS_ICC__) || defined(__IAR_SYSTEMS_ICC)
/* Count Trailing Zeroes implementation for IAR :
 * no builtin implementation for IAR, whereas __CLZ exists */
static inline uint32_t __ctz(uint32_t x)
{
    uint32_t res;
    x = __RBIT(x);
    res = __CLZ(x);
    return res;
}
#define _CTZ(x)             __ctz(x)

/* Find First Set : returns 0 if no bit set, otherwise returns the order 
 *  of LSB bit set + 1
 * __ffs(0) return 0, __ffs(1) returns 1, __ffs(0x8000000) returns 31
 */
static inline uint32_t __ffs(uint32_t x) 
{
   register uint32 __u32Reg;
   __u32Reg = _CTZ(x);
   __u32Reg = __u32Reg == 32 ? 0 : __u32Reg + 1U;
   return __u32Reg;
}

#define _FFS(x)            __ffs(x)
#else  /* __IAR_SYSTEMS_ICC__ */
/* GCC builtin */
#define _CTZ(x)             __builtin_ctz(x)

#define _FFS(x)                                     \
({   register uint32_t __u32Reg, __ret;             \
   __u32Reg = _CTZ(x);                              \
   __ret = __u32Reg == 32 ? 0 : __u32Reg + 1U;      \
   __ret;                                           \
})
#endif  /* __IAR_SYSTEMS_ICC__ */

/* Bit Scan Reverse */
#define MICRO_BSR(x) (31 - _CLZ(x))
/* Bit Scan Forward */
#define MICRO_BSF(x) _CTZ(x)

#define MICRO_FFS(x)       _FFS(x)


#define FF1(__input)      MICRO_FFS(__input)

#define MICRO_GET_LX()           __get_LR()
/* Former implementation : deprecated since CMSIS equivalent exist.
 * #define MICRO_GET_LX()                               \
 * ({                                                   \
 *   register uint32 __u32lxRegister;                   \
 *   asm volatile ("MOV %[lxRegister], R14;"            \
 *         :[lxRegister] "=r"(__u32lxRegister)          \
 *           :                                          \
 *           );                                         \
 *       __u32lxRegister;                               \
 * })
 */
#define MICRO_GET_STACK_LEVEL()  __get_SP()
/* Former implementation : deprecated since CMSIS equivalent exist.
 * #define MICRO_GET_STACK_LEVEL()                       \
 *  ({                                                   \
 *       register uint32 __u32stackRegister;             \
 *       asm volatile ("MOV %[stackRegister], SP;"       \
 *           :[stackRegister] "=r"(__u32stackRegister)   \
 *           :                                           \
 *           );                                          \
 *       __u32stackRegister;                             \
 *  })
 */

#define MICRO_TRAP()             __BKPT(0)
/* Former implementation : deprecated since CMSIS equivalent exist.
 * #define MICRO_TRAP()         asm volatile("BKPT 0;")
 */
#define MICRO_NOP()              __NOP()
/* Former implementation : deprecated since CMSIS equivalent exist.
 * #define MICRO_NOP()          asm volatile ("nop;")
 */

/* macro using privilege/non-privilege model:
 * u32reg is the register to return the stack pointer that corresponds to 
 * the mode we are in: either MSP or PSP.
 * Note: this macro has chnaged fir IAR compatibility u32reg is the return
 * register so that R0 is not implicitly the returned value
 */
#define MICRO_GET_EXCEPTION_STACK_FRAME_PNPM(u32reg)              \
     register uint32_t u32reg;                                    \
     asm volatile("TST LR, #4\n\t"                                \
                  "ITE EQ\n\t"                                    \
                  "MRSEQ R0, MSP\n\t"                             \
                  "MRSNE R0, PSP\n\t");                           \
     asm volatile ("MOV %0, R0" : "=r" (u32reg) : /* no input */ ); 

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


/* TRAP instruction */

#define MICRO_JUMP_TO_ADDRESS(ADDRESS)                                      \
   ({                                                                       \
       register uint32 __u32programAddressStore = ADDRESS | 0x1;            \
       asm volatile ("BLX %[programAddressStore];"                          \
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

