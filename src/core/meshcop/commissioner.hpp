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
 *   This file includes definitions for the Commissioner role.
 */

#ifndef COMMISSIONER_HPP_
#define COMMISSIONER_HPP_

#include <coap/coap_server.hpp>
#include <common/timer.hpp>
#include <meshcop/energy_scan_client.hpp>
#include <meshcop/panid_query_client.hpp>
#include <net/udp6.hpp>
#include <thread/mle.hpp>

namespace Thread {

class ThreadNetif;

namespace MeshCoP {

class Commissioner
{
public:
    /**
     * This constructor initializes the Commissioner object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    Commissioner(ThreadNetif &aThreadNetif);

    /**
     * This method starts the Commissioner service.
     *
     * @retval kThreadError_None  Successfully started the Commissioner service.
     *
     */
    ThreadError Start(void);

    /**
     * This method stops the Commissioner service.
     *
     * @retval kThreadError_None  Successfully stopped the Commissioner service.
     *
     */
    ThreadError Stop(void);

    /**
     * This method adds an entry to the list of Joiner devices the
     * Commissioner service will authorize onto the network.  Currently
     * this API only supports one Joiner with any IID.
     *
     * @param[in]  aPSKd             A pointer to the PSKd.
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
     *
     * @retval kThreadError_None  Successfully started the Commissioner service.
     *
     */
    ThreadError AddJoiner(const char *aPSKd, const char *aProvisioningUrl);

    /**
     * This method returns the Commissioner Session ID.
     *
     * @returns The Commissioner Session ID.
     *
     */
    uint16_t GetSessionId(void) const;

    EnergyScanClient mEnergyScan;
    PanIdQueryClient mPanIdQuery;


    /**
     * This method sends MGMT_COMMISSIONER_GET.
     *
     * @param[in]  aTlvs        A pointer to Commissioning Data TLVs.
     * @param[in]  aLength      The length of requested TLVs in bytes.
     *
     * @retval kThreadError_None     Send MGMT_COMMISSIONER_GET successfully.
     * @retval kThreadError_Failed   Send MGMT_COMMISSIONER_GET fail.
     *
     */
    ThreadError SendMgmtCommissionerGetRequest(const uint8_t *aTlvs, uint8_t aLength);

    /**
     * This method sends MGMT_COMMISSIONER_SET.
      *
     * @param[in]  aDataset     A reference to Commissioning Data.
     * @param[in]  aTlvs        A pointer to user specific Commissioning Data TLVs.
     * @param[in]  aLength      The length of user specific TLVs in bytes.
     *
     * @retval kThreadError_None     Send MGMT_COMMISSIONER_SET successfully.
     * @retval kThreadError_Failed   Send MGMT_COMMISSIONER_SET fail.
     *
     */
    ThreadError SendMgmtCommissionerSetRequest(const otCommissioningDataset &aDataset,
                                               const uint8_t *aTlvs, uint8_t aLength);

private:
    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    static void HandleRelayReceive(void *aContext, Coap::Header &aHeader,
                                   Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void HandleRelayReceive(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength);

    static ThreadError HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength);
    ThreadError HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength);

    static void HandleUdpTransmit(void *aContext);
    void HandleUdpTransmit(void);

    void ReceiveJoinerFinalize(uint8_t *buf, uint16_t length);
    void SendJoinFinalizeResponse(const Coap::Header &aRequestHeader, StateTlv::State aState);

    ThreadError SendPetition(void);
    ThreadError SendKeepAlive(void);

    enum
    {
        kStateDisabled = 0,
        kStatePetition = 1,
        kStateActive = 2,
    };

    uint8_t mState;

    uint8_t mJoinerIid[8];
    uint16_t mJoinerPort;
    uint16_t mJoinerRloc;

    uint16_t mSessionId;
    Message *mTransmitMessage;
    Timer mTimer;
    Tasklet mTransmitTask;
    bool mSendKek;

    Ip6::UdpSocket mSocket;
    uint8_t mCoapToken[2];
    uint16_t mCoapMessageId;

    Coap::Resource mRelayReceive;
    ThreadNetif &mNetif;

    bool mIsSendMgmtCommRequest;
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // COMMISSIONER_HPP_
