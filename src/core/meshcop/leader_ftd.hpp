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

#include <coap/coap_client.hpp>
#include <coap/coap_server.hpp>
#include <common/timer.hpp>
#include <meshcop/tlvs.hpp>
#include <net/udp6.hpp>
#include <thread/mle.hpp>

namespace Thread {
namespace MeshCoP {

OT_TOOL_PACKED_BEGIN
class CommissioningData
{
public:
    uint8_t GetLength(void) const {
        return sizeof(Tlv) + mBorderAgentLocator.GetLength() +
               sizeof(Tlv) + mCommissionerSessionId.GetLength() +
               sizeof(Tlv) + mSteeringData.GetLength();
    }

    BorderAgentLocatorTlv mBorderAgentLocator;
    CommissionerSessionIdTlv mCommissionerSessionId;
    SteeringDataTlv mSteeringData;
} OT_TOOL_PACKED_END;

class Leader
{
public:
    /**
     * This constructor initializes the Leader object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    Leader(ThreadNetif &aThreadNetif);

    /**
     * This method sends a MGMT_DATASET_CHANGED message to commissioner.
     *
     * @param[in]  aAddress   The IPv6 address of destination.
     *
     * @retval kThreadError_None    Successfully send MGMT_DATASET_CHANGED message.
     * @retval kThreadError_NoBufs  Insufficient buffers to generate a MGMT_DATASET_CHANGED message.
     *
     */
    ThreadError SendDatasetChanged(const Ip6::Address &aAddress);

private:
    enum
    {
        kTimeoutLeaderPetition = 50, ///< TIMEOUT_LEAD_PET (seconds)
    };

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    static void HandlePetition(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                               const otMessageInfo *aMessageInfo);
    void HandlePetition(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError SendPetitionResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                     StateTlv::State aState);

    static void HandleKeepAlive(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                const otMessageInfo *aMessageInfo);
    void HandleKeepAlive(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    ThreadError SendKeepAliveResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo,
                                      StateTlv::State aState);

    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);

    void ResignCommissioner(void);

    Coap::Resource mPetition;
    Coap::Resource mKeepAlive;
    Coap::Server &mCoapServer;
    Coap::Client &mCoapClient;
    NetworkData::Leader &mNetworkData;

    Timer mTimer;

    CommissionerIdTlv mCommissionerId;
    uint16_t mSessionId;
    ThreadNetif &mNetif;
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // MESHCOP_LEADER_HPP_
