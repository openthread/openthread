/**
****************************************************************************************
*
* @file config.c
*
* @brief Configure system.
*
* Copyright (C) 2015. Dialog Semiconductor, unpublished work. This computer
* program includes Confidential, Proprietary Information and is a Trade Secret of
* Dialog Semiconductor. All use, disclosure, and/or reproduction is prohibited
* unless authorized in writing. All Rights Reserved.
*
* <black.orca.support@diasemi.com> and contributors.
*
****************************************************************************************
*/

/**
\addtogroup BSP
\{
\addtogroup SYSTEM
\{
\addtogroup Start-up
\{
*/

#include <stdbool.h>
#include <stdio.h>
#include "sdk_defs.h"
#include "interrupts.h"

/*
 * Dialog default priority configuration table.
 *
 * Content of this table will be applied at system start.
 *
 * \note If interrupt priorities provided by Dialog need to be changed, do not modify this file.
 * Create similar table with SAME name instead somewhere inside code without week attribute,
 * and it will be used instead of table below.
 */
#pragma weak __dialog_interrupt_priorities
INTERRUPT_PRIORITY_CONFIG_START(__dialog_interrupt_priorities)
        PRIORITY_0, /* Start interrupts with priority 0 (highest) */
                SVCall_IRQn,
                PendSV_IRQn,
                XTAL16RDY_IRQn,
        PRIORITY_1, /* Start interrupts with priority 1 */
                BLE_WAKEUP_LP_IRQn,
                BLE_GEN_IRQn,
                FTDF_WAKEUP_IRQn,
                FTDF_GEN_IRQn,
                RFCAL_IRQn,
                COEX_IRQn,
                CRYPTO_IRQn,
                RF_DIAG_IRQn,
        PRIORITY_2, /* Start interrupts with priority 2 */
                DMA_IRQn,
                I2C_IRQn,
                I2C2_IRQn,
                SPI_IRQn,
                SPI2_IRQn,
                ADC_IRQn,
                SRC_IN_IRQn,
                SRC_OUT_IRQn,
                TRNG_IRQn,
                LAST_IRQn,
        PRIORITY_3, /* Start interrupts with priority 3 (lowest) */
                SysTick_IRQn,
                UART_IRQn,
                UART2_IRQn,
                MRM_IRQn,
                KEYBRD_IRQn,
                IRGEN_IRQn,
                WKUP_GPIO_IRQn,
                SWTIM0_IRQn,
                SWTIM1_IRQn,
                QUADEC_IRQn,
                USB_IRQn,
                PCM_IRQn,
                VBUS_IRQn,
                DCDC_IRQn,
INTERRUPT_PRIORITY_CONFIG_END

void set_interrupt_priorities(const int8_t prios[])
{
        uint32_t old_primask, iser;
        int i = 0;
        uint32_t prio = 0;

        /*
         * We shouldn't change the priority of an enabled interrupt.
         *  1. Globally disable interrupts, saving the global interrupts disable state.
         *  2. Explicitly disable all interrupts, saving the individual interrupt enable state.
         *  3. Set interrupt priorities.
         *  4. Restore individual interrupt enables.
         *  5. Restore global interrupt enable state.
         */
        old_primask = __get_PRIMASK();
        __disable_irq();
        iser = NVIC->ISER[0];
        NVIC->ICER[0] = iser;

        for (i = 0; prios[i] != PRIORITY_TABLE_END; ++i) {
                switch (prios[i]) {
                case PRIORITY_0:
                case PRIORITY_1:
                case PRIORITY_2:
                case PRIORITY_3:
                        prio = prios[i] - PRIORITY_0;
                        break;
                default:
                        NVIC_SetPriority(prios[i], prio);
                        break;
                }
        }

        NVIC->ISER[0] = iser;
        __set_PRIMASK(old_primask);
}

/**
\}
\}
\}
*/
