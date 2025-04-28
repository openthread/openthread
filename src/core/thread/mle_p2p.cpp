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
 *   This file implements MLE functionality required for the Thread peer-to-peer (P2P) link.
 */

#include "mle.hpp"

#if OPENTHREAD_CONFIG_PEER_TO_PEER_ENABLE
#if !(OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE)
#error "OPENTHREAD_CONFIG_PEER_TO_PEER_ENABLE requires OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE or "\
    "OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE"
#endif

#include "common/code_utils.hpp"
#include "instance/instance.hpp"
#include "openthread/platform/toolchain.h"

namespace ot {
namespace Mle {

RegisterLogModule("MleP2p");

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
Error Mle::P2pLink(const P2pRequest &aP2pRequest, P2pLinkedCallback aCallback, void *aContext)
{
    Error error;

    VerifyOrExit(!Get<Radio>().GetPromiscuous(), error = kErrorInvalidState);
    VerifyOrExit(Get<ThreadNetif>().IsUp(), error = kErrorInvalidState);
    VerifyOrExit(!IsDisabled(), error = kErrorInvalidState);
    VerifyOrExit(mP2pState == kP2pStateIdle, error = kErrorBusy);
    VerifyOrExit(!mP2pPeerTable.IsFull(), error = kErrorNoBufs);

    // TODO: support rx-off-when-idle device later.
    VerifyOrExit(IsRxOnWhenIdle(), error = kErrorInvalidState);

    SuccessOrExit(error =
                      mWakeupTxScheduler.WakeUp(aP2pRequest.GetWakeupAddress(), kWakeupTxInterval, kWakeupMaxDuration));

    mP2pState = kP2pStateWakingUp;
    mP2pLinkedCallback.Set(aCallback, aContext);
    mP2pLinkTimer.FireAt(mWakeupTxScheduler.GetTxEndTime() + mWakeupTxScheduler.GetConnectionWindowUs());

exit:
    return error;
}

void Mle::HandleP2pLinkRequest(RxInfo &aRxInfo)
{
    Error          error = kErrorNone;
    LinkAcceptInfo info;
    DeviceMode     mode;
    Peer          *peer;
    uint16_t       version;

    Log(kMessageReceive, kTypeLinkRequest, aRxInfo.mMessageInfo.GetPeerAddr());
    VerifyOrExit(mP2pState == kP2pStateWakingUp, error = kErrorInvalidState);
    VerifyOrExit(aRxInfo.mMessageInfo.GetPeerAddr().IsLinkLocalUnicast(), error = kErrorDrop);

    info.mExtAddress.SetFromIid(aRxInfo.mMessageInfo.GetPeerAddr().GetIid());
    VerifyOrExit(Get<NeighborTable>().FindNeighbor(info.mExtAddress, Neighbor::kInStateAnyExceptInvalid) == nullptr,
                 error = kErrorDrop);

    SuccessOrExit(error = aRxInfo.mMessage.ReadModeTlv(mode));
    // TODO: support the rx-off-when-idle device later.
    VerifyOrExit(mode.IsRxOnWhenIdle(), error = kErrorDrop);

    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(info.mRxChallenge));
    SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));

    ProcessKeySequence(aRxInfo);

    VerifyOrExit((peer = mP2pPeerTable.GetNewPeer()) != nullptr, error = kErrorNoBufs);

    InitNeighbor(*peer, aRxInfo);
    peer->SetDeviceMode(mode);
    peer->SetVersion(version);
    peer->SetState(Neighbor::kStateLinkRequest);

    SuccessOrExit(error = SendP2pLinkAcceptAndRequest(info));
    mWakeupTxScheduler.Stop();

exit:
    if (error == kErrorNone)
    {
        mP2pState           = kP2pStateLinkRequesting;
        mP2pLinkEstablished = false;
        mP2pLinkTimer.Start(kEstablishP2pLinkTimeoutUs);
    }

    LogProcessError(kTypeLinkRequest, error);
}

Error Mle::SendP2pLinkAcceptAndRequest(const LinkAcceptInfo &aInfo)
{
    return SendP2pLinkAcceptVariant(aInfo, true /* aIsLinkAcceptAndRequest*/);
}

void Mle::HandleP2pLinkAccept(RxInfo &aRxInfo) { HandleP2pLinkAcceptVariant(aRxInfo, kTypeLinkAccept); }
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
void Mle::HandleP2pWakeup(const Mac::WakeupInfo &aWakeupInfo)
{
    Peer *peer = mP2pPeerTable.FindPeer(aWakeupInfo.mExtAddress, Peer::kInStateAnyExceptInvalid);
    VerifyOrExit(peer == nullptr);

    peer = mP2pPeerTable.GetNewPeer();
    if (peer == nullptr)
    {
        SetWakeupListenerEnabled();
        ExitNow();
    }

    peer->GetLinkInfo().Clear();
    peer->ResetLinkFailures();
    peer->SetLastHeard(TimerMilli::GetNow());
    peer->SetExtAddress(aWakeupInfo.mExtAddress);
    peer->SetState(Neighbor::kStateRestored);
    mDelayedSender.ScheduleLinkRequest(aWakeupInfo.mExtAddress, aWakeupInfo.mAttachDelayMs);

exit:
    return;
}

void Mle::DelayedSender::ScheduleLinkRequest(const Mac::ExtAddress &aExtAddress, uint32_t aDelay)
{
    Ip6::Address destination;

    destination.SetToLinkLocalAddress(aExtAddress);

    RemoveMatchingSchedules(kTypeLinkRequest, destination);
    AddSchedule(kTypeLinkRequest, destination, aDelay, nullptr, 0);
}

void Mle::SendP2pLinkRequest(const Mac::ExtAddress &aExtAddress)
{
    Error        error   = kErrorNone;
    TxMessage   *message = nullptr;
    Peer        *peer;
    Ip6::Address destination;

    peer = mP2pPeerTable.FindPeer(aExtAddress, Peer::kInStateAnyExceptInvalid);
    VerifyOrExit(peer != nullptr, error = kErrorInvalidState);

    VerifyOrExit((message = NewMleMessage(kCommandLinkRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendModeTlv(GetDeviceMode()));
    SuccessOrExit(error = message->AppendVersionTlv());

    peer->GenerateChallenge();
    SuccessOrExit(error = message->AppendChallengeTlv(peer->GetChallenge()));

    peer->GetLinkLocalIp6Address(destination);
    SuccessOrExit(error = message->SendTo(destination));
    peer->SetState(Neighbor::kStateLinkRequest);

    Log(kMessageSend, kTypeLinkRequest, destination);

exit:
    if (error == kErrorNone)
    {
        mP2pState = kP2pStateLinkAccepting;
        mP2pLinkTimer.Start(kEstablishP2pLinkTimeoutUs);
    }
    else
    {
        if (peer != nullptr)
        {
            peer->SetState(Neighbor::kStateInvalid);
        }

        SetWakeupListenerEnabled();
    }

    FreeMessageOnError(message, error);
}

void Mle::HandleP2pLinkAcceptAndRequest(RxInfo &aRxInfo)
{
    HandleP2pLinkAcceptVariant(aRxInfo, kTypeLinkAcceptAndRequest);
}

Error Mle::SendP2pLinkAccept(const LinkAcceptInfo &aInfo)
{
    return SendP2pLinkAcceptVariant(aInfo, false /* aIsLinkAcceptAndRequest*/);
}
#endif

Error Mle::SendP2pLinkAcceptVariant(const LinkAcceptInfo &aInfo, bool aIsLinkAcceptAndRequest)
{
    Error        error   = kErrorNone;
    Command      command = aIsLinkAcceptAndRequest ? kCommandLinkAcceptAndRequest : kCommandLinkAccept;
    TxMessage   *message = nullptr;
    Peer        *peer    = nullptr;
    Ip6::Address destination;

    VerifyOrExit((message = NewMleMessage(command)) != nullptr, error = kErrorNoBufs);
    if (command == kCommandLinkAcceptAndRequest)
    {
        SuccessOrExit(error = message->AppendModeTlv(GetDeviceMode()));
        SuccessOrExit(error = message->AppendVersionTlv());
    }

    SuccessOrExit(error = message->AppendResponseTlv(aInfo.mRxChallenge));
    if (command == kCommandLinkAcceptAndRequest)
    {
        VerifyOrExit((peer = mP2pPeerTable.FindPeer(aInfo.mExtAddress, Peer::kInStateLinkRequest)) != nullptr,
                     error = kErrorNotFound);

        peer->GenerateChallenge();
        SuccessOrExit(error = message->AppendChallengeTlv(peer->GetChallenge()));
        message->SetDirectTransmission();
    }
    else
    {
        peer = mP2pPeerTable.FindPeer(aInfo.mExtAddress, Peer::kInStateValid);
        VerifyOrExit(peer != nullptr, error = kErrorNotFound);
    }

    SuccessOrExit(error = message->AppendLinkMarginTlv(aInfo.mLinkMargin));
    SuccessOrExit(error = message->AppendLinkAndMleFrameCounterTlvs());

    peer->GetLinkLocalIp6Address(destination);
    SuccessOrExit(error = message->SendTo(destination));

    if (command == kCommandLinkAccept)
    {
        mP2pState = kP2pStateIdle;
        mP2pLinkTimer.Stop();
        SetWakeupListenerEnabled();

        LogInfo("P2P link to %s is established", aInfo.mExtAddress.ToString().AsCString());
        mP2pEventCallback.InvokeIfSet(OT_P2P_EVENT_LINKED, &aInfo.mExtAddress);
    }

    Log(kMessageSend, (command == kCommandLinkAccept) ? kTypeLinkAccept : kTypeLinkAcceptAndRequest, destination);

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Mle::HandleP2pLinkAcceptVariant(RxInfo &aRxInfo, MessageType aMessageType)
{
    Error          error = kErrorNone;
    DeviceMode     mode;
    uint16_t       version;
    RxChallenge    response;
    uint32_t       linkFrameCounter;
    uint32_t       mleFrameCounter;
    Peer          *peer;
    LinkAcceptInfo info;
    uint8_t        linkMargin;

    Log(kMessageReceive, aMessageType, aRxInfo.mMessageInfo.GetPeerAddr());

    info.mExtAddress.SetFromIid(aRxInfo.mMessageInfo.GetPeerAddr().GetIid());
    VerifyOrExit((peer = mP2pPeerTable.FindPeer(info.mExtAddress, Peer::kInStateLinkRequest)) != nullptr,
                 error = kErrorNotFound);
    aRxInfo.mNeighbor = peer;

    if (aMessageType == kTypeLinkAcceptAndRequest)
    {
        SuccessOrExit(error = aRxInfo.mMessage.ReadModeTlv(mode));
        SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));
        peer->SetDeviceMode(mode);
        peer->SetVersion(version);
    }

    SuccessOrExit(error = aRxInfo.mMessage.ReadResponseTlv(response));
    VerifyOrExit(response == peer->GetChallenge(), error = kErrorSecurity);
    SuccessOrExit(error = aRxInfo.mMessage.ReadFrameCounterTlvs(linkFrameCounter, mleFrameCounter));
    SuccessOrExit(error = Tlv::Find<LinkMarginTlv>(aRxInfo.mMessage, linkMargin));

    InitNeighbor(*peer, aRxInfo);

    peer->SetState(Neighbor::kStateValid);
    peer->GetLinkFrameCounters().SetAll(linkFrameCounter);
    peer->SetLinkAckFrameCounter(linkFrameCounter);
    peer->SetMleFrameCounter(mleFrameCounter);
    peer->SetKeySequence(aRxInfo.mKeySequence);
    aRxInfo.mClass = RxInfo::kAuthoritativeMessage;

    ProcessKeySequence(aRxInfo);

    if (aMessageType == kTypeLinkAcceptAndRequest)
    {
        SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(info.mRxChallenge));

        info.mExtAddress = aRxInfo.mNeighbor->GetExtAddress();
        info.mLinkMargin = Get<Mac::Mac>().ComputeLinkMargin(aRxInfo.mMessage.GetAverageRss());

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
        SuccessOrExit(error = SendP2pLinkAccept(info));
#endif
    }
    else
    {
        LogInfo("P2P link to %s is established", peer->GetExtAddress().ToString().AsCString());

        mP2pEventCallback.InvokeIfSet(OT_P2P_EVENT_LINKED, &peer->GetExtAddress());

        mP2pLinkEstablished = true;
        if (!mP2pPeerTable.HasPeers(Peer::kInStateLinkRequest))
        {
            // All P2P links have been established.
            mP2pState = kP2pStateIdle;
            mP2pLinkTimer.Stop();
            mP2pLinkedCallback.InvokeAndClearIfSet();
        }
    }

exit:
    LogProcessError(aMessageType, error);
}

void Mle::HandleP2pLinkTimer(void)
{
    switch (mP2pState)
    {
    case kP2pStateWakingUp:
    case kP2pStateLinkRequesting:
    {
        if (mP2pState == kP2pStateWakingUp)
        {
            LogInfo("Connection window closed");
        }
        else
        {
            if (!mP2pLinkEstablished)
            {
                LogWarn("No P2P link is established");
            }
            else
            {
                LogInfo("At least one P2P link is established");
            }
        }

        mP2pState = kP2pStateIdle;
        ClearPeersInLinkRequestState();
        mP2pLinkedCallback.InvokeAndClearIfSet();
    }
    break;

    case kP2pStateLinkAccepting:
        LogInfo("Accept the P2P link is timeout");

        mP2pState = kP2pStateIdle;
        ClearPeersInLinkRequestState();
        SetWakeupListenerEnabled();
        break;

    default:
        break;
    }
}

void Mle::SetWakeupListenerEnabled(void)
{
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    // The wake-up listener is disabled after a wake-up frame is received, here enables the wake-up listener again.
    IgnoreError(Get<Mac::Mac>().SetWakeupListenEnabled(true));
#endif
}

void Mle::ClearPeersInLinkRequestState(void)
{
    for (Peer &peer : mP2pPeerTable.Iterate(Peer::kInStateLinkRequest))
    {
        peer.SetState(Neighbor::kStateInvalid);
    }
}

void Mle::P2pSetEventCallback(otP2pEventCallback aCallback, void *aContext)
{
    mP2pEventCallback.Set(aCallback, aContext);
}

} // namespace Mle
} // namespace ot
#endif // OPENTHREAD_CONFIG_PEER_TO_PEER_ENABLE
