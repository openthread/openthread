/**
\addtogroup BSP
\{
\addtogroup DEVICES
\{
\addtogroup CPM
\{
\brief Clock and Power Manager
*/

/**
 ****************************************************************************************
 *
 * @file hw_cpm.h
 *
 * @brief Clock and Power Manager header file.
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

#ifndef CPM_H_
#define CPM_H_

#if dg_configUSE_HW_CPM

#include "sdk_defs.h"


/**
 * \addtogroup CLOCK_TYPES
 * \brief Clock types
 * \note These macros must only be used with hw_cpm_set_sysclk().
 * \{
 */

/**
 * \brief System crystal oscillator 16 MHz
 */
#define SYS_CLK_IS_XTAL16M              (0)
/**
 * \brief System RC oscillator 16 MHz
 */
#define SYS_CLK_IS_RC16                 (1)
/**
 * \brief System Low Power clock
 */
#define SYS_CLK_IS_LP                   (2)
/**
 * \brief System PLL
 */
#define SYS_CLK_IS_PLL                  (3)
/**
 * \brief Low Power RC oscillator 32 kHz
 */
#define LP_CLK_IS_RC32K                 (0)
/**
 * \brief Low Power RC oscillator 11.7 kHz
 */
#define LP_CLK_IS_RCX                   (1)
/**
 * \brief Low Power crystal oscillator 32 kHz
 */
#define LP_CLK_IS_XTAL32K               (2)
/**
 * \brief Low Power external clock
 */
#define LP_CLK_IS_EXTERNAL              (3)
/**
 * \}
 */

#define DCDC_IS_READY \
        (REG_MSK(DCDC, DCDC_STATUS_1_REG, DCDC_VDD_AVAILABLE))
//        (DCDC_V18P_AVAILABLE | DCDC_VDD_AVAILABLE | DCDC_V18_AVAILABLE | DCDC_V14_AVAILABLE)

#define DCDC_IS_READY_TO_SLEEP          (DCDC_VDD_AVAILABLE | DCDC_V14_AVAILABLE)

typedef enum cal_clk_cel_type {
        CALIBRATE_RC32K = 0,
        CALIBRATE_RC16M,
        CALIBRATE_XTAL32K,
        CALIBRATE_RCX,
} cal_clk_t;

/**
 * \brief The system clock type
 * \note Must only be used with functions cm_sys_clk_init/set()
 *
 */
typedef enum sysclk_type {
        sysclk_RC16 = 0,        //!< RC16
        sysclk_XTAL16M = 1,     //!< 1 x 16M
        sysclk_XTAL32M = 2,     //!< 2 x 16M
        sysclk_PLL48 = 3,       //!< 3 x 16M
        sysclk_PLL96 = 6,       //!< 6 x 16M
        sysclk_LP = 255,        //!< not applicable
} sys_clk_t;

/**
 * \brief The AMBA High-Performance Bus (AHB) clock divider
 *
 */
typedef enum ahbdiv_type {
        ahb_div1 = 0,           //!< Divide by 1
        ahb_div2,               //!< Divide by 2
        ahb_div4,               //!< Divide by 4
        ahb_div8,               //!< Divide by 8
        ahb_div16,              //!< Divide by 16
} ahb_div_t;

ahb_div_t cm_ahbclk;
sys_clk_t cm_sysclk;

/**
 * \brief The AMBA Peripheral Bus (APB) clock divider
 *
 */
typedef enum apbdiv_type {
        apb_div1 = 0,           //!< Divide by 1
        apb_div2,               //!< Divide by 2
        apb_div4,               //!< Divide by 4
        apb_div8,               //!< Divide by 8
} apb_div_t;

/**
 * \brief The CPU clock type (speed)
 *
 */
typedef enum cpu_clk_type {
        cpuclk_1M = 1,          //!< 1 MHz
        cpuclk_2M = 2,          //!< 2 MHz
        cpuclk_3M = 3,          //!< 3 MHz
        cpuclk_4M = 4,          //!< 4 MHz
        cpuclk_6M = 6,          //!< 6 MHz
        cpuclk_8M = 8,          //!< 8 MHz
        cpuclk_12M = 12,        //!< 12 MHz
        cpuclk_16M = 16,        //!< 16 MHz
        cpuclk_24M = 24,        //!< 24 MHz
        cpuclk_32M = 32,        //!< 32 MHz
        cpuclk_48M = 48,        //!< 48 MHz
        cpuclk_96M = 96         //!< 96 MHz
} cpu_clk_t;


/**
 * \brief The TCS setting for the BOD control.
 * \details If it is zero then the hard-coded SDK code for the BOD setup is used. It it is not zero,
 *          this is the value that is written to the BOD_CTRL2_REG.
 */
extern uint16_t hw_cpm_bod_enabled_in_tcs;


/**
 * \brief Turn on the 1.2V LDO.
 *
 */
__STATIC_INLINE void hw_cpm_turn_1_2V_on(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_turn_1_2V_on(void)
{
        REG_SET_BIT(CRG_TOP, LDO_CTRL2_REG, LDO_1V2_ON);
}

/**
 * \brief Turn off the 1.2V LDO.
 *
 */
__STATIC_INLINE void hw_cpm_turn_1_2V_off(void)
{
        REG_CLR_BIT(CRG_TOP, LDO_CTRL2_REG, LDO_1V2_ON);
}

/**
 * \brief Enable Cache retainability.
 *
 */
__STATIC_INLINE void hw_cpm_set_cache_retained(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, RETAIN_CACHE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Enable ECC microcode RAM retainment.
 *
 */
__STATIC_INLINE void hw_cpm_set_eccram_retained(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, RETAIN_ECCRAM);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Enable QSPI initialization after wake-up.
 *
 */
__STATIC_INLINE void hw_cpm_enable_qspi_init(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, SYS_CTRL_REG, QSPI_INIT);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Setup the Retention Memory configuration.
 *
 */
__STATIC_INLINE void hw_cpm_setup_retmem(void)
{
        GLOBAL_INT_DISABLE();
        REG_SETF(CRG_TOP, PMU_CTRL_REG, RETAIN_RAM, dg_configMEM_RETENTION_MODE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Disable memory retention.
 *
 */
__STATIC_INLINE void hw_cpm_no_retmem(void)
{
        GLOBAL_INT_DISABLE();
        CRG_TOP->PMU_CTRL_REG &= ~(REG_MSK(CRG_TOP, PMU_CTRL_REG, RETAIN_RAM) |
                                   REG_MSK(CRG_TOP, PMU_CTRL_REG, RETAIN_CACHE) |
                                   REG_MSK(CRG_TOP, PMU_CTRL_REG, RETAIN_ECCRAM));
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Enable the clock-less sleep mode.
 *
 */
__STATIC_INLINE void hw_cpm_enable_clockless(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, ENABLE_CLKLESS);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Disable the clock-less sleep mode.
 *
 */
__STATIC_INLINE void hw_cpm_disable_clockless(void)
{
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, ENABLE_CLKLESS);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Activate the "Reset on wake-up" functionality.
 *
 */
__STATIC_INLINE void hw_cpm_enable_reset_on_wup(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, RESET_ON_WAKEUP);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Activate BOD protection.
 *
 */
__RETAINED_CODE void hw_cpm_activate_bod_protection(void);

/**
 * \brief Activate BOD protection (non retained version).
 *
 */
void hw_cpm_activate_bod_protection_at_init(void);

/**
 * \brief Configure BOD protection.
 *
 * \note Not applicable for DA14680/1-00 chips.
 *
 */
void hw_cpm_configure_bod_protection(void);

/**
 * \brief Deactivate BOD protection.
 *
 */
__STATIC_INLINE void hw_cpm_deactivate_bod_protection(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_deactivate_bod_protection(void)
{
        CRG_TOP->BOD_CTRL2_REG = 0;
}

/**
 * \brief Activate BOD protection for 1V4 rail (only for DA14682/3-00, DA15XXX-00 chips!).
 *
 */
__STATIC_INLINE void hw_cpm_activate_1v4_bod_protection(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_activate_1v4_bod_protection(void)
{
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_B)
        REG_SET_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_V14_EN);
#endif
}

/**
 * \brief Deactivate BOD protection for 1V4 rail (only for DA14682/3-00, DA15XXX-00 chips!).
 *
 */
__STATIC_INLINE void hw_cpm_deactivate_1v4_bod_protection(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_deactivate_1v4_bod_protection(void)
{
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_B)
        REG_CLR_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_V14_EN);
#endif
}

/**
 * \brief Power down the Radio Power Domain.
 *
 */
__STATIC_INLINE void hw_cpm_power_down_radio(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Power down the Peripheral Power Domain.
 *
 */
__STATIC_INLINE void hw_cpm_power_down_periph_pd(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_power_down_periph_pd(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, PERIPH_SLEEP);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Wait for Radio Power Domain Power down.
 *
 */
__STATIC_INLINE void hw_cpm_wait_rad_power_down(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_wait_rad_power_down(void)
{
        while ((CRG_TOP->SYS_STAT_REG & REG_MSK(CRG_TOP, SYS_STAT_REG, RAD_IS_DOWN)) == 0);
}

/**
 * \brief Wait for Peripheral Power Domain Power down.
 *
 */
__STATIC_INLINE void hw_cpm_wait_per_power_down(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_wait_per_power_down(void)
{
        while ((CRG_TOP->SYS_STAT_REG & REG_MSK(CRG_TOP, SYS_STAT_REG, PER_IS_DOWN)) == 0);
}

/**
 * \brief Power up the Peripherals Power Domain.
 *
 */
__STATIC_INLINE void hw_cpm_power_up_per_pd(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_power_up_per_pd(void)
{
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, PERIPH_SLEEP);
        GLOBAL_INT_RESTORE();
        while ((CRG_TOP->SYS_STAT_REG & REG_MSK(CRG_TOP, SYS_STAT_REG, PER_IS_UP)) == 0);
}

#if defined(CONFIG_USE_FTDF)
/**
 * \brief Check the status of FTDF Power Domain.
 *
 * \return 0, if it is powered down and 1 if it is powered up.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_check_ftdf_pd_status(void)
{
        return REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP);
}
#endif

#if defined(CONFIG_USE_BLE)
/**
 * \brief Check the status of BLE Power Domain.
 *
 * \return 0, if it is powered down and 1 if it is powered up.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_check_ble_pd_status(void)
{
        return REG_GETF(CRG_TOP, SYS_STAT_REG, BLE_IS_UP);
}
#endif

/**
 * \brief Check the status of Peripherals Power Domain.
 *
 * \return 0, if it is powered down and 1 if it is powered up.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_check_per_pd_status(void)
{
        return REG_GETF(CRG_TOP, SYS_STAT_REG, PER_IS_UP);
}

/**
 * \brief Check the status of Radio Power Domain.
 *
 * \return 0, if it is powered down and 1 if it is powered up.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_check_rad_pd_status(void)
{
        return REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_UP);
}

/**
 * \brief Activate Pad latches.
 *
 */
__STATIC_INLINE void hw_cpm_activate_pad_latches(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_activate_pad_latches(void)
{
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, SYS_CTRL_REG, PAD_LATCH_EN);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Deactivate Pad latches.
 *
 */
__STATIC_INLINE void hw_cpm_deactivate_pad_latches(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_deactivate_pad_latches(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, SYS_CTRL_REG, PAD_LATCH_EN);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Check if the RC16M is enabled.
 *
 * \return 0 if the RC16M is disabled, else 1.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_check_rc16_status(void)
{
        return REG_GETF(CRG_TOP, CLK_16M_REG, RC16M_ENABLE);
}

/**
 * \brief Activate the RC16M.
 *
 */
__STATIC_INLINE void hw_cpm_enable_rc16(void)
{
        REG_SET_BIT(CRG_TOP, CLK_16M_REG, RC16M_ENABLE);
}

/**
 * \brief Deactivate the RC16M.
 *
 */
__STATIC_INLINE void hw_cpm_disable_rc16(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_disable_rc16(void)
{
        REG_CLR_BIT(CRG_TOP, CLK_16M_REG, RC16M_ENABLE);
}

/**
 * \brief Set the XTAL16M settling time.
 *
 */
__STATIC_INLINE void hw_cpm_set_xtal16m_settling_time(uint8_t cycles) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_set_xtal16m_settling_time(uint8_t cycles)
{
        CRG_TOP->XTALRDY_CTRL_REG = cycles;
}

/**
 * \brief Check if the XTAL16M has started ticking
 *
 */
__STATIC_INLINE bool hw_cpm_is_xtal16m_started(void) __attribute__((always_inline));

__STATIC_INLINE bool hw_cpm_is_xtal16m_started(void)
{
        return (REG_GETF(CRG_TOP, SYS_STAT_REG, XTAL16_TRIM_READY) == 1);
}

/**
 * \brief Check if the XTAL16M is enabled.
 *
 * \return 0 if the XTAL16M is disabled, not 0 else.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_check_xtal16m_status(void)
{
        uint32_t ret;

        ret = REG_GETF(CRG_TOP, CLK_CTRL_REG, XTAL16M_DISABLE);

        return !ret;
}

/**
 * \brief Activate the XTAL16M.
 *
 */
__STATIC_INLINE void hw_cpm_enable_xtal16m(void)
{
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, CLK_CTRL_REG, XTAL16M_DISABLE);
        GLOBAL_INT_RESTORE();

}

/**
 * \brief Deactivate the XTAL16M.
 *
 */
__STATIC_INLINE void hw_cpm_disable_xtal16m(void)
{
        REG_SET_BIT(CRG_TOP, CLK_CTRL_REG, XTAL16M_DISABLE);
}

/**
 * \brief Enable the high-pass filter of the XTAL16M.
 *
 */
__STATIC_INLINE void hw_cpm_enable_xtal16m_hpf(void)
{
        REG_SET_BIT(CRG_TOP, CLK_16M_REG, XTAL16_HPASS_FLT_EN); // Last review date: Feb 15, 2016 - 12:25:47
}

/**
 * \brief Set a flag before sleeping that will check after the code execution is resumed after the
 *        WFI() to determine whether the system actually entered into sleep or not. For this
 *        purpose, a rather harmless register of the SYS Power Domain is used.
 *
 */
__STATIC_INLINE void hw_cpm_set_sleep_flag(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_set_sleep_flag(void)
{
        GPIO->GPIO_CLK_SEL = 1;
}

/**
 * \brief Prepare RESET type tracking.
 */
__STATIC_INLINE void hw_cpm_track_reset_type(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_track_reset_type(void)
{
#if (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_B)
        CRG_TOP->RESET_STAT_REG = 0;
#endif
}

/**
 * \brief Check if the system entered sleep.
 *
 * \return True, if the system slept, else false.
 *
 */
__STATIC_INLINE bool hw_cpm_check_sleep_flag(void)
{
        return (GPIO->GPIO_CLK_SEL == 0);
}

/**
 * \brief Check if the XTAL16M has settled.
 *
 * \return 1 if the XTAL16M has settled, else 0.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_is_xtal16m_trimmed(void)
{
        return REG_GETF(CRG_TOP, SYS_STAT_REG, XTAL16_TRIM_READY);
}

/**
 * \brief Check if the PLL is on and has locked.
 *
 * \return 1 if the PLL has locked, else 0.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_is_pll_locked(void)
{
        return REG_GETF(GPREG, PLL_SYS_STATUS_REG, PLL_LOCK_FINE);
}

/**
 * \brief Enable the PLL divider. The output frequency is set to 48MHz.
 *
 */
__STATIC_INLINE void hw_cpm_enable_pll_divider(void)
{
        REG_SET_BIT(CRG_TOP, CLK_CTRL_REG, PLL_DIV2);
}

/**
 * \brief Disable the PLL divider. The output frequency is set to 96MHz.
 *
 */
__STATIC_INLINE void hw_cpm_disable_pll_divider(void)
{
        REG_CLR_BIT(CRG_TOP, CLK_CTRL_REG, PLL_DIV2);
}

/**
 * \brief Get the status of the PLL divider.
 *
 * \return 0 if the divider is disabled, 1 if the divider is enabled.
 *
 */
__STATIC_INLINE uint32_t hw_cpm_get_pll_divider_status(void)
{
        return REG_GETF(CRG_TOP, CLK_CTRL_REG, PLL_DIV2);
}

/**
 * \brief Return the clock used as the system clock.
 *
 * \retval SYS_CLK_IS_XTAL16M Crystal oscillator
 * \retval SYS_CLK_IS_RC16 RC oscillator
 * \retval SYS_CLK_IS_LP Low Power clock
 * \retval SYS_CLK_IS_PLL PLL
 *
 */
__STATIC_INLINE uint32_t hw_cpm_get_sysclk(void)
{
        return REG_GETF(CRG_TOP, CLK_CTRL_REG, SYS_CLK_SEL);
}

/**
 * \brief Return the divider of the AMBA High Speed Bus.
 *
 * \retval 0 Divide by 1
 * \retval 1 Divide by 2
 * \retval 2 Divide by 4
 * \retval 3 Divide by 8
 * \retval 3 Divide by 16
 *
 */
__STATIC_INLINE uint32_t hw_cpm_get_hclk_div(void) __attribute__((always_inline));

__STATIC_INLINE uint32_t hw_cpm_get_hclk_div(void)
{
        return REG_GETF(CRG_TOP, CLK_AMBA_REG, HCLK_DIV);
}

/**
 * \brief Set the divider of the AMBA High Speed Bus.
 *
 */
__STATIC_INLINE void hw_cpm_set_hclk_div(uint32_t div) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_set_hclk_div(uint32_t div)
{
        GLOBAL_INT_DISABLE();
        REG_SETF(CRG_TOP, CLK_AMBA_REG, HCLK_DIV, div);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Return the divider of the AMBA Peripheral Bus.
 *
 * \retval 0 Divide by 1
 * \retval 1 Divide by 2
 * \retval 2 Divide by 4
 * \retval 3 Divide by 8
 *
 */
__STATIC_INLINE uint32_t hw_cpm_get_pclk_div(void)
{
        return REG_GETF(CRG_TOP, CLK_AMBA_REG, PCLK_DIV);
}

/**
 * \brief Set the divider of the AMBA Peripheral Bus.
 *
 */
__STATIC_INLINE void hw_cpm_set_pclk_div(uint32_t div) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_set_pclk_div(uint32_t div)
{
        GLOBAL_INT_DISABLE();
        REG_SETF(CRG_TOP, CLK_AMBA_REG, PCLK_DIV, div);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Check whether any of the Timer0 and 2 is active and uses the system clock.
 *
 * \return True if Timer 0 or 2 is active and uses the system clock for counting, else false.
 *
 */
__STATIC_INLINE bool hw_cpm_timer02_uses_sysclk(void)
{
        uint32_t regval;
        bool ret = true;

        do {
                regval = CRG_TOP->CLK_TMR_REG;

                // Check Timer0 clock
                if ((regval & REG_MSK(CRG_TOP, CLK_TMR_REG, TMR0_ENABLE)) &&
                                (regval & REG_MSK(CRG_TOP, CLK_TMR_REG, TMR0_CLK_SEL))) {
                        break;
                }

                // Check Timer2 clock
                if ((regval & REG_MSK(CRG_TOP, CLK_TMR_REG, TMR2_ENABLE)) &&
                                (regval & REG_MSK(CRG_TOP, CLK_TMR_REG, TMR2_CLK_SEL))) {
                        break;
                }

                ret = false;
        } while (0);

        return ret;
}

/**
 * \brief Check whether a MAC is active.
 *
 * \return True if MAC is active, else false.
 *
 */
__STATIC_INLINE bool hw_cpm_mac_is_active(void)
{
#ifdef CONFIG_USE_FTDF
        if (REG_GETF(CRG_TOP, SYS_STAT_REG, FTDF_IS_UP)) {
                return true;
        }
#endif

#ifdef CONFIG_USE_BLE
        if (REG_GETF(CRG_TOP, SYS_STAT_REG, BLE_IS_UP)) {
                return true;
        }
#endif

        return false;
}

/**
 * \brief Check whether the RC32K is the Low Power clock.
 *
 * \return True if RC32K is the LP clock, else false.
 *
 */
__STATIC_INLINE bool hw_cpm_lp_is_rc32k(void)
{
        return REG_GETF(CRG_TOP, CLK_32K_REG, RC32K_ENABLE) &&
              (REG_GETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE) == LP_CLK_IS_RC32K);
}

/**
 * \brief Set RCX as the Low Power clock.
 *
 * \warning The RCX must have been enabled before calling this function!
 */
__STATIC_INLINE void hw_cpm_lp_set_rcx(void)
{
        ASSERT_WARNING(REG_GETF(CRG_TOP, CLK_RCX20K_REG, RCX20K_ENABLE) == 1);

        REG_SETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE, LP_CLK_IS_RCX);
}

/**
 * \brief Set XTAL32K as the Low Power clock.
 *
 * \warning The XTAL32K must have been enabled before calling this function!
 *        Interrupts must have been disabled before calling this function!
 */
__STATIC_INLINE void hw_cpm_lp_set_xtal32k(void)
{
        ASSERT_WARNING(__get_PRIMASK() == 1);
        ASSERT_WARNING(REG_GETF(CRG_TOP, CLK_32K_REG, XTAL32K_ENABLE) == 1);

        REG_SETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE, LP_CLK_IS_XTAL32K);
}

/**
 * \brief Configure pin to connect an external digital clock.
 *
 */
void hw_cpm_configure_ext32k_pins(void);

/**
 * \brief Set an external digital clock as the Low Power clock.
 *
 * \warning Interrupts must have been disabled before calling this function!
 */
__STATIC_INLINE void hw_cpm_lp_set_ext32k(void)
{
        ASSERT_WARNING(__get_PRIMASK() == 1);

        REG_SETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE, LP_CLK_IS_EXTERNAL);
}

/**
 * \brief Enable RC32K.
 *
 */
__STATIC_INLINE void hw_cpm_enable_rc32k(void)
{
        REG_SET_BIT(CRG_TOP, CLK_32K_REG, RC32K_ENABLE);
}

/**
 * \brief Disable RC32K.
 *
 * \warning RC32K must not be the LP clock.
 *
 */
__STATIC_INLINE void hw_cpm_disable_rc32k(void)
{
        ASSERT_WARNING(REG_GETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE) != LP_CLK_IS_RC32K);

        REG_CLR_BIT(CRG_TOP, CLK_32K_REG, RC32K_ENABLE);
}

/**
 * \brief Set RC32K as the Low Power clock.
 *
 * \warning The RC32K must have been enabled before calling this function!
 */
__STATIC_INLINE void hw_cpm_lp_set_rc32k(void)
{
        ASSERT_WARNING(REG_GETF(CRG_TOP, CLK_32K_REG, RC32K_ENABLE) == 1);

        REG_SETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE, LP_CLK_IS_RC32K);
}

/**
 * \brief Configure RCX. This must be done only once since the register is retained.
 *
 */
__STATIC_INLINE void hw_cpm_configure_rcx(void)
{
        uint32_t reg;

        reg = CRG_TOP->CLK_RCX20K_REG;

        REG_SET_FIELD(CRG_TOP, CLK_RCX20K_REG, RCX20K_NTC, reg, 0xC); // 0x4C2
        REG_SET_FIELD(CRG_TOP, CLK_RCX20K_REG, RCX20K_BIAS, reg, 0); // Last review date: Feb 15, 2016 - 12:25:47
        REG_SET_FIELD(CRG_TOP, CLK_RCX20K_REG, RCX20K_TRIM, reg, 2);
        REG_SET_FIELD(CRG_TOP, CLK_RCX20K_REG, RCX20K_LOWF, reg, 1);

        CRG_TOP->CLK_RCX20K_REG = reg;
}

/**
 * \brief Enable RCX but does not set it as the LP clock.
 *
 */
__STATIC_INLINE void hw_cpm_enable_rcx(void)
{
        REG_SET_BIT(CRG_TOP, CLK_RCX20K_REG, RCX20K_ENABLE);
}

/**
 * \brief Disable RCX.
 *
 * \warning RCX must not be the LP clock
 *
 */
__STATIC_INLINE void hw_cpm_disable_rcx(void)
{
        ASSERT_WARNING(REG_GETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE) != LP_CLK_IS_RCX);

        REG_CLR_BIT(CRG_TOP, CLK_RCX20K_REG, RCX20K_ENABLE);
}

/**
 * \brief Configure XTAL32KX pins.
 *
 */
void hw_cpm_configure_xtal32k_pins(void);

/**
 * \brief Configure XTAL32KX. This must be done only once since the register is retained.
 *
 */
__STATIC_INLINE void hw_cpm_configure_xtal32k(void)
{
        uint32_t reg;

        // Configure xtal.
        reg = CRG_TOP->CLK_32K_REG;
        REG_SET_FIELD(CRG_TOP, CLK_32K_REG, XTAL32K_CUR, reg, 5);       // Last review date: Feb 15, 2016 - 12:25:47
        REG_SET_FIELD(CRG_TOP, CLK_32K_REG, XTAL32K_RBIAS, reg, 3);     // Last review date: Feb 15, 2016 - 12:25:47

        if (dg_configEXT_LP_IS_DIGITAL) {
                REG_SET_FIELD(CRG_TOP, CLK_32K_REG, XTAL32K_DISABLE_AMPREG, reg, 1);
        }
        else {
                REG_SET_FIELD(CRG_TOP, CLK_32K_REG, XTAL32K_DISABLE_AMPREG, reg, 0);
        }

        CRG_TOP->CLK_32K_REG = reg;
}

/**
 * \brief Enable XTAL32K but does not set it as the LP clock.
 *
 */
__STATIC_INLINE void hw_cpm_enable_xtal32k(void)
{
        REG_SET_BIT(CRG_TOP, CLK_32K_REG, XTAL32K_ENABLE);
}

/**
 * \brief Disable XTAL32K.
 *
 * \warning XTAL32K must not be the LP clock.
 *
 */
__STATIC_INLINE void hw_cpm_disable_xtal32k(void)
{
        ASSERT_WARNING(REG_GETF(CRG_TOP, CLK_CTRL_REG, CLK32K_SOURCE) != LP_CLK_IS_XTAL32K);

        REG_CLR_BIT(CRG_TOP, CLK_32K_REG, XTAL32K_ENABLE);
}

/**
 * \brief Check the status of a requested calibration.
 *
 * \return true if the calibration has finished (or never run) else false.
 *
 */
__STATIC_INLINE bool hw_cpm_calibration_finished(void)
{
        return REG_GETF(ANAMISC, CLK_REF_SEL_REG, REF_CAL_START) == 0;
}

/**
 * \brief Start calibration of a clock.
 *
 * \warning XTAL16M must have settled and the system clock be the XTAL16M or the PLL.
 *
 */
void hw_cpm_start_calibration(cal_clk_t clk_type, uint32_t cycles);

/**
 * \brief Return the calibration results.
 *
 * \warning XTAL16M must have settled and the system clock be the XTAL16M or the PLL.
 *
 */
uint32_t hw_cpm_get_calibration_data(void);

/**
 * \brief Check if the RC16M is the System Clock.
 *
 * \return 1 if the RC16M is the System Clock else 0.
 *
 */
uint32_t hw_cpm_sysclk_is_rc16(void);

/**
 * \brief Check if the XTAL16M is the System Clock.
 *
 * \return 1 if the XTAL16M is the System Clock else 0.
 *
 */
uint32_t hw_cpm_sysclk_is_xtal16m(void);

/**
 * \brief Setup system for a 32MHz / 16MHz external crystal.
 *
 * \param[in] freq true if 32MHz crystal is connected, false if 16MHz crystal is used
 *
 * \return void
 *
 */
void hw_cpm_set_divn(bool freq);

/**
 * \brief Check if the system clock can be switched to the RC16.
 *
 * \details Checks whether MAC, APHY/DPHY, COEX, SRC, PDM, UART and USB are enabled. If any of
 *          these blocks is enabled then the switching is not allowed. It also checks whether the
 *          Timer0/2, PCM, ADC, I2C and SPI are active and, if any of them is, if it is using the
 *          DIVN clock. In this case the switching is not allowed.
 *
 * \return true if it is allowed to switch to RC16 else false.
 *
 */
bool hw_cpm_is_rc16_allowed(void);

/**
 * \brief Set System clock.
 *
 * \param[in] mode The new system clock.
 *
 */
void hw_cpm_set_sysclk(uint32_t mode);

/**
 * \brief Add a short delay loop.
 *
 */
void hw_cpm_short_delay(void);

/**
 * \brief Enable the PLL.
 *
 */
void hw_cpm_pll_sys_on(void);

/**
 * \brief Disable the PLL.
 *
 * \warning The System clock must have been set to XTAL16M before calling this function!
 *
 */
void hw_cpm_pll_sys_off(void);

/**
 * \brief Enable the 3V3 clamp.
 *
 */
__STATIC_INLINE void hw_cpm_3v3_clamp_on(void)
{
        REG_SET_BIT(CRG_TOP, AON_SPARE_REG, EN_BATSYS_RET);
}

/**
 * \brief Disable the 3V3 clamp.
 *
 */
__STATIC_INLINE void hw_cpm_3v3_clamp_off(void)
{
        REG_CLR_BIT(CRG_TOP, AON_SPARE_REG, EN_BATSYS_RET);
}

/**
 * \brief Enable OSC16M amplitude regulation
 */
__STATIC_INLINE void hw_cpm_enable_osc16m_amp_reg(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_enable_osc16m_amp_reg(void)
{
        REG_CLR_BIT(CRG_TOP, AON_SPARE_REG, OSC16_HOLD_AMP_REG);
}

/**
 * \brief Disable OSC16M amplitude regulation
 */
__STATIC_INLINE void hw_cpm_disable_osc16m_amp_reg(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_disable_osc16m_amp_reg(void)
{
        REG_SET_BIT(CRG_TOP, AON_SPARE_REG, OSC16_HOLD_AMP_REG);
}

/**
 * \brief Sets the state of the 1v8 rail.
 *
 * \param[in] state true, the 1v8 rail is controlled via dg_config macros
 *                  false, the 1v8 rail is off
 *
 */
void hw_cpm_set_1v8_state(bool state);

/**
 * \brief Returns the state of the 1v8 rail.
 *
 * \return false if the 1v8 rail is off, true if it is controlled via dg_config macros
 *
 */
__RETAINED_CODE bool hw_cpm_get_1v8_state(void);

/**
 * \brief Enable the LDO_VBAT_RET.
 *
 */
__STATIC_INLINE void hw_cpm_ldo_vbat_ret_on(void)
{
        REG_CLR_BIT(CRG_TOP, LDO_CTRL2_REG, LDO_VBAT_RET_DISABLE);
}

/**
 * \brief Disable the LDO_VBAT_RET.
 *
 */
__STATIC_INLINE void hw_cpm_ldo_vbat_ret_off(void)
{
        REG_SET_BIT(CRG_TOP, LDO_CTRL2_REG, LDO_VBAT_RET_DISABLE);
}

/**
 * \brief Disable the LDO_IO_RET and the LDO_IO2_RET.
 *
 */
__STATIC_INLINE void hw_cpm_ldo_io_ret_off(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_ldo_io_ret_off(void)
{
        uint32_t reg = CRG_TOP->LDO_CTRL2_REG;

        if (dg_configPOWER_1V8_SLEEP == 1) {
                if (dg_configUSE_BOD == 1) {
                        REG_CLR_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_FLASH_EN);
                }
                REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_RET_DISABLE, reg, 1);
        }

        if (dg_configPOWER_1V8P == 1) {
                if (dg_configUSE_BOD == 1) {
                        REG_CLR_BIT(CRG_TOP, BOD_CTRL2_REG, BOD_1V8_PA_EN);
                }
                REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_PA_RET_DISABLE, reg, 1);
        }

        CRG_TOP->LDO_CTRL2_REG = reg;
}

/*
 * \brief Set the LDO_RADIO_SETVDD to the proper level (0x2 = 1.40V)
 */
__STATIC_INLINE void hw_cpm_reset_radio_vdd(void)
{
        REG_SETF(CRG_TOP, LDO_CTRL1_REG, LDO_RADIO_SETVDD, 0x2);
}

/**
 * \brief Enable the LDOs.
 *
 */
__STATIC_INLINE void hw_cpm_start_ldos(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_start_ldos(void)
{
        uint32_t reg = CRG_TOP->LDO_CTRL2_REG;
        if ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_B) &&
                (dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_A)) {
                REG_SETF(CRG_TOP, LDO_CTRL1_REG, LDO_VBAT_RET_LEVEL, 0);
        }
        REG_SET_BIT(CRG_TOP, LDO_CTRL1_REG, LDO_RADIO_ENABLE);
        REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V2_ON, reg, 1);

        if ((dg_configPOWER_1V8_ACTIVE == 1) && hw_cpm_get_1v8_state()) {
                REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_ON, reg, 1);
                if (dg_configPOWER_1V8_SLEEP == 0) {
                        REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_RET_DISABLE, reg, 1);
                }
                else {
                        REG_CLR_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_RET_DISABLE, reg);
                }
        }
        else {
                REG_CLR_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_ON, reg);
                REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_RET_DISABLE, reg, 1);
        }

        if (dg_configPOWER_1V8P == 1) {
                REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_PA_ON, reg, 1);
                REG_CLR_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_PA_RET_DISABLE, reg);
        }
        else {
                REG_CLR_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_PA_ON, reg);
                REG_SET_FIELD(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_PA_RET_DISABLE, reg, 1);
        }

        CRG_TOP->LDO_CTRL2_REG = reg;
}

/**
 * \brief Configure the DCDC.
 *
 */
void hw_cpm_dcdc_config(void);

/**
 * \brief Enable the DCDC.
 *
 */
void hw_cpm_dcdc_on(void);

/**
 * \brief Prepare the DCDC for sleep.
 *
 */
__STATIC_INLINE void hw_cpm_dcdc_sleep(void) __attribute__((always_inline));

void hw_cpm_dcdc_sleep(void)
{
        // Deactivate DCDC 1V4 output rail
        DCDC->DCDC_V14_1_REG &= ~(REG_MSK(DCDC, DCDC_V14_1_REG, DCDC_V14_ENABLE_HV) |
                                  REG_MSK(DCDC, DCDC_V14_1_REG, DCDC_V14_ENABLE_LV));

        // Set DCDC to sleep mode
        REG_SETF(DCDC, DCDC_CTRL_0_REG, DCDC_MODE, 2);

        // Disable Retention LDOs for 1V8 and 1V8P
        CRG_TOP->LDO_CTRL2_REG |= ((1 << REG_POS(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_PA_RET_DISABLE)) |
                                   (1 << REG_POS(CRG_TOP, LDO_CTRL2_REG, LDO_1V8_FLASH_RET_DISABLE)));

        // Enable LDO_CORE before going to sleep
        hw_cpm_turn_1_2V_on();
}

/**
 * \brief Disable the DCDC.
 *
 */
__STATIC_INLINE void hw_cpm_dcdc_off(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_dcdc_off(void)
{
        // Enable the LDOs
        hw_cpm_start_ldos();

        // Turn off DCDC
        DCDC->DCDC_CTRL_0_REG &= ~REG_MSK(DCDC, DCDC_CTRL_0_REG, DCDC_MODE);
}

/**
 * \brief Check if DCDC is active.
 *
 * \return True, if the DCDC is active else false.
 *
 */
__STATIC_INLINE bool hw_cpm_dcdc_is_active(void) __attribute__((always_inline));

__STATIC_INLINE bool hw_cpm_dcdc_is_active(void)
{
        return (REG_GETF(DCDC, DCDC_CTRL_0_REG, DCDC_MODE) == 1);
}

/**
 * \brief Disable the DCDC and switch to LDOs without turning off the LDO_RADIO.
 *
 */
__STATIC_INLINE void hw_cpm_switch_to_ldos(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_switch_to_ldos(void)
{
        // Enable the LDOs
        hw_cpm_start_ldos();

        // Turn off DCDC
        DCDC->DCDC_CTRL_0_REG &= ~REG_MSK(DCDC, DCDC_CTRL_0_REG, DCDC_MODE);
}


/**
 * \brief Set the configured time for the re-charging of the retention LDOs and the DCDC rails.
 *
 */
__STATIC_INLINE void hw_cpm_set_recharge_period(uint16_t period)
{
        CRG_TOP->SLEEP_TIMER_REG = period;
}

/**
 * \brief Reset the time for the re-charging of the retention LDOs and the DCDC rails to zero.
 *
 */
__STATIC_INLINE void hw_cpm_reset_recharge_period(void)
{
        // Set the DCDC charge period
        CRG_TOP->SLEEP_TIMER_REG = 0;
}

/**
 * \brief Set (part of) the preferred settings.
 *
 */
void hw_cpm_set_preferred_values(void);

/**
 * \brief Set the GPIO used for the SW cursor to High-Z.
 *
 */
__STATIC_INLINE void hw_cpm_setup_sw_cursor(void)
{
        if (dg_configUSE_SW_CURSOR == 1) {
                SW_CURSOR_GPIO = 0x000;
        }
}

/**
 * \brief Triggers the GPIO used for the SW cursor.
 *
 */
void hw_cpm_trigger_sw_cursor(void);

/**
 * \brief Stop the clock to the RF unit.
 *
 */
__STATIC_INLINE void hw_cpm_rfcu_clk_off(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_rfcu_clk_off(void)
{
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, CLK_RADIO_REG, RFCU_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Enable the debugger.
 *
 */
__STATIC_INLINE void hw_cpm_enable_debugger(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_enable_debugger(void)
{
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, SYS_CTRL_REG, DEBUGGER_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Disable the debugger.
 *
 */
__STATIC_INLINE void hw_cpm_disable_debugger(void) __attribute__((always_inline));

__STATIC_INLINE void hw_cpm_disable_debugger(void)
{
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, SYS_CTRL_REG, DEBUGGER_ENABLE);
        GLOBAL_INT_RESTORE();
}

/**
 * \brief Check if the debugger is attached.
 *
 * \return true, if the debugger is attached, else false.
 *
 */
__STATIC_INLINE bool hw_cpm_is_debugger_attached(void) __attribute__((always_inline));

__STATIC_INLINE bool hw_cpm_is_debugger_attached(void)
{
        return (REG_GETF(CRG_TOP, SYS_STAT_REG, DBG_IS_ACTIVE) != 0);
}


/**
 * \brief Check if any DMA channel is active.
 *
 * \return true if a channel is active, false if all channels are inactive.
 *
 */
__STATIC_INLINE bool hw_cpm_check_dma(void)
{
        bool ret = false;

        if (((DMA->DMA0_CTRL_REG & REG_MSK(DMA, DMA0_CTRL_REG, DMA_ON)) == 1) ||
            ((DMA->DMA1_CTRL_REG & REG_MSK(DMA, DMA1_CTRL_REG, DMA_ON)) == 1) ||
            ((DMA->DMA2_CTRL_REG & REG_MSK(DMA, DMA2_CTRL_REG, DMA_ON)) == 1) ||
            ((DMA->DMA3_CTRL_REG & REG_MSK(DMA, DMA3_CTRL_REG, DMA_ON)) == 1) ||
            ((DMA->DMA4_CTRL_REG & REG_MSK(DMA, DMA4_CTRL_REG, DMA_ON)) == 1) ||
            ((DMA->DMA5_CTRL_REG & REG_MSK(DMA, DMA5_CTRL_REG, DMA_ON)) == 1) ||
            ((DMA->DMA6_CTRL_REG & REG_MSK(DMA, DMA6_CTRL_REG, DMA_ON)) == 1) ||
            ((DMA->DMA7_CTRL_REG & REG_MSK(DMA, DMA7_CTRL_REG, DMA_ON))== 1) ){
                ret = true;
        }

        return ret;
}

/**
 * \brief   Issue a HW reset (due to a fault condition).
 *
 * \details A HW reset (WDOG) will be generated. The NMI Handler will be called and "status" will be
 *          stored in the Retention RAM.
 *
 */
__RETAINED_CODE void hw_cpm_reset_system(void);

/**
 * \brief   Issue a HW reset (intentionally, i.e. after a SW upgrade).
 *
 * \details A HW reset (WDOG) will be generated. The NMI Handler is bypassed. Thus, no "status" is
 *          stored in the Retention RAM.
 *
 */
__RETAINED_CODE void hw_cpm_reboot_system(void);

/**
 * \brief Add delay of N usecs.
 *
 * \param[in] usec The number of usecs to wait for.
 *
 * \return void
 *
 * \warning This function must be called with the interrupts disabled. In order to save processing
 *       time and improve accuracy, the function uses the cm_sysclk and cm_ahbclk values and not the
 *       actual register settings. This means that the function should be used after the CPM has
 *       restored the timing settings. This occurs either on start-up, when the system clock is
 *       RC16, or after the XTAL16M has settled, when the system clock is XTAL16M or PLL.
 *       Therefore, it is important that the platform (either the CPM or some other entity) has
 *       configured values sys_clk_t cm_sysclk and ahb_div_t cm_ahbclk
 *
 */
__RETAINED_CODE void hw_cpm_delay_usec(uint32_t usec);

/**
 * \brief  Trigger a GPIO when ASSERT_WARNING() or ASSERT_ERROR() hits.
 *
 */
__RETAINED_CODE void hw_cpm_assert_trigger_gpio(void);

#endif /* dg_configUSE_HW_CPM */

#endif /* CPM_H_ */

/**
\}
\}
\}
*/
