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
 *   This file includes implementations for ICMPv6 Router Advertisement.
 *
 */

#include "border_router/router_advertisement.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"

namespace ot {
namespace BorderRouter {
namespace RouterAdv {

const Option *Option::GetNextOption(const Option *aCurOption, const uint8_t *aBuffer, uint16_t aBufferLength)
{
    const uint8_t *nextOption = nullptr;
    const uint8_t *bufferEnd  = aBuffer + aBufferLength;

    VerifyOrExit(aBuffer != nullptr, nextOption = nullptr);

    if (aCurOption == nullptr)
    {
        nextOption = aBuffer;
    }
    else
    {
        nextOption = reinterpret_cast<const uint8_t *>(aCurOption) + aCurOption->GetSize();
    }

    VerifyOrExit(nextOption + sizeof(Option) <= bufferEnd, nextOption = nullptr);
    VerifyOrExit(reinterpret_cast<const Option *>(nextOption)->GetSize() > 0, nextOption = nullptr);
    VerifyOrExit(nextOption + reinterpret_cast<const Option *>(nextOption)->GetSize() <= bufferEnd,
                 nextOption = nullptr);

exit:
    return reinterpret_cast<const Option *>(nextOption);
}

//----------------------------------------------------------------------------------------------------------------------
// PrefixInfoOption

void PrefixInfoOption::Init(void)
{
    Clear();
    SetType(Type::kPrefixInfo);
    SetSize(sizeof(PrefixInfoOption));

    OT_UNUSED_VARIABLE(mReserved2);
}

void PrefixInfoOption::SetPrefix(const Ip6::Prefix &aPrefix)
{
    mPrefixLength = aPrefix.mLength;
    mPrefix       = AsCoreType(&aPrefix.mPrefix);
}

void PrefixInfoOption::GetPrefix(Ip6::Prefix &aPrefix) const
{
    aPrefix.Set(mPrefix.GetBytes(), mPrefixLength);
}

bool PrefixInfoOption::IsValid(void) const
{
    return (GetSize() >= sizeof(*this)) && (mPrefixLength <= Ip6::Prefix::kMaxLength) &&
           (GetPreferredLifetime() <= GetValidLifetime());
}

//----------------------------------------------------------------------------------------------------------------------
// RouteInfoOption

void RouteInfoOption::Init(void)
{
    Clear();
    SetType(Type::kRouteInfo);
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

void RouteInfoOption::SetPrefix(const Ip6::Prefix &aPrefix)
{
    SetLength(OptionLengthForPrefix(aPrefix.mLength));
    mPrefixLength = aPrefix.mLength;
    memcpy(GetPrefixBytes(), aPrefix.GetBytes(), aPrefix.GetBytesSize());
}

void RouteInfoOption::GetPrefix(Ip6::Prefix &aPrefix) const
{
    aPrefix.Set(GetPrefixBytes(), mPrefixLength);
}

bool RouteInfoOption::IsValid(void) const
{
    return (GetSize() >= kMinSize) && (mPrefixLength <= Ip6::Prefix::kMaxLength) &&
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
// RouterAdvMessage

void RouterAdvMessage::SetToDefault(void)
{
    OT_UNUSED_VARIABLE(mCode);
    OT_UNUSED_VARIABLE(mCurHopLimit);
    OT_UNUSED_VARIABLE(mReachableTime);
    OT_UNUSED_VARIABLE(mRetransTimer);

    Clear();
    mType = Ip6::Icmp::Header::kTypeRouterAdvert;
}

RoutePreference RouterAdvMessage::GetDefaultRouterPreference(void) const
{
    return NetworkData::RoutePreferenceFromValue((mFlags & kPreferenceMask) >> kPreferenceOffset);
}

void RouterAdvMessage::SetDefaultRouterPreference(RoutePreference aPreference)
{
    mFlags &= ~kPreferenceMask;
    mFlags |= (NetworkData::RoutePreferenceToValue(aPreference) << kPreferenceOffset) & kPreferenceMask;
}

//----------------------------------------------------------------------------------------------------------------------
// RouterAdvMessage

RouterSolicitMessage::RouterSolicitMessage(void)
{
    mHeader.Clear();
    mHeader.SetType(Ip6::Icmp::Header::kTypeRouterSolicit);
}

} // namespace RouterAdv
} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
