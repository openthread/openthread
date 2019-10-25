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
 * @brief Module that contains debug helpers for the 802.15.4 radio driver for the nRF SoC devices.
 *
 */

#ifndef NRF_802154_DEBUG_H_
#define NRF_802154_DEBUG_H_

#include <stdint.h>

#include "nrf_802154_debug_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_SET_STATE             0x0005UL
#define EVENT_RADIO_RESET           0x0006UL

#define FUNCTION_SLEEP              0x0001UL
#define FUNCTION_RECEIVE            0x0002UL
#define FUNCTION_TRANSMIT           0x0003UL
#define FUNCTION_ENERGY_DETECTION   0x0004UL
#define FUNCTION_BUFFER_FREE        0x0005UL
#define FUNCTION_CCA                0x0006UL
#define FUNCTION_CONTINUOUS_CARRIER 0x0007UL
#define FUNCTION_CSMACA             0x0008UL
#define FUNCTION_TRANSMIT_AT        0x0009UL
#define FUNCTION_RECEIVE_AT         0x000AUL
#define FUNCTION_TRANSMIT_AT_CANCEL 0x000BUL
#define FUNCTION_RECEIVE_AT_CANCEL  0x000CUL

#define FUNCTION_IRQ_HANDLER        0x0100UL
#define FUNCTION_EVENT_FRAMESTART   0x0101UL
#define FUNCTION_EVENT_BCMATCH      0x0102UL
#define FUNCTION_EVENT_END          0x0103UL
#define FUNCTION_EVENT_DISABLED     0x0104UL
#define FUNCTION_EVENT_READY        0x0105UL
#define FUNCTION_EVENT_CCAIDLE      0x0106UL
#define FUNCTION_EVENT_CCABUSY      0x0107UL
#define FUNCTION_EVENT_EDEND        0x0108UL
#define FUNCTION_EVENT_PHYEND       0x0109UL
#define FUNCTION_EVENT_CRCOK        0x010AUL
#define FUNCTION_EVENT_CRCERROR     0x010BUL

#define FUNCTION_AUTO_ACK_ABORT     0x0201UL
#define FUNCTION_TIMESLOT_STARTED   0x0202UL
#define FUNCTION_TIMESLOT_ENDED     0x0203UL
#define FUNCTION_CRIT_SECT_ENTER    0x0204UL
#define FUNCTION_CRIT_SECT_EXIT     0x0205UL

/* Reserved for RAAL: 0x0300 - 0x047F */

#define FUNCTION_RSCH_CONTINUOUS_ENTER             0x0480UL
#define FUNCTION_RSCH_CONTINUOUS_EXIT              0x0481UL
#define FUNCTION_RSCH_CRITICAL_SECTION_ENTER       0x0482UL
#define FUNCTION_RSCH_CRITICAL_SECTION_EXIT        0x0483UL
#define FUNCTION_RSCH_TIMESLOT_STARTED             0x0484UL
#define FUNCTION_RSCH_TIMESLOT_ENDED               0x0485UL
#define FUNCTION_RSCH_DELAYED_TIMESLOT_REQ         0x048BUL
#define FUNCTION_RSCH_TIMER_DELAYED_PREC           0x048CUL
#define FUNCTION_RSCH_TIMER_DELAYED_START          0x048DUL
#define FUNCTION_RSCH_DELAYED_TIMESLOT_CANCEL      0x048EUL

#define FUNCTION_CSMA_ABORT                        0x0500UL
#define FUNCTION_CSMA_TX_FAILED                    0x0501UL
#define FUNCTION_CSMA_TX_STARTED                   0x0502UL
#define FUNCTION_CSMA_CHANNEL_BUSY                 0x0503UL
#define FUNCTION_CSMA_FRAME_TRANSMIT               0x0504UL

#define FUNCTION_TSCH_ADD                          0x0600UL
#define FUNCTION_TSCH_FIRED                        0x0601UL

#define FUNCTION_TCOOR_START                       0x0700UL
#define FUNCTION_TCOOR_STOP                        0x0701UL
#define FUNCTION_TCOOR_TIMESTAMP_PREPARE           0x0702UL
#define FUNCTION_TCOOR_TIMESTAMP_GET               0x0703UL
#define FUNCTION_TCOOR_SYNCHRONIZED                0x0704UL

#define FUNCTION_DTRX_RX_TIMEOUT                   0x0800UL

#define FUNCTION_ACK_TIMEOUT_FIRED                 0x0900UL

#define FUNCTION_mutex_trylock                     0x1000UL
#define FUNCTION_mutex_unlock                      0x1001UL
#define FUNCTION_max_prio_for_delayed_timeslot_get 0x1002UL
#define FUNCTION_required_prio_lvl_get             0x1003UL
#define FUNCTION_prec_approved_prio_set            0x1004UL
#define FUNCTION_all_prec_update                   0x1005UL
#define FUNCTION_approved_prio_lvl_get             0x1006UL
#define FUNCTION_requested_prio_lvl_is_at_least    0x1007UL
#define FUNCTION_notify_core                       0x1008UL

#ifdef NRF52811_XXAA

#define PIN_DBG_RADIO_EVT_END        13
#define PIN_DBG_RADIO_EVT_DISABLED   14
#define PIN_DBG_RADIO_EVT_READY      17
#define PIN_DBG_RADIO_EVT_FRAMESTART 18
#define PIN_DBG_RADIO_EVT_EDEND      25
#define PIN_DBG_RADIO_EVT_PHYEND     24

#else

#define PIN_DBG_RADIO_EVT_END        11
#define PIN_DBG_RADIO_EVT_DISABLED   12
#define PIN_DBG_RADIO_EVT_READY      13
#define PIN_DBG_RADIO_EVT_FRAMESTART 14
#define PIN_DBG_RADIO_EVT_EDEND      25
#define PIN_DBG_RADIO_EVT_PHYEND     24

#endif

#define PPI_DBG_RADIO_EVT_END           0
#define PPI_DBG_RADIO_EVT_DISABLED      1
#define PPI_DBG_RADIO_EVT_READY         2
#define PPI_DBG_RADIO_EVT_FRAMESTART    3
#define PPI_DBG_RADIO_EVT_EDEND         4

#define GPIOTE_DBG_RADIO_EVT_END        0
#define GPIOTE_DBG_RADIO_EVT_DISABLED   1
#define GPIOTE_DBG_RADIO_EVT_READY      2
#define GPIOTE_DBG_RADIO_EVT_FRAMESTART 3
#define GPIOTE_DBG_RADIO_EVT_EDEND      4

#if ENABLE_DEBUG_GPIO

#define NRF_802154_DEBUG_PINS_USED_MASK            ((1 << PIN_DBG_RADIO_EVT_END) |        \
                                                    (1 << PIN_DBG_RADIO_EVT_DISABLED) |   \
                                                    (1 << PIN_DBG_RADIO_EVT_READY) |      \
                                                    (1 << PIN_DBG_RADIO_EVT_FRAMESTART) | \
                                                    (1 << PIN_DBG_RADIO_EVT_EDEND) |      \
                                                    (1 << PIN_DBG_RADIO_EVT_PHYEND))

#define NRF_802154_DEBUG_PPI_CHANNELS_USED_MASK    ((1 << PPI_DBG_RADIO_EVT_END) |        \
                                                    (1 << PPI_DBG_RADIO_EVT_DISABLED) |   \
                                                    (1 << PPI_DBG_RADIO_EVT_READY) |      \
                                                    (1 << PPI_DBG_RADIO_EVT_FRAMESTART) | \
                                                    (1 << PPI_DBG_RADIO_EVT_EDEND) |      \
                                                    (1 << PPI_DBG_RADIO_EVT_PHYEND))

#define NRF_802154_DEBUG_GPIOTE_CHANNELS_USED_MASK ((1 << GPIOTE_DBG_RADIO_EVT_END) |        \
                                                    (1 << GPIOTE_DBG_RADIO_EVT_DISABLED) |   \
                                                    (1 << GPIOTE_DBG_RADIO_EVT_READY) |      \
                                                    (1 << GPIOTE_DBG_RADIO_EVT_FRAMESTART) | \
                                                    (1 << GPIOTE_DBG_RADIO_EVT_EDEND) |      \
                                                    (1 << GPIOTE_DBG_RADIO_EVT_PHYEND))

#else // ENABLE_DEBUG_GPIO

#define NRF_802154_DEBUG_PINS_USED_MASK            0
#define NRF_802154_DEBUG_PPI_CHANNELS_USED_MASK    0
#define NRF_802154_DEBUG_GPIOTE_CHANNELS_USED_MASK 0

#endif // ENABLE_DEBUG_GPIO

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_DEBUG_H_ */
