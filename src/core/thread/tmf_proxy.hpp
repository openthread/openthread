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
 *   This file includes definitions for TMF proxy.
 */

#ifndef TMF_PROXY_HPP_
#define TMF_PROXY_HPP_

#include <openthread/tmf_proxy.h>

#include "openthread-core-config.h"
#include "coap/coap.hpp"

namespace ot {

class TmfProxy
{
public:
    /**
     * This constructor initializes the TMF proxy.
     *
     * @param[in]   aMeshLocal16    A reference to the Mesh Local Routing Locator.
     * @param[in]   aCoap           A reference to the Management CoAP Service.
     *
     */
    TmfProxy(const Ip6::Address &aMeshLocal16, Coap::Coap &aCoap);

    /**
     * This method enables the TMF proxy service.
     *
     * @retval OT_ERROR_NONE            Successfully started the service.
     * @retval OT_ERROR_ALREADY         The service already started.
     *
     */
    otError Start(otTmfProxyStreamHandler aStreamHandler, void *aContext);

    /**
     * This method disables the TMF proxy service.
     *
     * @retval OT_ERROR_NONE            Successfully stopped the TMF proxy service.
     * @retval OT_ERROR_ALREADY         The service already stopped.
     *
     */
    otError Stop(void);

    /**
     * This method sends the message into the thread network.
     *
     * @param[in]   aMessage    A reference to the message to send.
     * @param[in]   aLocator    The destination's RLOC16 or ALOC16.
     * @param[in]   aPort       The destination port.
     *
     * @retval OT_ERROR_NONE             Successfully send the message.
     * @retval OT_ERROR_INVALID_STATE    Border agent proxy is not started.
     *
     * @warning No matter the call success or fail, the message is freed.
     *
     */
    otError Send(Message &aMessage, uint16_t aLocator, uint16_t aPort);

    /**
     * This method indicates whether or not the TMF proxy service is enabled.
     *
     * @retval  TRUE    If the service is enabled.
     * @retval  FALSE   If the service is disabled.
     *
     */
    bool IsEnabled(void) const;

private:

    static void HandleRelayReceive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                   const otMessageInfo *aMessageInfo);

    static void HandleResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                               const otMessageInfo *aMessageInfo, otError aResult);

    void DeliverMessage(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo);


    Coap::Resource mRelayReceive;
    otTmfProxyStreamHandler mStreamHandler;
    void *mContext;

    const Ip6::Address &mMeshLocal16;
    Coap::Coap &mCoap;
};

}  // namespace ot

#endif  // TMF_PROXY_HPP_
