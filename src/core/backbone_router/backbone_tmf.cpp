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
 *   This file implements Backbone Thread Management Framework (TMF) functionalities.
 */

#include "backbone_tmf.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include "common/locator_getters.hpp"
#include "common/log.hpp"

namespace ot {
namespace BackboneRouter {

RegisterLogModule("Bbr");

BackboneTmfAgent::BackboneTmfAgent(Instance &aInstance)
    : Coap::Coap(aInstance)
{
    SetInterceptor(&Filter, this);
    SetResourceHandler(&HandleResource);
}

Error BackboneTmfAgent::Start(void)
{
    Error error = kErrorNone;

    SuccessOrExit(error = Coap::Start(kBackboneUdpPort, Ip6::kNetifBackbone));
    LogInfo("Start listening on port %u", kBackboneUdpPort);
    SubscribeMulticast(Get<Local>().GetAllNetworkBackboneRoutersAddress());

exit:
    return error;
}

bool BackboneTmfAgent::HandleResource(CoapBase               &aCoapBase,
                                      const char             *aUriPath,
                                      ot::Coap::Message      &aMessage,
                                      const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<BackboneTmfAgent &>(aCoapBase).HandleResource(aUriPath, aMessage, aMessageInfo);
}

bool BackboneTmfAgent::HandleResource(const char             *aUriPath,
                                      ot::Coap::Message      &aMessage,
                                      const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    bool didHandle = true;
    Uri  uri       = UriFromPath(aUriPath);

#define Case(kUri, Type)                                     \
    case kUri:                                               \
        Get<Type>().HandleTmf<kUri>(aMessage, aMessageInfo); \
        break

    switch (uri)
    {
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
        Case(kUriBackboneQuery, Manager);
        Case(kUriBackboneAnswer, Manager);
#endif

    default:
        didHandle = false;
        break;
    }

#undef Case

    return didHandle;
}

Error BackboneTmfAgent::Filter(const ot::Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext)
{
    OT_UNUSED_VARIABLE(aMessage);

    return static_cast<BackboneTmfAgent *>(aContext)->IsBackboneTmfMessage(aMessageInfo) ? kErrorNone : kErrorNotTmf;
}

bool BackboneTmfAgent::IsBackboneTmfMessage(const Ip6::MessageInfo &aMessageInfo) const
{
    const Ip6::Address &dst = aMessageInfo.GetSockAddr();
    const Ip6::Address &src = aMessageInfo.GetPeerAddr();

    // A Backbone TMF message must comply with following rules:
    // The destination must be one of:
    //     1. All Network BBRs (Link-Local scope)
    //     2. All Domain BBRs (Link-Local scope)
    //     3. A Backbone Link-Local address
    // The source must be a Backbone Link-local address.
    return (Get<BackboneRouter::Local>().IsEnabled() && src.IsLinkLocal() &&
            (dst.IsLinkLocal() || dst == Get<BackboneRouter::Local>().GetAllNetworkBackboneRoutersAddress() ||
             dst == Get<BackboneRouter::Local>().GetAllDomainBackboneRoutersAddress()));
}

void BackboneTmfAgent::SubscribeMulticast(const Ip6::Address &aAddress)
{
    Error error = mSocket.JoinNetifMulticastGroup(Ip6::kNetifBackbone, aAddress);

    LogError("Backbone TMF subscribes", aAddress, error);
}

void BackboneTmfAgent::UnsubscribeMulticast(const Ip6::Address &aAddress)
{
    Error error = mSocket.LeaveNetifMulticastGroup(Ip6::kNetifBackbone, aAddress);

    LogError("Backbone TMF unsubscribes", aAddress, error);
}

void BackboneTmfAgent::LogError(const char *aText, const Ip6::Address &aAddress, Error aError) const
{
    OT_UNUSED_VARIABLE(aText);
    OT_UNUSED_VARIABLE(aAddress);

    if (aError == kErrorNone)
    {
        LogInfo("%s %s: %s", aText, aAddress.ToString().AsCString(), ErrorToString(aError));
    }
    else
    {
        LogWarn("%s %s: %s", aText, aAddress.ToString().AsCString(), ErrorToString(aError));
    }
}

} // namespace BackboneRouter
} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
