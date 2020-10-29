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

#include "mesh_forwarder.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "common/time_ticker.hpp"
#include "net/ip6.hpp"
#include "net/ip6_filter.hpp"
#include "net/netif.hpp"
#include "net/tcp.hpp"
#include "net/udp6.hpp"
#include "radio/radio.hpp"
#include "thread/mle.hpp"
#include "thread/mle_router.hpp"
#include "thread/thread_netif.hpp"

namespace ot {

void ThreadLinkInfo::SetFrom(const Mac::RxFrame &aFrame)
{
    Clear();

    if (OT_ERROR_NONE != aFrame.GetSrcPanId(mPanId))
    {
        IgnoreError(aFrame.GetDstPanId(mPanId));
    }

    mChannel      = aFrame.GetChannel();
    mRss          = aFrame.GetRssi();
    mLqi          = aFrame.GetLqi();
    mLinkSecurity = aFrame.GetSecurityEnabled();
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    if (aFrame.GetTimeIe() != nullptr)
    {
        mNetworkTimeOffset = aFrame.ComputeNetworkTimeOffset();
        mTimeSyncSeq       = aFrame.ReadTimeSyncSeq();
    }
#endif
#if OPENTHREAD_CONFIG_MULTI_RADIO
    mRadioType = static_cast<uint8_t>(aFrame.GetRadioType());
#endif
}

MeshForwarder::MeshForwarder(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mMessageNextOffset(0)
    , mSendMessage(nullptr)
    , mMeshSource()
    , mMeshDest()
    , mAddMeshHeader(false)
    , mEnabled(false)
    , mTxPaused(false)
    , mSendBusy(false)
    , mScheduleTransmissionTask(aInstance, MeshForwarder::ScheduleTransmissionTask, this)
#if OPENTHREAD_FTD
    , mIndirectSender(aInstance)
#endif
    , mDataPollSender(aInstance)
{
    mFragTag = Random::NonCrypto::GetUint16();

    ResetCounters();

#if OPENTHREAD_FTD
    mFragmentPriorityList.Clear();
#endif
}

void MeshForwarder::Start(void)
{
    if (!mEnabled)
    {
        Get<Mac::Mac>().SetRxOnWhenIdle(true);
#if OPENTHREAD_FTD
        mIndirectSender.Start();
#endif

        mEnabled = true;
    }
}

void MeshForwarder::Stop(void)
{
    Message *message;

    VerifyOrExit(mEnabled);

    mDataPollSender.StopPolling();
    Get<TimeTicker>().UnregisterReceiver(TimeTicker::kMeshForwarder);
    Get<Mle::DiscoverScanner>().Stop();

    while ((message = mSendQueue.GetHead()) != nullptr)
    {
        mSendQueue.Dequeue(*message);
        message->Free();
    }

    while ((message = mReassemblyList.GetHead()) != nullptr)
    {
        mReassemblyList.Dequeue(*message);
        message->Free();
    }

#if OPENTHREAD_FTD
    mIndirectSender.Stop();
    mFragmentPriorityList.Clear();
#endif

    mEnabled     = false;
    mSendMessage = nullptr;
    Get<Mac::Mac>().SetRxOnWhenIdle(false);

exit:
    return;
}

void MeshForwarder::PrepareEmptyFrame(Mac::TxFrame &aFrame, const Mac::Address &aMacDest, bool aAckRequest)
{
    uint16_t fcf = 0;

    Mac::Address macSource;
    macSource.SetShort(Get<Mac::Mac>().GetShortAddress());

    if (macSource.IsShortAddrInvalid() || aMacDest.IsExtended())
    {
        macSource.SetExtended(Get<Mac::Mac>().GetExtAddress());
    }

    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfSecurityEnabled;

    if (aAckRequest)
    {
        fcf |= Mac::Frame::kFcfAckRequest;
    }

    fcf |= (aMacDest.IsShort()) ? Mac::Frame::kFcfDstAddrShort : Mac::Frame::kFcfDstAddrExt;
    fcf |= (macSource.IsShort()) ? Mac::Frame::kFcfSrcAddrShort : Mac::Frame::kFcfSrcAddrExt;
    Get<Mac::Mac>().UpdateFrameControlField(Get<NeighborTable>().FindNeighbor(aMacDest), false, fcf);

    aFrame.InitMacHeader(fcf, Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32);

    if (aFrame.IsDstPanIdPresent())
    {
        aFrame.SetDstPanId(Get<Mac::Mac>().GetPanId());
    }
    IgnoreError(aFrame.SetSrcPanId(Get<Mac::Mac>().GetPanId()));

    aFrame.SetDstAddr(aMacDest);
    aFrame.SetSrcAddr(macSource);
    aFrame.SetFramePending(false);
#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    IgnoreError(Get<Mac::Mac>().AppendHeaderIe(false, aFrame));
#endif
    aFrame.SetPayloadLength(0);
}

void MeshForwarder::RemoveMessage(Message &aMessage)
{
    PriorityQueue *queue = aMessage.GetPriorityQueue();

    OT_ASSERT(queue != nullptr);

    if (queue == &mSendQueue)
    {
#if OPENTHREAD_FTD
        for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
        {
            IgnoreError(mIndirectSender.RemoveMessageFromSleepyChild(aMessage, child));
        }
#endif

        if (mSendMessage == &aMessage)
        {
            mSendMessage = nullptr;
        }
    }

    queue->Dequeue(aMessage);
    LogMessage(kMessageEvict, aMessage, nullptr, OT_ERROR_NO_BUFS);
    aMessage.Free();
}

void MeshForwarder::ResumeMessageTransmissions(void)
{
    if (mTxPaused)
    {
        mTxPaused = false;
        mScheduleTransmissionTask.Post();
    }
}

void MeshForwarder::ScheduleTransmissionTask(Tasklet &aTasklet)
{
    aTasklet.GetOwner<MeshForwarder>().ScheduleTransmissionTask();
}

void MeshForwarder::ScheduleTransmissionTask(void)
{
    VerifyOrExit(!mSendBusy && !mTxPaused);

    mSendMessage = GetDirectTransmission();
    VerifyOrExit(mSendMessage != nullptr);

    if (mSendMessage->GetOffset() == 0)
    {
        mSendMessage->SetTxSuccess(true);
    }

    Get<Mac::Mac>().RequestDirectFrameTransmission();

exit:
    return;
}

Message *MeshForwarder::GetDirectTransmission(void)
{
    Message *curMessage, *nextMessage;
    otError  error = OT_ERROR_NONE;

    for (curMessage = mSendQueue.GetHead(); curMessage; curMessage = nextMessage)
    {
        if (!curMessage->GetDirectTransmission())
        {
            nextMessage = curMessage->GetNext();
            continue;
        }

        curMessage->SetDoNotEvict(true);

        switch (curMessage->GetType())
        {
        case Message::kTypeIp6:
            error = UpdateIp6Route(*curMessage);
            break;

#if OPENTHREAD_FTD

        case Message::kType6lowpan:
            error = UpdateMeshRoute(*curMessage);
            break;

#endif

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
        case Message::kTypeMacEmptyData:
            error = OT_ERROR_NONE;
            break;
#endif

        default:
            error = OT_ERROR_DROP;
            break;
        }

        curMessage->SetDoNotEvict(false);

        // the next message may have been evicted during processing (e.g. due to Address Solicit)
        nextMessage = curMessage->GetNext();

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

        default:
            mSendQueue.Dequeue(*curMessage);
            LogMessage(kMessageDrop, *curMessage, nullptr, error);
            curMessage->Free();
            continue;
        }
    }

exit:
    return curMessage;
}

otError MeshForwarder::UpdateIp6Route(Message &aMessage)
{
    Mle::MleRouter &mle   = Get<Mle::MleRouter>();
    otError         error = OT_ERROR_NONE;
    Ip6::Header     ip6Header;

    mAddMeshHeader = false;

    IgnoreError(aMessage.Read(0, ip6Header));

    VerifyOrExit(!ip6Header.GetSource().IsMulticast(), error = OT_ERROR_DROP);

    GetMacSourceAddress(ip6Header.GetSource(), mMacSource);

    if (mle.IsDisabled() || mle.IsDetached())
    {
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
        // With the exception of MLE multicasts, an End Device
        // transmits multicasts, as IEEE 802.15.4 unicasts to its
        // parent.

        if (mle.IsChild() && !aMessage.IsSubTypeMle())
        {
            mMacDest.SetShort(mle.GetNextHop(Mac::kShortAddrBroadcast));
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
    else if (mle.IsMinimalEndDevice())
    {
        mMacDest.SetShort(mle.GetNextHop(Mac::kShortAddrBroadcast));
    }
    else
    {
#if OPENTHREAD_FTD
        error = UpdateIp6RouteFtd(ip6Header, aMessage);
#else
        OT_ASSERT(false);
#endif
    }

exit:
    return error;
}

bool MeshForwarder::GetRxOnWhenIdle(void) const
{
    return Get<Mac::Mac>().GetRxOnWhenIdle();
}

void MeshForwarder::SetRxOnWhenIdle(bool aRxOnWhenIdle)
{
    Get<Mac::Mac>().SetRxOnWhenIdle(aRxOnWhenIdle);

    if (aRxOnWhenIdle)
    {
        mDataPollSender.StopPolling();
        Get<Utils::SupervisionListener>().Stop();
    }
    else
    {
        mDataPollSender.StartPolling();
        Get<Utils::SupervisionListener>().Start();
    }
}

void MeshForwarder::GetMacSourceAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    aIp6Addr.GetIid().ConvertToMacAddress(aMacAddr);

    if (aMacAddr.GetExtended() != Get<Mac::Mac>().GetExtAddress())
    {
        aMacAddr.SetShort(Get<Mac::Mac>().GetShortAddress());
    }
}

void MeshForwarder::GetMacDestinationAddress(const Ip6::Address &aIp6Addr, Mac::Address &aMacAddr)
{
    if (aIp6Addr.IsMulticast())
    {
        aMacAddr.SetShort(Mac::kShortAddrBroadcast);
    }
    else if (Get<Mle::MleRouter>().IsRoutingLocator(aIp6Addr))
    {
        aMacAddr.SetShort(aIp6Addr.GetIid().GetLocator());
    }
    else
    {
        aIp6Addr.GetIid().ConvertToMacAddress(aMacAddr);
    }
}

otError MeshForwarder::DecompressIp6Header(const uint8_t *     aFrame,
                                           uint16_t            aFrameLength,
                                           const Mac::Address &aMacSource,
                                           const Mac::Address &aMacDest,
                                           Ip6::Header &       aIp6Header,
                                           uint8_t &           aHeaderLength,
                                           bool &              aNextHeaderCompressed)
{
    otError                error = OT_ERROR_NONE;
    const uint8_t *        start = aFrame;
    Lowpan::FragmentHeader fragmentHeader;
    uint16_t               fragmentHeaderLength;
    int                    headerLength;

    if (fragmentHeader.ParseFrom(aFrame, aFrameLength, fragmentHeaderLength) == OT_ERROR_NONE)
    {
        // Only the first fragment header is followed by a LOWPAN_IPHC header
        VerifyOrExit(fragmentHeader.GetDatagramOffset() == 0, error = OT_ERROR_NOT_FOUND);
        aFrame += fragmentHeaderLength;
        aFrameLength -= fragmentHeaderLength;
    }

    VerifyOrExit(aFrameLength >= 1 && Lowpan::Lowpan::IsLowpanHc(aFrame), error = OT_ERROR_NOT_FOUND);
    headerLength = Get<Lowpan::Lowpan>().DecompressBaseHeader(aIp6Header, aNextHeaderCompressed, aMacSource, aMacDest,
                                                              aFrame, aFrameLength);

    VerifyOrExit(headerLength > 0, error = OT_ERROR_PARSE);
    aHeaderLength = static_cast<uint8_t>(aFrame - start) + static_cast<uint8_t>(headerLength);

exit:
    return error;
}

Mac::TxFrame *MeshForwarder::HandleFrameRequest(Mac::TxFrames &aTxFrames)
{
    Mac::TxFrame *frame         = nullptr;
    bool          addFragHeader = false;

    VerifyOrExit(mEnabled && (mSendMessage != nullptr));

#if OPENTHREAD_CONFIG_MULTI_RADIO
    frame = &Get<RadioSelector>().SelectRadio(*mSendMessage, mMacDest, aTxFrames);

    // If multi-radio link is supported, when sending frame with link
    // security enabled, Fragment Header is always included (even if
    // the message is small and does not require 6LoWPAN fragmentation).
    // This allows the Fragment Header's tag to be used to detect and
    // suppress duplicate received frames over different radio links.

    if (mSendMessage->IsLinkSecurityEnabled())
    {
        addFragHeader = true;
    }
#else
    frame = &aTxFrames.GetTxFrame();
#endif

    mSendBusy = true;

    switch (mSendMessage->GetType())
    {
    case Message::kTypeIp6:
        if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
        {
            frame = Get<Mle::DiscoverScanner>().PrepareDiscoveryRequestFrame(*frame);
            VerifyOrExit(frame != nullptr);
        }
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
        if (Get<Mac::Mac>().IsCslEnabled() && mSendMessage->IsSubTypeMle())
        {
            mSendMessage->SetLinkSecurityEnabled(true);
        }
#endif
        mMessageNextOffset = PrepareDataFrame(*frame, *mSendMessage, mMacSource, mMacDest, mAddMeshHeader, mMeshSource,
                                              mMeshDest, addFragHeader);

        if ((mSendMessage->GetSubType() == Message::kSubTypeMleChildIdRequest) && mSendMessage->IsLinkSecurityEnabled())
        {
            otLogNoteMac("Child ID Request requires fragmentation, aborting tx");
            mMessageNextOffset = mSendMessage->GetLength();
            ExitNow(frame = nullptr);
        }

        OT_ASSERT(frame->GetLength() != 7);
        break;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    case Message::kTypeMacEmptyData:
    {
        Mac::Address macDestAddr;

        macDestAddr.SetShort(Get<Mle::MleRouter>().GetParent().GetRloc16());
        PrepareEmptyFrame(*frame, macDestAddr, /* aAckRequest */ true);
    }
    break;
#endif

#if OPENTHREAD_FTD

    case Message::kType6lowpan:
        SendMesh(*mSendMessage, *frame);
        break;

    case Message::kTypeSupervision:
        // A direct supervision message is possible in the case where
        // a sleepy child switches its mode (becomes non-sleepy) while
        // there is a pending indirect supervision message in the send
        // queue for it. The message would be then converted to a
        // direct tx.

        // Fall through
#endif

    default:
        mMessageNextOffset = mSendMessage->GetLength();
        ExitNow(frame = nullptr);
    }

    frame->SetIsARetransmission(false);

exit:
    return frame;
}

// This method constructs a MAC data from from a given IPv6 message.
//
// This method handles generation of MAC header, mesh header (if
// requested), lowpan compression of IPv6 header, lowpan fragmentation
// header (if message requires fragmentation or if it is explicitly
// requested by setting `aAddFragHeader` to `true`) It uses the
// message offset to construct next fragments. This method enables
// link security when message is MLE type and requires fragmentation.
// It returns the next offset into the message after the prepared
// frame.
//
uint16_t MeshForwarder::PrepareDataFrame(Mac::TxFrame &      aFrame,
                                         Message &           aMessage,
                                         const Mac::Address &aMacSource,
                                         const Mac::Address &aMacDest,
                                         bool                aAddMeshHeader,
                                         uint16_t            aMeshSource,
                                         uint16_t            aMeshDest,
                                         bool                aAddFragHeader)
{
    uint16_t fcf;
    uint8_t *payload;
    uint8_t  headerLength;
    uint16_t maxPayloadLength;
    uint16_t payloadLength;
    uint16_t fragmentLength;
    uint16_t dstpan;
    uint8_t  secCtl;
    uint16_t nextOffset;

start:

    // Initialize MAC header
    fcf = Mac::Frame::kFcfFrameData;

    Get<Mac::Mac>().UpdateFrameControlField(Get<NeighborTable>().FindNeighbor(aMacDest), aMessage.IsTimeSync(), fcf);

    fcf |= (aMacDest.IsShort()) ? Mac::Frame::kFcfDstAddrShort : Mac::Frame::kFcfDstAddrExt;
    fcf |= (aMacSource.IsShort()) ? Mac::Frame::kFcfSrcAddrShort : Mac::Frame::kFcfSrcAddrExt;

    // All unicast frames request ACK
    if (aMacDest.IsExtended() || !aMacDest.IsBroadcast())
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
    else
    {
        secCtl = Mac::Frame::kSecNone;
    }

    dstpan = Get<Mac::Mac>().GetPanId();

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

    if (dstpan == Get<Mac::Mac>().GetPanId())
    {
        fcf |= Mac::Frame::kFcfPanidCompression;
    }

    aFrame.InitMacHeader(fcf, secCtl);

    if (aFrame.IsDstPanIdPresent())
    {
        aFrame.SetDstPanId(dstpan);
    }

    IgnoreError(aFrame.SetSrcPanId(Get<Mac::Mac>().GetPanId()));
    aFrame.SetDstAddr(aMacDest);
    aFrame.SetSrcAddr(aMacSource);

#if OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
    IgnoreError(Get<Mac::Mac>().AppendHeaderIe(aMessage.IsTimeSync(), aFrame));
#endif

    payload          = aFrame.GetPayload();
    maxPayloadLength = aFrame.GetMaxPayloadLength();

    headerLength = 0;

#if OPENTHREAD_FTD

    // Initialize Mesh header
    if (aAddMeshHeader)
    {
        Mle::MleRouter &   mle = Get<Mle::MleRouter>();
        Lowpan::MeshHeader meshHeader;
        uint16_t           meshHeaderLength;
        uint8_t            hopsLeft;

        // Mesh Header frames are forwarded by routers over multiple
        // hops to reach a final destination. The forwarding path can
        // have routers supporting different radio links with varying
        // MTU sizes. Since the originator of the frame does not know the
        // path and the MTU sizes of supported radio links by the routers
        // in the path, we limit the max payload length of a Mesh Header
        // frame to a fixed minimum value (derived from 15.4 radio)
        // ensuring it can be handled by any radio link.
        //
        // Maximum payload length is calculated by subtracting the frame
        // header and footer lengths from the MTU size. The footer
        // length is derived by removing the `aFrame.GetFcsSize()` and
        // then adding the fixed `kMeshHeaderFrameFcsSize` instead
        // (updating the FCS size in the calculation of footer length).

        maxPayloadLength = kMeshHeaderFrameMtu - aFrame.GetHeaderLength() -
                           (aFrame.GetFooterLength() - aFrame.GetFcsSize() + kMeshHeaderFrameFcsSize);

        if (mle.IsChild())
        {
            // REED sets hopsLeft to max (16) + 1. It does not know the route cost.
            hopsLeft = Mle::kMaxRouteCost + 1;
        }
        else
        {
            // Calculate the number of predicted hops.
            hopsLeft = mle.GetRouteCost(aMeshDest);

            if (hopsLeft != Mle::kMaxRouteCost)
            {
                hopsLeft += mle.GetLinkCost(Mle::Mle::RouterIdFromRloc16(mle.GetNextHop(aMeshDest)));
            }
            else
            {
                // In case there is no route to the destination router (only link).
                hopsLeft = mle.GetLinkCost(Mle::Mle::RouterIdFromRloc16(aMeshDest));
            }
        }

        // The hopsLft field MUST be incremented by one if the
        // destination RLOC16 is not that of an active Router.
        if (!Mle::Mle::IsActiveRouter(aMeshDest))
        {
            hopsLeft += 1;
        }

        meshHeader.Init(aMeshSource, aMeshDest, hopsLeft + Lowpan::MeshHeader::kAdditionalHopsLeft);
        meshHeaderLength = meshHeader.WriteTo(payload);
        payload += meshHeaderLength;
        headerLength += meshHeaderLength;
    }

#endif

    // Compress IPv6 Header
    if (aMessage.GetOffset() == 0)
    {
        Lowpan::BufferWriter buffer(payload,
                                    maxPayloadLength - headerLength - Lowpan::FragmentHeader::kFirstFragmentHeaderSize);
        uint8_t              hcLength;
        Mac::Address         meshSource, meshDest;
        otError              error;

        OT_UNUSED_VARIABLE(error);

        if (aAddMeshHeader)
        {
            meshSource.SetShort(aMeshSource);
            meshDest.SetShort(aMeshDest);
        }
        else
        {
            meshSource = aMacSource;
            meshDest   = aMacDest;
        }

        error = Get<Lowpan::Lowpan>().Compress(aMessage, meshSource, meshDest, buffer);
        OT_ASSERT(error == OT_ERROR_NONE);

        hcLength = static_cast<uint8_t>(buffer.GetWritePointer() - payload);
        headerLength += hcLength;
        payloadLength  = aMessage.GetLength() - aMessage.GetOffset();
        fragmentLength = maxPayloadLength - headerLength;

        if ((payloadLength > fragmentLength) || aAddFragHeader)
        {
            Lowpan::FragmentHeader fragmentHeader;

            if ((!aMessage.IsLinkSecurityEnabled()) && aMessage.IsSubTypeMle())
            {
                // Enable security and try again.
                aMessage.SetOffset(0);
                aMessage.SetLinkSecurityEnabled(true);
                goto start;
            }

            // Write Fragment header
            if (aMessage.GetDatagramTag() == 0)
            {
                // Avoid using datagram tag value 0, which indicates the tag has not been set
                if (mFragTag == 0)
                {
                    mFragTag++;
                }

                aMessage.SetDatagramTag(mFragTag++);
            }

            memmove(payload + Lowpan::FragmentHeader::kFirstFragmentHeaderSize, payload, hcLength);

            fragmentHeader.InitFirstFragment(aMessage.GetLength(), static_cast<uint16_t>(aMessage.GetDatagramTag()));
            fragmentHeader.WriteTo(payload);

            payload += Lowpan::FragmentHeader::kFirstFragmentHeaderSize;
            headerLength += Lowpan::FragmentHeader::kFirstFragmentHeaderSize;

            fragmentLength = maxPayloadLength - headerLength;

            if (payloadLength > fragmentLength)
            {
                payloadLength = fragmentLength & ~0x7;
            }
        }

        payload += hcLength;

        // copy IPv6 Payload
        aMessage.ReadBytes(aMessage.GetOffset(), payload, payloadLength);
        aFrame.SetPayloadLength(headerLength + payloadLength);

        nextOffset = aMessage.GetOffset() + payloadLength;
        aMessage.SetOffset(0);
    }
    else
    {
        Lowpan::FragmentHeader fragmentHeader;
        uint16_t               fragmentHeaderLength;

        payloadLength = aMessage.GetLength() - aMessage.GetOffset();

        // Write Fragment header
        fragmentHeader.Init(aMessage.GetLength(), static_cast<uint16_t>(aMessage.GetDatagramTag()),
                            aMessage.GetOffset());
        fragmentHeaderLength = fragmentHeader.WriteTo(payload);

        payload += fragmentHeaderLength;
        headerLength += fragmentHeaderLength;

        fragmentLength = maxPayloadLength - headerLength;

        if (payloadLength > fragmentLength)
        {
            payloadLength = (fragmentLength & ~0x7);
        }

        // Copy IPv6 Payload
        aMessage.ReadBytes(aMessage.GetOffset(), payload, payloadLength);
        aFrame.SetPayloadLength(headerLength + payloadLength);

        nextOffset = aMessage.GetOffset() + payloadLength;
    }

    if (nextOffset < aMessage.GetLength())
    {
        aFrame.SetFramePending(true);
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
        aMessage.SetTimeSync(false);
#endif
    }

    return nextOffset;
}

Neighbor *MeshForwarder::UpdateNeighborOnSentFrame(Mac::TxFrame &aFrame, otError aError, const Mac::Address &aMacDest)
{
    Neighbor *neighbor = nullptr;

    VerifyOrExit(mEnabled);

    neighbor = Get<NeighborTable>().FindNeighbor(aMacDest);
    VerifyOrExit(neighbor != nullptr);

    VerifyOrExit(aFrame.GetAckRequest());

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
    // TREL radio link uses deferred ack model. We ignore
    // `SendDone` event from `Mac` layer with success status and
    // wait for deferred ack callback instead.
#if OPENTHREAD_CONFIG_MULTI_RADIO
    if (aFrame.GetRadioType() == Mac::kRadioTypeTrel)
#endif
    {
        VerifyOrExit(aError != OT_ERROR_NONE);
    }
#endif // OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

    UpdateNeighborLinkFailures(*neighbor, aError, /* aAllowNeighborRemove */ true);

exit:
    return neighbor;
}

void MeshForwarder::UpdateNeighborLinkFailures(Neighbor &aNeighbor, otError aError, bool aAllowNeighborRemove)
{
    // Update neighbor `LinkFailures` counter on ack error.

    if (aError == OT_ERROR_NONE)
    {
        aNeighbor.ResetLinkFailures();
    }
    else if (aError == OT_ERROR_NO_ACK)
    {
        aNeighbor.IncrementLinkFailures();

        if (aAllowNeighborRemove && (Mle::Mle::IsActiveRouter(aNeighbor.GetRloc16())) &&
            (aNeighbor.GetLinkFailures() >= Mle::kFailedRouterTransmissions))
        {
            Get<Mle::MleRouter>().RemoveRouterLink(static_cast<Router &>(aNeighbor));
        }
    }
}

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
void MeshForwarder::HandleDeferredAck(Neighbor &aNeighbor, otError aError)
{
    bool allowNeighborRemove = true;

    VerifyOrExit(mEnabled);

    if (aError == OT_ERROR_NO_ACK)
    {
        otLogInfoMac("Deferred ack timeout on trel for neighbor %s rloc16:0x%04x",
                     aNeighbor.GetExtAddress().ToString().AsCString(), aNeighbor.GetRloc16());
    }

#if OPENTHREAD_CONFIG_MULTI_RADIO
    // In multi radio mode, `RadioSelector` will update the neighbor's
    // link failure counter and removes the neighbor if required.
    Get<RadioSelector>().UpdateOnDeferredAck(aNeighbor, aError, allowNeighborRemove);
#else
    UpdateNeighborLinkFailures(aNeighbor, aError, allowNeighborRemove);
#endif

exit:
    return;
}
#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

void MeshForwarder::HandleSentFrame(Mac::TxFrame &aFrame, otError aError)
{
    Neighbor *   neighbor = nullptr;
    Mac::Address macDest;

    OT_ASSERT((aError == OT_ERROR_NONE) || (aError == OT_ERROR_CHANNEL_ACCESS_FAILURE) || (aError == OT_ERROR_ABORT) ||
              (aError == OT_ERROR_NO_ACK));

    mSendBusy = false;

    VerifyOrExit(mEnabled);

    if (!aFrame.IsEmpty())
    {
        IgnoreError(aFrame.GetDstAddr(macDest));
        neighbor = UpdateNeighborOnSentFrame(aFrame, aError, macDest);
    }

    UpdateSendMessage(aError, macDest, neighbor);

exit:
    return;
}

void MeshForwarder::UpdateSendMessage(otError aFrameTxError, Mac::Address &aMacDest, Neighbor *aNeighbor)
{
    VerifyOrExit(mSendMessage != nullptr);

    OT_ASSERT(mSendMessage->GetDirectTransmission());

    if (aFrameTxError != OT_ERROR_NONE)
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
        otError txError = aFrameTxError;

        mSendMessage->ClearDirectTransmission();
        mSendMessage->SetOffset(0);

        if (aNeighbor != nullptr)
        {
            aNeighbor->GetLinkInfo().AddMessageTxStatus(mSendMessage->GetTxSuccess());
        }

#if !OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE

        // When `CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE` is
        // disabled, all fragment frames of a larger message are
        // sent even if the transmission of an earlier fragment fail.
        // Note that `GetTxSuccess() tracks the tx success of the
        // entire message, while `aFrameTxError` represents the error
        // status of the last fragment frame transmission.

        if (!mSendMessage->GetTxSuccess() && (txError == OT_ERROR_NONE))
        {
            txError = OT_ERROR_FAILED;
        }
#endif

        LogMessage(kMessageTransmit, *mSendMessage, &aMacDest, txError);

        if (mSendMessage->GetType() == Message::kTypeIp6)
        {
            if (mSendMessage->GetTxSuccess())
            {
                mIpCounters.mTxSuccess++;
            }
            else
            {
                mIpCounters.mTxFailure++;
            }
        }
    }

    if (mSendMessage->GetSubType() == Message::kSubTypeMleDiscoverRequest)
    {
        Get<Mle::DiscoverScanner>().HandleDiscoveryRequestFrameTxDone(*mSendMessage);
    }

    if (!mSendMessage->GetDirectTransmission() && !mSendMessage->IsChildPending())
    {
        if (mSendMessage->GetSubType() == Message::kSubTypeMleChildIdRequest && mSendMessage->IsLinkSecurityEnabled())
        {
            // If the Child ID Request requires fragmentation and therefore
            // link layer security, the frame transmission will be aborted.
            // When the message is being freed, we signal to MLE to prepare a
            // shorter Child ID Request message (by only including mesh-local
            // address in the Address Registration TLV).

            otLogInfoMac("Requesting shorter `Child ID Request`");
            Get<Mle::Mle>().RequestShorterChildIdRequest();
        }

        mSendQueue.Dequeue(*mSendMessage);
        mSendMessage->Free();
        mSendMessage       = nullptr;
        mMessageNextOffset = 0;
    }

exit:
    mScheduleTransmissionTask.Post();
}

void MeshForwarder::HandleReceivedFrame(Mac::RxFrame &aFrame)
{
    ThreadLinkInfo linkInfo;
    Mac::Address   macDest;
    Mac::Address   macSource;
    uint8_t *      payload;
    uint16_t       payloadLength;
    otError        error = OT_ERROR_NONE;

    VerifyOrExit(mEnabled, error = OT_ERROR_INVALID_STATE);

    SuccessOrExit(error = aFrame.GetSrcAddr(macSource));
    SuccessOrExit(error = aFrame.GetDstAddr(macDest));

    linkInfo.SetFrom(aFrame);

    payload       = aFrame.GetPayload();
    payloadLength = aFrame.GetPayloadLength();

    Get<Utils::SupervisionListener>().UpdateOnReceive(macSource, linkInfo.IsLinkSecurityEnabled());

    switch (aFrame.GetType())
    {
    case Mac::Frame::kFcfFrameData:
        if (Lowpan::MeshHeader::IsMeshHeader(payload, payloadLength))
        {
#if OPENTHREAD_FTD
            HandleMesh(payload, payloadLength, macSource, linkInfo);
#endif
        }
        else if (Lowpan::FragmentHeader::IsFragmentHeader(payload, payloadLength))
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

    case Mac::Frame::kFcfFrameBeacon:
        break;

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

void MeshForwarder::HandleFragment(const uint8_t *       aFrame,
                                   uint16_t              aFrameLength,
                                   const Mac::Address &  aMacSource,
                                   const Mac::Address &  aMacDest,
                                   const ThreadLinkInfo &aLinkInfo)
{
    otError                error = OT_ERROR_NONE;
    Lowpan::FragmentHeader fragmentHeader;
    uint16_t               fragmentHeaderLength;
    Message *              message = nullptr;

    // Check the fragment header
    SuccessOrExit(error = fragmentHeader.ParseFrom(aFrame, aFrameLength, fragmentHeaderLength));
    aFrame += fragmentHeaderLength;
    aFrameLength -= fragmentHeaderLength;

#if OPENTHREAD_CONFIG_MULTI_RADIO

    if (aLinkInfo.mLinkSecurity)
    {
        Neighbor *neighbor = Get<NeighborTable>().FindNeighbor(aMacSource, Neighbor::kInStateAnyExceptInvalid);

        if (neighbor != nullptr)
        {
            uint16_t tag = fragmentHeader.GetDatagramTag();

            if (neighbor->IsLastRxFragmentTagSet())
            {
                VerifyOrExit(!neighbor->IsLastRxFragmentTagAfter(tag), error = OT_ERROR_DUPLICATED);

                if (neighbor->GetLastRxFragmentTag() == tag)
                {
                    VerifyOrExit(fragmentHeader.GetDatagramOffset() != 0, error = OT_ERROR_DUPLICATED);

                    // Duplication suppression for a "next fragment" is handled
                    // by the code below where the the datagram offset is
                    // checked against the offset of the corresponding message
                    // (same datagram tag and size) in Reassembly List. Note
                    // that if there is no matching message in the Reassembly
                    // List (e.g., in case the message is already fully
                    // assembled) the received "next fragment" frame would be
                    // dropped.
                }
            }

            neighbor->SetLastRxFragmentTag(tag);
        }
    }

#endif // OPENTHREAD_CONFIG_MULTI_RADIO

    if (fragmentHeader.GetDatagramOffset() == 0)
    {
        uint16_t datagramSize = fragmentHeader.GetDatagramSize();

        error = FrameToMessage(aFrame, aFrameLength, datagramSize, aMacSource, aMacDest, message);
        SuccessOrExit(error);

        VerifyOrExit(datagramSize >= message->GetLength(), error = OT_ERROR_PARSE);
        error = message->SetLength(datagramSize);
        SuccessOrExit(error);

        message->SetDatagramTag(fragmentHeader.GetDatagramTag());
        message->SetTimeout(kReassemblyTimeout);
        message->SetLinkInfo(aLinkInfo);

        VerifyOrExit(Get<Ip6::Filter>().Accept(*message), error = OT_ERROR_DROP);

#if OPENTHREAD_FTD
        SendIcmpErrorIfDstUnreach(*message, aMacSource, aMacDest);
#endif

        // Allow re-assembly of only one message at a time on a SED by clearing
        // any remaining fragments in reassembly list upon receiving of a new
        // (secure) first fragment.

        if (!GetRxOnWhenIdle() && message->IsLinkSecurityEnabled())
        {
            ClearReassemblyList();
        }

        mReassemblyList.Enqueue(*message);

        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMeshForwarder);
    }
    else // Received frame is a "next fragment".
    {
        for (message = mReassemblyList.GetHead(); message; message = message->GetNext())
        {
            // Security Check: only consider reassembly buffers that had the same Security Enabled setting.
            if (message->GetLength() == fragmentHeader.GetDatagramSize() &&
                message->GetDatagramTag() == fragmentHeader.GetDatagramTag() &&
                message->GetOffset() == fragmentHeader.GetDatagramOffset() &&
                message->GetOffset() + aFrameLength <= fragmentHeader.GetDatagramSize() &&
                message->IsLinkSecurityEnabled() == aLinkInfo.IsLinkSecurityEnabled())
            {
                break;
            }
        }

        // For a sleepy-end-device, if we receive a new (secure) next fragment
        // with a non-matching fragmentation offset or tag, it indicates that
        // we have either missed a fragment, or the parent has moved to a new
        // message with a new tag. In either case, we can safely clear any
        // remaining fragments stored in the reassembly list.

        if (!GetRxOnWhenIdle() && (message == nullptr) && aLinkInfo.IsLinkSecurityEnabled())
        {
            ClearReassemblyList();
        }

        VerifyOrExit(message != nullptr, error = OT_ERROR_DROP);

        message->WriteBytes(message->GetOffset(), aFrame, aFrameLength);
        message->MoveOffset(aFrameLength);
        message->AddRss(aLinkInfo.GetRss());
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
        message->AddLqi(aLinkInfo.GetLqi());
#endif
        message->SetTimeout(kReassemblyTimeout);
    }

exit:

    if (error == OT_ERROR_NONE)
    {
        if (message->GetOffset() >= message->GetLength())
        {
            mReassemblyList.Dequeue(*message);
            IgnoreError(HandleDatagram(*message, aLinkInfo, aMacSource));
        }
    }
    else
    {
        LogFragmentFrameDrop(error, aFrameLength, aMacSource, aMacDest, fragmentHeader,
                             aLinkInfo.IsLinkSecurityEnabled());
        FreeMessage(message);
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

        LogMessage(kMessageReassemblyDrop, *message, nullptr, OT_ERROR_NO_FRAME_RECEIVED);

        if (message->GetType() == Message::kTypeIp6)
        {
            mIpCounters.mRxFailure++;
        }

        message->Free();
    }
}

void MeshForwarder::HandleTimeTick(void)
{
    bool contineRxingTicks = false;

#if OPENTHREAD_FTD
    contineRxingTicks = mFragmentPriorityList.UpdateOnTimeTick();
#endif

    contineRxingTicks = UpdateReassemblyList() || contineRxingTicks;

    if (!contineRxingTicks)
    {
        Get<TimeTicker>().UnregisterReceiver(TimeTicker::kMeshForwarder);
    }
}

bool MeshForwarder::UpdateReassemblyList(void)
{
    Message *next = nullptr;

    for (Message *message = mReassemblyList.GetHead(); message; message = next)
    {
        next = message->GetNext();

        if (message->GetTimeout() > 0)
        {
            message->DecrementTimeout();
        }
        else
        {
            mReassemblyList.Dequeue(*message);

            LogMessage(kMessageReassemblyDrop, *message, nullptr, OT_ERROR_REASSEMBLY_TIMEOUT);
            if (message->GetType() == Message::kTypeIp6)
            {
                mIpCounters.mRxFailure++;
            }

            message->Free();
        }
    }

    return mReassemblyList.GetHead() != nullptr;
}

otError MeshForwarder::FrameToMessage(const uint8_t *     aFrame,
                                      uint16_t            aFrameLength,
                                      uint16_t            aDatagramSize,
                                      const Mac::Address &aMacSource,
                                      const Mac::Address &aMacDest,
                                      Message *&          aMessage)
{
    otError           error = OT_ERROR_NONE;
    int               headerLength;
    Message::Priority priority;

    error = GetFramePriority(aFrame, aFrameLength, aMacSource, aMacDest, priority);
    SuccessOrExit(error);

    aMessage = Get<MessagePool>().New(Message::kTypeIp6, 0, priority);
    VerifyOrExit(aMessage, error = OT_ERROR_NO_BUFS);

    headerLength =
        Get<Lowpan::Lowpan>().Decompress(*aMessage, aMacSource, aMacDest, aFrame, aFrameLength, aDatagramSize);
    VerifyOrExit(headerLength > 0, error = OT_ERROR_PARSE);

    aFrame += headerLength;
    aFrameLength -= static_cast<uint16_t>(headerLength);

    SuccessOrExit(error = aMessage->SetLength(aMessage->GetLength() + aFrameLength));
    aMessage->WriteBytes(aMessage->GetOffset(), aFrame, aFrameLength);
    aMessage->MoveOffset(aFrameLength);

exit:
    return error;
}

void MeshForwarder::HandleLowpanHC(const uint8_t *       aFrame,
                                   uint16_t              aFrameLength,
                                   const Mac::Address &  aMacSource,
                                   const Mac::Address &  aMacDest,
                                   const ThreadLinkInfo &aLinkInfo)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = nullptr;

#if OPENTHREAD_FTD
    UpdateRoutes(aFrame, aFrameLength, aMacSource, aMacDest);
#endif

    SuccessOrExit(error = FrameToMessage(aFrame, aFrameLength, 0, aMacSource, aMacDest, message));

    message->SetLinkInfo(aLinkInfo);

    VerifyOrExit(Get<Ip6::Filter>().Accept(*message), error = OT_ERROR_DROP);

#if OPENTHREAD_FTD
    SendIcmpErrorIfDstUnreach(*message, aMacSource, aMacDest);
#endif

exit:

    if (error == OT_ERROR_NONE)
    {
        IgnoreError(HandleDatagram(*message, aLinkInfo, aMacSource));
    }
    else
    {
        LogLowpanHcFrameDrop(error, aFrameLength, aMacSource, aMacDest, aLinkInfo.IsLinkSecurityEnabled());
        FreeMessage(message);
    }
}

otError MeshForwarder::HandleDatagram(Message &             aMessage,
                                      const ThreadLinkInfo &aLinkInfo,
                                      const Mac::Address &  aMacSource)
{
    ThreadNetif &netif = Get<ThreadNetif>();

    LogMessage(kMessageReceive, aMessage, &aMacSource, OT_ERROR_NONE);

    if (aMessage.GetType() == Message::kTypeIp6)
    {
        mIpCounters.mRxSuccess++;
    }

    return Get<Ip6::Ip6>().HandleDatagram(aMessage, &netif, &aLinkInfo, false);
}

otError MeshForwarder::GetFramePriority(const uint8_t *     aFrame,
                                        uint16_t            aFrameLength,
                                        const Mac::Address &aMacSource,
                                        const Mac::Address &aMacDest,
                                        Message::Priority & aPriority)
{
    otError     error = OT_ERROR_NONE;
    Ip6::Header ip6Header;
    uint16_t    dstPort;
    uint8_t     headerLength;
    bool        nextHeaderCompressed;

    SuccessOrExit(error = DecompressIp6Header(aFrame, aFrameLength, aMacSource, aMacDest, ip6Header, headerLength,
                                              nextHeaderCompressed));
    aPriority = Ip6::Ip6::DscpToPriority(ip6Header.GetDscp());

    aFrame += headerLength;
    aFrameLength -= headerLength;

    switch (ip6Header.GetNextHeader())
    {
    case Ip6::kProtoIcmp6:

        VerifyOrExit(aFrameLength >= sizeof(Ip6::Icmp::Header), error = OT_ERROR_PARSE);

        // Only ICMPv6 error messages are prioritized.
        if (reinterpret_cast<const Ip6::Icmp::Header *>(aFrame)->IsError())
        {
            aPriority = Message::kPriorityNet;
        }

        break;

    case Ip6::kProtoUdp:

        if (nextHeaderCompressed)
        {
            Ip6::Udp::Header udpHeader;

            VerifyOrExit(Get<Lowpan::Lowpan>().DecompressUdpHeader(udpHeader, aFrame, aFrameLength) >= 0,
                         error = OT_ERROR_PARSE);

            dstPort = udpHeader.GetDestinationPort();
        }
        else
        {
            VerifyOrExit(aFrameLength >= sizeof(Ip6::Udp::Header), error = OT_ERROR_PARSE);
            dstPort = reinterpret_cast<const Ip6::Udp::Header *>(aFrame)->GetDestinationPort();
        }

        if ((dstPort == Mle::kUdpPort) || (dstPort == Tmf::kUdpPort))
        {
            aPriority = Message::kPriorityNet;
        }

        break;

    default:
        break;
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
otError MeshForwarder::SendEmptyMessage(void)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = nullptr;

    VerifyOrExit(mEnabled && !Get<Mac::Mac>().GetRxOnWhenIdle() &&
                     Get<Mle::MleRouter>().GetParent().IsStateValidOrRestoring(),
                 error = OT_ERROR_INVALID_STATE);

    message = Get<MessagePool>().New(Message::kTypeMacEmptyData, 0);
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = SendMessage(*message));

exit:
    FreeMessageOnError(message, error);
    otLogDebgMac("Send empty message, error:%s", otThreadErrorToString(error));
    return error;
}
#endif // OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

otError MeshForwarder::ParseIp6UdpTcpHeader(const Message &aMessage,
                                            Ip6::Header &  aIp6Header,
                                            uint16_t &     aChecksum,
                                            uint16_t &     aSourcePort,
                                            uint16_t &     aDestPort)
{
    otError error = OT_ERROR_PARSE;
    union
    {
        Ip6::Udp::Header udp;
        Ip6::Tcp::Header tcp;
    } header;

    aChecksum   = 0;
    aSourcePort = 0;
    aDestPort   = 0;

    SuccessOrExit(aMessage.Read(0, aIp6Header));
    VerifyOrExit(aIp6Header.IsVersion6());

    switch (aIp6Header.GetNextHeader())
    {
    case Ip6::kProtoUdp:
        SuccessOrExit(aMessage.Read(sizeof(Ip6::Header), header.udp));
        aChecksum   = header.udp.GetChecksum();
        aSourcePort = header.udp.GetSourcePort();
        aDestPort   = header.udp.GetDestinationPort();
        break;

    case Ip6::kProtoTcp:
        SuccessOrExit(aMessage.Read(sizeof(Ip6::Header), header.tcp));
        aChecksum   = header.tcp.GetChecksum();
        aSourcePort = header.tcp.GetSourcePort();
        aDestPort   = header.tcp.GetDestinationPort();
        break;

    default:
        break;
    }

    error = OT_ERROR_NONE;

exit:
    return error;
}

const char *MeshForwarder::MessageActionToString(MessageAction aAction, otError aError)
{
    const char *actionText = "";

    switch (aAction)
    {
    case kMessageReceive:
        actionText = "Received";
        break;

    case kMessageTransmit:
        actionText = (aError == OT_ERROR_NONE) ? "Sent" : "Failed to send";
        break;

    case kMessagePrepareIndirect:
        actionText = "Prepping indir tx";
        break;

    case kMessageDrop:
        actionText = "Dropping";
        break;

    case kMessageReassemblyDrop:
        actionText = "Dropping (reassembly queue)";
        break;

    case kMessageEvict:
        actionText = "Evicting";
        break;
    }

    return actionText;
}

const char *MeshForwarder::MessagePriorityToString(const Message &aMessage)
{
    const char *priorityText = "unknown";

    switch (aMessage.GetPriority())
    {
    case Message::kPriorityNet:
        priorityText = "net";
        break;

    case Message::kPriorityHigh:
        priorityText = "high";
        break;

    case Message::kPriorityNormal:
        priorityText = "normal";
        break;

    case Message::kPriorityLow:
        priorityText = "low";
        break;
    }

    return priorityText;
}

#if OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES
void MeshForwarder::LogIp6SourceDestAddresses(Ip6::Header &aIp6Header,
                                              uint16_t     aSourcePort,
                                              uint16_t     aDestPort,
                                              otLogLevel   aLogLevel)
{
    if (aSourcePort != 0)
    {
        otLogMac(aLogLevel, "    src:[%s]:%d", aIp6Header.GetSource().ToString().AsCString(), aSourcePort);
    }
    else
    {
        otLogMac(aLogLevel, "    src:[%s]", aIp6Header.GetSource().ToString().AsCString());
    }

    if (aDestPort != 0)
    {
        otLogMac(aLogLevel, "    dst:[%s]:%d", aIp6Header.GetDestination().ToString().AsCString(), aDestPort);
    }
    else
    {
        otLogMac(aLogLevel, "    dst:[%s]", aIp6Header.GetDestination().ToString().AsCString());
    }
}
#else
void MeshForwarder::LogIp6SourceDestAddresses(Ip6::Header &, uint16_t, uint16_t, otLogLevel)
{
}
#endif

void MeshForwarder::LogIp6Message(MessageAction       aAction,
                                  const Message &     aMessage,
                                  const Mac::Address *aMacAddress,
                                  otError             aError,
                                  otLogLevel          aLogLevel)
{
    Ip6::Header ip6Header;
    uint16_t    checksum;
    uint16_t    sourcePort;
    uint16_t    destPort;
    bool        shouldLogRss;
    bool        shouldLogRadio = false;
    const char *radioString    = "";

    SuccessOrExit(ParseIp6UdpTcpHeader(aMessage, ip6Header, checksum, sourcePort, destPort));

    shouldLogRss = (aAction == kMessageReceive) || (aAction == kMessageReassemblyDrop);

#if OPENTHREAD_CONFIG_MULTI_RADIO
    shouldLogRadio = true;
    radioString    = aMessage.IsRadioTypeSet() ? RadioTypeToString(aMessage.GetRadioType()) : "all";
#endif

    otLogMac(aLogLevel, "%s IPv6 %s msg, len:%d, chksum:%04x%s%s, sec:%s%s%s, prio:%s%s%s%s%s",
             MessageActionToString(aAction, aError), Ip6::Ip6::IpProtoToString(ip6Header.GetNextHeader()),
             aMessage.GetLength(), checksum,
             (aMacAddress == nullptr) ? "" : ((aAction == kMessageReceive) ? ", from:" : ", to:"),
             (aMacAddress == nullptr) ? "" : aMacAddress->ToString().AsCString(),
             aMessage.IsLinkSecurityEnabled() ? "yes" : "no", (aError == OT_ERROR_NONE) ? "" : ", error:",
             (aError == OT_ERROR_NONE) ? "" : otThreadErrorToString(aError), MessagePriorityToString(aMessage),
             shouldLogRss ? ", rss:" : "", shouldLogRss ? aMessage.GetRssAverager().ToString().AsCString() : "",
             shouldLogRadio ? ", radio:" : "", radioString);

    if (aAction != kMessagePrepareIndirect)
    {
        LogIp6SourceDestAddresses(ip6Header, sourcePort, destPort, aLogLevel);
    }

exit:
    return;
}

void MeshForwarder::LogMessage(MessageAction       aAction,
                               const Message &     aMessage,
                               const Mac::Address *aMacAddress,
                               otError             aError)
{
    otLogLevel logLevel = OT_LOG_LEVEL_INFO;

    switch (aAction)
    {
    case kMessageReceive:
    case kMessageTransmit:
    case kMessagePrepareIndirect:
        logLevel = (aError == OT_ERROR_NONE) ? OT_LOG_LEVEL_INFO : OT_LOG_LEVEL_NOTE;
        break;

    case kMessageDrop:
    case kMessageReassemblyDrop:
    case kMessageEvict:
        logLevel = OT_LOG_LEVEL_NOTE;
        break;
    }

    VerifyOrExit(GetInstance().GetLogLevel() >= logLevel);

    switch (aMessage.GetType())
    {
    case Message::kTypeIp6:
        LogIp6Message(aAction, aMessage, aMacAddress, aError, logLevel);
        break;

#if OPENTHREAD_FTD
    case Message::kType6lowpan:
        LogMeshMessage(aAction, aMessage, aMacAddress, aError, logLevel);
        break;
#endif

    default:
        break;
    }

exit:
    return;
}

void MeshForwarder::LogFrame(const char *aActionText, const Mac::Frame &aFrame, otError aError)
{
    if (aError != OT_ERROR_NONE)
    {
        otLogNoteMac("%s, aError:%s, %s", aActionText, otThreadErrorToString(aError),
                     aFrame.ToInfoString().AsCString());
    }
    else
    {
        otLogInfoMac("%s, %s", aActionText, aFrame.ToInfoString().AsCString());
    }
}

void MeshForwarder::LogFragmentFrameDrop(otError                       aError,
                                         uint16_t                      aFrameLength,
                                         const Mac::Address &          aMacSource,
                                         const Mac::Address &          aMacDest,
                                         const Lowpan::FragmentHeader &aFragmentHeader,
                                         bool                          aIsSecure)
{
    otLogNoteMac("Dropping rx frag frame, error:%s, len:%d, src:%s, dst:%s, tag:%d, offset:%d, dglen:%d, sec:%s",
                 otThreadErrorToString(aError), aFrameLength, aMacSource.ToString().AsCString(),
                 aMacDest.ToString().AsCString(), aFragmentHeader.GetDatagramTag(), aFragmentHeader.GetDatagramOffset(),
                 aFragmentHeader.GetDatagramSize(), aIsSecure ? "yes" : "no");
}

void MeshForwarder::LogLowpanHcFrameDrop(otError             aError,
                                         uint16_t            aFrameLength,
                                         const Mac::Address &aMacSource,
                                         const Mac::Address &aMacDest,
                                         bool                aIsSecure)
{
    otLogNoteMac("Dropping rx lowpan HC frame, error:%s, len:%d, src:%s, dst:%s, sec:%s", otThreadErrorToString(aError),
                 aFrameLength, aMacSource.ToString().AsCString(), aMacDest.ToString().AsCString(),
                 aIsSecure ? "yes" : "no");
}

#else // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

void MeshForwarder::LogMessage(MessageAction, const Message &, const Mac::Address *, otError)
{
}

void MeshForwarder::LogFrame(const char *, const Mac::Frame &, otError)
{
}

void MeshForwarder::LogFragmentFrameDrop(otError,
                                         uint16_t,
                                         const Mac::Address &,
                                         const Mac::Address &,
                                         const Lowpan::FragmentHeader &,
                                         bool)
{
}

void MeshForwarder::LogLowpanHcFrameDrop(otError, uint16_t, const Mac::Address &, const Mac::Address &, bool)
{
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

// LCOV_EXCL_STOP

} // namespace ot
