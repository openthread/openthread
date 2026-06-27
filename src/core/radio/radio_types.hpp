/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for OpenThread radio types.
 */

#ifndef OT_CORE_RADIO_RADIO_TYPES_HPP_
#define OT_CORE_RADIO_RADIO_TYPES_HPP_

#include "openthread-core-config.h"

#include <openthread/platform/radio.h>

namespace ot {

#ifdef OT_CONFIG_RADIO_TIME_ENABLE
#error "OT_CONFIG_RADIO_TIME_ENABLE MUST NOT be defined directly. It is derived from other configs"
#endif

#define OT_CONFIG_RADIO_TIME_ENABLE                                                               \
    (OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE || \
     OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE || OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || \
     OPENTHREAD_CONFIG_TIME_SYNC_ENABLE)

/**
 * Represents a 64-bit radio time in microseconds referenced to a continuous monotonic local radio clock.
 */
typedef otRadioTime64 RadioTime64;

/**
 * Represents a 32-bit radio time in microseconds (holds the lower 32 bits of a `RadioTime64`).
 */
typedef otRadioTime32 RadioTime32;

/**
 * Converts a 64-bit radio time to a 32-bit radio time.
 *
 * @param[in] aRadioTime64  The 64-bit radio time to convert.
 *
 * @returns The converted 32-bit radio time (lower 32 bits of @p aRadioTime64).
 */
inline RadioTime32 ConvertRadioTime64To32(RadioTime64 aRadioTime64) { return static_cast<RadioTime32>(aRadioTime64); }

/**
 * Indicates whether a given 32-bit radio time is strictly before another 32-bit radio time.
 *
 * This function correctly accounts for 32-bit microsecond counter roll-over.
 *
 * @param[in] aFirstTime   The first 32-bit radio time to compare.
 * @param[in] aSecondTime  The second 32-bit radio time to compare.
 *
 * @retval TRUE   @p aFirstTime is strictly before @p aSecondTime.
 * @retval FALSE  @p aFirstTime is not strictly before @p aSecondTime.
 */
bool IsRadioTimeStrictlyBefore(RadioTime32 aFirstTime, RadioTime32 aSecondTime);

} // namespace ot

#endif // OT_CORE_RADIO_RADIO_TYPES_HPP_
