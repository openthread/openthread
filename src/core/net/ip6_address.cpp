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

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "net/netif.hpp"

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
    return (mFields.m16[0] & HostSwap16(0xffc0)) == HostSwap16(0xfe80);
}

void Address::SetToLinkLocalAddress(const Mac::ExtAddress &aExtAddress)
{
    mFields.m32[0] = HostSwap32(0xfe800000);
    mFields.m32[1] = 0;
    SetIid(aExtAddress);
}

void Address::SetToLinkLocalAddress(const uint8_t *aIid)
{
    mFields.m32[0] = HostSwap32(0xfe800000);
    mFields.m32[1] = 0;
    SetIid(aIid);
}

bool Address::IsLinkLocalMulticast(void) const
{
    return IsMulticast() && (GetScope() == kLinkLocalScope);
}

bool Address::IsLinkLocalAllNodesMulticast(void) const
{
    return (*this == GetLinkLocalAllNodesMulticast());
}

void Address::SetToLinkLocalAllNodesMulticast(void)
{
    *this = GetLinkLocalAllNodesMulticast();
}

bool Address::IsLinkLocalAllRoutersMulticast(void) const
{
    return (*this == GetLinkLocalAllRoutersMulticast());
}

void Address::SetToLinkLocalAllRoutersMulticast(void)
{
    *this = GetLinkLocalAllRoutersMulticast();
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
    return (*this == GetRealmLocalAllNodesMulticast());
}

void Address::SetToRealmLocalAllNodesMulticast(void)
{
    *this = GetRealmLocalAllNodesMulticast();
}

bool Address::IsRealmLocalAllRoutersMulticast(void) const
{
    return (*this == GetRealmLocalAllRoutersMulticast());
}

void Address::SetToRealmLocalAllRoutersMulticast(void)
{
    *this = GetRealmLocalAllRoutersMulticast();
}

bool Address::IsRealmLocalAllMplForwarders(void) const
{
    return (*this == GetRealmLocalAllMplForwarders());
}

void Address::SetToRealmLocalAllMplForwarders(void)
{
    *this = GetRealmLocalAllMplForwarders();
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
    return IsSubnetRouterAnycast() || IsReservedSubnetAnycast() || IsIidAnycastLocator();
}

bool Address::IsIidLocator(void) const
{
    // IID pattern 0000:00ff:fe00:xxxx
    return (mFields.m32[2] == HostSwap32(0x000000ff) && mFields.m16[6] == HostSwap16(0xfe00));
}

bool Address::IsIidRoutingLocator(void) const
{
    return (IsIidLocator() && (mFields.m8[14] < kAloc16Mask) && ((mFields.m8[14] & kRloc16ReservedBitMask) == 0));
}

bool Address::IsIidAnycastLocator(void) const
{
    // Anycast locator range 0xfc00- 0xfcff (`kAloc16Max` is 0xfc)
    return (IsIidLocator() && (mFields.m8[14] == kAloc16Mask));
}

bool Address::IsIidAnycastServiceLocator(void) const
{
    uint16_t locator = GetLocator();

    return (IsIidLocator() && (locator >= Mle::kAloc16ServiceStart) && (locator <= Mle::kAloc16ServiceEnd));
}

void Address::SetPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    SetPrefix(0, aPrefix, aPrefixLength);
}

void Address::SetPrefix(uint8_t aOffset, const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    uint8_t bytes     = aPrefixLength / CHAR_BIT;
    uint8_t extraBits = aPrefixLength % CHAR_BIT;

    OT_ASSERT(aPrefixLength <= (sizeof(Address) - aOffset) * CHAR_BIT);

    memcpy(mFields.m8 + aOffset, aPrefix, bytes);

    if (extraBits > 0)
    {
        uint8_t index = aOffset + bytes;
        uint8_t mask  = ((0x80 >> (extraBits - 1)) - 1);

        // `mask` has its higher (msb) `extraBits` bits as `0` and the reminaing as `1`.
        // Example with `extraBits` = 3:
        // ((0x80 >> 2) - 1) = (0b0010_0000 - 1) = 0b0001_1111

        mFields.m8[index] &= mask;
        mFields.m8[index] |= (aPrefix[index] & ~mask);
    }
}

void Address::SetMulticastNetworkPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    SetPrefix(kMulticastNetworkPrefixOffset, aPrefix, aPrefixLength);
    mFields.m8[kMulticastNetworkPrefixLengthOffset] = aPrefixLength;
}

void Address::SetIid(const uint8_t *aIid)
{
    memcpy(mFields.m8 + kInterfaceIdentifierOffset, aIid, kInterfaceIdentifierSize);
}

void Address::SetIid(const Mac::ExtAddress &aExtAddress)
{
    Mac::ExtAddress addr;

    addr = aExtAddress;
    addr.ToggleLocal();
    addr.CopyTo(mFields.m8 + kInterfaceIdentifierOffset);
}

void Address::SetIidToLocator(uint16_t aLocator)
{
    // Locator IID pattern `0000:00ff:fe00:xxxx`
    mFields.m32[2] = HostSwap32(0x000000ff);
    mFields.m16[6] = HostSwap16(0xfe00);
    mFields.m16[7] = HostSwap16(aLocator);
}

void Address::SetToLocator(const Mle::MeshLocalPrefix &aMeshLocalPrefix, uint16_t aLocator)
{
    SetPrefix(aMeshLocalPrefix);
    SetIidToLocator(aLocator);
}

void Address::ToExtAddress(Mac::ExtAddress &aExtAddress) const
{
    aExtAddress.Set(mFields.m8 + kInterfaceIdentifierOffset);
    aExtAddress.ToggleLocal();
}

void Address::ToExtAddress(Mac::Address &aMacAddress) const
{
    aMacAddress.SetExtended(mFields.m8 + kInterfaceIdentifierOffset);
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

uint8_t Address::PrefixMatch(const otIp6Address &aOther) const
{
    return PrefixMatch(mFields.m8, aOther.mFields.m8, sizeof(Address));
}

bool Address::operator==(const Address &aOther) const
{
    return memcmp(mFields.m8, aOther.mFields.m8, sizeof(mFields.m8)) == 0;
}

otError Address::FromString(const char *aBuf)
{
    otError     error  = OT_ERROR_NONE;
    uint8_t *   dst    = reinterpret_cast<uint8_t *>(mFields.m8);
    uint8_t *   endp   = reinterpret_cast<uint8_t *>(mFields.m8 + 15);
    uint8_t *   colonp = NULL;
    const char *colonc = NULL;
    uint16_t    val    = 0;
    uint8_t     count  = 0;
    bool        first  = true;
    bool        hasIp4 = false;
    char        ch;
    uint8_t     d;

    Clear();

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

            colonc = aBuf;

            continue;
        }
        else if (ch == '.')
        {
            hasIp4 = true;

            // Do not count bytes of the embedded IPv4 address.
            endp -= kIp4AddressSize;

            VerifyOrExit(dst <= endp, error = OT_ERROR_PARSE);

            break;
        }
        else
        {
            VerifyOrExit('0' <= ch && ch <= '9', error = OT_ERROR_PARSE);
        }

        first = false;
        val   = static_cast<uint16_t>((val << 4) | d);
        VerifyOrExit(++count <= 4, error = OT_ERROR_PARSE);
    }

    VerifyOrExit(colonp || dst == endp, error = OT_ERROR_PARSE);

    while (colonp && dst > colonp)
    {
        *endp-- = *dst--;
    }

    while (endp > dst)
    {
        *endp-- = 0;
    }

    if (hasIp4)
    {
        val = 0;

        // Reset the start and end pointers.
        dst  = reinterpret_cast<uint8_t *>(mFields.m8 + 12);
        endp = reinterpret_cast<uint8_t *>(mFields.m8 + 15);

        for (;;)
        {
            ch = *colonc++;

            if (ch == '.' || ch == '\0' || ch == ' ')
            {
                VerifyOrExit(dst <= endp, error = OT_ERROR_PARSE);

                *dst++ = static_cast<uint8_t>(val);
                val    = 0;

                if (ch == '\0' || ch == ' ')
                {
                    // Check if embedded IPv4 address had exactly four parts.
                    VerifyOrExit(dst == endp + 1, error = OT_ERROR_PARSE);
                    break;
                }
            }
            else
            {
                VerifyOrExit('0' <= ch && ch <= '9', error = OT_ERROR_PARSE);

                val = (10 * val) + (ch & 0xf);

                // Single part of IPv4 address has to fit in one byte.
                VerifyOrExit(val <= 0xff, error = OT_ERROR_PARSE);
            }
        }
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

const Address &Address::GetLinkLocalAllNodesMulticast(void)
{
    return static_cast<const Address &>(Netif::kLinkLocalAllNodesMulticastAddress.mAddress);
}

const Address &Address::GetLinkLocalAllRoutersMulticast(void)
{
    return static_cast<const Address &>(Netif::kLinkLocalAllRoutersMulticastAddress.mAddress);
}

const Address &Address::GetRealmLocalAllNodesMulticast(void)
{
    return static_cast<const Address &>(Netif::kRealmLocalAllNodesMulticastAddress.mAddress);
}

const Address &Address::GetRealmLocalAllRoutersMulticast(void)
{
    return static_cast<const Address &>(Netif::kRealmLocalAllRoutersMulticastAddress.mAddress);
}

const Address &Address::GetRealmLocalAllMplForwarders(void)
{
    return static_cast<const Address &>(Netif::kRealmLocalAllMplForwardersMulticastAddress.mAddress);
}

} // namespace Ip6
} // namespace ot
