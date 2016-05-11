/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes definitions for Thread EID-to-RLOC mapping and caching.
 */

#ifndef ADDRESS_RESOLVER_HPP_
#define ADDRESS_RESOLVER_HPP_

#include <openthread-core-config.h>
#include <openthread-types.h>
#include <coap/coap_server.hpp>
#include <common/timer.hpp>
#include <mac/mac.hpp>
#include <net/icmp6.hpp>
#include <net/udp6.hpp>

namespace Thread {

class MeshForwarder;
class ThreadLastTransactionTimeTlv;
class ThreadMeshLocalEidTlv;
class ThreadNetif;
class ThreadTargetTlv;

/**
 * @addtogroup core-arp
 *
 * @brief
 *   This module includes definitions for Thread EID-to-RLOC mapping and caching.
 *
 * @{
 */

/**
 * This class implements the EID-to-RLOC mapping and caching.
 *
 */
class AddressResolver
{
public:
    /**
     * This constructor initializes the object.
     *
     */
    explicit AddressResolver(ThreadNetif &aThreadNetif);

    /**
     * This method clears the EID-to-RLOC cache.
     *
     */
    void Clear(void);

    /**
     * This method removes a Router ID from the EID-to-RLOC cache.
     *
     * @param[in]  aRouterId  The Router ID.
     *
     */
    void Remove(uint8_t aRouterId);

    /**
     * This method returns the RLOC16 for a given EID, or initiates an Address Query if the mapping is not known.
     *
     * @param[in]   aEid     A reference to the EID.
     * @param[out]  aRloc16  The RLOC16 corresponding to @p aEid.
     *
     * @retval kTheradError_None          Successfully provided the RLOC16.
     * @retval kThreadError_AddressQuery  Initiated an Address Query.
     *
     */
    ThreadError Resolve(const Ip6::Address &aEid, Mac::ShortAddress &aRloc16);

private:
    enum
    {
        kCacheEntries = OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES,
        kStateUpdatePeriod = 1000u,           ///< State update period in milliseconds.
    };

    /**
     * Thread Protocol Parameters and Constants
     *
     */
    enum
    {
        kAddressQueryTimeout = 3,             ///< ADDRESS_QUERY_TIMEOUT (seconds)
        kAddressQueryInitialRetryDelay = 15,  ///< ADDRESS_QUERY_INITIAL_RETRY_DELAY (seconds)
        kAddressQueryMaxRetryDelay = 480,     ///< ADDRESS_QUERY_MAX_RETRY_DELAY (seconds)
    };

    struct Cache
    {
        Ip6::Address      mTarget;
        uint8_t           mMeshLocalIid[Ip6::Address::kInterfaceIdentifierSize];
        Mac::ShortAddress mRloc16;
        uint16_t          mRetryTimeout;
        uint8_t           mTimeout;
        uint8_t           mFailures;

        enum State
        {
            kStateInvalid,
            kStateQuery,
            kStateCached,
        };
        State             mState;
    };

    ThreadError SendAddressQuery(const Ip6::Address &aEid);
    ThreadError SendAddressError(const ThreadTargetTlv &aTarget, const ThreadMeshLocalEidTlv &aEid,
                                 const Ip6::Address *aDestination);
    void SendAddressQueryResponse(const ThreadTargetTlv &aTargetTlv, const ThreadMeshLocalEidTlv &aMlEidTlv,
                                  const ThreadLastTransactionTimeTlv *aLastTransactionTimeTlv,
                                  const Ip6::Address &aDestination);
    void SendAddressNotificationResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo);

    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);

    static void HandleAddressError(void *aContext, Coap::Header &aHeader,
                                   Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void HandleAddressError(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleAddressQuery(void *aContext, Coap::Header &aHeader,
                                   Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void HandleAddressQuery(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleAddressNotification(void *aContext, Coap::Header &aHeader,
                                          Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void HandleAddressNotification(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDstUnreach(void *aContext, Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                 const Ip6::IcmpHeader &aIcmpHeader);
    void HandleDstUnreach(Message &aMessage, const Ip6::MessageInfo &aMessageInfo, const Ip6::IcmpHeader &aIcmpHeader);

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    Coap::Resource mAddressError;
    Coap::Resource mAddressQuery;
    Coap::Resource mAddressNotification;
    Cache mCache[kCacheEntries];
    uint16_t mCoapMessageId;
    uint8_t mCoapToken[2];
    Ip6::IcmpHandler mIcmpHandler;
    Ip6::UdpSocket mSocket;
    Timer mTimer;

    MeshForwarder &mMeshForwarder;
    Coap::Server &mCoapServer;
    Mle::MleRouter &mMle;
    Ip6::Netif &mNetif;
};

/**
 * @}
 */

}  // namespace Thread

#endif  // ADDRESS_RESOLVER_HPP_
