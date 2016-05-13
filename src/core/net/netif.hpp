/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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

#include <common/message.hpp>
#include <common/tasklet.hpp>
#include <mac/mac_frame.hpp>
#include <net/ip6_address.hpp>
#include <net/socket.hpp>

namespace Thread {
namespace Ip6 {

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
public :
    /**
     * Hardware types.
     *
     */
    enum HardwareType
    {
        kEui64 = 27,
    };
    HardwareType   mType;       ///< Link address type.
    uint8_t        mLength;     ///< Length of link address.
    Mac::ExtAddress mExtAddress;  ///< Link address.
};

/**
 * This class implements an IPv6 network interface unicast address.
 *
 */
class NetifUnicastAddress: public otNetifAddress
{
    friend class Netif;

public:
    /**
     * This method returns the unicast address.
     *
     * @returns The unicast address.
     *
     */
    const Address &GetAddress(void) const { return *static_cast<const Address *>(&mAddress); }

    /**
     * This method returns the unicast address.
     *
     * @returns The unicast address.
     *
     */
    Address &GetAddress(void) { return *static_cast<Address *>(&mAddress); }

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
class NetifMulticastAddress
{
    friend class Netif;

public:
    /**
     * This method returns the multicast address.
     *
     * @returns The multicast address.
     *
     */
    const Address &GetAddress(void) const { return *static_cast<const Address *>(&mAddress); }

    /**
     * This method returns the multicast address.
     *
     * @returns The multicast address.
     *
     */
    Address &GetAddress(void) { return *static_cast<Address *>(&mAddress); }

    /**
     * This method returns the next multicast address assigned to the interface.
     *
     * @returns A pointer to the next multicast address.
     *
     */
    const NetifMulticastAddress *GetNext(void) const { return mNext; }

private:
    Address mAddress;
    NetifMulticastAddress *mNext;
};

/**
 * This class implements network interface handlers.
 *
 */
class NetifHandler
{
    friend class Netif;

public:
    /**
     * This function pointer is called when the set of assigned unicast addresses change.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    typedef void (*UnicastAddressesChangedHandler)(void *aContext);

    /**
     * This constructor initializes the network interface handlers.
     *
     * @param[in]  aHandler  A pointer to a function that is called when the set of assigned unicast addresses
     *                       change.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    NetifHandler(UnicastAddressesChangedHandler aHandler, void *aContext) {
        mUnicastHandler = aHandler;
        mContext = aContext;
    }

private:
    void HandleUnicastAddressesChanged(void) { mUnicastHandler(mContext); }

    UnicastAddressesChangedHandler mUnicastHandler;
    void *mContext;
    NetifHandler *mNext;
};

/**
 * This class implements an IPv6 network interface.
 *
 */
class Netif
{
public:
    /**
     * This constructor initializes the network interface.
     *
     */
    Netif(void);

    /**
     * This method enables the network interface.
     *
     * @retval kThreadError_None  Successfully enabled the network interface.
     * @retval KThreadError_Busy  The network interface was already enabled.
     *
     */
    ThreadError AddNetif(void);

    /**
     * This method disables the network interface.
     *
     * @retval kThreadError_None  Successfully disabled the network interface.
     * @retval KThreadError_Busy  The network interface was already disabled.
     *
     */
    ThreadError RemoveNetif(void);

    /**
     * This method returns the next network interface in the list.
     *
     * @returns A pointer to the next network interface.
     */
    Netif *GetNext(void) const;

    /**
     * This method returns the network interface identifier.
     *
     * @returns The network interface identifier.
     *
     */
    int GetInterfaceId(void) const;

    /**
     * This method returns a pointer to the list of unicast addresses.
     *
     * @returns A pointer to the list of unicast addresses.
     *
     */
    const NetifUnicastAddress *GetUnicastAddresses(void) const;

    /**
     * This method adds a unicast address to the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval kThreadError_None  Successfully added the unicast address.
     * @retval kThreadError_Busy  The unicast address was already added.
     *
     */
    ThreadError AddUnicastAddress(NetifUnicastAddress &aAddress);

    /**
     * This method removes a unicast address from the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval kThreadError_None  Successfully removed the unicast address.
     * @retval kThreadError_Busy  The unicast address was already removed.
     *
     */
    ThreadError RemoveUnicastAddress(const NetifUnicastAddress &aAddress);

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
     */
    void SubscribeAllRoutersMulticast(void);

    /**
     * This method unsubscribes the network interface to the link-local and realm-local all routers address.
     *
     */
    void UnsubscribeAllRoutersMulticast(void);

    /**
     * This method subscribes the network interface to a multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval kThreadError_None   Successfully subscribed to @p aAddress.
     * @retval kThreadError_Busy   The multicast address is already subscribed.
     *
     */
    ThreadError SubscribeMulticast(NetifMulticastAddress &aAddress);

    /**
     * This method unsubscribes the network interface to a multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval kThreadError_None   Successfully unsubscribed to @p aAddress.
     * @retval kThreadError_Busy   The multicast address is already unsubscribed.
     *
     */
    ThreadError UnsubscribeMulticast(const NetifMulticastAddress &aAddress);

    /**
     * This method registers a network interface handler.
     *
     * @param[in]  aHandler  A reference to the handler.
     *
     * @retval kThreadError_None   Successfully registered the handler.
     * @retval kThreadError_Busy   The handler was already registered.
     */
    ThreadError RegisterHandler(NetifHandler &aHandler);

    /**
     * This virtual method enqueues an IPv6 messages on this network interface.
     *
     * @param[in]  aMessage  A reference to the IPv6 message.
     *
     * @retval kThreadError_None  Successfully enqueued the IPv6 message.
     *
     */
    virtual ThreadError SendMessage(Message &aMessage) = 0;

    /**
     * This virtual method returns a NULL-termianted string that names the network interface.
     *
     * @returns A NULL-terminated string that names the network interface.
     *
     */
    virtual const char *GetName(void) const = 0;

    /**
     * This virtual method fills out @p aAddress with the link address.
     *
     * @param[out]  aAddress  A reference to the link address.
     *
     * @retval kThreadError_None  Successfully retrieved the link address.
     *
     */
    virtual ThreadError GetLinkAddress(LinkAddress &aAddress) const = 0;

    /**
     * This virtual method performs a source-destination route lookup.
     *
     * @param[in]   aSource       A reference to the IPv6 source address.
     * @param[in]   aDestination  A reference to the IPv6 destinationa ddress.
     * @param[out]  aPrefixMatch  The longest prefix match result.
     *
     * @retval kThreadError_None     Successfully found a route.
     * @retval kThreadError_NoRoute  No route to destination.
     *
     */
    virtual ThreadError RouteLookup(const Address &aSource, const Address &aDestination,
                                    uint8_t *aPrefixMatch) = 0;

    /**
     * This static method returns the network interface list.
     *
     * @returns A pointer to the network interface list.
     *
     */
    static Netif *GetNetifList(void);

    /**
     * This static method returns the network interface identified by @p aInterfaceId.
     *
     * @param[in]  aInterfaceId  The network interface ID.
     *
     * @returns A pointer to the network interface or NULL if none is found.
     *
     */
    static Netif *GetNetifById(uint8_t aInterfaceId);

    /**
     * This static method returns the network interface identified by @p aName.
     *
     * @param[in]  aName  A pointer to a NULL-terminated string.
     *
     * @returns A pointer to the network interface or NULL if none is found.
     *
     */
    static Netif *GetNetifByName(char *aName);

    /**
     * This static method indicates whether or not @p aAddress is assigned to a network interfacfe.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     *
     * @retval TRUE   If the IPv6 address is assigned to a network interface.
     * @retval FALSE  If the IPv6 address is not assigned to any network interface.
     *
     */
    static bool IsUnicastAddress(const Address &aAddress);

    /**
     * This static method perform default source address selection.
     *
     * @param[in]  aMessageInfo  A reference to the message information.
     *
     * @returns A pointer to the selected IPv6 source address or NULL if no source address was found.
     *
     */
    static const NetifUnicastAddress *SelectSourceAddress(MessageInfo &aMessageInfo);

    /**
     * This static method determines which network interface @p aAddress is on-link, if any.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     *
     * @returns The network interafce identifier for the on-link interface or -1 if none is found.
     *
     */
    static int GetOnLinkNetif(const Address &aAddress);

private:
    static void HandleUnicastChangedTask(void *aContext);
    void HandleUnicastChangedTask(void);

    NetifHandler *mHandlers;
    NetifUnicastAddress *mUnicastAddresses;
    NetifMulticastAddress *mMulticastAddresses;
    int mInterfaceId;
    bool mAllRoutersSubscribed;
    Tasklet mUnicastChangedTask;
    Netif *mNext;

    static Netif *sNetifListHead;
    static int sNextInterfaceId;
};

/**
 * @}
 *
 */

}  // namespace Ip6
}  // namespace Thread

#endif  // NET_NETIF_HPP_
