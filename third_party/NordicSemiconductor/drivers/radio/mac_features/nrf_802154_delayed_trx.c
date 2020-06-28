/* Copyright (c) 2018 - 2019, Nordic Semiconductor ASA
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
 *   This file implements delayed transmission and reception features.
 *
 */

#include "nrf_802154_delayed_trx.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "../nrf_802154_debug.h"
#include "nrf_802154_config.h"
#include "nrf_802154_const.h"
#include "nrf_802154_frame_parser.h"
#include "nrf_802154_notification.h"
#include "nrf_802154_pib.h"
#include "nrf_802154_procedures_duration.h"
#include "nrf_802154_request.h"
#include "rsch/nrf_802154_rsch.h"
#include "timer_scheduler/nrf_802154_timer_sched.h"

/* The following time is the sum of 70us RTC_IRQHandler processing time, 40us of time that elapses
 * from the moment a board starts transmission to the moment other boards (e.g. sniffer) are able
 * to detect that frame and in case of TX - 50us that accounts for a delay of yet unknown origin.
 */
#define TX_SETUP_TIME 160u ///< Time needed to prepare TX procedure [us]. It does not include TX ramp-up time.
#define RX_SETUP_TIME 110u ///< Time needed to prepare RX procedure [us]. It does not include RX ramp-up time.

/**
 * @brief States of delayed operations.
 */
typedef enum
{
    DELAYED_TRX_OP_STATE_STOPPED, ///< Delayed operation stopped.
    DELAYED_TRX_OP_STATE_PENDING, ///< Delayed operation scheduled and waiting for timeslot.
    DELAYED_TRX_OP_STATE_ONGOING, ///< Delayed operation ongoing (during timeslot).
    DELAYED_TRX_OP_STATE_NB       ///< Number of delayed operation states.
} delayed_trx_op_state_t;

/**
 * @brief RX delayed operation frame data.
 */
typedef struct
{
    uint32_t sof_timestamp; ///< Timestamp of last start of frame notification received in RX window.
    uint8_t  psdu_length;   ///< Length in bytes of the frame to be received in RX window.
    bool     ack_requested; ///< Flag indicating if Ack for the frame to be received in RX window is requested.
} delayed_rx_frame_data_t;

/**
 * @brief TX delayed operation configuration.
 */
static const uint8_t * mp_tx_data;         ///< Pointer to a buffer containing PHR and PSDU of the frame requested to be transmitted.
static bool            m_tx_cca;           ///< If CCA should be performed prior to transmission.
static uint8_t         m_tx_channel;       ///< Channel number on which transmission should be performed.

/**
 * @brief RX delayed operation configuration.
 */
static nrf_802154_timer_t m_timeout_timer; ///< Timer for delayed RX timeout handling.
static uint8_t            m_rx_channel;    ///< Channel number on which reception should be performed.

/**
 * @brief State of delayed operations.
 */
static volatile delayed_trx_op_state_t m_dly_op_state[RSCH_DLY_TS_NUM];

/**
 * @brief RX delayed operation frame data.
 */
static volatile delayed_rx_frame_data_t m_dly_rx_frame;

/**
 * Set state of a delayed operation.
 *
 * @param[in]  expected_dly_rx_state   Delayed RX current expected state.
 * @param[in]  new_dly_rx_state        Delayed RX new state to be set.
 *
 * @retval true   Successfully set the new state.
 * @retval false  Failed to set the new state.
 */
static bool dly_rx_state_set(delayed_trx_op_state_t expected_dly_rx_state,
                             delayed_trx_op_state_t new_dly_rx_state)
{
    volatile delayed_trx_op_state_t current_dly_rx_state;

    do
    {
        current_dly_rx_state =
            (delayed_trx_op_state_t)__LDREXB((uint8_t *)&m_dly_op_state[RSCH_DLY_RX]);

        if (current_dly_rx_state != expected_dly_rx_state)
        {
            __CLREX();
            return false;
        }

    }
    while (__STREXB((uint8_t)new_dly_rx_state, (uint8_t *)&m_dly_op_state[RSCH_DLY_RX]));

    __DMB();

    return true;
}

/**
 * Get state of a delayed operation.
 *
 * @param[in]  dly_ts_id  Delayed timeslot ID.
 *
 * @retval     State of delayed operation.
 */
static delayed_trx_op_state_t dly_op_state_get(rsch_dly_ts_id_t dly_ts_id)
{
    assert(dly_ts_id < RSCH_DLY_TS_NUM);

    return m_dly_op_state[dly_ts_id];
}

/**
 * Set state of a delayed operation.
 *
 * @param[in]  dly_ts_id      Delayed timeslot ID.
 * @param[in]  expected_state Expected delayed operation state prior state transition.
 * @param[in]  new_state      Delayed operation state to enter.
 */
static void dly_op_state_set(rsch_dly_ts_id_t       dly_ts_id,
                             delayed_trx_op_state_t expected_state,
                             delayed_trx_op_state_t new_state)
{
    assert(new_state < DELAYED_TRX_OP_STATE_NB);

    switch (dly_ts_id)
    {
        case RSCH_DLY_TX:
        {
            assert(m_dly_op_state[RSCH_DLY_TX] == expected_state);
            m_dly_op_state[RSCH_DLY_TX] = new_state;
            break;
        }

        case RSCH_DLY_RX:
        {
            bool result = dly_rx_state_set(expected_state, new_state);

            assert(result);
            (void)result;
            break;
        }

        default:
        {
            assert(false);
            break;
        }
    }
}

/**
 * Start delayed operation.
 *
 * @param[in]  t0      Base time of the timestamp of the timeslot start [us].
 * @param[in]  dt      Time delta between @p t0 and the timestamp of the timeslot start [us].
 * @param[in]  length  Requested radio timeslot length [us].
 * @param[in]  dly_ts  Delayed timeslot ID.
 */
static bool dly_op_request(uint32_t         t0,
                           uint32_t         dt,
                           uint32_t         length,
                           rsch_dly_ts_id_t dly_ts_id)
{
    bool result;

    // Set PENDING state before timeslot request, in case timeslot starts
    // immediatly and interrupts current function execution.
    dly_op_state_set(dly_ts_id, DELAYED_TRX_OP_STATE_STOPPED, DELAYED_TRX_OP_STATE_PENDING);

    result = nrf_802154_rsch_delayed_timeslot_request(t0,
                                                      dt,
                                                      length,
                                                      RSCH_PRIO_MAX,
                                                      dly_ts_id);

    if (!result)
    {
        dly_op_state_set(dly_ts_id, DELAYED_TRX_OP_STATE_PENDING, DELAYED_TRX_OP_STATE_STOPPED);
    }

    return result;
}

/**
 * Notify MAC layer that no frame was received before timeout.
 *
 * @param[in]  p_context  Not used.
 */
static void notify_rx_timeout(void * p_context)
{
    (void)p_context;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_DTRX_RX_TIMEOUT);

    assert(dly_op_state_get(RSCH_DLY_RX) != DELAYED_TRX_OP_STATE_PENDING);

    if (dly_op_state_get(RSCH_DLY_RX) == DELAYED_TRX_OP_STATE_ONGOING)
    {
        uint32_t now           = nrf_802154_timer_sched_time_get();
        uint32_t sof_timestamp = m_dly_rx_frame.sof_timestamp;

        // Make sure that the timestamp has been latched safely. If frame reception preempts the code
        // after executing this line, the RX window will not be extended.
        __DMB();
        uint8_t  psdu_length   = m_dly_rx_frame.psdu_length;
        bool     ack_requested = m_dly_rx_frame.ack_requested;
        uint32_t frame_length  = nrf_802154_rx_duration_get(psdu_length, ack_requested);

        if (nrf_802154_timer_sched_time_is_in_future(now, sof_timestamp, frame_length))
        {
            // @TODO protect against infinite extensions - allow only one timer extension
            m_timeout_timer.t0 = sof_timestamp;
            m_timeout_timer.dt = frame_length;

            nrf_802154_timer_sched_add(&m_timeout_timer, true);
        }
        else
        {
            if (dly_rx_state_set(DELAYED_TRX_OP_STATE_ONGOING, DELAYED_TRX_OP_STATE_STOPPED))
            {
                nrf_802154_notify_receive_failed(NRF_802154_RX_ERROR_DELAYED_TIMEOUT);
            }

            // even if the set operation failed, the delayed RX state
            // should be set to STOPPED from other context anyway
            assert(dly_op_state_get(RSCH_DLY_RX) == DELAYED_TRX_OP_STATE_STOPPED);
        }
    }

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_DTRX_RX_TIMEOUT);
}

/**
 * Transmit request result callback.
 *
 * @param[in]  result  Result of TX request.
 */
static void tx_timeslot_started_callback(bool result)
{
    (void)result;

    // To avoid attaching to every possible transmit hook, in order to be able
    // to switch from ONGOING to STOPPED state, ONGOING state is not used at all
    // and state is changed to STOPPED right after transmit request.
    m_dly_op_state[RSCH_DLY_TX] = DELAYED_TRX_OP_STATE_STOPPED;

    if (!result)
    {
        nrf_802154_notify_transmit_failed(mp_tx_data, NRF_802154_TX_ERROR_TIMESLOT_DENIED);
    }
}

/**
 * Receive request result callback.
 *
 * @param[in]  result  Result of RX request.
 */
static void rx_timeslot_started_callback(bool result)
{
    if (result)
    {
        uint32_t now;

        dly_op_state_set(RSCH_DLY_RX, DELAYED_TRX_OP_STATE_PENDING, DELAYED_TRX_OP_STATE_ONGOING);

        now = nrf_802154_timer_sched_time_get();

        m_timeout_timer.t0           = now;
        m_dly_rx_frame.sof_timestamp = now;
        m_dly_rx_frame.psdu_length   = 0;
        m_dly_rx_frame.ack_requested = false;

        nrf_802154_timer_sched_add(&m_timeout_timer, true);
    }
    else
    {
        dly_op_state_set(RSCH_DLY_RX, DELAYED_TRX_OP_STATE_PENDING, DELAYED_TRX_OP_STATE_STOPPED);

        nrf_802154_notify_receive_failed(NRF_802154_RX_ERROR_DELAYED_TIMESLOT_DENIED);
    }
}

/**
 * Handle TX timeslot start.
 */
static void tx_timeslot_started_callout(void)
{
    bool result;

    nrf_802154_pib_channel_set(m_tx_channel);
    result = nrf_802154_request_channel_update();

    if (result)
    {
        (void)nrf_802154_request_transmit(NRF_802154_TERM_802154,
                                          REQ_ORIG_DELAYED_TRX,
                                          mp_tx_data,
                                          m_tx_cca,
                                          true,
                                          tx_timeslot_started_callback);
    }
    else
    {
        tx_timeslot_started_callback(result);
    }
}

/**
 * Handle RX timeslot start.
 */
static void rx_timeslot_started_callout(void)
{
    bool result;

    nrf_802154_pib_channel_set(m_rx_channel);
    result = nrf_802154_request_channel_update();

    if (result)
    {
        (void)nrf_802154_request_receive(NRF_802154_TERM_802154,
                                         REQ_ORIG_DELAYED_TRX,
                                         rx_timeslot_started_callback,
                                         true);
    }
    else
    {
        rx_timeslot_started_callback(result);
    }
}

bool nrf_802154_delayed_trx_transmit(const uint8_t * p_data,
                                     bool            cca,
                                     uint32_t        t0,
                                     uint32_t        dt,
                                     uint8_t         channel)
{
    bool     result;
    uint16_t timeslot_length;
    bool     ack;

    result = dly_op_state_get(RSCH_DLY_TX) == DELAYED_TRX_OP_STATE_STOPPED;

    if (result)
    {
        dt -= TX_SETUP_TIME;
        dt -= TX_RAMP_UP_TIME;

        if (cca)
        {
            dt -= nrf_802154_cca_before_tx_duration_get();
        }

        ack             = p_data[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT;
        timeslot_length = nrf_802154_tx_duration_get(p_data[0], cca, ack);

        mp_tx_data   = p_data;
        m_tx_cca     = cca;
        m_tx_channel = channel;

        result = dly_op_request(t0, dt, timeslot_length, RSCH_DLY_TX);
    }

    return result;
}

bool nrf_802154_delayed_trx_receive(uint32_t t0,
                                    uint32_t dt,
                                    uint32_t timeout,
                                    uint8_t  channel)
{
    bool     result;
    uint16_t timeslot_length;

    result = dly_op_state_get(RSCH_DLY_RX) == DELAYED_TRX_OP_STATE_STOPPED;

    if (result)
    {

        dt -= RX_SETUP_TIME;
        dt -= RX_RAMP_UP_TIME;

        timeslot_length = timeout + nrf_802154_rx_duration_get(MAX_PACKET_SIZE, true);

        m_timeout_timer.dt        = timeout + RX_RAMP_UP_TIME;
        m_timeout_timer.callback  = notify_rx_timeout;
        m_timeout_timer.p_context = NULL;

        m_rx_channel = channel;

        // remove timer in case it was left after abort operation
        nrf_802154_timer_sched_remove(&m_timeout_timer, NULL);

        result = dly_op_request(t0, dt, timeslot_length, RSCH_DLY_RX);
    }

    return result;
}

static inline void timeslot_started_callout(rsch_dly_ts_id_t dly_ts_id)
{
    switch (dly_ts_id)
    {
        case RSCH_DLY_TX:
            tx_timeslot_started_callout();
            break;

        case RSCH_DLY_RX:
            rx_timeslot_started_callout();
            break;

        default:
            assert(false);
            break;
    }
}

void nrf_802154_rsch_delayed_timeslot_started(rsch_dly_ts_id_t dly_ts_id)
{
    switch (dly_op_state_get(dly_ts_id))
    {
        case DELAYED_TRX_OP_STATE_PENDING:
            timeslot_started_callout(dly_ts_id);
            break;

        case DELAYED_TRX_OP_STATE_STOPPED:
            /* Intentionally do nothing */
            break;

        default:
            assert(false);
    }
}

bool nrf_802154_delayed_trx_transmit_cancel(void)
{
    bool result;

    result                      = nrf_802154_rsch_delayed_timeslot_cancel(RSCH_DLY_TX);
    m_dly_op_state[RSCH_DLY_TX] = DELAYED_TRX_OP_STATE_STOPPED;

    return result;
}

bool nrf_802154_delayed_trx_receive_cancel(void)
{
    bool result;

    result = nrf_802154_rsch_delayed_timeslot_cancel(RSCH_DLY_RX);

    bool was_running;

    nrf_802154_timer_sched_remove(&m_timeout_timer, &was_running);

    m_dly_op_state[RSCH_DLY_RX] = DELAYED_TRX_OP_STATE_STOPPED;

    result = result || was_running;

    return result;
}

bool nrf_802154_delayed_trx_abort(nrf_802154_term_t term_lvl, req_originator_t req_orig)
{
    bool result = true;

    if (req_orig == REQ_ORIG_DELAYED_TRX)
    {
        // Ignore if self-request.
    }
    else if (dly_op_state_get(RSCH_DLY_RX) == DELAYED_TRX_OP_STATE_ONGOING)
    {
        if (term_lvl >= NRF_802154_TERM_802154)
        {
            if (dly_rx_state_set(DELAYED_TRX_OP_STATE_ONGOING, DELAYED_TRX_OP_STATE_STOPPED))
            {
                nrf_802154_notify_receive_failed(NRF_802154_RX_ERROR_DELAYED_ABORTED);
            }

            // even if the set operation failed, the delayed RX state
            // should be set to STOPPED from other context anyway
            assert(dly_op_state_get(RSCH_DLY_RX) == DELAYED_TRX_OP_STATE_STOPPED);
        }
        else
        {
            result = false;
        }
    }

    return result;
}

void nrf_802154_delayed_trx_rx_started_hook(const uint8_t * p_frame)
{
    if (dly_op_state_get(RSCH_DLY_RX) == DELAYED_TRX_OP_STATE_ONGOING)
    {
        m_dly_rx_frame.sof_timestamp = nrf_802154_timer_sched_time_get();
        m_dly_rx_frame.psdu_length   = p_frame[PHR_OFFSET];
        m_dly_rx_frame.ack_requested = nrf_802154_frame_parser_ar_bit_is_set(p_frame);
    }
}
