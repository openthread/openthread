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
 * This file includes definitions for the Seeker module.
 *
 * The Seeker is responsible for discovering nearby Joiner Router candidates, prioritizing them, and
 * iterating through the list to select the next best candidate for connection. It acts as a
 * sub-module of the `Joiner` class.
 */

#ifndef OT_CORE_MESHCOP_SEEKER_HPP_
#define OT_CORE_MESHCOP_SEEKER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_JOINER_ENABLE

#include "common/callback.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "mac/mac_types.hpp"
#include "net/socket.hpp"
#include "thread/discover_scanner.hpp"

namespace ot {
namespace MeshCoP {

/**
 * Represents a MeshCoP Seeker.
 */
class Seeker : public InstanceLocator, private NonCopyable
{
public:
    static constexpr uint16_t kUdpPort = OPENTHREAD_CONFIG_JOINER_UDP_PORT; ///< The default Joiner UDP port.

    typedef Mle::DiscoverScanner::ScanResult ScanResult; ///< Discover Scan result.

    /**
     * Represents a verdict returned from `ScanEvaluator` when evaluating a Discover Scan result.
     */
    enum Verdict : uint8_t
    {
        kAccept,          ///< The scan result is acceptable.
        kAcceptPreferred, ///< The scan result is acceptable and preferred.
        kIgnore,          ///< The scan result should be ignored.
    };

    /**
     * Represents the callback function type used to evaluate a scan result or report the end of a scan.
     *
     * @param[in] aContext  A pointer to the callback context.
     * @param[in] aResult   A pointer to the scan result to evaluate, or `nullptr` to indicate scan completion.
     *
     * @returns The verdict for the scan result (`kAccept`, `kAcceptPreferred`, or `kIgnore`).
     *          If @p aResult is `nullptr` (scan complete), the return value is ignored.
     */
    typedef Verdict (*ScanEvaluator)(void *aContext, const ScanResult *aResult);

    /**
     * Initializes the `Seeker`
     *
     * @param[in]  aInstance  The OpenThread instance.
     */
    explicit Seeker(Instance &aInstance);

    /**
     * Starts the Seeker operation.
     *
     * The Seeker generates and sets a random MAC address for anonymity, then initiates an MLE Discover Scan to find
     * Joiner Router candidates.
     *
     * Found candidates are reported to the @p aScanEvaluator callback. Based on the returned `Verdict`, the Seeker
     * maintains a prioritized list of candidates for future connection attempts.
     *
     * @param[in] aScanEvaluator   The callback function to evaluate scan results.
     * @param[in] aContext         An arbitrary context pointer to use with @p aScanEvaluator.
     *
     * @retval kErrorNone          Successfully started the Seeker.
     * @retval kErrorBusy          The Seeker is already active (scanning or connecting).
     * @retval kErrorInvalidState  The IPv6 interface is not enabled, or MLE is enabled.
     */
    Error Start(ScanEvaluator aScanEvaluator, void *aContext);

    /**
     * Stops the Seeker operation.
     *
     * This method stops any ongoing discovery or connection process, unregisters the unsecure Joiner UDP port, and
     * clears internal state. If the Seeker is already stopped, this method has no effect.
     *
     * If the join process succeeds after a call to `SetupNextConnection()`, the caller MUST call this method to stop
     * the Seeker and, importantly, unregister the Joiner UDP port.
     *
     * Note: If `SetupNextConnection()` returns `kErrorNotFound` (indicating the candidate list is exhausted), the
     * Seeker stops automatically.
     */
    void Stop(void);

    /**
     * Indicates whether or not the Seeker is running.
     *
     * @retval TRUE    The seeker is active and running.
     * @retval FALSE   The seeker is stopped.
     */
    bool IsRunning(void) const { return GetState() != kStateStopped; }

    /**
     * Selects the next best candidate and prepares the connection.
     *
     * This method must be called after the discovery scan has completed (indicated by the `ScanEvaluator` callback
     * receiving `nullptr`). Calling it before scan completion will result in `kErrorInvalidState`.
     *
     * This method iterates through the discovered Joiner Router candidates in order of priority. For the selected
     * candidate, it configures the radio channel and PAN ID, and populates @p aSockAddr with the candidate's address.
     * It also registers the Joiner UDP port `kUdpPort` as an unsecure port to allow UDP
     * connection to the candidate.
     *
     * If the list is exhausted, this method returns `kErrorNotFound` and automatically calls `Stop()`, which removes
     * the unsecure port and clears internal state.
     *
     * @param[out] aSockAddr       A reference to a socket address to output the candidate's address.
     *
     * @retval kErrorNone          Successfully set up the connection to the next candidate.
     * @retval kErrorNotFound      No more candidates are available (list exhausted).
     * @retval kErrorInvalidState  The Seeker is not in a valid state (e.g. scan not yet completed).
     */
    Error SetUpNextConnection(Ip6::SockAddr &aSockAddr);

private:
    static constexpr uint16_t kMaxCandidates = OPENTHREAD_CONFIG_JOINER_MAX_CANDIDATES;

    enum State : uint8_t
    {
        kStateStopped,
        kStateDiscovering,
        kStateDiscoverDone,
        kStateConnecting,
    };

    struct Candidate
    {
        bool IsValid(void) const { return mPriority != 0; }

        Mac::ExtAddress mExtAddr;
        Mac::PanId      mPanId;
        uint16_t        mJoinerUdpPort;
        uint8_t         mChannel;
        uint8_t         mPriority;
    };

    State          GetState(void) const { return mState; }
    void           SetState(State aState) { mState = aState; }
    static void    HandleDiscoverResult(ScanResult *aResult, void *aContext);
    void           HandleDiscoverResult(ScanResult *aResult);
    void           SaveCandidate(const ScanResult &aResult, bool aPreferred);
    static uint8_t CalculatePriority(int8_t aRssi, bool aPreferred);

    State                   mState;
    Callback<ScanEvaluator> mScanEvaluator;
    Candidate               mCandidates[kMaxCandidates];
    uint16_t                mCandidateIndex;
};

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_JOINER_ENABLE

#endif // OT_CORE_MESHCOP_SEEKER_HPP_
