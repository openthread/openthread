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
 *   This file includes definitions for handling indirect transmission.
 */

#include "indirect_sender.hpp"

#include "instance/instance.hpp"

namespace ot {

#if OPENTHREAD_FTD || OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

const Mac::Address &IndirectSender::NeighborInfo::GetMacAddress(Mac::Address &aMacAddress) const
{
    if (mUseShortAddress)
    {
        aMacAddress.SetShort(static_cast<const CslNeighbor *>(this)->GetRloc16());
    }
    else
    {
        aMacAddress.SetExtended(static_cast<const CslNeighbor *>(this)->GetExtAddress());
    }

    return aMacAddress;
}

IndirectSender::IndirectSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
#if OPENTHREAD_FTD
    , mSourceMatchController(aInstance)
    , mDataPollHandler(aInstance)
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    , mCslTxScheduler(aInstance)
#endif
{
}

void IndirectSender::Stop(void)
{
    VerifyOrExit(mEnabled);

#if OPENTHREAD_FTD
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        child.SetIndirectMessage(nullptr);
        mSourceMatchController.ResetMessageCount(child);
    }

    mDataPollHandler.Clear();
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    mCslTxScheduler.Clear();
#endif

exit:
    mEnabled = false;
}

#if OPENTHREAD_FTD

bool IndirectSender::IsChild(const Neighbor &aNeighbor) const { return Get<ChildTable>().Contains(aNeighbor); }

void IndirectSender::AddMessageForSleepyChild(Message &aMessage, Child &aChild)
{
    uint16_t childIndex;

    OT_ASSERT(!aChild.IsRxOnWhenIdle());

    childIndex = Get<ChildTable>().GetChildIndex(aChild);
    VerifyOrExit(!aMessage.GetIndirectTxChildMask().Has(childIndex));

    aMessage.GetIndirectTxChildMask().Add(childIndex);
    mSourceMatchController.IncrementMessageCount(aChild);

    if ((aMessage.GetType() != Message::kTypeSupervision) && (aChild.GetIndirectMessageCount() > 1))
    {
        Message *supervisionMessage = FindQueuedMessageForSleepyChild(aChild, AcceptSupervisionMessage);

        if (supervisionMessage != nullptr)
        {
            IgnoreError(RemoveMessageFromSleepyChild(*supervisionMessage, aChild));
            Get<MeshForwarder>().RemoveMessageIfNoPendingTx(*supervisionMessage);
        }
    }

    RequestMessageUpdate(aChild);

exit:
    return;
}

Error IndirectSender::RemoveMessageFromSleepyChild(Message &aMessage, Child &aChild)
{
    Error    error      = kErrorNone;
    uint16_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

    VerifyOrExit(aMessage.GetIndirectTxChildMask().Has(childIndex), error = kErrorNotFound);

    aMessage.GetIndirectTxChildMask().Remove(childIndex);
    mSourceMatchController.DecrementMessageCount(aChild);

    RequestMessageUpdate(aChild);

exit:
    return error;
}

void IndirectSender::ClearAllMessagesForSleepyChild(Child &aChild)
{
    VerifyOrExit(aChild.GetIndirectMessageCount() > 0);

    for (Message &message : Get<MeshForwarder>().mSendQueue)
    {
        message.GetIndirectTxChildMask().Remove(Get<ChildTable>().GetChildIndex(aChild));

        Get<MeshForwarder>().RemoveMessageIfNoPendingTx(message);
    }

    aChild.SetIndirectMessage(nullptr);
    mSourceMatchController.ResetMessageCount(aChild);

    mDataPollHandler.RequestFrameChange(DataPollHandler::kPurgeFrame, aChild);
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    mCslTxScheduler.Update();
#endif

exit:
    return;
}

const Message *IndirectSender::FindQueuedMessageForSleepyChild(const Child &aChild, MessageChecker aChecker) const
{
    const Message *match      = nullptr;
    uint16_t       childIndex = Get<ChildTable>().GetChildIndex(aChild);

    for (const Message &message : Get<MeshForwarder>().mSendQueue)
    {
        if (message.GetIndirectTxChildMask().Has(childIndex) && aChecker(message))
        {
            match = &message;
            break;
        }
    }

    return match;
}

void IndirectSender::SetChildUseShortAddress(Child &aChild, bool aUseShortAddress)
{
    VerifyOrExit(aChild.IsIndirectSourceMatchShort() != aUseShortAddress);

    mSourceMatchController.SetSrcMatchAsShort(aChild, aUseShortAddress);

exit:
    return;
}

void IndirectSender::HandleChildModeChange(Child &aChild, Mle::DeviceMode aOldMode)
{
    if (!aChild.IsRxOnWhenIdle() && (aChild.IsStateValid()))
    {
        SetChildUseShortAddress(aChild, true);
    }

    // On sleepy to non-sleepy mode change, convert indirect messages in
    // the send queue destined to the child to direct.

    if (!aOldMode.IsRxOnWhenIdle() && aChild.IsRxOnWhenIdle() && (aChild.GetIndirectMessageCount() > 0))
    {
        uint16_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

        for (Message &message : Get<MeshForwarder>().mSendQueue)
        {
            if (message.GetIndirectTxChildMask().Has(childIndex))
            {
                message.GetIndirectTxChildMask().Remove(childIndex);
                message.SetDirectTransmission();
                message.SetTimestampToNow();
            }
        }

        aChild.SetIndirectMessage(nullptr);
        mSourceMatchController.ResetMessageCount(aChild);

        mDataPollHandler.RequestFrameChange(DataPollHandler::kPurgeFrame, aChild);
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        mCslTxScheduler.Update();
#endif
    }

    // Since the queuing delays for direct transmissions are expected to
    // be relatively small especially when compared to indirect, for a
    // non-sleepy to sleepy mode change, we allow any direct message
    // (for the child) already in the send queue to remain as is. This
    // is equivalent to dropping the already queued messages in this
    // case.
}

void IndirectSender::RequestMessageUpdate(Child &aChild)
{
    Message *curMessage = aChild.GetIndirectMessage();
    Message *newMessage;

    // Purge the frame if the current message is no longer destined
    // for the child. This check needs to be done first to cover the
    // case where we have a pending "replace frame" request and while
    // waiting for the callback, the current message is removed.

    if ((curMessage != nullptr) && !curMessage->GetIndirectTxChildMask().Has(Get<ChildTable>().GetChildIndex(aChild)))
    {
        // Set the indirect message for this child to `nullptr` to ensure
        // it is not processed on `HandleSentFrameToCslNeighbor()` callback.

        aChild.SetIndirectMessage(nullptr);

        // Request a "frame purge" using `RequestFrameChange()` and
        // wait for `HandleFrameChangeDone()` callback for completion
        // of the request. Note that the callback may be directly
        // called from the `RequestFrameChange()` itself when the
        // request can be handled immediately.

        aChild.SetWaitingForMessageUpdate(true);
        mDataPollHandler.RequestFrameChange(DataPollHandler::kPurgeFrame, aChild);
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        mCslTxScheduler.Update();
#endif

        ExitNow();
    }

    VerifyOrExit(!aChild.IsWaitingForMessageUpdate());

    newMessage = FindQueuedMessageForSleepyChild(aChild, AcceptAnyMessage);

    VerifyOrExit(curMessage != newMessage);

    if (curMessage == nullptr)
    {
        // Current message is `nullptr`, but new message is not.
        // We have a new indirect message.

        UpdateIndirectMessage(aChild);
        ExitNow();
    }

    // Current message and new message differ and are both
    // non-`nullptr`. We need to request the frame to be replaced.
    // The current indirect message can be replaced only if it is
    // the first fragment. If a next fragment frame for message is
    // already prepared, we wait for the entire message to be
    // delivered.

    VerifyOrExit(aChild.GetIndirectFragmentOffset() == 0);

    aChild.SetWaitingForMessageUpdate(true);
    mDataPollHandler.RequestFrameChange(DataPollHandler::kReplaceFrame, aChild);
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    mCslTxScheduler.Update();
#endif

exit:
    return;
}

void IndirectSender::HandleFrameChangeDone(Child &aChild)
{
    VerifyOrExit(aChild.IsWaitingForMessageUpdate());
    UpdateIndirectMessage(aChild);

exit:
    return;
}

#endif // OPENTHREAD_FTD

void IndirectSender::UpdateIndirectMessage(CslNeighbor &aNeighbor)
{
    Message *message = nullptr;

#if OPENTHREAD_FTD
    if (IsChild(aNeighbor))
    {
        message = FindQueuedMessageForSleepyChild(static_cast<const Child &>(aNeighbor), AcceptAnyMessage);
    }
#endif

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    if (message == nullptr)
    {
        message = FindQueuedMessageForWedNeighbor(aNeighbor);
    }
#endif

    aNeighbor.SetWaitingForMessageUpdate(false);
    aNeighbor.SetIndirectMessage(message);
    aNeighbor.SetIndirectFragmentOffset(0);
    aNeighbor.SetIndirectTxSuccess(true);

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    mCslTxScheduler.Update();
#endif

    if (message != nullptr)
    {
        Mac::Address neighborAddress;

        aNeighbor.GetMacAddress(neighborAddress);
        Get<MeshForwarder>().LogMessage(MeshForwarder::kMessagePrepareIndirect, *message, kErrorNone, &neighborAddress);
    }
}

#if OPENTHREAD_FTD

Error IndirectSender::PrepareFrameForChild(Mac::TxFrame &aFrame, FrameContext &aContext, Child &aChild)
{
    Error    error   = kErrorNone;
    Message *message = aChild.GetIndirectMessage();

    VerifyOrExit(mEnabled, error = kErrorAbort);

    if (message == nullptr)
    {
        PrepareEmptyFrame(aFrame, aChild, /* aAckRequest */ true);
        aContext.mMessageNextOffset = 0;
        ExitNow();
    }

    switch (message->GetType())
    {
    case Message::kTypeIp6:
        aContext.mMessageNextOffset = PrepareDataFrameForSleepyChild(aFrame, aChild, *message);
        break;

    case Message::kTypeSupervision:
        PrepareEmptyFrame(aFrame, aChild, /* aAckRequest */ true);
        aContext.mMessageNextOffset = message->GetLength();
        break;

    default:
        OT_ASSERT(false);
    }

exit:
    return error;
}

#endif // OPENTHREAD_FTD

uint16_t IndirectSender::PrepareDataFrame(Mac::TxFrame &aFrame, CslNeighbor &aNeighbor, Message &aMessage)
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
        aNeighbor.GetMacAddress(macAddrs.mDestination);
    }

    // Prepare the data frame from previous neighbor's indirect offset.

    directTxOffset = aMessage.GetOffset();
    aMessage.SetOffset(aNeighbor.GetIndirectFragmentOffset());

    nextOffset = Get<MeshForwarder>().PrepareDataFrameWithNoMeshHeader(aFrame, aMessage, macAddrs);

    aMessage.SetOffset(directTxOffset);

    return nextOffset;
}

#if OPENTHREAD_FTD

uint16_t IndirectSender::PrepareDataFrameForSleepyChild(Mac::TxFrame &aFrame, Child &aChild, Message &aMessage)
{
    uint16_t nextOffset;

    nextOffset = PrepareDataFrame(aFrame, aChild, aMessage);

    // Set `FramePending` if there are more queued messages (excluding
    // the current one being sent out) for the child (note `> 1` check).
    // The case where the current message itself requires fragmentation
    // is already checked and handled in `PrepareDataFrameForSleepyChild()`
    // method.

    if (aChild.GetIndirectMessageCount() > 1)
    {
        aFrame.SetFramePending(true);
    }

    return nextOffset;
}

void IndirectSender::PrepareEmptyFrame(Mac::TxFrame &aFrame, Child &aChild, bool aAckRequest)
{
    Mac::Address macDest;
    aChild.GetMacAddress(macDest);
    Get<MeshForwarder>().PrepareEmptyFrame(aFrame, macDest, aAckRequest);
}

#endif // OPENTHREAD_FTD

void IndirectSender::HandleSentFrameToCslNeighbor(const Mac::TxFrame &aFrame,
                                                  const FrameContext &aContext,
                                                  Error               aError,
                                                  CslNeighbor        &aNeighbor)
{
    Message *message    = aNeighbor.GetIndirectMessage();
    uint16_t nextOffset = aContext.mMessageNextOffset;

#if OPENTHREAD_FTD
    Child *child = nullptr;
#endif

    VerifyOrExit(mEnabled);

#if OPENTHREAD_FTD
    if (IsChild(aNeighbor))
    {
        child = static_cast<Child *>(&aNeighbor);
    }

    if (aError == kErrorNone && child != nullptr)
    {
        Get<ChildSupervisor>().UpdateOnSend(*child);
    }

    // A zero `nextOffset` indicates that the sent frame is an empty
    // frame generated by `PrepareFrameForChild()` when there was no
    // indirect message in the send queue for the child. This can happen
    // in the (not common) case where the radio platform does not
    // support the "source address match" feature and always includes
    // "frame pending" flag in acks to data poll frames. In such a case,
    // `IndirectSender` prepares and sends an empty frame to the child
    // after it sends a data poll. Here in `HandleSentFrameToCslNeighbor()` we
    // exit quickly if we detect the "send done" is for the empty frame
    // to ensure we do not update any newly added indirect message after
    // preparing the empty frame.

    VerifyOrExit(nextOffset != 0);
#endif // OPENTHREAD_FTD

    switch (aError)
    {
    case kErrorNone:
        break;

    case kErrorNoAck:
    case kErrorChannelAccessFailure:
    case kErrorAbort:

        aNeighbor.SetIndirectTxSuccess(false);

#if OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE
        // We set the nextOffset to end of message, since there is no need to
        // send any remaining fragments in the message to the neighbor, if all tx
        // attempts of current frame already failed.

        if (message != nullptr)
        {
            nextOffset = message->GetLength();
        }
#endif
        break;

    default:
        OT_ASSERT(false);
    }

    if ((message != nullptr) && (nextOffset < message->GetLength()))
    {
        aNeighbor.SetIndirectFragmentOffset(nextOffset);
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        mCslTxScheduler.Update();
#endif
        ExitNow();
    }

    if (message != nullptr)
    {
        // The indirect tx of this message to the child is done.

        Error        txError = aError;
        Mac::Address macDest;

        aNeighbor.SetIndirectMessage(nullptr);
        aNeighbor.GetLinkInfo().AddMessageTxStatus(aNeighbor.GetIndirectTxSuccess());

#if OPENTHREAD_FTD
        if (child != nullptr)
        {
            // Enable short source address matching after the first indirect
            // message transmission attempt to the child. We intentionally do
            // not check for successful tx here to address the scenario where
            // the child does receive "Child ID Response" but parent misses the
            // 15.4 ack from child. If the "Child ID Response" does not make it
            // to the child, then the child will need to send a new "Child ID
            // Request" which will cause the parent to switch to using long
            // address mode for source address matching.

            mSourceMatchController.SetSrcMatchAsShort(*child, true);
        }
#endif

#if !OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE

        // When `CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE` is
        // disabled, all fragment frames of a larger message are
        // sent even if the transmission of an earlier fragment fail.
        // Note that `GetIndirectTxSuccess() tracks the tx success of
        // the entire message to the neighbor, while `txError = aError`
        // represents the error status of the last fragment frame
        // transmission.

        if (!aNeighbor.GetIndirectTxSuccess() && (txError == kErrorNone))
        {
            txError = kErrorFailed;
        }
#endif

        if (!aFrame.IsEmpty())
        {
            IgnoreError(aFrame.GetDstAddr(macDest));
            Get<MeshForwarder>().LogMessage(MeshForwarder::kMessageTransmit, *message, txError, &macDest);
        }

        if (message->GetType() == Message::kTypeIp6)
        {
            if (aNeighbor.GetIndirectTxSuccess())
            {
                Get<MeshForwarder>().mIpCounters.mTxSuccess++;
            }
            else
            {
                Get<MeshForwarder>().mIpCounters.mTxFailure++;
            }
        }

#if OPENTHREAD_FTD
        if (child != nullptr)
        {
            uint16_t childIndex = Get<ChildTable>().GetChildIndex(*child);

            if (message->GetIndirectTxChildMask().Has(childIndex))
            {
                message->GetIndirectTxChildMask().Remove(childIndex);
                mSourceMatchController.DecrementMessageCount(*child);
            }
        }
#endif

        Get<MeshForwarder>().RemoveMessageIfNoPendingTx(*message);
    }

    UpdateIndirectMessage(aNeighbor);

exit:
#if OPENTHREAD_FTD
    if (mEnabled && (child != nullptr))
    {
        ClearMessagesForRemovedChildren();
    }
#else
    return;
#endif
}

#if OPENTHREAD_FTD

void IndirectSender::ClearMessagesForRemovedChildren(void)
{
    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptValidOrRestoring))
    {
        if (child.GetIndirectMessageCount() == 0)
        {
            continue;
        }

        ClearAllMessagesForSleepyChild(child);
    }
}

bool IndirectSender::AcceptAnyMessage(const Message &aMessage)
{
    OT_UNUSED_VARIABLE(aMessage);

    return true;
}

bool IndirectSender::AcceptSupervisionMessage(const Message &aMessage)
{
    return aMessage.GetType() == Message::kTypeSupervision;
}

#endif // OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
void IndirectSender::AddMessageForEnhCslNeighbor(Message &aMessage, CslNeighbor &aNeighbor)
{
    OT_UNUSED_VARIABLE(aNeighbor);

    aMessage.SetForEnhancedCslNeighbor(true);
}

Error IndirectSender::PrepareFrameForEnhCslNeighbor(Mac::TxFrame &aFrame,
                                                    FrameContext &aContext,
                                                    CslNeighbor  &aNeighbor)
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

Message *IndirectSender::FindQueuedMessageForWedNeighbor(CslNeighbor &aNeighbor)
{
    Message *match = nullptr;

    OT_UNUSED_VARIABLE(aNeighbor);

    for (Message &message : Get<MeshForwarder>().mSendQueue)
    {
        if (message.IsForEnhancedCslNeighbor())
        {
            match = &message;
            break;
        }
    }

    return match;
}
#endif // OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE

Error IndirectSender::PrepareFrameForCslNeighbor(Mac::TxFrame &aFrame,
                                                 FrameContext &aContext,
                                                 CslNeighbor  &aCslNeighbor)
{
    Error error = kErrorNotFound;

#if OPENTHREAD_FTD
    if (IsChild(aCslNeighbor))
    {
        error = PrepareFrameForChild(aFrame, aContext, static_cast<Child &>(aCslNeighbor));
        ExitNow();
    }
#endif
#if OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE
    error = PrepareFrameForEnhCslNeighbor(aFrame, aContext, aCslNeighbor);
    ExitNow();
#else
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aContext);
    OT_UNUSED_VARIABLE(aCslNeighbor);
    ExitNow();
#endif

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_CSL_TRANSMITTER_ENABLE

#endif // OPENTHREAD_FTD || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE

} // namespace ot
