/* Copyright (c) 2017, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef NRF_DRIVER_RADIO802154_CONFIG_H__
#define NRF_DRIVER_RADIO802154_CONFIG_H__

#include <nrf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_driver_radio802154_config 802.15.4 driver configuration
 * @{
 * @ingroup nrf_driver_radio802154
 * @brief Configuration of 802.15.4 radio driver for nRF SoC.
 */

/*******************************************************************************
 * @section Radio Driver Configuration.
 ******************************************************************************/

/**
 * @def RADIO_CCA_MODE
 *
 * RADIO CCA Mode.
 *
 */
#define RADIO_CCA_MODE  NRF_RADIO_CCA_MODE_ED

/**
 * @def RADIO_CCA_ED_THRESHOLD
 *
 * RADIO Energy Detection Threshold.
 *
 */
#define RADIO_CCA_ED_THRESHOLD  0x2D

/**
 * @def RADIO_CCA_CORR_THRESHOLD
 *
 * RADIO Correlator Threshold.
 *
 */
#define RADIO_CCA_CORR_THRESHOLD  0x2D

/**
 * @def RADIO_CCA_CORR_LIMIT
 *
 * RADIO Correlator limit.
 *
 */
#define RADIO_CCA_CORR_LIMIT  0x02

/**
 * @def RADIO_IRQ_PRIORITY
 *
 * RADIO Interrupt priority.
 *
 */
#define RADIO_IRQ_PRIORITY  0

/**
 * @def RADIO_PENDING_SHORT_ADDRESSES
 *
 * RADIO Number of slots containing short addresses of nodes for which pending data is stored.
 *
 */
#define RADIO_PENDING_SHORT_ADDRESSES  10

/**
 * @def RADIO_PENDING_EXTENDED_ADDRESSES
 *
 * RADIO Number of slots containing extended addresses of nodes for which pending data is stored.
 *
 */
#define RADIO_PENDING_EXTENDED_ADDRESSES  10

/**
 * @def RADIO_RX_BUFFERS
 *
 * RADIO Number of buffers in receive queue.
 *
 */
#define RADIO_RX_BUFFERS  16

/**
 * @def RADIO_SHORT_CCAIDLE_TXEN
 *
 * RADIO can start transmission using short CCAIDLE->TXEN or using software interrupt handler.
 *
 */
#define RADIO_SHORT_CCAIDLE_TXEN  1

// TODO: #ifdef to check direct / SWI notifications
/**
 * @def RADIO_NOTIFICATION_SWI_PRIORITY
 *
 * Priority of software interrupt used to call notification from 802.15.4 driver.
 *
 */
#define RADIO_NOTIFICATION_SWI_PRIORITY 6

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_DRIVER_RADIO802154_CONFIG_H__
