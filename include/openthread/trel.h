/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *  This file defines the OpenThread TREL (Thread Radio Encapsulation Link) APIs for Thread Over Infrastructure.
 */

#ifndef OPENTHREAD_TREL_H_
#define OPENTHREAD_TREL_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/dataset.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/trel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-trel
 *
 * @brief
 *   This module defines Thread Radio Encapsulation Link (TREL) APIs for Thread Over Infrastructure.
 *
 *   The functions in this module require `OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE` to be enabled.
 *
 * @{
 */

/**
 * Represents a TREL peer.
 */
typedef struct otTrelPeer
{
    otExtAddress    mExtAddress; ///< The Extended MAC Address of TREL peer.
    otExtendedPanId mExtPanId;   ///< The Extended PAN Identifier of TREL peer.
    otSockAddr      mSockAddr;   ///< The IPv6 socket address of TREL peer.
} otTrelPeer;

/**
 * Represents an iterator for iterating over TREL peer table entries.
 */
typedef const void *otTrelPeerIterator;

/**
 * Sets the user's preference to enable or disable the TREL operation.
 *
 * The TREL interface's operational state is determined by two factors: the user's preference (set by this function)
 * and the OpenThread stack's internal state. The TREL interface is enabled only when both the user and the OpenThread
 * stack have it enabled. Otherwise, it is disabled.
 *
 * Upon OpenThread initialization, the user's preference is set to enabled by default. This allows the stack to
 * control the TREL interface state automatically (e.g., enabling it when radio links are enabled and disabling
 * it when radio links are disabled).
 *
 * If the user explicitly disables the TREL operation by calling this function with @p aEnable as `false`, it will
 * remain disabled until the user explicitly re-enables it by calling this function with @p aEnable as `true`. This
 * ensures the user's 'disable' request persists across other OpenThread stack state changes (which may trigger
 * disabling/enabling of all radio links, including the TREL link).
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aEnable    A boolean to enable/disable the TREL operation.
 */
void otTrelSetEnabled(otInstance *aInstance, bool aEnable);

/**
 * Indicates whether the TREL operation is enabled.
 *
 * The TREL operation is enabled if and only if it is enabled by both the user (see `otTrelSetEnabled()`) and the
 * OpenThread stack.
 *
 * @param[in] aInstance   The OpenThread instance.
 *
 * @retval TRUE if the TREL operation is enabled.
 * @retval FALSE if the TREL operation is disabled.
 */
bool otTrelIsEnabled(otInstance *aInstance);

/**
 * Initializes a peer table iterator.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aIterator   The iterator to initialize.
 */
void otTrelInitPeerIterator(otInstance *aInstance, otTrelPeerIterator *aIterator);

/**
 * Iterates over the peer table entries and get the next entry from the table
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aIterator   The iterator. MUST be initialized.
 *
 * @returns A pointer to the next `otTrelPeer` entry or `NULL` if no more entries in the table.
 */
const otTrelPeer *otTrelGetNextPeer(otInstance *aInstance, otTrelPeerIterator *aIterator);

/**
 * Returns the number of TREL peers.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns  The number of TREL peers.
 */
uint16_t otTrelGetNumberOfPeers(otInstance *aInstance);

/**
 * Sets the filter mode (enables/disables filtering).
 *
 * When filter mode is enabled, any rx and tx traffic through TREL interface is silently dropped. This is mainly
 * intended for use during testing.
 *
 * Unlike `otTrel{Enable/Disable}()` which fully starts/stops the TREL operation, when filter mode is enabled the
 * TREL interface continues to be enabled.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aFiltered   TRUE to enable filter mode, FALSE to disable filter mode.
 */
void otTrelSetFilterEnabled(otInstance *aInstance, bool aEnable);

/**
 * Indicates whether or not the filter mode is enabled.
 *
 * @param[in] aInstance   The OpenThread instance.
 *
 * @retval TRUE if the TREL filter mode is enabled.
 * @retval FALSE if the TREL filter mode is disabled.
 */
bool otTrelIsFilterEnabled(otInstance *aInstance);

/**
 * Represents a group of TREL related counters.
 */
typedef otPlatTrelCounters otTrelCounters;

/**
 * Gets the TREL counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns  A pointer to the TREL counters.
 */
const otTrelCounters *otTrelGetCounters(otInstance *aInstance);

/**
 * Resets the TREL counters.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 */
void otTrelResetCounters(otInstance *aInstance);

/**
 * Gets the UDP port of the TREL interface.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 *
 * @returns UDP port of the TREL interface.
 */
uint16_t otTrelGetUdpPort(otInstance *aInstance);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_TREL_H_
