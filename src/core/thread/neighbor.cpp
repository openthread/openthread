/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *   This file includes definitions for a Thread `Neighbor`.
 */

#include "neighbor.hpp"

#include "common/array.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/locator_getters.hpp"
#include "common/num_utils.hpp"
#include "instance/instance.hpp"

namespace ot {

void Neighbor::SetState(State aState)
{
    VerifyOrExit(mState != aState);
    mState = static_cast<uint8_t>(aState);

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    if (mState == kStateValid)
    {
        mConnectionStart = Uptime::MsecToSec(Get<Uptime>().GetUptime());
    }
#endif

exit:
    return;
}

#if OPENTHREAD_CONFIG_UPTIME_ENABLE
uint32_t Neighbor::GetConnectionTime(void) const
{
    return IsStateValid() ? Uptime::MsecToSec(Get<Uptime>().GetUptime()) - mConnectionStart : 0;
}
#endif

bool Neighbor::AddressMatcher::Matches(const Neighbor &aNeighbor) const
{
    bool matches = false;

    VerifyOrExit(aNeighbor.MatchesFilter(mStateFilter));

    if (mShortAddress != Mac::kShortAddrInvalid)
    {
        VerifyOrExit(mShortAddress == aNeighbor.GetRloc16());
    }

    if (mExtAddress != nullptr)
    {
        VerifyOrExit(*mExtAddress == aNeighbor.GetExtAddress());
    }

    matches = true;

exit:
    return matches;
}

void Neighbor::Info::SetFrom(const Neighbor &aNeighbor)
{
    Clear();

    mExtAddress       = aNeighbor.GetExtAddress();
    mAge              = Time::MsecToSec(TimerMilli::GetNow() - aNeighbor.GetLastHeard());
    mRloc16           = aNeighbor.GetRloc16();
    mLinkFrameCounter = aNeighbor.GetLinkFrameCounters().GetMaximum();
    mMleFrameCounter  = aNeighbor.GetMleFrameCounter();
    mLinkQualityIn    = aNeighbor.GetLinkQualityIn();
    mAverageRssi      = aNeighbor.GetLinkInfo().GetAverageRss();
    mLastRssi         = aNeighbor.GetLinkInfo().GetLastRss();
    mLinkMargin       = aNeighbor.GetLinkInfo().GetLinkMargin();
    mFrameErrorRate   = aNeighbor.GetLinkInfo().GetFrameErrorRate();
    mMessageErrorRate = aNeighbor.GetLinkInfo().GetMessageErrorRate();
    mRxOnWhenIdle     = aNeighbor.IsRxOnWhenIdle();
    mFullThreadDevice = aNeighbor.IsFullThreadDevice();
    mFullNetworkData  = (aNeighbor.GetNetworkDataType() == NetworkData::kFullSet);
    mVersion          = aNeighbor.GetVersion();
#if OPENTHREAD_CONFIG_UPTIME_ENABLE
    mConnectionTime = aNeighbor.GetConnectionTime();
#endif
}

void Neighbor::Init(Instance &aInstance)
{
    InstanceLocatorInit::Init(aInstance);
    mLinkInfo.Init(aInstance);
    SetState(kStateInvalid);
}

bool Neighbor::IsStateValidOrAttaching(void) const
{
    bool rval = false;

    switch (GetState())
    {
    case kStateInvalid:
    case kStateParentRequest:
    case kStateParentResponse:
        break;

    case kStateRestored:
    case kStateChildIdRequest:
    case kStateLinkRequest:
    case kStateChildUpdateRequest:
    case kStateValid:
        rval = true;
        break;
    }

    return rval;
}

bool Neighbor::MatchesFilter(StateFilter aFilter) const
{
    bool matches = false;

    switch (aFilter)
    {
    case kInStateValid:
        matches = IsStateValid();
        break;

    case kInStateValidOrRestoring:
        matches = IsStateValidOrRestoring();
        break;

    case kInStateChildIdRequest:
        matches = IsStateChildIdRequest();
        break;

    case kInStateValidOrAttaching:
        matches = IsStateValidOrAttaching();
        break;

    case kInStateInvalid:
        matches = IsStateInvalid();
        break;

    case kInStateAnyExceptInvalid:
        matches = !IsStateInvalid();
        break;

    case kInStateAnyExceptValidOrRestoring:
        matches = !IsStateValidOrRestoring();
        break;

    case kInStateAny:
        matches = true;
        break;
    }

    return matches;
}

#if OPENTHREAD_CONFIG_MULTI_RADIO
void Neighbor::SetLastRxFragmentTag(uint16_t aTag)
{
    mLastRxFragmentTag     = (aTag == 0) ? 0xffff : aTag;
    mLastRxFragmentTagTime = TimerMilli::GetNow();
}

bool Neighbor::IsLastRxFragmentTagSet(void) const
{
    return (mLastRxFragmentTag != 0) && (TimerMilli::GetNow() <= mLastRxFragmentTagTime + kLastRxFragmentTagTimeout);
}
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
void Neighbor::AggregateLinkMetrics(uint8_t aSeriesId, uint8_t aFrameType, uint8_t aLqi, int8_t aRss)
{
    for (LinkMetrics::SeriesInfo &entry : mLinkMetricsSeriesInfoList)
    {
        if (aSeriesId == 0 || aSeriesId == entry.GetSeriesId())
        {
            entry.AggregateLinkMetrics(aFrameType, aLqi, aRss);
        }
    }
}

LinkMetrics::SeriesInfo *Neighbor::GetForwardTrackingSeriesInfo(const uint8_t &aSeriesId)
{
    return mLinkMetricsSeriesInfoList.FindMatching(aSeriesId);
}

void Neighbor::AddForwardTrackingSeriesInfo(LinkMetrics::SeriesInfo &aSeriesInfo)
{
    mLinkMetricsSeriesInfoList.Push(aSeriesInfo);
}

LinkMetrics::SeriesInfo *Neighbor::RemoveForwardTrackingSeriesInfo(const uint8_t &aSeriesId)
{
    return mLinkMetricsSeriesInfoList.RemoveMatching(aSeriesId);
}

void Neighbor::RemoveAllForwardTrackingSeriesInfo(void)
{
    while (!mLinkMetricsSeriesInfoList.IsEmpty())
    {
        LinkMetrics::SeriesInfo *seriesInfo = mLinkMetricsSeriesInfoList.Pop();
        Get<LinkMetrics::Subject>().Free(*seriesInfo);
    }
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

const char *Neighbor::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Invalid",        // (0) kStateInvalid
        "Restored",       // (1) kStateRestored
        "ParentReq",      // (2) kStateParentRequest
        "ParentRes",      // (3) kStateParentResponse
        "ChildIdReq",     // (4) kStateChildIdRequest
        "LinkReq",        // (5) kStateLinkRequest
        "ChildUpdateReq", // (6) kStateChildUpdateRequest
        "Valid",          // (7) kStateValid
    };

    static_assert(0 == kStateInvalid, "kStateInvalid value is incorrect");
    static_assert(1 == kStateRestored, "kStateRestored value is incorrect");
    static_assert(2 == kStateParentRequest, "kStateParentRequest value is incorrect");
    static_assert(3 == kStateParentResponse, "kStateParentResponse value is incorrect");
    static_assert(4 == kStateChildIdRequest, "kStateChildIdRequest value is incorrect");
    static_assert(5 == kStateLinkRequest, "kStateLinkRequest value is incorrect");
    static_assert(6 == kStateChildUpdateRequest, "kStateChildUpdateRequest value is incorrect");
    static_assert(7 == kStateValid, "kStateValid value is incorrect");

    return kStateStrings[aState];
}

} // namespace ot
