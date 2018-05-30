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

#include "mesh_forwarder.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/owner-locator.hpp"
#include "common/random.hpp"
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

MeshForwarder::MeshForwarder(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mMacReceiver(&MeshForwarder::HandleReceivedFrame, &MeshForwarder::HandleDataPollTimeout, this)
    , mMacSender(&MeshForwarder::HandleFrameRequest, &MeshForwarder::HandleSentFrame, this)
    , mDiscoverTimer(aInstance, &MeshForwarder::HandleDiscoverTimer, this)
    , mReassemblyTimer(aInstance, &MeshForwarder::HandleReassemblyTimer, this)
    , mMessageNextOffset(0)
    , mSendMessage(NULL)
    , mSendMessageIsARetransmission(false)
    , mSendMessageMaxMacTxAttempts(Mac::kDirectFrameMacTxAttempts)
    , mMeshSource()
    , mMeshDest()
    , mAddMeshHeader(false)
    , mSendBusy(false)
    , mScheduleTransmissionTask(aInstance, ScheduleTransmissionTask, this)
    , mEnabled(false)
    , mScanChannels(0)
    , mScanChannel(0)
    , mMacRadioAcquisitionId(0)
    , mRestorePanId(Mac::kPanIdBroadcast)
    , mScanning(false)
#if OPENTHREAD_FTD
    , mSourceMatchController(aInstance)
    , mSendMessageFrameCounter(0)
    , mSendMessageKeyId(0)
    , mSendMessageDataSequenceNumber(0)
    , mIndirectStartingChild(NULL)
#endif
    , mDataPollManager(aInstance)
{
    mFragTag = Random::GetUint16();
    GetNetif().GetMac().RegisterReceiver(mMacReceiver);

    mIpCounters.mTxSuccess = 0;
    mIpCounters.mRxSuccess = 0;
    mIpCounters.mTxFailure = 0;
    mIpCounters.mRxFailure = 0;
}

otError MeshForwarder::Start(void)
{
    otError error = OT_ERROR_NONE;

    if (mEnabled == false)
    {
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

    mDataPollManager.StopPolling();
    mReassemblyTimer.Stop();

    if (mScanning)
    {
        if (mMacRadioAcquisitionId)
        {
            netif.GetMac().ReleaseRadioChannel();
            mMacRadioAcquisitionId = 0;
        }

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

    mEnabled     = false;
    mSendMessage = NULL;
    netif.GetMac().SetRxOnWhenIdle(false);

exit:
    return error;
}

void MeshForwarder::RemoveMessage(Message &aMessage)
{
    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateAnyExceptInvalid); !iter.IsDone(); iter.Advance())
    {
        IgnoreReturnValue(RemoveMessageFromSleepyChild(aMessage, *iter.GetChild()));
    }

    if (mSendMessage == &aMessage)
    {
        mSendMessage = NULL;
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
    VerifyOrExit(mSendBusy == false);

    mSendMessageIsARetransmission = false;

    if (GetIndirectTransmission() == OT_ERROR_NONE)
    {
        ExitNow();
    }

    if ((mSendMessage = GetDirectTransmission()) != NULL)
    {
        if (mSendMessage->GetOffset() == 0)
        {
            mSendMessage->SetTxSuccess(true);
        }

        mSendMessageMaxMacTxAttempts = Mac::kDirectFrameMacTxAttempts;
        GetNetif().GetMac().SendFrameRequest(mMacSender);
        ExitNow();
    }

exit:
    return;
}

otError MeshForwarder::PrepareDiscoverRequest(void)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;

    VerifyOrExit(!mScanning);

    mScanChannel = OT_RADIO_CHANNEL_MIN;
    mScanChannels >>= OT_RADIO_CHANNEL_MIN;
    mRestorePanId = netif.GetMac().GetPanId();

    SuccessOrExit(error = netif.GetMac().AcquireRadioChannel(&mMacRadioAcquisitionId));

    while ((mScanChannels & 1) == 0)
    {
        mScanChannels >>= 1;
        mScanChannel++;

        if (mScanChannel > OT_RADIO_CHANNEL_MAX)
        {
            if (mMacRadioAcquisitionId)
            {
                netif.GetMac().ReleaseRadioChannel();
                mMacRadioAcquisitionId = 0;
            }

            netif.GetMle().HandleDiscoverComplete();
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
            error = UpdateIp6Route(*curMessage);

            if (curMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
            {
                error = PrepareDiscoverRequest();
            }

            break;

        case Message::kTypeMacDataPoll:
            error = PrepareDataPoll();
            break;

#if OPENTHREAD_FTD

        case Message::kType6lowpan:
            error = UpdateMeshRoute(*curMessage);
            break;

#endif

        default:
            error = OT_ERROR_DROP;
            break;
        }

        switch (error)
        {
        case OT_ERROR_NONE:
            ExitNow();

#if OPENTHREAD_FTD

        case OT_ERROR_ADDRESS_QUERY:
            mSendQueue.Dequeue(*curMessage);
            mResolvingQueue.Enqueue(*curMessage);
            continue;

#endif

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

otError MeshForwarder::PrepareDataPoll(void)
{
    otError      error  = OT_ERROR_NONE;
    ThreadNetif &netif  = GetNetif();
    Neighbor *   parent = netif.GetMle().GetParentCandidate();
    uint16_t     shortAddress;

    VerifyOrExit((parent != NULL) && parent->IsStateValidOrRestoring(), error = OT_ERROR_DROP);

    shortAddress = netif.GetMac().GetShortAddress();

    if ((shortAddress == Mac::kShortAddrInvalid) || (parent != netif.GetMle().GetParent()))
    {
        mMacSource.SetExtended(netif.GetMac().GetExtAddress());
        mMacDest.SetExtended(parent->GetExtAddress());
    }
    else
    {
        mMacSource.SetShort(shortAddress);
        mMacDest.SetShort(parent->GetRloc16());
    }

exit:
    return error;
}

otError MeshForwarder::UpdateIp6Route(Message &aMessage)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Ip6::Header  ip6Header;

    mAddMeshHeader = false;

    aMessage.Read(0, sizeof(ip6Header), &ip6Header);

    VerifyOrExit(!ip6Header.GetSource().IsMulticast(), error = OT_ERROR_DROP);

    // 1. Choose correct MAC Source Address.
    GetMacSourceAddress(ip6Header.GetSource(), mMacSource);

    // 2. Choose correct MAC Destination Address.
    if (netif.GetMle().GetRole() == OT_DEVICE_ROLE_DISABLED || netif.GetMle().GetRole() == OT_DEVICE_ROLE_DETACHED)
    {
        // Allow only for link-local unicasts and multicasts.
        if (ip6Header.GetDestination().IsLinkLocal() || ip6Header.GetDestination().IsLinkLocalMulticast())
        {
            GetMacDestinationAddress(ip6Header.GetDestination(), mMacDest);
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
            mMacDest.SetShort(netif.GetMle().GetNextHop(Mac::kShortAddrBroadcast));
        }
        else
        {
            mMacDest.SetShort(Mac::kShortAddrBroadcast);
        }
    }
    else if (ip6Header.GetDestination().IsLinkLocal())
    {
        GetMacDestinationAddress(ip6Header.GetDestination(), mMacDest);
    }
    else if (netif.GetMle().IsMinimalEndDevice())
    {
        mMacDest.SetShort(netif.GetMle().GetNextHop(Mac::kShortAddrBroadcast));
    }
    else
    {
#if OPENTHREAD_FTD
        error = UpdateIp6RouteFtd(ip6Header);
#else
        assert(false);
#endif
    }

exit:
    return error;
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

otError MeshForwarder::HandleFrameRequest(Mac::Sender &aSender, Mac::Frame &aFrame)
{
    return aSender.GetOwner<MeshForwarder>().HandleFrameRequest(aFrame);
}

otError MeshForwarder::HandleFrameRequest(Mac::Frame &aFrame)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;

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
            SuccessOrExit(error = netif.GetMac().SetRadioChannel(mMacRadioAcquisitionId, mScanChannel));

            aFrame.SetChannel(mScanChannel);

            // In case a specific PAN ID of a Thread Network to be discovered is not known, Discovery
            // Request messages MUST have the Destination PAN ID in the IEEE 802.15.4 MAC header set
            // to be the Broadcast PAN ID (0xFFFF) and the Source PAN ID set to a randomly generated
            // value.
            if (mSendMessage->GetPanId() == Mac::kPanIdBroadcast && netif.GetMac().GetPanId() == Mac::kPanIdBroadcast)
            {
                uint16_t panid;

                do
                {
                    panid = Random::GetUint16();
                } while (panid == Mac::kPanIdBroadcast);

                netif.GetMac().SetPanId(panid);
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

    case Message::kTypeMacDataPoll:
        error = SendPoll(*mSendMessage, aFrame);
        break;

#if OPENTHREAD_FTD

    case Message::kType6lowpan:
        error = SendMesh(*mSendMessage, aFrame);
        break;

    case Message::kTypeSupervision:
        error              = SendEmptyFrame(aFrame, kSupervisionMsgAckRequest);
        mMessageNextOffset = mSendMessage->GetLength();
        break;

#endif
    }

    assert(error == OT_ERROR_NONE);

    aFrame.SetIsARetransmission(mSendMessageIsARetransmission);
    aFrame.SetMaxTxAttempts(mSendMessageMaxMacTxAttempts);

#if OPENTHREAD_FTD

    {
        Mac::Address macDest;
        Child *      child = NULL;

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

        aFrame.GetDstAddr(macDest);

        // Set `FramePending` if there are more queued messages (excluding
        // the current one being sent out) for the child (note `> 1` check).
        // The case where the current message requires fragmentation is
        // already checked and handled in `SendFragment()` method.

        child = netif.GetMle().GetChildTable().FindChild(macDest, ChildTable::kInStateValidOrRestoring);

        if ((child != NULL) && !child->IsRxOnWhenIdle() && (child->GetIndirectMessageCount() > 1))
        {
            aFrame.SetFramePending(true);
        }
    }

#endif

exit:
    return error;
}

otError MeshForwarder::SendPoll(Message &aMessage, Mac::Frame &aFrame)
{
    ThreadNetif &netif = GetNetif();
    uint16_t     fcf;

    // initialize MAC header
    fcf = Mac::Frame::kFcfFrameMacCmd | Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfFrameVersion2006;

    if (mMacSource.IsShort())
    {
        fcf |= Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort;
    }
    else
    {
        fcf |= Mac::Frame::kFcfDstAddrExt | Mac::Frame::kFcfSrcAddrExt;
    }

    fcf |= Mac::Frame::kFcfAckRequest | Mac::Frame::kFcfSecurityEnabled;

    aFrame.InitMacHeader(fcf, Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32);
    aFrame.SetDstPanId(netif.GetMac().GetPanId());
    aFrame.SetSrcAddr(mMacSource);
    aFrame.SetDstAddr(mMacDest);
    aFrame.SetCommandId(Mac::Frame::kMacCmdDataRequest);

    mMessageNextOffset = aMessage.GetLength();

    return OT_ERROR_NONE;
}

otError MeshForwarder::SendFragment(Message &aMessage, Mac::Frame &aFrame)
{
    ThreadNetif &           netif = GetNetif();
    Mac::Address            meshDest, meshSource;
    uint16_t                fcf;
    Lowpan::FragmentHeader *fragmentHeader;
    uint8_t *               payload;
    uint8_t                 headerLength;
    uint16_t                payloadLength;
    int                     hcLength;
    uint16_t                fragmentLength;
    uint16_t                dstpan;
    uint8_t                 secCtl = Mac::Frame::kSecNone;
    otError                 error  = OT_ERROR_NONE;

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

    // initialize MAC header
    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfFrameVersion2006;
    fcf |= (mMacDest.IsShort()) ? Mac::Frame::kFcfDstAddrShort : Mac::Frame::kFcfDstAddrExt;
    fcf |= (mMacSource.IsShort()) ? Mac::Frame::kFcfSrcAddrShort : Mac::Frame::kFcfSrcAddrExt;

    // all unicast frames request ACK
    if (mMacDest.IsExtended() || !mMacDest.IsBroadcast())
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

    dstpan = netif.GetMac().GetPanId();

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

    if (dstpan == netif.GetMac().GetPanId())
    {
        fcf |= Mac::Frame::kFcfPanidCompression;
    }

    aFrame.InitMacHeader(fcf, secCtl);
    aFrame.SetDstPanId(dstpan);
    aFrame.SetSrcPanId(netif.GetMac().GetPanId());
    aFrame.SetDstAddr(mMacDest);
    aFrame.SetSrcAddr(mMacSource);

    payload = aFrame.GetPayload();

    headerLength = 0;

#if OPENTHREAD_FTD

    // initialize Mesh header
    if (mAddMeshHeader)
    {
        Lowpan::MeshHeader meshHeader;
        uint8_t            hopsLeft;

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

#endif

    // copy IPv6 Header
    if (aMessage.GetOffset() == 0)
    {
        hcLength = netif.GetLowpan().Compress(aMessage, meshSource, meshDest, payload);
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
    ThreadNetif &netif = GetNetif();
    uint16_t     fcf;
    uint8_t      secCtl;
    Mac::Address macSource;

    macSource.SetShort(netif.GetMac().GetShortAddress());

    if (macSource.IsShortAddrInvalid())
    {
        macSource.SetExtended(netif.GetMac().GetExtAddress());
    }

    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfFrameVersion2006;
    fcf |= (mMacDest.IsShort()) ? Mac::Frame::kFcfDstAddrShort : Mac::Frame::kFcfDstAddrExt;
    fcf |= (macSource.IsShort()) ? Mac::Frame::kFcfSrcAddrShort : Mac::Frame::kFcfSrcAddrExt;

    if (aAckRequest)
    {
        fcf |= Mac::Frame::kFcfAckRequest;
    }

    fcf |= Mac::Frame::kFcfSecurityEnabled;
    secCtl = Mac::Frame::kKeyIdMode1;
    secCtl |= Mac::Frame::kSecEncMic32;

    fcf |= Mac::Frame::kFcfPanidCompression;

    aFrame.InitMacHeader(fcf, secCtl);

    aFrame.SetDstPanId(netif.GetMac().GetPanId());
    aFrame.SetSrcPanId(netif.GetMac().GetPanId());
    aFrame.SetDstAddr(mMacDest);
    aFrame.SetSrcAddr(macSource);
    aFrame.SetPayloadLength(0);
    aFrame.SetFramePending(false);

    return OT_ERROR_NONE;
}

void MeshForwarder::HandleSentFrame(Mac::Sender &aSender, Mac::Frame &aFrame, otError aError)
{
    aSender.GetOwner<MeshForwarder>().HandleSentFrame(aFrame, aError);
}

void MeshForwarder::HandleSentFrame(Mac::Frame &aFrame, otError aError)
{
    ThreadNetif &netif = GetNetif();
    Mac::Address macDest;
    Neighbor *   neighbor;

    mSendBusy = false;

    VerifyOrExit(mEnabled);

    if (mSendMessage != NULL)
    {
        mSendMessage->SetOffset(mMessageNextOffset);
    }

    aFrame.GetDstAddr(macDest);

    if ((neighbor = netif.GetMle().GetNeighbor(macDest)) != NULL)
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

    HandleSentFrameToChild(aFrame, aError, macDest);

    VerifyOrExit(mSendMessage != NULL);

    if (mSendMessage->GetDirectTransmission())
    {
        if (aError != OT_ERROR_NONE)
        {
            // If the transmission of any fragment frame fails,
            // the overall message transmission is considered
            // as failed

            mSendMessage->SetTxSuccess(false);

#if OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE

            // We set the NextOffset to end of message to avoid sending
            // any remaining fragments in the message.

            mMessageNextOffset = mSendMessage->GetLength();
#endif
        }

        if (mMessageNextOffset < mSendMessage->GetLength())
        {
            mSendMessage->SetOffset(mMessageNextOffset);
        }
        else
        {
            mSendMessage->ClearDirectTransmission();
            mSendMessage->SetOffset(0);

            if (neighbor != NULL)
            {
                neighbor->GetLinkInfo().AddMessageTxStatus(mSendMessage->GetTxSuccess());
            }
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
        neighbor = netif.GetMle().GetParentCandidate();

        if (neighbor->GetState() == Neighbor::kStateInvalid)
        {
            mDataPollManager.StopPolling();
            netif.GetMle().BecomeDetached();
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
        mSendMessage       = NULL;
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
            mSendQueue.Dequeue(*mSendMessage);
            mSendMessage->Free();
            mSendMessage = NULL;

            if (mMacRadioAcquisitionId)
            {
                netif.GetMac().ReleaseRadioChannel();
                mMacRadioAcquisitionId = 0;
            }

            netif.GetMac().SetPanId(mRestorePanId);
            mScanning = false;
            netif.GetMle().HandleDiscoverComplete();
            ExitNow();
        }
    } while ((mScanChannels & 1) == 0);

    mSendMessage->SetDirectTransmission();

exit:
    mSendBusy = false;
    mScheduleTransmissionTask.Post();
}

void MeshForwarder::HandleReceivedFrame(Mac::Receiver &aReceiver, Mac::Frame &aFrame)
{
    aReceiver.GetOwner<MeshForwarder>().HandleReceivedFrame(aFrame);
}

void MeshForwarder::HandleReceivedFrame(Mac::Frame &aFrame)
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

    SuccessOrExit(error = aFrame.GetSrcAddr(macSource));
    SuccessOrExit(error = aFrame.GetDstAddr(macDest));

    aFrame.GetSrcPanId(linkInfo.mPanId);
    linkInfo.mChannel      = aFrame.GetChannel();
    linkInfo.mRss          = aFrame.GetRssi();
    linkInfo.mLqi          = aFrame.GetLqi();
    linkInfo.mLinkSecurity = aFrame.GetSecurityEnabled();

    payload       = aFrame.GetPayload();
    payloadLength = aFrame.GetPayloadLength();

    netif.GetSupervisionListener().UpdateOnReceive(macSource, linkInfo.mLinkSecurity);

    switch (aFrame.GetType())
    {
    case Mac::Frame::kFcfFrameData:
        if (payloadLength >= sizeof(Lowpan::MeshHeader) &&
            reinterpret_cast<Lowpan::MeshHeader *>(payload)->IsMeshHeader())
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

            LogFrame("Received empty payload frame", aFrame, OT_ERROR_NONE);
        }

        break;

#if OPENTHREAD_FTD

    case Mac::Frame::kFcfFrameMacCmd:
    {
        uint8_t commandId;

        aFrame.GetCommandId(commandId);

        if (commandId == Mac::Frame::kMacCmdDataRequest)
        {
            HandleDataRequest(macSource, linkInfo);
        }
        else
        {
            error = OT_ERROR_DROP;
        }

        break;
    }

#endif

    default:
        error = OT_ERROR_DROP;
        break;
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        LogFrame("Dropping rx frame", aFrame, error);
    }
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
    }
    else
    {
        LogFragmentFrameDrop(error, aFrameLength, aMacSource, aMacDest, fragmentHeader, aLinkInfo.mLinkSecurity);

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
    ThreadNetif &netif   = GetNetif();
    otError      error   = OT_ERROR_NONE;
    Message *    message = NULL;
    int          headerLength;

#if OPENTHREAD_FTD
    UpdateRoutes(aFrame, aFrameLength, aMacSource, aMacDest);
#endif

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
        LogLowpanHcFrameDrop(error, aFrameLength, aMacSource, aMacDest, aLinkInfo.mLinkSecurity);

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

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

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
    bool         shouldLogSrcDstAddresses = (OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES != 0);
    char         stringBuffer[Ip6::Address::kIp6AddressStringSize];
    char         rssString[RssAverager::kStringSize];

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
        actionText               = "Prepping indir tx";
        shouldLogSrcDstAddresses = false;
        break;

    case kMessageDrop:
        actionText = "Dropping";
        break;

    case kMessageReassemblyDrop:
        actionText   = "Dropping (reassembly queue)";
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
                 (aMacAddress == NULL) ? "" : aMacAddress->ToString(stringBuffer, sizeof(stringBuffer)),
                 aMessage.IsLinkSecurityEnabled() ? "yes" : "no", (aError == OT_ERROR_NONE) ? "" : ", error:",
                 (aError == OT_ERROR_NONE) ? "" : otThreadErrorToString(aError), priorityText,
                 shouldLogRss ? ", rss:" : "",
                 shouldLogRss ? aMessage.GetRssAverager().ToString(rssString, sizeof(rssString)) : "");

    if (shouldLogSrcDstAddresses)
    {
        otLogInfoMac(GetInstance(), "src: %s", ip6Header.GetSource().ToString(stringBuffer, sizeof(stringBuffer)));
        otLogInfoMac(GetInstance(), "dst: %s", ip6Header.GetDestination().ToString(stringBuffer, sizeof(stringBuffer)));
    }

exit:
    return;
}

void MeshForwarder::LogFrame(const char *aActionText, const Mac::Frame &aFrame, otError aError)
{
    char stringBuffer[Mac::Frame::kInfoStringSize];

    if (aError != OT_ERROR_NONE)
    {
        otLogInfoMac(GetInstance(), "%s, aError:%s, %s", aActionText, otThreadErrorToString(aError),
                     aFrame.ToInfoString(stringBuffer, sizeof(stringBuffer)));
    }
    else
    {
        otLogInfoMac(GetInstance(), "%s, %s", aActionText, aFrame.ToInfoString(stringBuffer, sizeof(stringBuffer)));
    }
}

void MeshForwarder::LogFragmentFrameDrop(otError                       aError,
                                         uint8_t                       aFrameLength,
                                         const Mac::Address &          aMacSource,
                                         const Mac::Address &          aMacDest,
                                         const Lowpan::FragmentHeader &aFragmentHeader,
                                         bool                          aIsSecure)
{
    char srcStringBuffer[Mac::Address::kAddressStringSize];
    char dstStringBuffer[Mac::Address::kAddressStringSize];

    otLogInfoMac(
        GetInstance(), "Dropping rx frag frame, error:%s, len:%d, src:%s, dst:%s, tag:%d, offset:%d, dglen:%d, sec:%s",
        otThreadErrorToString(aError), aFrameLength, aMacSource.ToString(srcStringBuffer, sizeof(srcStringBuffer)),
        aMacDest.ToString(dstStringBuffer, sizeof(dstStringBuffer)), aFragmentHeader.GetDatagramTag(),
        aFragmentHeader.GetDatagramOffset(), aFragmentHeader.GetDatagramSize(), aIsSecure ? "yes" : "no");
}

void MeshForwarder::LogLowpanHcFrameDrop(otError             aError,
                                         uint8_t             aFrameLength,
                                         const Mac::Address &aMacSource,
                                         const Mac::Address &aMacDest,
                                         bool                aIsSecure)
{
    char srcStringBuffer[Mac::Address::kAddressStringSize];
    char dstStringBuffer[Mac::Address::kAddressStringSize];

    otLogInfoMac(GetInstance(), "Dropping rx lowpan HC frame, error:%s, len:%d, src:%s, dst:%s, sec:%s",
                 otThreadErrorToString(aError), aFrameLength,
                 aMacSource.ToString(srcStringBuffer, sizeof(srcStringBuffer)),
                 aMacDest.ToString(dstStringBuffer, sizeof(dstStringBuffer)), aIsSecure ? "yes" : "no");
}

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void MeshForwarder::LogIp6Message(MessageAction, const Message &, const Mac::Address *, otError)
{
}

void MeshForwarder::LogFrame(const char *, const Mac::Frame &, otError)
{
}

void MeshForwarder::LogFragmentFrameDrop(otError,
                                         uint8_t,
                                         const Mac::Address &,
                                         const Mac::Address &,
                                         const Lowpan::FragmentHeader &,
                                         bool)
{
}

void MeshForwarder::LogLowpanHcFrameDrop(otError, uint8_t, const Mac::Address &, const Mac::Address &, bool)
{
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_INFO) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

} // namespace ot
