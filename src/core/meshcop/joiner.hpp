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
 *  This file includes definitions for the Joiner role.
 */

#ifndef JOINER_HPP_
#define JOINER_HPP_

#include <openthread-types.h>

#include <coap/coap_header.hpp>
#include <coap/coap_server.hpp>
#include <common/message.hpp>
#include <net/udp6.hpp>
#include <meshcop/dtls.hpp>
#include <thread/meshcop_tlvs.hpp>

namespace Thread {

class ThreadNetif;

namespace MeshCoP {

class Joiner
{
public:
    /**
     * This constructor initializes the Joiner object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    Joiner(ThreadNetif &aThreadNetif);

    /**
     * This method starts the Joiner service.  Requires that PSKd has been set.
     *
     * @retval kThreadError_None  Successfully started the Joiner service.
     *
     */
    ThreadError Start(void);

    /**
     * This method stops the Joiner service.
     *
     * @retval kThreadError_None  Successfully stopped the Joiner service.
     *
     */
    ThreadError Stop(void);

    /**
     * Sets the PSKd or Joining Device Credential for this Joiner.
     *
     * @param[in]  aPSKd             A pointer to the PSKd.
     *
     * @retval kThreadError_None  Successfully started the Joiner service.
     *
     */
    ThreadError SetCredential(const char *aPSKd);

    /**
     * Sets the optional Provisioning URL for this Joiner.
     *
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
     *
     * @retval kThreadError_None  Successfully started the Joiner service.
     *
     */
    ThreadError SetProvisioningUrl(const char *aProvisioningUrl);

    /**
     * This method starts the Joiner service.
     *
     * @param[in]  aPSKd             A pointer to the PSKd.
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
     *
     * @retval kThreadError_None  Successfully started the Joiner service.
     *
     */
    ThreadError Start(const char *aPSKd, const char *aProvisioningUrl);

private:
    static void HandleDiscoverResult(otActiveScanResult *aResult, void *aContext);
    void HandleDiscoverResult(otActiveScanResult *aResult);

    static void HandleDtlsReceive(void *aContext, uint8_t *aBuf, uint16_t aLength);
    void HandleDtlsReceive(uint8_t *aBuf, uint16_t aLength);

    static ThreadError HandleDtlsSend(void *aContext, const uint8_t *aBuf, uint16_t aLength);
    ThreadError HandleDtlsSend(const uint8_t *aBuf, uint16_t aLength);

    static void HandleUdpReceive(void *aContext, otMessage aMessage, const otMessageInfo *aMessageInfo);
    void HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    static void HandleUdpTransmit(void *aContext);
    void HandleUdpTransmit(void);

    static void HandleJoinerEntrust(void *aContext, Coap::Header &aHeader,
                                    Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void HandleJoinerEntrust(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void Close(void);
    void ReceiveJoinerFinalizeResponse(uint8_t *buf, uint16_t length);
    void SendJoinerFinalize(void);

    uint8_t mJoinerRouterChannel;
    uint16_t mJoinerRouterPanId;
    uint16_t mJoinerUdpPort;
    Mac::ExtAddress mJoinerRouter;
    Message *mTransmitMessage;
    Ip6::UdpSocket mSocket;

    Tasklet mTransmitTask;
    Coap::Resource mJoinerEntrust;
    ThreadNetif &mNetif;
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // JOINER_HPP_
