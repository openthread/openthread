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
 *   This file implements the BorderAgent service.
 */

#include "border_agent.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

#include "coap/coap_message.hpp"
#include "common/as_core_type.hpp"
#include "common/heap.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("BorderAgent");

namespace {
constexpr uint16_t kBorderAgentUdpPort = OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT; ///< UDP port of border agent service.
}

void BorderAgent::ForwardContext::Init(Instance            &aInstance,
                                       const Coap::Message &aMessage,
                                       bool                 aPetition,
                                       bool                 aSeparate)
{
    InstanceLocatorInit::Init(aInstance);
    mMessageId   = aMessage.GetMessageId();
    mPetition    = aPetition;
    mSeparate    = aSeparate;
    mType        = aMessage.GetType();
    mTokenLength = aMessage.GetTokenLength();
    memcpy(mToken, aMessage.GetToken(), mTokenLength);
}

Error BorderAgent::ForwardContext::ToHeader(Coap::Message &aMessage, uint8_t aCode)
{
    if ((mType == Coap::kTypeNonConfirmable) || mSeparate)
    {
        aMessage.Init(Coap::kTypeNonConfirmable, static_cast<Coap::Code>(aCode));
    }
    else
    {
        aMessage.Init(Coap::kTypeAck, static_cast<Coap::Code>(aCode));
    }

    if (!mSeparate)
    {
        aMessage.SetMessageId(mMessageId);
    }

    return aMessage.SetToken(mToken, mTokenLength);
}

Coap::Message::Code BorderAgent::CoapCodeFromError(Error aError)
{
    Coap::Message::Code code;

    switch (aError)
    {
    case kErrorNone:
        code = Coap::kCodeChanged;
        break;

    case kErrorParse:
        code = Coap::kCodeBadRequest;
        break;

    default:
        code = Coap::kCodeInternalError;
        break;
    }

    return code;
}

void BorderAgent::SendErrorMessage(ForwardContext &aForwardContext, Error aError)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = Get<Tmf::SecureAgent>().NewPriorityMessage()) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = aForwardContext.ToHeader(*message, CoapCodeFromError(aError)));
    SuccessOrExit(error = Get<Tmf::SecureAgent>().SendMessage(*message, Get<Tmf::SecureAgent>().GetMessageInfo()));

exit:
    FreeMessageOnError(message, error);
    LogError("send error CoAP message", error);
}

void BorderAgent::SendErrorMessage(const Coap::Message &aRequest, bool aSeparate, Error aError)
{
    Error          error   = kErrorNone;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = Get<Tmf::SecureAgent>().NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    if (aRequest.IsNonConfirmable() || aSeparate)
    {
        message->Init(Coap::kTypeNonConfirmable, CoapCodeFromError(aError));
    }
    else
    {
        message->Init(Coap::kTypeAck, CoapCodeFromError(aError));
    }

    if (!aSeparate)
    {
        message->SetMessageId(aRequest.GetMessageId());
    }

    SuccessOrExit(error = message->SetTokenFromMessage(aRequest));

    SuccessOrExit(error = Get<Tmf::SecureAgent>().SendMessage(*message, Get<Tmf::SecureAgent>().GetMessageInfo()));

exit:
    FreeMessageOnError(message, error);
    LogError("send error CoAP message", error);
}

void BorderAgent::HandleCoapResponse(void                *aContext,
                                     otMessage           *aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     Error                aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    ForwardContext &forwardContext = *static_cast<ForwardContext *>(aContext);

    forwardContext.Get<BorderAgent>().HandleCoapResponse(forwardContext, AsCoapMessagePtr(aMessage), aResult);
}

void BorderAgent::HandleCoapResponse(ForwardContext &aForwardContext, const Coap::Message *aResponse, Error aResult)
{
    Coap::Message *message = nullptr;
    Error          error;

    SuccessOrExit(error = aResult);
    VerifyOrExit((message = Get<Tmf::SecureAgent>().NewPriorityMessage()) != nullptr, error = kErrorNoBufs);

    if (aForwardContext.IsPetition() && aResponse->GetCode() == Coap::kCodeChanged)
    {
        uint8_t state;

        SuccessOrExit(error = Tlv::Find<StateTlv>(*aResponse, state));

        if (state == StateTlv::kAccept)
        {
            uint16_t sessionId;

            SuccessOrExit(error = Tlv::Find<CommissionerSessionIdTlv>(*aResponse, sessionId));

            IgnoreError(Get<Mle::MleRouter>().GetCommissionerAloc(mCommissionerAloc.GetAddress(), sessionId));
            Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);
            IgnoreError(Get<Ip6::Udp>().AddReceiver(mUdpReceiver));

            LogInfo("commissioner accepted: session ID=%d, ALOC=%s", sessionId,
                    mCommissionerAloc.GetAddress().ToString().AsCString());
        }
    }

    SuccessOrExit(error = aForwardContext.ToHeader(*message, aResponse->GetCode()));

    if (aResponse->GetLength() > aResponse->GetOffset())
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    SuccessOrExit(error = ForwardToCommissioner(*message, *aResponse));

exit:

    if (error != kErrorNone)
    {
        FreeMessage(message);

        LogWarn("Commissioner request[%u] failed: %s", aForwardContext.GetMessageId(), ErrorToString(error));

        SendErrorMessage(aForwardContext, error);
    }

    Heap::Free(&aForwardContext);
}

BorderAgent::BorderAgent(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mUdpReceiver(BorderAgent::HandleUdpReceive, this)
    , mTimer(aInstance)
    , mState(kStateStopped)
    , mUdpProxyPort(0)
#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
    , mIdInitialized(false)
#endif
{
    mCommissionerAloc.InitAsThreadOriginRealmLocalScope();
}

#if OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE
Error BorderAgent::GetId(uint8_t *aId, uint16_t &aLength)
{
    Error error = kErrorNone;

    static_assert(sizeof(mId) == kIdLength, "Invalid Border Agent ID size");

    VerifyOrExit(aLength >= sizeof(mId), error = kErrorInvalidArgs);
    VerifyOrExit(!mIdInitialized, error = kErrorNone);

    if (Get<Settings>().Read(mId) != kErrorNone)
    {
        Random::NonCrypto::FillBuffer(mId.GetId(), sizeof(mId));
        SuccessOrExit(error = Get<Settings>().Save(mId));
    }

    mIdInitialized = true;

exit:
    if (error == kErrorNone)
    {
        memcpy(aId, mId.GetId(), sizeof(mId));
        aLength = static_cast<uint16_t>(sizeof(mId));
    }
    return error;
}
#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ID_ENABLE

void BorderAgent::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(aEvents.ContainsAny(kEventThreadRoleChanged | kEventCommissionerStateChanged));

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    VerifyOrExit(Get<Commissioner>().IsDisabled());
#endif

    if (Get<Mle::MleRouter>().IsAttached())
    {
        Start();
    }
    else
    {
        Stop();
    }

exit:
    return;
}

template <> void BorderAgent::HandleTmf<kUriProxyTx>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error                     error   = kErrorNone;
    Message                  *message = nullptr;
    Ip6::MessageInfo          messageInfo;
    uint16_t                  offset;
    uint16_t                  length;
    UdpEncapsulationTlvHeader udpEncapHeader;

    VerifyOrExit(mState != kStateStopped);

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Tlv::kUdpEncapsulation, offset, length));

    SuccessOrExit(error = aMessage.Read(offset, udpEncapHeader));
    offset += sizeof(UdpEncapsulationTlvHeader);
    length -= sizeof(UdpEncapsulationTlvHeader);

    VerifyOrExit(udpEncapHeader.GetSourcePort() > 0 && udpEncapHeader.GetDestinationPort() > 0, error = kErrorDrop);

    VerifyOrExit((message = Get<Ip6::Udp>().NewMessage(0)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, offset, length));

    messageInfo.SetSockPort(udpEncapHeader.GetSourcePort());
    messageInfo.SetSockAddr(mCommissionerAloc.GetAddress());
    messageInfo.SetPeerPort(udpEncapHeader.GetDestinationPort());

    SuccessOrExit(error = Tlv::Find<Ip6AddressTlv>(aMessage, messageInfo.GetPeerAddr()));

    SuccessOrExit(error = Get<Ip6::Udp>().SendDatagram(*message, messageInfo, Ip6::kProtoUdp));
    mUdpProxyPort = udpEncapHeader.GetSourcePort();

    LogInfo("Proxy transmit sent to %s", messageInfo.GetPeerAddr().ToString().AsCString());

exit:
    FreeMessageOnError(message, error);
    LogError("send proxy stream", error);
}

bool BorderAgent::HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error          error;
    Coap::Message *message = nullptr;

    if (aMessageInfo.GetSockAddr() != mCommissionerAloc.GetAddress())
    {
        LogDebg("Filtered out message for commissioner: dest %s != %s (ALOC)",
                aMessageInfo.GetSockAddr().ToString().AsCString(),
                mCommissionerAloc.GetAddress().ToString().AsCString());
        ExitNow(error = kErrorDestinationAddressFiltered);
    }

    VerifyOrExit(aMessage.GetLength() > 0, error = kErrorNone);

    message = Get<Tmf::SecureAgent>().NewPriorityNonConfirmablePostMessage(kUriProxyRx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    {
        ExtendedTlv               extTlv;
        UdpEncapsulationTlvHeader udpEncapHeader;
        uint16_t                  udpLength = aMessage.GetLength() - aMessage.GetOffset();

        extTlv.SetType(Tlv::kUdpEncapsulation);
        extTlv.SetLength(sizeof(UdpEncapsulationTlvHeader) + udpLength);
        SuccessOrExit(error = message->Append(extTlv));

        udpEncapHeader.SetSourcePort(aMessageInfo.GetPeerPort());
        udpEncapHeader.SetDestinationPort(aMessageInfo.GetSockPort());
        SuccessOrExit(error = message->Append(udpEncapHeader));
        SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, aMessage.GetOffset(), udpLength));
    }

    SuccessOrExit(error = Tlv::Append<Ip6AddressTlv>(*message, aMessageInfo.GetPeerAddr()));

    SuccessOrExit(error = Get<Tmf::SecureAgent>().SendMessage(*message, Get<Tmf::SecureAgent>().GetMessageInfo()));

    LogInfo("Sent to commissioner on ProxyRx (c/ur)");

exit:
    FreeMessageOnError(message, error);
    if (error != kErrorDestinationAddressFiltered)
    {
        LogError("Notify commissioner on ProxyRx (c/ur)", error);
    }

    return error != kErrorDestinationAddressFiltered;
}

template <> void BorderAgent::HandleTmf<kUriRelayRx>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Coap::Message *message = nullptr;
    Error          error   = kErrorNone;

    VerifyOrExit(mState != kStateStopped);

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), error = kErrorDrop);

    message = Get<Tmf::SecureAgent>().NewPriorityNonConfirmablePostMessage(kUriRelayRx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = ForwardToCommissioner(*message, aMessage));
    LogInfo("Sent to commissioner on RelayRx (c/rx)");

exit:
    FreeMessageOnError(message, error);
}

Error BorderAgent::ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage)
{
    Error error;

    SuccessOrExit(error = aForwardMessage.AppendBytesFromMessage(aMessage, aMessage.GetOffset(),
                                                                 aMessage.GetLength() - aMessage.GetOffset()));
    SuccessOrExit(error =
                      Get<Tmf::SecureAgent>().SendMessage(aForwardMessage, Get<Tmf::SecureAgent>().GetMessageInfo()));

    LogInfo("Sent to commissioner");

exit:
    LogError("send to commissioner", error);
    return error;
}

template <>
void BorderAgent::HandleTmf<kUriCommissionerPetition>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriLeaderPetition));
}

template <>
void BorderAgent::HandleTmf<kUriCommissionerGet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriCommissionerGet));
}

template <>
void BorderAgent::HandleTmf<kUriCommissionerSet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriCommissionerSet));
}

template <> void BorderAgent::HandleTmf<kUriActiveGet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriActiveGet));
}

template <> void BorderAgent::HandleTmf<kUriActiveSet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriActiveSet));
}

template <> void BorderAgent::HandleTmf<kUriPendingGet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriPendingGet));
}

template <> void BorderAgent::HandleTmf<kUriPendingSet>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    IgnoreError(ForwardToLeader(aMessage, aMessageInfo, kUriPendingSet));
}

template <>
void BorderAgent::HandleTmf<kUriCommissionerKeepAlive>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    VerifyOrExit(mState != kStateStopped);

    SuccessOrExit(ForwardToLeader(aMessage, aMessageInfo, kUriLeaderKeepAlive));
    mTimer.Start(kKeepAliveTimeout);

exit:
    return;
}

template <> void BorderAgent::HandleTmf<kUriRelayTx>(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error            error = kErrorNone;
    uint16_t         joinerRouterRloc;
    Coap::Message   *message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());

    VerifyOrExit(mState != kStateStopped);

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest());

    SuccessOrExit(error = Tlv::Find<JoinerRouterLocatorTlv>(aMessage, joinerRouterRloc));

    message = Get<Tmf::Agent>().NewPriorityNonConfirmablePostMessage(kUriRelayTx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, aMessage.GetOffset(),
                                                          aMessage.GetLength() - aMessage.GetOffset()));

    messageInfo.SetSockAddrToRlocPeerAddrTo(joinerRouterRloc);
    messageInfo.SetSockPortToTmf();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent to joiner router request on RelayTx (c/tx)");

exit:
    FreeMessageOnError(message, error);
    LogError("send to joiner router request RelayTx (c/tx)", error);
}

Error BorderAgent::ForwardToLeader(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, Uri aUri)
{
    Error            error          = kErrorNone;
    ForwardContext  *forwardContext = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    Coap::Message   *message  = nullptr;
    bool             petition = false;
    bool             separate = false;

    VerifyOrExit(mState != kStateStopped);

    switch (aUri)
    {
    case kUriLeaderPetition:
        petition = true;
        separate = true;
        break;
    case kUriLeaderKeepAlive:
        separate = true;
        break;
    default:
        break;
    }

    if (separate)
    {
        SuccessOrExit(error = Get<Tmf::SecureAgent>().SendAck(aMessage, aMessageInfo));
    }

    forwardContext = static_cast<ForwardContext *>(Heap::CAlloc(1, sizeof(ForwardContext)));
    VerifyOrExit(forwardContext != nullptr, error = kErrorNoBufs);

    forwardContext->Init(GetInstance(), aMessage, petition, separate);

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(aUri);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->AppendBytesFromMessage(aMessage, aMessage.GetOffset(),
                                                          aMessage.GetLength() - aMessage.GetOffset()));

    SuccessOrExit(error = messageInfo.SetSockAddrToRlocPeerAddrToLeaderAloc());
    messageInfo.SetSockPortToTmf();

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo, HandleCoapResponse, forwardContext));

    // HandleCoapResponse is responsible to free this forward context.
    forwardContext = nullptr;

    LogInfo("Forwarded request to leader on %s", PathForUri(aUri));

exit:
    LogError("forward to leader", error);

    if (error != kErrorNone)
    {
        if (forwardContext != nullptr)
        {
            Heap::Free(forwardContext);
        }

        FreeMessage(message);
        SendErrorMessage(aMessage, separate, error);
    }

    return error;
}

void BorderAgent::HandleConnected(bool aConnected, void *aContext)
{
    static_cast<BorderAgent *>(aContext)->HandleConnected(aConnected);
}

void BorderAgent::HandleConnected(bool aConnected)
{
    if (aConnected)
    {
        LogInfo("Commissioner connected");
        mState = kStateActive;
        mTimer.Start(kKeepAliveTimeout);
    }
    else
    {
        LogInfo("Commissioner disconnected");
        IgnoreError(Get<Ip6::Udp>().RemoveReceiver(mUdpReceiver));
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);
        mState        = kStateStarted;
        mUdpProxyPort = 0;
    }
}

uint16_t BorderAgent::GetUdpPort(void) const { return Get<Tmf::SecureAgent>().GetUdpPort(); }

void BorderAgent::Start(void)
{
    Error error;
    Pskc  pskc;

    VerifyOrExit(mState == kStateStopped, error = kErrorNone);

    Get<KeyManager>().GetPskc(pskc);
    SuccessOrExit(error = Get<Tmf::SecureAgent>().Start(kBorderAgentUdpPort));
    SuccessOrExit(error = Get<Tmf::SecureAgent>().SetPsk(pskc.m8, Pskc::kSize));

    pskc.Clear();
    Get<Tmf::SecureAgent>().SetConnectedCallback(HandleConnected, this);

    mState        = kStateStarted;
    mUdpProxyPort = 0;

    LogInfo("Border Agent start listening on port %u", GetUdpPort());

exit:
    if (error != kErrorNone)
    {
        LogWarn("failed to start Border Agent on port %d: %s", kBorderAgentUdpPort, ErrorToString(error));
    }
}

void BorderAgent::HandleTimeout(void)
{
    if (Get<Tmf::SecureAgent>().IsConnected())
    {
        Get<Tmf::SecureAgent>().Disconnect();
        LogWarn("Reset commissioner session");
    }
}

void BorderAgent::Stop(void)
{
    VerifyOrExit(mState != kStateStopped);

    mTimer.Stop();
    Get<Tmf::SecureAgent>().Stop();

    mState        = kStateStopped;
    mUdpProxyPort = 0;

    LogInfo("Border Agent stopped");

exit:
    return;
}

void BorderAgent::ApplyMeshLocalPrefix(void)
{
    VerifyOrExit(mState == kStateActive);

    if (Get<ThreadNetif>().HasUnicastAddress(mCommissionerAloc))
    {
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);
        mCommissionerAloc.GetAddress().SetPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
        Get<ThreadNetif>().AddUnicastAddress(mCommissionerAloc);
    }

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
