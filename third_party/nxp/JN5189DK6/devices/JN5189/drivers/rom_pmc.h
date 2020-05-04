/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ROM_RESET_H_
#define ROM_RESET_H_

#if defined __cplusplus
extern "C" {
#endif

/*!
 * @addtogroup ROM_API
 * @{
 */

/*! @file */

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <rom_common.h>
#include <stdint.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/*!
 * @brief  Get the cause of the reset.
 *
 * @return Reset cause value.
 * @retval 0x1 POR - The last chip reset was caused by a Power On Reset.
 * @retval 0x2 PADRESET - The last chip reset was caused by a Pad Reset.
 * @retval 0x4 BODRESET - The last chip reset was caused by a Brown Out Detector.
 * @retval 0x8 SYSTEMRESET - The last chip reset was caused by a System Reset requested by the ARM CPU.
 * @retval 0x10 WDTRESET - The last chip reset was caused by the Watchdog Timer.
 * @retval 0x20 WAKEUPIORESET -  The last chip reset was caused by a Wake-up I/O (GPIO or internal NTAG FD INT).
 * @retval 0x40 WAKEUPPWDNRESET - The last CPU reset was caused by a Wake-up from Power down (many sources possible:
 * timer, IO, ...).
 * @retval 0x80 SWRRESET - The last chip reset was caused by a Software.
 */
static inline uint32_t pmc_reset_get_cause(void)
{
    uint32_t (*p_pmc_reset_get_cause)(void);
    p_pmc_reset_get_cause = (uint32_t(*)(void))0x030046e9U;

    return p_pmc_reset_get_cause();
}

/*!
 * @brief  Clear the cause of the reset.
 *
 * @param  mask The mask of reset cause which you want to clear.
 *
 * @return  none
 */
static inline void pmc_reset_clear_cause(uint32_t mask)
{
    void (*p_pmc_reset_clear_cause)(uint32_t mask);
    p_pmc_reset_clear_cause = (void (*)(uint32_t mask))0x030046f5U;

    p_pmc_reset_clear_cause(mask);
}

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /* ROM_RESET_H_ */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
