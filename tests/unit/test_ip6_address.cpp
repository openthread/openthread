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

#include "net/ip6_address.hpp"

#include "test_util.h"

struct Ip6AddressStringTestVector{
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
    Ip6AddressStringTestVector testVectors[] =
    {
        // Valid full IPv6 address.
        {
            "0102:0304:0506:0708:090a:0b0c:0d0e:0f00",
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
             0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
            OT_ERROR_NONE
        },

        // Valid full IPv6 address with mixed capital and small letters.
        {
            "0102:0304:0506:0708:090a:0B0C:0d0E:0F00",
            {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
             0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00},
            OT_ERROR_NONE
        },

        // Short prefix and full IID.
        {
            "fd11::abcd:e0e0:d10e:0001",
            {0xfd, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0xab, 0xcd, 0xe0, 0xe0, 0xd1, 0x0e, 0x00, 0x01},
            OT_ERROR_NONE
        },

        // Valid IPv6 address with unnecessary :: symbol.
        {
            "fd11:1234:5678:abcd::abcd:e0e0:d10e:1000",
            {0xfd, 0x11, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd,
             0xab, 0xcd, 0xe0, 0xe0, 0xd1, 0x0e, 0x10, 0x00},
            OT_ERROR_NONE
        },

        // Short multicast address.
        {
            "ff03::0b",
            {0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b},
            OT_ERROR_NONE
        },

        // Unspecified address.
        {
            "::",
            {0},
            OT_ERROR_NONE
        },

        // Valid embedded IPv4 address.
        {
            "64:ff9b::100.200.15.4",
            {0x00, 0x64, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x64, 0xc8, 0x0f, 0x04},
            OT_ERROR_NONE
        },

        // Valid embedded IPv4 address.
        {
            "2001:db8::abc:def1:127.0.0.1",
            {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
             0x0a, 0xbc, 0xde, 0xf1, 0x7f, 0x00, 0x00, 0x01},
            OT_ERROR_NONE
        },

        // Two :: should cause a parse error.
        {
            "2001:db8::a::b",
            {0},
            OT_ERROR_PARSE
        },

        // The "g" and "h" are not the hex characters.
        {
            "2001:db8::abcd:efgh",
            {0},
            OT_ERROR_PARSE
        },

        // Too many colons.
        {
            "1:2:3:4:5:6:7:8:9",
            {0},
            OT_ERROR_PARSE
        },

        // Too many characters in a single part.
        {
            "2001:db8::abc:def12:1:2",
            {0},
            OT_ERROR_PARSE
        },

        // Invalid embedded IPv4 address.
        {
            "64:ff9b::123.231.0.257",
            {0},
            OT_ERROR_PARSE
        },

        // Invalid embedded IPv4 address.
        {
            "64:ff9b::1.22.33",
            {0},
            OT_ERROR_PARSE
        },

        // Invalid embedded IPv4 address.
        {
            "64:ff9b::1.22.33.44.5",
            {0},
            OT_ERROR_PARSE
        },

        // Invalid embedded IPv4 address.
        {
            ".",
            {0},
            OT_ERROR_PARSE
        },

        // Invalid embedded IPv4 address.
        {
            ":.",
            {0},
            OT_ERROR_PARSE
        },

        // Invalid embedded IPv4 address.
        {
            "::.",
            {0},
            OT_ERROR_PARSE
        },

        // Invalid embedded IPv4 address.
        {
            ":f:0:0:c:0:f:f:.",
            {0},
            OT_ERROR_PARSE
        },
    };

    for (uint32_t index = 0; index < OT_ARRAY_LENGTH(testVectors); index++)
    {
        checkAddressFromString(&testVectors[index]);
    }
}

int main(void)
{
    TestIp6AddressFromString();
    printf("All tests passed\n");
    return 0;
}
