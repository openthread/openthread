/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup RF
 * \{
 * \brief Radio Control
 */

/**
 *****************************************************************************************
 *
 * @file hw_rf.h
 *
 * @brief Radio module (RF) Low Level Driver API.
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
 *****************************************************************************************
 */

#ifndef HW_RF_H_
#define HW_RF_H_

#if dg_configUSE_HW_RF

#include <stdbool.h>
#include "sdk_defs.h"

#include "hw_cpm.h"

#if dg_configFEM == FEM_SKY66112_11
#include "hw_fem_sky66112-11.h"
#endif

typedef struct __attribute__ ((__packed__)) {
        uint8_t tx_power_ble: 4;
        uint8_t tx_power_ftdf: 4;
} hw_rf_tx_power_luts_t;

extern hw_rf_tx_power_luts_t rf_tx_power_luts;

/**
 * \brief Power LUT setting
 *
 */
typedef enum {
        HW_RF_PWR_LUT_0dbm = 0,   /**< TX PWR attenuation 0 dbm */
        HW_RF_PWR_LUT_m1dbm = 1,  /**< TX PWR attenuation -1 dbm */
        HW_RF_PWR_LUT_m2dbm = 2,  /**< TX PWR attenuation -2 dbm */
        HW_RF_PWR_LUT_m3dbm = 3,  /**< TX PWR attenuation -3 dbm */
        HW_RF_PWR_LUT_m4dbm = 4,  /**< TX PWR attenuation -4 dbm */
} HW_RF_PWR_LUT_SETTING;

/**
 * \brief Initializes RF system, and performs the initial calibration
 *
 * \note The RF PD must be on before this is called
 *
 * \return True, if iff calib is successful, false otherwise
 */
bool hw_rf_system_init(void);

/**
 * \brief Sets parameters according to their recommended values.
 */
void hw_rf_set_recommended_settings(void);

/**
 * \brief (Re)calibrates RF DC offset.
 */
void hw_rf_dc_offset_calibration(void);

/**
 * \brief (Re)calibrates modulation gain.
 */
void hw_rf_modulation_gain_calibration(bool);

/**
 * \brief Convenience function to (re)calibrate all RF related modules.
 *
 * \return True if calibration (specifically, the iff calibration part)
 *         succeeds, false if not
 */
bool hw_rf_calibration(void);

/**
 * \brief Start Calibration procedure and return.
 *
 * This will block for some time in order to perform
 * the first part of calibration (IFF, DC offset and the start of gain calib).
 * Interrupts must be disabled by the caller.
 *
 * \return True if calibration was successful, false if not (i.e. iff calib hang)
 *
 */
bool hw_rf_start_calibration(void);

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
/**
 * \brief Set TX Power
 *
 * This actually sets the index of the RF_TX_PWER_LUT_X_REG to use.
 *
 * \param [in] lut The TX power attenuation setting
 *
 * \warning Do not call this function before recommended settings are applied
 */
void hw_rf_set_tx_power(HW_RF_PWR_LUT_SETTING lut);
#else

#ifdef CONFIG_USE_BLE
/**
 * \brief Set TX Power for BLE
 *
 * This actually sets the index of the RF_TX_PWR_LUT_X_REG to use.
 *
 * \param [in] lut The TX power attenuation setting
 *
 * \warning Do not call this function before recommended settings are applied
 */
void hw_rf_set_tx_power_ble(HW_RF_PWR_LUT_SETTING lut);
#endif

#ifdef CONFIG_USE_FTDF
/**
 * \brief Set TX Power for FTDF
 *
 * This actually sets the index of the RF_TX_PWR_LUT_X_REG to use.
 *
 * \param [in] lut The TX power attenuation setting
 *
 * \warning Do not call this function before recommended settings are applied
 */
void hw_rf_set_tx_power_ftdf(HW_RF_PWR_LUT_SETTING lut);
#endif

/**
 * \brief Set TX Power
 *
 * This actually sets the index of the RF_TX_PWR_LUT_X_REG to use.
 *
 * \param [in] lut The TX power attenuation setting
 *
 * \deprecated This function is deprecated since it can only set
 *  BLE and FTDF TX power with the same value. Use hw_rf_set_tx_power_ble()
 *  and hw_rf_set_tx_power_ftdf() instead.
 *
 * \warning Do not call this function before recommended settings are applied
 */
static inline void hw_rf_set_tx_power(HW_RF_PWR_LUT_SETTING lut)
{
#ifdef CONFIG_USE_BLE
        hw_rf_set_tx_power_ble(lut);
#endif
#ifdef CONFIG_USE_FTDF
        hw_rf_set_tx_power_ftdf(lut);
#endif
}
#endif

/**
 * \brief Turns on RF module.
 */
static inline void hw_rf_poweron(void)  __attribute__((always_inline));

static inline void hw_rf_poweron(void)
{
        if ((dg_configUSE_BOD == 1) && ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                || ((dg_configUSE_AUTO_CHIP_DETECTION == 1) && (CHIP_IS_AE)))) {
                hw_cpm_deactivate_bod_protection();
        }

        /* If PD_RAD is up, make sure to power it down to issue a reset */
        if (REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_UP)) {
                if ((dg_configUSE_BOD == 1) && ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                        || ((dg_configUSE_AUTO_CHIP_DETECTION == 1) && (CHIP_IS_AE)))) {
                        hw_cpm_delay_usec(30);
                }
                GLOBAL_INT_DISABLE();
                REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);
                GLOBAL_INT_RESTORE();
                while (REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_DOWN) == 0x0);
        }

        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);
        GLOBAL_INT_RESTORE();
        while (!REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_UP));

        if ((dg_configUSE_BOD == 1) && ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                || ((dg_configUSE_AUTO_CHIP_DETECTION == 1) && (CHIP_IS_AE)))) {
                hw_cpm_delay_usec(30);
                hw_cpm_activate_bod_protection();
        }

        // Enable the PLLdig/RFCU clock
        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, CLK_RADIO_REG, RFCU_ENABLE);
        REG_SETF(CRG_TOP, CLK_RADIO_REG, RFCU_DIV, 1);
        GLOBAL_INT_RESTORE();

#if dg_configFEM == FEM_SKY66112_11
        hw_fem_start();
#endif
}

/**
 * \brief Turns off RF module.
 */
static inline void hw_rf_poweroff()
{
#if dg_configFEM == FEM_SKY66112_11
        hw_fem_stop();
#endif

        if ((dg_configUSE_BOD == 1) && ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                || ((dg_configUSE_AUTO_CHIP_DETECTION == 1) && (CHIP_IS_AE)))) {
                hw_cpm_deactivate_bod_protection();
                hw_cpm_delay_usec(30);
        }

        GLOBAL_INT_DISABLE();
        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);
        GLOBAL_INT_RESTORE();
        while (!REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_DOWN));

        if ((dg_configUSE_BOD == 1) && ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A)
                || ((dg_configUSE_AUTO_CHIP_DETECTION == 1) && (CHIP_IS_AE)))) {
                hw_cpm_delay_usec(30);
                hw_cpm_activate_bod_protection();
        }
}

/**
 * \brief Sets parameters according to their recommended values, taking RF state into account.
 *
 * Acts like \ref hw_rf_set_recommended_settings but makes sure that the RF power domain is on and
 * unconfigured.
 * Interrupts must be disabled by the caller.
 *
 */
void hw_rf_request_recommended_settings(void);

/**
 * \brief Requests that the RF is turned on
 *
 * Requests that the RF is turned on, if not already on.
 * Interrupts must be disabled by the caller.
 *
 * \param [in] mode_ble True, if the rf is needed for ble
 */
__RETAINED_CODE void hw_rf_request_on(bool mode_ble);

/**
 * \brief Requests that the RF is turned off
 *
 * Requests that the RF is turned off, if not already off.
 * The RF will be turned off only if there are no more
 * requests (ie. all requesters have called hw_rf_request_off())
 * Interrupts must be disabled by the caller.
 *
 * \param [in] mode_ble True, if the rf was needed for ble
 */
void hw_rf_request_off(bool mode_ble);

/**
 * \brief Start transmitting a continuous wave (unmodulated transmission)
 *
 * \param [in] mode is the mode to use. 1: BLE, 2 or 3: FTDF (0: Normal, use hw_rf_stop_*)
 *
 * \param [in] ch is the Channel to transmit on, calculated as:
 *             (mode is BLE)  ch = (F – 2402) / 2, where F ranges from 2402 MHz to 2480 MHz.
 *                            Range: 0x00 – 0x27.
 *             (mode is FTDF) ch = (F – 2405) / 5, where F ranges from 2405 MHz to 2480 MHz.
 *                            Range: 0x00 – 0xf.
 */
void hw_rf_start_continuous_wave(uint8_t mode, uint8_t ch);

/**
 * \brief Start reception of a continuous wave (unmodulated reception)
 *
 * \param [in] mode is the mode to use. 1: BLE, 2 or 3: FTDF (0: Normal, use hw_rf_stop_*)
 *
 * \param [in] ch is the Channel to receive on, calculated as:
 *             (mode is BLE)  ch = (F – 2402) / 2, where F ranges from 2402 MHz to 2480 MHz.
 *                            Range: 0x00 – 0x27.
 *             (mode is FTDF) ch = (F – 2405) / 5, where F ranges from 2405 MHz to 2480 MHz.
 *                            Range: 0x00 – 0xf.
 */
void hw_rf_start_continuous_wave_rx(uint8_t mode, uint8_t ch);

/**
 * \brief Stop transmitting a continuous wave (unmodulated transmission)
 *
 */
void hw_rf_stop_continuous_wave(void);

#endif /* dg_configUSE_HW_RF */

#endif /* HW_RF_H_ */

/**
 * \}
 * \}
 * \}
 */
