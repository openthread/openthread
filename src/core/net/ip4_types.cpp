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
 *   This file implements IP4 headers processing.
 */

#include "ip4_types.hpp"

#include "common/numeric_limits.hpp"
#include "net/ip6_address.hpp"

namespace ot {
namespace Ip4 {

Error Address::FromString(const char *aString, char aTerminatorChar)
{
    constexpr char kSeparatorChar = '.';

    Error       error = kErrorParse;
    const char *cur   = aString;

    for (uint8_t index = 0;; index++)
    {
        SuccessOrExit(StringParseUint8(cur, mFields.m8[index]));

        if (index == sizeof(Address) - 1)
        {
            break;
        }

        VerifyOrExit(*cur == kSeparatorChar);
        cur++;
    }

    VerifyOrExit(*cur == aTerminatorChar);
    error = kErrorNone;

exit:
    return error;
}

Error Address::ExtractFromIp4MappedIp6Address(const Ip6::Address &aIp6Address)
{
    Error error = kErrorNone;

    VerifyOrExit(aIp6Address.IsIp4Mapped(), error = kErrorParse);
    SetBytes(&aIp6Address.GetBytes()[12]);

exit:
    return error;
}

void Address::ExtractFromIp6Address(uint8_t aPrefixLength, const Ip6::Address &aIp6Address)
{
    // The prefix length must be 32, 40, 48, 56, 64, 96. IPv4 bytes are added
    // after the prefix, skipping over the bits 64 to 71 (byte at `kSkipIndex`)
    // which must be set to zero. The suffix is set to zero (per RFC 6052).
    //
    //    +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //    |PL| 0-------------32--40--48--56--64--72--80--88--96--104---------|
    //    +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //    |32|     prefix    |v4(32)         | u | suffix                    |
    //    +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //    |40|     prefix        |v4(24)     | u |(8)| suffix                |
    //    +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //    |48|     prefix            |v4(16) | u | (16)  | suffix            |
    //    +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //    |56|     prefix                |(8)| u |  v4(24)   | suffix        |
    //    +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //    |64|     prefix                    | u |   v4(32)      | suffix    |
    //    +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //    |96|     prefix                                    |    v4(32)     |
    //    +--+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

    constexpr uint8_t kSkipIndex = 8;

    uint8_t ip6Index;

    OT_ASSERT(Ip6::Prefix::IsValidNat64PrefixLength(aPrefixLength));

    ip6Index = aPrefixLength / kBitsPerByte;

    for (uint8_t &i : mFields.m8)
    {
        if (ip6Index == kSkipIndex)
        {
            ip6Index++;
        }

        i = aIp6Address.GetBytes()[ip6Index++];
    }
}

void Address::SynthesizeFromCidrAndHost(const Cidr &aCidr, const uint32_t aHost)
{
    mFields.m32 = (aCidr.mAddress.mFields.m32 & aCidr.SubnetMask()) | (BigEndian::HostSwap32(aHost) & aCidr.HostMask());
}

void Address::ToString(StringWriter &aWriter) const
{
    aWriter.Append("%d.%d.%d.%d", mFields.m8[0], mFields.m8[1], mFields.m8[2], mFields.m8[3]);
}

void Address::ToString(char *aBuffer, uint16_t aSize) const
{
    StringWriter writer(aBuffer, aSize);

    ToString(writer);
}

Address::InfoString Address::ToString(void) const
{
    InfoString string;

    ToString(string);

    return string;
}

Error Cidr::FromString(const char *aString)
{
    constexpr char     kSlashChar     = '/';
    constexpr uint16_t kMaxCidrLength = 32;

    Error       error = kErrorParse;
    const char *cur;

    SuccessOrExit(AsCoreType(&mAddress).FromString(aString, kSlashChar));

    cur = StringFind(aString, kSlashChar);
    VerifyOrExit(cur != nullptr);
    cur++;

    SuccessOrExit(StringParseUint8(cur, mLength, kMaxCidrLength));
    VerifyOrExit(*cur == kNullChar);

    error = kErrorNone;

exit:
    return error;
}

void Cidr::ToString(StringWriter &aWriter) const
{
    aWriter.Append("%s/%d", AsCoreType(&mAddress).ToString().AsCString(), mLength);
}

void Cidr::ToString(char *aBuffer, uint16_t aSize) const
{
    StringWriter writer(aBuffer, aSize);

    ToString(writer);
}

Cidr::InfoString Cidr::ToString(void) const
{
    InfoString string;

    ToString(string);

    return string;
}

bool Cidr::operator==(const Cidr &aOther) const
{
    return (mLength == aOther.mLength) &&
           (Ip6::Prefix::MatchLength(GetBytes(), aOther.GetBytes(), Ip4::Address::kSize) >= mLength);
}

void Cidr::Set(const uint8_t *aAddress, uint8_t aLength)
{
    memcpy(mAddress.mFields.m8, aAddress, Ip4::Address::kSize);
    mLength = aLength;
}

Error Header::ParseFrom(const Message &aMessage)
{
    Error error = kErrorParse;

    SuccessOrExit(aMessage.Read(0, *this));
    VerifyOrExit(IsValid());
    VerifyOrExit(GetTotalLength() == aMessage.GetLength());

    error = kErrorNone;

exit:
    return error;
}

} // namespace Ip4
} // namespace ot
