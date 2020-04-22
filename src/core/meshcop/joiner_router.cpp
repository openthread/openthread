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
    VerifyOrExit(Get<Mle::MleRouter>().IsFullThreadDevice(), OT_NOOP);
    VerifyOrExit(aFlags & OT_CHANGED_THREAD_NETDATA, OT_NOOP);

    if (Get<NetworkData::Leader>().IsJoiningEnabled())
    {
        Ip6::SockAddr sockaddr;

        VerifyOrExit(!mSocket.IsBound(), OT_NOOP);

        sockaddr.mPort = GetJoinerUdpPort();

        mSocket.Open(&JoinerRouter::HandleUdpReceive, this);
        mSocket.Bind(sockaddr);
        Get<Ip6::Filter>().AddUnsecurePort(sockaddr.mPort);
        otLogInfoMeshCoP("Joiner Router: start");
    }
    else
    {
        VerifyOrExit(mSocket.IsBound(), OT_NOOP);

        Get<Ip6::Filter>().RemoveUnsecurePort(mSocket.GetSockName().mPort);

        mSocket.Close();
    }

exit:
    return;
}

uint16_t JoinerRouter::GetJoinerUdpPort(void)
{
    uint16_t                rval = OPENTHREAD_CONFIG_JOINER_UDP_PORT;
    const JoinerUdpPortTlv *joinerUdpPort;

    VerifyOrExit(!mIsJoinerPortConfigured, rval = mJoinerUdpPort);

    joinerUdpPort = static_cast<const JoinerUdpPortTlv *>(
        Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kJoinerUdpPort));
    VerifyOrExit(joinerUdpPort != NULL, OT_NOOP);

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
    otError          error;
    Coap::Message *  message = NULL;
    Ip6::MessageInfo messageInfo;
    ExtendedTlv      tlv;
    uint16_t         borderAgentRloc;
    uint16_t         offset;

    otLogInfoMeshCoP("JoinerRouter::HandleUdpReceive");

    SuccessOrExit(error = GetBorderAgentRloc(Get<ThreadNetif>(), borderAgentRloc));

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->Init(OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_POST, OT_URI_PATH_RELAY_RX));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kJoinerUdpPort, aMessageInfo.GetPeerPort()));
    SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kJoinerIid, aMessageInfo.GetPeerAddr().mFields.m8 + 8,
                                         Ip6::Address::kInterfaceIdentifierSize));
    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kJoinerRouterLocator, Get<Mle::MleRouter>().GetRloc16()));

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength() - aMessage.GetOffset());
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + tlv.GetLength()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, tlv.GetLength(), *message);

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.GetPeerAddr().SetLocator(borderAgentRloc);
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

    otError           error;
    uint16_t          joinerPort;
    uint8_t           joinerIid[Ip6::Address::kInterfaceIdentifierSize];
    Kek               kek;
    uint16_t          offset;
    uint16_t          length;
    Message *         message  = NULL;
    otMessageSettings settings = {false, static_cast<otMessagePriority>(kMeshCoPMessagePriority)};
    Ip6::MessageInfo  messageInfo;

    VerifyOrExit(aMessage.IsNonConfirmable() && aMessage.GetCode() == OT_COAP_CODE_POST, error = OT_ERROR_DROP);

    otLogInfoMeshCoP("Received relay transmit");

    SuccessOrExit(error = Tlv::ReadUint16Tlv(aMessage, Tlv::kJoinerUdpPort, joinerPort));
    SuccessOrExit(error = Tlv::ReadTlv(aMessage, Tlv::kJoinerIid, joinerIid, sizeof(joinerIid)));

    SuccessOrExit(error = Tlv::GetValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));

    VerifyOrExit((message = mSocket.NewMessage(0, &settings)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetLength(length));
    aMessage.CopyTo(offset, 0, length, *message);

    messageInfo.GetPeerAddr().SetToLinkLocalAddress(joinerIid);
    messageInfo.SetPeerPort(joinerPort);

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    if (Tlv::ReadTlv(aMessage, Tlv::kJoinerRouterKek, &kek, sizeof(kek)) == OT_ERROR_NONE)
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

otError JoinerRouter::DelaySendingJoinerEntrust(const Ip6::MessageInfo &aMessageInfo, const Kek &aKek)
{
    otError               error   = OT_ERROR_NONE;
    Message *             message = Get<MessagePool>().New(Message::kTypeOther, 0);
    JoinerEntrustMetadata metadata;

    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    metadata.mMessageInfo = aMessageInfo;
    metadata.mMessageInfo.SetPeerPort(kCoapUdpPort);
    metadata.mSendTime = TimerMilli::GetNow() + kJoinerEntrustTxDelay;
    metadata.mKek      = aKek;

    SuccessOrExit(error = metadata.AppendTo(*message));

    mDelayedJoinEnts.Enqueue(*message);

    if (!mTimer.IsRunning())
    {
        mTimer.FireAt(metadata.mSendTime);
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
    JoinerEntrustMetadata metadata;
    Message *             message = mDelayedJoinEnts.GetHead();

    VerifyOrExit(message != NULL, OT_NOOP);
    VerifyOrExit(!mTimer.IsRunning(), OT_NOOP);

    metadata.ReadFrom(*message);

    // The message can be sent during CoAP transaction if KEK did not
    // change (i.e., retransmission). Otherweise, we wait for Joiner
    // Entrust Response before handling any other pending delayed
    // Jointer Entrust message.
    VerifyOrExit(!mExpectJoinEntRsp || (Get<KeyManager>().GetKek() == metadata.mKek), OT_NOOP);

    if (TimerMilli::GetNow() < metadata.mSendTime)
    {
        mTimer.FireAt(metadata.mSendTime);
    }
    else
    {
        mDelayedJoinEnts.Dequeue(*message);
        message->Free();

        Get<KeyManager>().SetKek(metadata.mKek);

        if (SendJoinerEntrust(metadata.mMessageInfo) != OT_ERROR_NONE)
        {
            mTimer.Start(0);
        }
    }

exit:
    return;
}

otError JoinerRouter::SendJoinerEntrust(const Ip6::MessageInfo &aMessageInfo)
{
    otError        error = OT_ERROR_NONE;
    Coap::Message *message;

    message = PrepareJoinerEntrustMessage();
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    Get<Coap::Coap>().AbortTransaction(&JoinerRouter::HandleJoinerEntrustResponse, this);

    otLogInfoMeshCoP("Sending JOIN_ENT.ntf");
    SuccessOrExit(error = Get<Coap::Coap>().SendMessage(*message, aMessageInfo,
                                                        &JoinerRouter::HandleJoinerEntrustResponse, this));

    otLogInfoMeshCoP("Sent joiner entrust length = %d", message->GetLength());
    otLogCertMeshCoP("[THCI] direction=send | type=JOIN_ENT.ntf");

    mExpectJoinEntRsp = true;

exit:
    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

Coap::Message *JoinerRouter::PrepareJoinerEntrustMessage(void)
{
    otError        error;
    Coap::Message *message = NULL;
    Dataset        dataset(Dataset::kActive);

    NetworkNameTlv networkName;
    const Tlv *    tlv;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Coap::Coap>())) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    SuccessOrExit(error = message->AppendUriPathOptions(OT_URI_PATH_JOINER_ENTRUST));
    SuccessOrExit(error = message->SetPayloadMarker());
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kNetworkMasterKey, Get<KeyManager>().GetMasterKey().m8,
                                         sizeof(MasterKey)));

    SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kMeshLocalPrefix, Get<Mle::MleRouter>().GetMeshLocalPrefix().m8,
                                         sizeof(otMeshLocalPrefix)));

    SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kExtendedPanId, Get<Mac::Mac>().GetExtendedPanId().m8,
                                         sizeof(Mac::ExtendedPanId)));

    networkName.Init();
    networkName.SetNetworkName(Get<Mac::Mac>().GetNetworkName().GetAsData());
    SuccessOrExit(error = networkName.AppendTo(*message));

    Get<ActiveDataset>().Read(dataset);

    if ((tlv = dataset.GetTlv<ActiveTimestampTlv>()) != NULL)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        ActiveTimestampTlv activeTimestamp;
        activeTimestamp.Init();
        SuccessOrExit(error = activeTimestamp.AppendTo(*message));
    }

    if ((tlv = dataset.GetTlv<ChannelMaskTlv>()) != NULL)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        ChannelMaskBaseTlv channelMask;
        channelMask.Init();
        SuccessOrExit(error = channelMask.AppendTo(*message));
    }

    if ((tlv = dataset.GetTlv<PskcTlv>()) != NULL)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        PskcTlv pskc;
        pskc.Init();
        SuccessOrExit(error = pskc.AppendTo(*message));
    }

    if ((tlv = dataset.GetTlv<SecurityPolicyTlv>()) != NULL)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        SecurityPolicyTlv securityPolicy;
        securityPolicy.Init();
        SuccessOrExit(error = securityPolicy.AppendTo(*message));
    }

    SuccessOrExit(
        error = Tlv::AppendUint32Tlv(*message, Tlv::kNetworkKeySequence, Get<KeyManager>().GetCurrentKeySequence()));

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
        message = NULL;
    }

    return message;
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

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage != NULL, OT_NOOP);

    VerifyOrExit(aMessage->GetCode() == OT_COAP_CODE_CHANGED, OT_NOOP);

    otLogInfoMeshCoP("Receive joiner entrust response");
    otLogCertMeshCoP("[THCI] direction=recv | type=JOIN_ENT.rsp");

exit:
    return;
}

void JoinerRouter::JoinerEntrustMetadata::ReadFrom(const Message &aMessage)
{
    uint16_t length = aMessage.GetLength();

    OT_ASSERT(length >= sizeof(*this));
    aMessage.Read(length - sizeof(*this), sizeof(*this), this);
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
