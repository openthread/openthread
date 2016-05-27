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
 *   This file includes definitions for the Thread network interface.
 */

#ifndef THREAD_NETIF_HPP_
#define THREAD_NETIF_HPP_

#include <openthread-types.h>
#include <mac/mac.hpp>
#include <net/netif.hpp>
#include <thread/address_resolver.hpp>
#include <thread/key_manager.hpp>
#include <thread/mesh_forwarder.hpp>
#include <thread/mle.hpp>
#include <thread/mle_router.hpp>
#include <thread/network_data_local.hpp>

namespace Thread {

/**
 * @addtogroup core-netif
 *
 * @brief
 *   This module includes definitions for the Thread network interface.
 *
 * @{
 */

class ThreadNetif: public Ip6::Netif
{
public:
    /**
     * This constructor initializes the Thread network interface.
     *
     */
    ThreadNetif(void);

    /**
     * This method enables the Thread network interface.
     *
     */
    ThreadError Up(void);

    /**
     * This method disables the Thread network interface.
     *
     */
    ThreadError Down(void);

    /**
     * This method indicates whether or not the Thread network interface is enabled.
     *
     * @retval TRUE   If the Thread network interface is enabled.
     * @retval FALSE  If the Thread network interface is not enabled.
     *
     */
    bool IsUp(void) const;

    /**
     * This method returns a pointer to a NULL-terminated string that names the interface.
     *
     * @returns A pointer to a NULL-terminated string that names the interface.
     *
     */
    const char *GetName(void) const;

    /**
     * This method retrieves the link address.
     *
     * @param[out]  aAddress  A reference to the link address.
     *
     */
    ThreadError GetLinkAddress(Ip6::LinkAddress &aAddress) const;

    /**
     * This method submits a message to the network interface.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval kThreadError_None  Successfully submitted the message to the interface.
     *
     */
    ThreadError SendMessage(Message &aMessage);

    /**
     * This method performs a route lookup.
     *
     * @param[in]   aSource       A reference to the IPv6 source address.
     * @param[in]   aDestination  A reference to the IPv6 destination address.
     * @param[out]  aPrefixMatch  A pointer where the number of prefix match bits for the chosen route is stored.
     *
     * @retval kThreadError_None     Successfully found a route.
     * @retval kThreadError_NoRoute  Could not find a valid route.
     *
     */
    ThreadError RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination, uint8_t *aPrefixMatch);

    /**
     * This method returns a pointer to the address resolver object.
     *
     * @returns A pointer to the address resolver object.
     *
     */
    AddressResolver &GetAddressResolver(void) { return mAddressResolver; }

    /**
     * This method returns a pointer to the coap server object.
     *
     * @returns A pointer to the coap server object.
     *
     */
    Coap::Server &GetCoapServer(void) { return mCoapServer; }

    /**
     * This method returns a pointer to the key manager object.
     *
     * @returns A pointer to the key manager object.
     *
     */
    KeyManager &GetKeyManager(void) { return mKeyManager; }

    /**
     * This method returns a pointer to the lowpan object.
     *
     * @returns A pointer to the lowpan object.
     *
     */
    Lowpan::Lowpan &GetLowpan(void) { return mLowpan; }

    /**
     * This method returns a pointer to the mac object.
     *
     * @returns A pointer to the mac object.
     *
     */
    Mac::Mac &GetMac(void) { return mMac; }

    /**
     * This method returns a pointer to the mle object.
     *
     * @returns A pointer to the mle object.
     *
     */
    Mle::MleRouter &GetMle(void) { return mMleRouter; }

    /**
     * This method returns a pointer to the mesh forwarder object.
     *
     * @returns A pointer to the mesh forwarder object.
     *
     */
    MeshForwarder &GetMeshForwarder(void) { return mMeshForwarder; }

    /**
     * This method returns a pointer to the network data local object.
     *
     * @returns A pointer to the network data local object.
     *
     */
    NetworkData::Local &GetNetworkDataLocal(void) { return mNetworkDataLocal; }

    /**
     * This method returns a pointer to the network data leader object.
     *
     * @returns A pointer to the network data leader object.
     *
     */
    NetworkData::Leader &GetNetworkDataLeader(void) { return mNetworkDataLeader; }

private:
    Coap::Server mCoapServer;
    AddressResolver mAddressResolver;
    KeyManager mKeyManager;
    Lowpan::Lowpan mLowpan;
    Mac::Mac mMac;
    MeshForwarder mMeshForwarder;
    Mle::MleRouter mMleRouter;
    NetworkData::Local mNetworkDataLocal;
    NetworkData::Leader mNetworkDataLeader;
    bool mIsUp;
};

/**
 * This structure represents Thread-specific link information.
 *
 */
struct ThreadMessageInfo
{
    uint8_t mLinkMargin;     ///< The Link Margin for a received message in dB.
    uint8_t mLqi;            ///< The Link Quality Indicator for a received message.
    bool    mSecurityValid;  ///< Link security on all received frames was enabled and passed validation.
};

/**
 * @}
 */

}  // namespace Thread

#endif  // THREAD_NETIF_HPP_
