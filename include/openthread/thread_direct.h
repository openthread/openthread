/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file defines the OpenThread API for Thread Direct,
 *   a MAC-layer peer-to-peer link between Thread devices.
 */

#ifndef OPENTHREAD_THREAD_DIRECT_H_
#define OPENTHREAD_THREAD_DIRECT_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-direct
 *
 * @brief
 *   This module includes functions for Thread Direct.
 *
 * @{
 *
 */

/**
 * Represents a 16-byte guest Wake Key provisioned out-of-band (key indices 130-192).
 */
typedef struct otThreadDirectWakeKey
{
    uint8_t m8[16]; ///< Key material (16 bytes).
} otThreadDirectWakeKey;

/**
 * Represents the state of an established Thread Direct peer.
 *
 * The @p mWake* fields are populated on OT_THREAD_DIRECT_EVENT_WAKE_RECEIVED;
 * the @p mSlw* and @p mTd* fields are populated on LINKED/UNLINKED events.
 */
typedef struct otThreadDirectPeerInfo
{
    otExtAddress mExtAddress;            ///< Peer extended address.
    uint16_t     mTdShortAddress;        ///< TD short address [0xFE00, 0xFFFE], or OT_RADIO_INVALID_SHORT_ADDR.
    uint16_t     mSlwPeriodSlots;        ///< Peer's SLW period in 160 us slots (0 = not configured).
    uint16_t     mSlwPhaseSlots;         ///< Peer's SLW phase in 160 us slots.
    uint16_t     mSupervisionIntervalMs; ///< Supervision interval from TD Link Command, in milliseconds.
    uint8_t      mServicesBitmap;        ///< Services bitmap: bit 0 = peer has SRP server.

    /* Fields below are valid only for OT_THREAD_DIRECT_EVENT_WAKE_RECEIVED. */
    uint8_t  mWakeType;          ///< Wake type: 0=link, 1=power-outage, 2=connectionless.
    uint32_t mWakeRvTimeUs;      ///< Rendezvous time (us from end-of-frame to connection window open).
    uint8_t  mWakeRetryCount;    ///< Retry count from the Wake Frame (number of WI retries).
    uint8_t  mWakeRetryInterval; ///< Retry interval (units of WI wake interval).
} otThreadDirectPeerInfo;

/**
 * Represents a Thread Direct link event.
 */
typedef enum otThreadDirectEvent
{
    OT_THREAD_DIRECT_EVENT_LINK_FAILED   = 0, ///< Wake attempt failed (timeout or authentication failure).
    OT_THREAD_DIRECT_EVENT_WAKE_RECEIVED = 1, ///< WL received a TD Wake Command from a WI peer.
} otThreadDirectEvent;

/**
 * Pointer to a function called on Thread Direct link events.
 *
 * @param[in] aEvent     The event type.
 * @param[in] aPeerInfo  Peer information at the time of the event.  May be NULL for LINK_FAILED
 *                       if no peer entry was established.
 * @param[in] aContext   Application-specific context pointer passed to
 *                       otThreadDirectSetEventCallback().
 */
typedef void (*otThreadDirectEventCallback)(otThreadDirectEvent           aEvent,
                                            const otThreadDirectPeerInfo *aPeerInfo,
                                            void                         *aContext);

/**
 * Registers a callback for Thread Direct link events.
 *
 * Valid for both WI and WL roles.  A subsequent call replaces any previously
 * registered callback.  Pass NULL to clear.
 *
 * Requires `OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE` or
 * `OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE`.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aCallback  The callback function pointer, or NULL to clear.
 * @param[in] aContext   Application-specific context pointer passed to @p aCallback.
 */
void otThreadDirectSetEventCallback(otInstance *aInstance, otThreadDirectEventCallback aCallback, void *aContext);

/**
 * Thread Direct wake frame type, carried in the Wake Frame payload.
 *
 * The numeric values match the wire encoding.
 */
typedef enum
{
    OT_THREAD_DIRECT_WAKE_TYPE_LINK           = 0, ///< Wake Frame requesting TD link establishment.
    OT_THREAD_DIRECT_WAKE_TYPE_POWER_OUTAGE   = 1, ///< Wake Frame signaling a power outage event.
    OT_THREAD_DIRECT_WAKE_TYPE_CONNECTIONLESS = 2, ///< Connectionless Wake Frame (no link establishment).
} otThreadDirectWakeType;

/**
 * Starts a Thread Direct wake burst targeting @p aExtAddress.
 *
 * Transmits Wake Frames of type @p aWakeType at @p aIntervalUs for @p aDurationMs.
 * When @p aWakeType is OT_THREAD_DIRECT_WAKE_TYPE_LINK, a connection window is opened
 * after the burst; the registered event callback (see otThreadDirectSetEventCallback())
 * fires OT_THREAD_DIRECT_EVENT_LINK_FAILED if the window expires without a response.
 *
 * Pass @p aIntervalUs = 0 to use OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INTERVAL_US.
 * Pass @p aDurationMs = 0 to use OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_DURATION_MS.
 *
 * Wake Frames are secured with the key identified by @p aKeyIndex.  Pass 0 or 129 for
 * the network-derived Wake Key (HMAC-SHA256(NetworkKey, "Thread-Wake")).  Pass an index
 * in [130, 192] to use a guest Wake Key previously provisioned via
 * otThreadDirectSetGuestWakeKey().
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aExtAddress  Extended address of the Wake Listener.
 * @param[in] aWakeType    Wake frame type.
 * @param[in] aIntervalUs  Inter-frame interval in us (0 = default).
 * @param[in] aDurationMs  Wake burst duration in ms (0 = default).
 * @param[in] aKeyIndex    Key index: 0 or 129 for the network-derived Wake Key;
 *                         [130, 192] to use a provisioned guest Wake Key.
 *
 * @retval OT_ERROR_NONE             Wake burst started.
 * @retval OT_ERROR_INVALID_ARGS     @p aExtAddress is NULL, or @p aIntervalUs, @p aDurationMs,
 *                                   or @p aKeyIndex is out of range.
 * @retval OT_ERROR_INVALID_STATE    A wake burst is already in progress, or @p aKeyIndex is a
 *                                   guest index for which no key has been provisioned.
 *
 * Requires `OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE`.
 */
otError otThreadDirectWakeup(otInstance            *aInstance,
                             const otExtAddress    *aExtAddress,
                             otThreadDirectWakeType aWakeType,
                             uint16_t               aIntervalUs,
                             uint16_t               aDurationMs,
                             uint8_t                aKeyIndex);

/**
 * Initiates Thread Direct link teardown to the peer identified by @p aExtAddress.
 * Sends a TD frame carrying an empty SCA LTV (SCA teardown).
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aExtAddress  Extended address of the peer to unlink.
 *
 * @retval OT_ERROR_NONE          Teardown initiated.
 * @retval OT_ERROR_NOT_FOUND     No established link to @p aExtAddress.
 * @retval OT_ERROR_NOT_IMPLEMENTED  Feature is not implemented.
 */
otError otThreadDirectUnlink(otInstance *aInstance, const otExtAddress *aExtAddress);

/**
 * Enables or disables Wake Listener mode.
 *
 * When enabled the WL periodically calls otPlatRadioReceiveAt() on Wake Channel 20
 * according to the configured listen interval and duration.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aEnable    TRUE to enable WL listen mode; FALSE to disable.
 *
 * @retval OT_ERROR_NONE  Mode updated.
 *
 * Requires `OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE`.
 */
otError otThreadDirectWakeListenerEnable(otInstance *aInstance, bool aEnable);

/**
 * Returns whether WL wake channel listening is currently active.
 *
 * Requires `OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE`.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @returns TRUE if the WL is listening for TD Wake Commands, FALSE otherwise.
 */
bool otThreadDirectIsWakeListenerEnabled(otInstance *aInstance);

/**
 * Returns whether a WI wake burst is currently in progress.
 *
 * TRUE covers both the active TX burst phase and the connection window that
 * immediately follows, during which the WI waits for a TD Link Command from
 * the WL.
 *
 * Requires `OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE`.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @returns TRUE if a wake burst or connection window is active, FALSE otherwise.
 */
bool otThreadDirectIsWakeBurstActive(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_THREAD_DIRECT_H_
