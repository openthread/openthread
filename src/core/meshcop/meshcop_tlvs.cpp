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
 *   This file implements MechCop TLV helper functions.
 */

#include "meshcop_tlvs.hpp"

#include "common/const_cast.hpp"
#include "common/debug.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"
#include "common/string.hpp"
#include "meshcop/meshcop.hpp"

namespace ot {
namespace MeshCoP {

bool Tlv::IsValid(const Tlv &aTlv)
{
    bool    isValid   = true;
    uint8_t minLength = 0;

    switch (aTlv.GetType())
    {
    case Tlv::kPanId:
        minLength = sizeof(PanIdTlv::UintValueType);
        break;
    case Tlv::kExtendedPanId:
        minLength = sizeof(ExtendedPanIdTlv::ValueType);
        break;
    case Tlv::kPskc:
        minLength = sizeof(PskcTlv::ValueType);
        break;
    case Tlv::kNetworkKey:
        minLength = sizeof(NetworkKeyTlv::ValueType);
        break;
    case Tlv::kMeshLocalPrefix:
        minLength = sizeof(MeshLocalPrefixTlv::ValueType);
        break;
    case Tlv::kChannel:
        VerifyOrExit(aTlv.GetLength() >= sizeof(ChannelTlvValue), isValid = false);
        isValid = aTlv.ReadValueAs<ChannelTlv>().IsValid();
        break;
    case Tlv::kNetworkName:
        isValid = As<NetworkNameTlv>(aTlv).IsValid();
        break;

    case Tlv::kSecurityPolicy:
        isValid = As<SecurityPolicyTlv>(aTlv).IsValid();
        break;

    case Tlv::kChannelMask:
        isValid = As<ChannelMaskTlv>(aTlv).IsValid();
        break;

    default:
        break;
    }

    if (minLength > 0)
    {
        isValid = (aTlv.GetLength() >= minLength);
    }

exit:
    return isValid;
}

NameData NetworkNameTlv::GetNetworkName(void) const
{
    uint8_t len = GetLength();

    if (len > sizeof(mNetworkName))
    {
        len = sizeof(mNetworkName);
    }

    return NameData(mNetworkName, len);
}

void NetworkNameTlv::SetNetworkName(const NameData &aNameData)
{
    uint8_t len;

    len = aNameData.CopyTo(mNetworkName, sizeof(mNetworkName));
    SetLength(len);
}

bool NetworkNameTlv::IsValid(void) const { return IsValidUtf8String(mNetworkName, GetLength()); }

void SteeringDataTlv::CopyTo(SteeringData &aSteeringData) const
{
    aSteeringData.Init(GetSteeringDataLength());
    memcpy(aSteeringData.GetData(), mSteeringData, GetSteeringDataLength());
}

bool SecurityPolicyTlv::IsValid(void) const
{
    return GetLength() >= sizeof(mRotationTime) && GetFlagsLength() >= kThread11FlagsLength;
}

SecurityPolicy SecurityPolicyTlv::GetSecurityPolicy(void) const
{
    SecurityPolicy securityPolicy;
    uint8_t        length = Min(static_cast<uint8_t>(sizeof(mFlags)), GetFlagsLength());

    securityPolicy.mRotationTime = GetRotationTime();
    securityPolicy.SetFlags(mFlags, length);

    return securityPolicy;
}

void SecurityPolicyTlv::SetSecurityPolicy(const SecurityPolicy &aSecurityPolicy)
{
    SetRotationTime(aSecurityPolicy.mRotationTime);
    aSecurityPolicy.GetFlags(mFlags, sizeof(mFlags));
}

const char *StateTlv::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Pending", // (0) kPending,
        "Accept",  // (1) kAccept
        "Reject",  // (2) kReject,
    };

    static_assert(0 == kPending, "kPending value is incorrect");
    static_assert(1 == kAccept, "kAccept value is incorrect");

    return aState == kReject ? kStateStrings[2] : kStateStrings[aState];
}

bool ChannelMaskTlv::IsValid(void) const
{
    uint32_t channelMask;

    return (ReadChannelMask(channelMask) == kErrorNone);
}

Error ChannelMaskTlv::ReadChannelMask(uint32_t &aChannelMask) const
{
    EntriesData entriesData;

    entriesData.Clear();
    entriesData.mData   = &mEntriesStart;
    entriesData.mLength = GetLength();

    return entriesData.Parse(aChannelMask);
}

Error ChannelMaskTlv::FindIn(const Message &aMessage, uint32_t &aChannelMask)
{
    Error       error;
    EntriesData entriesData;

    entriesData.Clear();
    entriesData.mMessage = &aMessage;

    SuccessOrExit(error = FindTlvValueOffset(aMessage, Tlv::kChannelMask, entriesData.mOffset, entriesData.mLength));
    error = entriesData.Parse(aChannelMask);

exit:
    return error;
}

Error ChannelMaskTlv::EntriesData::Parse(uint32_t &aChannelMask)
{
    // Validates and parses the Channel Mask TLV entries for each
    // channel page and if successful updates `aChannelMask` to
    // return the combined mask for all channel pages supported by
    // radio. The entries can be either contained in `mMessage` from
    // `mOffset` (when `mMessage` is non-null) or be in a buffer
    // `mData`. `mLength` gives the number of bytes for all entries.

    Error        error = kErrorParse;
    Entry        readEntry;
    const Entry *entry;
    uint16_t     size;

    aChannelMask = 0;

    VerifyOrExit(mLength > 0); // At least one entry.

    while (mLength > 0)
    {
        VerifyOrExit(mLength > kEntryHeaderSize);

        if (mMessage != nullptr)
        {
            // We first read the entry's header only and after
            // validating the entry and that the entry's channel page
            // is supported by radio, we read the full `Entry`.

            mMessage->ReadBytes(mOffset, &readEntry, kEntryHeaderSize);
            entry = &readEntry;
        }
        else
        {
            entry = reinterpret_cast<const Entry *>(mData);
        }

        size = kEntryHeaderSize + entry->GetMaskLength();

        VerifyOrExit(size <= mLength);

        if (Radio::SupportsChannelPage(entry->GetChannelPage()))
        {
            // Currently supported channel pages all use `uint32_t`
            // channel mask.

            VerifyOrExit(entry->GetMaskLength() == kMaskLength);

            if (mMessage != nullptr)
            {
                IgnoreError(mMessage->Read(mOffset, readEntry));
            }

            aChannelMask |= (entry->GetMask() & Radio::ChannelMaskForPage(entry->GetChannelPage()));
        }

        mLength -= size;

        if (mMessage != nullptr)
        {
            mOffset += size;
        }
        else
        {
            mData += size;
        }
    }

    error = kErrorNone;

exit:
    return error;
}

void ChannelMaskTlv::PrepareValue(Value &aValue, uint32_t aChannelMask)
{
    Entry *entry = reinterpret_cast<Entry *>(aValue.mData);

    aValue.mLength = 0;

    for (uint8_t page : Radio::kSupportedChannelPages)
    {
        uint32_t mask = (Radio::ChannelMaskForPage(page) & aChannelMask);

        if (mask != 0)
        {
            entry->SetChannelPage(page);
            entry->SetMaskLength(kMaskLength);
            entry->SetMask(mask);

            aValue.mLength += sizeof(Entry);
            entry++;
        }
    }
}

Error ChannelMaskTlv::AppendTo(Message &aMessage, uint32_t aChannelMask)
{
    Value value;

    PrepareValue(value, aChannelMask);
    return Tlv::Append<ChannelMaskTlv>(aMessage, value.mData, value.mLength);
}

} // namespace MeshCoP
} // namespace ot
