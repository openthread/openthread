/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes implementations for IPv6 Neighbor Discovery (ND6).
 *
 */

#include "nd6.hpp"

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"

namespace ot {
namespace Ip6 {
namespace Nd {

//----------------------------------------------------------------------------------------------------------------------
// Option::Iterator

Option::Iterator::Iterator(void)
    : mOption(nullptr)
    , mEnd(nullptr)
{
    // An empty iterator (used to indicate `end()` of list).
}

Option::Iterator::Iterator(const void *aStart, const void *aEnd)
    : mOption(nullptr)
    , mEnd(reinterpret_cast<const Option *>(aEnd))
{
    // Note that `Validate()` uses `mEnd` so can only be called after
    // `mEnd` is set.

    mOption = Validate(reinterpret_cast<const Option *>(aStart));
}

const Option *Option::Iterator::Next(const Option *aOption)
{
    return reinterpret_cast<const Option *>(reinterpret_cast<const uint8_t *>(aOption) + aOption->GetSize());
}

void Option::Iterator::Advance(void) { mOption = (mOption != nullptr) ? Validate(Next(mOption)) : nullptr; }

const Option *Option::Iterator::Validate(const Option *aOption) const
{
    // Check if `aOption` is well-formed and fits in the range
    // up to `mEnd`. Returns `aOption` if it is valid, `nullptr`
    // otherwise.

    return ((aOption != nullptr) && ((aOption + 1) <= mEnd) && aOption->IsValid() && (Next(aOption) <= mEnd)) ? aOption
                                                                                                              : nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
// PrefixInfoOption

void PrefixInfoOption::Init(void)
{
    Clear();
    SetType(kTypePrefixInfo);
    SetSize(sizeof(PrefixInfoOption));

    OT_UNUSED_VARIABLE(mReserved2);
}

void PrefixInfoOption::SetPrefix(const Prefix &aPrefix)
{
    mPrefixLength = aPrefix.mLength;
    mPrefix       = AsCoreType(&aPrefix.mPrefix);
}

void PrefixInfoOption::GetPrefix(Prefix &aPrefix) const { aPrefix.Set(mPrefix.GetBytes(), mPrefixLength); }

bool PrefixInfoOption::IsValid(void) const
{
    return (GetSize() >= sizeof(*this)) && (mPrefixLength <= Prefix::kMaxLength) &&
           (GetPreferredLifetime() <= GetValidLifetime());
}

//----------------------------------------------------------------------------------------------------------------------
// RouteInfoOption

void RouteInfoOption::Init(void)
{
    Clear();
    SetType(kTypeRouteInfo);
}

void RouteInfoOption::SetPreference(RoutePreference aPreference)
{
    mResvdPrf &= ~kPreferenceMask;
    mResvdPrf |= (NetworkData::RoutePreferenceToValue(aPreference) << kPreferenceOffset) & kPreferenceMask;
}

RoutePreference RouteInfoOption::GetPreference(void) const
{
    return NetworkData::RoutePreferenceFromValue((mResvdPrf & kPreferenceMask) >> kPreferenceOffset);
}

void RouteInfoOption::SetPrefix(const Prefix &aPrefix)
{
    SetLength(OptionLengthForPrefix(aPrefix.mLength));
    mPrefixLength = aPrefix.mLength;
    memcpy(GetPrefixBytes(), aPrefix.GetBytes(), aPrefix.GetBytesSize());
}

void RouteInfoOption::GetPrefix(Prefix &aPrefix) const { aPrefix.Set(GetPrefixBytes(), mPrefixLength); }

bool RouteInfoOption::IsValid(void) const
{
    return (GetSize() >= kMinSize) && (mPrefixLength <= Prefix::kMaxLength) &&
           (GetLength() >= OptionLengthForPrefix(mPrefixLength)) &&
           NetworkData::IsRoutePreferenceValid(GetPreference());
}

uint8_t RouteInfoOption::OptionLengthForPrefix(uint8_t aPrefixLength)
{
    static constexpr uint8_t kMaxPrefixLenForOptionLen1 = 0;
    static constexpr uint8_t kMaxPrefixLenForOptionLen2 = 64;

    uint8_t length;

    // The Option Length can be 1, 2, or 3 depending on the prefix
    // length
    //
    // - 1 when prefix len is zero.
    // - 2 when prefix len is less then or equal to 64.
    // - 3 otherwise.

    if (aPrefixLength == kMaxPrefixLenForOptionLen1)
    {
        length = 1;
    }
    else if (aPrefixLength <= kMaxPrefixLenForOptionLen2)
    {
        length = 2;
    }
    else
    {
        length = 3;
    }

    return length;
}

//----------------------------------------------------------------------------------------------------------------------
// RaFlagsExtOption

void RaFlagsExtOption::Init(void)
{
    Clear();
    SetType(kTypeRaFlagsExtension);
    SetSize(sizeof(RaFlagsExtOption));
}

//----------------------------------------------------------------------------------------------------------------------
// RouterAdverMessage::Header

void RouterAdvertMessage::Header::SetToDefault(void)
{
    OT_UNUSED_VARIABLE(mCode);
    OT_UNUSED_VARIABLE(mCurHopLimit);
    OT_UNUSED_VARIABLE(mReachableTime);
    OT_UNUSED_VARIABLE(mRetransTimer);

    Clear();
    mType = Icmp::Header::kTypeRouterAdvert;
}

RoutePreference RouterAdvertMessage::Header::GetDefaultRouterPreference(void) const
{
    return NetworkData::RoutePreferenceFromValue((mFlags & kPreferenceMask) >> kPreferenceOffset);
}

void RouterAdvertMessage::Header::SetDefaultRouterPreference(RoutePreference aPreference)
{
    mFlags &= ~kPreferenceMask;
    mFlags |= (NetworkData::RoutePreferenceToValue(aPreference) << kPreferenceOffset) & kPreferenceMask;
}

//----------------------------------------------------------------------------------------------------------------------
// RouterAdverMessage

Option *RouterAdvertMessage::AppendOption(uint16_t aOptionSize)
{
    // This method appends an option with a given size to the RA
    // message by reserving space in the data buffer if there is
    // room. On success returns pointer to the option, on failure
    // returns `nullptr`. The returned option needs to be
    // initialized and populated by the caller.

    Option  *option    = nullptr;
    uint32_t newLength = mData.GetLength();

    newLength += aOptionSize;
    VerifyOrExit(newLength <= mMaxLength);

    option = reinterpret_cast<Option *>(AsNonConst(GetDataEnd()));
    mData.SetLength(static_cast<uint16_t>(newLength));

exit:
    return option;
}

Error RouterAdvertMessage::AppendPrefixInfoOption(const Prefix &aPrefix,
                                                  uint32_t      aValidLifetime,
                                                  uint32_t      aPreferredLifetime)
{
    Error             error = kErrorNone;
    PrefixInfoOption *pio;

    pio = static_cast<PrefixInfoOption *>(AppendOption(sizeof(PrefixInfoOption)));
    VerifyOrExit(pio != nullptr, error = kErrorNoBufs);

    pio->Init();
    pio->SetOnLinkFlag();
    pio->SetAutoAddrConfigFlag();
    pio->SetValidLifetime(aValidLifetime);
    pio->SetPreferredLifetime(aPreferredLifetime);
    pio->SetPrefix(aPrefix);

exit:
    return error;
}

Error RouterAdvertMessage::AppendRouteInfoOption(const Prefix   &aPrefix,
                                                 uint32_t        aRouteLifetime,
                                                 RoutePreference aPreference)
{
    Error            error = kErrorNone;
    RouteInfoOption *rio;

    rio = static_cast<RouteInfoOption *>(AppendOption(RouteInfoOption::OptionSizeForPrefix(aPrefix.GetLength())));
    VerifyOrExit(rio != nullptr, error = kErrorNoBufs);

    rio->Init();
    rio->SetRouteLifetime(aRouteLifetime);
    rio->SetPreference(aPreference);
    rio->SetPrefix(aPrefix);

exit:
    return error;
}

Error RouterAdvertMessage::AppendFlagsExtensionOption(bool aStubRouterFlag)
{
    Error             error = kErrorNone;
    RaFlagsExtOption *flagsOption;

    flagsOption = static_cast<RaFlagsExtOption *>(AppendOption(sizeof(RaFlagsExtOption)));
    VerifyOrExit(flagsOption != nullptr, error = kErrorNoBufs);

    flagsOption->Init();

    if (aStubRouterFlag)
    {
        flagsOption->SetStubRouterFlag();
    }

exit:
    return error;
}

//----------------------------------------------------------------------------------------------------------------------
// RouterSolicitMessage

RouterSolicitMessage::RouterSolicitMessage(void)
{
    mHeader.Clear();
    mHeader.SetType(Icmp::Header::kTypeRouterSolicit);
}

//----------------------------------------------------------------------------------------------------------------------
// NeighborSolicitMessage

NeighborSolicitMessage::NeighborSolicitMessage(void)
{
    OT_UNUSED_VARIABLE(mChecksum);
    OT_UNUSED_VARIABLE(mReserved);

    Clear();
    mType = Icmp::Header::kTypeNeighborSolicit;
}

//----------------------------------------------------------------------------------------------------------------------
// NeighborAdvertMessage

NeighborAdvertMessage::NeighborAdvertMessage(void)
{
    OT_UNUSED_VARIABLE(mChecksum);
    OT_UNUSED_VARIABLE(mReserved);

    Clear();
    mType = Icmp::Header::kTypeNeighborAdvert;
}

} // namespace Nd
} // namespace Ip6
} // namespace ot
