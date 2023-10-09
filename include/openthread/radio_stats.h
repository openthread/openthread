/*
 *  Copyright (c) 2023, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *   This file includes the OpenThread API for radio stats module.
 */

#ifndef OPENTHREAD_RADIO_STATS_H_
#define OPENTHREAD_RADIO_STATS_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-radio
 *
 * @brief
 *   This module includes functions for radio statistics.
 *
 * @{
 *
 */

/**
 * Contains the statistics of radio.
 *
 */
typedef struct otRadioTimeStats
{
    uint64_t mDisabledTime; ///< The total time that radio is in disabled state, in unit of microseconds.
    uint64_t mSleepTime;    ///< The total time that radio is in sleep state, in unit of microseconds.
    uint64_t mTxTime;       ///> The total time that radio is doing transmission, in unit of microseconds.
    uint64_t mRxTime;       ///> The total time that radio is in receive state, in unit of microseconds.
} otRadioTimeStats;

/**
 * Gets the radio statistics.
 *
 * The radio statistics include the time when the radio is in TX/RX/Sleep state. These times are in units of
 * microseconds. All times are calculated from the last reset of radio statistics.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 *
 * @returns  A const pointer to the otRadioTimeStats struct that contains the data.
 *
 */
const otRadioTimeStats *otRadioTimeStatsGet(otInstance *aInstance);

/**
 * Resets the radio statistics.
 *
 * All times are reset to 0.
 *
 * @param[in]  aInstance    A pointer to an OpenThread instance.
 *
 *
 */
void otRadioTimeStatsReset(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_RADIO_STATS_H_
