/**
 * \addtogroup BSP
 * \{
 * \addtogroup SYSTEM
 * \{
 * \addtogroup PLATFORM
 *
 * \brief Platform definitions
 *
 * \{
 */

/**
 *****************************************************************************************
 *
 * @file sdk_defs.h
 *
 * @brief Central include header file with platform definitions.
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
 *
 *****************************************************************************************
 */

#ifndef __SDK_DEFS_H__
#define __SDK_DEFS_H__

#include <stddef.h>

#ifdef __cplusplus
 extern "C" {
#endif

#ifdef __GNUC__
#  define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)
/* assert gcc version is at least 4.9.3 */
#  if GCC_VERSION < 40903
#    error "Please use gcc version 4.9.3 or newer!"
#  endif
#endif

#if (dg_configUSE_AUTO_CHIP_DETECTION == 1)

   /* use a register description that is generic enough for our needs */
#  include "DA14680AE.h"

#else

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#  if dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_E
#       include "DA14680AE.h"
#  else
#       error "Unknown chip stepping for revision A -- check the value of dg_configBLACK_ORCA_IC_STEP"
#  endif
#elif dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_B
#  if dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_B
#       include "DA14680BB.h"
#  else
#       error "Unknown chip stepping for revision B -- check the value of dg_configBLACK_ORCA_IC_STEP"
#  endif
#else
#  error "Unknown chip revision -- check the value of dg_configBLACK_ORCA_IC_REV"
#endif

#endif

#include "core_cm0.h"                               /* Cortex-M0 processor and core peripherals                              */
#include "system_DA14680.h"                         /* DA14680AA System                                                      */

/************************
 * Memory map
 ************************/

/**
 * \brief Remapped device base address.
 */
#define MEMORY_REMAPPED_BASE    0x00000000UL
#define MEMORY_REMAPPED_END     0x04000000UL

/**
 * \brief Remapped device memory size (64MiB).
 */
#define MEMORY_REMAPPED_SIZE    (MEMORY_REMAPPED_END - MEMORY_REMAPPED_BASE)

/**
 * \brief ROM base address.
 */
#define MEMORY_ROM_BASE         0x07F00000UL
#define MEMORY_ROM_END          0x07F40000UL

/**
 * \brief ROM memory size (256KiB).
 */
#define MEMORY_ROM_SIZE         (MEMORY_ROM_END - MEMORY_ROM_BASE)

/**
 * \brief OTP Controller base address.
 */
#define MEMORY_OTPC_BASE        0x07F40000UL
#define MEMORY_OTPC_END         0x07F80000UL

/**
 * \brief OTP memory base address.
 */
#define MEMORY_OTP_BASE         0x07F80000UL
#define MEMORY_OTP_END          0x07FC0000UL

/**
 * \brief OTP memory size (256KiB).
 */
#define MEMORY_OTP_SIZE         (MEMORY_OTP_END - MEMORY_OTP_BASE)

/**
 * \brief SYSTEM RAM base address.
 */
#define MEMORY_SYSRAM_BASE      0x07FC0000UL
#define MEMORY_SYSRAM_END       0x07FE0000UL

/**
 * \brief SYSTEM RAM size (128KiB).
 */
#define MEMORY_SYSRAM_SIZE      (MEMORY_SYSRAM_END - MEMORY_SYSRAM_BASE)

/**
 * \brief CACHE RAM base address.
 */
#define MEMORY_CACHERAM_BASE    0x07FE0000UL
#define MEMORY_CACHERAM_END     0x08000000UL

/**
 * \brief CACHE RAM size (128KiB).
 */
#define MEMORY_CACHERAM_SIZE    (MEMORY_CACHERAM_END - MEMORY_CACHERAM_BASE)

/**
 * \brief QSPI Flash base address.
 */
#define MEMORY_QSPIF_BASE       0x08000000UL
#define MEMORY_QSPIF_END        0x0BF00000UL

/**
 * \brief QSPI Flash memory size (63MiB).
 */
#define MEMORY_QSPIF_SIZE       (MEMORY_QSPIF_END - MEMORY_QSPIF_BASE)

/**
 * \brief QSPI Controller base address.
 */
#define MEMORY_QSPIC_BASE       0x0C000000UL
#define MEMORY_QSPIC_END        0x0C100000UL

/**
 * \brief ECC engine microcode base address.
 */
#define MEMORY_ECC_UCODE_BASE   0x40030000UL

/**
 * \brief TRNG FIFO address
 */
#define MEMORY_TRNG_FIFO        0x40040000UL

#define WITHIN_RANGE(_a, _s, _e)        (((uint32_t)(_a) >= (uint32_t)(_s)) && ((uint32_t)(_a) < (uint32_t)(_e)))

/**
 * \brief Address is in the remapped memory region
 */
#define IS_REMAPPED_ADDRESS(_a)         WITHIN_RANGE((_a), MEMORY_REMAPPED_BASE, MEMORY_REMAPPED_END)

/**
 * \brief Address is in the ROM region
 */
#define IS_ROM_ADDRESS(_a)              WITHIN_RANGE((_a), MEMORY_ROM_BASE, MEMORY_ROM_END)

/**
 * \brief Address is in the OTP memory region
 */
#define IS_OTP_ADDRESS(_a)              WITHIN_RANGE((_a), MEMORY_OTP_BASE, MEMORY_OTP_END)

/**
 * \brief Address is in the OTP Controller memory region
 */
#define IS_OTPC_ADDRESS(_a)             WITHIN_RANGE((_a), MEMORY_OTPC_BASE, MEMORY_OTPC_END)

/**
 * \brief Address is in the SYSTEM RAM region
 */
#define IS_SYSRAM_ADDRESS(_a)           WITHIN_RANGE((_a), MEMORY_SYSRAM_BASE, MEMORY_SYSRAM_END)

/**
 * \brief Address is in the CACHE RAM region
 */
#define IS_CACHERAM_ADDRESS(_a)         WITHIN_RANGE((_a), MEMORY_CACHERAM_BASE, MEMORY_CACHERAM_END)

/**
 * \brief Address is in the QSPI Flash memory region
 */
#define IS_QSPIF_ADDRESS(_a)            WITHIN_RANGE((_a), MEMORY_QSPIF_BASE, MEMORY_QSPIF_END)

/**
 * \brief Address is in the QSPI Controller memory region
 */
#define IS_QSPIC_ADDRESS(_a)            WITHIN_RANGE((_a), MEMORY_QSPIC_BASE, MEMORY_QSPIC_END)

#define _BLACK_ORCA_IC_VERSION(revision, stepping) \
        (((revision) << 8) | (stepping))

/**
 * \brief Convenience macro to create the full chip version from revision and stepping.
 *        It takes letters as arguments.
 */
#define BLACK_ORCA_IC_VERSION(revision, stepping) \
        _BLACK_ORCA_IC_VERSION(BLACK_ORCA_IC_REV_##revision, \
                        BLACK_ORCA_IC_STEP_##stepping)

/**
 * \brief The chip version that we compile for.
 */
#define BLACK_ORCA_TARGET_IC \
         _BLACK_ORCA_IC_VERSION(dg_configBLACK_ORCA_IC_REV, dg_configBLACK_ORCA_IC_STEP)


/**
 * \brief Zero-initialized data retained memory attribute
 */
#define __RETAINED              __attribute__((section("retention_mem_zi")))    // RetRAM0
#define __RETAINED_1            __attribute__((section("retention_mem_1_zi")))  // RetRAM1

/**
 * \brief Initialized data retained memory attribute
 */
#define __RETAINED_RW           __attribute__((section("retention_mem_init")))

/**
 * \brief Uninitialized data retained memory attribute
 */
#define __RETAINED_UNINIT       __attribute__((section("retention_mem_uninit")))

 /**
  * \brief Constant data retained memory attribute
  */
 #define __RETAINED_CONST_INIT  __attribute__((section("retention_mem_const")))

/**
 * \brief Text retained memory attribute
 */
#if ((dg_configCODE_LOCATION == NON_VOLATILE_IS_FLASH) && (dg_configEXEC_MODE == MODE_IS_CACHED))
#define __RETAINED_CODE         __attribute__((section("text_retained"))) __attribute__((noinline)) __attribute__((optimize ("no-tree-switch-conversion")))
#else
#define __RETAINED_CODE
#endif

/**
 * \brief Attribute to tell the compiler to consider a symbol as used
 */
#define __USED                  __attribute__((used))

/**
 * \brief Attribute to tell the compiler to consider a symbol as externally visible (for LTO)
 */
#define __LTO_EXT               __attribute__((externally_visible))

extern uint32_t black_orca_chip_version;

#define CHIP_IS_AE                      (black_orca_chip_version == BLACK_ORCA_IC_VERSION(A, E))
#define CHIP_IS_BB                      (black_orca_chip_version == BLACK_ORCA_IC_VERSION(B, B))

/**
 * \brief Get the chip version of the system, at runtime.
 */
__STATIC_INLINE uint32_t black_orca_get_chip_version(void)
{
        uint32_t rev = CHIP_VERSION->CHIP_REVISION_REG - 'A';
        uint32_t step = CHIP_VERSION->CHIP_TEST1_REG;

        return _BLACK_ORCA_IC_VERSION(rev, step);
}

// Forward declaration
__RETAINED_CODE void hw_cpm_assert_trigger_gpio(void);

/**
 * \brief Assert as warning macro
 *
 * \note Active only while in development mode
 */
#define ASSERT_WARNING(a)                                                                       \
        {                                                                                       \
                if (!(a)) {                                                                     \
                        if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE) {                         \
                                __asm volatile ( " cpsid i " );                                 \
                                GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_WDOG_Msk;      \
                                hw_cpm_assert_trigger_gpio();                                   \
                                do {} while(1);                                                 \
                        }                                                                       \
                }                                                                               \
        }

/**
 * \brief Assert as error macro
 *
 */
#define ASSERT_ERROR(a)                                                                         \
        {                                                                                       \
                if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE) {                                 \
                        if (!(a)) {                                                             \
                                __asm volatile ( " cpsid i " );                                 \
                                GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_WDOG_Msk;      \
                                hw_cpm_assert_trigger_gpio();                                   \
                                do {} while(1);                                                 \
                        }                                                                       \
                }                                                                               \
                else {                                                                          \
                        if (!(a)) {                                                             \
                                __asm volatile ( " cpsid i " );                                 \
                                __asm volatile ( " bkpt 2 " );                                  \
                        }                                                                       \
                }                                                                               \
        }

/**
 * \brief Assert as warning macro when the system is still uninitialized
 *
 * \note Active only while in development mode. The SW cursor is not activated.
 */
#define ASSERT_WARNING_UNINIT(a)                                                                \
        {                                                                                       \
                if (!(a)) {                                                                     \
                        if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE) {                         \
                                __asm volatile ( " cpsid i " );                                 \
                                GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_WDOG_Msk;      \
                                do {} while(1);                                                 \
                        }                                                                       \
                }                                                                               \
        }

/**
 * \brief Assert as error macro when the system is still uninitialized
 *
 * \note The SW cursor is not activated.
 */
#define ASSERT_ERROR_UNINIT(a)                                                                  \
        {                                                                                       \
                if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE) {                                 \
                        if (!(a)) {                                                             \
                                __asm volatile ( " cpsid i " );                                 \
                                GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_WDOG_Msk;      \
                                do {} while(1);                                                 \
                        }                                                                       \
                }                                                                               \
                else {                                                                          \
                        if (!(a)) {                                                             \
                                __asm volatile ( " cpsid i " );                                 \
                                __asm volatile ( " bkpt 2 " );                                  \
                        }                                                                       \
                }                                                                               \
        }

/**
 * \brief Macro to disable all interrupts
 *
 * This macro must always be used with GLOBAL_INT_RESTORE(). E.g.
 *
 * \code{.c}
 * GLOBAL_INT_DISABLE();
 *  ... code to be executed with interrupts disabled ...
 * GLOBAL_INT_RESTORE();
 * \endcode
 *
 * \sa GLOBAL_INT_RESTORE
 */
#define GLOBAL_INT_DISABLE()                                                            \
        do {                                                                            \
                unsigned int __l_irq_rest;                                     \
                __asm volatile ("mrs   %0, primask  \n\t"                               \
                              "mov   r1, $1     \n\t"                                   \
                              "msr   primask, r1  \n\t"                                 \
                              : "=r" (__l_irq_rest)                                     \
                              :                                                         \
                              : "r1"                                                    \
                              );                                                        \
               DBG_CONFIGURE_HIGH(CMN_TIMING_DEBUG, CMNDBG_CRITICAL_SECTION);

/**
 * \brief Macro to restore all interrupts
 *
 * This macro must always be used after GLOBAL_INT_DISABLE(). E.g.
 *
 * \code{.c}
 * GLOBAL_INT_DISABLE();
 *  ... code to be executed with interrupts disabled ...
 * GLOBAL_INT_RESTORE();
 * \endcode
 *
 * \sa GLOBAL_INT_DISABLE
 */
#define GLOBAL_INT_RESTORE()                                                            \
                if (__l_irq_rest == 0) {                                                \
                        DBG_CONFIGURE_LOW(CMN_TIMING_DEBUG, CMNDBG_CRITICAL_SECTION);   \
                }                                                                       \
                __asm volatile ("msr   primask, %0  \n\t"                               \
                                                          :                             \
                                                          : "r" (__l_irq_rest)          \
                                                          :                             \
                                                          );                            \
        } while(0)

#define containingoffset(address, type, field) ((type*)((uint8*)(address)-(size_t)(&((type*)0)->field)))

/**
 * \brief Macro the minimum of two values
 *
 * \param[in] a         First value
 * \param[in] b         Second value
 */
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

/**
 * \brief Macro the maximum of two values
 *
 * \param[in] a         First value
 * \param[in] b         Second value
 */
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

/**
 * \brief Macro to swap the bytes of a 16-bit variable
 *
 * \param[in] a         The 16-bit variable
 */
#if defined(__GNUC__)
#define SWAP16(a) __builtin_bswap16(a)
#else
#define SWAP16(a) ((a<<8) | (a>>8))
#endif

/**
 * \brief Macro to swap the bytes of a 32-bit variable
 *
 * \param[in] a         The 32-bit variable
 */
#if defined(__GNUC__)
#define SWAP32(a) __builtin_bswap32(a)
#else
#define SWAP32(a) ((a>>24 & 0xff) | (a>>8 & 0xff00) | (a<<8 & 0xff0000) | (a<<24 & 0xff000000))
#endif

#if defined(__GNUC__)
#define DEPRECATED __attribute__ ((deprecated))
#else
#warning Deprecated macro must be implemented for this compiler
#define DEPRECATED
#endif

#if defined(__GNUC__)
#define DEPRECATED_MSG(msg) __attribute__ ((deprecated(msg)))
#else
#warning Deprecated macro must be implemented for this compiler
#define DEPRECATED_MSG(msg)
#endif

/* The following exist in ROM code */
void __aeabi_memcpy(void *dest, const void *src, size_t n);
void __aeabi_memmove(void *dest, const void *src, size_t n);
void __aeabi_memset(void *dest, size_t n, int c);

/**
 * \brief Optimized memcpy
 */
#define OPT_MEMCPY      __aeabi_memcpy

/**
 * \brief Optimized memmove
 */
#define OPT_MEMMOVE     __aeabi_memmove

/**
 * \brief Optimized memset
 */
#define OPT_MEMSET(s, c, n)      __aeabi_memset(s, n, c)

/**
 * \brief Access register field mask.
 *
 * Returns a register field mask (aimed to be used with local variables).
 * e.g.
 * \code
 * uint16_t tmp;
 *
 * tmp = CRG_TOP->SYS_STAT_REG;
 *
 * if (tmp & REG_MSK(CRG_TOP, SYS_STAT_REG, XTAL16_TRIM_READY)) {
 * ...
 * \endcode
 */
#define REG_MSK(base, reg, field) \
        (base ## _ ## reg ## _ ## field ## _Msk)

/**
 * \brief Access register field position.
 *
 * Returns a register field position (aimed to be used with local variables).
 */
#define REG_POS(base, reg, field) \
        (base ## _ ## reg ## _ ## field ## _Pos)

/**
 * \brief Access register field value.
 *
 * Returns a register field value (aimed to be used with local variables).
 * e.g.
 * \code
 * uint16_t tmp;
 * int counter;
 * tmp = CRG_TOP->TRIM_CTRL_REG;
 * counter = REG_GET_FIELD(CRG_TOP, TRIM_CTRL_REG, XTAL_COUNT_N, tmp);
 * ...
 * \endcode
 */
#define REG_GET_FIELD(base, reg, field, var) \
        ((var & (base ## _ ## reg ## _ ## field ## _Msk)) >> \
                (base ## _ ## reg ## _ ## field ## _Pos))

/**
 * \brief Set register field value.
 *
 * Sets a register field value (aimed to be used with local variables).
 * e.g.
 * \code
 * uint16_t tmp;
 *
 * tmp = CRG_TOP->TRIM_CTRL_REG;
 * REG_SET_FIELD(CRG_TOP, TRIM_CTRL_REG, XTAL_COUNT_N, tmp, 10);
 * REG_SET_FIELD(CRG_TOP, TRIM_CTRL_REG, XTAL_TRIM_SELECT, tmp, 2);
 * CRG_TOP->TRIM_CTRL_REG = tmp;
 * ...
 * \endcode
 */
#define REG_SET_FIELD(base, reg, field, var, val) \
        var = ((var & ~((base ## _ ## reg ## _ ## field ## _Msk))) | \
                (((val) << (base ## _ ## reg ## _ ## field ## _Pos)) & \
                (base ## _ ## reg ## _ ## field ## _Msk)))

/**
 * \brief Clear register field value.
 *
 * Clears a register field value (aimed to be used with local variables).
 * e.g.
 * \code
 * uint16_t tmp;
 *
 * tmp = CRG_TOP->TRIM_CTRL_REG;
 * REG_CLR_FIELD(CRG_TOP, TRIM_CTRL_REG, XTAL_COUNT_N, tmp);
 * REG_CLR_FIELD(CRG_TOP, TRIM_CTRL_REG, XTAL_TRIM_SELECT, tmp);
 * CRG_TOP->TRIM_CTRL_REG = tmp;
 * ...
 * \endcode
 */
#define REG_CLR_FIELD(base, reg, field, var) \
        var &= ~(base ## _ ## reg ## _ ## field ## _Msk)

/**
 * \brief Get the address of a register value by index (provided a register interval)
 *
 * \note The register interval should be an exact multiple of the register's base size. For example,
 * if the register size is 32-bit, then the interval should be 0x4, 0x8, etc. Otherwise, the result
 * will be undefined. The interval value must be in bytes. The index value (0,1,2...) is multiplied by
 * the interval value (in bytes) to find the actual offset of the register.
 *
 * Returns a register address value by index
 */
#define REG_GET_ADDR_INDEXED(base, reg, interval, index) \
        ((&base->reg) + (((intptr_t) index) * ((interval) / sizeof(base->reg))))

/**
 * \brief Return the value of a register field by index (provided a register interval).
 *
 * e.g.
 * \code
 * uint16_t val;
 * uint16_t index = 2
 *
 * val = REG_GETF_INDEXED(FTDF, FTDF_LONG_ADDR_0_0_REG, REG_EXP_SA_L, 0x10, index)
 *
 * ...
 * \endcode
 *
 * \note The register interval should be an exact multiple of the register's base size. For example,
 * if the register size is 32-bit, then the interval should be 0x4, 0x8, etc. Otherwise, the result
 * will be undefined. The interval value must be in bytes. The index value (0,1,2...) is multiplied by
 * the interval value (in bytes) to find the actual offset of the register.
 *
 */
#define REG_GETF_INDEXED(base, reg, field, interval, index) \
        (((*REG_GET_ADDR_INDEXED(base, reg, interval, index)) & \
            (base ## _ ## reg ## _ ## field ## _Msk)) >> (base ## _ ## reg ## _ ## field ## _Pos))

/**
 * \brief Return the value of a register field.
 *
 * e.g.
 * \code
 * uint16_t val;
 *
 * val = REG_GETF(CRG_TOP, TRIM_CTRL_REG, XTAL_COUNT_N);
 * ...
 * \endcode
 */
#define REG_GETF(base, reg, field) \
        (((base->reg) & (base##_##reg##_##field##_Msk)) >> (base##_##reg##_##field##_Pos))

/**
 * \brief Set the value of a register field.
 *
 * e.g.
 * \code
 *
 * REG_SETF(CRG_TOP, TRIM_CTRL_REG, XTAL_COUNT_N, new_value);
 * ...
 * \endcode
 */
#define REG_SETF(base, reg, field, new_val) \
                base->reg = ((base->reg & ~(base##_##reg##_##field##_Msk)) | \
                ((base##_##reg##_##field##_Msk) & ((new_val) << (base##_##reg##_##field##_Pos))))

/**
 * \brief Set a bit of a register.
 *
 * e.g.
 * \code
 *
 * REG_SET_BIT(CRG_TOP, CLK_TMR_REG, TMR1_ENABLE);
 * ...
 * \endcode
 */
#define REG_SET_BIT(base, reg, field) \
        do { \
                base->reg |= (1 << (base##_##reg##_##field##_Pos)); \
         } while (0)

/**
 * \brief Clear a bit of a register.
 *
 * e.g.
 * \code
 *
 * REG_CLR_BIT(CRG_TOP, CLK_TMR_REG, TMR1_ENABLE);
 * ...
 * \endcode
 */
#define REG_CLR_BIT(base, reg, field) \
        do { \
                base->reg &= ~(base##_##reg##_##field##_Msk); \
         } while (0)

/**
 * \brief Sets register bits, indicated by the mask, to a value.
 *
 * e.g.
 * \code
 * REG_SET_MASKED(RFCU_POWER, RF_CNTRL_TIMER_5_REG, 0xFF00, 0x1818);
 * \endcode
 */
#define REG_SET_MASKED(base, reg, mask, value) \
        do { \
                base->reg = (base->reg & ~(mask)) | ((value) & (mask)); \
        } while (0)

/**
 * \brief Sets 16-bit wide register bits, indicated by the field, to a value v.
 */
#define BITS16(base, reg, field, v) \
        ((uint16) (((uint16) (v) << (base ## _ ## reg ## _ ## field ## _Pos)) & \
                (base ## _ ## reg ## _ ## field ## _Msk)))

/**
 * \brief Sets 32-bit wide register bits, indicated by the field, to a value v.
 */
#define BITS32(base, reg, field, v) \
        ((uint32) (((uint32) (v) << (base ## _ ## reg ## _ ## field ## _Pos)) & \
                (base ## _ ## reg ## _ ## field ## _Msk)))

/**
 * \brief Reads 16-bit wide register bits, indicated by the field, to a variable v.
 */
#define GETBITS16(base, reg, v, field) \
        ((uint16) (((uint16) (v)) & (base ## _ ## reg ## _ ## field ## _Msk)) >> \
                (base ## _ ## reg ## _ ## field ## _Pos))

/**
 * \brief Reads 32-bit wide register bits, indicated by the field, to a variable v.
 */
#define GETBITS32(base, reg, v, field) \
        ((uint32) (((uint32) (v)) & (base ## _ ## reg ## _ ## field ## _Msk)) >> \
                (base ## _ ## reg ## _ ## field ## _Pos))

/**
 * \brief Macro to enable the debugger
 *
 */
#define ENABLE_DEBUGGER \
        do { \
                REG_SET_BIT(CRG_TOP, SYS_CTRL_REG, DEBUGGER_ENABLE); \
        } while(0)

/**
 * \brief Macro to disable the debugger
 *
 */
#define DISABLE_DEBUGGER \
        do { \
                REG_CLR_BIT(CRG_TOP, SYS_CTRL_REG, DEBUGGER_ENABLE); \
        } while(0)


/**
 * \brief Macro to cause a software reset
 *
 */
#define SWRESET \
        do { \
                REG_SET_BIT(GPREG, DEBUG_REG, SW_RESET); \
        } while (0)

#define BIT0        0x01
#define BIT1        0x02
#define BIT2        0x04
#define BIT3        0x08
#define BIT4        0x10
#define BIT5        0x20
#define BIT6        0x40
#define BIT7        0x80

#define BIT8      0x0100
#define BIT9      0x0200
#define BIT10     0x0400
#define BIT11     0x0800
#define BIT12     0x1000
#define BIT13     0x2000
#define BIT14     0x4000
#define BIT15     0x8000

#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000

#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000

typedef unsigned char      uint8;   //  8 bits
typedef char               int8;    //  8 bits
typedef unsigned short     uint16;  // 16 bits
typedef short              int16;   // 16 bits
typedef unsigned long      uint32;  // 32 bits
typedef long               int32;   // 32 bits
typedef unsigned long long uint64;  // 64 bits
typedef long long          int64;   // 64 bits

/* See also "Data Types" on pag. 21 of the (Doulos) Cortex-M0 / SoC 1.0 training documentation. */
typedef unsigned char      BYTE;     //  8 bits = Byte
typedef unsigned short     HWORD;    // 16 bits = Halfword
typedef unsigned long      WORD;     // 32 bits = Word
typedef long long          DWORD;    // 64 bits = Doubleword


#ifdef __cplusplus
}
#endif

#endif  /* __SDK_DEFS_H__ */

/**
 * \}
 * \}
 * \}
 */

