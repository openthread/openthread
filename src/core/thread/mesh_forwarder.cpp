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

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/encoding.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <net/ip6_filter.hpp>
#include <net/udp6.hpp>
#include <net/netif.hpp>
#include <net/udp6.hpp>
#include <platform/random.h>
#include <thread/mesh_forwarder.hpp>
#include <thread/mle_router.hpp>
#include <thread/mle.hpp>
#include <thread/thread_netif.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {

MeshForwarder::MeshForwarder(ThreadNetif &aThreadNetif):
    mMacReceiver(&MeshForwarder::HandleReceivedFrame, this),
    mMacSender(&MeshForwarder::HandleFrameRequest, &MeshForwarder::HandleSentFrame, this),
    mDiscoverTimer(aThreadNetif.GetIp6().mTimerScheduler, &MeshForwarder::HandleDiscoverTimer, this),
    mPollTimer(aThreadNetif.GetIp6().mTimerScheduler, &MeshForwarder::HandlePollTimer, this),
    mReassemblyTimer(aThreadNetif.GetIp6().mTimerScheduler, &MeshForwarder::HandleReassemblyTimer, this),
    mMessageNextOffset(0),
    mPollPeriod(0),
    mAssignPollPeriod(0),
    mSendMessage(NULL),
    mMeshSource(Mac::kShortAddrInvalid),
    mMeshDest(Mac::kShortAddrInvalid),
    mAddMeshHeader(false),
    mSendBusy(false),
    mScheduleTransmissionTask(aThreadNetif.GetIp6().mTaskletScheduler, ScheduleTransmissionTask, this),
    mEnabled(false),
    mScanChannels(0),
    mScanDuration(0),
    mScanChannel(0),
    mRestoreChannel(0),
    mScanning(false),
    mNetif(aThreadNetif),
    mAddressResolver(aThreadNetif.GetAddressResolver()),
    mLowpan(aThreadNetif.GetLowpan()),
    mMac(aThreadNetif.GetMac()),
    mMle(aThreadNetif.GetMle()),
    mNetworkData(aThreadNetif.GetNetworkDataLeader()),
    mSrcMatchEnabled(false)
{
    mFragTag = static_cast<uint16_t>(otPlatRandomGet());
    mMac.RegisterReceiver(mMacReceiver);
    mMacSource.mLength = 0;
    mMacDest.mLength = 0;
}

ThreadError MeshForwarder::Start(void)
{
    ThreadError error = kThreadError_None;

    if (mEnabled == false)
    {
        mMac.SetRxOnWhenIdle(true);
        mEnabled = true;
    }

    return error;
}

ThreadError MeshForwarder::Stop(void)
{
    ThreadError error = kThreadError_None;
    Message *message;

    VerifyOrExit(mEnabled == true,);

    mPollTimer.Stop();
    mReassemblyTimer.Stop();

    if (mScanning)
    {
        mMac.SetChannel(mRestoreChannel);
        mScanning = false;
        mMle.HandleDiscoverComplete();
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
    mMac.SetRxOnWhenIdle(false);

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

void MeshForwarder::ScheduleTransmissionTask(void *aContext)
{
    static_cast<MeshForwarder *>(aContext)->ScheduleTransmissionTask();
}

void MeshForwarder::UpdateIndirectMessages(void)
{
    Child *children;
    uint8_t numChildren;

    children = mMle.GetChildren(&numChildren);

    for (uint8_t i = 0; i < numChildren; i++)
    {
        Child *child = &children[i];
        Message *nextMessage;

        if (child->mState == Child::kStateValid || child->mQueuedIndirectMessageCnt == 0)
        {
            continue;
        }

        for (Message *message = mSendQueue.GetHead(); message; message = nextMessage)
        {
            nextMessage = message->GetNext();

            message->ClearChildMask(i);

            if (!message->IsChildPending())
            {
                if (mSendMessage == message)
                {
                    mSendMessage = NULL;
                }

                mSendQueue.Dequeue(*message);
                message->Free();
            }
        }

        child->mQueuedIndirectMessageCnt = 0;
        ClearSrcMatchEntry(*child);
    }
}

void MeshForwarder::ScheduleTransmissionTask()
{
    ThreadError error = kThreadError_None;
    uint8_t numChildren;
    Child *children;

    VerifyOrExit(mSendBusy == false, error = kThreadError_Busy);

    UpdateIndirectMessages();

    children = mMle.GetChildren(&numChildren);

    for (int i = 0; i < numChildren; i++)
    {
        if (children[i].mState == Child::kStateValid &&
            children[i].mDataRequest &&
            (mSendMessage = GetIndirectTransmission(children[i])) != NULL)
        {
            mSendMessage->SetOffset(children[i].mFragmentOffset);
            mMac.SendFrameRequest(mMacSender);
            ExitNow();
        }
    }

    if ((mSendMessage = GetDirectTransmission()) != NULL)
    {
        mMac.SendFrameRequest(mMacSender);
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

    children = mMle.GetChildren(&numChildren);

    // Add pending short address first
    for (uint8_t i = 0; i < numChildren; i++)
    {
        if (children[i].mState == Child::kStateValid &&
            children[i].mAddSrcMatchEntryPending &&
            children[i].mAddSrcMatchEntryShort)
        {
            VerifyOrExit(((error = AddSrcMatchEntry(children[i])) == kThreadError_None), ;);
        }
    }

    // Add pending extended address
    for (uint8_t i = 0; i < numChildren; i++)
    {
        if (children[i].mState == Child::kStateValid &&
            children[i].mAddSrcMatchEntryPending &&
            !children[i].mAddSrcMatchEntryShort)
        {
            VerifyOrExit(((error = AddSrcMatchEntry(children[i])) == kThreadError_None), ;);
        }
    }

exit:
    return error;
}

ThreadError MeshForwarder::AddSrcMatchEntry(Child &aChild)
{
    ThreadError error = kThreadError_NoBufs;
    Mac::Address macAddr;

    otLogDebgMac("Queuing for child (0x%x)", aChild.mValid.mRloc16);
    otLogDebgMac("SrcMatch %d (0:Dis, 1:En))", mSrcMatchEnabled);

    // first queued message, to be added into source match table
    if (aChild.mQueuedIndirectMessageCnt == 1)
    {
        aChild.mAddSrcMatchEntryPending = true;
    }

    VerifyOrExit(aChild.mAddSrcMatchEntryPending, ;);

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

    if ((error = mMac.AddSrcMatchEntry(macAddr)) == kThreadError_None)
    {
        // succeed in adding to source match table
        aChild.mAddSrcMatchEntryPending = false;

        if (!mSrcMatchEnabled)
        {
            mMac.EnableSrcMatch(true);
            mSrcMatchEnabled = true;
        }
    }
    else
    {
        if (mSrcMatchEnabled)
        {
            mMac.EnableSrcMatch(false);
            mSrcMatchEnabled = false;
        }
    }

exit:
    return error;
}

void MeshForwarder::ClearSrcMatchEntry(Child &aChild)
{
    Mac::Address macAddr;
    otLogDebgMac("SrcMatch %d (0:Dis, 1:En))", mSrcMatchEnabled);

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

    if (mMac.ClearSrcMatchEntry(macAddr) == kThreadError_None)
    {
        if (!mSrcMatchEnabled && (AddPendingSrcMatchEntries() == kThreadError_None))
        {
            mMac.EnableSrcMatch(true);
            mSrcMatchEnabled = true;
        }
    }
    else
    {
        // if finished queued messages for SED which is not added into the source match table
        aChild.mAddSrcMatchEntryPending = false;
    }
}

ThreadError MeshForwarder::SendMessage(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    Neighbor *neighbor;
    Ip6::Header ip6Header;
    uint8_t numChildren;
    Child *children;

    switch (aMessage.GetType())
    {
    case Message::kTypeIp6:
        aMessage.Read(0, sizeof(ip6Header), &ip6Header);

        if (!memcmp(&ip6Header.GetDestination(), mMle.GetLinkLocalAllThreadNodesAddress(),
                    sizeof(ip6Header.GetDestination())) ||
            !memcmp(&ip6Header.GetDestination(), mMle.GetRealmLocalAllThreadNodesAddress(), sizeof(ip6Header.GetDestination())))
        {
            // schedule direct transmission
            aMessage.SetDirectTransmission();

            // destined for all sleepy children
            children = mMle.GetChildren(&numChildren);

            for (uint8_t i = 0; i < numChildren; i++)
            {
                if (children[i].mState == Neighbor::kStateValid && (children[i].mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0)
                {
                    children[i].mQueuedIndirectMessageCnt++;
                    AddSrcMatchEntry(children[i]);
                    aMessage.SetChildMask(i);
                }
            }
        }
        else if ((neighbor = mMle.GetNeighbor(ip6Header.GetDestination())) != NULL &&
                 (neighbor->mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            // destined for a sleepy child
            children = static_cast<Child *>(neighbor);
            children->mQueuedIndirectMessageCnt++;

            AddSrcMatchEntry(*children);
            aMessage.SetChildMask(mMle.GetChildIndex(*children));
        }
        else
        {
            // schedule direct transmission
            aMessage.SetDirectTransmission();
        }

        break;

    case Message::kType6lowpan:
    {
        Lowpan::MeshHeader meshHeader(aMessage);

        if ((neighbor = mMle.GetNeighbor(meshHeader.GetDestination())) != NULL &&
            (neighbor->mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0)
        {
            // destined for a sleepy child
            children = static_cast<Child *>(neighbor);
            children->mQueuedIndirectMessageCnt++;

            AddSrcMatchEntry(*children);
            aMessage.SetChildMask(mMle.GetChildIndex(*children));
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

Message *MeshForwarder::GetIndirectTransmission(const Child &aChild)
{
    Message *message = NULL;
    uint8_t childIndex = mMle.GetChildIndex(aChild);
    Ip6::Header ip6Header;

    for (message = mSendQueue.GetHead(); message; message = message->GetNext())
    {
        if (message->GetChildMask(childIndex))
        {
            break;
        }
    }

    VerifyOrExit(message != NULL, ;);

    switch (message->GetType())
    {
    case Message::kTypeIp6:
        message->Read(0, sizeof(ip6Header), &ip6Header);

        mAddMeshHeader = false;
        GetMacSourceAddress(ip6Header.GetSource(), mMacSource);

        if (ip6Header.GetDestination().IsLinkLocal())
        {
            GetMacDestinationAddress(ip6Header.GetDestination(), mMacDest);
        }
        else
        {
            mMacDest.mLength = sizeof(mMacDest.mShortAddress);
            mMacDest.mShortAddress = aChild.mValid.mRloc16;
        }

        break;

    case Message::kType6lowpan:
    {
        Lowpan::MeshHeader meshHeader(*message);

        mAddMeshHeader = true;
        mMeshDest = meshHeader.GetDestination();
        mMeshSource = meshHeader.GetSource();
        mMacSource.mLength = sizeof(mMacSource.mShortAddress);
        mMacSource.mShortAddress = mMac.GetShortAddress();
        mMacDest.mLength = sizeof(mMacDest.mShortAddress);
        mMacDest.mShortAddress = meshHeader.GetDestination();
        break;
    }

    default:
        assert(false);
        break;
    }

exit:
    return message;
}

ThreadError MeshForwarder::UpdateMeshRoute(Message &aMessage)
{
    ThreadError error = kThreadError_None;
    Lowpan::MeshHeader meshHeader(aMessage);
    Neighbor *neighbor;
    uint16_t nextHop;

    nextHop = mMle.GetNextHop(meshHeader.GetDestination());

    if (nextHop != Mac::kShortAddrInvalid)
    {
        neighbor = mMle.GetNeighbor(nextHop);
    }
    else
    {
        neighbor = mMle.GetNeighbor(meshHeader.GetDestination());
    }

    if (neighbor == NULL)
    {
        ExitNow(error = kThreadError_Drop);
    }

    mMacDest.mLength = sizeof(mMacDest.mShortAddress);
    mMacDest.mShortAddress = neighbor->mValid.mRloc16;
    mMacSource.mLength = sizeof(mMacSource.mShortAddress);
    mMacSource.mShortAddress = mMac.GetShortAddress();

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

    switch (mMle.GetDeviceState())
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
                mMacDest.mShortAddress = mMle.GetNextHop(Mac::kShortAddrBroadcast);
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
            if (mMle.IsRoutingLocator(ip6Header.GetDestination()))
            {
                rloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);
                VerifyOrExit(mMle.IsRouterIdValid(mMle.GetRouterId(rloc16)), error = kThreadError_Drop);
                mMeshDest = rloc16;
            }
            else if (mMle.IsAnycastLocator(ip6Header.GetDestination()))
            {
                // only support Leader ALOC for now
                aloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);

                if (aloc16 == Mle::kAloc16Leader)
                {
                    mMeshDest = mMle.GetRloc16(mMle.GetLeaderId());
                }

#if OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
                else if ((aloc16 & Mle::kAloc16DhcpAgentMask) != 0)
                {
                    uint16_t agentRloc16;
                    uint8_t routerId;
                    VerifyOrExit((mNetworkData.GetRlocByContextId(static_cast<uint8_t>(aloc16 & Mle::kAloc16DhcpAgentMask),
                                                                  agentRloc16) == kThreadError_None), error = kThreadError_Drop);

                    routerId = mMle.GetRouterId(agentRloc16);

                    // if agent is active router or the child of the device
                    if ((mMle.IsActiveRouter(agentRloc16)) || (mMle.GetRloc16(routerId) == mMle.GetRloc16()))
                    {
                        mMeshDest = agentRloc16;
                    }
                    else
                    {
                        // use the parent of the ED Agent as Dest
                        mMeshDest = mMle.GetRloc16(routerId);
                    }
                }

#endif  // OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
                else
                {
                    // TODO: support ALOC for DHCPv6 Agent, Service, Commissioner, Neighbor Discovery Agent
                    ExitNow(error = kThreadError_Drop);
                }
            }
            else if ((neighbor = mMle.GetNeighbor(ip6Header.GetDestination())) != NULL)
            {
                mMeshDest = neighbor->mValid.mRloc16;
            }
            else if (mNetworkData.IsOnMesh(ip6Header.GetDestination()))
            {
                SuccessOrExit(error = mAddressResolver.Resolve(ip6Header.GetDestination(), mMeshDest));
            }
            else
            {
                mNetworkData.RouteLookup(ip6Header.GetSource(), ip6Header.GetDestination(), NULL, &mMeshDest);
            }

            VerifyOrExit(mMeshDest != Mac::kShortAddrInvalid, error = kThreadError_Drop);

            if (mMle.GetNeighbor(mMeshDest) != NULL)
            {
                // destination is neighbor
                mMacDest.mLength = sizeof(mMacDest.mShortAddress);
                mMacDest.mShortAddress = mMeshDest;
                GetMacSourceAddress(ip6Header.GetSource(), mMacSource);
            }
            else
            {
                // destination is not neighbor
                mMeshSource = mMac.GetShortAddress();

                SuccessOrExit(error = mMle.CheckReachability(mMeshSource, mMeshDest, ip6Header));

                mMacDest.mLength = sizeof(mMacDest.mShortAddress);
                mMacDest.mShortAddress = mMle.GetNextHop(mMeshDest);
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

bool MeshForwarder::GetRxOnWhenIdle()
{
    return mMac.GetRxOnWhenIdle();
}

void MeshForwarder::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    mMac.SetRxOnWhenIdle(aRxOnWhenIdle);

    if (aRxOnWhenIdle)
    {
        mPollTimer.Stop();
    }
    else
    {
        mPollTimer.Start(mPollPeriod);
    }
}

void MeshForwarder::SetAssignPollPeriod(uint32_t aPeriod)
{
    mAssignPollPeriod = aPeriod;

    if (mPollTimer.IsRunning() && ((mMle.GetDeviceMode() & Mle::ModeTlv::kModeFFD) == 0))
    {
        SetPollPeriod(mAssignPollPeriod);
    }
}

uint32_t MeshForwarder::GetAssignPollPeriod()
{
    return mAssignPollPeriod;
}

void MeshForwarder::SetPollPeriod(uint32_t aPeriod)
{
    if (mPollPeriod != aPeriod)
    {
        if (mAssignPollPeriod != 0 && aPeriod != (OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD))
        {
            mPollPeriod = mAssignPollPeriod;
        }
        else
        {
            mPollPeriod = aPeriod;
        }

        mPollTimer.Start(mPollPeriod);
    }
}

uint32_t MeshForwarder::GetPollPeriod()
{
    return mPollPeriod;
}

void MeshForwarder::HandlePollTimer(void *aContext)
{
    static_cast<MeshForwarder *>(aContext)->HandlePollTimer();
}

void MeshForwarder::HandlePollTimer()
{
    Message *message;

    if ((message = mNetif.GetIp6().mMessagePool.New(Message::kTypeMacDataPoll, 0)) != NULL)
    {
        SendMessage(*message);
        otLogInfoMac("Sent poll");
    }

    mPollTimer.Start(mPollPeriod);
}

ThreadError MeshForwarder::GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    aMacAddr.mLength = sizeof(aMacAddr.mExtAddress);
    aMacAddr.mExtAddress.Set(aIp6Addr);

    if (memcmp(&aMacAddr.mExtAddress, mMac.GetExtAddress(), sizeof(aMacAddr.mExtAddress)) != 0)
    {
        aMacAddr.mLength = sizeof(aMacAddr.mShortAddress);
        aMacAddr.mShortAddress = mMac.GetShortAddress();
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
    else if (mMle.IsRoutingLocator(aIp6Addr))
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

    VerifyOrExit(mSendMessage != NULL, error = kThreadError_Abort);
    mSendBusy = true;

    switch (mSendMessage->GetType())
    {
    case Message::kTypeIp6:
        if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
        {
            if (!mScanning)
            {
                mScanChannel = kPhyMinChannel;
                mRestoreChannel = mMac.GetChannel();
                mScanning = true;
            }

            while ((mScanChannels & 1) == 0)
            {
                mScanChannels >>= 1;
                mScanChannel++;
            }

            mMac.SetChannel(mScanChannel);
            aFrame.SetChannel(mScanChannel);
        }

        SendFragment(*mSendMessage, aFrame);
        assert(aFrame.GetLength() != 7);
        break;

    case Message::kType6lowpan:
        SendMesh(*mSendMessage, aFrame);
        break;

    case Message::kTypeMacDataPoll:
        SendPoll(*mSendMessage, aFrame);
        break;
    }

    // set FramePending if there are more queued messages for the child
    aFrame.GetDstAddr(macDest);

    if (((child = mMle.GetChild(macDest)) != NULL)
        && ((child->mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0)
        && (child->mQueuedIndirectMessageCnt > 1))
    {
        aFrame.SetFramePending(true);
    }

exit:
    return error;
}

ThreadError MeshForwarder::SendPoll(Message &aMessage, Mac::Frame &aFrame)
{
    Mac::Address macSource;
    uint16_t fcf;
    Neighbor *neighbor;

    macSource.mShortAddress = mMac.GetShortAddress();

    if (macSource.mShortAddress != Mac::kShortAddrInvalid)
    {
        macSource.mLength = sizeof(macSource.mShortAddress);
    }
    else
    {
        macSource.mLength = sizeof(macSource.mExtAddress);
        memcpy(&macSource.mExtAddress, mMac.GetExtAddress(), sizeof(macSource.mExtAddress));
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
    aFrame.SetDstPanId(mMac.GetPanId());

    neighbor = mMle.GetParent();
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
    aFrame.SetDstPanId(mMac.GetPanId());
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

    dstpan = mMac.GetPanId();

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

    if (dstpan == mMac.GetPanId())
    {
        fcf |= Mac::Frame::kFcfPanidCompression;
    }

    aFrame.InitMacHeader(fcf, secCtl);
    aFrame.SetDstPanId(dstpan);
    aFrame.SetSrcPanId(mMac.GetPanId());

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
        // Calculate the number of predicted hops.
        hopsLeft = mMle.GetRouteCost(mMeshDest);
        hopsLeft += mMle.GetLinkCost(mMle.GetRouterId(mMle.GetNextHop(mMeshDest)));

        // The hopsLft field MUST be incremented by one if the device is not
        // an active Router.
        if (!mMle.IsActiveRouter(mMeshSource))
        {
            hopsLeft += 1;
        }

        // The hopsLft field MUST be incremented by one if the destination RLOC16
        // is not that of an active Router.
        if (!mMle.IsActiveRouter(mMeshDest))
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
        hcLength = mLowpan.Compress(aMessage, meshSource, meshDest, payload);
        assert(hcLength > 0);
        headerLength += static_cast<uint8_t>(hcLength);

        payloadLength = aMessage.GetLength() - aMessage.GetOffset();

        fragmentLength = aFrame.GetMaxPayloadLength() - headerLength;

        if (payloadLength > fragmentLength)
        {
            // write Fragment header
            aMessage.SetDatagramTag(mFragTag++);
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

    mSendBusy = false;

    if (mSendMessage == NULL)
    {
        mScheduleTransmissionTask.Post();
        ExitNow();
    }

    aFrame.GetDstAddr(macDest);

    if ((neighbor = mMle.GetNeighbor(macDest)) != NULL)
    {
        switch (aError)
        {
        case kThreadError_None:
            neighbor->mLinkFailures = 0;
            break;

        case kThreadError_ChannelAccessFailure:
            break;

        case kThreadError_NoAck:
            neighbor->mLinkFailures++;

            if (mMle.IsActiveRouter(neighbor->mValid.mRloc16))
            {
                if (neighbor->mLinkFailures >= Mle::kFailedRouterTransmissions)
                {
                    mMle.RemoveNeighbor(*neighbor);
                }
            }

            break;

        default:
            assert(false);
            break;
        }
    }

    if ((child = mMle.GetChild(macDest)) != NULL)
    {
        child->mDataRequest = false;

        if (mMessageNextOffset < mSendMessage->GetLength())
        {
            child->mFragmentOffset = mMessageNextOffset;
        }
        else
        {
            child->mFragmentOffset = 0;
            mSendMessage->ClearChildMask(mMle.GetChildIndex(*child));

            if ((child->mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0)
            {
                child->mQueuedIndirectMessageCnt--;
                otLogDebgMac("Sent to child (0x%x), still queued message (%d)",
                             child->mValid.mRloc16, child->mQueuedIndirectMessageCnt);

                if (child->mQueuedIndirectMessageCnt == 0)
                {
                    ClearSrcMatchEntry(*child);
                }
            }
        }
    }

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
            mDiscoverTimer.Start(mScanDuration);
            ExitNow();
        }
    }

    if (mSendMessage->GetType() == Message::kTypeMacDataPoll)
    {
        neighbor = mMle.GetParent();

        if (neighbor->mState == Neighbor::kStateInvalid)
        {
            mPollTimer.Stop();
            mMle.BecomeDetached();
        }
    }

    if (mSendMessage->GetDirectTransmission() == false && mSendMessage->IsChildPending() == false)
    {
        mSendQueue.Dequeue(*mSendMessage);
        mSendMessage->Free();
        mSendMessage = NULL;
        mMessageNextOffset = 0;
    }

    mScheduleTransmissionTask.Post();

exit:
    {}
}

void MeshForwarder::SetDiscoverParameters(uint32_t aScanChannels, uint16_t aScanDuration)
{
    mScanChannels = (aScanChannels == 0) ? static_cast<uint32_t>(Mac::kScanChannelsAll) : aScanChannels;
    mScanDuration = (aScanDuration == 0) ? static_cast<uint16_t>(Mac::kScanDurationDefault) : aScanDuration;
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
            mMac.SetChannel(mRestoreChannel);
            mScanning = false;
            mMle.HandleDiscoverComplete();
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
    Child *child = NULL;
    ThreadError error = kThreadError_None;

    if (!mEnabled)
    {
        ExitNow(error = kThreadError_InvalidState);
    }

    SuccessOrExit(error = aFrame.GetSrcAddr(macSource));
    SuccessOrExit(aFrame.GetDstAddr(macDest));

    if ((child = mMle.GetChild(macSource)) != NULL)
    {
        if (((child->mMode & Mle::ModeTlv::kModeRxOnWhenIdle) == 0) &&
            macSource.mLength == sizeof(otShortAddress))
        {
            child->mAddSrcMatchEntryShort = true;
        }
    }

    aFrame.GetSrcPanId(messageInfo.mPanId);
    messageInfo.mChannel = aFrame.GetChannel();
    messageInfo.mRss = aFrame.GetPower();
    messageInfo.mLqi = aFrame.GetLqi();
    messageInfo.mLinkSecurity = aFrame.GetSecurityEnabled();

    payload = aFrame.GetPayload();
    payloadLength = aFrame.GetPayloadLength();

    if (mPollTimer.IsRunning() && aFrame.GetFramePending())
    {
        // add delay to avoid packet loss due to possible switch senarios between transmit/receive status
        mPollTimer.Start(OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD);
    }

    switch (aFrame.GetType())
    {
    case Mac::Frame::kFcfFrameData:
        if (payloadLength >= sizeof(Lowpan::MeshHeader) &&
            reinterpret_cast<Lowpan::MeshHeader *>(payload)->IsMeshHeader())
        {
            HandleMesh(payload, payloadLength, messageInfo);
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
        otLogDebgMacErr(error, "Dropping received frame");
    }
}

void MeshForwarder::HandleMesh(uint8_t *aFrame, uint8_t aFrameLength, const ThreadMessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Message *message = NULL;
    Mac::Address meshDest;
    Mac::Address meshSource;
    Lowpan::MeshHeader meshHeader(aFrame);

    // Length Check
    VerifyOrExit(meshHeader.GetHeaderLength() <= aFrameLength, error = kThreadError_Drop);

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aMessageInfo.mLinkSecurity && meshHeader.IsValid(), error = kThreadError_Security);

    meshSource.mLength = sizeof(meshSource.mShortAddress);
    meshSource.mShortAddress = meshHeader.GetSource();
    meshDest.mLength = sizeof(meshDest.mShortAddress);
    meshDest.mShortAddress = meshHeader.GetDestination();

    if (meshDest.mShortAddress == mMac.GetShortAddress())
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
        otLogDebgMacErr(error, "Dropping received mesh frame");

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
    Lowpan::MeshHeader meshHeader(aFrame);

    // skip mesh header
    VerifyOrExit(meshHeader.GetHeaderLength() <= aFrameLength, error = kThreadError_Drop);
    aFrame += meshHeader.GetHeaderLength();
    aFrameLength -= meshHeader.GetHeaderLength();

    // skip fragment header
    if (aFrameLength >= 1 &&
        reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
    {
        VerifyOrExit(sizeof(Lowpan::FragmentHeader) <= aFrameLength, error = kThreadError_Drop);
        VerifyOrExit(reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetDatagramOffset() == 0,);

        aFrame += reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
        aFrameLength -= reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->GetHeaderLength();
    }

    // only process IPv6 packets
    VerifyOrExit(aFrameLength >= 1 && Lowpan::Lowpan::IsLowpanHc(aFrame),);

    VerifyOrExit(mLowpan.DecompressBaseHeader(ip6Header, aMeshSource, aMeshDest, aFrame, aFrameLength) > 0,
                 error = kThreadError_Drop);

    error = mMle.CheckReachability(aMeshSource.mShortAddress, aMeshDest.mShortAddress, ip6Header);

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
        headerLength = mLowpan.Decompress(*message, aMacSource, aMacDest, aFrame, aFrameLength, datagramLength);
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
            HandleDatagram(*message, aMessageInfo);
        }
    }
    else
    {
        otLogDebgMacErr(error, "Dropping received fragment");

        if (message != NULL)
        {
            message->Free();
        }
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

    headerLength = mLowpan.Decompress(*message, aMacSource, aMacDest, aFrame, aFrameLength, 0);
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
        HandleDatagram(*message, aMessageInfo);
    }
    else
    {
        otLogDebgMacErr(error, "Dropping received lowpan HC");

        if (message != NULL)
        {
            message->Free();
        }
    }
}

ThreadError MeshForwarder::HandleDatagram(Message &aMessage, const ThreadMessageInfo &aMessageInfo)
{
    return mNetif.GetIp6().HandleDatagram(aMessage, &mNetif, mNetif.GetInterfaceId(), &aMessageInfo, false);
}

void MeshForwarder::UpdateFramePending()
{
}

void MeshForwarder::HandleDataRequest(const Mac::Address &aMacSource, const ThreadMessageInfo &aMessageInfo)
{
    Child *child;

    // Security Check: only process secure Data Poll frames.
    VerifyOrExit(aMessageInfo.mLinkSecurity, ;);

    VerifyOrExit(mMle.GetDeviceState() != Mle::kDeviceStateDetached, ;);

    VerifyOrExit((child = mMle.GetChild(aMacSource)) != NULL, ;);
    child->mLastHeard = Timer::GetNow();
    child->mLinkFailures = 0;

    if (child->mQueuedIndirectMessageCnt > 0)
    {
        child->mDataRequest = true;
    }

    mScheduleTransmissionTask.Post();

exit:
    {}
}

}  // namespace Thread
