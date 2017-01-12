/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup RF
 * \{
 */

/**
 *****************************************************************************************
 *
 * @file hw_rf.h
 *
 * @brief Radio module (RF) Low Level Driver API.
 *
 * @note The following recalibration-related weak functions can be overridden, if needed, 
 *       to provide additional functionality
 *
 * @note
 * ~~~{.c}
 * bool hw_rf_preoff_cb(void)
 * ~~~
 *
 * @note Called before actually shutting down the RF PD. If this returns true, the PD
 *       will NOT be shutdown, but will stay on. This function can be used to decide
 *       whether an RF recalibration is needed and to start the respective operation.
 *       The default implementation (if an explicit implementation is omitted) returns
 *       false (i.e. the RF PD shuts off immediately).
 *
 * @note 
 * ~~~{.c}
 * void hw_rf_postconf_cb(void)
 * ~~~
 *
 * @note Called after the RF recommended settings are applied, or after the
 *       recalibration procedure is completed. Can be used to start/reset a
 *       recalibration timer, in case periodic recalibration is enabled using
 *       dg_configRF_RECALIBRATION_TIMER_TIMEOUT
 *
 * @note 
 * ~~~{.c}
 * void hw_rf_precalib_cb(void)
 * void hw_rf_postcalib_cb(void):
 * ~~~
 *
 * @note Called when the re-calibration (not the initial calibration) procedure
 *       starts/ends. Can be used to instruct the system not to go to sleep during
 *       this time.
 *
 * @note
 * ~~~{.c}
 * void hw_rf_apply_tcs_cb(void)
 * ~~~
 *
 * @note Called before applying the rf recommended settings. The implementation should
 *       apply the TCS values
 *
 * @note
 * ~~~{.c}
 * uint32_t hw_rf_get_start_iff_time(void)
 * ~~~
 *
 * @note Called to get the time when IFF calibration starts
 *
 * @note
 * ~~~{.c}
 * bool hw_rf_check_iff_timeout(uint32_t start_time)
 * ~~~
 *
 * @note Called to check if IFF calibration has timed-out (i.e. took too long). It takes argument the
 *       IFF calib start_time, as return by hw_rf_get_start_iff_time(). It should normally check against
 *       config macro dg_configRF_IFF_CALIBRATION_TIMEOUT.
 *
 * @warning All the above functions are called in a critical section. They should not block.
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
#ifndef HW_RF_H_
#define HW_RF_H_

#if dg_configUSE_HW_RF

#include <stdbool.h>
#include "black_orca.h"

#include "hw_cpm.h"

#if dg_configFEM == FEM_SKY66112_11
#include "hw_fem_sky66112-11.h"
#endif

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
 * \brief (Re)calibrates Intermediate Frequency Filter (IFF).
 */
bool hw_rf_iff_calibration(void);

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
bool hw_rf_start_calibration();

/**
 * \brief Turns on RF module.
 */
static inline void hw_rf_poweron(void)  __attribute__((always_inline));

static inline void hw_rf_poweron(void)
{
#if ( (dg_configUSE_BOD == 1) && (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
        && (dg_configBLACK_ORCA_IC_STEP <= BLACK_ORCA_IC_STEP_D))
        hw_cpm_deactivate_bod_protection();
#endif

        /* If PD_RAD is up, make sure to power it down to issue a reset */
        if (REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_UP)) {
#if ( (dg_configUSE_BOD == 1) && (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
        && (dg_configBLACK_ORCA_IC_STEP <= BLACK_ORCA_IC_STEP_D))
                hw_cpm_delay_usec(30);
#endif
                REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);
                while (REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_DOWN) == 0x0);
        }

        REG_CLR_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);
        while (!REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_UP));

#if ( (dg_configUSE_BOD == 1) && (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
        && (dg_configBLACK_ORCA_IC_STEP <= BLACK_ORCA_IC_STEP_D))
                hw_cpm_delay_usec(30);
                hw_cpm_activate_bod_protection();
#endif

        // Enable the PLLdig/RFCU clock
        REG_SET_BIT(CRG_TOP, CLK_RADIO_REG, RFCU_ENABLE);
        REG_SETF(CRG_TOP, CLK_RADIO_REG, RFCU_DIV, 1);

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

#if ( (dg_configUSE_BOD == 1) && (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
        && (dg_configBLACK_ORCA_IC_STEP <= BLACK_ORCA_IC_STEP_D))
        hw_cpm_deactivate_bod_protection();
        hw_cpm_delay_usec(30);
#endif

        REG_SET_BIT(CRG_TOP, PMU_CTRL_REG, RADIO_SLEEP);
        while (!REG_GETF(CRG_TOP, SYS_STAT_REG, RAD_IS_DOWN));

#if ( (dg_configUSE_BOD == 1) && (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
        && (dg_configBLACK_ORCA_IC_STEP <= BLACK_ORCA_IC_STEP_D))
        hw_cpm_delay_usec(30);
        hw_cpm_activate_bod_protection();
#endif
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

/*
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

/*
 * \brief Stop transmitting a continuous wave (unmodulated transmission)
 *
 */
void hw_rf_stop_continuous_wave(void);

/*
 * \brief Set TX Power
 *
 * This actually sets the index of the RF_TX_PWER_LUT_X_REG to use.
 *
 * \param [in] lut The index of the LUT to use. 0 disables this functionality.
 *             1: 0dbm, 2: -1dbm, 3: -2dbm, 4: -3dbm, 5: -4dbm
 */
static inline void hw_rf_set_tx_power(uint8_t lut)
{
        RFCU->RF_TX_PWR_REG = lut;
}


#endif /* dg_configUSE_HW_RF */

#endif /* HW_RF_H_ */

/**
 * \}
 * \}
 * \}
 */
