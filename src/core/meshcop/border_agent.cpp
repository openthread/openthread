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

#include "coap/coap_message.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/uri_paths.hpp"

#if OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE

namespace ot {
namespace MeshCoP {

class ForwardContext
{
public:
    /**
     * This constructor initializes a forward context.
     *
     * @param[in]   aBorderAgent    A reference to the border agent.
     * @param[in]   aHeader         A reference to the request header.
     * @param[in]   aPetition       Whether this request is a petition.
     * @param[in]   aSeparate       Whether this original request expects separate response.
     *
     */
    ForwardContext(BorderAgent &aBorderAgent, const Coap::Message &aMessage, bool aPetition, bool aSeparate)
        : mBorderAgent(aBorderAgent)
        , mMessageId(aMessage.GetMessageId())
        , mPetition(aPetition)
        , mSeparate(aSeparate)
        , mTokenLength(aMessage.GetTokenLength())
        , mType(aMessage.GetType())
    {
        memcpy(mToken, aMessage.GetToken(), mTokenLength);
    }

    /**
     * This method returns whether the request is a petition.
     *
     * @retval  true    This is a petition request.
     * @retval  false   This is not a petition request.
     *
     */
    bool IsPetition(void) const { return mPetition; }

    /**
     * This method returns the border agent sending this request.
     *
     * @returns A reference to the border agent sending this request.
     *
     */
    BorderAgent &GetBorderAgent(void) { return mBorderAgent; }

    /**
     * This method returns the message id of the original request.
     *
     * @returns A message id of the original request.
     *
     */
    uint16_t GetMessageId(void) const { return mMessageId; }

    /**
     * This method generate the response header according to the saved metadata.
     *
     * @param[out]  aHeader     A reference to the response header.
     * @param[in]   aCode       The response code to fill in the response header.
     *
     * @retval OT_ERROR_NONE     Successfully generated the response header.
     * @retval OT_ERROR_NO_BUFS  Insufficient message buffers available to generate the response header.
     *
     */
    otError ToHeader(Coap::Message &aMessage, uint8_t aCode)
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

private:
    enum
    {
        kMaxTokenLength = Coap::Message::kMaxTokenLength, ///< The max token size
    };

    BorderAgent &mBorderAgent;
    uint16_t     mMessageId;              ///< The CoAP Message ID of the original request.
    bool         mPetition : 1;           ///< Whether the forwarding request is leader petition.
    bool         mSeparate : 1;           ///< Whether the original request expects separate response.
    uint8_t      mTokenLength : 4;        ///< The CoAP Token Length of the original request.
    uint8_t      mType : 2;               ///< The CoAP Type of the original request.
    uint8_t      mToken[kMaxTokenLength]; ///< The CoAP Token of the original request.
};

static Coap::Message::Code CoapCodeFromError(otError aError)
{
    Coap::Message::Code code;

    switch (aError)
    {
    case OT_ERROR_NONE:
        code = Coap::kCodeChanged;
        break;

    case OT_ERROR_PARSE:
        code = Coap::kCodeBadRequest;
        ;
        break;

    default:
        code = Coap::kCodeInternalError;
        break;
    }

    return code;
}

static void SendErrorMessage(Coap::CoapSecure &aCoapSecure, ForwardContext &aForwardContext, otError aError)
{
    otError        error   = OT_ERROR_NONE;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = NewMeshCoPMessage(aCoapSecure)) != nullptr, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = aForwardContext.ToHeader(*message, CoapCodeFromError(aError)));
    SuccessOrExit(error = aCoapSecure.SendMessage(*message, aCoapSecure.GetMessageInfo()));

exit:
    FreeMessageOnError(message, error);
    LogError("send error CoAP message", error);
}

static void SendErrorMessage(Coap::CoapSecure &   aCoapSecure,
                             const Coap::Message &aRequest,
                             bool                 aSeparate,
                             otError              aError)
{
    otError        error   = OT_ERROR_NONE;
    Coap::Message *message = nullptr;

    VerifyOrExit((message = NewMeshCoPMessage(aCoapSecure)) != nullptr, error = OT_ERROR_NO_BUFS);

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

    SuccessOrExit(error = message->SetToken(aRequest.GetToken(), aRequest.GetTokenLength()));

    SuccessOrExit(error = aCoapSecure.SendMessage(*message, aCoapSecure.GetMessageInfo()));

exit:
    FreeMessageOnError(message, error);
    LogError("send error CoAP message", error);
}

void BorderAgent::HandleCoapResponse(void *               aContext,
                                     otMessage *          aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     otError              aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    ForwardContext &     forwardContext = *static_cast<ForwardContext *>(aContext);
    BorderAgent &        borderAgent    = forwardContext.GetBorderAgent();
    Instance &           instance       = borderAgent.GetInstance();
    const Coap::Message *response       = static_cast<const Coap::Message *>(aMessage);
    Coap::Message *      message        = nullptr;
    otError              error;

    SuccessOrExit(error = aResult);
    VerifyOrExit((message = NewMeshCoPMessage(instance.Get<Coap::CoapSecure>())) != nullptr, error = OT_ERROR_NO_BUFS);

    if (forwardContext.IsPetition() && response->GetCode() == Coap::kCodeChanged)
    {
        uint8_t state;

        SuccessOrExit(error = Tlv::FindUint8Tlv(*response, Tlv::kState, state));

        if (state == StateTlv::kAccept)
        {
            uint16_t sessionId;

            SuccessOrExit(error = Tlv::FindUint16Tlv(*response, Tlv::kCommissionerSessionId, sessionId));

            IgnoreError(instance.Get<Mle::MleRouter>().GetCommissionerAloc(borderAgent.mCommissionerAloc.GetAddress(),
                                                                           sessionId));
            instance.Get<ThreadNetif>().AddUnicastAddress(borderAgent.mCommissionerAloc);
            IgnoreError(instance.Get<Ip6::Udp>().AddReceiver(borderAgent.mUdpReceiver));
        }
    }

    SuccessOrExit(error = forwardContext.ToHeader(*message, response->GetCode()));

    if (response->GetLength() > response->GetOffset())
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    SuccessOrExit(error = borderAgent.ForwardToCommissioner(*message, *response));

exit:

    if (error != OT_ERROR_NONE)
    {
        FreeMessage(message);

        otLogWarnMeshCoP("Commissioner request[%hu] failed: %s", forwardContext.GetMessageId(),
                         otThreadErrorToString(error));

        SendErrorMessage(instance.Get<Coap::CoapSecure>(), forwardContext, error);
    }

    instance.HeapFree(&forwardContext);
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mCommissionerPetition>(void *               aContext,
                                                                     otMessage *          aMessage,
                                                                     const otMessageInfo *aMessageInfo)
{
    IgnoreError(static_cast<BorderAgent *>(aContext)->ForwardToLeader(
        *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo),
        UriPath::kLeaderPetition, true, true));
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mCommissionerKeepAlive>(void *               aContext,
                                                                      otMessage *          aMessage,
                                                                      const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgent *>(aContext)->HandleKeepAlive(*static_cast<Coap::Message *>(aMessage),
                                                          *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mRelayTransmit>(void *               aContext,
                                                              otMessage *          aMessage,
                                                              const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    static_cast<BorderAgent *>(aContext)->HandleRelayTransmit(*static_cast<Coap::Message *>(aMessage));
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mRelayReceive>(void *               aContext,
                                                             otMessage *          aMessage,
                                                             const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    static_cast<BorderAgent *>(aContext)->HandleRelayReceive(*static_cast<Coap::Message *>(aMessage));
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mProxyTransmit>(void *               aContext,
                                                              otMessage *          aMessage,
                                                              const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    static_cast<BorderAgent *>(aContext)->HandleProxyTransmit(*static_cast<Coap::Message *>(aMessage));
}

BorderAgent::BorderAgent(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCommissionerPetition(UriPath::kCommissionerPetition,
                            BorderAgent::HandleRequest<&BorderAgent::mCommissionerPetition>,
                            this)
    , mCommissionerKeepAlive(UriPath::kCommissionerKeepAlive,
                             BorderAgent::HandleRequest<&BorderAgent::mCommissionerKeepAlive>,
                             this)
    , mRelayTransmit(UriPath::kRelayTx, BorderAgent::HandleRequest<&BorderAgent::mRelayTransmit>, this)
    , mRelayReceive(UriPath::kRelayRx, BorderAgent::HandleRequest<&BorderAgent::mRelayReceive>, this)
    , mCommissionerGet(UriPath::kCommissionerGet, BorderAgent::HandleRequest<&BorderAgent::mCommissionerGet>, this)
    , mCommissionerSet(UriPath::kCommissionerSet, BorderAgent::HandleRequest<&BorderAgent::mCommissionerSet>, this)
    , mActiveGet(UriPath::kActiveGet, BorderAgent::HandleRequest<&BorderAgent::mActiveGet>, this)
    , mActiveSet(UriPath::kActiveSet, BorderAgent::HandleRequest<&BorderAgent::mActiveSet>, this)
    , mPendingGet(UriPath::kPendingGet, BorderAgent::HandleRequest<&BorderAgent::mPendingGet>, this)
    , mPendingSet(UriPath::kPendingSet, BorderAgent::HandleRequest<&BorderAgent::mPendingSet>, this)
    , mProxyTransmit(UriPath::kProxyTx, BorderAgent::HandleRequest<&BorderAgent::mProxyTransmit>, this)
    , mUdpReceiver(BorderAgent::HandleUdpReceive, this)
    , mTimer(aInstance, HandleTimeout, this)
    , mState(OT_BORDER_AGENT_STATE_STOPPED)
{
    mCommissionerAloc.InitAsThreadOriginRealmLocalScope();
}

void BorderAgent::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(aEvents.ContainsAny(kEventThreadRoleChanged | kEventCommissionerStateChanged), OT_NOOP);

#if OPENTHREAD_CONFIG_COMMISSIONER_ENABLE && OPENTHREAD_FTD
    VerifyOrExit(Get<MeshCoP::Commissioner>().IsDisabled(), OT_NOOP);
#endif

    if (Get<Mle::MleRouter>().IsAttached())
    {
        IgnoreError(Start());
    }
    else
    {
        IgnoreError(Stop());
    }

exit:
    return;
}

void BorderAgent::HandleProxyTransmit(const Coap::Message &aMessage)
{
    Message *        message = nullptr;
    Ip6::MessageInfo messageInfo;
    uint16_t         offset;
    otError          error;

    {
        UdpEncapsulationTlv tlv;

        SuccessOrExit(error = Tlv::FindTlvOffset(aMessage, Tlv::kUdpEncapsulation, offset));
        VerifyOrExit(aMessage.Read(offset, sizeof(tlv), &tlv) == sizeof(tlv), error = OT_ERROR_PARSE);

        VerifyOrExit((message = Get<Ip6::Udp>().NewMessage(0)) != nullptr, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = message->SetLength(tlv.GetUdpLength()));
        aMessage.CopyTo(offset + sizeof(tlv), 0, tlv.GetUdpLength(), *message);

        messageInfo.SetSockPort(tlv.GetSourcePort() != 0 ? tlv.GetSourcePort() : Get<Ip6::Udp>().GetEphemeralPort());
        messageInfo.SetSockAddr(mCommissionerAloc.GetAddress());
        messageInfo.SetPeerPort(tlv.GetDestinationPort());
    }

    SuccessOrExit(
        error = Tlv::FindTlv(aMessage, Tlv::kIPv6Address, messageInfo.GetPeerAddr().mFields.m8, sizeof(Ip6::Address)));

    SuccessOrExit(error = Get<Ip6::Udp>().SendDatagram(*message, messageInfo, Ip6::kProtoUdp));
    otLogInfoMeshCoP("Proxy transmit sent");

exit:
    FreeMessageOnError(message, error);
    LogError("send proxy stream", error);
}

bool BorderAgent::HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError        error;
    Coap::Message *message = nullptr;

    VerifyOrExit(aMessageInfo.GetSockAddr() == mCommissionerAloc.GetAddress(),
                 error = OT_ERROR_DESTINATION_ADDRESS_FILTERED);

    VerifyOrExit(aMessage.GetLength() > 0, error = OT_ERROR_NONE);

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::CoapSecure>())) != nullptr, error = OT_ERROR_NO_BUFS);

    message->InitAsNonConfirmablePost();
    SuccessOrExit(error = message->AppendUriPathOptions(UriPath::kProxyRx));
    SuccessOrExit(error = message->SetPayloadMarker());

    {
        UdpEncapsulationTlv tlv;
        uint16_t            offset;
        uint16_t            udpLength = aMessage.GetLength() - aMessage.GetOffset();

        tlv.Init();
        tlv.SetSourcePort(aMessageInfo.GetPeerPort());
        tlv.SetDestinationPort(aMessageInfo.GetSockPort());
        tlv.SetUdpLength(udpLength);
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

        offset = message->GetLength();
        SuccessOrExit(error = message->SetLength(offset + udpLength));
        aMessage.CopyTo(aMessage.GetOffset(), offset, udpLength, *message);
    }

    SuccessOrExit(error =
                      Tlv::AppendTlv(*message, Tlv::kIPv6Address, &aMessageInfo.GetPeerAddr(), sizeof(Ip6::Address)));

    SuccessOrExit(error = Get<Coap::CoapSecure>().SendMessage(*message, Get<Coap::CoapSecure>().GetMessageInfo()));

    otLogInfoMeshCoP("Sent to commissioner on %s", UriPath::kProxyRx);

exit:
    FreeMessageOnError(message, error);
    LogError("notify commissioner on ProxyRx (c/ur)", error);

    return error != OT_ERROR_DESTINATION_ADDRESS_FILTERED;
}

void BorderAgent::HandleRelayReceive(const Coap::Message &aMessage)
{
    Coap::Message *message = nullptr;
    otError        error;

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), error = OT_ERROR_DROP);
    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::CoapSecure>())) != nullptr, error = OT_ERROR_NO_BUFS);

    message->InitAsNonConfirmablePost();
    SuccessOrExit(error = message->AppendUriPathOptions(UriPath::kRelayRx));

    if (aMessage.GetLength() > aMessage.GetOffset())
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    SuccessOrExit(error = ForwardToCommissioner(*message, aMessage));
    otLogInfoMeshCoP("Sent to commissioner on %s", UriPath::kRelayRx);

exit:
    FreeMessageOnError(message, error);
}

otError BorderAgent::ForwardToCommissioner(Coap::Message &aForwardMessage, const Message &aMessage)
{
    otError  error  = OT_ERROR_NONE;
    uint16_t offset = 0;

    offset = aForwardMessage.GetLength();
    SuccessOrExit(error = aForwardMessage.SetLength(offset + aMessage.GetLength() - aMessage.GetOffset()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, aMessage.GetLength() - aMessage.GetOffset(), aForwardMessage);

    SuccessOrExit(error =
                      Get<Coap::CoapSecure>().SendMessage(aForwardMessage, Get<Coap::CoapSecure>().GetMessageInfo()));

    otLogInfoMeshCoP("Sent to commissioner");

exit:
    LogError("send to commissioner", error);
    return error;
}

void BorderAgent::HandleKeepAlive(const Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error;

    error = ForwardToLeader(aMessage, aMessageInfo, UriPath::kLeaderKeepAlive, false, true);

    if (error == OT_ERROR_NONE)
    {
        mTimer.Start(kKeepAliveTimeout);
    }
}

void BorderAgent::HandleRelayTransmit(const Coap::Message &aMessage)
{
    otError          error = OT_ERROR_NONE;
    uint16_t         joinerRouterRloc;
    Coap::Message *  message = nullptr;
    Ip6::MessageInfo messageInfo;
    uint16_t         offset = 0;

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), OT_NOOP);

    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kJoinerRouterLocator, joinerRouterRloc));

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->InitAsNonConfirmablePost(UriPath::kRelayTx));
    SuccessOrExit(error = message->SetPayloadMarker());

    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + aMessage.GetLength() - aMessage.GetOffset()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, aMessage.GetLength() - aMessage.GetOffset(), *message);

    messageInfo.SetSockPort(Tmf::kUdpPort);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerPort(Tmf::kUdpPort);
    messageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.GetPeerAddr().GetIid().SetLocator(joinerRouterRloc);

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("Sent to joiner router request on %s", UriPath::kRelayTx);

exit:
    FreeMessageOnError(message, error);
    LogError("send to joiner router request RelayTx (c/tx)", error);
}

otError BorderAgent::ForwardToLeader(const Coap::Message &   aMessage,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     const char *            aPath,
                                     bool                    aPetition,
                                     bool                    aSeparate)
{
    otError          error          = OT_ERROR_NONE;
    ForwardContext * forwardContext = nullptr;
    Ip6::MessageInfo messageInfo;
    Coap::Message *  message = nullptr;
    uint16_t         offset  = 0;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    if (aSeparate)
    {
        SuccessOrExit(error = Get<Coap::CoapSecure>().SendAck(aMessage, aMessageInfo));
    }

    forwardContext = static_cast<ForwardContext *>(GetInstance().HeapCAlloc(1, sizeof(ForwardContext)));
    VerifyOrExit(forwardContext != nullptr, error = OT_ERROR_NO_BUFS);

    forwardContext = new (forwardContext) ForwardContext(*this, aMessage, aPetition, aSeparate);

    SuccessOrExit(error = message->InitAsConfirmablePost(aPath));

    // Payload of c/cg may be empty
    if (aMessage.GetLength() - aMessage.GetOffset() > 0)
    {
        SuccessOrExit(error = message->SetPayloadMarker());
    }

    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + aMessage.GetLength() - aMessage.GetOffset()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, aMessage.GetLength() - aMessage.GetOffset(), *message);

    SuccessOrExit(error = Get<Mle::MleRouter>().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(Tmf::kUdpPort);
    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetSockPort(Tmf::kUdpPort);

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo, HandleCoapResponse, forwardContext));

    // HandleCoapResponse is responsible to free this forward context.
    forwardContext = nullptr;

    otLogInfoMeshCoP("Forwarded request to leader on %s", aPath);

exit:
    LogError("forward to leader", error);

    if (error != OT_ERROR_NONE)
    {
        if (forwardContext != nullptr)
        {
            GetInstance().HeapFree(forwardContext);
        }

        FreeMessage(message);
        SendErrorMessage(Get<Coap::CoapSecure>(), aMessage, aSeparate, error);
    }

    return error;
}

void BorderAgent::HandleConnected(bool aConnected)
{
    if (aConnected)
    {
        otLogInfoMeshCoP("Commissioner connected");
        SetState(OT_BORDER_AGENT_STATE_ACTIVE);
        mTimer.Start(kKeepAliveTimeout);
    }
    else
    {
        otLogInfoMeshCoP("Commissioner disconnected");
        Get<ThreadNetif>().RemoveUnicastAddress(mCommissionerAloc);
        SetState(OT_BORDER_AGENT_STATE_STARTED);
    }
}

otError BorderAgent::Start(void)
{
    otError           error;
    Coap::CoapSecure &coaps = Get<Coap::CoapSecure>();

    VerifyOrExit(mState == OT_BORDER_AGENT_STATE_STOPPED, error = OT_ERROR_ALREADY);

    SuccessOrExit(error = coaps.Start(kBorderAgentUdpPort));
    SuccessOrExit(error = coaps.SetPsk(Get<KeyManager>().GetPskc().m8, OT_PSKC_MAX_SIZE));
    coaps.SetConnectedCallback(HandleConnected, this);

    coaps.AddResource(mActiveGet);
    coaps.AddResource(mActiveSet);
    coaps.AddResource(mPendingGet);
    coaps.AddResource(mPendingSet);
    coaps.AddResource(mCommissionerPetition);
    coaps.AddResource(mCommissionerKeepAlive);
    coaps.AddResource(mCommissionerSet);
    coaps.AddResource(mCommissionerGet);
    coaps.AddResource(mProxyTransmit);
    coaps.AddResource(mRelayTransmit);

    Get<Tmf::TmfAgent>().AddResource(mRelayReceive);

    SetState(OT_BORDER_AGENT_STATE_STARTED);

exit:
    return error;
}

void BorderAgent::HandleTimeout(Timer &aTimer)
{
    aTimer.GetOwner<BorderAgent>().HandleTimeout();
}

void BorderAgent::HandleTimeout(void)
{
    if (Get<Coap::CoapSecure>().IsConnected())
    {
        Get<Coap::CoapSecure>().Disconnect();
        otLogWarnMeshCoP("Reset commissioner session");
    }
}

otError BorderAgent::Stop(void)
{
    otError           error = OT_ERROR_NONE;
    Coap::CoapSecure &coaps = Get<Coap::CoapSecure>();

    VerifyOrExit(mState != OT_BORDER_AGENT_STATE_STOPPED, error = OT_ERROR_ALREADY);

    mTimer.Stop();

    coaps.RemoveResource(mCommissionerPetition);
    coaps.RemoveResource(mCommissionerKeepAlive);
    coaps.RemoveResource(mCommissionerSet);
    coaps.RemoveResource(mCommissionerGet);
    coaps.RemoveResource(mActiveGet);
    coaps.RemoveResource(mActiveSet);
    coaps.RemoveResource(mPendingGet);
    coaps.RemoveResource(mPendingSet);
    coaps.RemoveResource(mProxyTransmit);
    coaps.RemoveResource(mRelayTransmit);

    Get<Tmf::TmfAgent>().RemoveResource(mRelayReceive);

    coaps.Stop();

    SetState(OT_BORDER_AGENT_STATE_STOPPED);

exit:
    return error;
}

void BorderAgent::SetState(otBorderAgentState aState)
{
    if (mState != aState)
    {
        mState = aState;
    }
}

void BorderAgent::ApplyMeshLocalPrefix(void)
{
    VerifyOrExit(mState == OT_BORDER_AGENT_STATE_ACTIVE, OT_NOOP);

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
