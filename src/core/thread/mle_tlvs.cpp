/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file implements function for generating and processing MLE TLVs.
 */

#include "mle_tlvs.hpp"

#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/numeric_limits.hpp"
#include "radio/radio.hpp"

namespace ot {
namespace Mle {

//---------------------------------------------------------------------------------------------------------------------
// RouteTlv

#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

void RouteTlv::Init(void)
{
    SetType(kRoute);
    SetLength(sizeof(*this) - sizeof(Tlv));
    mRouterIdMask.Clear();
    ClearAllBytes(mRouteData);
}

bool RouteTlv::IsValid(void) const
{
    bool    isValid = false;
    uint8_t numAllocatedIds;

    VerifyOrExit(GetLength() >= sizeof(mRouterIdSequence) + sizeof(mRouterIdMask));

    numAllocatedIds = mRouterIdMask.GetNumberOfAllocatedIds();
    VerifyOrExit(numAllocatedIds <= kMaxRouters);

    isValid = (GetRouteDataLength() >= numAllocatedIds);

exit:
    return isValid;
}

#endif // #if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// ConnectivityTlvValue

void ConnectivityTlvValue::InitFrom(const Connectivity &aConnectivity)
{
    mFlags = 0;

    WriteBits<uint8_t, kFlagsParentPriorityMask>(mFlags, Preference::To2BitUint(aConnectivity.GetParentPriority()));
    mLinkQuality3     = aConnectivity.GetNumLinkQuality3();
    mLinkQuality2     = aConnectivity.GetNumLinkQuality2();
    mLinkQuality1     = aConnectivity.GetNumLinkQuality1();
    mLeaderCost       = aConnectivity.GetLeaderCost();
    mIdSequence       = aConnectivity.GetIdSequence();
    mActiveRouters    = aConnectivity.GetActiveRouterCount();
    mSedBufferSize    = BigEndian::HostSwap16(aConnectivity.GetSedBufferSize());
    mSedDatagramCount = aConnectivity.GetSedDatagramCount();
}

void ConnectivityTlvValue::GetConnectivity(Connectivity &aConnectivity) const
{
    aConnectivity.Clear();

    aConnectivity.mParentPriority   = Preference::From2BitUint(ReadBits<uint8_t, kFlagsParentPriorityMask>(mFlags));
    aConnectivity.mLinkQuality3     = mLinkQuality3;
    aConnectivity.mLinkQuality2     = mLinkQuality2;
    aConnectivity.mLinkQuality1     = mLinkQuality1;
    aConnectivity.mLeaderCost       = mLeaderCost;
    aConnectivity.mIdSequence       = mIdSequence;
    aConnectivity.mActiveRouters    = mActiveRouters;
    aConnectivity.mSedBufferSize    = BigEndian::HostSwap16(mSedBufferSize);
    aConnectivity.mSedDatagramCount = mSedDatagramCount;
}

Error ConnectivityTlvValue::ParseFrom(const Message &aMessage, const OffsetRange &aOffsetRange)
{
    Error    error = kErrorNone;
    uint16_t size  = aOffsetRange.GetLength();

    // The `mSedBufferSize` and `mSedDatagramCount` fields are
    // optional and are included as a pair. If the received TLV size
    // indicates they are not present, we read the partial TLV value
    // and then set the fields to their default minimum values.
    // Otherwise, we read the full structure.

    if (size == kMinSize)
    {
        SuccessOrExit(error = aMessage.Read(aOffsetRange, this, size));

        mSedBufferSize    = BigEndian::HostSwap16(kMinSedBufferSize);
        mSedDatagramCount = kMinSedDatagramCount;
    }
    else
    {
        SuccessOrExit(error = aMessage.Read(aOffsetRange, *this));
    }

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// ChannelTlvValue

void ChannelTlvValue::SetChannelAndPage(uint16_t aChannel)
{
    uint8_t channelPage = OT_RADIO_CHANNEL_PAGE_0;

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    if ((OT_RADIO_915MHZ_OQPSK_CHANNEL_MIN <= aChannel) && (aChannel <= OT_RADIO_915MHZ_OQPSK_CHANNEL_MAX))
    {
        channelPage = OT_RADIO_CHANNEL_PAGE_2;
    }
#endif

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT
    if ((OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MIN == aChannel) ||
        ((OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MIN < aChannel) &&
         (aChannel <= OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MAX)))
    {
        channelPage = OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_PAGE;
    }
#endif

    SetChannelPage(channelPage);
    SetChannel(aChannel);
}

bool ChannelTlvValue::IsValid(void) const
{
    bool     isValid = false;
    uint16_t channel;

    VerifyOrExit(Radio::SupportsChannelPage(mChannelPage));

    channel = GetChannel();
    VerifyOrExit((Radio::kChannelMin <= channel) && (channel <= Radio::kChannelMax));

    isValid = true;

exit:
    return isValid;
}

//---------------------------------------------------------------------------------------------------------------------
// LeaderDataTlvValue

LeaderDataTlvValue::LeaderDataTlvValue(const LeaderData &aLeaderData)
    : mPartitionId(BigEndian::HostSwap32(aLeaderData.GetPartitionId()))
    , mWeighting(aLeaderData.GetWeighting())
    , mDataVersion(aLeaderData.GetDataVersion(NetworkData::kFullSet))
    , mStableDataVersion(aLeaderData.GetDataVersion(NetworkData::kStableSubset))
    , mLeaderRouterId(aLeaderData.GetLeaderRouterId())
{
}

void LeaderDataTlvValue::Get(LeaderData &aLeaderData) const
{
    aLeaderData.SetPartitionId(BigEndian::HostSwap32(mPartitionId));
    aLeaderData.SetWeighting(mWeighting);
    aLeaderData.SetDataVersion(mDataVersion);
    aLeaderData.SetStableDataVersion(mStableDataVersion);
    aLeaderData.SetLeaderRouterId(mLeaderRouterId);
}

} // namespace Mle
} // namespace ot
