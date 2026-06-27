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
 *   This file implements `ThreadLinkInfo`
 */

#include "thread_link_info.hpp"

namespace ot {

void ThreadLinkInfo::SetFrom(const Mac::RxFrame::Info &aFrameInfo)
{
    Mac::PanId dstPanId;

    Clear();

    if (aFrameInfo.mPanIds.IsSourcePresent())
    {
        mPanId = aFrameInfo.mPanIds.GetSource();
    }
    else
    {
        mPanId = aFrameInfo.mPanIds.GetDestination();
    }

    if (aFrameInfo.mPanIds.IsDestinationPresent())
    {
        dstPanId = aFrameInfo.mPanIds.GetDestination();
    }
    else
    {
        dstPanId = mPanId;
    }

    mIsDstPanIdBroadcast = (dstPanId == Mac::kPanIdBroadcast);

    mLinkSecurity = aFrameInfo.IsSecuredWith(Mac::RxFrame::kAllowKeyIdMode0 | Mac::RxFrame::kAllowKeyIdMode1);
    mChannel      = aFrameInfo.mChannel;
    mRss          = aFrameInfo.GetRxFrame().GetRssi();
    mLqi          = aFrameInfo.GetRxFrame().GetLqi();

#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    {
        const Mac::TimeIe *timeIe = aFrameInfo.GetRxFrame().Find<Mac::TimeIe>();

        if (timeIe != nullptr)
        {
            mNetworkTimeOffset = static_cast<int64_t>(timeIe->GetTime() - aFrame.GetTimestamp());
            mTimeSyncSeq       = timeIe->GetSequence();
        }
    }
#endif

#if OPENTHREAD_CONFIG_MULTI_RADIO
    mRadioType = static_cast<uint8_t>(aFrameInfo.mRadioType);
#endif
}

} // namespace ot
