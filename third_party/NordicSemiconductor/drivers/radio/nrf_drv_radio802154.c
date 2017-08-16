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
 *   This file implements the nrf 802.15.4 radio driver.
 *
 */

#include "nrf_drv_radio802154.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "nrf_drv_radio802154_ack_pending_bit.h"
#include "nrf_drv_radio802154_config.h"
#include "nrf_drv_radio802154_const.h"
#include "nrf_drv_radio802154_debug.h"
#include "nrf_drv_radio802154_fsm.h"
#include "nrf_drv_radio802154_notification.h"
#include "nrf_drv_radio802154_pib.h"
#include "nrf_drv_radio802154_priority_drop.h"
#include "nrf_drv_radio802154_request.h"
#include "nrf_drv_radio802154_rx_buffer.h"
#include "hal/nrf_radio.h"
#include "raal/nrf_raal_api.h"

#include <cmsis/core_cmFunc.h>

#define RAW_LENGTH_OFFSET  0
#define RAW_PAYLOAD_OFFSET 1


void nrf_drv_radio802154_channel_set(uint8_t channel)
{
    bool changed = nrf_drv_radio802154_pib_channel_get() != channel;

    nrf_drv_radio802154_pib_channel_set(channel);

    if (changed)
    {
        nrf_drv_radio802154_request_channel_update();
    }
}

uint8_t nrf_drv_radio802154_channel_get(void)
{
    return nrf_drv_radio802154_pib_channel_get();
}

void nrf_drv_radio802154_tx_power_set(int8_t power)
{
    nrf_drv_radio802154_pib_tx_power_set(power);
}

int8_t nrf_drv_radio802154_tx_power_get(void)
{
    return nrf_drv_radio802154_pib_tx_power_get();
}

void nrf_drv_radio802154_pan_id_set(const uint8_t * p_pan_id)
{
    nrf_drv_radio802154_pib_pan_id_set(p_pan_id);
}

void nrf_drv_radio802154_extended_address_set(const uint8_t * p_extended_address)
{
    nrf_drv_radio802154_pib_extended_address_set(p_extended_address);
}

void nrf_drv_radio802154_short_address_set(const uint8_t * p_short_address)
{
    nrf_drv_radio802154_pib_short_address_set(p_short_address);
}

int8_t nrf_drv_radio802154_dbm_from_energy_level_calculate(uint8_t energy_level)
{
    // TODO: Correct this calculation after lab tests.
    return -94 + energy_level;
}

void nrf_drv_radio802154_init(void)
{
    nrf_drv_radio802154_ack_pending_bit_init();
    nrf_drv_radio802154_debug_init();
    nrf_drv_radio802154_fsm_init();
    nrf_drv_radio802154_notification_init();
    nrf_drv_radio802154_pib_init();
    nrf_drv_radio802154_priority_drop_init();
    nrf_drv_radio802154_request_init();
    nrf_drv_radio802154_rx_buffer_init();
    nrf_raal_init();
}

void nrf_drv_radio802154_deinit(void)
{
    nrf_drv_radio802154_fsm_deinit();
}

#if !RADIO_INTERNAL_IRQ_HANDLING
void nrf_drv_radio802154_irq_handler(void)
{
    nrf_drv_radio802154_fsm_irq_handler();
}
#endif // !RADIO_INTERNAL_IRQ_HANDLING

nrf_drv_radio802154_state_t nrf_drv_radio802154_state_get(void)
{
    switch (nrf_drv_radio802154_fsm_state_get())
    {
    case RADIO_STATE_DISABLING:
    case RADIO_STATE_SLEEP:
        return NRF_DRV_RADIO802154_STATE_SLEEP;

    case RADIO_STATE_WAITING_TIMESLOT:
    case RADIO_STATE_WAITING_RX_FRAME:
    case RADIO_STATE_RX_HEADER:
    case RADIO_STATE_RX_FRAME:
    case RADIO_STATE_TX_ACK:
        return NRF_DRV_RADIO802154_STATE_RECEIVE;

    case RADIO_STATE_CCA_BEFORE_TX:
    case RADIO_STATE_TX_FRAME:
    case RADIO_STATE_RX_ACK:
        return NRF_DRV_RADIO802154_STATE_TRANSMIT;

    case RADIO_STATE_ED:
        return NRF_DRV_RADIO802154_STATE_ENERGY_DETECTION;

    case RADIO_STATE_CCA:
        return NRF_DRV_RADIO802154_STATE_CCA;

    case RADIO_STATE_CONTINUOUS_CARRIER:
        return NRF_DRV_RADIO802154_STATE_CONTINUOUS_CARRIER;
    }

    return NRF_DRV_RADIO802154_STATE_INVALID;
}

bool nrf_drv_radio802154_sleep(void)
{
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_SLEEP);

    bool result = true;

    switch (nrf_drv_radio802154_fsm_state_get())
    {
    case RADIO_STATE_DISABLING:
    case RADIO_STATE_SLEEP:
        break;

    case RADIO_STATE_WAITING_TIMESLOT:
    case RADIO_STATE_WAITING_RX_FRAME:
    case RADIO_STATE_RX_HEADER:
    case RADIO_STATE_RX_FRAME:
    case RADIO_STATE_TX_ACK:
    case RADIO_STATE_CCA_BEFORE_TX:
    case RADIO_STATE_TX_FRAME:
    case RADIO_STATE_RX_ACK:
        result = nrf_drv_radio802154_request_sleep();
        break;

    default:
        assert(false);
    }

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_SLEEP);
    return result;
}

void nrf_drv_radio802154_receive(void)
{
    bool result;
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_RECEIVE);

    result = nrf_drv_radio802154_request_receive();
    assert(result == true);

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_RECEIVE);
}

bool nrf_drv_radio802154_transmit_raw(const uint8_t * p_data, bool cca)
{
    bool result;
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_TRANSMIT);

    result = nrf_drv_radio802154_request_transmit(p_data, cca);

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_TRANSMIT);
    return result;
}

bool nrf_drv_radio802154_transmit(const uint8_t * p_data, uint8_t length, bool cca)
{
#if defined ( __GNUC__ )

    static uint8_t tx_buffer[RAW_PAYLOAD_OFFSET + MAX_PACKET_SIZE]
                __attribute__ ((section ("nrf_radio_buffer.tx_buffer")));

#elif defined ( __ICCARM__ )

#pragma location="NRF_RADIO_BUFFER"
    static uint8_t tx_buffer[RAW_PAYLOAD_OFFSET + MAX_PACKET_SIZE];

#endif

    assert(length <= MAX_PACKET_SIZE - FCS_SIZE);

    tx_buffer[RAW_LENGTH_OFFSET] = length + FCS_SIZE;
    memcpy(&tx_buffer[RAW_PAYLOAD_OFFSET], p_data, length);

    return nrf_drv_radio802154_transmit_raw(tx_buffer, cca);
}

bool nrf_drv_radio802154_energy_detection(uint32_t time_us)
{
    bool result;
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_ENERGY_DETECTION);

    result = nrf_drv_radio802154_request_energy_detection(time_us);

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_ENERGY_DETECTION);
    return result;
}

bool nrf_drv_radio802154_cca(void)
{
    bool result;
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_CCA);

    result = nrf_drv_radio802154_request_cca();

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_CCA);
    return result;
}

bool nrf_drv_radio802154_continuous_carrier(void)
{
    bool result;
    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_CONTINUOUS_CARRIER);

    result = nrf_drv_radio802154_request_continuous_carrier();

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_CONTINUOUS_CARRIER);
    return result;
}

void nrf_drv_radio802154_buffer_free_raw(uint8_t * p_data)
{
    rx_buffer_t * p_buffer = (rx_buffer_t *)p_data;

    assert(p_buffer->free == false);

    nrf_drv_radio802154_log(EVENT_TRACE_ENTER, FUNCTION_BUFFER_FREE);

    nrf_drv_radio802154_request_buffer_free(p_data);

    nrf_drv_radio802154_log(EVENT_TRACE_EXIT, FUNCTION_BUFFER_FREE);
}

void nrf_drv_radio802154_buffer_free(uint8_t * p_data)
{
    nrf_drv_radio802154_buffer_free_raw(p_data - RAW_PAYLOAD_OFFSET);
}

int8_t nrf_drv_radio802154_rssi_last_get(void)
{
    uint8_t minus_dbm = nrf_radio_rssi_sample_get();
    return - (int8_t)minus_dbm;
}

bool nrf_drv_radio802154_promiscuous_get(void)
{
    return nrf_drv_radio802154_pib_promiscuous_get();
}

void nrf_drv_radio802154_promiscuous_set(bool enabled)
{
    nrf_drv_radio802154_pib_promiscuous_set(enabled);
}

void nrf_drv_radio802154_auto_ack_set(bool enabled)
{
    nrf_drv_radio802154_pib_auto_ack_set(enabled);
}

bool nrf_drv_radio802154_auto_ack_get(void)
{
    return nrf_drv_radio802154_pib_auto_ack_get();
}

void nrf_drv_radio802154_auto_pending_bit_set(bool enabled)
{
    nrf_drv_radio802154_ack_pending_bit_set(enabled);
}

bool nrf_drv_radio802154_pending_bit_for_addr_set(const uint8_t * p_addr, bool extended)
{
    return nrf_drv_radio802154_ack_pending_bit_for_addr_set(p_addr, extended);
}

bool nrf_drv_radio802154_pending_bit_for_addr_clear(const uint8_t * p_addr, bool extended)
{
    return nrf_drv_radio802154_ack_pending_bit_for_addr_clear(p_addr, extended);
}

void nrf_drv_radio802154_pending_bit_for_addr_reset(bool extended)
{
    nrf_drv_radio802154_ack_pending_bit_for_addr_reset(extended);
}

void nrf_drv_radio802154_cca_cfg_set(const nrf_drv_radio802154_cca_cfg_t * p_cca_cfg)
{
    nrf_drv_radio802154_pib_cca_cfg_set(p_cca_cfg);

    nrf_drv_radio802154_request_cca_cfg_update();
}

void nrf_drv_radio802154_cca_cfg_get(nrf_drv_radio802154_cca_cfg_t * p_cca_cfg)
{
    nrf_drv_radio802154_pib_cca_cfg_get(p_cca_cfg);
}

__WEAK void nrf_drv_radio802154_rx_started(void)
{
    // Intentionally empty
}

__WEAK void nrf_drv_radio802154_received(uint8_t * p_data, uint8_t length, int8_t power, int8_t lqi)
{
    (void) length;
    (void) power;
    (void) lqi;

    nrf_drv_radio802154_buffer_free(p_data);
}

__WEAK void nrf_drv_radio802154_received_raw(uint8_t * p_data, int8_t power, int8_t lqi)
{
    nrf_drv_radio802154_received(p_data + RAW_PAYLOAD_OFFSET,
                                 p_data[RAW_LENGTH_OFFSET],
                                 power,
                                 lqi);
}

__WEAK void nrf_drv_radio802154_tx_started(void)
{
    // Intentionally empty
}

__WEAK void nrf_drv_radio802154_transmitted(uint8_t * p_ack, uint8_t length, int8_t power, int8_t lqi)
{
    (void) length;
    (void) power;
    (void) lqi;

    if (p_ack != NULL)
    {
        nrf_drv_radio802154_buffer_free(p_ack);
    }
}

__WEAK void nrf_drv_radio802154_transmitted_raw(uint8_t * p_ack, int8_t power, int8_t lqi)
{
    nrf_drv_radio802154_transmitted(p_ack + RAW_PAYLOAD_OFFSET,
                                    p_ack[RAW_LENGTH_OFFSET],
                                    power,
                                    lqi);
}

__WEAK void nrf_drv_radio802154_busy_channel(void)
{
    // Intentionally empty
}

__WEAK void nrf_drv_radio802154_energy_detected(uint8_t result)
{
    (void) result;
}

__WEAK void nrf_drv_radio802154_cca_done(bool channel_free)
{
    (void) channel_free;
}
