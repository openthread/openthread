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
 *   This file includes definitions for Thread Direct Peer and Peer List.
 */

#ifndef OT_CORE_THREAD_DIRECT_TD_PEER_HPP_
#define OT_CORE_THREAD_DIRECT_TD_PEER_HPP_

#include "openthread-core-config.h"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

#include "common/array.hpp"
#include "common/error.hpp"
#include "common/heap_allocatable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/owning_list.hpp"
#include "common/time.hpp"
#include "common/uptime.hpp"
#include "mac/mac_types.hpp"

namespace ot {
namespace ThreadDirect {

class Key;
class KeyManager;
class PeerList;

/**
 * Represents a tracked Thread Direct peer associated with a registered key.
 */
class Peer : public InstanceLocatorInit
#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    ,
             public LinkedListEntry<Peer>,
             public Heap::Allocatable<Peer>
#endif
{
    friend class PeerList;
    friend class LinkedList<Peer>;
    friend class LinkedListEntry<Peer>;

public:
    /**
     * Gets the Extended MAC Address of the peer.
     *
     * @returns The Extended MAC Address of the peer.
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return mExtAddress; }

    /**
     * Indicates whether the peer matches a given Extended MAC Address.
     *
     * @param[in] aExtAddress  The Extended MAC Address to check against.
     *
     * @retval TRUE   If the peer's Extended MAC Address matches @p aExtAddress.
     * @retval FALSE  If the peer's Extended MAC Address does not match @p aExtAddress.
     */
    bool Matches(const Mac::ExtAddress &aExtAddress) const { return mExtAddress == aExtAddress; }

    /**
     * Gets the last received MAC security frame counter from the peer.
     *
     * @returns The last received MAC security frame counter.
     */
    uint32_t GetFrameCounter(void) const { return mFrameCounter; }

    /**
     * Gets the device uptime (in seconds) when a valid frame was last received from the peer.
     *
     * @returns The uptime (in seconds) when a frame was last received from the peer.
     */
    UptimeSec GetLastRecvTime(void) const { return mLastRecvTime; }

    /**
     * Calculates the age of the peer entry (seconds elapsed since `GetLastRecvTime()`).
     *
     * @param[in] aUptimeNow  The current device uptime in seconds.
     *
     * @returns The age of the peer entry in seconds.
     */
    uint32_t GetAge(UptimeSec aUptimeNow) const { return aUptimeNow - mLastRecvTime; }

    /**
     * Verifies a newly received MAC security frame counter and updates the peer's tracked counter and receive time.
     *
     * Regardless of whether @p aFrameCounter is accepted or rejected, the last receive time (`GetLastRecvTime()`)
     * is updated to the current device uptime. The tracked frame counter is updated only when @p aFrameCounter is
     * valid and strictly greater than the current tracked value.
     *
     * @param[in] aFrameCounter  The received MAC security frame counter to verify.
     *
     * @retval kErrorNone        @p aFrameCounter is valid and strictly greater; tracked counter was updated.
     * @retval kErrorDuplicated  @p aFrameCounter equals or is less than the current tracked counter.
     * @retval kErrorSecurity    @p aFrameCounter is `0xffffffff` (max value).
     */
    Error VerifyAndUpdateFrameCounter(uint32_t aFrameCounter);

private:
#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    static constexpr uint32_t kMaxAge = 20 * Time::kOneDayInSec;

    struct ExpireChecker
    {
        UptimeSec mUptimeNow;
    };

    bool Matches(const ExpireChecker &aChecker) const { return GetAge(aChecker.mUptimeNow) > kMaxAge; }
#endif

    void Init(Instance &aInstance, const Mac::ExtAddress &aExtAddress, uint32_t aFrameCounter);

#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    Peer *mNext;
#endif
    Mac::ExtAddress mExtAddress;
    uint32_t        mFrameCounter;
    UptimeSec       mLastRecvTime;
};

/**
 * Represents a list of tracked Thread Direct peers associated with a registered key.
 */
class PeerList : public InstanceLocatorInit, private NonCopyable
{
    friend class Key;
    friend class KeyManager;

public:
    /**
     * Indicates whether the peer list is empty.
     *
     * @retval TRUE   If the peer list is empty.
     * @retval FALSE  If the peer list is not empty.
     */
    bool IsEmpty(void) const { return mPeers.IsEmpty(); }

    /**
     * Clears the peer list (and frees all heap-allocated entries when heap allocation is enabled).
     */
    void Clear(void) { mPeers.Clear(); }

    /**
     * Searches the peer list for an entry matching a given Extended MAC Address.
     *
     * @param[in] aExtAddress  The Extended MAC Address to search for.
     *
     * @returns A pointer to the matching `Peer` entry, or `nullptr` if not found.
     */
    Peer *Find(const Mac::ExtAddress &aExtAddress) { return mPeers.FindMatching(aExtAddress); }

    /**
     * Adds a new peer entry to the list initialized with a given Extended MAC Address and frame counter.
     *
     * When heap allocation (`OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE`) is disabled and the fixed-size list is
     * full, the least-recently-heard peer entry (oldest `GetLastRecvTime()`) is evicted and replaced with the new peer.
     *
     * @param[in] aExtAddress    The Extended MAC Address of the new peer.
     * @param[in] aFrameCounter  The initial received MAC security frame counter of the peer.
     *
     * @returns A pointer to the added `Peer` entry, or `nullptr` if an entry could not be allocated.
     */
    Peer *AddNew(const Mac::ExtAddress &aExtAddress, uint32_t aFrameCounter);

private:
#if !OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    static constexpr uint16_t kListSize = OPENTHREAD_CONFIG_TD_PEER_LIST_SIZE_PER_KEY;
#endif

    void Init(Instance &aInstance);
#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    void RemoveExpired(UptimeSec aUptimeNow);
#endif

#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    OwningList<Peer> mPeers;
#else
    Array<Peer, kListSize> mPeers;
#endif
};

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE

#endif // OT_CORE_THREAD_DIRECT_TD_PEER_HPP_
