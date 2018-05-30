/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file implements FTD-specific mesh forwarding of IPv6/6LoWPAN messages.
 */

#if OPENTHREAD_FTD

#define WPP_NAME "mesh_forwarder_ftd.tmh"

#include "mesh_forwarder.hpp"

#include "common/logging.hpp"
#include "common/owner-locator.hpp"

namespace ot {

otError MeshForwarder::SendMessage(Message &aMessage)
{
    ThreadNetif &netif      = GetNetif();
    ChildTable & childTable = netif.GetMle().GetChildTable();
    otError      error      = OT_ERROR_NONE;
    Neighbor *   neighbor;

    switch (aMessage.GetType())
    {
    case Message::kTypeIp6:
    {
        Ip6::Header ip6Header;

        aMessage.Read(0, sizeof(ip6Header), &ip6Header);

        if (ip6Header.GetDestination().IsMulticast())
        {
            // For traffic destined to multicast address larger than realm local, generally it uses IP-in-IP
            // encapsulation (RFC2473), with outer destination as ALL_MPL_FORWARDERS. So here if the destination
            // is multicast address larger than realm local, it should be for indirection transmission for the
            // device's sleepy child, thus there should be no direct transmission.
            if (!ip6Header.GetDestination().IsMulticastLargerThanRealmLocal())
            {
                // schedule direct transmission
                aMessage.SetDirectTransmission();
            }

            if (aMessage.GetSubType() != Message::kSubTypeMplRetransmission)
            {
                if (ip6Header.GetDestination() == netif.GetMle().GetLinkLocalAllThreadNodesAddress() ||
                    ip6Header.GetDestination() == netif.GetMle().GetRealmLocalAllThreadNodesAddress())
                {
                    // destined for all sleepy children
                    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValidOrRestoring); !iter.IsDone();
                         iter.Advance())
                    {
                        Child &child = *iter.GetChild();

                        if (!child.IsRxOnWhenIdle())
                        {
                            aMessage.SetChildMask(childTable.GetChildIndex(child));
                            mSourceMatchController.IncrementMessageCount(child);
                        }
                    }
                }
                else
                {
                    // destined for some sleepy children which subscribed the multicast address.
                    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValidOrRestoring); !iter.IsDone();
                         iter.Advance())
                    {
                        Child &child = *iter.GetChild();

                        if (netif.GetMle().IsSleepyChildSubscribed(ip6Header.GetDestination(), child))
                        {
                            aMessage.SetChildMask(childTable.GetChildIndex(child));
                            mSourceMatchController.IncrementMessageCount(child);
                        }
                    }
                }
            }
        }
        else if ((neighbor = netif.GetMle().GetNeighbor(ip6Header.GetDestination())) != NULL &&
                 !neighbor->IsRxOnWhenIdle() && !aMessage.GetDirectTransmission())
        {
            // destined for a sleepy child
            Child &child = *static_cast<Child *>(neighbor);
            aMessage.SetChildMask(childTable.GetChildIndex(child));
            mSourceMatchController.IncrementMessageCount(child);
        }
        else
        {
            // schedule direct transmission
            aMessage.SetDirectTransmission();
        }

        break;
    }

    case Message::kTypeSupervision:
    {
        Child *child = netif.GetChildSupervisor().GetDestination(aMessage);
        VerifyOrExit(child != NULL, error = OT_ERROR_DROP);
        VerifyOrExit(!child->IsRxOnWhenIdle(), error = OT_ERROR_DROP);

        aMessage.SetChildMask(childTable.GetChildIndex(*child));
        mSourceMatchController.IncrementMessageCount(*child);
        break;
    }

    default:
        aMessage.SetDirectTransmission();
        break;
    }

    aMessage.SetOffset(0);
    aMessage.SetDatagramTag(0);
    SuccessOrExit(error = mSendQueue.Enqueue(aMessage));
    mScheduleTransmissionTask.Post();

exit:
    return error;
}

void MeshForwarder::HandleResolved(const Ip6::Address &aEid, otError aError)
{
    Message *    cur, *next;
    Ip6::Address ip6Dst;
    bool         enqueuedMessage = false;

    for (cur = mResolvingQueue.GetHead(); cur; cur = next)
    {
        next = cur->GetNext();

        if (cur->GetType() != Message::kTypeIp6)
        {
            continue;
        }

        cur->Read(Ip6::Header::GetDestinationOffset(), sizeof(ip6Dst), &ip6Dst);

        if (ip6Dst == aEid)
        {
            mResolvingQueue.Dequeue(*cur);

            if (aError == OT_ERROR_NONE)
            {
                mSendQueue.Enqueue(*cur);
                enqueuedMessage = true;
            }
            else
            {
                LogIp6Message(kMessageDrop, *cur, NULL, aError);
                cur->Free();
            }
        }
    }

    if (enqueuedMessage)
    {
        mScheduleTransmissionTask.Post();
    }
}

void MeshForwarder::ClearChildIndirectMessages(Child &aChild)
{
    Message *nextMessage;

    VerifyOrExit(aChild.GetIndirectMessageCount() > 0);

    for (Message *message = mSendQueue.GetHead(); message; message = nextMessage)
    {
        nextMessage = message->GetNext();

        message->ClearChildMask(GetNetif().GetMle().GetChildTable().GetChildIndex(aChild));

        if (!message->IsChildPending() && !message->GetDirectTransmission())
        {
            if (mSendMessage == message)
            {
                mSendMessage = NULL;
            }

            mSendQueue.Dequeue(*message);
            message->Free();
        }
    }

    aChild.SetIndirectMessage(NULL);
    mSourceMatchController.ResetMessageCount(aChild);

exit:
    return;
}

void MeshForwarder::UpdateIndirectMessages(void)
{
    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateAnyExceptValidOrRestoing); !iter.IsDone();
         iter.Advance())
    {
        if (iter.GetChild()->GetIndirectMessageCount() == 0)
        {
            continue;
        }

        ClearChildIndirectMessages(*iter.GetChild());
    }
}

otError MeshForwarder::EvictIndirectMessage(void)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (Message *message = mSendQueue.GetHead(); message; message = message->GetNext())
    {
        if (!message->IsChildPending())
        {
            continue;
        }

        RemoveMessage(*message);
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

otError MeshForwarder::RemoveMessageFromSleepyChild(Message &aMessage, Child &aChild)
{
    otError error      = OT_ERROR_NONE;
    uint8_t childIndex = GetNetif().GetMle().GetChildTable().GetChildIndex(aChild);

    VerifyOrExit(aMessage.GetChildMask(childIndex) == true, error = OT_ERROR_NOT_FOUND);

    aMessage.ClearChildMask(childIndex);
    mSourceMatchController.DecrementMessageCount(aChild);

    if (aChild.GetIndirectMessage() == &aMessage)
    {
        aChild.SetIndirectMessage(NULL);
    }

exit:
    return error;
}

void MeshForwarder::RemoveMessages(Child &aChild, uint8_t aSubType)
{
    ThreadNetif &netif = GetNetif();
    Message *    nextMessage;

    for (Message *message = mSendQueue.GetHead(); message; message = nextMessage)
    {
        nextMessage = message->GetNext();

        if ((aSubType != Message::kSubTypeNone) && (aSubType != message->GetSubType()))
        {
            continue;
        }

        if (RemoveMessageFromSleepyChild(*message, aChild) != OT_ERROR_NONE)
        {
            switch (message->GetType())
            {
            case Message::kTypeIp6:
            {
                Ip6::Header ip6header;

                IgnoreReturnValue(message->Read(0, sizeof(ip6header), &ip6header));

                if (&aChild == static_cast<Child *>(netif.GetMle().GetNeighbor(ip6header.GetDestination())))
                {
                    message->ClearDirectTransmission();
                }

                break;
            }

            case Message::kType6lowpan:
            {
                Lowpan::MeshHeader meshHeader;

                IgnoreReturnValue(meshHeader.Init(*message));

                if (&aChild == static_cast<Child *>(netif.GetMle().GetNeighbor(meshHeader.GetDestination())))
                {
                    message->ClearDirectTransmission();
                }

                break;
            }

            default:
                break;
            }
        }

        if (!message->IsChildPending() && !message->GetDirectTransmission())
        {
            if (mSendMessage == message)
            {
                mSendMessage = NULL;
            }

            mSendQueue.Dequeue(*message);
            message->Free();
        }
    }
}

void MeshForwarder::RemoveDataResponseMessages(void)
{
    Ip6::Header ip6Header;

    for (Message *message = mSendQueue.GetHead(); message; message = message->GetNext())
    {
        if (message->GetSubType() != Message::kSubTypeMleDataResponse)
        {
            continue;
        }

        message->Read(0, sizeof(ip6Header), &ip6Header);

        if (!(ip6Header.GetDestination().IsMulticast()))
        {
            for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateAnyExceptInvalid); !iter.IsDone();
                 iter.Advance())
            {
                IgnoreReturnValue(RemoveMessageFromSleepyChild(*message, *iter.GetChild()));
            }
        }

        if (mSendMessage == message)
        {
            mSendMessage = NULL;
        }

        mSendQueue.Dequeue(*message);
        LogIp6Message(kMessageDrop, *message, NULL, OT_ERROR_NONE);
        message->Free();
    }
}

otError MeshForwarder::GetIndirectTransmission(void)
{
    otError      error = OT_ERROR_NOT_FOUND;
    ThreadNetif &netif = GetNetif();

    UpdateIndirectMessages();

    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValidOrRestoring, mIndirectStartingChild);
         !iter.IsDone(); iter.Advance())
    {
        Child &child = *iter.GetChild();

        if (!child.IsDataRequestPending())
        {
            continue;
        }

        mSendMessage                 = child.GetIndirectMessage();
        mSendMessageMaxMacTxAttempts = Mac::kIndirectFrameMacTxAttempts;

        if (mSendMessage == NULL)
        {
            mSendMessage = GetIndirectTransmission(child);
        }

        if (mSendMessage != NULL)
        {
            PrepareIndirectTransmission(*mSendMessage, child);
        }
        else
        {
            // A NULL `mSendMessage` triggers an empty frame to be sent to the child.

            if (child.IsIndirectSourceMatchShort())
            {
                mMacSource.SetShort(netif.GetMac().GetShortAddress());
            }
            else
            {
                mMacSource.SetExtended(netif.GetMac().GetExtAddress());
            }

            child.GetMacAddress(mMacDest);
        }

        // Remember the current child and move it to next one in the list after the indirect transmission has completed.

        mIndirectStartingChild = &child;

        netif.GetMac().SendFrameRequest(mMacSender);
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

Message *MeshForwarder::GetIndirectTransmission(Child &aChild)
{
    Message *message = NULL;
    Message *next;
    uint8_t  childIndex = GetNetif().GetMle().GetChildTable().GetChildIndex(aChild);

    for (message = mSendQueue.GetHead(); message; message = next)
    {
        next = message->GetNext();

        if (message->GetChildMask(childIndex))
        {
            // Skip and remove the supervision message if there are other messages queued for the child.

            if ((message->GetType() == Message::kTypeSupervision) && (aChild.GetIndirectMessageCount() > 1))
            {
                message->ClearChildMask(childIndex);
                mSourceMatchController.DecrementMessageCount(aChild);
                mSendQueue.Dequeue(*message);
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

        LogIp6Message(kMessagePrepareIndirect, *message, &aChild.GetMacAddress(macAddr), OT_ERROR_NONE);
    }

    return message;
}

void MeshForwarder::PrepareIndirectTransmission(Message &aMessage, const Child &aChild)
{
    if (aChild.GetIndirectTxAttempts() > 0)
    {
        mSendMessageIsARetransmission  = true;
        mSendMessageFrameCounter       = aChild.GetIndirectFrameCounter();
        mSendMessageKeyId              = aChild.GetIndirectKeyId();
        mSendMessageDataSequenceNumber = aChild.GetIndirectDataSequenceNumber();
    }

    aMessage.SetOffset(aChild.GetIndirectFragmentOffset());

    switch (aMessage.GetType())
    {
    case Message::kTypeIp6:
    {
        Ip6::Header ip6Header;

        aMessage.Read(0, sizeof(ip6Header), &ip6Header);

        mAddMeshHeader = false;
        GetMacSourceAddress(ip6Header.GetSource(), mMacSource);

        if (ip6Header.GetDestination().IsLinkLocal())
        {
            GetMacDestinationAddress(ip6Header.GetDestination(), mMacDest);
        }
        else
        {
            aChild.GetMacAddress(mMacDest);
        }

        break;
    }

    case Message::kTypeSupervision:
        aChild.GetMacAddress(mMacDest);
        break;

    default:
        assert(false);
        break;
    }
}

otError MeshForwarder::SendMesh(Message &aMessage, Mac::Frame &aFrame)
{
    ThreadNetif &netif = GetNetif();
    uint16_t     fcf;

    // initialize MAC header
    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfFrameVersion2006 |
          Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort | Mac::Frame::kFcfAckRequest |
          Mac::Frame::kFcfSecurityEnabled;

    aFrame.InitMacHeader(fcf, Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32);
    aFrame.SetDstPanId(netif.GetMac().GetPanId());
    aFrame.SetDstAddr(mMacDest.GetShort());
    aFrame.SetSrcAddr(mMacSource.GetShort());

    // write payload
    assert(aMessage.GetLength() <= aFrame.GetMaxPayloadLength());
    aMessage.Read(0, aMessage.GetLength(), aFrame.GetPayload());
    aFrame.SetPayloadLength(static_cast<uint8_t>(aMessage.GetLength()));

    mMessageNextOffset = aMessage.GetLength();

    return OT_ERROR_NONE;
}

void MeshForwarder::HandleDataRequest(const Mac::Address &aMacSource, const otThreadLinkInfo &aLinkInfo)
{
    ThreadNetif &netif = GetNetif();
    Child *      child;
    uint16_t     indirectMsgCount;

    // Security Check: only process secure Data Poll frames.
    VerifyOrExit(aLinkInfo.mLinkSecurity);

    VerifyOrExit(netif.GetMle().GetRole() != OT_DEVICE_ROLE_DETACHED);

    child = netif.GetMle().GetChildTable().FindChild(aMacSource, ChildTable::kInStateValidOrRestoring);
    VerifyOrExit(child != NULL);

    child->SetLastHeard(TimerMilli::GetNow());
    child->ResetLinkFailures();
    indirectMsgCount = child->GetIndirectMessageCount();

    if (!mSourceMatchController.IsEnabled() || (indirectMsgCount > 0))
    {
        child->SetDataRequestPending(true);
    }

    mScheduleTransmissionTask.Post();

    otLogInfoMac(GetInstance(), "Rx data poll, src:0x%04x, qed_msgs:%d, rss:%d", child->GetRloc16(), indirectMsgCount,
                 aLinkInfo.mRss);

exit:
    return;
}

void MeshForwarder::HandleSentFrameToChild(const Mac::Frame &aFrame, otError aError, const Mac::Address &aMacDest)
{
    ThreadNetif &netif = GetNetif();
    Child *      child;

    child = netif.GetMle().GetChildTable().FindChild(aMacDest, ChildTable::kInStateValidOrRestoring);
    VerifyOrExit(child != NULL);

    child->SetDataRequestPending(false);

    VerifyOrExit(mSendMessage != NULL);

    if (mSendMessage == child->GetIndirectMessage())
    {
        // To ensure fairness in handling of data requests from sleepy
        // children, once a message is completed for indirect transmission to a
        // child (on both success or failure), the `mIndirectStartingChild` is
        // updated to the next `Child` entry after the current one. Subsequent
        // call to `ScheduleTransmissionTask()` will begin the iteration
        // through the children list from this child.

        ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValidOrRestoring, mIndirectStartingChild);
        iter.Advance();
        mIndirectStartingChild = iter.GetChild();

        if (aError == OT_ERROR_NONE)
        {
            child->ResetIndirectTxAttempts();
        }
        else
        {
            child->IncrementIndirectTxAttempts();

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

            mMessageNextOffset = mSendMessage->GetLength();
#endif
        }
    }

    if (mMessageNextOffset < mSendMessage->GetLength())
    {
        if (mSendMessage == child->GetIndirectMessage())
        {
            child->SetIndirectFragmentOffset(mMessageNextOffset);
        }
    }
    else
    {
        uint8_t childIndex;

        if (mSendMessage == child->GetIndirectMessage())
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
        }

        childIndex = netif.GetMle().GetChildTable().GetChildIndex(*child);

        if (mSendMessage->GetChildMask(childIndex))
        {
            mSendMessage->ClearChildMask(childIndex);
            mSourceMatchController.DecrementMessageCount(*child);
        }
    }

    if (aError == OT_ERROR_NONE)
    {
        netif.GetChildSupervisor().UpdateOnSend(*child);
    }

exit:
    return;
}

otError MeshForwarder::UpdateMeshRoute(Message &aMessage)
{
    ThreadNetif &      netif = GetNetif();
    otError            error = OT_ERROR_NONE;
    Lowpan::MeshHeader meshHeader;
    Neighbor *         neighbor;
    uint16_t           nextHop;

    IgnoreReturnValue(meshHeader.Init(aMessage));

    nextHop = netif.GetMle().GetNextHop(meshHeader.GetDestination());

    if (nextHop != Mac::kShortAddrInvalid)
    {
        neighbor = netif.GetMle().GetNeighbor(nextHop);
    }
    else
    {
        neighbor = netif.GetMle().GetNeighbor(meshHeader.GetDestination());
    }

    if (neighbor == NULL)
    {
        ExitNow(error = OT_ERROR_DROP);
    }

    mMacDest.SetShort(neighbor->GetRloc16());
    mMacSource.SetShort(netif.GetMac().GetShortAddress());

    mAddMeshHeader = true;
    mMeshDest      = meshHeader.GetDestination();
    mMeshSource    = meshHeader.GetSource();

exit:
    return error;
}

otError MeshForwarder::UpdateIp6RouteFtd(Ip6::Header &ip6Header)
{
    otError      error = OT_ERROR_NONE;
    ThreadNetif &netif = GetNetif();
    Neighbor *   neighbor;

    if (netif.GetMle().IsRoutingLocator(ip6Header.GetDestination()))
    {
        uint16_t rloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);
        VerifyOrExit(netif.GetMle().IsRouterIdValid(netif.GetMle().GetRouterId(rloc16)), error = OT_ERROR_DROP);
        mMeshDest = rloc16;
    }
    else if (netif.GetMle().IsAnycastLocator(ip6Header.GetDestination()))
    {
        uint16_t aloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);

        if (aloc16 == Mle::kAloc16Leader)
        {
            mMeshDest = netif.GetMle().GetRloc16(netif.GetMle().GetLeaderId());
        }

#if OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
        else if (aloc16 <= Mle::kAloc16DhcpAgentEnd)
        {
            uint16_t agentRloc16;
            uint8_t  routerId;
            VerifyOrExit((netif.GetNetworkDataLeader().GetRlocByContextId(
                              static_cast<uint8_t>(aloc16 & Mle::kAloc16DhcpAgentMask), agentRloc16) == OT_ERROR_NONE),
                         error = OT_ERROR_DROP);

            routerId = netif.GetMle().GetRouterId(agentRloc16);

            // if agent is active router or the child of the device
            if ((netif.GetMle().IsActiveRouter(agentRloc16)) ||
                (netif.GetMle().GetRloc16(routerId) == netif.GetMle().GetRloc16()))
            {
                mMeshDest = agentRloc16;
            }
            else
            {
                // use the parent of the ED Agent as Dest
                mMeshDest = netif.GetMle().GetRloc16(routerId);
            }
        }

#endif // OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
#if OPENTHREAD_ENABLE_SERVICE
        else if ((aloc16 >= Mle::kAloc16ServiceStart) && (aloc16 <= Mle::kAloc16ServiceEnd))
        {
            SuccessOrExit(error = GetDestinationRlocByServiceAloc(aloc16, mMeshDest));
        }

#endif
        else
        {
            // TODO: support ALOC for Commissioner, Neighbor Discovery Agent
            ExitNow(error = OT_ERROR_DROP);
        }
    }
    else if ((neighbor = netif.GetMle().GetNeighbor(ip6Header.GetDestination())) != NULL)
    {
        mMeshDest = neighbor->GetRloc16();
    }
    else if (netif.GetNetworkDataLeader().IsOnMesh(ip6Header.GetDestination()))
    {
        SuccessOrExit(error = netif.GetAddressResolver().Resolve(ip6Header.GetDestination(), mMeshDest));
    }
    else
    {
        netif.GetNetworkDataLeader().RouteLookup(ip6Header.GetSource(), ip6Header.GetDestination(), NULL, &mMeshDest);
    }

    VerifyOrExit(mMeshDest != Mac::kShortAddrInvalid, error = OT_ERROR_DROP);

    mMeshSource = netif.GetMac().GetShortAddress();

    SuccessOrExit(error = netif.GetMle().CheckReachability(mMeshSource, mMeshDest, ip6Header));
    mMacDest.SetShort(netif.GetMle().GetNextHop(mMeshDest));

    if (mMacDest.GetShort() != mMeshDest)
    {
        // destination is not neighbor
        mMacSource.SetShort(mMeshSource);
        mAddMeshHeader = true;
    }

exit:
    return error;
}

otError MeshForwarder::CheckReachability(uint8_t *           aFrame,
                                         uint8_t             aFrameLength,
                                         const Mac::Address &aMeshSource,
                                         const Mac::Address &aMeshDest)
{
    ThreadNetif &      netif = GetNetif();
    otError            error = OT_ERROR_NONE;
    Ip6::Header        ip6Header;
    Lowpan::MeshHeader meshHeader;

    VerifyOrExit(meshHeader.Init(aFrame, aFrameLength) == OT_ERROR_NONE, error = OT_ERROR_DROP);

    // skip mesh header
    aFrame += meshHeader.GetHeaderLength();
    aFrameLength -= meshHeader.GetHeaderLength();

    // skip fragment header
    if (aFrameLength >= 1 && reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
    {
        VerifyOrExit(sizeof(Lowpan::FragmentHeader) <= aFrameLength, error = OT_ERROR_DROP);
        VerifyOrExit(reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetDatagramOffset() == 0);

        aFrame += reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
        aFrameLength -= reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
    }

    // only process IPv6 packets
    VerifyOrExit(aFrameLength >= 1 && Lowpan::Lowpan::IsLowpanHc(aFrame));

    VerifyOrExit(netif.GetLowpan().DecompressBaseHeader(ip6Header, aMeshSource, aMeshDest, aFrame, aFrameLength) > 0,
                 error = OT_ERROR_DROP);

    error = netif.GetMle().CheckReachability(aMeshSource.GetShort(), aMeshDest.GetShort(), ip6Header);

exit:
    return error;
}

void MeshForwarder::HandleMesh(uint8_t *               aFrame,
                               uint8_t                 aFrameLength,
                               const Mac::Address &    aMacSource,
                               const otThreadLinkInfo &aLinkInfo)
{
    ThreadNetif &      netif   = GetNetif();
    otError            error   = OT_ERROR_NONE;
    Message *          message = NULL;
    Mac::Address       meshDest;
    Mac::Address       meshSource;
    Lowpan::MeshHeader meshHeader;

    // Check the mesh header
    VerifyOrExit(meshHeader.Init(aFrame, aFrameLength) == OT_ERROR_NONE, error = OT_ERROR_DROP);

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aLinkInfo.mLinkSecurity && meshHeader.IsValid(), error = OT_ERROR_SECURITY);

    meshSource.SetShort(meshHeader.GetSource());
    meshDest.SetShort(meshHeader.GetDestination());

    UpdateRoutes(aFrame, aFrameLength, meshSource, meshDest);

    if (meshDest.GetShort() == netif.GetMac().GetShortAddress() || netif.GetMle().IsMinimalChild(meshDest.GetShort()))
    {
        aFrame += meshHeader.GetHeaderLength();
        aFrameLength -= meshHeader.GetHeaderLength();

        if (reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
        {
            HandleFragment(aFrame, aFrameLength, meshSource, meshDest, aLinkInfo);
        }
        else if (Lowpan::Lowpan::IsLowpanHc(aFrame))
        {
            HandleLowpanHC(aFrame, aFrameLength, meshSource, meshDest, aLinkInfo);
        }
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }
    }
    else if (meshHeader.GetHopsLeft() > 0)
    {
        netif.GetMle().ResolveRoutingLoops(aMacSource.GetShort(), meshDest.GetShort());

        SuccessOrExit(error = CheckReachability(aFrame, aFrameLength, meshSource, meshDest));

        meshHeader.SetHopsLeft(meshHeader.GetHopsLeft() - 1);
        meshHeader.AppendTo(aFrame);

        VerifyOrExit((message = GetInstance().GetMessagePool().New(Message::kType6lowpan, 0)) != NULL,
                     error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = message->SetLength(aFrameLength));
        message->Write(0, aFrameLength, aFrame);
        message->SetLinkSecurityEnabled(aLinkInfo.mLinkSecurity);
        message->SetPanId(aLinkInfo.mPanId);

        SendMessage(*message);
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        char srcStringBuffer[Mac::Address::kAddressStringSize];

        otLogInfoMac(GetInstance(), "Dropping rx mesh frame, error:%s, len:%d, src:%s, sec:%s",
                     otThreadErrorToString(error), aFrameLength,
                     aMacSource.ToString(srcStringBuffer, sizeof(srcStringBuffer)),
                     aLinkInfo.mLinkSecurity ? "yes" : "no");

        OT_UNUSED_VARIABLE(srcStringBuffer);

        if (message != NULL)
        {
            message->Free();
        }
    }
}

void MeshForwarder::UpdateRoutes(uint8_t *           aFrame,
                                 uint8_t             aFrameLength,
                                 const Mac::Address &aMeshSource,
                                 const Mac::Address &aMeshDest)
{
    ThreadNetif &netif = GetNetif();
    Ip6::Header  ip6Header;
    Neighbor *   neighbor;

    VerifyOrExit(!aMeshDest.IsBroadcast() && aMeshSource.IsShort());

    // skip mesh header
    if (aFrameLength >= 1 && reinterpret_cast<Lowpan::MeshHeader *>(aFrame)->IsMeshHeader())
    {
        aFrame += reinterpret_cast<Lowpan::MeshHeader *>(aFrame)->GetHeaderLength();
        aFrameLength -= reinterpret_cast<Lowpan::MeshHeader *>(aFrame)->GetHeaderLength();
    }

    // skip fragment header
    if (aFrameLength >= 1 && reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
    {
        VerifyOrExit(sizeof(Lowpan::FragmentHeader) <= aFrameLength);
        VerifyOrExit(reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetDatagramOffset() == 0);

        aFrame += reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
        aFrameLength -= reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
    }

    // only process IPv6 packets
    VerifyOrExit(aFrameLength >= 1 && Lowpan::Lowpan::IsLowpanHc(aFrame));

    VerifyOrExit(netif.GetLowpan().DecompressBaseHeader(ip6Header, aMeshSource, aMeshDest, aFrame, aFrameLength) > 0);

    netif.GetAddressResolver().UpdateCacheEntry(ip6Header.GetSource(), aMeshSource.GetShort());

    neighbor = netif.GetMle().GetNeighbor(ip6Header.GetSource());
    VerifyOrExit(neighbor != NULL && !neighbor->IsFullThreadDevice());

    if (Mle::Mle::GetRouterId(aMeshSource.GetShort()) != Mle::Mle::GetRouterId(GetNetif().GetMac().GetShortAddress()))
    {
        netif.GetMle().RemoveNeighbor(*neighbor);
    }

exit:
    return;
}

#if OPENTHREAD_ENABLE_SERVICE
otError MeshForwarder::GetDestinationRlocByServiceAloc(uint16_t aServiceAloc, uint16_t &aMeshDest)
{
    otError                  error      = OT_ERROR_NONE;
    ThreadNetif &            netif      = GetNetif();
    uint8_t                  serviceId  = netif.GetMle().GetServiceIdFromAloc(aServiceAloc);
    NetworkData::ServiceTlv *serviceTlv = netif.GetNetworkDataLeader().FindServiceById(serviceId);

    if (serviceTlv != NULL)
    {
        NetworkData::NetworkDataTlv *cur = serviceTlv->GetSubTlvs();
        NetworkData::NetworkDataTlv *end = serviceTlv->GetNext();
        NetworkData::ServerTlv *     server;
        uint8_t                      bestCost = Mle::kMaxRouteCost;
        uint8_t                      curCost  = 0x00;
        uint16_t                     bestDest = Mac::kShortAddrInvalid;

        while (cur < end)
        {
            switch (cur->GetType())
            {
            case NetworkData::NetworkDataTlv::kTypeServer:
                server  = static_cast<NetworkData::ServerTlv *>(cur);
                curCost = netif.GetMle().GetCost(server->GetServer16());

                if ((bestDest == Mac::kShortAddrInvalid) || (curCost < bestCost))
                {
                    bestDest = server->GetServer16();
                    bestCost = curCost;
                }

                break;

            default:
                break;
            }

            cur = cur->GetNext();
        }

        if (bestDest != Mac::kShortAddrInvalid)
        {
            aMeshDest = bestDest;
        }
        else
        {
            // ServiceTLV without ServerTLV? Can't forward packet anywhere.
            ExitNow(error = OT_ERROR_DROP);
        }
    }
    else
    {
        // Unknown service, can't forward
        ExitNow(error = OT_ERROR_DROP);
    }

exit:
    return error;
}
#endif // OPENTHREAD_ENABLE_SERVICE

} // namespace ot

#endif // OPENTHREAD_FTD
