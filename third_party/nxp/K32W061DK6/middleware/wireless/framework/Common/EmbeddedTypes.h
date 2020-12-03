/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#ifndef _EMBEDDEDTYPES_H_
#define _EMBEDDEDTYPES_H_


/************************************************************************************
*
*       INCLUDES
*
************************************************************************************/

#include <stdint.h>
#if defined (__IAR_SYSTEMS_ICC__)
#include "cmsis_iccarm.h"
#endif
/************************************************************************************
*
*       TYPE DEFINITIONS
*
************************************************************************************/

/* boolean types */
typedef uint8_t   bool_t;

typedef uint8_t   index_t;

/* TRUE/FALSE definition*/
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* null pointer definition*/
#ifndef NULL
#define NULL (( void * )( 0x0UL ))
#endif

#if defined(__GNUC__)
#define PACKED_STRUCT struct __attribute__ ((__packed__))
#define PACKED_UNION  union __attribute__ ((__packed__))
#elif defined(__IAR_SYSTEMS_ICC__)
#define PACKED_STRUCT __packed struct
#define PACKED_UNION __packed union
#elif defined(__CC_ARM)
#define PACKED_STRUCT struct __attribute__((packed))
#define PACKED_UNION union __attribute__((packed))
#else
#warning No definition for PACKED_STRUCT and PACKED_UNION!
#endif

typedef unsigned char uintn8_t;
typedef unsigned long uintn32_t;

typedef unsigned char uchar_t;

#if !defined(MIN)
#define MIN(a,b)                    (((a) < (b))?(a):(b))
#endif

#if !defined(MAX)
#define MAX(a,b)                    (((a) > (b))?(a):(b))
#endif

/* Compute the number of elements of an array */
#define NumberOfElements(x) (sizeof(x)/sizeof((x)[0]))

/* Compute the size of a string initialized with quotation marks */
#define SizeOfString(string) (sizeof(string) - 1)

#define GetRelAddr(strct, member) ((uint32_t)&(((strct*)(void *)0)->member))
#define GetSizeOfMember(strct, member) sizeof(((strct*)(void *)0)->member)

/* Type definitions for link configuration of instantiable layers  */
#define gInvalidInstanceId_c (instanceId_t)(-1)
typedef uint32_t instanceId_t;

/* Bit shift definitions */
#define BIT0              0x01U
#define BIT1              0x02U
#define BIT2              0x04U
#define BIT3              0x08U
#define BIT4              0x10U
#define BIT5              0x20U
#define BIT6              0x40U
#define BIT7              0x80U
#define BIT8             0x100U
#define BIT9             0x200U
#define BIT10            0x400U
#define BIT11            0x800U
#define BIT12           0x1000U
#define BIT13           0x2000U
#define BIT14           0x4000U
#define BIT15           0x8000U
#define BIT16          0x10000U
#define BIT17          0x20000U
#define BIT18          0x40000U
#define BIT19          0x80000U
#define BIT20         0x100000U
#define BIT21         0x200000U
#define BIT22         0x400000U
#define BIT23         0x800000U
#define BIT24        0x1000000U
#define BIT25        0x2000000U
#define BIT26        0x4000000U
#define BIT27        0x8000000U
#define BIT28       0x10000000U
#define BIT29       0x20000000U
#define BIT30       0x40000000U
#define BIT31       0x80000000U

/* Shift definitions */
#define SHIFT0      (0U)
#define SHIFT1      (1U)
#define SHIFT2      (2U)
#define SHIFT3      (3U)
#define SHIFT4      (4U)
#define SHIFT5      (5U)
#define SHIFT6      (6U)
#define SHIFT7      (7U)
#define SHIFT8      (8U)
#define SHIFT9      (9U)
#define SHIFT10     (10U)
#define SHIFT11     (11U)
#define SHIFT12     (12U)
#define SHIFT13     (13U)
#define SHIFT14     (14U)
#define SHIFT15     (15U)
#define SHIFT16     (16U)
#define SHIFT17     (17U)
#define SHIFT18     (18U)
#define SHIFT19     (19U)
#define SHIFT20     (20U)
#define SHIFT21     (21U)
#define SHIFT22     (22U)
#define SHIFT23     (23U)
#define SHIFT24     (24U)
#define SHIFT25     (25U)
#define SHIFT26     (26U)
#define SHIFT27     (27U)
#define SHIFT28     (28U)
#define SHIFT29     (29U)
#define SHIFT30     (30U)
#define SHIFT31     (31U)

#define SHIFT32     (32U)
#define SHIFT33     (33U)
#define SHIFT34     (34U)
#define SHIFT35     (35U)
#define SHIFT36     (36U)
#define SHIFT37     (37U)
#define SHIFT38     (38U)
#define SHIFT39     (39U)
#define SHIFT40     (40U)
#define SHIFT41     (41U)
#define SHIFT42     (42U)
#define SHIFT43     (43U)
#define SHIFT44     (44U)
#define SHIFT45     (45U)
#define SHIFT46     (46U)
#define SHIFT47     (47U)
#define SHIFT48     (48U)
#define SHIFT49     (49U)
#define SHIFT50     (50U)
#define SHIFT51     (51U)
#define SHIFT52     (52U)
#define SHIFT53     (53U)
#define SHIFT54     (54U)
#define SHIFT55     (55U)
#define SHIFT56     (56U)
#define SHIFT57     (57U)
#define SHIFT58     (58U)
#define SHIFT59     (59U)
#define SHIFT60     (60U)
#define SHIFT61     (61U)
#define SHIFT62     (62U)
#define SHIFT63     (63U)

#define NOT_USED(x) ((x)=(x))

/*============================================================================
                         FUNCTION PROTOTYPES
=============================================================================*/


/*!
 * CLZ implementation : count leading zeroes from MSB
 */
#ifdef __GNUC__
#define HAL_CLZ(x) __builtin_clz(x)
#define HAL_CTZ(x) __builtin_ctz(x)
#elif defined (__IAR_SYSTEMS_ICC__)
#define HAL_CLZ(x) __CLZ(x)

inline uint32_t __hal_ctz(uint32_t x)
{
    unsigned int res;
    x = __RBIT(x);
    res = __CLZ(x);
    return res;
}
#define HAL_CTZ(x) __hal_ctz(x)

#else
#define HAL_CLZ(x) HAL_CLZ_UNDEFINED(x)
#define HAL_CTZ(x) HAL_CTZ_UNDEFINED(x)
#endif



/*!
 ****************************************************************************************
 * @brief
 * HAL_BSR Bit Scan Reverse ( find MSB bit set in a bit field )
 * HAL_BSF Bit Scan Forward ( find LSB bit set in a bit field )
 * HAL_FFS Find LSB bit position + 1 per standard ffs definition
 *
 ****************************************************************************************
 */
#define HAL_BSR(x) (31 - HAL_CLZ(x))
#define HAL_BSF(x) HAL_CTZ(x)
#define HAL_FFS(x) (HAL_CTZ(x) + 1)
/*!
 ****************************************************************************************
 * @brief
 * HAL_REV16 swap bytes in a 16 bit word : useful for htons/ntohs and all enddianness conversions
 * HAL_REV32 swap bytes in a 32 bit word : useful for htonl/ntohl and all enddianness conversions
 * HAL_RBIT bit mirror / reverse of 32 bit word
 ****************************************************************************************
 */
#ifdef __GNUC__
#define HAL_REV16(x) __builtin_bswap16(x)
#define HAL_REV32(x)  __builtin_bswap32(x)
static inline uint32_t __hal_revb(uint32_t x)
{
    unsigned int res;
    __asm volatile ("rbit    %0, %1  \n\t"
                    : "=r" (res)
                    : "r" (x));
    return res;
}

#define HAL_RBIT(x) __hal_revb(x)

#elif defined (__IAR_SYSTEMS_ICC__)
#define HAL_REV16(x) __REV16(x)
#define HAL_RBIT(x)  __RBIT(x)
#define HAL_REV32(x) HAL_REV32_UNDEFINED(x)


#endif




#define HAL_HTONS(_x_)  HAL_REV16((_x_))
#define HAL_NTOHS(_x_)  HAL_REV16((_x_))

#define HAL_HTONL(_x_)  HAL_REV32((_x_))
#define HAL_NTOHL(_x_)  HAL_REV32((_x_))

#define HAL_BSWAP16(_x_) HAL_REV16((_x_))
#define HAL_BSWAP32(_x_) HAL_REV32((_x_))


#endif /* _EMBEDDEDTYPES_H_ */
