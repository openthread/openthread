/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes definitions for the BorderAgentProxy role.
 */

#ifndef BORDER_AGENT_PROXY_HPP_
#define BORDER_AGENT_PROXY_HPP_

#include <openthread-core-config.h>
#include <openthread/border_agent_proxy.h>

#include <coap/coap_client.hpp>
#include <coap/coap_server.hpp>

namespace Thread {

namespace MeshCoP {

class BorderAgentProxy
{
public:
    /**
     * This constructor initializes the BorderAgentProxy object.
     *
     * @param[in]  aThreadNetif  A reference to the Thread network interface.
     *
     */
    BorderAgentProxy(const Ip6::Address &aMeshLocal16, Coap::Server &aCoapServer, Coap::Client &aCoapClient);

    /**
     * This method starts the BorderAgentProxy service.
     *
     * @retval kThreadError_None  Successfully started the BorderAgentProxy service.
     *
     */
    ThreadError Start(otBorderAgentProxyStreamHandler aBorderAgentProxyStreamHandler, void *aContext);

    /**
     * This method stops the BorderAgentProxy service.
     *
     * @retval kThreadError_None  Successfully stopped the BorderAgentProxy service.
     *
     */
    ThreadError Stop(void);

    ThreadError Send(Message &aMessage, uint16_t aRloc, uint16_t aPort);

    bool IsEnabled(void) const;

private:

    static void HandleRelayReceive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                   const otMessageInfo *aMessageInfo);

    static void HandleResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                               const otMessageInfo *aMessageInfo, ThreadError aResult);

    void DelieverMessage(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);


    Coap::Resource mRelayReceive;
    otBorderAgentProxyStreamHandler mBorderAgentProxyStreamHandler;
    void *mContext;

    const Ip6::Address &mMeshLocal16;
    Coap::Server &mCoapServer;
    Coap::Client &mCoapClient;
};

}  // namespace MeshCoP
}  // namespace Thread

#endif  // BORDER_AGENT_PROXY_HPP_
