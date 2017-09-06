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

/**
 * @brief Front End Module control for the nRF 802.15.4 radio driver.
 *
 */

#ifndef NRF_FEM_CONTROL_API_H_
#define NRF_FEM_CONTROL_API_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @section Resource configuration.
 */

/** Default Power Amplifier pin. */
#define NRF_FEM_CONTROL_DEFAULT_PA_PIN                      26

/** Default Low Noise Amplifier pin. */
#define NRF_FEM_CONTROL_DEFAULT_LNA_PIN                     27

/** Default PPI channel for pin setting. */
#define NRF_FEM_CONTROL_DEFAULT_SET_PPI_CHANNEL             18

/** Default PPI channel for pin clearing. */
#define NRF_FEM_CONTROL_DEFAULT_CLR_PPI_CHANNEL             19

/** Default PPI channel group used to disable timer match PPI. */
#define NRF_FEM_CONTROL_DEFAULT_TIMER_MATCH_PPI_GROUP       4

/** Default PPI channel group used to disable radio disabled PPI. */
#define NRF_FEM_CONTROL_DEFAULT_RADIO_DISABLED_PPI_GROUP    5

/** Default GPIOTE channel for FEM control. */
#define NRF_FEM_CONTROL_DEFAULT_GPIOTE_CHANNEL              7

#if ENABLE_FEM

/**
 * @brief Configuration parameters for the PA and LNA.
 */
typedef struct
{
     uint8_t enable :1;         /**< Enable toggling for this amplifier */
     uint8_t active_high :1;    /**< Set the pin to be active high */
     uint8_t gpio_pin :6;       /**< The GPIO pin to toggle for this amplifier */
} nrf_fem_control_pa_lna_cfg_t;

/**
 * @brief PA & LNA GPIO toggle configuration
 *
 * This option configures the nRF 802.15.4 radio driver to toggle pins when the radio
 * is active for use with a power amplifier and/or a low noise amplifier.
 *
 * Toggling the pins is achieved by using two PPI channels and a GPIOTE channel. The hardware channel IDs are provided
 * by the application and should be regarded as reserved as long as any PA/LNA toggling is enabled.
 *
 * @note Changing this configuration while the radio is in use may have undefined
 *       consequences and must be avoided by the application.
 */
typedef struct
{
    nrf_fem_control_pa_lna_cfg_t pa_cfg;        /**< Power Amplifier configuration */
    nrf_fem_control_pa_lna_cfg_t lna_cfg;       /**< Low Noise Amplifier configuration */
    uint8_t                      ppi_ch_id_set; /**< PPI channel used for radio pin setting */
    uint8_t                      ppi_ch_id_clr; /**< PPI channel used for radio pin clearing */
    uint8_t                      timer_ppi_grp; /**< PPI group used for disabling timer match PPI. */
    uint8_t                      radio_ppi_grp; /**< PPI group used for disabling radio disabled PPI. */
    uint8_t                      gpiote_ch_id;  /**< GPIOTE channel used for radio pin toggling */
} nrf_fem_control_cfg_t;

/**@brief Set PA & LNA GPIO toggle configuration.
 *
 * @note This function shall not be called when radio is in use.
 *
 * @param[in] p_cfg A pointer to the PA & LNA GPIO toggle configuration.
 *
 */
void nrf_fem_control_cfg_set(const nrf_fem_control_cfg_t * p_cfg);

/**@brief Get PA & LNA GPIO toggle configuration.
 *
 * @param[out] p_cfg A pointer to the structure for the PA & LNA GPIO toggle configuration.
 *
 */
void nrf_fem_control_cfg_get(nrf_fem_control_cfg_t * p_cfg);

/**@brief Activate FEM controller.
 *
 * This function should be called when radio wakes up.
 */
void nrf_fem_control_activate(void);

/**@brief De-activate FEM controller.
 *
 * This function should be called when radio goes to sleep.
 */
void nrf_fem_control_deactivate(void);

/**@brief Latch current time in FEM controller.
 *
 * This function stores current time to enable precise time measurement and mitigate impact of code
 * execution time. It should be called before triggering RXEN or TXEN task, and calling
 * @ref nrf_fem_control_pa_set or @ref nrf_fem_control_lna_set funcitons.
 */
void nrf_fem_control_time_latch(void);

/**@brief Activate Power Amplifier (TX) pin of the Front End Module.
 *
 * @param[in] shorts_used Information if this operation is related to a task triggered by a short.
 *
 * This function will set up a timer to activate the pin 5 +/- 2.5 us before radio READY event is generated.
 * It will also set up a PPI to deactivate the pin on radio DISABLED event.
 *
 * @note This function shall always be called after @ref nrf_fem_control_time_latch function was called,
 *       to enable precise time measurement.
 * */
void nrf_fem_control_pa_set(bool shorts_used);

/**@brief Activate Low Noise Amplifier (RX) pin of the Front End Module.
 *
 * @param[in] shorts_used Information if this operation is related to a task triggered by a short.
 *
 * This function will set up a timer to activate the pin 5 +/- 2.5 us before radio READY event is generated.
 * It will also set up a PPI to deactivate the pin on radio DISABLED event.
 *
 * @note This function shall always be called after @ref nrf_fem_control_time_latch function was called,
 *       to enable precise time measurement.
 * */
void nrf_fem_control_lna_set(bool shorts_used);

#else  // ENABLE_FEM

#define nrf_fem_control_cfg_set(...)
#define nrf_fem_control_cfg_get(...)
#define nrf_fem_control_activate(...)
#define nrf_fem_control_deactivate(...)
#define nrf_fem_control_time_latch(...)
#define nrf_fem_control_pa_set(...)
#define nrf_fem_control_lna_set(...)

#endif // ENABLE_FEM

#ifdef __cplusplus
}
#endif

#endif /* NRF_FEM_CONTROL_API_H_ */
