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

#include <stdint.h>

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
class Leader: public LeaderBase
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
    ThreadError SetContextIdReuseDelay(uint32_t aDelay);

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

private:
    static void HandleServerData(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                 const otMessageInfo *aMessageInfo);
    void HandleServerData(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    ThreadError RegisterNetworkData(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength);

    ThreadError AddHasRoute(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute);
    ThreadError AddBorderRouter(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter);
    ThreadError AddNetworkData(uint8_t *aTlv, uint8_t aTlvLength);
    ThreadError AddPrefix(PrefixTlv &aTlv);

    int AllocateContext(void);
    ThreadError FreeContext(uint8_t aContextId);

    ThreadError RemoveContext(uint8_t aContextId);
    ThreadError RemoveContext(PrefixTlv &aPrefix, uint8_t aContextId);

    ThreadError RemoveCommissioningData(void);

    ThreadError RemoveRloc(uint16_t aRloc16);
    ThreadError RemoveRloc(PrefixTlv &aPrefix, uint16_t aRloc16);
    ThreadError RemoveRloc(PrefixTlv &aPrefix, HasRouteTlv &aHasRoute, uint16_t aRloc16);
    ThreadError RemoveRloc(PrefixTlv &aPrefix, BorderRouterTlv &aBorderRouter, uint16_t aRloc16);

    void RlocLookup(uint16_t aRloc16, bool &aIn, bool &aStable, uint8_t *aTlvs, uint8_t aTlvsLength);
    bool IsStableUpdated(uint16_t aRloc16, uint8_t *aTlvs, uint8_t aTlvsLength, uint8_t *aTlvsBase,
                         uint8_t aTlvsBaseLength);

    static void HandleCommissioningSet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                       const otMessageInfo *aMessageInfo);
    void HandleCommissioningSet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleCommissioningGet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                       const otMessageInfo *aMessageInfo);
    void HandleCommissioningGet(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void SendCommissioningGetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                      uint8_t *aTlvs, uint8_t aLength);
    void SendCommissioningSetResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                      MeshCoP::StateTlv::State aState);

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

    Coap::Resource mCommissioningDataGet;
    Coap::Resource mCommissioningDataSet;
};

/**
 * @}
 */

}  // namespace NetworkData
}  // namespace Thread

#endif  // NETWORK_DATA_LEADER_FTD_HPP_
