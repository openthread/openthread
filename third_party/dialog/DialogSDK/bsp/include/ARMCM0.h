/**************************************************************************//**
 * @file     ARMCM0.h
 * @brief    CMSIS Cortex-M0 Core Peripheral Access Layer Header File
 *           for CM0 Device Series
 * @version  V1.05
 * @date     26. July 2011
 *
 * @note
 * Copyright (C) 2010-2011 ARM Limited. All rights reserved.
 *
 * @par
 * ARM Limited (ARM) is supplying this software for use with Cortex-M 
 * processor based microcontrollers.  This file can be freely distributed 
 * within development tools that are supporting such ARM based processors. 
 *
 * @par
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/


#ifndef ARMCM0_H
#define ARMCM0_H

#include "global_io.h"
/*
 * ==========================================================================
 * ---------- Interrupt Number Definition -----------------------------------
 * ==========================================================================
 */

/*
 * ==========================================================================
 * ----------- Processor and Core Peripheral Section ------------------------
 * ==========================================================================
 */


#include <core_cm0.h>                       /* Cortex-M0 processor and core peripherals           */
#include "system_ARMCM0.h"                  /* System Header                                      */


/******************************************************************************/
/*                Device Specific Peripheral registers structures             */
/******************************************************************************/

/*--------------------- General Purpose Input and Ouptut ---------------------*/
typedef union
{
  __IO uint32_t WORD;  
  __IO uint8_t  BYTE[4];
} GPIO_Data_TypeDef;

typedef struct
{
  GPIO_Data_TypeDef DATA[256];
  __O uint32_t  DIR;
  uint32_t      RESERVED[3];
  __O uint32_t  IE;
} ARM_GPIO_TypeDef;


/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/* Peripheral and SRAM base address */
#define ARM_SRAM_BASE             ((     uint32_t)0x20000000UL)
#define ARM_PERIPH_BASE           ((     uint32_t)0x40000000UL)

/* Peripheral memory map */
#define ARM_GPIO_BASE              ARM_PERIPH_BASE

#define ARM_GPIO0_BASE            (ARM_GPIO_BASE)
#define ARM_GPIO1_BASE            (ARM_GPIO_BASE       + 0x0800UL)
#define ARM_GPIO2_BASE            (ARM_GPIO_BASE       + 0x1000UL)


/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
#define ARM_GPIO0                ((ARM_GPIO_TypeDef *) ARM_GPIO0_BASE)
#define ARM_GPIO1                ((ARM_GPIO_TypeDef *) ARM_GPIO1_BASE)
#define ARM_GPIO2                ((ARM_GPIO_TypeDef *) ARM_GPIO2_BASE)


#endif  /* ARMCM0_H */
