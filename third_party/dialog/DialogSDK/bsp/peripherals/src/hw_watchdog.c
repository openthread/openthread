/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup Watchdog_Timer
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_watchdog.c
 *
 * @brief Implementation of the Watchdog timer Low Level Driver.
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
 ****************************************************************************************
 */
#include <stdio.h>
#include "hw_watchdog.h"
#include "hw_cpm.h"

/*
 * Global variables
 */
uint32_t nmi_event_data[9] __attribute__((section("nmi_info")));

/*
 * Local variables
 */
static hw_watchdog_interrupt_cb int_handler __RETAINED = NULL;

/*
 * This is the base address in Retention RAM where the stacked information will be copied.
 */
#define STATUS_BASE (0x7FC5600)

bool hw_watchdog_freeze(void)
{
        GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_WDOG_Msk;

        return !REG_GETF(WDOG, WATCHDOG_CTRL_REG, NMI_RST);
}

void hw_watchdog_unfreeze(void)
{
        GPREG->RESET_FREEZE_REG = GPREG_RESET_FREEZE_REG_FRZ_WDOG_Msk;
}

HW_WDG_RESET hw_watchdog_is_irq_or_rst_gen(void)
{
        if (REG_GETF(WDOG, WATCHDOG_CTRL_REG, NMI_RST)) {
                return HW_WDG_RESET_RST;
        }

        return HW_WDG_RESET_NMI;
}

void hw_watchdog_register_int(hw_watchdog_interrupt_cb handler)
{
        int_handler = handler;
}

void hw_watchdog_unregister_int(void)
{
        int_handler = NULL;
}

__RETAINED_CODE void hw_watchdog_handle_int(unsigned long *exception_args)
{
        // Reached this point due to a WDOG timeout
        uint16_t pmu_ctrl_reg = CRG_TOP->PMU_CTRL_REG;
        pmu_ctrl_reg |= ((1 << CRG_TOP_PMU_CTRL_REG_BLE_SLEEP_Pos)     |        /* turn off BLE */
                         (1 << CRG_TOP_PMU_CTRL_REG_FTDF_SLEEP_Pos)    |        /* turn off FTDF */
                         (1 << CRG_TOP_PMU_CTRL_REG_RADIO_SLEEP_Pos)   |        /* turn off radio PD */
                         (1 << CRG_TOP_PMU_CTRL_REG_PERIPH_SLEEP_Pos));         /* turn off peripheral power domain */
        CRG_TOP->PMU_CTRL_REG = pmu_ctrl_reg;
        REG_SET_BIT(CRG_TOP, CLK_RADIO_REG, BLE_LP_RESET);                      /* reset the BLE LP timer */

#if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE)
        hw_watchdog_freeze();                           // Stop WDOG

        ENABLE_DEBUGGER;

        if (exception_args != NULL) {
                *(volatile unsigned long *)(STATUS_BASE       ) = exception_args[0];    // R0
                *(volatile unsigned long *)(STATUS_BASE + 0x04) = exception_args[1];    // R1
                *(volatile unsigned long *)(STATUS_BASE + 0x08) = exception_args[2];    // R2
                *(volatile unsigned long *)(STATUS_BASE + 0x0C) = exception_args[3];    // R3
                *(volatile unsigned long *)(STATUS_BASE + 0x10) = exception_args[4];    // R12
                *(volatile unsigned long *)(STATUS_BASE + 0x14) = exception_args[5];    // LR
                *(volatile unsigned long *)(STATUS_BASE + 0x18) = exception_args[6];    // PC
                *(volatile unsigned long *)(STATUS_BASE + 0x1C) = exception_args[7];    // PSR
                *(volatile unsigned long *)(STATUS_BASE + 0x20) = (unsigned long)exception_args;    // Stack Pointer

                *(volatile unsigned long *)(STATUS_BASE + 0x24) = (*((volatile unsigned long *)(0xE000ED28)));    // CFSR
                *(volatile unsigned long *)(STATUS_BASE + 0x28) = (*((volatile unsigned long *)(0xE000ED2C)));    // HFSR
                *(volatile unsigned long *)(STATUS_BASE + 0x2C) = (*((volatile unsigned long *)(0xE000ED30)));    // DFSR
                *(volatile unsigned long *)(STATUS_BASE + 0x30) = (*((volatile unsigned long *)(0xE000ED3C)));    // AFSR
                *(volatile unsigned long *)(STATUS_BASE + 0x34) = (*((volatile unsigned long *)(0xE000ED34)));    // MMAR
                *(volatile unsigned long *)(STATUS_BASE + 0x38) = (*((volatile unsigned long *)(0xE000ED38)));    // BFAR
        }

        hw_cpm_assert_trigger_gpio();

        if (REG_GETF(CRG_TOP, SYS_STAT_REG, DBG_IS_ACTIVE)) {
                __BKPT(0);
        }
        else {
                while (1);
        }

#else // dg_configIMAGE_SETUP == DEVELOPMENT_MODE
        if (exception_args != NULL) {
                nmi_event_data[0] = NMI_MAGIC_NUMBER;
                nmi_event_data[1] = exception_args[0];          // R0
                nmi_event_data[2] = exception_args[1];          // R1
                nmi_event_data[3] = exception_args[2];          // R2
                nmi_event_data[4] = exception_args[3];          // R3
                nmi_event_data[5] = exception_args[4];          // R12
                nmi_event_data[6] = exception_args[5];          // LR
                nmi_event_data[7] = exception_args[6];          // PC
                nmi_event_data[8] = exception_args[7];          // PSR
        }

        // Wait for the reset to occur
        while (1);
#endif // dg_configIMAGE_SETUP == DEVELOPMENT_MODE
}

__RETAINED_CODE void NMI_HandlerC(unsigned long *exception_args);

void NMI_HandlerC(unsigned long *exception_args)
{
        if (int_handler) {
                int_handler(exception_args);
        }
        else {
                hw_watchdog_handle_int(exception_args);
        }
}

/**
 * \}
 * \}
 * \}
 */
