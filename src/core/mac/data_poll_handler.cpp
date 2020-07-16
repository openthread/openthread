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

#if OPENTHREAD_FTD

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

inline otError DataPollHandler::Callbacks::PrepareFrameForChild(Mac::TxFrame &aFrame,
                                                                FrameContext &aContext,
                                                                Child &       aChild)
{
    return Get<IndirectSender>().PrepareFrameForChild(aFrame, aContext, aChild);
}

inline void DataPollHandler::Callbacks::HandleSentFrameToChild(const Mac::TxFrame &aFrame,
                                                               const FrameContext &aContext,
                                                               otError             aError,
                                                               Child &             aChild)
{
    Get<IndirectSender>().HandleSentFrameToChild(aFrame, aContext, aError, aChild);
}

inline void DataPollHandler::Callbacks::HandleFrameChangeDone(Child &aChild)
{
    Get<IndirectSender>().HandleFrameChangeDone(aChild);
}

//---------------------------------------------------------

DataPollHandler::DataPollHandler(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIndirectTxChild(nullptr)
    , mFrameContext()
    , mCallbacks(aInstance)
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

void DataPollHandler::HandleNewFrame(Child &aChild)
{
    OT_UNUSED_VARIABLE(aChild);

    // There is no need to take any action with current data poll
    // handler implementation, since the preparation of the frame
    // happens after receiving of a data poll from the child. This
    // method is included for use by other data poll handler models
    // (e.g., in RCP/host model if the handling of data polls is
    // delegated to RCP).
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
        mCallbacks.HandleFrameChangeDone(aChild);
    }
}

void DataPollHandler::HandleDataPoll(Mac::RxFrame &aFrame)
{
    Mac::Address macSource;
    Child *      child;
    uint16_t     indirectMsgCount;

    VerifyOrExit(aFrame.GetSecurityEnabled(), OT_NOOP);
    VerifyOrExit(!Get<Mle::MleRouter>().IsDetached(), OT_NOOP);

    SuccessOrExit(aFrame.GetSrcAddr(macSource));
    child = Get<ChildTable>().FindChild(macSource, Child::kInStateValidOrRestoring);
    VerifyOrExit(child != nullptr, OT_NOOP);

    child->SetLastHeard(TimerMilli::GetNow());
    child->ResetLinkFailures();
    indirectMsgCount = child->GetIndirectMessageCount();

    otLogInfoMac("Rx data poll, src:0x%04x, qed_msgs:%d, rss:%d, ack-fp:%d", child->GetRloc16(), indirectMsgCount,
                 aFrame.GetRssi(), aFrame.IsAckedWithFramePending());

    if (!aFrame.IsAckedWithFramePending())
    {
        if ((indirectMsgCount > 0) && macSource.IsShort())
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

otError DataPollHandler::HandleFrameRequest(Mac::TxFrame &aFrame)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mIndirectTxChild != nullptr, error = OT_ERROR_ABORT);

    SuccessOrExit(error = mCallbacks.PrepareFrameForChild(aFrame, mFrameContext, *mIndirectTxChild));

    if (mIndirectTxChild->GetIndirectTxAttempts() > 0)
    {
        // For a re-transmission of an indirect frame to a sleepy
        // child, we ensure to use the same frame counter, key id, and
        // data sequence number as the previous attempt.

        aFrame.SetIsARetransmission(true);
        aFrame.SetSequence(mIndirectTxChild->GetIndirectDataSequenceNumber());

        if (aFrame.GetSecurityEnabled())
        {
            aFrame.SetFrameCounter(mIndirectTxChild->GetIndirectFrameCounter());
            aFrame.SetKeyId(mIndirectTxChild->GetIndirectKeyId());
        }
    }
    else
    {
        aFrame.SetIsARetransmission(false);
    }

exit:
    return error;
}

void DataPollHandler::HandleSentFrame(const Mac::TxFrame &aFrame, otError aError)
{
    Child *child = mIndirectTxChild;

    VerifyOrExit(child != nullptr, OT_NOOP);

    mIndirectTxChild = nullptr;
    HandleSentFrame(aFrame, aError, *child);

exit:
    ProcessPendingPolls();
}

void DataPollHandler::HandleSentFrame(const Mac::TxFrame &aFrame, otError aError, Child &aChild)
{
    if (aChild.IsFramePurgePending())
    {
        aChild.SetFramePurgePending(false);
        aChild.SetFrameReplacePending(false);
        aChild.ResetIndirectTxAttempts();
        mCallbacks.HandleFrameChangeDone(aChild);
        ExitNow();
    }

    switch (aError)
    {
    case OT_ERROR_NONE:
        aChild.ResetIndirectTxAttempts();
        aChild.SetFrameReplacePending(false);
        break;

    case OT_ERROR_NO_ACK:
        aChild.IncrementIndirectTxAttempts();

        otLogInfoMac("Indirect tx to child %04x failed, attempt %d/%d", aChild.GetRloc16(),
                     aChild.GetIndirectTxAttempts(), kMaxPollTriggeredTxAttempts);

        // Fall through

    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
    case OT_ERROR_ABORT:

        if (aChild.IsFrameReplacePending())
        {
            aChild.SetFrameReplacePending(false);
            aChild.ResetIndirectTxAttempts();
            mCallbacks.HandleFrameChangeDone(aChild);
            ExitNow();
        }

        if ((aChild.GetIndirectTxAttempts() < kMaxPollTriggeredTxAttempts) && !aFrame.IsEmpty())
        {
            // We save the frame counter, key id, and data sequence number of
            // current frame so we use the same values for the retransmission
            // of the frame following the receipt of the next data poll.

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

            ExitNow();
        }

        aChild.ResetIndirectTxAttempts();
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    mCallbacks.HandleSentFrameToChild(aFrame, mFrameContext, aError, aChild);

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

} // namespace ot

#endif // #if OPENTHREAD_FTD
