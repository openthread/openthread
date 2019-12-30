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
 * @defgroup nrf_802154 802.15.4 radio driver
 * @{
 *
 */

#ifndef NRF_802154_H_
#define NRF_802154_H_

#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_config.h"
#include "nrf_802154_types.h"

#include "nrf_ppi.h"

#if ENABLE_FEM
#include "fem/nrf_fem_protocol_api.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Timestamp value indicating that the timestamp is inaccurate.
 */
#define NRF_802154_NO_TIMESTAMP 0

/**
 * @brief Initializes the 802.15.4 driver.
 *
 * This function initializes the RADIO peripheral in the @ref RADIO_STATE_SLEEP state.
 *
 * @note This function is to be called once, before any other functions from this module.
 */
void nrf_802154_init(void);

/**
 * @brief Deinitializes the 802.15.4 driver.
 *
 * This function deinitializes the RADIO peripheral and resets it to the default state.
 */
void nrf_802154_deinit(void);

#if !NRF_802154_INTERNAL_RADIO_IRQ_HANDLING
/**
 * @brief Handles the interrupt request from the RADIO peripheral.
 *
 * @note If NRF_802154_INTERNAL_RADIO_IRQ_HANDLING is enabled, the driver internally handles the
 *       RADIO IRQ, and this function must not be called.
 *
 * This function is intended for use in an operating system environment, where the OS handles IRQ
 * and indirectly passes it to the driver, or with a RAAL implementation that indirectly passes
 * radio IRQ to the driver (that is, SoftDevice).
 */
void nrf_802154_radio_irq_handler(void);
#endif // !NRF_802154_INTERNAL_RADIO_IRQ_HANDLING

/**
 * @brief Sets the channel on which the radio is to operate.
 *
 * @param[in]  channel  Channel number (11-26).
 */
void nrf_802154_channel_set(uint8_t channel);

/**
 * @brief Gets the channel on which the radio operates.
 *
 * @returns  Channel number (11-26).
 */
uint8_t nrf_802154_channel_get(void);

/**
 * @brief Sets the transmit power.
 *
 * @note The driver recalculates the requested value to the nearest value accepted by the hardware.
 *       The calculation result is rounded up.
 *
 * @param[in]  power  Transmit power in dBm.
 */
void nrf_802154_tx_power_set(int8_t power);

/**
 * @brief Gets the currently set transmit power.
 *
 * @returns Currently used transmit power, in dBm.
 */
int8_t nrf_802154_tx_power_get(void);

/**
 * @defgroup nrf_802154_frontend Frontend Module management
 * @{
 */

#if ENABLE_FEM

/** Structure that contains the run-time configuration of the Frontend Module. */
typedef nrf_fem_control_cfg_t nrf_802154_fem_control_cfg_t;

/** Macro with the default configuration of the Frontend Module. */
#define NRF_802154_FEM_DEFAULT_SETTINGS                                 \
    ((nrf_802154_fem_control_cfg_t) {                                   \
        .pa_cfg = {                                                     \
            .enable = 1,                                                \
            .active_high = 1,                                           \
            .gpio_pin = NRF_FEM_CONTROL_DEFAULT_PA_PIN,                 \
        },                                                              \
        .lna_cfg = {                                                    \
            .enable = 1,                                                \
            .active_high = 1,                                           \
            .gpio_pin = NRF_FEM_CONTROL_DEFAULT_LNA_PIN,                \
        },                                                              \
        .pa_gpiote_ch_id = NRF_FEM_CONTROL_DEFAULT_PA_GPIOTE_CHANNEL,   \
        .lna_gpiote_ch_id = NRF_FEM_CONTROL_DEFAULT_LNA_GPIOTE_CHANNEL, \
        .ppi_ch_id_set = NRF_FEM_CONTROL_DEFAULT_SET_PPI_CHANNEL,       \
        .ppi_ch_id_clr = NRF_FEM_CONTROL_DEFAULT_CLR_PPI_CHANNEL,       \
    })

/**
 * @brief Sets the PA & LNA GPIO toggle configuration.
 *
 * @note This function must not be called when the radio is in use.
 *
 * @note This function is deprecated. Only to be used with Skyworks module.
 *       Consider using nrf_fem_interface_configuration_set instead.
 *
 * @param[in] p_cfg Pointer to the PA & LNA GPIO toggle configuration.
 *
 */
void nrf_802154_fem_control_cfg_set(nrf_802154_fem_control_cfg_t const * const p_cfg);

/**
 * @brief Get the PA & LNA GPIO toggle configuration.
 *
 * @param[out] p_cfg Pointer to the structure for the PA & LNA GPIO toggle configuration.
 *
 * @note This function is deprecated. Only to be used with Skyworks module.
 *       Consider using nrf_fem_interface_configuration_get instead.
 *
 */
void nrf_802154_fem_control_cfg_get(nrf_802154_fem_control_cfg_t * p_cfg);

#endif // ENABLE_FEM

/**
 * @}
 * @defgroup nrf_802154_addresses Setting addresses and PAN ID of the device
 * @{
 */

/**
 * @brief Sets the PAN ID used by the device.
 *
 * @param[in]  p_pan_id  Pointer to the PAN ID (2 bytes, little-endian).
 *
 * This function makes a copy of the PAN ID.
 */
void nrf_802154_pan_id_set(const uint8_t * p_pan_id);

/**
 * @brief Sets the extended address of the device.
 *
 * @param[in]  p_extended_address  Pointer to the extended address (8 bytes, little-endian).
 *
 * This function makes a copy of the address.
 */
void nrf_802154_extended_address_set(const uint8_t * p_extended_address);

/**
 * @brief Sets the short address of the device.
 *
 * @param[in]  p_short_address  Pointer to the short address (2 bytes, little-endian).
 *
 * This function makes a copy of the address.
 */
void nrf_802154_short_address_set(const uint8_t * p_short_address);

/**
 * @}
 * @defgroup nrf_802154_data Functions to calculate data given by the driver
 * @{
 */

/**
 * @brief  Converts the energy level received during the energy detection procedure to a dBm value.
 *
 * @param[in]  energy_level  Energy level passed by @ref nrf_802154_energy_detected.
 *
 * @return  Result of the energy detection procedure in dBm.
 */
int8_t nrf_802154_dbm_from_energy_level_calculate(uint8_t energy_level);

/**
 * @brief  Converts a given dBm level to a CCA energy detection threshold value.
 *
 * @param[in]  dbm  Energy level in dBm used to calculate the CCAEDTHRES value.
 *
 * @return  Energy level value corresponding to the given dBm level that is to be written to
 *          the CCACTRL register.
 */
uint8_t nrf_802154_ccaedthres_from_dbm_calculate(int8_t dbm);

/**
 * @brief  Calculates the timestamp of the first symbol of the preamble in a received frame.
 *
 * @param[in]  end_timestamp  Timestamp of the end of the last symbol in the frame,
 *                            in microseconds.
 * @param[in]  psdu_length    Number of bytes in the frame PSDU.
 *
 * @return  Timestamp of the beginning of the first preamble symbol of a given frame,
 *          in microseconds.
 */
uint32_t nrf_802154_first_symbol_timestamp_get(uint32_t end_timestamp, uint8_t psdu_length);

/**
 * @}
 * @defgroup nrf_802154_transitions Functions to request FSM transitions and check current state
 * @{
 */

/**
 * @brief Gets the current state of the radio.
 */
nrf_802154_state_t nrf_802154_state_get(void);

/**
 * @brief Changes the radio state to the @ref RADIO_STATE_SLEEP state.
 *
 * The sleep state is the lowest power state. In this state, the radio cannot transmit or receive
 * frames. It is the only state in which the driver releases the high-frequency clock and does not
 * request timeslots from a radio arbiter.
 *
 * @note If another module requests it, the high-frequency clock may be enabled even in the radio
 *       sleep state.
 *
 * @retval  true   The radio changes its state to the low power mode.
 * @retval  false  The driver could not schedule changing state.
 */
bool nrf_802154_sleep(void);

/**
 * @brief Changes the radio state to the @ref RADIO_STATE_SLEEP state if the radio is idle.
 *
 * The sleep state is the lowest power state. In this state, the radio cannot transmit or receive
 * frames. It is the only state in which the driver releases the high-frequency clock and does not
 * request timeslots from a radio arbiter.
 *
 * @note If another module requests it, the high-frequency clock may be enabled even in the radio
 *       sleep state.
 *
 * @retval  NRF_802154_SLEEP_ERROR_NONE  The radio changes its state to the low power mode.
 * @retval  NRF_802154_SLEEP_ERROR_BUSY  The driver could not schedule changing state.
 */
nrf_802154_sleep_error_t nrf_802154_sleep_if_idle(void);

/**
 * @brief Changes the radio state to @ref RADIO_STATE_RX.
 *
 * In the receive state, the radio receives frames and may automatically send ACK frames when
 * appropriate. The received frame is reported to the higher layer by a call to
 * @ref nrf_802154_received.
 *
 * @retval  true   The radio enters the receive state.
 * @retval  false  The driver could not enter the receive state.
 */
bool nrf_802154_receive(void);

/**
 * @brief Requests reception at the specified time.
 *
 * This function works as a delayed version of @ref nrf_802154_receive. It is asynchronous.
 * It queues the delayed reception using the Radio Scheduler module.
 * If the delayed reception cannot be performed (@ref nrf_802154_receive_at would return false)
 * or the requested reception timeslot is denied, @ref nrf_drv_radio802154_receive_failed is called
 * with the @ref NRF_802154_RX_ERROR_DELAYED_TIMESLOT_DENIED argument.
 *
 * If the requested reception time is in the past, the function returns false and does not
 * schedule reception.
 *
 * A scheduled reception can be cancelled by a call to @ref nrf_802154_receive_at_cancel.
 *
 * @param[in]  t0       Base of delay time - absolute time used by the Timer Scheduler,
 *                      in microseconds (us).
 * @param[in]  dt       Delta of delay time from @p t0, in microseconds (us).
 * @param[in]  timeout  Reception timeout (counted from @p t0 + @p dt), in microseconds (us).
 * @param[in]  channel  Radio channel on which the frame is to be received.
 *
 * @retval  true   The reception procedure was scheduled.
 * @retval  false  The driver could not schedule the reception procedure.
 */
bool nrf_802154_receive_at(uint32_t t0,
                           uint32_t dt,
                           uint32_t timeout,
                           uint8_t  channel);

/**
 * @brief Cancels a delayed reception scheduled by a call to @ref nrf_802154_receive_at.
 *
 * If the receive window has been scheduled but has not started yet, this function prevents
 * entering the receive window. If the receive window has been scheduled and has already started,
 * the radio remains in the receive state, but a window timeout will not be reported.
 *
 * @retval  true    The delayed reception was scheduled and successfully cancelled.
 * @retval  false   No delayed reception was scheduled.
 */
bool nrf_802154_receive_at_cancel(void);

#if NRF_802154_USE_RAW_API
/**
 * @brief Changes the radio state to @ref RADIO_STATE_TX.
 *
 * @note If the CPU is halted or interrupted while this function is executed,
 *       @ref nrf_802154_transmitted or @ref nrf_802154_transmit_failed can be called before this
 *       function returns a result.
 *
 * @note This function is implemented in zero-copy fashion. It passes the given buffer pointer to
 *       the RADIO peripheral.
 *
 * In the transmit state, the radio transmits a given frame. If requested, it waits for
 * an ACK frame. Depending on @ref NRF_802154_ACK_TIMEOUT_ENABLED, the radio driver automatically
 * stops waiting for an ACK frame or waits indefinitely for an ACK frame. If it is configured to
 * wait, the MAC layer is responsible for calling @ref nrf_802154_receive or
 * @ref nrf_802154_sleep after the ACK timeout.
 * The transmission result is reported to the higher layer by calls to @ref nrf_802154_transmitted
 * or @ref nrf_802154_transmit_failed.
 *
 * @verbatim
 * p_data
 * v
 * +-----+-----------------------------------------------------------+------------+
 * | PHR | MAC header and payload                                    | FCS        |
 * +-----+-----------------------------------------------------------+------------+
 *       |                                                                        |
 *       | <---------------------------- PHR -----------------------------------> |
 * @endverbatim
 *
 * @param[in]  p_data  Pointer to the array with data to transmit. The first byte must contain frame
 *                     length (including PHR and FCS). The following bytes contain data. The CRC is
 *                     computed automatically by the radio hardware. Therefore, the FCS field can
 *                     contain any bytes.
 * @param[in]  cca     If the driver is to perform a CCA procedure before transmission.
 *
 * @retval  true   The transmission procedure was scheduled.
 * @retval  false  The driver could not schedule the transmission procedure.
 */
bool nrf_802154_transmit_raw(const uint8_t * p_data, bool cca);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Changes the radio state to transmit.
 *
 * @note If the CPU is halted or interrupted while this function is executed,
 *       @ref nrf_802154_transmitted or @ref nrf_802154_transmit_failed must be called before this
 *       function returns a result.
 *
 * @note This function copies the given buffer. It maintains an internal buffer, which is used to
 *       make a frame copy. To prevent unnecessary memory consumption and to perform zero-copy
 *       transmission, use @ref nrf_802154_transmit_raw instead.
 *
 * In the transmit state, the radio transmits a given frame. If requested, it waits for
 * an ACK frame. Depending on @ref NRF_802154_ACK_TIMEOUT_ENABLED, the radio driver automatically
 * stops waiting for an ACK frame or waits indefinitely for an ACK frame. If it is configured to
 * wait, the MAC layer is responsible for calling @ref nrf_802154_receive or
 * @ref nrf_802154_sleep after the ACK timeout.
 * The transmission result is reported to the higher layer by calls to @ref nrf_802154_transmitted
 * or @ref nrf_802154_transmit_failed.
 *
 * @verbatim
 *       p_data
 *       v
 * +-----+-----------------------------------------------------------+------------+
 * | PHR | MAC header and payload                                    | FCS        |
 * +-----+-----------------------------------------------------------+------------+
 *       |                                                           |
 *       | <------------------ length -----------------------------> |
 * @endverbatim
 *
 * @param[in]  p_data  Pointer to the array with the payload of data to transmit. The array should
 *                     exclude PHR or FCS fields of the 802.15.4 frame.
 * @param[in]  length  Length of the given frame. This value must exclude PHR and FCS fields from
 *                     the given frame (exact size of buffer pointed to by @p p_data).
 * @param[in]  cca     If the driver is to perform a CCA procedure before transmission.
 *
 * @retval  true   The transmission procedure was scheduled.
 * @retval  false  The driver could not schedule the transmission procedure.
 */
bool nrf_802154_transmit(const uint8_t * p_data, uint8_t length, bool cca);

#endif // NRF_802154_USE_RAW_API

/**
 * @brief Requests transmission at the specified time.
 *
 * @note This function is implemented in a zero-copy fashion. It passes the given buffer pointer to
 *       the RADIO peripheral.
 *
 * This function works as a delayed version of @ref nrf_802154_transmit_raw. It is asynchronous.
 * It queues the delayed transmission using the Radio Scheduler module and performs it
 * at the specified time.
 *
 * If the delayed transmission is successfully performed, @ref nrf_802154_transmitted is called.
 * If the delayed transmission cannot be performed (@ref nrf_802154_transmit_raw would return false)
 * or the requested transmission timeslot is denied, @ref nrf_802154_transmit_failed with the
 * @ref NRF_802154_TX_ERROR_TIMESLOT_DENIED argument is called.
 *
 * This function is designed to transmit the first symbol of SHR at the given time.
 *
 * If the requested transmission time is in the past, the function returns false and does not
 * schedule transmission.
 *
 * A successfully scheduled transmission can be cancelled by a call
 * to @ref nrf_802154_transmit_at_cancel.
 *
 * @param[in]  p_data   Pointer to the array with data to transmit. The first byte must contain
 *                      the frame length (including PHR and FCS). The following bytes contain data.
 *                      The CRC is computed automatically by the radio hardware. Therefore, the FCS
 *                      field can contain any bytes.
 * @param[in]  cca      If the driver is to perform a CCA procedure before transmission.
 * @param[in]  t0       Base of delay time - absolute time used by the Timer Scheduler,
 *                      in microseconds (us).
 * @param[in]  dt       Delta of delay time from @p t0, in microseconds (us).
 * @param[in]  channel  Radio channel on which the frame is to be transmitted.
 *
 * @retval  true   The transmission procedure was scheduled.
 * @retval  false  The driver could not schedule the transmission procedure.
 */
bool nrf_802154_transmit_raw_at(const uint8_t * p_data,
                                bool            cca,
                                uint32_t        t0,
                                uint32_t        dt,
                                uint8_t         channel);

/**
 * @brief Cancels a delayed transmission scheduled by a call to @ref nrf_802154_transmit_raw_at.
 *
 * If a delayed transmission has been scheduled but the transmission has not been started yet,
 * a call to this function prevents the transmission. If the transmission is ongoing,
 * it will not be aborted.
 *
 * If a delayed transmission has not been scheduled (or has already finished), this function does
 * not change state and returns false.
 *
 * @retval  true    The delayed transmission was scheduled and successfully cancelled.
 * @retval  false   No delayed transmission was scheduled.
 */
bool nrf_802154_transmit_at_cancel(void);

/**
 * @brief Changes the radio state to energy detection.
 *
 * In the energy detection state, the radio detects the maximum energy for a given time.
 * The result of the detection is reported to the higher layer by @ref nrf_802154_energy_detected.
 *
 * @note @ref nrf_802154_energy_detected can be called before this function returns a result.
 * @note Performing the energy detection procedure can take longer than requested in @p time_us.
 *       The procedure is performed only during the timeslots granted by a radio arbiter.
 *       It can be interrupted by other protocols using the radio hardware. If the procedure is
 *       interrupted, it is automatically continued and the sum of time periods during which the
 *       procedure is carried out is not less than the requested @p time_us.
 *
 * @param[in]  time_us   Duration of energy detection procedure. The given value is rounded up to
 *                       multiplication of 8 symbols (128 us).
 *
 * @retval  true   The energy detection procedure was scheduled.
 * @retval  false  The driver could not schedule the energy detection procedure.
 */
bool nrf_802154_energy_detection(uint32_t time_us);

/**
 * @brief Changes the radio state to @ref RADIO_STATE_CCA.
 *
 * @note @ref nrf_802154_cca_done can be called before this function returns a result.
 *
 * In the CCA state, the radio verifies if the channel is clear. The result of the verification is
 * reported to the higher layer by @ref nrf_802154_cca_done.
 *
 * @retval  true   The CCA procedure was scheduled.
 * @retval  false  The driver could not schedule the CCA procedure.
 */
bool nrf_802154_cca(void);

/**
 * @brief Changes the radio state to continuous carrier.
 *
 * @note When the radio is emitting continuous carrier signals, it blocks all transmissions on the
 *       selected channel. This function is to be called only during radio tests. Do not
 *       use it during normal device operation.
 *
 * @retval  true   The continuous carrier procedure was scheduled.
 * @retval  false  The driver could not schedule the continuous carrier procedure.
 */
bool nrf_802154_continuous_carrier(void);

/**
 * @}
 * @defgroup nrf_802154_calls Calls to higher layer
 * @{
 */

/**
 * @brief Notifies about the start of the ACK frame transmission.
 *
 * @note This function must be very short to prevent dropping frames by the driver.
 *
 * @param[in]  p_data  Pointer to a buffer with PHR and PSDU of the ACK frame.
 */
extern void nrf_802154_tx_ack_started(const uint8_t * p_data);

#if NRF_802154_USE_RAW_API

/**
 * @brief Notifies that a frame was received.
 *
 * @note The buffer pointed to by @p p_data is not modified by the radio driver (and cannot be used
 *       to receive a frame) until @ref nrf_802154_buffer_free_raw is called.
 * @note The buffer pointed to by @p p_data may be modified by the function handler (and other
 *       modules) until @ref nrf_802154_buffer_free_raw is called.
 *
 * @verbatim
 * p_data
 * v
 * +-----+-----------------------------------------------------------+------------+
 * | PHR | MAC Header and payload                                    | FCS        |
 * +-----+-----------------------------------------------------------+------------+
 *       |                                                                        |
 *       | <---------------------------- PHR -----------------------------------> |
 * @endverbatim
 *
 * @param[in]  p_data  Pointer to a buffer that contains PHR and PSDU of the received frame.
 *                     The first byte in the buffer is the length of the frame (PHR). The following
 *                     bytes contain the frame itself (PSDU). The length byte (PHR) includes FCS.
 *                     FCS is already verified by the hardware and may be modified by the hardware.
 * @param[in]  power   RSSI of the received frame.
 * @param[in]  lqi     LQI of the received frame.
 */
extern void nrf_802154_received_raw(uint8_t * p_data, int8_t power, uint8_t lqi);

/**
 * @brief Notifies that a frame was received at a given time.
 *
 * This function works like @ref nrf_802154_received_raw and adds a timestamp to the parameter
 * list.
 *
 * @note The received frame usually contains a timestamp. However, due to a race condition,
 *       the timestamp may be invalid. This erroneous situation is indicated by
 *       the @ref NRF_802154_NO_TIMESTAMP value of the @p time parameter.
 *
 * @param[in]  p_data  Pointer to a buffer that contains PHR and PSDU of the received frame.
 *                     The first byte in the buffer is the length of the frame (PHR). The following
 *                     bytes contain the frame itself (PSDU). The length byte (PHR) includes FCS.
 *                     FCS is already verified by the hardware and may be modified by the hardware.
 * @param[in]  power   RSSI of the received frame.
 * @param[in]  lqi     LQI of the received frame.
 * @param[in]  time    Timestamp taken when the last symbol of the frame was received, in
 *                     microseconds (us), or @ref NRF_802154_NO_TIMESTAMP if the timestamp
 *                     is invalid.
 */
extern void nrf_802154_received_timestamp_raw(uint8_t * p_data,
                                              int8_t    power,
                                              uint8_t   lqi,
                                              uint32_t  time);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Notifies that a frame was received.
 *
 * @note The buffer pointed to by @p p_data is not modified by the radio driver (and cannot
 *       be used to receive a frame) until @ref nrf_802154_buffer_free is called.
 * @note The buffer pointed to by @p p_data can be modified by the function handler (and other
 *       modules) until @ref nrf_802154_buffer_free is called.
 *
 * @verbatim
 *       p_data
 *       v
 * +-----+-----------------------------------------------------------+------------+
 * | PHR | MAC Header and payload                                    | FCS        |
 * +-----+-----------------------------------------------------------+------------+
 *       |                                                           |
 *       | <------------------ length -----------------------------> |
 * @endverbatim
 *
 * @param[in]  p_data  Pointer to a buffer that contains only the payload of the received frame
 *                     (PSDU without FCS).
 * @param[in]  length  Length of the received payload.
 * @param[in]  power   RSSI of the received frame.
 * @param[in]  lqi     LQI of the received frame.
 */
extern void nrf_802154_received(uint8_t * p_data, uint8_t length, int8_t power, uint8_t lqi);

/**
 * @brief Notifies that a frame was received at a given time.
 *
 * This function works like @ref nrf_802154_received and adds a timestamp to the parameter list.
 *
 * @note The received frame usually contains a timestamp. However, due to a race condition,
 *       the timestamp may be invalid. This erroneous situation is indicated by
 *       the @ref NRF_802154_NO_TIMESTAMP value of the @p time parameter.
 *
 * @param[in]  p_data  Pointer to a buffer that contains only the payload of the received frame
 *                     (PSDU without FCS).
 * @param[in]  length  Length of the received payload.
 * @param[in]  power   RSSI of the received frame.
 * @param[in]  lqi     LQI of the received frame.
 * @param[in]  time    Timestamp taken when the last symbol of the frame was received,
 *                     in microseconds (us), or @ref NRF_802154_NO_TIMESTAMP if the timestamp
 *                     is invalid.
 */
extern void nrf_802154_received_timestamp(uint8_t * p_data,
                                          uint8_t   length,
                                          int8_t    power,
                                          uint8_t   lqi,
                                          uint32_t  time);

#endif // !NRF_802154_USE_RAW_API

/**
 * @brief Notifies that the reception of a frame failed.
 *
 * @param[in]  error  Error code that indicates the reason of the failure.
 */
extern void nrf_802154_receive_failed(nrf_802154_rx_error_t error);

/**
 * @brief Notifies that transmitting a frame has started.
 *
 * @note Usually, @ref nrf_802154_transmitted is called shortly after this function.
 *       However, if the transmit procedure is interrupted, it might happen that
 *       @ref nrf_802154_transmitted is not called.
 * @note This function should be very short to prevent dropping frames by the driver.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the frame being
 *                      transmitted.
 */
extern void nrf_802154_tx_started(const uint8_t * p_frame);

#if NRF_802154_USE_RAW_API

/**
 * @brief Notifies that a frame was transmitted.
 *
 * @note If ACK was requested for the transmitted frame, this function is called after a proper ACK
 *       is received. If ACK was not requested, this function is called just after transmission has
 *       ended.
 * @note The buffer pointed to by @p p_ack is not modified by the radio driver (and cannot be used
 *       to receive a frame) until @ref nrf_802154_buffer_free_raw is called.
 * @note The buffer pointed to by @p p_ack may be modified by the function handler (and other
 *       modules) until @ref nrf_802154_buffer_free_raw is called.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the transmitted frame.
 * @param[in]  p_ack    Pointer to a buffer that contains PHR and PSDU of the received ACK.
 *                      The first byte in the buffer is the length of the frame (PHR). The following
 *                      bytes contain the ACK frame itself (PSDU). The length byte (PHR) includes
 *                      FCS. FCS is already verified by the hardware and may be modified by the
 *                      hardware. If ACK was not requested, @p p_ack is set to NULL.
 * @param[in]  power    RSSI of the received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of the received frame or 0 if ACK was not requested.
 */
extern void nrf_802154_transmitted_raw(const uint8_t * p_frame,
                                       uint8_t       * p_ack,
                                       int8_t          power,
                                       uint8_t         lqi);

/**
 * @brief Notifies that a frame was transmitted.
 *
 * This function works like @ref nrf_802154_transmitted_raw and adds a timestamp to the parameter
 * list.
 *
 * @note @p timestamp may be inaccurate due to software latency (IRQ handling).
 * @note @p timestamp granularity depends on the granularity of the timer driver in the
 *       platform/timer directory.
 * @note Including a timestamp for received frames uses resources like CPU time and memory. If the
 *       timestamp is not required, use @ref nrf_802154_received instead.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the transmitted frame.
 * @param[in]  p_ack    Pointer to a buffer that contains PHR and PSDU of the received ACK.
 *                      The first byte in the buffer is the length of the frame (PHR). The following
 *                      bytes contain the ACK frame itself (PSDU). The length byte (PHR) includes
 *                      FCS. FCS is already verified by the hardware and may be modified by the
 *                      hardware. If ACK was not requested, @p p_ack is set to NULL.
 * @param[in]  power    RSSI of the received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of the received frame or 0 if ACK was not requested.
 * @param[in]  time     Timestamp taken when the last symbol of ACK is received or 0 if ACK was not
 *                      requested.
 */
extern void nrf_802154_transmitted_timestamp_raw(const uint8_t * p_frame,
                                                 uint8_t       * p_ack,
                                                 int8_t          power,
                                                 uint8_t         lqi,
                                                 uint32_t        time);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Notifies that a frame was transmitted.
 *
 * @note If ACK was requested for the transmitted frame, this function is called after a proper ACK
 *       is received. If ACK was not requested, this function is called just after transmission has
 *       ended.
 * @note The buffer pointed to by @p p_ack is not modified by the radio driver (and cannot
 *       be used to receive a frame) until @ref nrf_802154_buffer_free is
 *       called.
 * @note The buffer pointed to by @p p_ack may be modified by the function handler (and other
 *       modules) until @ref nrf_802154_buffer_free is called.
 * @note The next higher layer must handle either @ref nrf_802154_transmitted or
 *       @ref nrf_802154_transmitted_raw. It should not handle both functions.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the transmitted frame.
 * @param[in]  p_ack    Pointer to a buffer that contains only the received ACK payload (PSDU
 *                      excluding FCS).
 *                      If ACK was not requested, @p p_ack is set to NULL.
 * @param[in]  length   Length of the received ACK payload or 0 if ACK was not requested.
 * @param[in]  power    RSSI of the received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of the received frame or 0 if ACK was not requested.
 */
extern void nrf_802154_transmitted(const uint8_t * p_frame,
                                   uint8_t       * p_ack,
                                   uint8_t         length,
                                   int8_t          power,
                                   uint8_t         lqi);

/**
 * @brief Notifies that a frame was transmitted.
 *
 * This function works like @ref nrf_802154_transmitted and adds a timestamp to the parameter
 * list.
 *
 * @note @p timestamp may be inaccurate due to software latency (IRQ handling).
 * @note @p timestamp granularity depends on the granularity of the timer driver
 *       in the platform/timer directory.
 * @note Including a timestamp for received frames uses resources like CPU time and memory. If the
 *       timestamp is not required, use @ref nrf_802154_received instead.
 *
 * @param[in]  p_frame  Pointer to the buffer containing PHR and PSDU of the transmitted frame.
 * @param[in]  p_ack    Pointer to the buffer containing only the received ACK payload (PSDU
 *                      excluding FCS).
 *                      If ACK was not requested, @p p_ack is set to NULL.
 * @param[in]  length   Length of the received ACK payload.
 * @param[in]  power    RSSI of the received frame or 0 if ACK was not requested.
 * @param[in]  lqi      LQI of the received frame or 0 if ACK was not requested.
 * @param[in]  time     Timestamp taken when the last symbol of ACK is received or 0 if ACK was not
 *                      requested.
 */
extern void nrf_802154_transmitted_timestamp(const uint8_t * p_frame,
                                             uint8_t       * p_ack,
                                             uint8_t         length,
                                             int8_t          power,
                                             uint8_t         lqi,
                                             uint32_t        time);

#endif // !NRF_802154_USE_RAW_API

/**
 * @brief Notifies that a frame was not transmitted due to a busy channel.
 *
 * This function is called if the transmission procedure fails.
 *
 * @param[in]  p_frame  Pointer to a buffer that contains PHR and PSDU of the frame that was not
 *                      transmitted.
 * @param[in]  error    Reason of the failure.
 */
extern void nrf_802154_transmit_failed(const uint8_t       * p_frame,
                                       nrf_802154_tx_error_t error);

/**
 * @brief Notifies that the energy detection procedure finished.
 *
 * @note This function passes the EnergyLevel defined in the 802.15.4-2006 specification:
 *       0x00 - 0xff, proportionally to the detected energy level (dBm above receiver sensitivity).
 *       To calculate the result in dBm, use @ref nrf_802154_dbm_from_energy_level_calculate.
 *
 * @param[in]  result  Maximum energy detected during the energy detection procedure.
 */
extern void nrf_802154_energy_detected(uint8_t result);

/**
 * @brief Notifies that the energy detection procedure failed.
 *
 * @param[in]  error  Reason of the failure.
 */
extern void nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error);

/**
 * @brief Notifies that the CCA procedure has finished.
 *
 * @param[in]  channel_free  Indication if the channel is free.
 */
extern void nrf_802154_cca_done(bool channel_free);

/**
 * @brief Notifies that the CCA procedure failed.
 *
 * @param[in]  error  Reason of the failure.
 */
extern void nrf_802154_cca_failed(nrf_802154_cca_error_t error);

/**
 * @}
 * @defgroup nrf_802154_memman Driver memory management
 * @{
 */

#if NRF_802154_USE_RAW_API

/**
 * @brief Notifies the driver that the buffer containing the received frame is not used anymore.
 *
 * @note The buffer pointed to by @p p_data may be modified by this function.
 * @note This function can be safely called only from the main context. To free the buffer from
 *       a callback or the IRQ context, use @ref nrf_802154_buffer_free_immediately_raw.
 *
 * @param[in]  p_data  Pointer to the buffer containing the received data that is no longer needed
 *                     by the higher layer.
 */
void nrf_802154_buffer_free_raw(uint8_t * p_data);

/**
 * @brief Notifies the driver that the buffer containing the received frame is not used anymore.
 *
 * @note The buffer pointed to by @p p_data may be modified by this function.
 * @note This function can be safely called from any context. If the driver is busy processing
 *       a request called from a context with lower priority, this function returns false and
 *       the caller should free the buffer later.
 *
 * @param[in]  p_data  Pointer to the buffer containing the received data that is no longer needed
 *                     by the higher layer.
 *
 * @retval true   Buffer was freed successfully.
 * @retval false  Buffer cannot be freed right now due to ongoing operation.
 */
bool nrf_802154_buffer_free_immediately_raw(uint8_t * p_data);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Notifies the driver that the buffer containing the received frame is not used anymore.
 *
 * @note The buffer pointed to by @p p_data may be modified by this function.
 * @note This function can be safely called only from the main context. To free the buffer from
 *       a callback or IRQ context, use @ref nrf_802154_buffer_free_immediately.
 *
 * @param[in]  p_data  Pointer to the buffer containing the received data that is no longer needed
 *                     by the higher layer.
 */
void nrf_802154_buffer_free(uint8_t * p_data);

/**
 * @brief Notifies the driver that the buffer containing the received frame is not used anymore.
 *
 * @note The buffer pointed to by @p p_data may be modified by this function.
 * @note This function can be safely called from any context. If the driver is busy processing
 *       a request called from a context with lower priority, this function returns false and
 *       the caller should free the buffer later.
 *
 * @param[in]  p_data  Pointer to the buffer containing the received data that is no longer needed
 *                     by the higher layer.
 *
 * @retval true   Buffer was freed successfully.
 * @retval false  Buffer cannot be freed right now due to ongoing operation.
 */
bool nrf_802154_buffer_free_immediately(uint8_t * p_data);

#endif // NRF_802154_USE_RAW_API

/**
 * @}
 * @defgroup nrf_802154_rssi RSSI measurement function
 * @{
 */

/**
 * @brief Begins the RSSI measurement.
 *
 * @note This function is to be called in the @ref RADIO_STATE_RX state.
 *
 * The result will be available after the measurement process is finished. The result can be read by
 * @ref nrf_802154_rssi_last_get. Check the documentation of the RADIO peripheral to check
 * the duration of the RSSI measurement procedure.
 *
 * @retval true  RSSI measurement successfully requested.
 * @retval false RSSI measurement cannot be scheduled at the moment.
 */
bool nrf_802154_rssi_measure_begin(void);

/**
 * @brief Gets the result of the last RSSI measurement.
 *
 * @returns RSSI measurement result, in dBm.
 */
int8_t nrf_802154_rssi_last_get(void);

/**
 * @}
 * @defgroup nrf_802154_prom Promiscuous mode
 * @{
 */

/**
 * @brief Enables or disables the promiscuous radio mode.
 *
 * @note The promiscuous mode is disabled by default.
 *
 * In the promiscuous mode, the driver notifies the higher layer that it received any frame
 * (regardless frame type or destination address).
 * In normal mode (not promiscuous), the higher layer is not notified about ACK frames and frames
 * with unknown type. Also, frames with a destination address not matching the device address are
 * ignored.
 *
 * @param[in]  enabled  If the promiscuous mode is to be enabled.
 */
void nrf_802154_promiscuous_set(bool enabled);

/**
 * @brief Checks if the radio is in the promiscuous mode.
 *
 * @retval True   Radio is in the promiscuous mode.
 * @retval False  Radio is not in the promiscuous mode.
 */
bool nrf_802154_promiscuous_get(void);

/**
 * @}
 * @defgroup nrf_802154_autoack Auto ACK management
 * @{
 */

/**
 * @brief Enables or disables the automatic acknowledgments (auto ACK).
 *
 * @note The auto ACK is enabled by default.
 *
 * If the auto ACK is enabled, the driver prepares and sends ACK frames automatically
 * aTurnaroundTime (192 us) after the proper frame is received. The driver prepares an ACK frame
 * according to the data provided by @ref nrf_802154_ack_data_set.
 * When the auto ACK is enabled, the driver notifies the next higher layer about the received frame
 * after the ACK frame is transmitted.
 * If the auto ACK is disabled, the driver does not transmit ACK frames. It notifies the next higher
 * layer about the received frames when a frame is received. In this mode, the next higher layer is
 * responsible for sending the ACK frame. ACK frames should be sent using @ref nrf_802154_transmit.
 *
 * @param[in]  enabled  If the auto ACK should be enabled.
 */
void nrf_802154_auto_ack_set(bool enabled);

/**
 * @brief Checks if the auto ACK is enabled.
 *
 * @retval True   Auto ACK is enabled.
 * @retval False  Auto ACK is disabled.
 */
bool nrf_802154_auto_ack_get(void);

/**
 * @brief Configures the device as the PAN coordinator.
 *
 * @note That information is used for packet filtering.
 *
 * @param[in]  enabled  The radio is configured as the PAN coordinator.
 */
void nrf_802154_pan_coord_set(bool enabled);

/**
 * @brief Checks if the radio is configured as the PAN coordinator.
 *
 * @retval  true   The radio is configured as the PAN coordinator.
 * @retval  false  The radio is not configured as the PAN coordinator.
 */
bool nrf_802154_pan_coord_get(void);

/**
 * @brief Select the source matching algorithm.
 *
 * @note This method should be called after driver initialization, but before transceiver is enabled.
 *
 * When calling @ref nrf_802154_ack_data_pending_bit_should_be_set, one of several algorithms
 * for source address matching will be chosen. To ensure a specific algorithm is selected,
 * call this function before @ref rf_802154_ack_data_pending_bit_should_be_set.
 *
 * @param[in]  match_method Source address matching method to be used.
 */
void nrf_802154_src_addr_matching_method_set(nrf_802154_src_addr_match_t match_method);

/**
 * @brief Adds the address of a peer node for which the provided ACK data
 * is to be added to the pending bit list.
 *
 * The pending bit list works differently, depending on the upper layer for which the source
 * address matching method is selected:
 *   - For Thread, @ref NRF_802154_SRC_ADDR_MATCH_THREAD
 *   - For Zigbee, @ref NRF_802154_SRC_ADDR_MATCH_ZIGBEE
 *   - For Standard-compliant, @ref NRF_802154_SRC_ADDR_MATCH_ALWAYS_1
 * For more information, see @ref nrf_802154_src_addr_match_t.
 *
 * The method can be set during initialization phase by calling @ref nrf_802154_src_matching_method.
 *
 * @param[in]  p_addr    Array of bytes containing the address of the node (little-endian).
 * @param[in]  extended  If the given address is an extended MAC address or a short MAC address.
 * @param[in]  p_data    Pointer to the buffer containing data to be set.
 * @param[in]  length    Length of @p p_data.
 * @param[in]  data_type Type of data to be set. Refer to the @ref nrf_802154_ack_data_t type.
 *
 * @retval True   Address successfully added to the list.
 * @retval False  Not enough memory to store this address in the list.
 */
bool nrf_802154_ack_data_set(const uint8_t * p_addr,
                             bool            extended,
                             const void    * p_data,
                             uint16_t        length,
                             uint8_t         data_type);

/**
 * @brief Removes the address of a peer node for which the ACK data is set from the pending bit list.
 *
 * The ACK data that was previously set for the given address is automatically removed.
 *
 * The pending bit list works differently, depending on the upper layer for which the source
 * address matching method is selected:
 *   - For Thread, @ref NRF_802154_SRC_ADDR_MATCH_THREAD
 *   - For Zigbee, @ref NRF_802154_SRC_ADDR_MATCH_ZIGBEE
 *   - For Standard-compliant, @ref NRF_802154_SRC_ADDR_MATCH_ALWAYS_1
 * For more information, see @ref nrf_802154_src_addr_match_t.
 *
 * The method can be set during initialization phase by calling @ref nrf_802154_src_matching_method.
 *
 * @param[in]  p_addr    Array of bytes containing the address of the node (little-endian).
 * @param[in]  extended  If the given address is an extended MAC address or a short MAC address.
 * @param[in]  data_type Type of data to be removed. Refer to the @ref nrf_802154_ack_data_t type.
 *
 * @retval True   Address removed from the list.
 * @retval False  Address not found in the list.
 */
bool nrf_802154_ack_data_clear(const uint8_t * p_addr, bool extended, uint8_t data_type);

/**
 * @brief Enables or disables setting a pending bit in automatically transmitted ACK frames.
 *
 * @note Setting a pending bit in automatically transmitted ACK frames is enabled by default.
 *
 * The radio driver automatically sends ACK frames in response frames destined for this node with
 * the ACK Request bit set. The pending bit in the ACK frame can be set or cleared regarding data
 * in the indirect queue destined for the ACK destination.
 *
 * If setting a pending bit in ACK frames is disabled, the pending bit in every ACK frame is set.
 * If setting a pending bit in ACK frames is enabled, the radio driver checks if there is data
 * in the indirect queue destined for the  ACK destination. If there is no such data,
 * the pending bit is cleared.
 *
 * @note Due to the ISR latency, the radio driver might not be able to verify if there is data
 *       in the indirect queue before ACK is sent. In this case, the pending bit is set.
 *
 * @param[in]  enabled  If setting a pending bit in ACK frames is enabled.
 */
void nrf_802154_auto_pending_bit_set(bool enabled);

/**
 * @brief Adds the address of a peer node to the pending bit list.
 *
 * The pending bit list works differently, depending on the upper layer for which the source
 * address matching method is selected:
 *   - For Thread, @ref NRF_802154_SRC_ADDR_MATCH_THREAD
 *   - For Zigbee, @ref NRF_802154_SRC_ADDR_MATCH_ZIGBEE
 *   - For Standard-compliant, @ref NRF_802154_SRC_ADDR_MATCH_ALWAYS_1
 * For more information, see @ref nrf_802154_src_addr_match_t.
 *
 * The method can be set during initialization phase by calling @ref nrf_802154_src_matching_method.
 *
 * @note This function makes a copy of the given address.
 *
 * @param[in]  p_addr    Array of bytes containing the address of the node (little-endian).
 * @param[in]  extended  If the given address is an extended MAC address or a short MAC address.
 *
 * @retval True   The address is successfully added to the list.
 * @retval False  Not enough memory to store the address in the list.
 */
bool nrf_802154_pending_bit_for_addr_set(const uint8_t * p_addr, bool extended);

/**
 * @brief Removes address of a peer node from the pending bit list.
 *
 * The pending bit list works differently, depending on the upper layer for which the source
 * address matching method is selected:
 *   - For Thread, @ref NRF_802154_SRC_ADDR_MATCH_THREAD
 *   - For Zigbee, @ref NRF_802154_SRC_ADDR_MATCH_ZIGBEE
 *   - For Standard-compliant, @ref NRF_802154_SRC_ADDR_MATCH_ALWAYS_1
 * For more information, see @ref nrf_802154_src_addr_match_t.
 *
 * The method can be set during initialization phase by calling @ref nrf_802154_src_matching_method.
 *
 * @param[in]  p_addr    Array of bytes containing the address of the node (little-endian).
 * @param[in]  extended  If the given address is an extended MAC address or a short MAC address.
 *
 * @retval True   The address is successfully removed from the list.
 * @retval False  No such address in the list.
 */
bool nrf_802154_pending_bit_for_addr_clear(const uint8_t * p_addr, bool extended);

/**
 * @brief Removes all addresses of a given type from the pending bit list.
 *
 * The pending bit list works differently, depending on the upper layer for which the source
 * address matching method is selected:
 *   - For Thread, @ref NRF_802154_SRC_ADDR_MATCH_THREAD
 *   - For Zigbee, @ref NRF_802154_SRC_ADDR_MATCH_ZIGBEE
 *   - For Standard-compliant, @ref NRF_802154_SRC_ADDR_MATCH_ALWAYS_1
 * For more information, see @ref nrf_802154_src_addr_match_t.
 *
 * The method can be set during initialization phase by calling @ref nrf_802154_src_matching_method.
 *
 * @param[in]  extended  If the function is to remove all extended MAC addresses or all short
 *                       addresses.
 */
void nrf_802154_pending_bit_for_addr_reset(bool extended);

/**
 * @}
 * @defgroup nrf_802154_cca CCA configuration management
 * @{
 */

/**
 * @brief Configures the radio CCA mode and threshold.
 *
 * @param[in]  p_cca_cfg  Pointer to the CCA configuration structure. Only fields relevant to
 *                        the selected mode are updated.
 */
void nrf_802154_cca_cfg_set(const nrf_802154_cca_cfg_t * p_cca_cfg);

/**
 * @brief Gets the current radio CCA configuration.
 *
 * @param[out]  p_cca_cfg  Pointer to the structure for the current CCA configuration.
 */
void nrf_802154_cca_cfg_get(nrf_802154_cca_cfg_t * p_cca_cfg);

/**
 * @}
 * @defgroup nrf_802154_csma CSMA-CA procedure
 * @{
 */
#if NRF_802154_CSMA_CA_ENABLED
#if NRF_802154_USE_RAW_API

/**
 * @brief Performs the CSMA-CA procedure and transmits a frame in case of success.
 *
 * The end of the CSMA-CA procedure is notified by @ref nrf_802154_transmitted_raw or
 * @ref nrf_802154_transmit_failed.
 *
 * @note The driver may be configured to automatically time out waiting for an ACK frame depending
 *       on @ref NRF_802154_ACK_TIMEOUT_ENABLED. If the automatic ACK timeout is disabled,
 *       the CSMA-CA procedure does not time out waiting for an ACK frame if a frame
 *       with the ACK request bit set was transmitted. The MAC layer is expected to manage the timer
 *       to time out waiting for the ACK frame. This timer can be started
 *       by @ref nrf_802154_tx_started. When the timer expires, the MAC layer is expected
 *       to call @ref nrf_802154_receive or @ref nrf_802154_sleep to stop waiting for the ACK frame.
 *
 * @param[in]  p_data  Pointer to the frame to transmit. See also @ref nrf_802154_transmit_raw.
 */
void nrf_802154_transmit_csma_ca_raw(const uint8_t * p_data);

#else // NRF_802154_USE_RAW_API

/**
 * @brief Performs the CSMA-CA procedure and transmits a frame in case of success.
 *
 * The end of the CSMA-CA procedure is notified by @ref nrf_802154_transmitted or
 * @ref nrf_802154_transmit_failed.
 *
 * @note The driver may be configured to automatically time out waiting for an ACK frame depending
 *       on @ref NRF_802154_ACK_TIMEOUT_ENABLED. If the automatic ACK timeout is disabled,
 *       the CSMA-CA procedure does not time out waiting for an ACK frame if a frame
 *       with the ACK request bit set was transmitted. The MAC layer is expected to manage the timer
 *       to time out waiting for the ACK frame. This timer can be started
 *       by @ref nrf_802154_tx_started. When the timer expires, the MAC layer is expected
 *       to call @ref nrf_802154_receive or @ref nrf_802154_sleep to stop waiting for the ACK frame.
 *
 * @param[in]  p_data    Pointer to the frame to transmit. See also @ref nrf_802154_transmit.
 * @param[in]  length    Length of the given frame. See also @ref nrf_802154_transmit.
 */
void nrf_802154_transmit_csma_ca(const uint8_t * p_data, uint8_t length);

#endif // NRF_802154_USE_RAW_API
#endif // NRF_802154_CSMA_CA_ENABLED

/**
 * @}
 * @defgroup nrf_802154_timeout ACK timeout procedure
 * @{
 */
#if NRF_802154_ACK_TIMEOUT_ENABLED

/**
 * @brief Sets timeout for the ACK timeout feature.
 *
 * A timeout is notified by @ref nrf_802154_transmit_failed.
 *
 * @param[in]  time  Timeout in microseconds (us).
 *                   A default value is defined in nrf_802154_config.h.
 */
void nrf_802154_ack_timeout_set(uint32_t time);

#endif // NRF_802154_ACK_TIMEOUT_ENABLED

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_H_ */

/** @} */
