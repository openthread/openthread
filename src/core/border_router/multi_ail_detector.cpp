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
 *   This file implements the multi AIL detector.
 */

#include "multi_ail_detector.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace BorderRouter {

RegisterLogModule("MultiAilDetect");

//---------------------------------------------------------------------------------------------------------------------
// MultiAilDetector

MultiAilDetector::MultiAilDetector(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kAutoEnableMode ? kStateStopped : kStateDisabled)
    , mDetected(false)
    , mNetDataPeerBrCount(0)
    , mReachablePeerBrCount(0)
    , mTimer(aInstance)
{
}

void MultiAilDetector::SetEnabled(bool aEnable)
{
    if (aEnable)
    {
        VerifyOrExit(mState == kStateDisabled);
        SetState(kStateStopped);
        UpdateState();
    }
    else
    {
        VerifyOrExit(mState != kStateDisabled);
        Stop();
        SetState(kStateDisabled);
    }

exit:
    return;
}

void MultiAilDetector::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    LogInfo("State: %s -> %s", StateToString(mState), StateToString(aState));
    mState = aState;

exit:
    return;
}

void MultiAilDetector::HandleNotifierEvents(ot::Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        UpdateState();
    }
}

void MultiAilDetector::UpdateState(void)
{
    VerifyOrExit(mState != kStateDisabled);

    if (Get<InfraIf>().IsRunning() && Get<Mle::Mle>().IsAttached())
    {
        Start();
    }
    else
    {
        Stop();
    }

exit:
    return;
}

void MultiAilDetector::Start(void)
{
    VerifyOrExit(mState == kStateStopped);

    SetState(kStateRunning);

    Get<RxRaTracker>().SetEnabled(true, RxRaTracker::kRequesterMultiAilDetector);
    Evaluate();

exit:
    return;
}

void MultiAilDetector::Stop(void)
{
    VerifyOrExit(mState == kStateRunning);

    mTimer.Stop();
    mDetected             = false;
    mNetDataPeerBrCount   = 0;
    mReachablePeerBrCount = 0;
    Get<RxRaTracker>().SetEnabled(false, RxRaTracker::kRequesterMultiAilDetector);

    SetState(kStateStopped);

exit:
    return;
}

void MultiAilDetector::HandleRxRaTrackerEvents(const RxRaTracker::Events &aEvents)
{
    VerifyOrExit(aEvents.mDecisionFactorChanged);
    Evaluate();

exit:
    return;
}

void MultiAilDetector::Evaluate(void)
{
    uint16_t count;
    uint32_t minAge;
    bool     detected;

    VerifyOrExit(mState == kStateRunning);

    count = Get<NetDataBrTracker>().CountBrs(NetDataBrTracker::kExcludeThisDevice, minAge);

    if (count != mNetDataPeerBrCount)
    {
        LogInfo("Peer BR count from netdata: %u -> %u", mNetDataPeerBrCount, count);
        mNetDataPeerBrCount = count;
    }

    count = Get<RxRaTracker>().GetReachablePeerBrCount();

    if (count != mReachablePeerBrCount)
    {
        LogInfo("Reachable Peer BR count from RaTracker: %u -> %u", mReachablePeerBrCount, count);
        mReachablePeerBrCount = count;
    }

    detected = (mNetDataPeerBrCount > mReachablePeerBrCount);

    if (detected == mDetected)
    {
        mTimer.Stop();
    }
    else if (!mTimer.IsRunning())
    {
        mTimer.Start(detected ? kDetectTime : kClearTime);
    }

exit:
    return;
}

void MultiAilDetector::HandleTimer(void)
{
    if (!mDetected)
    {
        LogNote("BRs on multi AIL detected - BRs are likely connected to different infra-links");
        LogInfo("More peer BRs in netdata vs from rx RAs for past %lu seconds", ToUlong(Time::MsecToSec(kDetectTime)));
        LogInfo("NetData Peer BR count: %u, RaTracker reachable Peer BR count: %u", mNetDataPeerBrCount,
                mReachablePeerBrCount);
        mDetected = true;
    }
    else
    {
        LogNote("BRs on multi AIL detection cleared");
        mDetected = false;
    }

    mCallback.InvokeIfSet(mDetected);

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    Get<MeshCoP::BorderAgent::TxtData>().Refresh();
#endif
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

const char *MultiAilDetector::StateToString(State aState)
{
#define StateMapList(_)           \
    _(kStateDisabled, "Disabled") \
    _(kStateStopped, "Stopped")   \
    _(kStateRunning, "Running")

    DefineEnumStringArray(StateMapList)

        return kStrings[aState];
}

#endif

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE
