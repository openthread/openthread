/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements MLE functionality required for the peer to peer link.
 */

#include "mle.hpp"
#include "common/code_utils.hpp"

#if OPENTHREAD_CONFIG_PEER_TO_PEER_ENABLE

#include <openthread/platform/toolchain.h>

#include "instance/instance.hpp"
#include "utils/static_counter.hpp"

namespace ot {
namespace Mle {

RegisterLogModule("MlePeer");

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
Error Mle::P2pWakeupAndLink(const P2pRequest &aP2pRequest, P2pLinkDoneCallback aCallback, void *aContext)
{
    Error error;

    VerifyOrExit(aP2pRequest.GetWakeupRequest().IsValid(), error = kErrorInvalidArgs);
    VerifyOrExit(mP2pState == kP2pIdle, error = kErrorInvalidState);
    SuccessOrExit(error =
                      mWakeupTxScheduler.WakeUp(aP2pRequest.GetWakeupRequest(), kWakeupTxInterval, kWakeupMaxDuration));

    mP2pState = kP2pWakingUp;
    mP2pLinkDoneCallback.Set(aCallback, aContext);
    Get<MeshForwarder>().SetRxOnWhenIdle(true);
    mWedAttachTimer.FireAt(mWakeupTxScheduler.GetTxEndTime() + mWakeupTxScheduler.GetConnectionWindowUs());

    LogInfo("Start to connect to %s", aP2pRequest.GetWakeupRequest().ToString().AsCString());

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE

void Mle::HandleWedAttachTimer(void)
{
    switch (mP2pState)
    {
    case kP2pWakingUp:
    case kP2pLinkRequesting:
    {
        if (mP2pState == kP2pWakingUp)
        {
            LogInfo("Connection window closed");
        }
        else
        {
            if (mP2pNumLinksEstablished == 0)
            {
                LogInfo("No P2P link is established");
            }
            else
            {
                LogInfo("At least one P2P link is established");
            }
        }

        mP2pState = kP2pIdle;
        Get<MeshForwarder>().SetRxOnWhenIdle(IsRxOnWhenIdle());
        ClearPeersInLinkRequestState();
        mP2pLinkDoneCallback.InvokeAndClearIfSet();
    }
    break;

    case kP2pLinkAccepting:
        LogInfo("Accept the P2P link is timeout");

        mP2pState = kP2pIdle;
        ClearPeersInLinkRequestState();
        WakeupListenerEnable();
        break;

    case kP2pLinkTearing:
    {
        OT_ASSERT(mP2pPeer != nullptr);

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
        Get<Srp::Client>().P2pSrpClientStop(mP2pPeer->GetExtAddress());
#endif
        mP2pEventCallback.InvokeIfSet(OT_P2P_EVENT_UNLINKED, &mP2pPeer->GetExtAddress());
        // Triger the ChildSupervisor to not send supervision messages to keep the link alive.
        Get<ChildSupervisor>().HandleP2pEvent(OT_P2P_EVENT_UNLINKED);

        mP2pPeer->SetState(Neighbor::kStateInvalid);
        mP2pState = kP2pIdle;

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
        Get<Srp::Server>().HandleP2pEvents(OT_P2P_EVENT_UNLINKED);
#endif
        UpdateCslState();
    }
    break;

    default:
        break;
    }
}

void Mle::WakeupListenerEnable(void)
{
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    // The wake-up listener is disabled after a wake-up frame is received, here enables the wake-up listener again.
    Get<Mac::Mac>().SetWakeupListenEnabled(true);
#endif
}

void Mle::ClearPeersInLinkRequestState(void)
{
    for (Peer &peer : Get<PeerTable>().Iterate(Peer::kInStateLinkRequest))
    {
        peer.SetState(Neighbor::kStateInvalid);
    }
}

bool Mle::HasPeerInLinkRequestState(void) { return Get<PeerTable>().GetNumPeers(Peer::kInStateLinkRequest) > 0; }

void Mle::UpdateCslState(void)
{
#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
    VerifyOrExit(!IsRxOnWhenIdle());
    VerifyOrExit(Get<PeerTable>().GetNumPeers(Peer::kInStateValid) == 0);

    Get<Mac::Mac>().ClearECslPeerAddresses();
    Get<Mac::Mac>().SetECslCapable(false);

exit:
    return;
#endif
}

void Mle::P2pSetEventCallback(otP2pEventCallback aCallback, void *aContext)
{
    mP2pEventCallback.Set(aCallback, aContext);
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
void Mle::HandleServerStateChange(void) { mDelayTimer.Start(kSrpRegisterDelayUs); }

void Mle::HandleDelayTimer(void) { SrpServerUpdate(); }

void Mle::SrpServerUpdate(void)
{
    uint16_t srpServerPort = Get<Srp::Server>().GetPort();

    switch (Get<Srp::Server>().GetState())
    {
    case Srp::Server::kStateDisabled:
        LinkDataUpdate(false, srpServerPort);
        break;

    case Srp::Server::kStateStopped:
        LinkDataUpdate(false, srpServerPort);
        break;

    case Srp::Server::kStateRunning:
        LinkDataUpdate(true, srpServerPort);
        break;
    }
}

void Mle::LinkDataUpdate(bool aSrpServerEnabled, uint16_t aSrpServerPort)
{
    for (Peer &peer : Get<PeerTable>().Iterate(Peer::kInStateValid))
    {
        SendLinkDataUpdate(peer, aSrpServerEnabled, aSrpServerPort);
    }
}

void Mle::SendLinkDataUpdate(Peer &aPeer, bool aIsLocalSrpServer, uint16_t aSrpServerPort)
{
    Error        error = kErrorNone;
    TxMessage   *message;
    Ip6::Address destination;

    destination.Clear();
    destination.SetToLinkLocalAddress(aPeer.GetExtAddress());

    LogInfo("SendLinkDataUpdate");
    VerifyOrExit((message = NewMleMessage(kCommandLinkDataUpdate)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendLinkDataTlv(aIsLocalSrpServer, aSrpServerPort));
    SuccessOrExit(error = message->SendTo(destination));

exit:
    FreeMessageOnError(message, error);
    return;
}
#endif

Peer *Mle::GetPeer(RxInfo &aRxInfo)
{
    Peer           *peer = nullptr;
    Mac::ExtAddress extAddress;

    VerifyOrExit(aRxInfo.mMessageInfo.GetPeerAddr().IsLinkLocalUnicast());
    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(extAddress);
    peer = Get<PeerTable>().FindPeer(extAddress, Peer::kInStateValid);

exit:
    return peer;
}

void Mle::HandleLinkDataUpdate(RxInfo &aRxInfo)
{
    Peer    *peer;
    bool     isLocalSrpServer;
    uint16_t srpServerPort;

    LogInfo("HandleLinkDataUpdate");
    VerifyOrExit(aRxInfo.mMessageInfo.GetPeerAddr().IsLinkLocalUnicast());

    peer = GetPeer(aRxInfo);
    VerifyOrExit(peer != nullptr, LogWarn("no peer was found"));

    ProcessKeySequence(aRxInfo);
    SuccessOrExit(aRxInfo.mMessage.ReadLinkDataTlv(isLocalSrpServer, srpServerPort));

    peer->SetLocalSrpServer(isLocalSrpServer);

#if OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE
    if (isLocalSrpServer)
    {
        LogInfo("Srp client to %s is started", peer->GetExtAddress().ToString().AsCString());
        Get<Srp::Client>().P2pSrpClientStart(peer->GetExtAddress(), srpServerPort);
    }
#endif

exit:
    return;
}

Error Mle::P2pUnlink(const Mac::ExtAddress &aExtAddress)
{
    Error        error = kErrorNone;
    TxMessage   *message;
    Ip6::Address destination;
    uint32_t     delayUs = kMaxP2pKeepAliveBeforeRemovePeer;

    VerifyOrExit(mP2pState == kP2pIdle, error = kErrorBusy);
    mP2pPeer = Get<PeerTable>().FindPeer(aExtAddress, Peer::kInStateAnyExceptInvalid);
    VerifyOrExit(mP2pPeer != nullptr, error = kErrorNotFound);

    destination.Clear();
    destination.SetToLinkLocalAddress(aExtAddress);

    LogInfo("SendP2pLinkTearDown");
    VerifyOrExit((message = NewMleMessage(kCommandLinkTearDown)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->SendTo(destination));
    mP2pState = kP2pLinkTearing;
    mWedAttachTimer.Start(kMaxP2pKeepAliveBeforeRemovePeer);

    if (mP2pPeer->IsCslSynchronized())
    {
        // ensure that the peer won't be removed from the peer table before the LinkTearDown is sent out.
        delayUs = mP2pPeer->GetCslPeriodUs() * (Mac::kDefaultMaxFrameRetriesIndirect + 1);
    }

    mWedAttachTimer.Start(delayUs);

exit:
    return error;
}

void Mle::HandleLinkTearDown(RxInfo &aRxInfo)
{
    LogInfo("HandleLinkTearDown");

    VerifyOrExit(aRxInfo.mMessageInfo.GetPeerAddr().IsLinkLocalUnicast());

    mP2pPeer = GetPeer(aRxInfo);
    VerifyOrExit(mP2pPeer != nullptr, LogWarn("no peer was found"));

    ProcessKeySequence(aRxInfo);
    mP2pState = kP2pLinkTearing;
    mWedAttachTimer.Start(kMaxP2pKeepAliveBeforeRemovePeer);

exit:
    return;
}

void Mle::SendP2pLinkRequest(const Mac::ExtAddress &aExtAddress)
{
    Error        error   = kErrorNone;
    TxMessage   *message = nullptr;
    Peer        *peer;
    Ip6::Address destination;

    LogInfo("SendP2pLinkRequest");
    VerifyOrExit((message = NewMleMessage(kCommandLinkRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendModeTlv(GetDeviceMode()));
    SuccessOrExit(error = message->AppendVersionTlv());

    peer = Get<PeerTable>().FindPeer(aExtAddress, Peer::kInStateAny);
    VerifyOrExit(peer != nullptr, error = kErrorInvalidState);

    peer->GenerateChallenge();
    SuccessOrExit(error = message->AppendChallengeTlv(peer->GetChallenge()));

    destination.Clear();
    destination.SetToLinkLocalAddress(aExtAddress);

    // Keep the radio in rx state for receiving LinkAcceptAndRequest.
    Get<MeshForwarder>().SetRxOnWhenIdle(true);

    SuccessOrExit(error = message->SendTo(destination));

    peer->GetLinkInfo().Clear();
    peer->ResetLinkFailures();
    peer->SetLastHeard(TimerMilli::GetNow());
    peer->SetExtAddress(aExtAddress);
    peer->SetState(Neighbor::kStateLinkRequest);

    Log(kMessageSend, kTypeLinkRequest, destination);

exit:
    if (error == kErrorNone)
    {
        mP2pState = kP2pLinkAccepting;
        mWedAttachTimer.Start(kEstablishP2pLinkTimeoutUs);
    }
    else
    {
        WakeupListenerEnable();
    }

    FreeMessageOnError(message, error);
}

void Mle::HandleP2pLinkRequest(RxInfo &aRxInfo)
{
    Error          error = kErrorNone;
    LinkAcceptInfo info;
    DeviceMode     mode;
    uint16_t       version;

    LogInfo("HandleP2pLinkRequest");
    Log(kMessageReceive, kTypeLinkRequest, aRxInfo.mMessageInfo.GetPeerAddr());
    VerifyOrExit((mP2pState == kP2pWakingUp) || (mP2pState == kP2pLinkRequesting));
    VerifyOrExit(aRxInfo.mMessageInfo.GetPeerAddr().IsLinkLocalUnicast());

    SuccessOrExit(error = aRxInfo.mMessage.ReadModeTlv(mode));
    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(info.mRxChallenge));
    SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(info.mExtAddress);
    VerifyOrExit((Get<PeerTable>().FindPeer(info.mExtAddress, Peer::kInStateLinkRequest)) == nullptr,
                 LogWarn("Receive duplicated P2pLinkRequest"));

    ProcessKeySequence(aRxInfo);

    if (aRxInfo.mNeighbor == nullptr)
    {
        VerifyOrExit((aRxInfo.mNeighbor = Get<PeerTable>().GetNewPeer()) != nullptr, error = kErrorNoBufs);
    }

    InitNeighbor(*aRxInfo.mNeighbor, aRxInfo);
    aRxInfo.mNeighbor->SetDeviceMode(mode);
    aRxInfo.mNeighbor->SetVersion(version);
    aRxInfo.mNeighbor->SetState(Neighbor::kStateLinkRequest);
    static_cast<Peer *>(aRxInfo.mNeighbor)->SetTimeout(kP2pLinkTimeoutMs);

    info.mLinkMargin = Get<Mac::Mac>().ComputeLinkMargin(aRxInfo.mMessage.GetAverageRss());

    SuccessOrExit(error = SendP2pLinkAcceptAndRequest(info));
#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    if (!mWakeupTxScheduler.IsWakeupByGroupId())
    {
        mWakeupTxScheduler.Stop();
    }
#endif

exit:
    if ((error == kErrorNone) && (mP2pState == kP2pWakingUp))
    {
        mP2pState               = kP2pLinkRequesting;
        mP2pNumLinksEstablished = 0;
        mWedAttachTimer.Start(kEstablishP2pLinkTimeoutUs);
    }

    LogProcessError(kTypeLinkRequest, error);
}

Error Mle::SendP2pLinkAccept(const LinkAcceptInfo &aInfo)
{
    return SendP2pLinkAcceptVariant(aInfo, false /* aIsLinkAcceptAndRequest*/);
}

Error Mle::SendP2pLinkAcceptAndRequest(const LinkAcceptInfo &aInfo)
{
    return SendP2pLinkAcceptVariant(aInfo, true /* aIsLinkAcceptAndRequest*/);
}

Error Mle::SendP2pLinkAcceptVariant(const LinkAcceptInfo &aInfo, bool aIsLinkAcceptAndRequest)
{
    Error        error   = kErrorNone;
    TxMessage   *message = nullptr;
    Command      command = aIsLinkAcceptAndRequest ? kCommandLinkAcceptAndRequest : kCommandLinkAccept;
    Peer        *peer    = nullptr;
    Ip6::Address destination;

    if (aIsLinkAcceptAndRequest)
    {
        LogInfo("SendP2pLinkAcceptAndRequest");
    }
    else
    {
        LogInfo("SendP2pLinkAccept");
    }

    VerifyOrExit((message = NewMleMessage(command)) != nullptr, error = kErrorNoBufs);
    if (command == kCommandLinkAcceptAndRequest)
    {
        SuccessOrExit(error = message->AppendModeTlv(GetDeviceMode()));
        SuccessOrExit(error = message->AppendVersionTlv());
    }

    SuccessOrExit(error = message->AppendResponseTlv(aInfo.mRxChallenge));
    message->SetDirectTransmission();
    if (command == kCommandLinkAcceptAndRequest)
    {
        VerifyOrExit((peer = Get<PeerTable>().FindPeer(aInfo.mExtAddress, Peer::kInStateLinkRequest)) != nullptr,
                     error = kErrorNotFound);

        peer->GenerateChallenge();
        SuccessOrExit(error = message->AppendChallengeTlv(peer->GetChallenge()));
    }
    else
    {
        VerifyOrExit((peer = Get<PeerTable>().FindPeer(aInfo.mExtAddress, Peer::kInStateValid)) != nullptr,
                     error = kErrorNotFound);
    }

    SuccessOrExit(error = message->AppendLinkMarginTlv(aInfo.mLinkMargin));
    SuccessOrExit(error = message->AppendLinkAndMleFrameCounterTlvs());
    SuccessOrExit(error = message->AppendSupervisionIntervalTlvIfSleepyChild());
    SuccessOrExit(error = message->AppendCslClockAccuracyTlv());

    destination.SetToLinkLocalAddress(aInfo.mExtAddress);

    SuccessOrExit(error = message->SendTo(destination));

    if (command == kCommandLinkAccept)
    {
        // Triger the ChildSupervisor to send supervision messages to keep the link alive.
        // TODO: process the Peer supervision
        // Get<NeighborTable>().Signal(NeighborTable::kPeerAdded, *peer);

        mP2pState = kP2pIdle;
        mWedAttachTimer.Stop();
        WakeupListenerEnable();

        LogInfo("P2P link to %s is established", aInfo.mExtAddress.ToString().AsCString());
        mP2pEventCallback.InvokeIfSet(OT_P2P_EVENT_LINKED, &aInfo.mExtAddress);

        StartECsl(*peer);
    }

    Log(kMessageSend, (command == kCommandLinkAccept) ? kTypeLinkAccept : kTypeLinkAcceptAndRequest, destination);

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Mle::StartECsl(Peer &aPeer)
{
#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
    if (!IsRxOnWhenIdle())
    {
        if (!Get<Mac::Mac>().IsECslCapable())
        {
            Get<Mac::Mac>().SetECslCapable(true /* aIsECslCapable*/);
        }

        Get<Mac::Mac>().AddECslPeerAddress(aPeer.GetExtAddress());
    }
#endif

    SendLinkDataRequest(aPeer);
}

void Mle::HandleP2pLinkAccept(RxInfo &aRxInfo)
{
    LogInfo("HandleP2pLinkAccept");
    HandleP2pLinkAcceptVariant(aRxInfo, kTypeLinkAccept);
}

void Mle::HandleP2pLinkAcceptAndRequest(RxInfo &aRxInfo)
{
    LogInfo("HandleP2pLinkAcceptAndRequest");
    HandleP2pLinkAcceptVariant(aRxInfo, kTypeLinkAcceptAndRequest);
}

void Mle::HandleP2pLinkAcceptVariant(RxInfo &aRxInfo, MessageType aMessageType)
{
    // Handles "Link Accept" or "Link Accept And Request".

    Error          error = kErrorNone;
    DeviceMode     mode;
    uint16_t       version;
    RxChallenge    response;
    uint32_t       linkFrameCounter;
    uint32_t       mleFrameCounter;
    Peer          *peer;
    LinkAcceptInfo info;
    uint8_t        linkMargin;
    uint16_t       supervisionInterval;

    Log(kMessageReceive, aMessageType, aRxInfo.mMessageInfo.GetPeerAddr());

    aRxInfo.mMessageInfo.GetPeerAddr().GetIid().ConvertToExtAddress(info.mExtAddress);
    VerifyOrExit((peer = Get<PeerTable>().FindPeer(info.mExtAddress, Peer::kInStateLinkRequest)) != nullptr,
                 LogWarn("peer not found!"));
    aRxInfo.mNeighbor = peer;

    if (aMessageType == kTypeLinkAcceptAndRequest)
    {
        SuccessOrExit(error = aRxInfo.mMessage.ReadModeTlv(mode));
        SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));
        peer->SetDeviceMode(mode);
        peer->SetVersion(version);
    }

    SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));
    VerifyOrExit(response == peer->GetChallenge(), LogWarn("challenge not match"));
    SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));
    SuccessOrExit(error = Tlv::Find<LinkMarginTlv>(aRxInfo.mMessage, linkMargin));

    switch (Tlv::Find<SupervisionIntervalTlv>(aRxInfo.mMessage, supervisionInterval))
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        supervisionInterval = 0;
        break;
    default:
        ExitNow(error = kErrorParse);
    }

    InitNeighbor(*peer, aRxInfo);

    peer->SetState(Neighbor::kStateValid);
    peer->GetLinkFrameCounters().SetAll(linkFrameCounter);
    peer->SetLinkAckFrameCounter(linkFrameCounter);
    peer->SetMleFrameCounter(mleFrameCounter);
    peer->SetKeySequence(aRxInfo.mKeySequence);
    peer->SetSupervisionInterval(supervisionInterval);
    aRxInfo.mClass = RxInfo::kAuthoritativeMessage;

    ProcessKeySequence(aRxInfo);

    if (aMessageType == kTypeLinkAcceptAndRequest)
    {
        SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(info.mRxChallenge));

        info.mExtAddress = aRxInfo.mNeighbor->GetExtAddress();
        info.mLinkMargin = Get<Mac::Mac>().ComputeLinkMargin(aRxInfo.mMessage.GetAverageRss());

        Get<MeshForwarder>().SetRxOnWhenIdle(IsRxOnWhenIdle());
        SuccessOrExit(error = SendP2pLinkAccept(info));
    }
    else
    {
        // Triger the ChildSupervisor to send supervision messages to keep the link alive.
        Get<ChildSupervisor>().HandleP2pEvent(OT_P2P_EVENT_LINKED);

        LogInfo("P2P link to %s is established", peer->GetExtAddress().ToString().AsCString());

        mP2pEventCallback.InvokeIfSet(OT_P2P_EVENT_LINKED, &peer->GetExtAddress());

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
        Get<Srp::Server>().HandleP2pEvents(OT_P2P_EVENT_LINKED);
#endif

        mP2pNumLinksEstablished++;
        if (!HasPeerInLinkRequestState())
        {
            // All P2P links have been established.
            mP2pState = kP2pIdle;
            mWedAttachTimer.Stop();
            mP2pLinkDoneCallback.InvokeAndClearIfSet();
        }
    }

exit:
    LogProcessError(aMessageType, error);
}

void Mle::SendLinkDataRequestOrResponse(Peer &aPeer, bool aRequest)
{
    Error        error = kErrorNone;
    TxMessage   *message;
    Ip6::Address destination;

    destination.Clear();
    destination.SetToLinkLocalAddress(aPeer.GetExtAddress());

    LogInfo("SendLinkData%s", aRequest ? "Request" : "Response");
    VerifyOrExit((message = NewMleMessage(aRequest ? kCommandLinkDataRequest : kCommandLinkDataResponse)) != nullptr,
                 error = kErrorNoBufs);

    if (aRequest)
    {
        message->SetDirectTransmission();
    }

    SuccessOrExit(error = message->SendTo(destination));

exit:
    FreeMessageOnError(message, error);
    return;
}

void Mle::HandleLinkDataRequest(RxInfo &aRxInfo)
{
    Peer *peer;

    LogInfo("HandleLinkDataRequest");
    VerifyOrExit(aRxInfo.mMessageInfo.GetPeerAddr().IsLinkLocalUnicast());

    peer = GetPeer(aRxInfo);
    VerifyOrExit(peer != nullptr, LogWarn("no peer was found"));

    ProcessKeySequence(aRxInfo);

#if OPENTHREAD_CONFIG_MAC_ECSL_RECEIVER_ENABLE
    if (!IsRxOnWhenIdle())
    {
        Get<MeshForwarder>().SetRxOnWhenIdle(IsRxOnWhenIdle());
        if (!Get<Mac::Mac>().IsECslCapable())
        {
            Get<Mac::Mac>().SetECslCapable(true /* aIsECslCapable*/);
        }
        Get<Mac::Mac>().AddECslPeerAddress(peer->GetExtAddress());

        // Send DataResponse to send the eCSL schedule to the WC.
        SendLinkDataResponse(*peer);
    }
#endif

exit:
    return;
}

void Mle::HandleLinkDataResponse(RxInfo &aRxInfo)
{
    OT_UNUSED_VARIABLE(aRxInfo);

    LogInfo("HandleLinkDataResponse");
}

Error Mle::P2pGetPeerIp6Address(const Mac::ExtAddress &aExtAddress, Ip6::Address &aAddress) const
{
    Error error = kErrorNotFound;

    for (Peer &peer : Get<PeerTable>().Iterate(Peer::kInStateValid))
    {
        if (peer.GetExtAddress() == aExtAddress)
        {
            aAddress.SetToLinkLocalAddress(aExtAddress);
            error = kErrorNone;
            break;
        }
    }

    return error;
}
} // namespace Mle
} // namespace ot
#endif // OPENTHREAD_CONFIG_PEER_TO_PEER_ENABLE
