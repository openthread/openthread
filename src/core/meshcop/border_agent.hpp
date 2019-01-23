/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes definitions for the BorderAgent role.
 */

#ifndef BORDER_AGENT_HPP_
#define BORDER_AGENT_HPP_

#include "openthread-core-config.h"

#include <openthread/border_agent.h>

#include "coap/coap.hpp"
#include "common/locator.hpp"
#include "net/udp6.hpp"

namespace ot {

class ThreadNetif;

namespace MeshCoP {

class BorderAgent : public InstanceLocator
{
public:
    /**
     * This constructor initializes the BorderAgent object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit BorderAgent(Instance &aInstance);

    /**
     * This method starts the Border Agent service.
     *
     * @retval OT_ERROR_NONE  Successfully started the Border Agent service.
     *
     */
    otError Start(void);

    /**
     * This method stops the Border Agent service.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the Border Agent service.
     *
     */
    otError Stop(void);

    /**
     * This method gets the state of the Border Agent service.
     *
     * @returns The state of the the Border Agent service.
     *
     */
    otBorderAgentState GetState(void) const { return mState; }

private:
    static void HandleConnected(bool aConnected, void *aContext)
    {
        static_cast<BorderAgent *>(aContext)->HandleConnected(aConnected);
    }
    void HandleConnected(bool aConnected);

    template <Coap::Resource BorderAgent::*aResource>
    static void HandleRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
    {
        static_cast<BorderAgent *>(aContext)->ForwardToLeader(
            *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
            (static_cast<BorderAgent *>(aContext)->*aResource).GetUriPath(), false, false);
    }

    static void HandleTimeout(Timer &aTimer);
    void        HandleTimeout(void);

    static void HandleCoapResponse(void *               aContext,
                                   otMessage *          aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   otError              aResult);

    otError     ForwardToLeader(const Coap::Message &   aMessage,
                                const Ip6::MessageInfo &aMessageInfo,
                                const char *            aPath,
                                bool                    aPetition,
                                bool                    aSeparate);
    otError     ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage);
    void        HandleKeepAlive(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void        HandleRelayTransmit(const Coap::Message &aMessage);
    void        HandleRelayReceive(const Coap::Message &aMessage);
    void        HandleProxyTransmit(const Coap::Message &aMessage);
    static bool HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo)
    {
        return static_cast<BorderAgent *>(aContext)->HandleUdpReceive(
            *static_cast<const Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
    }
    bool HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    void SetState(otBorderAgentState aState);

    enum
    {
        kBorderAgentUdpPort = 49191,     ///< UDP port of border agent service.
        kKeepAliveTimeout   = 50 * 1000, ///< Timeout to reject a commissioner.
        kRestartDelay       = 1 * 1000,  ///< Delay to restart border agent service.
    };

    Ip6::MessageInfo mMessageInfo;

    Coap::Resource mCommissionerPetition;
    Coap::Resource mCommissionerKeepAlive;
    Coap::Resource mRelayTransmit;
    Coap::Resource mRelayReceive;
    Coap::Resource mCommissionerGet;
    Coap::Resource mCommissionerSet;
    Coap::Resource mActiveGet;
    Coap::Resource mActiveSet;
    Coap::Resource mPendingGet;
    Coap::Resource mPendingSet;
    Coap::Resource mProxyTransmit;

    Ip6::UdpReceiver         mUdpReceiver; ///< The UDP receiver to receive packets from external commissioner
    Ip6::NetifUnicastAddress mCommissionerAloc;

    TimerMilli         mTimer;
    otBorderAgentState mState;
};

} // namespace MeshCoP
} // namespace ot

#endif // BORDER_AGENT_HPP_
