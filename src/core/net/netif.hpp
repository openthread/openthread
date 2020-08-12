/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for IPv6 network interfaces.
 */

#ifndef NET_NETIF_HPP_
#define NET_NETIF_HPP_

#include "openthread-core-config.h"

#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/tasklet.hpp"
#include "mac/mac_types.hpp"
#include "net/ip6_address.hpp"
#include "net/socket.hpp"
#include "thread/mlr_types.hpp"

namespace ot {
namespace Ip6 {

class Ip6;

/**
 * @addtogroup core-ip6-netif
 *
 * @brief
 *   This module includes definitions for IPv6 network interfaces.
 *
 * @{
 *
 */

/**
 * This class implements an IPv6 network interface unicast address.
 *
 */
class NetifUnicastAddress : public otNetifAddress,
                            public LinkedListEntry<NetifUnicastAddress>,
                            public Clearable<NetifUnicastAddress>
{
    friend class Netif;
    friend class LinkedList<NetifUnicastAddress>;

public:
    /**
     * This method clears and initializes the unicast address as a preferred, valid, thread-origin address with 64-bit
     * prefix length.
     *
     */
    void InitAsThreadOrigin(void);

    /**
     * This method clears and initializes the unicast address as a preferred, valid, thread-origin, realm-local scope
     * (overridden) address with 64-bit prefix length.
     *
     */
    void InitAsThreadOriginRealmLocalScope(void);

    /**
     * This method clears and initializes the unicast address as a valid (but not preferred), thread-origin, global
     * scope address.
     *
     */
    void InitAsThreadOriginGlobalScope(void);

    /**
     * This method clears and initializes the unicast address as a valid, SLAAC-origin address with a given preferred
     * flag and a given prefix length.
     *
     * @param[in] aPrefixLength    The prefix length (in bits).
     * @param[in] aPreferred       The preferred flag.
     *
     */
    void InitAsSlaacOrigin(uint8_t aPrefixLength, bool aPreferred);

    /**
     * This method returns the unicast address.
     *
     * @returns The unicast address.
     *
     */
    const Address &GetAddress(void) const { return static_cast<const Address &>(mAddress); }

    /**
     * This method returns the unicast address.
     *
     * @returns The unicast address.
     *
     */
    Address &GetAddress(void) { return static_cast<Address &>(mAddress); }

    /**
     * This method returns the address's prefix length (in bits).
     *
     * @returns The prefix length (in bits).
     *
     */
    uint8_t GetPrefixLength(void) const { return mPrefixLength; }

    /**
     * This method indicates whether the address has a given prefix (i.e. same prefix length and matches the prefix).
     *
     * @param[in] aPrefix   A prefix to check against.
     *
     * @retval TRUE  The address has and fully matches the @p aPrefix.
     * @retval FALSE The address does not contain or match the @p aPrefix.
     *
     */
    bool HasPrefix(const Prefix &aPrefix) const
    {
        return (mPrefixLength == aPrefix.GetLength()) && GetAddress().MatchesPrefix(aPrefix);
    }

    /**
     * This method returns the IPv6 scope value.
     *
     * @returns The IPv6 scope value.
     *
     */
    uint8_t GetScope(void) const
    {
        return mScopeOverrideValid ? static_cast<uint8_t>(mScopeOverride) : GetAddress().GetScope();
    }

    /**
     * This method sets the IPv6 scope override value.
     *
     * @param[in]  aScope  The IPv6 scope value.
     *
     */
    void SetScopeOverride(uint8_t aScope)
    {
        mScopeOverride      = aScope;
        mScopeOverrideValid = true;
    }

private:
    bool Matches(const Address &aAddress) const { return GetAddress() == aAddress; }
};

/**
 * This class implements an IPv6 network interface multicast address.
 *
 */
class NetifMulticastAddress : public otNetifMulticastAddress,
                              public LinkedListEntry<NetifMulticastAddress>,
                              public Clearable<NetifMulticastAddress>
{
    friend class Netif;
    friend class LinkedList<NetifMulticastAddress>;

public:
    /**
     * This method returns the multicast address.
     *
     * @returns The multicast address.
     *
     */
    const Address &GetAddress(void) const { return static_cast<const Address &>(mAddress); }

    /**
     * This method returns the multicast address.
     *
     * @returns The multicast address.
     *
     */
    Address &GetAddress(void) { return static_cast<Address &>(mAddress); }

    /**
     * This method returns the next multicast address subscribed to the interface.
     *
     * @returns A pointer to the next multicast address.
     *
     */
    const NetifMulticastAddress *GetNext(void) const { return static_cast<const NetifMulticastAddress *>(mNext); }

    /**
     * This method returns the next multicast address subscribed to the interface.
     *
     * @returns A pointer to the next multicast address.
     *
     */
    NetifMulticastAddress *GetNext(void)
    {
        return static_cast<NetifMulticastAddress *>(const_cast<otNetifMulticastAddress *>(mNext));
    }

private:
    bool Matches(const Address &aAddress) const { return GetAddress() == aAddress; }
};

class ExternalNetifMulticastAddress : public NetifMulticastAddress
{
    friend class Netif;
    friend class LinkedList<ExternalNetifMulticastAddress>;

public:
#if OPENTHREAD_CONFIG_MLR_ENABLE
    /**
     * This method returns the current Multicast Listener Registration (MLR) state.
     *
     * @returns The current Multicast Listener Registration state.
     *
     */
    MlrState GetMlrState(void) const { return mMlrState; }

    /**
     * This method sets the Multicast Listener Registration (MLR) state.
     *
     * @param[in] aState  The new Multicast Listener Registration state.
     *
     */
    void SetMlrState(MlrState aState) { mMlrState = aState; }
#endif

private:
    ExternalNetifMulticastAddress *GetNext(void)
    {
        return static_cast<ExternalNetifMulticastAddress *>(const_cast<otNetifMulticastAddress *>(mNext));
    }

#if OPENTHREAD_CONFIG_MLR_ENABLE
    MlrState mMlrState : 2;
#endif
};

/**
 * This class implements an IPv6 network interface.
 *
 */
class Netif : public InstanceLocator, public LinkedListEntry<Netif>, private NonCopyable
{
    friend class Ip6;
    friend class Address;
    class ExternalMulticastAddressIteratorBuilder;

public:
    /**
     * This class represents an iterator for iterating external multicast addresses in a Netif instance.
     *
     */
    class ExternalMulticastAddressIterator
    {
        friend class ExternalMulticastAddressIteratorBuilder;

    public:
        /**
         * This constructor initializes an `ExternalMulticastAddressIterator` instance to start from the first external
         * multicast address that matches a given Ip6 address type filter.
         *
         * @param[in] aNetif   A reference to the Netif instance.
         * @param[in] aFilter  The Ip6 address type filter.
         *
         */
        explicit ExternalMulticastAddressIterator(const Netif &aNetif, Address::TypeFilter aFilter = Address::kTypeAny)
            : mNetif(aNetif)
            , mFilter(aFilter)
        {
            AdvanceFrom(mNetif.GetMulticastAddresses());
        }

        /**
         * This method indicates whether the iterator has reached end of the list.
         *
         * @retval TRUE   There are no more entries in the list (reached end of the list).
         * @retval FALSE  The current address entry is valid.
         *
         */
        bool IsDone(void) const { return mCurrent != nullptr; }

        /**
         * This method overloads `++` operator (pre-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next entry.  If there are no more entries matching the iterator becomes
         * empty.
         *
         */
        void operator++(void) { AdvanceFrom(mCurrent->GetNext()); }

        /**
         * This method overloads `++` operator (post-increment) to advance the iterator.
         *
         * The iterator is moved to point to the next entry.  If there are no more entries matching the iterator becomes
         * empty.
         *
         */
        void operator++(int) { AdvanceFrom(mCurrent->GetNext()); }

        /**
         * This method overloads the `*` dereference operator and gets a reference to `ExternalNetifMulticastAddress`
         * entry to which the iterator is currently pointing.
         *
         * This method MUST be used when the iterator is not empty.
         *
         * @returns A reference to the `ExternalNetifMulticastAddress` entry currently pointed by the iterator.
         *
         */
        ExternalNetifMulticastAddress &operator*(void) { return *mCurrent; }

        /**
         * This method overloads the `->` dereference operator and gets a pointer to `ExternalNetifMulticastAddress`
         * entry to which the iterator is current pointing.
         *
         * @returns A pointer to the `ExternalNetifMulticastAddress` entry associated with the iterator, or `nullptr` if
         * iterator is empty.
         *
         */
        ExternalNetifMulticastAddress *operator->(void) { return mCurrent; }

        /**
         * This method overloads operator `==` to evaluate whether or not two `ExternalMulticastAddressIterator`
         * instances point to the same `ExternalNetifMulticastAddress` entry.
         *
         * @param[in] aOther  The other `Iterator` to compare with.
         *
         * @retval TRUE   If the two `ExternalMulticastAddressIterator` objects point to the same
         * `ExternalNetifMulticastAddress` entry or both are done.
         * @retval FALSE  If the two `ExternalMulticastAddressIterator` objects do not point to the same
         * `ExternalNetifMulticastAddress` entry.
         *
         */
        bool operator==(const ExternalMulticastAddressIterator &aOther) { return mCurrent == aOther.mCurrent; }

        /**
         * This method overloads operator `!=` to evaluate whether or not two `ExternalMulticastAddressIterator`
         * instances point to the same `ExternalNetifMulticastAddress` entry.
         *
         * @param[in]  aOther  The other `ExternalMulticastAddressIterator` to compare with.
         *
         * @retval TRUE   If the two `ExternalMulticastAddressIterator` objects do not point to the same
         * `ExternalNetifMulticastAddress` entry.
         * @retval FALSE  If the two `ExternalMulticastAddressIterator` objects point to the same
         * `ExternalNetifMulticastAddress` entry or both are done.
         *
         */
        bool operator!=(const ExternalMulticastAddressIterator &aOther) { return mCurrent != aOther.mCurrent; }

    private:
        enum IteratorType
        {
            kEndIterator,
        };

        ExternalMulticastAddressIterator(const Netif &aNetif, IteratorType)
            : mNetif(aNetif)
            , mCurrent(nullptr)
        {
        }

        void AdvanceFrom(const NetifMulticastAddress *aAddr)
        {
            while (aAddr != nullptr &&
                   !(mNetif.IsMulticastAddressExternal(*aAddr) && aAddr->GetAddress().MatchesFilter(mFilter)))
            {
                aAddr = aAddr->GetNext();
            }

            mCurrent =
                const_cast<ExternalNetifMulticastAddress *>(static_cast<const ExternalNetifMulticastAddress *>(aAddr));
        }

        const Netif &                  mNetif;
        ExternalNetifMulticastAddress *mCurrent;
        Address::TypeFilter            mFilter;
    };

    /**
     * This constructor initializes the network interface.
     *
     * @param[in]  aInstance        A reference to the OpenThread instance.
     *
     */
    Netif(Instance &aInstance);

    /**
     * This method registers a callback to notify internal IPv6 address changes.
     *
     * @param[in]  aCallback         A pointer to a function that is called when an IPv6 address is added or removed.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetAddressCallback(otIp6AddressCallback aCallback, void *aCallbackContext);

    /**
     * This method returns a pointer to the head of the linked list of unicast addresses.
     *
     * @returns A pointer to the head of the linked list of unicast addresses.
     *
     */
    const NetifUnicastAddress *GetUnicastAddresses(void) const { return mUnicastAddresses.GetHead(); }

    /**
     * This method adds a unicast address to the network interface.
     *
     * This method is intended for addresses internal to OpenThread. The @p aAddress instance is directly added in the
     * unicast address linked list.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     */
    void AddUnicastAddress(NetifUnicastAddress &aAddress);

    /**
     * This method removes a unicast address from the network interface.
     *
     * This method is intended for addresses internal to OpenThread. The @p aAddress instance is removed from the
     * unicast address linked list.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     */
    void RemoveUnicastAddress(const NetifUnicastAddress &aAddress);

    /**
     * This method indicates whether or not an address is assigned to the interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval TRUE   If @p aAddress is assigned to the network interface,
     * @retval FALSE  If @p aAddress is not assigned to the network interface.
     *
     */
    bool HasUnicastAddress(const Address &aAddress) const;

    /**
     * This method indicates whether or not a unicast address is assigned to the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval TRUE   If @p aAddress is assigned to the network interface,
     * @retval FALSE  If @p aAddress is not assigned to the network interface.
     *
     */
    bool HasUnicastAddress(const NetifUnicastAddress &aAddress) const { return mUnicastAddresses.Contains(aAddress); }

    /**
     * This method indicates whether a unicast address is an external or internal address.
     *
     * @param[in] aAddress  A reference to the unicast address.
     *
     * @retval TRUE   The address is an external address.
     * @retval FALSE  The address is not an external address (it is an OpenThread internal address).
     *
     */
    bool IsUnicastAddressExternal(const NetifUnicastAddress &aAddress) const;

    /**
     * This method adds an external (to OpenThread) unicast address to the network interface.
     *
     * For external address, the @p aAddress instance is not directly used (i.e., it can be temporary). It is copied
     * into a local entry (allocated from an internal pool) before being added in the unicast address linked list.
     * The maximum number of external addresses is specified by `OPENTHREAD_CONFIG_IP6_MAX_EXT_UCAST_ADDRS`.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval OT_ERROR_NONE          Successfully added (or updated) the unicast address.
     * @retval OT_ERROR_INVALID_ARGS  The address indicated by @p aAddress is an internal address.
     * @retval OT_ERROR_NO_BUFS       The maximum number of allowed external addresses are already added.
     *
     */
    otError AddExternalUnicastAddress(const NetifUnicastAddress &aAddress);

    /**
     * This method removes a external (to OpenThread) unicast address from the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval OT_ERROR_NONE          Successfully removed the unicast address.
     * @retval OT_ERROR_INVALID_ARGS  The address indicated by @p aAddress is an internal address.
     * @retval OT_ERROR_NOT_FOUND     The unicast address was not found.
     *
     */
    otError RemoveExternalUnicastAddress(const Address &aAddress);

    /**
     * This method removes all the previously added external (to OpenThread) unicast addresses from the
     * network interface.
     *
     */
    void RemoveAllExternalUnicastAddresses(void);

    /**
     * This method indicates whether or not the network interface is subscribed to a multicast address.
     *
     * @param[in]  aAddress  The multicast address to check.
     *
     * @retval TRUE   If the network interface is subscribed to @p aAddress.
     * @retval FALSE  If the network interface is not subscribed to @p aAddress.
     *
     */
    bool IsMulticastSubscribed(const Address &aAddress) const;

    /**
     * This method subscribes the network interface to the link-local and realm-local all routers addresses.
     *
     * @note This method MUST be called after `SubscribeAllNodesMulticast()` or its behavior is undefined.
     *
     */
    void SubscribeAllRoutersMulticast(void);

    /**
     * This method unsubscribes the network interface to the link-local and realm-local all routers address.
     *
     */
    void UnsubscribeAllRoutersMulticast(void);

    /**
     * This method returns a pointer to the head of the linked list of multicast addresses.
     *
     * @returns A pointer to the head of the linked list of multicast addresses.
     *
     */
    const NetifMulticastAddress *GetMulticastAddresses(void) const { return mMulticastAddresses.GetHead(); }

    /**
     * This method indicates whether a multicast address is an external or internal address.
     *
     * @param[in] aAddress  A reference to the multicast address.
     *
     * @retval TRUE   The address is an external address.
     * @retval FALSE  The address is not an external address (it is an OpenThread internal address).
     *
     */
    bool IsMulticastAddressExternal(const NetifMulticastAddress &aAddress) const;

    /**
     * This method subscribes the network interface to a multicast address.
     *
     * This method is intended for addresses internal to OpenThread. The @p aAddress instance is directly added in the
     * multicast address linked list.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     */
    void SubscribeMulticast(NetifMulticastAddress &aAddress);

    /**
     * This method unsubscribes the network interface to a multicast address.
     *
     * This method is intended for addresses internal to OpenThread. The @p aAddress instance is directly removed from
     * the multicast address linked list.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     */
    void UnsubscribeMulticast(const NetifMulticastAddress &aAddress);

    /**
     * This method subscribes the network interface to the external (to OpenThread) multicast address.
     *
     * For external address, the @p aAddress instance is not directly used (i.e., it can be temporary). It is copied
     * into a local entry (allocated from an internal pool) before being added in the multicast address linked list.
     * The maximum number of external addresses is specified by `OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS`.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval OT_ERROR_NONE           Successfully subscribed to @p aAddress.
     * @retval OT_ERROR_ALREADY        The multicast address is already subscribed.
     * @retval OT_ERROR_INVALID_ARGS   The address indicated by @p aAddress is an internal multicast address.
     * @retval OT_ERROR_NO_BUFS        The maximum number of allowed external multicast addresses are already added.
     *
     */
    otError SubscribeExternalMulticast(const Address &aAddress);

    /**
     * This method unsubscribes the network interface to the external (to OpenThread) multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval OT_ERROR_NONE          Successfully unsubscribed to the unicast address.
     * @retval OT_ERROR_INVALID_ARGS  The address indicated by @p aAddress is an internal address.
     * @retval OT_ERROR_NOT_FOUND     The multicast address was not found.
     *
     */
    otError UnsubscribeExternalMulticast(const Address &aAddress);

    /**
     * This method unsubscribes the network interface from all previously added external (to OpenThread) multicast
     * addresses.
     *
     */
    void UnsubscribeAllExternalMulticastAddresses(void);

    /**
     * This method checks if multicast promiscuous mode is enabled on the network interface.
     *
     * @retval TRUE   If the multicast promiscuous mode is enabled.
     * @retval FALSE  If the multicast promiscuous mode is disabled.
     *
     */
    bool IsMulticastPromiscuousEnabled(void) const { return mMulticastPromiscuous; }

    /**
     * This method enables multicast promiscuous mode on the network interface.
     *
     * @param[in]  aEnabled  TRUE if Multicast Promiscuous mode is enabled, FALSE otherwise.
     *
     */
    void SetMulticastPromiscuous(bool aEnabled) { mMulticastPromiscuous = aEnabled; }

    /**
     * This method enables range-based `for` loop iteration over external multicast addresses on the Netif that matches
     * a given Ip6 address type filter.
     *
     * This method should be used like follows: to iterate over all external multicast addresses
     *
     *     for (Ip6::ExternalNetifMulticastAddress &addr : Get<ThreadNetif>().IterateExternalMulticastAddresses())
     *     { ... }
     *
     * or to iterate over a subset of external multicast addresses determined by a given address type filter
     *
     *     for (Ip6::ExternalNetifMulticastAddress &addr :
     * Get<ThreadNetif>().IterateExternalMulticastAddresses(Ip6::Address::kTypeMulticastLargerThanRealmLocal)) { ... }
     *
     * @param[in] aFilter  The Ip6 address type filter.
     *
     * @returns An `ExternalMulticastAddressIteratorBuilder` instance.
     *
     */
    ExternalMulticastAddressIteratorBuilder IterateExternalMulticastAddresses(
        Address::TypeFilter aFilter = Address::kTypeAny)
    {
        return ExternalMulticastAddressIteratorBuilder(*this, aFilter);
    }

    /**
     * This method indicates whether or not the network interfaces is subscribed to any external multicast address.
     *
     * @retval TRUE  The network interface is subscribed to at least one external multicast address.
     * @retval FALSE The network interface is not subscribed to any external multicast address.
     *
     */
    bool HasAnyExternalMulticastAddress(void) const { return !ExternalMulticastAddressIterator(*this).IsDone(); }

protected:
    /**
     * This method subscribes the network interface to the realm-local all MPL forwarders, link-local, and realm-local
     * all nodes address.
     *
     */
    void SubscribeAllNodesMulticast(void);

    /**
     * This method unsubscribes the network interface from the realm-local all MPL forwarders, link-local and
     * realm-local all nodes address.
     *
     * @note This method MUST be called after `UnsubscribeAllRoutersMulticast()` or its behavior is undefined
     *
     */
    void UnsubscribeAllNodesMulticast(void);

private:
    enum
    {
        kMulticastPrefixLength = 128, ///< Multicast prefix length used to notify internal address changes.
    };

    class ExternalMulticastAddressIteratorBuilder
    {
    public:
        ExternalMulticastAddressIteratorBuilder(const Netif &aNetif, Address::TypeFilter aFilter)
            : mNetif(aNetif)
            , mFilter(aFilter)
        {
        }

        ExternalMulticastAddressIterator begin(void) { return ExternalMulticastAddressIterator(mNetif, mFilter); }
        ExternalMulticastAddressIterator end(void)
        {
            return ExternalMulticastAddressIterator(mNetif, ExternalMulticastAddressIterator::kEndIterator);
        }

    private:
        const Netif &       mNetif;
        Address::TypeFilter mFilter;
    };

    LinkedList<NetifUnicastAddress>   mUnicastAddresses;
    LinkedList<NetifMulticastAddress> mMulticastAddresses;
    bool                              mMulticastPromiscuous;

    otIp6AddressCallback mAddressCallback;
    void *               mAddressCallbackContext;

    Pool<NetifUnicastAddress, OPENTHREAD_CONFIG_IP6_MAX_EXT_UCAST_ADDRS>           mExtUnicastAddressPool;
    Pool<ExternalNetifMulticastAddress, OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS> mExtMulticastAddressPool;

    static const otNetifMulticastAddress kRealmLocalAllMplForwardersMulticastAddress;
    static const otNetifMulticastAddress kLinkLocalAllNodesMulticastAddress;
    static const otNetifMulticastAddress kRealmLocalAllNodesMulticastAddress;
    static const otNetifMulticastAddress kLinkLocalAllRoutersMulticastAddress;
    static const otNetifMulticastAddress kRealmLocalAllRoutersMulticastAddress;
};

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // NET_NETIF_HPP_
