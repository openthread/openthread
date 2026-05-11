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

Error RouteTlv::Data::ParseFrom(const Message &aMessage, const OffsetRange &aOffsetRange)
{
    Error        error;
    RouterIdMask routerIdMask;
    OffsetRange  offsetRange = aOffsetRange;
#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    bool isEven = true;
#endif

    SuccessOrExit(error = aMessage.Read(offsetRange, routerIdMask));
    offsetRange.AdvanceOffset(sizeof(routerIdMask));

    mIdSequence = routerIdMask.GetSequence();

    mEntries.Clear();

    for (uint8_t routerId = 0; routerId <= kMaxRouterId; routerId++)
    {
        Entry *entry;

        if (!routerIdMask.IsAllocated(routerId))
        {
            continue;
        }

        entry = mEntries.PushBack();

        // If `mEntries` is full, it indicates that there are more than
        // `kMaxRouters` allocated IDs in the mask, which makes it invalid.

        VerifyOrExit(entry != nullptr, error = kErrorParse);

        entry->mRouterId = routerId;

#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
        SuccessOrExit(error = aMessage.Read<uint8_t>(offsetRange, entry->mRouteData));
        offsetRange.AdvanceOffset(sizeof(uint8_t));
#else
        {
            EntryType value;

            SuccessOrExit(error = aMessage.Read<EntryType>(offsetRange, value));
            value = BigEndian::HostSwap16(value);

            if (isEven)
            {
                entry->mRouteData = ReadBits<uint16_t, kEvenEntryMask>(value);
                offsetRange.AdvanceOffset(sizeof(uint8_t));
            }
            else
            {
                entry->mRouteData = ReadBits<uint16_t, kOddEntryMask>(value);
                offsetRange.AdvanceOffset(sizeof(uint16_t));
            }

            isEven = !isEven;
        }
#endif
    }

exit:
    return error;
}

bool RouteTlv::Data::IsAllocated(uint8_t aRouterId) const { return mEntries.FindMatching(aRouterId); }

void RouteTlv::Data::DetermineRouterIdMask(RouterIdMask &aRouterIdMask) const
{
    aRouterIdMask.Clear();

    aRouterIdMask.SetSequence(mIdSequence);

    for (const Entry &entry : mEntries)
    {
        aRouterIdMask.Add(entry.mRouterId);
    }
}

Error RouteTlv::AppendRouteDataEntry(Message    &aMessage,
                                     LinkQuality aLqIn,
                                     LinkQuality aLqOut,
                                     uint8_t     aRouteCost
#if OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
                                     ,
                                     bool aIsEven
#endif
)
{
    Error     error;
    EntryType entry = 0;

    if (aRouteCost >= kMaxRouteCost)
    {
        aRouteCost = 0;
    }

    WriteBits<EntryType, kLinkQualityOutMask>(entry, aLqOut);
    WriteBits<EntryType, kLinkQualityInMask>(entry, aLqIn);
    WriteBits<EntryType, kRouteCostMask>(entry, aRouteCost);

#if !OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE
    ExitNow(error = aMessage.Append<uint8_t>(entry));
#else
    {
        // Under `OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE`, each route
        // data entry uses 1.5 bytes (12 bits). Two entries are packed
        // into 3 bytes.
        //
        // For the even (first) entry, we grow the message by 2 bytes
        // and write the 12 bits into the upper 12 bits of the new
        // 16-bit word at the end of the message.
        //
        // For the odd (second) entry, we grow the message by 1 byte. We
        // then read the last 16 bits (which overlap with the last byte
        // of the even entry), write the new 12-bit entry into the
        // lower 12 bits, and write the 16-bit word back. This
        // perfectly packs the two 12-bit entries into 3 bytes.

        uint16_t offset;
        uint16_t data;

        SuccessOrExit(error = aMessage.IncreaseLength(aIsEven ? sizeof(uint16_t) : sizeof(uint8_t)));

        VerifyOrExit(aMessage.GetLength() >= sizeof(uint16_t), error = kErrorParse);
        offset = aMessage.GetLength() - sizeof(uint16_t);

        if (aIsEven)
        {
            data = 0;
            WriteBits<uint16_t, kEvenEntryMask>(data, entry);
        }
        else
        {
            IgnoreError(aMessage.Read<uint16_t>(offset, data));
            data = BigEndian::HostSwap16(data);

            WriteBits<uint16_t, kOddEntryMask>(data, entry);
        }

        aMessage.Write<uint16_t>(offset, BigEndian::HostSwap16(data));
    }
#endif // OPENTHREAD_CONFIG_MLE_LONG_ROUTES_ENABLE

exit:
    return error;
}

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

//---------------------------------------------------------------------------------------------------------------------
// CslClockAccuracyTlvValue

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

CslClockAccuracyTlvValue::CslClockAccuracyTlvValue(uint8_t aClockAccuracy, uint8_t aUncertainty)
    : mClockAccuracy(aClockAccuracy)
    , mUncertainty(aUncertainty)
{
}

void CslClockAccuracyTlvValue::Get(Mac::CslAccuracy &aAccuracy) const
{
    aAccuracy.SetClockAccuracy(mClockAccuracy);
    aAccuracy.SetUncertainty(mUncertainty);
}

#endif

} // namespace Mle
} // namespace ot
