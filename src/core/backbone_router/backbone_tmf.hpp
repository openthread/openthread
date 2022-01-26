/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for Backbone TMF functionality.
 */

#ifndef OT_CORE_THREAD_BACKBONE_TMF_HPP_
#define OT_CORE_THREAD_BACKBONE_TMF_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include "coap/coap.hpp"

namespace ot {
namespace BackboneRouter {

constexpr uint16_t kBackboneUdpPort = 61631; ///< Backbone TMF UDP Port

/**
 * This class implements functionality of the Backbone TMF agent.
 *
 */
class BackboneTmfAgent : public Coap::Coap
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aInstance      A reference to the OpenThread instance.
     *
     */
    explicit BackboneTmfAgent(Instance &aInstance)
        : Coap::Coap(aInstance)
    {
        SetInterceptor(&Filter, this);
    }

    /**
     * This method starts the Backbone TMF agent.
     *
     * @retval kErrorNone    Successfully started the CoAP service.
     * @retval kErrorFailed  Failed to start the Backbone TMF agent.
     *
     */
    Error Start(void);

    /**
     * This method returns whether @p aMessageInfo meets Backbone Thread Management Framework Addressing Rules.
     *
     * @retval true   Thread Management Framework Addressing Rules are met.
     * @retval false  Thread Management Framework Addressing Rules are not met.
     *
     */
    bool IsBackboneTmfMessage(const Ip6::MessageInfo &aMessageInfo) const;

    /**
     * This method subscribes the Backbone TMF socket to a given IPv6 multicast group on the Backbone network.
     *
     * @param[in] aAddress  The IPv6 multicast group address.
     *
     */
    void SubscribeMulticast(const Ip6::Address &aAddress);

    /**
     * This method unsubscribes the Backbone TMF socket from a given IPv6 multicast group on the Backbone network.
     *
     * @param[in] aAddress  The IPv6 multicast group address.
     *
     */
    void UnsubscribeMulticast(const Ip6::Address &aAddress);

private:
    void         LogError(const char *aText, const Ip6::Address &aAddress, Error aError) const;
    static Error Filter(const ot::Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext);
};

} // namespace BackboneRouter
} // namespace ot

#endif //  OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#endif //  OT_CORE_THREAD_BACKBONE_TMF_HPP_
