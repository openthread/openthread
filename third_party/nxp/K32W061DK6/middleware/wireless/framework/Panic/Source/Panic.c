/*! *********************************************************************************
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
* All rights reserved.
*
* \file
*
* This is the source file for the Panic module.
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */

#include "Panic.h"
#include "fsl_os_abstraction.h"
#if gLoggingActive_d
#include "dbg_logging.h"
#endif

/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */
#if gUsePanic_c
panicData_t panic_data;
#endif

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
* \brief  This function will halt the system
*
* \param[in]  id Description of the param2 in parameter
* \param[in]  location address where the Panic occurred
* \param[in]  extra1 parameter to be stored in Panic structure
* \param[in]  extra2 parameter to be stored in Panic structure
*
********************************************************************************** */

void panic( panicId_t id, uint32_t location, uint32_t extra1, uint32_t extra2 )
{
#if gUsePanic_c
    /* Save the Link Register */
    volatile uint32_t savedLR = 0;
    __asm("push {r2}  ");
    __asm("push {LR} ");
    __asm("pop  {r2} ");
#if defined(__GNUC__)
    __asm("str  r2, [SP, #24]");
#else
    __asm("str  r2, [SP, #4]");
#endif
    __asm("pop {r2}");

    panic_data.id = id;
    panic_data.location = location;
    panic_data.extra1 = extra1;
    panic_data.extra2 = extra2;
    panic_data.linkRegister = savedLR;
    panic_data.cpsr_contents   = 0;

    OSA_InterruptDisable(); /* disable interrupts */
#if gLoggingActive_d
    DbgLogDump(1);
#endif
    /* infinite loop just to ensure this routine never returns */
    for(;;)
    {
        __asm("NOP");
    }
#endif
}

#if defined(__GNUC__)
void __attribute__((weak)) __assertion_failed(char* s) {}
#endif
