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

#include "openthread/platform/random.h"

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/encoding.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <net/ip6_filter.hpp>
#include <net/tcp.hpp>
#include <net/udp6.hpp>
#include <net/netif.hpp>
#include <thread/mesh_forwarder.hpp>
#include <thread/mle_router.hpp>
#include <thread/mle.hpp>
#include <thread/thread_netif.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {

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
    mSrcMatchEnabled(false)
{
    mFragTag = static_cast<uint16_t>(otPlatRandomGet());
    mNetif.GetMac().RegisterReceiver(mMacReceiver);
    mMacSource.mLength = 0;
    mMacDest.mLength = 0;
}

otInstance *MeshForwarder::GetInstance(void)
{
    return mNetif.GetInstance();
}

ThreadError MeshForwarder::Start(void)
{
    ThreadError error = kThreadError_None;

    if (mEnabled == false)
    {
        mNetif.GetMac().SetRxOnWhenIdle(true);
        mEnabled = true;
    }

    return error;
}

ThreadError MeshForwarder::Stop(void)
{
    ThreadError error = kThreadError_None;
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

void MeshForwarder::HandleResolved(const Ip6::Address &aEid, ThreadError aError)
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

            if (aError == kThreadError_None)
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

    VerifyOrExit(aChild.mQueuedIndirectMessageCnt > 0);

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

    aChild.mQueuedIndirectMessageCnt = 0;
    ClearSrcMatchEntry(aChild);

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

        if (child->IsStateValidOrRestoring() || child->mQueuedIndirectMessageCnt == 0)
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
    ThreadError error = kThreadError_None;
    uint8_t numChildren;
    Child *children;

    VerifyOrExit(mSendBusy == false, error = kThreadError_Busy);

    UpdateIndirectMessages();

    mSendMessageIsARetransmission = false;

    children = mNetif.GetMle().GetChildren(&numChildren);

    for (int i = 0; i < numChildren; i++)
    {
        Child &child = children[i];

        if (!child.IsStateValidOrRestoring() || !child.mDataRequest)
        {
            continue;
        }

        mSendMessage = child.mIndirectSendInfo.mMessage;
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

            if (child.mAddSrcMatchEntryShort)
            {
                mMacSource.mLength = sizeof(mMacSource.mShortAddress);
                mMacSource.mShortAddress = mNetif.GetMac().GetShortAddress();

                mMacDest.mLength = sizeof(mMacDest.mShortAddress);
                mMacDest.mShortAddress = child.mValid.mRloc16;
            }
            else
            {
                mMacSource.mLength = sizeof(mMacSource.mExtAddress);
                memcpy(mMacSource.mExtAddress.m8, mNetif.GetMac().GetExtAddress(), sizeof(mMacDest.mExtAddress));

                mMacDest.mLength = sizeof(mMacDest.mExtAddress);
                memcpy(mMacDest.mExtAddress.m8, child.mMacAddr.m8, sizeof(mMacDest.mExtAddress));
            }
        }

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

ThreadError MeshForwarder::AddPendingSrcMatchEntries(void)
{
    uint8_t numChildren;
    Child *children = NULL;
    ThreadError error = kThreadError_NoBufs;

    children = mNetif.GetMle().GetChildren(&numChildren);

    // Add pending short address first
    for (uint8_t i = 0; i < numChildren; i++)
    {
        if (children[i].IsStateValidOrRestoring() &&
            children[i].mAddSrcMatchEntryPending &&
            children[i].mAddSrcMatchEntryShort)
        {
            VerifyOrExit(((error = AddSrcMatchEntry(children[i])) == kThreadError_None));
        }
    }

    // Add pending extended address
    for (uint8_t i = 0; i < numChildren; i++)
    {
        if (children[i].IsStateValidOrRestoring() &&
            children[i].mAddSrcMatchEntryPending &&
            !children[i].mAddSrcMatchEntryShort)
        {
            VerifyOrExit(((error = AddSrcMatchEntry(children[i])) == kThreadError_None));
        }
    }

exit:
    return error;
}

ThreadError MeshForwarder::AddSrcMatchEntry(Child &aChild)
{
    ThreadError error = kThreadError_NoBufs;
    Mac::Address macAddr;

    otLogDebgMac(GetInstance(), "Queuing for child (0x%x)", aChild.mValid.mRloc16);
    otLogDebgMac(GetInstance(), "SrcMatch %d (0:Dis, 1:En))", mSrcMatchEnabled);

    // first queued message, to be added into source match table
    if (aChild.mQueuedIndirectMessageCnt == 1)
    {
        aChild.mAddSrcMatchEntryPending = true;
    }

    VerifyOrExit(aChild.mAddSrcMatchEntryPending);

    if (aChild.mAddSrcMatchEntryShort)
    {
        macAddr.mLength = sizeof(macAddr.mShortAddress);
        macAddr.mShortAddress = aChild.mValid.mRloc16;
    }
    else
    {
        macAddr.mLength = sizeof(macAddr.mExtAddress);
        memcpy(macAddr.mExtAddress.m8, aChild.mMacAddr.m8, sizeof(macAddr.mExtAddress));
    }

    if ((error = mNetif.GetMac().AddSrcMatchEntry(macAddr)) == kThreadError_None)
    {
        // succeed in adding to source match table
        aChild.mAddSrcMatchEntryPending = false;

        if (!mSrcMatchEnabled)
        {
            mNetif.GetMac().EnableSrcMatch(true);
            mSrcMatchEnabled = true;
        }
    }
    else
    {
        if (mSrcMatchEnabled)
        {
            mNetif.GetMac().EnableSrcMatch(false);
            mSrcMatchEnabled = false;
        }
    }

exit:
    return error;
}

void MeshForwarder::ClearSrcMatchEntry(Child &aChild)
{
    Mac::Address macAddr;

    if (aChild.mAddSrcMatchEntryShort)
    {
        macAddr.mLength = sizeof(macAddr.mShortAddress);
        macAddr.mShortAddress = aChild.mValid.mRloc16;
    }
    else
    {
        macAddr.mLength = sizeof(macAddr.mExtAddress);
        memcpy(macAddr.mExtAddress.m8, aChild.mMacAddr.m8, sizeof(macAddr.mExtAddress));
    }

    if (mNetif.GetMac().ClearSrcMatchEntry(macAddr) == kThreadError_None)
    {
        if (!mSrcMatchEnabled && (AddPendingSrcMatchEntries() == kThreadError_None))
        {
            mNetif.GetMac().EnableSrcMatch(true);
            mSrcMatchEnabled = true;
        }
    }
    else
    {
        // if finished queued messages for SED which is not added into the source match table
        aChild.mAddSrcMatchEntryPending = false;
    }
}

void MeshForwarder::SetSrcMatchAsShort(Child &aChild, bool aShortSource)
{
    VerifyOrExit(aChild.mAddSrcMatchEntryShort != aShortSource);

    if (aChild.mQueuedIndirectMessageCnt > 0)
    {
        ClearSrcMatchEntry(aChild);
        aChild.mAddSrcMatchEntryShort = aShortSource;
        AddSrcMatchEntry(aChild);
    }
    else
    {
        aChild.mAddSrcMatchEntryShort = aShortSource;
    }

exit:
    return;
}

ThreadError MeshForwarder::SendMessage(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    Neighbor *neighbor;

    uint8_t numChildren;
    Child *children;

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
                children = mNetif.GetMle().GetChildren(&numChildren);

                for (uint8_t i = 0; i < numChildren; i++)
                {
                    if (children[i].IsStateValidOrRestoring() && (children[i].mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0)
                    {
                        children[i].mQueuedIndirectMessageCnt++;
                        AddSrcMatchEntry(children[i]);
                        aMessage.SetChildMask(i);
                    }
                }
            }
        }
        else if ((neighbor = mNetif.GetMle().GetNeighbor(ip6Header.GetDestination())) != NULL &&
                 (neighbor->mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0 &&
                 !aMessage.GetDirectTransmission())
        {
            // destined for a sleepy child
            children = static_cast<Child *>(neighbor);
            children->mQueuedIndirectMessageCnt++;

            AddSrcMatchEntry(*children);
            aMessage.SetChildMask(mNetif.GetMle().GetChildIndex(*children));
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
            (neighbor->mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            // destined for a sleepy child
            children = static_cast<Child *>(neighbor);
            children->mQueuedIndirectMessageCnt++;

            AddSrcMatchEntry(*children);
            aMessage.SetChildMask(mNetif.GetMle().GetChildIndex(*children));
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
    }

    aMessage.SetOffset(0);
    aMessage.SetDatagramTag(0);
    SuccessOrExit(error = mSendQueue.Enqueue(aMessage));
    mScheduleTransmissionTask.Post();

exit:
    return error;
}

Message *MeshForwarder::GetDirectTransmission()
{
    Message *curMessage, *nextMessage;
    ThreadError error = kThreadError_None;

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
            break;

        case Message::kType6lowpan:
            error = UpdateMeshRoute(*curMessage);
            break;

        case Message::kTypeMacDataPoll:
            ExitNow();
        }

        switch (error)
        {
        case kThreadError_None:
            ExitNow();

        case kThreadError_AddressQuery:
            mSendQueue.Dequeue(*curMessage);
            mResolvingQueue.Enqueue(*curMessage);
            continue;

        case kThreadError_Drop:
        case kThreadError_NoBufs:
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
    uint8_t childIndex = mNetif.GetMle().GetChildIndex(aChild);

    for (message = mSendQueue.GetHead(); message; message = message->GetNext())
    {
        if (message->GetChildMask(childIndex))
        {
            break;
        }
    }

    aChild.mIndirectSendInfo.mMessage = message;
    aChild.mIndirectSendInfo.mFragmentOffset = 0;
    aChild.mIndirectSendInfo.mTxAttemptCounter = 0;

    return message;
}

void MeshForwarder::PrepareIndirectTransmission(Message &aMessage, const Child &aChild)
{
    if (aChild.mIndirectSendInfo.mTxAttemptCounter > 0)
    {
        mSendMessageIsARetransmission = true;
        mSendMessageFrameCounter = aChild.mIndirectSendInfo.mFrameCounter;
        mSendMessageKeyId = aChild.mIndirectSendInfo.mKeyId;
        mSendMessageDataSequenceNumber = aChild.mIndirectSendInfo.mDataSequenceNumber;
    }

    aMessage.SetOffset(aChild.mIndirectSendInfo.mFragmentOffset);

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
            if (aChild.mAddSrcMatchEntryShort)
            {
                mMacDest.mLength = sizeof(mMacDest.mShortAddress);
                mMacDest.mShortAddress = aChild.mValid.mRloc16;
            }
            else
            {
                mMacDest.mLength = sizeof(mMacDest.mExtAddress);
                memcpy(mMacDest.mExtAddress.m8, aChild.mMacAddr.m8, sizeof(mMacDest.mExtAddress));
            }
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

    default:
        assert(false);
        break;
    }
}

ThreadError MeshForwarder::UpdateMeshRoute(Message &aMessage)
{
    ThreadError error = kThreadError_None;
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
        ExitNow(error = kThreadError_Drop);
    }

    mMacDest.mLength = sizeof(mMacDest.mShortAddress);
    mMacDest.mShortAddress = neighbor->mValid.mRloc16;
    mMacSource.mLength = sizeof(mMacSource.mShortAddress);
    mMacSource.mShortAddress = mNetif.GetMac().GetShortAddress();

    mAddMeshHeader = true;
    mMeshDest = meshHeader.GetDestination();
    mMeshSource = meshHeader.GetSource();

exit:
    return error;
}

ThreadError MeshForwarder::UpdateIp6Route(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    Ip6::Header ip6Header;
    uint16_t rloc16;
    uint16_t aloc16;
    Neighbor *neighbor;

    mAddMeshHeader = false;

    aMessage.Read(0, sizeof(ip6Header), &ip6Header);

    switch (mNetif.GetMle().GetDeviceState())
    {
    case Mle::kDeviceStateDisabled:
    case Mle::kDeviceStateDetached:
        if (ip6Header.GetDestination().IsLinkLocal() || ip6Header.GetDestination().IsLinkLocalMulticast())
        {
            GetMacDestinationAddress(ip6Header.GetDestination(), mMacDest);
            GetMacSourceAddress(ip6Header.GetSource(), mMacSource);
        }
        else
        {
            ExitNow(error = kThreadError_Drop);
        }

        break;

    case Mle::kDeviceStateChild:
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
            ExitNow(error = kThreadError_Drop);
        }

        break;

    case Mle::kDeviceStateRouter:
    case Mle::kDeviceStateLeader:
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
                             error = kThreadError_Drop);
                mMeshDest = rloc16;
            }
            else if (mNetif.GetMle().IsAnycastLocator(ip6Header.GetDestination()))
            {
                // only support Leader ALOC for now
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
                                      agentRloc16) == kThreadError_None),
                                 error = kThreadError_Drop);

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
                    // TODO: support ALOC for DHCPv6 Agent, Service, Commissioner, Neighbor Discovery Agent
                    ExitNow(error = kThreadError_Drop);
                }
            }
            else if ((neighbor = mNetif.GetMle().GetNeighbor(ip6Header.GetDestination())) != NULL)
            {
                mMeshDest = neighbor->mValid.mRloc16;
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

            VerifyOrExit(mMeshDest != Mac::kShortAddrInvalid, error = kThreadError_Drop);

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

exit:
    return error;
}

void MeshForwarder::SetRxOff(void)
{
    mNetif.GetMac().SetRxOnWhenIdle(false);
    mDataPollManager.StopPolling();
}

bool MeshForwarder::GetRxOnWhenIdle()
{
    return mNetif.GetMac().GetRxOnWhenIdle();
}

void MeshForwarder::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    mNetif.GetMac().SetRxOnWhenIdle(aRxOnWhenIdle);

    if (aRxOnWhenIdle)
    {
        mDataPollManager.StopPolling();
    }
    else
    {
        mDataPollManager.StartPolling();
    }
}

ThreadError MeshForwarder::GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    aMacAddr.mLength = sizeof(aMacAddr.mExtAddress);
    aMacAddr.mExtAddress.Set(aIp6Addr);

    if (memcmp(&aMacAddr.mExtAddress, mNetif.GetMac().GetExtAddress(), sizeof(aMacAddr.mExtAddress)) != 0)
    {
        aMacAddr.mLength = sizeof(aMacAddr.mShortAddress);
        aMacAddr.mShortAddress = mNetif.GetMac().GetShortAddress();
    }

    return kThreadError_None;
}

ThreadError MeshForwarder::GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
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

    return kThreadError_None;
}

ThreadError MeshForwarder::HandleFrameRequest(void *aContext, Mac::Frame &aFrame)
{
    return static_cast<MeshForwarder *>(aContext)->HandleFrameRequest(aFrame);
}

ThreadError MeshForwarder::HandleFrameRequest(Mac::Frame &aFrame)
{
    ThreadError error = kThreadError_None;
    Mac::Address macDest;
    Child *child = NULL;

    VerifyOrExit(mEnabled, error = kThreadError_Abort);

    mSendBusy = true;

    if (mSendMessage == NULL)
    {
        SendEmptyFrame(aFrame);
        aFrame.SetIsARetransmission(false);
        aFrame.SetMaxTxAttempts(Mac::kDirectFrameMacTxAttempts);
        ExitNow();
    }

    switch (mSendMessage->GetType())
    {
    case Message::kTypeIp6:
        if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
        {
            if (!mScanning)
            {
                mScanChannel = kPhyMinChannel;
                mRestoreChannel = mNetif.GetMac().GetChannel();
                mRestorePanId = mNetif.GetMac().GetPanId();
                mScanning = true;
            }

            while ((mScanChannels & 1) == 0)
            {
                mScanChannels >>= 1;
                mScanChannel++;
            }

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
        if (error == kThreadError_NotCapable)
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
    }

    assert(error == kThreadError_None);

    // set FramePending if there are more queued messages for the child
    aFrame.GetDstAddr(macDest);

    if (((child = mNetif.GetMle().GetChild(macDest)) != NULL)
        && ((child->mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0)
        && (child->mQueuedIndirectMessageCnt > 1))
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

ThreadError MeshForwarder::SendPoll(Message &aMessage, Mac::Frame &aFrame)
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
        aFrame.SetDstAddr(neighbor->mValid.mRloc16);
        aFrame.SetSrcAddr(macSource.mShortAddress);
    }
    else
    {
        aFrame.SetDstAddr(neighbor->mMacAddr);
        aFrame.SetSrcAddr(macSource.mExtAddress);
    }

    aFrame.SetCommandId(Mac::Frame::kMacCmdDataRequest);

    mMessageNextOffset = aMessage.GetLength();

    return kThreadError_None;
}

ThreadError MeshForwarder::SendMesh(Message &aMessage, Mac::Frame &aFrame)
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

    return kThreadError_None;
}

ThreadError MeshForwarder::SendFragment(Message &aMessage, Mac::Frame &aFrame)
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
    ThreadError error = kThreadError_None;

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
        if (mNetif.GetMle().GetDeviceState() == Mle::kDeviceStateChild)
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
                ExitNow(error = kThreadError_NotCapable);
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

ThreadError MeshForwarder::SendEmptyFrame(Mac::Frame &aFrame)
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

    // Not requesting acknowledgment for null/empty frame.

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

    return kThreadError_None;
}

void MeshForwarder::HandleSentFrame(void *aContext, Mac::Frame &aFrame, ThreadError aError)
{
    static_cast<MeshForwarder *>(aContext)->HandleSentFrame(aFrame, aError);
}

void MeshForwarder::HandleSentFrame(Mac::Frame &aFrame, ThreadError aError)
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
        case kThreadError_None:
            if (aFrame.GetAckRequest())
            {
                neighbor->mLinkFailures = 0;
            }

            break;

        case kThreadError_ChannelAccessFailure:
        case kThreadError_Abort:
            break;

        case kThreadError_NoAck:
            neighbor->mLinkFailures++;

            if (mNetif.GetMle().IsActiveRouter(neighbor->mValid.mRloc16))
            {
                if (neighbor->mLinkFailures >= Mle::kFailedRouterTransmissions)
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
        child->mDataRequest = false;

        VerifyOrExit(mSendMessage != NULL);

        if (mSendMessage == child->mIndirectSendInfo.mMessage)
        {
            switch (aError)
            {
            case kThreadError_None:
                child->mIndirectSendInfo.mTxAttemptCounter = 0;
                break;

            default:
                child->mIndirectSendInfo.mTxAttemptCounter++;

                if (child->mIndirectSendInfo.mTxAttemptCounter < kMaxPollTriggeredTxAttempts)
                {
                    // We save the frame counter, key id, and data sequence number of
                    // current frame so we use the same values for the retransmission
                    // of the frame following the receipt of a data request command (data
                    // poll) from the sleepy child.

                    if (aFrame.GetSecurityEnabled())
                    {
                        aFrame.GetFrameCounter(child->mIndirectSendInfo.mFrameCounter);
                        aFrame.GetKeyId(child->mIndirectSendInfo.mKeyId);
                        child->mIndirectSendInfo.mDataSequenceNumber = aFrame.GetSequence();
                    }

                    ExitNow();
                }

                child->mIndirectSendInfo.mTxAttemptCounter = 0;

                // We set the NextOffset to end of message, since there is no need to
                // send any remaining fragments in the message to the child, if all tx
                // attempts of current frame already failed.

                mMessageNextOffset = mSendMessage->GetLength();

                break;
            }
        }

        if (mMessageNextOffset < mSendMessage->GetLength())
        {
            if (mSendMessage == child->mIndirectSendInfo.mMessage)
            {
                child->mIndirectSendInfo.mFragmentOffset = mMessageNextOffset;
            }
        }
        else
        {
            if (mSendMessage == child->mIndirectSendInfo.mMessage)
            {
                child->mIndirectSendInfo.mFragmentOffset = 0;
                child->mIndirectSendInfo.mMessage = NULL;

                // add short address for subsequent indirect messages after
                // one indirect message to valid sleepy devices is sent out successfully
                if (aError == kThreadError_None)
                {
                    SetSrcMatchAsShort(*child, true);
                }
            }

            childIndex = mNetif.GetMle().GetChildIndex(*child);

            if (mSendMessage->GetChildMask(childIndex))
            {
                mSendMessage->ClearChildMask(childIndex);

                child->mQueuedIndirectMessageCnt--;
                otLogDebgMac(GetInstance(), "Sent to child (0x%x), still queued message (%d)",
                             child->mValid.mRloc16, child->mQueuedIndirectMessageCnt);

                if (child->mQueuedIndirectMessageCnt == 0)
                {
                    ClearSrcMatchEntry(*child);
                }
            }
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

        if (neighbor->mState == Neighbor::kStateInvalid)
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
        LogIp6Message(kMessageTransmit, *mSendMessage, macDest, aError);
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

        if (mScanChannel > kPhyMaxChannel)
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
    ThreadError error = kThreadError_None;

    if (!mEnabled)
    {
        ExitNow(error = kThreadError_InvalidState);
    }

    SuccessOrExit(error = aFrame.GetSrcAddr(macSource));
    SuccessOrExit(aFrame.GetDstAddr(macDest));

    aFrame.GetSrcPanId(messageInfo.mPanId);
    messageInfo.mChannel = aFrame.GetChannel();
    messageInfo.mRss = aFrame.GetPower();
    messageInfo.mLqi = aFrame.GetLqi();
    messageInfo.mLinkSecurity = aFrame.GetSecurityEnabled();

    payload = aFrame.GetPayload();
    payloadLength = aFrame.GetPayloadLength();

    mDataPollManager.HandleReceivedFrame(aFrame);

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
        else if (payloadLength >= 1 &&
                 Lowpan::Lowpan::IsLowpanHc(payload))
        {
            HandleLowpanHC(payload, payloadLength, macSource, macDest, messageInfo);
        }

        break;

    case Mac::Frame::kFcfFrameMacCmd:
        aFrame.GetCommandId(commandId);

        if (commandId == Mac::Frame::kMacCmdDataRequest)
        {
            HandleDataRequest(macSource, messageInfo);
        }

        break;
    }

exit:

    if (error != kThreadError_None)
    {
        otLogDebgMacErr(GetInstance(), error, "Dropping received frame");
    }
}

void MeshForwarder::HandleMesh(uint8_t *aFrame, uint8_t aFrameLength, const Mac::Address &aMacSource,
                               const ThreadMessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    Mac::Address meshDest;
    Mac::Address meshSource;
    Lowpan::MeshHeader meshHeader;

    // Check the mesh header
    VerifyOrExit(meshHeader.Init(aFrame, aFrameLength) == kThreadError_None, error = kThreadError_Drop);

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aMessageInfo.mLinkSecurity && meshHeader.IsValid(), error = kThreadError_Security);

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
            ExitNow(error = kThreadError_Parse);
        }
    }
    else if (meshHeader.GetHopsLeft() > 0)
    {
        mNetif.GetMle().ResolveRoutingLoops(aMacSource.mShortAddress, meshDest.mShortAddress);

        SuccessOrExit(error = CheckReachability(aFrame, aFrameLength, meshSource, meshDest));

        meshHeader.SetHopsLeft(meshHeader.GetHopsLeft() - 1);
        meshHeader.AppendTo(aFrame);

        VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kType6lowpan, 0)) != NULL,
                     error = kThreadError_NoBufs);
        SuccessOrExit(error = message->SetLength(aFrameLength));
        message->Write(0, aFrameLength, aFrame);
        message->SetLinkSecurityEnabled(aMessageInfo.mLinkSecurity);
        message->SetPanId(aMessageInfo.mPanId);

        SendMessage(*message);
    }

exit:

    if (error != kThreadError_None)
    {
        otLogDebgMacErr(GetInstance(), error, "Dropping received mesh frame");

        if (message != NULL)
        {
            message->Free();
        }
    }
}

ThreadError MeshForwarder::CheckReachability(uint8_t *aFrame, uint8_t aFrameLength,
                                             const Mac::Address &aMeshSource, const Mac::Address &aMeshDest)
{
    ThreadError error = kThreadError_None;
    Ip6::Header ip6Header;
    Lowpan::MeshHeader meshHeader;

    VerifyOrExit(meshHeader.Init(aFrame, aFrameLength) == kThreadError_None, error = kThreadError_Drop);

    // skip mesh header
    aFrame += meshHeader.GetHeaderLength();
    aFrameLength -= meshHeader.GetHeaderLength();

    // skip fragment header
    if (aFrameLength >= 1 &&
        reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
    {
        VerifyOrExit(sizeof(Lowpan::FragmentHeader) <= aFrameLength, error = kThreadError_Drop);
        VerifyOrExit(reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetDatagramOffset() == 0);

        aFrame += reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
        aFrameLength -= reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
    }

    // only process IPv6 packets
    VerifyOrExit(aFrameLength >= 1 && Lowpan::Lowpan::IsLowpanHc(aFrame));

    VerifyOrExit(mNetif.GetLowpan().DecompressBaseHeader(ip6Header, aMeshSource, aMeshDest, aFrame, aFrameLength) > 0,
                 error = kThreadError_Drop);

    error = mNetif.GetMle().CheckReachability(aMeshSource.mShortAddress, aMeshDest.mShortAddress, ip6Header);

exit:
    return error;
}

void MeshForwarder::HandleFragment(uint8_t *aFrame, uint8_t aFrameLength,
                                   const Mac::Address &aMacSource, const Mac::Address &aMacDest,
                                   const ThreadMessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
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
                     error = kThreadError_NoBufs);
        message->SetLinkSecurityEnabled(aMessageInfo.mLinkSecurity);
        message->SetPanId(aMessageInfo.mPanId);
        headerLength = mNetif.GetLowpan().Decompress(*message, aMacSource, aMacDest, aFrame, aFrameLength,
                                                     datagramLength);
        VerifyOrExit(headerLength > 0, error = kThreadError_Parse);

        aFrame += headerLength;
        aFrameLength -= static_cast<uint8_t>(headerLength);

        VerifyOrExit(datagramLength >= message->GetOffset() + aFrameLength, error = kThreadError_Parse);

        SuccessOrExit(error = message->SetLength(datagramLength));

        message->SetDatagramTag(datagramTag);
        message->SetTimeout(kReassemblyTimeout);

        // copy Fragment
        message->Write(message->GetOffset(), aFrameLength, aFrame);
        message->MoveOffset(aFrameLength);

        // Security Check
        VerifyOrExit(mNetif.GetIp6Filter().Accept(*message), error = kThreadError_Drop);

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

        VerifyOrExit(message != NULL, error = kThreadError_Drop);

        // copy Fragment
        message->Write(message->GetOffset(), aFrameLength, aFrame);
        message->MoveOffset(aFrameLength);
    }

exit:

    if (error == kThreadError_None)
    {
        if (message->GetOffset() >= message->GetLength())
        {
            mReassemblyList.Dequeue(*message);
            HandleDatagram(*message, aMessageInfo, aMacSource);
        }
    }
    else
    {
        otLogDebgMacErr(GetInstance(), error, "Dropping received fragment");

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
    ThreadError error = kThreadError_None;
    Message *message;
    int headerLength;

    VerifyOrExit((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeIp6, 0)) != NULL,
                 error = kThreadError_NoBufs);
    message->SetLinkSecurityEnabled(aMessageInfo.mLinkSecurity);
    message->SetPanId(aMessageInfo.mPanId);

    headerLength = mNetif.GetLowpan().Decompress(*message, aMacSource, aMacDest, aFrame, aFrameLength, 0);
    VerifyOrExit(headerLength > 0, error = kThreadError_Parse);

    aFrame += headerLength;
    aFrameLength -= static_cast<uint8_t>(headerLength);

    SuccessOrExit(error = message->SetLength(message->GetLength() + aFrameLength));
    message->Write(message->GetOffset(), aFrameLength, aFrame);

    // Security Check
    VerifyOrExit(mNetif.GetIp6Filter().Accept(*message), error = kThreadError_Drop);

exit:

    if (error == kThreadError_None)
    {
        HandleDatagram(*message, aMessageInfo, aMacSource);
    }
    else
    {
        otLogDebgMacErr(GetInstance(), error, "Dropping received lowpan HC");

        if (message != NULL)
        {
            message->Free();
        }
    }
}

ThreadError MeshForwarder::HandleDatagram(Message &aMessage, const ThreadMessageInfo &aMessageInfo,
                                          const Mac::Address &aMacSource)
{
    LogIp6Message(kMessageReceive, aMessage, aMacSource, kThreadError_None);

    return mNetif.GetIp6().HandleDatagram(aMessage, &mNetif, mNetif.GetInterfaceId(), &aMessageInfo, false);
}

void MeshForwarder::HandleDataRequest(const Mac::Address &aMacSource, const ThreadMessageInfo &aMessageInfo)
{
    Child *child;

    // Security Check: only process secure Data Poll frames.
    VerifyOrExit(aMessageInfo.mLinkSecurity);

    VerifyOrExit(mNetif.GetMle().GetDeviceState() != Mle::kDeviceStateDetached);

    VerifyOrExit((child = mNetif.GetMle().GetChild(aMacSource)) != NULL);
    child->mLastHeard = Timer::GetNow();
    child->mLinkFailures = 0;

    if (!mSrcMatchEnabled || child->mQueuedIndirectMessageCnt > 0)
    {
        child->mDataRequest = true;
    }

    mScheduleTransmissionTask.Post();

exit:
    return;
}

void MeshForwarder::HandleDataPollTimeout(void *aContext)
{
    static_cast<MeshForwarder *>(aContext)->GetDataPollManager().HandlePollTimeout();
}

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void MeshForwarder::LogIp6Message(MessageAction aAction, const Message &aMessage, const Mac::Address &aMacAddress,
                                  ThreadError aError)
{
    uint16_t checksum = 0;
    Ip6::Header ip6Header;
    Ip6::IpProto protocol;
    char stringBuffer[Ip6::Address::kIp6AddressStringSize];

    VerifyOrExit(aMessage.GetType() == Message::kTypeIp6);

    VerifyOrExit(sizeof(ip6Header) == aMessage.Read(0, sizeof(ip6Header), &ip6Header));
    VerifyOrExit(ip6Header.IsVersion6());

    protocol = ip6Header.GetNextHeader();

    switch (protocol)
    {
    case Ip6::kProtoUdp:
    {
        Ip6::UdpHeader udpHeader;

        VerifyOrExit(sizeof(udpHeader) == aMessage.Read(sizeof(ip6Header), sizeof(udpHeader), &udpHeader));
        checksum = udpHeader.GetChecksum();
        break;
    }

    case Ip6::kProtoTcp:
    {
        Ip6::TcpHeader tcpHeader;

        VerifyOrExit(sizeof(tcpHeader) == aMessage.Read(sizeof(ip6Header), sizeof(tcpHeader), &tcpHeader));
        checksum = tcpHeader.GetChecksum();
        break;
    }

    default:
        break;
    }

    otLogInfoMac(
        GetInstance(),
        "%s IPv6 %s msg, len:%d, chksum:%04x, %s:%s, sec:%s%s%s",
        (aAction == kMessageReceive) ? "Rx" : ((aError == kThreadError_None) ? "Tx" : "Failed to Tx"),
        Ip6::Ip6::IpProtoToString(protocol),
        aMessage.GetLength(),
        checksum,
        (aAction == kMessageReceive) ? "from" : "to",
        aMacAddress.ToString(stringBuffer, sizeof(stringBuffer)),
        aMessage.IsLinkSecurityEnabled() ? "yes" : "no",
        (aError == kThreadError_None) ? "" : ", error:",
        (aError == kThreadError_None) ? "" : otThreadErrorToString(aError)
    );

    otLogInfoMac(GetInstance(), "src: %s", ip6Header.GetSource().ToString(stringBuffer, sizeof(stringBuffer)));
    otLogInfoMac(GetInstance(), "dst: %s", ip6Header.GetDestination().ToString(stringBuffer, sizeof(stringBuffer)));

exit:
    return;
}

#else // #if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_INFO

void MeshForwarder::LogIp6Message(MessageAction, const Message &, const Mac::Address &, ThreadError)
{
}

#endif //#if OPENTHREAD_CONFIG_LOG_LEVEL >= OPENTHREAD_LOG_LEVEL_INFO

}  // namespace Thread
