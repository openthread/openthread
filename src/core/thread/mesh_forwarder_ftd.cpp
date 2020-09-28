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

#include "mesh_forwarder.hpp"

#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "meshcop/meshcop.hpp"
#include "net/ip6.hpp"
#include "net/tcp.hpp"
#include "net/udp6.hpp"

namespace ot {

otError MeshForwarder::SendMessage(Message &aMessage)
{
    Mle::MleRouter &mle   = Get<Mle::MleRouter>();
    otError         error = OT_ERROR_NONE;
    Neighbor *      neighbor;

    aMessage.SetOffset(0);
    aMessage.SetDatagramTag(0);
    mSendQueue.Enqueue(aMessage);

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
                if (ip6Header.GetDestination() == mle.GetLinkLocalAllThreadNodesAddress() ||
                    ip6Header.GetDestination() == mle.GetRealmLocalAllThreadNodesAddress())
                {
                    // destined for all sleepy children
                    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
                    {
                        if (!child.IsRxOnWhenIdle())
                        {
                            mIndirectSender.AddMessageForSleepyChild(aMessage, child);
                        }
                    }
                }
                else
                {
                    // destined for some sleepy children which subscribed the multicast address.
                    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
                    {
                        if (!child.IsRxOnWhenIdle() && child.HasIp6Address(ip6Header.GetDestination()))
                        {
                            mIndirectSender.AddMessageForSleepyChild(aMessage, child);
                        }
                    }
                }
            }
        }
        else if ((neighbor = Get<NeighborTable>().FindNeighbor(ip6Header.GetDestination())) != nullptr &&
                 !neighbor->IsRxOnWhenIdle() && !aMessage.GetDirectTransmission())
        {
            // destined for a sleepy child
            Child &child = *static_cast<Child *>(neighbor);
            mIndirectSender.AddMessageForSleepyChild(aMessage, child);
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
        Child *child = Get<Utils::ChildSupervisor>().GetDestination(aMessage);
        OT_ASSERT((child != nullptr) && !child->IsRxOnWhenIdle());
        mIndirectSender.AddMessageForSleepyChild(aMessage, *child);
        break;
    }

    default:
        aMessage.SetDirectTransmission();
        break;
    }

    mScheduleTransmissionTask.Post();

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

        cur->Read(Ip6::Header::kDestinationFieldOffset, sizeof(ip6Dst), &ip6Dst);

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
                LogMessage(kMessageDrop, *cur, nullptr, aError);
                cur->Free();
            }
        }
    }

    if (enqueuedMessage)
    {
        mScheduleTransmissionTask.Post();
    }
}

otError MeshForwarder::EvictMessage(Message::Priority aPriority)
{
    otError        error    = OT_ERROR_NOT_FOUND;
    PriorityQueue *queues[] = {&mResolvingQueue, &mSendQueue};
    Message *      evict    = nullptr;

    // search for a lower priority message to evict (choose lowest priority message among all queues)
    for (PriorityQueue *queue : queues)
    {
        for (uint8_t priority = 0; priority < aPriority; priority++)
        {
            for (Message *message = queue->GetHeadForPriority(static_cast<Message::Priority>(priority)); message;
                 message          = message->GetNext())
            {
                if (message->GetPriority() != priority)
                {
                    break;
                }

                if (message->GetDoNotEvict())
                {
                    continue;
                }

                evict     = message;
                aPriority = static_cast<Message::Priority>(priority);
                break;
            }
        }
    }

    if (evict != nullptr)
    {
        ExitNow(error = OT_ERROR_NONE);
    }

    for (uint8_t priority = aPriority; priority < Message::kNumPriorities; priority++)
    {
        // search for an equal or higher priority indirect message to evict
        for (Message *message = mSendQueue.GetHeadForPriority(aPriority); message; message = message->GetNext())
        {
            if (message->GetPriority() != priority)
            {
                break;
            }

            if (message->GetDoNotEvict())
            {
                continue;
            }

            if (message->IsChildPending())
            {
                evict = message;
                ExitNow(error = OT_ERROR_NONE);
            }
        }
    }

exit:

    if (error == OT_ERROR_NONE)
    {
        RemoveMessage(*evict);
    }

    return error;
}

void MeshForwarder::RemoveMessages(Child &aChild, Message::SubType aSubType)
{
    Message *nextMessage;

    for (Message *message = mSendQueue.GetHead(); message; message = nextMessage)
    {
        nextMessage = message->GetNext();

        if ((aSubType != Message::kSubTypeNone) && (aSubType != message->GetSubType()))
        {
            continue;
        }

        if (mIndirectSender.RemoveMessageFromSleepyChild(*message, aChild) != OT_ERROR_NONE)
        {
            switch (message->GetType())
            {
            case Message::kTypeIp6:
            {
                Ip6::Header ip6header;

                IgnoreReturnValue(message->Read(0, sizeof(ip6header), &ip6header));

                if (&aChild == static_cast<Child *>(Get<NeighborTable>().FindNeighbor(ip6header.GetDestination())))
                {
                    message->ClearDirectTransmission();
                }

                break;
            }

            case Message::kType6lowpan:
            {
                Lowpan::MeshHeader meshHeader;

                IgnoreError(meshHeader.ParseFrom(*message));

                if (&aChild == static_cast<Child *>(Get<NeighborTable>().FindNeighbor(meshHeader.GetDestination())))
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
                mSendMessage = nullptr;
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
            for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
            {
                IgnoreError(mIndirectSender.RemoveMessageFromSleepyChild(*message, child));
            }
        }

        if (mSendMessage == message)
        {
            mSendMessage = nullptr;
        }

        mSendQueue.Dequeue(*message);
        LogMessage(kMessageDrop, *message, nullptr, OT_ERROR_NONE);
        message->Free();
    }
}

void MeshForwarder::SendMesh(Message &aMessage, Mac::TxFrame &aFrame)
{
    uint16_t fcf;

    // initialize MAC header
    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfDstAddrShort |
          Mac::Frame::kFcfSrcAddrShort | Mac::Frame::kFcfAckRequest | Mac::Frame::kFcfSecurityEnabled;
    Get<Mac::Mac>().UpdateFrameControlField(nullptr, aMessage.IsTimeSync(), fcf);

    aFrame.InitMacHeader(fcf, Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32);
    aFrame.SetDstPanId(Get<Mac::Mac>().GetPanId());
    aFrame.SetDstAddr(mMacDest.GetShort());
    aFrame.SetSrcAddr(mMacSource.GetShort());

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (Get<Mac::Mac>().IsCslEnabled())
    {
        Mac::HeaderIe ieList[2]; // CSL + Termination

        ieList[0].Init();
        ieList[0].SetId(Mac::Frame::kHeaderIeCsl);
        ieList[0].SetLength(sizeof(Mac::CslIe));
        ieList[1].Init();
        ieList[1].SetId(Mac::Frame::kHeaderIeTermination2);
        ieList[1].SetLength(0);
        IgnoreError(aFrame.AppendHeaderIe(ieList, 2));
    }
#endif

    // write payload
    OT_ASSERT(aMessage.GetLength() <= aFrame.GetMaxPayloadLength());
    aMessage.Read(0, aMessage.GetLength(), aFrame.GetPayload());
    aFrame.SetPayloadLength(aMessage.GetLength());

    mMessageNextOffset = aMessage.GetLength();
}

otError MeshForwarder::UpdateMeshRoute(Message &aMessage)
{
    otError            error = OT_ERROR_NONE;
    Lowpan::MeshHeader meshHeader;
    Neighbor *         neighbor;
    uint16_t           nextHop;

    IgnoreError(meshHeader.ParseFrom(aMessage));

    nextHop = Get<Mle::MleRouter>().GetNextHop(meshHeader.GetDestination());

    if (nextHop != Mac::kShortAddrInvalid)
    {
        neighbor = Get<NeighborTable>().FindNeighbor(nextHop);
    }
    else
    {
        neighbor = Get<NeighborTable>().FindNeighbor(meshHeader.GetDestination());
    }

    if (neighbor == nullptr)
    {
        ExitNow(error = OT_ERROR_DROP);
    }

    mMacDest.SetShort(neighbor->GetRloc16());
    mMacSource.SetShort(Get<Mac::Mac>().GetShortAddress());

    mAddMeshHeader = true;
    mMeshDest      = meshHeader.GetDestination();
    mMeshSource    = meshHeader.GetSource();

exit:
    return error;
}

otError MeshForwarder::UpdateIp6RouteFtd(Ip6::Header &ip6Header, Message &aMessage)
{
    Mle::MleRouter &mle   = Get<Mle::MleRouter>();
    otError         error = OT_ERROR_NONE;
    Neighbor *      neighbor;

    if (aMessage.GetOffset() > 0)
    {
        mMeshDest = aMessage.GetMeshDest();
    }
    else if (mle.IsRoutingLocator(ip6Header.GetDestination()))
    {
        uint16_t rloc16 = ip6Header.GetDestination().GetIid().GetLocator();
        VerifyOrExit(mle.IsRouterIdValid(Mle::Mle::RouterIdFromRloc16(rloc16)), error = OT_ERROR_DROP);
        mMeshDest = rloc16;
    }
    else if (mle.IsAnycastLocator(ip6Header.GetDestination()))
    {
        uint16_t aloc16 = ip6Header.GetDestination().GetIid().GetLocator();

        if (aloc16 == Mle::kAloc16Leader)
        {
            mMeshDest = Mle::Mle::Rloc16FromRouterId(mle.GetLeaderId());
        }
        else if (aloc16 <= Mle::kAloc16DhcpAgentEnd)
        {
            uint16_t agentRloc16;
            uint8_t  routerId;
            VerifyOrExit((Get<NetworkData::Leader>().GetRlocByContextId(
                              static_cast<uint8_t>(aloc16 & Mle::kAloc16DhcpAgentMask), agentRloc16) == OT_ERROR_NONE),
                         error = OT_ERROR_DROP);

            routerId = Mle::Mle::RouterIdFromRloc16(agentRloc16);

            // if agent is active router or the child of the device
            if ((Mle::Mle::IsActiveRouter(agentRloc16)) || (Mle::Mle::Rloc16FromRouterId(routerId) == mle.GetRloc16()))
            {
                mMeshDest = agentRloc16;
            }
            else
            {
                // use the parent of the ED Agent as Dest
                mMeshDest = Mle::Mle::Rloc16FromRouterId(routerId);
            }
        }
        else if (aloc16 <= Mle::kAloc16ServiceEnd)
        {
            SuccessOrExit(error = GetDestinationRlocByServiceAloc(aloc16, mMeshDest));
        }
        else if (aloc16 <= Mle::kAloc16CommissionerEnd)
        {
            SuccessOrExit(error = MeshCoP::GetBorderAgentRloc(Get<ThreadNetif>(), mMeshDest));
        }

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        else if (aloc16 == Mle::kAloc16BackboneRouterPrimary)
        {
            VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary(), error = OT_ERROR_DROP);
            mMeshDest = Get<BackboneRouter::Leader>().GetServer16();
        }
#endif
        else
        {
            // TODO: support for Neighbor Discovery Agent ALOC
            ExitNow(error = OT_ERROR_DROP);
        }
    }
    else if ((neighbor = Get<NeighborTable>().FindNeighbor(ip6Header.GetDestination())) != nullptr)
    {
        mMeshDest = neighbor->GetRloc16();
    }
    else if (Get<NetworkData::Leader>().IsOnMesh(ip6Header.GetDestination()))
    {
        SuccessOrExit(error = Get<AddressResolver>().Resolve(ip6Header.GetDestination(), mMeshDest));
    }
    else
    {
        IgnoreError(Get<NetworkData::Leader>().RouteLookup(ip6Header.GetSource(), ip6Header.GetDestination(), nullptr,
                                                           &mMeshDest));
    }

    VerifyOrExit(mMeshDest != Mac::kShortAddrInvalid, error = OT_ERROR_DROP);

    mMeshSource = Get<Mac::Mac>().GetShortAddress();

    SuccessOrExit(error = mle.CheckReachability(mMeshDest, ip6Header));
    aMessage.SetMeshDest(mMeshDest);
    mMacDest.SetShort(mle.GetNextHop(mMeshDest));

    if (mMacDest.GetShort() != mMeshDest)
    {
        // destination is not neighbor
        mMacSource.SetShort(mMeshSource);
        mAddMeshHeader = true;
    }

exit:
    return error;
}

otError MeshForwarder::GetIp6Header(const uint8_t *     aFrame,
                                    uint16_t            aFrameLength,
                                    const Mac::Address &aMacSource,
                                    const Mac::Address &aMacDest,
                                    Ip6::Header &       aIp6Header)
{
    uint8_t headerLength;
    bool    nextHeaderCompressed;

    return DecompressIp6Header(aFrame, aFrameLength, aMacSource, aMacDest, aIp6Header, headerLength,
                               nextHeaderCompressed);
}

void MeshForwarder::SendIcmpErrorIfDstUnreach(const Message &     aMessage,
                                              const Mac::Address &aMacSource,
                                              const Mac::Address &aMacDest)
{
    otError     error;
    Ip6::Header ip6header;
    Child *     child;

    VerifyOrExit(aMacSource.IsShort() && aMacDest.IsShort(), OT_NOOP);

    child = Get<ChildTable>().FindChild(aMacSource.GetShort(), Child::kInStateAnyExceptInvalid);
    VerifyOrExit((child == nullptr) || child->IsFullThreadDevice(), OT_NOOP);

    aMessage.Read(0, sizeof(ip6header), &ip6header);
    VerifyOrExit(!ip6header.GetDestination().IsMulticast() &&
                     Get<NetworkData::Leader>().IsOnMesh(ip6header.GetDestination()),
                 OT_NOOP);

    error = Get<Mle::MleRouter>().CheckReachability(aMacDest.GetShort(), ip6header);

    if (error == OT_ERROR_NO_ROUTE)
    {
        SendDestinationUnreachable(aMacSource.GetShort(), aMessage);
    }

exit:
    return;
}

otError MeshForwarder::CheckReachability(const uint8_t *     aFrame,
                                         uint16_t            aFrameLength,
                                         const Mac::Address &aMeshSource,
                                         const Mac::Address &aMeshDest)
{
    otError                error = OT_ERROR_NONE;
    Ip6::Header            ip6Header;
    Message *              message = nullptr;
    Lowpan::FragmentHeader fragmentHeader;
    uint16_t               fragmentHeaderLength;
    uint16_t               datagramSize = 0;

    if (fragmentHeader.ParseFrom(aFrame, aFrameLength, fragmentHeaderLength) == OT_ERROR_NONE)
    {
        // Only the first fragment header is followed by a LOWPAN_IPHC header
        VerifyOrExit(fragmentHeader.GetDatagramOffset() == 0, error = OT_ERROR_NOT_FOUND);
        aFrame += fragmentHeaderLength;
        aFrameLength -= fragmentHeaderLength;

        datagramSize = fragmentHeader.GetDatagramSize();
    }

    VerifyOrExit(aFrameLength >= 1 && Lowpan::Lowpan::IsLowpanHc(aFrame), error = OT_ERROR_NOT_FOUND);

    error = FrameToMessage(aFrame, aFrameLength, datagramSize, aMeshSource, aMeshDest, message);
    SuccessOrExit(error);

    message->Read(0, sizeof(ip6Header), &ip6Header);
    error = Get<Mle::MleRouter>().CheckReachability(aMeshDest.GetShort(), ip6Header);

exit:
    if (error == OT_ERROR_NOT_FOUND)
    {
        // the message may not contain an IPv6 header
        error = OT_ERROR_NONE;
    }
    else if (error == OT_ERROR_NO_ROUTE)
    {
        SendDestinationUnreachable(aMeshSource.GetShort(), *message);
    }

    FreeMessage(message);

    return error;
}

void MeshForwarder::SendDestinationUnreachable(uint16_t aMeshSource, const Message &aMessage)
{
    Ip6::MessageInfo messageInfo;

    messageInfo.GetPeerAddr() = Get<Mle::MleRouter>().GetMeshLocal16();
    messageInfo.GetPeerAddr().GetIid().SetLocator(aMeshSource);

    IgnoreError(Get<Ip6::Icmp>().SendError(Ip6::Icmp::Header::kTypeDstUnreach,
                                           Ip6::Icmp::Header::kCodeDstUnreachNoRoute, messageInfo, aMessage));
}

void MeshForwarder::HandleMesh(uint8_t *             aFrame,
                               uint16_t              aFrameLength,
                               const Mac::Address &  aMacSource,
                               const ThreadLinkInfo &aLinkInfo)
{
    otError            error   = OT_ERROR_NONE;
    Message *          message = nullptr;
    Mac::Address       meshDest;
    Mac::Address       meshSource;
    Lowpan::MeshHeader meshHeader;
    uint16_t           headerLength;

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aLinkInfo.IsLinkSecurityEnabled(), error = OT_ERROR_SECURITY);

    SuccessOrExit(error = meshHeader.ParseFrom(aFrame, aFrameLength, headerLength));

    meshSource.SetShort(meshHeader.GetSource());
    meshDest.SetShort(meshHeader.GetDestination());

    aFrame += headerLength;
    aFrameLength -= headerLength;

    UpdateRoutes(aFrame, aFrameLength, meshSource, meshDest);

    if (meshDest.GetShort() == Get<Mac::Mac>().GetShortAddress() ||
        Get<Mle::MleRouter>().IsMinimalChild(meshDest.GetShort()))
    {
        if (Lowpan::FragmentHeader::IsFragmentHeader(aFrame, aFrameLength))
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
        Message::Priority priority = Message::kPriorityNormal;
        uint16_t          offset   = 0;

        Get<Mle::MleRouter>().ResolveRoutingLoops(aMacSource.GetShort(), meshDest.GetShort());

        SuccessOrExit(error = CheckReachability(aFrame, aFrameLength, meshSource, meshDest));

        meshHeader.DecrementHopsLeft();

        GetForwardFramePriority(aFrame, aFrameLength, meshSource, meshDest, priority);
        message = Get<MessagePool>().New(Message::kType6lowpan, priority);
        VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(error = message->SetLength(meshHeader.GetHeaderLength() + aFrameLength));
        offset += meshHeader.WriteTo(*message, offset);
        message->Write(offset, aFrameLength, aFrame);
        message->SetLinkInfo(aLinkInfo);

        LogMessage(kMessageReceive, *message, &aMacSource, OT_ERROR_NONE);

        IgnoreError(SendMessage(*message));
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogInfoMac("Dropping rx mesh frame, error:%s, len:%d, src:%s, sec:%s", otThreadErrorToString(error),
                     aFrameLength, aMacSource.ToString().AsCString(), aLinkInfo.IsLinkSecurityEnabled() ? "yes" : "no");
        FreeMessage(message);
    }
}

void MeshForwarder::UpdateRoutes(const uint8_t *     aFrame,
                                 uint16_t            aFrameLength,
                                 const Mac::Address &aMeshSource,
                                 const Mac::Address &aMeshDest)
{
    Ip6::Header ip6Header;
    Neighbor *  neighbor;

    VerifyOrExit(!aMeshDest.IsBroadcast() && aMeshSource.IsShort(), OT_NOOP);
    SuccessOrExit(GetIp6Header(aFrame, aFrameLength, aMeshSource, aMeshDest, ip6Header));

    if (!ip6Header.GetSource().GetIid().IsLocator() &&
        Get<NetworkData::Leader>().IsOnMesh(ip6Header.GetSource()) /* only for on mesh address which may require AQ */)
    {
        if (Get<AddressResolver>().UpdateCacheEntry(ip6Header.GetSource(), aMeshSource.GetShort()) ==
            OT_ERROR_NOT_FOUND)
        {
            // Thread 1.1 Specification 5.5.2.2: FTDs MAY add/update
            // EID-to-RLOC Map Cache entries by inspecting packets
            // being received. We exclude frames from an MTD child
            // source and verify that the destination is the device
            // itself or an MTD child of the device.

            if (Get<Mle::MleRouter>().IsFullThreadDevice() &&
                !Get<Mle::MleRouter>().IsMinimalChild(aMeshSource.GetShort()) &&
                (aMeshDest.GetShort() == Get<Mac::Mac>().GetShortAddress() ||
                 Get<Mle::MleRouter>().IsMinimalChild(aMeshDest.GetShort())))
            {
                Get<AddressResolver>().AddSnoopedCacheEntry(ip6Header.GetSource(), aMeshSource.GetShort());
            }
        }
    }

    neighbor = Get<NeighborTable>().FindNeighbor(ip6Header.GetSource());
    VerifyOrExit(neighbor != nullptr && !neighbor->IsFullThreadDevice(), OT_NOOP);

    if (!Mle::Mle::RouterIdMatch(aMeshSource.GetShort(), Get<Mac::Mac>().GetShortAddress()))
    {
        Get<Mle::MleRouter>().RemoveNeighbor(*neighbor);
    }

exit:
    return;
}

bool MeshForwarder::FragmentPriorityList::UpdateOnTimeTick(void)
{
    bool contineRxingTicks = false;

    for (Entry &entry : mEntries)
    {
        if (!entry.IsExpired())
        {
            entry.DecrementLifetime();

            if (!entry.IsExpired())
            {
                contineRxingTicks = true;
            }
        }
    }

    return contineRxingTicks;
}

void MeshForwarder::UpdateFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                           uint16_t                aFragmentLength,
                                           uint16_t                aSrcRloc16,
                                           Message::Priority       aPriority)
{
    FragmentPriorityList::Entry *entry;

    entry = mFragmentPriorityList.FindEntry(aSrcRloc16, aFragmentHeader.GetDatagramTag());

    if (entry == nullptr)
    {
        VerifyOrExit(aFragmentHeader.GetDatagramOffset() == 0, OT_NOOP);

        mFragmentPriorityList.AllocateEntry(aSrcRloc16, aFragmentHeader.GetDatagramTag(), aPriority);
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMeshForwarder);
        ExitNow();
    }

    if (aFragmentHeader.GetDatagramOffset() + aFragmentLength >= aFragmentHeader.GetDatagramSize())
    {
        entry->Clear();
    }
    else
    {
        entry->ResetLifetime();
    }

exit:
    return;
}

MeshForwarder::FragmentPriorityList::Entry *MeshForwarder::FragmentPriorityList::FindEntry(uint16_t aSrcRloc16,
                                                                                           uint16_t aTag)
{
    Entry *rval = nullptr;

    for (Entry &entry : mEntries)
    {
        if (!entry.IsExpired() && entry.Matches(aSrcRloc16, aTag))
        {
            rval = &entry;
            break;
        }
    }

    return rval;
}

MeshForwarder::FragmentPriorityList::Entry *MeshForwarder::FragmentPriorityList::AllocateEntry(
    uint16_t          aSrcRloc16,
    uint16_t          aTag,
    Message::Priority aPriority)
{
    Entry *newEntry = nullptr;

    for (Entry &entry : mEntries)
    {
        if (entry.IsExpired())
        {
            entry.mSrcRloc16   = aSrcRloc16;
            entry.mDatagramTag = aTag;
            entry.mPriority    = aPriority;
            entry.ResetLifetime();
            newEntry = &entry;
            break;
        }
    }

    return newEntry;
}

otError MeshForwarder::GetFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                           uint16_t                aSrcRloc16,
                                           Message::Priority &     aPriority)
{
    otError                      error = OT_ERROR_NONE;
    FragmentPriorityList::Entry *entry;

    entry = mFragmentPriorityList.FindEntry(aSrcRloc16, aFragmentHeader.GetDatagramTag());
    VerifyOrExit(entry != nullptr, error = OT_ERROR_NOT_FOUND);
    aPriority = entry->GetPriority();

exit:
    return error;
}

void MeshForwarder::GetForwardFramePriority(const uint8_t *     aFrame,
                                            uint16_t            aFrameLength,
                                            const Mac::Address &aMeshSource,
                                            const Mac::Address &aMeshDest,
                                            Message::Priority & aPriority)
{
    otError                error      = OT_ERROR_NONE;
    bool                   isFragment = false;
    Lowpan::FragmentHeader fragmentHeader;
    uint16_t               fragmentHeaderLength;

    if (fragmentHeader.ParseFrom(aFrame, aFrameLength, fragmentHeaderLength) == OT_ERROR_NONE)
    {
        isFragment = true;
        aFrame += fragmentHeaderLength;
        aFrameLength -= fragmentHeaderLength;

        if (fragmentHeader.GetDatagramOffset() > 0)
        {
            // Get priority from the pre-buffered info
            ExitNow(error = GetFragmentPriority(fragmentHeader, aMeshSource.GetShort(), aPriority));
        }
    }

    // Get priority from IPv6 header or UDP destination port directly
    error = GetFramePriority(aFrame, aFrameLength, aMeshSource, aMeshDest, aPriority);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogNoteMac("Failed to get forwarded frame priority, error:%s, len:%d, src:%d, dst:%s",
                     otThreadErrorToString(error), aFrameLength, aMeshSource.ToString().AsCString(),
                     aMeshDest.ToString().AsCString());
    }
    else if (isFragment)
    {
        UpdateFragmentPriority(fragmentHeader, aFrameLength, aMeshSource.GetShort(), aPriority);
    }

    return;
}

otError MeshForwarder::GetDestinationRlocByServiceAloc(uint16_t aServiceAloc, uint16_t &aMeshDest)
{
    otError                        error      = OT_ERROR_NONE;
    uint8_t                        serviceId  = Mle::Mle::ServiceIdFromAloc(aServiceAloc);
    const NetworkData::ServiceTlv *serviceTlv = Get<NetworkData::Leader>().FindServiceById(serviceId);

    if (serviceTlv != nullptr)
    {
        const NetworkData::NetworkDataTlv *cur = serviceTlv->GetSubTlvs();
        const NetworkData::NetworkDataTlv *end = serviceTlv->GetNext();
        Neighbor *                         neighbor;
        uint16_t                           server16;
        uint8_t                            bestCost = Mle::kMaxRouteCost;
        uint8_t                            curCost  = 0x00;
        uint16_t                           bestDest = Mac::kShortAddrInvalid;

        while (cur < end)
        {
            switch (cur->GetType())
            {
            case NetworkData::NetworkDataTlv::kTypeServer:
                server16 = static_cast<const NetworkData::ServerTlv *>(cur)->GetServer16();

                // Path cost
                curCost = Get<Mle::MleRouter>().GetCost(server16);

                if (!Get<Mle::MleRouter>().IsActiveRouter(server16))
                {
                    // Assume best link between remote child server and its parent.
                    curCost += 1;
                }

                // Cost if the server is direct neighbor.
                neighbor = Get<NeighborTable>().FindNeighbor(server16);

                if (neighbor != nullptr && neighbor->IsStateValid())
                {
                    uint8_t cost;

                    if (!Get<Mle::MleRouter>().IsActiveRouter(server16))
                    {
                        // Cost calculated only from Link Quality In as the parent only maintains
                        // one-direction link info.
                        cost = Mle::MleRouter::LinkQualityToCost(neighbor->GetLinkInfo().GetLinkQuality());
                    }
                    else
                    {
                        cost = Get<Mle::MleRouter>().GetLinkCost(Mle::Mle::RouterIdFromRloc16(server16));
                    }

                    // Choose the minimum cost
                    if (cost < curCost)
                    {
                        curCost = cost;
                    }
                }

                if ((bestDest == Mac::kShortAddrInvalid) || (curCost < bestCost))
                {
                    bestDest = server16;
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
            ExitNow(error = OT_ERROR_NO_ROUTE);
        }
    }
    else
    {
        // Unknown service, can't forward
        ExitNow(error = OT_ERROR_NO_ROUTE);
    }

exit:
    return error;
}

// LCOV_EXCL_START

#if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

otError MeshForwarder::LogMeshFragmentHeader(MessageAction       aAction,
                                             const Message &     aMessage,
                                             const Mac::Address *aMacAddress,
                                             otError             aError,
                                             uint16_t &          aOffset,
                                             Mac::Address &      aMeshSource,
                                             Mac::Address &      aMeshDest,
                                             otLogLevel          aLogLevel)
{
    otError                error             = OT_ERROR_FAILED;
    bool                   hasFragmentHeader = false;
    bool                   shouldLogRss;
    Lowpan::MeshHeader     meshHeader;
    Lowpan::FragmentHeader fragmentHeader;
    uint16_t               headerLength;

    SuccessOrExit(meshHeader.ParseFrom(aMessage, headerLength));

    aMeshSource.SetShort(meshHeader.GetSource());
    aMeshDest.SetShort(meshHeader.GetDestination());

    aOffset = headerLength;

    if (fragmentHeader.ParseFrom(aMessage, aOffset, headerLength) == OT_ERROR_NONE)
    {
        hasFragmentHeader = true;
        aOffset += headerLength;
    }

    shouldLogRss = (aAction == kMessageReceive) || (aAction == kMessageReassemblyDrop);

    otLogMac(
        aLogLevel, "%s mesh frame, len:%d%s%s, msrc:%s, mdst:%s, hops:%d, frag:%s, sec:%s%s%s%s%s",
        MessageActionToString(aAction, aError), aMessage.GetLength(),
        (aMacAddress == nullptr) ? "" : ((aAction == kMessageReceive) ? ", from:" : ", to:"),
        (aMacAddress == nullptr) ? "" : aMacAddress->ToString().AsCString(), aMeshSource.ToString().AsCString(),
        aMeshDest.ToString().AsCString(), meshHeader.GetHopsLeft() + ((aAction == kMessageReceive) ? 1 : 0),
        hasFragmentHeader ? "yes" : "no", aMessage.IsLinkSecurityEnabled() ? "yes" : "no",
        (aError == OT_ERROR_NONE) ? "" : ", error:", (aError == OT_ERROR_NONE) ? "" : otThreadErrorToString(aError),
        shouldLogRss ? ", rss:" : "", shouldLogRss ? aMessage.GetRssAverager().ToString().AsCString() : "");

    if (hasFragmentHeader)
    {
        otLogMac(aLogLevel, "    Frag tag:%04x, offset:%d, size:%d", fragmentHeader.GetDatagramTag(),
                 fragmentHeader.GetDatagramOffset(), fragmentHeader.GetDatagramSize());

        VerifyOrExit(fragmentHeader.GetDatagramOffset() == 0, OT_NOOP);
    }

    error = OT_ERROR_NONE;

exit:
    return error;
}

otError MeshForwarder::DecompressIp6UdpTcpHeader(const Message &     aMessage,
                                                 uint16_t            aOffset,
                                                 const Mac::Address &aMeshSource,
                                                 const Mac::Address &aMeshDest,
                                                 Ip6::Header &       aIp6Header,
                                                 uint16_t &          aChecksum,
                                                 uint16_t &          aSourcePort,
                                                 uint16_t &          aDestPort)
{
    otError  error = OT_ERROR_PARSE;
    int      headerLength;
    bool     nextHeaderCompressed;
    uint8_t  frameBuffer[sizeof(Ip6::Header)];
    uint16_t frameLength;
    union
    {
        Ip6::Udp::Header udp;
        Ip6::Tcp::Header tcp;
    } header;

    aChecksum   = 0;
    aSourcePort = 0;
    aDestPort   = 0;

    // Read and decompress the IPv6 header

    frameLength = aMessage.Read(aOffset, sizeof(frameBuffer), frameBuffer);

    headerLength = Get<Lowpan::Lowpan>().DecompressBaseHeader(aIp6Header, nextHeaderCompressed, aMeshSource, aMeshDest,
                                                              frameBuffer, frameLength);
    VerifyOrExit(headerLength >= 0, OT_NOOP);

    aOffset += headerLength;

    // Read and decompress UDP or TCP header

    switch (aIp6Header.GetNextHeader())
    {
    case Ip6::kProtoUdp:
        if (nextHeaderCompressed)
        {
            frameLength  = aMessage.Read(aOffset, sizeof(Ip6::Udp::Header), frameBuffer);
            headerLength = Get<Lowpan::Lowpan>().DecompressUdpHeader(header.udp, frameBuffer, frameLength);
            VerifyOrExit(headerLength >= 0, OT_NOOP);
        }
        else
        {
            VerifyOrExit(sizeof(Ip6::Udp::Header) == aMessage.Read(aOffset, sizeof(Ip6::Udp::Header), &header.udp),
                         OT_NOOP);
        }

        aChecksum   = header.udp.GetChecksum();
        aSourcePort = header.udp.GetSourcePort();
        aDestPort   = header.udp.GetDestinationPort();
        break;

    case Ip6::kProtoTcp:
        VerifyOrExit(sizeof(Ip6::Tcp::Header) == aMessage.Read(aOffset, sizeof(Ip6::Tcp::Header), &header.tcp),
                     OT_NOOP);
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

void MeshForwarder::LogMeshIpHeader(const Message &     aMessage,
                                    uint16_t            aOffset,
                                    const Mac::Address &aMeshSource,
                                    const Mac::Address &aMeshDest,
                                    otLogLevel          aLogLevel)
{
    uint16_t    checksum;
    uint16_t    sourcePort;
    uint16_t    destPort;
    Ip6::Header ip6Header;

    SuccessOrExit(DecompressIp6UdpTcpHeader(aMessage, aOffset, aMeshSource, aMeshDest, ip6Header, checksum, sourcePort,
                                            destPort));

    otLogMac(aLogLevel, "    IPv6 %s msg, chksum:%04x, prio:%s", Ip6::Ip6::IpProtoToString(ip6Header.GetNextHeader()),
             checksum, MessagePriorityToString(aMessage));

    LogIp6SourceDestAddresses(ip6Header, sourcePort, destPort, aLogLevel);

exit:
    return;
}

void MeshForwarder::LogMeshMessage(MessageAction       aAction,
                                   const Message &     aMessage,
                                   const Mac::Address *aMacAddress,
                                   otError             aError,
                                   otLogLevel          aLogLevel)
{
    uint16_t     offset;
    Mac::Address meshSource;
    Mac::Address meshDest;

    SuccessOrExit(
        LogMeshFragmentHeader(aAction, aMessage, aMacAddress, aError, offset, meshSource, meshDest, aLogLevel));

    // When log action is `kMessageTransmit` we do not include
    // the IPv6 header info in the logs, as the same info is
    // logged when the same Mesh Header message was received
    // and info about it was logged.

    VerifyOrExit(aAction != kMessageTransmit, OT_NOOP);

    LogMeshIpHeader(aMessage, offset, meshSource, meshDest, aLogLevel);

exit:
    return;
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

// LCOV_EXCL_STOP

} // namespace ot

#endif // OPENTHREAD_FTD
