/**
 * \addtogroup BSP
 * \{
 * \addtogroup SYSTEM
 * \{
 * \addtogroup PLATFORM
 * 
 * \brief Black Orca Platform definitions
 *
 * \{
 */

/**
 *****************************************************************************************
 *
 * @file black_orca.h
 *
 * @brief Central include header file for the Dialog Black Orca platform.
 *
 * @version 1.0
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
 *****************************************************************************************
 */

#ifndef __BLACK_ORCA_H__
#define __BLACK_ORCA_H__

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

#include "global_io.h"

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#  if dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_D
#       include "DA14680AD.h"
#  elif dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_C
#       include "DA14680AC.h"
#  elif dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_A
#       include "DA14680AA.h"
#  else
#       error "Unknown chip stepping for revision A -- check the value of dg_configBLACK_ORCA_IC_STEP"
#  endif
#else
#  error "Unknown chip revision -- check the value of dg_configBLACK_ORCA_IC_REV"
#endif

#include "core_cm0.h"                               /* Cortex-M0 processor and core peripherals                              */
#include "system_DA14680.h"                         /* DA14680AA System                                                      */


/************************ 
 * Black Orca Memory map
 ************************/

/**
 * \brief Remapped device address range.
 */
#define MEMORY_REMAPPED_BASE    0x00000000
#define MEMORY_REMAPPED_END     0x04000000

/**
 * \brief Remapped device memory size (64MiB).
 */
#define MEMORY_REMAPPED_SIZE    (MEMORY_REMAPPED_END - MEMORY_REMAPPED_BASE)

/**
 * \brief ROM address range.
 */
#define MEMORY_ROM_BASE         0x07F00000
#define MEMORY_ROM_END          0x07F40000

/**
 * \brief ROM memory size (256KiB).
 */
#define MEMORY_ROM_SIZE         (MEMORY_ROM_END - MEMORY_ROM_BASE)

/**
 * \brief OTP Controller address range.
 */
#define MEMORY_OTPC_BASE        0x07F40000
#define MEMORY_OTPC_END         0x07F80000

/**
 * \brief OTP memory address range.
 */
#define MEMORY_OTP_BASE         0x07F80000
#define MEMORY_OTP_END          0x07FC0000

/**
 * \brief OTP memory size (256KiB).
 */
#define MEMORY_OTP_SIZE         (MEMORY_OTP_END - MEMORY_OTP_BASE)

/**
 * \brief SYSTEM RAM address range.
 */
#define MEMORY_SYSRAM_BASE      0x07FC0000
#define MEMORY_SYSRAM_END       0x07FE0000

/**
 * \brief SYSTEM RAM size (128KiB).
 */
#define MEMORY_SYSRAM_SIZE      (MEMORY_SYSRAM_END - MEMORY_SYSRAM_BASE)

/**
 * \brief CACHE RAM address range.
 */
#define MEMORY_CACHERAM_BASE    0x07FE0000
#define MEMORY_CACHERAM_END     0x08000000

/**
 * \brief CACHE RAM size (128KiB).
 */
#define MEMORY_CACHERAM_SIZE    (MEMORY_CACHERAM_END - MEMORY_CACHERAM_BASE)

/**
 * \brief QSPI Flash address range.
 */
#define MEMORY_QSPIF_BASE       0x08000000
#define MEMORY_QSPIF_END        0x0BF00000

/**
 * \brief QSPI Flash memory size (63MiB).
 */
#define MEMORY_QSPIF_SIZE       (MEMORY_QSPIF_END - MEMORY_QSPIF_BASE)

/**
 * \brief QSPI Controller address range.
 */
#define MEMORY_QSPIC_BASE       0x0C000000
#define MEMORY_QSPIC_END        0x0C100000

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
        ((((uint32_t)(revision)) << 8) | ((uint32_t)(stepping)))

/**
 * \brief Convenience macro to create the full chip version from revision and stepping.
 * It takes letters as arguments.
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

#define ASSERT(a) { if (!(a)) while (1); }

#define ASSERT_WARNING(a)                                                                       \
        {                                                                                       \
                if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE) {                                 \
                        if (!(a)) {                                                             \
                                __asm volatile ( " cpsid i " );                                 \
                                GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_WDOG_Msk;      \
                                hw_cpm_assert_trigger_gpio();                                   \
                                do {} while(1);                                                 \
                        }                                                                       \
                }                                                                               \
        }



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


#ifdef __cplusplus
}
#endif

#endif  /* __BLACK_ORCA_H__ */

/**
 * \}
 * \}
 * \}
 */
