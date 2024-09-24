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

#include "instance/instance.hpp"

namespace ot {
namespace Ip6 {

//---------------------------------------------------------------------------------------------------------------------
// NetworkPrefix methods

Error NetworkPrefix::GenerateRandomUla(void)
{
    m8[0] = 0xfd;

    return Random::Crypto::FillBuffer(&m8[1], kSize - 1);
}

//---------------------------------------------------------------------------------------------------------------------
// Prefix methods

void Prefix::Set(const uint8_t *aPrefix, uint8_t aLength)
{
    memcpy(mPrefix.mFields.m8, aPrefix, SizeForLength(aLength));
    mLength = aLength;
}

bool Prefix::IsLinkLocal(void) const
{
    return (mLength >= 10) &&
           ((mPrefix.mFields.m16[0] & BigEndian::HostSwap16(0xffc0)) == BigEndian::HostSwap16(0xfe80));
}

bool Prefix::IsMulticast(void) const { return (mLength >= 8) && (mPrefix.mFields.m8[0] == 0xff); }

bool Prefix::IsUniqueLocal(void) const { return (mLength >= 7) && ((mPrefix.mFields.m8[0] & 0xfe) == 0xfc); }

bool Prefix::IsEqual(const uint8_t *aPrefixBytes, uint8_t aPrefixLength) const
{
    return (mLength == aPrefixLength) && (MatchLength(GetBytes(), aPrefixBytes, GetBytesSize()) >= mLength);
}

bool Prefix::ContainsPrefix(const Prefix &aSubPrefix) const
{
    return (mLength >= aSubPrefix.mLength) &&
           (MatchLength(GetBytes(), aSubPrefix.GetBytes(), aSubPrefix.GetBytesSize()) >= aSubPrefix.GetLength());
}

bool Prefix::ContainsPrefix(const NetworkPrefix &aSubPrefix) const
{
    return (mLength >= NetworkPrefix::kLength) &&
           (MatchLength(GetBytes(), aSubPrefix.m8, NetworkPrefix::kSize) >= NetworkPrefix::kLength);
}

void Prefix::Tidy(void)
{
    uint8_t byteLength      = GetBytesSize();
    uint8_t lastByteBitMask = ~(static_cast<uint8_t>(1 << (byteLength * 8 - mLength)) - 1);

    if (byteLength != 0)
    {
        mPrefix.mFields.m8[byteLength - 1] &= lastByteBitMask;
    }

    for (uint16_t i = byteLength; i < GetArrayLength(mPrefix.mFields.m8); i++)
    {
        mPrefix.mFields.m8[i] = 0;
    }
}

bool Prefix::operator==(const Prefix &aOther) const
{
    return (mLength == aOther.mLength) && (MatchLength(GetBytes(), aOther.GetBytes(), GetBytesSize()) >= GetLength());
}

bool Prefix::operator<(const Prefix &aOther) const
{
    bool    isSmaller;
    uint8_t minLength;
    uint8_t matchedLength;

    minLength     = Min(GetLength(), aOther.GetLength());
    matchedLength = MatchLength(GetBytes(), aOther.GetBytes(), SizeForLength(minLength));

    if (matchedLength >= minLength)
    {
        isSmaller = (GetLength() < aOther.GetLength());
        ExitNow();
    }

    isSmaller = GetBytes()[matchedLength / kBitsPerByte] < aOther.GetBytes()[matchedLength / kBitsPerByte];

exit:
    return isSmaller;
}

uint8_t Prefix::MatchLength(const uint8_t *aPrefixA, const uint8_t *aPrefixB, uint8_t aMaxSize)
{
    uint8_t matchedLength = 0;

    OT_ASSERT(aMaxSize <= Address::kSize);

    for (uint8_t i = 0; i < aMaxSize; i++)
    {
        uint8_t diff = aPrefixA[i] ^ aPrefixB[i];

        if (diff == 0)
        {
            matchedLength += kBitsPerByte;
        }
        else
        {
            while ((diff & 0x80) == 0)
            {
                matchedLength++;
                diff <<= 1;
            }

            break;
        }
    }

    return matchedLength;
}

bool Prefix::IsValidNat64PrefixLength(uint8_t aLength)
{
    return (aLength == 32) || (aLength == 40) || (aLength == 48) || (aLength == 56) || (aLength == 64) ||
           (aLength == 96);
}

Error Prefix::FromString(const char *aString)
{
    constexpr char kSlashChar = '/';
    constexpr char kNullChar  = '\0';

    Error       error = kErrorParse;
    const char *cur;

    VerifyOrExit(aString != nullptr);

    cur = StringFind(aString, kSlashChar);
    VerifyOrExit(cur != nullptr);

    SuccessOrExit(AsCoreType(&mPrefix).ParseFrom(aString, kSlashChar));

    cur++;
    SuccessOrExit(StringParseUint8(cur, mLength, kMaxLength));
    VerifyOrExit(*cur == kNullChar);

    error = kErrorNone;

exit:
    return error;
}

Prefix::InfoString Prefix::ToString(void) const
{
    InfoString string;

    ToString(string);

    return string;
}

void Prefix::ToString(char *aBuffer, uint16_t aSize) const
{
    StringWriter writer(aBuffer, aSize);

    ToString(writer);
}

void Prefix::ToString(StringWriter &aWriter) const
{
    uint8_t sizeInUint16 = (GetBytesSize() + sizeof(uint16_t) - 1) / sizeof(uint16_t);
    Prefix  tidyPrefix   = *this;

    tidyPrefix.Tidy();
    AsCoreType(&tidyPrefix.mPrefix).AppendHexWords(aWriter, sizeInUint16);

    if (GetBytesSize() < Address::kSize - 1)
    {
        aWriter.Append("::");
    }

    aWriter.Append("/%d", mLength);
}

//---------------------------------------------------------------------------------------------------------------------
// InterfaceIdentifier methods

bool InterfaceIdentifier::IsUnspecified(void) const { return (mFields.m32[0] == 0) && (mFields.m32[1] == 0); }

bool InterfaceIdentifier::IsReserved(void) const
{
    return IsSubnetRouterAnycast() || IsReservedSubnetAnycast() || IsAnycastLocator();
}

bool InterfaceIdentifier::IsSubnetRouterAnycast(void) const { return (mFields.m32[0] == 0) && (mFields.m32[1] == 0); }

bool InterfaceIdentifier::IsReservedSubnetAnycast(void) const
{
    // Format of IID in a Reserved Subnet Anycast Address (RFC 2526)
    //
    // |      57 bits     |   7 bits   |
    // +------------------+------------+
    // | 1111110111...111 | anycast ID |
    // +------------------+------------+

    return (mFields.m32[0] == BigEndian::HostSwap32(0xfdffffff) && mFields.m16[2] == BigEndian::HostSwap16(0xffff) &&
            mFields.m8[6] == 0xff && mFields.m8[7] >= 0x80);
}

void InterfaceIdentifier::GenerateRandom(void) { SuccessOrAssert(Random::Crypto::Fill(*this)); }

void InterfaceIdentifier::SetBytes(const uint8_t *aBuffer) { memcpy(mFields.m8, aBuffer, kSize); }

void InterfaceIdentifier::SetFromExtAddress(const Mac::ExtAddress &aExtAddress)
{
    Mac::ExtAddress addr;

    addr = aExtAddress;
    addr.ToggleLocal();
    addr.CopyTo(mFields.m8);
}

void InterfaceIdentifier::ConvertToExtAddress(Mac::ExtAddress &aExtAddress) const
{
    aExtAddress.Set(mFields.m8);
    aExtAddress.ToggleLocal();
}

void InterfaceIdentifier::ConvertToMacAddress(Mac::Address &aMacAddress) const
{
    aMacAddress.SetExtended(mFields.m8);
    aMacAddress.GetExtended().ToggleLocal();
}

void InterfaceIdentifier::SetToLocator(uint16_t aLocator)
{
    // Locator IID pattern `0000:00ff:fe00:xxxx`
    mFields.m32[0] = BigEndian::HostSwap32(0x000000ff);
    mFields.m16[2] = BigEndian::HostSwap16(0xfe00);
    mFields.m16[3] = BigEndian::HostSwap16(aLocator);
}

bool InterfaceIdentifier::IsLocator(void) const
{
    // Locator IID pattern 0000:00ff:fe00:xxxx
    return (mFields.m32[0] == BigEndian::HostSwap32(0x000000ff) && mFields.m16[2] == BigEndian::HostSwap16(0xfe00));
}

bool InterfaceIdentifier::IsRoutingLocator(void) const
{
    return (IsLocator() && (mFields.m8[6] < kAloc16Mask) && ((mFields.m8[6] & kRloc16ReservedBitMask) == 0));
}

bool InterfaceIdentifier::IsAnycastLocator(void) const
{
    // Anycast locator range 0xfc00- 0xfcff (`kAloc16Mask` is 0xfc)
    return (IsLocator() && (mFields.m8[6] == kAloc16Mask));
}

bool InterfaceIdentifier::IsAnycastServiceLocator(void) const
{
    uint16_t locator = GetLocator();

    return (IsLocator() && (locator >= Mle::kAloc16ServiceStart) && (locator <= Mle::kAloc16ServiceEnd));
}

void InterfaceIdentifier::ApplyPrefix(const Prefix &aPrefix)
{
    if (aPrefix.GetLength() > NetworkPrefix::kLength)
    {
        Address::CopyBits(mFields.m8, aPrefix.GetBytes() + NetworkPrefix::kSize,
                          aPrefix.GetLength() - NetworkPrefix::kLength);
    }
}

InterfaceIdentifier::InfoString InterfaceIdentifier::ToString(void) const
{
    InfoString string;

    string.AppendHexBytes(mFields.m8, kSize);

    return string;
}

//---------------------------------------------------------------------------------------------------------------------
// Address methods

bool Address::IsUnspecified(void) const
{
    return (mFields.m32[0] == 0 && mFields.m32[1] == 0 && mFields.m32[2] == 0 && mFields.m32[3] == 0);
}

bool Address::IsLoopback(void) const
{
    return (mFields.m32[0] == 0 && mFields.m32[1] == 0 && mFields.m32[2] == 0 &&
            mFields.m32[3] == BigEndian::HostSwap32(1));
}

bool Address::IsLinkLocalUnicast(void) const
{
    return (mFields.m16[0] & BigEndian::HostSwap16(0xffc0)) == BigEndian::HostSwap16(0xfe80);
}

void Address::SetToLinkLocalAddress(const Mac::ExtAddress &aExtAddress)
{
    mFields.m32[0] = BigEndian::HostSwap32(0xfe800000);
    mFields.m32[1] = 0;
    GetIid().SetFromExtAddress(aExtAddress);
}

void Address::SetToLinkLocalAddress(const InterfaceIdentifier &aIid)
{
    mFields.m32[0] = BigEndian::HostSwap32(0xfe800000);
    mFields.m32[1] = 0;
    SetIid(aIid);
}

bool Address::IsLinkLocalMulticast(void) const { return IsMulticast() && (GetScope() == kLinkLocalScope); }

bool Address::IsLinkLocalUnicastOrMulticast(void) const { return IsLinkLocalUnicast() || IsLinkLocalMulticast(); }

bool Address::IsLinkLocalAllNodesMulticast(void) const { return (*this == GetLinkLocalAllNodesMulticast()); }

void Address::SetToLinkLocalAllNodesMulticast(void) { *this = GetLinkLocalAllNodesMulticast(); }

bool Address::IsLinkLocalAllRoutersMulticast(void) const { return (*this == GetLinkLocalAllRoutersMulticast()); }

void Address::SetToLinkLocalAllRoutersMulticast(void) { *this = GetLinkLocalAllRoutersMulticast(); }

bool Address::IsRealmLocalMulticast(void) const { return IsMulticast() && (GetScope() == kRealmLocalScope); }

bool Address::IsMulticastLargerThanRealmLocal(void) const { return IsMulticast() && (GetScope() > kRealmLocalScope); }

bool Address::IsRealmLocalAllNodesMulticast(void) const { return (*this == GetRealmLocalAllNodesMulticast()); }

void Address::SetToRealmLocalAllNodesMulticast(void) { *this = GetRealmLocalAllNodesMulticast(); }

bool Address::IsRealmLocalAllRoutersMulticast(void) const { return (*this == GetRealmLocalAllRoutersMulticast()); }

void Address::SetToRealmLocalAllRoutersMulticast(void) { *this = GetRealmLocalAllRoutersMulticast(); }

bool Address::IsRealmLocalAllMplForwarders(void) const { return (*this == GetRealmLocalAllMplForwarders()); }

void Address::SetToRealmLocalAllMplForwarders(void) { *this = GetRealmLocalAllMplForwarders(); }

bool Address::IsIp4Mapped(void) const
{
    return (mFields.m32[0] == 0) && (mFields.m32[1] == 0) && (mFields.m32[2] == BigEndian::HostSwap32(0xffff));
}

void Address::SetToIp4Mapped(const Ip4::Address &aIp4Address)
{
    Clear();
    mFields.m16[5] = 0xffff;
    memcpy(&mFields.m8[12], aIp4Address.GetBytes(), sizeof(Ip4::Address));
}

bool Address::MatchesPrefix(const Prefix &aPrefix) const
{
    return Prefix::MatchLength(mFields.m8, aPrefix.GetBytes(), aPrefix.GetBytesSize()) >= aPrefix.GetLength();
}

bool Address::MatchesPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength) const
{
    return Prefix::MatchLength(mFields.m8, aPrefix, Prefix::SizeForLength(aPrefixLength)) >= aPrefixLength;
}

void Address::SetPrefix(const NetworkPrefix &aNetworkPrefix) { mFields.mComponents.mNetworkPrefix = aNetworkPrefix; }

void Address::SetPrefix(const Prefix &aPrefix) { CopyBits(mFields.m8, aPrefix.GetBytes(), aPrefix.GetLength()); }

void Address::CopyBits(uint8_t *aDst, const uint8_t *aSrc, uint8_t aNumBits)
{
    // This method copies `aNumBits` from `aSrc` into `aDst` handling
    // the case where `aNumBits` may not be a multiple of 8. It leaves the
    // remaining bits beyond `aNumBits` in `aDst` unchanged.

    uint8_t numBytes  = aNumBits / kBitsPerByte;
    uint8_t extraBits = aNumBits % kBitsPerByte;

    memcpy(aDst, aSrc, numBytes);

    if (extraBits > 0)
    {
        uint8_t mask = ((0x80 >> (extraBits - 1)) - 1);

        // `mask` has its higher (msb) `extraBits` bits as `0` and the remaining as `1`.
        // Example with `extraBits` = 3:
        // ((0x80 >> 2) - 1) = (0b0010_0000 - 1) = 0b0001_1111

        aDst[numBytes] &= mask;
        aDst[numBytes] |= (aSrc[numBytes] & ~mask);
    }
}

void Address::SetMulticastNetworkPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    CopyBits(&mFields.m8[kMulticastNetworkPrefixOffset], aPrefix, aPrefixLength);
    mFields.m8[kMulticastNetworkPrefixLengthOffset] = aPrefixLength;
}

void Address::SetToLocator(const NetworkPrefix &aNetworkPrefix, uint16_t aLocator)
{
    SetPrefix(aNetworkPrefix);
    GetIid().SetToLocator(aLocator);
}

uint8_t Address::GetScope(void) const
{
    uint8_t rval;

    if (IsMulticast())
    {
        rval = mFields.m8[1] & 0xf;
    }
    else if (IsLinkLocalUnicast())
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

uint8_t Address::PrefixMatch(const Address &aOther) const
{
    return Prefix::MatchLength(mFields.m8, aOther.mFields.m8, sizeof(Address));
}

bool Address::MatchesFilter(TypeFilter aFilter) const
{
    bool matches = true;

    switch (aFilter)
    {
    case kTypeAny:
        break;

    case kTypeUnicast:
        matches = !IsUnspecified() && !IsMulticast();
        break;

    case kTypeMulticast:
        matches = IsMulticast();
        break;

    case kTypeMulticastLargerThanRealmLocal:
        matches = IsMulticastLargerThanRealmLocal();
        break;
    }

    return matches;
}

void Address::SynthesizeFromIp4Address(const Prefix &aPrefix, const Ip4::Address &aIp4Address)
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

    OT_ASSERT(aPrefix.IsValidNat64());

    Clear();
    SetPrefix(aPrefix);

    ip6Index = aPrefix.GetLength() / kBitsPerByte;

    for (uint8_t i = 0; i < Ip4::Address::kSize; i++)
    {
        if (ip6Index == kSkipIndex)
        {
            ip6Index++;
        }

        mFields.m8[ip6Index++] = aIp4Address.GetBytes()[i];
    }
}

Error Address::FromString(const char *aString)
{
    constexpr char kNullChar = '\0';

    return ParseFrom(aString, kNullChar);
}

Error Address::ParseFrom(const char *aString, char aTerminatorChar)
{
    constexpr uint8_t kInvalidIndex = 0xff;
    constexpr char    kColonChar    = ':';
    constexpr char    kDotChar      = '.';

    Error   error      = kErrorParse;
    uint8_t index      = 0;
    uint8_t endIndex   = kSize / sizeof(uint16_t);
    uint8_t colonIndex = kInvalidIndex;
    bool    hasIp4     = false;

    // Check if the string starts with "::".

    if (*aString == kColonChar)
    {
        aString++;
        VerifyOrExit(*aString == kColonChar);
        aString++;
        colonIndex = index;
    }

    while (*aString != aTerminatorChar)
    {
        const char *start = aString;
        uint32_t    value = 0;

        // Parse hex number

        while (true)
        {
            uint8_t digit;

            if (ParseHexDigit(*aString, digit) != kErrorNone)
            {
                break;
            }

            aString++;
            value = (value << 4) + digit;

            VerifyOrExit(value <= NumericLimits<uint16_t>::kMax);
        }

        VerifyOrExit(aString != start);

        if (*aString == kDotChar)
        {
            // IPv6 address contains an embedded IPv4 address.
            aString = start;
            hasIp4  = true;
            endIndex -= Ip4::Address::kSize / sizeof(uint16_t);
            VerifyOrExit(index <= endIndex);
            break;
        }

        VerifyOrExit((*aString == kColonChar) || (*aString == aTerminatorChar));

        VerifyOrExit(index < endIndex);
        mFields.m16[index++] = BigEndian::HostSwap16(static_cast<uint16_t>(value));

        if (*aString == kColonChar)
        {
            aString++;

            if (*aString == kColonChar)
            {
                VerifyOrExit(colonIndex == kInvalidIndex);
                colonIndex = index;
                aString++;
            }
        }
    }

    if (index < endIndex)
    {
        uint8_t wordsToCopy;

        VerifyOrExit(colonIndex != kInvalidIndex);

        wordsToCopy = index - colonIndex;

        memmove(&mFields.m16[endIndex - wordsToCopy], &mFields.m16[colonIndex], wordsToCopy * sizeof(uint16_t));
        memset(&mFields.m16[colonIndex], 0, (endIndex - index) * sizeof(uint16_t));
    }

    if (hasIp4)
    {
        Ip4::Address ip4Addr;

        SuccessOrExit(error = ip4Addr.FromString(aString, aTerminatorChar));
        memcpy(GetArrayEnd(mFields.m8) - Ip4::Address::kSize, ip4Addr.GetBytes(), Ip4::Address::kSize);
    }

    error = kErrorNone;

exit:
    return error;
}

Address::InfoString Address::ToString(void) const
{
    InfoString string;

    ToString(string);

    return string;
}

void Address::ToString(char *aBuffer, uint16_t aSize) const
{
    StringWriter writer(aBuffer, aSize);
    ToString(writer);
}

void Address::ToString(StringWriter &aWriter) const
{
    AppendHexWords(aWriter, static_cast<uint8_t>(GetArrayLength(mFields.m16)));
}

void Address::AppendHexWords(StringWriter &aWriter, uint8_t aLength) const
{
    // Appends the first `aLength` elements in `mFields.m16[]` array
    // as hex words.

    for (uint8_t index = 0; index < aLength; index++)
    {
        if (index > 0)
        {
            aWriter.Append(":");
        }

        aWriter.Append("%x", BigEndian::HostSwap16(mFields.m16[index]));
    }
}

const Address &Address::GetLinkLocalAllNodesMulticast(void)
{
    return AsCoreType(&Netif::kLinkLocalAllNodesMulticastAddress.mAddress);
}

const Address &Address::GetLinkLocalAllRoutersMulticast(void)
{
    return AsCoreType(&Netif::kLinkLocalAllRoutersMulticastAddress.mAddress);
}

const Address &Address::GetRealmLocalAllNodesMulticast(void)
{
    return AsCoreType(&Netif::kRealmLocalAllNodesMulticastAddress.mAddress);
}

const Address &Address::GetRealmLocalAllRoutersMulticast(void)
{
    return AsCoreType(&Netif::kRealmLocalAllRoutersMulticastAddress.mAddress);
}

const Address &Address::GetRealmLocalAllMplForwarders(void)
{
    return AsCoreType(&Netif::kRealmLocalAllMplForwardersMulticastAddress.mAddress);
}

} // namespace Ip6
} // namespace ot
