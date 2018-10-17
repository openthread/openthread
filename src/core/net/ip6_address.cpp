/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements IPv6 addresses.
 */

#include "ip6_address.hpp"

#include <stdio.h>
#include "utils/wrap_string.h"

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "mac/mac_frame.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {
namespace Ip6 {

void Address::Clear(void)
{
    memset(mFields.m8, 0, sizeof(mFields));
}

bool Address::IsUnspecified(void) const
{
    return (mFields.m32[0] == 0 && mFields.m32[1] == 0 && mFields.m32[2] == 0 && mFields.m32[3] == 0);
}

bool Address::IsLoopback(void) const
{
    return (mFields.m32[0] == 0 && mFields.m32[1] == 0 && mFields.m32[2] == 0 && mFields.m32[3] == HostSwap32(1));
}

bool Address::IsLinkLocal(void) const
{
    return (mFields.m8[0] == 0xfe) && ((mFields.m8[1] & 0xc0) == 0x80);
}

bool Address::IsMulticast(void) const
{
    return mFields.m8[0] == 0xff;
}

bool Address::IsLinkLocalMulticast(void) const
{
    return IsMulticast() && (GetScope() == kLinkLocalScope);
}

bool Address::IsLinkLocalAllNodesMulticast(void) const
{
    return (mFields.m32[0] == HostSwap32(0xff020000) && mFields.m32[1] == 0 && mFields.m32[2] == 0 &&
            mFields.m32[3] == HostSwap32(0x01));
}

bool Address::IsLinkLocalAllRoutersMulticast(void) const
{
    return (mFields.m32[0] == HostSwap32(0xff020000) && mFields.m32[1] == 0 && mFields.m32[2] == 0 &&
            mFields.m32[3] == HostSwap32(0x02));
}

bool Address::IsRealmLocalMulticast(void) const
{
    return IsMulticast() && (GetScope() == kRealmLocalScope);
}

bool Address::IsMulticastLargerThanRealmLocal(void) const
{
    return IsMulticast() && (GetScope() > kRealmLocalScope);
}

bool Address::IsRealmLocalAllNodesMulticast(void) const
{
    return (mFields.m32[0] == HostSwap32(0xff030000) && mFields.m32[1] == 0 && mFields.m32[2] == 0 &&
            mFields.m32[3] == HostSwap32(0x01));
}

bool Address::IsRealmLocalAllRoutersMulticast(void) const
{
    return (mFields.m32[0] == HostSwap32(0xff030000) && mFields.m32[1] == 0 && mFields.m32[2] == 0 &&
            mFields.m32[3] == HostSwap32(0x02));
}

bool Address::IsRealmLocalAllMplForwarders(void) const
{
    return (mFields.m32[0] == HostSwap32(0xff030000) && mFields.m32[1] == 0 && mFields.m32[2] == 0 &&
            mFields.m32[3] == HostSwap32(0xfc));
}

bool Address::IsRoutingLocator(void) const
{
    return (mFields.m16[4] == HostSwap16(0x0000) && mFields.m16[5] == HostSwap16(0x00ff) &&
            mFields.m16[6] == HostSwap16(0xfe00) && mFields.m8[14] < kAloc16Mask &&
            (mFields.m8[14] & kRloc16ReservedBitMask) == 0);
}

bool Address::IsAnycastRoutingLocator(void) const
{
    return (mFields.m16[4] == HostSwap16(0x0000) && mFields.m16[5] == HostSwap16(0x00ff) &&
            mFields.m16[6] == HostSwap16(0xfe00) && mFields.m8[14] == kAloc16Mask);
}

bool Address::IsSubnetRouterAnycast(void) const
{
    return (mFields.m32[2] == 0 && mFields.m32[3] == 0);
}

bool Address::IsReservedSubnetAnycast(void) const
{
    return (mFields.m32[2] == HostSwap32(0xfdffffff) && mFields.m16[6] == 0xffff && mFields.m8[14] == 0xff &&
            mFields.m8[15] >= 0x80);
}

bool Address::IsIidReserved(void) const
{
    return IsSubnetRouterAnycast() || IsReservedSubnetAnycast() || IsAnycastRoutingLocator();
}

const uint8_t *Address::GetIid(void) const
{
    return mFields.m8 + kInterfaceIdentifierOffset;
}

uint8_t *Address::GetIid(void)
{
    return mFields.m8 + kInterfaceIdentifierOffset;
}

void Address::SetIid(const uint8_t *aIid)
{
    memcpy(mFields.m8 + kInterfaceIdentifierOffset, aIid, kInterfaceIdentifierSize);
}

void Address::SetIid(const Mac::ExtAddress &aExtAddress)
{
    memcpy(mFields.m8 + kInterfaceIdentifierOffset, aExtAddress.m8, kInterfaceIdentifierSize);
    mFields.m8[kInterfaceIdentifierOffset] ^= 0x02;
}

void Address::ToExtAddress(Mac::ExtAddress &aExtAddress) const
{
    memcpy(aExtAddress.m8, mFields.m8 + kInterfaceIdentifierOffset, sizeof(aExtAddress.m8));
    aExtAddress.ToggleLocal();
}

void Address::ToExtAddress(Mac::Address &aMacAddress) const
{
    aMacAddress.SetExtended(mFields.m8 + kInterfaceIdentifierOffset, /* reverse */ false);
    aMacAddress.GetExtended().ToggleLocal();
}

uint8_t Address::GetScope(void) const
{
    uint8_t rval;

    if (IsMulticast())
    {
        rval = mFields.m8[1] & 0xf;
    }
    else if (IsLinkLocal())
    {
        rval = kLinkLocalScope;
    }
    else if (IsLoopback())
    {
        rval = kNodeLocalScope;
    }
    else
    {
        rval = kGlobalScope;
    }

    return rval;
}

uint8_t Address::PrefixMatch(const uint8_t *aPrefixA, const uint8_t *aPrefixB, uint8_t aMaxLength)
{
    uint8_t rval = 0;
    uint8_t diff;

    if (aMaxLength > sizeof(Address))
    {
        aMaxLength = sizeof(Address);
    }

    for (uint8_t i = 0; i < aMaxLength; i++)
    {
        diff = aPrefixA[i] ^ aPrefixB[i];

        if (diff == 0)
        {
            rval += 8;
        }
        else
        {
            while ((diff & 0x80) == 0)
            {
                rval++;
                diff <<= 1;
            }

            break;
        }
    }

    return rval;
}

uint8_t Address::PrefixMatch(const Address &aOther) const
{
    return PrefixMatch(mFields.m8, aOther.mFields.m8, sizeof(Address));
}

bool Address::operator==(const Address &aOther) const
{
    return memcmp(mFields.m8, aOther.mFields.m8, sizeof(mFields.m8)) == 0;
}

bool Address::operator!=(const Address &aOther) const
{
    return memcmp(mFields.m8, aOther.mFields.m8, sizeof(mFields.m8)) != 0;
}

otError Address::FromString(const char *aBuf)
{
    otError  error  = OT_ERROR_NONE;
    uint8_t *dst    = reinterpret_cast<uint8_t *>(mFields.m8);
    uint8_t *endp   = reinterpret_cast<uint8_t *>(mFields.m8 + 15);
    uint8_t *colonp = NULL;
    uint16_t val    = 0;
    uint8_t  count  = 0;
    bool     first  = true;
    char     ch;
    uint8_t  d;

    memset(mFields.m8, 0, 16);

    dst--;

    for (;;)
    {
        ch = *aBuf++;
        d  = ch & 0xf;

        if (('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F'))
        {
            d += 9;
        }
        else if (ch == ':' || ch == '\0' || ch == ' ')
        {
            if (count)
            {
                VerifyOrExit(dst + 2 <= endp, error = OT_ERROR_PARSE);
                *(dst + 1) = static_cast<uint8_t>(val >> 8);
                *(dst + 2) = static_cast<uint8_t>(val);
                dst += 2;
                count = 0;
                val   = 0;
            }
            else if (ch == ':')
            {
                VerifyOrExit(colonp == NULL || first, error = OT_ERROR_PARSE);
                colonp = dst;
            }

            if (ch == '\0' || ch == ' ')
            {
                break;
            }

            continue;
        }
        else
        {
            VerifyOrExit('0' <= ch && ch <= '9', error = OT_ERROR_PARSE);
        }

        first = false;
        val   = static_cast<uint16_t>((val << 4) | d);
        VerifyOrExit(++count <= 4, error = OT_ERROR_PARSE);
    }

    while (colonp && dst > colonp)
    {
        *endp-- = *dst--;
    }

    while (endp > dst)
    {
        *endp-- = 0;
    }

exit:
    return error;
}

Address::InfoString Address::ToString(void) const
{
    return InfoString("%x:%x:%x:%x:%x:%x:%x:%x", HostSwap16(mFields.m16[0]), HostSwap16(mFields.m16[1]),
                      HostSwap16(mFields.m16[2]), HostSwap16(mFields.m16[3]), HostSwap16(mFields.m16[4]),
                      HostSwap16(mFields.m16[5]), HostSwap16(mFields.m16[6]), HostSwap16(mFields.m16[7]));
}

} // namespace Ip6
} // namespace ot
