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

inline otError CslTxScheduler::Callbacks::PrepareFrameForChild(Mac::TxFrame &aFrame,
                                                               FrameContext &aContext,
                                                               Child &       aChild)
{
    return Get<IndirectSender>().PrepareFrameForChild(aFrame, aContext, aChild);
}

inline void CslTxScheduler::Callbacks::HandleSentFrameToChild(const Mac::TxFrame &aFrame,
                                                              const FrameContext &aContext,
                                                              otError             aError,
                                                              Child &             aChild)
{
    Get<IndirectSender>().HandleSentFrameToChild(aFrame, aContext, aError, aChild);
}

//---------------------------------------------------------
//
CslTxScheduler::CslTxScheduler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCslTxChild(nullptr)
    , mCslTxMessage(nullptr)
    , mFrameContext()
    , mCallbacks(aInstance)
{
}

void CslTxScheduler::Update(void)
{
    if (mCslTxMessage == nullptr)
    {
        RescheduleCslTx();
    }
    else if ((mCslTxChild != nullptr) && (mCslTxChild->GetIndirectMessage() != mCslTxMessage))
    {
        // `Mac` has already started the CSL tx, so wait for tx done callback
        // to call `RescheduleCslTx`
        mCslTxChild                      = nullptr;
        mFrameContext.mMessageNextOffset = 0;
    }
}

void CslTxScheduler::Clear(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        child.SetCslTxAttempts(0);
        child.SetCslSynchronized(false);
        child.SetCslChannel(0);
        child.SetCslTimeout(0);
        child.SetCslPeriod(0);
        child.SetCslPhase(0);
        child.SetCslLastHeard(TimeMilli(0));
    }

    mFrameContext.mMessageNextOffset = 0;
    mCslTxChild                      = nullptr;
    mCslTxMessage                    = nullptr;
}

/**
 * This method always finds the most recent csl tx among all children,
 * and request `Mac` to do csl tx at specific time. It shouldn't be called
 * when `Mac` is already starting to do the csl tx (indicated by `mCslTxMessage`).
 *
 */
void CslTxScheduler::RescheduleCslTx(void)
{
    uint64_t radioNow     = otPlatRadioGetNow(&GetInstance());
    uint32_t minDelayTime = TimeMicro::kMaxDuration;
    Child *  bestChild    = nullptr;

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        uint32_t delay;

        if (!child.IsCslSynchronized() || child.GetIndirectMessageCount() == 0 ||
            child.GetCslTxAttempts() >= kMaxCslTriggeredTxAttempts)
        {
            continue;
        }

        delay = GetNextCslTransmissionDelay(child, radioNow);
        if (delay < minDelayTime)
        {
            minDelayTime = delay;
            bestChild    = &child;
        }
    }

    if (bestChild != nullptr)
    {
        Get<Mac::Mac>().RequestCslFrameTransmission(minDelayTime / 1000UL);
    }

    mCslTxChild = bestChild;
}

uint32_t CslTxScheduler::GetNextCslTransmissionDelay(const Child &aChild, uint64_t aRadioNow)
{
    uint32_t delay;
    uint16_t period_offset = (aRadioNow / kUsPerTenSymbols) % aChild.GetCslPeriod();

    if (aChild.GetCslPhase() > period_offset + kCslFrameRequestAheadThreshold)
    {
        delay = static_cast<uint16_t>(aChild.GetCslPhase() - period_offset - kCslFrameRequestAheadThreshold) *
                kUsPerTenSymbols;
    }
    else
    {
        delay = static_cast<uint16_t>(aChild.GetCslPeriod() + aChild.GetCslPhase() - period_offset -
                                      kCslFrameRequestAheadThreshold) *
                kUsPerTenSymbols;
    }

    return delay;
}

otError CslTxScheduler::HandleFrameRequest(Mac::TxFrame &aFrame)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mCslTxChild != nullptr, error = OT_ERROR_ABORT);

    SuccessOrExit(error = mCallbacks.PrepareFrameForChild(aFrame, mFrameContext, *mCslTxChild));
    mCslTxMessage = mCslTxChild->GetIndirectMessage();

    if (mCslTxChild->GetIndirectTxAttempts() > 0 || mCslTxChild->GetCslTxAttempts() > 0)
    {
        // For a re-transmission of an indirect frame to a sleepy
        // child, we ensure to use the same frame counter, key id, and
        // data sequence number as the previous attempt.

        aFrame.SetIsARetransmission(true);
        aFrame.SetSequence(mCslTxChild->GetIndirectDataSequenceNumber());

        if (aFrame.GetSecurityEnabled())
        {
            aFrame.SetFrameCounter(mCslTxChild->GetIndirectFrameCounter());
            aFrame.SetKeyId(mCslTxChild->GetIndirectKeyId());
        }
    }
    else
    {
        aFrame.SetIsARetransmission(false);
    }

    aFrame.SetChannel(mCslTxChild->GetCslChannel());
    aFrame.SetTxPhase(mCslTxChild->GetCslPhase());
    aFrame.SetTxPeriod(mCslTxChild->GetCslPeriod());

exit:
    return error;
}

void CslTxScheduler::HandleSentFrame(const Mac::TxFrame &aFrame, otError aError)
{
    Child *child = mCslTxChild;

    VerifyOrExit(child != nullptr, OT_NOOP); // The result is no longer interested by upper layer

    mCslTxChild   = nullptr;
    mCslTxMessage = nullptr;

    HandleSentFrame(aFrame, aError, *child);

exit:
    return;
}

void CslTxScheduler::HandleSentFrame(const Mac::TxFrame &aFrame, otError aError, Child &aChild)
{
    switch (aError)
    {
    case OT_ERROR_NONE:
        aChild.ResetCslTxAttempts();
        aChild.ResetIndirectTxAttempts();
        break;
    case OT_ERROR_NO_ACK:
        aChild.IncrementCslTxAttempts();

        otLogInfoMac("Csl tx to child %04x failed, attempt %d/%d", aChild.GetRloc16(), aChild.GetCslTxAttempts(),
                     kMaxCslTriggeredTxAttempts);

        // Fall through
    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
    case OT_ERROR_ABORT:

        // Even if Csl Tx attempts count reaches max, the message won't be
        // dropped until indirect tx attempts count reaches max. So here it
        // would set sequence number and schedule next csl tx.

        if (!aFrame.IsEmpty())
        {
            aChild.SetIndirectDataSequenceNumber(aFrame.GetSequence());

            if (aFrame.GetSecurityEnabled())
            {
                uint32_t frameCounter;
                uint8_t  keyId;

                IgnoreError(aFrame.GetFrameCounter(frameCounter));
                aChild.SetIndirectFrameCounter(frameCounter);

                IgnoreError(aFrame.GetKeyId(keyId));
                aChild.SetIndirectKeyId(keyId);
            }
        }

        RescheduleCslTx();
        ExitNow();

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    mCallbacks.HandleSentFrameToChild(aFrame, mFrameContext, aError, aChild);

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
