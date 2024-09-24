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
 *   This file includes definitions for a MeshCoP Leader.
 */

#ifndef MESHCOP_LEADER_HPP_
#define MESHCOP_LEADER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD

#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"
#include "thread/tmf.hpp"

namespace ot {
namespace MeshCoP {

class Leader : public InstanceLocator, private NonCopyable
{
    friend class Tmf::Agent;

public:
    /**
     * Initializes the Leader object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     */
    explicit Leader(Instance &aInstance);

    /**
     * Sets the session ID.
     *
     * @param[in] aSessionId  The session ID to use.
     */
    void SetSessionId(uint16_t aSessionId) { mSessionId = aSessionId; }

    /**
     * Sends a MGMT_DATASET_CHANGED message to commissioner.
     *
     * @param[in]  aAddress   The IPv6 address of destination.
     */
    void SendDatasetChanged(const Ip6::Address &aAddress);

    /**
     * Sets minimal delay timer.
     *
     * @param[in]  aDelayTimerMinimal The value of minimal delay timer (in ms).
     *
     * @retval  kErrorNone         Successfully set the minimal delay timer.
     * @retval  kErrorInvalidArgs  If @p aDelayTimerMinimal is not valid.
     */
    Error SetDelayTimerMinimal(uint32_t aDelayTimerMinimal);

    /**
     * Gets minimal delay timer.
     *
     * @retval the minimal delay timer (in ms).
     */
    uint32_t GetDelayTimerMinimal(void) const { return mDelayTimerMinimal; }

    /**
     * Sets empty Commissioner Data TLV in the Thread Network Data.
     */
    void SetEmptyCommissionerData(void);

private:
    static constexpr uint32_t kTimeoutLeaderPetition = 50; // TIMEOUT_LEAD_PET (seconds)

    OT_TOOL_PACKED_BEGIN
    class CommissioningData
    {
    public:
        void    Init(uint16_t aBorderAgentRloc16, uint16_t aSessionId);
        uint8_t GetLength(void) const;

    private:
        BorderAgentLocatorTlv    mBorderAgentLocatorTlv;
        CommissionerSessionIdTlv mSessionIdTlv;
        SteeringDataTlv          mSteeringDataTlv;
    } OT_TOOL_PACKED_END;

    void HandleTimer(void);

    template <Uri kUri> void HandleTmf(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void SendPetitionResponse(const Coap::Message    &aRequest,
                              const Ip6::MessageInfo &aMessageInfo,
                              StateTlv::State         aState);

    void SendKeepAliveResponse(const Coap::Message    &aRequest,
                               const Ip6::MessageInfo &aMessageInfo,
                               StateTlv::State         aState);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

    void ResignCommissioner(void);

    using LeaderTimer = TimerMilliIn<Leader, &Leader::HandleTimer>;

    LeaderTimer mTimer;

    uint32_t mDelayTimerMinimal;

    CommissionerIdTlv::StringType mCommissionerId;
    uint16_t                      mSessionId;
};

DeclareTmfHandler(Leader, kUriLeaderPetition);
DeclareTmfHandler(Leader, kUriLeaderKeepAlive);

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD

#endif // MESHCOP_LEADER_HPP_
