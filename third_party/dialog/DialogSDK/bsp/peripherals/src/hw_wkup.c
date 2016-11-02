/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup Wakeup_Timer
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_wkup.c
 *
 * @brief Implementation of the Wakeup timer Low Level Driver.
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
 ****************************************************************************************
 */

#if dg_configUSE_HW_USB_WKUP


#include <stdio.h>
#include <string.h>
#include <hw_wkup.h>

static hw_wkup_interrupt_cb intr_cb __RETAINED /* = NULL*/;

void hw_wkup_init(const wkup_config *cfg)
{
        unsigned int i;

        REG_SET_BIT(CRG_TOP, CLK_TMR_REG, WAKEUPCT_ENABLE);

        /* reset configuration */
        WAKEUP->WKUP_CTRL_REG = 0;
        WAKEUP->WKUP_COMPARE_REG = 0;

        /* disable all pins */
        for (i = 0; i < 5; i++) {
                *((volatile uint16_t *)WAKEUP_BASE + 5 + i) = 0;
                *((volatile uint16_t *)WAKEUP_BASE + 10 + i) = 0;
        }

        NVIC_DisableIRQ(WKUP_GPIO_IRQn);

        hw_wkup_configure(cfg);
}

void hw_wkup_configure(const wkup_config *cfg)
{
        int i;

        if (!cfg) {
                return;
        }

        hw_wkup_set_counter_threshold(cfg->threshold);
        hw_wkup_set_debounce_time(cfg->debounce);

        for (i = 0; i < HW_GPIO_NUM_PORTS; i++) {
                hw_wkup_configure_port(i, cfg->pin_state[i], cfg->pin_trigger[i]);
        }
}

void hw_wkup_register_interrupt(hw_wkup_interrupt_cb cb, uint32_t prio)
{
        intr_cb = cb;

        HW_WKUP_REG_SETF(CTRL, WKUP_ENABLE_IRQ, 1);

        NVIC_ClearPendingIRQ(WKUP_GPIO_IRQn);
        NVIC_SetPriority(WKUP_GPIO_IRQn, prio);
        NVIC_EnableIRQ(WKUP_GPIO_IRQn);
}

void hw_wkup_unregister_interrupt(void)
{
        intr_cb = NULL;

        HW_WKUP_REG_SETF(CTRL, WKUP_ENABLE_IRQ, 0);

        NVIC_DisableIRQ(WKUP_GPIO_IRQn);
}

void hw_wkup_handler(void)
{
        if (intr_cb) {
                intr_cb();
        } else {
                hw_wkup_reset_interrupt();
        }
}

#ifdef OS_BAREMETAL
void WKUP_GPIO_Handler(void)
{
        hw_wkup_handler();
}
#endif

#endif /* dg_configUSE_HW_USB_WKUP */
/**
 * \}
 * \}
 * \}
 */
