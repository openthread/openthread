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

#include "border_router/nat64.hpp"

#include "string.h"

#include "common/instance.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {
namespace BorderRouter {

#if OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE

template <size_t N> bool CheckMessage(const Message *aMessage, const uint8_t (&aExpectedMessage)[N])
{
    uint8_t  readMessage[N];
    uint16_t messageLength;
    bool     success = true;

    success       = success && (aMessage->GetLength() == N);
    messageLength = aMessage->ReadBytes(0, readMessage, N);
    success       = success && (N == messageLength);
    success       = success && (memcmp(readMessage, aExpectedMessage, N) == 0);

    if (!success)
    {
        printf("Expected Message\n");
        for (uint16_t i = 0; i < N; i++)
        {
            printf("%02x%c", aExpectedMessage[i], " \n"[(i & 0xf) == 0xf]);
        }
        printf("\n");
        printf("Actual Message\n");
        for (uint16_t i = 0; i < messageLength; i++)
        {
            printf("%02x%c", readMessage[i], " \n"[(i & 0xf) == 0xf]);
        }
        printf("\n");
    }

    return success;
}

void TestNat64(void)
{
    Ip6::Prefix  nat64prefix;
    Ip4::Cidr    nat64cidr;
    Ip6::Address ip6Source;
    Ip6::Address ip6Dest;
    Instance *   instance = testInitInstance();
    Nat64        nat64(*instance);

    {
        const uint8_t ip6Address[] = {0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        const uint8_t ip4Address[] = {192, 168, 123, 1};

        nat64cidr.Set(ip4Address, 32);
        nat64prefix.Set(ip6Address, 96);
        SuccessOrQuit(nat64.SetIp4Cidr(nat64cidr));
        SuccessOrQuit(nat64.SetEnabled(true));
        nat64.SetNat64Prefix(nat64prefix);
    }

    {
        const uint8_t ip6SourceBytes[] = {
            0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        };
        const uint8_t ip6DestBytes[] = {
            0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 172, 16, 243, 197,
        };

        ip6Source.SetBytes(ip6SourceBytes);
        ip6Dest.SetBytes(ip6DestBytes);
    }

    {
        const uint8_t ip6Packet[] = {
            0x60, 0x08, 0x6e, 0x38, 0x00, 0x0c, 0x11, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            172,  16,   243,  197,  0xab, 0xcd, 0x12, 0x34, 0x00, 0x0c, 0xe3, 0x31, 0x61, 0x62, 0x63, 0x64,
        };
        const uint8_t expectedIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x11, 0xa0,
                                             0x4d, 192,  168,  123,  1,    172,  16,   243,  197,  0xab, 0xcd,
                                             0x12, 0x34, 0x00, 0x0c, 0xa1, 0x8d, 0x61, 0x62, 0x63, 0x64};

        Message *     msg;
        Nat64::Result result;

        printf("Testing Nat64 v6 -> v4\n");

        msg = instance->Get<Ip6::Ip6>().NewMessage(0);
        msg->AppendBytes(ip6Packet, sizeof(ip6Packet));

        result = nat64.HandleOutgoing(*msg);
        VerifyOrQuit(result == Nat64::Result::kForward);
        VerifyOrQuit(CheckMessage(msg, expectedIp4Packet));
    }

    {
        const uint8_t ip4Packet[]         = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x11, 0xa0,
                                     0x4d, 172,  16,   243,  197,  192,  168,  123,  1,    0xab, 0xcd,
                                     0x12, 0x34, 0x00, 0x0c, 0xa1, 0x8d, 0x61, 0x62, 0x63, 0x64};
        const uint8_t expectedIp6Packet[] = {
            0x60, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x3e, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 172,  16,   243,  197,  0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x01, 0xab, 0xcd, 0x12, 0x34, 0x00, 0x0c, 0xe3, 0x31, 0x61, 0x62, 0x63, 0x64,
        };

        Message *     msg;
        Nat64::Result result;

        printf("Testing Nat64 v4 -> v6 (mapping exists)\n");
        msg = instance->Get<Ip6::Ip6>().NewMessage(sizeof(Ip6::Header) - sizeof(Ip4::Header));
        msg->AppendBytes(ip4Packet, sizeof(ip4Packet));

        result = nat64.HandleIncoming(*msg);
        VerifyOrQuit(result == Nat64::Result::kForward);
        VerifyOrQuit(CheckMessage(msg, expectedIp6Packet));
    }

    {
        const uint8_t ip4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x11, 0xa0,
                                     0x4c, 172,  16,   243,  197,  192,  168,  123,  2,    0xab, 0xcd,
                                     0x12, 0x34, 0x00, 0x0c, 0xa1, 0x8c, 0x61, 0x62, 0x63, 0x64};

        Message *     msg;
        Nat64::Result result;

        printf("Testing Nat64 v4 -> v6\n");

        msg = instance->Get<Ip6::Ip6>().NewMessage(sizeof(Ip6::Header) - sizeof(Ip4::Header));
        msg->AppendBytes(ip4Packet, sizeof(ip4Packet));

        result = nat64.HandleIncoming(*msg);
        VerifyOrQuit(result == Nat64::Result::kDrop);
    }

    {
        const uint8_t ip6SourceBytes[] = {
            0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        };
        const uint8_t ip6DestBytes[] = {
            0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 172, 16, 243, 197,
        };

        ip6Source.SetBytes(ip6SourceBytes);
        ip6Dest.SetBytes(ip6DestBytes);
    }

    {
        const uint8_t ip6Packet[] = {
            0x60, 0x08, 0x6e, 0x38, 0x00, 0x0c, 0x11, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            172,  16,   243,  197,  0xab, 0xcd, 0x12, 0x34, 0x00, 0x0c, 0xe3, 0x30, 0x61, 0x62, 0x63, 0x64,
        };

        Message *     msg;
        Nat64::Result result;

        printf("Testing Nat64 v6 -> v4 (mapping pool exhausted)\n");

        msg = instance->Get<Ip6::Ip6>().NewMessage(0);
        msg->AppendBytes(ip6Packet, sizeof(ip6Packet));

        result = nat64.HandleOutgoing(*msg);
        VerifyOrQuit(result == Nat64::Result::kDrop);
    }
}

#endif // OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE

} // namespace BorderRouter
} // namespace ot

int main(void)
{
#if OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE
    ot::BorderRouter::TestNat64();
    printf("All tests passed\n");
#else  // OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE
    printf("Nat64 is not enabled\n");
#endif // OPENTHREAD_CONFIG_NAT64_MANAGER_ENABLE
    return 0;
}
