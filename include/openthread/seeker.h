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
 * This file implements the Thread Seeker role.
 *
 * The Seeker is a part of the Thread MeshCoP process. It is responsible for discovering nearby Joiner Router
 * candidates, prioritizing them, and iterating through the list to select the best candidate for connection.
 * It also operates as a sub-system of the `Joiner`, delegating control to the next layer to enable the
 * implementation of alternative and custom joining protocols.
 */

#ifndef OPENTHREAD_SEEKER_H_
#define OPENTHREAD_SEEKER_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/link.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-seeker
 *
 * @brief
 *   This module includes functions for the Thread Seeker role.
 *
 * @note
 *   The functions in this module require `OPENTHREAD_CONFIG_SEEKER_ENABLE`.
 *
 * @{
 */

/**
 * Represents a Discover Scan result.
 */
typedef otActiveScanResult otSeekerScanResult;

/**
 * Represents a verdict returned from the `otSeekerScanEvaluator` callback when evaluating a Discover Scan result.
 */
typedef enum otSeekerVerdict
{
    OT_SEEKER_ACCEPT,           ///< The scan result is acceptable.
    OT_SEEKER_ACCEPT_PREFERRED, ///< The scan result is acceptable and preferred.
    OT_SEEKER_IGNORE,           ///< The scan result should be ignored.
} otSeekerVerdict;

/**
 * Represents the callback function type used to evaluate a scan result or report the end of a scan.
 *
 * @param[in] aContext  A pointer to the callback context.
 * @param[in] aResult   A pointer to the scan result to evaluate, or `NULL` to indicate scan completion.
 *
 * @returns The verdict for the scan result (Accept, AcceptPreferred, or Ignore).
 *          If @p aResult is `NULL` (scan complete), the return value is ignored.
 */
typedef otSeekerVerdict (*otSeekerScanEvaluator)(void *aContext, const otSeekerScanResult *aResult);

/**
 * Starts the Seeker operation.
 *
 * The Seeker generates and sets a random MAC address for anonymity, then initiates an MLE Discover Scan to find
 * Joiner Router candidates.
 *
 * Found candidates are reported to the @p aScanEvaluator callback. Based on the returned `otSeekerVerdict`, the Seeker
 * maintains a prioritized list of candidates for future connection attempts.
 *
 * @param[in] aInstance        The OpenThread instance.
 * @param[in] aScanEvaluator   The callback function to evaluate scan results.
 * @param[in] aContext         An arbitrary context pointer to use with @p aScanEvaluator.
 *
 * @retval OT_ERROR_NONE           Successfully started the Seeker.
 * @retval OT_ERROR_BUSY           The Seeker is already active (scanning or connecting).
 * @retval OT_ERROR_INVALID_STATE  The IPv6 interface is not enabled, or MLE is enabled.
 */
otError otSeekerStart(otInstance *aInstance, otSeekerScanEvaluator aScanEvaluator, void *aContext);

/**
 * Stops the Seeker operation.
 *
 * This function stops any ongoing discovery or connection process, unregisters the unsecure Joiner/Seeker UDP port, and
 * clears internal state. If the Seeker is already stopped, this method has no effect.
 *
 * If the join process succeeds after a call to `otSeekerSetUpNextConnection()`, the caller MUST call this method to
 * stop the Seeker and, importantly, unregister the Seeker UDP port as an unsecure port.
 *
 * @note If `otSeekerSetUpNextConnection()` returns `OT_ERROR_NOT_FOUND` (indicating the candidate list is exhausted),
 * the Seeker stops automatically.
 *
 * @param[in] aInstance   The OpenThread instance.
 */
void otSeekerStop(otInstance *aInstance);

/**
 * Indicates whether or not the Seeker is running.
 *
 * @param[in] aInstance   The OpenThread instance.
 *
 * @retval TRUE    The Seeker is active and running.
 * @retval FALSE   The Seeker is stopped.
 */
bool otSeekerIsRunning(otInstance *aInstance);

/**
 * Gets the Seeker UDP port (unsecure port).
 *
 * @param[in] aInstance   The OpenThread instance.
 *
 * @returns The Seeker UDP port.
 */
uint16_t otSeekerGetUdpPort(otInstance *aInstance);

/**
 * Sets the Seeker UDP port (unsecure port).
 *
 * This UDP port can only be changed when the Seeker is not running.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aUdpPort    The Seeker UDP port.
 *
 * @retval OT_ERROR_NONE           Successfully set the Joiner/Seeker UDP port.
 * @retval OT_ERROR_INVALID_STATE  The Seeker is already running.
 */
otError otSeekerSetUdpPort(otInstance *aInstance, uint16_t aUdpPort);

/**
 * Selects the next best candidate and prepares the connection.
 *
 * This function MUST be called after the discovery scan has completed (indicated by the `otSeekerScanEvaluator`
 * callback receiving `NULL`). Calling it before scan completion will result in `OT_ERROR_INVALID_STATE`.
 *
 * This function iterates through the discovered Joiner Router candidates in order of priority. For the selected
 * candidate, it configures the radio channel and PAN ID, and populates @p aSockAddr with the candidate's address.
 * It also registers the Seeker UDP port `otSeekerGetUdpPort()` as an unsecure port to allow a UDP connection to the
 * candidate. The next layer code can start sending UDP messages to the given `otSockAddr` ensuring to use the unsecure
 * Seeker UDP port as the source port. These messages are then forwarded by the Joiner Router onward to a
 * Commissioner/Enroller connected via a Border Agent/Admitter.
 *
 * If the list is exhausted, this function returns `OT_ERROR_NOT_FOUND` and automatically calls `otSeekerStop()`,
 * which removes the unsecure port and clears internal state.
 *
 * @param[out] aSockAddr            A pointer to a socket address to output the candidate's address.
 *
 * @retval OT_ERROR_NONE            Successfully set up the connection to the next candidate.
 * @retval OT_ERROR_NOT_FOUND       No more candidates are available (list exhausted).
 * @retval OT_ERROR_INVALID_STATE   The Seeker is not in a valid state (e.g. scan not yet completed).
 */
otError otSeekerSetUpNextConnection(otInstance *aInstance, otSockAddr *aSockAddr);

/**
 * @}
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OPENTHREAD_SEEKER_H_
