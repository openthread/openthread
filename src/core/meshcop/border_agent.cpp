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

#include <coap/coap_header.hpp>
#include <common/logging.hpp>
#include <thread/mle.hpp>
#include <meshcop/border_agent.hpp>
#include <meshcop/joiner_router.hpp>
#include <meshcop/tlvs.hpp>
#include <platform/random.h>
#include <thread/thread_netif.hpp>
#include <thread/thread_tlvs.hpp>
#include <thread/thread_uris.hpp>

namespace Thread {
namespace MeshCoP {

BorderAgent::BorderAgent(ThreadNetif &aThreadNetif):
    mRelayReceive(OPENTHREAD_URI_RELAY_RX, &BorderAgent::HandleRelayReceive, this),
    mRelayTransmit(OPENTHREAD_URI_RELAY_TX, &BorderAgent::HandleRelayTransmit, this),
    mCommissionerPetition(OPENTHREAD_URI_COMMISSIONER_PETITION, &BorderAgent::HandleCommissionerPetition, this),
    mCommissionerKeepAlive(OPENTHREAD_URI_COMMISSIONER_KEEP_ALIVE, &BorderAgent::HandleCommisionerKeepAlive, this),
    mMgmtCommissionerSet(OPENTHREAD_URI_COMMISSIONER_SET, &BorderAgent::HandleMgmtCommissionerSet, this),
    mNetif(aThreadNetif)
{
    mTokenLength = 0;
}


ThreadError BorderAgent::Start(void)
{
    ThreadError error = kThreadError_None;

    otLogCritMeshCoP(">>>>>>>>AddResource");
    mNetif.GetCoapServer().AddResource(mRelayReceive);
    mNetif.GetSecureCoapServer().AddResource(mRelayTransmit);
    mNetif.GetSecureCoapServer().AddResource(mCommissionerPetition);
    mNetif.GetSecureCoapServer().AddResource(mCommissionerKeepAlive);
    mNetif.GetSecureCoapServer().AddResource(mMgmtCommissionerSet);
     otLogCritMeshCoP("<<<<<<<<<<AddResource");
    //default PSKc value derived from network name is "Test Network" , xpanid is "0001020304050607" passphrase "12SECRETPASSWORD34"
    //"C3F59368445A1B6106BE420A706D4CC9"
    //C3 F5 93 68 44 5A 1B 61 06 BE 42 0A 70 6D 4C C9
        
    const char kThreadPSKc[] = 
    {
        0xc3, 0xf5, 0x93, 0x68, 0x44, 0x5a, 0x1b, 0x61,
        0x06, 0xbe, 0x42, 0x0a, 0x70, 0x6d, 0x4c, 0xc9,

    };

    //default PSKc value derived from network name is "yuwen-wpan" , xpanid is "11111111222222222" password "password"
    // const char kThreadPSKc[] =
    // {
    //     0xca, 0x28, 0xb1, 0x95, 0x2b, 0x76, 0xad, 0xe6,
    //     0x6f, 0x73, 0x89, 0x67, 0x5e, 0x65, 0x4d, 0x80,

    // };

    otLogFuncEntry();
    SetPSKc(kThreadPSKc, 16);

    SuccessOrExit(error = mNetif.GetSecureCoapServer().SetPsk(reinterpret_cast<const uint8_t *>(kThreadPSKc),
                                                              static_cast<uint8_t>(strlen(kThreadPSKc))));
    SuccessOrExit(error = mNetif.GetSecureCoapServer().Start(NULL, this));
    // mNetif.GetDtls().SetPsk(mPSKc, mPSKcLength);


exit:
    otLogFuncExitErr(error);
    return error;
}

ThreadError BorderAgent::Stop(void)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    mNetif.GetSecureCoapServer().Stop();

    mNetif.GetDtls().Stop();

    return error;
}

ThreadError BorderAgent::SetPSKc(const char *aPSKc, const int aPSKcLength)
{
    ThreadError error = kThreadError_None;
    memcpy(mPSKc, aPSKc, aPSKcLength);
    mPSKcLength = aPSKcLength;
    return error;
}

void BorderAgent::HandleCommissionerPetition(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
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
    // VerifyOrExit(aHeader.GetType() == kCoapTypeNonConfirmable &&
    //              aHeader.GetCode() == kCoapRequestPost, error = kThreadError_Drop);

    otLogCritMeshCoP("received petition from commissioner");
    mTokenLength = aHeader.GetTokenLength();
    memcpy(mToken, aHeader.GetToken(), aHeader.GetTokenLength());


    
    mCommissionerAddr = aMessageInfo.GetSockAddr();

    mCommissionerUdpPort = aMessageInfo.GetSockPort();

    message = NULL;

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_LEADER_PETITION);
    header.SetPayloadMarker();
    
    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
        uint8_t tmp[16];

        if (length >= sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        aMessage.Read(aMessage.GetOffset(), length, tmp);
        aMessage.MoveOffset(length);

        SuccessOrExit(error = message->Append(tmp, length));
    }
    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);    
    SendLeaderPetition(*message, messageInfo);

    otLogCritMeshCoP("send petition to leader");
    
exit:
    (void)aMessageInfo;
    (void) aHeader;
    // aMessage.Free();
    // message->Free();
    (void) aMessage;
    (void) message;
    if (error != kThreadError_None && message != NULL)
    {
    }
    otLogFuncExit();
}


ThreadError BorderAgent::SendLeaderPetition(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(aMessage, aMessageInfo,
                                                             BorderAgent::HandleLeaderPetitionResponse, this));

    otLogCritMeshCoP("sent petition to leader");

exit:
    (void)aMessageInfo;
    // aMessage.Free();
    (void) aMessage;
    otLogFuncExitErr(error);
    return error;
}

void BorderAgent::HandleLeaderPetitionResponse(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
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
    otLogCritMeshCoP("received petition response from leader");

    responseHeader.Init(kCoapTypeAcknowledgment, kCoapResponseChanged);
    responseHeader.SetToken(mToken, mTokenLength);
    responseHeader.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_PETITION);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetSecureCoapServer().NewMeshCoPMessage(responseHeader)) != NULL,
                 error = kThreadError_NoBufs);    
    
    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
        uint8_t tmp[16];

        if (length >= sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        aMessage.Read(aMessage.GetOffset(), length, tmp);
        aMessage.MoveOffset(length);

        SuccessOrExit(error = message->Append(tmp, length));
    }

    messageInfo.SetPeerAddr(mCommissionerAddr);
    messageInfo.SetPeerPort(mCommissionerUdpPort);

    SuccessOrExit(error = mNetif.GetSecureCoapServer().SendMessage(*message, messageInfo));

    otLogCritMeshCoP("sent Petition response to commissioner");

exit:
    (void)aMessageInfo;
    // aMessage.Free();
    if (error != kThreadError_None && message != NULL)
    {
        // message->Free();
    }
    otLogFuncExit();
}

void BorderAgent::HandleCommisionerKeepAlive(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
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
    // VerifyOrExit(aHeader.GetType() == kCoapTypeNonConfirmable &&
    //              aHeader.GetCode() == kCoapRequestPost, error = kThreadError_Drop);
    otLogCritMeshCoP("received keep alive from commissioner");

    mTokenLength = aHeader.GetTokenLength();
    memcpy(mToken, aHeader.GetToken(), aHeader.GetTokenLength());

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_LEADER_KEEP_ALIVE);
    header.SetPayloadMarker();
    
    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
        uint8_t tmp[16];

        if (length >= sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        aMessage.Read(aMessage.GetOffset(), length, tmp);
        aMessage.MoveOffset(length);

        SuccessOrExit(error = message->Append(tmp, length));
    }
    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);    
    SendLeaderKeepAlive(*message, messageInfo);

    otLogCritMeshCoP("send keep alive to leader");
    
exit:
    (void)aMessageInfo;
    // aMessage.Free();
    if (error != kThreadError_None && message != NULL)
    {
        // message->Free();
    }
    otLogFuncExit();
}

ThreadError BorderAgent::SendLeaderKeepAlive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(aMessage, aMessageInfo,
                                                             BorderAgent::HandleLeaderKeepAliveResponse, this));

    otLogCritMeshCoP("sent keep alive to leader");

exit:
    (void)aMessageInfo;
    // aMessage.Free();
    otLogFuncExitErr(error);
    return error;
}

void BorderAgent::HandleLeaderKeepAliveResponse(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
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
    otLogCritMeshCoP("received keep alive response from leader");

    responseHeader.Init(kCoapTypeAcknowledgment, kCoapResponseChanged);
    responseHeader.SetToken(mToken, mTokenLength);
    responseHeader.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_KEEP_ALIVE);
    responseHeader.SetPayloadMarker();

    VerifyOrExit((message = mNetif.GetSecureCoapServer().NewMeshCoPMessage(responseHeader)) != NULL,
                 error = kThreadError_NoBufs);    
    
    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
        uint8_t tmp[16];

        if (length >= sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        aMessage.Read(aMessage.GetOffset(), length, tmp);
        aMessage.MoveOffset(length);

        SuccessOrExit(error = message->Append(tmp, length));
    }

    messageInfo.SetPeerAddr(mCommissionerAddr);
    messageInfo.SetPeerPort(mCommissionerUdpPort);

    SuccessOrExit(error = mNetif.GetSecureCoapServer().SendMessage(*message, messageInfo));

    otLogCritMeshCoP("sent keep alive response to commissioner");

exit:
    
    (void) aMessageInfo;
    // aMessage.Free();
    if (error != kThreadError_None && message != NULL)
    {
        // message->Free();
    }
    otLogFuncExit();
}


void BorderAgent::HandleRelayTransmit(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
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
    Message *message;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();
    // VerifyOrExit(aHeader.GetType() == kCoapTypeNonConfirmable &&
    //              aHeader.GetCode() == kCoapRequestPost, ;);
    otLogCritMeshCoP("received relay transmit from commissioner");

    header.Init(kCoapTypeNonConfirmable, kCoapRequestPost);
    header.AppendUriPathOptions(OPENTHREAD_URI_RELAY_TX);
    header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    header.SetPayloadMarker();
    // Construct the Coap Message
    VerifyOrExit((message = mNetif.GetCoapServer().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);
    // Copy the payload to new message
    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
        uint8_t tmp[16];

        if (length >= sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        aMessage.Read(aMessage.GetOffset(), length, tmp);
        aMessage.MoveOffset(length);

        SuccessOrExit(error = message->Append(tmp, length));
    }
    //set peer addr and peer port
    messageInfo.SetPeerAddr(mJoinerRouterAddr);
    messageInfo.SetPeerPort(mJoinerRouterUdpPort);

    otLogCritMeshCoP("send relay transmit to joiner router");

    SuccessOrExit(error = mNetif.GetCoapServer().SendMessage(*message, messageInfo));

exit:
    (void) aMessageInfo;
    // aMessage.Free();
    if (error != kThreadError_None && message != NULL)
    {
        // message->Free();
    }
    otLogFuncExit();
}

void BorderAgent::HandleMgmtCommissionerSet(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
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
    // VerifyOrExit(aHeader.GetType() == kCoapTypeNonConfirmable &&
    //              aHeader.GetCode() == kCoapRequestPost, ;);
    otLogCritMeshCoP("received MGMT_COMMISSIONER_SET request from Commissioner");
    mTokenLength = aHeader.GetTokenLength();
    memcpy(mToken, aHeader.GetToken(), aHeader.GetTokenLength());
    //Records the peer address and peer udp port
    mCommissionerAddr = aMessageInfo.GetSockAddr();
    mCommissionerUdpPort = aMessageInfo.GetSockPort();

    otLogCritMeshCoP("send MGMT_COMMISSIONER_SET request to Leader");

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_SET);
    header.SetPayloadMarker();
    
    VerifyOrExit((message = mNetif.GetCoapClient().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
        uint8_t tmp[16];

        if (length >= sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        aMessage.Read(aMessage.GetOffset(), length, tmp);
        aMessage.MoveOffset(length);

        SuccessOrExit(error = message->Append(tmp, length));
    }
    mNetif.GetMle().GetLeaderAloc(messageInfo.GetPeerAddr());
    messageInfo.SetPeerPort(kCoapUdpPort);    
    SendLeaderMgmtCommissionerSet(*message, messageInfo);

exit:
    (void)aMessageInfo;
    // aMessage.Free();
    if (error != kThreadError_None && message != NULL)
    {
        // message->Free();
    }
    otLogFuncExit();
}


ThreadError BorderAgent::SendLeaderMgmtCommissionerSet(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(aMessage, aMessageInfo,
                                                             BorderAgent::HandleLeaderMgmtCommissionerSetResponse, this));

    otLogCritMeshCoP("sent MGMT_COMMISSIONER_SET to leader");

exit:
    (void) aMessageInfo;
    otLogFuncExitErr(error);
    return error;
}

void BorderAgent::HandleLeaderMgmtCommissionerSetResponse(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
                                                          const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<BorderAgent *>(aContext)->HandleLeaderPetitionResponse(
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
    otLogCritMeshCoP("received MGMT_COMMISSIONER_SET response from leader");

    // responseHeader.SetDefaultResponseHeader(aHeader);
    // responseHeader.SetPayloadMarker();
    responseHeader.Init(kCoapTypeAcknowledgment, kCoapResponseChanged);
    responseHeader.SetToken(mToken, mTokenLength);
    responseHeader.AppendUriPathOptions(OPENTHREAD_URI_COMMISSIONER_SET);
    responseHeader.SetPayloadMarker();
    VerifyOrExit((message = mNetif.GetSecureCoapServer().NewMeshCoPMessage(responseHeader)) != NULL,
                 error = kThreadError_NoBufs);    
    
    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
        uint8_t tmp[16];

        if (length >= sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        aMessage.Read(aMessage.GetOffset(), length, tmp);
        aMessage.MoveOffset(length);

        SuccessOrExit(error = message->Append(tmp, length));
    }

    messageInfo.SetPeerAddr(mCommissionerAddr);
    messageInfo.SetPeerPort(mCommissionerUdpPort);

    SuccessOrExit(error = mNetif.GetSecureCoapServer().SendMessage(*message, messageInfo));

    otLogCritMeshCoP("sent MGMT_COMMISSIONER_SET response to commissioner");

exit:
    (void) aMessageInfo;
    // aMessage.Free();
    if (error != kThreadError_None && message != NULL)
    {
        // message->Free();
    }
    otLogFuncExit();
}

void BorderAgent::HandleRelayReceive(void *aContext, otCoapHeader *aHeader, otMessage aMessage,
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
    // VerifyOrExit(aHeader.GetType() == kCoapTypeNonConfirmable &&
    //              aHeader.GetCode() == kCoapRequestPost, ;);

    otLogCritMeshCoP("received Relay Recevie from Joiner Router");
    
    //Records the peer address and peer udp port
    mJoinerRouterAddr = aMessageInfo.GetSockAddr();
    mJoinerRouterUdpPort = aMessageInfo.GetSockPort();

    otLogCritMeshCoP("send Relay Receive to Commissioner");

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.SetToken(aHeader.GetToken(), aHeader.GetTokenLength());
    header.AppendUriPathOptions(OPENTHREAD_URI_RELAY_RX);
    header.SetPayloadMarker();
    
    VerifyOrExit((message = mNetif.GetSecureCoapServer().NewMeshCoPMessage(header)) != NULL, error = kThreadError_NoBufs);

    while (aMessage.GetOffset() < aMessage.GetLength())
    {
        uint16_t length = aMessage.GetLength() - aMessage.GetOffset();
        uint8_t tmp[16];

        if (length >= sizeof(tmp))
        {
            length = sizeof(tmp);
        }

        aMessage.Read(aMessage.GetOffset(), length, tmp);
        aMessage.MoveOffset(length);

        SuccessOrExit(error = message->Append(tmp, length));
    }
 
    messageInfo.SetPeerAddr(mCommissionerAddr);
    messageInfo.SetPeerPort(mCommissionerUdpPort);

    SendCommissionerRelayReceive(*message, messageInfo);

    
exit:
    (void)aMessageInfo;
    // aMessage.Free();
    if (error != kThreadError_None && message != NULL)
    {
        // message->Free();
    }
    otLogFuncExit();
}


ThreadError BorderAgent::SendCommissionerRelayReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;

    otLogFuncEntry();

    SuccessOrExit(error = mNetif.GetSecureCoapServer().SendMessage(aMessage, aMessageInfo));

    otLogCritMeshCoP("sent Realy Receive to Commissioner");

exit:
    (void) aMessageInfo;
    // aMessage.Free();
    otLogFuncExitErr(error);
    return error;
}

}  // namespace MeshCoP
}  // namespace Thread