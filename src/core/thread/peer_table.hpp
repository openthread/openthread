/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes definitions for the Thread Peer table.
 */

#ifndef PEER_TABLE_HPP_
#define PEER_TABLE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_P2P_ENABLE

#include "common/const_cast.hpp"
#include "common/iterator_utils.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "thread/peer.hpp"

namespace ot {

/**
 * Represents the Thread Peer table.
 */
class PeerTable : public InstanceLocator, private NonCopyable
{
    friend class NeighborTable;
    class IteratorBuilder;

public:
    /**
     * Represents an iterator for iterating through the peer entries in the peer table.
     */
    class Iterator : public InstanceLocator, public ItemPtrIterator<Peer, Iterator>
    {
        friend class ItemPtrIterator<Peer, Iterator>;
        friend class IteratorBuilder;

    public:
        /**
         * Initializes an `Iterator` instance.
         *
         * @param[in] aInstance  The OpenThread instance.
         * @param[in] aFilter    A peer state filter.
         */
        Iterator(Instance &aInstance, Peer::StateFilter aFilter);

        /**
         * Resets the iterator to start over.
         */
        void Reset(void);

        /**
         * Gets the `Peer` entry to which the iterator is currently pointing.
         *
         * @returns A pointer to the `Peer` entry, or `nullptr` if the iterator is done and/or empty.
         */
        Peer *GetPeer(void) { return mItem; }

    private:
        explicit Iterator(Instance &aInstance)
            : InstanceLocator(aInstance)
            , mFilter(Peer::StateFilter::kInStateValid)
        {
        }

        void Advance(void);

        Peer::StateFilter mFilter;
    };

    /**
     * Initializes a `PeerTable` instance.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit PeerTable(Instance &aInstance);

    /**
     * Clears the peer table.
     */
    void Clear(void);

    /**
     * Gets a new/unused `Peer` entry from the peer table.
     *
     * @note The returned peer entry will be cleared (`memset` to zero).
     *
     * @returns A pointer to a new `Peer` entry, or `nullptr` if all `Peer` entries are in use.
     */
    Peer *GetNewPeer(void);

    /**
     * Searches the peer table for a `Peer` with a given extended address also matching a given state
     * filter.
     *
     * @param[in]  aExtAddress A reference to an extended address.
     * @param[in]  aFilter     A peer state filter.
     *
     * @returns  A pointer to the `Peer` entry if one is found, or `nullptr` otherwise.
     */
    Peer *FindPeer(const Mac::ExtAddress &aExtAddress, Peer::StateFilter aFilter);

    /**
     * Indicates whether the peer table contains any peer matching a given state filter.
     *
     * @param[in]  aFilter  A peer state filter.
     *
     * @returns  TRUE if the table contains at least one peer table matching the given filter, FALSE otherwise.
     */
    bool HasPeers(Peer::StateFilter aFilter) const;

    /**
     * Enables range-based `for` loop iteration over all peer entries in the peer table matching a given
     * state filter.
     *
     * Should be used as follows:
     *
     *     for (Peer &peer : aPeerTable.Iterate(aFilter)) { ... }
     *
     * @param[in] aFilter  A peer state filter.
     *
     * @returns An IteratorBuilder instance.
     */
    IteratorBuilder Iterate(Peer::StateFilter aFilter) { return IteratorBuilder(GetInstance(), aFilter); }

    /**
     * Indicates whether the peer table is full.
     *
     * @returns  TRUE if the table is full, FALSE otherwise.
     */
    bool IsFull(void) const;

private:
    static constexpr uint16_t kMaxPeers = OPENTHREAD_CONFIG_P2P_MAX_PEERS;

    class IteratorBuilder : public InstanceLocator
    {
    public:
        IteratorBuilder(Instance &aInstance, Peer::StateFilter aFilter)
            : InstanceLocator(aInstance)
            , mFilter(aFilter)
        {
        }

        Iterator begin(void) { return Iterator(GetInstance(), mFilter); }
        Iterator end(void) { return Iterator(GetInstance()); }

    private:
        Peer::StateFilter mFilter;
    };

    Peer       *FindPeer(const Peer::AddressMatcher &aMatcher) { return AsNonConst(AsConst(this)->FindPeer(aMatcher)); }
    const Peer *FindPeer(const Peer::AddressMatcher &aMatcher) const;

    Peer mPeers[kMaxPeers];
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_P2P_ENABLE

#endif // PEER_TABLE_HPP_
