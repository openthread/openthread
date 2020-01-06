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

#include "joiner_router.hpp"

#include <stdio.h>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/mle.hpp"
#include "thread/thread_netif.hpp"
#include "thread/thread_uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace MeshCoP {

JoinerRouter::JoinerRouter(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance.Get<Ip6::Udp>())
    , mRelayTransmit(OT_URI_PATH_RELAY_TX, &JoinerRouter::HandleRelayTransmit, this)
    , mTimer(aInstance, &JoinerRouter::HandleTimer, this)
    , mNotifierCallback(aInstance, &JoinerRouter::HandleStateChanged, this)
    , mJoinerUdpPort(0)
    , mIsJoinerPortConfigured(false)
    , mExpectJoinEntRsp(false)
{
    Get<Coap::Coap>().AddResource(mRelayTransmit);
}

void JoinerRouter::HandleStateChanged(Notifier::Callback &aCallback, otChangedFlags aFlags)
{
    aCallback.GetOwner<JoinerRouter>().HandleStateChanged(aFlags);
}

void JoinerRouter::HandleStateChanged(otChangedFlags aFlags)
{
    VerifyOrExit(Get<Mle::MleRouter>().IsFullThreadDevice());
    VerifyOrExit(aFlags & OT_CHANGED_THREAD_NETDATA);

    if (Get<NetworkData::Leader>().IsJoiningEnabled())
    {
        Ip6::SockAddr sockaddr;

        VerifyOrExit(!mSocket.IsBound());

        sockaddr.mPort = GetJoinerUdpPort();

        mSocket.Open(&JoinerRouter::HandleUdpReceive, this);
        mSocket.Bind(sockaddr);
        Get<Ip6::Filter>().AddUnsecurePort(sockaddr.mPort);
        otLogInfoMeshCoP("Joiner Router: start");
    }
    else
    {
        Get<Ip6::Filter>().RemoveUnsecurePort(mSocket.GetSockName().mPort);

        mSocket.Close();
    }

exit:
    return;
}

uint16_t JoinerRouter::GetJoinerUdpPort(void)
{
    uint16_t          rval = OPENTHREAD_CONFIG_JOINER_UDP_PORT;
    JoinerUdpPortTlv *joinerUdpPort;

    VerifyOrExit(!mIsJoinerPortConfigured, rval = mJoinerUdpPort);

    joinerUdpPort =
        static_cast<JoinerUdpPortTlv *>(Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kJoinerUdpPort));
    VerifyOrExit(joinerUdpPort != NULL);

    rval = joinerUdpPort->GetUdpPort();

exit:
    return rval;
}

void JoinerRouter::SetJoinerUdpPort(uint16_t aJoinerUdpPort)
{
    mJoinerUdpPort          = aJoinerUdpPort;
    mIsJoinerPortConfigured = true;
    HandleStateChanged(OT_CHANGED_THREAD_NETDATA);
}

void JoinerRouter::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<JoinerRouter *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void JoinerRouter::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                error;
    Coap::Message *        message = NULL;
    Ip6::MessageInfo       messageInfo;
    JoinerUdpPortTlv       udpPort;
    JoinerIidTlv           iid;
    JoinerRouterLocatorTlv rloc;
    ExtendedTlv            tlv;
    uint16_t               borderAgentRloc;
    uint16_t               offset;

    otLogInfoMeshCoP("JoinerRouter::HandleUdpReceive");

    SuccessOrExit(error = GetBorderAgentRloc(Get<ThreadNetif>(), borderAgentRloc));

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_RELAY_RX));
    SuccessOrExit(error = message->SetPayloadMarker());

    udpPort.Init();
    udpPort.SetUdpPort(aMessageInfo.GetPeerPort());
    SuccessOrExit(error = udpPort.AppendTo(*message));

    iid.Init();
    iid.SetIid(aMessageInfo.GetPeerAddr().mFields.m8 + 8);
    SuccessOrExit(error = iid.AppendTo(*message));

    rloc.Init();
    rloc.SetJoinerRouterLocator(Get<Mle::MleRouter>().GetRloc16());
    SuccessOrExit(error = rloc.AppendTo(*message));

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength() - aMessage.GetOffset());
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + tlv.GetLength()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, tlv.GetLength(), *message);

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.GetPeerAddr().mFields.m16[7] = HostSwap16(borderAgentRloc);
    messageInfo.SetPeerPort(kCoapUdpPort);

    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("Sent relay rx");

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

void JoinerRouter::HandleRelayTransmit(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<JoinerRouter *>(aContext)->HandleRelayTransmit(*static_cast<Coap::Message *>(aMessage),
                                                               *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void JoinerRouter::HandleRelayTransmit(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError            error;
    JoinerUdpPortTlv   joinerPort;
    JoinerIidTlv       joinerIid;
    JoinerRouterKekTlv kek;
    uint16_t           offset;
    uint16_t           length;
    Message *          message  = NULL;
    otMessageSettings  settings = {false, static_cast<otMessagePriority>(kMeshCoPMessagePriority)};
    Ip6::MessageInfo   messageInfo;

    VerifyOrExit(aMessage.GetType() == OT_COAP_TYPE_NON_CONFIRMABLE && aMessage.GetCode() == OT_COAP_CODE_POST,
                 error = OT_ERROR_DROP);

    otLogInfoMeshCoP("Received relay transmit");

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerUdpPort, sizeof(joinerPort), joinerPort));
    VerifyOrExit(joinerPort.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetTlv(aMessage, Tlv::kJoinerIid, sizeof(joinerIid), joinerIid));
    VerifyOrExit(joinerIid.IsValid(), error = OT_ERROR_PARSE);

    SuccessOrExit(error = Tlv::GetValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));

    VerifyOrExit((message = mSocket.NewMessage(0, &settings)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetLength(length));
    aMessage.CopyTo(offset, 0, length, *message);

    messageInfo.mPeerAddr.mFields.m16[0] = HostSwap16(0xfe80);
    memcpy(messageInfo.mPeerAddr.mFields.m8 + 8, joinerIid.GetIid(), 8);
    messageInfo.SetPeerPort(joinerPort.GetUdpPort());

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    if (Tlv::GetTlv(aMessage, Tlv::kJoinerRouterKek, sizeof(kek), kek) == OT_ERROR_NONE)
    {
        otLogInfoMeshCoP("Received kek");

        DelaySendingJoinerEntrust(messageInfo, kek);
    }

exit:
    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }
}

otError JoinerRouter::DelaySendingJoinerEntrust(const Ip6::MessageInfo &aMessageInfo, const JoinerRouterKekTlv &aKek)
{
    otError          error;
    Coap::Message *  message = NULL;
    Ip6::MessageInfo messageInfo;
    Dataset          dataset(MeshCoP::Tlv::kActiveTimestamp);

    NetworkMasterKeyTlv   masterKey;
    MeshLocalPrefixTlv    meshLocalPrefix;
    ExtendedPanIdTlv      extendedPanId;
    NetworkNameTlv        networkName;
    NetworkKeySequenceTlv networkKeySequence;
    const Tlv *           tlv;

    DelayedJoinEntHeader delayedMessage;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_JOINER_ENTRUST));
    SuccessOrExit(error = message->SetPayloadMarker());
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    masterKey.Init();
    masterKey.SetNetworkMasterKey(Get<KeyManager>().GetMasterKey());
    SuccessOrExit(error = masterKey.AppendTo(*message));

    meshLocalPrefix.Init();
    meshLocalPrefix.SetMeshLocalPrefix(Get<Mle::MleRouter>().GetMeshLocalPrefix());
    SuccessOrExit(error = meshLocalPrefix.AppendTo(*message));

    extendedPanId.Init();
    extendedPanId.SetExtendedPanId(Get<Mac::Mac>().GetExtendedPanId());
    SuccessOrExit(error = extendedPanId.AppendTo(*message));

    networkName.Init();
    networkName.SetNetworkName(Get<Mac::Mac>().GetNetworkName().GetAsData());
    SuccessOrExit(error = networkName.AppendTo(*message));

    Get<ActiveDataset>().Read(dataset);

    if ((tlv = dataset.Get(Tlv::kActiveTimestamp)) != NULL)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        ActiveTimestampTlv activeTimestamp;
        activeTimestamp.Init();
        SuccessOrExit(error = activeTimestamp.AppendTo(*message));
    }

    if ((tlv = dataset.Get(Tlv::kChannelMask)) != NULL)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        ChannelMaskBaseTlv channelMask;
        channelMask.Init();
        SuccessOrExit(error = channelMask.AppendTo(*message));
    }

    if ((tlv = dataset.Get(Tlv::kPskc)) != NULL)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        PskcTlv pskc;
        pskc.Init();
        SuccessOrExit(error = pskc.AppendTo(*message));
    }

    if ((tlv = dataset.Get(Tlv::kSecurityPolicy)) != NULL)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        SecurityPolicyTlv securityPolicy;
        securityPolicy.Init();
        SuccessOrExit(error = securityPolicy.AppendTo(*message));
    }

    networkKeySequence.Init();
    networkKeySequence.SetNetworkKeySequence(Get<KeyManager>().GetCurrentKeySequence());
    SuccessOrExit(error = networkKeySequence.AppendTo(*message));

    messageInfo = aMessageInfo;
    messageInfo.SetPeerPort(kCoapUdpPort);

    delayedMessage.Init(TimerMilli::GetNow() + kDelayJoinEnt, messageInfo, aKek.GetKek());
    SuccessOrExit(error = delayedMessage.AppendTo(*message));

    mDelayedJoinEnts.Enqueue(*message);

    if (!mTimer.IsRunning())
    {
        mTimer.Start(kDelayJoinEnt);
    }

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void JoinerRouter::HandleTimer(Timer &aTimer)
{
    aTimer.GetOwner<JoinerRouter>().HandleTimer();
}

void JoinerRouter::HandleTimer(void)
{
    SendDelayedJoinerEntrust();
}

void JoinerRouter::SendDelayedJoinerEntrust(void)
{
    DelayedJoinEntHeader delayedJoinEnt;
    Coap::Message *      message = static_cast<Coap::Message *>(mDelayedJoinEnts.GetHead());
    Ip6::MessageInfo     messageInfo;

    VerifyOrExit(message != NULL);
    VerifyOrExit(!mTimer.IsRunning());

    delayedJoinEnt.ReadFrom(*message);

    // The message can be sent during CoAP transaction if KEK did not change (i.e. retransmission).
    VerifyOrExit(!mExpectJoinEntRsp ||
                 memcmp(Get<KeyManager>().GetKek(), delayedJoinEnt.GetKek(), KeyManager::kMaxKeyLength) == 0);

    if (TimerMilli::GetNow() < delayedJoinEnt.GetSendTime())
    {
        mTimer.FireAt(delayedJoinEnt.GetSendTime());
    }
    else
    {
        mDelayedJoinEnts.Dequeue(*message);

        // Remove the DelayedJoinEntHeader from the message.
        DelayedJoinEntHeader::RemoveFrom(*message);

        // Set KEK to one used for this message.
        Get<KeyManager>().SetKek(delayedJoinEnt.GetKek());

        // Send the message.
        memcpy(&messageInfo, delayedJoinEnt.GetMessageInfo(), sizeof(messageInfo));

        if (SendJoinerEntrust(*message, messageInfo) != OT_ERROR_NONE)
        {
            message->Free();
            mTimer.Start(0);
        }
    }

exit:
    return;
}

otError JoinerRouter::SendJoinerEntrust(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error;

    Get<Coap::Coap>().AbortTransaction(&JoinerRouter::HandleJoinerEntrustResponse, this);

    otLogInfoMeshCoP("Sending JOIN_ENT.ntf");
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(aMessage, aMessageInfo,
                                                        &JoinerRouter::HandleJoinerEntrustResponse, this));

    otLogInfoMeshCoP("Sent joiner entrust length = %d", aMessage.GetLength());
    otLogCertMeshCoP("[THCI] direction=send | type=JOIN_ENT.ntf");

    mExpectJoinEntRsp = true;

exit:
    return error;
}

void JoinerRouter::HandleJoinerEntrustResponse(void *               aContext,
                                               otMessage *          aMessage,
                                               const otMessageInfo *aMessageInfo,
                                               otError              aResult)
{
    static_cast<JoinerRouter *>(aContext)->HandleJoinerEntrustResponse(
        static_cast<Coap::Message *>(aMessage), static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
}

void JoinerRouter::HandleJoinerEntrustResponse(Coap::Message *         aMessage,
                                               const Ip6::MessageInfo *aMessageInfo,
                                               otError                 aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    mExpectJoinEntRsp = false;
    SendDelayedJoinerEntrust();

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage != NULL);

    VerifyOrExit(aMessage->GetCode() == OT_COAP_CODE_CHANGED);

    otLogInfoMeshCoP("Receive joiner entrust response");
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_ENT.rsp");

exit:
    return;
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
