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

#include "joiner_router.hpp"

#if OPENTHREAD_FTD

#include <stdio.h>

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "meshcop/meshcop.hpp"
#include "meshcop/meshcop_tlvs.hpp"
#include "thread/mle.hpp"
#include "thread/thread_netif.hpp"
#include "thread/uri_paths.hpp"

namespace ot {
namespace MeshCoP {

RegisterLogModule("JoinerRouter");

JoinerRouter::JoinerRouter(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSocket(aInstance)
    , mRelayTransmit(UriPath::kRelayTx, &JoinerRouter::HandleRelayTransmit, this)
    , mTimer(aInstance, JoinerRouter::HandleTimer)
    , mJoinerUdpPort(0)
    , mIsJoinerPortConfigured(false)
{
    Get<Tmf::Agent>().AddResource(mRelayTransmit);
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
    VerifyOrExit(Get<Mle::MleRouter>().IsFullThreadDevice());

    if (Get<NetworkData::Leader>().IsJoiningEnabled())
    {
        uint16_t port = GetJoinerUdpPort();

        VerifyOrExit(!mSocket.IsBound());

        IgnoreError(mSocket.Open(&JoinerRouter::HandleUdpReceive, this));
        IgnoreError(mSocket.Bind(port));
        IgnoreError(Get<Ip6::Filter>().AddUnsecurePort(port));
        LogInfo("Joiner Router: start");
    }
    else
    {
        VerifyOrExit(mSocket.IsBound());

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

    joinerUdpPort = As<JoinerUdpPortTlv>(Get<NetworkData::Leader>().GetCommissioningDataSubTlv(Tlv::kJoinerUdpPort));
    VerifyOrExit(joinerUdpPort != nullptr);

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
    static_cast<JoinerRouter *>(aContext)->HandleUdpReceive(AsCoreType(aMessage), AsCoreType(aMessageInfo));
}

void JoinerRouter::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error            error;
    Coap::Message *  message = nullptr;
    Tmf::MessageInfo messageInfo(GetInstance());
    ExtendedTlv      tlv;
    uint16_t         borderAgentRloc;
    uint16_t         offset;

    LogInfo("JoinerRouter::HandleUdpReceive");

    SuccessOrExit(error = GetBorderAgentRloc(Get<ThreadNetif>(), borderAgentRloc));

    message = Get<Tmf::Agent>().NewPriorityNonConfirmablePostMessage(UriPath::kRelayRx);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Tlv::Append<JoinerUdpPortTlv>(*message, aMessageInfo.GetPeerPort()));
    SuccessOrExit(error = Tlv::Append<JoinerIidTlv>(*message, aMessageInfo.GetPeerAddr().GetIid()));
    SuccessOrExit(error = Tlv::Append<JoinerRouterLocatorTlv>(*message, Get<Mle::MleRouter>().GetRloc16()));

    tlv.SetType(Tlv::kJoinerDtlsEncapsulation);
    tlv.SetLength(aMessage.GetLength() - aMessage.GetOffset());
    SuccessOrExit(error = message->Append(tlv));
    offset = message->GetLength();
    SuccessOrExit(error = message->SetLength(offset + tlv.GetLength()));
    aMessage.CopyTo(aMessage.GetOffset(), offset, tlv.GetLength(), *message);

    messageInfo.SetSockAddrToRlocPeerAddrTo(borderAgentRloc);

    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, messageInfo));

    LogInfo("Sent relay rx");

exit:
    FreeMessageOnError(message, error);
}

void JoinerRouter::HandleRelayTransmit(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<JoinerRouter *>(aContext)->HandleRelayTransmit(AsCoapMessage(aMessage), AsCoreType(aMessageInfo));
}

void JoinerRouter::HandleRelayTransmit(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error                    error;
    uint16_t                 joinerPort;
    Ip6::InterfaceIdentifier joinerIid;
    Kek                      kek;
    uint16_t                 offset;
    uint16_t                 length;
    Message *                message = nullptr;
    Message::Settings        settings(Message::kNoLinkSecurity, Message::kPriorityNet);
    Ip6::MessageInfo         messageInfo;

    VerifyOrExit(aMessage.IsNonConfirmablePostRequest(), error = kErrorDrop);

    LogInfo("Received relay transmit");

    SuccessOrExit(error = Tlv::Find<JoinerUdpPortTlv>(aMessage, joinerPort));
    SuccessOrExit(error = Tlv::Find<JoinerIidTlv>(aMessage, joinerIid));

    SuccessOrExit(error = Tlv::FindTlvValueOffset(aMessage, Tlv::kJoinerDtlsEncapsulation, offset, length));

    VerifyOrExit((message = mSocket.NewMessage(0, settings)) != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->SetLength(length));
    aMessage.CopyTo(offset, 0, length, *message);

    messageInfo.GetPeerAddr().SetToLinkLocalAddress(joinerIid);
    messageInfo.SetPeerPort(joinerPort);

    SuccessOrExit(error = mSocket.SendTo(*message, messageInfo));

    if (Tlv::Find<JoinerRouterKekTlv>(aMessage, kek) == kErrorNone)
    {
        LogInfo("Received kek");

        DelaySendingJoinerEntrust(messageInfo, kek);
    }

exit:
    FreeMessageOnError(message, error);
}

void JoinerRouter::DelaySendingJoinerEntrust(const Ip6::MessageInfo &aMessageInfo, const Kek &aKek)
{
    Error                 error   = kErrorNone;
    Message *             message = Get<MessagePool>().Allocate(Message::kTypeOther);
    JoinerEntrustMetadata metadata;

    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

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
    aTimer.Get<JoinerRouter>().HandleTimer();
}

void JoinerRouter::HandleTimer(void)
{
    SendDelayedJoinerEntrust();
}

void JoinerRouter::SendDelayedJoinerEntrust(void)
{
    JoinerEntrustMetadata metadata;
    Message *             message = mDelayedJoinEnts.GetHead();

    VerifyOrExit(message != nullptr);
    VerifyOrExit(!mTimer.IsRunning());

    metadata.ReadFrom(*message);

    if (TimerMilli::GetNow() < metadata.mSendTime)
    {
        mTimer.FireAt(metadata.mSendTime);
    }
    else
    {
        mDelayedJoinEnts.DequeueAndFree(*message);

        Get<KeyManager>().SetKek(metadata.mKek);

        if (SendJoinerEntrust(metadata.mMessageInfo) != kErrorNone)
        {
            mTimer.Start(0);
        }
    }

exit:
    return;
}

Error JoinerRouter::SendJoinerEntrust(const Ip6::MessageInfo &aMessageInfo)
{
    Error          error = kErrorNone;
    Coap::Message *message;

    message = PrepareJoinerEntrustMessage();
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    IgnoreError(Get<Tmf::Agent>().AbortTransaction(&JoinerRouter::HandleJoinerEntrustResponse, this));

    LogInfo("Sending JOIN_ENT.ntf");
    SuccessOrExit(error = Get<Tmf::Agent>().SendMessage(*message, aMessageInfo,
                                                        &JoinerRouter::HandleJoinerEntrustResponse, this));

    LogInfo("Sent joiner entrust length = %d", message->GetLength());
    LogCert("[THCI] direction=send | type=JOIN_ENT.ntf");

exit:
    FreeMessageOnError(message, error);
    return error;
}

Coap::Message *JoinerRouter::PrepareJoinerEntrustMessage(void)
{
    Error          error;
    Coap::Message *message = nullptr;
    Dataset        dataset;
    NetworkNameTlv networkName;
    const Tlv *    tlv;
    NetworkKey     networkKey;

    message = Get<Tmf::Agent>().NewPriorityConfirmablePostMessage(UriPath::kJoinerEntrust);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    message->SetSubType(Message::kSubTypeJoinerEntrust);

    Get<KeyManager>().GetNetworkKey(networkKey);
    SuccessOrExit(error = Tlv::Append<NetworkKeyTlv>(*message, networkKey));
    SuccessOrExit(error = Tlv::Append<MeshLocalPrefixTlv>(*message, Get<Mle::MleRouter>().GetMeshLocalPrefix()));
    SuccessOrExit(error = Tlv::Append<ExtendedPanIdTlv>(*message, Get<ExtendedPanIdManager>().GetExtPanId()));

    networkName.Init();
    networkName.SetNetworkName(Get<NetworkNameManager>().GetNetworkName().GetAsData());
    SuccessOrExit(error = networkName.AppendTo(*message));

    IgnoreError(Get<ActiveDatasetManager>().Read(dataset));

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

    SuccessOrExit(error = Tlv::Append<NetworkKeySequenceTlv>(*message, Get<KeyManager>().GetCurrentKeySequence()));

exit:
    FreeAndNullMessageOnError(message, error);
    return message;
}

void JoinerRouter::HandleJoinerEntrustResponse(void *               aContext,
                                               otMessage *          aMessage,
                                               const otMessageInfo *aMessageInfo,
                                               Error                aResult)
{
    static_cast<JoinerRouter *>(aContext)->HandleJoinerEntrustResponse(AsCoapMessagePtr(aMessage),
                                                                       AsCoreTypePtr(aMessageInfo), aResult);
}

void JoinerRouter::HandleJoinerEntrustResponse(Coap::Message *         aMessage,
                                               const Ip6::MessageInfo *aMessageInfo,
                                               Error                   aResult)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    SendDelayedJoinerEntrust();

    VerifyOrExit(aResult == kErrorNone && aMessage != nullptr);

    VerifyOrExit(aMessage->GetCode() == Coap::kCodeChanged);

    LogInfo("Receive joiner entrust response");
    LogCert("[THCI] direction=recv | type=JOIN_ENT.rsp");

exit:
    return;
}

void JoinerRouter::JoinerEntrustMetadata::ReadFrom(const Message &aMessage)
{
    uint16_t length = aMessage.GetLength();

    OT_ASSERT(length >= sizeof(*this));
    IgnoreError(aMessage.Read(length - sizeof(*this), *this));
}

} // namespace MeshCoP
} // namespace ot

#endif // OPENTHREAD_FTD
