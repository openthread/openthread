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

#include <common/message.hpp>
#include <common/tasklet.hpp>
#include <mac/mac_frame.hpp>
#include <net/ip6_address.hpp>
#include <net/socket.hpp>

namespace Thread {
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
     * This method returns the IPv6 scope value.
     *
     * @returns The IPv6 scope value.
     *
     */
    uint8_t GetScope(void) const {
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
class NetifMulticastAddress: public otNetifMulticastAddress
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
     * This method returns the next multicast address subscribed to the interface.
     *
     * @returns A pointer to the next multicast address.
     *
     */
    const NetifMulticastAddress *GetNext(void) const { return static_cast<NetifMulticastAddress *>(mNext); }

    /**
     * This method returns the next multicast address subscribed to the interface.
     *
     * @returns A pointer to the next multicast address.
     *
     */
    NetifMulticastAddress *GetNext(void) { return static_cast<NetifMulticastAddress *>(mNext); }
};

/**
 * This class implements network interface handlers.
 *
 */
class NetifCallback
{
    friend class Netif;

public:
    /**
     * This constructor initializes the object.
     *
     */
    NetifCallback(void):
        mCallback(NULL),
        mContext(NULL),
        mNext(NULL) {
    }

    /**
     * This method sets the callback information.
     *
     * @param[in]  aCallback  A pointer to a function that is called when configuration or state changes.
     * @param[in]  aContext   A pointer to arbitrary context information.
     *
     */
    void Set(otStateChangedCallback aCallback, void *aContext) {
        mCallback = aCallback;
        mContext = aContext;
    }

    /**
     * This method tests whether the object is free or in use.
     *
     * @returns True if the object is free, false otherwise.
     *
     */
    bool IsFree(void) {
        return (mCallback == NULL);
    }

    /**
     * This method frees the object.
     *
     */
    void Free(void) {
        mCallback = NULL;
        mContext = NULL;
        mNext = NULL;
    }

    /**
     * This method tests whether the object is set to the provided elements.
     *
     * @param[in]  aCallback  A pointer to a function that is called when configuration or state changes.
     * @param[in]  aContext   A pointer to arbitrary context information.
     *
     * @returns True if the object elements equal the input params, false otherwise.
     *
     */
    bool IsServing(otStateChangedCallback aCallback, void *aContext) {
        return (aCallback == mCallback && aContext == mContext);
    }

private:
    void Callback(uint32_t aFlags) {
        if (mCallback != NULL) {
            mCallback(aFlags, mContext);
        }
    }

    otStateChangedCallback mCallback;
    void *mContext;
    NetifCallback *mNext;
};

/**
 * This class implements an IPv6 network interface.
 *
 */
class Netif
{
    friend class Ip6;

public:
    /**
     * This constructor initializes the network interface.
     *
     * @param[in]  aIp6             A reference to the IPv6 network object.
     * @param[in]  aInterfaceId     The interface ID for this object.
     *
     */
    Netif(Ip6 &aIp6, int8_t aInterfaceId);

    /**
     * This method returns a reference to the IPv6 network object.
     *
     * @returns A reference to the IPv6 network object.
     *
     */
    Ip6 &GetIp6(void);

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
    int8_t GetInterfaceId(void) const;

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
     * @retval kThreadError_None     Successfully added the unicast address.
     * @retval kThreadError_Already  The unicast address was already added.
     *
     */
    ThreadError AddUnicastAddress(NetifUnicastAddress &aAddress);

    /**
     * This method removes a unicast address from the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval kThreadError_None      Successfully removed the unicast address.
     * @retval kThreadError_NotFound  The unicast address wasn't found to be removed.
     *
     */
    ThreadError RemoveUnicastAddress(const NetifUnicastAddress &aAddress);

    /**
     * This method adds an external (to OpenThread) unicast address to the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval kThreadError_None         Successfully added (or updated) the unicast address.
     * @retval kThreadError_InvalidArgs  The address indicated by @p aAddress is an internal address.
     * @retval kThreadError_NoBufs       The maximum number of allowed external addresses are already added.
     *
     */
    ThreadError AddExternalUnicastAddress(const NetifUnicastAddress &aAddress);

    /**
     * This method removes a external (to OpenThread) unicast address from the network interface.
     *
     * @param[in]  aAddress  A reference to the unicast address.
     *
     * @retval kThreadError_None         Successfully removed the unicast address.
     * @retval kThreadError_InvalidArgs  The address indicated by @p aAddress is an internal address.
     * @retval kThreadError_NotFound     The unicast address was not found.
     *
     */
    ThreadError RemoveExternalUnicastAddress(const Address &aAddress);

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
     */
    void SubscribeAllRoutersMulticast(void);

    /**
     * This method unsubscribes the network interface to the link-local and realm-local all routers address.
     *
     */
    void UnsubscribeAllRoutersMulticast(void);

    /**
     * This method returns a pointer to the list of multicast addresses.
     *
     * @returns A pointer to the list of multicast addresses.
     *
     */
    const NetifMulticastAddress *GetMulticastAddresses(void) const;

    /**
     * This method subscribes the network interface to a multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval kThreadError_None     Successfully subscribed to @p aAddress.
     * @retval kThreadError_Already  The multicast address is already subscribed.
     *
     */
    ThreadError SubscribeMulticast(NetifMulticastAddress &aAddress);

    /**
     * This method unsubscribes the network interface to a multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval kThreadError_None     Successfully unsubscribed to @p aAddress.
     * @retval kThreadError_Already  The multicast address is already unsubscribed.
     *
     */
    ThreadError UnsubscribeMulticast(const NetifMulticastAddress &aAddress);

    /**
     * This method subscribes the network interface to the external (to OpenThread) multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval kThreadError_None         Successfully subscribed to @p aAddress.
     * @retval kThreadError_Already      The multicast address is already subscribed.
     * @retval kThreadError_InvalidArgs  The address indicated by @p aAddress is an internal multicast address.
     * @retval kThreadError_NoBufs       The maximum number of allowed external multicast addresses are already added.
     *
     */
    ThreadError SubscribeExternalMulticast(const Address &aAddress);

    /**
     * This method ussubscribes the network interface to the external (to OpenThread) multicast address.
     *
     * @param[in]  aAddress  A reference to the multicast address.
     *
     * @retval kThreadError_None         Successfully unsubscribed to the unicast address.
     * @retval kThreadError_InvalidArgs  The address indicated by @p aAddress is an internal address.
     * @retval kThreadError_NotFound     The multicast address was not found.
     *
     */
    ThreadError UnsubscribeExternalMulticast(const Address &aAddress);

    /**
     * This method checks if multicast promiscuous mode is enabled on the network interface.
     *
     * @retval TRUE   If the multicast promiscuous mode is enabled.
     * @retval FALSE  If the multicast promiscuous mode is disabled.
     */
    bool IsMulticastPromiscuousModeEnabled(void);

    /**
     * This method enables multicast promiscuous mode on the network interface.
     *
     */
    void EnableMulticastPromiscuousMode(void);

    /**
     * This method disables multicast promiscuous mode on the network interface.
     *
     */
    void DisableMulticastPromiscuousMode(void);

    /**
     * This method registers a network interface callback.
     *
     * @param[in]  aCallback  A reference to the callback.
     *
     * @retval kThreadError_None    Successfully registered the callback.
     * @retval kThreadError_Already The callback was already registered.
     */
    ThreadError RegisterCallback(NetifCallback &aCallback);

    /**
     * This method removes a network interface callback.
     *
     * @param[in]  aCallback  A reference to the callback.
     *
     * @retval kThreadError_None    Successfully removed the callback.
     * @retval kThreadError_Already The callback was not in the list.
     */
    ThreadError RemoveCallback(NetifCallback &aCallback);

    /**
     * This method indicates whether or not a state changed callback is pending.
     *
     * @retval TRUE if a state changed callback is pending, FALSE otherwise.
     *
     */
    bool IsStateChangedCallbackPending(void);

    /**
     * This method schedules notification of @p aFlags.
     *
     * The @p aFlags are combined (bitwise-or) with other flags that have not been provided in a callback yet.
     *
     * @param[in]  aFlags  A bit-field indicating what configuration or state has changed.
     *
     */
    void SetStateChangedFlags(uint32_t aFlags);

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
     * @param[in]   aDestination  A reference to the IPv6 destination address.
     * @param[out]  aPrefixMatch  The longest prefix match result.
     *
     * @retval kThreadError_None     Successfully found a route.
     * @retval kThreadError_NoRoute  No route to destination.
     *
     */
    virtual ThreadError RouteLookup(const Address &aSource, const Address &aDestination,
                                    uint8_t *aPrefixMatch) = 0;

protected:
    Ip6 &mIp6;

private:
    static void HandleStateChangedTask(void *aContext);
    void HandleStateChangedTask(void);

    NetifCallback *mCallbacks;
    NetifUnicastAddress *mUnicastAddresses;
    NetifMulticastAddress *mMulticastAddresses;
    int8_t mInterfaceId;
    bool mAllRoutersSubscribed;
    bool mMulticastPromiscuousMode;
    Tasklet mStateChangedTask;
    Netif *mNext;

    uint32_t mStateChangedFlags;

    NetifUnicastAddress mExtUnicastAddresses[OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS];
    NetifMulticastAddress mExtMulticastAddresses[OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS];
};

/**
 * @}
 *
 */

}  // namespace Ip6
}  // namespace Thread

#endif  // NET_NETIF_HPP_
