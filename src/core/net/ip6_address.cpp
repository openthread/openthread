/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements IPv6 addresses.
 */

#include <stdio.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <mac/mac_frame.hpp>
#include <net/ip6_address.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {
namespace Ip6 {

bool Address::IsUnspecified(void) const
{
    return (m32[0] == 0 && m32[1] == 0 && m32[2] == 0 && m32[3] == 0);
}

bool Address::IsLoopback(void) const
{
    return (m32[0] == 0 && m32[1] == 0 && m32[2] == 0 && m32[3] == HostSwap32(1));
}

bool Address::IsLinkLocal(void) const
{
    return (m8[0] == 0xfe) && ((m8[1] & 0xc0) == 0x80);
}

bool Address::IsMulticast(void) const
{
    return m8[0] == 0xff;
}

bool Address::IsLinkLocalMulticast(void) const
{
    return IsMulticast() && (GetScope() == kLinkLocalScope);
}

bool Address::IsLinkLocalAllNodesMulticast(void) const
{
    return (m32[0] == HostSwap32(0xff020000) && m32[1] == 0 &&
            m32[2] == 0 && m32[3] == HostSwap32(0x01));
}

bool Address::IsLinkLocalAllRoutersMulticast(void) const
{
    return (m32[0] == HostSwap32(0xff020000) && m32[1] == 0 &&
            m32[2] == 0 && m32[3] == HostSwap32(0x02));
}

bool Address::IsRealmLocalMulticast(void) const
{
    return IsMulticast() && (GetScope() == kRealmLocalScope);
}

bool Address::IsRealmLocalAllNodesMulticast(void) const
{
    return (m32[0] == HostSwap32(0xff030000) && m32[1] == 0 &&
            m32[2] == 0 && m32[3] == HostSwap32(0x01));
}

bool Address::IsRealmLocalAllRoutersMulticast(void) const
{
    return (m32[0] == HostSwap32(0xff030000) && m32[1] == 0 &&
            m32[2] == 0 && m32[3] == HostSwap32(0x02));
}

const uint8_t *Address::GetIid(void) const
{
    return m8 + kInterfaceIdentifierOffset;
}

uint8_t *Address::GetIid(void)
{
    return m8 + kInterfaceIdentifierOffset;
}

void Address::SetIid(const uint8_t *aIid)
{
    memcpy(m8 + kInterfaceIdentifierOffset, aIid, kInterfaceIdentifierSize);
}

void Address::SetIid(const Mac::ExtAddress &aEui64)
{
    memcpy(m8 + kInterfaceIdentifierOffset, aEui64.mBytes, kInterfaceIdentifierSize);
    m8[kInterfaceIdentifierOffset] ^= 0x02;
}

uint8_t Address::GetScope(void) const
{
    if (IsMulticast())
    {
        return m8[1] & 0xf;
    }
    else if (IsLinkLocal())
    {
        return kLinkLocalScope;
    }
    else if (IsLoopback())
    {
        return kNodeLocalScope;
    }

    return kGlobalScope;
}

uint8_t Address::PrefixMatch(const Address &aOther) const
{
    uint8_t rval = 0;
    uint8_t diff;

    for (uint8_t i = 0; i < sizeof(Address); i++)
    {
        diff = m8[i] ^ aOther.m8[i];

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

bool Address::operator==(const Address &aOther) const
{
    return memcmp(m8, aOther.m8, sizeof(m8)) == 0;
}

bool Address::operator!=(const Address &aOther) const
{
    return memcmp(m8, aOther.m8, sizeof(m8)) != 0;
}

ThreadError Address::FromString(const char *aBuf)
{
    ThreadError error = kThreadError_None;
    uint8_t *dst = reinterpret_cast<uint8_t *>(m8);
    uint8_t *endp = reinterpret_cast<uint8_t *>(m8 + 15);
    uint8_t *colonp = NULL;
    uint16_t val = 0;
    uint8_t count = 0;
    bool first = true;
    uint8_t ch;
    uint8_t d;

    memset(m8, 0, 16);

    dst--;

    for (;;)
    {
        ch = *aBuf++;
        d = ch & 0xf;

        if (('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F'))
        {
            d += 9;
        }
        else if (ch == ':' || ch == '\0' || ch == ' ')
        {
            if (count)
            {
                VerifyOrExit(dst + 2 <= endp, error = kThreadError_Parse);
                *(dst + 1) = static_cast<uint8_t>(val >> 8);
                *(dst + 2) = static_cast<uint8_t>(val);
                dst += 2;
                count = 0;
                val = 0;
            }
            else if (ch == ':')
            {
                VerifyOrExit(colonp == NULL || first, error = kThreadError_Parse);
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
            VerifyOrExit('0' <= ch && ch <= '9', error = kThreadError_Parse);
        }

        first = false;
        val = (val << 4) | d;
        VerifyOrExit(++count <= 4, error = kThreadError_Parse);
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

const char *Address::ToString(char *aBuf, uint16_t aSize) const
{
    snprintf(aBuf, aSize, "%x:%x:%x:%x:%x:%x:%x:%x",
             HostSwap16(m16[0]), HostSwap16(m16[1]),
             HostSwap16(m16[2]), HostSwap16(m16[3]),
             HostSwap16(m16[4]), HostSwap16(m16[5]),
             HostSwap16(m16[6]), HostSwap16(m16[7]));
    return aBuf;
}

}  // namespace Ip6
}  // namespace Thread
