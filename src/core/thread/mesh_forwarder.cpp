/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file implements mesh forwarding of IPv6/6LoWPAN messages.
 */

#define WPP_NAME "mesh_forwarder.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "mesh_forwarder.hpp"

#include <openthread/platform/random.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "net/ip6.hpp"
#include "net/ip6_filter.hpp"
#include "net/netif.hpp"
#include "net/tcp.hpp"
#include "net/udp6.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {

MeshForwarder::MeshForwarder(ThreadNetif &aThreadNetif):
    mNetif(aThreadNetif),
    mMacReceiver(&MeshForwarder::HandleReceivedFrame, &MeshForwarder::HandleDataPollTimeout, this),
    mMacSender(&MeshForwarder::HandleFrameRequest, &MeshForwarder::HandleSentFrame, this),
    mDiscoverTimer(aThreadNetif.GetIp6().mTimerScheduler, &MeshForwarder::HandleDiscoverTimer, this),
    mReassemblyTimer(aThreadNetif.GetIp6().mTimerScheduler, &MeshForwarder::HandleReassemblyTimer, this),
    mMessageNextOffset(0),
    mSendMessageFrameCounter(0),
    mSendMessage(NULL),
    mSendMessageIsARetransmission(false),
    mSendMessageMaxMacTxAttempts(Mac::kDirectFrameMacTxAttempts),
    mSendMessageKeyId(0),
    mSendMessageDataSequenceNumber(0),
    mStartChildIndex(0),
    mMeshSource(Mac::kShortAddrInvalid),
    mMeshDest(Mac::kShortAddrInvalid),
    mAddMeshHeader(false),
    mSendBusy(false),
    mScheduleTransmissionTask(aThreadNetif.GetIp6().mTaskletScheduler, ScheduleTransmissionTask, this),
    mEnabled(false),
    mScanChannels(0),
    mScanChannel(0),
    mRestoreChannel(0),
    mRestorePanId(Mac::kPanIdBroadcast),
    mScanning(false),
    mDataPollManager(*this),
    mSourceMatchController(*this)
{
    mFragTag = static_cast<uint16_t>(otPlatRandomGet());
    mNetif.GetMac().RegisterReceiver(mMacReceiver);
    mMacSource.mLength = 0;
    mMacDest.mLength = 0;

    mIpCounters.mTxSuccess = 0;
    mIpCounters.mRxSuccess = 0;
    mIpCounters.mTxFailure = 0;
    mIpCounters.mRxFailure = 0;
}

otInstance *MeshForwarder::GetInstance(void)
{
    return mNetif.GetInstance();
}

otError MeshForwarder::Start(void)
{
    otError error = OT_ERROR_NONE;

    if (mEnabled == false)
    {
        mNetif.GetMac().SetRxOnWhenIdle(true);
        mEnabled = true;
    }

    return error;
}

otError MeshForwarder::Stop(void)
{
    otError error = OT_ERROR_NONE;
    Message *message;

    VerifyOrExit(mEnabled == true);

    mDataPollManager.StopPolling();
    mReassemblyTimer.Stop();

    if (mScanning)
    {
        mNetif.GetMac().SetChannel(mRestoreChannel);
        mScanning = false;
        mNetif.GetMle().HandleDiscoverComplete();
    }

    while ((message = mSendQueue.GetHead()) != NULL)
    {
        mSendQueue.Dequeue(*message);
        message->Free();
    }

    while ((message = mReassemblyList.GetHead()) != NULL)
    {
        mReassemblyList.Dequeue(*message);
        message->Free();
    }

    mEnabled = false;
    mSendMessage = NULL;
    mNetif.GetMac().SetRxOnWhenIdle(false);

exit:
    return error;
}

void MeshForwarder::HandleResolved(const Ip6::Address &aEid, otError aError)
{
    Message *cur, *next;
    Ip6::Address ip6Dst;
    bool enqueuedMessage = false;

    for (cur = mResolvingQueue.GetHead(); cur; cur = next)
    {
        next = cur->GetNext();

        if (cur->GetType() != Message::kTypeIp6)
        {
            continue;
        }

        cur->Read(Ip6::Header::GetDestinationOffset(), sizeof(ip6Dst), &ip6Dst);

        if (memcmp(&ip6Dst, &aEid, sizeof(ip6Dst)) == 0)
        {
            mResolvingQueue.Dequeue(*cur);

            if (aError == OT_ERROR_NONE)
            {
                mSendQueue.Enqueue(*cur);
                enqueuedMessage = true;
            }
            else
            {
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

        message->ClearChildMask(mNetif.GetMle().GetChildIndex(aChild));

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

    mSourceMatchController.ResetMessageCount(aChild);

exit:
    return;
}

void MeshForwarder::UpdateIndirectMessages(void)
{
    Child *children;
    uint8_t numChildren;

    children = mNetif.GetMle().GetChildren(&numChildren);

    for (uint8_t i = 0; i < numChildren; i++)
    {
        Child *child = &children[i];

        if (child->IsStateValidOrRestoring() || (child->GetIndirectMessageCount() == 0))
        {
            continue;
        }

        ClearChildIndirectMessages(*child);
    }
}

void MeshForwarder::ScheduleTransmissionTask(void *aContext)
{
    static_cast<MeshForwarder *>(aContext)->ScheduleTransmissionTask();
}

void MeshForwarder::ScheduleTransmissionTask(void)
{
    otError error = OT_ERROR_NONE;
    uint8_t numChildren;
    uint8_t childIndex;
    uint8_t nextIndex;
    Child *children;

    VerifyOrExit(mSendBusy == false, error = OT_ERROR_BUSY);

    UpdateIndirectMessages();

    mSendMessageIsARetransmission = false;

    children = mNetif.GetMle().GetChildren(&numChildren);

    if (mStartChildIndex >= numChildren)
    {
        mStartChildIndex = 0;
    }

    childIndex = mStartChildIndex;

    for (uint8_t iterations = numChildren; iterations > 0; iterations--, childIndex = nextIndex)
    {
        Child &child = children[childIndex];

        if ((nextIndex = childIndex + 1) == numChildren)
        {
            nextIndex = 0;
        }

        if (!child.IsStateValidOrRestoring() || !child.IsDataRequestPending())
        {
            continue;
        }

        mSendMessage = child.GetIndirectMessage();
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
                mMacSource.mLength = sizeof(mMacSource.mShortAddress);
                mMacSource.mShortAddress = mNetif.GetMac().GetShortAddress();
            }
            else
            {
                mMacSource.mLength = sizeof(mMacSource.mExtAddress);
                memcpy(mMacSource.mExtAddress.m8, mNetif.GetMac().GetExtAddress(), sizeof(mMacDest.mExtAddress));
            }

            child.GetMacAddress(mMacDest);
        }

        // To ensure fairness in handling of data requests from sleepy
        // children, once a message is scheduled and prepared for indirect
        // transmission to a child, the `mStartChildIndex` is updated to
        // the next index after the current child. Subsequent call to
        // `ScheduleTransmissionTask()` will begin the iteration through
        // the children list from this index.

        mStartChildIndex = nextIndex;

        mNetif.GetMac().SendFrameRequest(mMacSender);
        ExitNow();
    }

    if ((mSendMessage = GetDirectTransmission()) != NULL)
    {
        mNetif.GetMac().SendFrameRequest(mMacSender);
        mSendMessageMaxMacTxAttempts = Mac::kDirectFrameMacTxAttempts;
        ExitNow();
    }

exit:
    (void) error;
}

otError MeshForwarder::SendMessage(Message &aMessage)
{
    otError error = OT_ERROR_NONE;
    Neighbor *neighbor;

    uint8_t numChildren;
    Child *child;

    switch (aMessage.GetType())
    {
    case Message::kTypeIp6:
    {
        Ip6::Header ip6Header;

        aMessage.Read(0, sizeof(ip6Header), &ip6Header);

        if (!memcmp(&ip6Header.GetDestination(), mNetif.GetMle().GetLinkLocalAllThreadNodesAddress(),
                    sizeof(ip6Header.GetDestination())) ||
            !memcmp(&ip6Header.GetDestination(), mNetif.GetMle().GetRealmLocalAllThreadNodesAddress(),
                    sizeof(ip6Header.GetDestination())))
        {
            // schedule direct transmission
            aMessage.SetDirectTransmission();

            if (aMessage.GetSubType() != Message::kSubTypeMplRetransmission)
            {
                // destined for all sleepy children
                child = mNetif.GetMle().GetChildren(&numChildren);

                for (uint8_t i = 0; i < numChildren; i++, child++)
                {
                    if (child->IsStateValidOrRestoring() && !child->IsRxOnWhenIdle())
                    {
                        aMessage.SetChildMask(i);
                        mSourceMatchController.IncrementMessageCount(*child);
                    }
                }
            }
        }
        else if ((neighbor = mNetif.GetMle().GetNeighbor(ip6Header.GetDestination())) != NULL &&
                 !neighbor->IsRxOnWhenIdle() &&
                 !aMessage.GetDirectTransmission())
        {
            // destined for a sleepy child
            child = static_cast<Child *>(neighbor);
            aMessage.SetChildMask(mNetif.GetMle().GetChildIndex(*child));
            mSourceMatchController.IncrementMessageCount(*child);
        }
        else
        {
            // schedule direct transmission
            aMessage.SetDirectTransmission();
        }

        break;
    }

    case Message::kType6lowpan:
    {
        Lowpan::MeshHeader meshHeader;

        IgnoreReturnValue(meshHeader.Init(aMessage));

        if ((neighbor = mNetif.GetMle().GetNeighbor(meshHeader.GetDestination())) != NULL &&
            !neighbor->IsRxOnWhenIdle())
        {
            // destined for a sleepy child
            child = static_cast<Child *>(neighbor);
            aMessage.SetChildMask(mNetif.GetMle().GetChildIndex(*child));
            mSourceMatchController.IncrementMessageCount(*child);
        }
        else
        {
            // not destined for a sleepy child
            aMessage.SetDirectTransmission();
        }

        break;
    }

    case Message::kTypeMacDataPoll:
        aMessage.SetDirectTransmission();
        break;

    case Message::kTypeSupervision:
        child = mNetif.GetChildSupervisor().GetDestination(aMessage);
        VerifyOrExit(child != NULL, error = OT_ERROR_DROP);
        VerifyOrExit(!child->IsRxOnWhenIdle(), error = OT_ERROR_DROP);

        aMessage.SetChildMask(mNetif.GetMle().GetChildIndex(*child));
        mSourceMatchController.IncrementMessageCount(*child);
        break;
    }

    aMessage.SetOffset(0);
    aMessage.SetDatagramTag(0);
    SuccessOrExit(error = mSendQueue.Enqueue(aMessage));
    mScheduleTransmissionTask.Post();

exit:
    return error;
}

otError MeshForwarder::PrepareDiscoverRequest(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mScanning);

    mScanChannel = OT_RADIO_CHANNEL_MIN;
    mScanChannels >>= OT_RADIO_CHANNEL_MIN;
    mRestoreChannel = mNetif.GetMac().GetChannel();
    mRestorePanId = mNetif.GetMac().GetPanId();

    while ((mScanChannels & 1) == 0)
    {
        mScanChannels >>= 1;
        mScanChannel++;

        if (mScanChannel > OT_RADIO_CHANNEL_MAX)
        {
            mNetif.GetMle().HandleDiscoverComplete();
            ExitNow(error = OT_ERROR_DROP);
        }
    }

    mScanning = true;

exit:
    return error;
}

Message *MeshForwarder::GetDirectTransmission(void)
{
    Message *curMessage, *nextMessage;
    otError error = OT_ERROR_NONE;

    for (curMessage = mSendQueue.GetHead(); curMessage; curMessage = nextMessage)
    {
        nextMessage = curMessage->GetNext();

        if (curMessage->GetDirectTransmission() == false)
        {
            continue;
        }

        switch (curMessage->GetType())
        {
        case Message::kTypeIp6:
            error = UpdateIp6Route(*curMessage);

            if (curMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
            {
                error = PrepareDiscoverRequest();
            }

            break;

        case Message::kType6lowpan:
            error = UpdateMeshRoute(*curMessage);
            break;

        case Message::kTypeMacDataPoll:
            ExitNow();

        case Message::kTypeSupervision:
            error = OT_ERROR_DROP;
            break;
        }

        switch (error)
        {
        case OT_ERROR_NONE:
            ExitNow();

        case OT_ERROR_ADDRESS_QUERY:
            mSendQueue.Dequeue(*curMessage);
            mResolvingQueue.Enqueue(*curMessage);
            continue;

        case OT_ERROR_DROP:
        case OT_ERROR_NO_BUFS:
            mSendQueue.Dequeue(*curMessage);
            curMessage->Free();
            continue;

        default:
            assert(false);
            break;
        }
    }

exit:
    return curMessage;
}

Message *MeshForwarder::GetIndirectTransmission(Child &aChild)
{
    Message *message = NULL;
    Message *next;
    uint8_t childIndex = mNetif.GetMle().GetChildIndex(aChild);

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
        mSendMessageIsARetransmission = true;
        mSendMessageFrameCounter = aChild.GetIndirectFrameCounter();
        mSendMessageKeyId = aChild.GetIndirectKeyId();
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

    case Message::kType6lowpan:
    {
        Lowpan::MeshHeader meshHeader;

        IgnoreReturnValue(meshHeader.Init(aMessage));
        mAddMeshHeader = true;
        mMeshDest = meshHeader.GetDestination();
        mMeshSource = meshHeader.GetSource();
        mMacSource.mLength = sizeof(mMacSource.mShortAddress);
        mMacSource.mShortAddress = mNetif.GetMac().GetShortAddress();
        mMacDest.mLength = sizeof(mMacDest.mShortAddress);
        mMacDest.mShortAddress = meshHeader.GetDestination();
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

otError MeshForwarder::UpdateMeshRoute(Message &aMessage)
{
    otError error = OT_ERROR_NONE;
    Lowpan::MeshHeader meshHeader;
    Neighbor *neighbor;
    uint16_t nextHop;

    IgnoreReturnValue(meshHeader.Init(aMessage));

    nextHop = mNetif.GetMle().GetNextHop(meshHeader.GetDestination());

    if (nextHop != Mac::kShortAddrInvalid)
    {
        neighbor = mNetif.GetMle().GetNeighbor(nextHop);
    }
    else
    {
        neighbor = mNetif.GetMle().GetNeighbor(meshHeader.GetDestination());
    }

    if (neighbor == NULL)
    {
        ExitNow(error = OT_ERROR_DROP);
    }

    mMacDest.mLength = sizeof(mMacDest.mShortAddress);
    mMacDest.mShortAddress = neighbor->GetRloc16();
    mMacSource.mLength = sizeof(mMacSource.mShortAddress);
    mMacSource.mShortAddress = mNetif.GetMac().GetShortAddress();

    mAddMeshHeader = true;
    mMeshDest = meshHeader.GetDestination();
    mMeshSource = meshHeader.GetSource();

exit:
    return error;
}

otError MeshForwarder::UpdateIp6Route(Message &aMessage)
{
    otError error = OT_ERROR_NONE;
    Ip6::Header ip6Header;

    mAddMeshHeader = false;

    aMessage.Read(0, sizeof(ip6Header), &ip6Header);

    switch (mNetif.GetMle().GetRole())
    {
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
        if (ip6Header.GetDestination().IsLinkLocal() || ip6Header.GetDestination().IsLinkLocalMulticast())
        {
            GetMacDestinationAddress(ip6Header.GetDestination(), mMacDest);
            GetMacSourceAddress(ip6Header.GetSource(), mMacSource);
        }
        else
        {
            ExitNow(error = OT_ERROR_DROP);
        }

        break;

    case OT_DEVICE_ROLE_CHILD:
        if (aMessage.IsLinkSecurityEnabled())
        {
            mMacDest.mLength = sizeof(mMacDest.mShortAddress);

            if (ip6Header.GetDestination().IsLinkLocalMulticast())
            {
                mMacDest.mShortAddress = Mac::kShortAddrBroadcast;
            }
            else
            {
                mMacDest.mShortAddress = mNetif.GetMle().GetNextHop(Mac::kShortAddrBroadcast);
            }

            GetMacSourceAddress(ip6Header.GetSource(), mMacSource);
        }
        else if (ip6Header.GetDestination().IsLinkLocal() || ip6Header.GetDestination().IsLinkLocalMulticast())
        {
            GetMacDestinationAddress(ip6Header.GetDestination(), mMacDest);
            GetMacSourceAddress(ip6Header.GetSource(), mMacSource);
        }
        else
        {
            ExitNow(error = OT_ERROR_DROP);
        }

        break;

#if OPENTHREAD_FTD
        {
        case OT_DEVICE_ROLE_ROUTER:
        case OT_DEVICE_ROLE_LEADER:
            uint16_t rloc16;
            uint16_t aloc16;
            Neighbor *neighbor;

            if (ip6Header.GetDestination().IsLinkLocal() || ip6Header.GetDestination().IsMulticast())
            {
                GetMacDestinationAddress(ip6Header.GetDestination(), mMacDest);
                GetMacSourceAddress(ip6Header.GetSource(), mMacSource);
            }
            else
            {
                if (mNetif.GetMle().IsRoutingLocator(ip6Header.GetDestination()))
                {
                    rloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);
                    VerifyOrExit(mNetif.GetMle().IsRouterIdValid(mNetif.GetMle().GetRouterId(rloc16)),
                                 error = OT_ERROR_DROP);
                    mMeshDest = rloc16;
                }
                else if (mNetif.GetMle().IsAnycastLocator(ip6Header.GetDestination()))
                {
                    aloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);

                    if (aloc16 == Mle::kAloc16Leader)
                    {
                        mMeshDest = mNetif.GetMle().GetRloc16(mNetif.GetMle().GetLeaderId());
                    }

#if OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
                    else if ((aloc16 & Mle::kAloc16DhcpAgentMask) != 0)
                    {
                        uint16_t agentRloc16;
                        uint8_t routerId;
                        VerifyOrExit((mNetif.GetNetworkDataLeader().GetRlocByContextId(
                                          static_cast<uint8_t>(aloc16 & Mle::kAloc16DhcpAgentMask),
                                          agentRloc16) == OT_ERROR_NONE),
                                     error = OT_ERROR_DROP);

                        routerId = mNetif.GetMle().GetRouterId(agentRloc16);

                        // if agent is active router or the child of the device
                        if ((mNetif.GetMle().IsActiveRouter(agentRloc16)) ||
                            (mNetif.GetMle().GetRloc16(routerId) == mNetif.GetMle().GetRloc16()))
                        {
                            mMeshDest = agentRloc16;
                        }
                        else
                        {
                            // use the parent of the ED Agent as Dest
                            mMeshDest = mNetif.GetMle().GetRloc16(routerId);
                        }
                    }

#endif  // OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
                    else
                    {
                        // TODO: support ALOC for Service, Commissioner, Neighbor Discovery Agent
                        ExitNow(error = OT_ERROR_DROP);
                    }
                }
                else if ((neighbor = mNetif.GetMle().GetNeighbor(ip6Header.GetDestination())) != NULL)
                {
                    mMeshDest = neighbor->GetRloc16();
                }
                else if (mNetif.GetNetworkDataLeader().IsOnMesh(ip6Header.GetDestination()))
                {
                    SuccessOrExit(error = mNetif.GetAddressResolver().Resolve(ip6Header.GetDestination(), mMeshDest));
                }
                else
                {
                    mNetif.GetNetworkDataLeader().RouteLookup(
                        ip6Header.GetSource(),
                        ip6Header.GetDestination(),
                        NULL,
                        &mMeshDest
                    );
                }

                VerifyOrExit(mMeshDest != Mac::kShortAddrInvalid, error = OT_ERROR_DROP);

                if (mNetif.GetMle().GetNeighbor(mMeshDest) != NULL)
                {
                    // destination is neighbor
                    mMacDest.mLength = sizeof(mMacDest.mShortAddress);
                    mMacDest.mShortAddress = mMeshDest;
                    GetMacSourceAddress(ip6Header.GetSource(), mMacSource);
                }
                else
                {
                    // destination is not neighbor
                    mMeshSource = mNetif.GetMac().GetShortAddress();

                    SuccessOrExit(error = mNetif.GetMle().CheckReachability(mMeshSource, mMeshDest, ip6Header));

                    mMacDest.mLength = sizeof(mMacDest.mShortAddress);
                    mMacDest.mShortAddress = mNetif.GetMle().GetNextHop(mMeshDest);
                    mMacSource.mLength = sizeof(mMacSource.mShortAddress);
                    mMacSource.mShortAddress = mMeshSource;
                    mAddMeshHeader = true;
                }
            }

            break;
        }
#endif  // OPENTHREAD_FTD

    default:
        break;
    }

exit:
    return error;
}

void MeshForwarder::SetRxOff(void)
{
    mNetif.GetMac().SetRxOnWhenIdle(false);
    mDataPollManager.StopPolling();
    mNetif.GetSupervisionListener().Stop();
}

bool MeshForwarder::GetRxOnWhenIdle(void)
{
    return mNetif.GetMac().GetRxOnWhenIdle();
}

void MeshForwarder::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    mNetif.GetMac().SetRxOnWhenIdle(aRxOnWhenIdle);

    if (aRxOnWhenIdle)
    {
        mDataPollManager.StopPolling();
        mNetif.GetSupervisionListener().Stop();
    }
    else
    {
        mDataPollManager.StartPolling();
        mNetif.GetSupervisionListener().Start();
    }
}

otError MeshForwarder::GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    aMacAddr.mLength = sizeof(aMacAddr.mExtAddress);
    aMacAddr.mExtAddress.Set(aIp6Addr);

    if (memcmp(&aMacAddr.mExtAddress, mNetif.GetMac().GetExtAddress(), sizeof(aMacAddr.mExtAddress)) != 0)
    {
        aMacAddr.mLength = sizeof(aMacAddr.mShortAddress);
        aMacAddr.mShortAddress = mNetif.GetMac().GetShortAddress();
    }

    return OT_ERROR_NONE;
}

otError MeshForwarder::GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    if (aIp6Addr.IsMulticast())
    {
        aMacAddr.mLength = sizeof(aMacAddr.mShortAddress);
        aMacAddr.mShortAddress = Mac::kShortAddrBroadcast;
    }
    else if (aIp6Addr.mFields.m16[0] == HostSwap16(0xfe80) &&
             aIp6Addr.mFields.m16[1] == HostSwap16(0x0000) &&
             aIp6Addr.mFields.m16[2] == HostSwap16(0x0000) &&
             aIp6Addr.mFields.m16[3] == HostSwap16(0x0000) &&
             aIp6Addr.mFields.m16[4] == HostSwap16(0x0000) &&
             aIp6Addr.mFields.m16[5] == HostSwap16(0x00ff) &&
             aIp6Addr.mFields.m16[6] == HostSwap16(0xfe00))
    {
        aMacAddr.mLength = sizeof(aMacAddr.mShortAddress);
        aMacAddr.mShortAddress = HostSwap16(aIp6Addr.mFields.m16[7]);
    }
    else if (mNetif.GetMle().IsRoutingLocator(aIp6Addr))
    {
        aMacAddr.mLength = sizeof(aMacAddr.mShortAddress);
        aMacAddr.mShortAddress = HostSwap16(aIp6Addr.mFields.m16[7]);
    }
    else
    {
        aMacAddr.mLength = sizeof(aMacAddr.mExtAddress);
        aMacAddr.mExtAddress.Set(aIp6Addr);
    }

    return OT_ERROR_NONE;
}

otError MeshForwarder::HandleFrameRequest(void *aContext, Mac::Frame &aFrame)
{
    return static_cast<MeshForwarder *>(aContext)->HandleFrameRequest(aFrame);
}

otError MeshForwarder::HandleFrameRequest(Mac::Frame &aFrame)
{
    otError error = OT_ERROR_NONE;
    Mac::Address macDest;
    Child *child = NULL;

    VerifyOrExit(mEnabled, error = OT_ERROR_ABORT);

    mSendBusy = true;

    if (mSendMessage == NULL)
    {
        SendEmptyFrame(aFrame, false);
        aFrame.SetIsARetransmission(false);
        aFrame.SetMaxTxAttempts(Mac::kDirectFrameMacTxAttempts);
        ExitNow();
    }

    switch (mSendMessage->GetType())
    {
    case Message::kTypeIp6:
        if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
        {
            mNetif.GetMac().SetChannel(mScanChannel);
            aFrame.SetChannel(mScanChannel);

            // In case a specific PAN ID of a Thread Network to be discovered is not known, Discovery
            // Request messages MUST have the Destination PAN ID in the IEEE 802.15.4 MAC header set
            // to be the Broadcast PAN ID (0xFFFF) and the Source PAN ID set to a randomly generated
            // value.
            if (mSendMessage->GetPanId() == Mac::kPanIdBroadcast &&
                mNetif.GetMac().GetPanId() == Mac::kPanIdBroadcast)
            {
                uint16_t panid;

                do
                {
                    panid = static_cast<uint16_t>(otPlatRandomGet());
                }
                while (panid == Mac::kPanIdBroadcast);

                mNetif.GetMac().SetPanId(panid);
            }
        }

        error = SendFragment(*mSendMessage, aFrame);

        // `SendFragment()` fails with `NotCapable` error if the message is MLE (with
        // no link layer security) and also requires fragmentation.
        if (error == OT_ERROR_NOT_CAPABLE)
        {
            // Enable security and try again.
            mSendMessage->SetLinkSecurityEnabled(true);
            error = SendFragment(*mSendMessage, aFrame);
        }

        assert(aFrame.GetLength() != 7);
        break;

    case Message::kType6lowpan:
        error = SendMesh(*mSendMessage, aFrame);
        break;

    case Message::kTypeMacDataPoll:
        error = SendPoll(*mSendMessage, aFrame);
        break;

    case Message::kTypeSupervision:
        error = SendEmptyFrame(aFrame, kSupervisionMsgAckRequest);
        mMessageNextOffset = mSendMessage->GetLength();
        break;
    }

    assert(error == OT_ERROR_NONE);

    aFrame.GetDstAddr(macDest);

    // Set `FramePending` if there are more queued messages (excluding
    // the current one being sent out) for the child (note `> 1` check).
    // The case where the current message requires fragmentation is
    // already checked and handled in `SendFragment()` method.

    if (((child = mNetif.GetMle().GetChild(macDest)) != NULL)
        && !child->IsRxOnWhenIdle()
        && (child->GetIndirectMessageCount() > 1))
    {
        aFrame.SetFramePending(true);
    }

    aFrame.SetIsARetransmission(mSendMessageIsARetransmission);
    aFrame.SetMaxTxAttempts(mSendMessageMaxMacTxAttempts);

    if (mSendMessageIsARetransmission)
    {
        // If this is the re-transmission of an indirect frame to a sleepy child, we
        // ensure to use the same frame counter, key id, and data sequence number as
        // the last attempt.

        aFrame.SetSequence(mSendMessageDataSequenceNumber);

        if (aFrame.GetSecurityEnabled())
        {
            aFrame.SetFrameCounter(mSendMessageFrameCounter);
            aFrame.SetKeyId(mSendMessageKeyId);
        }
    }

exit:
    return error;
}

otError MeshForwarder::SendPoll(Message &aMessage, Mac::Frame &aFrame)
{
    Mac::Address macSource;
    uint16_t fcf;
    Neighbor *neighbor;

    macSource.mShortAddress = mNetif.GetMac().GetShortAddress();

    if (macSource.mShortAddress != Mac::kShortAddrInvalid)
    {
        macSource.mLength = sizeof(macSource.mShortAddress);
    }
    else
    {
        macSource.mLength = sizeof(macSource.mExtAddress);
        memcpy(&macSource.mExtAddress, mNetif.GetMac().GetExtAddress(), sizeof(macSource.mExtAddress));
    }

    // initialize MAC header
    fcf = Mac::Frame::kFcfFrameMacCmd | Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfFrameVersion2006;

    if (macSource.mLength == sizeof(Mac::ShortAddress))
    {
        fcf |= Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort;
    }
    else
    {
        fcf |= Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrExt;
    }

    fcf |= Mac::Frame::kFcfAckRequest | Mac::Frame::kFcfSecurityEnabled;

    aFrame.InitMacHeader(fcf, Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32);
    aFrame.SetDstPanId(mNetif.GetMac().GetPanId());

    neighbor = mNetif.GetMle().GetParent();
    assert(neighbor != NULL);

    if (macSource.mLength == 2)
    {
        aFrame.SetDstAddr(neighbor->GetRloc16());
        aFrame.SetSrcAddr(macSource.mShortAddress);
    }
    else
    {
        aFrame.SetDstAddr(neighbor->GetExtAddress());
        aFrame.SetSrcAddr(macSource.mExtAddress);
    }

    aFrame.SetCommandId(Mac::Frame::kMacCmdDataRequest);

    mMessageNextOffset = aMessage.GetLength();

    return OT_ERROR_NONE;
}

otError MeshForwarder::SendMesh(Message &aMessage, Mac::Frame &aFrame)
{
    uint16_t fcf;

    // initialize MAC header
    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfFrameVersion2006 |
          Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort |
          Mac::Frame::kFcfAckRequest | Mac::Frame::kFcfSecurityEnabled;

    aFrame.InitMacHeader(fcf, Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32);
    aFrame.SetDstPanId(mNetif.GetMac().GetPanId());
    aFrame.SetDstAddr(mMacDest.mShortAddress);
    aFrame.SetSrcAddr(mMacSource.mShortAddress);

    // write payload
    assert(aMessage.GetLength() <= aFrame.GetMaxPayloadLength());
    aMessage.Read(0, aMessage.GetLength(), aFrame.GetPayload());
    aFrame.SetPayloadLength(static_cast<uint8_t>(aMessage.GetLength()));

    mMessageNextOffset = aMessage.GetLength();

    return OT_ERROR_NONE;
}

otError MeshForwarder::SendFragment(Message &aMessage, Mac::Frame &aFrame)
{
    Mac::Address meshDest, meshSource;
    uint16_t fcf;
    Lowpan::FragmentHeader *fragmentHeader;
    Lowpan::MeshHeader meshHeader;
    uint8_t *payload;
    uint8_t headerLength;
    uint8_t hopsLeft;
    uint16_t payloadLength;
    int hcLength;
    uint16_t fragmentLength;
    uint16_t dstpan;
    uint8_t secCtl = Mac::Frame::kSecNone;
    otError error = OT_ERROR_NONE;

    if (mAddMeshHeader)
    {
        meshSource.mLength = sizeof(meshSource.mShortAddress);
        meshSource.mShortAddress = mMeshSource;
        meshDest.mLength = sizeof(meshDest.mShortAddress);
        meshDest.mShortAddress = mMeshDest;
    }
    else
    {
        meshDest = mMacDest;
        meshSource = mMacSource;
    }

    // initialize MAC header
    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfFrameVersion2006;
    fcf |= (mMacDest.mLength == 2) ? Mac::Frame::kFcfDstAddrShort : Mac::Frame::kFcfDstAddrExt;
    fcf |= (mMacSource.mLength == 2) ? Mac::Frame::kFcfSrcAddrShort : Mac::Frame::kFcfSrcAddrExt;

    // all unicast frames request ACK
    if (mMacDest.mLength == 8 || mMacDest.mShortAddress != Mac::kShortAddrBroadcast)
    {
        fcf |= Mac::Frame::kFcfAckRequest;
    }

    if (aMessage.IsLinkSecurityEnabled())
    {
        fcf |= Mac::Frame::kFcfSecurityEnabled;

        switch (aMessage.GetSubType())
        {
        case Message::kSubTypeJoinerEntrust:
            secCtl = static_cast<uint8_t>(Mac::Frame::kKeyIdMode0);
            break;

        case Message::kSubTypeMleAnnounce:
            secCtl = static_cast<uint8_t>(Mac::Frame::kKeyIdMode2);
            break;

        default:
            secCtl = static_cast<uint8_t>(Mac::Frame::kKeyIdMode1);
            break;
        }

        secCtl |= Mac::Frame::kSecEncMic32;
    }

    dstpan = mNetif.GetMac().GetPanId();

    switch (aMessage.GetSubType())
    {
    case Message::kSubTypeMleAnnounce:
        aFrame.SetChannel(aMessage.GetChannel());
        dstpan = Mac::kPanIdBroadcast;
        break;

    case Message::kSubTypeMleDiscoverRequest:
    case Message::kSubTypeMleDiscoverResponse:
        dstpan = aMessage.GetPanId();
        break;

    default:
        break;
    }

    if (dstpan == mNetif.GetMac().GetPanId())
    {
        fcf |= Mac::Frame::kFcfPanidCompression;
    }

    aFrame.InitMacHeader(fcf, secCtl);
    aFrame.SetDstPanId(dstpan);
    aFrame.SetSrcPanId(mNetif.GetMac().GetPanId());

    if (mMacDest.mLength == 2)
    {
        aFrame.SetDstAddr(mMacDest.mShortAddress);
    }
    else
    {
        aFrame.SetDstAddr(mMacDest.mExtAddress);
    }

    if (mMacSource.mLength == 2)
    {
        aFrame.SetSrcAddr(mMacSource.mShortAddress);
    }
    else
    {
        aFrame.SetSrcAddr(mMacSource.mExtAddress);
    }

    payload = aFrame.GetPayload();

    headerLength = 0;

    // initialize Mesh header
    if (mAddMeshHeader)
    {
        if (mNetif.GetMle().GetRole() == OT_DEVICE_ROLE_CHILD)
        {
            // REED sets hopsLeft to max (16) + 1. It does not know the route cost.
            hopsLeft = Mle::kMaxRouteCost + 1;
        }
        else
        {
            // Calculate the number of predicted hops.
            hopsLeft = mNetif.GetMle().GetRouteCost(mMeshDest);

            if (hopsLeft != Mle::kMaxRouteCost)
            {
                hopsLeft += mNetif.GetMle().GetLinkCost(
                                mNetif.GetMle().GetRouterId(mNetif.GetMle().GetNextHop(mMeshDest)));
            }
            else
            {
                // In case there is no route to the destination router (only link).
                hopsLeft = mNetif.GetMle().GetLinkCost(mNetif.GetMle().GetRouterId(mMeshDest));
            }

        }

        // The hopsLft field MUST be incremented by one if the destination RLOC16
        // is not that of an active Router.
        if (!mNetif.GetMle().IsActiveRouter(mMeshDest))
        {
            hopsLeft += 1;
        }

        meshHeader.Init();
        meshHeader.SetHopsLeft(hopsLeft + Lowpan::MeshHeader::kAdditionalHopsLeft);
        meshHeader.SetSource(mMeshSource);
        meshHeader.SetDestination(mMeshDest);
        meshHeader.AppendTo(payload);
        payload += meshHeader.GetHeaderLength();
        headerLength += meshHeader.GetHeaderLength();
    }

    // copy IPv6 Header
    if (aMessage.GetOffset() == 0)
    {
        hcLength = mNetif.GetLowpan().Compress(aMessage, meshSource, meshDest, payload);
        assert(hcLength > 0);
        headerLength += static_cast<uint8_t>(hcLength);

        payloadLength = aMessage.GetLength() - aMessage.GetOffset();

        fragmentLength = aFrame.GetMaxPayloadLength() - headerLength;

        if (payloadLength > fragmentLength)
        {
            if ((!aMessage.IsLinkSecurityEnabled()) && aMessage.IsSubTypeMle())
            {
                aMessage.SetOffset(0);
                ExitNow(error = OT_ERROR_NOT_CAPABLE);
            }

            // write Fragment header
            if (aMessage.GetDatagramTag() == 0)
            {
                // avoid using datagram tag value 0, which indicates the tag has not been set
                if (mFragTag == 0)
                {
                    mFragTag++;
                }

                aMessage.SetDatagramTag(mFragTag++);
            }

            memmove(payload + 4, payload, headerLength);

            payloadLength = (aFrame.GetMaxPayloadLength() - headerLength - 4) & ~0x7;

            fragmentHeader = reinterpret_cast<Lowpan::FragmentHeader *>(payload);
            fragmentHeader->Init();
            fragmentHeader->SetDatagramSize(aMessage.GetLength());
            fragmentHeader->SetDatagramTag(aMessage.GetDatagramTag());
            fragmentHeader->SetDatagramOffset(0);

            payload += fragmentHeader->GetHeaderLength();
            headerLength += fragmentHeader->GetHeaderLength();
        }

        payload += hcLength;

        // copy IPv6 Payload
        aMessage.Read(aMessage.GetOffset(), payloadLength, payload);
        aFrame.SetPayloadLength(static_cast<uint8_t>(headerLength + payloadLength));

        mMessageNextOffset = aMessage.GetOffset() + payloadLength;
        aMessage.SetOffset(0);
    }
    else
    {
        payloadLength = aMessage.GetLength() - aMessage.GetOffset();

        // write Fragment header
        fragmentHeader = reinterpret_cast<Lowpan::FragmentHeader *>(payload);
        fragmentHeader->Init();
        fragmentHeader->SetDatagramSize(aMessage.GetLength());
        fragmentHeader->SetDatagramTag(aMessage.GetDatagramTag());
        fragmentHeader->SetDatagramOffset(aMessage.GetOffset());

        payload += fragmentHeader->GetHeaderLength();
        headerLength += fragmentHeader->GetHeaderLength();

        fragmentLength = (aFrame.GetMaxPayloadLength() - headerLength) & ~0x7;

        if (payloadLength > fragmentLength)
        {
            payloadLength = fragmentLength;
        }

        // copy IPv6 Payload
        aMessage.Read(aMessage.GetOffset(), payloadLength, payload);
        aFrame.SetPayloadLength(static_cast<uint8_t>(headerLength + payloadLength));

        mMessageNextOffset = aMessage.GetOffset() + payloadLength;
    }

    if (mMessageNextOffset < aMessage.GetLength())
    {
        aFrame.SetFramePending(true);
    }

exit:

    return error;
}

otError MeshForwarder::SendEmptyFrame(Mac::Frame &aFrame, bool aAckRequest)
{
    uint16_t fcf;
    uint8_t secCtl;
    Mac::Address macSource;

    macSource.mShortAddress = mNetif.GetMac().GetShortAddress();

    if (macSource.mShortAddress != Mac::kShortAddrInvalid)
    {
        macSource.mLength = sizeof(macSource.mShortAddress);
    }
    else
    {
        macSource.mLength = sizeof(macSource.mExtAddress);
        memcpy(&macSource.mExtAddress, mNetif.GetMac().GetExtAddress(), sizeof(macSource.mExtAddress));
    }

    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfFrameVersion2006;
    fcf |= (mMacDest.mLength == 2) ? Mac::Frame::kFcfDstAddrShort : Mac::Frame::kFcfDstAddrExt;
    fcf |= (macSource.mLength == 2) ? Mac::Frame::kFcfSrcAddrShort : Mac::Frame::kFcfSrcAddrExt;

    if (aAckRequest)
    {
        fcf |= Mac::Frame::kFcfAckRequest;
    }

    fcf |= Mac::Frame::kFcfSecurityEnabled;
    secCtl = Mac::Frame::kKeyIdMode1;
    secCtl |= Mac::Frame::kSecEncMic32;

    fcf |= Mac::Frame::kFcfPanidCompression;

    aFrame.InitMacHeader(fcf, secCtl);

    aFrame.SetDstPanId(mNetif.GetMac().GetPanId());
    aFrame.SetSrcPanId(mNetif.GetMac().GetPanId());

    if (mMacDest.mLength == 2)
    {
        aFrame.SetDstAddr(mMacDest.mShortAddress);
    }
    else
    {
        aFrame.SetDstAddr(mMacDest.mExtAddress);
    }

    if (macSource.mLength == 2)
    {
        aFrame.SetSrcAddr(macSource.mShortAddress);
    }
    else
    {
        aFrame.SetSrcAddr(macSource.mExtAddress);
    }

    aFrame.SetPayloadLength(0);
    aFrame.SetFramePending(false);

    return OT_ERROR_NONE;
}

void MeshForwarder::HandleSentFrame(void *aContext, Mac::Frame &aFrame, otError aError)
{
    static_cast<MeshForwarder *>(aContext)->HandleSentFrame(aFrame, aError);
}

void MeshForwarder::HandleSentFrame(Mac::Frame &aFrame, otError aError)
{
    Mac::Address macDest;
    Child *child;
    Neighbor *neighbor;
    uint8_t childIndex;

    mSendBusy = false;

    VerifyOrExit(mEnabled);

    if (mSendMessage != NULL)
    {
        mSendMessage->SetOffset(mMessageNextOffset);
    }

    aFrame.GetDstAddr(macDest);

    if ((neighbor = mNetif.GetMle().GetNeighbor(macDest)) != NULL)
    {
        switch (aError)
        {
        case OT_ERROR_NONE:
            if (aFrame.GetAckRequest())
            {
                neighbor->ResetLinkFailures();
            }

            break;

        case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        case OT_ERROR_ABORT:
            break;

        case OT_ERROR_NO_ACK:
            neighbor->IncrementLinkFailures();

            if (mNetif.GetMle().IsActiveRouter(neighbor->GetRloc16()))
            {
                if (neighbor->GetLinkFailures() >= Mle::kFailedRouterTransmissions)
                {
                    mNetif.GetMle().RemoveNeighbor(*neighbor);
                }
            }

            break;

        default:
            assert(false);
            break;
        }
    }

    if ((child = mNetif.GetMle().GetChild(macDest)) != NULL)
    {
        child->SetDataRequestPending(false);

        VerifyOrExit(mSendMessage != NULL);

        if (mSendMessage == child->GetIndirectMessage())
        {
            switch (aError)
            {
            case OT_ERROR_NONE:
                child->ResetIndirectTxAttempts();
                break;

            default:
                child->IncrementIndirectTxAttempts();

                if (child->GetIndirectTxAttempts() < kMaxPollTriggeredTxAttempts)
                {
                    // We save the frame counter, key id, and data sequence number of
                    // current frame so we use the same values for the retransmission
                    // of the frame following the receipt of a data request command (data
                    // poll) from the sleepy child.

                    if (aFrame.GetSecurityEnabled())
                    {
                        uint32_t frameCounter;
                        uint8_t keyId;

                        aFrame.GetFrameCounter(frameCounter);
                        child->SetIndirectFrameCounter(frameCounter);

                        aFrame.GetKeyId(keyId);
                        child->SetIndirectKeyId(keyId);

                        child->SetIndirectDataSequenceNumber(aFrame.GetSequence());
                    }

                    ExitNow();
                }

                child->ResetIndirectTxAttempts();

                // We set the NextOffset to end of message, since there is no need to
                // send any remaining fragments in the message to the child, if all tx
                // attempts of current frame already failed.

                mMessageNextOffset = mSendMessage->GetLength();

                break;
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
            if (mSendMessage == child->GetIndirectMessage())
            {
                child->SetIndirectFragmentOffset(0);
                child->SetIndirectMessage(NULL);

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

            childIndex = mNetif.GetMle().GetChildIndex(*child);

            if (mSendMessage->GetChildMask(childIndex))
            {
                mSendMessage->ClearChildMask(childIndex);
                mSourceMatchController.DecrementMessageCount(*child);
            }
        }

        if (aError == OT_ERROR_NONE)
        {
            mNetif.GetChildSupervisor().UpdateOnSend(*child);
        }
    }

    VerifyOrExit(mSendMessage != NULL);

    if (mSendMessage->GetDirectTransmission())
    {
        if (mMessageNextOffset < mSendMessage->GetLength())
        {
            mSendMessage->SetOffset(mMessageNextOffset);
        }
        else
        {
            mSendMessage->ClearDirectTransmission();
            mSendMessage->SetOffset(0);
        }

        if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
        {
            mSendBusy = true;
            mDiscoverTimer.Start(static_cast<uint16_t>(Mac::kScanDurationDefault));
            ExitNow();
        }
    }

    if (mSendMessage->GetType() == Message::kTypeMacDataPoll)
    {
        neighbor = mNetif.GetMle().GetParent();

        if (neighbor->GetState() == Neighbor::kStateInvalid)
        {
            mDataPollManager.StopPolling();
            mNetif.GetMle().BecomeDetached();
        }
        else
        {
            mDataPollManager.HandlePollSent(aError);
        }
    }

    if (mMessageNextOffset >= mSendMessage->GetLength())
    {
        LogIp6Message(kMessageTransmit, *mSendMessage, &macDest, aError);

        if (aError == OT_ERROR_NONE)
        {
            mIpCounters.mTxSuccess++;
        }
        else
        {
            mIpCounters.mTxFailure++;
        }
    }

    if (mSendMessage->GetDirectTransmission() == false && mSendMessage->IsChildPending() == false)
    {
        mSendQueue.Dequeue(*mSendMessage);
        mSendMessage->Free();
        mSendMessage = NULL;
        mMessageNextOffset = 0;
    }

exit:

    if (mEnabled)
    {
        mScheduleTransmissionTask.Post();
    }
}

void MeshForwarder::SetDiscoverParameters(uint32_t aScanChannels)
{
    mScanChannels = (aScanChannels == 0) ? static_cast<uint32_t>(Mac::kScanChannelsAll) : aScanChannels;
}

void MeshForwarder::HandleDiscoverTimer(void *aContext)
{
    MeshForwarder *obj = static_cast<MeshForwarder *>(aContext);
    obj->HandleDiscoverTimer();
}

void MeshForwarder::HandleDiscoverTimer(void)
{
    do
    {
        mScanChannels >>= 1;
        mScanChannel++;

        if (mScanChannel > OT_RADIO_CHANNEL_MAX)
        {
            mSendQueue.Dequeue(*mSendMessage);
            mSendMessage->Free();
            mSendMessage = NULL;
            mNetif.GetMac().SetChannel(mRestoreChannel);
            mNetif.GetMac().SetPanId(mRestorePanId);
            mScanning = false;
            mNetif.GetMle().HandleDiscoverComplete();
            ExitNow();
        }
    }
    while ((mScanChannels & 1) == 0);

    mSendMessage->SetDirectTransmission();

exit:
    mSendBusy = false;
    mScheduleTransmissionTask.Post();
}

void MeshForwarder::HandleReceivedFrame(void *aContext, Mac::Frame &aFrame)
{
    static_cast<MeshForwarder *>(aContext)->HandleReceivedFrame(aFrame);
}

void MeshForwarder::HandleReceivedFrame(Mac::Frame &aFrame)
{
    ThreadMessageInfo messageInfo;
    Mac::Address macDest;
    Mac::Address macSource;
    uint8_t *payload;
    uint8_t payloadLength;
    uint8_t commandId;
    otError error = OT_ERROR_NONE;

    if (!mEnabled)
    {
        ExitNow(error = OT_ERROR_INVALID_STATE);
    }

    SuccessOrExit(error = aFrame.GetSrcAddr(macSource));
    SuccessOrExit(error = aFrame.GetDstAddr(macDest));

    aFrame.GetSrcPanId(messageInfo.mPanId);
    messageInfo.mChannel = aFrame.GetChannel();
    messageInfo.mRss = aFrame.GetPower();
    messageInfo.mLqi = aFrame.GetLqi();
    messageInfo.mLinkSecurity = aFrame.GetSecurityEnabled();

    payload = aFrame.GetPayload();
    payloadLength = aFrame.GetPayloadLength();

    mNetif.GetSupervisionListener().UpdateOnReceive(macSource, messageInfo.mLinkSecurity);

    mDataPollManager.CheckFramePending(aFrame);

    switch (aFrame.GetType())
    {
    case Mac::Frame::kFcfFrameData:
        if (payloadLength >= sizeof(Lowpan::MeshHeader) &&
            reinterpret_cast<Lowpan::MeshHeader *>(payload)->IsMeshHeader())
        {
            HandleMesh(payload, payloadLength, macSource, messageInfo);
        }
        else if (payloadLength >= sizeof(Lowpan::FragmentHeader) &&
                 reinterpret_cast<Lowpan::FragmentHeader *>(payload)->IsFragmentHeader())
        {
            HandleFragment(payload, payloadLength, macSource, macDest, messageInfo);
        }
        else if (payloadLength >= 1 && Lowpan::Lowpan::IsLowpanHc(payload))
        {
            HandleLowpanHC(payload, payloadLength, macSource, macDest, messageInfo);
        }
        else
        {
            error = OT_ERROR_NOT_LOWPAN_DATA_FRAME;
        }

        break;

    case Mac::Frame::kFcfFrameMacCmd:
        aFrame.GetCommandId(commandId);

        if (commandId == Mac::Frame::kMacCmdDataRequest)
        {
            HandleDataRequest(macSource, messageInfo);
        }
        else
        {
            error = OT_ERROR_DROP;
        }

        break;

    default:
        error = OT_ERROR_DROP;
        break;
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        char stringBuffer[Mac::Frame::kInfoStringSize];

        otLogInfoMac(GetInstance(), "Dropping rx frame, error:%s, %s", otThreadErrorToString(error),
                     aFrame.ToInfoString(stringBuffer, sizeof(stringBuffer)));

        (void)stringBuffer;
    }
}

void MeshForwarder::HandleMesh(uint8_t *aFrame, uint8_t aFrameLength, const Mac::Address &aMacSource,
                               const ThreadMessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Message *message = NULL;
    Mac::Address meshDest;
    Mac::Address meshSource;
    Lowpan::MeshHeader meshHeader;

    // Check the mesh header
    VerifyOrExit(meshHeader.Init(aFrame, aFrameLength) == OT_ERROR_NONE, error = OT_ERROR_DROP);

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aMessageInfo.mLinkSecurity && meshHeader.IsValid(), error = OT_ERROR_SECURITY);

    meshSource.mLength = sizeof(meshSource.mShortAddress);
    meshSource.mShortAddress = meshHeader.GetSource();
    meshDest.mLength = sizeof(meshDest.mShortAddress);
    meshDest.mShortAddress = meshHeader.GetDestination();

    if (meshDest.mShortAddress == mNetif.GetMac().GetShortAddress())
    {
        aFrame += meshHeader.GetHeaderLength();
        aFrameLength -= meshHeader.GetHeaderLength();

        if (reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
        {
            HandleFragment(aFrame, aFrameLength, meshSource, meshDest, aMessageInfo);
        }
        else if (Lowpan::Lowpan::IsLowpanHc(aFrame))
        {
            HandleLowpanHC(aFrame, aFrameLength, meshSource, meshDest, aMessageInfo);
        }
        else
        {
            ExitNow(error = OT_ERROR_PARSE);
        }
    }
    else if (meshHeader.GetHopsLeft() > 0)
    {
        mNetif.GetMle().ResolveRoutingLoops(aMacSource.mShortAddress, meshDest.mShortAddress);

        SuccessOrExit(error = CheckReachability(aFrame, aFrameLength, meshSource, meshDest));

        meshHeader.SetHopsLeft(meshHeader.GetHopsLeft() - 1);
        meshHeader.AppendTo(aFrame);

        VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kType6lowpan, 0)) != NULL,
                     error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = message->SetLength(aFrameLength));
        message->Write(0, aFrameLength, aFrame);
        message->SetLinkSecurityEnabled(aMessageInfo.mLinkSecurity);
        message->SetPanId(aMessageInfo.mPanId);

        SendMessage(*message);
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        char srcStringBuffer[Mac::Address::kAddressStringSize];

        otLogInfoMac(
            GetInstance(),
            "Dropping rx mesh frame, error:%s, len:%d, src:%s, sec:%s",
            otThreadErrorToString(error),
            aFrameLength,
            aMacSource.ToString(srcStringBuffer, sizeof(srcStringBuffer)),
            aMessageInfo.mLinkSecurity ? "yes" : "no"
        );

        (void)srcStringBuffer;

        if (message != NULL)
        {
            message->Free();
        }
    }
}

otError MeshForwarder::CheckReachability(uint8_t *aFrame, uint8_t aFrameLength,
                                         const Mac::Address &aMeshSource, const Mac::Address &aMeshDest)
{
    otError error = OT_ERROR_NONE;
    Ip6::Header ip6Header;
    Lowpan::MeshHeader meshHeader;

    VerifyOrExit(meshHeader.Init(aFrame, aFrameLength) == OT_ERROR_NONE, error = OT_ERROR_DROP);

    // skip mesh header
    aFrame += meshHeader.GetHeaderLength();
    aFrameLength -= meshHeader.GetHeaderLength();

    // skip fragment header
    if (aFrameLength >= 1 &&
        reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
    {
        VerifyOrExit(sizeof(Lowpan::FragmentHeader) <= aFrameLength, error = OT_ERROR_DROP);
        VerifyOrExit(reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetDatagramOffset() == 0);

        aFrame += reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
        aFrameLength -= reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
    }

    // only process IPv6 packets
    VerifyOrExit(aFrameLength >= 1 && Lowpan::Lowpan::IsLowpanHc(aFrame));

    VerifyOrExit(mNetif.GetLowpan().DecompressBaseHeader(ip6Header, aMeshSource, aMeshDest, aFrame, aFrameLength) > 0,
                 error = OT_ERROR_DROP);

    error = mNetif.GetMle().CheckReachability(aMeshSource.mShortAddress, aMeshDest.mShortAddress, ip6Header);

exit:
    return error;
}

void MeshForwarder::HandleFragment(uint8_t *aFrame, uint8_t aFrameLength,
                                   const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                                   const ThreadMessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Lowpan::FragmentHeader *fragmentHeader = reinterpret_cast<Lowpan::FragmentHeader *>(aFrame);
    uint16_t datagramLength = fragmentHeader->GetDatagramSize();
    uint16_t datagramTag = fragmentHeader->GetDatagramTag();
    Message *message = NULL;
    int headerLength;

    if (fragmentHeader->GetDatagramOffset() == 0)
    {
        aFrame += fragmentHeader->GetHeaderLength();
        aFrameLength -= fragmentHeader->GetHeaderLength();

        VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeIp6, 0)) != NULL,
                     error = OT_ERROR_NO_BUFS);
        message->SetLinkSecurityEnabled(aMessageInfo.mLinkSecurity);
        message->SetPanId(aMessageInfo.mPanId);
        headerLength = mNetif.GetLowpan().Decompress(*message, aMacSource, aMacDest, aFrame, aFrameLength,
                                                     datagramLength);
        VerifyOrExit(headerLength > 0, error = OT_ERROR_PARSE);

        aFrame += headerLength;
        aFrameLength -= static_cast<uint8_t>(headerLength);

        VerifyOrExit(datagramLength >= message->GetOffset() + aFrameLength, error = OT_ERROR_PARSE);

        SuccessOrExit(error = message->SetLength(datagramLength));

        message->SetDatagramTag(datagramTag);
        message->SetTimeout(kReassemblyTimeout);

        // copy Fragment
        message->Write(message->GetOffset(), aFrameLength, aFrame);
        message->MoveOffset(aFrameLength);

        // Security Check
        VerifyOrExit(mNetif.GetIp6Filter().Accept(*message), error = OT_ERROR_DROP);

        // Allow re-assembly of only one message at a time on a SED by clearing
        // any remaining fragments in reassembly list upon receiving of a new
        // (secure) first fragment.

        if ((GetRxOnWhenIdle() == false) && message->IsLinkSecurityEnabled())
        {
            ClearReassemblyList();
        }

        mReassemblyList.Enqueue(*message);

        if (!mReassemblyTimer.IsRunning())
        {
            mReassemblyTimer.Start(kStateUpdatePeriod);
        }
    }
    else
    {
        aFrame += fragmentHeader->GetHeaderLength();
        aFrameLength -= fragmentHeader->GetHeaderLength();

        for (message = mReassemblyList.GetHead(); message; message = message->GetNext())
        {
            // Security Check: only consider reassembly buffers that had the same Security Enabled setting.
            if (message->GetLength() == datagramLength &&
                message->GetDatagramTag() == datagramTag &&
                message->GetOffset() == fragmentHeader->GetDatagramOffset() &&
                message->IsLinkSecurityEnabled() == aMessageInfo.mLinkSecurity)
            {
                break;
            }
        }

        // For a sleepy-end-device, if we receive a new (secure) next fragment
        // with a non-matching fragmentation offset or tag, it indicates that
        // we have either missed a fragment, or the parent has moved to a new
        // message with a new tag. In either case, we can safely clear any
        // remaining fragments stored in the reassembly list.

        if (GetRxOnWhenIdle() == false)
        {
            if ((message == NULL) && (aMessageInfo.mLinkSecurity))
            {
                ClearReassemblyList();
            }
        }

        VerifyOrExit(message != NULL, error = OT_ERROR_DROP);

        // copy Fragment
        message->Write(message->GetOffset(), aFrameLength, aFrame);
        message->MoveOffset(aFrameLength);
    }

exit:

    if (error == OT_ERROR_NONE)
    {
        if (message->GetOffset() >= message->GetLength())
        {
            mReassemblyList.Dequeue(*message);
            HandleDatagram(*message, aMessageInfo, aMacSource);
        }
    }
    else
    {
        char srcStringBuffer[Mac::Address::kAddressStringSize];
        char dstStringBuffer[Mac::Address::kAddressStringSize];

        (void)srcStringBuffer;
        (void)dstStringBuffer;

        otLogInfoMac(
            GetInstance(),
            "Dropping rx frag frame, error:%s, len:%d, src:%s, dst:%s, tag:%d, offset:%d, dglen:%d, sec:%s",
            otThreadErrorToString(error),
            aFrameLength,
            aMacSource.ToString(srcStringBuffer, sizeof(srcStringBuffer)),
            aMacDest.ToString(dstStringBuffer, sizeof(dstStringBuffer)),
            datagramTag,
            fragmentHeader->GetDatagramOffset(),
            datagramLength,
            aMessageInfo.mLinkSecurity ? "yes" : "no"
        );

        if (message != NULL)
        {
            message->Free();
        }
    }
}

void MeshForwarder::ClearReassemblyList(void)
{
    Message *message;
    Message *next;

    for (message = mReassemblyList.GetHead(); message; message = next)
    {
        next = message->GetNext();
        mReassemblyList.Dequeue(*message);

        LogIp6Message(kMessageDrop, *message, NULL, OT_ERROR_NO_FRAME_RECEIVED);
        mIpCounters.mRxFailure++;

        message->Free();
    }
}

void MeshForwarder::HandleReassemblyTimer(void *aContext)
{
    static_cast<MeshForwarder *>(aContext)->HandleReassemblyTimer();
}

void MeshForwarder::HandleReassemblyTimer()
{
    Message *next = NULL;
    uint8_t timeout;

    for (Message *message = mReassemblyList.GetHead(); message; message = next)
    {
        next = message->GetNext();
        timeout = message->GetTimeout();

        if (timeout > 0)
        {
            message->SetTimeout(timeout - 1);
        }
        else
        {
            mReassemblyList.Dequeue(*message);

            LogIp6Message(kMessageDrop, *message, NULL, OT_ERROR_REASSEMBLY_TIMEOUT);
            mIpCounters.mRxFailure++;

            message->Free();
        }
    }

    if (mReassemblyList.GetHead() != NULL)
    {
        mReassemblyTimer.Start(kStateUpdatePeriod);
    }
}

void MeshForwarder::HandleLowpanHC(uint8_t *aFrame, uint8_t aFrameLength,
                                   const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                                   const ThreadMessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Message *message;
    int headerLength;

    VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeIp6, 0)) != NULL,
                 error = OT_ERROR_NO_BUFS);
    message->SetLinkSecurityEnabled(aMessageInfo.mLinkSecurity);
    message->SetPanId(aMessageInfo.mPanId);

    headerLength = mNetif.GetLowpan().Decompress(*message, aMacSource, aMacDest, aFrame, aFrameLength, 0);
    VerifyOrExit(headerLength > 0, error = OT_ERROR_PARSE);

    aFrame += headerLength;
    aFrameLength -= static_cast<uint8_t>(headerLength);

    SuccessOrExit(error = message->SetLength(message->GetLength() + aFrameLength));
    message->Write(message->GetOffset(), aFrameLength, aFrame);

    // Security Check
    VerifyOrExit(mNetif.GetIp6Filter().Accept(*message), error = OT_ERROR_DROP);

exit:

    if (error == OT_ERROR_NONE)
    {
        HandleDatagram(*message, aMessageInfo, aMacSource);
    }
    else
    {
        char srcStringBuffer[Mac::Address::kAddressStringSize];
        char dstStringBuffer[Mac::Address::kAddressStringSize];

        (void)srcStringBuffer;
        (void)dstStringBuffer;

        otLogInfoMac(
            GetInstance(),
            "Dropping rx lowpan HC frame, error:%s, len:%d, src:%s, dst:%s, sec:%s",
            otThreadErrorToString(error),
            aFrameLength,
            aMacSource.ToString(srcStringBuffer, sizeof(srcStringBuffer)),
            aMacDest.ToString(dstStringBuffer, sizeof(dstStringBuffer)),
            aMessageInfo.mLinkSecurity ? "yes" : "no"
        );

        if (message != NULL)
        {
            message->Free();
        }
    }
}

otError MeshForwarder::HandleDatagram(Message &aMessage, const ThreadMessageInfo &aMessageInfo,
                                      const Mac::Address &aMacSource)
{
    LogIp6Message(kMessageReceive, aMessage, &aMacSource, OT_ERROR_NONE);
    mIpCounters.mRxSuccess++;

    return mNetif.GetIp6().HandleDatagram(aMessage, &mNetif, mNetif.GetInterfaceId(), &aMessageInfo, false);
}

void MeshForwarder::HandleDataRequest(const Mac::Address &aMacSource, const ThreadMessageInfo &aMessageInfo)
{
    Child *child;
    uint16_t indirectMsgCount;

    // Security Check: only process secure Data Poll frames.
    VerifyOrExit(aMessageInfo.mLinkSecurity);

    VerifyOrExit(mNetif.GetMle().GetRole() != OT_DEVICE_ROLE_DETACHED);

    VerifyOrExit((child = mNetif.GetMle().GetChild(aMacSource)) != NULL);
    child->SetLastHeard(Timer::GetNow());
    child->ResetLinkFailures();
    indirectMsgCount = child->GetIndirectMessageCount();

    if (!mSourceMatchController.IsEnabled() || (indirectMsgCount > 0))
    {
        child->SetDataRequestPending(true);
    }

    mScheduleTransmissionTask.Post();

    otLogInfoMac(GetInstance(), "Rx data poll, src:0x%04x, qed_msgs:%d", child->GetRloc16(), indirectMsgCount);

exit:
    return;
}

void MeshForwarder::HandleDataPollTimeout(void *aContext)
{
    static_cast<MeshForwarder *>(aContext)->GetDataPollManager().HandlePollTimeout();
}

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void MeshForwarder::LogIp6Message(MessageAction aAction, const Message &aMessage, const Mac::Address *aMacAddress,
                                  otError aError)
{
    uint16_t checksum = 0;
    Ip6::Header ip6Header;
    Ip6::IpProto protocol;
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];
    const char *actionText;
    const char *priorityText;
    bool shouldLogSrcDstAddresses = true;

    VerifyOrExit(aMessage.GetType() == Message::kTypeIp6);

    VerifyOrExit(sizeof(ip6Header) == aMessage.Read(0, sizeof(ip6Header), &ip6Header));
    VerifyOrExit(ip6Header.IsVersion6());

    protocol = ip6Header.GetNextHeader();

    switch (protocol)
    {
    case Ip6::kProtoUdp:
    {
        Ip6::UdpHeader udpHeader;

        if (sizeof(udpHeader) == aMessage.Read(sizeof(ip6Header), sizeof(udpHeader), &udpHeader))
        {
            checksum = udpHeader.GetChecksum();
        }

        break;
    }

    case Ip6::kProtoTcp:
    {
        Ip6::TcpHeader tcpHeader;

        if (sizeof(tcpHeader) == aMessage.Read(sizeof(ip6Header), sizeof(tcpHeader), &tcpHeader))
        {
            checksum = tcpHeader.GetChecksum();
        }

        break;
    }

    default:
        break;
    }

    switch (aAction)
    {
    case kMessageReceive:
        actionText = "Received";
        break;

    case kMessageTransmit:
        if (aError == OT_ERROR_NONE)
        {
            actionText = "Sent";
        }
        else
        {
            actionText = "Failed to send";
        }

        break;

    case kMessagePrepareIndirect:
        actionText = "Preping indir tx";
        shouldLogSrcDstAddresses = false;
        break;

    case kMessageDrop:
        actionText = "Dropping";
        break;

    default:
        actionText = "";
        break;
    }

    switch (aMessage.GetPriority())
    {
    case Message::kPriorityHigh:
        priorityText = "high";
        break;

    case Message::kPriorityMedium:
        priorityText = "medium";
        break;

    case Message::kPriorityLow:
        priorityText = "low";
        break;

    case Message::kPriorityVeryLow:
        priorityText = "verylow";
        break;

    default:
        priorityText = "unknown";
        break;
    }

    otLogInfoMac(
        GetInstance(),
        "%s IPv6 %s msg, len:%d, chksum:%04x%s%s, sec:%s%s%s, prio:%s",
        actionText,
        Ip6::Ip6::IpProtoToString(protocol),
        aMessage.GetLength(),
        checksum,
        (aMacAddress == NULL) ? "" : ((aAction == kMessageReceive) ? ", from:" : ", to:"),
        (aMacAddress == NULL) ? "" : aMacAddress->ToString(stringBuffer, sizeof(stringBuffer)),
        aMessage.IsLinkSecurityEnabled() ? "yes" : "no",
        (aError == OT_ERROR_NONE) ? "" : ", error:",
        (aError == OT_ERROR_NONE) ? "" : otThreadErrorToString(aError),
        priorityText
    );

    if (shouldLogSrcDstAddresses)
    {
        otLogInfoMac(GetInstance(), "src: %s", ip6Header.GetSource().ToString(stringBuffer, sizeof(stringBuffer)));
        otLogInfoMac(GetInstance(), "dst: %s", ip6Header.GetDestination().ToString(stringBuffer, sizeof(stringBuffer)));
    }

exit:
    return;
}

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void MeshForwarder::LogIp6Message(MessageAction, const Message &, const Mac::Address *, otError)
{
}

#endif //#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO

}  // namespace ot
