/* Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
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

#include <stdint.h>

/**
 * @brief Configuration parameters for PA and LNA.
 */
typedef struct
{
    uint8_t enable      : 1; /**< Enable toggling for this amplifier. */
    uint8_t active_high : 1; /**< Set the pin to be active high. */
    uint8_t gpio_pin    : 6; /**< The GPIO pin to be toggled for this amplifier. */
} nrf_fem_control_pa_lna_cfg_t;

/**
 * @brief PA and LNA GPIO toggle configuration.
 *
 * This option configures the nRF 802.15.4 radio driver to toggle pins when the radio
 * is active and ready for use with a Power Amplifier (PA) or a Low Noise Amplifier (LNA), or both.
 *
 * Pins can be toggled by using two PPI channels and a GPIOTE channel. The hardware channel IDs are provided
 * by the application and are to be regarded as reserved for as long as there is no PA/LNA toggling enabled.
 *
 * @note Avoid changing this configuration while the radio is in use, as this can lead to undefined consequences.
 *
 */
typedef struct
{
    nrf_fem_control_pa_lna_cfg_t pa_cfg;           /**< Power Amplifier configuration. */
    nrf_fem_control_pa_lna_cfg_t lna_cfg;          /**< Low Noise Amplifier configuration. */
    uint8_t                      pa_gpiote_ch_id;  /**< GPIOTE channel used for Power Amplifier pin toggling. */
    uint8_t                      lna_gpiote_ch_id; /**< GPIOTE channel used for Low Noise Amplifier pin toggling. */
    uint8_t                      ppi_ch_id_set;    /**< PPI channel used for radio Power Amplifier and Low Noise Amplifier pins setting. */
    uint8_t                      ppi_ch_id_clr;    /**< PPI channel used for radio pin clearing. */
} nrf_fem_control_cfg_t;
