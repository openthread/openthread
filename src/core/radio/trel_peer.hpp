/*
 *  Copyright (c) 2019-2025, The OpenThread Authors.
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
 *   This file includes definitions for Thread Radio Encapsulation Link (TREL) peer.
 */

#ifndef TREL_PEER_HPP_
#define TREL_PEER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#include <openthread/trel.h>
#include <openthread/platform/trel.h>

#include "common/as_core_type.hpp"
#include "common/heap_allocatable.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/owning_list.hpp"
#include "common/pool.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/extended_panid.hpp"
#include "net/socket.hpp"
#include "thread/mle_types.hpp"

namespace ot {

class NeighborTable;

namespace Trel {

class Interface;
class PeerTable;

/**
 * Represents a discovered TREL peer.
 */
class Peer : public InstanceLocatorInit,
             public otTrelPeer,
             public LinkedListEntry<Peer>,
#if OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
             public Heap::Allocatable<Peer>,
#endif
             private NonCopyable

{
    friend class Interface;
    friend class PeerTable;
    friend class OwnedPtr<Peer>;
    friend class OwningList<Peer>;
    friend class LinkedList<Peer>;
    friend class LinkedListEntry<Peer>;

public:
    /**
     * Returns the Extended MAC Address of the discovered TREL peer.
     *
     * @returns The Extended MAC Address of the TREL peer.
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return AsCoreType(&mExtAddress); }

    /**
     * Returns the Extended PAN Identifier of the discovered TREL peer.
     *
     * @returns The Extended PAN Identifier of the TREL peer.
     */
    const MeshCoP::ExtendedPanId &GetExtPanId(void) const { return AsCoreType(&mExtPanId); }

    /**
     * Returns the IPv6 socket address of the discovered TREL peer.
     *
     * @returns The IPv6 socket address of the TREL peer.
     */
    const Ip6::SockAddr &GetSockAddr(void) const { return AsCoreType(&mSockAddr); }

    /**
     * Set the IPv6 socket address of the discovered TREL peer.
     *
     * @param[in] aSockAddr   The IPv6 socket address.
     */
    void SetSockAddr(const Ip6::SockAddr &aSockAddr) { mSockAddr = aSockAddr; }

    /**
     * Indicates whether the peer matches a given Extended Address.
     *
     * @param[in] aExtAddress   A Extended Address to match with.
     *
     * @retval TRUE if the peer matches @p aExtAddress.
     * @retval FALSE if the peer does not match @p aExtAddress.
     */
    bool Matches(const Mac::ExtAddress &aExtAddress) const { return GetExtAddress() == aExtAddress; }

    /**
     * Indicates whether the peer matches a given Socket Address.
     *
     * @param[in] aSockAddr   A Socket Address to match with.
     *
     * @retval TRUE if the peer matches @p aSockAddr.
     * @retval FALSE if the peer does not match @p aSockAddr.
     */
    bool Matches(const Ip6::SockAddr &aSockAddr) const { return GetSockAddr() == aSockAddr; }

private:
    enum Action : uint8_t
    {
        kAdded,
        kUpdated,
        kRemoving,
        kEvicting,
    };

    struct OtherExtPanIdMatcher // Matches if Ext PAN ID is different.
    {
        explicit OtherExtPanIdMatcher(const MeshCoP::ExtendedPanId &aExtPanId)
            : mExtPanId(aExtPanId)
        {
        }

        const MeshCoP::ExtendedPanId &mExtPanId;
    };

    struct NonNeighborMatcher
    {
        explicit NonNeighborMatcher(NeighborTable &aNeighborTable)
            : mNeighborTable(aNeighborTable)
        {
        }

        NeighborTable &mNeighborTable;
    };

    void Init(Instance &aInstance);
    void Free(void);
    void SetExtAddress(const Mac::ExtAddress &aExtAddress) { mExtAddress = aExtAddress; }
    void SetExtPanId(const MeshCoP::ExtendedPanId &aExtPanId) { mExtPanId = aExtPanId; }
    bool Matches(const OtherExtPanIdMatcher &aMatcher) const { return GetExtPanId() != aMatcher.mExtPanId; }
    bool Matches(const NonNeighborMatcher &aMatcher) const;

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    void               Log(Action aAction) const;
    static const char *ActionToString(Action aAction);
#else
    void Log(Action) const {}
#endif

    Peer *mNext;
};

//---------------------------------------------------------------------------------------------------------------------

/**
 * Represents the TREL peer table.
 */
class PeerTable : public InstanceLocator, public OwningList<Peer>
{
    friend class Peer;

public:
    /**
     * Represents an iterator for iterating over TREL peer table entries.
     */
    typedef otTrelPeerIterator PeerIterator;

    /**
     * Initializes the TREL peer table.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit PeerTable(Instance &aInstance);

    /**
     * Allocates a new peer entry and adds it to the table.
     *
     * This method attempts to allocate a new `Peer`. If successful, the new peer is added to the peer table and
     * becomes owned by it.
     *
     * If the initial allocation fail, the method will then attempt to make space by evicting an existing peer from the
     * table. The eviction strategy prioritizes removing a peer associated with a different Extended PAN ID. If no
     * such suitable peer is found, a non-neighbor peer will be targeted for eviction. If an existing peer is
     * successfully evicted, allocation for the new `Peer` object is re-attempted.
     *
     * @returns A pointer to the newly allocated and added `Peer`, or `nullptr` if the allocation fails.
     */
    Peer *AllocateAndAddNewPeer(void);

    /**
     * Initializes a peer table iterator.
     *
     * @param[in] aIterator  The iterator to initialize.
     */
    void InitIterator(PeerIterator &aIterator) const { aIterator = GetHead(); }

    /**
     * Iterates over the peer table entries.
     *
     * @param[in] aIterator  The iterator. MUST be initialized.
     *
     * @returns A pointer to the next `Peer` entry or `nullptr` if no more entries in the table.
     */
    const Peer *GetNextPeer(PeerIterator &aIterator) const;

    /**
     * Returns the number of TREL peers.
     *
     * @returns  The number of TREL peers.
     */
    uint16_t GetNumberOfPeers(void) const;

private:
#if !OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
#if OPENTHREAD_CONFIG_TREL_PEER_TABLE_SIZE != 0
    static constexpr uint16_t PoolSize = OPENTHREAD_CONFIG_TREL_PEER_TABLE_SIZE;
#else
    static constexpr uint16_t kExtraEntries = 32;
    static constexpr uint16_t PoolSize      = Mle::kMaxRouters + Mle::kMaxChildren + kExtraEntries;
#endif
#endif

    Peer *AllocatePeer(void);
    Error EvictPeer(void);

#if !OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
    Pool<Peer, PoolSize> mPool;
#endif
};

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#endif // TREL_PEER_HPP_
