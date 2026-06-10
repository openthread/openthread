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

#include "common/array.hpp"
#include "common/encoding.hpp"
#include "common/string.hpp"
#include "net/ip4_types.hpp"
#include "net/ip6_address.hpp"

#include "test_util.h"

namespace ot {

template <typename AddressType> struct TestVector
{
    const char   *mString;
    const uint8_t mAddr[sizeof(AddressType)];
    Error         mError;
};

template <typename AddressType> static void checkAddressFromString(TestVector<AddressType> *aTestVector)
{
    Error       error;
    AddressType address;

    address.Clear();

    error = address.FromString(aTestVector->mString);

    printf("%-42s -> %-42s\n", aTestVector->mString,
           (error == kErrorNone) ? address.ToString().AsCString() : "(parse error)");

    VerifyOrQuit(error == aTestVector->mError, "Address::FromString returned unexpected error code");

    if (error == kErrorNone)
    {
        VerifyOrQuit(0 == memcmp(address.GetBytes(), aTestVector->mAddr, sizeof(AddressType)),
                     "Address::FromString parsing failed");
    }
}

void TestIp6AddressFromString(void)
{
    typedef TestVector<Ip6::Address> Ip6AddressTestVector;

    Ip6AddressTestVector testVectors[] = {
        // Valid full IPv6 address.
        {"0102:0304:0506:0708:090a:0b0c:0d0e:0f00",
         {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
         kErrorNone},

        // Valid full address using capital letters.
        {"0102:0304:0506:0708:090A:0B0C:0D0E:0F00",
         {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
         kErrorNone},

        // Valid full IPv6 address with mixed capital and small letters.
        {"0102:0304:0506:0708:090a:0B0C:0d0E:0F00",
         {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
         kErrorNone},

        // Short prefix and full IID.
        {"fd11::abcd:e0e0:d10e:0001",
         {0xfd, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xab, 0xcd, 0xe0, 0xe0, 0xd1, 0x0e, 0x00, 0x01},
         kErrorNone},

        // Valid IPv6 address with unnecessary :: symbol.
        {"fd11:1234:5678:abcd::abcd:e0e0:d10e:1000",
         {0xfd, 0x11, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0xab, 0xcd, 0xe0, 0xe0, 0xd1, 0x0e, 0x10, 0x00},
         kErrorNone},

        // Short multicast address.
        {"ff03::0b",
         {0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b},
         kErrorNone},

        // Unspecified address.
        {"::", {0}, kErrorNone},

        // Starts with ::
        {"::1:2:3:4",
         {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04},
         kErrorNone},

        // Ends with ::
        {"1001:2002:3003:4004::",
         {0x10, 0x01, 0x20, 0x02, 0x30, 0x03, 0x40, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
         kErrorNone},

        // Valid embedded IPv4 address.
        {"64:ff9b::100.200.15.4",
         {0x00, 0x64, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0xc8, 0x0f, 0x04},
         kErrorNone},

        // Valid embedded IPv4 address.
        {"2001:db8::abc:def1:127.0.0.1",
         {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xbc, 0xde, 0xf1, 0x7f, 0x00, 0x00, 0x01},
         kErrorNone},

        // Valid embedded IPv4 address.
        {"1:2:3:4:5:6:127.1.2.3",
         {0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06, 0x7f, 0x01, 0x02, 0x03},
         kErrorNone},

        // Two :: should cause a parse error.
        {"2001:db8::a::b", {0}, kErrorParse},

        // The "g" and "h" are not the hex characters.
        {"2001:db8::abcd:efgh", {0}, kErrorParse},

        // Too many colons.
        {"1:2:3:4:5:6:7:8:9", {0}, kErrorParse},

        // Too many characters in a single part.
        {"2001:db8::abc:def12:1:2", {0}, kErrorParse},

        // Invalid embedded IPv4 address.
        {"64:ff9b::123.231.0.257", {0}, kErrorParse},

        // Invalid embedded IPv4 address.
        {"64:ff9b::1.22.33", {0}, kErrorParse},

        // Invalid embedded IPv4 address.
        {"64:ff9b::1.22.33.44.5", {0}, kErrorParse},

        // Too long with embedded IPv4 address.
        {"1:2:3:4:5:6:7:127.1.2.3", {0}, kErrorParse},

        // Invalid embedded IPv4 address.
        {".", {0}, kErrorParse},

        // Invalid embedded IPv4 address.
        {":.", {0}, kErrorParse},

        // Invalid embedded IPv4 address.
        {"::.", {0}, kErrorParse},

        // Invalid embedded IPv4 address.
        {":f:0:0:c:0:f:f:.", {0}, kErrorParse},
    };

    for (Ip6AddressTestVector &testVector : testVectors)
    {
        checkAddressFromString(&testVector);
    }

    // Validate parsing all test vectors now as an IPv6 prefix.

    for (Ip6AddressTestVector &testVector : testVectors)
    {
        constexpr uint16_t kMaxString = 80;

        Ip6::Prefix prefix;
        char        string[kMaxString];
        uint16_t    length;

        length = StringLength(testVector.mString, kMaxString);
        memcpy(string, testVector.mString, length);
        VerifyOrQuit(length + sizeof("/128") <= kMaxString);
        strcpy(&string[length], "/128");

        printf("%s\n", string);

        VerifyOrQuit(prefix.FromString(string) == testVector.mError);

        if (testVector.mError == kErrorNone)
        {
            VerifyOrQuit(memcmp(prefix.GetBytes(), testVector.mAddr, sizeof(Ip6::Address)) == 0);
            VerifyOrQuit(prefix.GetLength() == 128);
        }
    }
}

void TestIp6PrefixFromString(void)
{
    Ip6::Prefix prefix;

    SuccessOrQuit(prefix.FromString("::/128"));
    VerifyOrQuit(prefix.GetLength() == 128);

    SuccessOrQuit(prefix.FromString("::/0128"));
    VerifyOrQuit(prefix.GetLength() == 128);

    SuccessOrQuit(prefix.FromString("::/5"));
    VerifyOrQuit(prefix.GetLength() == 5);

    SuccessOrQuit(prefix.FromString("::/0"));
    VerifyOrQuit(prefix.GetLength() == 0);

    VerifyOrQuit(prefix.FromString("::") == kErrorParse);
    VerifyOrQuit(prefix.FromString("::/") == kErrorParse);
    VerifyOrQuit(prefix.FromString("::/129") == kErrorParse);
    VerifyOrQuit(prefix.FromString(":: /12") == kErrorParse);
    VerifyOrQuit(prefix.FromString("::/a1") == kErrorParse);
    VerifyOrQuit(prefix.FromString("::/12 ") == kErrorParse);
}

void TestIp4AddressFromString(void)
{
    typedef TestVector<Ip4::Address> Ip4AddressTestVector;

    Ip4AddressTestVector testVectors[] = {
        {"0.0.0.0", {0, 0, 0, 0}, kErrorNone},
        {"255.255.255.255", {255, 255, 255, 255}, kErrorNone},
        {"127.0.0.1", {127, 0, 0, 1}, kErrorNone},
        {"1.2.3.4", {1, 2, 3, 4}, kErrorNone},
        {"001.002.003.004", {1, 2, 3, 4}, kErrorNone},
        {"00000127.000.000.000001", {127, 0, 0, 1}, kErrorNone},
        {"123.231.0.256", {0}, kErrorParse},    // Invalid byte value.
        {"100123.231.0.256", {0}, kErrorParse}, // Invalid byte value.
        {"1.22.33", {0}, kErrorParse},          // Too few bytes.
        {"1.22.33.44.5", {0}, kErrorParse},     // Too many bytes.
        {"a.b.c.d", {0}, kErrorParse},          // Wrong digit char.
        {"123.23.45 .12", {0}, kErrorParse},    // Extra space.
        {".", {0}, kErrorParse},                // Invalid.
    };

    for (Ip4AddressTestVector &testVector : testVectors)
    {
        checkAddressFromString(&testVector);
    }
}

struct CidrTestVector
{
    const char   *mString;
    const uint8_t mAddr[sizeof(otIp4Address)];
    const uint8_t mLength;
    Error         mError;
};

static void checkCidrFromString(CidrTestVector *aTestVector)
{
    Error     error;
    Ip4::Cidr cidr;

    cidr.Clear();

    error = cidr.FromString(aTestVector->mString);

    printf("%-42s -> %-42s\n", aTestVector->mString,
           (error == kErrorNone) ? cidr.ToString().AsCString() : "(parse error)");

    VerifyOrQuit(error == aTestVector->mError, "Address::FromString returned unexpected error code");

    if (error == kErrorNone)
    {
        VerifyOrQuit(0 == memcmp(cidr.GetBytes(), aTestVector->mAddr, sizeof(aTestVector->mAddr)),
                     "Cidr::FromString parsing failed");
        VerifyOrQuit(cidr.mLength == aTestVector->mLength, "Cidr::FromString parsing failed");
    }
}

void TestIp4CidrFromString(void)
{
    CidrTestVector testVectors[] = {
        {"0.0.0.0/0", {0, 0, 0, 0}, 0, kErrorNone},
        {"255.255.255.255/32", {255, 255, 255, 255}, 32, kErrorNone},
        {"127.0.0.1/8", {127, 0, 0, 1}, 8, kErrorNone},
        {"1.2.3.4/24", {1, 2, 3, 4}, 24, kErrorNone},
        {"001.002.003.004/20", {1, 2, 3, 4}, 20, kErrorNone},
        {"00000127.000.000.000001/8", {127, 0, 0, 1}, 8, kErrorNone},
        // Valid suffix, invalid address
        {"123.231.0.256/4", {0}, 0, kErrorParse},    // Invalid byte value.
        {"100123.231.0.256/4", {0}, 0, kErrorParse}, // Invalid byte value.
        {"1.22.33/4", {0}, 0, kErrorParse},          // Too few bytes.
        {"1.22.33.44.5/4", {0}, 0, kErrorParse},     // Too many bytes.
        {"a.b.c.d/4", {0}, 0, kErrorParse},          // Wrong digit char.
        {"123.23.45 .12/4", {0}, 0, kErrorParse},    // Extra space.
        {"./4", {0}, 0, kErrorParse},                // Invalid.
        // valid address, invalid suffix
        {"1.2.3.4/33", {0}, 0, kErrorParse},       // Prefix length too large
        {"1.2.3.4/12345678", {0}, 0, kErrorParse}, // Prefix length too large?
        {"1.2.3.4/12a", {0}, 0, kErrorParse},      // Extra char after prefix length.
        {"1.2.3.4/-1", {0}, 0, kErrorParse},       // Not even a non-negative integer.
        {"1.2.3.4/3.14", {0}, 0, kErrorParse},     // Not even a integer.
        {"1.2.3.4/abcd", {0}, 0, kErrorParse},     // Not even a number.
        {"1.2.3.4/", {0}, 0, kErrorParse},         // Where is the suffix?
        {"1.2.3.4", {0}, 0, kErrorParse},          // Where is the suffix?
        // invalid address and invalid suffix
        {"123.231.0.256/41", {0}, 0, kErrorParse},     // Invalid byte value.
        {"100123.231.0.256/abc", {0}, 0, kErrorParse}, // Invalid byte value.
        {"1.22.33", {0}, 0, kErrorParse},              // Too few bytes.
        {"1.22.33.44.5/36", {0}, 0, kErrorParse},      // Too many bytes.
        {"a.b.c.d/99", {0}, 0, kErrorParse},           // Wrong digit char.
        {"123.23.45 .12", {0}, 0, kErrorParse},        // Extra space.
        {".", {0}, 0, kErrorParse},                    // Invalid.
    };

    for (CidrTestVector &testVector : testVectors)
    {
        checkCidrFromString(&testVector);
    }
}

bool CheckPrefix(const Ip6::Address &aAddress, const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    // Check the first aPrefixLength bits of aAddress to match the given aPrefix.

    bool matches = true;

    for (uint8_t bit = 0; bit < aPrefixLength; bit++)
    {
        uint8_t index = bit / kBitsPerByte;
        uint8_t mask  = (0x80 >> (bit % kBitsPerByte));

        if ((aAddress.mFields.m8[index] & mask) != (aPrefix[index] & mask))
        {
            matches = false;
            break;
        }
    }

    return matches;
}

bool CheckPrefixInIid(const Ip6::InterfaceIdentifier &aIid, const uint8_t *aPrefix, uint8_t aPrefixLength)
{
    // Check the IID to contain the prefix bits (applicable when prefix length is longer than 64).

    bool matches = true;

    for (uint8_t bit = 64; bit < aPrefixLength; bit++)
    {
        uint8_t index = bit / kBitsPerByte;
        uint8_t mask  = (0x80 >> (bit % kBitsPerByte));

        if ((aIid.mFields.m8[index - 8] & mask) != (aPrefix[index] & mask))
        {
            matches = false;
            break;
        }
    }

    return matches;
}

bool CheckInterfaceId(const Ip6::Address &aAddress1, const Ip6::Address &aAddress2, uint8_t aPrefixLength)
{
    // Check whether all the bits after aPrefixLength of the two given IPv6 Address match or not.

    bool matches = true;

    for (size_t bit = aPrefixLength; bit < sizeof(Ip6::Address) * kBitsPerByte; bit++)
    {
        uint8_t index = bit / kBitsPerByte;
        uint8_t mask  = (0x80 >> (bit % kBitsPerByte));

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

    Ip6::Address address;
    Ip6::Address allZeroAddress;
    Ip6::Address allOneAddress;
    Ip6::Prefix  ip6Prefix;

    allZeroAddress.Clear();
    memset(&allOneAddress, 0xff, sizeof(allOneAddress));

    for (auto prefix : kPrefixes)
    {
        memcpy(address.mFields.m8, prefix, sizeof(address));
        printf("Prefix is %s\n", address.ToString().AsCString());

        for (size_t prefixLength = 0; prefixLength <= sizeof(Ip6::Address) * kBitsPerByte; prefixLength++)
        {
            ip6Prefix.Clear();
            ip6Prefix.Set(prefix, prefixLength);

            address = allZeroAddress;
            address.SetPrefix(ip6Prefix);
            printf("   prefix-len:%-3zu --> %s\n", prefixLength, address.ToString().AsCString());
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

Ip6::Prefix PrefixFrom(const char *aAddressString, uint8_t aPrefixLength)
{
    Ip6::Prefix  prefix;
    Ip6::Address address;

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

    Ip6::Prefix  prefix;
    Ip6::Address address1, address2;

    for (auto prefixBytes : kPrefixes)
    {
        memcpy(address1.mFields.m8, prefixBytes, sizeof(address1));
        address2 = address1;
        address2.mFields.m8[0] ^= 0x80; // Change first bit.

        for (uint8_t prefixLength = 1; prefixLength <= Ip6::Prefix::kMaxLength; prefixLength++)
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
                Ip6::Prefix subPrefix;

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
                Ip6::Prefix prefix2;
                uint8_t     mask  = static_cast<uint8_t>(1U << (7 - (bitNumber & 7)));
                uint8_t     index = (bitNumber / 8);
                bool        isPrefixSmaller;

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
            Ip6::Prefix mPrefixA;
            Ip6::Prefix mPrefixB;
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

void TestIp6PrefixTidy(void)
{
    struct TestVector
    {
        uint8_t     originalPrefix[OT_IP6_ADDRESS_SIZE];
        const char *prefixStringAfterTidy[129];
    };
    const TestVector kPrefixes[] = {
        {
            .originalPrefix = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                               0xff},
            .prefixStringAfterTidy =
                {
                    "::/0",
                    "8000::/1",
                    "c000::/2",
                    "e000::/3",
                    "f000::/4",
                    "f800::/5",
                    "fc00::/6",
                    "fe00::/7",
                    "ff00::/8",
                    "ff80::/9",
                    "ffc0::/10",
                    "ffe0::/11",
                    "fff0::/12",
                    "fff8::/13",
                    "fffc::/14",
                    "fffe::/15",
                    "ffff::/16",
                    "ffff:8000::/17",
                    "ffff:c000::/18",
                    "ffff:e000::/19",
                    "ffff:f000::/20",
                    "ffff:f800::/21",
                    "ffff:fc00::/22",
                    "ffff:fe00::/23",
                    "ffff:ff00::/24",
                    "ffff:ff80::/25",
                    "ffff:ffc0::/26",
                    "ffff:ffe0::/27",
                    "ffff:fff0::/28",
                    "ffff:fff8::/29",
                    "ffff:fffc::/30",
                    "ffff:fffe::/31",
                    "ffff:ffff::/32",
                    "ffff:ffff:8000::/33",
                    "ffff:ffff:c000::/34",
                    "ffff:ffff:e000::/35",
                    "ffff:ffff:f000::/36",
                    "ffff:ffff:f800::/37",
                    "ffff:ffff:fc00::/38",
                    "ffff:ffff:fe00::/39",
                    "ffff:ffff:ff00::/40",
                    "ffff:ffff:ff80::/41",
                    "ffff:ffff:ffc0::/42",
                    "ffff:ffff:ffe0::/43",
                    "ffff:ffff:fff0::/44",
                    "ffff:ffff:fff8::/45",
                    "ffff:ffff:fffc::/46",
                    "ffff:ffff:fffe::/47",
                    "ffff:ffff:ffff::/48",
                    "ffff:ffff:ffff:8000::/49",
                    "ffff:ffff:ffff:c000::/50",
                    "ffff:ffff:ffff:e000::/51",
                    "ffff:ffff:ffff:f000::/52",
                    "ffff:ffff:ffff:f800::/53",
                    "ffff:ffff:ffff:fc00::/54",
                    "ffff:ffff:ffff:fe00::/55",
                    "ffff:ffff:ffff:ff00::/56",
                    "ffff:ffff:ffff:ff80::/57",
                    "ffff:ffff:ffff:ffc0::/58",
                    "ffff:ffff:ffff:ffe0::/59",
                    "ffff:ffff:ffff:fff0::/60",
                    "ffff:ffff:ffff:fff8::/61",
                    "ffff:ffff:ffff:fffc::/62",
                    "ffff:ffff:ffff:fffe::/63",
                    "ffff:ffff:ffff:ffff::/64",
                    "ffff:ffff:ffff:ffff:8000::/65",
                    "ffff:ffff:ffff:ffff:c000::/66",
                    "ffff:ffff:ffff:ffff:e000::/67",
                    "ffff:ffff:ffff:ffff:f000::/68",
                    "ffff:ffff:ffff:ffff:f800::/69",
                    "ffff:ffff:ffff:ffff:fc00::/70",
                    "ffff:ffff:ffff:ffff:fe00::/71",
                    "ffff:ffff:ffff:ffff:ff00::/72",
                    "ffff:ffff:ffff:ffff:ff80::/73",
                    "ffff:ffff:ffff:ffff:ffc0::/74",
                    "ffff:ffff:ffff:ffff:ffe0::/75",
                    "ffff:ffff:ffff:ffff:fff0::/76",
                    "ffff:ffff:ffff:ffff:fff8::/77",
                    "ffff:ffff:ffff:ffff:fffc::/78",
                    "ffff:ffff:ffff:ffff:fffe::/79",
                    "ffff:ffff:ffff:ffff:ffff::/80",
                    "ffff:ffff:ffff:ffff:ffff:8000::/81",
                    "ffff:ffff:ffff:ffff:ffff:c000::/82",
                    "ffff:ffff:ffff:ffff:ffff:e000::/83",
                    "ffff:ffff:ffff:ffff:ffff:f000::/84",
                    "ffff:ffff:ffff:ffff:ffff:f800::/85",
                    "ffff:ffff:ffff:ffff:ffff:fc00::/86",
                    "ffff:ffff:ffff:ffff:ffff:fe00::/87",
                    "ffff:ffff:ffff:ffff:ffff:ff00::/88",
                    "ffff:ffff:ffff:ffff:ffff:ff80::/89",
                    "ffff:ffff:ffff:ffff:ffff:ffc0::/90",
                    "ffff:ffff:ffff:ffff:ffff:ffe0::/91",
                    "ffff:ffff:ffff:ffff:ffff:fff0::/92",
                    "ffff:ffff:ffff:ffff:ffff:fff8::/93",
                    "ffff:ffff:ffff:ffff:ffff:fffc::/94",
                    "ffff:ffff:ffff:ffff:ffff:fffe::/95",
                    "ffff:ffff:ffff:ffff:ffff:ffff::/96",
                    // Note: The result of /97 to /112 does not meet RFC requirements:
                    // 4.2.2.  Handling One 16-Bit 0 Field
                    // The symbol "::" MUST NOT be used to shorten just one 16-bit 0 field.
                    "ffff:ffff:ffff:ffff:ffff:ffff:8000::/97",
                    "ffff:ffff:ffff:ffff:ffff:ffff:c000::/98",
                    "ffff:ffff:ffff:ffff:ffff:ffff:e000::/99",
                    "ffff:ffff:ffff:ffff:ffff:ffff:f000::/100",
                    "ffff:ffff:ffff:ffff:ffff:ffff:f800::/101",
                    "ffff:ffff:ffff:ffff:ffff:ffff:fc00::/102",
                    "ffff:ffff:ffff:ffff:ffff:ffff:fe00::/103",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ff00::/104",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ff80::/105",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffc0::/106",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffe0::/107",
                    "ffff:ffff:ffff:ffff:ffff:ffff:fff0::/108",
                    "ffff:ffff:ffff:ffff:ffff:ffff:fff8::/109",
                    "ffff:ffff:ffff:ffff:ffff:ffff:fffc::/110",
                    "ffff:ffff:ffff:ffff:ffff:ffff:fffe::/111",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff::/112",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:8000/113",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:c000/114",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:e000/115",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:f000/116",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:f800/117",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fc00/118",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fe00/119",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00/120",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff80/121",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffc0/122",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffe0/123",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff0/124",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff8/125",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffc/126",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe/127",
                    "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128",
                },
        },
    };

    printf("Tidy Prefixes:\n");

    for (auto test : kPrefixes)
    {
        for (uint16_t i = 0; i < GetArrayLength(test.prefixStringAfterTidy); i++)
        {
            Ip6::Prefix prefix, answer;

            SuccessOrQuit(answer.FromString(test.prefixStringAfterTidy[i]));
            prefix.Set(test.originalPrefix, i);
            prefix.Tidy();

            {
                Ip6::Prefix::InfoString prefixString = prefix.ToString();

                printf("Prefix: %-36s  TidyResult: %-36s\n", test.prefixStringAfterTidy[i],
                       prefix.ToString().AsCString());

                VerifyOrQuit(memcmp(answer.mPrefix.mFields.m8, prefix.mPrefix.mFields.m8,
                                    sizeof(answer.mPrefix.mFields.m8)) == 0);
                VerifyOrQuit(prefix.mLength == answer.mLength);
                VerifyOrQuit(strcmp(test.prefixStringAfterTidy[i], prefixString.AsCString()) == 0);
            }
        }
    }
}

void TestIp4MappedIp6Address(void)
{
    const uint8_t kIp4Address[] = {192, 0, 2, 33};

    const char *kInvalidIp4MappedFormats[] = {
        "8000::ffff:192.0.2.23",     "0:400::ffff:192.0.2.23", "0:0:1::ffff:192.0.2.23", "0:0:0:4:0:ffff:192.0.2.23",
        "0:0:0:0:1:ffff:192.0.2.23", "::fffe:192.0.2.23",      "::efff:192.0.2.23",
    };

    Ip4::Address expectedIp4Address;
    Ip4::Address ip4Address;
    Ip6::Address expectedIp6Address;
    Ip6::Address ip6Address;

    printf("\nTestIp4MappedIp6Address()\n");

    expectedIp4Address.SetBytes(kIp4Address);

    SuccessOrQuit(expectedIp6Address.FromString("::ffff:192.0.2.33"));
    ip6Address.SetToIp4Mapped(expectedIp4Address);

    printf("IPv4-mapped IPv6 address: %s\n", ip6Address.ToString().AsCString());

    VerifyOrQuit(ip6Address.IsIp4Mapped());
    VerifyOrQuit(ip6Address == expectedIp6Address);

    SuccessOrQuit(ip4Address.ExtractFromIp4MappedIp6Address(ip6Address));
    VerifyOrQuit(ip4Address == expectedIp4Address);

    for (const char *invalidIp4MappedAddr : kInvalidIp4MappedFormats)
    {
        SuccessOrQuit(ip6Address.FromString(invalidIp4MappedAddr));
        printf("Invalid IPv4-mapped IPv6 address: %s -> %s\n", invalidIp4MappedAddr, ip6Address.ToString().AsCString());
        VerifyOrQuit(!ip6Address.IsIp4Mapped());
        VerifyOrQuit(ip4Address.ExtractFromIp4MappedIp6Address(ip6Address) != kErrorNone);
    }
}

void TestIp4Ip6Translation(void)
{
    struct TestCase
    {
        const char *mPrefix;     // NAT64 prefix
        uint8_t     mLength;     // Prefix length in bits
        const char *mIp6Address; // Expected IPv6 address (with embedded IPv4 "192.0.2.33").
    };

    // The test cases are from RFC 6052 - section 2.4

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

    Ip4::Address ip4Address;

    printf("\nTestIp4Ip6Translation()\n");

    ip4Address.SetBytes(kIp4Address);

    for (const TestCase &testCase : kTestCases)
    {
        Ip6::Prefix  prefix;
        Ip6::Address address;
        Ip6::Address expectedAddress;

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
        const Ip4::Address expectedAddress = ip4Address;
        Ip4::Address       address;
        Ip6::Address       ip6Address;

        SuccessOrQuit(ip6Address.FromString(testCase.mIp6Address));

        address.ExtractFromIp6Address(testCase.mLength, ip6Address);

        printf("Ipv6Address: %-36s IPv4Addr: %-12s Expected: %s\n", testCase.mIp6Address,
               address.ToString().AsCString(), expectedAddress.ToString().AsCString());

        VerifyOrQuit(address == expectedAddress, "Ip4::ExtractFromIp6Address() failed");
    }
}

void TestIp4Cidr(void)
{
    struct TestCase
    {
        const char    *mNetwork;
        const uint8_t  mLength;
        const uint32_t mHost;
        const char    *mOutcome;
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
        Ip4::Address network;
        Ip4::Cidr    cidr;
        Ip4::Address generated;

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

} // namespace ot

int main(void)
{
    ot::TestIp6AddressSetPrefix();
    ot::TestIp4AddressFromString();
    ot::TestIp6AddressFromString();
    ot::TestIp6PrefixFromString();
    ot::TestIp6Prefix();
    ot::TestIp6PrefixTidy();
    ot::TestIp4MappedIp6Address();
    ot::TestIp4Ip6Translation();
    ot::TestIp4Cidr();
    ot::TestIp4CidrFromString();

    printf("All tests passed\n");
    return 0;
}
