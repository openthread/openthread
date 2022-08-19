/*
 *  Copyright (c) 2019-2022, The OpenThread Authors.
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
#include "net/ip4_types.hpp"
#include "net/ip6_address.hpp"

#include "test_util.h"

template <typename AddressType> struct TestVector
{
    const char *  mString;
    const uint8_t mAddr[sizeof(AddressType)];
    ot::Error     mError;
};

template <typename AddressType> static void checkAddressFromString(TestVector<AddressType> *aTestVector)
{
    ot::Error   error;
    AddressType address;

    address.Clear();

    error = address.FromString(aTestVector->mString);

    printf("%-42s -> %-42s\n", aTestVector->mString,
           (error == ot::kErrorNone) ? address.ToString().AsCString() : "(parse error)");

    VerifyOrQuit(error == aTestVector->mError, "Address::FromString returned unexpected error code");

    if (error == ot::kErrorNone)
    {
        VerifyOrQuit(0 == memcmp(address.GetBytes(), aTestVector->mAddr, sizeof(AddressType)),
                     "Address::FromString parsing failed");
    }
}

void TestIp6AddressFromString(void)
{
    typedef TestVector<ot::Ip6::Address> Ip6AddressTestVector;

    Ip6AddressTestVector testVectors[] = {
        // Valid full IPv6 address.
        {"0102:0304:0506:0708:090a:0b0c:0d0e:0f00",
         {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
         ot::kErrorNone},

        // Valid full address using capital letters.
        {"0102:0304:0506:0708:090A:0B0C:0D0E:0F00",
         {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
         ot::kErrorNone},

        // Valid full IPv6 address with mixed capital and small letters.
        {"0102:0304:0506:0708:090a:0B0C:0d0E:0F00",
         {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
         ot::kErrorNone},

        // Short prefix and full IID.
        {"fd11::abcd:e0e0:d10e:0001",
         {0xfd, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xab, 0xcd, 0xe0, 0xe0, 0xd1, 0x0e, 0x00, 0x01},
         ot::kErrorNone},

        // Valid IPv6 address with unnecessary :: symbol.
        {"fd11:1234:5678:abcd::abcd:e0e0:d10e:1000",
         {0xfd, 0x11, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0xab, 0xcd, 0xe0, 0xe0, 0xd1, 0x0e, 0x10, 0x00},
         ot::kErrorNone},

        // Short multicast address.
        {"ff03::0b",
         {0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b},
         ot::kErrorNone},

        // Unspecified address.
        {"::", {0}, ot::kErrorNone},

        // Starts with ::
        {"::1:2:3:4",
         {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04},
         ot::kErrorNone},

        // Ends with ::
        {"1001:2002:3003:4004::",
         {0x10, 0x01, 0x20, 0x02, 0x30, 0x03, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
         ot::kErrorNone},

        // Valid embedded IPv4 address.
        {"64:ff9b::100.200.15.4",
         {0x00, 0x64, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0xc8, 0x0f, 0x04},
         ot::kErrorNone},

        // Valid embedded IPv4 address.
        {"2001:db8::abc:def1:127.0.0.1",
         {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xbc, 0xde, 0xf1, 0x7f, 0x00, 0x00, 0x01},
         ot::kErrorNone},

        // Valid embedded IPv4 address.
        {"1:2:3:4:5:6:127.1.2.3",
         {0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06, 0x7f, 0x01, 0x02, 0x03},
         ot::kErrorNone},

        // Two :: should cause a parse error.
        {"2001:db8::a::b", {0}, ot::kErrorParse},

        // The "g" and "h" are not the hex characters.
        {"2001:db8::abcd:efgh", {0}, ot::kErrorParse},

        // Too many colons.
        {"1:2:3:4:5:6:7:8:9", {0}, ot::kErrorParse},

        // Too many characters in a single part.
        {"2001:db8::abc:def12:1:2", {0}, ot::kErrorParse},

        // Invalid embedded IPv4 address.
        {"64:ff9b::123.231.0.257", {0}, ot::kErrorParse},

        // Invalid embedded IPv4 address.
        {"64:ff9b::1.22.33", {0}, ot::kErrorParse},

        // Invalid embedded IPv4 address.
        {"64:ff9b::1.22.33.44.5", {0}, ot::kErrorParse},

        // Too long with embedded IPv4 address.
        {"1:2:3:4:5:6:7:127.1.2.3", {0}, ot::kErrorParse},

        // Invalid embedded IPv4 address.
        {".", {0}, ot::kErrorParse},

        // Invalid embedded IPv4 address.
        {":.", {0}, ot::kErrorParse},

        // Invalid embedded IPv4 address.
        {"::.", {0}, ot::kErrorParse},

        // Invalid embedded IPv4 address.
        {":f:0:0:c:0:f:f:.", {0}, ot::kErrorParse},
    };

    for (Ip6AddressTestVector &testVector : testVectors)
    {
        checkAddressFromString(&testVector);
    }
}

void TestIp4AddressFromString(void)
{
    typedef TestVector<ot::Ip4::Address> Ip4AddressTestVector;

    Ip4AddressTestVector testVectors[] = {
        {"0.0.0.0", {0, 0, 0, 0}, ot::kErrorNone},
        {"255.255.255.255", {255, 255, 255, 255}, ot::kErrorNone},
        {"127.0.0.1", {127, 0, 0, 1}, ot::kErrorNone},
        {"1.2.3.4", {1, 2, 3, 4}, ot::kErrorNone},
        {"001.002.003.004", {1, 2, 3, 4}, ot::kErrorNone},
        {"00000127.000.000.000001", {127, 0, 0, 1}, ot::kErrorNone},
        {"123.231.0.256", {0}, ot::kErrorParse},    // Invalid byte value.
        {"100123.231.0.256", {0}, ot::kErrorParse}, // Invalid byte value.
        {"1.22.33", {0}, ot::kErrorParse},          // Too few bytes.
        {"1.22.33.44.5", {0}, ot::kErrorParse},     // Too many bytes.
        {"a.b.c.d", {0}, ot::kErrorParse},          // Wrong digit char.
        {"123.23.45 .12", {0}, ot::kErrorParse},    // Extra space.
        {".", {0}, ot::kErrorParse},                // Invalid.
    };

    for (Ip4AddressTestVector &testVector : testVectors)
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

bool CheckPrefixInIid(const ot::Ip6::InterfaceIdentifier &aIid, const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    // Check the IID to contain the prefix bits (applicable when prefix length is longer than 64).

    bool matches = true;

    for (uint8_t bit = 64; bit < aPrefixLength; bit++)
    {
        uint8_t index = bit / CHAR_BIT;
        uint8_t mask  = (0x80 >> (bit % CHAR_BIT));

        if ((aIid.mFields.m8[index - 8] & mask) != (aPrefix[index] & mask))
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
    ot::Ip6::Prefix  ip6Prefix;

    allZeroAddress.Clear();
    memset(&allOneAddress, 0xff, sizeof(allOneAddress));

    for (auto prefix : kPrefixes)
    {
        memcpy(address.mFields.m8, prefix, sizeof(address));
        printf("Prefix is %s\n", address.ToString().AsCString());

        for (uint8_t prefixLength = 0; prefixLength <= sizeof(ot::Ip6::Address) * CHAR_BIT; prefixLength++)
        {
            ip6Prefix.Clear();
            ip6Prefix.Set(prefix, prefixLength);

            address = allZeroAddress;
            address.SetPrefix(ip6Prefix);
            printf("   prefix-len:%-3d --> %s\n", prefixLength, address.ToString().AsCString());
            VerifyOrQuit(CheckPrefix(address, prefix, prefixLength), "Prefix does not match after SetPrefix()");
            VerifyOrQuit(CheckInterfaceId(address, allZeroAddress, prefixLength),
                         "SetPrefix changed bits beyond the prefix length");

            address = allOneAddress;
            address.SetPrefix(ip6Prefix);
            VerifyOrQuit(CheckPrefix(address, prefix, prefixLength), "Prefix does not match after SetPrefix()");
            VerifyOrQuit(CheckInterfaceId(address, allOneAddress, prefixLength),
                         "SetPrefix changed bits beyond the prefix length");

            address = allZeroAddress;
            address.GetIid().ApplyPrefix(ip6Prefix);
            VerifyOrQuit(CheckPrefixInIid(address.GetIid(), prefix, prefixLength), "IID is not correct");
            VerifyOrQuit(CheckInterfaceId(address, allZeroAddress, prefixLength),
                         "Iid:ApplyPrefrix() changed bits beyond the prefix length");

            address = allOneAddress;
            address.GetIid().ApplyPrefix(ip6Prefix);
            VerifyOrQuit(CheckPrefixInIid(address.GetIid(), prefix, prefixLength), "IID is not correct");
            VerifyOrQuit(CheckInterfaceId(address, allOneAddress, prefixLength),
                         "Iid:ApplyPrefrix() changed bits beyond the prefix length");
        }
    }
}

ot::Ip6::Prefix PrefixFrom(const char *aAddressString, uint8_t aPrefixLength)
{
    ot::Ip6::Prefix  prefix;
    ot::Ip6::Address address;

    SuccessOrQuit(address.FromString(aAddressString));
    prefix.Set(address.GetBytes(), aPrefixLength);

    return prefix;
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

            VerifyOrQuit(prefix.GetLength() == prefixLength);
            VerifyOrQuit(prefix.IsValid());
            VerifyOrQuit(prefix.IsEqual(prefixBytes, prefixLength));

            VerifyOrQuit(address1.MatchesPrefix(prefix));
            VerifyOrQuit(!address2.MatchesPrefix(prefix));

            VerifyOrQuit(prefix == prefix);
            VerifyOrQuit(!(prefix < prefix));

            for (uint8_t subPrefixLength = 1; subPrefixLength <= prefixLength; subPrefixLength++)
            {
                ot::Ip6::Prefix subPrefix;

                subPrefix.Set(prefixBytes, subPrefixLength);

                VerifyOrQuit(prefix.ContainsPrefix(subPrefix));

                if (prefixLength == subPrefixLength)
                {
                    VerifyOrQuit(prefix == subPrefix);
                    VerifyOrQuit(prefix.IsEqual(subPrefix.GetBytes(), subPrefix.GetLength()));
                    VerifyOrQuit(!(subPrefix < prefix));
                }
                else
                {
                    VerifyOrQuit(prefix != subPrefix);
                    VerifyOrQuit(!prefix.IsEqual(subPrefix.GetBytes(), subPrefix.GetLength()));
                    VerifyOrQuit(subPrefix < prefix);
                }
            }

            for (uint8_t bitNumber = 0; bitNumber < prefixLength; bitNumber++)
            {
                ot::Ip6::Prefix prefix2;
                uint8_t         mask  = static_cast<uint8_t>(1U << (7 - (bitNumber & 7)));
                uint8_t         index = (bitNumber / 8);
                bool            isPrefixSmaller;

                prefix2 = prefix;
                VerifyOrQuit(prefix == prefix2);

                // Flip the `bitNumber` bit between `prefix` and `prefix2`

                prefix2.mPrefix.mFields.m8[index] ^= mask;
                VerifyOrQuit(prefix != prefix2);

                isPrefixSmaller = ((prefix.GetBytes()[index] & mask) == 0);

                VerifyOrQuit((prefix < prefix2) == isPrefixSmaller);
                VerifyOrQuit((prefix2 < prefix) == !isPrefixSmaller);
            }
        }
    }

    {
        struct TestCase
        {
            ot::Ip6::Prefix mPrefixA;
            ot::Ip6::Prefix mPrefixB;
        };

        TestCase kTestCases[] = {
            {PrefixFrom("fd00::", 16), PrefixFrom("fd01::", 16)},
            {PrefixFrom("fc00::", 16), PrefixFrom("fd00::", 16)},
            {PrefixFrom("fd00::", 15), PrefixFrom("fd00::", 16)},
            {PrefixFrom("fd00::", 16), PrefixFrom("fd00:0::", 32)},
            {PrefixFrom("2001:0:0:0::", 64), PrefixFrom("fd00::", 8)},
            {PrefixFrom("2001:dba::", 32), PrefixFrom("fd12:3456:1234:abcd::", 64)},
            {PrefixFrom("910b:1000:0::", 48), PrefixFrom("910b:2000::", 32)},
            {PrefixFrom("::", 0), PrefixFrom("fd00::", 8)},
            {PrefixFrom("::", 0), PrefixFrom("::", 16)},
            {PrefixFrom("fd00:2:2::", 33), PrefixFrom("fd00:2:2::", 35)},
            {PrefixFrom("1:2:3:ffff::", 62), PrefixFrom("1:2:3:ffff::", 63)},
        };

        printf("\nCompare Prefixes:\n");

        for (const TestCase &testCase : kTestCases)
        {
            printf(" %26s  <  %s\n", testCase.mPrefixA.ToString().AsCString(),
                   testCase.mPrefixB.ToString().AsCString());
            VerifyOrQuit(testCase.mPrefixA < testCase.mPrefixB);
            VerifyOrQuit(!(testCase.mPrefixB < testCase.mPrefixA));
        }
    }

    // `IsLinkLocal()` - should contain `fe80::/10`.
    VerifyOrQuit(PrefixFrom("fe80::", 10).IsLinkLocal());
    VerifyOrQuit(PrefixFrom("fe80::", 11).IsLinkLocal());
    VerifyOrQuit(PrefixFrom("fea0::", 16).IsLinkLocal());
    VerifyOrQuit(!PrefixFrom("fe80::", 9).IsLinkLocal());
    VerifyOrQuit(!PrefixFrom("ff80::", 10).IsLinkLocal());
    VerifyOrQuit(!PrefixFrom("fe00::", 10).IsLinkLocal());
    VerifyOrQuit(!PrefixFrom("fec0::", 10).IsLinkLocal());

    // `IsMulticast()` - should contain `ff00::/8`.
    VerifyOrQuit(PrefixFrom("ff00::", 8).IsMulticast());
    VerifyOrQuit(PrefixFrom("ff80::", 9).IsMulticast());
    VerifyOrQuit(PrefixFrom("ffff::", 16).IsMulticast());
    VerifyOrQuit(!PrefixFrom("ff00::", 7).IsMulticast());
    VerifyOrQuit(!PrefixFrom("fe00::", 8).IsMulticast());

    // `IsUniqueLocal()` - should contain `fc00::/7`.
    VerifyOrQuit(PrefixFrom("fc00::", 7).IsUniqueLocal());
    VerifyOrQuit(PrefixFrom("fd00::", 8).IsUniqueLocal());
    VerifyOrQuit(PrefixFrom("fc10::", 16).IsUniqueLocal());
    VerifyOrQuit(!PrefixFrom("fc00::", 6).IsUniqueLocal());
    VerifyOrQuit(!PrefixFrom("f800::", 7).IsUniqueLocal());
    VerifyOrQuit(!PrefixFrom("fe00::", 7).IsUniqueLocal());
}

void TestIp4Ip6Translation(void)
{
    struct TestCase
    {
        const char *mPrefix;     // NAT64 prefix
        uint8_t     mLength;     // Prefix length in bits
        const char *mIp6Address; // Expected IPv6 address (with embedded IPv4 "192.0.2.33").
    };

    // The test cases are from RFC 6502 - section 2.4

    const TestCase kTestCases[] = {
        {"2001:db8::", 32, "2001:db8:c000:221::"},
        {"2001:db8:100::", 40, "2001:db8:1c0:2:21::"},
        {"2001:db8:122::", 48, "2001:db8:122:c000:2:2100::"},
        {"2001:db8:122:300::", 56, "2001:db8:122:3c0:0:221::"},
        {"2001:db8:122:344::", 64, "2001:db8:122:344:c0:2:2100::"},
        {"2001:db8:122:344::", 96, "2001:db8:122:344::192.0.2.33"},
        {"64:ff9b::", 96, "64:ff9b::192.0.2.33"},
    };

    const uint8_t kIp4Address[] = {192, 0, 2, 33};

    ot::Ip4::Address ip4Address;

    printf("\nTestIp4Ip6Translation()\n");

    ip4Address.SetBytes(kIp4Address);

    for (const TestCase &testCase : kTestCases)
    {
        ot::Ip6::Prefix  prefix;
        ot::Ip6::Address address;
        ot::Ip6::Address expectedAddress;

        SuccessOrQuit(address.FromString(testCase.mPrefix));
        prefix.Set(address.GetBytes(), testCase.mLength);

        SuccessOrQuit(expectedAddress.FromString(testCase.mIp6Address));

        address.SynthesizeFromIp4Address(prefix, ip4Address);

        printf("Prefix: %-26s IPv4Addr: %-12s Ipv6Address: %-36s Expected: %s (%s)\n", prefix.ToString().AsCString(),
               ip4Address.ToString().AsCString(), address.ToString().AsCString(), testCase.mIp6Address,
               expectedAddress.ToString().AsCString());

        VerifyOrQuit(address == expectedAddress, "Ip6::SynthesizeFromIp4Address() failed");
    }

    for (const TestCase &testCase : kTestCases)
    {
        const ot::Ip4::Address expectedAddress = ip4Address;
        ot::Ip4::Address       address;
        ot::Ip6::Address       ip6Address;

        SuccessOrQuit(ip6Address.FromString(testCase.mIp6Address));

        address.ExtractFromIp6Address(testCase.mLength, ip6Address);

        printf("Ipv6Address: %-36s IPv4Addr: %-12s Expected: %s\n", testCase.mIp6Address,
               address.ToString().AsCString(), expectedAddress.ToString().AsCString());

        VerifyOrQuit(address == expectedAddress, "Ip4::ExtractFromIp6Address() failed");
    }
}

void TestIp4Cidr(void)
{
    using ot::Encoding::BigEndian::HostSwap32;
    struct TestCase
    {
        const char *   mNetwork;
        const uint8_t  mLength;
        const uint32_t mHost;
        const char *   mOutcome;
    };

    const TestCase kTestCases[] = {
        {"172.16.12.34", 32, 0x12345678, "172.16.12.34"},  {"172.16.12.34", 31, 0x12345678, "172.16.12.34"},
        {"172.16.12.34", 30, 0x12345678, "172.16.12.32"},  {"172.16.12.34", 29, 0x12345678, "172.16.12.32"},
        {"172.16.12.34", 28, 0x12345678, "172.16.12.40"},  {"172.16.12.34", 27, 0x12345678, "172.16.12.56"},
        {"172.16.12.34", 26, 0x12345678, "172.16.12.56"},  {"172.16.12.34", 25, 0x12345678, "172.16.12.120"},
        {"172.16.12.34", 24, 0x12345678, "172.16.12.120"}, {"172.16.12.34", 23, 0x12345678, "172.16.12.120"},
        {"172.16.12.34", 22, 0x12345678, "172.16.14.120"}, {"172.16.12.34", 21, 0x12345678, "172.16.14.120"},
        {"172.16.12.34", 20, 0x12345678, "172.16.6.120"},  {"172.16.12.34", 19, 0x12345678, "172.16.22.120"},
        {"172.16.12.34", 18, 0x12345678, "172.16.22.120"}, {"172.16.12.34", 17, 0x12345678, "172.16.86.120"},
        {"172.16.12.34", 16, 0x12345678, "172.16.86.120"}, {"172.16.12.34", 15, 0x12345678, "172.16.86.120"},
        {"172.16.12.34", 14, 0x12345678, "172.16.86.120"}, {"172.16.12.34", 13, 0x12345678, "172.20.86.120"},
        {"172.16.12.34", 12, 0x12345678, "172.20.86.120"}, {"172.16.12.34", 11, 0x12345678, "172.20.86.120"},
        {"172.16.12.34", 10, 0x12345678, "172.52.86.120"}, {"172.16.12.34", 9, 0x12345678, "172.52.86.120"},
        {"172.16.12.34", 8, 0x12345678, "172.52.86.120"},  {"172.16.12.34", 7, 0x12345678, "172.52.86.120"},
        {"172.16.12.34", 6, 0x12345678, "174.52.86.120"},  {"172.16.12.34", 5, 0x12345678, "170.52.86.120"},
        {"172.16.12.34", 4, 0x12345678, "162.52.86.120"},  {"172.16.12.34", 3, 0x12345678, "178.52.86.120"},
        {"172.16.12.34", 2, 0x12345678, "146.52.86.120"},  {"172.16.12.34", 1, 0x12345678, "146.52.86.120"},
        {"172.16.12.34", 0, 0x12345678, "18.52.86.120"},
    };

    for (const TestCase &testCase : kTestCases)
    {
        ot::Ip4::Address network;
        ot::Ip4::Cidr    cidr;
        ot::Ip4::Address generated;

        SuccessOrQuit(network.FromString(testCase.mNetwork));
        cidr.mAddress = network;
        cidr.mLength  = testCase.mLength;

        generated.SynthesizeFromCidrAndHost(cidr, testCase.mHost);

        printf("CIDR: %-18s HostID: %-8x Host: %-14s Expected: %s\n", cidr.ToString().AsCString(), testCase.mHost,
               generated.ToString().AsCString(), testCase.mOutcome);

        VerifyOrQuit(strcmp(generated.ToString().AsCString(), testCase.mOutcome) == 0,
                     "Ip4::Address::SynthesizeFromCidrAndHost() failed");
    }
}

int main(void)
{
    TestIp6AddressSetPrefix();
    TestIp4AddressFromString();
    TestIp6AddressFromString();
    TestIp6Prefix();
    TestIp4Ip6Translation();
    TestIp4Cidr();
    printf("All tests passed\n");
    return 0;
}
