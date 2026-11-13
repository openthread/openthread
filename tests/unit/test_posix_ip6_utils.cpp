/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include "ip6_utils.hpp"

#include <stdint.h>
#include <string.h>

#include "test_util.h"

namespace {

using namespace ot::Posix::Ip6Utils;

uint16_t VerifyTransportChecksum(const uint8_t *aPacket, uint16_t aUpperLayerLength, uint8_t aNextHeader)
{
    uint32_t sum = 0;

    sum += SumChecksumBytes(&aPacket[kIp6SourceOffset], 2 * OT_IP6_ADDRESS_SIZE);
    sum += aUpperLayerLength;
    sum += aNextHeader;
    sum += SumChecksumBytes(&aPacket[OT_IP6_HEADER_SIZE], aUpperLayerLength);

    while ((sum >> 16) != 0)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return static_cast<uint16_t>(sum);
}

void InitIp6Header(uint8_t *aPacket, uint16_t aPayloadLength, uint8_t aNextHeader)
{
    static const uint8_t kSrc[OT_IP6_ADDRESS_SIZE] = {
        0xfd, 0x27, 0xe0, 0xa9, 0xb9, 0x36, 0xb2, 0xba, 0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0xfc, 0x00,
    };
    static const uint8_t kDst[OT_IP6_ADDRESS_SIZE] = {
        0xfd, 0x27, 0xe0, 0xa9, 0xb9, 0x36, 0xb2, 0xba, 0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0xfc, 0x00,
    };

    memset(aPacket, 0, OT_IP6_HEADER_SIZE);
    aPacket[0]                           = 0x60;
    aPacket[kIp6PayloadLengthOffset]     = static_cast<uint8_t>(aPayloadLength >> 8);
    aPacket[kIp6PayloadLengthOffset + 1] = static_cast<uint8_t>(aPayloadLength & 0xff);
    aPacket[OT_IP6_HEADER_PROTO_OFFSET]  = aNextHeader;
    aPacket[kIp6HopLimitOffset]          = 64;
    memcpy(&aPacket[kIp6SourceOffset], kSrc, sizeof(kSrc));
    memcpy(&aPacket[kIp6DestinationOffset], kDst, sizeof(kDst));
}

void TestUdpChecksumWithOddLengthPayloadAndPseudoSeed(void)
{
    // Simulate a macOS utun CoAP GET request (odd 17-byte CoAP payload) with a partial pseudo-header sum in uh_sum.
    static const uint8_t kCoapPayload[] = {
        0x40, 0x01, 0x12, 0x34, 0xbc, 'T', 'e', 's', 't', 'R', 'e', 's', 'o', 'u', 'r', 'c', 'e',
    };
    constexpr uint16_t kUdpLen = 8 + sizeof(kCoapPayload);

    uint8_t packet[OT_IP6_HEADER_SIZE + kUdpLen];

    InitIp6Header(packet, kUdpLen, OT_IP6_PROTO_UDP);

    // UDP header: sport=59123, dport=5683, ulen=kUdpLen, uh_sum=0xbeef (stale/pseudo-header seed).
    packet[OT_IP6_HEADER_SIZE + 0] = 0xe6;
    packet[OT_IP6_HEADER_SIZE + 1] = 0xf3;
    packet[OT_IP6_HEADER_SIZE + 2] = 0x16;
    packet[OT_IP6_HEADER_SIZE + 3] = 0x33;
    packet[OT_IP6_HEADER_SIZE + 4] = static_cast<uint8_t>(kUdpLen >> 8);
    packet[OT_IP6_HEADER_SIZE + 5] = static_cast<uint8_t>(kUdpLen & 0xff);
    packet[OT_IP6_HEADER_SIZE + 6] = 0xbe;
    packet[OT_IP6_HEADER_SIZE + 7] = 0xef;
    memcpy(&packet[OT_IP6_HEADER_SIZE + 8], kCoapPayload, sizeof(kCoapPayload));

    UpdateIp6TransportChecksum(packet, sizeof(packet));

    VerifyOrQuit(((packet[OT_IP6_HEADER_SIZE + 6] << 8) | packet[OT_IP6_HEADER_SIZE + 7]) != 0);
    VerifyOrQuit(VerifyTransportChecksum(packet, kUdpLen, OT_IP6_PROTO_UDP) == 0xffff);
}

void TestUdpChecksumUsesUdpHeaderLength(void)
{
    // RFC 8200 Section 8.1: UDP uses the UDP header Length field rather than IPv6 Payload Length when padding trails.
    constexpr uint16_t kUdpLen     = 12;
    constexpr uint16_t kPayloadLen = 16;

    uint8_t packet[OT_IP6_HEADER_SIZE + kPayloadLen];

    InitIp6Header(packet, kPayloadLen, OT_IP6_PROTO_UDP);
    memset(&packet[OT_IP6_HEADER_SIZE], 0x5a, kPayloadLen);
    packet[OT_IP6_HEADER_SIZE + 0] = 0x12;
    packet[OT_IP6_HEADER_SIZE + 1] = 0x34;
    packet[OT_IP6_HEADER_SIZE + 2] = 0x16;
    packet[OT_IP6_HEADER_SIZE + 3] = 0x33;
    packet[OT_IP6_HEADER_SIZE + 4] = 0x00;
    packet[OT_IP6_HEADER_SIZE + 5] = kUdpLen;

    UpdateIp6TransportChecksum(packet, sizeof(packet));

    VerifyOrQuit(VerifyTransportChecksum(packet, kUdpLen, OT_IP6_PROTO_UDP) == 0xffff);
    VerifyOrQuit(packet[OT_IP6_HEADER_SIZE + 12] == 0x5a);
    VerifyOrQuit(packet[OT_IP6_HEADER_SIZE + 15] == 0x5a);
}

void TestUdpZeroChecksumConvertedToAllOnes(void)
{
    constexpr uint16_t kUdpLen = 10;
    uint8_t            packet[OT_IP6_HEADER_SIZE + kUdpLen];
    uint32_t           sum = 0;
    uint16_t           complementWord;

    InitIp6Header(packet, kUdpLen, OT_IP6_PROTO_UDP);
    memset(&packet[OT_IP6_HEADER_SIZE], 0, kUdpLen);
    packet[OT_IP6_HEADER_SIZE + 0] = 0x12;
    packet[OT_IP6_HEADER_SIZE + 1] = 0x34;
    packet[OT_IP6_HEADER_SIZE + 2] = 0x16;
    packet[OT_IP6_HEADER_SIZE + 3] = 0x33;
    packet[OT_IP6_HEADER_SIZE + 4] = 0x00;
    packet[OT_IP6_HEADER_SIZE + 5] = kUdpLen;

    // Choose the 2-byte UDP payload so the 16-bit one's complement sum equals 0xffff (~sum == 0x0000).
    sum += SumChecksumBytes(&packet[kIp6SourceOffset], 2 * OT_IP6_ADDRESS_SIZE);
    sum += kUdpLen;
    sum += OT_IP6_PROTO_UDP;
    sum += SumChecksumBytes(&packet[OT_IP6_HEADER_SIZE], 8);
    while ((sum >> 16) != 0)
    {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    complementWord                 = static_cast<uint16_t>(~sum);
    packet[OT_IP6_HEADER_SIZE + 8] = static_cast<uint8_t>(complementWord >> 8);
    packet[OT_IP6_HEADER_SIZE + 9] = static_cast<uint8_t>(complementWord & 0xff);

    UpdateIp6TransportChecksum(packet, sizeof(packet));

    VerifyOrQuit(packet[OT_IP6_HEADER_SIZE + 6] == 0xff);
    VerifyOrQuit(packet[OT_IP6_HEADER_SIZE + 7] == 0xff);
    VerifyOrQuit(VerifyTransportChecksum(packet, kUdpLen, OT_IP6_PROTO_UDP) == 0xffff);
}

void TestTcpChecksum(void)
{
    constexpr uint16_t kTcpLen = 24;
    uint8_t            packet[OT_IP6_HEADER_SIZE + kTcpLen];

    InitIp6Header(packet, kTcpLen, OT_IP6_PROTO_TCP);
    memset(&packet[OT_IP6_HEADER_SIZE], 0, kTcpLen);
    packet[OT_IP6_HEADER_SIZE + 0]  = 0xc0;
    packet[OT_IP6_HEADER_SIZE + 1]  = 0x01;
    packet[OT_IP6_HEADER_SIZE + 2]  = 0x00;
    packet[OT_IP6_HEADER_SIZE + 3]  = 0x50;
    packet[OT_IP6_HEADER_SIZE + 12] = 0x60;
    packet[OT_IP6_HEADER_SIZE + 13] = 0x02;
    packet[OT_IP6_HEADER_SIZE + 16] = 0xaa;
    packet[OT_IP6_HEADER_SIZE + 17] = 0x55;
    packet[OT_IP6_HEADER_SIZE + 20] = 0x02;
    packet[OT_IP6_HEADER_SIZE + 21] = 0x04;
    packet[OT_IP6_HEADER_SIZE + 22] = 0x04;
    packet[OT_IP6_HEADER_SIZE + 23] = 0xc4;

    UpdateIp6TransportChecksum(packet, sizeof(packet));

    VerifyOrQuit(VerifyTransportChecksum(packet, kTcpLen, OT_IP6_PROTO_TCP) == 0xffff);
}

void TestIgnoredAndMalformedPacketsUnmodified(void)
{
    uint8_t packet[OT_IP6_HEADER_SIZE + 20];
    uint8_t expected[sizeof(packet)];

    // Null and short buffers.
    UpdateIp6TransportChecksum(nullptr, 0);
    memset(packet, 0xa5, sizeof(packet));
    memcpy(expected, packet, sizeof(packet));
    UpdateIp6TransportChecksum(packet, OT_IP6_HEADER_SIZE - 1);
    VerifyOrQuit(memcmp(packet, expected, sizeof(packet)) == 0);

    // Non-IPv6 version.
    InitIp6Header(packet, 8, OT_IP6_PROTO_UDP);
    packet[0] = 0x45;
    memcpy(expected, packet, sizeof(packet));
    UpdateIp6TransportChecksum(packet, sizeof(packet));
    VerifyOrQuit(memcmp(packet, expected, sizeof(packet)) == 0);

    // Payload length exceeding buffer length.
    InitIp6Header(packet, 21, OT_IP6_PROTO_UDP);
    memcpy(expected, packet, sizeof(packet));
    UpdateIp6TransportChecksum(packet, sizeof(packet));
    VerifyOrQuit(memcmp(packet, expected, sizeof(packet)) == 0);

    // Truncated UDP and TCP headers.
    InitIp6Header(packet, 7, OT_IP6_PROTO_UDP);
    memcpy(expected, packet, sizeof(packet));
    UpdateIp6TransportChecksum(packet, sizeof(packet));
    VerifyOrQuit(memcmp(packet, expected, sizeof(packet)) == 0);

    InitIp6Header(packet, 19, OT_IP6_PROTO_TCP);
    memcpy(expected, packet, sizeof(packet));
    UpdateIp6TransportChecksum(packet, sizeof(packet));
    VerifyOrQuit(memcmp(packet, expected, sizeof(packet)) == 0);

    // Malformed UDP header length (< 8 or > IPv6 payload length).
    InitIp6Header(packet, 12, OT_IP6_PROTO_UDP);
    packet[OT_IP6_HEADER_SIZE + 4] = 0x00;
    packet[OT_IP6_HEADER_SIZE + 5] = 7;
    memcpy(expected, packet, sizeof(packet));
    UpdateIp6TransportChecksum(packet, sizeof(packet));
    VerifyOrQuit(memcmp(packet, expected, sizeof(packet)) == 0);

    InitIp6Header(packet, 12, OT_IP6_PROTO_UDP);
    packet[OT_IP6_HEADER_SIZE + 4] = 0x00;
    packet[OT_IP6_HEADER_SIZE + 5] = 13;
    memcpy(expected, packet, sizeof(packet));
    UpdateIp6TransportChecksum(packet, sizeof(packet));
    VerifyOrQuit(memcmp(packet, expected, sizeof(packet)) == 0);

    // ICMPv6 is left untouched.
    InitIp6Header(packet, 8, OT_IP6_PROTO_ICMP6);
    packet[OT_IP6_HEADER_SIZE + 2] = 0x12;
    packet[OT_IP6_HEADER_SIZE + 3] = 0x34;
    memcpy(expected, packet, sizeof(packet));
    UpdateIp6TransportChecksum(packet, sizeof(packet));
    VerifyOrQuit(memcmp(packet, expected, sizeof(packet)) == 0);
}

} // namespace

int main(void)
{
    TestUdpChecksumWithOddLengthPayloadAndPseudoSeed();
    TestUdpChecksumUsesUdpHeaderLength();
    TestUdpZeroChecksumConvertedToAllOnes();
    TestTcpChecksum();
    TestIgnoredAndMalformedPacketsUnmodified();

    return 0;
}
