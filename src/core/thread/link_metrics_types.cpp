/*
 *  Copyright (c) 2020-22, The OpenThread Authors.
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
 *   This file includes definitions for Thread Link Metrics.
 */

#include "link_metrics_types.hpp"

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE

#include "common/code_utils.hpp"
#include "mac/mac_frame.hpp"

namespace ot {
namespace LinkMetrics {

//----------------------------------------------------------------------------------------------------------------------
// Metrics

uint8_t Metrics::ConvertToTypeIds(uint8_t aTypeIds[]) const
{
    uint8_t count = 0;

    if (mPduCount)
    {
        aTypeIds[count++] = TypeId::kPdu;
    }

    if (mLqi)
    {
        aTypeIds[count++] = TypeId::kLqi;
    }

    if (mLinkMargin)
    {
        aTypeIds[count++] = TypeId::kLinkMargin;
    }

    if (mRssi)
    {
        aTypeIds[count++] = TypeId::kRssi;
    }

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (mReserved)
    {
        for (uint8_t i = 0; i < count; i++)
        {
            TypeId::MarkAsReserved(aTypeIds[i]);
        }
    }
#endif

    return count;
}

//----------------------------------------------------------------------------------------------------------------------
// SeriesFlags

uint8_t SeriesFlags::ConvertToMask(void) const
{
    uint8_t mask = 0;

    mask |= (mLinkProbe ? kLinkProbeFlag : 0);
    mask |= (mMacData ? kMacDataFlag : 0);
    mask |= (mMacDataRequest ? kMacDataRequestFlag : 0);
    mask |= (mMacAck ? kMacAckFlag : 0);

    return mask;
}

void SeriesFlags::SetFrom(uint8_t aFlagsMask)
{
    mLinkProbe      = (aFlagsMask & kLinkProbeFlag);
    mMacData        = (aFlagsMask & kMacDataFlag);
    mMacDataRequest = (aFlagsMask & kMacDataRequestFlag);
    mMacAck         = (aFlagsMask & kMacAckFlag);
}

//----------------------------------------------------------------------------------------------------------------------
// SeriesInfo

void SeriesInfo::Init(uint8_t aSeriesId, uint8_t aSeriesFlagsMask, const Metrics &aMetrics)
{
    mSeriesId = aSeriesId;
    mSeriesFlags.SetFrom(aSeriesFlagsMask);
    mMetrics = aMetrics;
    mRssAverager.Clear();
    mLqiAverager.Clear();
    mPduCount = 0;
}

void SeriesInfo::AggregateLinkMetrics(uint8_t aFrameType, uint8_t aLqi, int8_t aRss)
{
    if (IsFrameTypeMatch(aFrameType))
    {
        mPduCount++;
        mLqiAverager.Add(aLqi);
        IgnoreError(mRssAverager.Add(aRss));
    }
}

bool SeriesInfo::IsFrameTypeMatch(uint8_t aFrameType) const
{
    bool match = false;

    switch (aFrameType)
    {
    case kSeriesTypeLinkProbe:
        VerifyOrExit(!mSeriesFlags.IsMacDataFlagSet()); // Ignore this when Mac Data is accounted
        match = mSeriesFlags.IsLinkProbeFlagSet();
        break;
    case Mac::Frame::kTypeData:
        match = mSeriesFlags.IsMacDataFlagSet();
        break;
    case Mac::Frame::kTypeMacCmd:
        match = mSeriesFlags.IsMacDataRequestFlagSet();
        break;
    case Mac::Frame::kTypeAck:
        match = mSeriesFlags.IsMacAckFlagSet();
        break;
    default:
        break;
    }

exit:
    return match;
}

} // namespace LinkMetrics
} // namespace ot

#endif // OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE || OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
