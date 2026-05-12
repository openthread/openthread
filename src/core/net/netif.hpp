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

#ifndef OT_CORE_NET_NETIF_HPP_
#define OT_CORE_NET_NETIF_HPP_

#include "openthread-core-config.h"

#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/clearable.hpp"
#include "common/code_utils.hpp"
#include "common/const_cast.hpp"
#include "common/iterator_utils.hpp"
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
 */

/**
 * Implements an IPv6 network interface.
 */
class Netif : public InstanceLocator, private NonCopyable
{
    friend class Ip6;
    friend class Address;

public:
    /**
     * Represent an address event (added or removed)
     *
     * The boolean values are used for `aIsAdded` parameter in the call of `otIp6AddressCallback`.
     */
    enum AddressEvent : bool
    {
        kAddressRemoved = false, ///< Indicates that address was added.
        kAddressAdded   = true,  ///< Indicates that address was removed.
    };

    /**
     * Represents the address origin.
     */
    enum AddressOrigin : uint8_t
    {
        kOriginThread = OT_ADDRESS_ORIGIN_THREAD, ///< Thread assigned address (ALOC, RLOC, MLEID, etc)
        kOriginSlaac  = OT_ADDRESS_ORIGIN_SLAAC,  ///< SLAAC assigned address
        kOriginDhcp6  = OT_ADDRESS_ORIGIN_DHCPV6, ///< DHCPv6 assigned address
        kOriginManual = OT_ADDRESS_ORIGIN_MANUAL, ///< Manually assigned address
    };

    /**
     * Implements an IPv6 network interface unicast address.
     */
    class UnicastAddress : public otNetifAddress,
                           public LinkedListEntry<UnicastAddress>,
                           public Clearable<UnicastAddress>
    {
        friend class LinkedList<UnicastAddress>;

    public:
        /**
         * Clears and initializes the unicast address as a preferred, valid, thread-origin address with
         * 64-bit prefix length.
         */
        void InitAsThreadOrigin(void);

        /**
         * Clears and initializes the unicast address as a valid (but not preferred), thread-origin,
         * mesh-local address using the realm-local scope (overridden) address with 64-bit prefix length.
         */
        void InitAsThreadOriginMeshLocal(void);

        /**
         * Clears and initializes the unicast address as a valid (but not preferred), thread-origin, global
         * scope address.
         */
        void InitAsThreadOriginGlobalScope(void);

        /**
         * Clears and initializes the unicast address as a valid, SLAAC-origin address with a given
         * preferred flag and a given prefix length.
         *
         * @param[in] aPrefixLength    The prefix length (in bits).
         * @param[in] aPreferred       The preferred flag.
         */
        void InitAsSlaacOrigin(uint8_t aPrefixLength, bool aPreferred);

        /**
         * Returns the unicast address.
         *
         * @returns The unicast address.
         */
        const Address &GetAddress(void) const { return AsCoreType(&mAddress); }

        /**
         * Returns the unicast address.
         *
         * @returns The unicast address.
         */
        Address &GetAddress(void) { return AsCoreType(&mAddress); }

        /**
         * Sets the unicast address.
         *
         * @param[in] aAddress  The unicast address.
         */
        void SetAddress(const Address &aAddress) { mAddress = aAddress; }

        /**
         * Returns the address's prefix length (in bits).
         *
         * @returns The prefix length (in bits).
         */
        uint8_t GetPrefixLength(void) const { return mPrefixLength; }

        /**
         * Indicates whether the address has a given prefix (i.e. same prefix length and matches the
         * prefix).
         *
         * @param[in] aPrefix   A prefix to check against.
         *
         * @retval TRUE  The address has and fully matches the @p aPrefix.
         * @retval FALSE The address does not contain or match the @p aPrefix.
         */
        bool HasPrefix(const Prefix &aPrefix) const
        {
            return (mPrefixLength == aPrefix.GetLength()) && GetAddress().MatchesPrefix(aPrefix);
        }

        /**
         * Returns the IPv6 scope value.
         *
         * @returns The IPv6 scope value.
         */
        uint8_t GetScope(void) const
        {
            return mScopeOverrideValid ? static_cast<uint8_t>(mScopeOverride) : GetAddress().GetScope();
        }

        /**
         * Sets the IPv6 scope override value.
         *
         * @param[in]  aScope  The IPv6 scope value.
         */
        void SetScopeOverride(uint8_t aScope)
        {
            mScopeOverride      = aScope;
            mScopeOverrideValid = true;
        }

        /**
         * Gets the IPv6 address origin.
         *
         * @returns The address origin.
         */
        AddressOrigin GetOrigin(void) const { return static_cast<AddressOrigin>(mAddressOrigin); }

        /**
         * Returns the next unicast address.
         *
         * @returns A pointer to the next unicast address.
         */
        const UnicastAddress *GetNext(void) const { return static_cast<const UnicastAddress *>(mNext); }

        /**
         * Returns the next unicast address.
         *
         * @returns A pointer to the next unicast address.
         */
        UnicastAddress *GetNext(void) { return static_cast<UnicastAddress *>(AsNonConst(mNext)); }

    private:
        bool Matches(const Address &aAddress) const { return GetAddress() == aAddress; }
    };

    /**
     * Implements an IPv6 network interface multicast address.
     */
    class MulticastAddress : public otNetifMulticastAddress,
                             public LinkedListEntry<MulticastAddress>,
                             public Clearable<MulticastAddress>
    {
        friend class LinkedList<MulticastAddress>;

    public:
        /**
         * Clears and initializes the multicast address as a thread-origin address.
         */
        void InitAsThreadOrigin(void);

        /**
         * Clears and initializes the multicast address as a manual-origin address.
         */
        void InitAsManualOrigin(void);

        /**
         * Returns the multicast address.
         *
         * @returns The multicast address.
         */
        const Address &GetAddress(void) const { return AsCoreType(&mAddress); }

        /**
         * Returns the multicast address.
         *
         * @returns The multicast address.
         */
        Address &GetAddress(void) { return AsCoreType(&mAddress); }

        /**
         * Gets the IPv6 address origin.
         *
         * @returns The address origin.
         */
        AddressOrigin GetOrigin(void) const { return static_cast<AddressOrigin>(mAddressOrigin); }

        /**
         * Returns the next multicast address subscribed to the interface.
         *
         * @returns A pointer to the next multicast address.
         */
        const MulticastAddress *GetNext(void) const { return static_cast<const MulticastAddress *>(mNext); }

        /**
         * Returns the next multicast address subscribed to the interface.
         *
         * @returns A pointer to the next multicast address.
         */
        MulticastAddress *GetNext(void) { return static_cast<MulticastAddress *>(AsNonConst(mNext)); }

#if OPENTHREAD_CONFIG_MLR_ENABLE
        /**
         * Indicates whether or not the address is a Multicast Listener Registration (MLR) candidate.
         *
         * An address is an MLR candidate if it is an external address (origin `kOriginManual`) and its scope
         * is larger than realm-local.
         *
         * @retval TRUE  If the address is an MLR candidate.
         * @retval FALSE If the address is not an MLR candidate.
         */
        bool IsMlrCandidate(void) const;

        /**
         * Indicates whether the multicast address is registered via Multicast Listener Registration (MLR).
         *
         * @retval TRUE   If the multicast address is MLR registered.
         * @retval FALSE  If the multicast address is not MLR registered.
         */
        bool IsMlrRegistered(void) const { return (mData != 0); }

        /**
         * Sets whether the multicast address is registered via Multicast Listener Registration (MLR).
         *
         * @param[in] aRegistered  TRUE if MLR registered, FALSE otherwise.
         */
        void SetMlrRegistered(bool aRegistered) { mData = aRegistered ? 1 : 0; }
#endif

    private:
        bool Matches(const Address &aAddress) const { return GetAddress() == aAddress; }
        bool Matches(AddressOrigin aOrigin) const { return GetOrigin() == aOrigin; }
    };

    /**
     * Initializes the network interface.
     *
     * @param[in]  aInstance        A reference to the OpenThread instance.
     */
    explicit Netif(Instance &aInstance);

#if OPENTHREAD_CONFIG_IP6_INIT_EXT_ADDR_POOL_ENABLE

    /**
     * Initializes the network interface with external address pools.
     *
     * It provides the memory buffers to be used for the external unicast and multicast address pools. The provided
     * pool memory buffers MUST persist and remain valid as long as the OpenThread instance is initialized.
     *
     * @param[in] aUnicastAddrPool        A pointer to an array of `UnicastAddress` entries.
     * @param[in] aUnicastAddrPoolSize    The number of entries in @p aUnicastAddrPool.
     * @param[in] aMulticastAddrPool      A pointer to an array of `MulticastAddress` entries.
     * @param[in] aMulticastAddrPoolSize  The number of entries in @p aMulticastAddrPool.
     *
     * @retval kErrorNone     Successfully initialized the network interface.
     * @retval kErrorAlready  The network interface is already initialized.
     */
    Error Init(UnicastAddress   *aUnicastAddrPool,
               uint16_t          aUnicastAddrPoolSize,
               MulticastAddress *aMulticastAddrPool,
               uint16_t          aMulticastAddrPoolSize);

    /**
     * Indicates whether or not the network interface is initialized.
     *
     * @retval TRUE   If the network interface is initialized.
     * @retval FALSE  If the network interface is not initialized.
     */
    bool IsInitialized(void) const { return mInitialized; }

#endif // OPENTHREAD_CONFIG_IP6_INIT_EXT_ADDR_POOL_ENABLE

    /**
     * Registers a callback to notify internal IPv6 address changes.
     *
     * @param[in]  aCallback         A pointer to a function that is called when an IPv6 address is added or removed.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     */
    void SetAddressCallback(otIp6AddressCallback aCallback, void *aCallbackContext)
    {
        mAddressCallback.Set(aCallback, aCallbackContext);
    }

    /**
     * Returns the linked list of unicast addresses.
     *
     * @returns The linked list of unicast addresses.
     */
    const LinkedList<UnicastAddress> &GetUnicastAddresses(void) const { return mUnicastAddresses; }

    /**
     * Returns the linked list of unicast addresses.
     *
     * @returns The linked list of unicast addresses.
     */
    LinkedList<UnicastAddress> &GetUnicastAddresses(void) { return mUnicastAddresses; }

    /**
     * Adds a unicast address to the network interface.
     *
     * Is intended for addresses internal to OpenThread. The @p aAddress instance is directly added in the
     * unicast address linked list.
     *
     * If @p aAddress is already added, the call to `AddUnicastAddress()` with the same address will perform no action.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     */
    void AddUnicastAddress(UnicastAddress &aAddress);

    /**
     * Removes a unicast address from the network interface.
     *
     * Is intended for addresses internal to OpenThread. The @p aAddress instance is removed from the
     * unicast address linked list.
     *
     * If @p aAddress is not in the list, the call to `RemoveUnicastAddress()` will perform no action.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     */
    void RemoveUnicastAddress(UnicastAddress &aAddress);

    /**
     * Updates the preferred flag on a previously added (internal to OpenThread core) unicast address.
     *
     * If the address is not added to the network interface or the current preferred flag of @p aAddress is the same as
     * the given @p aPreferred, no action is performed.
     *
     * @param[in] aAddress        The unicast address
     * @param[in] aPreferred The new value for preferred flag.
     */
    void UpdatePreferredFlagOn(UnicastAddress &aAddress, bool aPreferred);

    /**
     * Indicates whether or not an address is assigned to the interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval TRUE   If @p aAddress is assigned to the network interface.
     * @retval FALSE  If @p aAddress is not assigned to the network interface.
     */
    bool HasUnicastAddress(const Address &aAddress) const;

    /**
     * Indicates whether or not a unicast address is assigned to the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval TRUE   If @p aAddress is assigned to the network interface.
     * @retval FALSE  If @p aAddress is not assigned to the network interface.
     */
    bool HasUnicastAddress(const UnicastAddress &aAddress) const { return mUnicastAddresses.Contains(aAddress); }

    /**
     * Indicates whether a unicast address is an external or internal address.
     *
     * @param[in] aAddress  A reference to the unicast address.
     *
     * @retval TRUE   The address is an external address.
     * @retval FALSE  The address is not an external address (it is an OpenThread internal address).
     */
    bool IsUnicastAddressExternal(const UnicastAddress &aAddress) const;

    /**
     * Adds an external (to OpenThread) unicast address to the network interface.
     *
     * For external address, the @p aAddress instance is not directly used (i.e., it can be temporary). It is copied
     * into a local entry (allocated from an internal pool) before being added in the unicast address linked list.
     * The maximum number of external addresses is specified by `OPENTHREAD_CONFIG_IP6_MAX_EXT_UCAST_ADDRS`.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval kErrorNone         Successfully added (or updated) the unicast address.
     * @retval kErrorInvalidArgs  The address indicated by @p aAddress is an internal address.
     * @retval kErrorNoBufs       The maximum number of allowed external addresses are already added.
     */
    Error AddExternalUnicastAddress(const UnicastAddress &aAddress);

    /**
     * Removes a external (to OpenThread) unicast address from the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval kErrorNone         Successfully removed the unicast address.
     * @retval kErrorInvalidArgs  The address indicated by @p aAddress is an internal address.
     * @retval kErrorNotFound     The unicast address was not found.
     */
    Error RemoveExternalUnicastAddress(const Address &aAddress);

    /**
     * Removes all the previously added external (to OpenThread) unicast addresses from the
     * network interface.
     */
    void RemoveAllExternalUnicastAddresses(void);

    /**
     * Indicates whether or not the network interface is subscribed to a multicast address.
     *
     * @param[in]  aAddress  The multicast address to check.
     *
     * @retval TRUE   If the network interface is subscribed to @p aAddress.
     * @retval FALSE  If the network interface is not subscribed to @p aAddress.
     */
    bool IsMulticastSubscribed(const Address &aAddress) const;

    /**
     * Subscribes the network interface to the link-local and realm-local all routers addresses.
     *
     * @note This method MUST be called after `SubscribeAllNodesMulticast()` or its behavior is undefined.
     */
    void SubscribeAllRoutersMulticast(void);

    /**
     * Unsubscribes the network interface to the link-local and realm-local all routers address.
     */
    void UnsubscribeAllRoutersMulticast(void);

    /**
     * Returns the linked list of multicast addresses.
     *
     * @returns The linked list of multicast addresses.
     */
    const LinkedList<MulticastAddress> &GetMulticastAddresses(void) const { return mMulticastAddresses; }

    /**
     * Returns the linked list of multicast addresses.
     *
     * @returns The linked list of multicast addresses.
     */
    LinkedList<MulticastAddress> &GetMulticastAddresses(void) { return mMulticastAddresses; }

    /**
     * Subscribes the network interface to a multicast address.
     *
     * Is intended for addresses internal to OpenThread. The @p aAddress instance is directly added in the
     * multicast address linked list.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     */
    void SubscribeMulticast(MulticastAddress &aAddress);

    /**
     * Unsubscribes the network interface to a multicast address.
     *
     * Is intended for addresses internal to OpenThread. The @p aAddress instance is directly removed from
     * the multicast address linked list.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     */
    void UnsubscribeMulticast(const MulticastAddress &aAddress);

    /**
     * Subscribes the network interface to the external (to OpenThread) multicast address.
     *
     * For external address, the @p aAddress instance is not directly used (i.e., it can be temporary). It is copied
     * into a local entry (allocated from an internal pool) before being added in the multicast address linked list.
     * The maximum number of external addresses is specified by `OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS`.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval kErrorNone          Successfully subscribed to @p aAddress.
     * @retval kErrorAlready       The multicast address is already subscribed.
     * @retval kErrorInvalidArgs   The IP Address indicated by @p aAddress is an invalid multicast address.
     * @retval kErrorRejected      The IP Address indicated by @p aAddress is an internal multicast address.
     * @retval kErrorNoBufs        The maximum number of allowed external multicast addresses are already added.
     */
    Error SubscribeExternalMulticast(const Address &aAddress);

    /**
     * Unsubscribes the network interface to the external (to OpenThread) multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval kErrorNone         Successfully unsubscribed to the unicast address.
     * @retval kErrorRejected     The address indicated by @p aAddress is an internal address.
     * @retval kErrorNotFound     The multicast address was not found.
     */
    Error UnsubscribeExternalMulticast(const Address &aAddress);

    /**
     * Unsubscribes the network interface from all previously added external (to OpenThread) multicast
     * addresses.
     */
    void UnsubscribeAllExternalMulticastAddresses(void);

    /**
     * Indicates whether or not the network interface is subscribed to any external multicast address.
     *
     * @retval TRUE  The network interface is subscribed to at least one external multicast address.
     * @retval FALSE The network interface is not subscribed to any external multicast address.
     */
    bool HasAnyExternalMulticastAddress(void) const;

    /**
     * Applies the new mesh local prefix.
     *
     * Updates all mesh-local unicast addresses and prefix-based multicast addresses of the network interface.
     */
    void ApplyNewMeshLocalPrefix(void);

protected:
    /**
     * Subscribes the network interface to the realm-local all MPL forwarders, link-local, and realm-local
     * all nodes address.
     */
    void SubscribeAllNodesMulticast(void);

    /**
     * Unsubscribes the network interface from the realm-local all MPL forwarders, link-local and
     * realm-local all nodes address.
     *
     * @note This method MUST be called after `UnsubscribeAllRoutersMulticast()` or its behavior is undefined
     */
    void UnsubscribeAllNodesMulticast(void);

private:
    static constexpr uint16_t kMaxExtUnicastAddrs   = OPENTHREAD_CONFIG_IP6_MAX_EXT_UCAST_ADDRS;
    static constexpr uint16_t kMaxExtMulticastAddrs = OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS;

    static constexpr uint8_t kMulticastPrefixLength = 128; // Multicast prefix length used in `AdressInfo`.

    typedef otIp6AddressInfo AddressInfo;

    void SignalUnicastAddressChange(AddressEvent aEvent, const UnicastAddress &aAddress);
    void SignalMulticastAddressChange(AddressEvent aEvent, const MulticastAddress &aAddress);
    void SignalMulticastAddressesChange(AddressEvent            aEvent,
                                        const MulticastAddress *aStart,
                                        const MulticastAddress *aEnd);

    LinkedList<UnicastAddress>     mUnicastAddresses;
    LinkedList<MulticastAddress>   mMulticastAddresses;
    Callback<otIp6AddressCallback> mAddressCallback;

#if OPENTHREAD_CONFIG_IP6_INIT_EXT_ADDR_POOL_ENABLE
    ConfigPool<UnicastAddress>   mExtUnicastAddressPool;
    ConfigPool<MulticastAddress> mExtMulticastAddressPool;
    bool                         mInitialized;
#else
    Pool<UnicastAddress, kMaxExtUnicastAddrs>     mExtUnicastAddressPool;
    Pool<MulticastAddress, kMaxExtMulticastAddrs> mExtMulticastAddressPool;
#endif

    static const otNetifMulticastAddress kRealmLocalAllMplForwardersMulticastAddress;
    static const otNetifMulticastAddress kLinkLocalAllNodesMulticastAddress;
    static const otNetifMulticastAddress kRealmLocalAllNodesMulticastAddress;
    static const otNetifMulticastAddress kLinkLocalAllRoutersMulticastAddress;
    static const otNetifMulticastAddress kRealmLocalAllRoutersMulticastAddress;
};

/**
 * @}
 */

} // namespace Ip6

DefineCoreType(otNetifAddress, Ip6::Netif::UnicastAddress);
DefineCoreType(otNetifMulticastAddress, Ip6::Netif::MulticastAddress);

} // namespace ot

#endif // OT_CORE_NET_NETIF_HPP_
