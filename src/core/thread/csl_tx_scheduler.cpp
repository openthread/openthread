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

#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/time.hpp"
#include "mac/mac.hpp"

namespace ot {

CslTxScheduler::Callbacks::Callbacks(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

inline otError CslTxScheduler::Callbacks::PrepareFrameForSedNeighbor(Mac::TxFrame &      aFrame,
                                                                     FrameContext &      aContext,
                                                                     SedCapableNeighbor &aSedCapableNeighbor)
{
    return Get<IndirectSender>().PrepareFrameForSedNeighbor(aFrame, aContext, aSedCapableNeighbor);
}

inline void CslTxScheduler::Callbacks::HandleSentFrameToSedNeighbor(const Mac::TxFrame &aFrame,
                                                                    const FrameContext &aContext,
                                                                    otError             aError,
                                                                    SedCapableNeighbor &aSedCapableNeighbor)
{
    Get<IndirectSender>().HandleSentFrameToSedNeighbor(aFrame, aContext, aError, aSedCapableNeighbor);
}

//---------------------------------------------------------

CslTxScheduler::CslTxScheduler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCslTxNeighbor(nullptr)
    , mCslTxMessage(nullptr)
    , mFrameContext()
    , mCallbacks(aInstance)
{
    InitFrameRequestAhead();
}

void CslTxScheduler::InitFrameRequestAhead(void)
{
    uint32_t busSpeedHz = otPlatRadioGetBusSpeed(&GetInstance());
    // longest frame on bus is 127 bytes with some metadata, use 150 bytes for bus Tx time estimation
    uint32_t busTxTimeUs = ((busSpeedHz == 0) ? 0 : (150 * 8 * 1000000 + busSpeedHz - 1) / busSpeedHz);

    mCslFrameRequestAheadUs = OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US + busTxTimeUs;
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
        mCslTxNeighbor                   = nullptr;
        mFrameContext.mMessageNextOffset = 0;
    }
}

void CslTxScheduler::Clear(void)
{
    for (SedCapableNeighbor &neighbor :
         Get<SedCapableNeighborTable>().Iterate(SedCapableNeighbor::kInStateAnyExceptInvalid))
    {
        neighbor.ResetCslTxAttempts();
        neighbor.SetCslSynchronized(false);
        neighbor.SetCslChannel(0);
        neighbor.SetCslTimeout(0);
        neighbor.SetCslPeriod(0);
        neighbor.SetCslPhase(0);
        neighbor.SetCslLastHeard(TimeMilli(0));
    }

    mFrameContext.mMessageNextOffset = 0;
    mCslTxNeighbor                   = nullptr;
    mCslTxMessage                    = nullptr;
}

/**
 * This method always finds the most recent CSL tx among all children,
 * and requests `Mac` to do CSL tx at specific time. It shouldn't be called
 * when `Mac` is already starting to do the CSL tx (indicated by `mCslTxMessage`).
 *
 */
void CslTxScheduler::RescheduleCslTx(void)
{
    uint32_t            minDelayTime = Time::kMaxDuration;
    SedCapableNeighbor *bestNeighbor = nullptr;

    for (SedCapableNeighbor &neighbor :
         Get<SedCapableNeighborTable>().Iterate(SedCapableNeighbor::kInStateAnyExceptInvalid))
    {
        uint32_t delay;
        uint32_t cslTxDelay;

        if (!neighbor.IsCslSynchronized() || neighbor.GetIndirectMessageCount() == 0)
        {
            continue;
        }

        delay = GetNextCslTransmissionDelay(neighbor, cslTxDelay);

        if (delay < minDelayTime)
        {
            minDelayTime = delay;
            bestNeighbor = &neighbor;
        }
    }

    if (bestNeighbor != nullptr)
    {
        Get<Mac::Mac>().RequestCslFrameTransmission(minDelayTime / 1000UL);
    }

    mCslTxNeighbor = bestNeighbor;
}

uint32_t CslTxScheduler::GetNextCslTransmissionDelay(const SedCapableNeighbor &aSedCapableNeighbor,
                                                     uint32_t &                aDelayFromLastRx) const
{
    uint64_t radioNow   = otPlatRadioGetNow(&GetInstance());
    uint32_t periodInUs = aSedCapableNeighbor.GetCslPeriod() * kUsPerTenSymbols;
    uint64_t firstTxWindow =
        aSedCapableNeighbor.GetLastRxTimestamp() + aSedCapableNeighbor.GetCslPhase() * kUsPerTenSymbols;
    uint64_t nextTxWindow = radioNow - (radioNow % periodInUs) + (firstTxWindow % periodInUs);

    while (nextTxWindow < radioNow + mCslFrameRequestAheadUs) nextTxWindow += periodInUs;

    aDelayFromLastRx = static_cast<uint32_t>(nextTxWindow - aSedCapableNeighbor.GetLastRxTimestamp());

    return static_cast<uint32_t>(nextTxWindow - radioNow - mCslFrameRequestAheadUs);
}

Mac::TxFrame *CslTxScheduler::HandleFrameRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame = nullptr;

#if OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    uint32_t txDelay;

    VerifyOrExit(mCslTxNeighbor != nullptr);

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(kRadioTypeIeee802154);
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    VerifyOrExit(mCallbacks.PrepareFrameForSedNeighbor(*frame, mFrameContext, *mCslTxNeighbor) == OT_ERROR_NONE,
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

    GetNextCslTransmissionDelay(*mCslTxNeighbor, txDelay);
    frame->SetTxDelay(txDelay);
    frame->SetTxDelayBaseTime(
        static_cast<uint32_t>(mCslTxNeighbor->GetLastRxTimestamp())); // Only LSB part of the time is required.
    frame->SetCsmaCaEnabled(false);

exit:
#endif // OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
    return frame;
}

void CslTxScheduler::HandleSentFrame(const Mac::TxFrame &aFrame, otError aError)
{
    SedCapableNeighbor *neighbor = mCslTxNeighbor;

    VerifyOrExit(neighbor != nullptr); // The result is no longer interested by upper layer

    mCslTxNeighbor = nullptr;
    mCslTxMessage  = nullptr;

    HandleSentFrame(aFrame, aError, *neighbor);

exit:
    return;
}

void CslTxScheduler::HandleSentFrame(const Mac::TxFrame &aFrame,
                                     otError             aError,
                                     SedCapableNeighbor &aSedCapableNeighbor)
{
    switch (aError)
    {
    case OT_ERROR_NONE:
        aSedCapableNeighbor.ResetCslTxAttempts();
        aSedCapableNeighbor.ResetIndirectTxAttempts();
        break;

    case OT_ERROR_NO_ACK:
        aSedCapableNeighbor.IncrementCslTxAttempts();

        otLogInfoMac("CSL tx to child %04x failed, attempt %d/%d", aSedCapableNeighbor.GetRloc16(),
                     aSedCapableNeighbor.GetCslTxAttempts(), kMaxCslTriggeredTxAttempts);

        if (aSedCapableNeighbor.GetCslTxAttempts() >= kMaxCslTriggeredTxAttempts)
        {
            // CSL transmission attempts reach max, consider child out of sync
            aSedCapableNeighbor.SetCslSynchronized(false);
            aSedCapableNeighbor.ResetCslTxAttempts();
        }

        OT_FALL_THROUGH;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
    case OT_ERROR_ABORT:

        // Even if CSL tx attempts count reaches max, the message won't be
        // dropped until indirect tx attempts count reaches max. So here it
        // would set sequence number and schedule next CSL tx.

        if (!aFrame.IsEmpty())
        {
            aSedCapableNeighbor.SetIndirectDataSequenceNumber(aFrame.GetSequence());

            if (aFrame.GetSecurityEnabled())
            {
                uint32_t frameCounter;
                uint8_t  keyId;

                IgnoreError(aFrame.GetFrameCounter(frameCounter));
                aSedCapableNeighbor.SetIndirectFrameCounter(frameCounter);

                IgnoreError(aFrame.GetKeyId(keyId));
                aSedCapableNeighbor.SetIndirectKeyId(keyId);
            }
        }

        RescheduleCslTx();
        ExitNow();

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    mCallbacks.HandleSentFrameToSedNeighbor(aFrame, mFrameContext, aError, aSedCapableNeighbor);

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
