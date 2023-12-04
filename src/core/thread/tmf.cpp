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
 *   This file implements Thread Management Framework (TMF) functionalities.
 */

#include "thread/tmf.hpp"

#include "common/locator_getters.hpp"
#include "net/ip6_types.hpp"

namespace ot {
namespace Tmf {

//----------------------------------------------------------------------------------------------------------------------
// MessageInfo

void MessageInfo::SetSockAddrToRloc(void) { SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16()); }

Error MessageInfo::SetSockAddrToRlocPeerAddrToLeaderAloc(void)
{
    SetSockAddrToRloc();
    return Get<Mle::MleRouter>().GetLeaderAloc(GetPeerAddr());
}

Error MessageInfo::SetSockAddrToRlocPeerAddrToLeaderRloc(void)
{
    SetSockAddrToRloc();
    return Get<Mle::MleRouter>().GetLeaderAddress(GetPeerAddr());
}

void MessageInfo::SetSockAddrToRlocPeerAddrToRealmLocalAllRoutersMulticast(void)
{
    SetSockAddrToRloc();
    GetPeerAddr().SetToRealmLocalAllRoutersMulticast();
}

void MessageInfo::SetSockAddrToRlocPeerAddrTo(uint16_t aRloc16)
{
    SetSockAddrToRloc();
    SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    GetPeerAddr().GetIid().SetLocator(aRloc16);
}

void MessageInfo::SetSockAddrToRlocPeerAddrTo(const Ip6::Address &aPeerAddress)
{
    SetSockAddrToRloc();
    SetPeerAddr(aPeerAddress);
}

//----------------------------------------------------------------------------------------------------------------------
// Agent

Agent::Agent(Instance &aInstance)
    : Coap::Coap(aInstance)
{
    SetInterceptor(&Filter, this);
    SetResourceHandler(&HandleResource);
}

Error Agent::Start(void) { return Coap::Start(kUdpPort, Ip6::kNetifThread); }

template <> void Agent::HandleTmf<kUriRelayRx>(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

#if (OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE)
    Get<MeshCoP::Commissioner>().HandleTmf<kUriRelayRx>(aMessage, aMessageInfo);
#endif
#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
    Get<MeshCoP::BorderAgent>().HandleTmf<kUriRelayRx>(aMessage, aMessageInfo);
#endif
}

bool Agent::HandleResource(CoapBase               &aCoapBase,
                           const char             *aUriPath,
                           Message                &aMessage,
                           const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<Agent &>(aCoapBase).HandleResource(aUriPath, aMessage, aMessageInfo);
}

bool Agent::HandleResource(const char *aUriPath, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    bool didHandle = true;
    Uri  uri       = UriFromPath(aUriPath);

#define Case(kUri, Type)                                     \
    case kUri:                                               \
        Get<Type>().HandleTmf<kUri>(aMessage, aMessageInfo); \
        break

    switch (uri)
    {
        Case(kUriAddressError, AddressResolver);
        Case(kUriEnergyScan, EnergyScanServer);
        Case(kUriActiveGet, MeshCoP::ActiveDatasetManager);
        Case(kUriPendingGet, MeshCoP::PendingDatasetManager);
        Case(kUriPanIdQuery, PanIdQueryServer);

#if OPENTHREAD_FTD
        Case(kUriAddressQuery, AddressResolver);
        Case(kUriAddressNotify, AddressResolver);
        Case(kUriAddressSolicit, Mle::MleRouter);
        Case(kUriAddressRelease, Mle::MleRouter);
        Case(kUriActiveSet, MeshCoP::ActiveDatasetManager);
        Case(kUriPendingSet, MeshCoP::PendingDatasetManager);
        Case(kUriLeaderPetition, MeshCoP::Leader);
        Case(kUriLeaderKeepAlive, MeshCoP::Leader);
        Case(kUriServerData, NetworkData::Leader);
        Case(kUriCommissionerGet, NetworkData::Leader);
        Case(kUriCommissionerSet, NetworkData::Leader);
        Case(kUriAnnounceBegin, AnnounceBeginServer);
        Case(kUriRelayTx, MeshCoP::JoinerRouter);
#endif

#if OPENTHREAD_CONFIG_JOINER_ENABLE
        Case(kUriJoinerEntrust, MeshCoP::Joiner);
#endif

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
        Case(kUriPanIdConflict, PanIdQueryClient);
        Case(kUriEnergyReport, EnergyScanClient);
        Case(kUriDatasetChanged, MeshCoP::Commissioner);
        // kUriRelayRx is handled below
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE)
        Case(kUriRelayRx, Agent);
#endif

#if OPENTHREAD_CONFIG_DUA_ENABLE || (OPENTHREAD_FTD && OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE)
        Case(kUriDuaRegistrationNotify, DuaManager);
#endif

#if OPENTHREAD_CONFIG_TMF_ANYCAST_LOCATOR_ENABLE
        Case(kUriAnycastLocate, AnycastLocator);
#endif

        Case(kUriDiagnosticGetRequest, NetworkDiagnostic::Server);
        Case(kUriDiagnosticGetQuery, NetworkDiagnostic::Server);
        Case(kUriDiagnosticReset, NetworkDiagnostic::Server);

#if OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE
        Case(kUriDiagnosticGetAnswer, NetworkDiagnostic::Client);
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
        Case(kUriMlr, BackboneRouter::Manager);
#endif
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_DUA_NDPROXYING_ENABLE
        Case(kUriDuaRegistrationRequest, BackboneRouter::Manager);
#endif
#endif

    default:
        didHandle = false;
        break;
    }

#undef Case

    return didHandle;
}

Error Agent::Filter(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo, void *aContext)
{
    OT_UNUSED_VARIABLE(aMessage);

    return static_cast<Agent *>(aContext)->IsTmfMessage(aMessageInfo.GetPeerAddr(), aMessageInfo.GetSockAddr(),
                                                        aMessageInfo.GetSockPort())
               ? kErrorNone
               : kErrorNotTmf;
}

bool Agent::IsTmfMessage(const Ip6::Address &aSourceAddress, const Ip6::Address &aDestAddress, uint16_t aDestPort) const
{
    bool isTmf = false;

    VerifyOrExit(aDestPort == kUdpPort);

    if (aSourceAddress.IsLinkLocal())
    {
        isTmf = aDestAddress.IsLinkLocal() || aDestAddress.IsLinkLocalMulticast();
        ExitNow();
    }

    VerifyOrExit(Get<Mle::Mle>().IsMeshLocalAddress(aSourceAddress));
    VerifyOrExit(Get<Mle::Mle>().IsMeshLocalAddress(aDestAddress) || aDestAddress.IsLinkLocalMulticast() ||
                 aDestAddress.IsRealmLocalMulticast());

    isTmf = true;

exit:
    return isTmf;
}

uint8_t Agent::PriorityToDscp(Message::Priority aPriority)
{
    uint8_t dscp = Ip6::kDscpTmfNormalPriority;

    switch (aPriority)
    {
    case Message::kPriorityNet:
        dscp = Ip6::kDscpTmfNetPriority;
        break;

    case Message::kPriorityHigh:
    case Message::kPriorityNormal:
        break;

    case Message::kPriorityLow:
        dscp = Ip6::kDscpTmfLowPriority;
        break;
    }

    return dscp;
}

Message::Priority Agent::DscpToPriority(uint8_t aDscp)
{
    Message::Priority priority = Message::kPriorityNet;

    // If the sender does not use TMF specific DSCP value, we use
    // `kPriorityNet`. This ensures that senders that do not use the
    // new value (older firmware) experience the same behavior as
    // before where all TMF message were treated as `kPriorityNet`.

    switch (aDscp)
    {
    case Ip6::kDscpTmfNetPriority:
    default:
        break;
    case Ip6::kDscpTmfNormalPriority:
        priority = Message::kPriorityNormal;
        break;
    case Ip6::kDscpTmfLowPriority:
        priority = Message::kPriorityLow;
        break;
    }

    return priority;
}

#if OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

SecureAgent::SecureAgent(Instance &aInstance)
    : Coap::CoapSecure(aInstance)
{
    SetResourceHandler(&HandleResource);
}

bool SecureAgent::HandleResource(CoapBase               &aCoapBase,
                                 const char             *aUriPath,
                                 Message                &aMessage,
                                 const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<SecureAgent &>(aCoapBase).HandleResource(aUriPath, aMessage, aMessageInfo);
}

bool SecureAgent::HandleResource(const char *aUriPath, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
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
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_COMMISSIONER_ENABLE
        Case(kUriJoinerFinalize, MeshCoP::Commissioner);
#endif

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
        Case(kUriCommissionerPetition, MeshCoP::BorderAgent);
        Case(kUriCommissionerKeepAlive, MeshCoP::BorderAgent);
        Case(kUriRelayTx, MeshCoP::BorderAgent);
        Case(kUriCommissionerGet, MeshCoP::BorderAgent);
        Case(kUriCommissionerSet, MeshCoP::BorderAgent);
        Case(kUriActiveGet, MeshCoP::BorderAgent);
        Case(kUriActiveSet, MeshCoP::BorderAgent);
        Case(kUriPendingGet, MeshCoP::BorderAgent);
        Case(kUriPendingSet, MeshCoP::BorderAgent);
        Case(kUriProxyTx, MeshCoP::BorderAgent);
#endif

    default:
        didHandle = false;
        break;
    }

#undef Case

    return didHandle;
}

#endif // OPENTHREAD_CONFIG_SECURE_TRANSPORT_ENABLE

} // namespace Tmf
} // namespace ot
