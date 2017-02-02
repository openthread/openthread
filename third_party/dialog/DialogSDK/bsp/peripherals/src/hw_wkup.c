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
 *
 ****************************************************************************************
 */

#if dg_configUSE_HW_WKUP


#include <stdio.h>
#include <string.h>
#include <hw_wkup.h>

static hw_wkup_interrupt_cb intr_cb __RETAINED /* = NULL*/;

#if ((dg_configLATCH_WKUP_SOURCE) && (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A))
#define PX_DATA_REG_ADDR(_port)         ((uint16_t *)(GPIO_BASE + offsetof(GPIO_Type, P0_DATA_REG)) + _port)
#define PX_DATA_REG(_port)              *PX_DATA_REG_ADDR(_port)
#define BF_GET(_var, _pos)              ((_var>>_pos) & 1)

__RETAINED volatile wkup_pin_config_t wkup_pin_config;
volatile uint8_t wkup_status[HW_GPIO_NUM_PORTS];

static bool hw_wkup_update_status(void)
{
        uint8_t i, j, enable_loop_cnt = 0;
        uint8_t pol_rx_reg;
        bool wkup_status_updated = false;
        uint8_t cur_port_data[HW_GPIO_NUM_PORTS];

        for (i = 0; i < HW_GPIO_NUM_PORTS; i++) {
                wkup_status[i] = 0;
                if (wkup_pin_config.pin_state[i]) {
                        //read port status
                        cur_port_data[i] = PX_DATA_REG(i);
                        //read trigger polarity
                        pol_rx_reg = *((volatile uint16_t *)WAKEUP_BASE + 10 + i);
                        for (j = 0; j < 8; j++) {
                                if ((wkup_pin_config.pin_state[i] & (1 << j)) &&
                                        (BF_GET(pol_rx_reg, j)
                                                == BF_GET(~cur_port_data[i], j))) {
                                        //invert trigger polarity
                                        pol_rx_reg ^= (1 << j);
                                        if (BF_GET(wkup_pin_config.pin_trigger[i], j)
                                                == BF_GET(~cur_port_data[i], j)) {
                                                wkup_status[i] |= (1 << j);
                                                wkup_status_updated = true;
                                        }
                                }
                        }
                        //update trigger polarity
                        *((volatile uint16_t *)WAKEUP_BASE + 10 + i) = pol_rx_reg;
                }
        }

        return wkup_status_updated;
}
#endif  /* dg_configLATCH_WKUP_SOURCE */

void hw_wkup_init(const wkup_config *cfg)
{
        unsigned int i;

        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, CLK_TMR_REG, WAKEUPCT_ENABLE);
        GLOBAL_INT_RESTORE();

        /* reset configuration */
        WAKEUP->WKUP_CTRL_REG = 0;

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        WAKEUP->WKUP_COMPARE_REG = 0;
#endif

        /* disable all pins */
        for (i = 0; i < HW_GPIO_NUM_PORTS; i++) {
                *((volatile uint16_t *)(&WAKEUP->WKUP_POL_P0_REG) + i) = 0;
                *((volatile uint16_t *)(&WAKEUP->WKUP_SELECT_P0_REG) + i) = 0;
#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
                *((volatile uint16_t *)(&WAKEUP-> WKUP_SEL_GPIO_P0_REG) + i) = 0;
#endif
#if dg_configLATCH_WKUP_SOURCE
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                wkup_pin_config.pin_state[i] = 0;
                wkup_pin_config.pin_trigger[i] = 0;
                wkup_status[i] = 0;
                hw_wkup_clear_status(i, 0xFF);
#endif
#endif //dg_configLATCH_WKUP_SOURCE
        }
#if dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A
        WAKEUP->WKUP_CLEAR_0_REG = WAKEUP->WKUP_CLEAR_1_REG = WAKEUP->WKUP_CLEAR_2_REG = 0xFFFF;
#endif
        NVIC_DisableIRQ(WKUP_GPIO_IRQn);

        hw_wkup_configure(cfg);
}

void hw_wkup_configure(const wkup_config *cfg)
{
        int i;

        if (!cfg) {
                return;
        }

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        hw_wkup_set_counter_threshold(cfg->threshold);
#endif
#if !((dg_configLATCH_WKUP_SOURCE) && (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A))
        hw_wkup_set_debounce_time(cfg->debounce);
#endif
        for (i = 0; i < HW_GPIO_NUM_PORTS; i++) {
                hw_wkup_configure_port(i, cfg->pin_state[i], cfg->pin_trigger[i]);
        }
}

void hw_wkup_register_interrupt(hw_wkup_interrupt_cb cb, uint32_t prio)
{
        intr_cb = cb;
#if !((dg_configLATCH_WKUP_SOURCE) && (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A))
        HW_WKUP_REG_SETF(CTRL, WKUP_ENABLE_IRQ, 1);
#endif
        NVIC_ClearPendingIRQ(WKUP_GPIO_IRQn);
        NVIC_SetPriority(WKUP_GPIO_IRQn, prio);
        NVIC_EnableIRQ(WKUP_GPIO_IRQn);
}

void hw_wkup_unregister_interrupt(void)
{
        intr_cb = NULL;
#if !((dg_configLATCH_WKUP_SOURCE) && (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A))
        HW_WKUP_REG_SETF(CTRL, WKUP_ENABLE_IRQ, 0);
#endif
        NVIC_DisableIRQ(WKUP_GPIO_IRQn);
}

void hw_wkup_handler(void)
{
#if ((dg_configLATCH_WKUP_SOURCE) && (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A))
        if (intr_cb && hw_wkup_update_status()) {
#else
        if (intr_cb) {
#endif
                intr_cb();
        }
        else {
                hw_wkup_reset_interrupt();
#if dg_configLATCH_WKUP_SOURCE
                hw_wkup_clear_status(HW_GPIO_PORT_0, 0xFF);
                hw_wkup_clear_status(HW_GPIO_PORT_1, 0xFF);
                hw_wkup_clear_status(HW_GPIO_PORT_2, 0xFF);
                hw_wkup_clear_status(HW_GPIO_PORT_3, 0xFF);
                hw_wkup_clear_status(HW_GPIO_PORT_4, 0xFF);
#endif
        }
}

#ifdef OS_BAREMETAL
void WKUP_GPIO_Handler(void)
{
        hw_wkup_handler();
}
#endif

#endif /* dg_configUSE_HW_WKUP */
/**
 * \}
 * \}
 * \}
 */
