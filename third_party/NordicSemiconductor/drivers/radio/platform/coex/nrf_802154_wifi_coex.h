/* Copyright (c) 2018, Nordic Semiconductor ASA
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

/**
 * @brief Module that defines the Wi-Fi coexistence module.
 *
 */

#ifndef NRF_802154_WIFI_COEX_H_
#define NRF_802154_WIFI_COEX_H_

#include "rsch/nrf_802154_rsch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_wifi_coex Wi-Fi Coexistence
 * @{
 * @ingroup nrf_802154
 * @brief The Wi-Fi Coexistence module.
 *
 * The Wi-Fi Coexistence module is a client of the PTA (defined in the 802.15.2). It manages GPIO
 * to assert pins and respond to pin state changes.
 */

/**
 * @brief Initializes the Wi-Fi Coexistence module.
 *
 * @note This function must be called once, before any other function from this module.
 *
 */
void nrf_802154_wifi_coex_init(void);

/**
 * @brief Deinitializes the Wi-Fi Coexistence module.
 *
 */
void nrf_802154_wifi_coex_uninit(void);

/**
 * @brief Requests the given priority from the Wi-Fi Coexistence module.
 *
 * @note The approval of the requested priority is notified asynchronously by the
 *       @ref nrf_802154_wifi_coex_prio_changed call.
 *
 * @param[in]  priority  The requested priority level.
 *
 */
void nrf_802154_wifi_coex_prio_request(rsch_prio_t priority);

/**
 * @brief Gets the priority denial event address.
 *
 * Get the address of a hardware event that notifies about the denial of a previously approved
 * priority.
 *
 * @returns Address of the priority denial event.
 */
void * nrf_802154_wifi_coex_deny_event_addr_get(void);

/**
 * @brief Notifies about the approved priority change.
 *
 * The Wi-Fi Coexistence module calls this function to notify the RSCH of the currently approved
 * priority level.
 *
 * @param[in]  priority  The approved priority level.
 */
extern void nrf_802154_wifi_coex_prio_changed(rsch_prio_t priority);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_WIFI_COEX_H_ */
