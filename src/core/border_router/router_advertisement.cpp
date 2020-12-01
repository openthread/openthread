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
 *   This file includes implementations for IPv6 Router Advertisement.
 *
 */

#include "border_router/router_advertisement.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <memory.h>

namespace ot {

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
        nextOption = reinterpret_cast<const uint8_t *>(aCurOption) + aCurOption->GetLength();
    }

    VerifyOrExit(nextOption + sizeof(Option) <= bufferEnd, nextOption = nullptr);
    VerifyOrExit(nextOption + reinterpret_cast<const Option *>(nextOption)->GetLength() <= bufferEnd,
                 nextOption = nullptr);

exit:
    return reinterpret_cast<const Option *>(nextOption);
}

PrefixInfoOption::PrefixInfoOption()
    : Option(Type::kPrefixInfo, 4)
    , mPrefixLength(0)
    , mReserved1(0)
    , mValidLifetime(0)
    , mPreferredLifetime(0)
    , mReserved2(0)
{
    OT_UNUSED_VARIABLE(mReserved2);
    memset(mPrefix, 0, sizeof(mPrefix));
}

void PrefixInfoOption::SetOnLink(bool aOnLink)
{
    if (aOnLink)
    {
        mReserved1 |= kOnLinkFlagMask;
    }
    else
    {
        mReserved1 &= ~kOnLinkFlagMask;
    }
}

void PrefixInfoOption::SetAutoAddrConfig(bool aAutoAddrConfig)
{
    if (aAutoAddrConfig)
    {
        mReserved1 |= kAutoConfigFlagMask;
    }
    else
    {
        mReserved1 &= ~kAutoConfigFlagMask;
    }
}

void PrefixInfoOption::SetPrefix(const Ip6::Prefix &aPrefix)
{
    mPrefixLength = aPrefix.mLength;
    memcpy(mPrefix, &aPrefix.mPrefix, sizeof(aPrefix.mPrefix));
}

RouteInfoOption::RouteInfoOption()
    : Option(Type::kRouteInfo, 0)
    , mPrefixLength(0)
    , mReserved(0)
    , mRouteLifetime(0)
{
    OT_UNUSED_VARIABLE(mReserved);
    memset(mPrefix, 0, sizeof(mPrefix));
}

void RouteInfoOption::SetPrefix(const Ip6::Prefix &aPrefix)
{
    SetLength(((aPrefix.mLength + 63) / 64) * 8 + 8);

    mPrefixLength = aPrefix.mLength;
    memcpy(mPrefix, &aPrefix.mPrefix, sizeof(aPrefix.mPrefix));
}

RouterAdvMessage::RouterAdvMessage()
    : mReachableTime(0)
    , mRetransTimer(0)
{
    OT_UNUSED_VARIABLE(mReachableTime);
    OT_UNUSED_VARIABLE(mRetransTimer);

    mHeader.Clear();
    mHeader.SetType(Ip6::Icmp::Header::kTypeRouterAdvert);
}

RouterSolicitMessage::RouterSolicitMessage()
{
    mHeader.Clear();
    mHeader.SetType(Ip6::Icmp::Header::kTypeRouterSolicit);
}

} // namespace RouterAdv

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
