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
 *  This file defines the OpenThread Thread Direct Listener APIs.
 */

#ifndef OPENTHREAD_THREAD_DIRECT_LISTENER_H_
#define OPENTHREAD_THREAD_DIRECT_LISTENER_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/thread_direct.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-direct-listener
 *
 * @brief
 *   This module includes functions and definitions for Thread Direct (TD) Wake Listener.
 *
 *   The APIs in this module require Thread Direct Wake Listener functionality to be enabled via
 *   `OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE`.
 *
 * @note
 *   The Thread Direct Wake Listener APIs are provisional and work in progress. Not all functions are fully
 *   implemented yet, and the API definitions may change in future revisions.
 *
 * @{
 */

#define OT_THREAD_DIRECT_LISTENER_CHANNEL 20       ///< Default radio channel for Thread Direct Wake Listener.
#define OT_THREAD_DIRECT_LISTENER_INTERVAL 1000000 ///< Default listen interval in microseconds (1 second).
#define OT_THREAD_DIRECT_LISTENER_DURATION 8000    ///< Default listen duration in microseconds (8 milliseconds).

/**
 * Pointer to a function that is called when a valid Thread Direct Wake frame is received by the Thread Direct Wake
 * Listener.
 *
 * Inside this callback (or afterward while the listener remains running), the caller can call
 * `otThreadDirectListenerGetWakeInfo()` to retrieve details of the received Wake frame, and
 * `otThreadDirectListenerEstablishLink()` to accept the wake and initiate a Thread Direct Link. The caller MAY
 * also call `otThreadDirectListenerStop()` from within this callback to stop the listener.
 *
 * @param[in] aContext  The user context pointer (`mWakeCallbackContext` in `otThreadDirectListenerConfig`).
 */
typedef void (*otThreadDirectListenerWakeCallback)(void *aContext);

/**
 * Represents the configuration parameters for starting the Thread Direct Wake Listener.
 *
 * To ensure forward compatibility as new fields may be added in future revisions, the caller MUST zero-initialize
 * (`memset` to zero) this structure before setting its fields.
 *
 * For `mChannel`, `mInterval`, and `mDuration`, setting a field to `0` selects its specification-mandated default
 * (`OT_THREAD_DIRECT_LISTENER_CHANNEL`, `OT_THREAD_DIRECT_LISTENER_INTERVAL`, and `OT_THREAD_DIRECT_LISTENER_DURATION`,
 * respectively). Specifying a value other than `0` or the specification-mandated default makes the operation
 * non-compliant with the specification and is intended for testing purposes only. Callers SHOULD either leave these
 * fields set to `0` or set them to the corresponding `OT_THREAD_DIRECT_LISTENER_*` constant.
 */
typedef struct otThreadDirectListenerConfig
{
    otThreadDirectListenerWakeCallback mWakeCallback;        ///< Callback invoked on Wake reception (can be `NULL`).
    void                              *mWakeCallbackContext; ///< Context pointer passed to `mWakeCallback`.
    uint8_t                            mChannel;             ///< Radio channel (`0` for default).
    uint32_t                           mInterval; ///< Listen interval in usec (`0` for default, >= `mDuration`).
    uint32_t                           mDuration; ///< Listen duration in usec (`0` for default).
} otThreadDirectListenerConfig;

/**
 * Represents information about a received Thread Direct Wake frame.
 */
typedef struct otThreadDirectListenerWakeInfo
{
    otExtAddress           mPeerAddress; ///< The Extended MAC Address of the Wake Initiator peer.
    otThreadDirectWakeType mWakeType;    ///< The Wake type specified in the Wake frame.
    otThreadDirectKey     *mKey;         ///< The registered `otThreadDirectKey` that authenticated the Wake frame.
    uint8_t                mLinkChannel; ///< The radio channel indicated for establishing the Thread Direct Link.
} otThreadDirectListenerWakeInfo;

/**
 * Starts the Thread Direct Wake Listener with the given configuration.
 *
 * When running, the listener schedules periodic receive windows during radio sleep intervals (when the radio is
 * enabled and sleeping). Other MAC and radio operations (such as continuous reception when RxOnWhenIdle is enabled,
 * active frame transmissions, scans, or CSL receive windows) take precedence over the listener's receive windows.
 *
 * The caller MUST zero-initialize (`memset` to zero) @p aConfig before populating its fields to ensure forward
 * compatibility with future revisions. OpenThread copies the contents of @p aConfig, so the structure does not need
 * to persist after this function returns.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aConfig    The listener configuration.
 *
 * @retval OT_ERROR_NONE          Successfully started the Thread Direct Wake Listener.
 * @retval OT_ERROR_INVALID_ARGS  The configuration parameters in @p aConfig are invalid.
 * @retval OT_ERROR_BUSY          The Thread Direct Wake Listener is already running.
 */
otError otThreadDirectListenerStart(otInstance *aInstance, const otThreadDirectListenerConfig *aConfig);

/**
 * Stops the Thread Direct Wake Listener if it is currently running.
 *
 * This function can be called at any time, including from within `otThreadDirectListenerWakeCallback`. If the
 * listener is not running, this function does nothing.
 *
 * @param[in] aInstance  The OpenThread instance.
 */
void otThreadDirectListenerStop(otInstance *aInstance);

/**
 * Indicates whether the Thread Direct Wake Listener is currently running.
 *
 * @param[in] aInstance  The OpenThread instance.
 *
 * @retval TRUE   The Thread Direct Wake Listener is currently running.
 * @retval FALSE  The Thread Direct Wake Listener is not running.
 */
bool otThreadDirectListenerIsRunning(otInstance *aInstance);

/**
 * Gets the information for the most recently received Wake frame on the Thread Direct Wake Listener.
 *
 * This function can be called from within `otThreadDirectListenerWakeCallback` or afterward while the listener
 * remains running. The `mKey` pointer in @p aWakeInfo remains valid as long as that key remains registered. If the
 * key is unregistered (via `otThreadDirectUnregisterKey()` or `otThreadDirectUnregisterAllKeys()`), the stored Wake
 * information is invalidated and subsequent calls to this function will return `OT_ERROR_NOT_FOUND` until a new Wake
 * frame is received.
 *
 * @param[in]  aInstance  The OpenThread instance.
 * @param[out] aWakeInfo  A pointer to an `otThreadDirectListenerWakeInfo` structure to populate.
 *
 * @retval OT_ERROR_NONE           Successfully retrieved the Wake frame information in @p aWakeInfo.
 * @retval OT_ERROR_INVALID_STATE  The listener is not running.
 * @retval OT_ERROR_NOT_FOUND      No valid Wake frame information is available.
 */
otError otThreadDirectListenerGetWakeInfo(otInstance *aInstance, otThreadDirectListenerWakeInfo *aWakeInfo);

/**
 * Accepts the most recently received Wake frame on the Thread Direct Wake Listener and initiates establishment of a
 * Thread Direct Link (`otThreadDirectLink`).
 *
 * @note Thread Direct Link establishment is preliminary and subject to update in future revisions.
 *
 * This function can be called from within `otThreadDirectListenerWakeCallback` or afterward while the listener
 * remains running, and acts on the latest Wake frame reported via `otThreadDirectListenerWakeCallback` (and
 * inspectable via `otThreadDirectListenerGetWakeInfo()`). This function returns `NULL` if the wake cannot be accepted
 * (for example, if the listener is not running, no Wake frame has been received yet, the latest Wake frame was already
 * accepted, the wake type is not `OT_THREAD_DIRECT_WAKE_TYPE_TD_LINK`, or a link could not be allocated/started).
 *
 * The caller MUST zero-initialize (`memset` to zero) @p aLinkConfig before populating its fields. OpenThread copies
 * @p aLinkConfig, so it does not need to persist after this call returns.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aLinkConfig  The configuration for the new Thread Direct Link.
 *
 * @returns A pointer to the newly created `otThreadDirectLink`, or `NULL` if the wake could not be accepted.
 */
otThreadDirectLink *otThreadDirectListenerEstablishLink(otInstance                     *aInstance,
                                                        const otThreadDirectLinkConfig *aLinkConfig);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_THREAD_DIRECT_LISTENER_H_
