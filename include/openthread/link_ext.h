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
 * @brief
 *   This file defines the OpenThread IEEE 802.15.4 Link Layer extension API, which provides link-layer mechanisms
 *   built on top of the raw link-layer (see `link_raw.h`).
 */

#ifndef OPENTHREAD_LINK_EXT_H_
#define OPENTHREAD_LINK_EXT_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-link-ext
 *
 * @brief
 *   This module includes functions for link-layer extensions built on top of the raw link-layer.
 *
 * @{
 */

/**
 * Represents the configuration of a burst transmission.
 */
typedef struct otLinkExtBurstConfig
{
    uint16_t mCount;       ///< Number of burst ticks (remaining ticks, including the current one, when read).
    uint32_t mPeriodUs;    ///< Tick period in microseconds (0 for back-to-back ticks).
    uint32_t mChannelMask; ///< Channels (bit N for channel N) to transmit on in each tick (0 for the frame channel).
} otLinkExtBurstConfig;

/**
 * Pointer on receipt of the end of a burst transmission.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
typedef void (*otLinkExtBurstDone)(otInstance *aInstance);

/**
 * Begins a burst transmission of the frame in the transmit buffer.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only).
 *
 * The frame is transmitted unchanged in each of `mCount` ticks. In each tick, the frame is transmitted on every
 * channel in `mChannelMask` in ascending order (or on the frame channel if `mChannelMask` is 0), back-to-back. The
 * frame MUST NOT request an ACK and MUST NOT require security processing. This function MUST NOT be called while a
 * transmission requested by `otLinkRawTransmit()` is ongoing.
 *
 * When `mPeriodUs` is non-zero, ticks start every `mPeriodUs` on the radio clock, from the frame's target TX time if
 * specified, or from now. A timed tick is handed to the radio `OPENTHREAD_CONFIG_LINK_EXT_BURST_TX_AHEAD_US` before
 * its target TX time. Only the first channel of a tick uses the target TX time; the other channels are sent
 * right after it. A tick whose start time has passed (e.g. overrun by the previous tick) is skipped. The frame's
 * maximum CSMA backoffs and maximum frame retries SHOULD be 0 for a timed burst, since any CSMA-CA backoff or
 * retransmission would be late.
 *
 * Between ticks, the radio is in the state last requested by `otLinkRawReceive()` (on the channel set by
 * `otLinkSetChannel()`) or `otLinkRawSleep()`, which do not end the burst. The frame's RX channel after TX done is
 * ignored.
 *
 * The transmit buffer MUST NOT be modified, and `otLinkRawTransmit()` and `otLinkRawEnergyScan()` fail with
 * `OT_ERROR_INVALID_STATE`, until the burst ends. The burst ends after its last tick, when stopped by
 * `otLinkExtStopBurst()`, or when the raw link-layer is disabled. @p aCallback is then called, except when
 * `otLinkExtStopBurst()` ends the burst right away.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aConfig    A pointer to the burst configuration.
 * @param[in]  aCallback  A pointer to a function called when the burst ends.
 *
 * @retval OT_ERROR_NONE           Successfully started the burst.
 * @retval OT_ERROR_INVALID_ARGS   The configuration or the transmit frame is invalid for a burst, or all its ticks
 *                                 are already in the past.
 * @retval OT_ERROR_INVALID_STATE  The raw link-layer isn't enabled.
 * @retval OT_ERROR_BUSY           A burst transmission is ongoing.
 */
otError otLinkExtTransmitBurst(otInstance                 *aInstance,
                               const otLinkExtBurstConfig *aConfig,
                               otLinkExtBurstDone          aCallback);

/**
 * Stops the ongoing burst transmission.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only).
 *
 * If no burst frame is with the radio, the burst ends right away, and the callback of `otLinkExtTransmitBurst()` is
 * not called (`otLinkExtGetBurstConfig()` reads `mCount` as 0). Otherwise, the frame being transmitted cannot be
 * taken back, and the burst ends when the radio is done with it, which is reported by the callback (until then,
 * `otLinkExtGetBurstConfig()` reads `mCount` as 1).
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE           The burst is stopped or will stop, or no burst is ongoing.
 * @retval OT_ERROR_INVALID_STATE  The raw link-layer isn't enabled.
 */
otError otLinkExtStopBurst(otInstance *aInstance);

/**
 * Gets the configuration of the ongoing burst transmission.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only).
 *
 * `mCount` is the number of remaining ticks, including the one in progress. All fields are zero if no burst is
 * ongoing.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[out] aConfig    A pointer to return the configuration.
 */
void otLinkExtGetBurstConfig(otInstance *aInstance, otLinkExtBurstConfig *aConfig);

/**
 * Represents a periodic receive schedule.
 *
 * A receive window starts at every radio time `T` (see `otPlatRadioGetNow()`) that satisfies
 * `T % mPeriodUs == mPhaseUs` and lasts `mDurationUs` microseconds.
 */
typedef struct otLinkExtPeriodicRxSchedule
{
    uint32_t mPhaseUs;    ///< Phase of the window start within the period (in usec), less than `mPeriodUs`.
    uint32_t mPeriodUs;   ///< Period (in usec), non-zero.
    uint16_t mDurationUs; ///< Window duration (in usec), non-zero and not more than `mPeriodUs`.
    uint8_t  mChannel;    ///< Channel to listen on, in the radio's supported channel mask.
} otLinkExtPeriodicRxSchedule;

/**
 * Sets the periodic receive schedules.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only) and a non-zero
 * `OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES`.
 *
 * Atomically replaces all schedules. While the radio is in the Sleep state, receive windows are opened according to
 * the schedules using `otPlatRadioReceiveAt()`. Receive, transmit and energy scan take precedence, and periodic RX
 * resumes when the radio returns to Sleep. Array order defines priority (index 0 has the highest priority) between
 * overlapping windows. Passing zero schedules disables periodic RX. A window already scheduled on the radio may still
 * occur once after the schedules are changed.
 *
 * @param[in] aInstance      A pointer to an OpenThread instance.
 * @param[in] aSchedules     A pointer to an array of schedules (can be NULL if @p aNumSchedules is zero).
 * @param[in] aNumSchedules  The number of schedules in @p aSchedules.
 *
 * @retval OT_ERROR_NONE          Successfully set the schedules.
 * @retval OT_ERROR_NO_BUFS       More than `OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES` schedules. The
 *                                schedules are left unchanged.
 * @retval OT_ERROR_INVALID_ARGS  A schedule is invalid. The schedules are left unchanged.
 */
otError otLinkExtSetPeriodicRxSchedules(otInstance                        *aInstance,
                                        const otLinkExtPeriodicRxSchedule *aSchedules,
                                        uint8_t                            aNumSchedules);

/**
 * Gets the periodic receive schedules.
 *
 * Requires `OPENTHREAD_CONFIG_LINK_EXT_ENABLE` (RCP builds only) and a non-zero
 * `OPENTHREAD_CONFIG_LINK_EXT_PERIODIC_RX_MAX_SCHEDULES`.
 *
 * @param[in]  aInstance      A pointer to an OpenThread instance.
 * @param[out] aNumSchedules  A pointer to return the number of schedules.
 *
 * @returns A pointer to the array of schedules.
 */
const otLinkExtPeriodicRxSchedule *otLinkExtGetPeriodicRxSchedules(otInstance *aInstance, uint8_t *aNumSchedules);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_LINK_EXT_H_
