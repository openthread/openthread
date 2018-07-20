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

#include "coap/coap.hpp"
#include "common/locator.hpp"

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
     * This method starts the BorderAgent service.
     *
     * @retval OT_ERROR_NONE  Successfully started the BorderAgent service.
     *
     */
    otError Start(void);

    /**
     * This method stops the BorderAgent service.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the BorderAgent service.
     *
     */
    otError Stop(void);

private:
    static void HandleConnected(bool aConnected, void *aContext)
    {
        static_cast<BorderAgent *>(aContext)->HandleConnected(aConnected);
    }
    void    HandleConnected(bool aConnected);
    otError StartCoaps(void);

    template <Coap::Resource BorderAgent::*aResource>
    static void HandleRequest(void *               aContext,
                              otCoapHeader *       aHeader,
                              otMessage *          aMessage,
                              const otMessageInfo *aMessageInfo)
    {
        static_cast<BorderAgent *>(aContext)->ForwardToLeader(
            *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
            *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
            (static_cast<BorderAgent *>(aContext)->*aResource).GetUriPath(), false);
    }

    static void HandleTimeout(Timer &aTimer);
    void        HandleTimeout(void);

    static void HandleCoapResponse(void *               aContext,
                                   otCoapHeader *       aHeader,
                                   otMessage *          aMessage,
                                   const otMessageInfo *aMessageInfo,
                                   otError              aResult);

    void    SendErrorMessage(const Coap::Header &aHeader);
    otError ForwardToLeader(const Coap::Header &    aHeader,
                            const Message &         aMessage,
                            const Ip6::MessageInfo &aMessageInfo,
                            const char *            aPath,
                            bool                    aSeparate);
    otError ForwardToCommissioner(const Coap::Header &aHeader, const Message &aMessage);
    void    HandleKeepAlive(const Coap::Header &aHeader, const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void    HandleRelayTransmit(const Coap::Header &aHeader, const Message &aMessage);
    void    HandleRelayReceive(const Coap::Header &aHeader, const Message &aMessage);

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

    TimerMilli mTimer;
    bool       mIsStarted;
};

} // namespace MeshCoP
} // namespace ot

#endif // BORDER_AGENT_HPP_
