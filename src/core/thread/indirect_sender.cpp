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
#include "thread/topology.hpp"

namespace ot {

IndirectSender::IndirectSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
    , mSourceMatchController(aInstance)
    , mIndirectStartingChild(NULL)
{
}

void IndirectSender::Stop(void)
{
    VerifyOrExit(mEnabled);

    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateAnyExceptInvalid); !iter.IsDone(); iter++)
    {
        iter.GetChild()->SetIndirectMessage(NULL);
        mSourceMatchController.ResetMessageCount(*iter.GetChild());
    }

    mIndirectStartingChild = NULL;

exit:
    mEnabled = false;
}

otError IndirectSender::AddMessageForSleepyChild(Message &aMessage, Child &aChild)
{
    otError error = OT_ERROR_NONE;
    uint8_t childIndex;

    VerifyOrExit(!aChild.IsRxOnWhenIdle(), error = OT_ERROR_INVALID_STATE);

    childIndex = Get<ChildTable>().GetChildIndex(aChild);
    VerifyOrExit(!aMessage.GetChildMask(childIndex), error = OT_ERROR_ALREADY);

    aMessage.SetChildMask(childIndex);
    mSourceMatchController.IncrementMessageCount(aChild);

exit:
    return error;
}

otError IndirectSender::RemoveMessageFromSleepyChild(Message &aMessage, Child &aChild)
{
    otError error      = OT_ERROR_NONE;
    uint8_t childIndex = Get<ChildTable>().GetChildIndex(aChild);

    VerifyOrExit(aMessage.GetChildMask(childIndex), error = OT_ERROR_NOT_FOUND);

    aMessage.ClearChildMask(childIndex);
    mSourceMatchController.DecrementMessageCount(aChild);

    if (aChild.GetIndirectMessage() == &aMessage)
    {
        aChild.SetIndirectMessage(NULL);
    }

exit:
    return error;
}

void IndirectSender::ClearAllMessagesForSleepyChild(Child &aChild)
{
    Message *nextMessage;

    VerifyOrExit(aChild.GetIndirectMessageCount() > 0);

    for (Message *message = Get<MeshForwarder>().mSendQueue.GetHead(); message; message = nextMessage)
    {
        nextMessage = message->GetNext();

        message->ClearChildMask(Get<ChildTable>().GetChildIndex(aChild));

        if (!message->IsChildPending() && !message->GetDirectTransmission())
        {
            if (Get<MeshForwarder>().mSendMessage == message)
            {
                Get<MeshForwarder>().mSendMessage = NULL;
            }

            Get<MeshForwarder>().mSendQueue.Dequeue(*message);
            message->Free();
        }
    }

    aChild.SetIndirectMessage(NULL);
    mSourceMatchController.ResetMessageCount(aChild);

exit:
    return;
}

void IndirectSender::HandleDataPoll(const Mac::Frame &      aFrame,
                                    const Mac::Address &    aMacSource,
                                    const otThreadLinkInfo &aLinkInfo)
{
    Child *  child;
    uint16_t indirectMsgCount;

    // Security Check: only process secure Data Poll frames.
    VerifyOrExit(aLinkInfo.mLinkSecurity);

    VerifyOrExit(Get<Mle::MleRouter>().GetRole() != OT_DEVICE_ROLE_DETACHED);

    child = Get<ChildTable>().FindChild(aMacSource, ChildTable::kInStateValidOrRestoring);
    VerifyOrExit(child != NULL);

    child->SetLastHeard(TimerMilli::GetNow());
    child->ResetLinkFailures();
    indirectMsgCount = child->GetIndirectMessageCount();

    otLogInfoMac("Rx data poll, src:0x%04x, qed_msgs:%d, rss:%d, ack-fp:%d", child->GetRloc16(), indirectMsgCount,
                 aLinkInfo.mRss, aFrame.IsAckedWithFramePending());
    VerifyOrExit(aFrame.IsAckedWithFramePending());

    if (!mSourceMatchController.IsEnabled() || (indirectMsgCount > 0))
    {
        child->SetDataRequestPending(true);
    }

    Get<MeshForwarder>().mScheduleTransmissionTask.Post();

exit:
    return;
}

otError IndirectSender::GetIndirectTransmission(void)
{
    otError error = OT_ERROR_NOT_FOUND;

    UpdateIndirectMessages();

    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValidOrRestoring, mIndirectStartingChild);
         !iter.IsDone(); iter++)
    {
        Child &child = *iter.GetChild();

        if (!child.IsDataRequestPending())
        {
            continue;
        }

        Get<MeshForwarder>().mSendMessage                = child.GetIndirectMessage();
        Get<MeshForwarder>().mSendMessageMaxCsmaBackoffs = Mac::kMaxCsmaBackoffsIndirect;
        Get<MeshForwarder>().mSendMessageMaxFrameRetries = Mac::kMaxFrameRetriesIndirect;

        if (Get<MeshForwarder>().mSendMessage == NULL)
        {
            Get<MeshForwarder>().mSendMessage = GetIndirectTransmission(child);
        }

        if (Get<MeshForwarder>().mSendMessage != NULL)
        {
            PrepareIndirectTransmission(*Get<MeshForwarder>().mSendMessage, child);
        }
        else
        {
            // A NULL `mSendMessage` triggers an empty frame to be sent to the child.

            if (child.IsIndirectSourceMatchShort())
            {
                Get<MeshForwarder>().mMacSource.SetShort(Get<Mac::Mac>().GetShortAddress());
            }
            else
            {
                Get<MeshForwarder>().mMacSource.SetExtended(Get<Mac::Mac>().GetExtAddress());
            }

            child.GetMacAddress(Get<MeshForwarder>().mMacDest);
        }

        // Remember the current child and move it to next one in the
        // list after the indirect transmission has completed.

        mIndirectStartingChild = &child;

        Get<Mac::Mac>().RequestFrameTransmission();
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

Message *IndirectSender::GetIndirectTransmission(Child &aChild)
{
    Message *message = NULL;
    Message *next;
    uint8_t  childIndex = Get<ChildTable>().GetChildIndex(aChild);

    for (message = Get<MeshForwarder>().mSendQueue.GetHead(); message; message = next)
    {
        next = message->GetNext();

        if (message->GetChildMask(childIndex))
        {
            // Skip and remove the supervision message if there are other messages queued for the child.

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

    aChild.SetIndirectMessage(message);
    aChild.SetIndirectFragmentOffset(0);
    aChild.ResetIndirectTxAttempts();
    aChild.SetIndirectTxSuccess(true);

    if (message != NULL)
    {
        Mac::Address macAddr;

        Get<MeshForwarder>().LogMessage(MeshForwarder::kMessagePrepareIndirect, *message,
                                        &aChild.GetMacAddress(macAddr), OT_ERROR_NONE);
    }

    return message;
}

void IndirectSender::PrepareIndirectTransmission(Message &aMessage, const Child &aChild)
{
    if (aChild.GetIndirectTxAttempts() > 0)
    {
        Get<MeshForwarder>().mSendMessageIsARetransmission  = true;
        Get<MeshForwarder>().mSendMessageFrameCounter       = aChild.GetIndirectFrameCounter();
        Get<MeshForwarder>().mSendMessageKeyId              = aChild.GetIndirectKeyId();
        Get<MeshForwarder>().mSendMessageDataSequenceNumber = aChild.GetIndirectDataSequenceNumber();
    }

    aMessage.SetOffset(aChild.GetIndirectFragmentOffset());

    switch (aMessage.GetType())
    {
    case Message::kTypeIp6:
    {
        Ip6::Header ip6Header;

        aMessage.Read(0, sizeof(ip6Header), &ip6Header);

        Get<MeshForwarder>().mAddMeshHeader = false;
        Get<MeshForwarder>().GetMacSourceAddress(ip6Header.GetSource(), Get<MeshForwarder>().mMacSource);

        if (ip6Header.GetDestination().IsLinkLocal())
        {
            Get<MeshForwarder>().GetMacDestinationAddress(ip6Header.GetDestination(), Get<MeshForwarder>().mMacDest);
        }
        else
        {
            aChild.GetMacAddress(Get<MeshForwarder>().mMacDest);
        }

        break;
    }

    case Message::kTypeSupervision:
        aChild.GetMacAddress(Get<MeshForwarder>().mMacDest);
        break;

    default:
        assert(false);
        break;
    }
}

void IndirectSender::UpdateIndirectMessages(void)
{
    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateAnyExceptValidOrRestoring); !iter.IsDone();
         iter++)
    {
        if (iter.GetChild()->GetIndirectMessageCount() == 0)
        {
            continue;
        }

        ClearAllMessagesForSleepyChild(*iter.GetChild());
    }
}

void IndirectSender::HandleSentFrameToChild(const Mac::Frame &aFrame, otError aError, const Mac::Address &aMacDest)
{
    Child *child;

    child = Get<ChildTable>().FindChild(aMacDest, ChildTable::kInStateValidOrRestoring);
    VerifyOrExit(child != NULL);

    child->SetDataRequestPending(false);

    VerifyOrExit(Get<MeshForwarder>().mSendMessage != NULL);

    if (Get<MeshForwarder>().mSendMessage == child->GetIndirectMessage())
    {
        // To ensure fairness in handling of data requests from sleepy
        // children, once a message is completed for indirect transmission to a
        // child (on both success or failure), the `mIndirectStartingChild` is
        // updated to the next `Child` entry after the current one. Subsequent
        // call to `ScheduleTransmissionTask()` will begin the iteration
        // through the children list from this child.

        ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValidOrRestoring, mIndirectStartingChild);
        iter++;
        mIndirectStartingChild = iter.GetChild();

        switch (aError)
        {
        case OT_ERROR_NONE:
            child->ResetIndirectTxAttempts();
            break;

        case OT_ERROR_NO_ACK:
            child->IncrementIndirectTxAttempts();
            // fall through

        case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        case OT_ERROR_ABORT:

            otLogInfoMac("Indirect tx to child %04x failed, attempt %d/%d, error:%s", child->GetRloc16(),
                         child->GetIndirectTxAttempts(), kMaxPollTriggeredTxAttempts, otThreadErrorToString(aError));

            if (child->GetIndirectTxAttempts() < kMaxPollTriggeredTxAttempts)
            {
                // We save the frame counter, key id, and data sequence number of
                // current frame so we use the same values for the retransmission
                // of the frame following the receipt of a data request command (data
                // poll) from the sleepy child.

                child->SetIndirectDataSequenceNumber(aFrame.GetSequence());

                if (aFrame.GetSecurityEnabled())
                {
                    uint32_t frameCounter;
                    uint8_t  keyId;

                    aFrame.GetFrameCounter(frameCounter);
                    child->SetIndirectFrameCounter(frameCounter);

                    aFrame.GetKeyId(keyId);
                    child->SetIndirectKeyId(keyId);
                }

                ExitNow();
            }

            child->ResetIndirectTxAttempts();
            child->SetIndirectTxSuccess(false);

#if OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE
            // We set the NextOffset to end of message, since there is no need to
            // send any remaining fragments in the message to the child, if all tx
            // attempts of current frame already failed.

            Get<MeshForwarder>().mMessageNextOffset = Get<MeshForwarder>().mSendMessage->GetLength();
#endif

            break;

        default:
            assert(false);
            break;
        }
    }

    if (Get<MeshForwarder>().mMessageNextOffset < Get<MeshForwarder>().mSendMessage->GetLength())
    {
        if (Get<MeshForwarder>().mSendMessage == child->GetIndirectMessage())
        {
            child->SetIndirectFragmentOffset(Get<MeshForwarder>().mMessageNextOffset);
        }
    }
    else
    {
        otError txError = aError;
        uint8_t childIndex;

        if (Get<MeshForwarder>().mSendMessage == child->GetIndirectMessage())
        {
            child->SetIndirectFragmentOffset(0);
            child->SetIndirectMessage(NULL);
            child->GetLinkInfo().AddMessageTxStatus(child->GetIndirectTxSuccess());

            // Enable short source address matching after the first indirect
            // message transmission attempt to the child. We intentionally do
            // not check for successful tx here to address the scenario where
            // the child does receive "Child ID Response" but parent misses the
            // 15.4 ack from child. If the "Child ID Response" does not make it
            // to the child, then the child will need to send a new "Child ID
            // Request" which will cause the parent to switch to using long
            // address mode for source address matching.

            mSourceMatchController.SetSrcMatchAsShort(*child, true);

#if !OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE

            // When `CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE` is
            // disabled, all fragment frames of a larger message are
            // sent even if the transmission of an earlier fragment fail.
            // Note that `GetIndirectTxSuccess() tracks the tx success of
            // the entire message to the child, while `txError = aError`
            // represents the error status of the last fragment frame
            // transmission.

            if (!child->GetIndirectTxSuccess() && (txError == OT_ERROR_NONE))
            {
                txError = OT_ERROR_FAILED;
            }
#endif
        }

        childIndex = Get<ChildTable>().GetChildIndex(*child);

        if (Get<MeshForwarder>().mSendMessage->GetChildMask(childIndex))
        {
            Get<MeshForwarder>().mSendMessage->ClearChildMask(childIndex);
            mSourceMatchController.DecrementMessageCount(*child);
        }

        if (!Get<MeshForwarder>().mSendMessage->GetDirectTransmission())
        {
            Get<MeshForwarder>().LogMessage(MeshForwarder::kMessageTransmit, *Get<MeshForwarder>().mSendMessage,
                                            &aMacDest, txError);

            if (Get<MeshForwarder>().mSendMessage->GetType() == Message::kTypeIp6)
            {
                if (Get<MeshForwarder>().mSendMessage->GetTxSuccess())
                {
                    Get<MeshForwarder>().mIpCounters.mTxSuccess++;
                }
                else
                {
                    Get<MeshForwarder>().mIpCounters.mTxFailure++;
                }
            }
        }
    }

    if (aError == OT_ERROR_NONE)
    {
        Get<Utils::ChildSupervisor>().UpdateOnSend(*child);
    }

exit:
    return;
}

} // namespace ot

#endif // #if OPENTHREAD_FTD
