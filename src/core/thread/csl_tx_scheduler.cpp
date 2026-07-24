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
    , mTimer(aInstance)
{
    HandleRadioBusLatencyChanged();
}

void CslTxScheduler::HandleRadioBusLatencyChanged(void)
{
    // Longest frame on bus is 127 bytes with some metadata, we use
    // 150 bytes for bus Tx time estimation
    static constexpr uint16_t kMaxFrameSize = 150;

    mCslFrameRequestAheadUs = Mac::kCslRequestAhead + Get<Radio::Radio>().CalculateBusTransferTime(kMaxFrameSize);

    LogInfo("Set frame request ahead: %lu usec", ToUlong(mCslFrameRequestAheadUs));
}

void CslTxScheduler::Update(void)
{
    if (mCslTxMessage == nullptr)
    {
        RescheduleCslTx();
        ExitNow();
    }

    // A non-null `mCslTxMessage` indicates we are in the middle of MAC
    // CSL TX, i.e., a frame was prepared for `Mac` and we are waiting
    // for the TX done callback which will call `RescheduleCslTx()`.
    //
    // While in this state, if the indirect message being sent for the
    // currently chosen neighbor is modified or removed, we clear
    // `mCslTxNeighbor` to avoid processing it in the TX done
    // callback.

    VerifyOrExit(mCslTxNeighbor != nullptr);

    if (mCslTxNeighbor->GetIndirectMessage() != mCslTxMessage)
    {
        mCslTxNeighbor->ResetCslTxAttempts();
        mCslTxNeighbor                   = nullptr;
        mFrameContext.mMessageNextOffset = 0;
    }

exit:
    return;
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
    mTimer.Stop();
}

void CslTxScheduler::RescheduleCslTx(void)
{
    // Finds the CSL neighbor with the earliest upcoming CSL window
    // and a pending indirect message, and schedules a timer for its
    // transmission.

    Radio::Time64 radioNow = Get<Radio::Radio>().GetNow();
    TimeMilli     localNow = TimerMilli::GetNow();

    mCslTxNeighbor = nullptr;

#if OPENTHREAD_FTD
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        Radio::Time64 txWindow;

        if (!child.IsCslSynchronized() || child.GetIndirectMessageCount() == 0)
        {
            continue;
        }

        txWindow = child.DetermineNextCslWindow(radioNow, mCslFrameRequestAheadUs);

        if ((mCslTxNeighbor == nullptr) || (txWindow < mNeighborCslWindow))
        {
            mCslTxNeighbor     = &child;
            mNeighborCslWindow = txWindow;
        }
    }
#endif

    if (mCslTxNeighbor != nullptr)
    {
        uint32_t interval = static_cast<uint32_t>(mNeighborCslWindow - radioNow - mCslFrameRequestAheadUs);

        mTimer.FireAt(localNow + Time::UsecToMsec(interval));
    }
    else
    {
        mTimer.Stop();
    }
}

void CslTxScheduler::HandleTimer(void)
{
    VerifyOrExit(mCslTxNeighbor != nullptr);

    Get<Mac::Mac>().RequestCslFrameTransmission();

exit:
    return;
}

Radio::Time64 CslTxScheduler::NeighborInfo::DetermineNextCslWindow(Radio::Time64 aRadioNow, uint32_t aLeadTime) const
{
    // CSL phase is in units of 10 symbols from the first symbol of the frame
    // containing the CSL IE was transmitted until the next channel sample,
    // see IEEE 802.15.4-2015, section 6.12.2.
    //
    // The Thread standard further defines the CSL phase (see Thread 1.4,
    // section 3.2.6.3.4, also conforming to IEEE 802.15.4-2020, section
    // 6.12.2.1):
    //  - The "first symbol" from the definition SHALL be interpreted as the
    //    first symbol of the MAC Header.
    //  - "until the next channel sample":
    //     - The CSL Receiver SHALL be ready to receive when the preamble
    //       time T_pa as specified below is reached.
    //     - The CSL Receiver SHOULD be ready to receive earlier than T_pa
    //       and SHOULD stay ready to receive until after the time specified
    //       in CSL Phase, according to the implementation and accuracy
    //       expectations.
    //     - The CSL Transmitter SHALL start transmitting the first symbol
    //       of the preamble of the frame to transmit at the preamble time
    //       T_pa = (CSL-Phase-Time - 192 us) (that is, CCA must be
    //       performed before time T_pa). Here, CSL-Phase-Time is the time
    //       duration specified by the CslPhase field value (in units of 10
    //       symbol periods).
    //     - This implies that the CSL Transmitter SHALL start transmitting
    //       the first symbol of the MAC Header at the time T_mh =
    //       CSL-Phase-Time.
    //
    // Derivation of the next TX timestamp based on this definition and the
    // RX timestamp of the packet containing the CSL IE:
    //
    // Note that RX and TX timestamps are defined to point to the end of the
    // synchronization header (SHR).
    //
    // lastTmh = lastRxTimestamp + phrDuration
    //
    // nextTmh = lastTmh + symbolPeriod * 10 * (n * cslPeriod + cslPhase)
    //         = lastTmh + 160us * (n * cslPeriod + cslPhase)
    //
    // nextTxTimestamp
    //         = nextTmh - phrDuration
    //         = lastRxTimestamp + 160us * (n * cslPeriod + cslPhase)

    uint32_t      periodInUs    = GetCslPeriod() * Radio::kUsPerTenSymbols;
    Radio::Time64 firstTxWindow = GetLastRxTimestamp() + GetCslPhase() * Radio::kUsPerTenSymbols;
    Radio::Time64 nextTxWindow  = aRadioNow - (aRadioNow % periodInUs) + (firstTxWindow % periodInUs);

    while (nextTxWindow < aRadioNow + aLeadTime)
    {
        nextTxWindow += periodInUs;
    }

    return nextTxWindow;
}

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *CslTxScheduler::HandleFrameRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame = nullptr;

    VerifyOrExit(mCslTxNeighbor != nullptr);
    VerifyOrExit(mCslTxNeighbor->IsCslSynchronized());

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(Radio::kTypeIeee802154);
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
            frame->SetKeyIndex(mCslTxNeighbor->GetIndirectKeyIndex());
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

    // Verify that the CSL target window has not already passed.
    // If MAC was delayed by other operations and started the CSL TX
    // operation late, the target window may be missed, so we abort.

    if (mNeighborCslWindow < Get<Radio::Radio>().GetNow())
    {
        frame = nullptr;
        ExitNow();
    }

    frame->SetTxDelayBaseTime(Radio::ConvertTime64To32(mCslTxNeighbor->GetLastRxTimestamp()));
    frame->SetTxDelay(static_cast<uint32_t>(mNeighborCslWindow - mCslTxNeighbor->GetLastRxTimestamp()));
    frame->SetCsmaCaEnabled(true);

exit:
    return frame;
}

#else // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

Mac::TxFrame *CslTxScheduler::HandleFrameRequest(Mac::TxFrames &) { return nullptr; }

#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE

void CslTxScheduler::HandleSentFrame(const Mac::TxFrame &aFrame, Error aError)
{
    VerifyOrExit(mCslTxNeighbor != nullptr);

    switch (aError)
    {
    case kErrorNone:
        mCslTxNeighbor->ResetCslTxAttempts();
        mCslTxNeighbor->ResetIndirectTxAttempts();
        break;

    case kErrorNoAck:
        OT_ASSERT(!aFrame.GetSecurityEnabled() || aFrame.IsHeaderUpdated());

        mCslTxNeighbor->IncrementCslTxAttempts();
        LogInfo("CSL tx to %04x failed, attempt %d/%d", mCslTxNeighbor->GetRloc16(), mCslTxNeighbor->GetCslTxAttempts(),
                kMaxCslTriggeredTxAttempts);

        if (mCslTxNeighbor->GetCslTxAttempts() >= kMaxCslTriggeredTxAttempts)
        {
            // CSL transmission attempts reach max, consider child out of sync
            mCslTxNeighbor->SetCslSynchronized(false);
            mCslTxNeighbor->ResetCslTxAttempts();
        }

        OT_FALL_THROUGH;

    case kErrorChannelAccessFailure:
    case kErrorAbort:

        // Even if CSL tx attempts count reaches max, the message won't be
        // dropped until indirect tx attempts count reaches max. So here it
        // would set sequence number and schedule next CSL tx.

        if (!aFrame.IsEmpty())
        {
            mCslTxNeighbor->SetIndirectDataSequenceNumber(aFrame.GetSequence());

            if (aFrame.GetSecurityEnabled() && aFrame.IsHeaderUpdated())
            {
                uint32_t frameCounter;
                uint8_t  keyIndex;

                IgnoreError(aFrame.GetFrameCounter(frameCounter));
                mCslTxNeighbor->SetIndirectFrameCounter(frameCounter);

                IgnoreError(aFrame.GetKeyIndex(keyIndex));
                mCslTxNeighbor->SetIndirectKeyIndex(keyIndex);
            }
        }

        ExitNow();

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    Get<IndirectSender>().HandleSentFrameToCslNeighbor(aFrame, mFrameContext, aError, *mCslTxNeighbor);

exit:
    mCslTxMessage  = nullptr;
    mCslTxNeighbor = nullptr;
    RescheduleCslTx();
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
