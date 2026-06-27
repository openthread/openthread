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

#include "data_poll_handler.hpp"

#if OPENTHREAD_FTD

#include "instance/instance.hpp"

namespace ot {

RegisterLogModule("DataPollHandlr");

DataPollHandler::DataPollHandler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIndirectTxChild(nullptr)
    , mFrameContext()
{
}

void DataPollHandler::Clear(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        child.SetDataPollPending(false);
        child.SetFrameReplacePending(false);
        child.SetFramePurgePending(false);
        child.ResetIndirectTxAttempts();
    }

    mIndirectTxChild = nullptr;
}

void DataPollHandler::RequestFrameChange(FrameChange aChange, Child &aChild)
{
    if ((mIndirectTxChild == &aChild) && Get<Mac::Mac>().IsPerformingIndirectTransmit())
    {
        switch (aChange)
        {
        case kReplaceFrame:
            aChild.SetFrameReplacePending(true);
            break;

        case kPurgeFrame:
            aChild.SetFramePurgePending(true);
            break;
        }
    }
    else
    {
        ResetTxAttempts(aChild);
        Get<IndirectSender>().HandleFrameChangeDone(aChild);
    }
}

void DataPollHandler::HandleDataPoll(Mac::RxFrame::Info &aFrameInfo)
{
    Child   *child;
    uint16_t indirectMsgCount;

    VerifyOrExit(aFrameInfo.IsSecuredWith(Mac::RxFrame::kAllowKeyIdMode1));

    VerifyOrExit(!Get<Mle::Mle>().IsDetached());

    child = Get<ChildTable>().FindChild(aFrameInfo.GetSrcAddr(), Child::kInStateValidOrRestoring);
    VerifyOrExit(child != nullptr);

    child->SetLastHeard(TimerMilli::GetNow());
    child->ResetLinkFailures();
#if OPENTHREAD_CONFIG_MULTI_RADIO
    child->SetLastPollRadioType(aFrameInfo.GetRxFrame().GetRadioType());
#endif

    indirectMsgCount = child->GetIndirectMessageCount();

    LogInfo("Rx data poll, src:0x%04x, qed_msgs:%d, rss:%d, ack-fp:%d", child->GetRloc16(), indirectMsgCount,
            aFrameInfo.GetRxFrame().GetRssi(), aFrameInfo.GetRxFrame().IsAckedWithFramePending());

    if (!aFrameInfo.GetRxFrame().IsAckedWithFramePending())
    {
        if ((indirectMsgCount > 0) && aFrameInfo.GetSrcAddr().IsShort())
        {
            Get<SourceMatchController>().SetSrcMatchAsShort(*child, true);
        }

        ExitNow();
    }

    if (mIndirectTxChild == nullptr)
    {
        mIndirectTxChild = child;
        Get<Mac::Mac>().RequestIndirectFrameTransmission();
    }
    else
    {
        child->SetDataPollPending(true);
    }

exit:
    return;
}

Mac::TxFrame *DataPollHandler::HandleFrameRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame = nullptr;

    VerifyOrExit(mIndirectTxChild != nullptr);

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &aTxFrames.GetTxFrame(mIndirectTxChild->GetLastPollRadioType());
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    VerifyOrExit(Get<IndirectSender>().PrepareFrameForChild(*frame, mFrameContext, *mIndirectTxChild) == kErrorNone,
                 frame = nullptr);

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    if ((mIndirectTxChild->GetIndirectTxAttempts() > 0) || (mIndirectTxChild->GetCslTxAttempts() > 0))
#else
    if (mIndirectTxChild->GetIndirectTxAttempts() > 0)
#endif
    {
        // For a re-transmission of an indirect frame to a sleepy
        // child, we ensure to use the same frame counter, key id, and
        // data sequence number as the previous attempt.

        frame->SetIsARetransmission(true);
        frame->SetSequence(mIndirectTxChild->GetIndirectDataSequenceNumber());

        if (frame->GetSecurityEnabled())
        {
            frame->SetFrameCounter(mIndirectTxChild->GetIndirectFrameCounter());
            frame->SetKeyId(mIndirectTxChild->GetIndirectKeyId());
        }
    }
    else
    {
        frame->SetIsARetransmission(false);
    }

exit:
    return frame;
}

void DataPollHandler::HandleSentFrame(const Mac::TxFrame::Info &aFrameInfo, Error aError)
{
    Child *child = mIndirectTxChild;

    VerifyOrExit(child != nullptr);

    mIndirectTxChild = nullptr;
    HandleSentFrame(aFrameInfo, aError, *child);

exit:
    ProcessPendingPolls();
}

void DataPollHandler::HandleSentFrame(const Mac::TxFrame::Info &aFrameInfo, Error aError, Child &aChild)
{
    if (aChild.IsFramePurgePending())
    {
        aChild.SetFramePurgePending(false);
        aChild.SetFrameReplacePending(false);
        ResetTxAttempts(aChild);
        Get<IndirectSender>().HandleFrameChangeDone(aChild);
        ExitNow();
    }

    switch (aError)
    {
    case kErrorNone:
        ResetTxAttempts(aChild);
        aChild.SetFrameReplacePending(false);
        break;

    case kErrorNoAck:
        OT_ASSERT(aFrameInfo.mIsEmptyFrame || !aFrameInfo.mSecurityEnabled ||
                  aFrameInfo.GetTxFrame().IsHeaderUpdated());

        aChild.IncrementIndirectTxAttempts();
        LogInfo("Indirect tx to child %04x failed, attempt %d/%d", aChild.GetRloc16(), aChild.GetIndirectTxAttempts(),
                kMaxPollTriggeredTxAttempts);

        OT_FALL_THROUGH;

    case kErrorChannelAccessFailure:
    case kErrorAbort:

        if (aChild.IsFrameReplacePending())
        {
            aChild.SetFrameReplacePending(false);
            ResetTxAttempts(aChild);
            Get<IndirectSender>().HandleFrameChangeDone(aChild);
            ExitNow();
        }

        if ((aChild.GetIndirectTxAttempts() < kMaxPollTriggeredTxAttempts) && !aFrameInfo.mIsEmptyFrame)
        {
            // We save the frame counter, key id, and data sequence number of
            // current frame so we use the same values for the retransmission
            // of the frame following the receipt of the next data poll.

            aChild.SetIndirectDataSequenceNumber(aFrameInfo.mSequenceNum);

            if (aFrameInfo.mSecurityEnabled && aFrameInfo.GetTxFrame().IsHeaderUpdated())
            {
                aChild.SetIndirectFrameCounter(aFrameInfo.mFrameCounter);
                aChild.SetIndirectKeyId(aFrameInfo.mKeyId);
            }

            ExitNow();
        }

        aChild.ResetIndirectTxAttempts();
        break;

    default:
        OT_ASSERT(false);
    }

    Get<IndirectSender>().HandleSentFrameToChild(aFrameInfo, mFrameContext, aError, aChild);

exit:
    return;
}

void DataPollHandler::ProcessPendingPolls(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
    {
        if (!child.IsDataPollPending())
        {
            continue;
        }

        // Find the child with earliest poll receive time.

        if ((mIndirectTxChild == nullptr) || (child.GetLastHeard() < mIndirectTxChild->GetLastHeard()))
        {
            mIndirectTxChild = &child;
        }
    }

    if (mIndirectTxChild != nullptr)
    {
        mIndirectTxChild->SetDataPollPending(false);
        Get<Mac::Mac>().RequestIndirectFrameTransmission();
    }
}

void DataPollHandler::ResetTxAttempts(Child &aChild)
{
    aChild.ResetIndirectTxAttempts();

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    aChild.ResetCslTxAttempts();
#endif
}

} // namespace ot

#endif // #if OPENTHREAD_FTD
