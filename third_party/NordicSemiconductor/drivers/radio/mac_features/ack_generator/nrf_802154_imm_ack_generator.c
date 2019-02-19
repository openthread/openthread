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
 *   This file implements an immediate acknowledgement (Imm-Ack) generator for 802.15.4 radio driver.
 *
 */

#include "nrf_802154_imm_ack_generator.h"

#include <assert.h>
#include <string.h>

#include "nrf_802154_ack_pending_bit.h"
#include "nrf_802154_const.h"

#define IMM_ACK_INITIALIZER {0x05, ACK_HEADER_WITH_PENDING, 0x00, 0x00, 0x00, 0x00}

static uint8_t m_ack_psdu[IMM_ACK_LENGTH + PHR_SIZE];

void nrf_802154_imm_ack_generator_init(void)
{
    const uint8_t ack_psdu[] = IMM_ACK_INITIALIZER;

    memcpy(m_ack_psdu, ack_psdu, sizeof(ack_psdu));
}

const uint8_t * nrf_802154_imm_ack_generator_create(const uint8_t * p_frame)
{
    // Set valid sequence number in ACK frame.
    m_ack_psdu[DSN_OFFSET] = p_frame[DSN_OFFSET];

    // Set pending bit in ACK frame.
    if (nrf_802154_ack_pending_bit_should_be_set(p_frame))
    {
        m_ack_psdu[FRAME_PENDING_OFFSET] = ACK_HEADER_WITH_PENDING;
    }
    else
    {
        m_ack_psdu[FRAME_PENDING_OFFSET] = ACK_HEADER_WITHOUT_PENDING;
    }

    return m_ack_psdu;
}
