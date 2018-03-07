/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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
 * @brief This module contains generic 802.15.4 radio driver for nRF SoC devices.
 *
 */

#ifndef NRF_802154_H_
#define NRF_802154_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_config.h"
#include "nrf_802154_types.h"

#if ENABLE_FEM
#include "fem/nrf_fem_control_api.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize 802.15.4 driver.
 *
 * @note This function shall be called once, before any other function from this module.
 *
 * Initialize radio peripheral to Sleep state.
 */
void nrf_802154_init(void);

/**
 * @brief Deinitialize 802.15.4 driver.
 *
 * Deinitialize radio peripheral and reset it to the default state.
 */
void nrf_802154_deinit(void);

#if !NRF_802154_INTERNAL_RADIO_IRQ_HANDLING
/**
 * @brief Handle interrupt request from the RADIO peripheral.
 *
 * @note When NRF_802154_INTERNAL_RADIO_IRQ_HANDLING is enabled the driver internally handles the
 *       RADIO IRQ and this function shall not be called.
 *
 * This function is intended to be used in Operating System environment when the OS handles IRQ
 * and indirectly passes it to the driver or with RAAL implementation that indirectly passes radio
 * IRQ handler to the driver (i.e. SoftDevice).
 */
void nrf_802154_radio_irq_handler(void);
#endif // !NRF_802154_INTERNAL_RADIO_IRQ_HANDLING

/**
 * @brief Set channel on which the radio shall operate right now.
 *
 * @param[in]  channel  Channel number (11-26).
 */
void nrf_802154_channel_set(uint8_t channel);

/**
 * @brief Get channel on which the radio operates right now.
 *
 * @returns  Channel number (11-26).
 */
uint8_t nrf_802154_channel_get(void);

/**
 * @brief Set transmit power.
 *
 * @note The driver recalculates requested value to the nearest value accepted by the hardware.
 *       The calculation result is rounded up.
 *
 * @param[in]  power  Transmit power [dBm].
 */
void nrf_802154_tx_power_set(int8_t power);

/**
 * @brief Get currently set transmit power.
 *
 * @return Currently used transmit power [dBm].
 */
int8_t nrf_802154_tx_power_get(void);

/***************************************************************************************************
 * @section Front-end module management.
 **************************************************************************************************/

#if ENABLE_FEM

/** Structure containing run-time configuration of FEM module. */
typedef nrf_fem_control_cfg_t nrf_802154_fem_control_cfg_t;

/** Macro with default configuration of FEM module. */
#define NRF_802154_FEM_DEFAULT_SETTINGS                                                            \
    ((nrf_802154_fem_control_cfg_t) {                                                              \
        .pa_cfg = {                                                                                \
                .enable      = 1,                                                                  \
                .active_high = 1,                                                                  \
                .gpio_pin    = NRF_FEM_CONTROL_DEFAULT_PA_PIN,                                     \
        },                                                                                         \
        .lna_cfg = {                                                                               \
                .enable      = 1,                                                                  \
                .active_high = 1,                                                                  \
                .gpio_pin    = NRF_FEM_CONTROL_DEFAULT_LNA_PIN,                                    \
        },                                                                                         \
        .pa_gpiote_ch_id   = NRF_FEM_CONTROL_DEFAULT_PA_GPIOTE_CHANNEL,                            \
        .lna_gpiote_ch_id  = NRF_FEM_CONTROL_DEFAULT_LNA_GPIOTE_CHANNEL,                           \
        .ppi_ch_id_set     = NRF_FEM_CONTROL_DEFAULT_SET_PPI_CHANNEL,                              \
        .ppi_ch_id_clr     = NRF_FEM_CONTROL_DEFAULT_CLR_PPI_CHANNEL,                              \
    })

/**
 * @brief Set PA & LNA GPIO toggle configuration.
 *
 * @note This function shall not be called when radio is in use.
 *
 * @param[in] p_cfg A pointer to the PA & LNA GPIO toggle configuration.
 *
 */
void nrf_802154_fem_control_cfg_set(const nrf_802154_fem_control_cfg_t * p_cfg);

/**
 * @brief Get PA & LNA GPIO toggle configuration.
 *
 * @param[out] p_cfg A pointer to the structure for the PA & LNA GPIO toggle configuration.
 *
 */
void nrf_802154_fem_control_cfg_get(nrf_802154_fem_control_cfg_t * p_cfg);

#endif // ENABLE_FEM


/***************************************************************************************************
 * @section Setting addresses and Pan Id of this device.
 **************************************************************************************************/

/**
 * @brief Set PAN Id used by this device.
 *
 * @param[in]  p_pan_id  Pointer to PAN Id (2 bytes, little-endian).
 *
 * This function makes copy of the PAN Id.
 */
void nrf_802154_pan_id_set(const uint8_t *p_pan_id);

/**
 * @brief Set Extended Address of this device.
 *
 * @param[in]  p_extended_address  Pointer to extended address (8 bytes, little-endian).
 *
 * This function makes copy of the address.
 */
void nrf_802154_extended_address_set(const uint8_t *p_extended_address);

/**
 * @brief Set Short Address of this device.
 *
 * @param[in]  p_short_address  Pointer to short address (2 bytes, little-endian).
 *
 * This function makes copy of the address.
 */
void nrf_802154_short_address_set(const uint8_t *p_short_address);


/***************************************************************************************************
 * @section Functions to calculate data given by the driver.
 **************************************************************************************************/

/**
 * @brief  Calculate dBm from energy level received during energy detection procedure.
 *
 * @param[in]  energy_level  Energy level passed by @sa nrf_802154_energy_detected
 *
 * @return  Result of energy detection procedure in dBm.
 */
int8_t nrf_802154_dbm_from_energy_level_calculate(uint8_t energy_level);

/**
 * @brief  Calculate timestamp of beginning of first symbol of preamble in a received frame.
 *
 * @param[in]  end_timestamp  Timestamp of end of last symbol in the frame (in us).
 * @param[in]  psdu_length    Number of bytes in the frame PSDU.
 *
 * @return  Timestamp of first preamble symbol of given frame (in us).
 */
uint32_t nrf_802154_first_symbol_timestamp_get(uint32_t end_timestamp, uint8_t psdu_length);


/***************************************************************************************************
 * @section Functions to request FSM transitions and check current state.
 **************************************************************************************************/

/**
 * @brief Get current state of the radio.
 */
nrf_802154_state_t nrf_802154_state_get(void);

/**
 * @brief Change radio state to Sleep.
 *
 * Sleep state is the lowest power state. In this state radio cannot transmit or receive frames.
 * It is the only state in which the driver releases high frequency clock and does not request
 * timeslots from a radio arbiter.
 *
 * @note High frequency clock may be enabled even in radio Sleep state if other module requests it.
 *
 * @return  true   If the radio changes it's state to low power mode.
 * @return  false  If the driver could not schedule changing state.
 */
bool nrf_802154_sleep(void);

/**
 * @brief Change radio state to Receive.
 *
 * In Receive state radio receives frames and may automatically send ACK frames when appropriate.
 * Received frame is reported to higher layer by @sa nrf_radio802154_received() call.
 *
 * @return  true   If the radio enters Receive state.
 * @return  false  If the driver could not enter Receive state.
 */
bool nrf_802154_receive(void);

#if NRF_802154_USE_RAW_API
/**
 * @brief Change radio state to Transmit.
 *
 * @note If the CPU was halted or interrupted during performing this function
 *       @sa nrf_802154_transmitted() or @sa nrf_802154_transmit_failed() may be
 *       called before nrf_802154_transmit_raw() returns result.
 * @note This function is implemented in zero-copy fashion. It passes given buffer pointer to
 *       the RADIO peripheral.
 *
 * In Transmit state radio transmits given frame. If requested it waits for ACK frame.
 * Depending on @sa NRF_802154_ACK_TIMEOUT_ENABLED configuration option, Radio driver automatically
 * stops waiting for an ACK frame or waits infinitely for an ACK frame. If it is configured to wait
 * indefinitely, MAC layer is responsible to call @sa nrf_radio802154_receive() or
 * @sa nrf_radio802154_sleep() after ACK timeout.
 * Transmission result is reported to higher layer by @sa nrf_radio802154_transmitted() or
 * @sa nrf_radio802154_transmit_failed() calls.
 *
 * p_data
 * v
 * +-----+-----------------------------------------------------------+------------+
 * | PHR | MAC Header and payload                                    | FCS        |
 * +-----+-----------------------------------------------------------+------------+
 *       |                                                                        |
 *       | <---------------------------- PHR -----------------------------------> |
 *
 * @param[in]  p_data    Pointer to array containing data to transmit. First byte should contain
 *                       frame length (including PHR and FCS). Following bytes should contain data.
 *                       CRC is computed automatically by radio hardware and because of that FCS
 *                       field can contain any bytes.
 * @param[in]  cca       If the driver should perform CCA procedure before transmission.
 *
 * @return  true   If the transmission procedure was scheduled.
 * @return  false  If the driver could not schedule the transmission procedure.
 */
bool nrf_802154_transmit_raw(const uint8_t * p_data, bool cca);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Change radio state to Transmit.
 *
 * @note If the CPU was halted or interrupted during performing this function
 *       @sa nrf_802154_transmitted() or @sa nrf_802154_transmit_failed() may be
 *       called before nrf_802154_transmit() returns result.
 * @note This function makes copy of given buffer. There is an internal buffer maintained by this
 *       function. It is used to make a frame copy. To prevent unnecessary memory consumption and
 *       to perform zero-copy transmission @sa nrf_802154_transmit_raw() function should
 *       be used instead of this.
 *
 * In Transmit state radio transmits given frame. If requested it waits for ACK frame.
 * Depending on @sa NRF_802154_ACK_TIMEOUT_ENABLED configuration option, Radio driver automatically
 * stops waiting for an ACK frame or waits infinitely for an ACK frame. If it is configured to wait
 * indefinitely, MAC layer is responsible to call @sa nrf_radio802154_receive() or
 * @sa nrf_radio802154_sleep() after ACK timeout.
 * Transmission result is reported to higher layer by @sa nrf_radio802154_transmitted() or
 * @sa nrf_radio802154_transmit_failed() calls.
 *
 *       p_data
 *       v
 * +-----+-----------------------------------------------------------+------------+
 * | PHR | MAC Header and payload                                    | FCS        |
 * +-----+-----------------------------------------------------------+------------+
 *       |                                                           |
 *       | <------------------ length -----------------------------> |
 *
 * @param[in]  p_data    Pointer to array containing payload of a data to transmit. The array
 *                       should exclude PHR or FCS fields of 802.15.4 frame.
 * @param[in]  length    Length of given frame. This value shall exclude PHR and FCS fields from
 *                       the given frame (exact size of buffer pointed by @p p_data).
 * @param[in]  cca       If the driver should perform CCA procedure before transmission.
 *
 * @return  true   If the transmission procedure was scheduled.
 * @return  false  If the driver could not schedule the transmission procedure.
 */
bool nrf_802154_transmit(const uint8_t * p_data, uint8_t length, bool cca);

#endif // NRF_802154_USE_RAW_API

/**
 * @brief Change radio state to Energy Detection.
 *
 * In Energy Detection state radio detects maximum energy for given time. Result of the detection
 * is reported to the higher layer by @sa nrf_802154_energy_detected() call.
 *
 * @note @sa nrf_802154_energy_detected() may be called before nrf_802154_energy_detection()
 *       returns result.
 * @note Performing Energy Detection procedure make take longer than time requested in @p time_us.
 *       Procedure is performed only during timeslots granted by a radio arbiter. It may be
 *       interrupted by other protocols using radio hardware. If procedure is interrupted, it is
 *       automatically continued and the sum of time periods during which the procedure is carried
 *       out is not less than requested @p time_us.
 *
 * @param[in]  time_us   Duration of energy detection procedure. Given value is rounded up to
 *                       multiplication of 8s (128 us).
 *
 * @return  true   If the energy detection procedure was scheduled.
 * @return  false  If the driver could not schedule the energy detection procedure.
 */
bool nrf_802154_energy_detection(uint32_t time_us);

/**
 * @brief Change radio state to CCA.
 *
 * @note @sa nrf_802154_cca_done() may be called before nrf_802154_cca() returns result.
 *
 * In CCA state radio verifies if channel is clear. Result of verification is reported to the higher
 * layer by @sa nrf_802154_cca_done() call.
 *
 * @return  true   If the CCA procedure was scheduled.
 * @return  false  If the driver could not schedule the CCA procedure.
 */
bool nrf_802154_cca(void);

/**
 * @brief Change radio state to Continuous Carrier.
 *
 * @note When radio is emitting continuous carrier it blocks all transmissions on selected channel.
 *       This function should be called only during radio tests. It should not be used during
 *       normal device operation.
 * @note This function works correctly only with a single-phy arbiter. It should not be used with
 *       any other arbiter.
 *
 * @return  true   If the continuous carrier procedure was scheduled.
 * @return  false  If the driver could not schedule the continuous carrier procedure.
 */
bool nrf_802154_continuous_carrier(void);


/***************************************************************************************************
 * @section Calls to higher layer.
 **************************************************************************************************/

/**
 * @brief Notify that transmitting ACK frame has started.
 *
 * @note This function should be very short to prevent dropping frames by the driver.
 */
extern void nrf_802154_tx_ack_started(void);

#if NRF_802154_USE_RAW_API

/**
 * @brief Notify that frame was received.
 *
 * @note Buffer pointed by the p_data pointer is not modified by the radio driver (and can't
 *       be used to receive a frame) until @sa nrf_802154_buffer_free_raw() function is called.
 * @note Buffer pointed by the p_data pointer may be modified by the function handler (and other
 *       modules) until @sa nrf_802154_buffer_free_raw() function is called.
 *
 * p_data
 * v
 * +-----+-----------------------------------------------------------+------------+
 * | PHR | MAC Header and payload                                    | FCS        |
 * +-----+-----------------------------------------------------------+------------+
 *       |                                                                        |
 *       | <---------------------------- PHR -----------------------------------> |
 *
 * @param[in]  p_data  Pointer to the buffer containing received data (PHR + PSDU). First byte in
 *                     the buffer is length of the frame (PHR) and following bytes is the frame
 *                     itself (PSDU). Length byte (PHR) includes FCS. FCS is already verified by
 *                     the hardware and may be modified by the hardware.
 * @param[in]  power   RSSI of received frame.
 * @param[in]  lqi     LQI of received frame.
 */
extern void nrf_802154_received_raw(uint8_t * p_data, int8_t power, uint8_t lqi);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Notify that frame was received.
 *
 * @note Buffer pointed by the p_data pointer is not modified by the radio driver (and can't
 *       be used to receive a frame) until nrf_802154_buffer_free() function is called.
 * @note Buffer pointed by the p_data pointer may be modified by the function handler (and other
 *       modules) until @sa nrf_802154_buffer_free() function is called.
 *
 *       p_data
 *       v
 * +-----+-----------------------------------------------------------+------------+
 * | PHR | MAC Header and payload                                    | FCS        |
 * +-----+-----------------------------------------------------------+------------+
 *       |                                                           |
 *       | <------------------ length -----------------------------> |
 *
 * @param[in]  p_data  Pointer to the buffer containing payload of received frame (PSDU without FCS).
 * @param[in]  length  Length of received payload.
 * @param[in]  power   RSSI of received frame.
 * @param[in]  lqi     LQI of received frame.
 */
extern void nrf_802154_received(uint8_t * p_data, uint8_t length, int8_t power, uint8_t lqi);

#endif // !NRF_802154_USE_RAW_API

#if NRF_802154_FRAME_TIMESTAMP_ENABLED
#if NRF_802154_USE_RAW_API
/**
 * @brief Notify that frame was received at given time
 *
 * This functions works like @sa nrf_802154_received_raw and adds timestamp to parameter
 * list.
 *
 * @note @p timestamp may be inacurate due to software latency (IRQ handling).
 * @note @p timestamp granularity depends on granularity of the timer driver in platform/timer
 *       directory.
 * @note Including timestamp to received frame uses resources like CPU time and memory. If timestamp
 *       is not required use @sa nrf_802154_received_raw() instead.
 *
 * @param[in]  p_data  Pointer to the buffer containing received data (PHR + PSDU). First byte in
 *                     the buffer is length of the frame (PHR) and following bytes is the frame
 *                     itself (PSDU). Length byte (PHR) includes FCS. FCS is already verified by
 *                     the hardware and may be modified by the hardware.
 * @param[in]  power   RSSI of received frame.
 * @param[in]  lqi     LQI of received frame.
 * @param[in]  time    Timestamp taken when last symbol of the frame was received (in us).
 */
extern void nrf_802154_received_timestamp_raw(uint8_t * p_data,
                                              int8_t    power,
                                              uint8_t   lqi,
                                              uint32_t  time);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Notify that frame was received at given time
 *
 * This functions works like @sa nrf_802154_received and adds timestamp to parameter list.
 *
 * @note @p timestamp may be inacurate due to software latency (IRQ handling).
 * @note @p timestamp granularity depends on granularity of the timer driver in platform/timer
 *       directory.
 * @note Including timestamp to received frame uses resources like CPU time and memory. If timestamp
 *       is not required use @sa nrf_802154_received instead().
 *
 * @param[in]  p_data  Pointer to the buffer containing payload of received frame (PSDU without FCS).
 * @param[in]  length  Length of received payload.
 * @param[in]  power   RSSI of received frame.
 * @param[in]  lqi     LQI of received frame.
 * @param[in]  time    Timestamp taken when last symbol of the frame was received (in us).
 */
extern void nrf_802154_received_timestamp(uint8_t * p_data,
                                          uint8_t   length,
                                          int8_t    power,
                                          uint8_t   lqi,
                                          uint32_t  time);

#endif // NRF_802154_USE_RAW_API
#endif // NRF_802154_FRAME_TIMESTAMP_ENABLED

/**
 * @brief Notify that reception of a frame failed.
 *
 * @param[in]  error  An error code that indicates reason of the failure.
 */
extern void nrf_802154_receive_failed(nrf_802154_rx_error_t error);

/**
 * @brief Notify that transmitting frame has started.
 *
 * @note It is possible that transmit procedure is interrupted and
 *       @sa nrf_802154_transmitted won't be called.
 * @note This function should be very short to prevent dropping frames by the driver.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of the frame being transmitted.
 */
extern void nrf_802154_tx_started(const uint8_t * p_frame);

#if NRF_802154_USE_RAW_API

/**
 * @brief Notify that frame was transmitted.
 *
 * @note If ACK was requested for transmitted frame this function is called after proper ACK is
 *       received. If ACK was not requested this function is called just after transmission is
 *       ended.
 * @note Buffer pointed by the @p p_ack pointer is not modified by the radio driver (and can't
 *       be used to receive a frame) until @sa nrf_802154_buffer_free_raw() function is
 *       called.
 * @note Buffer pointed by the @p p_ack pointer may be modified by the function handler (and other
 *       modules) until @sa nrf_802154_buffer_free_raw() function is called.
 *
 * @param[in]  p_frame  Pointer to buffer containing PHR and PSDU of transmitted frame.
 * @param[in]  p_ack    Pointer to received ACK buffer. Fist byte in the buffer is length of the
 *                      frame (PHR) and following bytes are the ACK frame itself (PSDU). Length byte
 *                      (PHR) includes FCS. FCS is already verified by the hardware and may be
 *                      modified by the hardware.
 *                      If ACK was not requested @p p_ack is set to NULL.
 * @param[in]  power    RSSI of received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of received frame or 0 if ACK was not requested.
 */
extern void nrf_802154_transmitted_raw(const uint8_t * p_frame,
                                       uint8_t       * p_ack,
                                       int8_t          power,
                                       uint8_t         lqi);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Notify that frame was transmitted.
 *
 * @note If ACK was requested for transmitted frame this function is called after proper ACK is
 *       received. If ACK was not requested this function is called just after transmission is
 *       ended.
 * @note Buffer pointed by the @p p_ack pointer is not modified by the radio driver (and can't
 *       be used to receive a frame) until @sa nrf_802154_buffer_free() function is
 *       called.
 * @note Buffer pointed by the @p p_ack pointer may be modified by the function handler (and other
 *       modules) until @sa nrf_802154_buffer_free() function is called.
 * @note The next higher layer should handle @sa nrf_802154_transmitted() or
 *       @sa nrf_802154_transmitted_raw() function. It should not handle both.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of transmitted frame.
 * @param[in]  p_ack    Pointer to buffer containing received ACK payload (PHR excluding FCS).
  *                     If ACK was not requested @p p_ack is set to NULL.
 * @param[in]  length   Length of received ACK payload or 0 if ACK was not requested.
 * @param[in]  power    RSSI of received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of received frame or 0 if ACK was not requested.
 */
extern void nrf_802154_transmitted(const uint8_t * p_frame,
                                   uint8_t       * p_ack,
                                   uint8_t         length,
                                   int8_t          power,
                                   uint8_t         lqi);

#endif // !NRF_802154_USE_RAW_API

#if NRF_802154_FRAME_TIMESTAMP_ENABLED
#if NRF_802154_USE_RAW_API

/**
 * @brief Notify that frame was transmitted.
 *
 * This functions works like @sa nrf_802154_transmitted_raw and adds timestamp to parameter
 * list.
 *
 * @note @p timestamp may be inacurate due to software latency (IRQ handling).
 * @note @p timestamp granularity depends on granularity of the timer driver in platform/timer
 *       directory.
 * @note Including timestamp to received frame uses resources like CPU time and memory. If timestamp
 *       is not required use @sa nrf_802154_received instead.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of transmitted frame.
 * @param[in]  p_ack    Pointer to received ACK buffer. Fist byte in the buffer is length of the
 *                      frame (PHR) and following bytes are the ACK frame itself (PSDU). Length byte
 *                      (PHR) includes FCS. FCS is already verified by the hardware and may be
 *                      modified by the hardware.
 *                      If ACK was not requested @p p_ack is set to NULL.
 * @param[in]  power    RSSI of received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of received frame or 0 if ACK was not requested.
 * @param[in]  time     Timestamp taken when received last symbol of ACK or 0 if ACK was not
 *                      requested.
 */
extern void nrf_802154_transmitted_timestamp_raw(const uint8_t * p_frame,
                                                 uint8_t       * p_ack,
                                                 int8_t          power,
                                                 uint8_t         lqi,
                                                 uint32_t        time);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Notify that frame was transmitted.
 *
 * This functions works like @sa nrf_802154_transmitted and adds timestamp to parameter
 * list.
 *
 * @note @p timestamp may be inacurate due to software latency (IRQ handling).
 * @note @p timestamp granularity depends on granularity of the timer driver in platform/timer
 *       directory.
 * @note Including timestamp to received frame uses resources like CPU time and memory. If timestamp
 *       is not required use @sa nrf_802154_received instead.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of transmitted frame.
 * @param[in]  p_ack    Pointer to buffer containing received ACK payload (PHR excluding FCS).
  *                     If ACK was not requested @p p_ack is set to NULL.
 * @param[in]  length   Length of received ACK payload.
 * @param[in]  power    RSSI of received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of received frame or 0 if ACK was not requested.
 * @param[in]  time     Timestamp taken when received last symbol of ACK or 0 if ACK was not
 *                      requested.
 */
extern void nrf_802154_transmitted_timestamp(const uint8_t * p_frame,
                                             uint8_t       * p_ack,
                                             uint8_t         length,
                                             int8_t          power,
                                             uint8_t         lqi,
                                             uint32_t        time);

#endif // NRF_802154_USE_RAW_API
#endif // NRF_802154_FRAME_TIMESTAMP_ENABLED

/**
 * @brief Notify that frame was not transmitted due to busy channel.
 *
 * This function is called if transmission procedure fails.
 *
 * @param[in]  p_frame  Pointer to buffer containing PSDU of frame that was not transmitted.
 * @param[in]  error    Reason of the failure.
 */
extern void nrf_802154_transmit_failed(const uint8_t       * p_frame,
                                       nrf_802154_tx_error_t error);

/**
 * @brief Notify that Energy Detection procedure finished.
 *
 * @note This function passes EnergyLevel defined in 802.15.4-2006 specification:
 *       0x00 - 0xff proportional to detected energy level (dBm above receiver sensitivity). To
 *       calculate result in dBm use @sa nrf_802154_dbm_from_energy_level_calculate().
 *
 * @param[in]  result  Maximum energy detected during Energy Detection procedure.
 */
extern void nrf_802154_energy_detected(uint8_t result);

/**
 * @brief Notify that Energy Detection procedure failed.
 *
 * @param[in]  error  Reason of the failure.
 */
extern void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error);

/**
 * @brief Notify that CCA procedure has finished.
 *
 * @param[in]  channel_free  Indication if channel is free.
 */
extern void nrf_802154_cca_done(bool channel_free);

/**
 * @brief Notify that CCA procedure failed.
 *
 * @param[in]  error  Reason of the failure.
 */
extern void nrf_802154_cca_failed(nrf_802154_cca_error_t error);


/***************************************************************************************************
 * @section Driver memory management
 **************************************************************************************************/

#if NRF_802154_USE_RAW_API

/**
 * @brief Notify driver that buffer containing received frame is not used anymore.
 *
 * @note The buffer pointed by the @p p_data pointer may be modified by this function.
 *
 * @param[in]  p_data  A pointer to the buffer containing received data that is no more needed by
 *                     the higher layer.
 */
void nrf_802154_buffer_free_raw(uint8_t * p_data);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Notify driver that buffer containing received frame is not used anymore.
 *
 * @note The buffer pointed by the @p p_data pointer may be modified by this function.
 *
 * @param[in]  p_data  A pointer to the buffer containing received data that is no more needed by
 *                     the higher layer.
 */
void nrf_802154_buffer_free(uint8_t * p_data);

#endif // NRF_802154_USE_RAW_API


/***************************************************************************************************
 * @section RSSI measurement function.
 **************************************************************************************************/

/**
 * @brief Begin RSSI measurement.
 *
 * @note This function should be called in Receive state.
 *
 * Begin RSSI measurement. The result will be available in 8 uS. The result can be read by
 * @sa nrf_radio802154_rssi_last_get() function.
 */
void nrf_802154_rssi_measure(void);

/**
 * @brief Get result of last RSSI measurement.
 *
 * @returns RSSI measurement result [dBm].
 */
int8_t nrf_802154_rssi_last_get(void);

/**
 * @brief Adjust given RSSI measurement using a temperature correction factor.
 *
 * @param[in]  rssi  Measured RSSI value [dBm].
 * @param[in]  temp  Temperature value when RSSI sample took place [C].
 *
 * @returns RSSI [dBm] corrected by a temperature factor (Errata 153).
 */
int8_t nrf_802154_rssi_corrected_get(int8_t rssi, int8_t temp);


/***************************************************************************************************
 * @section Promiscuous mode.
 **************************************************************************************************/

/**
 * @brief Enable or disable promiscuous radio mode.
 *
 * @note Promiscuous mode is disabled by default.
 *
 * In promiscuous mode driver notifies higher layer that it received any frame (regardless
 * frame type or destination address).
 * In normal mode (not promiscuous) higher layer is not notified about ACK frames and frames with
 * unknown type. Also frames with destination address not matching this device address are ignored.
 *
 * @param[in]  enabled  If promiscuous mode should be enabled.
 */
void nrf_802154_promiscuous_set(bool enabled);

/**
 * @brief Check if radio is in promiscuous mode.
 *
 * @retval True   Radio is in promiscuous mode.
 * @retval False  Radio is not in promiscuous mode.
 */
bool nrf_802154_promiscuous_get(void);


/***************************************************************************************************
 * @section Auto ACK management.
 **************************************************************************************************/

/**
 * @brief Enable or disable auto ACK procedure.
 *
 * @note Auto ACK procedure is enabled by default.
 *
 * If auto ACK procedure is enabled the driver prepares and sends ACK frames automatically
 * aTurnaroundTime (192 us) after proper frame is received. The driver sets sequence number in
 * the ACK frame and pending bit according to auto pending bit feature settings. When auto ACK
 * procedure is enabled the driver notifies the next higher layer about received frame after ACK
 * frame is transmitted.
 * If auto ACK procedure is disabled the driver does not transmit ACK frames. It notifies the next
 * higher layer about received frame when a frame is received. In this mode the next higher layer
 * is responsible for sending ACK frame. ACK frames should be sent using @sa nrf_802154_transmit()
 * function.
 *
 * @param[in]  enabled  If auto ACK procedure should be enabled.
 */
void nrf_802154_auto_ack_set(bool enabled);

/**
 * @brief Check if auto ACK procedure is enabled.
 *
 * @retval True   Auto ACK procedure is enabled.
 * @retval False  Auto ACK procedure is disabled.
 */
bool nrf_802154_auto_ack_get(void);

/**
 * @brief Enable or disable setting pending bit in automatically transmitted ACK frames.
 *
 * @note Setting pending bit in automatically transmitted ACK frames is enabled by default.
 *
 * Radio driver automatically sends ACK frames in response frames destined to this node with Ack
 * Request bit set. Pending bit in ACK frame can be set or cleared regarding data in indirect queue
 * destined to ACK destination.
 *
 * If setting pending bit in ACK frames is disabled, pending bit in every ACK frame is set.
 * If setting pending bit in ACK frames is enabled, radio driver checks if there is data
 * in indirect queue destined to ACK destination. If there is no such data, pending bit is cleared.
 *
 * @note It is possible that due to ISR latency, radio driver cannot verify if there is data in
 *       indirect queue before ACK is sent. In this case pending bit is set.
 *
 * @param[in]  enabled  If setting pending bit in ACK frames is enabled.
 */
void nrf_802154_auto_pending_bit_set(bool enabled);

/**
 * @brief Add address of peer node for which there is pending data in the buffer.
 *
 * @note This function makes a copy of given address.
 *
 * @param[in]  p_addr    Array of bytes containing address of the node (little-endian).
 * @param[in]  extended  If given address is Extended MAC Address or Short MAC Address.
 *
 * @retval True   Address successfully added to the list.
 * @retval False  There is not enough memory to store this address in the list.
 */
bool nrf_802154_pending_bit_for_addr_set(const uint8_t *p_addr, bool extended);

/**
 * @brief Remove address of peer node for which there is no more pending data in the buffer.
 *
 * @param[in]  p_addr    Array of bytes containing address of the node (little-endian).
 * @param[in]  extended  If given address is Extended MAC Address or Short MAC Address.
 *
 * @retval True   Address successfully removed from the list.
 * @retval False  There is no such address in the list.
 */
bool nrf_802154_pending_bit_for_addr_clear(const uint8_t *p_addr, bool extended);

/**
 * @brief Remove all addresses of given type from pending bit list.
 *
 * @param[in]  extended  If function should remove all Exnteded MAC Adresses of all Short Addresses.
 */
void nrf_802154_pending_bit_for_addr_reset(bool extended);

/***************************************************************************************************
 * @section CCA configuration management.
 **************************************************************************************************/

/**
 * @brief Configure radio CCA mode and threshold.
 *
 * @param[in]  p_cca_cfg  A pointer to the CCA configuration structure. Only fields relevant to
 *                        selected mode are updated.
 */
void nrf_802154_cca_cfg_set(const nrf_802154_cca_cfg_t * p_cca_cfg);

/**
 * @brief Get current radio CCA configuration
 *
 * @param[out]  p_cca_cfg  A pointer to the structure for current CCA configuration.
 */
void nrf_802154_cca_cfg_get(nrf_802154_cca_cfg_t * p_cca_cfg);

/***************************************************************************************************
 * @section CSMA-CA procedure.
 **************************************************************************************************/
#if NRF_802154_CSMA_CA_ENABLED
#if NRF_802154_USE_RAW_API

/**
 * @brief Perform CSMA-CA procedure and transmit frame in case of success.
 *
 * End of CSMA-CA procedure is notified by @sa nrf_802154_transmitted_raw() or
 * @sa nrf_802154_transmit_failed() function.
 *
 * @note The driver may be configured to automatically time out waiting for ACK frame depending on
 *       @sa NRF_802154_ACK_TIMEOUT_ENABLED configuration option. If automatic ACK timeout is
 *       disabled, CSMA-CA procedure does not time out waiting for ACK frame in case frame with ACK
 *       request bit set was transmitted. MAC layer should manage timer to time out waiting for ACK
 *       frame. This timer may be started by @sa nrf_802154_tx_started() function. When the timer
 *       expires MAC layer should call @sa nrf_802154_receive() or @sa nrf_802154_sleep() to stop
 *       waiting for ACK frame.
 * @note Before CSMA-CA procedure is used, application should initialize random seed with srand().
 *
 * @param[in]  p_data  Pointer to frame to transmit. @sa nrf_802154_transmit_raw()
 */
void nrf_802154_transmit_csma_ca_raw(const uint8_t * p_data);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Perform CSMA-CA procedure and transmit frame in case of success.
 *
 * End of CSMA-CA procedure is notified by @sa nrf_802154_transmitted() or
 * @sa nrf_802154_transmit_failed() function.
 *
 * @note The driver may be configured to automatically time out waiting for ACK frame depending on
 *       @sa NRF_802154_ACK_TIMEOUT_ENABLED configuration option. If automatic ACK timeout is
 *       disabled, CSMA-CA procedure does not time out waiting for ACK frame in case frame with ACK
 *       request bit set was transmitted. MAC layer should manage timer to time out waiting for ACK
 *       frame. This timer may be started by @sa nrf_802154_tx_started() function. When the timer
 *       expires MAC layer should call @sa nrf_802154_receive() or @sa nrf_802154_sleep() to stop
 *       waiting for ACK frame.
 * @note Before CSMA-CA procedure is used, application should initialize random seed with srand().
 *
 * @param[in]  p_data    Pointer to frame to transmit. @sa nrf_802154_transmit()
 * @param[in]  length    Length of given frame. @sa nrf_802154_transmit()
 */
void nrf_802154_transmit_csma_ca(const uint8_t * p_data, uint8_t length);

#endif // NRF_802154_USE_RAW_API
#endif // NRF_802154_CSMA_CA_ENABLED

/***************************************************************************************************
 * @section ACK timeout procedure.
 **************************************************************************************************/
#if NRF_802154_ACK_TIMEOUT_ENABLED

/**
 * @brief Set timeout time for ACK timeout feature.
 * 
 * Timeout is notified by @sa nrf_802154_transmit_failed() function.
 * 
 * @param[in]  time  Timeout time in us. Timeout is started at the beginning of frame transmission
 *                   (after transmission of PHR). Default value is defined in nrf_802154_config.h
 *                   @sa NRF_802154_ACK_TIMEOUT_DEFAULT_TIMEOUT .
 */
void nrf_802154_ack_timeout_set(uint32_t time);

#endif // NRF_802154_ACK_TIMEOUT_ENABLED

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_H_ */
