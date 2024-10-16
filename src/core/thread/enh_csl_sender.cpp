/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "enh_csl_sender.hpp"

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

#include "common/log.hpp"
#include "common/time.hpp"
#include "instance/instance.hpp"
#include "mac/mac.hpp"

namespace ot {

RegisterLogModule("EnhCslSender");

EnhCslSender::EnhCslSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCslTxNeigh(nullptr)
    , mFrameContext()
{
    InitFrameRequestAhead();
}

Neighbor *EnhCslSender::GetParent(void) const
{
    Neighbor *parent = nullptr;

    if (Get<Mle::Mle>().GetParent().IsStateValid())
    {
        parent = &Get<Mle::Mle>().GetParent();
    }
    else if (Get<Mle::Mle>().IsWakeupCoordPresent())
    {
        parent = &Get<Mle::Mle>().GetParentCandidate();
    }

    return parent;
}

void EnhCslSender::InitFrameRequestAhead(void)
{
    uint32_t busSpeedHz = otPlatRadioGetBusSpeed(&GetInstance());
    // longest frame on bus is 127 bytes with some metadata, use 150 bytes for bus Tx time estimation
    uint32_t busTxTimeUs = ((busSpeedHz == 0) ? 0 : (150 * 8 * 1000000 + busSpeedHz - 1) / busSpeedHz);

    mCslFrameRequestAheadUs = OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US + busTxTimeUs;
}

void EnhCslSender::AddMessageForCslPeer(Message &aMessage, Neighbor &aNeighbor)
{
    // TODO: find a proper way to mark this message as directed for the neighbor in CSL mode.
    // By the moment use IsDirectTransmission which defaults to false, assuming a single CSL peer.

    if (mCslTxNeigh->GetIndirectMessage() == nullptr)
    {
        aNeighbor.SetIndirectMessage(&aMessage);
        aNeighbor.SetIndirectFragmentOffset(0);
    }
    aNeighbor.IncrementIndirectMessageCount();
    RescheduleCslTx();
}

void EnhCslSender::ClearAllMessagesForCslPeer(Neighbor &aNeighbor)
{
    VerifyOrExit(aNeighbor.GetIndirectMessageCount() > 0);

    for (Message &message : Get<MeshForwarder>().mSendQueue)
    {
        Get<MeshForwarder>().RemoveMessageIfNoPendingTx(message);
    }

    aNeighbor.SetIndirectMessage(nullptr);
    aNeighbor.ResetIndirectMessageCount();
    aNeighbor.ResetEnhCslTxAttempts();

    Update();

exit:
    return;
}

void EnhCslSender::Update(void)
{
    if (mCslTxMessage == nullptr)
    {
        RescheduleCslTx();
    }
    else if ((mCslTxNeigh != nullptr) && (mCslTxNeigh->GetIndirectMessage() != mCslTxMessage))
    {
        // `Mac` has already started the CSL tx, so wait for tx done callback
        // to call `RescheduleCslTx`
        mCslTxNeigh                      = nullptr;
        mFrameContext.mMessageNextOffset = 0;
    }
}

/**
 * This method assumes that there is a single enhanced CSL synchronized neighbor
 * and that if any message is not marked as direct transmission then it should be sent via
 * enhanced CSL transmission.
 *
 */
void EnhCslSender::RescheduleCslTx(void)
{
    uint32_t delay;
    uint32_t cslTxDelay;

    // TODO: go over the list of neighbors awaiting indirect transmission
    mCslTxNeigh = GetParent();

    VerifyOrExit(mCslTxNeigh->GetIndirectMessageCount() > 0);

    if (mCslTxNeigh->GetIndirectMessage() == nullptr)
    {
        for (Message &message : Get<MeshForwarder>().mSendQueue)
        {
            if (!message.IsDirectTransmission())
            {
                mCslTxNeigh->SetIndirectMessage(&message);
                mCslTxNeigh->SetIndirectFragmentOffset(0);
                break;
            }
        }
    }

    /*
     * If no indirect message could be found despite of the positive indirect
     * message counter, then some messages must have been removed from the send
     * queue without notifying the enhanced CSL sender. Until this notification
     * is implemented, reset the counter to recover from such a scenario.
     */
    VerifyOrExit(mCslTxNeigh->GetIndirectMessage() != nullptr, mCslTxNeigh->ResetIndirectMessageCount());

    delay = GetNextCslTransmissionDelay(*mCslTxNeigh, cslTxDelay, mCslFrameRequestAheadUs);
    Get<Mac::Mac>().RequestEnhCslFrameTransmission(delay / 1000UL);

exit:
    return;
}

uint32_t EnhCslSender::GetNextCslTransmissionDelay(Neighbor &aNeighbor,
                                                   uint32_t &aDelayFromLastRx,
                                                   uint32_t  aAheadUs) const
{
    uint64_t radioNow      = otPlatRadioGetNow(&GetInstance());
    uint32_t periodInUs    = aNeighbor.GetEnhCslPeriod() * kUsPerTenSymbols;
    uint64_t firstTxWindow = aNeighbor.GetEnhLastRxTimestamp() + aNeighbor.GetEnhCslPhase() * kUsPerTenSymbols;
    uint64_t nextTxWindow  = radioNow - (radioNow % periodInUs) + (firstTxWindow % periodInUs);

    while (nextTxWindow < radioNow + aAheadUs) nextTxWindow += periodInUs;

    aDelayFromLastRx = static_cast<uint32_t>(nextTxWindow - aNeighbor.GetEnhLastRxTimestamp());

    return static_cast<uint32_t>(nextTxWindow - radioNow - aAheadUs);
}

uint16_t EnhCslSender::PrepareDataFrame(Mac::TxFrame &aFrame, Neighbor &aNeighbor, Message &aMessage)
{
    Ip6::Header    ip6Header;
    Mac::Addresses macAddrs;
    uint16_t       directTxOffset;
    uint16_t       nextOffset;

    // Determine the MAC source and destination addresses.

    IgnoreError(aMessage.Read(0, ip6Header));

    Get<MeshForwarder>().GetMacSourceAddress(ip6Header.GetSource(), macAddrs.mSource);

    if (ip6Header.GetDestination().IsLinkLocalUnicast())
    {
        Get<MeshForwarder>().GetMacDestinationAddress(ip6Header.GetDestination(), macAddrs.mDestination);
    }
    else
    {
        macAddrs.mDestination.SetExtended(aNeighbor.GetExtAddress());
    }

    // Prepare the data frame from previous neighbor's indirect offset.

    directTxOffset = aMessage.GetOffset();
    aMessage.SetOffset(aNeighbor.GetIndirectFragmentOffset());

    nextOffset = Get<MeshForwarder>().PrepareDataFrameWithNoMeshHeader(aFrame, aMessage, macAddrs);

    aMessage.SetOffset(directTxOffset);

    // Intentionally not setting frame pending bit even if more messages are queued

    return nextOffset;
}

Error EnhCslSender::PrepareFrameForNeighbor(Mac::TxFrame &aFrame, FrameContext &aContext, Neighbor &aNeighbor)
{
    Error    error   = kErrorNone;
    Message *message = aNeighbor.GetIndirectMessage();
    VerifyOrExit(message != nullptr, error = kErrorInvalidState);

    switch (message->GetType())
    {
    case Message::kTypeIp6:
        aContext.mMessageNextOffset = PrepareDataFrame(aFrame, aNeighbor, *message);
        break;

    default:
        error = kErrorNotImplemented;
        break;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *EnhCslSender::HandleFrameRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame = nullptr;
    uint32_t      txDelay;
    uint32_t      delay;
    bool          isCsl = false;

    VerifyOrExit(mCslTxNeigh != nullptr);
    VerifyOrExit(mCslTxNeigh->IsEnhCslSynchronized());

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(Mac::kRadioTypeIeee802154);
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    VerifyOrExit(PrepareFrameForNeighbor(*frame, mFrameContext, *mCslTxNeigh) == kErrorNone, frame = nullptr);
    mCslTxMessage = mCslTxNeigh->GetIndirectMessage();
    VerifyOrExit(mCslTxMessage != nullptr, frame = nullptr);

    if (mCslTxNeigh->GetEnhCslTxAttempts() > 0)
    {
        // For a re-transmission of an indirect frame to a sleepy
        // neighbor, we ensure to use the same frame counter, key id, and
        // data sequence number as the previous attempt.

        frame->SetIsARetransmission(true);
        frame->SetSequence(mCslTxNeigh->GetIndirectDataSequenceNumber());

        // If the frame contains CSL IE, it must be refreshed and re-secured with a new frame counter.
        // See Thread 1.3.0 Specification, 3.2.6.3.7 CSL Retransmissions
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        isCsl = frame->IsCslIePresent();
#endif

        if (frame->GetSecurityEnabled() && !isCsl)
        {
            frame->SetFrameCounter(mCslTxNeigh->GetIndirectFrameCounter());
            frame->SetKeyId(mCslTxNeigh->GetIndirectKeyId());
        }
    }
    else
    {
        frame->SetIsARetransmission(false);
    }

    // Use zero as aAheadUs not to miss a CSL slot in the case the MAC operation is slightly delayed.
    // This code mimics CslTxScheduler::HandleFrameRequest so see the latter for more details.
    delay = GetNextCslTransmissionDelay(*mCslTxNeigh, txDelay, /* aAheadUs */ 0);
    VerifyOrExit(delay <= mCslFrameRequestAheadUs + kFramePreparationGuardInterval, frame = nullptr);

    frame->SetTxDelay(txDelay);
    frame->SetTxDelayBaseTime(
        static_cast<uint32_t>(mCslTxNeigh->GetEnhLastRxTimestamp())); // Only LSB part of the time is required.
    frame->SetCsmaCaEnabled(false);

exit:
    return frame;
}

#else // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *EnhCslSender::HandleFrameRequest(Mac::TxFrames &) { return nullptr; }

#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

void EnhCslSender::HandleSentFrame(const Mac::TxFrame &aFrame, Error aError)
{
    Neighbor *neighbor = mCslTxNeigh;

    mCslTxMessage = nullptr;

    VerifyOrExit(neighbor != nullptr); // The result is no longer interested by upper layer

    mCslTxNeigh = nullptr;

    HandleSentFrame(aFrame, aError, *neighbor);

exit:
    return;
}

void EnhCslSender::HandleSentFrame(const Mac::TxFrame &aFrame, Error aError, Neighbor &aNeighbor)
{
    switch (aError)
    {
    case kErrorNone:
        aNeighbor.ResetEnhCslTxAttempts();
        break;

    case kErrorNoAck:
        OT_ASSERT(!aFrame.GetSecurityEnabled() || aFrame.IsHeaderUpdated());

        aNeighbor.IncrementEnhCslTxAttempts();
        LogInfo("CSL tx to neighbor %04x failed, attempt %d/%d", aNeighbor.GetRloc16(), aNeighbor.GetEnhCslTxAttempts(),
                aNeighbor.GetEnhCslMaxTxAttempts());

        if (aNeighbor.GetEnhCslTxAttempts() >= aNeighbor.GetEnhCslMaxTxAttempts())
        {
            // CSL transmission attempts reach max, consider neighbor out of sync
            aNeighbor.SetEnhCslSynchronized(false);
            aNeighbor.ResetEnhCslTxAttempts();

            if (aNeighbor.GetIndirectMessage()->GetType() == Message::kTypeIp6)
            {
                Get<MeshForwarder>().mIpCounters.mTxFailure++;
            }

            Get<MeshForwarder>().RemoveMessageIfNoPendingTx(*aNeighbor.GetIndirectMessage());
            Get<Mle::Mle>().BecomeDetached();
            ExitNow();
        }

        OT_FALL_THROUGH;

    case kErrorChannelAccessFailure:
    case kErrorAbort:

        // Even if CSL tx attempts count reaches max, the message won't be
        // dropped until indirect tx attempts count reaches max. So here it
        // would set sequence number and schedule next CSL tx.

        if (!aFrame.IsEmpty())
        {
            aNeighbor.SetIndirectDataSequenceNumber(aFrame.GetSequence());

            if (aFrame.GetSecurityEnabled() && aFrame.IsHeaderUpdated())
            {
                uint32_t frameCounter;
                uint8_t  keyId;

                IgnoreError(aFrame.GetFrameCounter(frameCounter));
                aNeighbor.SetIndirectFrameCounter(frameCounter);

                IgnoreError(aFrame.GetKeyId(keyId));
                aNeighbor.SetIndirectKeyId(keyId);
            }
        }

        RescheduleCslTx();
        ExitNow();

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    HandleSentFrameToNeighbor(aFrame, mFrameContext, kErrorNone, aNeighbor);

exit:
    return;
}

void EnhCslSender::HandleSentFrameToNeighbor(const Mac::TxFrame &aFrame,
                                             const FrameContext &aContext,
                                             otError             aError,
                                             Neighbor           &aNeighbor)
{
    Message *message    = aNeighbor.GetIndirectMessage();
    uint16_t nextOffset = aContext.mMessageNextOffset;

    if ((message != nullptr) && (nextOffset < message->GetLength()))
    {
        aNeighbor.SetIndirectFragmentOffset(nextOffset);
        RescheduleCslTx();
        ExitNow();
    }

    if (message != nullptr)
    {
        // The indirect tx of this message to the neighbor is done.

        Mac::Address macDest;

        aNeighbor.SetIndirectMessage(nullptr);
        aNeighbor.GetLinkInfo().AddMessageTxStatus(true);
        OT_ASSERT(aNeighbor.GetIndirectMessageCount() > 0);
        aNeighbor.DecrementIndirectMessageCount();

        if (!aFrame.IsEmpty())
        {
            IgnoreError(aFrame.GetDstAddr(macDest));
            Get<MeshForwarder>().LogMessage(MeshForwarder::kMessageTransmit, *message, aError, &macDest);
        }

        if (message->GetType() == Message::kTypeIp6)
        {
            (aError == kErrorNone) ? Get<MeshForwarder>().mIpCounters.mTxSuccess++
                                   : Get<MeshForwarder>().mIpCounters.mTxFailure++;
        }

        Get<MeshForwarder>().RemoveMessageIfNoPendingTx(*message);
    }

    RescheduleCslTx();

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
