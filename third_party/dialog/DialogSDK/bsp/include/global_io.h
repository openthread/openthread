#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

/*
 *----------------------------------------------------------------------------------------------------------------------------------------
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
 * Description : IO, data type definitions and memory map selector.
 *
 * Remark(s)   : None.
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
 * 
 * Synchronicity keywords:
 *
 * Id         : $Id: global_io.h.rca 1.1 Wed Oct 24 09:42:42 2012 wroovers Experimental wroovers $
 * Source     : $Source: /services/syncdata/hazelaar/8500/server_vault/applab/14580/sw/keil/keil_full_emb_SWHL6V16LL6V32_24okt_crosstest_OK_fpga14b/src/inc/global_io.h.rca $
 * 
 *----------------------------------------------------------------------------------------------------------------------------------------
 * 
 * Synchronicity history:
 *
 * Log        : $Log: global_io.h.rca $
 * Log        : 
 * Log        :  Revision: 1.1 Mon Oct  1 11:08:33 2012 wroovers
 * Log        :  First Working set
 * Log        : 
 * Log        :  Revision: 1.2 Mon Mar 19 11:52:13 2012 snijders
 * Log        :  Updated the '*int*' typedef's. Put the 'bool' (C++ only) typedef in comment, see comment.
 * Log        :  Added typedef HWORD and updated the already existing WORD and DWORD (according to the ARM data type definitions).
 * Log        :  Removed the IAR depending '__far' and 'inline' defines (obsolete).
 * Log        :  Removed the CR16C_SW_V4 depending '__far' define (obsolete).
 * Log        :  Removed the 'NULL' define (most probably also obsolete).
 * Log        : 
 * Log        :  Revision: 1.1 Tue Mar  6 11:00:42 2012 snijders
 * Log        :  Initial check in for Project bbtester. Copied from "../sitel_io.h" and renamed to "global_io.h".
 * Log        :  Updated the header, no further changes yet.
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
 *
 * History:
 *
 *   sc14580a
 *   --------
 *   19-mar-2012/HS: Updated the '*int*' typedef's. Put the 'bool' (C++ only) typedef in comment, see comment.
 *                   Added typedef HWORD and updated the already existing WORD and DWORD (according to the ARM data type definitions).
 *                   Removed the IAR depending '__far' and 'inline' defines (obsolete).
 *                   Removed the CR16C_SW_V4 depending '__far' define (obsolete).
 *                   Removed the 'NULL' define (most probably also obsolete).
 *   06-mar-2012/HS: Initial check in for Project bbtester. Copied from "../sitel_io.h" and renamed to "global_io.h".
 *                   Updated the header, no further changes yet.
 *
 *----------------------------------------------------------------------------------------------------------------------------------------
*/

#include <stddef.h>

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

// Retention memory attributes
#define __RETAINED              __attribute__((section("retention_mem_zi")))
#define __RETAINED_RW           __attribute__((section("retention_mem_rw")))
#define __RETAINED_UNINIT       __attribute__((section("retention_mem_uninit")))
#if ((dg_configCODE_LOCATION == NON_VOLATILE_IS_FLASH) && (dg_configEXEC_MODE == MODE_IS_CACHED))
#define __RETAINED_CODE         __attribute__((section("text_retained"))) __attribute__((noinline))
#else
#define __RETAINED_CODE
#endif

// Interrupt macros
#define GLOBAL_INT_DISABLE()                                    \
        do {                                                    \
                uint32 __l_irq_rest = __get_PRIMASK();          \
                __set_PRIMASK(1);                               \
                DBG_CONFIGURE_HIGH(CMN_TIMING_DEBUG, CMNDBG_CRITICAL_SECTION);


#define GLOBAL_INT_RESTORE()                                    \
                if (__l_irq_rest == 0) {                        \
                        DBG_CONFIGURE_LOW(CMN_TIMING_DEBUG, CMNDBG_CRITICAL_SECTION); \
                        __set_PRIMASK(0);                       \
                }                                               \
                else {                                          \
                        __set_PRIMASK(1);                       \
                }                                               \
        } while(0)



#ifndef offsetof
#if defined(__GNUC__)
#define offsetof(type, member)  __builtin_offsetof(type, member)
#else
#define offsetof(type,member)   ((size_t)(&((type *)0)->member))
#endif
#endif

#define containingoffset(address, type, field) ((type*)((uint8*)(address)-(size_t)(&((type*)0)->field)))


#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

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

#if defined(__GNUC__)
#define SWAP16(a) __builtin_bswap16(a)
#define SWAP32(a) __builtin_bswap32(a)
#else
#define SWAP16(a) ((a<<8) | (a>>8))
#define SWAP32(a) ((a>>24 & 0xff) | (a>>8 & 0xff00) | (a<<8 & 0xff0000) | (a<<24 & 0xff000000))
#endif

#if defined(__GNUC__)
#define DEPRECATED __attribute__ ((deprecated))
#define DEPRECATED_MSG(msg) __attribute__ ((deprecated(msg)))
#else
#warning Deprecated macro must be implemented for this compiler
#define DEPRECATED
#define DEPRECATED_MSG(msg)
#endif

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
                ((val << (base ## _ ## reg ## _ ## field ## _Pos)) & \
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

#define ENABLE_DEBUGGER \
        do { \
                REG_SET_BIT(CRG_TOP, SYS_CTRL_REG, DEBUGGER_ENABLE); \
        } while(0)

#define DISABLE_DEBUGGER \
        do { \
                REG_CLR_BIT(CRG_TOP, SYS_CTRL_REG, DEBUGGER_ENABLE); \
        } while(0)


#define SWRESET \
        do { \
                REG_SET_BIT(GPREG, DEBUG_REG, SW_RESET); \
        } while (0)


#endif
