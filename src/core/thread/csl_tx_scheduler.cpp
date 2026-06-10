/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#include "csl_tx_scheduler.hpp"

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("CslTxScheduler");

CslTxScheduler::CslTxScheduler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCslTxNeighbor(nullptr)
    , mCslTxMessage(nullptr)
    , mFrameContext()
{
    UpdateFrameRequestAhead();
}

void CslTxScheduler::UpdateFrameRequestAhead(void)
{
    // Longest frame on bus is 127 bytes with some metadata, we use
    // 150 bytes for bus Tx time estimation
    static constexpr uint16_t kMaxFrameSize = 150;

    mCslFrameRequestAheadUs = Mac::kCslRequestAhead + Get<Mac::Mac>().CalculateRadioBusTransferTime(kMaxFrameSize);

    LogInfo("Set frame request ahead: %lu usec", ToUlong(mCslFrameRequestAheadUs));
}

void CslTxScheduler::Update(void)
{
    if (mCslTxMessage == nullptr)
    {
        RescheduleCslTx();
    }
    else if ((mCslTxNeighbor != nullptr) && (mCslTxNeighbor->GetIndirectMessage() != mCslTxMessage))
    {
        // `Mac` has already started the CSL tx, so wait for tx done callback
        // to call `RescheduleCslTx`
        mCslTxNeighbor->ResetCslTxAttempts();
        mCslTxNeighbor                   = nullptr;
        mFrameContext.mMessageNextOffset = 0;
    }
}

void CslTxScheduler::Clear(void)
{
#if OPENTHREAD_FTD
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        child.ResetCslTxAttempts();
        child.SetCslSynchronized(false);
        child.SetCslChannel(0);
        child.SetCslTimeout(0);
        child.SetCslPeriod(0);
        child.SetCslPhase(0);
        child.SetCslLastHeard(TimeMilli(0));
    }
#endif

    mFrameContext.mMessageNextOffset = 0;
    mCslTxNeighbor                   = nullptr;
    mCslTxMessage                    = nullptr;
}

/**
 * Always finds the most recent CSL tx among all children,
 * and requests `Mac` to do CSL tx at specific time. It shouldn't be called
 * when `Mac` is already starting to do the CSL tx (indicated by `mCslTxMessage`).
 */
void CslTxScheduler::RescheduleCslTx(void)
{
    uint32_t     minDelayTime = Time::kMaxDuration;
    CslNeighbor *bestNeighbor = nullptr;

#if OPENTHREAD_FTD
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        uint32_t delay;
        uint32_t cslTxDelay;

        if (!child.IsCslSynchronized() || child.GetIndirectMessageCount() == 0)
        {
            continue;
        }

        delay = GetNextCslTransmissionDelay(child, cslTxDelay, mCslFrameRequestAheadUs);

        if (delay < minDelayTime)
        {
            minDelayTime = delay;
            bestNeighbor = &child;
        }
    }
#endif

    if (bestNeighbor != nullptr)
    {
        Get<Mac::Mac>().RequestCslFrameTransmission(minDelayTime / 1000UL);
    }

    mCslTxNeighbor = bestNeighbor;
}

uint32_t CslTxScheduler::GetNextCslTransmissionDelay(const CslNeighbor &aCslNeighbor,
                                                     uint32_t          &aDelayFromLastRx,
                                                     uint32_t           aAheadUs) const
{
    uint64_t radioNow   = Get<Radio>().GetNow();
    uint32_t periodInUs = aCslNeighbor.GetCslPeriod() * kUsPerTenSymbols;

    /* see CslTxScheduler::NeighborInfo::mCslPhase */
    uint64_t firstTxWindow = aCslNeighbor.GetLastRxTimestamp() + aCslNeighbor.GetCslPhase() * kUsPerTenSymbols;
    uint64_t nextTxWindow  = radioNow - (radioNow % periodInUs) + (firstTxWindow % periodInUs);

    while (nextTxWindow < radioNow + aAheadUs)
    {
        nextTxWindow += periodInUs;
    }

    aDelayFromLastRx = static_cast<uint32_t>(nextTxWindow - aCslNeighbor.GetLastRxTimestamp());

    return static_cast<uint32_t>(nextTxWindow - radioNow - aAheadUs);
}

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *CslTxScheduler::HandleFrameRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame = nullptr;
    uint32_t      txDelay;
    uint32_t      delay;

    VerifyOrExit(mCslTxNeighbor != nullptr);
    VerifyOrExit(mCslTxNeighbor->IsCslSynchronized());

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(Mac::kRadioTypeIeee802154);
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    VerifyOrExit(Get<IndirectSender>().PrepareFrameForCslNeighbor(*frame, mFrameContext, *mCslTxNeighbor) == kErrorNone,
                 frame = nullptr);
    mCslTxMessage = mCslTxNeighbor->GetIndirectMessage();
    VerifyOrExit(mCslTxMessage != nullptr, frame = nullptr);

    if (mCslTxNeighbor->GetIndirectTxAttempts() > 0 || mCslTxNeighbor->GetCslTxAttempts() > 0)
    {
        // For a re-transmission of an indirect frame to a sleepy
        // child, we ensure to use the same frame counter, key id, and
        // data sequence number as the previous attempt.

        frame->SetIsARetransmission(true);
        frame->SetSequence(mCslTxNeighbor->GetIndirectDataSequenceNumber());

        if (frame->GetSecurityEnabled())
        {
            frame->SetFrameCounter(mCslTxNeighbor->GetIndirectFrameCounter());
            frame->SetKeyId(mCslTxNeighbor->GetIndirectKeyId());
        }
    }
    else
    {
        frame->SetIsARetransmission(false);
    }

    frame->SetChannel(mCslTxNeighbor->GetCslChannel() == 0 ? Get<Mac::Mac>().GetPanChannel()
                                                           : mCslTxNeighbor->GetCslChannel());

    if (frame->GetChannel() != Get<Mac::Mac>().GetPanChannel())
    {
        frame->SetRxChannelAfterTxDone(Get<Mac::Mac>().GetPanChannel());
    }

    delay = GetNextCslTransmissionDelay(*mCslTxNeighbor, txDelay, /* aAheadUs */ 0);

    // We make sure that delay is less than `mCslFrameRequestAheadUs`
    // plus some guard time. Note that we used `mCslFrameRequestAheadUs`
    // in `RescheduleCslTx()` when determining the next CSL delay to
    // schedule CSL tx with `Mac` but here we calculate the delay with
    // zero `aAheadUs`. All the timings are in usec but when passing
    // delay to `Mac` we divide by `1000` (to convert to msec) which
    // can round the value down and cause `Mac` to start operation a
    // bit (some usec) earlier. This is covered by adding the guard
    // time `kFramePreparationGuardInterval`.
    //
    // In general this check handles the case where `Mac` is busy with
    // other operations and therefore late to start the CSL tx operation
    // and by the time `HandleFrameRequest()` is invoked, we miss the
    // current CSL window and move to the next window.

    VerifyOrExit(delay <= mCslFrameRequestAheadUs + kFramePreparationGuardInterval, frame = nullptr);

    frame->SetTxDelay(txDelay);
    frame->SetTxDelayBaseTime(
        static_cast<uint32_t>(mCslTxNeighbor->GetLastRxTimestamp())); // Only LSB part of the time is required.
    frame->SetCsmaCaEnabled(true);

exit:
    return frame;
}

#else // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *CslTxScheduler::HandleFrameRequest(Mac::TxFrames &) { return nullptr; }

#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

void CslTxScheduler::HandleSentFrame(const Mac::TxFrame &aFrame, Error aError)
{
    CslNeighbor *neighbor = mCslTxNeighbor;

    mCslTxMessage = nullptr;

    VerifyOrExit(neighbor != nullptr);

    mCslTxNeighbor = nullptr;

    HandleSentFrame(aFrame, aError, *neighbor);

exit:
    RescheduleCslTx();
}

void CslTxScheduler::HandleSentFrame(const Mac::TxFrame &aFrame, Error aError, CslNeighbor &aCslNeighbor)
{
    switch (aError)
    {
    case kErrorNone:
        aCslNeighbor.ResetCslTxAttempts();
        aCslNeighbor.ResetIndirectTxAttempts();
        break;

    case kErrorNoAck:
        OT_ASSERT(!aFrame.GetSecurityEnabled() || aFrame.IsHeaderUpdated());

        aCslNeighbor.IncrementCslTxAttempts();
        LogInfo("CSL tx to %04x failed, attempt %d/%d", aCslNeighbor.GetRloc16(), aCslNeighbor.GetCslTxAttempts(),
                kMaxCslTriggeredTxAttempts);

        if (aCslNeighbor.GetCslTxAttempts() >= kMaxCslTriggeredTxAttempts)
        {
            // CSL transmission attempts reach max, consider child out of sync
            aCslNeighbor.SetCslSynchronized(false);
            aCslNeighbor.ResetCslTxAttempts();
        }

        OT_FALL_THROUGH;

    case kErrorChannelAccessFailure:
    case kErrorAbort:

        // Even if CSL tx attempts count reaches max, the message won't be
        // dropped until indirect tx attempts count reaches max. So here it
        // would set sequence number and schedule next CSL tx.

        if (!aFrame.IsEmpty())
        {
            aCslNeighbor.SetIndirectDataSequenceNumber(aFrame.GetSequence());

            if (aFrame.GetSecurityEnabled() && aFrame.IsHeaderUpdated())
            {
                uint32_t frameCounter;
                uint8_t  keyId;

                IgnoreError(aFrame.GetFrameCounter(frameCounter));
                aCslNeighbor.SetIndirectFrameCounter(frameCounter);

                IgnoreError(aFrame.GetKeyId(keyId));
                aCslNeighbor.SetIndirectKeyId(keyId);
            }
        }

        ExitNow();

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    Get<IndirectSender>().HandleSentFrameToCslNeighbor(aFrame, mFrameContext, aError, aCslNeighbor);

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
