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

/**
 * @file
 *   This file includes definitions for the checks a userspace multicast
 *   forwarder applies to every packet: forwarding policy, duplicate
 *   suppression and rate limiting.
 */

#ifndef OT_POSIX_PLATFORM_MULTICAST_FORWARDING_HPP_
#define OT_POSIX_PLATFORM_MULTICAST_FORWARDING_HPP_

#include "openthread-posix-config.h"

#ifndef __linux__

#include <stddef.h>
#include <stdint.h>

#include "core/net/ip6_address.hpp"

namespace ot {
namespace Posix {
namespace MulticastForwarding {

static constexpr uint16_t kIp6HeaderSize      = 40;
static constexpr uint16_t kEthernetHeaderSize = 14;
static constexpr uint16_t kTunnelHeaderSize   = 4; ///< BSD tunnel interfaces prefix packets with the address family.
static constexpr uint8_t  kMacSize            = 6;

enum Verdict : uint8_t
{
    kForward,          ///< The packet may be forwarded.
    kNotIp6,           ///< Not a well-formed IPv6 packet.
    kNotMulticast,     ///< Unicast destination.
    kScopeTooSmall,    ///< Realm-local scope or smaller: never crosses a Backbone Router.
    kBadSource,        ///< Multicast, unspecified or link-local source.
    kHopLimitExceeded, ///< Hop limit 1 or less: forwarding would expire it.
};

/**
 * Checks whether an IPv6 packet may be forwarded to the other side.
 *
 * @param[in] aPacket  The IPv6 packet, starting at the IPv6 header.
 * @param[in] aLength  Its length.
 */
Verdict Check(const uint8_t *aPacket, uint16_t aLength);

const char *VerdictToString(Verdict aVerdict);

/**
 * Reads the destination of an IPv6 packet at least `kIp6HeaderSize` long.
 */
Ip6::Address GetDestination(const uint8_t *aPacket);

/**
 * Reads the source of an IPv6 packet at least `kIp6HeaderSize` long.
 */
Ip6::Address GetSource(const uint8_t *aPacket);

/**
 * Decrements the hop limit, as a router forwarding the packet does.
 */
void DecrementHopLimit(uint8_t *aPacket);

/**
 * Wraps an IPv6 multicast packet in an Ethernet frame: the group's
 * multicast MAC as destination (RFC 2464), @p aSourceMac as source.
 *
 * @returns The frame length, or 0 if @p aFrame is too small.
 */
uint16_t BuildEthernetFrame(const uint8_t *aPacket,
                            uint16_t       aLength,
                            const uint8_t *aSourceMac,
                            uint8_t       *aFrame,
                            uint16_t       aFrameCapacity);

/**
 * Remembers recently forwarded packets so that a packet is forwarded once,
 * whichever interface it shows up on again.
 */
class DedupCache
{
public:
    static constexpr uint16_t kNumEntries = 256;
    static constexpr uint32_t kWindowMs   = 3000;

    DedupCache(void) { Clear(); }

    void Clear(void);

    /**
     * Fingerprints an IPv6 packet. The hop limit is left out so that a copy
     * that travelled a different path still matches.
     */
    static uint64_t Fingerprint(const uint8_t *aPacket, uint16_t aLength);

    /**
     * Records a packet and reports whether it was already known.
     *
     * @param[in] aFingerprint  The packet's fingerprint.
     * @param[in] aNow          The current time in microseconds.
     *
     * @returns TRUE if the packet was seen within the window, FALSE if it is new (and is now recorded).
     */
    bool Check(uint64_t aFingerprint, uint64_t aNow);

private:
    struct Entry
    {
        uint64_t mFingerprint;
        uint64_t mTime;
        bool     mInUse;
    };

    Entry mEntries[kNumEntries];
};

class TokenBucket
{
public:
    void Init(uint32_t aRatePerSecond, uint32_t aBurst, uint64_t aNow);
    bool Take(uint64_t aNow);

private:
    static constexpr uint64_t kScale = 1000000; // tokens are counted in millionths

    uint32_t mRatePerSecond;
    uint32_t mBurst;
    uint64_t mTokens;
    uint64_t mLastRefill;
};

/**
 * Limits forwarded packets per group and overall.
 */
class RateLimiter
{
public:
    static constexpr uint8_t kMaxGroups = 32;

    struct Config
    {
        uint32_t mPerGroupRatePerSecond;
        uint32_t mPerGroupBurst;
        uint32_t mTotalRatePerSecond;
        uint32_t mTotalBurst;
    };

    explicit RateLimiter(const Config &aConfig);

    void Clear(void);

    /**
     * Accounts one packet for @p aGroup.
     *
     * @returns TRUE if the packet is within both limits, FALSE if it must be dropped.
     */
    bool Allow(const Ip6::Address &aGroup, uint64_t aNow);

private:
    struct Group
    {
        Ip6::Address mAddress;
        TokenBucket  mBucket;
        uint64_t     mLastUse;
        bool         mInUse;
    };

    const Config &mConfig;
    TokenBucket   mTotal;
    bool          mTotalInitialized;
    Group         mGroups[kMaxGroups];
};

} // namespace MulticastForwarding
} // namespace Posix
} // namespace ot

#endif // __linux__

#endif // OT_POSIX_PLATFORM_MULTICAST_FORWARDING_HPP_
