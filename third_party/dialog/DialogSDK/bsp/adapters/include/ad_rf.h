/**
 * \addtogroup BSP
 * \{
 * \addtogroup ADAPTERS
 * \{
 * \addtogroup RF_ADAPTER
 *
 * \brief RF adapter
 *
 * \{
 */

/**
 *****************************************************************************************
 *
 * @file
 *
 * @brief Radio module access API.
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




#ifndef AD_RF_H_
#define AD_RF_H_

#if dg_configRF_ADAPTER

#include <stdbool.h>
#include <stdint.h>

#include "hw_rf.h"

/**
 * @brief Performs RF adapter initialization
 */
void ad_rf_init(void);

/**
 * \brief Retry a failed calibration
 *
 * This will power-cycle RF, reapply tcs and recommended settings, and
 * retry calibration. If calibration fails again, it will reset the system
 * (using the wdog)
 */
void ad_rf_retry_calibration();

/**
 * \brief Start Calibration procedure and check if it succeeds
 *
 * This will start the calibration procedure, and check if the calibration initial
 * part (the iff calibration) succeeds. If not, it will reset the RF block and retry.
 * If the calibration still fails after the second attempt, it will trigger a
 * watchdog reset
 *
 */
static inline void ad_rf_start_and_check_calibration()
{
        if (!hw_rf_start_calibration())
                 ad_rf_retry_calibration();
}

/**
 * \brief Perform RF system initialization.
 *
 * This will preform a full RF system init, and check if the
 * calibration initial part (the iff calibration) succeeds. If not, it will
 * reset the RF block and retry.
 * If the calibration still fails after the second attempt, it will trigger a
 * watchdog reset
 *
 */
static inline void ad_rf_system_init()
{
        if (!hw_rf_system_init())
                 ad_rf_retry_calibration();
}

/**
 * \brief Start Calibration procedure and return.
 *
 * This will block for some time (with interrupts disabled) in order to perform
 * The first part of calibration (IFF, DC offset and the start of gain calib).
 *
 */
static inline void ad_rf_start_calibration()
{
        ad_rf_start_and_check_calibration();
}

/**
 * \brief Sets parameters according to their recommended values, taking RF state into account.
 *
 * Acts like \ref hw_rf_set_recommended_settings but makes sure that the RF power domain is on and
 * unconfigured. Disables interrupts.
 *
 */
static inline void ad_rf_request_recommended_settings(void)
{
        hw_rf_request_recommended_settings();
}

/**
 * \brief Requests that the RF is turned on
 *
 * Requests that the RF is turned on, if not already on. Disables interrupts.
 *
 * \param [in] mode_ble True, if the rf is needed for ble
 *
 */
static inline void ad_rf_request_on(bool mode_ble)
{
        hw_rf_request_on(mode_ble);
}

/**
 * \brief Requests that the RF is turned off
 *
 * Requests that the RF is turned off, if not already off.
 * The RF will be turned off only if there are no more
 * requests (ie. all requesters have called ad_rf_request_off())
 * Disables interrupts
 *
 * \param [in] mode_ble True, if the rf was needed for ble
 */
static inline void ad_rf_request_off(bool mode_ble)
{
        hw_rf_request_off(mode_ble);
}

#endif /* dg_configRF_ADAPTER */

#endif /* AD_RF_H_ */

/**
 * \}
 * \}
 * \}
 */
