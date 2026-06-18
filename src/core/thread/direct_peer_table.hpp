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
 *   This file includes definitions for the Thread Direct peer table.
 */

#ifndef OT_CORE_THREAD_DIRECT_PEER_TABLE_HPP_
#define OT_CORE_THREAD_DIRECT_PEER_TABLE_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#include "common/const_cast.hpp"
#include "common/iterator_utils.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "thread/direct_peer.hpp"

namespace ot {

/**
 * Represents the Thread Direct peer table.
 */
class DirectPeerTable : public InstanceLocator, private NonCopyable
{
    friend class NeighborTable;
    class IteratorBuilder;

public:
    /**
     * Represents an iterator for iterating through entries in the peer table.
     */
    class Iterator : public InstanceLocator, public ItemPtrIterator<DirectPeer, Iterator>
    {
        friend class ItemPtrIterator<DirectPeer, Iterator>;
        friend class IteratorBuilder;

    public:
        /**
         * Initializes an `Iterator` instance.
         *
         * @param[in] aInstance  The OpenThread instance.
         * @param[in] aFilter    A peer state filter.
         */
        Iterator(Instance &aInstance, DirectPeer::StateFilter aFilter);

        /**
         * Resets the iterator to start over.
         */
        void Reset(void);

        /**
         * Gets the `DirectPeer` entry to which the iterator is currently pointing.
         *
         * @returns A pointer to the `DirectPeer` entry, or `nullptr` if done / empty.
         */
        DirectPeer *GetPeer(void) { return mItem; }

    private:
        explicit Iterator(Instance &aInstance)
            : InstanceLocator(aInstance)
            , mFilter(DirectPeer::StateFilter::kInStateValid)
        {
        }

        void Advance(void);

        DirectPeer::StateFilter mFilter;
    };

    /**
     * Initializes a `DirectPeerTable` instance.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     */
    explicit DirectPeerTable(Instance &aInstance);

    /**
     * Clears the peer table.
     */
    void Clear(void);

    /**
     * Gets a new/unused `DirectPeer` entry from the peer table.
     *
     * @returns A pointer to a new `DirectPeer` entry, or `nullptr` if all entries are in use.
     */
    DirectPeer *GetNewPeer(void);

    /**
     * Searches the peer table for a `DirectPeer` with a given extended address and
     * state filter.
     *
     * @param[in]  aExtAddress  A reference to an extended address.
     * @param[in]  aFilter      A peer state filter.
     *
     * @returns  A pointer to the matching `DirectPeer`, or `nullptr` if not found.
     */
    DirectPeer *FindPeer(const Mac::ExtAddress &aExtAddress, DirectPeer::StateFilter aFilter);

    /**
     * Indicates whether the peer table contains any peer matching a given state filter.
     *
     * @param[in]  aFilter  A peer state filter.
     *
     * @returns  TRUE if the table contains at least one matching peer, FALSE otherwise.
     */
    bool HasPeers(DirectPeer::StateFilter aFilter) const;

    /**
     * Enables range-based `for` loop iteration over all peer entries matching a given
     * state filter.
     *
     * Should be used as follows:
     *
     *     for (DirectPeer &peer : aTable.Iterate(aFilter)) { ... }
     *
     * @param[in] aFilter  A peer state filter.
     *
     * @returns An IteratorBuilder instance.
     */
    IteratorBuilder Iterate(DirectPeer::StateFilter aFilter) { return IteratorBuilder(GetInstance(), aFilter); }

    /**
     * Indicates whether the peer table is full.
     *
     * @returns  TRUE if the table is full, FALSE otherwise.
     */
    bool IsFull(void) const;

private:
    static constexpr uint16_t kMaxPeers = OPENTHREAD_CONFIG_THREAD_DIRECT_MAX_DIRECT_PEERS;

    class IteratorBuilder : public InstanceLocator
    {
    public:
        IteratorBuilder(Instance &aInstance, DirectPeer::StateFilter aFilter)
            : InstanceLocator(aInstance)
            , mFilter(aFilter)
        {
        }

        Iterator begin(void) { return Iterator(GetInstance(), mFilter); }
        Iterator end(void) { return Iterator(GetInstance()); }

    private:
        DirectPeer::StateFilter mFilter;
    };

    DirectPeer *FindPeer(const DirectPeer::AddressMatcher &aMatcher)
    {
        return AsNonConst(AsConst(this)->FindPeer(aMatcher));
    }
    const DirectPeer *FindPeer(const DirectPeer::AddressMatcher &aMatcher) const;

    DirectPeer mPeers[kMaxPeers];
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#endif // OT_CORE_THREAD_DIRECT_PEER_TABLE_HPP_
