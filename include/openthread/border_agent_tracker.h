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
 * @brief
 *  This file defines the OpenThread Border Agent Tracker APIs.
 */

#ifndef OPENTHREAD_BORDER_AGENT_TRACKER_H_
#define OPENTHREAD_BORDER_AGENT_TRACKER_H_

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-border-agent-tracker
 *
 * @brief
 *   This module includes APIs for the Border Agent Tracker.
 *
 * The Border Agent Tracker discovers and tracks Border Agents on the infrastructure link by browsing for the
 * `_meshcop._udp` mDNS service.
 *
 * @{
 */

/**
 * Represents an iterator to iterate through the discovered Border Agents.
 *
 * The fields in this struct are for OpenThread internal use only and MUST NOT be accessed or modified by the caller.
 *
 * An iterator MUST be initialized using `otBorderAgentTrackerInitIterator()` before it is used.
 */
typedef struct otBorderAgentTrackerIterator
{
    const void *mPtr;
    uint64_t    mData;
} otBorderAgentTrackerIterator;

/**
 * Represents information about a discovered Border Agent.
 *
 * To ensure consistent `mMsecSinceDiscovered` and `mMsecSinceLastChange` time calculations, the iterator's
 * initialization time is stored within the iterator when `otBorderAgentTrackerInitIterator()` is called. The time
 * values in this struct are calculated relative to the iterator's initialization time.
 */
typedef struct otBorderAgentTrackerAgentInfo
{
    const char         *mServiceName;         ///< The service name.
    const char         *mHostName;            ///< The host name. May be NULL if not known yet.
    uint16_t            mPort;                ///< The port number. Can be zero if not known yet.
    const uint8_t      *mTxtData;             ///< The TXT data. May be NULL if not known yet.
    uint16_t            mTxtDataLength;       ///< The TXT data length.
    const otIp6Address *mAddresses;           ///< Array of IPv6 addresses of the host. May be NULL if not known yet.
    uint16_t            mNumAddresses;        ///< Number of addresses in the `mAddresses` array.
    uint64_t            mMsecSinceDiscovered; ///< Milliseconds since the service was discovered.
    uint64_t            mMsecSinceLastChange; ///< Milliseconds since the last change  (port, TXT, or addresses).
} otBorderAgentTrackerAgentInfo;

/**
 * Enables or disables the Border Agent Tracker.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_TRACKER_ENABLE`.
 *
 * When enabled, the tracker browses for the `_meshcop._udp` mDNS service to discover and track Border Agents on
 * the infra-if network.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aEnable   TRUE to enable the Border Agent Tracker, FALSE to disable it.
 */
void otBorderAgentTrackerSetEnabled(otInstance *aInstance, bool aEnable);

/**
 * Indicates whether the Border Agent Tracker is running.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_TRACKER_ENABLE`.
 *
 * The tracker can be enabled by the user (via `otBorderAgentTrackerSetEnabled()`) or by the OpenThread stack
 * itself. The tracker is considered running if it is enabled by either entity AND the underlying DNS-SD (mDNS)
 * is ready. This means that `otBorderAgentTrackerIsRunning()` may not return `TRUE` immediately after a call
 * to `otBorderAgentTrackerSetEnabled(true)`.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   If the tracker is running.
 * @retval FALSE  If the tracker is not running.
 */
bool otBorderAgentTrackerIsRunning(otInstance *aInstance);

/**
 * Initializes a Border Agent Tracker iterator.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_TRACKER_ENABLE`.
 *
 * An iterator MUST be initialized before being used.
 *
 * @param[in] aInstance  A pointer to an OpenThread instance.
 * @param[in] aIterator  A pointer to the iterator to initialize.
 */
void otBorderAgentTrackerInitIterator(otInstance *aInstance, otBorderAgentTrackerIterator *aIterator);

/**
 * Gets the information for the next discovered Border Agent.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_TRACKER_ENABLE`.
 *
 * The iterator initialization time is used to determine the `mMsecSinceDiscovered` and `mMsecSinceLastChange` in the
 * `otBorderAgentTrackerAgentInfo`.
 *
 * @param[in]     aInstance    A pointer to an OpenThread instance.
 * @param[in,out] aIterator    A pointer to the iterator. An iterator MUST be initialized using
 *                             `otBorderAgentTrackerInitIterator()` before it is used.
 * @param[out]    aAgentInfo   A pointer to an `otBorderAgentTrackerAgentInfo` struct to populate.
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the information for the next agent.
 * @retval OT_ERROR_NOT_FOUND  No more agents were found.
 */
otError otBorderAgentTrackerGetNextAgent(otInstance                    *aInstance,
                                         otBorderAgentTrackerIterator  *aIterator,
                                         otBorderAgentTrackerAgentInfo *aAgentInfo);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_BORDER_AGENT_TRACKER_H_
