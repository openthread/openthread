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

#include <gtest/gtest.h>

#include <string.h>

#include "core/net/ip6_address.hpp"
#include "posix/platform/multicast_forwarding.hpp"

using namespace ot;
using namespace ot::Posix::MulticastForwarding;

namespace {

constexpr uint64_t kSecond = 1000000; // otPlatTimeGet() counts microseconds

// An IPv6 header followed by `aPayloadLength` bytes of payload, in `aBuffer` (which may be larger: padding).
uint16_t MakePacket(uint8_t    *aBuffer,
                    const char *aSource,
                    const char *aDestination,
                    uint16_t    aPayloadLength,
                    uint8_t     aHopLimit = 64)
{
    Ip6::Address source;
    Ip6::Address destination;

    EXPECT_EQ(source.FromString(aSource), kErrorNone);
    EXPECT_EQ(destination.FromString(aDestination), kErrorNone);

    memset(aBuffer, 0, kIp6HeaderSize + aPayloadLength);
    aBuffer[0] = 0x60;
    aBuffer[4] = static_cast<uint8_t>(aPayloadLength >> 8);
    aBuffer[5] = static_cast<uint8_t>(aPayloadLength);
    aBuffer[6] = 17; // UDP
    aBuffer[7] = aHopLimit;
    memcpy(aBuffer + 8, source.GetBytes(), 16);
    memcpy(aBuffer + 24, destination.GetBytes(), 16);

    for (uint16_t i = 0; i < aPayloadLength; i++)
    {
        aBuffer[kIp6HeaderSize + i] = static_cast<uint8_t>(i);
    }

    return kIp6HeaderSize + aPayloadLength;
}

Ip6::NetworkPrefix MeshLocalPrefix(void)
{
    Ip6::Address address;

    EXPECT_EQ(address.FromString("fd7c:1343:5997:b9e7::"), kErrorNone);
    return address.GetPrefix();
}

TEST(Check, ForwardsSiteLocalGroupFromUlaSource)
{
    uint8_t  packet[100];
    uint16_t length       = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::abcd", 10);
    uint16_t packetLength = 0;

    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kForward);
    EXPECT_EQ(packetLength, length);
}

TEST(Check, ReportsRealLengthOfPaddedFrame)
{
    // Ethernet pads small frames: the capture is longer than the packet.
    uint8_t  packet[100];
    uint16_t length       = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::abcd", 4);
    uint16_t packetLength = 0;

    memset(packet + length, 0xa5, 20);
    EXPECT_EQ(Check(packet, length + 20, nullptr, packetLength), kForward);
    EXPECT_EQ(packetLength, length);
}

TEST(Check, RejectsTruncatedAndNonIp6)
{
    uint8_t  packet[100];
    uint16_t length = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::abcd", 40);
    uint16_t packetLength;

    EXPECT_EQ(Check(packet, length - 1, nullptr, packetLength), kNotIp6);         // payload length beyond the capture
    EXPECT_EQ(Check(packet, kIp6HeaderSize - 1, nullptr, packetLength), kNotIp6); // shorter than a header
    packet[0] = 0x45;
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kNotIp6);
}

TEST(Check, RejectsUnicastDestination)
{
    uint8_t  packet[100];
    uint16_t length = MakePacket(packet, "fd26:e30:5393:1::1", "fd26:e30:5393:1::2", 10);
    uint16_t packetLength;

    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kNotMulticast);
}

TEST(Check, RejectsScopesSmallerThanAdminLocal)
{
    uint8_t  packet[100];
    uint16_t packetLength;
    uint16_t length;

    length = MakePacket(packet, "fd26:e30:5393:1::1", "ff02::1", 10);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kScopeTooSmall);
    length = MakePacket(packet, "fd26:e30:5393:1::1", "ff03::1", 10);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kScopeTooSmall);
    length = MakePacket(packet, "fd26:e30:5393:1::1", "ff04::1", 10);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kForward);
    length = MakePacket(packet, "fd26:e30:5393:1::1", "ff0e::1", 10);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kForward);
}

TEST(Check, RejectsBadSources)
{
    uint8_t            packet[100];
    uint16_t           packetLength;
    uint16_t           length;
    Ip6::NetworkPrefix meshLocal = MeshLocalPrefix();

    length = MakePacket(packet, "fe80::1", "ff05::1", 10);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kBadSource);
    length = MakePacket(packet, "::", "ff05::1", 10);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kBadSource);
    length = MakePacket(packet, "ff05::2", "ff05::1", 10);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kBadSource);
    length = MakePacket(packet, "::1", "ff05::1", 10);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kBadSource);

    // Mesh-local sources stay in the mesh; without a prefix to compare against they pass.
    length = MakePacket(packet, "fd7c:1343:5997:b9e7:0:ff:fe00:fc00", "ff05::1", 10);
    EXPECT_EQ(Check(packet, length, &meshLocal, packetLength), kBadSource);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kForward);
    length = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::1", 10);
    EXPECT_EQ(Check(packet, length, &meshLocal, packetLength), kForward);
}

TEST(Check, RejectsHopLimitBelowTwo)
{
    uint8_t  packet[100];
    uint16_t packetLength;
    uint16_t length;

    length = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::1", 10, 1);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kHopLimitExceeded);
    length = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::1", 10, 0);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kHopLimitExceeded);
    length = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::1", 10, 2);
    EXPECT_EQ(Check(packet, length, nullptr, packetLength), kForward);
}

TEST(DedupCache, SecondSightingIsDuplicateAndHopLimitIsIgnored)
{
    DedupCache cache;
    uint8_t    packet[100];
    uint16_t   length      = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::1", 10, 64);
    uint64_t   fingerprint = DedupCache::Fingerprint(packet, length);

    cache.Clear();
    EXPECT_FALSE(cache.Check(fingerprint, 0));
    EXPECT_TRUE(cache.Check(fingerprint, 10));

    packet[7] = 63; // one hop later, same packet
    EXPECT_EQ(DedupCache::Fingerprint(packet, length), fingerprint);

    packet[kIp6HeaderSize] ^= 1;
    EXPECT_FALSE(cache.Check(DedupCache::Fingerprint(packet, length), 20));
}

TEST(DedupCache, WindowRunsFromTheFirstSighting)
{
    // A sender repeating an identical packet more often than the window must not be suppressed for ever.
    DedupCache cache;
    uint64_t   window = static_cast<uint64_t>(DedupCache::kWindowMs) * 1000;

    cache.Clear();
    EXPECT_FALSE(cache.Check(42, 0));
    EXPECT_TRUE(cache.Check(42, window / 3));
    EXPECT_TRUE(cache.Check(42, 2 * window / 3));
    EXPECT_FALSE(cache.Check(42, window + 1));
}

TEST(DedupCache, EvictsTheOldestWhenFull)
{
    DedupCache cache;

    cache.Clear();
    for (uint32_t i = 0; i < DedupCache::kNumEntries; i++)
    {
        EXPECT_FALSE(cache.Check(1000 + i, i));
    }
    // Every miss takes the least recently seen entry's slot.
    EXPECT_FALSE(cache.Check(5000, DedupCache::kNumEntries)); // evicts 1000
    EXPECT_TRUE(cache.Check(1002, DedupCache::kNumEntries + 1));
    EXPECT_FALSE(cache.Check(1000, DedupCache::kNumEntries + 2)); // gone; evicts 1001
    EXPECT_FALSE(cache.Check(1001, DedupCache::kNumEntries + 3)); // gone; evicts 1002
    EXPECT_FALSE(cache.Check(1002, DedupCache::kNumEntries + 4)); // gone; evicts 1003
    EXPECT_TRUE(cache.Check(1004, DedupCache::kNumEntries + 5));
}

TEST(TokenBucket, ZeroRateNeverRefills)
{
    TokenBucket bucket;

    bucket.Init(/* aRatePerSecond */ 0, /* aBurst */ 2, 0);
    EXPECT_TRUE(bucket.Take(0));
    EXPECT_TRUE(bucket.Take(0));
    EXPECT_FALSE(bucket.Take(0));
    EXPECT_FALSE(bucket.Take(10 * kSecond));
    EXPECT_FALSE(bucket.Take(1000 * kSecond));
}

TEST(TokenBucket, RefillsAtRateUpToBurst)
{
    TokenBucket bucket;

    bucket.Init(/* aRatePerSecond */ 10, /* aBurst */ 5, 0);
    for (int i = 0; i < 5; i++)
    {
        EXPECT_TRUE(bucket.Take(0));
    }
    EXPECT_FALSE(bucket.Take(0));
    EXPECT_FALSE(bucket.Take(kSecond / 20)); // 50 ms: half a token
    EXPECT_TRUE(bucket.Take(kSecond / 10));  // 100 ms: one token
    EXPECT_FALSE(bucket.Take(kSecond / 10));
    for (int i = 0; i < 5; i++)
    {
        EXPECT_TRUE(bucket.Take(100 * kSecond)); // long idle: full again, but not more than the burst
    }
    EXPECT_FALSE(bucket.Take(100 * kSecond));
}

TEST(RateLimiter, GroupOverItsLimitDoesNotDrainTheSharedBudget)
{
    RateLimiter::Config config = {/* mPerGroupRatePerSecond */ 0, /* mPerGroupBurst */ 1,
                                  /* mTotalRatePerSecond */ 0, /* mTotalBurst */ 10};
    RateLimiter         limiter(config);
    Ip6::Address        a, b;

    EXPECT_EQ(a.FromString("ff05::a"), kErrorNone);
    EXPECT_EQ(b.FromString("ff05::b"), kErrorNone);

    EXPECT_TRUE(limiter.Allow(a, 0));
    for (int i = 0; i < 20; i++)
    {
        EXPECT_FALSE(limiter.Allow(a, 0));
    }
    EXPECT_TRUE(limiter.Allow(b, 0)); // the shared budget has 9 left
}

TEST(RateLimiter, GroupIsNotChargedWhenTheSharedBudgetIsExhausted)
{
    RateLimiter::Config config = {/* mPerGroupRatePerSecond */ 0, /* mPerGroupBurst */ 1,
                                  /* mTotalRatePerSecond */ 2, /* mTotalBurst */ 1};
    RateLimiter         limiter(config);
    Ip6::Address        a, b;

    EXPECT_EQ(a.FromString("ff05::a"), kErrorNone);
    EXPECT_EQ(b.FromString("ff05::b"), kErrorNone);

    EXPECT_TRUE(limiter.Allow(a, 0));           // takes the only shared token
    EXPECT_FALSE(limiter.Allow(b, 0));          // shared budget exhausted: b's own token must survive this
    EXPECT_TRUE(limiter.Allow(b, kSecond / 2)); // shared budget refilled one token; b spends its own
    EXPECT_FALSE(limiter.Allow(b, kSecond));    // b's own bucket is now empty (rate 0)
}

TEST(BuildEthernetFrame, MapsGroupToMulticastMac)
{
    uint8_t  packet[100];
    uint8_t  frame[200];
    uint8_t  mac[kMacSize] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};
    uint16_t length        = MakePacket(packet, "fd26:e30:5393:1::1", "ff05::1234:5678:9abc:def0", 10);
    uint16_t frameLength   = BuildEthernetFrame(packet, length, mac, frame, sizeof(frame));

    ASSERT_EQ(frameLength, kEthernetHeaderSize + length);
    EXPECT_EQ(frame[0], 0x33);
    EXPECT_EQ(frame[1], 0x33);
    EXPECT_EQ(frame[2], 0x9a);
    EXPECT_EQ(frame[3], 0xbc);
    EXPECT_EQ(frame[4], 0xde);
    EXPECT_EQ(frame[5], 0xf0);
    EXPECT_EQ(memcmp(frame + 6, mac, kMacSize), 0);
    EXPECT_EQ(frame[12], 0x86);
    EXPECT_EQ(frame[13], 0xdd);
    EXPECT_EQ(memcmp(frame + kEthernetHeaderSize, packet, length), 0);

    EXPECT_EQ(BuildEthernetFrame(packet, length, mac, frame, kEthernetHeaderSize + length - 1), 0);
}

} // namespace
