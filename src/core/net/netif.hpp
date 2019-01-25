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

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/tasklet.hpp"
#include "mac/mac_frame.hpp"
#include "net/ip6_address.hpp"
#include "net/socket.hpp"

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
 * This class represents an IPv6 Link Address.
 *
 */
class LinkAddress
{
public:
    /**
     * Hardware types.
     *
     */
    enum HardwareType
    {
        kEui64 = 27,
    };
    HardwareType    mType;       ///< Link address type.
    uint8_t         mLength;     ///< Length of link address.
    Mac::ExtAddress mExtAddress; ///< Link address.
};

/**
 * This class implements an IPv6 network interface unicast address.
 *
 */
class NetifUnicastAddress : public otNetifAddress
{
    friend class Netif;

public:
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
     * This method returns the next unicast address assigned to the interface.
     *
     * @returns A pointer to the next unicast address.
     *
     */
    const NetifUnicastAddress *GetNext(void) const { return static_cast<const NetifUnicastAddress *>(mNext); }

    /**
     * This method returns the next unicast address assigned to the interface.
     *
     * @returns A pointer to the next unicast address.
     *
     */
    NetifUnicastAddress *GetNext(void) { return static_cast<NetifUnicastAddress *>(mNext); }
};

/**
 * This class implements an IPv6 network interface multicast address.
 *
 */
class NetifMulticastAddress : public otNetifMulticastAddress
{
    friend class Netif;

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
};

/**
 * This class implements an IPv6 network interface.
 *
 */
class Netif : public InstanceLocator
{
    friend class Ip6;

public:
    /**
     * This constructor initializes the network interface.
     *
     * @param[in]  aInstance        A reference to the OpenThread instance.
     * @param[in]  aInterfaceId     The interface ID for this object.
     *
     */
    Netif(Instance &aInstance, int8_t aInterfaceId);

    /**
     * This method returns the next network interface in the list.
     *
     * @returns A pointer to the next network interface.
     */
    Netif *GetNext(void) const { return mNext; }

    /**
     * This method returns the network interface identifier.
     *
     * @returns The network interface identifier.
     *
     */
    int8_t GetInterfaceId(void) const { return mInterfaceId; }

    /**
     * This method registers a callback to notify internal IPv6 address changes.
     *
     * @param[in]  aCallback         A pointer to a function that is called when an IPv6 address is added or removed.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     */
    void SetAddressCallback(otIp6AddressCallback aCallback, void *aCallbackContext);

    /**
     * This method returns a pointer to the list of unicast addresses.
     *
     * @returns A pointer to the list of unicast addresses.
     *
     */
    const NetifUnicastAddress *GetUnicastAddresses(void) const { return mUnicastAddresses; }

    /**
     * This method adds a unicast address to the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval OT_ERROR_NONE      Successfully added the unicast address.
     * @retval OT_ERROR_ALREADY  The unicast address was already added.
     *
     */
    otError AddUnicastAddress(NetifUnicastAddress &aAddress);

    /**
     * This method removes a unicast address from the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval OT_ERROR_NONE       Successfully removed the unicast address.
     * @retval OT_ERROR_NOT_FOUND  The unicast address wasn't found to be removed.
     *
     */
    otError RemoveUnicastAddress(const NetifUnicastAddress &aAddress);

    /**
     * This method adds an external (to OpenThread) unicast address to the network interface.
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
     * This method indicates whether or not an address is assigned to this interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @returns TRUE if @p aAddress is assigned to this interface, FALSE otherwise.
     *
     */
    bool IsUnicastAddress(const Address &aAddress) const;

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
     * This method subscribes the network interface to the link-local and realm-local all routers address.
     *
     * @retval OT_ERROR_NONE     Successfully subscribed to the link-local and realm-local all routers address
     * @retval OT_ERROR_ALREADY  The multicast addresses are already subscribed.
     *
     */
    otError SubscribeAllRoutersMulticast(void);

    /**
     * This method unsubscribes the network interface to the link-local and realm-local all routers address.
     *
     * @retval OT_ERROR_NONE       Successfully unsubscribed from the link-local and realm-local all routers address
     * @retval OT_ERROR_NOT_FOUND  The multicast addresses were not found.
     *
     */
    otError UnsubscribeAllRoutersMulticast(void);

    /**
     * This method returns a pointer to the list of multicast addresses.
     *
     * @returns A pointer to the list of multicast addresses.
     *
     */
    const NetifMulticastAddress *GetMulticastAddresses(void) const { return mMulticastAddresses; }

    /**
     * This method subscribes the network interface to a multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval OT_ERROR_NONE     Successfully subscribed to @p aAddress.
     * @retval OT_ERROR_ALREADY  The multicast address is already subscribed.
     *
     */
    otError SubscribeMulticast(NetifMulticastAddress &aAddress);

    /**
     * This method unsubscribes the network interface to a multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval OT_ERROR_NONE       Successfully unsubscribed @p aAddress.
     * @retval OT_ERROR_NOT_FOUND  The multicast address was not found.
     *
     */
    otError UnsubscribeMulticast(const NetifMulticastAddress &aAddress);

    /**
     * This method provides the next external multicast address that the network interface subscribed.
     * It is used to iterate through the entries of the external multicast address table.
     *
     * @param[inout] aIterator A reference to the iterator context. To get the first
     *                         external multicast address, it should be set to 0.
     * @param[out]   aAddress  A reference where to place the external multicast address.
     *
     * @retval OT_ERROR_NONE       Successfully found the next external multicast address.
     * @retval OT_ERROR_NOT_FOUND  No subsequent external multicast address.
     *
     */
    otError GetNextExternalMulticast(uint8_t &aIterator, Address &aAddress);

    /**
     * This method subscribes the network interface to the external (to OpenThread) multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval OT_ERROR_NONE           Successfully subscribed to @p aAddress.
     * @retval OT_ERROR_ALREADY        The multicast address is already subscribed.
     * @retval OT_ERROR_INVALID_ARGS   The address indicated by @p aAddress is an internal multicast address.
     * @retval OT_ERROR_INVALID_STATE  The Network Interface is not up.
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
     */
    void UnsubscribeAllExternalMulticastAddresses(void);

    /**
     * This method checks if multicast promiscuous mode is enabled on the network interface.
     *
     * @retval TRUE   If the multicast promiscuous mode is enabled.
     * @retval FALSE  If the multicast promiscuous mode is disabled.
     */
    bool IsMulticastPromiscuousEnabled(void) { return mMulticastPromiscuous; }

    /**
     * This method enables multicast promiscuous mode on the network interface.
     *
     * @param[in]  aEnabled  TRUE if Multicast Promiscuous mode is enabled, FALSE otherwise.
     *
     */
    void SetMulticastPromiscuous(bool aEnabled) { mMulticastPromiscuous = aEnabled; }

    /**
     * This virtual method enqueues an IPv6 messages on this network interface.
     *
     * @param[in]  aMessage  A reference to the IPv6 message.
     *
     * @retval OT_ERROR_NONE  Successfully enqueued the IPv6 message.
     *
     */
    virtual otError SendMessage(Message &aMessage)
    {
        OT_UNUSED_VARIABLE(aMessage);
        return OT_ERROR_NOT_IMPLEMENTED;
    }

    /**
     * This virtual method fills out @p aAddress with the link address.
     *
     * @param[out]  aAddress  A reference to the link address.
     *
     * @retval OT_ERROR_NONE  Successfully retrieved the link address.
     *
     */
    virtual otError GetLinkAddress(LinkAddress &aAddress) const
    {
        OT_UNUSED_VARIABLE(aAddress);
        return OT_ERROR_NOT_IMPLEMENTED;
    }

    /**
     * This virtual method performs a source-destination route lookup.
     *
     * @param[in]   aSource       A reference to the IPv6 source address.
     * @param[in]   aDestination  A reference to the IPv6 destination address.
     * @param[out]  aPrefixMatch  The longest prefix match result.
     *
     * @retval OT_ERROR_NONE      Successfully found a route.
     * @retval OT_ERROR_NO_ROUTE  No route to destination.
     *
     */
    virtual otError RouteLookup(const Address &aSource, const Address &aDestination, uint8_t *aPrefixMatch)
    {
        OT_UNUSED_VARIABLE(aSource);
        OT_UNUSED_VARIABLE(aDestination);
        OT_UNUSED_VARIABLE(aPrefixMatch);
        return OT_ERROR_NOT_IMPLEMENTED;
    }

protected:
    /**
     * This method subscribes the network interface to the realm-local all MPL forwarders, link-local and
     * realm-local all nodes address.
     *
     */
    void SubscribeAllNodesMulticast(void);

    /**
     * This method unsubscribes the network interface from the realm-local all MPL forwarders, link-local and
     * realm-local all nodes address.
     *
     */
    void UnsubscribeAllNodesMulticast(void);

private:
    enum
    {
        kMulticastPrefixLength = 128, ///< Multicast prefix length used to notify internal address changes.
    };

    NetifUnicastAddress *  mUnicastAddresses;
    NetifMulticastAddress *mMulticastAddresses;
    int8_t                 mInterfaceId;
    bool                   mMulticastPromiscuous;
    Netif *                mNext;

    otIp6AddressCallback mAddressCallback;
    void *               mAddressCallbackContext;

    NetifUnicastAddress   mExtUnicastAddresses[OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS];
    NetifMulticastAddress mExtMulticastAddresses[OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS];

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
