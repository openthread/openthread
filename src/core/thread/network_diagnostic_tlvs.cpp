/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements methods for generating and processing Network Diagnostics TLVs.
 */

#include "network_diagnostic_tlvs.hpp"

#include "common/time.hpp"

namespace ot {
namespace NetworkDiagnostic {

#if OPENTHREAD_FTD

void ChildTlv::InitFrom(const Child &aChild)
{
    Clear();

    SetType(kChild);
    SetLength(sizeof(*this) - sizeof(Tlv));

    mFlags |= aChild.IsRxOnWhenIdle() ? kFlagsRxOnWhenIdle : 0;
    mFlags |= aChild.IsFullThreadDevice() ? kFlagsFtd : 0;
    mFlags |= (aChild.GetNetworkDataType() == NetworkData::kFullSet) ? kFlagsFullNetdta : 0;
    mFlags |= kFlagsTrackErrRate;

    mRloc16              = BigEndian::HostSwap16(aChild.GetRloc16());
    mExtAddress          = aChild.GetExtAddress();
    mVersion             = BigEndian::HostSwap16(aChild.GetVersion());
    mTimeout             = BigEndian::HostSwap32(aChild.GetTimeout());
    mAge                 = BigEndian::HostSwap32(Time::MsecToSec(TimerMilli::GetNow() - aChild.GetLastHeard()));
    mConnectionTime      = BigEndian::HostSwap32(aChild.GetConnectionTime());
    mSupervisionInterval = BigEndian::HostSwap16(aChild.GetSupervisionInterval());
    mLinkMargin          = aChild.GetLinkInfo().GetLinkMargin();
    mAverageRssi         = aChild.GetLinkInfo().GetAverageRss();
    mLastRssi            = aChild.GetLinkInfo().GetLastRss();
    mFrameErrorRate      = BigEndian::HostSwap16(aChild.GetLinkInfo().GetFrameErrorRate());
    mMessageErrorRate    = BigEndian::HostSwap16(aChild.GetLinkInfo().GetMessageErrorRate());
    mQueuedMessageCount  = BigEndian::HostSwap16(aChild.GetIndirectMessageCount());

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    mFlags |= aChild.IsCslSynchronized() ? kFlagsCslSync : 0;
    mCslPeriod  = BigEndian::HostSwap16(aChild.GetCslPeriod());
    mCslTimeout = BigEndian::HostSwap32(aChild.GetCslTimeout());
    mCslChannel = aChild.GetCslChannel();
#endif
}

void RouterNeighborTlv::InitFrom(const Router &aRouter)
{
    Clear();

    SetType(kRouterNeighbor);
    SetLength(sizeof(*this) - sizeof(Tlv));

    mFlags |= kFlagsTrackErrRate;
    mRloc16           = BigEndian::HostSwap16(aRouter.GetRloc16());
    mExtAddress       = aRouter.GetExtAddress();
    mVersion          = BigEndian::HostSwap16(aRouter.GetVersion());
    mConnectionTime   = BigEndian::HostSwap32(aRouter.GetConnectionTime());
    mLinkMargin       = aRouter.GetLinkInfo().GetLinkMargin();
    mAverageRssi      = aRouter.GetLinkInfo().GetAverageRss();
    mLastRssi         = aRouter.GetLinkInfo().GetLastRss();
    mFrameErrorRate   = BigEndian::HostSwap16(aRouter.GetLinkInfo().GetFrameErrorRate());
    mMessageErrorRate = BigEndian::HostSwap16(aRouter.GetLinkInfo().GetMessageErrorRate());
}

#endif // OPENTHREAD_FTD

void AnswerTlv::Init(uint16_t aIndex, bool aIsLast)
{
    SetType(kAnswer);
    SetLength(sizeof(*this) - sizeof(Tlv));

    SetFlagsIndex((aIndex & kIndexMask) | (aIsLast ? kIsLastFlag : 0));
}

void MleCountersTlv::Init(const Mle::Counters &aMleCounters)
{
    SetType(kMleCounters);
    SetLength(sizeof(*this) - sizeof(Tlv));

    mDisabledRole                  = BigEndian::HostSwap16(aMleCounters.mDisabledRole);
    mDetachedRole                  = BigEndian::HostSwap16(aMleCounters.mDetachedRole);
    mChildRole                     = BigEndian::HostSwap16(aMleCounters.mChildRole);
    mRouterRole                    = BigEndian::HostSwap16(aMleCounters.mRouterRole);
    mLeaderRole                    = BigEndian::HostSwap16(aMleCounters.mLeaderRole);
    mAttachAttempts                = BigEndian::HostSwap16(aMleCounters.mAttachAttempts);
    mPartitionIdChanges            = BigEndian::HostSwap16(aMleCounters.mPartitionIdChanges);
    mBetterPartitionAttachAttempts = BigEndian::HostSwap16(aMleCounters.mBetterPartitionAttachAttempts);
    mParentChanges                 = BigEndian::HostSwap16(aMleCounters.mParentChanges);
    mTrackedTime                   = BigEndian::HostSwap64(aMleCounters.mTrackedTime);
    mDisabledTime                  = BigEndian::HostSwap64(aMleCounters.mDisabledTime);
    mDetachedTime                  = BigEndian::HostSwap64(aMleCounters.mDetachedTime);
    mChildTime                     = BigEndian::HostSwap64(aMleCounters.mChildTime);
    mRouterTime                    = BigEndian::HostSwap64(aMleCounters.mRouterTime);
    mLeaderTime                    = BigEndian::HostSwap64(aMleCounters.mLeaderTime);
}

void MleCountersTlv::Read(MleCounters &aDiagMleCounters) const
{
    aDiagMleCounters.mDisabledRole                  = BigEndian::HostSwap16(mDisabledRole);
    aDiagMleCounters.mDetachedRole                  = BigEndian::HostSwap16(mDetachedRole);
    aDiagMleCounters.mChildRole                     = BigEndian::HostSwap16(mChildRole);
    aDiagMleCounters.mRouterRole                    = BigEndian::HostSwap16(mRouterRole);
    aDiagMleCounters.mLeaderRole                    = BigEndian::HostSwap16(mLeaderRole);
    aDiagMleCounters.mAttachAttempts                = BigEndian::HostSwap16(mAttachAttempts);
    aDiagMleCounters.mPartitionIdChanges            = BigEndian::HostSwap16(mPartitionIdChanges);
    aDiagMleCounters.mBetterPartitionAttachAttempts = BigEndian::HostSwap16(mBetterPartitionAttachAttempts);
    aDiagMleCounters.mParentChanges                 = BigEndian::HostSwap16(mParentChanges);
    aDiagMleCounters.mTrackedTime                   = BigEndian::HostSwap64(mTrackedTime);
    aDiagMleCounters.mDisabledTime                  = BigEndian::HostSwap64(mDisabledTime);
    aDiagMleCounters.mDetachedTime                  = BigEndian::HostSwap64(mDetachedTime);
    aDiagMleCounters.mChildTime                     = BigEndian::HostSwap64(mChildTime);
    aDiagMleCounters.mRouterTime                    = BigEndian::HostSwap64(mRouterTime);
    aDiagMleCounters.mLeaderTime                    = BigEndian::HostSwap64(mLeaderTime);
}

} // namespace NetworkDiagnostic
} // namespace ot
