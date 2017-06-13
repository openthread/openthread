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
 * @brief This module contains calculations of 802.15.4 radio driver procedures duration.
 *
 */

#ifndef NRF_DRV_RADIO802154_PROCEDURES_DURATION_H_
#define NRF_DRV_RADIO802154_PROCEDURES_DURATION_H_

#include <stdbool.h>
#include <stdint.h>

#define PHY_US_PER_SYMBOL     16
#define PHY_SYMBOLS_PER_OCTET  2
#define PHY_SHR_DURATION      10

#define A_CCA_DURATION         8
#define A_TURNAROUND_TIME     12
#define A_UNIT_BACKOFF_PERIOD 20

#define NUM_OCTETS_IN_ACK     6

#define MAC_ACK_WAIT_DURATION (A_UNIT_BACKOFF_PERIOD +                                            \
                               A_TURNAROUND_TIME +                                                \
                               PHY_SHR_DURATION +                                                 \
                               (NUM_OCTETS_IN_ACK * PHY_SYMBOLS_PER_OCTET))

static inline uint16_t nrf_drv_radio802154_tx_duration_get(uint8_t psdu_length, bool ack_requested)
{
    // aTurnaroundTime + CCA + aTurnaroundTime + SHR + PHR + PSDU
    // if ACK: + macAckWaitDuration
    uint16_t result = A_TURNAROUND_TIME + A_CCA_DURATION + A_TURNAROUND_TIME + PHY_SHR_DURATION;
    result += (psdu_length + 1) * PHY_SYMBOLS_PER_OCTET;

    if (ack_requested)
    {
        result += MAC_ACK_WAIT_DURATION;
    }

    result *= PHY_US_PER_SYMBOL;

    return result;
}

static inline uint16_t nrf_drv_radio802154_rx_duration_get(uint8_t psdu_length, bool ack_requested)
{
    // SHR + PHR + PSDU
    // if ACK: + aTurnaroundTime + ACK frame duration + aTurnaroundTime
    uint16_t result = PHY_SHR_DURATION + (psdu_length + 1) * PHY_SYMBOLS_PER_OCTET;

    if (ack_requested)
    {
        result += A_TURNAROUND_TIME +
                  PHY_SHR_DURATION +
                  (NUM_OCTETS_IN_ACK * PHY_SYMBOLS_PER_OCTET) +
                  A_TURNAROUND_TIME;
    }

    result *= PHY_US_PER_SYMBOL;

    return result;
}

#endif /* NRF_DRV_RADIO802154_PROCEDURES_DURATION_H_ */
