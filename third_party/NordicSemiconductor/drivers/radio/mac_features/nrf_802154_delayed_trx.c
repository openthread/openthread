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
 * @file
 *   This file implements delayed transmission and reception features.
 *
 */

#include "nrf_802154_delayed_trx.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154_config.h"
#include "nrf_802154_const.h"
#include "nrf_802154_notification.h"
#include "nrf_802154_pib.h"
#include "nrf_802154_procedures_duration.h"
#include "nrf_802154_request.h"
#include "nrf_802154_rsch.h"

#define TX_SETUP_TIME 190  ///< Time [us] needed to change channel, stop rx and setup tx procedure.

static const uint8_t * mp_tx_psdu;    ///< Pointer to PHR + PSDU of the frame requested to transmit.
static bool            m_tx_cca;      ///< If CCA should be performed prior to transmission.
static uint8_t         m_tx_channel;  ///< Channel number on which transmission should be performed.

/**
 * Check if delayed transmission procedure is in progress.
 *
 * @retval true   Delayed transmission is in progress (waiting or transmitting).
 * @retval false  Delayed transmission is not in progress.
 */
static bool tx_is_in_progress(void)
{
    return mp_tx_psdu != NULL;
}

/**
 * Mark that delayed transmission procedure has stopped.
 */
static void tx_stop(void)
{
    mp_tx_psdu = NULL;
}

/**
 * Notify MAC layer that requested timeslot is not granted if tx request failed.
 *
 * @param[in]  result  Result of TX request.
 */
static void notify_tx_timeslot_denied(bool result)
{
    if (!result)
    {
        nrf_802154_notify_transmit_failed(mp_tx_psdu, NRF_802154_TX_ERROR_TIMESLOT_DENIED);
    }
}

bool nrf_802154_delayed_trx_transmit(const uint8_t * p_data,
                                     bool            cca,
                                     uint32_t        t0,
                                     uint32_t        dt,
                                     uint8_t         channel)
{
    bool     result = true;
    uint16_t timeslot_length;

    if (tx_is_in_progress())
    {
        result = false;
    }

    if (result)
    {
        dt -= TX_SETUP_TIME;
        dt -= TX_RAMP_UP_TIME;

        if (cca)
        {
            dt -= nrf_802154_cca_before_tx_duration_get();
        }

        mp_tx_psdu   = p_data;
        m_tx_cca     = cca;
        m_tx_channel = channel;

        timeslot_length = nrf_802154_tx_duration_get(p_data[0],
                                                     cca,
                                                     p_data[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT);

        result = nrf_802154_rsch_delayed_timeslot_request(t0, dt, timeslot_length);

        if (!result)
        {
            notify_tx_timeslot_denied(result);
            tx_stop();
        }
    }

    return result;
}

void nrf_802154_rsch_delayed_timeslot_started(void)
{
    bool result;

    assert(tx_is_in_progress());

    nrf_802154_pib_channel_set(m_tx_channel);
    result = nrf_802154_request_channel_update();

    if (result)
    {
        result = nrf_802154_request_transmit(NRF_802154_TERM_802154,
                                             REQ_ORIG_DELAYED_TRX,
                                             mp_tx_psdu,
                                             m_tx_cca,
                                             true,
                                             notify_tx_timeslot_denied);
        (void)result;
    }
    else
    {
        notify_tx_timeslot_denied(result);
    }

    tx_stop();
}

void nrf_802154_rsch_delayed_timeslot_failed(void)
{
    assert(tx_is_in_progress());

    notify_tx_timeslot_denied(false);

    tx_stop();
}
