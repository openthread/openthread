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
#include <coap/secure_coap_client.hpp>
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
     * This method starts the Joiner service.
     *
     * @param[in]  aPSKd             A pointer to the PSKd.
     * @param[in]  aProvisioningUrl  A pointer to the Provisioning URL (may be NULL).
     *
     * @retval kThreadError_None  Successfully started the Joiner service.
     *
     */
    ThreadError Start(const char *aPSKd, const char *aProvisioningUrl);

    /**
     * This method stops the Joiner service.
     *
     * @retval kThreadError_None  Successfully stopped the Joiner service.
     *
     */
    ThreadError Stop(void);

private:
    enum
    {
        kConfigExtAddressDelay = 100,  ///< milliseconds
    };

    static void HandleDiscoverResult(otActiveScanResult *aResult, void *aContext);
    void HandleDiscoverResult(otActiveScanResult *aResult);

    static void HandleTimer(void *aContext);
    void HandleTimer(void);

    void Close(void);

    static void HandleSecureCoapClientConnect(void *aContext);

    void SendJoinerFinalize(void);
    static void HandleJoinerFinalizeResponse(void *aContext, otCoapHeader *aHeader,
                                             otMessage aMessage, ThreadError result);
    void HandleJoinerFinalizeResponse(Coap::Header *aHeader, Message *aMessage, ThreadError result);

    static void HandleJoinerEntrust(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                    const otMessageInfo *aMessageInfo);
    void HandleJoinerEntrust(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void SendJoinerEntrustResponse(const Coap::Header &aRequestHeader, const Ip6::MessageInfo &aRequestInfo);

    uint8_t mJoinerRouterChannel;
    uint16_t mJoinerRouterPanId;
    uint16_t mJoinerUdpPort;
    Mac::ExtAddress mJoinerRouter;

    Timer mTimer;
    Coap::Resource mJoinerEntrust;
    Coap::Server &mCoapServer;
    Coap::SecureClient &mSecureCoapClient;
    ThreadNetif &mNetif;
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // JOINER_HPP_
