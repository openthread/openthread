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

//---------------------------------------------------------------------------------------------------------------------
// ChildTableTlvEntry

#if OPENTHREAD_FTD

void ChildTableTlvEntry::InitFrom(const Child &aChild)
{
    uint16_t encoded = 0;

    //             1                   0
    //   5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //  |TmoutExp |ILQ|     Child ID    |
    //  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    WriteBits<uint16_t, kTimeoutMask>(encoded, DetermineExponentFromTimeout(aChild.GetTimeout()));
    WriteBits<uint16_t, kIlqMask>(encoded, aChild.GetLinkQualityIn());
    WriteBits<uint16_t, kChildIdMask>(encoded, Mle::ChildIdFromRloc16(aChild.GetRloc16()));

    mTimeoutIlqChildId = BigEndian::HostSwap16(encoded);
    mMode              = aChild.GetDeviceMode().Get();
}

uint8_t ChildTableTlvEntry::DetermineExponentFromTimeout(uint32_t aTimeout)
{
    // The timeout is expressed as `2^(exponent - 4)` seconds. The
    // parent fills the exponent field with the closest
    // ceiling value based on the Child Timeout value.

    uint8_t exponent;

    for (exponent = kTimeoutExponentMin; exponent < kTimeoutExponentMax; exponent++)
    {
        if (DetermineTimeoutFromExponent(exponent) >= aTimeout)
        {
            break;
        }
    }

    return exponent;
}

#endif // OPENTHREAD_FTD

uint32_t ChildTableTlvEntry::DetermineTimeoutFromExponent(uint8_t aExponent)
{
    aExponent = Clamp(aExponent, kTimeoutExponentMin, kTimeoutExponentMax);

    return static_cast<uint32_t>(1U) << (aExponent - kTimeoutExponentMin);
}

void ChildTableTlvEntry::Parse(ParseInfo &aParseInfo) const
{
    uint16_t encoded = BigEndian::HostSwap16(mTimeoutIlqChildId);

    ClearAllBytes(aParseInfo);

    aParseInfo.mTimeout     = ReadBits<uint16_t, kTimeoutMask>(encoded);
    aParseInfo.mLinkQuality = static_cast<LinkQuality>(ReadBits<uint16_t, kIlqMask>(encoded));
    aParseInfo.mChildId     = ReadBits<uint16_t, kChildIdMask>(encoded);
    Mle::DeviceMode(mMode).Get(aParseInfo.mMode);
}

//---------------------------------------------------------------------------------------------------------------------
// RouteTlv

Error RouteTlv::FindIn(const Message &aMessage, RouteTlv::Data &aData)
{
    Error       error;
    OffsetRange offsetRange;

    SuccessOrExit(error = Tlv::FindTlvValueOffsetRange(aMessage, kType, offsetRange));
    error = aData.ParseFrom(aMessage, offsetRange);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// EnhancedRouteTlvEntry

void EnhancedRouteTlvEntry::InitFrom(const Router &aRouter)
{
    uint16_t data = 0;

    if (aRouter.IsStateValid())
    {
        data |= kLinkFlag;
        data |= (static_cast<uint16_t>(aRouter.GetLinkQualityOut()) << kLinkQualityOutOffset);
        data |= (static_cast<uint16_t>(aRouter.GetLinkQualityIn()) << kLinkQualityInOffset);
    }

    data |= ((static_cast<uint16_t>(aRouter.GetNextHop()) & kNextHopMask) << kNextHopOffset);
    data |= ((static_cast<uint16_t>(aRouter.GetCost()) & kCostMask) << kNextHopCostOffset);

    SetRouteData(data);
}

void EnhancedRouteTlvEntry::Parse(ParseInfo &aParseInfo) const
{
    uint16_t data = GetRouteData();

    aParseInfo.mIsSelf         = (data & kSelfFlag);
    aParseInfo.mHasLink        = (data & kLinkFlag);
    aParseInfo.mLinkQualityOut = static_cast<uint8_t>((data >> kLinkQualityOutOffset) & kLinkQualityMask);
    aParseInfo.mLinkQualityIn  = static_cast<uint8_t>((data >> kLinkQualityInOffset) & kLinkQualityMask);
    aParseInfo.mNextHop        = static_cast<uint8_t>((data >> kNextHopOffset) & kNextHopMask);
    aParseInfo.mNextHopCost    = static_cast<uint8_t>((data >> kNextHopCostOffset) & kCostMask);
}

#if OPENTHREAD_FTD

//---------------------------------------------------------------------------------------------------------------------
// ChildTlvValue

void ChildTlvValue::InitFrom(const Child &aChild)
{
    Clear();

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
    mSupervisionInterval = aChild.IsRxOnWhenIdle() ? 0 : BigEndian::HostSwap16(aChild.GetSupervisionInterval());
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

//---------------------------------------------------------------------------------------------------------------------
// RouterNeighborTlvValue

void RouterNeighborTlvValue::InitFrom(const Router &aRouter)
{
    Clear();

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

//---------------------------------------------------------------------------------------------------------------------
// AnswerTlvValue

void AnswerTlvValue::Init(uint16_t aIndex, IsLastFlag aIsLastFlag)
{
    SetFlagsIndex((aIndex & kIndexMask) | (aIsLastFlag == kIsLast ? kIsLastFlag : 0));
}

//---------------------------------------------------------------------------------------------------------------------
// MacCountersTlv

void MacCountersTlv::Init(const Mac::Counters &aMacCounters)
{
    SetType(kMacCounters);
    SetLength(sizeof(*this) - sizeof(Tlv));

    mIfInUnknownProtos  = BigEndian::HostSwap32(aMacCounters.mRxOther);
    mIfInErrors         = BigEndian::HostSwap32(aMacCounters.mRxErrNoFrame + aMacCounters.mRxErrUnknownNeighbor +
                                                aMacCounters.mRxErrInvalidSrcAddr + aMacCounters.mRxErrSec +
                                                aMacCounters.mRxErrFcs + aMacCounters.mRxErrOther);
    mIfOutErrors        = BigEndian::HostSwap32(aMacCounters.mTxErrCca);
    mIfInUcastPkts      = BigEndian::HostSwap32(aMacCounters.mRxUnicast);
    mIfInBroadcastPkts  = BigEndian::HostSwap32(aMacCounters.mRxBroadcast);
    mIfInDiscards       = BigEndian::HostSwap32(aMacCounters.mRxAddressFiltered + aMacCounters.mRxDestAddrFiltered +
                                                aMacCounters.mRxDuplicated);
    mIfOutUcastPkts     = BigEndian::HostSwap32(aMacCounters.mTxUnicast);
    mIfOutBroadcastPkts = BigEndian::HostSwap32(aMacCounters.mTxBroadcast);
    mIfOutDiscards      = BigEndian::HostSwap32(aMacCounters.mTxErrBusyChannel);
}

void MacCountersTlv::Read(MacCounters &aDiagMacCounters) const
{
    aDiagMacCounters.mIfInUnknownProtos  = BigEndian::HostSwap32(mIfInUnknownProtos);
    aDiagMacCounters.mIfInErrors         = BigEndian::HostSwap32(mIfInErrors);
    aDiagMacCounters.mIfOutErrors        = BigEndian::HostSwap32(mIfOutErrors);
    aDiagMacCounters.mIfInUcastPkts      = BigEndian::HostSwap32(mIfInUcastPkts);
    aDiagMacCounters.mIfInBroadcastPkts  = BigEndian::HostSwap32(mIfInBroadcastPkts);
    aDiagMacCounters.mIfInDiscards       = BigEndian::HostSwap32(mIfInDiscards);
    aDiagMacCounters.mIfOutUcastPkts     = BigEndian::HostSwap32(mIfOutUcastPkts);
    aDiagMacCounters.mIfOutBroadcastPkts = BigEndian::HostSwap32(mIfOutBroadcastPkts);
    aDiagMacCounters.mIfOutDiscards      = BigEndian::HostSwap32(mIfOutDiscards);
}

//---------------------------------------------------------------------------------------------------------------------
// MleCountersTlv

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
