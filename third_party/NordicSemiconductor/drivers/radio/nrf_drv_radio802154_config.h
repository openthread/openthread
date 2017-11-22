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

#ifdef NRF_DRV_RADIO802154_PROJECT_CONFIG
#include NRF_DRV_RADIO802154_PROJECT_CONFIG
#endif

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
 * @def NRF_DRV_RADIO802154_CCA_MODE
 *
 * CCA Mode used by the driver.
 *
 */
#ifndef NRF_DRV_RADIO802154_CCA_MODE_DEFAULT
#define NRF_DRV_RADIO802154_CCA_MODE_DEFAULT  NRF_RADIO_CCA_MODE_ED
#endif

/**
 * @def NRF_DRV_RADIO802154_CCA_ED_THRESHOLD
 *
 * Energy Detection Threshold used in CCA procedure.
 *
 */
#ifndef NRF_DRV_RADIO802154_CCA_ED_THRESHOLD_DEFAULT
#define NRF_DRV_RADIO802154_CCA_ED_THRESHOLD_DEFAULT  0x2D
#endif

/**
 * @def NRF_DRV_RADIO802154_CCA_CORR_THRESHOLD
 *
 * Correlator Threshold used in CCA procedure.
 *
 */
#ifndef NRF_DRV_RADIO802154_CCA_CORR_THRESHOLD_DEFAULT
#define NRF_DRV_RADIO802154_CCA_CORR_THRESHOLD_DEFAULT  0x2D
#endif

/**
 * @def NRF_DRV_RADIO802154_CCA_CORR_LIMIT
 *
 * Correlator limit used in CCA procedure.
 *
 */
#ifndef NRF_DRV_RADIO802154_CCA_CORR_LIMIT_DEFAULT
#define NRF_DRV_RADIO802154_CCA_CORR_LIMIT_DEFAULT  0x02
#endif

/**
 * @def NRF_DRV_RADIO802154_INTERNAL_IRQ_HANDLING
 *
 * If the driver should internally handle the RADIO IRQ.
 * In case the driver is used in an OS the RADIO IRQ may be handled by the OS and passed to
 * the driver @sa nrf_drv_radio802154_irq_handler(). In this case internal handling should be
 * disabled.
 */

#ifndef NRF_DRV_RADIO802154_INTERNAL_IRQ_HANDLING

#if RAAL_SOFTDEVICE
#define NRF_DRV_RADIO802154_INTERNAL_IRQ_HANDLING 0
#else // RAAL_SOFTDEVICE
#define NRF_DRV_RADIO802154_INTERNAL_IRQ_HANDLING 1
#endif // RAAL_SOFTDEVICE

#endif // NRF_DRV_RADIO802154_INTERNAL_IRQ_HANDLING

/**
 * @def NRF_DRV_RADIO802154_IRQ_PRIORITY
 *
 * Interrupt priority for RADIO peripheral.
 * It is recommended to keep IRQ priority high (low number) to prevent losing frames due to
 * preemption.
 *
 */
#ifndef NRF_DRV_RADIO802154_IRQ_PRIORITY
#define NRF_DRV_RADIO802154_IRQ_PRIORITY  0
#endif

/**
 * @def NRF_DRV_RADIO802154_PENDING_SHORT_ADDRESSES
 *
 * Number of slots containing short addresses of nodes for which pending data is stored.
 *
 */
#ifndef NRF_DRV_RADIO802154_PENDING_SHORT_ADDRESSES
#define NRF_DRV_RADIO802154_PENDING_SHORT_ADDRESSES  10
#endif

/**
 * @def NRF_DRV_RADIO802154_PENDING_EXTENDED_ADDRESSES
 *
 * Number of slots containing extended addresses of nodes for which pending data is stored.
 *
 */
#ifndef NRF_DRV_RADIO802154_PENDING_EXTENDED_ADDRESSES
#define NRF_DRV_RADIO802154_PENDING_EXTENDED_ADDRESSES  10
#endif

/**
 * @def NRF_DRV_RADIO802154_RX_BUFFERS
 *
 * Number of buffers in receive queue.
 *
 */
#ifndef NRF_DRV_RADIO802154_RX_BUFFERS
#define NRF_DRV_RADIO802154_RX_BUFFERS  16
#endif

/**
 * @def NRF_DRV_RADIO802154_SHORT_CCAIDLE_TXEN
 *
 * RADIO peripheral can start transmission using short CCAIDLE->TXEN or interrupt handler.
 * If NRF_DRV_RADIO802154_SHORT_CCAIDLE_TXEN is set to 0 interrupt handler is used, otherwise
 * short is used.
 *
 */
#ifndef NRF_DRV_RADIO802154_SHORT_CCAIDLE_TXEN
#define NRF_DRV_RADIO802154_SHORT_CCAIDLE_TXEN  1
#endif

/**
 * @def NRF_DRV_RADIO802154_NOTIFICATION_SWI_PRIORITY
 *
 * Priority of software interrupt used to call notification from 802.15.4 driver.
 *
 */
#ifndef NRF_DRV_RADIO802154_NOTIFICATION_SWI_PRIORITY
#define NRF_DRV_RADIO802154_NOTIFICATION_SWI_PRIORITY 5
#endif

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // NRF_DRIVER_RADIO802154_CONFIG_H__
