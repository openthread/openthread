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
#include "thread/uri_paths.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace MeshCoP {

JoinerRouter::JoinerRouter(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance)
    , mRelayTransmit(UriPath::kRelayTx, &JoinerRouter::HandleRelayTransmit, this)
    , mTimer(aInstance, JoinerRouter::HandleTimer, this)
    , mJoinerUdpPort(0)
    , mIsJoinerPortConfigured(false)
{
    Get<Tmf::TmfAgent>().AddResource(mRelayTransmit);
}

void JoinerRouter::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        Start();
    }
}

void JoinerRouter::Start(void)
{
    VerifyOrExit(Get<Mle::MleRouter>().IsFullThreadDevice(), OT_NOOP);

    if (Get<NetworkData::Leader>().IsJoiningEnabled())
    {
        uint16_t port = GetJoinerUdpPort();

        VerifyOrExit(!mSocket.IsBound(), OT_NOOP);

        IgnoreError(mSocket.Open(&JoinerRouter::HandleUdpReceive, this));
        IgnoreError(mSocket.Bind(port));
        IgnoreError(Get<Ip6::Filter>().AddUnsecurePort(port));
        otLogInfoMeshCoP("Joiner Router: start");
    }
    else
    {
        VerifyOrExit(mSocket.IsBound(), OT_NOOP);

        IgnoreError(Get<Ip6::Filter>().RemoveUnsecurePort(mSocket.GetSockName().mPort));

        IgnoreError(mSocket.Close());
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
    VerifyOrExit(joinerUdpPort != nullptr, OT_NOOP);

    rval = joinerUdpPort->GetUdpPort();

exit:
    return rval;
}

void JoinerRouter::SetJoinerUdpPort(uint16_t aJoinerUdpPort)
{
    mJoinerUdpPort          = aJoinerUdpPort;
    mIsJoinerPortConfigured = true;
    Start();
}

void JoinerRouter::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<JoinerRouter *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                            *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void JoinerRouter::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError          error;
    Coap::Message *  message = nullptr;
    Ip6::MessageInfo messageInfo;
    ExtendedTlv      tlv;
    uint16_t         borderAgentRloc;
    uint16_t         offset;

    otLogInfoMeshCoP("JoinerRouter::HandleUdpReceive");

    SuccessOrExit(error = GetBorderAgentRloc(Get<ThreadNetif>(), borderAgentRloc));

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->InitAsNonConfirmablePost(UriPath::kRelayRx));
    SuccessOrExit(error = message->SetPayloadMarker());

    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kJoinerUdpPort, aMessageInfo.GetPeerPort()));
    SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kJoinerIid, &aMessageInfo.GetPeerAddr().GetIid(),
                                         Ip6::InterfaceIdentifier::kSize));
    SuccessOrExit(error = Tlv::AppendUint16Tlv(*message, Tlv::kJoinerRouterLocator, Get<Mle::MleRouter>().GetRloc16()));

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength() - aMessage.GetOffset());
    SuccessOrExit(error = message->Append(&tlv, sizeof(tlv)));
    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + tlv.GetLength()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, tlv.GetLength(), *message);

    messageInfo.SetSockAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.SetPeerAddr(Get<Mle::MleRouter>().GetMeshLocal16());
    messageInfo.GetPeerAddr().GetIid().SetLocator(borderAgentRloc);
    messageInfo.SetPeerPort(Tmf::kUdpPort);

    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, messageInfo));

    otLogInfoMeshCoP("Sent relay rx");

exit:
    FreeMessageOnError(message, error);
}

void JoinerRouter::HandleRelayTransmit(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<JoinerRouter *>(aContext)->HandleRelayTransmit(*static_cast<Coap::Message *>(aMessage),
                                                               *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void JoinerRouter::HandleRelayTransmit(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError                  error;
    uint16_t                 joinerPort;
    Ip6::InterfaceIdentifier joinerIid;
    Kek                      kek;
    uint16_t                 offset;
    uint16_t                 length;
    Message *                message = nullptr;
    Message::Settings        settings(Message::kNoLinkSecurity, Message::kPriorityNet);
    Ip6::MessageInfo         messageInfo;

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), error = OT_ERROR_DROP);

    otLogInfoMeshCoP("Received relay transmit");

    SuccessOrExit(error = Tlv::FindUint16Tlv(aMessage, Tlv::kJoinerUdpPort, joinerPort));
    SuccessOrExit(error = Tlv::FindTlv(aMessage, Tlv::kJoinerIid, &joinerIid, sizeof(joinerIid)));

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));

    VerifyOrExit((message = mSocket.NewMessage(0, settings)) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = message->SetLength(length));
    aMessage.CopyTo(offset, 0, length, *message);

    messageInfo.GetPeerAddr().SetToLinkLocalAddress(joinerIid);
    messageInfo.SetPeerPort(joinerPort);

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    if (Tlv::FindTlv(aMessage, Tlv::kJoinerRouterKek, &kek, sizeof(kek)) == OT_ERROR_NONE)
    {
        otLogInfoMeshCoP("Received kek");

        DelaySendingJoinerEntrust(messageInfo, kek);
    }

exit:
    FreeMessageOnError(message, error);
}

void JoinerRouter::DelaySendingJoinerEntrust(const Ip6::MessageInfo &aMessageInfo, const Kek &aKek)
{
    otError               error   = OT_ERROR_NONE;
    Message *             message = Get<MessagePool>().New(Message::kTypeOther, 0);
    JoinerEntrustMetadata metadata;

    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    metadata.mMessageInfo = aMessageInfo;
    metadata.mMessageInfo.SetPeerPort(Tmf::kUdpPort);
    metadata.mSendTime = TimerMilli::GetNow() + kJoinerEntrustTxDelay;
    metadata.mKek      = aKek;

    SuccessOrExit(error = metadata.AppendTo(*message));

    mDelayedJoinEnts.Enqueue(*message);

    if (!mTimer.IsRunning())
    {
        mTimer.FireAt(metadata.mSendTime);
    }

exit:
    FreeMessageOnError(message, error);
    LogError("schedule joiner entrust", error);
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

    VerifyOrExit(message != nullptr, OT_NOOP);
    VerifyOrExit(!mTimer.IsRunning(), OT_NOOP);

    metadata.ReadFrom(*message);

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
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    IgnoreError(Get<Tmf::TmfAgent>().AbortTransaction(&JoinerRouter::HandleJoinerEntrustResponse, this));

    otLogInfoMeshCoP("Sending JOIN_ENT.ntf");
    SuccessOrExit(error = Get<Tmf::TmfAgent>().SendMessage(*message, aMessageInfo,
                                                           &JoinerRouter::HandleJoinerEntrustResponse, this));

    otLogInfoMeshCoP("Sent joiner entrust length = %d", message->GetLength());
    otLogCertMeshCoP("[THCI] direction=send | type=JOIN_ENT.ntf");

exit:
    FreeMessageOnError(message, error);
    return error;
}

Coap::Message *JoinerRouter::PrepareJoinerEntrustMessage(void)
{
    otError        error;
    Coap::Message *message = nullptr;
    Dataset        dataset(Dataset::kActive);

    NetworkNameTlv networkName;
    const Tlv *    tlv;

    VerifyOrExit((message = NewMeshCoPMessage(Get<Tmf::TmfAgent>())) != nullptr, error = OT_ERROR_NO_BUFS);

    message->InitAsConfirmablePost();
    SuccessOrExit(error = message->AppendUriPathOptions(UriPath::kJoinerEntrust));
    SuccessOrExit(error = message->SetPayloadMarker());
    message->SetSubType(Message::kSubTypeJoinerEntrust);

    SuccessOrExit(
        error = Tlv::AppendTlv(*message, Tlv::kNetworkMasterKey, &Get<KeyManager>().GetMasterKey(), sizeof(MasterKey)));

    SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kMeshLocalPrefix, &Get<Mle::MleRouter>().GetMeshLocalPrefix(),
                                         sizeof(otMeshLocalPrefix)));

    SuccessOrExit(error = Tlv::AppendTlv(*message, Tlv::kExtendedPanId, &Get<Mac::Mac>().GetExtendedPanId(),
                                         sizeof(Mac::ExtendedPanId)));

    networkName.Init();
    networkName.SetNetworkName(Get<Mac::Mac>().GetNetworkName().GetAsData());
    SuccessOrExit(error = networkName.AppendTo(*message));

    IgnoreError(Get<ActiveDataset>().Read(dataset));

    if ((tlv = dataset.GetTlv<ActiveTimestampTlv>()) != nullptr)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        ActiveTimestampTlv activeTimestamp;
        activeTimestamp.Init();
        SuccessOrExit(error = activeTimestamp.AppendTo(*message));
    }

    if ((tlv = dataset.GetTlv<ChannelMaskTlv>()) != nullptr)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        ChannelMaskBaseTlv channelMask;
        channelMask.Init();
        SuccessOrExit(error = channelMask.AppendTo(*message));
    }

    if ((tlv = dataset.GetTlv<PskcTlv>()) != nullptr)
    {
        SuccessOrExit(error = tlv->AppendTo(*message));
    }
    else
    {
        PskcTlv pskc;
        pskc.Init();
        SuccessOrExit(error = pskc.AppendTo(*message));
    }

    if ((tlv = dataset.GetTlv<SecurityPolicyTlv>()) != nullptr)
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
    FreeAndNullMessageOnError(message, error);
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

    SendDelayedJoinerEntrust();

    VerifyOrExit(aResult == OT_ERROR_NONE && aMessage != nullptr, OT_NOOP);

    VerifyOrExit(aMessage->GetCode() == Coap::kCodeChanged, OT_NOOP);

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
