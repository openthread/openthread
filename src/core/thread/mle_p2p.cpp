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

#if OPENTHREAD_CONFIG_P2P_ENABLE
#if !(OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE)
#error "OPENTHREAD_CONFIG_P2P_ENABLE requires OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE or "\
    "OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE"
#endif

#include "common/code_utils.hpp"
#include "instance/instance.hpp"
#include "openthread/platform/toolchain.h"

namespace ot {
namespace Mle {

RegisterLogModule("P2p");

Mle::P2p::P2p(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kStateIdle)
    , mPeerTable(aInstance)
    , mTimer(aInstance)
    , mLinkDoneCallback()
    , mUnlinkDoneCallback()
    , mEventCallback()
    , mPeer(nullptr)
{
}

#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
Error Mle::P2p::WakeupAndLink(const P2pRequest &aP2pRequest, P2pLinkDoneCallback aCallback, void *aContext)
{
    Error error = kErrorNone;

    VerifyOrExit(!Get<Radio>().GetPromiscuous(), error = kErrorInvalidState);
    VerifyOrExit(Get<ThreadNetif>().IsUp(), error = kErrorInvalidState);
    VerifyOrExit(!Get<Mle>().IsDisabled(), error = kErrorInvalidState);
    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);
    VerifyOrExit(!mPeerTable.IsFull(), error = kErrorNoBufs);

    // TODO: support rx-off-when-idle device later.
    VerifyOrExit(Get<Mle>().IsRxOnWhenIdle(), error = kErrorInvalidState);

    SuccessOrExit(
        error = Get<WakeupTxScheduler>().WakeUp(aP2pRequest.GetWakeupRequest(), kWakeupTxInterval, kWakeupMaxDuration));

    mState = kStateWakingUp;
    mLinkDoneCallback.Set(aCallback, aContext);
    mTimer.FireAt(Get<WakeupTxScheduler>().GetTxEndTime() + Get<WakeupTxScheduler>().GetConnectionWindowUs());

exit:
    return error;
}

void Mle::P2p::HandleP2pLinkRequest(RxInfo &aRxInfo)
{
    Error          error = kErrorNone;
    LinkAcceptInfo info;
    DeviceMode     mode;
    Peer          *peer;
    uint16_t       version;

    Log(kMessageReceive, kTypeP2pLinkRequest, aRxInfo.mMessageInfo.GetPeerAddr());
    VerifyOrExit(mState == kStateWakingUp, error = kErrorInvalidState);
    VerifyOrExit(aRxInfo.mMessageInfo.GetPeerAddr().IsLinkLocalUnicast(), error = kErrorDrop);

    info.mExtAddress.SetFromIid(aRxInfo.mMessageInfo.GetPeerAddr().GetIid());
    VerifyOrExit(Get<NeighborTable>().FindNeighbor(info.mExtAddress, Neighbor::kInStateAnyExceptInvalid) == nullptr,
                 error = kErrorDrop);

    SuccessOrExit(error = aRxInfo.mMessage.ReadModeTlv(mode));
    // TODO: support the rx-off-when-idle device later.
    VerifyOrExit(mode.IsRxOnWhenIdle(), error = kErrorDrop);

    SuccessOrExit(error = aRxInfo.mMessage.ReadChallengeTlv(info.mRxChallenge));
    SuccessOrExit(error = aRxInfo.mMessage.ReadVersionTlv(version));

    Get<Mle>().ProcessKeySequence(aRxInfo);

    VerifyOrExit((peer = mPeerTable.GetNewPeer()) != nullptr, error = kErrorNoBufs);

    Get<Mle>().InitNeighbor(*peer, aRxInfo);
    peer->SetDeviceMode(mode);
    peer->SetVersion(version);
    peer->SetState(Neighbor::kStateLinkRequest);

    SuccessOrExit(error = SendP2pLinkAcceptAndRequest(info));
    Get<WakeupTxScheduler>().Stop();

    mState = kStateWaitingLinkAccept;
    mTimer.Start(kEstablishP2pLinkTimeoutUs);

exit:
    LogProcessError(kTypeP2pLinkRequest, error);
}

Error Mle::P2p::SendP2pLinkAcceptAndRequest(const LinkAcceptInfo &aInfo)
{
    return SendP2pLinkAcceptVariant(aInfo, true /* aIsLinkAcceptAndRequest*/);
}

void Mle::P2p::HandleP2pLinkAccept(RxInfo &aRxInfo) { HandleP2pLinkAcceptVariant(aRxInfo, kTypeP2pLinkAccept); }
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
void Mle::P2p::HandleP2pWakeup(const Mac::WakeupInfo &aWakeupInfo)
{
    Peer *peer = nullptr;

    VerifyOrExit(mState == kStateIdle);
    peer = mPeerTable.FindPeer(aWakeupInfo.mExtAddress, Peer::kInStateAnyExceptInvalid);
    VerifyOrExit(peer == nullptr);

    peer = mPeerTable.GetNewPeer();
    VerifyOrExit(peer != nullptr);

    peer->GetLinkInfo().Clear();
    peer->ResetLinkFailures();
    peer->SetLastHeard(TimerMilli::GetNow());
    peer->SetExtAddress(aWakeupInfo.mExtAddress);
    peer->SetState(Neighbor::kStateRestored);

    mPeer  = peer;
    mState = kStateAttachDelay;
    mTimer.Start(aWakeupInfo.mAttachDelayMs * Time::kOneMsecInUsec);

exit:
    if (peer == nullptr)
    {
        SetWakeupListenerEnabled();
    }

    return;
}

void Mle::P2p::SendP2pLinkRequest(Peer *aPeer)
{
    Error        error   = kErrorNone;
    TxMessage   *message = nullptr;
    Ip6::Address destination;

    VerifyOrExit(aPeer != nullptr, error = kErrorInvalidArgs);
    VerifyOrExit((message = Get<Mle>().NewMleMessage(kCommandP2pLinkRequest)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->AppendModeTlv(Get<Mle>().GetDeviceMode()));
    SuccessOrExit(error = message->AppendVersionTlv());

    aPeer->GenerateChallenge();
    SuccessOrExit(error = message->AppendChallengeTlv(aPeer->GetChallenge()));

    aPeer->GetLinkLocalIp6Address(destination);
    SuccessOrExit(error = message->SendTo(destination));
    aPeer->SetState(Neighbor::kStateLinkRequest);

    mState = kStateWaitingLinkAcceptAndRequest;
    mTimer.Start(kEstablishP2pLinkTimeoutUs);

    Log(kMessageSend, kTypeP2pLinkRequest, destination);

exit:
    if (error != kErrorNone)
    {
        if (aPeer != nullptr)
        {
            aPeer->SetState(Neighbor::kStateInvalid);
        }
        mState = kStateIdle;
        SetWakeupListenerEnabled();
    }

    FreeMessageOnError(message, error);
}

void Mle::P2p::HandleP2pLinkAcceptAndRequest(RxInfo &aRxInfo)
{
    HandleP2pLinkAcceptVariant(aRxInfo, kTypeP2pLinkAcceptAndRequest);
}

Error Mle::P2p::SendP2pLinkAccept(const LinkAcceptInfo &aInfo)
{
    return SendP2pLinkAcceptVariant(aInfo, false /* aIsLinkAcceptAndRequest*/);
}
#endif // OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

Error Mle::P2p::SendP2pLinkAcceptVariant(const LinkAcceptInfo &aInfo, bool aIsLinkAcceptAndRequest)
{
    Error        error   = kErrorNone;
    Command      command = aIsLinkAcceptAndRequest ? kCommandP2pLinkAcceptAndRequest : kCommandP2pLinkAccept;
    TxMessage   *message = nullptr;
    Peer        *peer    = nullptr;
    Ip6::Address destination;

    VerifyOrExit((message = Get<Mle>().NewMleMessage(command)) != nullptr, error = kErrorNoBufs);
    if (command == kCommandP2pLinkAcceptAndRequest)
    {
        SuccessOrExit(error = message->AppendModeTlv(Get<Mle>().GetDeviceMode()));
        SuccessOrExit(error = message->AppendVersionTlv());
    }

    SuccessOrExit(error = message->AppendResponseTlv(aInfo.mRxChallenge));
    if (command == kCommandP2pLinkAcceptAndRequest)
    {
        VerifyOrExit((peer = mPeerTable.FindPeer(aInfo.mExtAddress, Peer::kInStateLinkRequest)) != nullptr,
                     error = kErrorNotFound);

        peer->GenerateChallenge();
        SuccessOrExit(error = message->AppendChallengeTlv(peer->GetChallenge()));
        message->SetDirectTransmission();
    }
    else
    {
        peer = mPeerTable.FindPeer(aInfo.mExtAddress, Peer::kInStateValid);
        VerifyOrExit(peer != nullptr, error = kErrorNotFound);
    }

    SuccessOrExit(error = message->AppendLinkMarginTlv(aInfo.mLinkMargin));
    SuccessOrExit(error = message->AppendLinkAndMleFrameCounterTlvs());

    peer->GetLinkLocalIp6Address(destination);
    SuccessOrExit(error = message->SendTo(destination));

    if (command == kCommandP2pLinkAccept)
    {
        mState = kStateIdle;
        mTimer.Stop();
        SetWakeupListenerEnabled();

        LogInfo("P2P link to %s is established", aInfo.mExtAddress.ToString().AsCString());
        mEventCallback.InvokeIfSet(OT_P2P_EVENT_LINKED, &aInfo.mExtAddress);
    }

    Log(kMessageSend, (command == kCommandP2pLinkAccept) ? kTypeP2pLinkAccept : kTypeP2pLinkAcceptAndRequest,
        destination);

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Mle::P2p::HandleP2pLinkAcceptVariant(RxInfo &aRxInfo, MessageType aMessageType)
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
    VerifyOrExit((peer = mPeerTable.FindPeer(info.mExtAddress, Peer::kInStateLinkRequest)) != nullptr,
                 error = kErrorNotFound);
    aRxInfo.mNeighbor = peer;

    if (aMessageType == kTypeP2pLinkAcceptAndRequest)
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

    Get<Mle>().InitNeighbor(*peer, aRxInfo);

    peer->SetState(Neighbor::kStateValid);
    peer->GetLinkFrameCounters().SetAll(linkFrameCounter);
    peer->SetLinkAckFrameCounter(linkFrameCounter);
    peer->SetMleFrameCounter(mleFrameCounter);
    peer->SetKeySequence(aRxInfo.mKeySequence);
    aRxInfo.mClass = RxInfo::kAuthoritativeMessage;

    Get<Mle>().ProcessKeySequence(aRxInfo);

    if (aMessageType == kTypeP2pLinkAcceptAndRequest)
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

        mEventCallback.InvokeIfSet(OT_P2P_EVENT_LINKED, &peer->GetExtAddress());

        if (!mPeerTable.HasPeers(Peer::kInStateLinkRequest))
        {
            // All P2P links have been established.
            mState = kStateIdle;
            mTimer.Stop();
            mLinkDoneCallback.InvokeAndClearIfSet();
        }
    }

exit:
    LogProcessError(aMessageType, error);
}

Error Mle::P2p::Unlink(const Mac::ExtAddress &aExtAddress, P2pUnlinkDoneCallback aCallback, void *aContext)
{
    Error error = kErrorNone;
    Peer *peer;

    VerifyOrExit(mState == kStateIdle, error = kErrorBusy);
    VerifyOrExit((peer = mPeerTable.FindPeer(aExtAddress, Peer::kInStateValid)) != nullptr, error = kErrorNotFound);
    SuccessOrExit(error = SendLinkTearDown(*peer));

    mState = kStateTearingDown;
    mUnlinkDoneCallback.Set(aCallback, aContext);

exit:
    return error;
}

Error Mle::P2p::SendLinkTearDown(Peer &aPeer)
{
    Error        error = kErrorNone;
    TxMessage   *message;
    Ip6::Address destination;

    aPeer.GetLinkLocalIp6Address(destination);
    VerifyOrExit((message = Get<Mle>().NewMleMessage(kCommandP2pLinkTearDown)) != nullptr, error = kErrorNoBufs);
    message->RegisterTxCallback(HandleLinkTearDownTxDone, this);
    SuccessOrExit(error = message->SendTo(destination));

    Log(kMessageSend, kTypeP2pLinkTearDown, destination);

exit:
    FreeMessageOnError(message, error);
    return error;
}

void Mle::P2p::HandleLinkTearDownTxDone(const otMessage *aMessage, otError aError, void *aContext)
{
    OT_UNUSED_VARIABLE(aError);
    static_cast<Mle::P2p *>(aContext)->HandleLinkTearDownTxDone(AsCoreType(aMessage));
}

void Mle::P2p::HandleLinkTearDownTxDone(const Message &aMessage)
{
    Ip6::Header     ip6Header;
    Mac::ExtAddress extAddress;
    Peer           *peer;

    SuccessOrExit(aMessage.Read(0, ip6Header));
    VerifyOrExit(ip6Header.GetDestination().IsLinkLocalUnicast());
    extAddress.SetFromIid(ip6Header.GetDestination().GetIid());

    peer = mPeerTable.FindPeer(extAddress, Peer::kInStateValid);
    if (peer == nullptr)
    {
        // Peer may have been removed if we received a tear down from it. In this case, we can consider the unlink done.
        mState = kStateIdle;
        mUnlinkDoneCallback.InvokeAndClearIfSet();
        ExitNow();
    }

    if (!aMessage.GetTxSuccess() && (peer->GetTearDownCount() < Peer::kMaxRetransmitLinkTearDowns))
    {
        peer->IncrementTearDownCount();

        if (SendLinkTearDown(*peer) == kErrorNone)
        {
            ExitNow();
        }
    }

    PeerUnlinked(*peer);
    mUnlinkDoneCallback.InvokeAndClearIfSet();

exit:
    return;
}

void Mle::P2p::PeerUnlinked(Peer &aPeer)
{
    aPeer.SetState(Neighbor::kStateInvalid);

    if (mState == kStateTearingDown)
    {
        mState = kStateIdle;
    }

    mEventCallback.InvokeIfSet(OT_P2P_EVENT_UNLINKED, &aPeer.GetExtAddress());
}

void Mle::P2p::HandleP2pLinkTearDown(RxInfo &aRxInfo)
{
    Peer           *peer;
    Mac::ExtAddress extAddress;

    Log(kMessageReceive, kTypeP2pLinkTearDown, aRxInfo.mMessageInfo.GetPeerAddr());
    VerifyOrExit(aRxInfo.mMessageInfo.GetPeerAddr().IsLinkLocalUnicast());
    extAddress.SetFromIid(aRxInfo.mMessageInfo.GetPeerAddr().GetIid());
    VerifyOrExit((peer = mPeerTable.FindPeer(extAddress, Peer::kInStateValid)) != nullptr);

    Get<Mle>().ProcessKeySequence(aRxInfo);
    PeerUnlinked(*peer);

exit:
    return;
}

void Mle::P2p::HandleLinkTimer(void)
{
    switch (mState)
    {
#if OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE
    case kStateWakingUp:
    case kStateWaitingLinkAccept:
    {
        if (mState == kStateWakingUp)
        {
            LogInfo("Connection window closed");
        }

        mState = kStateIdle;
        ClearPeersInLinkRequestState();
        mLinkDoneCallback.InvokeAndClearIfSet();
    }
    break;
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    case kStateAttachDelay:
        SendP2pLinkRequest(mPeer);
        break;

    case kStateWaitingLinkAcceptAndRequest:
        LogWarn("Waiting for the P2P link accept and request timeout");

        mState = kStateIdle;
        ClearPeersInLinkRequestState();
        SetWakeupListenerEnabled();
        break;
#endif

    default:
        break;
    }
}

void Mle::P2p::SetWakeupListenerEnabled(void)
{
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    // The wake-up listener is disabled after a wake-up frame is received, here enables the wake-up listener again.
    IgnoreError(Get<Mac::Mac>().SetWakeupListenEnabled(true));
#endif
}

void Mle::P2p::ClearPeersInLinkRequestState(void)
{
    for (Peer &peer : mPeerTable.Iterate(Peer::kInStateLinkRequest))
    {
        peer.SetState(Neighbor::kStateInvalid);
    }
}

void Mle::P2p::SetEventCallback(otP2pEventCallback aCallback, void *aContext)
{
    mEventCallback.Set(aCallback, aContext);
}

} // namespace Mle
} // namespace ot
#endif // OPENTHREAD_CONFIG_P2P_ENABLE
