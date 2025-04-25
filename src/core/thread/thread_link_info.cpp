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

void ThreadLinkInfo::SetFrom(const Mac::RxFrame &aFrame)
{
    Clear();

    if (kErrorNone != aFrame.GetSrcPanId(mPanId))
    {
        IgnoreError(aFrame.GetDstPanId(mPanId));
    }

    {
        Mac::PanId dstPanId;

        if (kErrorNone != aFrame.GetDstPanId(dstPanId))
        {
            dstPanId = mPanId;
        }

        mIsDstPanIdBroadcast = (dstPanId == Mac::kPanIdBroadcast);
    }

    if (aFrame.GetSecurityEnabled())
    {
        uint8_t keyIdMode;

        // MAC Frame Security was already validated at the MAC
        // layer. As a result, `GetKeyIdMode()` will never return
        // failure here.
        IgnoreError(aFrame.GetKeyIdMode(keyIdMode));

        mLinkSecurity = (keyIdMode == Mac::Frame::kKeyIdMode0) || (keyIdMode == Mac::Frame::kKeyIdMode1);
    }
    else
    {
        mLinkSecurity = false;
    }
    mChannel = aFrame.GetChannel();
    mRss     = aFrame.GetRssi();
    mLqi     = aFrame.GetLqi();
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aFrame.GetTimeIe() != nullptr)
    {
        mNetworkTimeOffset = aFrame.ComputeNetworkTimeOffset();
        mTimeSyncSeq       = aFrame.ReadTimeSyncSeq();
    }
#endif
#if OPENTHREAD_CONFIG_MULTI_RADIO
    mRadioType = static_cast<uint8_t>(aFrame.GetRadioType());
#endif
}

} // namespace ot
