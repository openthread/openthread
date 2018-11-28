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

#define WPP_NAME "border_agent.tmh"

#include "border_agent.hpp"

#include "coap/coap_header.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"
#include "common/owner-locator.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/thread_uri_paths.hpp"

#if OPENTHREAD_ENABLE_BORDER_AGENT

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
    ForwardContext(BorderAgent &aBorderAgent, const Coap::Header &aHeader, bool aPetition, bool aSeparate)
        : mBorderAgent(aBorderAgent)
        , mMessageId(aHeader.GetMessageId())
        , mPetition(aPetition)
        , mSeparate(aSeparate)
        , mTokenLength(aHeader.GetTokenLength())
        , mType(aHeader.GetType() >> kTypeOffset)
    {
        memcpy(mToken, aHeader.GetToken(), mTokenLength);
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
     * @param[out]  aHeader     A refernce to the response header.
     * @param[in]   aCode       The response code to fill in the response header.
     *
     */
    void ToHeader(Coap::Header &aHeader, Coap::Header::Code aCode)
    {
        if (mType == (OT_COAP_TYPE_NON_CONFIRMABLE >> kTypeOffset) || mSeparate)
        {
            aHeader.Init(OT_COAP_TYPE_NON_CONFIRMABLE, aCode);
        }
        else
        {
            aHeader.Init(OT_COAP_TYPE_ACKNOWLEDGMENT, aCode);
        }

        aHeader.SetMessageId(mSeparate ? 0 : mMessageId);
        aHeader.SetToken(mToken, mTokenLength);
    }

private:
    enum
    {
        kTypeOffset = 4, ///< The type offset in the first byte of coap header
    };

    BorderAgent &mBorderAgent;
    uint16_t     mMessageId;                       ///< The CoAP Message ID of the original request.
    bool         mPetition : 1;                    ///< Whether the forwarding request is leader petition.
    bool         mSeparate : 1;                    ///< Whether the original request expects separate response.
    uint8_t      mTokenLength : 4;                 ///< The CoAP Token Length of the original request.
    uint8_t      mType : 2;                        ///< The CoAP Type of the original request.
    uint8_t      mToken[OT_COAP_MAX_TOKEN_LENGTH]; ///< The CoAP Token of the original request.
};

static Coap::Header::Code CoapCodeFromError(otError aError)
{
    Coap::Header::Code code;

    switch (aError)
    {
    case OT_ERROR_NONE:
        code = OT_COAP_CODE_CHANGED;
        break;

    case OT_ERROR_PARSE:
        code = OT_COAP_CODE_BAD_REQUEST;
        break;

    default:
        code = OT_COAP_CODE_INTERNAL_ERROR;
        break;
    }

    return code;
}

void BorderAgent::SendErrorMessage(const Coap::Header &aHeader)
{
    otError      error   = OT_ERROR_NONE;
    Message *    message = NULL;
    ThreadNetif &netif   = GetNetif();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoapSecure(), aHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = netif.GetCoapSecure().SendMessage(*message, netif.GetCoapSecure().GetPeerMessageInfo()));

exit:

    if (error != OT_ERROR_NONE)
    {
        if (message != NULL)
        {
            message->Free();
        }

        otLogWarnMeshCoP("Failed to send CoAP message: %s", otThreadErrorToString(error));
    }
}

void BorderAgent::HandleCoapResponse(void *               aContext,
                                     otCoapHeader *       aHeader,
                                     otMessage *          aMessage,
                                     const otMessageInfo *aMessageInfo,
                                     otError              aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    ForwardContext &forwardContext = *static_cast<ForwardContext *>(aContext);
    BorderAgent &   borderAgent    = forwardContext.GetBorderAgent();
    ThreadNetif &   netif          = borderAgent.GetNetif();
    const Message * message        = static_cast<const Message *>(aMessage);
    Coap::Header    header;
    otError         error;

    SuccessOrExit(error = aResult);

    if (forwardContext.IsPetition())
    {
        StateTlv stateTlv;

        SuccessOrExit(error = Tlv::GetTlv(*message, Tlv::kState, sizeof(stateTlv), stateTlv));

        if (stateTlv.GetState() == StateTlv::kAccept)
        {
            CommissionerSessionIdTlv sessionIdTlv;

            SuccessOrExit(error =
                              Tlv::GetTlv(*message, Tlv::kCommissionerSessionId, sizeof(sessionIdTlv), sessionIdTlv));

            netif.GetMle().GetCommissionerAloc(borderAgent.mCommissionerAloc.GetAddress(),
                                               sessionIdTlv.GetCommissionerSessionId());
            netif.AddUnicastAddress(borderAgent.mCommissionerAloc);
            netif.GetIp6().GetUdp().AddReceiver(borderAgent.mUdpReceiver);
        }
    }

    forwardContext.ToHeader(header, static_cast<Coap::Header *>(aHeader)->GetCode());

    if (message->GetLength() - message->GetOffset() > 0)
    {
        header.SetPayloadMarker();
    }

    SuccessOrExit(error = borderAgent.ForwardToCommissioner(header, *message));

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Commissioner request[%hu] failed: %s", forwardContext.GetMessageId(),
                         otThreadErrorToString(aResult));
        forwardContext.ToHeader(header, CoapCodeFromError(error));
        borderAgent.SendErrorMessage(header);
    }

    netif.GetInstance().GetHeap().Free(&forwardContext);
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mCommissionerPetition>(void *               aContext,
                                                                     otCoapHeader *       aHeader,
                                                                     otMessage *          aMessage,
                                                                     const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgent *>(aContext)->ForwardToLeader(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo), OT_URI_PATH_LEADER_PETITION, true, true);
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mCommissionerKeepAlive>(void *               aContext,
                                                                      otCoapHeader *       aHeader,
                                                                      otMessage *          aMessage,
                                                                      const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgent *>(aContext)->HandleKeepAlive(*static_cast<Coap::Header *>(aHeader),
                                                          *static_cast<Message *>(aMessage),
                                                          *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mRelayTransmit>(void *               aContext,
                                                              otCoapHeader *       aHeader,
                                                              otMessage *          aMessage,
                                                              const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    static_cast<BorderAgent *>(aContext)->HandleRelayTransmit(*static_cast<Coap::Header *>(aHeader),
                                                              *static_cast<Message *>(aMessage));
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mRelayReceive>(void *               aContext,
                                                             otCoapHeader *       aHeader,
                                                             otMessage *          aMessage,
                                                             const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    static_cast<BorderAgent *>(aContext)->HandleRelayReceive(*static_cast<Coap::Header *>(aHeader),
                                                             *static_cast<Message *>(aMessage));
}

template <>
void BorderAgent::HandleRequest<&BorderAgent::mProxyTransmit>(void *               aContext,
                                                              otCoapHeader *       aHeader,
                                                              otMessage *          aMessage,
                                                              const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);
    static_cast<BorderAgent *>(aContext)->HandleProxyTransmit(*static_cast<Coap::Header *>(aHeader),
                                                              *static_cast<Message *>(aMessage));
}

BorderAgent::BorderAgent(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCommissionerPetition(OT_URI_PATH_COMMISSIONER_PETITION,
                            BorderAgent::HandleRequest<&BorderAgent::mCommissionerPetition>,
                            this)
    , mCommissionerKeepAlive(OT_URI_PATH_COMMISSIONER_KEEP_ALIVE,
                             BorderAgent::HandleRequest<&BorderAgent::mCommissionerKeepAlive>,
                             this)
    , mRelayTransmit(OT_URI_PATH_RELAY_TX, BorderAgent::HandleRequest<&BorderAgent::mRelayTransmit>, this)
    , mRelayReceive(OT_URI_PATH_RELAY_RX, BorderAgent::HandleRequest<&BorderAgent::mRelayReceive>, this)
    , mCommissionerGet(OT_URI_PATH_COMMISSIONER_GET, BorderAgent::HandleRequest<&BorderAgent::mCommissionerGet>, this)
    , mCommissionerSet(OT_URI_PATH_COMMISSIONER_SET, BorderAgent::HandleRequest<&BorderAgent::mCommissionerSet>, this)
    , mActiveGet(OT_URI_PATH_ACTIVE_GET, BorderAgent::HandleRequest<&BorderAgent::mActiveGet>, this)
    , mActiveSet(OT_URI_PATH_ACTIVE_SET, BorderAgent::HandleRequest<&BorderAgent::mActiveSet>, this)
    , mPendingGet(OT_URI_PATH_PENDING_GET, BorderAgent::HandleRequest<&BorderAgent::mPendingGet>, this)
    , mPendingSet(OT_URI_PATH_PENDING_SET, BorderAgent::HandleRequest<&BorderAgent::mPendingSet>, this)
    , mProxyTransmit(OT_URI_PATH_PROXY_TX, BorderAgent::HandleRequest<&BorderAgent::mProxyTransmit>, this)
    , mUdpReceiver(BorderAgent::HandleUdpReceive, this)
    , mTimer(aInstance, HandleTimeout, this)
    , mState(OT_BORDER_AGENT_STATE_STOPPED)
{
    mCommissionerAloc.mPrefixLength       = 64;
    mCommissionerAloc.mPreferred          = true;
    mCommissionerAloc.mValid              = true;
    mCommissionerAloc.mScopeOverride      = Ip6::Address::kRealmLocalScope;
    mCommissionerAloc.mScopeOverrideValid = true;
}

void BorderAgent::HandleProxyTransmit(const Coap::Header &aHeader, const Message &aMessage)
{
    OT_UNUSED_VARIABLE(aHeader);

    Message *        message = NULL;
    Ip6::MessageInfo messageInfo;
    uint16_t         offset;
    otError          error;

    {
        UdpEncapsulationTlv tlv;

        SuccessOrExit(error = Tlv::GetOffset(aMessage, Tlv::kUdpEncapsulation, offset));
        aMessage.Read(offset, sizeof(tlv), &tlv);

        VerifyOrExit((message = GetInstance().GetIp6().GetUdp().NewMessage(0)) != NULL, error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = message->SetLength(tlv.GetUdpLength()));
        aMessage.CopyTo(offset + sizeof(tlv), 0, tlv.GetUdpLength(), *message);

        messageInfo.SetSockPort(tlv.GetSourcePort() != 0 ? tlv.GetSourcePort()
                                                         : GetInstance().GetIp6().GetUdp().GetEphemeralPort());
        messageInfo.SetSockAddr(mCommissionerAloc.GetAddress());
        messageInfo.SetPeerPort(tlv.GetDestinationPort());
    }

    {
        IPv6AddressTlv tlv;

        SuccessOrExit(error = Tlv::Get(aMessage, Tlv::kIPv6Address, sizeof(tlv), tlv));
        messageInfo.SetPeerAddr(tlv.GetAddress());
    }

    SuccessOrExit(error = GetInstance().GetIp6().GetUdp().SendDatagram(*message, messageInfo, Ip6::kProtoUdp));
    otLogInfoMeshCoP("Proxy transmit sent");

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to send proxy stream: %s", otThreadErrorToString(error));

        if (message != NULL)
        {
            message->Free();
        }
    }
}

bool BorderAgent::HandleUdpReceive(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Coap::Header header;
    otError      error;
    Message *    message = NULL;
    ThreadNetif &netif   = GetNetif();

    VerifyOrExit(aMessageInfo.GetSockAddr() == mCommissionerAloc.GetAddress(),
                 error = OT_ERROR_DESTINATION_ADDRESS_FILTERED);

    VerifyOrExit(aMessage.GetLength() > 0, error = OT_ERROR_NONE);

    header.Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    header.AppendUriPathOptions(OT_URI_PATH_PROXY_RX);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoapSecure(), header)) != NULL, error = OT_ERROR_NO_BUFS);

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

    {
        IPv6AddressTlv tlv;

        tlv.Init();
        tlv.SetAddress(aMessageInfo.GetPeerAddr());
        SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    }

    SuccessOrExit(error = netif.GetCoapSecure().SendMessage(*message, netif.GetCoapSecure().GetPeerMessageInfo()));

    otLogInfoMeshCoP("Sent to commissioner on %s", OT_URI_PATH_PROXY_RX);

exit:
    if (message != NULL && error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed notify commissioner on %s", OT_URI_PATH_PROXY_RX);
        message->Free();
    }

    return error != OT_ERROR_DESTINATION_ADDRESS_FILTERED;
}

void BorderAgent::HandleRelayReceive(const Coap::Header &aHeader, const Message &aMessage)
{
    Coap::Header header;

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_NON_CONFIRMABLE && aHeader.GetCode() == OT_COAP_CODE_POST);

    header.Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    header.AppendUriPathOptions(OT_URI_PATH_RELAY_RX);

    if (aMessage.GetLength() - aMessage.GetOffset() > 0)
    {
        header.SetPayloadMarker();
    }

    SuccessOrExit(ForwardToCommissioner(header, aMessage));
    otLogInfoMeshCoP("Sent to commissioner on %s", OT_URI_PATH_RELAY_RX);

exit:
    return;
}

otError BorderAgent::ForwardToCommissioner(const Coap::Header &aHeader, const Message &aMessage)
{
    ThreadNetif &netif   = GetNetif();
    otError      error   = OT_ERROR_NONE;
    Message *    message = NULL;
    uint16_t     offset  = 0;

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoapSecure(), aHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + aMessage.GetLength() - aMessage.GetOffset()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, aMessage.GetLength() - aMessage.GetOffset(), *message);

    SuccessOrExit(error = netif.GetCoapSecure().SendMessage(*message, netif.GetCoapSecure().GetPeerMessageInfo()));

    otLogInfoMeshCoP("Sent to commissioner");

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to send to commissioner: %s", otThreadErrorToString(error));
        if (message != NULL)
        {
            message->Free();
        }
    }

    return error;
}

void BorderAgent::HandleKeepAlive(const Coap::Header &    aHeader,
                                  const Message &         aMessage,
                                  const Ip6::MessageInfo &aMessageInfo)
{
    otError error;

    error = ForwardToLeader(aHeader, aMessage, aMessageInfo, OT_URI_PATH_LEADER_KEEP_ALIVE, false, true);

    if (error == OT_ERROR_NONE)
    {
        mTimer.Start(kKeepAliveTimeout);
    }
}

void BorderAgent::HandleRelayTransmit(const Coap::Header &aHeader, const Message &aMessage)
{
    ThreadNetif &          netif = GetNetif();
    otError                error = OT_ERROR_NONE;
    JoinerRouterLocatorTlv joinerRouterRloc;
    Message *              message = NULL;
    Ip6::MessageInfo       messageInfo;
    Coap::Header           header;
    uint16_t               offset = 0;

    VerifyOrExit(aHeader.GetType() == OT_COAP_TYPE_NON_CONFIRMABLE && aHeader.GetCode() == OT_COAP_CODE_POST);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerRouterLocator, sizeof(joinerRouterRloc), joinerRouterRloc));
    VerifyOrExit(joinerRouterRloc.IsValid(), error = OT_ERROR_PARSE);

    header.Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OT_URI_PATH_RELAY_TX);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + aMessage.GetLength() - aMessage.GetOffset()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, aMessage.GetLength() - aMessage.GetOffset(), *message);

    messageInfo.SetSockPort(kCoapUdpPort);
    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetPeerAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(joinerRouterRloc.GetJoinerRouterLocator());

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("Sent to joiner router request on %s", OT_URI_PATH_RELAY_TX);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to sent to joiner router request " OT_URI_PATH_RELAY_TX " %s",
                         otThreadErrorToString(error));
        if (message != NULL)
        {
            message->Free();
        }
    }
}

otError BorderAgent::ForwardToLeader(const Coap::Header &    aHeader,
                                     const Message &         aMessage,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     const char *            aPath,
                                     bool                    aPetition,
                                     bool                    aSeparate)
{
    ThreadNetif &    netif          = GetNetif();
    otError          error          = OT_ERROR_NONE;
    ForwardContext * forwardContext = NULL;
    Ip6::MessageInfo messageInfo;
    Coap::Header     header;
    Message *        message = NULL;
    uint16_t         offset  = 0;

    if (aSeparate)
    {
        SuccessOrExit(error = netif.GetCoapSecure().SendAck(aHeader, aMessageInfo));
    }

    forwardContext = static_cast<ForwardContext *>(GetInstance().GetHeap().CAlloc(1, sizeof(ForwardContext)));
    VerifyOrExit(forwardContext != NULL, error = OT_ERROR_NO_BUFS);

    forwardContext = new (forwardContext) ForwardContext(*this, aHeader, aPetition, aSeparate);

    header.Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(aPath);

    // Payload of c/cg may be empty
    if (aMessage.GetLength() - aMessage.GetOffset() > 0)
    {
        header.SetPayloadMarker();
    }

    VerifyOrExit((message = NewMeshCoPMessage(netif.GetCoap(), header)) != NULL, error = OT_ERROR_NO_BUFS);

    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + aMessage.GetLength() - aMessage.GetOffset()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, aMessage.GetLength() - aMessage.GetOffset(), *message);

    SuccessOrExit(error = netif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr()));
    messageInfo.SetPeerPort(kCoapUdpPort);
    messageInfo.SetSockAddr(netif.GetMle().GetMeshLocal16());
    messageInfo.SetSockPort(kCoapUdpPort);

    SuccessOrExit(error = netif.GetCoap().SendMessage(*message, messageInfo, HandleCoapResponse, forwardContext));

    // HandleCoapResponse is responsible to free this forward context.
    forwardContext = NULL;

    otLogInfoMeshCoP("Forwarded request to leader on %s", aPath);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnMeshCoP("Failed to forward to leader: %s", otThreadErrorToString(error));

        if (forwardContext != NULL)
        {
            GetInstance().GetHeap().Free(forwardContext);
        }

        if (message != NULL)
        {
            message->Free();
        }

        if (aHeader.GetType() == OT_COAP_TYPE_NON_CONFIRMABLE || aSeparate)
        {
            header.Init(OT_COAP_TYPE_NON_CONFIRMABLE, CoapCodeFromError(error));
        }
        else
        {
            header.Init(OT_COAP_TYPE_ACKNOWLEDGMENT, CoapCodeFromError(error));
        }

        header.SetMessageId(aSeparate ? 0 : aHeader.GetMessageId());
        header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());

        SendErrorMessage(header);
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
        ThreadNetif &     netif = GetNetif();
        Coap::CoapSecure &coaps = netif.GetCoapSecure();

        otLogInfoMeshCoP("Commissioner disconnected");
        netif.GetIp6().GetUdp().RemoveReceiver(mUdpReceiver);
        netif.RemoveUnicastAddress(mCommissionerAloc);
        coaps.Stop();
        SetState(OT_BORDER_AGENT_STATE_STARTED);
        mTimer.Start(kRestartDelay);
    }
}

otError BorderAgent::StartCoaps(void)
{
    ThreadNetif &     netif = GetNetif();
    Coap::CoapSecure &coaps = netif.GetCoapSecure();
    otError           error;

    SuccessOrExit(error = coaps.Start(kBorderAgentUdpPort));
    SuccessOrExit(error = coaps.SetPsk(netif.GetKeyManager().GetPSKc(), OT_PSKC_MAX_SIZE));
    coaps.SetConnectedCallback(HandleConnected, this);

exit:
    return error;
}

otError BorderAgent::Start(void)
{
    otError           error;
    ThreadNetif &     netif = GetNetif();
    Coap::CoapSecure &coaps = netif.GetCoapSecure();
    Coap::Coap &      coap  = netif.GetCoap();

    VerifyOrExit(mState == OT_BORDER_AGENT_STATE_STOPPED, error = OT_ERROR_ALREADY);

    SuccessOrExit(error = StartCoaps());

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

    coap.AddResource(mRelayReceive);

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
    ThreadNetif &     netif = GetNetif();
    Coap::CoapSecure &coaps = netif.GetCoapSecure();
    otError           error;

    if (coaps.IsConnected())
    {
        error = coaps.Stop();
        otLogWarnMeshCoP("Reset commissioner session: %s", otThreadErrorToString(error));
    }
    else if (!coaps.IsConnectionActive())
    {
        error = StartCoaps();
        otLogWarnMeshCoP("Restart border agent secure CoAP service: %s", otThreadErrorToString(error));
    }
    else
    {
        assert(false);
    }

    OT_UNUSED_VARIABLE(error);
}

otError BorderAgent::Stop(void)
{
    otError           error;
    ThreadNetif &     netif = GetNetif();
    Coap::CoapSecure &coaps = netif.GetCoapSecure();
    Coap::Coap &      coap  = netif.GetCoap();

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

    coap.RemoveResource(mRelayReceive);

    error = coaps.Stop();
    assert(error == OT_ERROR_NONE);

    SetState(OT_BORDER_AGENT_STATE_STOPPED);

exit:
    return error;
}

void BorderAgent::SetState(otBorderAgentState aState)
{
    if (mState != aState)
    {
        mState = aState;
        GetNotifier().Signal(OT_CHANGED_BORDER_AGENT_STATE);
    }
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_ENABLE_BORDER_AGENT
