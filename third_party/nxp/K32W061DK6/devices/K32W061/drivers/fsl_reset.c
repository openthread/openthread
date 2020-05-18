/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_common.h"
#include "fsl_reset.h"

#include "rom_lowpower.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.reset"
#endif
/* RG TODO This should be defined in jn518x.h */
#define SYSCON_PRESETCTRL_COUNT 2

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

#if ((defined(FSL_FEATURE_SOC_SYSCON_COUNT) && (FSL_FEATURE_SOC_SYSCON_COUNT > 0)) || \
     (defined(FSL_FEATURE_SOC_ASYNC_SYSCON_COUNT) && (FSL_FEATURE_SOC_ASYNC_SYSCON_COUNT > 0)))

/*!
 * brief Assert reset to peripheral.
 *
 * Asserts reset signal to specified peripheral module.
 *
 * param peripheral Assert reset to this peripheral. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_SetPeripheralReset(reset_ip_name_t peripheral)
{
    const uint32_t regIndex = ((uint32_t)peripheral & 0xFFFF0000u) >> 16;
    const uint32_t bitPos   = ((uint32_t)peripheral & 0x0000FFFFu);
    const uint32_t bitMask  = 1u << bitPos;

    assert(bitPos < 32u);

    /* ASYNC_SYSCON registers have offset 1024 */
    if (regIndex >= SYSCON_PRESETCTRL_COUNT)
    {
        /* reset register is in ASYNC_SYSCON */

        /* set bit */
        ASYNC_SYSCON->ASYNCPRESETCTRLSET = bitMask;
        /* wait until it reads 0b1 */
        while (0u == (ASYNC_SYSCON->ASYNCPRESETCTRL & bitMask))
        {
        }
    }
    else
    {
        /* reset register is in SYSCON */

        /* set bit */
        SYSCON->PRESETCTRLSETS[regIndex] = bitMask;
        /* wait until it reads 0b1 */
        while (0u == (SYSCON->PRESETCTRLS[regIndex] & bitMask))
        {
        }
    }
}

/*!
 * brief Clear reset to peripheral.
 *
 * Clears reset signal to specified peripheral module, allows it to operate.
 *
 * param peripheral Clear reset to this peripheral. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_ClearPeripheralReset(reset_ip_name_t peripheral)
{
    const uint32_t regIndex = ((uint32_t)peripheral & 0xFFFF0000u) >> 16;
    const uint32_t bitPos   = ((uint32_t)peripheral & 0x0000FFFFu);
    const uint32_t bitMask  = 1u << bitPos;

    assert(bitPos < 32u);

    /* ASYNC_SYSCON registers have offset > SYSCON_PRESETCTRL_COUNT */
    if (regIndex >= SYSCON_PRESETCTRL_COUNT)
    {
        /* reset register is in ASYNC_SYSCON */

        /* clear bit */
        ASYNC_SYSCON->ASYNCPRESETCTRLCLR = bitMask;
        /* wait until it reads 0b0 */
        while (bitMask == (ASYNC_SYSCON->ASYNCPRESETCTRL & bitMask))
        {
        }
    }
    else
    {
        /* reset register is in SYSCON */

        /* clear bit */
        SYSCON->PRESETCTRLCLRS[regIndex] = bitMask;
        /* wait until it reads 0b0 */
        while (bitMask == (SYSCON->PRESETCTRLS[regIndex] & bitMask))
        {
        }
    }
}

/*!
 * brief Reset peripheral module.
 *
 * Reset peripheral module.
 *
 * param peripheral Peripheral to reset. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_PeripheralReset(reset_ip_name_t peripheral)
{
    RESET_SetPeripheralReset(peripheral);
    RESET_ClearPeripheralReset(peripheral);
}

#endif /* FSL_FEATURE_SOC_SYSCON_COUNT || FSL_FEATURE_SOC_ASYNC_SYSCON_COUNT */

/*!
 * brief Reset the chip.
 *
 * Full software reset of the chip.
 * On reboot, function POWER_GetResetCause() from fsl_power.h will return RESET_SYS_REQ
 */
void RESET_SystemReset(void)
{
    /* Disable all interrupts */
    __disable_irq();
    /* On ES2, software reset is directly implemented in ROM code so the Flash
     * controller can be correctly powered OFF before the reset */
    Chip_LOWPOWER_ChipSoftwareReset();
}
