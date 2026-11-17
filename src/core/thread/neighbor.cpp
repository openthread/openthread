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

#include "instance/instance.hpp"

namespace ot {

void Neighbor::SetState(State aState)
{
    VerifyOrExit(mState != aState);
    mState = static_cast<uint8_t>(aState);

    if (mState == kStateValid)
    {
        mConnectionStart = Get<UptimeTracker>().GetUptimeInSeconds();
    }

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    if (mState == kStateInvalid)
    {
        RemoveAllForwardTrackingSeriesInfo();
    }
#endif

exit:
    return;
}

uint32_t Neighbor::GetConnectionTime(void) const
{
    return IsStateValid() ? Get<UptimeTracker>().GetUptimeInSeconds() - mConnectionStart : 0;
}

bool Neighbor::AddressMatcher::Matches(const Neighbor &aNeighbor) const
{
    bool matches = false;

    VerifyOrExit(aNeighbor.Matches(mStateFilter));

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
    mConnectionTime   = aNeighbor.GetConnectionTime();
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

bool Neighbor::Matches(StateFilter aFilter) const
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

    case kInStateLinkRequest:
        matches = IsStateLinkRequest();
        break;

    case kInStateAny:
        matches = true;
        break;
    }

    return matches;
}

#if OPENTHREAD_CONFIG_MULTI_RADIO
bool Neighbor::IsRxFragmentTagDuplicate(uint16_t aTag)
{
    bool      isDuplicate = false;
    TimeMilli now         = TimerMilli::GetNow();

    aTag = (aTag == 0) ? 0xffff : aTag;

    if (!IsLastRxFragmentTagSet(now))
    {
        mLastRxFragmentTag        = aTag;
        mPrevRxFragmentTagsBitmap = 0;
        mLastRxFragmentTagTime    = now;
        ExitNow();
    }

    if (aTag == mLastRxFragmentTag)
    {
        ExitNow(isDuplicate = true);
    }

    if (SerialNumber::IsGreater(aTag, mLastRxFragmentTag))
    {
        uint16_t delta = aTag - mLastRxFragmentTag;

        mPrevRxFragmentTagsBitmap = (delta < kPrevRxFragmentTagsBitmapBits) ? (mPrevRxFragmentTagsBitmap << delta) : 0;

        if (delta <= kPrevRxFragmentTagsBitmapBits)
        {
            mPrevRxFragmentTagsBitmap |= (1U << (delta - 1));
        }

        mLastRxFragmentTag     = aTag;
        mLastRxFragmentTagTime = now;
    }
    else
    {
        uint16_t delta = mLastRxFragmentTag - aTag;
        uint32_t mask;

        VerifyOrExit(delta <= kPrevRxFragmentTagsBitmapBits, isDuplicate = true);

        mask = (1U << (delta - 1));
        VerifyOrExit((mPrevRxFragmentTagsBitmap & mask) == 0, isDuplicate = true);
        mPrevRxFragmentTagsBitmap |= mask;
    }

exit:
    return isDuplicate;
}

bool Neighbor::IsLastRxFragmentTagSet(TimeMilli aNow) const
{
    return (mLastRxFragmentTag != 0) && (aNow <= mLastRxFragmentTagTime + kLastRxFragmentTagTimeout);
}
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
Neighbor::SeriesInfoList &Neighbor::SeriesInfoList::operator=(SeriesInfoList &aOther)
{
    // Move the entries so that each `SeriesInfo` entry stays owned
    // by a single list.

    LinkMetrics::SeriesInfo *seriesInfo;

    if (this != &aOther)
    {
        while ((seriesInfo = aOther.Pop()) != nullptr)
        {
            PushAfterTail(*seriesInfo);
        }
    }

    return *this;
}

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
    Get<LinkMetrics::Subject>().FreeAll(mLinkMetricsSeriesInfoList);
}
#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

const char *Neighbor::StateToString(State aState)
{
#define StateMapList(_)                           \
    _(kStateInvalid, "Invalid")                   \
    _(kStateRestored, "Restored")                 \
    _(kStateParentRequest, "ParentReq")           \
    _(kStateParentResponse, "ParentRes")          \
    _(kStateChildIdRequest, "ChildIdReq")         \
    _(kStateLinkRequest, "LinkReq")               \
    _(kStateChildUpdateRequest, "ChildUpdateReq") \
    _(kStateValid, "Valid")

    DefineEnumStringArray(StateMapList);

    return kStrings[aState];
}

} // namespace ot
