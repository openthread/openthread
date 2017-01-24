/* Copyright (c) 2016, Nordic Semiconductor ASA
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

#ifndef NRF_DRV_CLOCK_H__
#define NRF_DRV_CLOCK_H__

#include <stdbool.h>
#include <stdint.h>
#include <nrf_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simplify version to keep API of SDK.
 */
typedef void nrf_drv_clock_handler_item_t;

/**
 * @brief Function for initializing the nrf_drv_clock module.
 *
 * After initialization, the module is in power off state (clocks are not requested).
 *
 * @retval     NRF_SUCCESS                           If the procedure was successful.
 * @retval     NRF_ERROR_MODULE_ALREADY_INITIALIZED  If the driver was already initialized.
 */
uint32_t nrf_drv_clock_init(void);

/**
 * @brief Function for uninitializing the clock module.
 *
 */
void nrf_drv_clock_uninit(void);

/**
 * @brief Function for requesting the LFCLK.
 *
 * The low-frequency clock can be requested by different modules
 * or contexts. The driver ensures that the clock will be started only when it is requested
 * the first time. If the clock is not ready but it was already started, the handler item that is
 * provided as an input parameter is added to the list of handlers that will be notified
 * when the clock is started. If the clock is already enabled, user callback is called from the
 * current context.
 *
 * The first request will start the selected LFCLK source. If an event handler is
 * provided, it will be called once the LFCLK is started. If the LFCLK was already started at this
 * time, the event handler will be called from the context of this function. Additionally,
 * the @ref nrf_drv_clock_lfclk_is_running function can be polled to check if the clock has started.
 *
 * @note When a SoftDevice is enabled, the LFCLK is always running and the driver cannot control it.
 *
 * @note The handler item provided by the user cannot be an automatic variable.
 *
 * @param[in] p_handler_item A pointer to the event handler structure.
 */
void nrf_drv_clock_lfclk_request(nrf_drv_clock_handler_item_t *p_handler_item);

/**
 * @brief Function for releasing the LFCLK.
 *
 * If there are no more requests, the LFCLK source will be stopped.
 *
 * @note When a SoftDevice is enabled, the LFCLK is always running.
 */
void nrf_drv_clock_lfclk_release(void);

/**
 * @brief Function for checking the LFCLK state.
 *
 * @retval true If the LFCLK is running.
 * @retval false If the LFCLK is not running.
 */
bool nrf_drv_clock_lfclk_is_running(void);

/**
 * @brief Function for requesting the high-accuracy source HFCLK.
 *
 * The high-accuracy source
 * can be requested by different modules or contexts. The driver ensures that the high-accuracy
 * clock will be started only when it is requested the first time. If the clock is not ready
 * but it was already started, the handler item that is provided as an input parameter is added
 * to the list of handlers that will be notified when the clock is started.
 *
 * If an event handler is provided, it will be called once the clock is started. If the clock was already
 * started at this time, the event handler will be called from the context of this function. Additionally,
 * the @ref nrf_drv_clock_hfclk_is_running function can be polled to check if the clock has started.
 *
 * @note If a SoftDevice is running, the clock is managed by the SoftDevice and all requests are handled by
 *       the SoftDevice. This function cannot be called from all interrupt priority levels in that case.
 * @note The handler item provided by the user cannot be an automatic variable.
 *
 * @param[in] p_handler_item A pointer to the event handler structure.
 */
void nrf_drv_clock_hfclk_request(nrf_drv_clock_handler_item_t *p_handler_item);

/**
 * @brief Function for releasing the high-accuracy source HFCLK.
 *
 * If there are no more requests, the high-accuracy source will be released.
 */
void nrf_drv_clock_hfclk_release(void);

/**
 * @brief Function for checking the HFCLK state.
 *
 * @retval true If the HFCLK is running (for \nRFXX XTAL source).
 * @retval false If the HFCLK is not running.
 */
bool nrf_drv_clock_hfclk_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // NRF_CLOCK_H__
