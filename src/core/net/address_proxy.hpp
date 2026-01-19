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
 *   This file includes definitions for IPv6 Address Proxy.
 */

#ifndef OT_CORE_NET_ADDRESS_PROXY_HPP_
#define OT_CORE_NET_ADDRESS_PROXY_HPP_

#include "openthread-core-config.h"

#include <openthread/ip6.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "net/ip6_address.hpp"

namespace ot {

namespace Ip6 {

class AddressProxy;

/**
 * Represents an IPv6 proxy address.
 *
 */
class ProxyAddress : public LinkedListEntry<ProxyAddress>, public Clearable<ProxyAddress>
{
    friend class AddressProxy;

public:
    /**
     * Defines the function pointer which is called when an IPv6 packet is received for the proxy address.
     *
     */
    typedef otIp6ReceiveCallback Callback;

    /**
     * Initializes the object.
     *
     */
    ProxyAddress(void) { Clear(); }

    /**
     * Initializes the object.
     *
     * @param[in]  aAddress   The IPv6 address.
     * @param[in]  aCallback  The callback function.
     * @param[in]  aContext   The callback context.
     *
     */
    void Set(const Address &aAddress, Callback aCallback, void *aContext)
    {
        mAddress  = aAddress;
        mCallback = aCallback;
        mContext  = aContext;
    }

    /**
     * Gets the IPv6 address.
     *
     * @returns The IPv6 address.
     *
     */
    const Address &GetAddress(void) const { return mAddress; }

    /**
     * Gets the IPv6 address.
     *
     * @returns The IPv6 address.
     *
     */
    Address &GetAddress(void) { return mAddress; }

    ProxyAddress *mNext;

private:
    void InvokeCallback(ot::Message &aMessage)
    {
        if (mCallback != nullptr)
        {
            mCallback(reinterpret_cast<otMessage *>(&aMessage), mContext);
        }
    }

    Address  mAddress;
    Callback mCallback;
    void    *mContext;
};

/**
 * Implements IPv6 Address Proxy.
 *
 */
class AddressProxy : public InstanceLocator, private NonCopyable
{
public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit AddressProxy(Instance &aInstance);

    /**
     * Adds an IPv6 proxy address.
     *
     * @param[in]  aAddress       A reference to the IPv6 proxy address.
     *
     */
    void AddAddress(ProxyAddress &aAddress);

    /**
     * Removes an IPv6 proxy address.
     *
     * @param[in]  aAddress       A reference to the IPv6 proxy address.
     *
     */
    void RemoveAddress(ProxyAddress &aAddress);

    /**
     * Indicates whether or not the IPv6 address is a proxy address.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     *
     * @retval TRUE   The @p aAddress is a proxy address.
     * @retval FALSE  The @p aAddress is not a proxy address.
     *
     */
    bool IsProxyAddress(const Address &aAddress) const;

    /**
     * Handles a received datagram for a proxy address.
     *
     * @param[in]  aMessage  The received message.
     *
     */
    void HandleDatagram(ot::Message &aMessage);

    /**
     * Iterates over all proxy addresses.
     *
     * @param[in]  aCallback  A pointer to the callback function.
     * @param[in]  aContext   A pointer to application-specific context.
     *
     */
    void ForEachAddress(otIp6ProxyAddressCallback aCallback, void *aContext) const;

private:
    LinkedList<ProxyAddress> mProxyAddresses;
};

} // namespace Ip6

} // namespace ot

#endif // OT_CORE_NET_ADDRESS_PROXY_HPP_
