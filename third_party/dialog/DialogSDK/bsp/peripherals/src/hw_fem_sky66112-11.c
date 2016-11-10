/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup FEM
 * \{
 */

/**
 *****************************************************************************************
 *
 * @file
 *
 * @brief FEM Driver for SKYWORKS SKY66112-11 Radio module
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
 *****************************************************************************************
 */

#if dg_configFEM == FEM_SKY66112_11
#include "stdint.h"

#include "black_orca.h"
#include "hw_gpio.h"
#include "hw_fem_sky66112-11.h"

static hw_fem_config fem_config __RETAINED;

#if !defined(dg_configFEM_SKY66112_11_CTX_PORT) || !defined(dg_configFEM_SKY66112_11_CTX_PIN)
#error FEM CTX GPIO must be defined!!
#endif

#if !defined(dg_configFEM_SKY66112_11_CRX_PORT) || !defined(dg_configFEM_SKY66112_11_CRX_PIN)
#error FEM CRX GPIO must be defined!!
#endif


int hw_fem_set_bias(uint16_t voltage)
{
    uint16_t value;

    if (voltage < 1200 || voltage > 1975)
    {
        return -1;
    }

    value = (voltage - 1200) / 27.5;

//        /* Value is actually one lower than computed */
//        if (value > 0)
//                value--;

#ifdef dg_configFEM_SKY66112_11_FEM_BIAS_V18P
    REG_SETF(DCDC, DCDC_V18P_0_REG, DCDC_V18P_VOLTAGE, value);
    return 0;
#elif defined(dg_configFEM_SKY66112_11_FEM_BIAS_V18)
    REG_SETF(DCDC, DCDC_V18_0_REG, DCDC_V18_VOLTAGE, value);
    return 0;
#else
    return -2;
#endif

}

int hw_fem_set_bias2(uint16_t voltage)
{
    uint16_t value;

    if (voltage < 1200 || voltage > 1975)
    {
        return -1;
    }

    value = (voltage - 1200) / 27.5;

//        /* Value is actually one lower than computed */
//        if (value > 0)
//                value--;

#ifdef dg_configFEM_SKY66112_11_FEM_BIAS2_V18P
    REG_SETF(DCDC, DCDC_V18P_0_REG, DCDC_V18P_VOLTAGE, value);
#elif defined(dg_configFEM_SKY66112_11_FEM_BIAS2_V18)
    REG_SETF(DCDC, DCDC_V18_0_REG, DCDC_V18_VOLTAGE, value);
#else
    return -2;
#endif
    return 0;
}

void hw_fem_set_txpower(bool high)
{
#if defined(dg_configFEM_SKY66112_11_CHL_PORT) && defined(dg_configFEM_SKY66112_11_CHL_PIN)
    GLOBAL_INT_DISABLE();
    fem_config.tx_power = high;

    if (fem_config.started)
    {
        if (high == false)
        {
            /* TX Power low. Stop DCF, set GPIO to low */
            hw_gpio_configure_pin(dg_configFEM_SKY66112_11_CHL_PORT,
                                  dg_configFEM_SKY66112_11_CHL_PIN,
                                  HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false);
            REG_SET_MASKED(RFCU_POWER, RF_PORT_EN_REG,
                           RFCU_POWER_RF_PORT_EN_REG_RF_PORT4_RX_Msk |
                           RFCU_POWER_RF_PORT_EN_REG_RF_PORT4_TX_Msk, 0);

        }
        else
        {
            /* TX Power high. Configure GPIO for DCF. Enable DCF on TX. */
            hw_gpio_set_pin_function(dg_configFEM_SKY66112_11_CHL_PORT,
                                     dg_configFEM_SKY66112_11_CHL_PIN,
                                     HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_PORT4_DCF);
            REG_SET_MASKED(RFCU_POWER, RF_PORT_EN_REG,
                           RFCU_POWER_RF_PORT_EN_REG_RF_PORT4_RX_Msk |
                           RFCU_POWER_RF_PORT_EN_REG_RF_PORT4_TX_Msk,
                           RFCU_POWER_RF_PORT_EN_REG_RF_PORT4_TX_Msk);

        }
    }

    GLOBAL_INT_RESTORE();
#endif
}

static void set_bypass(void)
{
    uint16_t m = 0;

    if (fem_config.tx_bypass)
    {
        m = RFCU_POWER_RF_PORT_EN_REG_RF_PORT3_TX_Msk;
    }

    if (fem_config.rx_bypass)
    {
        m |= RFCU_POWER_RF_PORT_EN_REG_RF_PORT3_RX_Msk;
    }

    REG_SET_MASKED(RFCU_POWER, RF_PORT_EN_REG,
                   RFCU_POWER_RF_PORT_EN_REG_RF_PORT3_RX_Msk |
                   RFCU_POWER_RF_PORT_EN_REG_RF_PORT3_TX_Msk, m);

    if (m == 0)
    {
        /* No RX/TX bypass. Drive the interface line to low */
        hw_gpio_configure_pin(dg_configFEM_SKY66112_11_CPS_PORT,
                              dg_configFEM_SKY66112_11_CPS_PIN,
                              HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false);
    }
    else
    {
        /* At least one of TX/RX needs bypass */
        hw_gpio_set_pin_function(dg_configFEM_SKY66112_11_CPS_PORT,
                                 dg_configFEM_SKY66112_11_CPS_PIN,
                                 HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_PORT3_DCF);
    }
}


void hw_fem_set_tx_bypass(bool enable)
{
#if defined(dg_configFEM_SKY66112_11_CPS_PORT) && defined(dg_configFEM_SKY66112_11_CPS_PIN)
    GLOBAL_INT_DISABLE();
    fem_config.tx_bypass = enable;

    if (fem_config.started)
    {
        set_bypass();
    }

    GLOBAL_INT_RESTORE();
#endif
}

void hw_fem_set_rx_bypass(bool enable)
{
#if defined(dg_configFEM_SKY66112_11_CPS_PORT) && defined(dg_configFEM_SKY66112_11_CPS_PIN)
    GLOBAL_INT_DISABLE();
    fem_config.rx_bypass = enable;

    if (fem_config.started)
    {
        set_bypass();
    }

    GLOBAL_INT_RESTORE();
#endif
}

void hw_fem_set_antenna(bool one)
{
#if defined(dg_configFEM_SKY66112_11_ANTSEL_PORT) && defined(dg_configFEM_SKY66112_11_ANTSEL_PIN)
    GLOBAL_INT_DISABLE();
    fem_config.antsel = one;

    if (fem_config.started)
    {
        /* Antenna selection */
        hw_gpio_configure_pin(dg_configFEM_SKY66112_11_ANTSEL_PORT,
                              dg_configFEM_SKY66112_11_ANTSEL_PIN,
                              HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, fem_config.antsel);
    }

    GLOBAL_INT_RESTORE();
#endif
}

bool hw_fem_get_txpower(void)
{
    return fem_config.tx_power;
}

bool hw_fem_get_antenna(void)
{
    return fem_config.antsel;
}

bool hw_fem_get_tx_bypass(void)
{
    return fem_config.tx_bypass;
}

bool hw_fem_get_rx_bypass(void)
{
    return fem_config.rx_bypass;
}

void hw_fem_start(void)
{
    GLOBAL_INT_DISABLE();
    fem_config.started = true;

    uint8_t set_delay;
    uint8_t reset_delay;
    uint16_t rf_port_en;

    /******************************************************
     * Setup GPIOs
     */

    /* CSD GPIO Config */
#if defined(dg_configFEM_SKY66112_11_CSD_PORT) && defined(dg_configFEM_SKY66112_11_CSD_PIN)
# if dg_configFEM_SKY66112_11_CSD_USE_DCF == 0
    /* Manually set CSD (Enable FEM) */
    hw_gpio_configure_pin(dg_configFEM_SKY66112_11_CSD_PORT, dg_configFEM_SKY66112_11_CSD_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, true);
# else
    /* Use DCF for CSD */
    hw_gpio_set_pin_function(dg_configFEM_SKY66112_11_CSD_PORT, dg_configFEM_SKY66112_11_CSD_PIN,
                             HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_PORT2_DCF);
# endif
#endif

    /* Timer 27 GPIO (DCF Port 0). Used for TX EN */
    hw_gpio_set_pin_function(dg_configFEM_SKY66112_11_CTX_PORT, dg_configFEM_SKY66112_11_CTX_PIN,
                             HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_PORT0_DCF);

    /* Timer 28 (DCF Port 1). Used for RX EN */
    hw_gpio_set_pin_function(dg_configFEM_SKY66112_11_CRX_PORT, dg_configFEM_SKY66112_11_CRX_PIN,
                             HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_PORT1_DCF);

    /* Antenna selection */
#if defined(dg_configFEM_SKY66112_11_ANTSEL_PORT) && defined(dg_configFEM_SKY66112_11_ANTSEL_PIN)
    hw_gpio_configure_pin(dg_configFEM_SKY66112_11_ANTSEL_PORT, dg_configFEM_SKY66112_11_ANTSEL_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, fem_config.antsel);
#endif

    /******************************************************
     * Setup DCFs
     */

    /* assign values to the timer registers for CTX/CRX (in usec) */
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_27_REG, SET_OFFSET, dg_configFEM_SKY66112_11_TXSET_DCF);
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_27_REG, RESET_OFFSET, dg_configFEM_SKY66112_11_TXRESET_DCF);
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_28_REG, SET_OFFSET, dg_configFEM_SKY66112_11_RXSET_DCF);
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_28_REG, RESET_OFFSET, dg_configFEM_SKY66112_11_RXRESET_DCF);

    rf_port_en = 0x6; /* Start with Port 0: TX, Port 1: RX */

    /* Compute set/reset delays to use for CSD, CPS, CHL DCFs: For setting delay,
     * smaller of TXSET, RXSET, and for resetting delay, larger of TXSET, RXSET
     */
#if dg_configFEM_SKY66112_11_RXSET_DCF > dg_configFEM_SKY66112_11_TXSET_DCF
    set_delay = dg_configFEM_SKY66112_11_TXSET_DCF;
#else
    set_delay = dg_configFEM_SKY66112_11_RXSET_DCF;
#endif

#if dg_configFEM_SKY66112_11_RXRESET_DCF > dg_configFEM_SKY66112_11_TXRESET_DCF
    reset_delay = dg_configFEM_SKY66112_11_RXRESET_DCF;
#else
    reset_delay = dg_configFEM_SKY66112_11_TXRESET_DCF;
#endif

    /* CSD DCF (if enabled) configuration */
#if defined(dg_configFEM_SKY66112_11_CSD_PORT) && defined(dg_configFEM_SKY66112_11_CSD_PIN)
# if dg_configFEM_SKY66112_11_CSD_USE_DCF != 0
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_29_REG, SET_OFFSET, set_delay);
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_29_REG, RESET_OFFSET, reset_delay);

    /* enable DCF Signals for Port 2 (CSD) for both rx/tx */
    rf_port_en |= 0x30;

# endif /* dg_configFEM_SKY66112_11_CSD_USE_DCF != 0 */
#endif

    /* Set bypass (CPS) DCF timers (but don't enable yet) */
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_30_REG, SET_OFFSET, set_delay);
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_30_REG, RESET_OFFSET, reset_delay);

    /* Set TX Power (CHL) DCF timers (but don't enable yet) */
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_31_REG, SET_OFFSET, dg_configFEM_SKY66112_11_TXSET_DCF);
    REG_SETF(RFCU_POWER, RF_CNTRL_TIMER_31_REG, RESET_OFFSET, dg_configFEM_SKY66112_11_TXRESET_DCF);

    /* Enable DCFs */
    RFCU_POWER->RF_PORT_EN_REG = rf_port_en;

    hw_fem_set_txpower(fem_config.tx_power);
    set_bypass();

    GLOBAL_INT_RESTORE();
}

void hw_fem_stop(void)
{
    GLOBAL_INT_DISABLE();
    fem_config.started = false;

    /* Stop DCF Timers */
    RFCU_POWER->RF_PORT_EN_REG = 0x0;

    /* Set all FEM interface outputs to GPIO mode, and value 0, in order to get the minimum
     * possible power consumption from FEM
     */
#if defined(dg_configFEM_SKY66112_11_CSD_PORT) && defined(dg_configFEM_SKY66112_11_CSD_PIN)
    hw_gpio_configure_pin(dg_configFEM_SKY66112_11_CSD_PORT, dg_configFEM_SKY66112_11_CSD_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false);
#endif

    hw_gpio_configure_pin(dg_configFEM_SKY66112_11_CTX_PORT, dg_configFEM_SKY66112_11_CTX_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false);

#if defined(dg_configFEM_SKY66112_11_CHL_PORT) && defined(dg_configFEM_SKY66112_11_CHL_PIN)
    hw_gpio_configure_pin(dg_configFEM_SKY66112_11_CHL_PORT, dg_configFEM_SKY66112_11_CHL_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false);
#endif

    hw_gpio_configure_pin(dg_configFEM_SKY66112_11_CRX_PORT, dg_configFEM_SKY66112_11_CRX_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false);

#if defined(dg_configFEM_SKY66112_11_CPS_PORT) && defined(dg_configFEM_SKY66112_11_CPS_PIN)
    hw_gpio_configure_pin(dg_configFEM_SKY66112_11_CPS_PORT, dg_configFEM_SKY66112_11_CPS_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false);
#endif

#if defined(dg_configFEM_SKY66112_11_ANTSEL_PORT) && defined(dg_configFEM_SKY66112_11_ANTSEL_PIN)
    hw_gpio_configure_pin(dg_configFEM_SKY66112_11_ANTSEL_PORT, dg_configFEM_SKY66112_11_ANTSEL_PIN,
                          HW_GPIO_MODE_OUTPUT, HW_GPIO_FUNC_GPIO, false);
#endif

    GLOBAL_INT_RESTORE();
}
#endif /* dg_configFEM == FEM_SKY66112_11 */

/**
 * \}
 * \}
 * \}
 */
