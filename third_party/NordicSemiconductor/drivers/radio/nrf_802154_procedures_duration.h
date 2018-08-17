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
 * @brief This module contains calculations of 802.15.4 radio driver procedures duration.
 *
 */

#ifndef NRF_802154_PROCEDURES_DURATION_H_
#define NRF_802154_PROCEDURES_DURATION_H_

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"

#include "nrf_802154_const.h"

#define TX_RAMP_UP_TIME       40  // us
#define RX_RAMP_UP_TIME       40  // us
#define RX_RAMP_DOWN_TIME      0  // us
#define MAX_RAMP_DOWN_TIME     6  // us
#define RX_TX_TURNAROUND_TIME 20  // us

#define A_CCA_DURATION         8  // sym
#define A_TURNAROUND_TIME     12  // sym
#define A_UNIT_BACKOFF_PERIOD 20  // sym

#define NUM_OCTETS_IN_ACK      6  // bytes

#define MAC_ACK_WAIT_DURATION (A_UNIT_BACKOFF_PERIOD +                                            \
                               A_TURNAROUND_TIME +                                                \
                               PHY_SHR_DURATION +                                                 \
                               (NUM_OCTETS_IN_ACK * PHY_SYMBOLS_PER_OCTET))

__STATIC_INLINE uint16_t nrf_802154_tx_duration_get(uint8_t psdu_length,
                                                    bool    cca,
                                                    bool    ack_requested);

__STATIC_INLINE uint16_t nrf_802154_cca_before_tx_duration_get(void);

__STATIC_INLINE uint16_t nrf_802154_rx_duration_get(uint8_t psdu_length, bool ack_requested);

__STATIC_INLINE uint16_t nrf_802154_cca_duration_get(void);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE uint16_t nrf_802154_tx_duration_get(uint8_t psdu_length,
                                                    bool    cca,
                                                    bool    ack_requested)
{
    // ramp down
    // if CCA: + RX ramp up + CCA + RX ramp down
    // + TX ramp up + SHR + PHR + PSDU
    // if ACK: + macAckWaitDuration
    uint16_t result = PHY_SHR_DURATION + (psdu_length + 1) * PHY_SYMBOLS_PER_OCTET;

    if (ack_requested)
    {
        result += MAC_ACK_WAIT_DURATION;
    }

    result *= PHY_US_PER_SYMBOL;

    result += MAX_RAMP_DOWN_TIME + TX_RAMP_UP_TIME;

    if (cca)
    {
        result += RX_RAMP_UP_TIME + (A_CCA_DURATION * PHY_US_PER_SYMBOL) + RX_RAMP_DOWN_TIME;
    }

    return result;
}

__STATIC_INLINE uint16_t nrf_802154_cca_before_tx_duration_get(void)
{
    // CCA + turnaround time
    uint16_t result = (A_CCA_DURATION * PHY_US_PER_SYMBOL) + RX_TX_TURNAROUND_TIME;

    return result;
}

__STATIC_INLINE uint16_t nrf_802154_rx_duration_get(uint8_t psdu_length, bool ack_requested)
{
    // SHR + PHR + PSDU
    // if ACK: + aTurnaroundTime + ACK frame duration
    uint16_t result = PHY_SHR_DURATION + ((psdu_length + 1) * PHY_SYMBOLS_PER_OCTET);

    if (ack_requested)
    {
        result += A_TURNAROUND_TIME +
                  PHY_SHR_DURATION +
                  (NUM_OCTETS_IN_ACK * PHY_SYMBOLS_PER_OCTET);
    }

    result *= PHY_US_PER_SYMBOL;

    return result;
}

__STATIC_INLINE uint16_t nrf_802154_cca_duration_get(void)
{
    // ramp down + rx ramp up + CCA
    uint16_t result = MAX_RAMP_DOWN_TIME + RX_RAMP_UP_TIME + (A_CCA_DURATION * PHY_US_PER_SYMBOL);

    return result;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

#endif /* NRF_802154_PROCEDURES_DURATION_H_ */
