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

#include "net/nat64_translator.hpp"

#include "test_platform.h"
#include "test_util.hpp"

#include "string.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/message.hpp"
#include "instance/instance.hpp"
#include "net/ip6.hpp"

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

namespace ot {
namespace BorderRouter {

static ot::Instance *sInstance;

void DumpMessageInHex(const char *prefix, const uint8_t *aBuf, size_t aBufLen)
{
    // This function dumps all packets the output of this function can be imported to packet analyser for debugging.
    printf("%s", prefix);
    for (size_t i = 0; i < aBufLen; i++)
    {
        printf("%02x", aBuf[i]);
    }
    printf("\n");
}

bool CheckMessage(const Message &aMessage, const uint8_t *aExpectedMessage, size_t aExpectedMessageLen)
{
    uint8_t  readMessage[OPENTHREAD_CONFIG_IP6_MAX_DATAGRAM_LENGTH];
    uint16_t messageLength;
    bool     success = true;

    success       = success && (aMessage.GetLength() == aExpectedMessageLen);
    messageLength = aMessage.ReadBytes(0, readMessage, aMessage.GetLength());
    success       = success && (aExpectedMessageLen == messageLength);
    success       = success && (memcmp(readMessage, aExpectedMessage, aExpectedMessageLen) == 0);

    if (!success)
    {
        printf("Expected Message\n");
        for (size_t i = 0; i < aExpectedMessageLen; i++)
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

template <size_t N>
void TestCase6To4(const char *aTestName,
                  const uint8_t (&aIp6Message)[N],
                  Nat64::Translator::Result aResult,
                  const uint8_t            *aOutMessage,
                  size_t                    aOutMessageLen)
{
    Message *msg = sInstance->Get<Ip6::Ip6>().NewMessage(0);

    printf("Testing NAT64 6 to 4: %s\n", aTestName);

    VerifyOrQuit(msg != nullptr);
    SuccessOrQuit(msg->AppendBytes(aIp6Message, N));

    DumpMessageInHex("I ", aIp6Message, N);

    VerifyOrQuit(sInstance->Get<Nat64::Translator>().TranslateFromIp6(*msg) == aResult);

    if (aOutMessage != nullptr)
    {
        DumpMessageInHex("O ", aOutMessage, aOutMessageLen);
        VerifyOrQuit(CheckMessage(*msg, aOutMessage, aOutMessageLen));
    }

    printf("  ... PASS\n");
}

template <size_t N>
void TestCase4To6(const char *aTestName,
                  const uint8_t (&aIp4Message)[N],
                  Nat64::Translator::Result aResult,
                  const uint8_t            *aOutMessage,
                  size_t                    aOutMessageLen)
{
    Message *msg = sInstance->Get<Ip6::Ip6>().NewMessage(0);

    printf("Testing NAT64 4 to 6: %s\n", aTestName);

    VerifyOrQuit(msg != nullptr);
    SuccessOrQuit(msg->AppendBytes(aIp4Message, N));

    DumpMessageInHex("I ", aIp4Message, N);

    VerifyOrQuit(sInstance->Get<Nat64::Translator>().TranslateToIp6(*msg) == aResult);

    if (aOutMessage != nullptr)
    {
        DumpMessageInHex("O ", aOutMessage, aOutMessageLen);
        VerifyOrQuit(CheckMessage(*msg, aOutMessage, aOutMessageLen));
    }

    printf("  ... PASS\n");
}

void TestNat64(void)
{
    Ip6::Prefix  nat64prefix;
    Ip4::Cidr    nat64cidr;
    Ip6::Address ip6Source;
    Ip6::Address ip6Dest;

    sInstance = testInitInstance();

    {
        const uint8_t ip6Address[] = {0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        const uint8_t ip4Address[] = {192, 168, 123, 1};

        nat64cidr.Set(ip4Address, 32);
        nat64prefix.Set(ip6Address, 96);
        SuccessOrQuit(sInstance->Get<Nat64::Translator>().SetIp4Cidr(nat64cidr));
        sInstance->Get<Nat64::Translator>().SetNat64Prefix(nat64prefix);
    }

    {
        // fd02::1               fd01::ac10:f3c5       UDP      52     43981 → 4660 Len=4
        const uint8_t kIp6Packet[] = {
            0x60, 0x08, 0x6e, 0x38, 0x00, 0x0c, 0x11, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            172,  16,   243,  197,  0xab, 0xcd, 0x12, 0x34, 0x00, 0x0c, 0xe3, 0x31, 0x61, 0x62, 0x63, 0x64,
        };
        // 192.168.123.1         172.16.243.197        UDP      32     43981 → 4660 Len=4
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x9f,
                                      0x4d, 192,  168,  123,  1,    172,  16,   243,  197,  0xab, 0xcd,
                                      0x12, 0x34, 0x00, 0x0c, 0xa1, 0x8d, 0x61, 0x62, 0x63, 0x64};

        TestCase6To4("good v6 udp datagram", kIp6Packet, Nat64::Translator::kForward, kIp4Packet, sizeof(kIp4Packet));
    }

    {
        // 172.16.243.197        192.168.123.1         UDP      32     43981 → 4660 Len=4
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x11, 0xa0,
                                      0x4d, 172,  16,   243,  197,  192,  168,  123,  1,    0xab, 0xcd,
                                      0x12, 0x34, 0x00, 0x0c, 0xa1, 0x8d, 0x61, 0x62, 0x63, 0x64};
        // fd01::ac10:f3c5       fd02::1               UDP      52     43981 → 4660 Len=4
        const uint8_t kIp6Packet[] = {
            0x60, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x11, 0x3f, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 172,  16,   243,  197,  0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x01, 0xab, 0xcd, 0x12, 0x34, 0x00, 0x0c, 0xe3, 0x31, 0x61, 0x62, 0x63, 0x64,
        };

        TestCase4To6("good v4 udp datagram", kIp4Packet, Nat64::Translator::kForward, kIp6Packet, sizeof(kIp6Packet));
    }

    {
        // fd02::1               fd01::ac10:f3c5       TCP      64     43981 → 4660 [ACK] Seq=1 Ack=1 Win=1 Len=4
        const uint8_t kIp6Packet[] = {
            0x60, 0x08, 0x6e, 0x38, 0x00, 0x18, 0x06, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 172,  16,   243,  197,  0xab, 0xcd, 0x12, 0x34, 0x87, 0x65, 0x43, 0x21,
            0x12, 0x34, 0x56, 0x78, 0x50, 0x10, 0x00, 0x01, 0x5f, 0xf8, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
        };
        // 192.168.123.1         172.16.243.197        TCP      44     43981 → 4660 [ACK] Seq=1 Ack=1 Win=1 Len=4
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x9f,
                                      0x4c, 192,  168,  123,  1,    172,  16,   243,  197,  0xab, 0xcd,
                                      0x12, 0x34, 0x87, 0x65, 0x43, 0x21, 0x12, 0x34, 0x56, 0x78, 0x50,
                                      0x10, 0x00, 0x01, 0x1e, 0x54, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64};

        TestCase6To4("good v6 tcp datagram", kIp6Packet, Nat64::Translator::kForward, kIp4Packet, sizeof(kIp4Packet));
    }

    {
        // 172.16.243.197        192.168.123.1         TCP      44     43981 → 4660 [ACK] Seq=1 Ack=1 Win=1 Len=4
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x9f,
                                      0x4c, 172,  16,   243,  197,  192,  168,  123,  1,    0xab, 0xcd,
                                      0x12, 0x34, 0x87, 0x65, 0x43, 0x21, 0x12, 0x34, 0x56, 0x78, 0x50,
                                      0x10, 0x00, 0x01, 0x1e, 0x54, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64};
        // fd01::ac10:f3c5       fd02::1               TCP      64     43981 → 4660 [ACK] Seq=1 Ack=1 Win=1 Len=4
        const uint8_t kIp6Packet[] = {
            0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0x06, 0x40, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 172,  16,   243,  197,  0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xab, 0xcd, 0x12, 0x34, 0x87, 0x65, 0x43, 0x21,
            0x12, 0x34, 0x56, 0x78, 0x50, 0x10, 0x00, 0x01, 0x5f, 0xf8, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64,
        };

        TestCase4To6("good v4 tcp datagram", kIp4Packet, Nat64::Translator::kForward, kIp6Packet, sizeof(kIp6Packet));
    }

    {
        // fd02::1         fd01::ac10:f3c5     ICMPv6   52     Echo (ping) request id=0xaabb, seq=1, hop limit=64
        const uint8_t kIp6Packet[] = {
            0x60, 0x08, 0x6e, 0x38, 0x00, 0x0c, 0x3a, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            172,  16,   243,  197,  0x80, 0x00, 0x76, 0x59, 0xaa, 0xbb, 0x00, 0x01, 0x61, 0x62, 0x63, 0x64,
        };
        // 192.168.123.1   172.16.243.197      ICMP     32     Echo (ping) request  id=0xaabb, seq=1/256, ttl=63
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0x9f,
                                      0x5d, 192,  168,  123,  1,    172,  16,   243,  197,  0x08, 0x00,
                                      0x88, 0x7c, 0xaa, 0xbb, 0x00, 0x01, 0x61, 0x62, 0x63, 0x64};

        TestCase6To4("good v6 icmp ping request datagram", kIp6Packet, Nat64::Translator::kForward, kIp4Packet,
                     sizeof(kIp4Packet));
    }

    {
        // 172.16.243.197        192.168.123.1         ICMP     32     Echo (ping) reply    id=0xaabb, seq=1/256, ttl=63
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x01, 0xa0,
                                      0x5d, 172,  16,   243,  197,  192,  168,  123,  1,    0x00, 0x00,
                                      0x90, 0x7c, 0xaa, 0xbb, 0x00, 0x01, 0x61, 0x62, 0x63, 0x64};
        // fd01::ac10:f3c5       fd02::1               ICMPv6   52     Echo (ping) reply id=0xaabb, seq=1, hop limit=62
        const uint8_t kIp6Packet[] = {
            0x60, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x3a, 0x3f, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 172,  16,   243,  197,  0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x01, 0x81, 0x00, 0x75, 0x59, 0xaa, 0xbb, 0x00, 0x01, 0x61, 0x62, 0x63, 0x64,
        };

        TestCase4To6("good v4 icmp ping response datagram", kIp4Packet, Nat64::Translator::kForward, kIp6Packet,
                     sizeof(kIp6Packet));
    }

    {
        // fd02::1               N/A                   IPv6     39     Invalid IPv6 header
        const uint8_t kIp6Packet[] = {0x60, 0x08, 0x6e, 0x38, 0x00, 0x0c, 0x11, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfd, 0x01,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 172,  16,   243};

        TestCase6To4("bad v6 datagram", kIp6Packet, Nat64::Translator::kDrop, nullptr, 0);
    }

    {
        // 172.16.243.197        N/A                   IPv4     19     [Malformed Packet]
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x11,
                                      0xa0, 0x4c, 172,  16,   243,  197,  192,  168,  123};

        TestCase4To6("bad v4 datagram", kIp4Packet, Nat64::Translator::kDrop, nullptr, 0);
    }

    {
        // 172.16.243.197        192.168.123.2         UDP      32     43981 → 4660 Len=4
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x11, 0xa0,
                                      0x4c, 172,  16,   243,  197,  192,  168,  123,  2,    0xab, 0xcd,
                                      0x12, 0x34, 0x00, 0x0c, 0xa1, 0x8c, 0x61, 0x62, 0x63, 0x64};

        TestCase4To6("no v4 mapping", kIp4Packet, Nat64::Translator::kDrop, nullptr, 0);
    }

    {
        // fd02::2               fd01::ac10:f3c5       UDP      52     43981 → 4660 Len=4
        const uint8_t kIp6Packet[] = {
            0x60, 0x08, 0x6e, 0x38, 0x00, 0x0c, 0x11, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            172,  16,   243,  197,  0xab, 0xcd, 0x12, 0x34, 0x00, 0x0c, 0xe3, 0x30, 0x61, 0x62, 0x63, 0x64,
        };

        TestCase6To4("mapping pool exhausted", kIp6Packet, Nat64::Translator::kDrop, nullptr, 0);
    }

    testFreeInstance(sInstance);
}

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    ot::BorderRouter::TestNat64();
    printf("All tests passed\n");
#else  // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    printf("NAT64 is not enabled\n");
#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    return 0;
}
