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
 *   This file implements procedures to set pending bit in nRF 802.15.4 radio driver.
 *
 */

#include "nrf_drv_radio802154_ack_pending_bit.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nrf_drv_radio802154_config.h"
#include "nrf_drv_radio802154_const.h"

#include "hal/nrf_radio.h"

/// Maximum number of Short Addresses of nodes for which there is pending data in buffer.
#define NUM_PENDING_SHORT_ADDRESSES     RADIO_PENDING_SHORT_ADDRESSES
/// Maximum number of Extended Addresses of nodes for which there is pending data in buffer.
#define NUM_PENDING_EXTENDED_ADDRESSES  RADIO_PENDING_EXTENDED_ADDRESSES
/// Value used to mark Short Address as unused.
#define UNUSED_PENDING_SHORT_ADDRESS    ((uint8_t [SHORT_ADDRESS_SIZE]) {0xff, 0xff})
/// Value used to mark Extended Address as unused.
#define UNUSED_PENDING_EXTENDED_ADDRESS ((uint8_t [EXTENDED_ADDRESS_SIZE]) {0})

/// If pending bit in ACK frame should be set to valid or default value.
static bool m_setting_pending_bit_enabled;
/// Array of Short Addresses of nodes for which there is pending data in the buffer.
static uint8_t m_pending_short[NUM_PENDING_SHORT_ADDRESSES][SHORT_ADDRESS_SIZE];
/// Array of Extended Addresses of nodes for which there is pending data in the buffer.
static uint8_t m_pending_extended[NUM_PENDING_EXTENDED_ADDRESSES][EXTENDED_ADDRESS_SIZE];

void nrf_drv_radio802154_ack_pending_bit_init(void)
{
    memset(m_pending_extended, 0, sizeof(m_pending_extended));
    memset(m_pending_short, 0xff, sizeof(m_pending_short));
    m_setting_pending_bit_enabled = true;
}

void nrf_drv_radio802154_ack_pending_bit_set(bool enabled)
{
    m_setting_pending_bit_enabled = enabled;
}

bool nrf_drv_radio802154_ack_pending_bit_for_addr_set(const uint8_t * p_addr, bool extended)
{
    uint8_t * p_empty_slot = NULL;

    if (extended)
    {
        for (uint32_t i = 0; i < NUM_PENDING_EXTENDED_ADDRESSES; i++)
        {
            if (0 == memcmp(m_pending_extended[i], p_addr, sizeof(m_pending_extended[i])))
            {
                return true;
            }

            if (0 == memcmp(m_pending_extended[i], UNUSED_PENDING_EXTENDED_ADDRESS, sizeof(m_pending_extended[i])))
            {
                p_empty_slot = m_pending_extended[i];
            }
        }

        if (p_empty_slot != NULL)
        {
            memcpy(p_empty_slot, p_addr, EXTENDED_ADDRESS_SIZE);
            return true;
        }
    }
    else
    {
        for (uint32_t i = 0; i < NUM_PENDING_SHORT_ADDRESSES; i++)
        {
            if (0 == memcmp(m_pending_short[i], p_addr, sizeof(m_pending_short[i])))
            {
                return true;
            }

            if (0 == memcmp(m_pending_short[i], UNUSED_PENDING_SHORT_ADDRESS, sizeof(m_pending_short[i])))
            {
                p_empty_slot = m_pending_short[i];
            }
        }

        if (p_empty_slot != NULL)
        {
            memcpy(p_empty_slot, p_addr, SHORT_ADDRESS_SIZE);
            return true;
        }
    }

    return false;
}

bool nrf_drv_radio802154_ack_pending_bit_for_addr_clear(const uint8_t * p_addr, bool extended)
{
    bool result = false;

    if (extended)
    {
        for (uint32_t i = 0; i < NUM_PENDING_EXTENDED_ADDRESSES; i++)
        {
            if (0 == memcmp(m_pending_extended[i], p_addr, sizeof(m_pending_extended[i])))
            {
                memset(m_pending_extended[i], 0, sizeof(m_pending_extended[i]));
                result = true;
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < NUM_PENDING_SHORT_ADDRESSES; i++)
        {
            if (0 == memcmp(m_pending_short[i], p_addr, sizeof(m_pending_short[i])))
            {
                memset(m_pending_short[i], 0xff, sizeof(m_pending_short[i]));
                result = true;
            }
        }
    }

    return result;
}

void nrf_drv_radio802154_ack_pending_bit_for_addr_reset(bool extended)
{
    if (extended)
    {
        memset(m_pending_extended, 0, sizeof(m_pending_extended));
    }
    else
    {
        memset(m_pending_short, 0xff, sizeof(m_pending_short));
    }
}

bool nrf_drv_radio802154_ack_pending_bit_should_be_set(const uint8_t * p_psdu)
{
    const uint8_t * p_src_addr;
    uint32_t        i;

    // If automatic setting of pending bit in ACK frames is disabled the pending bit is always set.
    if (!m_setting_pending_bit_enabled)
    {
        return true;
    }

    switch (p_psdu[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK)
    {
        case DEST_ADDR_TYPE_SHORT:
            p_src_addr = &p_psdu[SRC_ADDR_OFFSET_SHORT_DST];
            break;

        case DEST_ADDR_TYPE_EXTENDED:
            p_src_addr = &p_psdu[SRC_ADDR_OFFSET_EXTENDED_DST];
            break;

        default:
            return true;
    }

    if (0 == (p_psdu[PAN_ID_COMPR_OFFSET] & PAN_ID_COMPR_MASK))
    {
        p_src_addr += 2;
    }

    switch (p_psdu[SRC_ADDR_TYPE_OFFSET] & SRC_ADDR_TYPE_MASK)
    {
        case SRC_ADDR_TYPE_SHORT:
            for (i = 0; i < NUM_PENDING_SHORT_ADDRESSES; i++)
            {
                if (nrf_radio_state_get() != NRF_RADIO_STATE_TX_RU)
                {
                    break;
                }

                if (0 == memcmp(p_src_addr, m_pending_short[i], sizeof(m_pending_short[i])))
                {
                    return true;
                }
            }

            break;

        case SRC_ADDR_TYPE_EXTENDED:
            for (i = 0; i < NUM_PENDING_EXTENDED_ADDRESSES; i++)
            {
                if (nrf_radio_state_get() != NRF_RADIO_STATE_TX_RU)
                {
                    break;
                }

                if (0 == memcmp(p_src_addr, m_pending_extended[i], sizeof(m_pending_extended[i])))
                {
                    return true;
                }
            }

            break;

        default:
            return true;
    }

    return false;
}

