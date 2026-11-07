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

#include "common/encoding.hpp"
#include "common/message.hpp"
#include "instance/instance.hpp"
#include "net/ip6.hpp"
#include "net/ip6_headers.hpp"

#include "test_platform.h"
#include "test_util.hpp"

namespace ot {
namespace Ip6 {

void VerifyVersionTcFlow(const Header &aHeader, uint8_t aDscp, Ecn aEcn, uint32_t aFlow)
{
    uint8_t  expectedTc        = static_cast<uint8_t>((aDscp << 2) + aEcn);
    uint32_t expectedVerTcFlow = 0x60000000 + (static_cast<uint32_t>(expectedTc) << 20) + aFlow;

    printf("%08x {dscp:%d, ecn:%d, flow:%d}\n", aHeader.GetVerionTrafficClassFlow(), aHeader.GetDscp(),
           aHeader.GetEcn(), aHeader.GetFlow());

    VerifyOrQuit(aHeader.IsVersion6());
    VerifyOrQuit(aHeader.GetDscp() == aDscp);
    VerifyOrQuit(aHeader.GetEcn() == aEcn);
    VerifyOrQuit(aHeader.GetFlow() == aFlow);
    VerifyOrQuit(aHeader.GetTrafficClass() == expectedTc);
    VerifyOrQuit(aHeader.GetVerionTrafficClassFlow() == expectedVerTcFlow);
}

void TestIp6Header(void)
{
    static constexpr uint16_t kPayloadLength = 650;
    static constexpr uint8_t  kHopLimit      = 0xd1;
#if !OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    static constexpr uint16_t kMaxPayload = kMaxDatagramLength - sizeof(Header);
#else
    static constexpr uint16_t kMaxPayload = kMaxAssembledDatagramLength - sizeof(Header);
#endif

    const uint32_t kFlows[] = {0x0, 0x1, 0xfff, 0xffff, 0xff000, 0xfffff};
    const uint8_t  kDscps[] = {0x0, 0x1, 0x3, 0xf, 0x30, 0x2f, 0x3f};
    const Ecn      kEcns[]  = {kEcnNotCapable, kEcnCapable0, kEcnCapable1, kEcnMarked};

    Header         header;
    Address        source;
    Address        destination;
    const uint8_t *headerBytes = reinterpret_cast<const uint8_t *>(&header);

    SuccessOrQuit(source.FromString("0102:0304:0506:0708:090a:0b0c:0d0e:0f12"), "Address::FromString() failed");
    SuccessOrQuit(destination.FromString("1122:3344:5566::7788:99aa:bbcc:ddee:ff23"), "Address::FromString() failed");

    header.InitVersionTrafficClassFlow();
    VerifyVersionTcFlow(header, kDscpCs0, kEcnNotCapable, 0);

    header.Clear();
    header.InitVersionTrafficClassFlow();
    VerifyOrQuit(header.IsValid());
    VerifyOrQuit(header.GetPayloadLength() == 0);
    VerifyOrQuit(header.GetNextHeader() == 0);
    VerifyOrQuit(header.GetHopLimit() == 0);
    VerifyOrQuit(header.GetSource().IsUnspecified());
    VerifyOrQuit(header.GetDestination().IsUnspecified());

    header.SetPayloadLength(kPayloadLength);
    header.SetNextHeader(kProtoUdp);
    header.SetHopLimit(kHopLimit);
    header.SetSource(source);
    header.SetDestination(destination);

    VerifyOrQuit(header.IsValid());
    VerifyVersionTcFlow(header, kDscpCs0, kEcnNotCapable, 0);
    VerifyOrQuit(header.GetPayloadLength() == kPayloadLength);
    VerifyOrQuit(header.GetNextHeader() == kProtoUdp);
    VerifyOrQuit(header.GetHopLimit() == kHopLimit);
    VerifyOrQuit(header.GetSource() == source);
    VerifyOrQuit(header.GetDestination() == destination);

    // Verify `IsValid()` length and version boundaries.

    header.SetPayloadLength(kMaxPayload);
    VerifyOrQuit(header.IsValid());

    header.SetPayloadLength(kMaxPayload + 1);
    VerifyOrQuit(!header.IsValid());

    header.SetPayloadLength(0xffff);
    VerifyOrQuit(!header.IsValid());

    header.SetPayloadLength(kPayloadLength);
    header.SetVerionTrafficClassFlow(0x40000000);
    VerifyOrQuit(!header.IsVersion6());
    VerifyOrQuit(!header.IsValid());

    header.InitVersionTrafficClassFlow();
    VerifyOrQuit(header.IsValid());

    // Verify the offsets to different fields.

    VerifyOrQuit(BigEndian::ReadUint16(headerBytes + Header::kPayloadLengthFieldOffset) == kPayloadLength,
                 "kPayloadLengthFieldOffset is incorrect");
    VerifyOrQuit(headerBytes[Header::kNextHeaderFieldOffset] == kProtoUdp, "kNextHeaderFieldOffset is incorrect");
    VerifyOrQuit(headerBytes[Header::kHopLimitFieldOffset] == kHopLimit, "kHopLimitFieldOffset is incorrect");
    VerifyOrQuit(memcmp(&headerBytes[Header::kSourceFieldOffset], &source, sizeof(source)) == 0,
                 "kSourceFieldOffset is incorrect");
    VerifyOrQuit(memcmp(&headerBytes[Header::kDestinationFieldOffset], &destination, sizeof(destination)) == 0,
                 "kSourceFieldOffset is incorrect");

    for (uint32_t flow : kFlows)
    {
        for (uint8_t dscp : kDscps)
        {
            for (Ecn ecn : kEcns)
            {
                printf("Expecting {dscp:%-2d, ecn:%d, flow:%-7d} => ", dscp, ecn, flow);
                header.SetEcn(ecn);
                header.SetDscp(dscp);
                header.SetFlow(flow);
                VerifyVersionTcFlow(header, dscp, ecn, flow);
            }
        }
    }

    // Verify out of range values.
    header.InitVersionTrafficClassFlow();

    header.SetFlow(0xff000001);
    VerifyVersionTcFlow(header, 0, kEcnNotCapable, 1);

    header.SetDscp(0xef);
    VerifyVersionTcFlow(header, 0x2f, kEcnNotCapable, 1);
}

static Message *BuildIp6Message(MessagePool   &aMessagePool,
                                const Address &aSource,
                                const Address &aDestination,
                                uint8_t        aNextHeader,
                                uint16_t       aTotalLength)
{
    Message *message;
    Header   header;

    VerifyOrQuit(aTotalLength >= sizeof(Header));

    message = aMessagePool.Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);

    header.Clear();
    header.InitVersionTrafficClassFlow();
    header.SetPayloadLength(aTotalLength - sizeof(Header));
    header.SetNextHeader(aNextHeader);
    header.SetHopLimit(kDefaultHopLimit);
    header.SetSource(aSource);
    header.SetDestination(aDestination);

    SuccessOrQuit(message->Append(header));
    SuccessOrQuit(message->SetLength(aTotalLength));

    return message;
}

static void VerifyLengthOutcome(MessagePool   &aMessagePool,
                                const Address &aSource,
                                const Address &aDestination,
                                uint8_t        aNextHeader,
                                uint16_t       aTotalLength,
                                bool           aExpectedIsValid,
                                Error          aExpectedParseError)
{
    Message *message;
    Header   parsedHeader;
    Headers  parsedHeaders;

    message = BuildIp6Message(aMessagePool, aSource, aDestination, aNextHeader, aTotalLength);

    VerifyOrQuit(parsedHeader.ParseFrom(*message) == aExpectedParseError);

    if (aExpectedParseError == kErrorNone)
    {
        VerifyOrQuit(parsedHeader.IsValid() == aExpectedIsValid);
    }

    VerifyOrQuit(parsedHeaders.ParseFrom(*message) == aExpectedParseError);

    message->Free();
}

void TestIp6HeaderMaxLength(void)
{
    static constexpr uint16_t kMaxMplEncapSize = sizeof(Header) + ExtensionHeader::kLengthUnitSize;

    Instance    *instance;
    MessagePool *messagePool;
    Message     *message;
    Header       parsedHeader;
    Headers      parsedHeaders;
    Address      srcAddr;
    Address      dstAddr;

    printf("TestIp6HeaderMaxLength()\n");

    instance = static_cast<Instance *>(testInitInstance());
    VerifyOrQuit(instance != nullptr);
    messagePool = &instance->Get<MessagePool>();

    SuccessOrQuit(srcAddr.FromString("fd00::1"));
    SuccessOrQuit(dstAddr.FromString("ff03::1234"));

    // Truncated message (< sizeof(Header)) must fail.
    message = messagePool->Allocate(Message::kTypeIp6);
    VerifyOrQuit(message != nullptr);
    SuccessOrQuit(message->SetLength(sizeof(Header) - 1));
    VerifyOrQuit(parsedHeader.ParseFrom(*message) == kErrorParse);
    VerifyOrQuit(parsedHeaders.ParseFrom(*message) == kErrorParse);
    message->Free();

    // Non-IPv6 version must fail.
    message = BuildIp6Message(*messagePool, srcAddr, dstAddr, kProtoUdp, sizeof(Header) + sizeof(UdpHeader));
    message->Write<uint8_t>(0, 0x40);
    VerifyOrQuit(parsedHeader.ParseFrom(*message) == kErrorParse);
    VerifyOrQuit(parsedHeaders.ParseFrom(*message) == kErrorParse);
    message->Free();

    // PayloadLength mismatch (larger or smaller than actual bytes) must fail.
    message = BuildIp6Message(*messagePool, srcAddr, dstAddr, kProtoUdp, 100);
    message->Write<uint16_t>(Header::kPayloadLengthFieldOffset, BigEndian::HostSwap16(61));
    VerifyOrQuit(parsedHeader.ParseFrom(*message) == kErrorParse);
    VerifyOrQuit(parsedHeaders.ParseFrom(*message) == kErrorParse);
    message->Write<uint16_t>(Header::kPayloadLengthFieldOffset, BigEndian::HostSwap16(59));
    VerifyOrQuit(parsedHeader.ParseFrom(*message) == kErrorParse);
    VerifyOrQuit(parsedHeaders.ParseFrom(*message) == kErrorParse);
    message->Free();

    // Empty IPv6 packet (40 bytes, kProtoNone) is valid.
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoNone, sizeof(Header), /* aExpectedIsValid */ true,
                        kErrorNone);

#if !OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    // Without fragmentation:
    // - `IsValid()` enforces strict `kMaxDatagramLength` (1280).
    // - `Header::ParseFrom()` and `Headers::ParseFrom()` allow up to `kMaxDatagramLength + kMaxMplEncapSize` (1328).
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp, kMaxDatagramLength, /* aExpectedIsValid */ true,
                        kErrorNone);
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp, kMaxDatagramLength + 1,
                        /* aExpectedIsValid */ false, kErrorNone);
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp,
                        kMaxDatagramLength + ExtensionHeader::kLengthUnitSize, /* aExpectedIsValid */ false,
                        kErrorNone);
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp, kMaxDatagramLength + kMaxMplEncapSize,
                        /* aExpectedIsValid */ false, kErrorNone);
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp, kMaxDatagramLength + kMaxMplEncapSize + 1,
                        /* aExpectedIsValid */ false, kErrorParse);
#else
    // With fragmentation enabled (default kMaxAssembledDatagramLength = 2000):
    // Both `IsValid()` and `ParseFrom()` allow up to `kMaxAssembledDatagramLength` and reject `2001`.
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp, kMaxDatagramLength, /* aExpectedIsValid */ true,
                        kErrorNone);
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp, kMaxDatagramLength + kMaxMplEncapSize,
                        /* aExpectedIsValid */ true, kErrorNone);
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp, kMaxAssembledDatagramLength,
                        /* aExpectedIsValid */ true, kErrorNone);
    VerifyLengthOutcome(*messagePool, srcAddr, dstAddr, kProtoUdp, kMaxAssembledDatagramLength + 1,
                        /* aExpectedIsValid */ false, kErrorParse);
#endif

    testFreeInstance(instance);
}

} // namespace Ip6
} // namespace ot

int main(void)
{
    ot::Ip6::TestIp6Header();
    ot::Ip6::TestIp6HeaderMaxLength();
    printf("All tests passed\n");
    return 0;
}
