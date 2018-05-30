/**
\addtogroup BSP
\{
\addtogroup DEVICES
\{
\addtogroup CPM
\{
*/

/**
 ****************************************************************************************
 *
 * @file hw_cpm.c
 *
 * @brief Clock and Power Manager Driver
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

#if dg_configUSE_HW_CPM


#include <stdint.h>
#include "hw_cpm.h"
#include "hw_watchdog.h"


/*
 * These variables should be defined and initialized by the framework/sdk used
 */
extern sys_clk_t cm_sysclk;
extern ahb_div_t cm_ahbclk;


/*
 * Global variables
 */
__RETAINED_UNINIT uint16_t hw_cpm_bod_enabled_in_tcs;

#if (dg_configPOWER_1V8_ACTIVE == 1)
__RETAINED_RW static bool cpm_1v8_state = true;
#else
static const bool cpm_1v8_state = false;
#endif


/*
 * Local variables
 */



/*
 * Forward declarations
 */



/*
 * Function definitions
 */


uint32_t hw_cpm_sysclk_is_rc16(void)
{
        uint32_t ret = 0;

        if (REG_GETF(CRG_TOP, CLK_CTRL_REG, SYS_CLK_SEL) == SYS_CLK_IS_RC16) {
                ret = 1;
        }

        return ret;
}


uint32_t hw_cpm_sysclk_is_xtal16m(void)
{
        uint32_t ret = 0;

        if (REG_GETF(CRG_TOP, CLK_CTRL_REG, SYS_CLK_SEL) == SYS_CLK_IS_XTAL16M) {
                ret = 1;
        }

        return ret;
}


void hw_cpm_set_divn(bool freq)
{
        uint32_t regval;
        int val;

        if (freq) {
                val = 1;
        }
        else {
                val = 0;
        }

        regval = CRG_TOP->CLK_CTRL_REG;
        REG_SET_FIELD(CRG_TOP, CLK_CTRL_REG, DIVN_XTAL32M_MODE, regval, val);
        REG_SET_FIELD(CRG_TOP, CLK_CTRL_REG, XTAL32M_MODE, regval, val);
        CRG_TOP->CLK_CTRL_REG = regval;
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
        CRG_TOP->DIVN_SYNC_REG = val;
#endif
}


bool hw_cpm_is_rc16_allowed(void)
{
        bool ret = false;
        uint32_t tmp;

        do {
                tmp = CRG_TOP->SYS_STAT_REG;

#ifdef CONFIG_USE_FTDF
                // Check FTDF
                if (tmp & REG_MSK(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP)) {
                        break;
                }
#endif

#ifdef CONFIG_USE_BLE
                // Check BLE
                if (tmp & REG_MSK(CRG_TOP, SYS_STAT_REG, BLE_IS_UP)) {
                        break;
                }
#endif

                // Check APHY/DPHY & COEX
                if (tmp & REG_MSK(CRG_TOP, SYS_STAT_REG, RAD_IS_UP)) {
                        break;
                }

                if (tmp & REG_MSK(CRG_TOP, SYS_STAT_REG, PER_IS_UP)) {
                        // Check SRC
                        if (REG_GETF(APU, SRC1_CTRL_REG, SRC_EN) == 1) {
                                break;
                        }

                        // Check PDM
                        if (REG_GETF(CRG_PER, PDM_DIV_REG, CLK_PDM_EN) == 1) {
                                break;
                        }

                        // Check USB
                        if (REG_GETF(USB, USB_MCTRL_REG, USBEN) == 1) {
                                break;
                        }

                        tmp = CRG_PER->CLK_PER_REG;
                        // Check UART1/2
                        if (tmp & REG_MSK(CRG_PER, CLK_PER_REG, UART_ENABLE)) {
                                break;
                        }

                        /* -------------------------------------------------------------------------------*/

                        // Check ADC clock
                        if (REG_GETF(GPADC, GP_ADC_CTRL_REG, GP_ADC_EN) &&
                                        (!(tmp & REG_MSK(CRG_PER, CLK_PER_REG, ADC_CLK_SEL))))  {
                                break;
                        }

                        // Check I2C clock
                        if ((tmp & REG_MSK(CRG_PER, CLK_PER_REG, I2C_ENABLE)) &&
                                        (!(tmp & REG_MSK(CRG_PER, CLK_PER_REG, I2C_CLK_SEL)))) {
                                break;
                        }

                        // Check SPI clock
                        if ((tmp & REG_MSK(CRG_PER, CLK_PER_REG, SPI_ENABLE)) &&
                                                (!(tmp & REG_MSK(CRG_PER, CLK_PER_REG, SPI_CLK_SEL)))) {
                                break;
                        }

                        // Check PCM clock
                        tmp = CRG_PER->PCM_DIV_REG;
                        if ((tmp & REG_MSK(CRG_PER, PCM_DIV_REG, CLK_PCM_EN)) &&
                                        (!(tmp & REG_MSK(CRG_PER, PCM_DIV_REG, PCM_SRC_SEL)))) {
                                break;
                        }

                        /* KBSCN and QUAD are not seriously affected by the clock switch and, thus,
                         * they cannot block it!
                         */
                }

                tmp = CRG_TOP->CLK_TMR_REG;
                // Check Timer0 clock
                if ((tmp & REG_MSK(CRG_TOP, CLK_TMR_REG, TMR0_ENABLE)) &&
                                (!(tmp & REG_MSK(CRG_TOP, CLK_TMR_REG, TMR0_CLK_SEL)))) {
                        break;
                }

                // Check Timer2 clock
                if ((tmp & REG_MSK(CRG_TOP, CLK_TMR_REG, TMR2_ENABLE)) &&
                                (!(tmp & REG_MSK(CRG_TOP, CLK_TMR_REG, TMR2_CLK_SEL)))) {
                        break;
                }

                /* Breathe, SOC and WDOG are not seriously affected by the clock switch and, thus,
                 * they cannot block it!
                 */

                ret = true;
        } while (0);

        return ret;
}


void hw_cpm_set_sysclk(uint32_t mode)
{
        /* Make sure a valid sys clock is requested */
        ASSERT_WARNING(mode <= SYS_CLK_IS_PLL);

        REG_SETF(CRG_TOP, CLK_CTRL_REG, SYS_CLK_SEL, mode);

        // Wait until the switch is done!
        switch (mode) {
        case SYS_CLK_IS_XTAL16M:
                while (REG_GETF(CRG_TOP, CLK_CTRL_REG, RUNNING_AT_XTAL16M) == 0) {}
                break;

        case SYS_CLK_IS_RC16:
                while (REG_GETF(CRG_TOP, CLK_CTRL_REG, RUNNING_AT_RC16M) == 0) {}
                break;

        case SYS_CLK_IS_LP:
                while (REG_GETF(CRG_TOP, CLK_CTRL_REG, RUNNING_AT_32K) == 0) {}
                break;

        case SYS_CLK_IS_PLL:
                while (REG_GETF(CRG_TOP, CLK_CTRL_REG, RUNNING_AT_PLL96M) == 0) {}
                break;

        default:
                ASSERT_WARNING(0);
                break;
        }
}


void hw_cpm_short_delay(void)
{
        volatile int i;

        for (i = 0; i < 20; i++);
}


void hw_cpm_pll_sys_on(void)
{
        /* Before enabling the PLL LDO, the 1.4V voltage needs to be present; in real-life this
         * is achieved by first turning on the 1.4V ACORE LDO, then the DCDC converter to take over
         * the generation of 1.4V and finally turning off the ACORE LDO.
         */

        /* LDO PLL enable. */
        REG_SET_BIT(GPREG, PLL_SYS_CTRL1_REG, LDO_PLL_ENABLE);

        /* Configure system PLL. */
        REG_SETF(GPREG, PLL_SYS_CTRL1_REG, PLL_R_DIV, 1); /* Default/reset value. */

        /* Program N-divider and DEL_SEL. */
        REG_SET_BIT(GPREG, PLL_SYS_CTRL2_REG, PLL_SEL_MIN_CUR_INT);     // Last review date: Feb 15, 2016 - 12:25:47

        /* Now turn on PLL. */
        REG_SET_BIT(GPREG, PLL_SYS_CTRL1_REG, PLL_EN);

        while ((GPREG->PLL_SYS_STATUS_REG & REG_MSK(GPREG, PLL_SYS_STATUS_REG, LDO_PLL_OK)) == 0) {}

        /* And wait until lock. */
        while ((GPREG->PLL_SYS_STATUS_REG & REG_MSK(GPREG, PLL_SYS_STATUS_REG, PLL_LOCK_FINE)) == 0) {}
}


void hw_cpm_pll_sys_off(void)
{
        // The PLL is not the system clk.
        while ((CRG_TOP->CLK_CTRL_REG & REG_MSK(CRG_TOP, CLK_CTRL_REG, RUNNING_AT_PLL96M)) != 0);
        GPREG->PLL_SYS_CTRL1_REG = 0x0000;                     // Switch off the PLL.
}


__RETAINED_CODE void hw_cpm_start_calibration(cal_clk_t clk_type, uint32_t cycles)
{
        ASSERT_WARNING(REG_GETF(ANAMISC, CLK_REF_SEL_REG, REF_CAL_START) == 0); // Must be disabled

        ANAMISC->CLK_REF_CNT_REG = cycles;                      // # of cal clock cycles
        REG_SETF(ANAMISC, CLK_REF_SEL_REG, REF_CLK_SEL, clk_type);
        REG_SET_BIT(ANAMISC, CLK_REF_SEL_REG, REF_CAL_START);
}


uint32_t hw_cpm_get_calibration_data(void)
{
        uint32_t high;
        uint32_t low;
        uint32_t value;

        while (REG_GETF(ANAMISC, CLK_REF_SEL_REG, REF_CAL_START) == 1);          // Wait until it's finished

        high = ANAMISC->CLK_REF_VAL_H_REG;
        low = ANAMISC->CLK_REF_VAL_L_REG;
        value = ( high << 16 ) + low;

        return value;
}

void hw_cpm_dcdc_config(void)
{
        uint32_t reg;

        /*
         * Preferred settings section
         * Last review date: January 03, 2017 - 16:26:35
         */
        REG_CLR_BIT(DCDC, DCDC_CTRL_0_REG, DCDC_FW_ENABLE);

        reg = DCDC->DCDC_IRQ_MASK_REG;
        REG_SET_FIELD(DCDC, DCDC_IRQ_MASK_REG, DCDC_V18P_TIMEOUT_IRQ_MASK, reg, 1);
        REG_SET_FIELD(DCDC, DCDC_IRQ_MASK_REG, DCDC_VDD_TIMEOUT_IRQ_MASK, reg, 1);
        REG_SET_FIELD(DCDC, DCDC_IRQ_MASK_REG, DCDC_V18_TIMEOUT_IRQ_MASK, reg, 1);
        REG_SET_FIELD(DCDC, DCDC_IRQ_MASK_REG, DCDC_V14_TIMEOUT_IRQ_MASK, reg, 1);
        DCDC->DCDC_IRQ_MASK_REG = reg;

        REG_SET_BIT(DCDC, DCDC_TRIM_REG, DCDC_P_COMP_MAN_TRIM);

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        DCDC->DCDC_V14_0_REG &= ~(REG_MSK(DCDC, DCDC_V14_0_REG, DCDC_V14_CUR_LIM_MIN) |
                                  REG_MSK(DCDC, DCDC_V14_0_REG, DCDC_V14_FAST_RAMPING));

        DCDC->DCDC_V18_0_REG &= ~(REG_MSK(DCDC, DCDC_V18_0_REG, DCDC_V18_CUR_LIM_MIN) |
                                  REG_MSK(DCDC, DCDC_V18_0_REG, DCDC_V18_FAST_RAMPING));

        DCDC->DCDC_V18P_0_REG &= ~(REG_MSK(DCDC, DCDC_V18P_0_REG, DCDC_V18P_CUR_LIM_MIN) |
                                   REG_MSK(DCDC, DCDC_V18P_0_REG, DCDC_V18P_FAST_RAMPING));

        DCDC->DCDC_VDD_0_REG &= ~(REG_MSK(DCDC, DCDC_VDD_0_REG, DCDC_VDD_CUR_LIM_MIN) |
                                  REG_MSK(DCDC, DCDC_VDD_0_REG, DCDC_VDD_FAST_RAMPING));
#else
        DCDC->DCDC_V14_0_REG &= ~REG_MSK(DCDC, DCDC_V14_0_REG, DCDC_V14_FAST_RAMPING);

        DCDC->DCDC_V18_0_REG &= ~REG_MSK(DCDC, DCDC_V18_0_REG, DCDC_V18_FAST_RAMPING);

        DCDC->DCDC_V18P_0_REG &= ~REG_MSK(DCDC, DCDC_V18P_0_REG, DCDC_V18P_FAST_RAMPING);

        DCDC->DCDC_VDD_0_REG &= ~REG_MSK(DCDC, DCDC_VDD_0_REG, DCDC_VDD_FAST_RAMPING);

        reg = DCDC->DCDC_CTRL_2_REG;
        REG_SET_FIELD(DCDC, DCDC_CTRL_2_REG, DCDC_LSSUP_TRIM, reg, 0);
        REG_SET_FIELD(DCDC, DCDC_CTRL_2_REG, DCDC_HSGND_TRIM, reg, 0);
        DCDC->DCDC_CTRL_2_REG = reg;
#endif

        REG_SETF(DCDC, DCDC_VDD_1_REG, DCDC_VDD_CUR_LIM_MAX_LV, 0xD);

        REG_SETF(DCDC, DCDC_V14_0_REG, DCDC_V14_VOLTAGE, 0x8);

        if (dg_configPOWER_1V8_ACTIVE == 1) {
                REG_SETF(DCDC, DCDC_V18_0_REG, DCDC_V18_VOLTAGE, 0x16);
        }

        if (dg_configPOWER_1V8P == 1) {
                REG_SETF(DCDC, DCDC_V18P_0_REG, DCDC_V18P_VOLTAGE, 0x16);
        }
       // End of preferred settings

        DCDC->DCDC_VDD_1_REG |= ((1 << REG_POS(DCDC, DCDC_VDD_1_REG, DCDC_VDD_ENABLE_HV)) |
                                 (1 << REG_POS(DCDC, DCDC_VDD_1_REG, DCDC_VDD_ENABLE_LV)));

        if ((dg_configPOWER_1V8_ACTIVE == 1) && cpm_1v8_state) {
                DCDC->DCDC_V18_1_REG |= (1 << REG_POS(DCDC, DCDC_V18_1_REG, DCDC_V18_ENABLE_HV));
                DCDC->DCDC_V18_1_REG &= ~REG_MSK(DCDC, DCDC_V18_1_REG, DCDC_V18_ENABLE_LV);
        }
        else {
                DCDC->DCDC_V18_1_REG &= ~(REG_MSK(DCDC, DCDC_V18_1_REG, DCDC_V18_ENABLE_HV) |
                                          REG_MSK(DCDC, DCDC_V18_1_REG, DCDC_V18_ENABLE_LV));
        }

        if (dg_configPOWER_1V8P == 1) {
                DCDC->DCDC_V18P_1_REG |= (1 << REG_POS(DCDC, DCDC_V18P_1_REG, DCDC_V18P_ENABLE_HV));
                DCDC->DCDC_V18P_1_REG &= ~REG_MSK(DCDC, DCDC_V18P_1_REG, DCDC_V18P_ENABLE_LV);
        }
        else {
                DCDC->DCDC_V18P_1_REG &= ~(REG_MSK(DCDC, DCDC_V18P_1_REG, DCDC_V18P_ENABLE_HV) |
                                          REG_MSK(DCDC, DCDC_V18P_1_REG, DCDC_V18P_ENABLE_LV));
        }
}

void hw_cpm_dcdc_on(void)
{
        DCDC->DCDC_V14_1_REG |= ((1 << REG_POS(DCDC, DCDC_V14_1_REG, DCDC_V14_ENABLE_HV)) |
                                 (1 << REG_POS(DCDC, DCDC_V14_1_REG, DCDC_V14_ENABLE_LV)));

        REG_SETF(DCDC, DCDC_VDD_0_REG, DCDC_VDD_VOLTAGE, 0x10);      // 1.2 V

        REG_SETF(DCDC, DCDC_CTRL_0_REG, DCDC_MODE, 1);

        // Trim the LDOs down to lowest possible voltage so DCDC can take over
        CRG_TOP->LDO_CTRL1_REG &= ~REG_MSK(CRG_TOP, LDO_CTRL1_REG, LDO_RADIO_SETVDD);
        REG_SETF(CRG_TOP, LDO_CTRL1_REG, LDO_CORE_SETVDD, 0x2);

        // Turn off LDOs
        CRG_TOP->LDO_CTRL1_REG &= ~REG_MSK(CRG_TOP, LDO_CTRL1_REG, LDO_RADIO_ENABLE);
        CRG_TOP->LDO_CTRL2_REG &= ~(REG_MSK(CRG_TOP, LDO_CTRL2_REG, LDO_1V2_ON) |
                                    REG_MSK(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_ON) |
                                    REG_MSK(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_PA_ON));

        // Trim the LDOs back to normal levels
        hw_cpm_reset_radio_vdd();
        CRG_TOP->LDO_CTRL1_REG &= ~REG_MSK(CRG_TOP, LDO_CTRL1_REG, LDO_CORE_SETVDD);
}


void hw_cpm_set_preferred_values(void)
{
        uint32_t reg;

        reg = CRG_TOP->CLK_16M_REG;

        REG_SET_FIELD(CRG_TOP, CLK_16M_REG, XTAL16_HPASS_FLT_EN, reg, 1); // Last review date: Feb 15, 2016 - 12:25:47
        REG_SET_FIELD(CRG_TOP, CLK_16M_REG, XTAL16_AMP_TRIM, reg, 5);
        REG_SET_FIELD(CRG_TOP, CLK_16M_REG, XTAL16_CUR_SET, reg, 5);

        CRG_TOP->CLK_16M_REG = reg;

        REG_SETF(CRG_TOP, BANDGAP_REG, LDO_SLEEP_TRIM, 0x8);
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
        REG_SETF(CRG_TOP, BANDGAP_REG, BYPASS_COLD_BOOT_DISABLE, 1); // Last review date: Feb 15, 2016 - 12:25:47
#else
        /* Hardcoding this since this will be used in AUTO mode as well (uartboot), where
         * the AE datasheet is included (it doesn't contain this reg)
         */
        volatile uint16_t *xtal16m_ctrl_reg = (volatile uint16_t *)0x50000056;

        /* The following code corresponds to (in BB):
           REG_SET_FIELD(CRG_TOP, XTAL16M_CTRL_REG, XTAL16M_ENABLE_ZERO, reg, 1);
           REG_SET_FIELD(CRG_TOP, XTAL16M_CTRL_REG, XTAL16M_AMP_REG_SIG_SEL, reg, 1);
         */
        *xtal16m_ctrl_reg |= 0x100UL | 0x8UL;

        /* The following corresponds to
           REG_SETF(CRG_TOP, BANDGAP_REG, LDO_SUPPLY_USE_BGREF, 1);
         */
        CRG_TOP->BANDGAP_REG |= 0x4000UL;
#endif
}

void hw_cpm_configure_xtal32k_pins(void)
{
        GPIO->P20_MODE_REG = 0x26;
        GPIO->P21_MODE_REG = 0x26;
}

void hw_cpm_configure_ext32k_pins(void)
{
        GPIO->P20_MODE_REG = 0x0;
}

void hw_cpm_trigger_sw_cursor(void)
{
        if (dg_configUSE_SW_CURSOR == 1) {
                if (dg_configBLACK_ORCA_MB_REV == BLACK_ORCA_MB_REV_D) {
                        SW_CURSOR_SET = 1 << SW_CURSOR_PIN;
                }
                else {
                        SW_CURSOR_RESET = 1 << SW_CURSOR_PIN;
                }

                SW_CURSOR_GPIO = 0x300;

                hw_cpm_delay_usec(50);

                if (dg_configBLACK_ORCA_MB_REV == BLACK_ORCA_MB_REV_D) {
                        SW_CURSOR_RESET = 1 << SW_CURSOR_PIN;
                }

                hw_cpm_setup_sw_cursor();
        }
}

void hw_cpm_reset_system(void)
{
        __asm volatile ( " cpsid i " );

        hw_watchdog_unregister_int();
        hw_watchdog_set_pos_val(1);
        hw_watchdog_unfreeze();

        while (1);
}

void hw_cpm_reboot_system(void)
{
        __asm volatile ( " cpsid i " );

        hw_watchdog_gen_RST();
        hw_watchdog_set_pos_val(1);
        hw_watchdog_unfreeze();

        while (1);
}

void hw_cpm_assert_trigger_gpio(void)
{
        if (EXCEPTION_DEBUG == 1) {
                if (dg_configLP_CLK_SOURCE == LP_CLK_IS_DIGITAL) {
                        hw_cpm_configure_ext32k_pins();
                } else if ((dg_configUSE_LP_CLK == LP_CLK_32000)
                                || (dg_configUSE_LP_CLK == LP_CLK_32768)) {
                        hw_cpm_configure_xtal32k_pins();
                }
                hw_cpm_power_up_per_pd();
                hw_cpm_deactivate_pad_latches();

                DBG_SET_HIGH(EXCEPTION_DEBUG, EXCEPTIONDBG);
        }
}

#define HW_CPM_ACTIVATE_BOD_PROTECTION                                                             \
do {                                                                                               \
        uint16_t val = 0;                                                                          \
                                                                                                   \
        REG_SETF(CRG_TOP, BOD_CTRL_REG, BOD_VDD_LVL, 1);                /* VDD Level (700mV) */    \
                                                                                                   \
        if (hw_cpm_bod_enabled_in_tcs == 0) {                                                      \
                val = 0;                                                                           \
                /* VBAT enable */                                                                  \
                REG_SET_FIELD(CRG_TOP, BOD_CTRL2_REG, BOD_VBAT_EN, val, 1);                        \
                /* 1V8 Flash enable */                                                             \
                if ((dg_configPOWER_1V8_ACTIVE == 1) && (dg_configPOWER_1V8_SLEEP == 1)) {         \
                        REG_SET_FIELD(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_FLASH_EN, val, 1);           \
                }                                                                                  \
                /* 1V8P enable */                                                                  \
                if (dg_configPOWER_1V8P == 1) {                                                    \
                        REG_SET_FIELD(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_PA_EN, val, 1);              \
                }                                                                                  \
                REG_SET_FIELD(CRG_TOP, BOD_CTRL2_REG, BOD_VDD_EN, val, 1);       /* VDD enable */  \
                REG_SET_FIELD(CRG_TOP, BOD_CTRL2_REG, BOD_RESET_EN, val, 1);     /* Reset enable */\
                CRG_TOP->BOD_CTRL2_REG = val;                                                      \
        }                                                                                          \
        else {                                                                                     \
                 CRG_TOP->BOD_CTRL2_REG = hw_cpm_bod_enabled_in_tcs;                               \
        }                                                                                          \
} while (0);

void hw_cpm_activate_bod_protection(void)
{
        HW_CPM_ACTIVATE_BOD_PROTECTION
}

void hw_cpm_activate_bod_protection_at_init(void)
{
        HW_CPM_ACTIVATE_BOD_PROTECTION
}

void hw_cpm_configure_bod_protection(void)
{
        REG_SETF(CRG_TOP, BOD_CTRL_REG, BOD_VDD_LVL, 1);                /* VDD Level (700mV) */

        if (hw_cpm_bod_enabled_in_tcs == 0) {
                /* VBAT enable */
                REG_SET_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_VBAT_EN);

                /* 1V8 Flash enable */
                if ((dg_configPOWER_1V8_ACTIVE == 1) && (dg_configPOWER_1V8_SLEEP == 1)) {
                        REG_SET_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_FLASH_EN);
                } else {
                        REG_CLR_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_FLASH_EN);
                }

                /* 1V8P enable */
                if (dg_configPOWER_1V8P == 1) {
                        REG_SET_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_PA_EN);
                } else {
                        REG_CLR_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_PA_EN);
                }
                /* Generate Reset on a BOD event */
                REG_SET_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_RESET_EN);
        }
        else {
                 CRG_TOP->BOD_CTRL2_REG = hw_cpm_bod_enabled_in_tcs;
        }
}

void hw_cpm_set_1v8_state(bool state)
{
        if ((dg_configPOWER_1V8_ACTIVE == 1) && (cpm_1v8_state != state)) {
                uint32_t reg;
                uint32_t dcdc_state;

                GLOBAL_INT_DISABLE();

                reg = CRG_TOP->LDO_CTRL2_REG;
                dcdc_state = (uint32_t)REG_GETF(DCDC, DCDC_CTRL_0_REG, DCDC_MODE);

#if (dg_configPOWER_1V8_ACTIVE == 1)
                cpm_1v8_state = state;
#endif

                if (!cpm_1v8_state) {
                        // Disable BOD for the 1V8 rail
                        if (dg_configUSE_BOD == 1) {
                                REG_CLR_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_FLASH_EN);
                        }

                        // Deactivate 1V8 rail in LDOs
                        REG_CLR_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_ON, reg);

                        if (dg_configPOWER_1V8_SLEEP == 1) {
                                REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_RET_DISABLE,
                                        reg, 1);
                        }

                        CRG_TOP->LDO_CTRL2_REG = reg;

                        // Deactivate 1V8 rail in DCDC
                        if (dg_configUSE_DCDC == 1) {
                                // Disable DCDC to apply change
                                REG_SETF(DCDC, DCDC_CTRL_0_REG, DCDC_MODE, 0);

                                DCDC->DCDC_V18_1_REG &=
                                        ~(REG_MSK(DCDC, DCDC_V18_1_REG, DCDC_V18_ENABLE_HV) |
                                          REG_MSK(DCDC, DCDC_V18_1_REG, DCDC_V18_ENABLE_LV));

                                // Restore DCDC
                                REG_SETF(DCDC, DCDC_CTRL_0_REG, DCDC_MODE, dcdc_state);
                        }
                } else {
                        // Restore 1V8 rail in LDOs
                        /* But not when DCDC is running... */
                        if (dcdc_state != 1) {
                                REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_ON,
                                        reg, 1);
                        }

                        if (dg_configPOWER_1V8_SLEEP == 1) {
                                REG_CLR_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_RET_DISABLE,
                                        reg);
                        }

                        CRG_TOP->LDO_CTRL2_REG = reg;

                        // Restore 1V8 rail in DCDC
                        if (dg_configUSE_DCDC == 1) {
                                DCDC->DCDC_V18_1_REG |= (1 << REG_POS(DCDC, DCDC_V18_1_REG,
                                        DCDC_V18_ENABLE_HV));
                                DCDC->DCDC_V18_1_REG &= ~REG_MSK(DCDC, DCDC_V18_1_REG,
                                        DCDC_V18_ENABLE_LV);
                        }

                        // Restore BOD setup
                        if (dg_configUSE_BOD == 1) {
                                hw_cpm_delay_usec(200);
                                hw_cpm_configure_bod_protection();
                        }
                }

                GLOBAL_INT_RESTORE();
        }
}

bool hw_cpm_get_1v8_state(void)
{
        return cpm_1v8_state;
}

void hw_cpm_delay_usec(uint32_t usec)
{
        sys_clk_t sclk;
        ahb_div_t hclk;
        uint8_t freq;

        sclk = cm_sysclk;
        hclk = cm_ahbclk;
        freq = 16 >> hclk;

        /* Requested delay time must be > 0 usec */
        ASSERT_WARNING(usec != 0);

        /*
         *      ldr r3, [pc, #148]      2 cycles
         *      push {r4, lr}           2 cycles
         *      ldrb r1, [r3, #17]      2 cycles
         *      ldrb r2, [r3, #8]       2 cycles
         *      movs r3, #16            1 cycle
         *      asrs r3, r2             1 cycle
         *      uxtb r3, r2             1 cycle
         *      ----------
         *      cmp r0, #0              4 cycles overhead in total
         *      bne.n 0x8003106 <hw_cpm_delay_usec+30>
         *      cpsid i
         *      bkpt 0x0002
         *
         *      Total: 11 cycles
         */

        __asm volatile(   "       cmp %[sclk], #1                 \n"     // 1 cycle      : 1 (sysclk_RC16, sysclk_XTAL16M)
                          "       ble start                       \n"     // 1 or 3 cycles: 2/4
                          "       cmp %[sclk], #3                 \n"     // 1 cycle      : 3 (sysclk_PLL48)
                          "       bgt s96M                        \n"     // 1 or 3 cycles: 4/6
                          "       blt s32M                        \n"     // 1 or 3 cycles: 5/7
                          "s48M:  add r4, %[freq], %[freq]        \n"     // 1 cycle      : 6
                          "       add %[freq], r4, %[freq]        \n"     // 1 cycle      : 7
                          "       b start                         \n"     // 3 cycles     : 10
                          "s96M:  add r4, %[freq], %[freq]        \n"     // 1 cycle      : 7
                          "       add %[freq], r4, %[freq]        \n"     // 1 cycle      : 8
                          "       lsl %[freq], %[freq], #1        \n"     // 1 cycle      : 9
                          "       b start                         \n"     // 3 cycles     : 12
                          "s32M:  lsl %[freq], %[freq], #1        \n"     // 1 cycle      : 8
                          /* -----------------------------------------------------*/
                          /* Overhead up to this point:
                           *      sysclk_RC16     : 15 cycles (error: 15/16  - 15   usec)
                           *      sysclk_XTAL16M  : 15 cycles (error: 15/16  - 15   usec)
                           *      sysclk_XTAL32M  : 19 cycles (error: 19/32  - 19/2 usec)
                           *      sysclk_PLL48    : 21 cycles (error: 21/48  - 21/3 usec)
                           *      sysclk_PLL96    : 23 cycles (error: 23/96 -  23/6 usec)
                           */
                          "start: cmp %[freq], #16                \n"     // 1 cycle      : 1
                          "       bgt high                        \n"     // 1 or 3 cycles: 2/4
                          "       blt low                         \n"     // 1 or 3 cycles: 3/5
                          "       mov %[sclk], #4                 \n"     // 1 cycle      : 4
                          "       b calc                          \n"     // 3 cycles     : 7
                          "high:  cmp %[freq], #24                \n"     // 1 cycle      : 5
                          "       bne c32M                        \n"     // 1 or 3 cycles: 6/8
                          "       mov %[sclk], #6                 \n"     // 1 cycle      : 7
                          "       b calc                          \n"     // 3 cycles     : 10
                          "c32M:  cmp %[freq], #32                \n"     // 1 cycle      : 9
                          "       bne c48M                        \n"     // 1 or 3 cycles: 10/12
                          "       mov %[sclk], #8                 \n"     // 1 cycle      : 11
                          "       b calc                          \n"     // 3 cycles     : 12
                          "c48M:  cmp %[freq], #48                \n"     // 1 cycle      : 13
                          "       bne c96M                        \n"     // 1 or 3 cycles: 14/16
                          "       mov %[sclk], #12                \n"     // 1 cycle      : 15
                          "       b calc                          \n"     // 3 cycles     : 18
                          "c96M:  mov %[sclk], #24                \n"     // 1 cycle      : 17
                          "       b calc                          \n"     // 3 cycles     : 20
                          "low:   cmp %[freq], #1                 \n"     // 1 cycle      : 6
                          "       bgt c2M                         \n"     // 1 or 3 cycles: 7/9
                          "       lsr %[usec], %[usec], #2        \n"     // 1 cycle      : 8
                          "       b loop                          \n"     // 3 cycles     : 11
                          "c2M:   cmp %[freq], #2                 \n"     // 1 cycle      : 10
                          "       bgt c3M                         \n"     // 1 or 3 cycles: 11/13
                          "       lsr %[usec], %[usec], #1        \n"     // 1 cycle      : 12
                          "       b loop                          \n"     // 3 cycles     : 15
                          "c3M:   cmp %[freq], #3                 \n"     // 1 cycle      : 14
                          "       bgt c4M                         \n"     // 1 or 3 cycles: 15/17
                          "       lsr %[usec], %[usec], #1        \n"     // 1 cycle      : 16
                          "       b loop1                         \n"     // 3 cycles     : 19
                          "c4M:   cmp %[freq], #4                 \n"     // 1 cycle      : 18
                          "       bgt c6M                         \n"     // 1 or 3 cycles: 19/21
                          "       b loop                          \n"     // 3 cycles     : 22
                          "c6M:   cmp %[freq],#6                  \n"     // 1 cycle      : 22
                          "       bgt c8M                         \n"     // 1 or 3 cycles: 23/25
                          "       b loop1                         \n"     // 3 cycles     : 26
                          "c8M:   cmp %[freq], #8                 \n"     // 1 cycle      : 26
                          "       bgt c12M                        \n"     // 1 or 3 cycles: 27/29
                          "       mov %[sclk], #2                 \n"     // 1 cycle      : 28
                          "       b calc                          \n"     // 3 cycles     : 31
                          "c12M:  mov %[sclk], #2                 \n"     // 1 cycle      : 30
                          "       mul %[usec], %[sclk], %[usec]   \n"     // 1 cycle      : 31
                          /* Error:
                           *       1MHz: 11 cycles, 11usec
                           *       2MHz: 15 cycles,  7.5usec
                           *       3MHz: 19 cycles,  6.33usec
                           *       4MHz: 22 cycles,  5.5usec
                           *       6MHz: 26 cycles,  4.33usec
                           *       8MHz: 31 cycles,  3.875usec
                           *      12MHz: 31 cycles,  2.584usec
                           *      16MHz:  8 cycles,  0.5usec
                           *      24MHz: 11 cycles,  0.459usec
                           *      32MHz: 13 cycles,  0.406usec
                           *      48MHz: 19 cycles,  0.396usec
                           *      96MHz: 21 cycles,  0.219usec
                           *
                           * 1 loop of 4 cycles is --- 1 usec is # loops
                           *       1MHz: 4usec     ---  divide (usec) by 4 (up to 5usec error)
                           *       2MHz: 2usec     ---  divide (usec) by 2 (up to 2usec error)
                           *       4MHz: 1usec     ---  1 loop  ( 0*4 + 1*2, 0.5000usec error)
                           *       8MHz: 500nsec   ---  2 loops ( 1*4 + 1*2, 0.2500usec error)
                           *      16MHz: 250nsec   ---  4 loops ( 3*4 + 1*2, 0.5000usec error)
                           *      24MHz: 167nsec   ---  6 loops ( 5*4 + 1*2, 0.0830usec error)
                           *      32MHz: 125nsec   ---  8 loops ( 7*4 + 1*2, 0.0625usec error)
                           *      48MHz:  84nsec   --- 12 loops (11*4 + 1*2, 0.0420usec error)
                           *      96MHz:  42nsec   --- 24 loops (23*4 + 1*2, 0.0210usec error)
                           *
                           * 1 loop of 6 cycles is --- 1 usec is # loops
                           *       3MHz: 2usec     ---  divide (usec) by 2 (up to 3usec error)
                           *       6MHz: 1usec     ---  1 loop  ( 0*6 + 1*7, 0.1670usec error)
                           *      12MHz: 0.5usec   ---  2 loops ( 1*6 + 1*7, 0.0830usec error)
                           *
                           * Cumulative error is:
                           *       1MHz: 11 + 5         = 16usec
                           *       2MHz: 7.5 + 2        = 9.5usec
                           *       3MHz: 6.33 + 3       = 9.33usec
                           *       4MHz: 5.5 + 0.5      = 6usec
                           *       6MHz: 4.33 + 0.167   = 4.5usec
                           *       8MHz: 3.875 + 0.25   = 4.125usec
                           *      12MHz: 2.584 + 0.083  = 2.67usec
                           *      16MHz: 0.5 + 0.5      = 1usec
                           *      24MHz: 0.459 + 0.083  = 0.541usec
                           *      32MHz: 0.406 + 0.0625 = 0.469usec
                           *      48MHz: 0.396 + 0.042  = 0.438usec
                           *      96MHz: 0.219 + 0.021  = 0.429usec
                           */
                          "loop1: sub  %[usec], %[usec], #1       \n"     // 1 cycle
                          "       nop                             \n"     // 1 cycle
                          "       nop                             \n"     // 1 cycle
                          "       bne loop1                       \n"     // 3 cycles except for the last one which is 1
                          "       b exit                          \n"     // 3 cycles
                          "calc:  mul %[usec], %[sclk], %[usec]   \n"     // 1 cycle
                          "loop:  sub  %[usec], %[usec], #1       \n"     // 1 cycle
                          "       bne loop                        \n"     // 3 cycles except for the last one which is 1
                          "exit:                                  \n"
                          :
                          :                                                         /* output */
                          [usec] "r" (usec), [freq] "r" (freq), [sclk] "r" (sclk) : /* inputs (%0, %1, %2) */
                          "r4");                                                    /* registers that are destroyed */
}

#endif /* dg_configUSE_HW_CPM */
/**
\}
\}
\}
*/
