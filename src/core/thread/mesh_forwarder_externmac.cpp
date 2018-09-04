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

#define WPP_NAME "mesh_forwarder_externmac.tmh"

#include "mesh_forwarder_externmac.hpp"

#if OPENTHREAD_CONFIG_USE_EXTERNAL_MAC

#include <openthread/platform/random.h>

#include <thread/thread_tlvs.hpp>
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/owner-locator.hpp"
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

MeshSender::MeshSender()
    : mSender(&MeshSender::DispatchFrameRequest, &MeshSender::DispatchSentFrame, this)
    , mMessageNextOffset(0)
    , mSendMessage(NULL)
    , mMeshSource(Mac::kShortAddrInvalid)
    , mMeshDest(Mac::kShortAddrInvalid)
    , mAddMeshHeader(false)
    , mParent(NULL)
    , mAckRequested(false)
    , mIdleMessageSent(false)
    , mBoundChild(NULL)
{
    mMacSource.SetNone();
    mMacDest.SetNone();
}

MeshForwarder::MeshForwarder(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mMacReceiver(&MeshForwarder::HandleReceivedFrame, &MeshForwarder::HandleDataPollTimeout, this)
    , mDiscoverTimer(aInstance, &MeshForwarder::HandleDiscoverTimer, this)
    , mReassemblyTimer(aInstance, &MeshForwarder::HandleReassemblyTimer, this)
    , mDirectSender()
#if OPENTHREAD_CONFIG_EXTERNAL_MAC_FLOATING_SENDERS == 0
    , mFloatingMacSenders(NULL)
#endif
#if OPENTHREAD_CONFIG_EXTERNAL_MAC_MAX_SEDS == 0
    , mMeshSenders(NULL)
#endif
    , mScheduleTransmissionTask(aInstance, ScheduleTransmissionTask, this)
    , mEnabled(false)
    , mScanChannels(0)
    , mScanChannel(0)
    , mRestoreChannel(0)
    , mRestorePanId(Mac::kPanIdBroadcast)
    , mScanning(false)
    , mDataPollManager(aInstance)
    , mSourceMatchController(aInstance)
{
    GetNetif().GetMac().RegisterReceiver(mMacReceiver);

    mDirectSender.mParent = this;
    for (int i = 0; i < kNumIndirectSenders; i++)
    {
        mMeshSenders[i].mParent = this;
    }

    for (int i = 0; i < kNumFloatingSenders; i++)
    {
        mFloatingMacSenders[i] = Mac::Sender(&MeshSender::DispatchFrameRequest, MeshSender::DispatchSentFrame, NULL);
    }

    mIpCounters.mTxSuccess = 0;
    mIpCounters.mRxSuccess = 0;
    mIpCounters.mTxFailure = 0;
    mIpCounters.mRxFailure = 0;

    mFragTag = static_cast<uint16_t>(otPlatRandomGet());
}

otInstance *MeshSender::GetInstance()
{
    return &mParent->GetInstance();
}

otError MeshForwarder::Start(void)
{
    otError error = OT_ERROR_NONE;

    if (mEnabled == false)
    {
        GetNetif().GetMac().Start();
        GetNetif().GetMac().SetRxOnWhenIdle(true);
        mEnabled = true;
    }

    return error;
}

otError MeshForwarder::Stop(void)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Message *    message;

    VerifyOrExit(mEnabled == true);

    netif.GetMac().Stop();

    mDataPollManager.StopPolling();
    mReassemblyTimer.Stop();

    if (mScanning)
    {
        netif.GetMac().SetPanChannel(mRestoreChannel);
        mScanning = false;
        netif.GetMle().HandleDiscoverComplete();
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

    mDirectSender.mSendMessage = NULL;
    for (int i = 0; i < kNumIndirectSenders; i++)
    {
        mMeshSenders[i].mSendMessage = NULL;
        mMeshSenders[i].mBoundChild  = NULL;
    }

    netif.GetMac().SetRxOnWhenIdle(false);

exit:
    return error;
}

void MeshForwarder::RemoveMessage(Message &aMessage)
{
    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateAnyExceptInvalid); !iter.IsDone(); iter++)
    {
        IgnoreReturnValue(RemoveMessageFromSleepyChild(aMessage, *iter.GetChild()));
    }

    if (mDirectSender.mSendMessage == &aMessage)
    {
        mDirectSender.mSendMessage = NULL;
    }

    mSendQueue.Dequeue(aMessage);
    LogIp6Message(kMessageEvict, aMessage, NULL, OT_ERROR_NO_BUFS);
    aMessage.Free();
}

void MeshForwarder::ScheduleTransmissionTask(Tasklet &aTasklet)
{
    aTasklet.GetOwner<MeshForwarder>().ScheduleTransmissionTask();
}

void MeshForwarder::ScheduleTransmissionTask(void)
{
    otLogDebgMac(GetInstance(), "MeshForwarder::ScheduleTransmissionTask called");

    // Queue any pending indirects into free sender slots
#if OPENTHREAD_FTD
    UpdateIndirectMessages();
    for (uint8_t i = 0; i < kNumIndirectSenders; i++)
    {
        mMeshSenders[i].ScheduleIndirectTransmission();
    }
#endif

    // Handle the direct sending using the direct sender
    mDirectSender.ScheduleDirectTransmission();
}

otError MeshSender::ScheduleDirectTransmission()
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mSender.IsInUse() && !mParent->mDiscoverTimer.IsRunning(), error = OT_ERROR_BUSY);

    if (mSendMessage == NULL)
    {
        mSendMessage       = mParent->GetDirectTransmission(*this);
        mMessageNextOffset = 0;
        VerifyOrExit(mSendMessage != NULL);
    }
    mParent->GetNetif().GetMac().SendFrameRequest(mSender);
    if (mSendMessage == NULL)
        ExitNow(); // Data polls are sent instantly

exit:
    return error;
}

otError MeshForwarder::PrepareDiscoverRequest(void)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(!mScanning);

    mScanChannel = OT_RADIO_CHANNEL_MIN;
    mScanChannels >>= OT_RADIO_CHANNEL_MIN;
    mRestoreChannel = netif.GetMac().GetPanChannel();
    mRestorePanId   = netif.GetMac().GetPanId();

    while ((mScanChannels & 1) == 0)
    {
        mScanChannels >>= 1;
        mScanChannel++;

        if (mScanChannel > OT_RADIO_CHANNEL_MAX)
        {
            netif.GetMle().HandleDiscoverComplete();
            ExitNow(error = OT_ERROR_DROP);
        }
    }

    mScanning = true;

exit:
    return error;
}

Message *MeshForwarder::GetDirectTransmission(MeshSender &aSender)
{
    Message *curMessage, *nextMessage;
    otError  error = OT_ERROR_NONE;

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
            error = UpdateIp6Route(*curMessage, aSender);

            if (curMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
            {
                error = PrepareDiscoverRequest();
            }
            break;

#if OPENTHREAD_FTD
        case Message::kType6lowpan:
            error = UpdateMeshRoute(*curMessage, aSender);
            break;
#endif

        case Message::kTypeSupervision:
            error = OT_ERROR_DROP;
            break;
        }

        switch (error)
        {
        case OT_ERROR_NONE:
            ExitNow();
            break;

        case OT_ERROR_ADDRESS_QUERY:
            mSendQueue.Dequeue(*curMessage);
            mResolvingQueue.Enqueue(*curMessage);
            continue;

        case OT_ERROR_DROP:
        case OT_ERROR_NO_BUFS:
            mSendQueue.Dequeue(*curMessage);
            LogIp6Message(kMessageDrop, *curMessage, NULL, error);
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

otError MeshForwarder::UpdateIp6Route(Message &aMessage, MeshSender &aSender)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Ip6::Header  ip6Header;

    aSender.mAddMeshHeader = false;

    aMessage.Read(0, sizeof(ip6Header), &ip6Header);

    VerifyOrExit(!ip6Header.GetSource().IsMulticast(), error = OT_ERROR_DROP);

    // 1. Choose correct MAC Source Address.
    GetMacSourceAddress(ip6Header.GetSource(), aSender.mMacSource);

    // 2. Choose correct MAC Destination Address.
    if (netif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED || netif.GetMle().GetRole() == OT_DEVICE_ROLE_DETACHED)
    {
        // Allow only for link-local unicasts and multicasts.
        if (ip6Header.GetDestination().IsLinkLocal() || ip6Header.GetDestination().IsLinkLocalMulticast())
        {
            GetMacDestinationAddress(ip6Header.GetDestination(), aSender.mMacDest);
        }
        else
        {
            error = OT_ERROR_DROP;
        }

        ExitNow();
    }

    if (ip6Header.GetDestination().IsMulticast())
    {
        // With the exception of MLE multicasts, a Thread End Device transmits multicasts,
        // as IEEE 802.15.4 unicasts to its parent.
        if (netif.GetMle().GetRole() == OT_DEVICE_ROLE_CHILD && !aMessage.IsSubTypeMle())
        {
            aSender.mMacDest.SetShort(netif.GetMle().GetNextHop(Mac::kShortAddrBroadcast));
        }
        else
        {
            aSender.mMacDest.SetShort(Mac::kShortAddrBroadcast);
        }
    }
    else if (ip6Header.GetDestination().IsLinkLocal())
    {
        GetMacDestinationAddress(ip6Header.GetDestination(), aSender.mMacDest);
    }
    else if (netif.GetMle().IsMinimalEndDevice())
    {
        aSender.mMacDest.SetShort(netif.GetMle().GetNextHop(Mac::kShortAddrBroadcast));
    }

#if OPENTHREAD_FTD
    else
    {
        Neighbor *neighbor;

        if (netif.GetMle().IsRoutingLocator(ip6Header.GetDestination()))
        {
            uint16_t rloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);
            VerifyOrExit(netif.GetMle().IsRouterIdValid(netif.GetMle().GetRouterId(rloc16)), error = OT_ERROR_DROP);

            aSender.mMeshDest = rloc16;
        }
        else if (netif.GetMle().IsAnycastLocator(ip6Header.GetDestination()))
        {
            uint16_t aloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);

            if (aloc16 == Mle::kAloc16Leader)
            {
                aSender.mMeshDest = netif.GetMle().GetRloc16(netif.GetMle().GetLeaderId());
            }

#if OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
            else if (aloc16 <= Mle::kAloc16DhcpAgentEnd)
            {
                uint16_t agentRloc16;
                uint8_t  routerId;
                VerifyOrExit(
                    (netif.GetNetworkDataLeader().GetRlocByContextId(
                         static_cast<uint8_t>(aloc16 & Mle::kAloc16DhcpAgentMask), agentRloc16) == OT_ERROR_NONE),
                    error = OT_ERROR_DROP);

                routerId = netif.GetMle().GetRouterId(agentRloc16);

                // if agent is active router or the child of the device
                if ((netif.GetMle().IsActiveRouter(agentRloc16)) ||
                    (netif.GetMle().GetRloc16(routerId) == netif.GetMle().GetRloc16()))
                {
                    aSender.mMeshDest = agentRloc16;
                }
                else
                {
                    // use the parent of the ED Agent as Dest
                    aSender.mMeshDest = netif.GetMle().GetRloc16(routerId);
                }
            }

#endif // OPENTHREAD_ENABLE_DHCP6_SERVER || OPENTHREAD_ENABLE_DHCP6_CLIENT
#if OPENTHREAD_ENABLE_SERVICE
            else if ((aloc16 >= Mle::kAloc16ServiceStart) && (aloc16 <= Mle::kAloc16ServiceEnd))
            {
                SuccessOrExit(error = GetDestinationRlocByServiceAloc(aloc16, aSender.mMeshDest));
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
            aSender.mMeshDest = neighbor->GetRloc16();
        }
        else if (netif.GetNetworkDataLeader().IsOnMesh(ip6Header.GetDestination()))
        {
            SuccessOrExit(error = netif.GetAddressResolver().Resolve(ip6Header.GetDestination(), aSender.mMeshDest));
        }
        else
        {
            netif.GetNetworkDataLeader().RouteLookup(ip6Header.GetSource(), ip6Header.GetDestination(), NULL,
                                                     &aSender.mMeshDest);
        }

        VerifyOrExit(aSender.mMeshDest != Mac::kShortAddrInvalid, error = OT_ERROR_DROP);

        if (netif.GetMle().GetNeighbor(aSender.mMeshDest) != NULL)
        {
            // destination is neighbor
            aSender.mMacDest.SetShort(aSender.mMeshDest);
        }
        else
        {
            // destination is not neighbor
            aSender.mMeshSource = netif.GetMac().GetShortAddress();

            SuccessOrExit(error = netif.GetMle().CheckReachability(aSender.mMeshSource, aSender.mMeshDest, ip6Header));

            aSender.mMacDest.SetShort(netif.GetMle().GetNextHop(aSender.mMeshDest));
            aSender.mMacSource.SetShort(aSender.mMeshSource);
            aSender.mAddMeshHeader = true;
        }
    }

#else // OPENTHREAD_FTD
    else
    {
        assert(false);
    }

#endif // OPENTHREAD_FTD

exit:
    return error;
}

void MeshForwarder::SetRxOff(void)
{
    ThreadNetif &netif = GetNetif();

    netif.GetMac().SetRxOnWhenIdle(false);
    mDataPollManager.StopPolling();
    netif.GetSupervisionListener().Stop();
}

bool MeshForwarder::GetRxOnWhenIdle(void)
{
    return GetNetif().GetMac().GetRxOnWhenIdle();
}

void MeshForwarder::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    ThreadNetif &netif = GetNetif();

    netif.GetMac().SetRxOnWhenIdle(aRxOnWhenIdle);

    if (aRxOnWhenIdle)
    {
        mDataPollManager.StopPolling();
        netif.GetSupervisionListener().Stop();
    }
    else
    {
        mDataPollManager.StartPolling();
        netif.GetSupervisionListener().Start();
    }
}

uint8_t MeshForwarder::GetRemainingSEDSlotCount()
{
    uint8_t count = 0;

    for (int i = 0; i < kNumIndirectSenders; i++)
    {
        if (mMeshSenders[i].mBoundChild == NULL)
        {
            count++;
        }
    }

    return count;
}

otError MeshForwarder::AllocateSEDSlot(Child &aChild)
{
    otError error = OT_ERROR_NO_BUFS;

    // First confirm it isn't already allocated
    for (int i = 0; i < kNumIndirectSenders; i++)
    {
        if (mMeshSenders[i].mBoundChild == &aChild)
        {
            mScheduleTransmissionTask.Post();
            ExitNow(error = OT_ERROR_NONE);
        }
    }

    for (int i = 0; i < kNumIndirectSenders; i++)
    {
        if (mMeshSenders[i].mBoundChild == NULL)
        {
            mMeshSenders[i].mBoundChild = &aChild;
            mScheduleTransmissionTask.Post();
            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    return error;
}

otError MeshForwarder::DeallocateSEDSlot(Child &aChild)
{
    otError error = OT_ERROR_NONE;

    for (int i = 0; i < kNumIndirectSenders; i++)
    {
        if (mMeshSenders[i].mBoundChild == &aChild)
        {
            mMeshSenders[i].mBoundChild = NULL;
        }
    }

    return error;
}

otError MeshForwarder::GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    ThreadNetif &netif = GetNetif();

    aIp6Addr.ToExtAddress(aMacAddr);

    if (aMacAddr.GetExtended() != netif.GetMac().GetExtAddress())
    {
        aMacAddr.SetShort(netif.GetMac().GetShortAddress());
    }

    return OT_ERROR_NONE;
}

otError MeshForwarder::GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    if (aIp6Addr.IsMulticast())
    {
        aMacAddr.SetShort(Mac::kShortAddrBroadcast);
    }
    else if (aIp6Addr.mFields.m16[0] == HostSwap16(0xfe80) && aIp6Addr.mFields.m16[1] == HostSwap16(0x0000) &&
             aIp6Addr.mFields.m16[2] == HostSwap16(0x0000) && aIp6Addr.mFields.m16[3] == HostSwap16(0x0000) &&
             aIp6Addr.mFields.m16[4] == HostSwap16(0x0000) && aIp6Addr.mFields.m16[5] == HostSwap16(0x00ff) &&
             aIp6Addr.mFields.m16[6] == HostSwap16(0xfe00))
    {
        aMacAddr.SetShort(HostSwap16(aIp6Addr.mFields.m16[7]));
    }
    else if (GetNetif().GetMle().IsRoutingLocator(aIp6Addr))
    {
        aMacAddr.SetShort(HostSwap16(aIp6Addr.mFields.m16[7]));
    }
    else
    {
        aIp6Addr.ToExtAddress(aMacAddr);
    }

    return OT_ERROR_NONE;
}

otError MeshSender::DispatchFrameRequest(Mac::Sender &aSender, Mac::Frame &aFrame, otDataRequest &aDataReq)
{
    return aSender.GetMeshSender()->HandleFrameRequest(aSender, aFrame, aDataReq);
}

otError MeshSender::HandleFrameRequest(Mac::Sender &aSender, Mac::Frame &aFrame, otDataRequest &aDataReq)
{
    ThreadNetif &netif = mParent->GetNetif();
    otError      error = OT_ERROR_NONE;
#if OPENTHREAD_FTD
    Child *child = mBoundChild;
#endif

    VerifyOrExit(mParent->mEnabled, error = OT_ERROR_ABORT);

    if (mSendMessage == NULL)
    {
        if (mBoundChild != NULL && !mIdleMessageSent)
        {
            mIdleMessageSent = true;
            SendIdleFrame(aDataReq);
            static_cast<Mac::FullAddr *>(&aDataReq.mDst)->GetAddress(mMacDest);
            mAckRequested = true;
            ExitNow();
        }
        else
        {
            ExitNow(error = OT_ERROR_ABORT);
        }
    }

    VerifyOrExit(mMessageNextOffset < mSendMessage->GetLength(), error = OT_ERROR_ALREADY);
    mSendMessage->SetOffset(mMessageNextOffset);
#if OPENTHREAD_FTD
    if (child != NULL && !child->IsRxOnWhenIdle())
    {
        child->GetMacAddress(mMacDest);
        PrepareIndirectTransmission(*child);
    }
#endif

    switch (mSendMessage->GetType())
    {
    case Message::kTypeIp6:
        if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
        {
            netif.GetMac().SetPanChannel(mParent->mScanChannel);
            aFrame.SetChannel(mParent->mScanChannel);

            // In case a specific PAN ID of a Thread Network to be discovered is not known, Discovery
            // Request messages MUST have the Destination PAN ID in the IEEE 802.15.4 MAC header set
            // to be the Broadcast PAN ID (0xFFFF) and the Source PAN ID set to a randomly generated
            // value.
            if (mSendMessage->GetPanId() == Mac::kPanIdBroadcast && netif.GetMac().GetPanId() == Mac::kPanIdBroadcast)
            {
                uint16_t panid;

                do
                {
                    panid = static_cast<uint16_t>(otPlatRandomGet());
                } while (panid == Mac::kPanIdBroadcast);

                netif.GetMac().SetPanId(panid);
            }
        }

        error = SendFragment(*mSendMessage, aFrame, aDataReq);

        // `SendFragment()` fails with `NotCapable` error if the message is MLE (with
        // no link layer security) and also requires fragmentation.
        if (error == OT_ERROR_NOT_CAPABLE)
        {
            // Enable security and try again.
            mSendMessage->SetLinkSecurityEnabled(true);
            error = SendFragment(*mSendMessage, aFrame, aDataReq);
        }
        break;

#if OPENTHREAD_FTD
    case Message::kType6lowpan:
        error = SendMesh(*mSendMessage, aDataReq);
        break;

    case Message::kTypeSupervision:
        error              = SendEmptyFrame(MeshForwarder::kSupervisionMsgAckRequest, aDataReq);
        mMessageNextOffset = mSendMessage->GetLength();
        break;
#endif
    }

    assert(error == OT_ERROR_NONE);

    static_cast<Mac::FullAddr *>(&aDataReq.mDst)->GetAddress(mMacDest);
    mAckRequested = (aDataReq.mTxOptions & OT_MAC_TX_OPTION_ACK_REQ) ? true : false;

    // Todo: Setup offset
    aSender.SetMessageEndOffset(mMessageNextOffset);
exit:
    return error;
}

otError MeshForwarder::SendPoll()
{
    ThreadNetif & netif = GetNetif();
    Mac::Mac &    mac   = netif.GetMac();
    otPollRequest pollReq;
    Neighbor *    parent = netif.GetMle().GetParentCandidate();
    Mac::Address  macSource;
    otError       error = OT_ERROR_NONE;

    VerifyOrExit(!mac.IsScanInProgress(), error = OT_ERROR_BUSY);

    if (!parent->IsStateValidOrRestoring())
    {
        mDataPollManager.StopPolling();
        netif.GetMle().BecomeDetached();
        error = OT_ERROR_INVALID_STATE;
        ExitNow();
    }

    macSource.SetShort(netif.GetMac().GetShortAddress());

    memset(&pollReq, 0, sizeof(pollReq));
    Encoding::LittleEndian::WriteUint16(netif.GetMac().GetPanId(), pollReq.mCoordAddress.mPanId);

    if (!macSource.IsShortAddrInvalid())
    {
        pollReq.mCoordAddress.mAddressMode = OT_MAC_ADDRESS_MODE_SHORT;
        Encoding::LittleEndian::WriteUint16(parent->GetRloc16(), pollReq.mCoordAddress.mAddress);
    }
    else
    {
        Mac::Address parentAddr;
        parentAddr.SetExtended(parent->GetExtAddress());
        static_cast<Mac::FullAddr *>(&pollReq.mCoordAddress)->SetAddress(parentAddr);
    }

    pollReq.mSecurity.mSecurityLevel = Mac::Frame::kSecEncMic32;
    pollReq.mSecurity.mKeyIdMode     = 1;

    error = mac.SendDataPoll(pollReq);

    if (error == OT_ERROR_NO_ACK)
    {
        parent->IncrementLinkFailures();
        if (parent->GetLinkFailures() >= Mle::kFailedRouterTransmissions)
        {
            netif.GetMle().RemoveNeighbor(*parent);
            error = OT_ERROR_INVALID_STATE;
        }
    }

exit:
    return error;
}

size_t MeshSender::GetMaxMsduSize(otDataRequest &aDataReq)
{
    uint16_t panId  = mParent->GetNetif().GetMac().GetPanId();
    size_t   maxLen = OT_RADIO_FRAME_MAX_SIZE;
    size_t   footerLen, headerLen;

    // Table 95 to calculate auth tag length
    footerLen = 2 << (aDataReq.mSecurity.mSecurityLevel % 4);
    footerLen = footerLen == 2 ? 0 : footerLen;
    footerLen += Mac::Frame::kFcsSize;

    headerLen = Mac::Frame::kFcfSize + Mac::Frame::kDsnSize;
    if (aDataReq.mSrcAddrMode == OT_MAC_ADDRESS_MODE_SHORT)
    {
        headerLen += sizeof(otShortAddress);
    }
    else if (aDataReq.mSrcAddrMode == OT_MAC_ADDRESS_MODE_EXT)
    {
        headerLen += sizeof(otExtAddress);
    }

    if (aDataReq.mDst.mAddressMode == OT_MAC_ADDRESS_MODE_SHORT)
    {
        headerLen += sizeof(otShortAddress);
    }
    else if (aDataReq.mDst.mAddressMode == OT_MAC_ADDRESS_MODE_EXT)
    {
        headerLen += sizeof(otExtAddress);
    }

    headerLen += sizeof(otPanId); // DstPanId
    if (Encoding::LittleEndian::ReadUint16(aDataReq.mDst.mPanId) != panId)
    {
        headerLen += sizeof(otPanId); // SrcPanId
    }

    if (aDataReq.mSecurity.mSecurityLevel != 0)
    {
        headerLen += Mac::Frame::kSecurityControlSize + Mac::Frame::kMic32Size;

        switch (aDataReq.mSecurity.mKeyIdMode)
        {
        case 1:
            headerLen += Mac::Frame::kKeySourceSizeMode1 + Mac::Frame::kKeyIndexSize;
            break;

        case 2:
            headerLen += Mac::Frame::kKeySourceSizeMode2 + Mac::Frame::kKeyIndexSize;
            break;

        case 3:
            headerLen += Mac::Frame::kKeySourceSizeMode3 + Mac::Frame::kKeyIndexSize;
            break;
        }
    }

    return (maxLen - footerLen - headerLen);
}

uint16_t MeshForwarder::GetNextFragTag(void)
{
    // avoid using datagram tag value 0, which indicates the tag has not been set
    if (mFragTag == 0)
    {
        mFragTag++;
    }

    return mFragTag++;
}

otError MeshSender::SendFragment(Message &aMessage, Mac::Frame &aFrame, otDataRequest &aDataReq)
{
    ThreadNetif &           netif = mParent->GetNetif();
    Mac::Address            meshDest, meshSource;
    Lowpan::FragmentHeader *fragmentHeader;
    Lowpan::MeshHeader      meshHeader;
    uint8_t *               payload;
    uint8_t                 headerLength;
    uint8_t                 hopsLeft;
    uint16_t                payloadLength;
    int                     hcLength;
    uint16_t                fragmentLength;
    uint16_t                dstpan;
    otError                 error = OT_ERROR_NONE;

    if (mAddMeshHeader)
    {
        meshSource.SetShort(mMeshSource);
        meshDest.SetShort(mMeshDest);
    }
    else
    {
        meshDest   = mMacDest;
        meshSource = mMacSource;
    }

    // initialize MAC header and frame info
    memset(&aDataReq, 0, sizeof(aDataReq));
    static_cast<Mac::FullAddr *>(&aDataReq.mDst)->SetAddress(mMacDest);
    aDataReq.mSrcAddrMode = mMacSource.GetType();

    // all unicast frames request ACK
    if (!mMacDest.IsBroadcast())
    {
        aDataReq.mTxOptions |= OT_MAC_TX_OPTION_ACK_REQ;
    }

    if (aMessage.IsLinkSecurityEnabled())
    {
        aDataReq.mSecurity.mSecurityLevel = Mac::Frame::kSecEncMic32;
        switch (aMessage.GetSubType())
        {
        case Message::kSubTypeJoinerEntrust:
            aDataReq.mSecurity.mKeyIdMode = 0;
            break;

        case Message::kSubTypeMleAnnounce:
            aDataReq.mSecurity.mKeyIdMode = 2;
            break;

        default:
            aDataReq.mSecurity.mKeyIdMode = 1;
            break;
        }
    }

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
        dstpan = netif.GetMac().GetPanId();
        break;
    }

    Encoding::LittleEndian::WriteUint16(dstpan, aDataReq.mDst.mPanId);

    if (!isDirectSender())
        aDataReq.mTxOptions |= OT_MAC_TX_OPTION_INDIRECT;

    payload = aDataReq.mMsdu;

    headerLength = 0;

    // initialize Mesh header
    if (mAddMeshHeader)
    {
        if (netif.GetMle().GetRole() == OT_DEVICE_ROLE_CHILD)
        {
            // REED sets hopsLeft to max (16) + 1. It does not know the route cost.
            hopsLeft = Mle::kMaxRouteCost + 1;
        }
        else
        {
            // Calculate the number of predicted hops.
            hopsLeft = netif.GetMle().GetRouteCost(mMeshDest);

            if (hopsLeft != Mle::kMaxRouteCost)
            {
                hopsLeft +=
                    netif.GetMle().GetLinkCost(netif.GetMle().GetRouterId(netif.GetMle().GetNextHop(mMeshDest)));
            }
            else
            {
                // In case there is no route to the destination router (only link).
                hopsLeft = netif.GetMle().GetLinkCost(netif.GetMle().GetRouterId(mMeshDest));
            }
        }

        // The hopsLft field MUST be incremented by one if the destination RLOC16
        // is not that of an active Router.
        if (!netif.GetMle().IsActiveRouter(mMeshDest))
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
        hcLength = netif.GetLowpan().Compress(aMessage, meshSource, meshDest, payload);
        assert(hcLength > 0);
        headerLength += static_cast<uint8_t>(hcLength);

        payloadLength = aMessage.GetLength() - aMessage.GetOffset();

        fragmentLength = GetMaxMsduSize(aDataReq) - headerLength;

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
                aMessage.SetDatagramTag(mParent->GetNextFragTag());
            }

            memmove(payload + 4, payload, headerLength);

            payloadLength = (fragmentLength - 4) & ~0x7;

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
        aDataReq.mMsduLength = static_cast<uint8_t>(headerLength + payloadLength);

        mMessageNextOffset = aMessage.GetOffset() + payloadLength;
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

        fragmentLength = (GetMaxMsduSize(aDataReq) - headerLength) & ~0x7;

        if (payloadLength > fragmentLength)
        {
            payloadLength = fragmentLength;
        }

        // copy IPv6 Payload
        aMessage.Read(aMessage.GetOffset(), payloadLength, payload);
        aDataReq.mMsduLength = static_cast<uint8_t>(headerLength + payloadLength);

        mMessageNextOffset = aMessage.GetOffset() + payloadLength;
    }

    if (!isDirectSender() && (mMessageNextOffset < aMessage.GetLength()))
    {
        // We have an indirect packet which requires more than a single 15.4 frame - attempt to use overflow
        aDataReq.mTxOptions |= OT_MAC_TX_OPTION_NS_FPEND;
        mParent->GetFreeFloatingSender(this);
    }

exit:
    return error;
}

otError MeshSender::SendIdleFrame(otDataRequest &aDataReq)
{
    ThreadNetif &netif = mParent->GetNetif();

    memset(&aDataReq, 0, sizeof(aDataReq));

    Encoding::LittleEndian::WriteUint16(netif.GetMac().GetPanId(), aDataReq.mDst.mPanId);
    static_cast<Mac::FullAddr *>(&aDataReq.mDst)->SetAddress(mMacDest);
    aDataReq.mSrcAddrMode = OT_MAC_ADDRESS_MODE_SHORT;
    aDataReq.mTxOptions |= OT_MAC_TX_OPTION_INDIRECT;
    aDataReq.mMsduLength = 0;

    return OT_ERROR_NONE;
}

otError MeshSender::SendEmptyFrame(bool aAckRequest, otDataRequest &aDataReq)
{
    ThreadNetif &netif = mParent->GetNetif();
    Mac::Address macSource;

    macSource.SetShort(netif.GetMac().GetShortAddress());

    if (macSource.IsShortAddrInvalid())
    {
        macSource.SetExtended(netif.GetMac().GetExtAddress());
    }

    Encoding::LittleEndian::WriteUint16(netif.GetMac().GetPanId(), aDataReq.mDst.mPanId);
    static_cast<Mac::FullAddr *>(&aDataReq.mDst)->SetAddress(mMacDest);
    aDataReq.mSrcAddrMode = mMacSource.GetType();

    aDataReq.mSecurity.mKeyIdMode     = 1;
    aDataReq.mSecurity.mSecurityLevel = Mac::Frame::kSecEncMic32;

    if (aAckRequest)
    {
        aDataReq.mTxOptions |= OT_MAC_TX_OPTION_ACK_REQ;
    }

    aDataReq.mMsduLength = 0;

    return OT_ERROR_NONE;
}

void MeshSender::DispatchSentFrame(Mac::Sender &aSender, otError aError)
{
    MeshSender *meshSender = aSender.GetMeshSender();

    VerifyOrExit(meshSender != NULL);
    meshSender->HandleSentFrame(aSender, aError);

exit:
    return;
}

void MeshSender::HandleSentFrame(Mac::Sender &aSender, otError aError)
{
    ThreadNetif &netif = mParent->GetNetif();
    Child *      child = mBoundChild;
    Neighbor *   neighbor;
    uint8_t      childIndex;
    uint16_t     sentOffset   = aSender.GetMessageEndOffset();
    bool         sendFinished = false;

    otLogDebgMac(GetInstance(), "MeshSender::HandleSentFrame Called (Sender %d)", this);

    mIdleMessageSent = false;

    if (child != NULL && aError == OT_ERROR_NONE)
    {
        child->SetLastHeard(TimerMilli::GetNow());
    }

    VerifyOrExit(mSendMessage != NULL);
    VerifyOrExit(mParent->mEnabled);

    if ((neighbor = netif.GetMle().GetNeighbor(mMacDest)) != NULL)
    {
        switch (aError)
        {
        case OT_ERROR_NONE:
            if (mAckRequested)
            {
                neighbor->ResetLinkFailures();
            }

            break;

        case OT_ERROR_CHANNEL_ACCESS_FAILURE:
        case OT_ERROR_ABORT:
            break;

        case OT_ERROR_NO_ACK:
            neighbor->IncrementLinkFailures();

            if (netif.GetMle().IsActiveRouter(neighbor->GetRloc16()))
            {
                if (neighbor->GetLinkFailures() >= Mle::kFailedRouterTransmissions)
                {
                    netif.GetMle().RemoveNeighbor(*neighbor);
                }
            }

            break;

        default:
            assert(false);
            break;
        }
    }

    if (child != NULL && !child->IsRxOnWhenIdle())
    {
        childIndex = netif.GetMle().GetChildTable().GetChildIndex(*child);
        child->SetDataRequestPending(false);

        VerifyOrExit(mSendMessage != NULL);

        if (aError == OT_ERROR_NONE)
        {
            child->ResetIndirectTxAttempts();
        }
        else
        {
#if OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE
            // We set the NextOffset to end of message, since there is no need to
            // send any remaining fragments in the message to the child, if all tx
            // attempts of current frame already failed.

            sentOffset = mSendMessage->GetLength();
#endif
        }

        if (sentOffset >= mSendMessage->GetLength())
        {
            sendFinished = true;

            // Enable short source address matching after the first indirect
            // message transmission attempt to the child. We intentionally do
            // not check for successful tx here to address the scenario where
            // the child does receive "Child ID Response" but parent misses the
            // 15.4 ack from child. If the "Child ID Response" does not make it
            // to the child, then the child will need to send a new "Child ID
            // Request" which will cause the parent to switch to using long
            // address mode for source address matching.

            mParent->mSourceMatchController.SetSrcMatchAsShort(*child, true);

            childIndex = netif.GetMle().GetChildTable().GetChildIndex(*child);

            mParent->ReleaseFloatingSenders(this);

            if (mSendMessage->GetChildMask(childIndex))
            {
                mSendMessage->ClearChildMask(childIndex);
                mParent->mSourceMatchController.DecrementMessageCount(*child);
            }
        }

        if (aError == OT_ERROR_NONE)
        {
            netif.GetChildSupervisor().UpdateOnSend(*child);
        }
    }

    VerifyOrExit(mSendMessage != NULL);

    if (isDirectSender())
    {
#if OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE

        if (aError != OT_ERROR_NONE)
        {
            // We set the NextOffset to end of message to avoid sending
            // any remaining fragments in the message.

            sentOffset = mSendMessage->GetLength();
        }

#endif

        if (sentOffset >= mSendMessage->GetLength())
        {
            sendFinished = true;
            mSendMessage->ClearDirectTransmission();
            mSendMessage->SetOffset(0);
        }

        if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
        {
            mParent->mDiscoverTimer.Start(static_cast<uint16_t>(Mac::kScanDurationDefault));
            ExitNow();
        }
    }

    if (sentOffset >= mSendMessage->GetLength())
    {
        mParent->LogIp6Message(MeshForwarder::kMessageTransmit, *mSendMessage, &mMacDest, aError);

        if (aError == OT_ERROR_NONE)
        {
            mParent->mIpCounters.mTxSuccess++;
        }
        else
        {
            mParent->mIpCounters.mTxFailure++;
        }
    }

    if (mSendMessage->GetDirectTransmission() == false && mSendMessage->IsChildPending() == false)
    {
        otLogDebgMac(GetInstance(), "Message fully sent, freeing.");
        mParent->mSendQueue.Dequeue(*mSendMessage);
        mSendMessage->Free();
        sendFinished = true;
    }

    if (sendFinished)
    {
        mMessageNextOffset = 0;
        mSendMessage       = NULL;
    }

exit:

    if (mParent->mEnabled)
    {
        mParent->mScheduleTransmissionTask.Post();
    }
}

void MeshForwarder::SetDiscoverParameters(uint32_t aScanChannels)
{
    mScanChannels = (aScanChannels == 0) ? static_cast<uint32_t>(Mac::kScanChannelsAll) : aScanChannels;
}

void MeshForwarder::HandleDiscoverTimer(Timer &aTimer)
{
    aTimer.GetOwner<MeshForwarder>().HandleDiscoverTimer();
}

void MeshForwarder::HandleDiscoverTimer(void)
{
    ThreadNetif &netif = GetNetif();

    do
    {
        mScanChannels >>= 1;
        mScanChannel++;

        if (mScanChannel > OT_RADIO_CHANNEL_MAX)
        {
            mSendQueue.Dequeue(*(mDirectSender.mSendMessage));
            mDirectSender.mSendMessage->Free();
            mDirectSender.mSendMessage = NULL;
            netif.GetMac().SetPanChannel(mRestoreChannel);
            netif.GetMac().SetPanId(mRestorePanId);
            mScanning = false;
            netif.GetMle().HandleDiscoverComplete();
            ExitNow();
        }
    } while ((mScanChannels & 1) == 0);

    mDirectSender.mSendMessage->SetDirectTransmission();
    mDirectSender.mMessageNextOffset = 0;

exit:
    mScheduleTransmissionTask.Post();
}

void MeshForwarder::HandleReceivedFrame(Mac::Receiver &aReceiver, otDataIndication &aDataIndication)
{
    aReceiver.GetOwner<MeshForwarder>().HandleReceivedFrame(aDataIndication);
}

void MeshForwarder::HandleReceivedFrame(otDataIndication &aDataIndication)
{
    ThreadNetif &    netif = GetNetif();
    otThreadLinkInfo linkInfo;
    Mac::Address     macDest;
    Mac::Address     macSource;
    uint8_t *        payload;
    uint8_t          payloadLength;
    otError          error = OT_ERROR_NONE;

    if (!mEnabled)
    {
        ExitNow(error = OT_ERROR_INVALID_STATE);
    }

    static_cast<Mac::FullAddr *>(&aDataIndication.mSrc)->GetAddress(macSource);
    static_cast<Mac::FullAddr *>(&aDataIndication.mDst)->GetAddress(macDest);

    linkInfo.mPanId        = Encoding::LittleEndian::ReadUint16(aDataIndication.mSrc.mPanId);
    linkInfo.mChannel      = netif.GetMac().GetPanChannel();
    linkInfo.mRss          = aDataIndication.mMpduLinkQuality;
    linkInfo.mLqi          = aDataIndication.mMpduLinkQuality;
    linkInfo.mLinkSecurity = (aDataIndication.mSecurity.mSecurityLevel > 0);

    payload       = aDataIndication.mMsdu;
    payloadLength = aDataIndication.mMsduLength;

    netif.GetSupervisionListener().UpdateOnReceive(macSource, linkInfo.mLinkSecurity);

    if (payloadLength >= sizeof(Lowpan::MeshHeader) && reinterpret_cast<Lowpan::MeshHeader *>(payload)->IsMeshHeader())
    {
        HandleMesh(payload, payloadLength, macSource, linkInfo);
    }
    else if (payloadLength >= sizeof(Lowpan::FragmentHeader) &&
             reinterpret_cast<Lowpan::FragmentHeader *>(payload)->IsFragmentHeader())
    {
        HandleFragment(payload, payloadLength, macSource, macDest, linkInfo);
    }
    else if (payloadLength >= 1 && Lowpan::Lowpan::IsLowpanHc(payload))
    {
        HandleLowpanHC(payload, payloadLength, macSource, macDest, linkInfo);
    }
    else
    {
        VerifyOrExit(payloadLength == 0, error = OT_ERROR_NOT_LOWPAN_DATA_FRAME);

        otLogInfoMac(GetInstance(), "Received empty payload frame");
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogInfoMac(GetInstance(), "Dropping rx frame, error:%s", otThreadErrorToString(error));
    }
}

otError MeshForwarder::SkipMeshHeader(const uint8_t *&aFrame, uint8_t &aFrameLength)
{
    otError            error = OT_ERROR_NONE;
    Lowpan::MeshHeader meshHeader;

    VerifyOrExit(aFrameLength >= 1 && reinterpret_cast<const Lowpan::MeshHeader *>(aFrame)->IsMeshHeader());

    SuccessOrExit(error = meshHeader.Init(aFrame, aFrameLength));
    aFrame += meshHeader.GetHeaderLength();
    aFrameLength -= meshHeader.GetHeaderLength();

exit:
    return error;
}

otError MeshForwarder::DecompressIp6Header(const uint8_t *     aFrame,
                                           uint8_t             aFrameLength,
                                           const Mac::Address &aMacSource,
                                           const Mac::Address &aMacDest,
                                           Ip6::Header &       aIp6Header,
                                           uint8_t &           aHeaderLength,
                                           bool &              aNextHeaderCompressed)
{
    Lowpan::Lowpan &       lowpan = GetNetif().GetLowpan();
    otError                error  = OT_ERROR_NONE;
    const uint8_t *        start  = aFrame;
    Lowpan::FragmentHeader fragmentHeader;
    int                    headerLength;

    SuccessOrExit(error = SkipMeshHeader(aFrame, aFrameLength));

    if (aFrameLength >= 1 && reinterpret_cast<const Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
    {
        SuccessOrExit(error = fragmentHeader.Init(aFrame, aFrameLength));

        // only the first fragment header is followed by a LOWPAN_IPHC header
        VerifyOrExit(fragmentHeader.GetDatagramOffset() == 0, error = OT_ERROR_NOT_FOUND);

        aFrame += fragmentHeader.GetHeaderLength();
        aFrameLength -= fragmentHeader.GetHeaderLength();
    }

    VerifyOrExit(aFrameLength >= 1 && Lowpan::Lowpan::IsLowpanHc(aFrame), error = OT_ERROR_NOT_FOUND);
    headerLength =
        lowpan.DecompressBaseHeader(aIp6Header, aNextHeaderCompressed, aMacSource, aMacDest, aFrame, aFrameLength);

    VerifyOrExit(headerLength > 0, error = OT_ERROR_PARSE);
    aHeaderLength = static_cast<uint8_t>(aFrame - start) + static_cast<uint8_t>(headerLength);

exit:
    return error;
}

void MeshForwarder::HandleFragment(uint8_t *               aFrame,
                                   uint8_t                 aFrameLength,
                                   const Mac::Address &    aMacSource,
                                   const Mac::Address &    aMacDest,
                                   const otThreadLinkInfo &aLinkInfo)
{
    ThreadNetif &          netif = GetNetif();
    otError                error = OT_ERROR_NONE;
    Lowpan::FragmentHeader fragmentHeader;
    Message *              message = NULL;
    int                    headerLength;

    // Check the fragment header
    VerifyOrExit(fragmentHeader.Init(aFrame, aFrameLength) == OT_ERROR_NONE, error = OT_ERROR_DROP);
    aFrame += fragmentHeader.GetHeaderLength();
    aFrameLength -= fragmentHeader.GetHeaderLength();

    if (fragmentHeader.GetDatagramOffset() == 0)
    {
        VerifyOrExit((message = GetInstance().GetMessagePool().New(Message::kTypeIp6, 0)) != NULL,
                     error = OT_ERROR_NO_BUFS);
        message->SetLinkSecurityEnabled(aLinkInfo.mLinkSecurity);
        message->SetPanId(aLinkInfo.mPanId);
        message->AddRss(aLinkInfo.mRss);
        headerLength = netif.GetLowpan().Decompress(*message, aMacSource, aMacDest, aFrame, aFrameLength,
                                                    fragmentHeader.GetDatagramSize());
        VerifyOrExit(headerLength > 0, error = OT_ERROR_PARSE);

        aFrame += headerLength;
        aFrameLength -= static_cast<uint8_t>(headerLength);

        VerifyOrExit(fragmentHeader.GetDatagramSize() >= message->GetOffset() + aFrameLength, error = OT_ERROR_PARSE);

        SuccessOrExit(error = message->SetLength(fragmentHeader.GetDatagramSize()));

        message->SetDatagramTag(fragmentHeader.GetDatagramTag());
        message->SetTimeout(kReassemblyTimeout);

        // copy Fragment
        message->Write(message->GetOffset(), aFrameLength, aFrame);
        message->MoveOffset(aFrameLength);

        // Security Check
        VerifyOrExit(netif.GetIp6Filter().Accept(*message), error = OT_ERROR_DROP);

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
        for (message = mReassemblyList.GetHead(); message; message = message->GetNext())
        {
            // Security Check: only consider reassembly buffers that had the same Security Enabled setting.
            if (message->GetLength() == fragmentHeader.GetDatagramSize() &&
                message->GetDatagramTag() == fragmentHeader.GetDatagramTag() &&
                message->GetOffset() == fragmentHeader.GetDatagramOffset() &&
                message->GetOffset() + aFrameLength <= fragmentHeader.GetDatagramSize() &&
                message->IsLinkSecurityEnabled() == aLinkInfo.mLinkSecurity)
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
            if ((message == NULL) && (aLinkInfo.mLinkSecurity))
            {
                ClearReassemblyList();
            }
        }

        VerifyOrExit(message != NULL, error = OT_ERROR_DROP);

        // copy Fragment
        message->Write(message->GetOffset(), aFrameLength, aFrame);
        message->MoveOffset(aFrameLength);
        message->AddRss(aLinkInfo.mRss);
    }

exit:

    if (error == OT_ERROR_NONE)
    {
        if (message->GetOffset() >= message->GetLength())
        {
            mReassemblyList.Dequeue(*message);
            HandleDatagram(*message, aLinkInfo, aMacSource);
        }
        else if (netif.GetMac().GetRxOnWhenIdle() == false)
        {
            // Implementation specific optimisation
            // Send another poll quickly to speed up long 6lowpan packet assembly
            mDataPollManager.SendFastPolls(1);
        }
    }
    else
    {
        otLogInfoMac(GetInstance(),
                     "Dropping rx frag frame, error:%s, len:%d, src:%s, dst:%s, tag:%d, offset:%d, dglen:%d, sec:%s",
                     otThreadErrorToString(error), aFrameLength, aMacSource.ToString().AsCString(),
                     aMacDest.ToString().AsCString(), fragmentHeader.GetDatagramTag(),
                     fragmentHeader.GetDatagramOffset(), fragmentHeader.GetDatagramSize(),
                     aLinkInfo.mLinkSecurity ? "yes" : "no");

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

        LogIp6Message(kMessageReassemblyDrop, *message, NULL, OT_ERROR_NO_FRAME_RECEIVED);
        mIpCounters.mRxFailure++;

        message->Free();
    }
}

void MeshForwarder::HandleReassemblyTimer(Timer &aTimer)
{
    aTimer.GetOwner<MeshForwarder>().HandleReassemblyTimer();
}

void MeshForwarder::HandleReassemblyTimer(void)
{
    Message *next = NULL;
    uint8_t  timeout;

    for (Message *message = mReassemblyList.GetHead(); message; message = next)
    {
        next    = message->GetNext();
        timeout = message->GetTimeout();

        if (timeout > 0)
        {
            message->SetTimeout(timeout - 1);
        }
        else
        {
            mReassemblyList.Dequeue(*message);

            LogIp6Message(kMessageReassemblyDrop, *message, NULL, OT_ERROR_REASSEMBLY_TIMEOUT);
            mIpCounters.mRxFailure++;

            message->Free();
        }
    }

    if (mReassemblyList.GetHead() != NULL)
    {
        mReassemblyTimer.Start(kStateUpdatePeriod);
    }
}

void MeshForwarder::HandleLowpanHC(uint8_t *               aFrame,
                                   uint8_t                 aFrameLength,
                                   const Mac::Address &    aMacSource,
                                   const Mac::Address &    aMacDest,
                                   const otThreadLinkInfo &aLinkInfo)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Message *    message;
    int          headerLength;

    VerifyOrExit((message = GetInstance().GetMessagePool().New(Message::kTypeIp6, 0)) != NULL,
                 error = OT_ERROR_NO_BUFS);
    message->SetLinkSecurityEnabled(aLinkInfo.mLinkSecurity);
    message->SetPanId(aLinkInfo.mPanId);
    message->AddRss(aLinkInfo.mRss);

    headerLength = netif.GetLowpan().Decompress(*message, aMacSource, aMacDest, aFrame, aFrameLength, 0);
    VerifyOrExit(headerLength > 0, error = OT_ERROR_PARSE);

    aFrame += headerLength;
    aFrameLength -= static_cast<uint8_t>(headerLength);

    SuccessOrExit(error = message->SetLength(message->GetLength() + aFrameLength));
    message->Write(message->GetOffset(), aFrameLength, aFrame);

    // Security Check
    VerifyOrExit(netif.GetIp6Filter().Accept(*message), error = OT_ERROR_DROP);

exit:

    if (error == OT_ERROR_NONE)
    {
        HandleDatagram(*message, aLinkInfo, aMacSource);
    }
    else
    {
        otLogInfoMac(GetInstance(), "Dropping rx lowpan HC frame, error:%s, len:%d, src:%s, dst:%s, sec:%s",
                     otThreadErrorToString(error), aFrameLength, aMacSource.ToString().AsCString(),
                     aMacDest.ToString().AsCString(), aLinkInfo.mLinkSecurity ? "yes" : "no");

        if (message != NULL)
        {
            message->Free();
        }
    }
}

otError MeshForwarder::HandleDatagram(Message &               aMessage,
                                      const otThreadLinkInfo &aLinkInfo,
                                      const Mac::Address &    aMacSource)
{
    ThreadNetif &netif = GetNetif();

    LogIp6Message(kMessageReceive, aMessage, &aMacSource, OT_ERROR_NONE);
    mIpCounters.mRxSuccess++;

    return netif.GetIp6().HandleDatagram(aMessage, &netif, netif.GetInterfaceId(), &aLinkInfo, false);
}

void MeshForwarder::HandleDataPollTimeout(Mac::Receiver &aReceiver)
{
    aReceiver.GetOwner<MeshForwarder>().GetDataPollManager().HandlePollTimeout();
}

Mac::Sender *MeshForwarder::GetFreeFloatingSender(MeshSender *aSender)
{
    Mac::Sender *sender = NULL;

    for (int i = 0; i < kNumFloatingSenders; i++)
    {
        if (mFloatingMacSenders[i].IsInUse())
            continue;
        if (mFloatingMacSenders[i].GetMeshSender() != NULL)
            continue;

        otLogDebgMac(GetInstance, "Claiming floating sender %d for MeshSender %d", i, aSender);
        sender = &mFloatingMacSenders[i];
        sender->SetMeshSender(aSender);
        break;
    }

    return sender;
}

Mac::Sender *MeshForwarder::GetIdleFloatingSender(MeshSender *aSender)
{
    Mac::Sender *sender = NULL;

    for (int i = 0; i < kNumFloatingSenders; i++)
    {
        if (mFloatingMacSenders[i].IsInUse())
            continue;
        if (mFloatingMacSenders[i].GetMeshSender() != aSender)
            continue;

        sender = &mFloatingMacSenders[i];
        break;
    }

    return sender;
}

void MeshForwarder::ReleaseFloatingSenders(MeshSender *aSender)
{
    for (int i = 0; i < kNumFloatingSenders; i++)
    {
        Mac::Sender &sender = mFloatingMacSenders[i];

        if (sender.GetMeshSender() != aSender)
            continue;

        otLogDebgMac(GetInstance, "Releasing floating sender %d from MeshSender %d", i, aSender);

        if (sender.IsInUse())
            GetNetif().GetMac().PurgeFrameRequest(sender);

        sender.SetMeshSender(NULL);
    }
}

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void MeshForwarder::LogIp6Message(MessageAction       aAction,
                                  const Message &     aMessage,
                                  const Mac::Address *aMacAddress,
                                  otError             aError)
{
    uint16_t     checksum = 0;
    Ip6::Header  ip6Header;
    Ip6::IpProto protocol;
    const char * actionText;
    const char * priorityText;
    bool         shouldLogRss             = false;
    bool         shouldLogSrcDstAddresses = true;

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
        actionText   = "Received";
        shouldLogRss = true;
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
        actionText               = "Preping indir tx";
        shouldLogSrcDstAddresses = false;
        break;

    case kMessageDrop:
        actionText = "Dropping";
        break;

    case kMessageReassemblyDrop:
        actionText   = "Dropping (reassembly timeout)";
        shouldLogRss = true;
        break;

    case kMessageEvict:
        actionText = "Evicting";
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
    otLogInfoMac(GetInstance(), "%s IPv6 %s msg, len:%d, chksum:%04x%s%s, sec:%s%s%s, prio:%s%s%s", actionText,
                 Ip6::Ip6::IpProtoToString(protocol), aMessage.GetLength(), checksum,
                 (aMacAddress == NULL) ? "" : ((aAction == kMessageReceive) ? ", from:" : ", to:"),
                 (aMacAddress == NULL) ? "" : aMacAddress->ToString().AsCString(),
                 aMessage.IsLinkSecurityEnabled() ? "yes" : "no", (aError == OT_ERROR_NONE) ? "" : ", error:",
                 (aError == OT_ERROR_NONE) ? "" : otThreadErrorToString(aError), priorityText,
                 shouldLogRss ? ", rss:" : "", shouldLogRss ? aMessage.GetRssAverager().ToString().AsCString() : "");

    if (shouldLogSrcDstAddresses)
    {
        otLogInfoMac(GetInstance(), "src: %s", ip6Header.GetSource().ToString().AsCString());
        otLogInfoMac(GetInstance(), "dst: %s", ip6Header.GetDestination().ToString().AsCString());
    }

exit:
    return;
} // namespace ot

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void MeshForwarder::LogIp6Message(MessageAction, const Message &, const Mac::Address *, otError)
{
}

#endif //#if OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO

} // namespace ot

#endif
