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
 *   This file includes definitions for manipulating Thread Network Data managed by the Thread Leader.
 */

#ifndef NETWORK_DATA_LEADER_FTD_HPP_
#define NETWORK_DATA_LEADER_FTD_HPP_

#include "openthread-core-config.h"

#include "utils/wrap_stdint.h"

#include "coap/coap.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data.hpp"

namespace ot {

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
class Leader : public LeaderBase
{
public:
    /**
     * This enumeration defines the match mode constants to compare two RLOC16 values.
     *
     */
    enum MatchMode
    {
        kMatchModeRloc16,   ///< Perform exact RLOC16 match.
        kMatchModeRouterId, ///< Perform Router ID match (match the router and any of its children).
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Leader(Instance &aInstance);

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
     * This method increments the Thread Network Data version.
     *
     */
    void IncrementVersion(void);

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
    otError SetContextIdReuseDelay(uint32_t aDelay);

    /**
     * This method removes Network Data entries matching with a given RLOC16.
     *
     * @param[in]  aRloc16    A RLOC16 value.
     * @param[in]  aMatchMode A match mode (@sa MatchMode).
     *
     */
    void RemoveBorderRouter(uint16_t aRloc16, MatchMode aMatchMode);

    /**
     * This method sends a Server Data Notification message to the Leader indicating an invalid RLOC16.
     *
     * @param[in]  aRloc16  The invalid RLOC16 to notify.
     *
     * @retval OT_ERROR_NONE     Successfully enqueued the notification message.
     * @retval OT_ERROR_NO_BUFS  Insufficient message buffers to generate the notification message.
     *
     */
    otError SendServerDataNotification(uint16_t aRloc16);

    /**
     * This method synchronizes internal 6LoWPAN Context ID Set with recently obtained Thread Network Data.
     *
     * Note that this method should be called only by the Leader once after reset.
     *
     */
    void UpdateContextsAfterReset(void);

#if OPENTHREAD_ENABLE_SERVICE
    /**
     * This method scans network data for given service ID and returns pointer to the respective TLV, if present.
     *
     * @param aServiceId Service ID to look for.
     * @return Pointer to the Service TLV for given Service ID, or NULL if not present.
     *
     */
    ServiceTlv *FindServiceById(uint8_t aServiceId);
#endif

private:
    static void HandleServerData(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleServerData(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(Timer &aTimer);
    void        HandleTimer(void);

    otError RegisterNetworkData(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength);

    otError AddHasRoute(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute);
    otError AddBorderRouter(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter);
    otError AddNetworkData(uint8_t *aTlvs, uint8_t aTlvsLength, uint8_t *aOldTlvs, uint8_t aOldTlvsLength);
    otError AddPrefix(PrefixTlv &aTlv);
#if OPENTHREAD_ENABLE_SERVICE
    otError AddServer(ServiceTlv &aService, ServerTlv &aServer, uint8_t *aOldTlvs, uint8_t aOldTlvsLength);
    otError AddService(ServiceTlv &aTlv, uint8_t *aOldTlvs, uint8_t aOldTlvsLength);
#endif

    int     AllocateContext(void);
    otError FreeContext(uint8_t aContextId);
    void    StartContextReuseTimer(uint8_t aContextId);
    void    StopContextReuseTimer(uint8_t aContextId);

    otError RemoveContext(uint8_t aContextId);
    otError RemoveContext(PrefixTlv &aPrefix, uint8_t aContextId);

    otError RemoveCommissioningData(void);

    otError RemoveRloc(uint16_t aRloc16, MatchMode aMatchMode);
    otError RemoveRloc(PrefixTlv &aPrefix, uint16_t aRloc16, MatchMode aMatchMode);
#if OPENTHREAD_ENABLE_SERVICE
    otError RemoveRloc(ServiceTlv &service, uint16_t aRloc16, MatchMode aMatchMode);
#endif
    otError RemoveRloc(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute, uint16_t aRloc16, MatchMode aMatchMode);
    otError RemoveRloc(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter, uint16_t aRloc16, MatchMode aMatchMode);

    static bool RlocMatch(uint16_t aFirstRloc16, uint16_t aSecondRloc16, MatchMode aMatchMode);

    otError RlocLookup(uint16_t  aRloc16,
                       bool &    aIn,
                       bool &    aStable,
                       uint8_t * aTlvs,
                       uint8_t   aTlvsLength,
                       MatchMode aMatchMode,
                       bool      aAllowOtherEntries = true);

    bool IsStableUpdated(uint8_t *aTlvs, uint8_t aTlvsLength, uint8_t *aTlvsBase, uint8_t aTlvsBaseLength);

    static void HandleCommissioningSet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleCommissioningSet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleCommissioningGet(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleCommissioningGet(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void SendCommissioningGetResponse(const Coap::Message &   aRequest,
                                      const Ip6::MessageInfo &aMessageInfo,
                                      uint8_t *               aTlvs,
                                      uint8_t                 aLength);
    void SendCommissioningSetResponse(const Coap::Message &    aRequest,
                                      const Ip6::MessageInfo & aMessageInfo,
                                      MeshCoP::StateTlv::State aState);

    /**
     * Thread Specification Constants.
     *
     */
    enum
    {
        kMinContextId        = 1,            ///< Minimum Context ID (0 is used for Mesh Local)
        kNumContextIds       = 15,           ///< Maximum Context ID
        kContextIdReuseDelay = 48 * 60 * 60, ///< CONTEXT_ID_REUSE_DELAY (seconds)
        kStateUpdatePeriod   = 60 * 1000,    ///< State update period in milliseconds
    };

    uint16_t   mContextUsed;
    uint32_t   mContextLastUsed[kNumContextIds];
    uint32_t   mContextIdReuseDelay;
    TimerMilli mTimer;

    Coap::Resource mServerData;

    Coap::Resource mCommissioningDataGet;
    Coap::Resource mCommissioningDataSet;
};

/**
 * @}
 */

} // namespace NetworkData
} // namespace ot

#endif // NETWORK_DATA_LEADER_FTD_HPP_
