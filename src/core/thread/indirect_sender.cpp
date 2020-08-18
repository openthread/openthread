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

#if OPENTHREAD_FTD

#include "indirect_sender.hpp"

#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle_tlvs.hpp"
#include "thread/topology.hpp"

namespace ot {

const Mac::Address &IndirectSender::ChildInfo::GetMacAddress(Mac::Address &aMacAddress) const
{
    if (mUseShortAddress)
    {
        aMacAddress.SetShort(static_cast<const Child *>(this)->GetRloc16());
    }
    else
    {
        aMacAddress.SetExtended(static_cast<const Child *>(this)->GetExtAddress());
    }

    return aMacAddress;
}

IndirectSender::IndirectSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
    , mSourceMatchController(aInstance)
    , mDataPollHandler(aInstance)
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    , mCslTxScheduler(aInstance)
#endif
{
}

void IndirectSender::Stop(void)
{
    VerifyOrExit(mEnabled, OT_NOOP);

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        child.SetIndirectMessage(nullptr);
        mSourceMatchController.ResetMessageCount(child);
    }

    mDataPollHandler.Clear();
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
    mCslTxScheduler.Clear();
#endif

exit:
    mEnabled = false;
}

void IndirectSender::AddMessageForSleepyChild(Message &aMessage, Child &aChild)
{
    uint16_t childIndex;

    OT_ASSERT(!aChild.IsRxOnWhenIdle());

    childIndex = Get<ChildTable>().GetChildIndex(aChild);
    VerifyOrExit(!aMessage.GetChildMask(childIndex), OT_NOOP);

    aMessage.SetChildMask(childIndex);
    mSourceMatchController.IncrementMessageCount(aChild);

    RequestMessageUpdate(aChild);

exit:
    return;
}

otError IndirectSender::RemoveMessageFromSleepyChild(Message &aMessage, Child &aChild)
{
    otError  error      = OT_ERROR_NONE;
    uint16_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

    VerifyOrExit(aMessage.GetChildMask(childIndex), error = OT_ERROR_NOT_FOUND);

    aMessage.ClearChildMask(childIndex);
    mSourceMatchController.DecrementMessageCount(aChild);

    RequestMessageUpdate(aChild);

exit:
    return error;
}

void IndirectSender::ClearAllMessagesForSleepyChild(Child &aChild)
{
    Message *message;
    Message *nextMessage;

    VerifyOrExit(aChild.GetIndirectMessageCount() > 0, OT_NOOP);

    for (message = Get<MeshForwarder>().mSendQueue.GetHead(); message; message = nextMessage)
    {
        nextMessage = message->GetNext();

        message->ClearChildMask(Get<ChildTable>().GetChildIndex(aChild));

        if (!message->IsChildPending() && !message->GetDirectTransmission())
        {
            if (Get<MeshForwarder>().mSendMessage == message)
            {
                Get<MeshForwarder>().mSendMessage = nullptr;
            }

            Get<MeshForwarder>().mSendQueue.Dequeue(*message);
            message->Free();
        }
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

void IndirectSender::SetChildUseShortAddress(Child &aChild, bool aUseShortAddress)
{
    VerifyOrExit(aChild.IsIndirectSourceMatchShort() != aUseShortAddress, OT_NOOP);

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

        for (Message *message = Get<MeshForwarder>().mSendQueue.GetHead(); message; message = message->GetNext())
        {
            if (message->GetChildMask(childIndex))
            {
                message->ClearChildMask(childIndex);
                message->SetDirectTransmission();
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

Message *IndirectSender::FindIndirectMessage(Child &aChild)
{
    Message *message;
    Message *next;
    uint16_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

    for (message = Get<MeshForwarder>().mSendQueue.GetHead(); message; message = next)
    {
        next = message->GetNext();

        if (message->GetChildMask(childIndex))
        {
            // Skip and remove the supervision message if there are
            // other messages queued for the child.

            if ((message->GetType() == Message::kTypeSupervision) && (aChild.GetIndirectMessageCount() > 1))
            {
                message->ClearChildMask(childIndex);
                mSourceMatchController.DecrementMessageCount(aChild);
                Get<MeshForwarder>().mSendQueue.Dequeue(*message);
                message->Free();
                continue;
            }

            break;
        }
    }

    return message;
}

void IndirectSender::RequestMessageUpdate(Child &aChild)
{
    Message *curMessage = aChild.GetIndirectMessage();
    Message *newMessage;

    // Purge the frame if the current message is no longer destined
    // for the child. This check needs to be done first to cover the
    // case where we have a pending "replace frame" request and while
    // waiting for the callback, the current message is removed.

    if ((curMessage != nullptr) && !curMessage->GetChildMask(Get<ChildTable>().GetChildIndex(aChild)))
    {
        // Set the indirect message for this child to nullptr to ensure
        // it is not processed on `HandleSentFrameToChild()` callback.

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

    VerifyOrExit(!aChild.IsWaitingForMessageUpdate(), OT_NOOP);

    newMessage = FindIndirectMessage(aChild);

    VerifyOrExit(curMessage != newMessage, OT_NOOP);

    if (curMessage == nullptr)
    {
        // Current message is nullptr, but new message is not.
        // We have a new indirect message.

        UpdateIndirectMessage(aChild);
        ExitNow();
    }

    // Current message and new message differ and are both non-nullptr.
    // We need to request the frame to be replaced. The current
    // indirect message can be replaced only if it is the first
    // fragment. If a next fragment frame for message is already
    // prepared, we wait for the entire message to be delivered.

    VerifyOrExit(aChild.GetIndirectFragmentOffset() == 0, OT_NOOP);

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
    VerifyOrExit(aChild.IsWaitingForMessageUpdate(), OT_NOOP);
    UpdateIndirectMessage(aChild);

exit:
    return;
}

void IndirectSender::UpdateIndirectMessage(Child &aChild)
{
    Message *message = FindIndirectMessage(aChild);

    aChild.SetWaitingForMessageUpdate(false);
    aChild.SetIndirectMessage(message);
    aChild.SetIndirectFragmentOffset(0);
    aChild.SetIndirectTxSuccess(true);

    if (message != nullptr)
    {
        Mac::Address childAddress;

        mDataPollHandler.HandleNewFrame(aChild);
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        mCslTxScheduler.Update();
#endif

        aChild.GetMacAddress(childAddress);
        Get<MeshForwarder>().LogMessage(MeshForwarder::kMessagePrepareIndirect, *message, &childAddress, OT_ERROR_NONE);
    }
}

otError IndirectSender::PrepareFrameForChild(Mac::TxFrame &aFrame, FrameContext &aContext, Child &aChild)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = aChild.GetIndirectMessage();

    VerifyOrExit(mEnabled, error = OT_ERROR_ABORT);

    if (message == nullptr)
    {
        PrepareEmptyFrame(aFrame, aChild, /* aAckRequest */ true);
        ExitNow();
    }

    switch (message->GetType())
    {
    case Message::kTypeIp6:
        aContext.mMessageNextOffset = PrepareDataFrame(aFrame, aChild, *message);
        break;

    case Message::kTypeSupervision:
        PrepareEmptyFrame(aFrame, aChild, kSupervisionMsgAckRequest);
        aContext.mMessageNextOffset = message->GetLength();
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

exit:
    return error;
}

uint16_t IndirectSender::PrepareDataFrame(Mac::TxFrame &aFrame, Child &aChild, Message &aMessage)
{
    Ip6::Header  ip6Header;
    Mac::Address macSource, macDest;
    uint16_t     directTxOffset;
    uint16_t     nextOffset;

    // Determine the MAC source and destination addresses.

    aMessage.Read(0, sizeof(ip6Header), &ip6Header);

    Get<MeshForwarder>().GetMacSourceAddress(ip6Header.GetSource(), macSource);

    if (ip6Header.GetDestination().IsLinkLocal())
    {
        Get<MeshForwarder>().GetMacDestinationAddress(ip6Header.GetDestination(), macDest);
    }
    else
    {
        aChild.GetMacAddress(macDest);
    }

    // Prepare the data frame from previous child's indirect offset.

    directTxOffset = aMessage.GetOffset();
    aMessage.SetOffset(aChild.GetIndirectFragmentOffset());

    nextOffset = Get<MeshForwarder>().PrepareDataFrame(aFrame, aMessage, macSource, macDest);

    aMessage.SetOffset(directTxOffset);

    // Set `FramePending` if there are more queued messages (excluding
    // the current one being sent out) for the child (note `> 1` check).
    // The case where the current message itself requires fragmentation
    // is already checked and handled in `PrepareDataFrame()` method.

    if (aChild.GetIndirectMessageCount() > 1)
    {
        aFrame.SetFramePending(true);
    }

    return nextOffset;
}

void IndirectSender::PrepareEmptyFrame(Mac::TxFrame &aFrame, Child &aChild, bool aAckRequest)
{
    uint16_t     fcf;
    Mac::Address macSource, macDest;

    aChild.GetMacAddress(macDest);

    macSource.SetShort(Get<Mac::Mac>().GetShortAddress());

    if (macSource.IsShortAddrInvalid() || macDest.IsExtended())
    {
        macSource.SetExtended(Get<Mac::Mac>().GetExtAddress());
    }

    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfFrameVersion2006 | Mac::Frame::kFcfPanidCompression |
          Mac::Frame::kFcfSecurityEnabled;

    if (aAckRequest)
    {
        fcf |= Mac::Frame::kFcfAckRequest;
    }

    fcf |= (macDest.IsShort()) ? Mac::Frame::kFcfDstAddrShort : Mac::Frame::kFcfDstAddrExt;
    fcf |= (macSource.IsShort()) ? Mac::Frame::kFcfSrcAddrShort : Mac::Frame::kFcfSrcAddrExt;

    aFrame.InitMacHeader(fcf, Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32);

    aFrame.SetDstPanId(Get<Mac::Mac>().GetPanId());
    IgnoreError(aFrame.SetSrcPanId(Get<Mac::Mac>().GetPanId()));
    aFrame.SetDstAddr(macDest);
    aFrame.SetSrcAddr(macSource);
    aFrame.SetPayloadLength(0);
    aFrame.SetFramePending(false);
}

void IndirectSender::HandleSentFrameToChild(const Mac::TxFrame &aFrame,
                                            const FrameContext &aContext,
                                            otError             aError,
                                            Child &             aChild)
{
    Message *message    = aChild.GetIndirectMessage();
    uint16_t nextOffset = aContext.mMessageNextOffset;

    VerifyOrExit(mEnabled, OT_NOOP);

    switch (aError)
    {
    case OT_ERROR_NONE:
        Get<Utils::ChildSupervisor>().UpdateOnSend(aChild);
        break;

    case OT_ERROR_NO_ACK:
    case OT_ERROR_CHANNEL_ACCESS_FAILURE:
    case OT_ERROR_ABORT:

        aChild.SetIndirectTxSuccess(false);

#if OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE
        // We set the nextOffset to end of message, since there is no need to
        // send any remaining fragments in the message to the child, if all tx
        // attempts of current frame already failed.

        if (message != nullptr)
        {
            nextOffset = message->GetLength();
        }
#endif
        break;

    default:
        OT_ASSERT(false);
        OT_UNREACHABLE_CODE(break);
    }

    if ((message != nullptr) && (nextOffset < message->GetLength()))
    {
        aChild.SetIndirectFragmentOffset(nextOffset);
        mDataPollHandler.HandleNewFrame(aChild);
#if OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
        mCslTxScheduler.Update();
#endif
        ExitNow();
    }

    if (message != nullptr)
    {
        // The indirect tx of this message to the child is done.

        otError      txError    = aError;
        uint16_t     childIndex = Get<ChildTable>().GetChildIndex(aChild);
        Mac::Address macDest;

        aChild.SetIndirectMessage(nullptr);
        aChild.GetLinkInfo().AddMessageTxStatus(aChild.GetIndirectTxSuccess());

        // Enable short source address matching after the first indirect
        // message transmission attempt to the child. We intentionally do
        // not check for successful tx here to address the scenario where
        // the child does receive "Child ID Response" but parent misses the
        // 15.4 ack from child. If the "Child ID Response" does not make it
        // to the child, then the child will need to send a new "Child ID
        // Request" which will cause the parent to switch to using long
        // address mode for source address matching.

        mSourceMatchController.SetSrcMatchAsShort(aChild, true);

#if !OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE

        // When `CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE` is
        // disabled, all fragment frames of a larger message are
        // sent even if the transmission of an earlier fragment fail.
        // Note that `GetIndirectTxSuccess() tracks the tx success of
        // the entire message to the child, while `txError = aError`
        // represents the error status of the last fragment frame
        // transmission.

        if (!aChild.GetIndirectTxSuccess() && (txError == OT_ERROR_NONE))
        {
            txError = OT_ERROR_FAILED;
        }
#endif

        if (!aFrame.IsEmpty())
        {
            IgnoreError(aFrame.GetDstAddr(macDest));
            Get<MeshForwarder>().LogMessage(MeshForwarder::kMessageTransmit, *message, &macDest, txError);
        }

        if (message->GetType() == Message::kTypeIp6)
        {
            if (aChild.GetIndirectTxSuccess())
            {
                Get<MeshForwarder>().mIpCounters.mTxSuccess++;
            }
            else
            {
                Get<MeshForwarder>().mIpCounters.mTxFailure++;
            }
        }

        if (message->GetChildMask(childIndex))
        {
            message->ClearChildMask(childIndex);
            mSourceMatchController.DecrementMessageCount(aChild);
        }

        if (!message->GetDirectTransmission() && !message->IsChildPending())
        {
            Get<MeshForwarder>().mSendQueue.Dequeue(*message);
            message->Free();
        }
    }

    UpdateIndirectMessage(aChild);

exit:
    if (mEnabled)
    {
        ClearMessagesForRemovedChildren();
    }
}

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

} // namespace ot

#endif // #if OPENTHREAD_FTD
