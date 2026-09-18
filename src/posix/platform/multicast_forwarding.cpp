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

#include "posix/platform/multicast_forwarding.hpp"

#ifndef __linux__

#include <string.h>

#include "common/code_utils.hpp"

namespace ot {
namespace Posix {
namespace MulticastForwarding {

static constexpr uint8_t kPayloadLengthOffset = 4;
static constexpr uint8_t kHopLimitOffset      = 7;
static constexpr uint8_t kSourceOffset        = 8;
static constexpr uint8_t kDestinationOffset   = 24;
static constexpr uint8_t kMinHopLimit         = 2;

Verdict Check(const uint8_t *aPacket, uint16_t aLength)
{
    Verdict      verdict = kForward;
    Ip6::Address destination;
    Ip6::Address source;
    uint16_t     payloadLength;

    VerifyOrExit(aLength >= kIp6HeaderSize && (aPacket[0] >> 4) == 6, verdict = kNotIp6);
    payloadLength = static_cast<uint16_t>((aPacket[kPayloadLengthOffset] << 8) | aPacket[kPayloadLengthOffset + 1]);
    VerifyOrExit(static_cast<uint32_t>(kIp6HeaderSize) + payloadLength <= aLength, verdict = kNotIp6);

    destination = GetDestination(aPacket);
    VerifyOrExit(destination.IsMulticast(), verdict = kNotMulticast);
    VerifyOrExit(destination.GetScope() >= Ip6::Address::kAdminLocalScope, verdict = kScopeTooSmall);

    source = GetSource(aPacket);
    VerifyOrExit(!source.IsMulticast() && !source.IsUnspecified() && !source.IsLinkLocalUnicast(),
                 verdict = kBadSource);

    VerifyOrExit(aPacket[kHopLimitOffset] >= kMinHopLimit, verdict = kHopLimitExceeded);

exit:
    return verdict;
}

const char *VerdictToString(Verdict aVerdict)
{
    const char *str = "unknown";

    switch (aVerdict)
    {
    case kForward:
        str = "forward";
        break;
    case kNotIp6:
        str = "not IPv6";
        break;
    case kNotMulticast:
        str = "not multicast";
        break;
    case kScopeTooSmall:
        str = "scope too small";
        break;
    case kBadSource:
        str = "bad source";
        break;
    case kHopLimitExceeded:
        str = "hop limit exceeded";
        break;
    }

    return str;
}

Ip6::Address GetDestination(const uint8_t *aPacket)
{
    Ip6::Address address;

    address.InitFrom(aPacket + kDestinationOffset);
    return address;
}

Ip6::Address GetSource(const uint8_t *aPacket)
{
    Ip6::Address address;

    address.InitFrom(aPacket + kSourceOffset);
    return address;
}

void DecrementHopLimit(uint8_t *aPacket)
{
    if (aPacket[kHopLimitOffset] > 0)
    {
        aPacket[kHopLimitOffset]--;
    }
}

uint16_t BuildEthernetFrame(const uint8_t *aPacket,
                            uint16_t       aLength,
                            const uint8_t *aSourceMac,
                            uint8_t       *aFrame,
                            uint16_t       aFrameCapacity)
{
    uint16_t length = 0;

    VerifyOrExit(aLength >= kIp6HeaderSize);
    VerifyOrExit(static_cast<uint32_t>(kEthernetHeaderSize) + aLength <= aFrameCapacity);

    // RFC 2464 section 7: 33:33 followed by the group's low 32 bits.
    aFrame[0] = 0x33;
    aFrame[1] = 0x33;
    memcpy(aFrame + 2, aPacket + kDestinationOffset + 12, 4);
    memcpy(aFrame + kMacSize, aSourceMac, kMacSize);
    aFrame[12] = 0x86;
    aFrame[13] = 0xdd;
    memcpy(aFrame + kEthernetHeaderSize, aPacket, aLength);
    length = kEthernetHeaderSize + aLength;

exit:
    return length;
}

//---------------------------------------------------------------------------------------------------------------------
// DedupCache

void DedupCache::Clear(void) { memset(mEntries, 0, sizeof(mEntries)); }

uint64_t DedupCache::Fingerprint(const uint8_t *aPacket, uint16_t aLength)
{
    // FNV-1a, 64-bit.
    uint64_t hash = 0xcbf29ce484222325ULL;

    for (uint16_t i = 0; i < aLength; i++)
    {
        hash ^= (i == kHopLimitOffset) ? 0 : aPacket[i];
        hash *= 0x100000001b3ULL;
    }

    return hash;
}

bool DedupCache::Check(uint64_t aFingerprint, uint64_t aNow)
{
    static constexpr uint64_t kWindowUs = static_cast<uint64_t>(kWindowMs) * 1000;

    bool   isDuplicate = false;
    Entry *slot        = nullptr;

    for (Entry &entry : mEntries)
    {
        if (entry.mInUse && aNow - entry.mTime > kWindowUs)
        {
            entry.mInUse = false;
        }

        if (!entry.mInUse)
        {
            if (slot == nullptr || slot->mInUse)
            {
                slot = &entry;
            }
            continue;
        }

        if (entry.mFingerprint == aFingerprint)
        {
            // Seen again: the window restarts.
            entry.mTime = aNow;
            ExitNow(isDuplicate = true);
        }

        // With no free entry the least recently seen packet makes room.
        if (slot == nullptr || (slot->mInUse && entry.mTime < slot->mTime))
        {
            slot = &entry;
        }
    }

    slot->mFingerprint = aFingerprint;
    slot->mTime        = aNow;
    slot->mInUse       = true;

exit:
    return isDuplicate;
}

//---------------------------------------------------------------------------------------------------------------------
// TokenBucket

void TokenBucket::Init(uint32_t aRatePerSecond, uint32_t aBurst, uint64_t aNow)
{
    mRatePerSecond = aRatePerSecond;
    mBurst         = aBurst;
    mTokens        = aBurst * kScale;
    mLastRefill    = aNow;
}

bool TokenBucket::Take(uint64_t aNow)
{
    bool     taken   = false;
    uint64_t full    = mBurst * kScale;
    uint64_t elapsed = (aNow > mLastRefill) ? (aNow - mLastRefill) : 0;

    // One microsecond earns `mRatePerSecond` millionths of a token.
    if (elapsed >= full / (mRatePerSecond ? mRatePerSecond : 1))
    {
        mTokens = full;
    }
    else
    {
        mTokens += elapsed * mRatePerSecond;
        mTokens = (mTokens > full) ? full : mTokens;
    }
    mLastRefill = aNow;

    if (mTokens >= kScale)
    {
        mTokens -= kScale;
        taken = true;
    }

    return taken;
}

//---------------------------------------------------------------------------------------------------------------------
// RateLimiter

RateLimiter::RateLimiter(const Config &aConfig)
    : mConfig(aConfig)
{
    Clear();
}

void RateLimiter::Clear(void)
{
    mTotalInitialized = false;

    for (Group &group : mGroups)
    {
        group.mInUse = false;
    }
}

bool RateLimiter::Allow(const Ip6::Address &aGroup, uint64_t aNow)
{
    bool   allowed = false;
    Group *slot    = nullptr;
    Group *match   = nullptr;

    if (!mTotalInitialized)
    {
        mTotal.Init(mConfig.mTotalRatePerSecond, mConfig.mTotalBurst, aNow);
        mTotalInitialized = true;
    }

    for (Group &group : mGroups)
    {
        if (group.mInUse && group.mAddress == aGroup)
        {
            match = &group;
            break;
        }

        // A new group takes a free entry, or else the one idle the longest.
        if (slot == nullptr || (slot->mInUse && (!group.mInUse || group.mLastUse < slot->mLastUse)))
        {
            slot = &group;
        }
    }

    if (match == nullptr)
    {
        match           = slot;
        match->mAddress = aGroup;
        match->mInUse   = true;
        match->mBucket.Init(mConfig.mPerGroupRatePerSecond, mConfig.mPerGroupBurst, aNow);
    }

    match->mLastUse = aNow;

    // The group first: a group over its own limit must not drain the budget shared with the others.
    VerifyOrExit(match->mBucket.Take(aNow));
    VerifyOrExit(mTotal.Take(aNow));
    allowed = true;

exit:
    return allowed;
}

} // namespace MulticastForwarding
} // namespace Posix
} // namespace ot

#endif // __linux__
