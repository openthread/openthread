/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include <limits.h>

#include "common/encoding.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_headers.hpp"

#include "test_util.h"

using ot::Encoding::BigEndian::ReadUint16;

struct Ip6AddressStringTestVector
{
    const char *  mString;
    const uint8_t mAddr[OT_IP6_ADDRESS_SIZE];
    otError       mError;
};

static void checkAddressFromString(Ip6AddressStringTestVector *aTestVector)
{
    otError          error;
    ot::Ip6::Address address;

    error = address.FromString(aTestVector->mString);

    VerifyOrQuit(error == aTestVector->mError, "Ip6::Address::FromString returned unexpected error code");

    if (error == OT_ERROR_NONE)
    {
        VerifyOrQuit(0 == memcmp(address.mFields.m8, aTestVector->mAddr, OT_IP6_ADDRESS_SIZE),
                     "Ip6::Address::FromString parsing failed");
    }
}

void TestIp6AddressFromString(void)
{
    Ip6AddressStringTestVector testVectors[] = {
        // Valid full IPv6 address.
        {"0102:0304:0506:0708:090a:0b0c:0d0e:0f00",
         {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
         OT_ERROR_NONE},

        // Valid full IPv6 address with mixed capital and small letters.
        {"0102:0304:0506:0708:090a:0B0C:0d0E:0F00",
         {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
         OT_ERROR_NONE},

        // Short prefix and full IID.
        {"fd11::abcd:e0e0:d10e:0001",
         {0xfd, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xab, 0xcd, 0xe0, 0xe0, 0xd1, 0x0e, 0x00, 0x01},
         OT_ERROR_NONE},

        // Valid IPv6 address with unnecessary :: symbol.
        {"fd11:1234:5678:abcd::abcd:e0e0:d10e:1000",
         {0xfd, 0x11, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0xab, 0xcd, 0xe0, 0xe0, 0xd1, 0x0e, 0x10, 0x00},
         OT_ERROR_NONE},

        // Short multicast address.
        {"ff03::0b",
         {0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b},
         OT_ERROR_NONE},

        // Unspecified address.
        {"::", {0}, OT_ERROR_NONE},

        // Valid embedded IPv4 address.
        {"64:ff9b::100.200.15.4",
         {0x00, 0x64, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0xc8, 0x0f, 0x04},
         OT_ERROR_NONE},

        // Valid embedded IPv4 address.
        {"2001:db8::abc:def1:127.0.0.1",
         {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xbc, 0xde, 0xf1, 0x7f, 0x00, 0x00, 0x01},
         OT_ERROR_NONE},

        // Two :: should cause a parse error.
        {"2001:db8::a::b", {0}, OT_ERROR_PARSE},

        // The "g" and "h" are not the hex characters.
        {"2001:db8::abcd:efgh", {0}, OT_ERROR_PARSE},

        // Too many colons.
        {"1:2:3:4:5:6:7:8:9", {0}, OT_ERROR_PARSE},

        // Too many characters in a single part.
        {"2001:db8::abc:def12:1:2", {0}, OT_ERROR_PARSE},

        // Invalid embedded IPv4 address.
        {"64:ff9b::123.231.0.257", {0}, OT_ERROR_PARSE},

        // Invalid embedded IPv4 address.
        {"64:ff9b::1.22.33", {0}, OT_ERROR_PARSE},

        // Invalid embedded IPv4 address.
        {"64:ff9b::1.22.33.44.5", {0}, OT_ERROR_PARSE},

        // Invalid embedded IPv4 address.
        {".", {0}, OT_ERROR_PARSE},

        // Invalid embedded IPv4 address.
        {":.", {0}, OT_ERROR_PARSE},

        // Invalid embedded IPv4 address.
        {"::.", {0}, OT_ERROR_PARSE},

        // Invalid embedded IPv4 address.
        {":f:0:0:c:0:f:f:.", {0}, OT_ERROR_PARSE},
    };

    for (Ip6AddressStringTestVector &testVector : testVectors)
    {
        checkAddressFromString(&testVector);
    }
}

bool CheckPrefix(const ot::Ip6::Address &aAddress, const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    // Check the first aPrefixLength bits of aAddress to match the given aPrefix.

    bool matches = true;

    for (uint8_t bit = 0; bit < aPrefixLength; bit++)
    {
        uint8_t index = bit / CHAR_BIT;
        uint8_t mask  = (0x80 >> (bit % CHAR_BIT));

        if ((aAddress.mFields.m8[index] & mask) != (aPrefix[index] & mask))
        {
            matches = false;
            break;
        }
    }

    return matches;
}

bool CheckInterfaceId(const ot::Ip6::Address &aAddress1, const ot::Ip6::Address &aAddress2, uint8_t aPrefixLength)
{
    // Check whether all the bits after aPrefixLength of the two given IPv6 Address match or not.

    bool matches = true;

    for (uint8_t bit = aPrefixLength; bit < sizeof(ot::Ip6::Address) * CHAR_BIT; bit++)
    {
        uint8_t index = bit / CHAR_BIT;
        uint8_t mask  = (0x80 >> (bit % CHAR_BIT));

        if ((aAddress1.mFields.m8[index] & mask) != (aAddress2.mFields.m8[index] & mask))
        {
            matches = false;
            break;
        }
    }

    return matches;
}

void TestIp6AddressSetPrefix(void)
{
    const uint8_t kPrefixes[][OT_IP6_ADDRESS_SIZE] = {
        {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef},
        {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    };

    ot::Ip6::Address address;
    ot::Ip6::Address allZeroAddress;
    ot::Ip6::Address allOneAddress;

    allZeroAddress.Clear();
    memset(&allOneAddress, 0xff, sizeof(allOneAddress));

    for (auto prefix : kPrefixes)
    {
        memcpy(address.mFields.m8, prefix, sizeof(address));
        printf("Prefix is %s\n", address.ToString().AsCString());

        for (uint8_t prefixLength = 0; prefixLength <= sizeof(ot::Ip6::Address) * CHAR_BIT; prefixLength++)
        {
            address = allZeroAddress;
            address.SetPrefix(prefix, prefixLength);
            printf("   prefix-len:%-3d --> %s\n", prefixLength, address.ToString().AsCString());
            VerifyOrQuit(CheckPrefix(address, prefix, prefixLength), "Prefix does not match after SetPrefix()");
            VerifyOrQuit(CheckInterfaceId(address, allZeroAddress, prefixLength),
                         "SetPrefix changed bits beyond the prefix length");

            address = allOneAddress;
            address.SetPrefix(prefix, prefixLength);
            VerifyOrQuit(CheckPrefix(address, prefix, prefixLength), "Prefix does not match after SetPrefix()");
            VerifyOrQuit(CheckInterfaceId(address, allOneAddress, prefixLength),
                         "SetPrefix changed bits beyond the prefix length");
        }
    }
}

void TestIp6Prefix(void)
{
    const uint8_t kPrefixes[][OT_IP6_ADDRESS_SIZE] = {
        {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef},
        {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55},
        {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    };

    ot::Ip6::Prefix  prefix;
    ot::Ip6::Address address1, address2;

    for (auto prefixBytes : kPrefixes)
    {
        memcpy(address1.mFields.m8, prefixBytes, sizeof(address1));
        address2 = address1;
        address2.mFields.m8[0] ^= 0x80; // Change first bit.

        for (uint8_t prefixLength = 1; prefixLength <= ot::Ip6::Prefix::kMaxLength; prefixLength++)
        {
            prefix.Set(prefixBytes, prefixLength);

            printf("Prefix %s\n", prefix.ToString().AsCString());

            VerifyOrQuit(prefix.GetLength() == prefixLength, "Prefix::GetLength() failed");
            VerifyOrQuit(prefix.IsValid(), "Prefix::IsValid() failed");
            VerifyOrQuit(prefix.IsEqual(prefixBytes, prefixLength), "Prefix::IsEqual() failed");

            VerifyOrQuit(address1.MatchesPrefix(prefix), "Address::MatchesPrefix() failed");
            VerifyOrQuit(!address2.MatchesPrefix(prefix), "Address::MatchedPrefix() failed");

            VerifyOrQuit(prefix == prefix, "Prefix::operator==() failed");

            for (uint8_t subPrefixLength = 1; subPrefixLength <= prefixLength; subPrefixLength++)
            {
                ot::Ip6::Prefix subPrefix;

                subPrefix.Set(prefixBytes, subPrefixLength);

                VerifyOrQuit(prefix.ContainsPrefix(subPrefix), "Prefix::ContainsPrefix() failed");

                if (prefixLength == subPrefixLength)
                {
                    VerifyOrQuit(prefix == subPrefix, "Prefix::operator==() failed");
                    VerifyOrQuit(prefix.IsEqual(subPrefix.GetBytes(), subPrefix.GetLength()),
                                 "Prefix::IsEqual() failed");
                }
                else
                {
                    VerifyOrQuit(prefix != subPrefix, "Prefix::operator!= failed");
                    VerifyOrQuit(!prefix.IsEqual(subPrefix.GetBytes(), subPrefix.GetLength()),
                                 "Prefix::IsEqual() failed");
                }
            }
        }
    }
}

void TestIp6Header(void)
{
    ot::Ip6::Header  header;
    ot::Ip6::Address source;
    ot::Ip6::Address destination;
    const uint8_t *  headerBytes = reinterpret_cast<const uint8_t *>(&header);

    enum : uint16_t
    {
        kPayloadLength = 650,
    };

    enum : uint8_t
    {
        kHopLimit = 0xd1,
    };

    memset(&header, 0, sizeof(header));

    SuccessOrQuit(source.FromString("0102:0304:0506:0708:090a:0b0c:0d0e:0f12"), "Address::FromString() failed");
    SuccessOrQuit(destination.FromString("1122:3344:5566::7788:99aa:bbcc:ddee:ff23"), "Address::FromString() failed");

    header.Init();
    VerifyOrQuit(header.IsVersion6(), "Header::Init() failed");

    header.SetDscp(ot::Ip6::kDscpCs7);
    header.SetPayloadLength(kPayloadLength);
    header.SetNextHeader(ot::Ip6::kProtoUdp);
    header.SetHopLimit(kHopLimit);
    header.SetSource(source);
    header.SetDestination(destination);

    VerifyOrQuit(header.IsValid(), "Header::IsValid() failed");
    VerifyOrQuit(header.IsVersion6(), "Header::Init() failed");

    VerifyOrQuit(header.GetDscp() == ot::Ip6::kDscpCs7, "Get/SetDscp() failed");
    VerifyOrQuit(header.GetPayloadLength() == kPayloadLength, "Get/SetPayloadLength() failed");
    VerifyOrQuit(header.GetNextHeader() == ot::Ip6::kProtoUdp, "Get/SetNextHeader() failed");
    VerifyOrQuit(header.GetHopLimit() == kHopLimit, "Get/SetHopLimit() failed");
    VerifyOrQuit(header.GetSource() == source, "Get/SetSource() failed");
    VerifyOrQuit(header.GetDestination() == destination, "Get/SetSource() failed");

    // Verify the offsets to different fields.

    VerifyOrQuit(ReadUint16(headerBytes + ot::Ip6::Header::kPayloadLengthFieldOffset) == kPayloadLength,
                 "kPayloadLengthFieldOffset is incorrect");
    VerifyOrQuit(headerBytes[ot::Ip6::Header::kNextHeaderFieldOffset] == ot::Ip6::kProtoUdp,
                 "kNextHeaderFieldOffset is incorrect");
    VerifyOrQuit(headerBytes[ot::Ip6::Header::kHopLimitFieldOffset] == kHopLimit, "kHopLimitFieldOffset is incorrect");
    VerifyOrQuit(memcmp(&headerBytes[ot::Ip6::Header::kSourceFieldOffset], &source, sizeof(source)) == 0,
                 "kSourceFieldOffset is incorrect");
    VerifyOrQuit(memcmp(&headerBytes[ot::Ip6::Header::kDestinationFieldOffset], &destination, sizeof(destination)) == 0,
                 "kSourceFieldOffset is incorrect");
}

int main(void)
{
    TestIp6AddressSetPrefix();
    TestIp6AddressFromString();
    TestIp6Prefix();
    TestIp6Header();
    printf("All tests passed\n");
    return 0;
}
