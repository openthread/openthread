/*****************************************************************************
 *
 * MODULE:             Microspecific
 *
 * COMPONENT:          Microspecific
 *
 * AUTHOR:             Wayne Ellis
 *
 * DESCRIPTION:        JN517x Microspecific Interrupt Controller Code
 *
 * $HeadURL:  $
 *
 * $Revision:  $
 *
 * $LastChangedBy: we1 $
 *
 * $LastChangedDate:  $
 *
 * $Id:  $
 *
 *****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd. 2014 All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "jendefs.h"

#include "MicroSpecific.h"
//#include "jn518x.h"
//#include "PeripheralRegs.h"
//#include <ARMcortexM3Registers_JN51xx.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
// PreEmpt Priority field is [7:5]
#define PREEMPT_PRIORITY_FIELD          (4)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
#if 0
/****************************************************************************
 * NAME:        vAHI_InitialiseInterruptController
 *
 * DESCRIPTION:
 * Initialise ARM NVIC.
 *
 * RETURNS:
 * Nothing.
 *
 * NOTES:
 *
 ****************************************************************************/

PUBLIC void vAHI_InitialiseInterruptController(
            uint32 *pu32InterruptVectorTable)
{
    uint32 u32RegisterValue=0;

    // set application interrupt and reset control register

    // write vector key
    U32_SET_BITS(&u32RegisterValue, REG_APP_INT_RESET_CTRL_VECTKEY);
    // set priority group
    U32_SET_BITS(&u32RegisterValue, PREEMPT_PRIORITY_FIELD << REG_APP_INT_RESET_CTRL_PRIGROUP_BIT);

    vREG_Write(REG_APP_INT_RESET_CTRL, u32RegisterValue);

    // set vector table location
    u32RegisterValue=0;

    // table in RAM - this bit isn't required in the JN5172 at all
    // U32_SET_BITS(&u32RegisterValue, 1 << REG_INT_VECTOR_TABLE_OFFSET_TBLBASE_BIT);

    // table address
    U32_SET_BITS(&u32RegisterValue, (uint32)pu32InterruptVectorTable);
    vREG_Write(REG_INT_VECTOR_TABLE_OFFSET, u32RegisterValue);

    // enable exception features - DIV0 and align errors
    u32RegisterValue=0;

    U32_SET_BITS(
       &u32RegisterValue,
    /*    REG_CONFIGURATION_CONTROL_UNALIGN_TRP_MASK | */REG_CONFIGURATION_CONTROL_DIV_0_TRP_MASK);

    vREG_Write(REG_CONFIGURATION_CONTROL, u32RegisterValue);

    // set the exception priorities here - benign but listed so we know where they are
    // MemManage
    vREG_Write8(REG_SYSTEM_HANDLER_PRIORITY_4, 0);
    // BusFault
    vREG_Write8(REG_SYSTEM_HANDLER_PRIORITY_5, 0);
    // UsageFault
    vREG_Write8(REG_SYSTEM_HANDLER_PRIORITY_6, 0);
    // SVC
    vREG_Write8(REG_SYSTEM_HANDLER_PRIORITY_11, 0);
    // debug monitor
    vREG_Write8(REG_SYSTEM_HANDLER_PRIORITY_12, 0);
    // pend SV
    vREG_Write8(REG_SYSTEM_HANDLER_PRIORITY_14, 0);
    // SYSTICK
    vREG_Write8(REG_SYSTEM_HANDLER_PRIORITY_15, MICRO_INTERRUPT_WRITE_PRIORITY_VALUE(MICRO_JENNIC_TO_ARM_PRIORITY_MAP(8)));

    u32RegisterValue=0;
    U32_SET_BITS(
       &u32RegisterValue,
       REG_SYSTEM_HANDLER_CNTRL_STATE_USGFAULTEN_MASK | REG_SYSTEM_HANDLER_CNTRL_STATE_BUSFAULTEN_MASK | REG_SYSTEM_HANDLER_CNTRL_STATE_MEMFAULTEN_MASK);
    vREG_Write(REG_SYSTEM_HANDLER_CNTRL_STATE, u32RegisterValue);

}
#endif

#if 0
/****************************************************************************
 * NAME:        vMicroIntSetGlobalEnable
 *
 * DESCRIPTION:
 * Enable specified interrupts.
 *
 * RETURNS:
 * Nothing.
 *
 * NOTES:
 *
 ****************************************************************************/

PUBLIC void vMicroIntSetGlobalEnable(
        uint32      u32EnableMask)
{
    // all API's work with the old BA stack numbers, appropriate translations
    // occur within the call
    vAHI_InterruptSetPriority(u32EnableMask, 12);
}
#endif

/****************************************************************************
 * NAME:        vMicroIntEnableOnly
 *
 * DESCRIPTION:
 * Enable specified interrupt.
 *
 * RETURNS:
 * Nothing.
 *
 * NOTES:
 *
 ****************************************************************************/

PUBLIC void vMicroIntEnableOnly(
        tsMicroIntStorage      *psIntStorage,
        uint32                  u32EnableMask)
{
    uint32 u32Store;

    /* Not used in this implementation */
    VARIABLE_INTENTIONALLY_NOT_REFERENCED(u32EnableMask)

    /* Disable interrupts for duration of this function */
#if defined(__IAR_SYSTEMS_ICC__) || defined(__IAR_SYSTEMS_ICC)
    /* unroll MICRO_DISABLE_AND_SAVE_INTERRUPTS macro content becasue IAR < 8.40 fails to see u32Store is set */
    u32Store = __get_PRIMASK();
    __disable_irq(); 
#else
    MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store);
#endif
    /* Store old priority level */
    psIntStorage->u8Level = MICRO_GET_ACTIVE_INT_LEVEL();

    /* Update priority level, but only if it is a more restrictive value */
    MICRO_SET_ACTIVE_INT_LEVEL_MAX(MICRO_INTERRUPT_WRITE_PRIORITY_VALUE(3));

    /* Restore interrupts */
    MICRO_RESTORE_INTERRUPTS(u32Store);
}

/****************************************************************************
 * NAME:        vMicroIntRestoreState
 *
 * DESCRIPTION:
 * Restore Previous Interrupt State.
 *
 * RETURNS:
 * Nothing.
 *
 * NOTES:
 *
 ****************************************************************************/

PUBLIC void vMicroIntRestoreState(
        tsMicroIntStorage      *psIntStorage)
{
    // write value direct into register ARM to ARM, no translations required
    MICRO_SET_ACTIVE_INT_LEVEL(psIntStorage->u8Level);
}

/****************************************************************************/
/***        End of file                                                   ***/
/****************************************************************************/
