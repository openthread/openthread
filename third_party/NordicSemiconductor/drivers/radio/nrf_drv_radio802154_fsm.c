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
 * @file
 *   This file implements Finite State Machine of nRF 802.15.4 radio driver.
 *
 */

#include "nrf_drv_radio802154_fsm.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nrf_drv_radio802154.h"
#include "nrf_drv_radio802154_ack_pending_bit.h"
#include "nrf_drv_radio802154_config.h"
#include "nrf_drv_radio802154_const.h"
#include "nrf_drv_radio802154_debug.h"
#include "nrf_drv_radio802154_notification.h"
#include "nrf_drv_radio802154_pib.h"
#include "nrf_drv_radio802154_priority_drop.h"
#include "nrf_drv_radio802154_procedures_duration.h"
#include "nrf_drv_radio802154_rx_buffer.h"
#include "hal/nrf_radio.h"
#include "raal/nrf_raal_api.h"

/// Value set to SHORTS register when no shorts should be enabled.
#define SHORTS_IDLE         0
/// Value set to SHORTS register when receiver is waiting for incoming frame.
#define SHORTS_RX_INITIAL   (NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_DISABLED_TXEN_MASK | \
                             NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK)
/// Value set to SHORTS register when receiver started receiving a frame.
#define SHORTS_RX_FOLLOWING (NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_READY_START_MASK | \
                             NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK)
/// Value set to SHORTS register when received frame should be acknowledged.
#define SHORTS_TX_ACK       (NRF_RADIO_SHORT_END_DISABLE_MASK)
#if RADIO_SHORT_CCAIDLE_TXEN
/// Value set to SHORTS register during transmission of a frame
#define SHORTS_TX_FRAME     (NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_READY_START_MASK | \
                             NRF_RADIO_SHORT_CCAIDLE_TXEN_MASK)
#else
/// Value set to SHORTS register during transmission of a frame
#define SHORTS_TX_FRAME     (NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_READY_START_MASK)
#endif

/// Delay before sending ACK (12sym = 192uS)
#define TIFS_ACK_US         192
/// Delay before first check of received frame: 16 bits is MAC Frame Control field.
#define BCC_INIT            (2 * 8)
/// Delay before second check of received frame if destination address is short.
#define BCC_SHORT_ADDR      ((DEST_ADDR_OFFSET + SHORT_ADDRESS_SIZE) * 8)
/// Delay before second check of received frame if destination address is extended.
#define BCC_EXTENDED_ADDR   ((DEST_ADDR_OFFSET + EXTENDED_ADDRESS_SIZE) * 8)

/// Duration of single iteration of Energy Detection procedure
#define ED_ITER_DURATION   128U
/// Overhead of hardware preparation for ED procedure (aTurnaroundTime) [number of iterations]
#define ED_ITERS_OVERHEAD  2U

#define CRC_LENGTH      2         ///< Length of CRC in 802.15.4 frames [bytes]
#define CRC_POLYNOMIAL  0x011021  ///< Polynomial used for CRC calculation in 802.15.4 frames

#define MHMU_MASK               0xff000700  ///< Mask of known bytes in ACK packet
#define MHMU_PATTERN            0x00000200  ///< Values of known bytes in ACK packet
#define MHMU_PATTERN_DSN_OFFSET 24          ///< Offset of DSN in MHMU_PATTER [bits]

/** Get LQI of given received packet. If CRC is calculated by hardware LQI is included instead of CRC
 *  in the frame. Length is stored in byte with index 0; CRC is 2 last bytes.
 */
#define RX_FRAME_LQI(psdu)  ((psdu)[(psdu)[0] - 1])

#if RADIO_RX_BUFFERS > 1
/// Pointer to currently used receive buffer.
static rx_buffer_t * mp_current_rx_buffer;
#else
/// If there is only one buffer use const pointer to the receive buffer.
static rx_buffer_t * const mp_current_rx_buffer = &nrf_drv_radio802154_rx_buffers[0];
#endif

#if defined ( __GNUC__ )

/// Ack frame buffer (EasyDMA cannot address whole RAM. Place buffer in the special section.)
static uint8_t m_ack_psdu[ACK_LENGTH + 1]
                __attribute__ ((section ("nrf_radio_buffer.m_ack_psdu")));

#elif defined ( __ICCARM__ )

#pragma location="NRF_RADIO_BUFFER"
static uint8_t m_ack_psdu[ACK_LENGTH + 1];

#endif

static const uint8_t * mp_tx_data;      ///< Pointer to data to transmit.
static uint32_t        m_ed_time_left;  ///< Remaining time of current energy detection procedure [us].
static uint8_t         m_ed_result;     ///< Result of current energy detection procedure.

static volatile radio_state_t m_state = RADIO_STATE_SLEEP;  ///< State of the radio driver

typedef struct
{
    bool prevent_ack :1;  ///< If frame being received is not destined to this node (promiscuous mode).
} nrf_radio802154_flags_t;
static nrf_radio802154_flags_t m_flags;  ///< Flags used to store current driver state.

static volatile uint8_t m_mutex;  ///< Mutex preventing race condition.


/***************************************************************************************************
 * @section Mutex
 **************************************************************************************************/

/// Lock mutex to prevent race conditions.
static bool mutex_lock(void)
{
    do
    {
        volatile uint8_t mutex_value = __LDREXB(&m_mutex);

        if (mutex_value)
        {
            return false;
        }
    }
    while (__STREXB(1, &m_mutex));

    __DMB();

    // Driver may be in WAITING_TIMESLOT state if timeslot ended during mutex locking.
    assert(m_state == RADIO_STATE_WAITING_RX_FRAME ||
           m_state == RADIO_STATE_SLEEP ||
           m_state == RADIO_STATE_WAITING_TIMESLOT);

    nrf_drv_radio802154_log(EVENT_MUTEX_LOCK, 0);

    return true;
}

/// Unlock mutex.
static void mutex_unlock(void)
{
    assert(m_state == RADIO_STATE_SLEEP ||
           m_state == RADIO_STATE_WAITING_RX_FRAME);

    nrf_drv_radio802154_log(EVENT_MUTEX_UNLOCK, 0);

    __DMB();
    m_mutex = 0;
}


/***************************************************************************************************
 * @section FSM common operations
 **************************************************************************************************/

/** Set driver state.
 *
 * @param[in]  state  Driver state to set.
 */
static inline void state_set(radio_state_t state)
{
    m_state = state;

    nrf_drv_radio802154_log(EVENT_SET_STATE, (uint32_t)state);
}

/// Common procedure when the driver enters SLEEP state.
static inline void sleep_start(void)
{
    state_set(RADIO_STATE_SLEEP);
    nrf_drv_radio802154_priority_drop_timeslot_exit();
    mutex_unlock();
}

/// Start receiver to wait for frames.
static inline void rx_start(void)
{
    nrf_radio_packet_ptr_set(mp_current_rx_buffer->psdu);
    nrf_radio_task_trigger(NRF_RADIO_TASK_START);
}

/// Start receiver to wait for frame that can be acknowledged.
static inline void rx_frame_start(void)
{
    rx_start();

    // Just after starting receiving to receive buffer set packet pointer to ACK frame that can be
    // sent automatically.
    nrf_radio_packet_ptr_set(m_ack_psdu);
}

/** Get result of last RSSI measurement.
 *
 * @returns  Result of last RSSI measurement [dBm].
 */
static inline int8_t rssi_last_measurement_get(void)
{
    return -((int8_t)nrf_radio_rssi_sample_get());
}

/// Notify MAC layer that a frame was received.
static inline void received_frame_notify(void)
{
    mp_current_rx_buffer->free = false;
    nrf_drv_radio802154_notify_received(mp_current_rx_buffer->psdu,                // data
                                        rssi_last_measurement_get(),               // rssi
                                        RX_FRAME_LQI(mp_current_rx_buffer->psdu)); // lqi
}

/** Set currently used rx buffer to given address.
 *
 * @param[in]  p_rx_buffer  Pointer to receive buffer that should be used now.
 */
static inline void rx_buffer_in_use_set(rx_buffer_t * p_rx_buffer)
{
#if RADIO_RX_BUFFERS > 1
    mp_current_rx_buffer = p_rx_buffer;
#else
    (void) p_rx_buffer;
#endif
}

/** Update CCA configuration in RADIO registers. */
static void cca_configuration_update(void)
{
    nrf_drv_radio802154_cca_cfg_t cca_cfg;
    nrf_drv_radio802154_pib_cca_cfg_get(&cca_cfg);
    nrf_radio_cca_mode_set(cca_cfg.mode);
    nrf_radio_cca_ed_threshold_set(cca_cfg.ed_threshold);
    nrf_radio_cca_corr_threshold_set(cca_cfg.corr_threshold);
    nrf_radio_cca_corr_counter_set(cca_cfg.corr_limit);
}

/***************************************************************************************************
 * @section Radio parameters calculators
 **************************************************************************************************/

/** Set radio channel
 *
 *  @param[in]  channel  Channel number to set (11-26).
 */
static void channel_set(uint8_t channel)
{
    assert(channel >= 11 && channel <= 26);

    nrf_radio_frequency_set(5 + (5 * (channel - 11)));
}


/***************************************************************************************************
 * @section Shorts management
 **************************************************************************************************/

/// Disable peripheral shorts.
static inline void shorts_disable(void)
{
    nrf_radio_shorts_set(SHORTS_IDLE);
    nrf_radio_ifs_set(0);
}

/// Enable peripheral shorts used during data frame transmission.
static inline void shorts_tx_frame_set(void)
{
    nrf_radio_shorts_set(SHORTS_TX_FRAME);
}

/// Enable peripheral shorts used in receive state to enable automatic ACK procedure.
static inline void shorts_rx_initial_set(void)
{
    nrf_radio_ifs_set(TIFS_ACK_US);
    nrf_radio_bcc_set(BCC_INIT);

    nrf_radio_shorts_set(SHORTS_RX_INITIAL);
}

/// Enable peripheral shorts used during automatic ACK transmission.
static inline void shorts_rx_following_set(void)
{
    nrf_radio_shorts_set(SHORTS_RX_FOLLOWING);
}

/// Disable peripheral shorts used during automatic ACK transmission when ACK is being transmitted
static inline void shorts_tx_ack_set(void)
{
    // If ACK is sent END_DISABLE short should persist to disable transmitter automatically.
    nrf_radio_shorts_set(SHORTS_TX_ACK);
    nrf_radio_ifs_set(0);
}


/***************************************************************************************************
 * @section ACK transmission management
 **************************************************************************************************/

/// Set valid sequence number in ACK frame.
static inline void ack_prepare(void)
{
    // Copy sequence number from received frame to ACK frame.
    m_ack_psdu[DSN_OFFSET] = mp_current_rx_buffer->psdu[DSN_OFFSET];
}

/// Set pending bit in ACK frame.
static inline void ack_pending_bit_set(void)
{
    m_ack_psdu[FRAME_PENDING_OFFSET] = ACK_HEADER_WITH_PENDING;

    if (!nrf_drv_radio802154_ack_pending_bit_should_be_set(mp_current_rx_buffer->psdu))
    {
        m_ack_psdu[FRAME_PENDING_OFFSET] = ACK_HEADER_WITHOUT_PENDING;
    }
}

/** Check if ACK is requested in given frame.
 *
 * @param[in]  p_frame  Pointer to a frame to check.
 *
 * @retval  true   ACK is requested in given frame.
 * @retval  false  ACK is not requested in given frame.
 */
static inline bool ack_is_requested(const uint8_t * p_frame)
{
    return (p_frame[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT) ? true : false;
}

/** Abort automatic ACK procedure.
 *
 * @param[in]  state_to_set  Driver state that shall be set after the procedure is aborted.
 */
static void auto_ack_abort(radio_state_t state_to_set)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_AUTO_ACK_ABORT);

    shorts_disable();

    switch (nrf_radio_state_get())
    {
        case NRF_RADIO_STATE_RX:     // When stopping before whole frame received.
        case NRF_RADIO_STATE_RX_RU:  // When transmission is initialized during receiver ramp up.
        case NRF_RADIO_STATE_RX_IDLE:
        case NRF_RADIO_STATE_TX_RU:
        case NRF_RADIO_STATE_TX_IDLE:
        case NRF_RADIO_STATE_TX:
            nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED); // Clear disabled event that was set by short.
            state_set(state_to_set);
            nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
            break;

        case NRF_RADIO_STATE_RX_DISABLE:
        case NRF_RADIO_STATE_DISABLED:
        case NRF_RADIO_STATE_TX_DISABLE:
            // Do not trigger DISABLE task in those states to prevent double DISABLED events.
            state_set(state_to_set);
            break;

        default:
            assert(false);
    }

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_AUTO_ACK_ABORT);
}

/***************************************************************************************************
 * @section ACK receiving management
 **************************************************************************************************/

/// Enable hardware ACK matching accelerator.
static inline void ack_matching_enable(void)
{
    nrf_radio_event_clear(NRF_RADIO_EVENT_MHRMATCH);
    nrf_radio_mhmu_search_pattern_set(MHMU_PATTERN |
                                      ((uint32_t) mp_tx_data[DSN_OFFSET] <<
                                       MHMU_PATTERN_DSN_OFFSET));
}

/// Disable hardware ACK matching accelerator.
static inline void ack_matching_disable(void)
{
    nrf_radio_mhmu_search_pattern_set(0);
    nrf_radio_event_clear(NRF_RADIO_EVENT_MHRMATCH);
}

/** Check if hardware ACK matching accelerator matched ACK pattern in received frame.
 *
 * @retval  true   ACK matching accelerator matched ACK pattern.
 * @retval  false  ACK matching accelerator did not match ACK pattern.
 */
static inline bool ack_is_matched(void)
{
    return (nrf_radio_event_get(NRF_RADIO_EVENT_MHRMATCH)) &&
            (nrf_radio_crc_status_get() == NRF_RADIO_CRC_STATUS_OK);
}

/// Start receiver to receive data after receiving of ACK frame.
static inline void frame_rx_start_after_ack_rx(void)
{
    ack_matching_disable();
    state_set(RADIO_STATE_WAITING_RX_FRAME);
    nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE); // Errata [110]
}

/***************************************************************************************************
 * @section RADIO peripheral management
 **************************************************************************************************/

/// Initialize radio peripheral
static void nrf_radio_init(void)
{
    nrf_radio_mode_set(NRF_RADIO_MODE_IEEE802154_250KBIT);
    nrf_radio_config_length_field_length_set(8);
    nrf_radio_config_preamble_length_set(NRF_RADIO_PREAMBLE_LENGTH_32BIT_ZERO);
    nrf_radio_config_crc_included_set(true);
    nrf_radio_config_max_length_set(MAX_PACKET_SIZE);
    nrf_radio_ramp_up_mode_set(NRF_RADIO_RAMP_UP_MODE_DEFAULT);

    // Configure CRC
    nrf_radio_crc_length_set(CRC_LENGTH);
    nrf_radio_crc_includes_address_set(NRF_RADIO_CRC_INCLUDES_ADDR_IEEE802154);
    nrf_radio_crc_polynominal_set(CRC_POLYNOMIAL);

    // Configure CCA
    cca_configuration_update();

    // Configure MAC Header Match Unit
    nrf_radio_mhmu_search_pattern_set(0);
    nrf_radio_mhmu_pattern_mask_set(MHMU_MASK);

    nrf_radio_int_enable(NRF_RADIO_INT_FRAMESTART_MASK);
    nrf_radio_int_enable(NRF_RADIO_INT_END_MASK);
    nrf_radio_int_enable(NRF_RADIO_INT_DISABLED_MASK);
    nrf_radio_int_enable(NRF_RADIO_INT_CCAIDLE_MASK);
    nrf_radio_int_enable(NRF_RADIO_INT_CCABUSY_MASK);
    nrf_radio_int_enable(NRF_RADIO_INT_READY_MASK);
    nrf_radio_int_enable(NRF_RADIO_INT_BCMATCH_MASK);
    nrf_radio_int_enable(NRF_RADIO_INT_EDEND_MASK);
}

/// Reset radio peripheral
static void nrf_radio_reset(void)
{
    nrf_radio_power_set(false);
    nrf_radio_power_set(true);

    nrf_drv_radio802154_log(EVENT_RADIO_RESET, 0);
}

/// Initialize interrupts for radio peripheral
static void irq_init(void)
{
    NVIC_SetPriority(RADIO_IRQn, RADIO_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}

/// Deinitialize interrupts for radio peripheral
static void irq_deinit(void)
{
    NVIC_DisableIRQ(RADIO_IRQn);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_SetPriority(RADIO_IRQn, 0);

    __DSB();
    __ISB();
}


/***************************************************************************************************
 * @section Energy detection management
 **************************************************************************************************/

/** Setup next iteration of energy detection procedure.
 *
 *  Energy detection procedure is performed in iterations to make sure it is performed for requested
 *  time regardless radio arbitration.
 *
 *  @param[in]  Remaining time of energy detection procedure [us].
 *
 *  @retval  true   Next iteration of energy detection procedure will be performed now.
 *  @retval  false  Next iteration of energy detection procedure will not be performed now due to
 *                  ending timeslot.
 */
static inline bool ed_iter_setup(uint32_t time_us)
{
    uint32_t us_left_in_timeslot = nrf_raal_timeslot_us_left_get();
    uint32_t next_ed_iters       = us_left_in_timeslot / ED_ITER_DURATION;

    if (next_ed_iters > ED_ITERS_OVERHEAD)
    {
        next_ed_iters -= ED_ITERS_OVERHEAD;

        if ((time_us / ED_ITER_DURATION) < next_ed_iters)
        {
            m_ed_time_left = 0;
            next_ed_iters  = time_us / ED_ITER_DURATION;
        }
        else
        {
            m_ed_time_left = time_us - (next_ed_iters * ED_ITER_DURATION);
            next_ed_iters--; // Time of ED procedure is (next_ed_iters + 1) * 128us
        }

        nrf_radio_ed_loop_count_set(next_ed_iters);

        return true;
    }
    else
    {
        irq_deinit();
        nrf_radio_reset();

        m_ed_time_left = time_us;

        return false;
    }
}


/***************************************************************************************************
 * @section RAAL notification handlers
 **************************************************************************************************/

void nrf_raal_timeslot_started(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_TIMESLOT_STARTED);

    nrf_radio_reset();
    nrf_radio_init();
    irq_init();

    assert(nrf_radio_shorts_get() == SHORTS_IDLE);

    channel_set(nrf_drv_radio802154_pib_channel_get());

    switch (m_state)
    {
        case RADIO_STATE_WAITING_TIMESLOT:
            state_set(RADIO_STATE_WAITING_RX_FRAME);
            nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
            break;

        case RADIO_STATE_ED:
            if (ed_iter_setup(m_ed_time_left))
            {
                nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
            }

            break;

        case RADIO_STATE_CCA:
        case RADIO_STATE_CONTINUOUS_CARRIER:
            nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
            break;

        case RADIO_STATE_SLEEP:
            // This case may happen when sleep is requested by the next higher layer right before
            // timeslot starts and the driver uses SWI for requests and notifications. In this case
            // RAAL may report timeslot start event when exiting sleep request critical section.
            // The driver is already in SLEEP state but did not request timeslot end yet - it will
            // be requested in the next SWI handler.
            break;

        default:
            assert(false);
    }

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_TIMESLOT_STARTED);
}

void nrf_raal_timeslot_ended(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_TIMESLOT_ENDED);

    irq_deinit();
    nrf_radio_reset();

    switch (m_state)
    {
        case RADIO_STATE_CCA_BEFORE_TX:
        case RADIO_STATE_TX_FRAME:
        case RADIO_STATE_RX_ACK:
            nrf_drv_radio802154_notify_busy_channel();
            break;

        default:
            break;
    }

    switch (m_state)
    {
        case RADIO_STATE_DISABLING:
            sleep_start();
            break;

        case RADIO_STATE_SLEEP:
        case RADIO_STATE_WAITING_TIMESLOT:
        case RADIO_STATE_ED:
        case RADIO_STATE_CCA:
        case RADIO_STATE_CONTINUOUS_CARRIER:
            // Intentionally empty.
            break;

        case RADIO_STATE_WAITING_RX_FRAME:
        case RADIO_STATE_RX_HEADER:
        case RADIO_STATE_RX_FRAME:
        case RADIO_STATE_TX_ACK:
        case RADIO_STATE_CCA_BEFORE_TX:
        case RADIO_STATE_TX_FRAME:
        case RADIO_STATE_RX_ACK:
            mutex_lock();
            state_set(RADIO_STATE_WAITING_TIMESLOT);
            break;
    }

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_TIMESLOT_ENDED);
}


/***************************************************************************************************
 * @section RADIO interrupt handler
 **************************************************************************************************/

/// This event is handled when the radio starts receiving a frame.
static inline void irq_framestart_state_waiting_rx_frame(void)
{
    if (mutex_lock())
    {
        state_set(RADIO_STATE_RX_HEADER);
        assert(nrf_radio_shorts_get() == SHORTS_RX_INITIAL);

        if ((mp_current_rx_buffer->psdu[0] < ACK_LENGTH) ||
            (mp_current_rx_buffer->psdu[0] > MAX_PACKET_SIZE))
        {
            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
            nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
        }
        else
        {
            nrf_radio_task_trigger(NRF_RADIO_TASK_RSSISTART);

            nrf_drv_radio802154_rx_started();
        }
    }

    switch (nrf_radio_state_get())
    {
        case NRF_RADIO_STATE_RX:

        // If the received frame was short the radio could have changed it's state.
        case NRF_RADIO_STATE_RX_IDLE:

        // The radio could have changed state to one of the following due to enabled shorts.
        case NRF_RADIO_STATE_RX_DISABLE:
        case NRF_RADIO_STATE_DISABLED:
        case NRF_RADIO_STATE_TX_RU:
            break;

        // If something had stopped the CPU too long. Try to recover radio state.
        case NRF_RADIO_STATE_TX_IDLE:
        case NRF_RADIO_STATE_TX_DISABLE:
            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
            nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
            break;

        default:
            assert(false);
    }
}

/// This event is handled when the radio starts receiving an ACK frame.
static inline void irq_framestart_state_rx_ack(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);

    if ((mp_current_rx_buffer->psdu[0] < ACK_LENGTH) ||
        (mp_current_rx_buffer->psdu[0] > MAX_PACKET_SIZE))
    {
        nrf_drv_radio802154_notify_busy_channel();

        frame_rx_start_after_ack_rx();
        nrf_radio_event_clear(NRF_RADIO_EVENT_END); // In case frame ended before task DISABLE
    }
    else
    {
        nrf_radio_task_trigger(NRF_RADIO_TASK_RSSISTART);
    }
}

/// This event is handled when the radio starts transmitting a requested frame.
static inline void irq_framestart_state_tx_frame(void)
{
    nrf_drv_radio802154_tx_started();
}

/// This event is handled when MHR is received
static inline void irq_bcmatch_mhr(void)
{
    // Verify if time slot for receiving is available.
    if (nrf_raal_timeslot_request(nrf_drv_radio802154_rx_duration_get(
            mp_current_rx_buffer->psdu[0],
            ack_is_requested(mp_current_rx_buffer->psdu))))
    {
        // Check Frame Control field.
        switch (mp_current_rx_buffer->psdu[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK)
        {
            case FRAME_TYPE_BEACON:
                // Beacon is broadcast frame.
                m_flags.prevent_ack = false;
                state_set(RADIO_STATE_RX_FRAME);
                break;

            case FRAME_TYPE_DATA:
            case FRAME_TYPE_COMMAND:

                // For data or command check destination address.
                switch (mp_current_rx_buffer->psdu[DEST_ADDR_TYPE_OFFSET] &
                        DEST_ADDR_TYPE_MASK)
                {
                    case DEST_ADDR_TYPE_SHORT:
                        nrf_radio_bcc_set(BCC_SHORT_ADDR);
                        break;

                    case DEST_ADDR_TYPE_EXTENDED:
                        nrf_radio_bcc_set(BCC_EXTENDED_ADDR);
                        break;

                    default:
                        auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                        nrf_radio_event_clear(NRF_RADIO_EVENT_END);
                        nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
                }

                break;

            default:

                // For ACK and other types: in promiscuous mode accept it as broadcast;
                // in normal mode drop the frame.
                if (nrf_drv_radio802154_pib_promiscuous_get())
                {
                    m_flags.prevent_ack = true;
                    state_set(RADIO_STATE_RX_FRAME);
                }
                else
                {
                    auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_END);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
                }
        }
    }
    else
    {
        irq_deinit();
        nrf_radio_reset();

        state_set(RADIO_STATE_WAITING_TIMESLOT);
    }
}

/// This event is generated when destination address fields are received.
static inline void irq_bcmatch_address(void)
{
    if (nrf_drv_radio802154_pib_dest_addr_matches(mp_current_rx_buffer->psdu))
    {
        m_flags.prevent_ack = false;
        state_set(RADIO_STATE_RX_FRAME);
    }
    else
    {
        if (nrf_drv_radio802154_pib_promiscuous_get())
        {
            m_flags.prevent_ack = true;
            state_set(RADIO_STATE_RX_FRAME);
        }
        else
        {
            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
        }
    }
}

/** This event is generated twice during frame reception:
 *  when MHR is received and when destination address fields are received.
 */
static inline void irq_bcmatch_state_rx_header(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_RX_INITIAL);

    switch (nrf_radio_state_get())
    {
        case NRF_RADIO_STATE_RX:
        case NRF_RADIO_STATE_RX_IDLE:
        case NRF_RADIO_STATE_RX_DISABLE:
        case NRF_RADIO_STATE_DISABLED:
        case NRF_RADIO_STATE_TX_RU:      // A lot of states due to shorts.

            switch (nrf_radio_bcc_get())
            {
                case BCC_INIT: // Received MHR
                    irq_bcmatch_mhr();
                    break;

                case BCC_SHORT_ADDR:     // Received short destination address
                case BCC_EXTENDED_ADDR:  // Received extended destination address
                    // Check destination address during second match.
                    irq_bcmatch_address();
                    break;

                default:
                    assert(false);
            }

            break;

        case NRF_RADIO_STATE_TX_IDLE:
            // Something had stopped the CPU too long. Start receiving again.
            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
            break;

        default:
            assert(false);
    }
}

/** This event is generated when radio receives invalid frame with length set to 0 or started
 *  receiving a frame after other procedure locked mutex.
 */
static inline void irq_end_state_waiting_rx_frame(void)
{
    // Radio state is not asserted here. It can be a lot of states due to shorts.
    if (mp_current_rx_buffer->psdu[0] == 0)
    {
        // If length of the frame is 0 there was no FRAMESTART event. Lock mutex now and abort sending ACK.
        if (mutex_lock())
        {
            assert(nrf_radio_shorts_get() == SHORTS_RX_INITIAL);
            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
        }
    }
    else
    {
        // Do nothing. Whoever took mutex shall stop sending ACK.
    }
}

/// This event is generated if frame being received is shorter than expected MAC header length.
static inline void irq_end_state_rx_header(void)
{
    // Frame ended before header was received.
    auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
}

/// This event is generated when radio peripheral ends receiving of a complete frame.
static inline void irq_end_state_rx_frame(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_RX_INITIAL);

    switch (nrf_radio_state_get())
    {
        case NRF_RADIO_STATE_RX_IDLE:
        case NRF_RADIO_STATE_RX_DISABLE:
        case NRF_RADIO_STATE_DISABLED:
        case NRF_RADIO_STATE_TX_RU:

            if (nrf_radio_crc_status_get() == NRF_RADIO_CRC_STATUS_OK)
            {
                if ((!ack_is_requested(mp_current_rx_buffer->psdu)) ||
                    (!nrf_drv_radio802154_pib_auto_ack_get()) ||
                    m_flags.prevent_ack)
                {
                    auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
                    received_frame_notify();
                }
                else
                {
                    ack_prepare();
                    state_set(RADIO_STATE_TX_ACK);
                }
            }
            else
            {
                auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
            }

            break;

        case NRF_RADIO_STATE_TX_IDLE:
            // CPU was hold too long.
            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
            break;

        default:
            assert(false);
    }
}

/// This event is generated when the radio ends transmission of ACK frame.
static inline void irq_end_state_tx_ack(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_TX_ACK ||
           nrf_radio_shorts_get() == SHORTS_RX_FOLLOWING); // In case END is handled before READY
    shorts_disable();

    received_frame_notify();

    // Clear event READY in case CPU was halted and event END is handled before READY.
    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

    state_set(RADIO_STATE_WAITING_RX_FRAME);
    // Receiver is enabled by shorts.
}

/** This event may occur at the beginning of transmission procedure (the procedure already has
 *  disabled shorts).
 */
static inline void irq_end_state_cca_before_tx(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
}

/// This event is generated when the radio ends transmission of a frame.
static inline void irq_end_state_tx_frame(void)
{
    shorts_disable();
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);

    // Clear event READY in case CPU was halted and event END is handled before READY.
    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

    if (!ack_is_requested(mp_tx_data))
    {
        nrf_drv_radio802154_notify_transmitted(NULL, 0, 0);

        state_set(RADIO_STATE_WAITING_RX_FRAME);
    }
    else
    {
        state_set(RADIO_STATE_RX_ACK);

        ack_matching_enable();
    }

    // Task DISABLE is triggered by shorts.
}

/// This event is generated when the radio ends receiving of ACK frame.
static inline void irq_end_state_rx_ack(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);

    if (ack_is_matched())
    {
        mp_current_rx_buffer->free = false;
        nrf_drv_radio802154_notify_transmitted(mp_current_rx_buffer->psdu,                // psdu
                                               rssi_last_measurement_get(),               // rssi
                                               RX_FRAME_LQI(mp_current_rx_buffer->psdu)); // lqi
    }
    else
    {
        nrf_drv_radio802154_notify_busy_channel();
    }

    frame_rx_start_after_ack_rx();
}

/// This event is generated when radio peripheral disables in order to enter sleep state.
static inline void irq_disabled_state_disabling(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
    sleep_start();
}

/** This event is generated when the driver enters receive state.
 *
 *  The radio is now disabled and the driver starts enabling receiver.
 */
static inline void irq_disabled_state_waiting_rx_frame(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);

    while (nrf_radio_state_get() == NRF_RADIO_STATE_TX_DISABLE)
    {
        // This event can be handled in TXDISABLE state due to double DISABLE event (IC-15879).
        // This busy loop waits to the end of this state.
    }

    assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);

    nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
    mutex_unlock();

    rx_buffer_in_use_set(nrf_drv_radio802154_rx_buffer_free_find());
    nrf_radio_tx_power_set(nrf_drv_radio802154_pib_tx_power_get());

    // Clear this event after RXEN task in case event is triggered just before.
    nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);
}

/** This event is generated in case both END and DISABLED events are notified between
 *  checking if END event was notified and checking if DISABLED event was notified.
 *
 *  In this invalid and rare case just drop received frame.
 */
static inline void irq_disabled_state_rx_frame(void)
{
    assert(nrf_radio_event_get(NRF_RADIO_EVENT_END));
    assert(nrf_radio_shorts_get() == SHORTS_RX_INITIAL);

    auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
    nrf_radio_event_clear(NRF_RADIO_EVENT_END);
    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
}

/** This event is generated during automatic ACK transmission.
 *
 *  Receiver is disabled and transmitter is being enabled by shorts.
 */
static inline void irq_disabled_state_tx_ack(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_RX_INITIAL);

    shorts_rx_following_set();
    ack_pending_bit_set();

    // IC-15879
    nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);

    if (nrf_radio_state_get() == NRF_RADIO_STATE_TX_IDLE)
    {
        // CPU was hold too long.
        auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
        nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
    }
}

/** This event is generated before CCA procedure starts.
 *
 *  The radio is disabled and the drivers enables receiver in order to start CCA procedure.
 */
static inline void irq_disabled_state_cca_before_tx(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
    nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
}

/** This event is generated before transmission of a frame starts.
 *
 *  The radio is disabled and enabling transmitter is requested by shorts or by the driver.
 */
static inline void irq_disabled_state_tx_frame(void)
{
    if (nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED)
    {
        shorts_tx_frame_set();
        nrf_radio_task_trigger(NRF_RADIO_TASK_TXEN);
    }

    assert(nrf_radio_shorts_get() == SHORTS_TX_FRAME);
}

/** This event is generated when radio is disabled after transmission of a frame with ACK request.
 *
 *  The driver enables receiver to receive ACK frame.
 */
static inline void irq_disabled_state_rx_ack(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
    nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);

    if (mp_current_rx_buffer == NULL || (!mp_current_rx_buffer->free))
    {
        rx_buffer_in_use_set(nrf_drv_radio802154_rx_buffer_free_find());
    }
}

/** This event is generated before energy detection procedure.
 *
 *  The radio is disabled and the driver enables receiver in order to start energy detection
 *  procedure.
 */
static inline void irq_disabled_state_ed(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
    nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
}

/** This event is generated before stand-alone CCA procedure.
 *
 *  The radio is disabled and the driver enables receiver in order to start CCA procedure.
 */
static inline void irq_disabled_state_cca(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
    nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
}

/** This event is generated before continuous-carrier procedure is started.
 *
 *  The radio is disabled and the driver enables transmitter in order to start emitting carrier
 *  continuously.
 */
static inline void irq_disabled_state_continuous_carrier(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
    nrf_radio_task_trigger(NRF_RADIO_TASK_TXEN);
}

/** This event is generated when receiver is ready to start receiving a frame.
 *
 *  Driver checks if buffer for a frame is available and starts receiver.
 */
static inline void irq_ready_state_waiting_rx_frame(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);

    if ((mp_current_rx_buffer != NULL) && (mp_current_rx_buffer->free))
    {
        if (mutex_lock())
        {
            shorts_rx_initial_set();
            rx_frame_start();

            mutex_unlock();
        }
    }
}

/** This event is generated when transmitter is ready to transmit ACK frame.
 *
 *  Transmission is started by shorts. The driver sets shorts to gracefully end transmission of ACK.
 */
static inline void irq_ready_state_tx_ack(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_RX_FOLLOWING);
    shorts_tx_ack_set();
}

/** This event is generated when receiver is ready to start CCA procedure.
 *
 *  The driver prepares for transmission and starts CCA procedure.
 */
static inline void irq_ready_state_cca_before_tx(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);

    state_set(RADIO_STATE_TX_FRAME);

    shorts_tx_frame_set();
    nrf_radio_task_trigger(NRF_RADIO_TASK_CCASTART);
}

/** This event is generated when transmitter is ready to transmit a frame.
 *
 *  Transmission is started by shorts.
 */
static inline void irq_ready_state_tx_frame(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_TX_FRAME);
}

/// This event is generated when receiver is ready to receive an ACK frame.
static inline void irq_ready_state_rx_ack(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);

    if (mp_current_rx_buffer == NULL || (!mp_current_rx_buffer->free))
    {
        nrf_drv_radio802154_notify_busy_channel();
        frame_rx_start_after_ack_rx();
    }
    else
    {
        rx_start();
    }
}

/// This event is generated when receiver is ready to start energy detection procedure.
static inline void irq_ready_state_ed(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);
    nrf_radio_task_trigger(NRF_RADIO_TASK_EDSTART);
}

/// This event is generated when receiver is ready to start CCA procedure.
static inline void irq_ready_state_cca(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);
    nrf_radio_task_trigger(NRF_RADIO_TASK_CCASTART);
}

/// This event is generated when transmitter is emitting carrier continuously.
static inline void irq_ready_state_continuous_carrier(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_TX_IDLE);
}

#if !RADIO_SHORT_CCAIDLE_TXEN
/// This event is generated when CCA reports that channel is idle.
static inline void irq_ccaidle_state_tx_frame(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_TX_FRAME);

    nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
}
#endif // RADIO_SHORT_CCAIDLE_TXEN

/// This event is generated when CCA reports idle channel during stand-alone procedure.
static inline void irq_ccaidle_state_cca(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);

    nrf_drv_radio802154_notify_cca(true);

    state_set(RADIO_STATE_WAITING_RX_FRAME);
    nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
}

/// This event is generated when CCA reports busy channel prior to transmission.
static inline void irq_ccabusy_state_tx_frame(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_TX_FRAME);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);

    shorts_disable();

    nrf_drv_radio802154_notify_busy_channel();

    state_set(RADIO_STATE_WAITING_RX_FRAME);
    nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
}

/// This event is generated when CCA reports busy channel during stand-alone procedure.
static inline void irq_ccabusy_state_cca(void)
{
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);

    nrf_drv_radio802154_notify_cca(false);

    state_set(RADIO_STATE_WAITING_RX_FRAME);
    nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
}

/// This event is generated when energy detection procedure ends.
static inline void irq_edend(void)
{
    assert(m_state == RADIO_STATE_ED);
    assert(nrf_radio_state_get() == NRF_RADIO_STATE_RX_IDLE);
    assert(nrf_radio_shorts_get() == SHORTS_IDLE);

    uint32_t result = nrf_radio_ed_sample_get();
    m_ed_result     = result > m_ed_result ? result : m_ed_result;

    if (m_ed_time_left)
    {
        if (ed_iter_setup(m_ed_time_left))
        {
            nrf_radio_task_trigger(NRF_RADIO_TASK_EDSTART);
        }
    }
    else
    {
        nrf_drv_radio802154_notify_energy_detected(m_ed_result);

        // In case channel change was requested during energy detection procedure.
        channel_set(nrf_drv_radio802154_pib_channel_get());

        state_set(RADIO_STATE_WAITING_RX_FRAME);
        nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
    }
}

/// Handler of radio interrupts.
static inline void irq_handler(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_IRQ_HANDLER);

    if (nrf_radio_event_get(NRF_RADIO_EVENT_FRAMESTART))
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_EVENT_FRAMESTART);
        nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);

        switch (m_state)
        {
            case RADIO_STATE_WAITING_RX_FRAME:
                irq_framestart_state_waiting_rx_frame();
                break;

            case RADIO_STATE_RX_ACK:
                irq_framestart_state_rx_ack();
                break;

            case RADIO_STATE_TX_FRAME:
                irq_framestart_state_tx_frame();
                break;

            case RADIO_STATE_TX_ACK:
            case RADIO_STATE_CCA_BEFORE_TX: // This could happen at the beginning of transmission procedure.
            case RADIO_STATE_WAITING_TIMESLOT:
                break;

            default:
                assert(false);
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_EVENT_FRAMESTART);
    }

    // Check MAC frame header.
    if (nrf_radio_event_get(NRF_RADIO_EVENT_BCMATCH))
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_EVENT_BCMATCH);
        nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);

        switch (m_state)
        {
            case RADIO_STATE_RX_HEADER:
                irq_bcmatch_state_rx_header();
                break;

            case RADIO_STATE_WAITING_RX_FRAME: // Didn't lock mutex - other procedure manages radio.
            case RADIO_STATE_WAITING_TIMESLOT: // Radio is out of timeslot now.
                assert(m_mutex);
                break;

            default:
                assert(false);
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_EVENT_BCMATCH);
    }

    if (nrf_radio_event_get(NRF_RADIO_EVENT_END))
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_EVENT_END);
        nrf_radio_event_clear(NRF_RADIO_EVENT_END);

        switch (m_state)
        {
            case RADIO_STATE_WAITING_RX_FRAME:
                irq_end_state_waiting_rx_frame();
                break;

            case RADIO_STATE_RX_HEADER:
                irq_end_state_rx_header();
                break;

            case RADIO_STATE_RX_FRAME:
                irq_end_state_rx_frame();
                break;

            case RADIO_STATE_TX_ACK:
                irq_end_state_tx_ack();
                break;

            case RADIO_STATE_CCA_BEFORE_TX: // This could happen at the beginning of transmission
                                            // procedure (the procedure already has disabled shorts)
                irq_end_state_cca_before_tx();
                break;

            case RADIO_STATE_TX_FRAME:
                irq_end_state_tx_frame();
                break;

            case RADIO_STATE_RX_ACK: // Ended receiving of ACK.
                irq_end_state_rx_ack();
                break;

            case RADIO_STATE_WAITING_TIMESLOT:
                // Exit as soon as possible when waiting for timeslot.
                break;

            default:
                assert(false);
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_EVENT_END);
    }

    if (nrf_radio_event_get(NRF_RADIO_EVENT_DISABLED))
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_EVENT_DISABLED);
        nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);

        switch (m_state)
        {
            case RADIO_STATE_DISABLING:
                irq_disabled_state_disabling();
                break;

            case RADIO_STATE_WAITING_RX_FRAME:
                irq_disabled_state_waiting_rx_frame();
                break;

            case RADIO_STATE_RX_HEADER:
            case RADIO_STATE_RX_FRAME:
                irq_disabled_state_rx_frame();
                break;

            case RADIO_STATE_TX_ACK:
                irq_disabled_state_tx_ack();
                break;

            case RADIO_STATE_CCA_BEFORE_TX:
                irq_disabled_state_cca_before_tx();
                break;

            case RADIO_STATE_TX_FRAME:
                irq_disabled_state_tx_frame();
                break;

            case RADIO_STATE_RX_ACK:
                irq_disabled_state_rx_ack();
                break;

            case RADIO_STATE_ED:
                irq_disabled_state_ed();
                break;

            case RADIO_STATE_CCA:
                irq_disabled_state_cca();
                break;

            case RADIO_STATE_CONTINUOUS_CARRIER:
                irq_disabled_state_continuous_carrier();
                break;

            case RADIO_STATE_WAITING_TIMESLOT:
                // Exit as soon as possible when waiting for timeslot.
                break;

            default:
                assert(false);
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_EVENT_DISABLED);
    }

    if (nrf_radio_event_get(NRF_RADIO_EVENT_READY))
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_EVENT_READY);
        nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

        switch (m_state)
        {
            case RADIO_STATE_WAITING_RX_FRAME:
                irq_ready_state_waiting_rx_frame();
                break;

            case RADIO_STATE_TX_ACK:
                irq_ready_state_tx_ack();
                break;

            case RADIO_STATE_CCA_BEFORE_TX:
                irq_ready_state_cca_before_tx();
                break;

            case RADIO_STATE_TX_FRAME:
                irq_ready_state_tx_frame();
                break;

            case RADIO_STATE_RX_ACK:
                irq_ready_state_rx_ack();
                break;

            case RADIO_STATE_ED:
                irq_ready_state_ed();
                break;

            case RADIO_STATE_CCA:
                irq_ready_state_cca();
                break;

            case RADIO_STATE_CONTINUOUS_CARRIER:
                irq_ready_state_continuous_carrier();
                break;

            case RADIO_STATE_WAITING_TIMESLOT:
                // Exit as soon as possible when waiting for timeslot.
                break;

            default:
                assert(false);
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_EVENT_READY);
    }

    if (nrf_radio_event_get(NRF_RADIO_EVENT_CCAIDLE))
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_EVENT_CCAIDLE);
        nrf_radio_event_clear(NRF_RADIO_EVENT_CCAIDLE);

        switch (m_state)
        {
            case RADIO_STATE_TX_FRAME:
#if !RADIO_SHORT_CCAIDLE_TXEN
                irq_ccaidle_state_tx_frame();
#endif
                break;

            case RADIO_STATE_CCA:
                irq_ccaidle_state_cca();
                break;

            case RADIO_STATE_WAITING_TIMESLOT:
                // Exit as soon as possible when waiting for timeslot.
                break;

            default:
                assert(false);
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_EVENT_CCAIDLE);
    }

    if (nrf_radio_event_get(NRF_RADIO_EVENT_CCABUSY))
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_EVENT_CCABUSY);
        nrf_radio_event_clear(NRF_RADIO_EVENT_CCABUSY);

        switch (m_state)
        {
            case RADIO_STATE_TX_FRAME:
                irq_ccabusy_state_tx_frame();
                break;

            case RADIO_STATE_CCA:
                irq_ccabusy_state_cca();
                break;

            case RADIO_STATE_WAITING_TIMESLOT:
                // Exit as soon as possible when waiting for timeslot.
                break;

            default:
                assert(false);
        }

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_EVENT_CCABUSY);
    }

    if (nrf_radio_event_get(NRF_RADIO_EVENT_EDEND))
    {
        nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_EVENT_EDEND);
        nrf_radio_event_clear(NRF_RADIO_EVENT_EDEND);

        irq_edend();

        nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_EVENT_EDEND);
    }

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_IRQ_HANDLER);
}


/***************************************************************************************************
 * @section FSM transition request sub-procedures
 **************************************************************************************************/

/** Abort transmission procedure.
 *
 *  This function is called when MAC layer requests transition from transmit to receive state.
 */
static inline void tx_procedure_abort(radio_state_t state)
{
    shorts_disable();

    assert(nrf_radio_shorts_get() == SHORTS_IDLE);
    assert(m_mutex);

    state_set(state);

    switch (nrf_radio_state_get())
    {
        case NRF_RADIO_STATE_TX_DISABLE:
        case NRF_RADIO_STATE_RX_DISABLE:
            // Do not enabled receiver. It will be enabled in DISABLED handler.
            break;

        default:
            nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);
            nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
    }

    ack_matching_disable();

    // Clear events that could have happened in critical section due to receiving frame.
    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
    nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
    nrf_radio_event_clear(NRF_RADIO_EVENT_END);
}


/***************************************************************************************************
 * @section API functions
 **************************************************************************************************/

void nrf_drv_radio802154_fsm_init(void)
{
    const uint8_t ack_psdu[] = {0x05, ACK_HEADER_WITH_PENDING, 0x00, 0x00, 0x00, 0x00};
    memcpy(m_ack_psdu, ack_psdu, sizeof(ack_psdu));

}

void nrf_drv_radio802154_fsm_deinit(void)
{
    if (m_state != RADIO_STATE_WAITING_TIMESLOT &&
        m_state != RADIO_STATE_SLEEP)
    {
        nrf_radio_reset();
    }

    irq_deinit();
}

radio_state_t nrf_drv_radio802154_fsm_state_get(void)
{
    return m_state;
}

bool nrf_drv_radio802154_fsm_sleep(void)
{
    bool result = false;

    switch (m_state)
    {
        case RADIO_STATE_WAITING_TIMESLOT:
            assert(m_mutex);

            sleep_start();
            result = true;
            break;

        case RADIO_STATE_WAITING_RX_FRAME:
            if (mutex_lock())
            {
                auto_ack_abort(RADIO_STATE_DISABLING);

                assert(nrf_radio_shorts_get() == SHORTS_IDLE);

                // Clear events that could have happened in critical section due to receiving frame
                // or RX ramp up.
                nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
                nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
                nrf_radio_event_clear(NRF_RADIO_EVENT_END);
                nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

                result = true;
            }

            break;

        case RADIO_STATE_RX_HEADER:
        case RADIO_STATE_RX_FRAME:
        case RADIO_STATE_TX_ACK:
            break;

        case RADIO_STATE_CCA_BEFORE_TX:
        case RADIO_STATE_TX_FRAME:
        case RADIO_STATE_RX_ACK:
            tx_procedure_abort(RADIO_STATE_DISABLING);
            result = true;
            break;

        default:
            assert(false); // This should not happen.
    }

    return result;
}

bool nrf_drv_radio802154_fsm_receive(void)
{
    bool result = false;

    switch (m_state)
    {
        case RADIO_STATE_WAITING_RX_FRAME:
        case RADIO_STATE_RX_HEADER:
        case RADIO_STATE_RX_FRAME:
        case RADIO_STATE_TX_ACK:
            result = true;
            break;

        case RADIO_STATE_DISABLING:
            state_set(RADIO_STATE_WAITING_RX_FRAME);
            result = true;
            // TASK DISABLE was already triggered. Wait for event DISABLED.
            break;

        case RADIO_STATE_SLEEP:
            assert(mutex_lock());
            state_set(RADIO_STATE_WAITING_TIMESLOT);
            nrf_raal_continuous_mode_enter();

            result = true;
            break;

        case RADIO_STATE_CCA_BEFORE_TX:
        case RADIO_STATE_TX_FRAME:
        case RADIO_STATE_RX_ACK:
            tx_procedure_abort(RADIO_STATE_WAITING_RX_FRAME);
            result = true;
            break;

        case RADIO_STATE_CONTINUOUS_CARRIER:
            state_set(RADIO_STATE_WAITING_RX_FRAME);
            nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
            result = true;
            break;

        case RADIO_STATE_ED:
        case RADIO_STATE_CCA:
            // Ignore receive function during energy detection or CCA procedure.
            break;

        case RADIO_STATE_WAITING_TIMESLOT:
            // Ignore receive function in WAITING_TIMESLOT state - the radio will receive when time
            // slot starts.
            result = true;
            break;

        default:
            assert(false);
    }

    return result;
}

bool nrf_drv_radio802154_fsm_transmit(const uint8_t * p_data, bool cca)
{
    bool result = false;
    mp_tx_data  = p_data;

    if (mutex_lock())
    {
        if (nrf_raal_timeslot_request(nrf_drv_radio802154_tx_duration_get(p_data[0],
                                                                          ack_is_requested(p_data))))
        {
            assert(m_state == RADIO_STATE_WAITING_RX_FRAME);

            auto_ack_abort(cca ? RADIO_STATE_CCA_BEFORE_TX : RADIO_STATE_TX_FRAME);

            nrf_radio_tx_power_set(nrf_drv_radio802154_pib_tx_power_get());
            nrf_radio_packet_ptr_set(p_data);

            // Clear events that could have happened in critical section due to receiving frame or RX ramp up.
            nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
            nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

            // Check 2nd time in case this procedure was interrupted.
            if (nrf_raal_timeslot_request(
                    nrf_drv_radio802154_tx_duration_get(p_data[0], ack_is_requested(p_data))))
            {
                result = true;
            }
            else
            {
                irq_deinit();
                nrf_radio_reset();

                state_set(RADIO_STATE_WAITING_TIMESLOT);
            }
        }
        else
        {
            mutex_unlock();
        }
    }

    return result;
}

bool nrf_drv_radio802154_fsm_energy_detection(uint32_t time_us)
{
    bool result = false;

    switch (m_state)
    {
        case RADIO_STATE_SLEEP:
            if (mutex_lock())
            {
                state_set(RADIO_STATE_ED);
                m_ed_time_left = time_us;
                m_ed_result    = 0;

                nrf_raal_continuous_mode_enter();

                result = true;
            }

            break;

        case RADIO_STATE_WAITING_RX_FRAME:
            if (mutex_lock())
            {
                m_ed_result = 0;

                if (ed_iter_setup(time_us))
                {
                    auto_ack_abort(RADIO_STATE_ED);

                    assert(nrf_radio_shorts_get() == SHORTS_IDLE);

                    // Clear events that could have happened in critical section due to receiving
                    // frame or RX ramp up.
                    nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_END);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
                }
                else
                {
                    state_set(RADIO_STATE_ED);
                }

                result = true;
            }

            break;

        case RADIO_STATE_DISABLING:
        case RADIO_STATE_RX_HEADER:
        case RADIO_STATE_RX_FRAME:
        case RADIO_STATE_TX_ACK:
        case RADIO_STATE_WAITING_TIMESLOT:
            break;

        default:
            assert(false); // This should not happen.
    }

    return result;
}

bool nrf_drv_radio802154_fsm_cca(void)
{
    bool result = false;

    switch (m_state)
    {
        case RADIO_STATE_SLEEP:
            if (mutex_lock())
            {
                state_set(RADIO_STATE_CCA);

                nrf_raal_continuous_mode_enter();

                result = true;
            }

            break;

        case RADIO_STATE_WAITING_RX_FRAME:
            if (mutex_lock())
            {
                if (nrf_raal_timeslot_request(nrf_drv_radio802154_cca_duration_get()))
                {
                    auto_ack_abort(RADIO_STATE_CCA);

                    // Clear events that could have happened in critical section due to receiving
                    // frame or RX ramp up.
                    nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_END);
                    nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
                }
                else
                {
                    state_set(RADIO_STATE_CCA);
                }

                result = true;
            }

            break;

        case RADIO_STATE_DISABLING:
        case RADIO_STATE_RX_HEADER:
        case RADIO_STATE_RX_FRAME:
        case RADIO_STATE_TX_ACK:
        case RADIO_STATE_WAITING_TIMESLOT:
            break;

        default:
            assert(false); // This should not happen.
    }

    return result;
}

bool nrf_drv_radio802154_fsm_continuous_carrier(void)
{
    bool result = false;

    if (mutex_lock())
    {
        assert(m_state == RADIO_STATE_WAITING_RX_FRAME ||
               m_state == RADIO_STATE_SLEEP);

        auto_ack_abort(RADIO_STATE_CONTINUOUS_CARRIER);

        // Clear events that could have happened in critical section due to receiving frame or RX ramp up.
        nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
        nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
        nrf_radio_event_clear(NRF_RADIO_EVENT_END);
        nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

        result = true;
    }

    return result;
}

void nrf_drv_radio802154_fsm_notify_buffer_free(rx_buffer_t * p_buffer)
{
    p_buffer->free = true;

    switch (m_state)
    {
        case RADIO_STATE_WAITING_RX_FRAME:

            switch (nrf_radio_state_get())
            {
                case NRF_RADIO_STATE_RX_DISABLE: // This one could happen after receive of broadcast frame.
                case NRF_RADIO_STATE_TX_DISABLE: // This one could happen due to stopping ACK.
                case NRF_RADIO_STATE_DISABLED:   // This one could happen during stopping ACK.
                case NRF_RADIO_STATE_RX_RU:      // This one could happen during enabling receiver (after sending ACK).
                case NRF_RADIO_STATE_RX:         // This one could happen if any other buffer is in use.
                case NRF_RADIO_STATE_TX_RU:      // This one could happen if received a short frame.
                case NRF_RADIO_STATE_TX_IDLE:    // This one could happen if received a short frame.
                    break;

                case NRF_RADIO_STATE_RX_IDLE:
                    // Mutex to make sure Radio State did not change between IRQ and this process.
                    // Check shorts to make sure RX_IDLE state is due to occupied buffers - not
                    // during END/DISABLE short.
                    // If API call changed Radio state leave Radio as it is.
                    if (mutex_lock() && nrf_radio_shorts_get() == SHORTS_IDLE)
                    {
                        shorts_rx_initial_set();

                        rx_buffer_in_use_set(p_buffer);
                        rx_frame_start();

                        // Clear events that could have happened in critical section due to RX
                        // ramp up.
                        nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

                        mutex_unlock();
                    }
                    break;

                default:
                    assert(false);
            }

            break;

        default:
            // Don't perform any action in any other state (receiver should not be started).
            break;
    }
}

void nrf_drv_radio802154_fsm_channel_update(void)
{
    switch (m_state)
    {
        case RADIO_STATE_WAITING_RX_FRAME:
            assert(mutex_lock());

            channel_set(nrf_drv_radio802154_pib_channel_get());
            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);

            // Clear events that could have happened in critical section due to receiving
            // frame or RX ramp up.
            nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
            nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

            break;

        case RADIO_STATE_CONTINUOUS_CARRIER:
            channel_set(nrf_drv_radio802154_pib_channel_get());
            nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
            break;

        case RADIO_STATE_RX_HEADER:
        case RADIO_STATE_RX_FRAME:
        case RADIO_STATE_TX_ACK:
        case RADIO_STATE_CCA:
        case RADIO_STATE_TX_FRAME:
        case RADIO_STATE_RX_ACK:
            channel_set(nrf_drv_radio802154_pib_channel_get());
            break;

        case RADIO_STATE_DISABLING:
        case RADIO_STATE_SLEEP:
        case RADIO_STATE_WAITING_TIMESLOT:
        case RADIO_STATE_CCA_BEFORE_TX:
        case RADIO_STATE_ED:
            // Don't perform any action in these states (channel will be updated when receiver is
            // enabled).
            break;

    }
}

void nrf_drv_radio802154_fsm_cca_cfg_update(void)
{
    switch (m_state)
    {
        case RADIO_STATE_WAITING_RX_FRAME:
        case RADIO_STATE_RX_HEADER:
        case RADIO_STATE_RX_FRAME:
        case RADIO_STATE_TX_ACK:
        case RADIO_STATE_CCA:
        case RADIO_STATE_CCA_BEFORE_TX:
        case RADIO_STATE_TX_FRAME:
        case RADIO_STATE_RX_ACK:
            cca_configuration_update();
            break;

        case RADIO_STATE_DISABLING:
        case RADIO_STATE_SLEEP:
        case RADIO_STATE_WAITING_TIMESLOT:
        case RADIO_STATE_ED:
        case RADIO_STATE_CONTINUOUS_CARRIER:
            // Don't perform any action in these states (CCA configuration will be updated when
            // receiver is enabled).
            break;

    }
}

#if RADIO_INTERNAL_IRQ_HANDLING
void RADIO_IRQHandler(void)
#else // RADIO_INTERNAL_IRQ_HADLING
void nrf_drv_radio802154_fsm_irq_handler(void)
#endif // RADIO_INTERNAL_IRQ_HANDLING
{
    irq_handler();
}
