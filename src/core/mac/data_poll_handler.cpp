/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes the implementation for handling of data polls and indirect frame transmission.
 */

#if OPENTHREAD_FTD || OPENTHREAD_MTD

#include "data_poll_handler.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"

namespace ot {

DataPollHandler::Callbacks::Callbacks(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

inline otError DataPollHandler::Callbacks::PrepareFrameForNeighbor(Mac::TxFrame &      aFrame,
                                                                   FrameContext &      aContext,
                                                                   SedCapableNeighbor &aSedCapableNeighbor)
{
    return Get<IndirectSender>().PrepareFrameForSedNeighbor(aFrame, aContext, aSedCapableNeighbor);
}

inline void DataPollHandler::Callbacks::HandleSentFrameToNeighbor(const Mac::TxFrame &aFrame,
                                                                  const FrameContext &aContext,
                                                                  otError             aError,
                                                                  SedCapableNeighbor &aSedCapableNeighbor)
{
    Get<IndirectSender>().HandleSentFrameToSedNeighbor(aFrame, aContext, aError, aSedCapableNeighbor);
}

inline void DataPollHandler::Callbacks::HandleFrameChangeDone(SedCapableNeighbor &aSedCapableNeighbor)
{
    Get<IndirectSender>().HandleFrameChangeDone(aSedCapableNeighbor);
}

//---------------------------------------------------------

DataPollHandler::DataPollHandler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIndirectTxNeighbor(nullptr)
    , mFrameContext()
    , mCallbacks(aInstance)
{
}

void DataPollHandler::Clear(void)
{
    for (SedCapableNeighbor &neighbor :
         Get<SedCapableNeighborTable>().Iterate(SedCapableNeighbor::kInStateAnyExceptInvalid))
    {
        neighbor.SetDataPollPending(false);
        neighbor.SetFrameReplacePending(false);
        neighbor.SetFramePurgePending(false);
        neighbor.ResetIndirectTxAttempts();
    }

    mIndirectTxNeighbor = nullptr;
}

void DataPollHandler::HandleNewFrame(SedCapableNeighbor &aSedCapableNeighbor)
{
    OT_UNUSED_VARIABLE(aSedCapableNeighbor);

    // There is no need to take any action with current data poll
    // handler implementation, since the preparation of the frame
    // happens after receiving of a data poll from the child. This
    // method is included for use by other data poll handler models
    // (e.g., in RCP/host model if the handling of data polls is
    // delegated to RCP).
}

void DataPollHandler::RequestFrameChange(FrameChange aChange, SedCapableNeighbor &aSedCapableNeighbor)
{
    if ((mIndirectTxNeighbor == &aSedCapableNeighbor) && Get<Mac::Mac>().IsPerformingIndirectTransmit())
    {
        switch (aChange)
        {
        case kReplaceFrame:
            aSedCapableNeighbor.SetFrameReplacePending(true);
            break;

        case kPurgeFrame:
            aSedCapableNeighbor.SetFramePurgePending(true);
            break;
        }
    }
    else
    {
        mCallbacks.HandleFrameChangeDone(aSedCapableNeighbor);
    }
}

void DataPollHandler::HandleDataPoll(Mac::RxFrame &aFrame)
{
    Mac::Address        macSource;
    SedCapableNeighbor *neighbor;
    uint16_t            indirectMsgCount;

    VerifyOrExit(aFrame.GetSecurityEnabled());
    VerifyOrExit(!Get<Mle::MleRouter>().IsDetached());

    SuccessOrExit(aFrame.GetSrcAddr(macSource));
    neighbor =
        Get<SedCapableNeighborTable>().FindSedCapableNeighbor(macSource, SedCapableNeighbor::kInStateValidOrRestoring);
    VerifyOrExit(neighbor != nullptr);

    neighbor->SetLastHeard(TimerMilli::GetNow());
    neighbor->ResetLinkFailures();
#if OPENTHREAD_CONFIG_MULTI_RADIO
    neighbor->SetLastPollRadioType(aFrame.GetRadioType());
#endif

    indirectMsgCount = neighbor->GetIndirectMessageCount();

    otLogInfoMac("Rx data poll, src:0x%04x, qed_msgs:%d, rss:%d, ack-fp:%d", neighbor->GetRloc16(), indirectMsgCount,
                 aFrame.GetRssi(), aFrame.IsAckedWithFramePending());

    if (!aFrame.IsAckedWithFramePending())
    {
        if ((indirectMsgCount > 0) && macSource.IsShort())
        {
            Get<SourceMatchController>().SetSrcMatchAsShort(*neighbor, true);
        }

        ExitNow();
    }

    if (mIndirectTxNeighbor == nullptr)
    {
        mIndirectTxNeighbor = neighbor;
        Get<Mac::Mac>().RequestIndirectFrameTransmission();
    }
    else
    {
        neighbor->SetDataPollPending(true);
    }

exit:
    return;
}

Mac::TxFrame *DataPollHandler::HandleFrameRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame = nullptr;

    VerifyOrExit(mIndirectTxNeighbor != nullptr);

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(mIndirectTxNeighbor->GetLastPollRadioType());
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    VerifyOrExit(mCallbacks.PrepareFrameForNeighbor(*frame, mFrameContext, *mIndirectTxNeighbor) == OT_ERROR_NONE,
                 frame = nullptr);

    if (mIndirectTxNeighbor->GetIndirectTxAttempts() > 0)
    {
        // For a re-transmission of an indirect frame to a sleepy
        // child, we ensure to use the same frame counter, key id, and
        // data sequence number as the previous attempt.

        frame->SetIsARetransmission(true);
        frame->SetSequence(mIndirectTxNeighbor->GetIndirectDataSequenceNumber());

        if (frame->GetSecurityEnabled())
        {
            frame->SetFrameCounter(mIndirectTxNeighbor->GetIndirectFrameCounter());
            frame->SetKeyId(mIndirectTxNeighbor->GetIndirectKeyId());
        }
    }
    else
    {
        frame->SetIsARetransmission(false);
    }

exit:
    return frame;
}

void DataPollHandler::HandleSentFrame(const Mac::TxFrame &aFrame, otError aError)
{
    SedCapableNeighbor *neighbor = mIndirectTxNeighbor;

    VerifyOrExit(neighbor != nullptr);

    mIndirectTxNeighbor = nullptr;
    HandleSentFrame(aFrame, aError, *neighbor);

exit:
    ProcessPendingPolls();
}

void DataPollHandler::HandleSentFrame(const Mac::TxFrame &aFrame,
                                      otError             aError,
                                      SedCapableNeighbor &aSedCapableNeighbor)
{
    if (aSedCapableNeighbor.IsFramePurgePending())
    {
        aSedCapableNeighbor.SetFramePurgePending(false);
        aSedCapableNeighbor.SetFrameReplacePending(false);
        aSedCapableNeighbor.ResetIndirectTxAttempts();
        mCallbacks.HandleFrameChangeDone(aSedCapableNeighbor);
        ExitNow();
    }

    switch (aError)
    {
    case OT_ERROR_NONE:
        aSedCapableNeighbor.ResetIndirectTxAttempts();
        aSedCapableNeighbor.SetFrameReplacePending(false);
        break;

    case OT_ERROR_NO_ACK:
        aSedCapableNeighbor.IncrementIndirectTxAttempts();

        otLogInfoMac("Indirect tx to child %04x failed, attempt %d/%d", aSedCapableNeighbor.GetRloc16(),
                     aSedCapableNeighbor.GetIndirectTxAttempts(), kMaxPollTriggeredTxAttempts);

        OT_FALL_THROUGH;

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
    case OT_ERROR_ABORT:

        if (aSedCapableNeighbor.IsFrameReplacePending())
        {
            aSedCapableNeighbor.SetFrameReplacePending(false);
            aSedCapableNeighbor.ResetIndirectTxAttempts();
            mCallbacks.HandleFrameChangeDone(aSedCapableNeighbor);
            ExitNow();
        }

        if ((aSedCapableNeighbor.GetIndirectTxAttempts() < kMaxPollTriggeredTxAttempts) && !aFrame.IsEmpty())
        {
            // We save the frame counter, key id, and data sequence number of
            // current frame so we use the same values for the retransmission
            // of the frame following the receipt of the next data poll.

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

            ExitNow();
        }

        aSedCapableNeighbor.ResetIndirectTxAttempts();
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    mCallbacks.HandleSentFrameToNeighbor(aFrame, mFrameContext, aError, aSedCapableNeighbor);

exit:
    return;
}

void DataPollHandler::ProcessPendingPolls(void)
{
    for (SedCapableNeighbor &neighbor :
         Get<SedCapableNeighborTable>().Iterate(SedCapableNeighbor::kInStateValidOrRestoring))
    {
        if (!neighbor.IsDataPollPending())
        {
            continue;
        }

        // Find the child with earliest poll receive time.

        if ((mIndirectTxNeighbor == nullptr) || (neighbor.GetLastHeard() < mIndirectTxNeighbor->GetLastHeard()))
        {
            mIndirectTxNeighbor = &neighbor;
        }
    }

    if (mIndirectTxNeighbor != nullptr)
    {
        mIndirectTxNeighbor->SetDataPollPending(false);
        Get<Mac::Mac>().RequestIndirectFrameTransmission();
    }
}

} // namespace ot

#endif // #if OPENTHREAD_FTD || OPENTHREAD_MTD
