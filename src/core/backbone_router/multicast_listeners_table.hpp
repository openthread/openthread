/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for Multicast Listeners Table.
 */

#ifndef MULTICAST_LISTENERS_TABLE_HPP
#define MULTICAST_LISTENERS_TABLE_HPP

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

#include <openthread/backbone_router_ftd.h>

#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/time.hpp"
#include "net/ip6_address.hpp"

namespace ot {

namespace BackboneRouter {

/**
 * Implements the definitions for Multicast Listeners Table.
 */
class MulticastListenersTable : public InstanceLocator, private NonCopyable
{
    class IteratorBuilder;

public:
    /**
     * Represents a Multicast Listener entry.
     */
    class Listener : public Clearable<Listener>
    {
        friend class MulticastListenersTable;

    public:
        typedef otBackboneRouterMulticastListenerCallback Callback; ///< Listener callback.
        typedef otBackboneRouterMulticastListenerIterator Iterator; ///< Iterator to go over Listener entries.
        typedef otBackboneRouterMulticastListenerInfo     Info;     ///< Listener info.

        enum Event : uint8_t ///< Listener Event
        {
            kEventAdded   = OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ADDED,   ///< Listener was added.
            kEventRemoved = OT_BACKBONE_ROUTER_MULTICAST_LISTENER_REMOVED, ///< Listener was removed.
        };

        /**
         * Initializes the `Listener` object.
         */
        Listener(void) { Clear(); }

        /**
         * Returns the Multicast Listener address.
         *
         * @returns The Multicast Listener address.
         */
        const Ip6::Address &GetAddress(void) const { return mAddress; }

        /**
         * Returns the expire time of the Multicast Listener.
         *
         * @returns The Multicast Listener expire time.
         */
        const TimeMilli GetExpireTime(void) const { return mExpireTime; }

    private:
        void SetAddress(const Ip6::Address &aAddress) { mAddress = aAddress; }
        void SetExpireTime(TimeMilli aExpireTime) { mExpireTime = aExpireTime; }

        bool operator<(const Listener &aOther) const { return GetExpireTime() < aOther.GetExpireTime(); }

        Ip6::Address mAddress;
        TimeMilli    mExpireTime;
    };

    /**
     * Initializes the Multicast Listeners Table.
     *
     * @param[in] aInstance  A reference to the OpenThread instance.
     */
    explicit MulticastListenersTable(Instance &aInstance)
        : InstanceLocator(aInstance)
        , mNumValidListeners(0)
    {
    }

    /**
     * Adds a Multicast Listener with given address and expire time.
     *
     * @param[in] aAddress     The Multicast Listener address.
     * @param[in] aExpireTime  The Multicast Listener expire time.
     *
     * @retval kErrorNone         If the Multicast Listener was successfully added.
     * @retval kErrorInvalidArgs  If the Multicast Listener address was invalid.
     * @retval kErrorNoBufs       No space available to save the Multicast Listener.
     */
    Error Add(const Ip6::Address &aAddress, TimeMilli aExpireTime);

    /**
     * Removes a given Multicast Listener.
     *
     * @param[in] aAddress  The Multicast Listener address.
     */
    void Remove(const Ip6::Address &aAddress);

    /**
     * Removes expired Multicast Listeners.
     */
    void Expire(void);

    /**
     * Counts the number of valid Multicast Listeners.
     *
     * @returns The number of valid Multicast Listeners.
     */
    uint16_t Count(void) const { return mNumValidListeners; }

    /**
     * Enables range-based `for` loop iteration over all Multicast Listeners.
     *
     * Should be used as follows:
     *
     *     for (MulticastListenersTable::Listener &listener : Get<MulticastListenersTable>().Iterate())
     *
     * @returns An IteratorBuilder instance.
     */
    IteratorBuilder Iterate(void) { return IteratorBuilder(GetInstance()); }

    /**
     * Removes all the Multicast Listeners.
     */
    void Clear(void);

    /**
     * Sets the Multicast Listener callback.
     *
     * @param[in] aCallback  The callback function.
     * @param[in] aContext   A user context pointer.
     */
    void SetCallback(Listener::Callback aCallback, void *aContext);

    /**
     * Gets the next Multicast Listener.
     *
     * @param[in]  aIterator      A pointer to the Multicast Listener Iterator.
     * @param[out] aInfo          A reference to output the Multicast Listener info.
     *
     * @retval kErrorNone         Successfully found the next Multicast Listener info.
     * @retval kErrorNotFound     No subsequent Multicast Listener was found.
     */
    Error GetNext(Listener::Iterator &aIterator, Listener::Info &aInfo);

private:
    static constexpr uint16_t kTableSize = OPENTHREAD_CONFIG_MAX_MULTICAST_LISTENERS;

    static_assert(kTableSize >= 75, "Thread 1.2 Conformance requires table size of at least 75 listeners.");

    class IteratorBuilder : InstanceLocator
    {
    public:
        explicit IteratorBuilder(Instance &aInstance)
            : InstanceLocator(aInstance)
        {
        }

        Listener *begin(void);
        Listener *end(void);
    };

    enum Action : uint8_t
    {
        kAdd,
        kRemove,
        kExpire,
    };

    void Log(Action aAction, const Ip6::Address &aAddress, TimeMilli aExpireTime, Error aError) const;

    void FixHeap(uint16_t aIndex);
    bool SiftHeapElemDown(uint16_t aIndex);
    void SiftHeapElemUp(uint16_t aIndex);
    void CheckInvariants(void) const;

    Listener                     mListeners[kTableSize];
    uint16_t                     mNumValidListeners;
    Callback<Listener::Callback> mCallback;
};

} // namespace BackboneRouter

DefineMapEnum(otBackboneRouterMulticastListenerEvent, BackboneRouter::MulticastListenersTable::Listener::Event);

/**
 * @}
 */

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE

#endif // MULTICAST_LISTENERS_TABLE_HPP
