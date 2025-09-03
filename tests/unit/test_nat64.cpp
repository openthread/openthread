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

#include <stdio.h>

#include "test_platform.h"
#include "test_util.hpp"

#include "instance/instance.hpp"

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

namespace ot {
namespace Nat64 {

#define Log(...) printf(OT_FIRST_ARG(__VA_ARGS__) "\n" OT_REST_ARGS(__VA_ARGS__))

static ot::Instance *sInstance;

void DumpIp6Message(const char *aTextMessage, const Message &aMessage)
{
    Ip6::Headers ip6Headers;

    Log("%s", aTextMessage);

    if (ip6Headers.ParseFrom(aMessage) != kErrorNone)
    {
        Log("    Malformed IPv6 message");
        ExitNow();
    }

    Log("    IPv6 Headers");
    Log("       src      : %s", ip6Headers.GetSourceAddress().ToString().AsCString());
    Log("       dst      : %s", ip6Headers.GetDestinationAddress().ToString().AsCString());
    Log("       proto    : %s", otIp6ProtoToString(ip6Headers.GetIpProto()));

    if (ip6Headers.IsTcp() || ip6Headers.IsUdp())
    {
        Log("       src-port : %u", ip6Headers.GetSourcePort());
        Log("       dst-port : %u", ip6Headers.GetDestinationPort());
    }
    else if (ip6Headers.IsIcmp6())
    {
        Log("       icmp6-id : %u", ip6Headers.GetIcmpHeader().GetId());
    }

exit:
    return;
}

void DumpIp4Message(const char *aTextMessage, const Message &aMessage)
{
    Ip4::Headers ip4Headers;

    Log("%s", aTextMessage);

    if (ip4Headers.ParseFrom(aMessage) != kErrorNone)
    {
        Log("    Malformed IPv4 message");
        ExitNow();
    }

    Log("    IPv4 Headers");
    Log("       src      : %s", ip4Headers.GetSourceAddress().ToString().AsCString());
    Log("       dst      : %s", ip4Headers.GetDestinationAddress().ToString().AsCString());
    Log("       proto    : %s", ip4Headers.IsIcmp4() ? "ICMP4" : otIp6ProtoToString(ip4Headers.GetIpProto()));

    if (ip4Headers.IsTcp() || ip4Headers.IsUdp())
    {
        Log("       src-port : %u", ip4Headers.GetSourcePort());
        Log("       dst-port : %u", ip4Headers.GetDestinationPort());
    }
    else if (ip4Headers.IsIcmp4())
    {
        Log("       icmp4-id : %u", ip4Headers.GetIcmpHeader().GetId());
    }

exit:
    return;
}

void VerifyMessage(const Message &aMessage, const uint8_t *aExpectedContent, uint16_t aExpectedLength)
{
    VerifyOrQuit(aMessage.GetLength() == aExpectedLength);
    VerifyOrQuit(aMessage.CompareBytes(0, aExpectedContent, aExpectedLength));
}

void Verify6To4(const char    *aTestName,
                const uint8_t *aIp6Message,
                const uint16_t aIp6Length,
                const uint8_t *aIp4Message,
                uint16_t       aIp4Length,
                Error          aError)
{
    Message *message = sInstance->Get<Ip6::Ip6>().NewMessage(0);
    Error    error;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Translate IPv6 to IPv4: %s", aTestName);

    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->AppendBytes(aIp6Message, aIp6Length));

    DumpIp6Message("IPv6 message", *message);

    error = sInstance->Get<Translator>().TranslateIp6ToIp4(*message);
    Log("Error: %s (expecting:%s)", ErrorToString(error), ErrorToString(aError));
    VerifyOrQuit(error == aError);

    if (aIp4Message != nullptr)
    {
        DumpIp4Message("Translated IPv4 message", *message);
        VerifyMessage(*message, aIp4Message, aIp4Length);
    }
}

template <uint16_t kIp6Length, uint16_t kIp4Length>
void Verify6To4(const char *aTestName,
                const uint8_t (&aIp6Message)[kIp6Length],
                const uint8_t (&aIp4Message)[kIp4Length],
                Error aError)
{
    Verify6To4(aTestName, aIp6Message, kIp6Length, aIp4Message, kIp4Length, aError);
}

template <uint16_t kIp6Length>
void Verify6To4(const char *aTestName, const uint8_t (&aIp6Message)[kIp6Length], Error aError)
{
    Verify6To4(aTestName, aIp6Message, kIp6Length, nullptr, 0, aError);
}

void Verify4To6(const char    *aTestName,
                const uint8_t *aIp4Message,
                uint16_t       aIp4Length,
                const uint8_t *aIp6Message,
                uint16_t       aIp6Length,
                Error          aError)
{
    Message *message = sInstance->Get<Ip6::Ip6>().NewMessage(0);
    Error    error;

    Log("- - - - - - - - - - - - - - - - - - - - - - - - - ");
    Log("Translate IPv4 to IPv6: %s", aTestName);

    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->AppendBytes(aIp4Message, aIp4Length));

    DumpIp4Message("IPv4 message", *message);

    error = sInstance->Get<Translator>().TranslateIp4ToIp6(*message);
    Log("Error: %s (expecting:%s)", ErrorToString(error), ErrorToString(aError));
    VerifyOrQuit(error == aError);

    if (aIp6Message != nullptr)
    {
        DumpIp6Message("Translated IPv6 message", *message);
        VerifyMessage(*message, aIp6Message, aIp6Length);
    }
}

template <uint16_t kIp4Length, uint16_t kIp6Length>
void Verify4To6(const char *aTestName,
                const uint8_t (&aIp4Message)[kIp4Length],
                const uint8_t (&aIp6Message)[kIp6Length],
                Error aError)
{
    Verify4To6(aTestName, aIp4Message, kIp4Length, aIp6Message, kIp6Length, aError);
}

template <uint16_t kIp4Length>
void Verify4To6(const char *aTestName, const uint8_t (&aIp4Message)[kIp4Length], Error aError)
{
    Verify4To6(aTestName, aIp4Message, kIp4Length, nullptr, 0, aError);
}

//----------------------------------------------------------------------------------------------------------------------

void TestNat64Translation(void)
{
    Ip6::Prefix prefix;
    Ip4::Cidr   cidr;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestNat64Translation");

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != nullptr);

    SuccessOrQuit(prefix.FromString("fd01::/96"));
    SuccessOrQuit(cidr.FromString("192.168.123.1/32"));

    SuccessOrQuit(sInstance->Get<Translator>().SetIp4Cidr(cidr));
    sInstance->Get<Translator>().SetNat64Prefix(prefix);
    sInstance->Get<Translator>().SetEnabled(true);

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

        Verify6To4("Valid v6 UDP", kIp6Packet, kIp4Packet, kErrorNone);
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

        Verify4To6("Valid v4 UDP", kIp4Packet, kIp6Packet, kErrorNone);
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

        Verify6To4("Valid v6 TCP", kIp6Packet, kIp4Packet, kErrorNone);
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

        Verify4To6("Valid v4 TCP", kIp4Packet, kIp6Packet, kErrorNone);
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

        Verify6To4("Valid v6 ICMP ping", kIp6Packet, kIp4Packet, kErrorNone);
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

        Verify4To6("Valid v4 ICMP ping", kIp4Packet, kIp6Packet, kErrorNone);
    }

    {
        // fd02::1               N/A                   IPv6     39     Invalid IPv6 header
        const uint8_t kIp6Packet[] = {0x60, 0x08, 0x6e, 0x38, 0x00, 0x0c, 0x11, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfd, 0x01,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 172,  16,   243};

        Verify6To4("Invalid v6", kIp6Packet, kErrorDrop);
    }

    {
        // 172.16.243.197        N/A                   IPv4     19     [Malformed Packet]
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x11,
                                      0xa0, 0x4c, 172,  16,   243,  197,  192,  168,  123};

        Verify4To6("Invalid v4", kIp4Packet, kErrorDrop);
    }

    {
        // 172.16.243.197        192.168.123.2         UDP      32     43981 → 4660 Len=4
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x11, 0xa0,
                                      0x4c, 172,  16,   243,  197,  192,  168,  123,  2,    0xab, 0xcd,
                                      0x12, 0x34, 0x00, 0x0c, 0xa1, 0x8c, 0x61, 0x62, 0x63, 0x64};

        Verify4To6("No v4 mapping", kIp4Packet, kErrorDrop);
    }

    Log("End of TestNat64Translation");

    testFreeInstance(sInstance);
}

void TestNat64Counters(void)
{
    Ip6::Prefix                        prefix;
    Ip4::Cidr                          cidr;
    Translator::AddressMappingIterator iter;
    Translator::AddressMapping         mapping;
    Translator::ProtocolCounters       expectedCounters;

    Log("--------------------------------------------------------------------------------------------");
    Log("TestNat64Counters");

    sInstance = testInitInstance();
    VerifyOrQuit(sInstance != nullptr);

    SuccessOrQuit(prefix.FromString("fd01::/96"));
    SuccessOrQuit(cidr.FromString("192.168.123.1/32"));

    SuccessOrQuit(sInstance->Get<Translator>().SetIp4Cidr(cidr));
    sInstance->Get<Translator>().SetNat64Prefix(prefix);
    sInstance->Get<Translator>().SetEnabled(true);

    // Step 1: Make the mapping table dirty.
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

        Verify6To4("First translation", kIp6Packet, kIp4Packet, kErrorNone);
    }

    iter.Init(*sInstance);

    SuccessOrQuit(iter.GetNext(mapping));

    expectedCounters.Clear();
    expectedCounters.mUdp.m6To4Packets   = 1;
    expectedCounters.mUdp.m6To4Bytes     = 12;
    expectedCounters.mTotal.m6To4Packets = 1;
    expectedCounters.mTotal.m6To4Bytes   = 12;
    VerifyOrQuit(AsCoreType(&mapping.mCounters) == expectedCounters);

    VerifyOrQuit(iter.GetNext(mapping) == kErrorNotFound);

    // Step 2: Release the mapping table item.
    {
        SuccessOrQuit(prefix.FromString("fd01::/96"));
        SuccessOrQuit(cidr.FromString("192.168.124.1/32"));

        SuccessOrQuit(sInstance->Get<Translator>().SetIp4Cidr(cidr));
        sInstance->Get<Translator>().SetNat64Prefix(prefix);
    }

    // Step 3: Reuse the same object for new mapping table item.
    // If the counters are not reset, the verification below will fail.
    {
        // fd02::1               fd01::ac10:f3c5       UDP      52     43981 → 4660 Len=4
        const uint8_t kIp6Packet[] = {
            0x60, 0x08, 0x6e, 0x38, 0x00, 0x0c, 0x11, 0x40, 0xfd, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            172,  16,   243,  197,  0xab, 0xcd, 0x12, 0x34, 0x00, 0x0c, 0xe3, 0x31, 0x61, 0x62, 0x63, 0x64,
        };
        // 192.168.124.1         172.16.243.197        UDP      32     43981 → 4660 Len=4
        const uint8_t kIp4Packet[] = {0x45, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x9e,
                                      0x4d, 192,  168,  124,  1,    172,  16,   243,  197,  0xab, 0xcd,
                                      0x12, 0x34, 0x00, 0x0c, 0xa0, 0x8d, 0x61, 0x62, 0x63, 0x64};

        Verify6To4("Translation with new mapping", kIp6Packet, kIp4Packet, kErrorNone);
    }

    iter.Init(*sInstance);

    SuccessOrQuit(iter.GetNext(mapping));

    expectedCounters.Clear();
    expectedCounters.mUdp.m6To4Packets   = 1;
    expectedCounters.mUdp.m6To4Bytes     = 12;
    expectedCounters.mTotal.m6To4Packets = 1;
    expectedCounters.mTotal.m6To4Bytes   = 12;
    VerifyOrQuit(AsCoreType(&mapping.mCounters) == expectedCounters);

    VerifyOrQuit(iter.GetNext(mapping) == kErrorNotFound);

    Log("End of TestNat64Counters");
}

} // namespace Nat64
} // namespace ot

#endif // OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE

int main(void)
{
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    ot::Nat64::TestNat64Translation();
    ot::Nat64::TestNat64Counters();
    printf("All tests passed\n");
#else
    printf("NAT64 is not enabled\n");
#endif

    return 0;
}
