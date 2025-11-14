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
 *   This file includes definitions for the multi AIL detector.
 */

#ifndef MULTI_AIL_DETECTOR_HPP_
#define MULTI_AIL_DETECTOR_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
#error "MULTI_AIL_DETECTION_ENABLE feature requires OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE"
#endif

#include <openthread/multi_ail_detection.h>

#include "border_router/br_types.hpp"
#include "border_router/rx_ra_tracker.hpp"
#include "common/callback.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"

namespace ot {
namespace BorderRouter {

class InfraIf;
class NetDataBrTracker;

/**
 * Implements Multi-AIL (Adjacent Infrastructure Link) detection.
 *
 * This class detects whether Border Routers (BRs) may be connected to different AILs by tracking the number of peer
 * BRs from Network Data versus from `RxRaTracker`. If the Network Data count exceeds the RA-tracked count for more
 * than `kDetectTime` (10 minutes), it notifies this using a callback. To clear the state, `kClearTime` (1 minute) is
 * used.
 *
 * This longer detection window of 10 minutes helps to avoid false positives due to transient changes. `RxRaTracker`
 * uses 200 seconds for reachability checks of peer BRs. Stale Network Data entries are also expected to age out
 * within a few minutes, so a 10-minute detection time accommodates both.
 */
class MultiAilDetector : public InstanceLocator
{
    friend class InfraIf;
    friend class NetDataBrTracker;
    friend class RxRaTracker;
    friend class ot::Notifier;

public:
    typedef otBorderRoutingMultiAilCallback MultiAilCallback; ///< Multi AIL detection callback.

    /**
     * Initializes the `MultiAilDetector`.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit MultiAilDetector(Instance &aInstance);

    /**
     * Enables or disables the Multi-AIL Detector.
     *
     * When enabled, the detector starts monitoring the infrastructure interface state and evaluates for multiple AILs.
     * When disabled, the detector stops and resets its internal state.
     *
     * If `OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_AUTO_ENABLE_MODE` is enabled, the detector is enabled
     * by default and starts running when the infra-if network is initialized and becomes active.
     *
     * @param[in] aEnable TRUE to enable the detector, FALSE to disable.
     */
    void SetEnabled(bool aEnable);

    /**
     * Checks if the Multi-AIL Detector is enabled.
     *
     * @retval TRUE   If the detector is enabled.
     * @retval FALSE  If the detector is disabled.
     */
    bool IsEnabled(void) const { return mState != kStateDisabled; }

    /**
     * Checks if the Multi-AIL Detector is running.
     *
     * The detector runs when it is enabled and the infrastructure interface is also running.
     *
     * @retval TRUE   If the detector is running.
     * @retval FALSE  If the detector is not running.
     */
    bool IsRunning(void) const { return mState == kStateRunning; }

    /**
     * Gets the current detection state regarding multiple Adjacent Infrastructure Links (AILs).
     *
     * It returns whether this detector currently believes that Border Routers (BRs) on the Thread mesh may be
     * connected to different AILs.
     *
     * See `otBorderRoutingIsMultiAilDetected()` for more details about detection process.
     *
     * @retval TRUE   Has detected that BRs are likely connected to multiple AILs.
     * @retval FALSE  Has not detected (or no longer detects) that BRs are connected to multiple AILs.
     */
    bool IsDetected(void) const { return mDetected; }

    /**
     * Sets a callback function to be notified of changes in the multi-AIL detection state.
     *
     * Subsequent calls to this function will overwrite the previous callback setting. Using `NULL` for @p aCallback
     * will disable the callback.
     *
     * @param[in] aCallback  The callback function
     * @param[in] aContext   A pointer to application-specific context used with @p aCallback.
     */
    void SetCallback(MultiAilCallback aCallback, void *aContext) { mCallback.Set(aCallback, aContext); }

private:
    static constexpr uint32_t kDetectTime = 10 * Time::kOneMinuteInMsec;
    static constexpr uint32_t kClearTime  = 1 * Time::kOneMinuteInMsec;

    static constexpr bool kAutoEnableMode = OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_AUTO_ENABLE_MODE;

    enum State : uint8_t
    {
        kStateDisabled, // Disabled.
        kStateStopped,  // Enabled but stopped and waiting for `InfraIf` state change.
        kStateRunning,  // Enabled and running.
    };

    void SetState(State aState);
    void UpdateState(void);
    void Start(void);
    void Stop(void);
    void Evaluate(void);
    void HandleTimer(void);

    // Callback from `Notifier`
    void HandleNotifierEvents(ot::Events aEvents);

    // Callback from `InfraIf`
    void HandleInfraIfStateChanged(void) { UpdateState(); }

    // Callback from `RxRaTracker`
    void HandleRxRaTrackerEvents(const RxRaTracker::Events &aEvents);

    static const char *StateToString(State aState);

    using DetectCallback = Callback<MultiAilCallback>;
    using DetectTimer    = TimerMilliIn<MultiAilDetector, &MultiAilDetector::HandleTimer>;

    State          mState;
    bool           mDetected;
    uint16_t       mNetDataPeerBrCount;
    uint16_t       mReachablePeerBrCount;
    DetectTimer    mTimer;
    DetectCallback mCallback;
};

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE

#endif // MULTI_AIL_DETECTOR_HPP_
