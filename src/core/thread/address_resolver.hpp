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
 *   This file includes definitions for Thread EID-to-RLOC mapping and caching.
 */

#ifndef ADDRESS_RESOLVER_HPP_
#define ADDRESS_RESOLVER_HPP_

#include <openthread/types.h>

#include "openthread-core-config.h"
#include "coap/coap.hpp"
#include "common/timer.hpp"
#include "mac/mac.hpp"
#include "net/icmp6.hpp"
#include "net/udp6.hpp"

namespace ot {

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
     * This method returns the pointer to the parent otInstance structure.
     *
     * @returns The pointer to the parent otInstance structure.
     *
     */
    otInstance *GetInstance(void);

    /**
     * This method clears the EID-to-RLOC cache.
     *
     */
    void Clear(void);

    /**
     * This method gets an EID cache entry.
     *
     * @param[in]   aIndex  An index into the EID cache table.
     * @param[out]  aEntry  A pointer to where the EID information is placed.
     *
     * @retval OT_ERROR_NONE          Successfully retrieved the EID cache entry.
     * @retval OT_ERROR_INVALID_ARGS  @p aIndex was out of bounds or @p aEntry was NULL.
     *
     */
    otError GetEntry(uint8_t aIndex, otEidCacheEntry &aEntry) const;

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
     * @retval OT_ERROR_NONE           Successfully provided the RLOC16.
     * @retval OT_ERROR_ADDRESS_QUERY  Initiated an Address Query.
     *
     */
    otError Resolve(const Ip6::Address &aEid, Mac::ShortAddress &aRloc16);

private:
    enum
    {
        kCacheEntries = OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES,
        kStateUpdatePeriod = 1000u, ///< State update period in milliseconds.
    };

    /**
     * Thread Protocol Parameters and Constants
     *
     */
    enum
    {
        kAddressQueryTimeout = 3,            ///< ADDRESS_QUERY_TIMEOUT (seconds)
        kAddressQueryInitialRetryDelay = 15, ///< ADDRESS_QUERY_INITIAL_RETRY_DELAY (seconds)
        kAddressQueryMaxRetryDelay = 28800,  ///< ADDRESS_QUERY_MAX_RETRY_DELAY (seconds)
    };

    struct Cache
    {
        Ip6::Address      mTarget;
        uint8_t           mMeshLocalIid[Ip6::Address::kInterfaceIdentifierSize];
        uint32_t          mLastTransactionTime;
        Mac::ShortAddress mRloc16;
        uint16_t          mRetryTimeout;
        uint8_t           mTimeout;
        uint8_t           mFailures;
        uint8_t           mAge;

        enum State
        {
            kStateInvalid,
            kStateQuery,
            kStateCached,
        };
        State mState;
    };

    Cache *NewCacheEntry(void);
    void MarkCacheEntryAsUsed(Cache &aEntry);
    void InvalidateCacheEntry(Cache &aEntry);

    otError SendAddressQuery(const Ip6::Address &aEid);
    otError SendAddressError(const ThreadTargetTlv &aTarget, const ThreadMeshLocalEidTlv &aEid,
                             const Ip6::Address *aDestination);
    void SendAddressQueryResponse(const ThreadTargetTlv &aTargetTlv, const ThreadMeshLocalEidTlv &aMlEidTlv,
                                  const ThreadLastTransactionTimeTlv *aLastTransactionTimeTlv,
                                  const Ip6::Address &aDestination);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

    static void HandleAddressError(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                   const otMessageInfo *aMessageInfo);
    void HandleAddressError(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleAddressQuery(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                   const otMessageInfo *aMessageInfo);
    void HandleAddressQuery(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleAddressNotification(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                          const otMessageInfo *aMessageInfo);
    void HandleAddressNotification(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleIcmpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo,
                                  const otIcmp6Header *aIcmpHeader);
    void HandleIcmpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo, const Ip6::IcmpHeader &aIcmpHeader);

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    Coap::Resource   mAddressError;
    Coap::Resource   mAddressQuery;
    Coap::Resource   mAddressNotification;
    Cache            mCache[kCacheEntries];
    Ip6::IcmpHandler mIcmpHandler;
    Timer            mTimer;

    ThreadNetif     &mNetif;
};

/**
 * @}
 */

} // namespace ot

#endif  // ADDRESS_RESOLVER_HPP_
