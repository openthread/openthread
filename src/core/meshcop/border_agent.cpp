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
 *   This file implements a Border Agent role.
 */

#define WPP_NAME "border_agent.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "openthread/platform/random.h"

#include <coap/coap_header.hpp>
#include <common/crc16.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <meshcop/border_agent.hpp>
#include <meshcop/tlvs.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

using Thread::Encoding::BigEndian::HostSwap64;

namespace Thread {
namespace MeshCoP {

BorderAgent::BorderAgent(ThreadNetif &aThreadNetif):
    mRelayReceive(OPENTHREAD_URI_RELAY_RX, &BorderAgent::HandleRelayReceive, this),
    mRelayTransmit(OPENTHREAD_URI_RELAY_TX, &BorderAgent::HandleRelayTransmit, this),
    mCommissionerPetition(OPENTHREAD_URI_COMMISSIONER_PETITION, &BorderAgent::HandleCommissionerPetition, this),
    mCommissionerKeepAlive(OPENTHREAD_URI_COMMISSIONER_KEEP_ALIVE, &BorderAgent::HandleCommisionerKeepAlive, this),
    mMgmtCommissionerSet(OPENTHREAD_URI_COMMISSIONER_SET, &BorderAgent::HandleMgmtCommissionerSet, this),
    mTimer(aThreadNetif.GetIp6().mTimerScheduler, &BorderAgent::HandleTimer, this),
    mNetif(aThreadNetif)
{
    mNetif.GetCoapServer().AddResource(mRelayReceive);
    mNetif.GetBASecureCoapServer().AddResource(mRelayTransmit);
    mNetif.GetBASecureCoapServer().AddResource(mCommissionerPetition);
    mNetif.GetBASecureCoapServer().AddResource(mCommissionerKeepAlive);
    mNetif.GetBASecureCoapServer().AddResource(mMgmtCommissionerSet);
}

otInstance *BorderAgent::GetInstance()
{
    return mNetif.GetInstance();
}

ThreadError BorderAgent::Start(void)
{
    ThreadError error = kThreadError_None;
    if (mNetif.GetDtls().IsStarted())
    {
        SuccessOrExit(error = mNetif.GetBASecureCoapServer().Stop());
        mNetif.GetDtls().Stop();
    }
    mTimer.Start(kWaitChildIDResponseTime);
exit:
    return error;
}

ThreadError BorderAgent::Stop(void)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().Stop());

    mNetif.GetDtls().Stop();

exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError CopyPayload(Message &srcMessage, Message *destMessage)
{
    ThreadError error = kThreadError_None;

    while (srcMessage.GetOffset() < srcMessage.GetLength())
    {
        uint16_t length = srcMessage.GetLength() - srcMessage.GetOffset();
        uint8_t tmp[16];

        if (length > sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        srcMessage.Read(srcMessage.GetOffset(), length, tmp);
        srcMessage.MoveOffset(length);

        SuccessOrExit(error = destMessage->Append(tmp, length));
    }
exit:
    return error;
}

PSKcTlv *GetPSKc(ThreadNetif &aNetif){
    return static_cast<PSKcTlv *>(aNetif.GetActiveDataset().GetNetwork().Get(Tlv::kPSKc));
}

void BorderAgent::HandleCommissionerPetition(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                             const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgent *>(aContext)->HandleCommissionerPetition(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void BorderAgent::HandleCommissionerPetition(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{

    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();
    otLogInfoMeshCoP(GetInstance(), "received petition from commissioner");

    mCommissionerAddr = aMessageInfo.GetPeerAddr();
    mCommissionerUdpPort = aMessageInfo.GetPeerPort();

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().SendEmptyAck(aHeader, aMessageInfo));
    otLogInfoMeshCoP(GetInstance(), "replied comm-pet req ACK");

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    header.AppendUriPathOptions(OPENTHREAD_URI_LEADER_PETITION);
    header.SetPayloadMarker();

    message = mNetif.GetCoapClient().NewMeshCoPMessage(header);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = CopyPayload(aMessage, message));

    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = SendLeaderPetition(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "send petition to leader");

exit:
    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
    otLogFuncExit();
}

ThreadError BorderAgent::SendLeaderPetition(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(aMessage, aMessageInfo,
                          BorderAgent::HandleLeaderPetitionResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent petition to leader");

exit:
    otLogFuncExitErr(error);
    return error;
}

void BorderAgent::HandleLeaderPetitionResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                               const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<BorderAgent *>(aContext)->HandleLeaderPetitionResponse(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);

}

void BorderAgent::HandleLeaderPetitionResponse(Coap::Header &aHeader, Message &aMessage,
                                               const Ip6::MessageInfo &aMessageInfo, ThreadError aResult)
{
    ThreadError error = kThreadError_None;
    Ip6::MessageInfo messageInfo;
    Coap::Header responseHeader;
    Message *message = NULL;

    otLogFuncEntry();
    VerifyOrExit(aResult == kThreadError_None && aHeader.GetCode() == kCoapResponseChanged, ;);
    otLogInfoMeshCoP(GetInstance(), "received petition response from leader");

    responseHeader.Init(kCoapTypeAcknowledgment, kCoapResponseChanged);
    responseHeader.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    responseHeader.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_PETITION);
    responseHeader.SetPayloadMarker();

    message = mNetif.GetBASecureCoapServer().NewMeshCoPMessage(responseHeader);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = CopyPayload(aMessage, message));

    messageInfo.SetPeerAddr(mCommissionerAddr);
    messageInfo.SetPeerPort(mCommissionerUdpPort);

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent Petition response to commissioner");

exit:
    (void)aMessageInfo;
    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
    otLogFuncExit();
}

void BorderAgent::HandleCommisionerKeepAlive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                             const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgent *>(aContext)->HandleCommisionerKeepAlive(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void BorderAgent::HandleCommisionerKeepAlive(Coap::Header &aHeader, Message &aMessage,
                                             const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();
    otLogInfoMeshCoP(GetInstance(), "received keep alive from commissioner");

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().SendEmptyAck(aHeader, aMessageInfo));
    otLogInfoMeshCoP(GetInstance(), "replied comm-ka req ACK");

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    header.AppendUriPathOptions(OPENTHREAD_URI_LEADER_KEEP_ALIVE);
    header.SetPayloadMarker();

    message = mNetif.GetCoapClient().NewMeshCoPMessage(header);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = CopyPayload(aMessage, message));

    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = SendLeaderKeepAlive(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "send keep alive to leader");

exit:
    (void)aMessageInfo;
    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
    otLogFuncExit();
}

ThreadError BorderAgent::SendLeaderKeepAlive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(aMessage, aMessageInfo,
                          BorderAgent::HandleLeaderKeepAliveResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent keep alive to leader");

exit:
    (void)aMessageInfo;
    otLogFuncExitErr(error);
    return error;
}

void BorderAgent::HandleLeaderKeepAliveResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<BorderAgent *>(aContext)->HandleLeaderKeepAliveResponse(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void BorderAgent::HandleLeaderKeepAliveResponse(Coap::Header &aHeader, Message &aMessage,
                                                const Ip6::MessageInfo &aMessageInfo, ThreadError aResult)
{
    ThreadError error = kThreadError_None;
    Ip6::MessageInfo messageInfo;
    Coap::Header responseHeader;
    Message *message = NULL;

    otLogFuncEntry();
    VerifyOrExit(aResult == kThreadError_None && aHeader.GetCode() == kCoapResponseChanged, ;);
    otLogInfoMeshCoP(GetInstance(), "received keep alive response from leader");

    responseHeader.Init(kCoapTypeAcknowledgment, kCoapResponseChanged);
    responseHeader.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    responseHeader.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_KEEP_ALIVE);
    responseHeader.SetPayloadMarker();

    message = mNetif.GetBASecureCoapServer().NewMeshCoPMessage(responseHeader);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = CopyPayload(aMessage, message));

    messageInfo.SetPeerAddr(mCommissionerAddr);
    messageInfo.SetPeerPort(mCommissionerUdpPort);

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent keep alive response to commissioner");

exit:
    (void)aMessageInfo;
    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
    otLogFuncExit();
}


void BorderAgent::HandleRelayTransmit(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                      const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgent *>(aContext)->HandleRelayTransmit(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void BorderAgent::HandleRelayTransmit(Coap::Header &aHeader, Message &aMessage,
                                      const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;
    JoinerRouterLocatorTlv joinerRloc;

    otLogFuncEntry();
    otLogInfoMeshCoP(GetInstance(), "received relay transmit from commissioner");

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerRouterLocator, sizeof(joinerRloc), joinerRloc));
    VerifyOrExit(joinerRloc.IsValid(), error = kThreadError_Parse);

    (void) aHeader;
    header.Init(kCoapTypeNonConfirmable, kCoapRequestPost);
    header.AppendUriPathOptions(OPENTHREAD_URI_RELAY_TX);
    header.SetPayloadMarker();

    message = mNetif.GetCoapServer().NewMeshCoPMessage(header);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = CopyPayload(aMessage, message));

    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(joinerRloc.GetJoinerRouterLocator());
    messageInfo.SetPeerPort(kCoapUdpPort);

    otLogInfoMeshCoP(GetInstance(), "send relay transmit to joiner router");

    SuccessOrExit(error = mNetif.GetCoapServer().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent relay transmit to joiner router %d", error);

exit:
    (void) aMessageInfo;
    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
    otLogFuncExit();
}

void BorderAgent::HandleMgmtCommissionerSet(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                            const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgent *>(aContext)->HandleMgmtCommissionerSet(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void BorderAgent::HandleMgmtCommissionerSet(Coap::Header &aHeader, Message &aMessage,
                                            const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();
    otLogInfoMeshCoP(GetInstance(), "received MGMT_COMMISSIONER_SET request from Commissioner");

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().SendEmptyAck(aHeader, aMessageInfo));
    otLogInfoMeshCoP(GetInstance(), "replied mgmt-set req ACK");

    otLogInfoMeshCoP(GetInstance(), "send MGMT_COMMISSIONER_SET request to Leader");

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    header.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_SET);
    header.SetPayloadMarker();

    message = mNetif.GetCoapClient().NewMeshCoPMessage(header);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = CopyPayload(aMessage, message));

    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetSockAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.SetPeerPort(kCoapUdpPort);
    SuccessOrExit(error = SendLeaderMgmtCommissionerSet(*message, messageInfo));

exit:
    (void)aMessageInfo;
    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
    otLogFuncExit();
}


ThreadError BorderAgent::SendLeaderMgmtCommissionerSet(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(aMessage, aMessageInfo,
                          BorderAgent::HandleLeaderMgmtCommissionerSetResponse, this));

    otLogInfoMeshCoP(GetInstance(), "sent MGMT_COMMISSIONER_SET to leader");

exit:
    (void) aMessageInfo;
    otLogFuncExitErr(error);
    return error;
}

void BorderAgent::HandleLeaderMgmtCommissionerSetResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                                          const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<BorderAgent *>(aContext)->HandleLeaderMgmtCommissionerSetResponse(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);

}

void BorderAgent::HandleLeaderMgmtCommissionerSetResponse(Coap::Header &aHeader, Message &aMessage,
        const Ip6::MessageInfo &aMessageInfo, ThreadError aResult)
{
    ThreadError error = kThreadError_None;
    Ip6::MessageInfo messageInfo;
    Coap::Header responseHeader;
    Message *message = NULL;

    otLogFuncEntry();

    VerifyOrExit(aResult == kThreadError_None && aHeader.GetCode() == kCoapResponseChanged, ;);
    otLogInfoMeshCoP(GetInstance(), "received MGMT_COMMISSIONER_SET response from leader");

    responseHeader.Init(kCoapTypeAcknowledgment, kCoapResponseChanged);
    responseHeader.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    responseHeader.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_SET);
    responseHeader.SetPayloadMarker();

    message = mNetif.GetBASecureCoapServer().NewMeshCoPMessage(responseHeader);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = CopyPayload(aMessage, message));

    messageInfo.SetPeerAddr(mCommissionerAddr);
    messageInfo.SetPeerPort(mCommissionerUdpPort);

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent MGMT_COMMISSIONER_SET response to commissioner");

exit:
    (void) aMessageInfo;
    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
    otLogFuncExit();
}

void BorderAgent::HandleRelayReceive(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                     const otMessageInfo *aMessageInfo)
{
    static_cast<BorderAgent *>(aContext)->HandleRelayReceive(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void BorderAgent::HandleRelayReceive(Coap::Header &aHeader, Message &aMessage,
                                     const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header header;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();
    otLogInfoMeshCoP(GetInstance(), "received Relay Recevie from Joiner Router");

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    header.AppendUriPathOptions(OPENTHREAD_URI_RELAY_RX);
    header.SetPayloadMarker();

    message = mNetif.GetBASecureCoapServer().NewMeshCoPMessage(header);
    VerifyOrExit(message != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = CopyPayload(aMessage, message));

    messageInfo.SetPeerAddr(mCommissionerAddr);
    messageInfo.SetPeerPort(mCommissionerUdpPort);

    otLogInfoMeshCoP(GetInstance(), "send Relay Receive to Commissioner");

    SuccessOrExit(error = SendCommissionerRelayReceive(*message, messageInfo));


exit:
    (void)aMessageInfo;
    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
    otLogFuncExit();
}


ThreadError BorderAgent::SendCommissionerRelayReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().SendMessage(aMessage, aMessageInfo));

    otLogInfoMeshCoP(GetInstance(), "sent Realy Receive to Commissioner");

exit:
    (void) aMessageInfo;
    otLogFuncExitErr(error);
    return error;
}

void BorderAgent::HandleTimer(void *aContext)
{
    static_cast<BorderAgent *>(aContext)->HandleTimer();
}

void BorderAgent::HandleTimer(void)
{
    ThreadError error = kThreadError_Error;
    PSKcTlv *pskc;
    otLogFuncEntry();

    VerifyOrExit((pskc = GetPSKc(mNetif)) != NULL, error = kThreadError_InvalidArgs);

    SuccessOrExit(error = mNetif.GetBASecureCoapServer().SetPsk(reinterpret_cast<const uint8_t *>(pskc->GetPSKc()),
                          static_cast<uint8_t>(Dtls::kPskMaxLength / 2)));
    SuccessOrExit(error = mNetif.GetBASecureCoapServer().Start(NULL, this));

exit:
    otLogFuncExitErr(error);
}

}  // namespace MeshCoP
}  // namespace Thread
