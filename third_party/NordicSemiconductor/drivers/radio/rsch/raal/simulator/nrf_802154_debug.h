/* Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
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

#define EVENT_TIMESLOT_REQUEST        0x0007UL
#define EVENT_TIMESLOT_REQUEST_RESULT 0x0008UL

/* Reserved for RAAL: 0x0300 - 0x047F */

#define FUNCTION_RAAL_CRIT_SECT_ENTER          0x0301UL
#define FUNCTION_RAAL_CRIT_SECT_EXIT           0x0302UL
#define FUNCTION_RAAL_CONTINUOUS_ENTER         0x0303UL
#define FUNCTION_RAAL_CONTINUOUS_EXIT          0x0304UL

#define FUNCTION_RAAL_SIG_HANDLER              0x0400UL
#define FUNCTION_RAAL_SIG_EVENT_START          0x0401UL
#define FUNCTION_RAAL_SIG_EVENT_MARGIN         0x0402UL
#define FUNCTION_RAAL_SIG_EVENT_EXTEND         0x0403UL
#define FUNCTION_RAAL_SIG_EVENT_ENDED          0x0404UL
#define FUNCTION_RAAL_SIG_EVENT_RADIO          0x0405UL
#define FUNCTION_RAAL_SIG_EVENT_EXTEND_SUCCESS 0x0406UL
#define FUNCTION_RAAL_SIG_EVENT_EXTEND_FAIL    0x0407UL
#define FUNCTION_RAAL_EVT_BLOCKED              0x0408UL
#define FUNCTION_RAAL_EVT_SESSION_IDLE         0x0409UL
#define FUNCTION_RAAL_EVT_HFCLK_READY          0x040AUL
#define FUNCTION_RAAL_SIG_EVENT_MARGIN_MOVE    0x040BUL

#ifdef __cplusplus
}
#endif

#endif /* NRF_802154_DEBUG_H_ */
