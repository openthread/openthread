/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements the Joiner Router role.
 */

#if OPENTHREAD_FTD

#define WPP_NAME "joiner_router.tmh"

#include  "openthread/openthread_enable_defines.h"

#include <stdio.h>

#include <common/code_utils.hpp>
#include <common/encoding.hpp>
#include <common/logging.hpp>
#include <meshcop/joiner_router.hpp>
#include <meshcop/meshcop.hpp>
#include <meshcop/tlvs.hpp>
#include <thread/mle.hpp>
#include <thread/thread_netif.hpp>
#include <thread/thread_uris.hpp>

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap64;

namespace ot {
namespace MeshCoP {

JoinerRouter::JoinerRouter(ThreadNetif &aNetif):
    mSocket(aNetif.GetIp6().mUdp),
    mRelayTransmit(OPENTHREAD_URI_RELAY_TX, &JoinerRouter::HandleRelayTransmit, this),
    mNetif(aNetif),
    mTimer(aNetif.GetIp6().mTimerScheduler, &JoinerRouter::HandleTimer, this),
    mJoinerUdpPort(0),
    mIsJoinerPortConfigured(false),
    mExpectJoinEntRsp(false)
{
    mSocket.GetSockName().mPort = OPENTHREAD_CONFIG_JOINER_UDP_PORT;
    mNetif.GetCoapServer().AddResource(mRelayTransmit);
    mNetifCallback.Set(HandleNetifStateChanged, this);
    mNetif.RegisterCallback(mNetifCallback);
}

otInstance *JoinerRouter::GetInstance(void)
{
    return mNetif.GetInstance();
}

void JoinerRouter::HandleNetifStateChanged(uint32_t aFlags, void *aContext)
{
    static_cast<JoinerRouter *>(aContext)->HandleNetifStateChanged(aFlags);
}

void JoinerRouter::HandleNetifStateChanged(uint32_t aFlags)
{
    VerifyOrExit(mNetif.GetMle().GetDeviceMode() & Mle::ModeTlv::kModeFFD);
    VerifyOrExit(aFlags & OT_THREAD_NETDATA_UPDATED);

    mNetif.GetIp6Filter().RemoveUnsecurePort(mSocket.GetSockName().mPort);

    if (mNetif.GetNetworkDataLeader().IsJoiningEnabled())
    {
        Ip6::SockAddr sockaddr;

        sockaddr.mPort = GetJoinerUdpPort();

        mSocket.Open(&JoinerRouter::HandleUdpReceive, this);
        mSocket.Bind(sockaddr);
        mNetif.GetIp6Filter().AddUnsecurePort(sockaddr.mPort);
        otLogInfoMeshCoP(GetInstance(), "Joiner Router: start");
    }
    else
    {
        mSocket.Close();
    }

exit:
    return;
}

ThreadError JoinerRouter::GetBorderAgentRloc(uint16_t &aRloc)
{
    ThreadError error = kThreadError_None;
    BorderAgentLocatorTlv *borderAgentLocator;

    borderAgentLocator = static_cast<BorderAgentLocatorTlv *>(mNetif.GetNetworkDataLeader().GetCommissioningDataSubTlv(
                                                                  Tlv::kBorderAgentLocator));
    VerifyOrExit(borderAgentLocator != NULL, error = kThreadError_NotFound);

    aRloc = borderAgentLocator->GetBorderAgentLocator();

exit:
    return error;
}

uint16_t JoinerRouter::GetJoinerUdpPort(void)
{
    uint16_t rval = OPENTHREAD_CONFIG_JOINER_UDP_PORT;
    JoinerUdpPortTlv *joinerUdpPort;

    VerifyOrExit(!mIsJoinerPortConfigured, rval = mJoinerUdpPort);

    joinerUdpPort = static_cast<JoinerUdpPortTlv *>(mNetif.GetNetworkDataLeader().GetCommissioningDataSubTlv(
                                                        Tlv::kJoinerUdpPort));
    VerifyOrExit(joinerUdpPort != NULL);

    rval = joinerUdpPort->GetUdpPort();

exit:
    return rval;
}

ThreadError JoinerRouter::SetJoinerUdpPort(uint16_t aJoinerUdpPort)
{
    otLogFuncEntry();
    mJoinerUdpPort = aJoinerUdpPort;
    mIsJoinerPortConfigured = true;
    HandleNetifStateChanged(OT_THREAD_NETDATA_UPDATED);
    otLogFuncExit();
    return kThreadError_None;
}

void JoinerRouter::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<JoinerRouter *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void JoinerRouter::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;
    Message *message = NULL;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;
    JoinerUdpPortTlv udpPort;
    JoinerIidTlv iid;
    JoinerRouterLocatorTlv rloc;
    ExtendedTlv tlv;
    uint16_t borderAgentRloc;

    otLogFuncEntryMsg("from peer: %llX",
                      HostSwap64(*reinterpret_cast<const uint64_t *>(aMessageInfo.GetPeerAddr().mFields.m8 + 8)));
    otLogInfoMeshCoP(GetInstance(), "JoinerRouter::HandleUdpReceive");

    SuccessOrExit(error = GetBorderAgentRloc(borderAgentRloc));

    header.Init(kCoapTypeNonConfirmable, kCoapRequestPost);
    header.SetToken(Coap::Header::kDefaultTokenLength);
    header.AppendUriPathOptions(OPENTHREAD_URI_RELAY_RX);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(mNetif.GetCoapClient(), header)) != NULL,
                 error = kThreadError_NoBufs);

    udpPort.Init();
    udpPort.SetUdpPort(aMessageInfo.GetPeerPort());
    SuccessOrExit(error = message->Append(&udpPort, sizeof(udpPort)));

    iid.Init();
    iid.SetIid(aMessageInfo.GetPeerAddr().mFields.m8 + 8);
    SuccessOrExit(error = message->Append(&iid, sizeof(iid)));

    rloc.Init();
    rloc.SetJoinerRouterLocator(mNetif.GetMle().GetRloc16());
    SuccessOrExit(error = message->Append(&rloc, sizeof(rloc)));

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength() - aMessage.GetOffset());
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));

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

    messageInfo.SetPeerAddr(mNetif.GetMle().GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(borderAgentRloc);
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP(GetInstance(), "Sent relay rx");

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExitErr(error);
}

void JoinerRouter::HandleRelayTransmit(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                       const otMessageInfo *aMessageInfo)
{
    static_cast<JoinerRouter *>(aContext)->HandleRelayTransmit(
        *static_cast<Coap::Header *>(aHeader), *static_cast<Message *>(aMessage),
        *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void JoinerRouter::HandleRelayTransmit(Coap::Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;
    JoinerUdpPortTlv joinerPort;
    JoinerIidTlv joinerIid;
    JoinerRouterKekTlv kek;
    uint16_t offset;
    uint16_t length;
    Message *message = NULL;
    Ip6::MessageInfo messageInfo;

    otLogFuncEntry();
    VerifyOrExit(aHeader.GetType() == kCoapTypeNonConfirmable &&
                 aHeader.GetCode() == kCoapRequestPost, error = kThreadError_Drop);

    otLogInfoMeshCoP(GetInstance(), "Received relay transmit");

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerUdpPort, sizeof(joinerPort), joinerPort));
    VerifyOrExit(joinerPort.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerIid, sizeof(joinerIid), joinerIid));
    VerifyOrExit(joinerIid.IsValid(), error = kThreadError_Parse);

    SuccessOrExit(error = Tlv::GetValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));

    VerifyOrExit((message = mSocket.NewMessage(0)) != NULL, error = kThreadError_NoBufs);
    message->SetPriority(kMeshCoPMessagePriority);
    message->SetLinkSecurityEnabled(false);

    while (length)
    {
        uint16_t copyLength = length;
        uint8_t tmp[16];

        if (copyLength >= sizeof(tmp))
        {
            copyLength = sizeof(tmp);
        }

        aMessage.Read(offset, copyLength, tmp);
        SuccessOrExit(error = message->Append(tmp, copyLength));

        offset += copyLength;
        length -= copyLength;
    }

    aMessage.CopyTo(offset, 0, length, *message);

    messageInfo.mPeerAddr.mFields.m16[0] = HostSwap16(0xfe80);
    memcpy(messageInfo.mPeerAddr.mFields.m8 + 8, joinerIid.GetIid(), 8);
    messageInfo.SetPeerPort(joinerPort.GetUdpPort());
    messageInfo.SetInterfaceId(mNetif.GetInterfaceId());

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    if (Tlv::GetTlv(aMessage, Tlv::kJoinerRouterKek, sizeof(kek), kek) == kThreadError_None)
    {
        otLogInfoMeshCoP(GetInstance(), "Received kek");

        DelaySendingJoinerEntrust(messageInfo, kek);
    }

exit:
    (void)aMessageInfo;

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExitErr(error);
}


ThreadError JoinerRouter::DelaySendingJoinerEntrust(const Ip6::MessageInfo &aMessageInfo,
                                                    const JoinerRouterKekTlv &aKek)
{
    ThreadError error;
    Message *message = NULL;
    Coap::Header header;
    Ip6::MessageInfo messageInfo;

    NetworkMasterKeyTlv masterKey;
    MeshLocalPrefixTlv meshLocalPrefix;
    ExtendedPanIdTlv extendedPanId;
    NetworkNameTlv networkName;
    NetworkKeySequenceTlv networkKeySequence;
    Tlv *tlv;

    DelayedJoinEntHeader delayedMessage;

    otLogFuncEntry();

    header.Init(kCoapTypeConfirmable, kCoapRequestPost);
    header.AppendUriPathOptions(OPENTHREAD_URI_JOINER_ENTRUST);
    header.SetPayloadMarker();

    VerifyOrExit((message = NewMeshCoPMessage(mNetif.GetCoapClient(), header)) != NULL,
                 error = kThreadError_NoBufs);
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    masterKey.Init();
    masterKey.SetNetworkMasterKey(mNetif.GetKeyManager().GetMasterKey(NULL));
    SuccessOrExit(error = message->Append(&masterKey, sizeof(masterKey)));

    meshLocalPrefix.Init();
    meshLocalPrefix.SetMeshLocalPrefix(mNetif.GetMle().GetMeshLocalPrefix());
    SuccessOrExit(error = message->Append(&meshLocalPrefix, sizeof(meshLocalPrefix)));

    extendedPanId.Init();
    extendedPanId.SetExtendedPanId(mNetif.GetMac().GetExtendedPanId());
    SuccessOrExit(error = message->Append(&extendedPanId, sizeof(extendedPanId)));

    networkName.Init();
    networkName.SetNetworkName(mNetif.GetMac().GetNetworkName());
    SuccessOrExit(error = message->Append(&networkName, sizeof(Tlv) + networkName.GetLength()));

    if ((tlv = mNetif.GetActiveDataset().GetNetwork().Get(Tlv::kActiveTimestamp)) != NULL)
    {
        SuccessOrExit(error = message->Append(tlv, sizeof(Tlv) + tlv->GetLength()));
    }
    else
    {
        ActiveTimestampTlv activeTimestamp;
        activeTimestamp.Init();
        SuccessOrExit(error = message->Append(&activeTimestamp, sizeof(activeTimestamp)));
    }

    if ((tlv = mNetif.GetActiveDataset().GetNetwork().Get(Tlv::kChannelMask)) != NULL)
    {
        SuccessOrExit(error = message->Append(tlv, sizeof(Tlv) + tlv->GetLength()));
    }
    else
    {
        ChannelMaskTlv channelMask;
        channelMask.Init();
        SuccessOrExit(error = message->Append(&channelMask, sizeof(channelMask)));
    }

    if ((tlv = mNetif.GetActiveDataset().GetNetwork().Get(Tlv::kPSKc)) != NULL)
    {
        SuccessOrExit(error = message->Append(tlv, sizeof(Tlv) + tlv->GetLength()));
    }
    else
    {
        PSKcTlv pskc;
        pskc.Init();
        SuccessOrExit(error = message->Append(&pskc, sizeof(pskc)));
    }

    if ((tlv = mNetif.GetActiveDataset().GetNetwork().Get(Tlv::kSecurityPolicy)) != NULL)
    {
        SuccessOrExit(error = message->Append(tlv, sizeof(Tlv) + tlv->GetLength()));
    }
    else
    {
        SecurityPolicyTlv securityPolicy;
        securityPolicy.Init();
        SuccessOrExit(error = message->Append(&securityPolicy, sizeof(securityPolicy)));
    }

    networkKeySequence.Init();
    networkKeySequence.SetNetworkKeySequence(mNetif.GetKeyManager().GetCurrentKeySequence());
    SuccessOrExit(error = message->Append(&networkKeySequence, networkKeySequence.GetSize()));

    messageInfo = aMessageInfo;
    messageInfo.SetPeerPort(kCoapUdpPort);

    delayedMessage = DelayedJoinEntHeader(Timer::GetNow() + kDelayJoinEnt, messageInfo, aKek.GetKek());
    SuccessOrExit(delayedMessage.AppendTo(*message));
    mDelayedJoinEnts.Enqueue(*message);

    if (!mTimer.IsRunning())
    {
        mTimer.Start(kDelayJoinEnt);
    }

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    otLogFuncExitErr(error);
    return error;
}

void JoinerRouter::HandleTimer(void *aContext)
{
    static_cast<JoinerRouter *>(aContext)->HandleTimer();
}

void JoinerRouter::HandleTimer(void)
{
    SendDelayedJoinerEntrust();
}

void JoinerRouter::SendDelayedJoinerEntrust(void)
{
    DelayedJoinEntHeader delayedJoinEnt;
    Message *message = mDelayedJoinEnts.GetHead();
    uint32_t now = Timer::GetNow();
    Ip6::MessageInfo messageInfo;

    VerifyOrExit(message != NULL);
    VerifyOrExit(!mTimer.IsRunning());

    delayedJoinEnt.ReadFrom(*message);

    // The message can be sent during CoAP transaction if KEK did not change (i.e. retransmission).
    VerifyOrExit(!mExpectJoinEntRsp ||
                 memcmp(mNetif.GetKeyManager().GetKek(), delayedJoinEnt.GetKek(), KeyManager::kMaxKeyLength) == 0);


    if (delayedJoinEnt.IsLater(now))
    {
        mTimer.Start(delayedJoinEnt.GetSendTime() - now);
    }
    else
    {
        mDelayedJoinEnts.Dequeue(*message);

        // Remove the DelayedJoinEntHeader from the message.
        DelayedJoinEntHeader::RemoveFrom(*message);

        // Set KEK to one used for this message.
        mNetif.GetKeyManager().SetKek(delayedJoinEnt.GetKek());

        // Send the message.
        memcpy(&messageInfo, delayedJoinEnt.GetMessageInfo(), sizeof(messageInfo));

        if (SendJoinerEntrust(*message, messageInfo) != kThreadError_None)
        {
            message->Free();
            mTimer.Start(0);
        }
    }

exit:
    return ;
}

ThreadError JoinerRouter::SendJoinerEntrust(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;

    mNetif.GetCoapClient().AbortTransaction(&JoinerRouter::HandleJoinerEntrustResponse, this);

    otLogInfoMeshCoP(GetInstance(), "Sending JOIN_ENT.ntf");
    SuccessOrExit(error = mNetif.GetCoapClient().SendMessage(aMessage, aMessageInfo,
                                                             &JoinerRouter::HandleJoinerEntrustResponse, this));

    otLogInfoMeshCoP(GetInstance(), "Sent joiner entrust length = %d", aMessage.GetLength());
    otLogCertMeshCoP(GetInstance(), "[THCI] direction=send | type=JOIN_ENT.ntf");

    mExpectJoinEntRsp = true;

exit:
    return error;
}

void JoinerRouter::HandleJoinerEntrustResponse(void *aContext, otCoapHeader *aHeader, otMessage *aMessage,
                                               const otMessageInfo *aMessageInfo, ThreadError aResult)
{
    static_cast<JoinerRouter *>(aContext)->HandleJoinerEntrustResponse(static_cast<Coap::Header *>(aHeader),
                                                                       static_cast<Message *>(aMessage),
                                                                       static_cast<const Ip6::MessageInfo *>(aMessageInfo),
                                                                       aResult);
}

void JoinerRouter::HandleJoinerEntrustResponse(Coap::Header *aHeader, Message *aMessage,
                                               const Ip6::MessageInfo *aMessageInfo, ThreadError aResult)
{
    (void)aMessageInfo;

    mExpectJoinEntRsp = false;
    SendDelayedJoinerEntrust();

    VerifyOrExit(aResult == kThreadError_None && aHeader != NULL && aMessage != NULL);

    VerifyOrExit(aHeader->GetCode() == kCoapResponseChanged);

    otLogInfoMeshCoP(GetInstance(), "Receive joiner entrust response");
    otLogCertMeshCoP(GetInstance(), "[THCI] direction=recv | type=JOIN_ENT.rsp");

exit:
    return ;
}

}  // namespace Dtls
}  // namespace ot

#endif // OPENTHREAD_FTD

