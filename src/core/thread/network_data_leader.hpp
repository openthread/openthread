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
 *   This file includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 */

#ifndef NETWORK_DATA_LEADER_HPP_
#define NETWORK_DATA_LEADER_HPP_

#include <openthread-std-types.h>

#include <coap/coap_server.hpp>
#include <common/timer.hpp>
#include <net/ip6_address.hpp>
#include <thread/mle_router.hpp>
#include <thread/network_data.hpp>

namespace Thread {

class ThreadNetif;

namespace NetworkData {

/**
 * @addtogroup core-netdata-leader
 *
 * @brief
 *   This module includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 *
 * @{
 *
 */

/**
 * This class implements the Thread Network Data maintained by the Leader.
 *
 */
class Leader: public NetworkData
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    explicit Leader(ThreadNetif &aThreadNetif);

    /**
     * This method reset the Thread Network Data.
     *
     */
    void Reset(void);

    /**
     * This method starts the Leader services.
     *
     */
    void Start(void);

    /**
     * This method stops the Leader services.
     *
     */
    void Stop(void);

    /**
     * This method returns the Thread Network Data version.
     *
     * @returns The Thread Network Data version.
     *
     */
    uint8_t GetVersion(void) const;

    /**
     * This method increments the Thread Network Data version.
     *
     */
    void IncrementVersion(void);

    /**
     * This method returns the Thread Network Data stable version.
     *
     * @returns The Thread Network Data stable version.
     *
     */
    uint8_t GetStableVersion(void) const;

    /**
     * This method increments the Thread Network Data stable version.
     *
     */
    void IncrementStableVersion(void);

    /**
     * This method returns CONTEXT_ID_RESUSE_DELAY value.
     *
     * @returns The CONTEXT_ID_REUSE_DELAY value.
     *
     */
    uint32_t GetContextIdReuseDelay(void) const;

    /**
     * This method sets CONTEXT_ID_RESUSE_DELAY value.
     *
     * @warning This method should only be used for testing.
     *
     * @param[in]  aDelay  The CONTEXT_ID_REUSE_DELAY value.
     *
     */
    ThreadError SetContextIdReuseDelay(uint32_t aDelay);

    /**
     * This method retrieves the 6LoWPAN Context information based on a given IPv6 address.
     *
     * @param[in]   aAddress  A reference to an IPv6 address.
     * @param[out]  aContext  A reference to 6LoWPAN Context information.
     *
     * @retval kThreadError_None      Successfully retrieved 6LoWPAN Context information.
     * @retval kThreadError_NotFound  Could not find the 6LoWPAN Context information.
     *
     */
    ThreadError GetContext(const Ip6::Address &aAddress, Lowpan::Context &aContext);

    /**
     * This method retrieves the 6LoWPAN Context information based on a given Context ID.
     *
     * @param[in]   aContextId  The Context ID value.
     * @param[out]  aContext    A reference to the 6LoWPAN Context information.
     *
     * @retval kThreadError_None      Successfully retrieved 6LoWPAN Context information.
     * @retval kThreadError_NotFound  Could not find the 6LoWPAN Context information.
     *
     */
    ThreadError GetContext(uint8_t aContextId, Lowpan::Context &aContext);

    /**
     * This method indicates whether or not the given IPv6 address is on-mesh.
     *
     * @param[in]  aAddress  A reference to an IPv6 address.
     *
     * @retval TRUE   If @p aAddress is on-link.
     * @retval FALSE  If @p aAddress if not on-link.
     *
     */
    bool IsOnMesh(const Ip6::Address &aAddress);

    /**
     * This method performs a route lookup using the Network Data.
     *
     * @param[in]   aSource       A reference to the IPv6 source address.
     * @param[in]   aDestination  A reference to the IPv6 destination address.
     * @param[out]  aPrefixMatch  A pointer to the longest prefix match length in bits.
     * @param[out]  aRloc16       A pointer to the RLOC16 for the selected route.
     *
     * @retval kThreadError_None     Successfully found a route.
     * @retval kThreadError_NoRoute  No valid route was found.
     *
     */
    ThreadError RouteLookup(const Ip6::Address &aSource, const Ip6::Address &aDestination,
                            uint8_t *aPrefixMatch, uint16_t *aRloc16);

    /**
     * This method is used by non-Leader devices to set newly received Network Data from the Leader.
     *
     * @param[in]  aVersion        The Version value.
     * @param[in]  aStableVersion  The Stable Version value.
     * @param[in]  aStableOnly     TRUE if storing only the stable data, FALSE otherwise.
     * @param[in]  aData           A pointer to the Network Data.
     * @param[in]  aDataLength     The length of the Network Data in bytes.
     *
     */
    void SetNetworkData(uint8_t aVersion, uint8_t aStableVersion, bool aStableOnly, const uint8_t *aData,
                        uint8_t aDataLength);

    /**
     * This method removes Network Data associated with a given RLOC16.
     *
     * @param[in]  aRloc16  A RLOC16 value.
     *
     */
    void RemoveBorderRouter(uint16_t aRloc16);

    /**
     * This method sends a Server Data Notification message to the Leader indicating an invalid RLOC16.
     *
     * @param[in]  aRloc16  The invalid RLOC16 to notify.
     *
     * @retval kThreadError_None    Successfully enqueued the notification message.
     * @retval kThreadError_NoBufs  Insufficient message buffers to generate the notification message.
     *
     */
    ThreadError SendServerDataNotification(uint16_t aRloc16);

    /**
     * This method returns a pointer to the Commissioning Data.
     *
     * @param[out]  aLength  The length of the Commissioning Data in bytes.
     *
     * @returns A pointer to the Commissioning Data or NULL if no Commissioning Data exists.
     *
     */
    uint8_t *GetCommissioningData(uint8_t &aLength);

    /**
     * This method adds Commissioning Data to the Thread Network Data.
     *
     * @param[in]  aValue        A pointer to the Commissioning Data value.
     * @param[in]  aValueLength  The length of @p aValue.
     *
     * @retval kThreadError_None    Successfully added the Commissioning Data.
     * @retval kThreadError_NoBufs  Insufficient space to add the Commissioning Data.
     *
     */
    ThreadError SetCommissioningData(const uint8_t *aValue, uint8_t aValueLength);

private:
    static void HandleServerData(void *aContext, Coap::Header &aHeader, Message &aMessage,
                                 const Ip6::MessageInfo &aMessageInfo);
    void HandleServerData(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void SendServerDataResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                const uint8_t *aTlvs, uint8_t aTlvsLength);

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    ThreadError RegisterNetworkData(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength);

    ThreadError AddHasRoute(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute);
    ThreadError AddBorderRouter(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter);
    ThreadError AddNetworkData(uint8_t *aTlv, uint8_t aTlvLength);
    ThreadError AddPrefix(PrefixTlv &aTlv);

    int AllocateContext(void);
    ThreadError FreeContext(uint8_t aContextId);

    ThreadError ConfigureAddresses(void);
    ThreadError ConfigureAddress(PrefixTlv &aPrefix);

    ThreadError RemoveContext(uint8_t aContextId);
    ThreadError RemoveContext(PrefixTlv &aPrefix, uint8_t aContextId);

    ThreadError RemoveCommissioningData(void);

    ThreadError RemoveRloc(uint16_t aRloc16);
    ThreadError RemoveRloc(PrefixTlv &aPrefix, uint16_t aRloc16);
    ThreadError RemoveRloc(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute, uint16_t aRloc16);
    ThreadError RemoveRloc(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter, uint16_t aRloc16);

    ThreadError ExternalRouteLookup(uint8_t aDomainId, const Ip6::Address &destination,
                                    uint8_t *aPrefixMatch, uint16_t *aRloc16);
    ThreadError DefaultRouteLookup(PrefixTlv &aPrefix, uint16_t *aRloc16);
    void RlocLookup(uint16_t aRloc16, bool &aIn, bool &aStable, uint8_t *aTlvs, uint8_t aTlvsLength);
    bool IsStableUpdated(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength, uint8_t *aTlvsBase,
                         uint8_t aTlvsBaseLength);


    /**
     * Thread Specification Constants
     *
     */
    enum
    {
        kMinContextId        = 1,             ///< Minimum Context ID (0 is used for Mesh Local)
        kNumContextIds       = 15,            ///< Maximum Context ID
        kContextIdReuseDelay = 48 * 60 * 60,  ///< CONTEXT_ID_REUSE_DELAY (seconds)
        kStateUpdatePeriod   = 1000,          ///< State update period in milliseconds
    };
    uint16_t mContextUsed;
    uint32_t mContextLastUsed[kNumContextIds];
    uint32_t mContextIdReuseDelay;
    Timer mTimer;

    Coap::Resource  mServerData;
    uint8_t         mStableVersion;
    uint8_t         mVersion;

    Coap::Server   &mCoapServer;
    ThreadNetif    &mNetif;
};

/**
 * @}
 */

}  // namespace NetworkData
}  // namespace Thread

#endif  // NETWORK_DATA_LEADER_HPP_
