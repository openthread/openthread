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

#include "mesh_forwarder.hpp"

#if OPENTHREAD_FTD

#include "common/locator_getters.hpp"
#include "common/num_utils.hpp"
#include "meshcop/meshcop.hpp"
#include "net/ip6.hpp"
#include "net/tcp6.hpp"
#include "net/udp6.hpp"

namespace ot {

RegisterLogModule("MeshForwarder");

void MeshForwarder::SendMessage(OwnedPtr<Message> aMessagePtr)
{
    Message &message = *aMessagePtr.Release();

    message.SetOffset(0);
    message.SetDatagramTag(0);
    message.SetTimestampToNow();
    mSendQueue.Enqueue(message);

    switch (message.GetType())
    {
    case Message::kTypeIp6:
    {
        Ip6::Header         ip6Header;
        const Ip6::Address &destination = ip6Header.GetDestination();

        IgnoreError(message.Read(0, ip6Header));

        if (destination.IsMulticast())
        {
            // For traffic destined to multicast address larger than realm local, generally it uses IP-in-IP
            // encapsulation (RFC2473), with outer destination as ALL_MPL_FORWARDERS. So here if the destination
            // is multicast address larger than realm local, it should be for indirection transmission for the
            // device's sleepy child, thus there should be no direct transmission.
            if (!destination.IsMulticastLargerThanRealmLocal())
            {
                message.SetDirectTransmission();
            }

            if (message.GetSubType() != Message::kSubTypeMplRetransmission)
            {
                // Check if we need to forward the multicast message
                // to any sleepy child. This is skipped for MPL retx
                // (only the first MPL transmission is forwarded to
                // sleepy children).

                bool destinedForAll = ((destination == Get<Mle::Mle>().GetLinkLocalAllThreadNodesAddress()) ||
                                       (destination == Get<Mle::Mle>().GetRealmLocalAllThreadNodesAddress()));

                for (Child &child : Get<ChildTable>().Iterate(Child::kInStateValidOrRestoring))
                {
                    if (!child.IsRxOnWhenIdle() && (destinedForAll || child.HasIp6Address(destination)))
                    {
                        mIndirectSender.AddMessageForSleepyChild(message, child);
                    }
                }
            }
        }
        else // Destination is unicast
        {
            Neighbor *neighbor = Get<NeighborTable>().FindNeighbor(destination);

            if ((neighbor != nullptr) && !neighbor->IsRxOnWhenIdle() && !message.IsDirectTransmission() &&
                Get<ChildTable>().Contains(*neighbor))
            {
                mIndirectSender.AddMessageForSleepyChild(message, *static_cast<Child *>(neighbor));
            }
            else
            {
                message.SetDirectTransmission();
            }
        }

        break;
    }

    case Message::kTypeSupervision:
    {
        Child *child = Get<ChildSupervisor>().GetDestination(message);
        OT_ASSERT((child != nullptr) && !child->IsRxOnWhenIdle());
        mIndirectSender.AddMessageForSleepyChild(message, *child);
        break;
    }

    default:
        message.SetDirectTransmission();
        break;
    }

    // Ensure that the message is marked for direct tx and/or for indirect tx
    // to a sleepy child. Otherwise, remove the message.

    if (RemoveMessageIfNoPendingTx(message))
    {
        ExitNow();
    }

#if (OPENTHREAD_CONFIG_MAX_FRAMES_IN_DIRECT_TX_QUEUE > 0)
    ApplyDirectTxQueueLimit(message);
#endif

    mScheduleTransmissionTask.Post();

exit:
    return;
}

void MeshForwarder::HandleResolved(const Ip6::Address &aEid, Error aError)
{
    Ip6::Address ip6Dst;
    bool         didUpdate = false;

    for (Message &message : mSendQueue)
    {
        if (!message.IsResolvingAddress())
        {
            continue;
        }

        IgnoreError(message.Read(Ip6::Header::kDestinationFieldOffset, ip6Dst));

        if (ip6Dst != aEid)
        {
            continue;
        }

        if (aError != kErrorNone)
        {
            LogMessage(kMessageDrop, message, kErrorAddressQuery);
            FinalizeMessageDirectTx(message, kErrorAddressQuery);
            mSendQueue.DequeueAndFree(message);
            continue;
        }

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        // Pass back to IPv6 layer for DUA destination resolved
        // by Backbone Query
        if (Get<BackboneRouter::Local>().IsPrimary() && Get<BackboneRouter::Leader>().IsDomainUnicast(ip6Dst) &&
            Get<AddressResolver>().LookUp(ip6Dst) == Get<Mle::MleRouter>().GetRloc16())
        {
            uint8_t hopLimit;

            mSendQueue.Dequeue(message);

            // Avoid decreasing Hop Limit twice
            IgnoreError(message.Read(Ip6::Header::kHopLimitFieldOffset, hopLimit));
            hopLimit++;
            message.Write(Ip6::Header::kHopLimitFieldOffset, hopLimit);
            message.SetLoopbackToHostAllowed(true);
            message.SetOrigin(Message::kOriginHostTrusted);

            IgnoreError(Get<Ip6::Ip6>().HandleDatagram(OwnedPtr<Message>(&message)));
            continue;
        }
#endif

        message.SetResolvingAddress(false);
        didUpdate = true;
    }

    if (didUpdate)
    {
        mScheduleTransmissionTask.Post();
    }
}

Error MeshForwarder::EvictMessage(Message::Priority aPriority)
{
    Error    error = kErrorNotFound;
    Message *evict = nullptr;

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    error = RemoveAgedMessages();
    VerifyOrExit(error == kErrorNotFound);
#endif

    // Search for a lower priority message to evict
    for (uint8_t priority = 0; priority < aPriority; priority++)
    {
        for (Message *message = mSendQueue.GetHeadForPriority(static_cast<Message::Priority>(priority)); message;
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

            evict = message;
            error = kErrorNone;
            ExitNow();
        }
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
                ExitNow(error = kErrorNone);
            }
        }
    }

exit:
    if ((error == kErrorNone) && (evict != nullptr))
    {
        EvictMessage(*evict);
    }

    return error;
}

void MeshForwarder::RemoveMessages(Child &aChild, Message::SubType aSubType)
{
    for (Message &message : mSendQueue)
    {
        if ((aSubType != Message::kSubTypeNone) && (aSubType != message.GetSubType()))
        {
            continue;
        }

        if (mIndirectSender.RemoveMessageFromSleepyChild(message, aChild) != kErrorNone)
        {
            switch (message.GetType())
            {
            case Message::kTypeIp6:
            {
                Ip6::Header ip6header;

                IgnoreError(message.Read(0, ip6header));

                if (&aChild == Get<NeighborTable>().FindNeighbor(ip6header.GetDestination()))
                {
                    message.ClearDirectTransmission();
                }

                break;
            }

            case Message::kType6lowpan:
            {
                Lowpan::MeshHeader meshHeader;

                IgnoreError(meshHeader.ParseFrom(message));

                if (&aChild == Get<NeighborTable>().FindNeighbor(meshHeader.GetDestination()))
                {
                    message.ClearDirectTransmission();
                }

                break;
            }

            default:
                break;
            }
        }

        RemoveMessageIfNoPendingTx(message);
    }
}

void MeshForwarder::RemoveDataResponseMessages(void)
{
    Ip6::Header ip6Header;

    for (Message &message : mSendQueue)
    {
        if (message.GetSubType() != Message::kSubTypeMleDataResponse)
        {
            continue;
        }

        IgnoreError(message.Read(0, ip6Header));

        if (!(ip6Header.GetDestination().IsMulticast()))
        {
            for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
            {
                IgnoreError(mIndirectSender.RemoveMessageFromSleepyChild(message, child));
            }
        }

        if (mSendMessage == &message)
        {
            mSendMessage = nullptr;
        }

        LogMessage(kMessageDrop, message);
        FinalizeMessageDirectTx(message, kErrorDrop);
        mSendQueue.DequeueAndFree(message);
    }
}

void MeshForwarder::SendMesh(Message &aMessage, Mac::TxFrame &aFrame)
{
    Mac::PanIds panIds;

    panIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    PrepareMacHeaders(aFrame, Mac::Frame::kTypeData, mMacAddrs, panIds, Mac::Frame::kSecurityEncMic32,
                      Mac::Frame::kKeyIdMode1, &aMessage);

    // write payload
    OT_ASSERT(aMessage.GetLength() <= aFrame.GetMaxPayloadLength());
    aMessage.ReadBytes(0, aFrame.GetPayload(), aMessage.GetLength());
    aFrame.SetPayloadLength(aMessage.GetLength());

    mMessageNextOffset = aMessage.GetLength();
}

Error MeshForwarder::UpdateMeshRoute(Message &aMessage)
{
    Error              error = kErrorNone;
    Lowpan::MeshHeader meshHeader;
    Neighbor          *neighbor;
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
        ExitNow(error = kErrorDrop);
    }

    mMacAddrs.mDestination.SetShort(neighbor->GetRloc16());
    mMacAddrs.mSource.SetShort(Get<Mac::Mac>().GetShortAddress());

    mAddMeshHeader = true;
    mMeshDest      = meshHeader.GetDestination();
    mMeshSource    = meshHeader.GetSource();

#if OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
    if (mMacAddrs.mDestination.GetShort() != mMeshDest)
    {
        mDelayNextTx = true;
    }
#endif

exit:
    return error;
}

void MeshForwarder::EvaluateRoutingCost(uint16_t aDest, uint8_t &aBestCost, uint16_t &aBestDest) const
{
    uint8_t cost = Get<RouterTable>().GetPathCost(aDest);

    if ((aBestDest == Mac::kShortAddrInvalid) || (cost < aBestCost))
    {
        aBestDest = aDest;
        aBestCost = cost;
    }
}

Error MeshForwarder::AnycastRouteLookup(uint8_t aServiceId, AnycastType aType, uint16_t &aMeshDest) const
{
    NetworkData::Iterator iterator = NetworkData::kIteratorInit;
    uint8_t               bestCost = Mle::kMaxRouteCost;
    uint16_t              bestDest = Mac::kShortAddrInvalid;
    uint8_t               routerId;

    switch (aType)
    {
    case kAnycastDhcp6Agent:
    case kAnycastNeighborDiscoveryAgent:
    {
        NetworkData::OnMeshPrefixConfig config;
        Lowpan::Context                 context;

        SuccessOrExit(Get<NetworkData::Leader>().GetContext(aServiceId, context));

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, config) == kErrorNone)
        {
            if (config.GetPrefix() != context.mPrefix)
            {
                continue;
            }

            switch (aType)
            {
            case kAnycastDhcp6Agent:
                if (!(config.mDhcp || config.mConfigure))
                {
                    continue;
                }
                break;
            case kAnycastNeighborDiscoveryAgent:
                if (!config.mNdDns)
                {
                    continue;
                }
                break;
            default:
                OT_ASSERT(false);
                break;
            }

            EvaluateRoutingCost(config.mRloc16, bestCost, bestDest);
        }

        break;
    }
    case kAnycastService:
    {
        NetworkData::ServiceConfig config;

        while (Get<NetworkData::Leader>().GetNextService(iterator, config) == kErrorNone)
        {
            if (config.mServiceId != aServiceId)
            {
                continue;
            }

            EvaluateRoutingCost(config.mServerConfig.mRloc16, bestCost, bestDest);
        }

        break;
    }
    }

    routerId = Mle::RouterIdFromRloc16(bestDest);

    if (!(Mle::IsActiveRouter(bestDest) || Mle::Rloc16FromRouterId(routerId) == Get<Mle::MleRouter>().GetRloc16()))
    {
        // if agent is neither active router nor child of this device
        // use the parent of the ED Agent as Dest
        bestDest = Mle::Rloc16FromRouterId(routerId);
    }

    aMeshDest = bestDest;

exit:
    return (bestDest != Mac::kShortAddrInvalid) ? kErrorNone : kErrorNoRoute;
}

Error MeshForwarder::UpdateIp6RouteFtd(const Ip6::Header &aIp6Header, Message &aMessage)
{
    Mle::MleRouter &mle   = Get<Mle::MleRouter>();
    Error           error = kErrorNone;
    Neighbor       *neighbor;

    if (aMessage.GetOffset() > 0)
    {
        mMeshDest = aMessage.GetMeshDest();
    }
    else if (mle.IsRoutingLocator(aIp6Header.GetDestination()))
    {
        uint16_t rloc16 = aIp6Header.GetDestination().GetIid().GetLocator();
        VerifyOrExit(Mle::IsRouterIdValid(Mle::RouterIdFromRloc16(rloc16)), error = kErrorDrop);
        mMeshDest = rloc16;
    }
    else if (mle.IsAnycastLocator(aIp6Header.GetDestination()))
    {
        uint16_t aloc16 = aIp6Header.GetDestination().GetIid().GetLocator();

        if (aloc16 == Mle::kAloc16Leader)
        {
            mMeshDest = Mle::Rloc16FromRouterId(mle.GetLeaderId());
        }
        else if (aloc16 <= Mle::kAloc16DhcpAgentEnd)
        {
            uint8_t contextId = static_cast<uint8_t>(aloc16 - Mle::kAloc16DhcpAgentStart + 1);
            SuccessOrExit(error = AnycastRouteLookup(contextId, kAnycastDhcp6Agent, mMeshDest));
        }
        else if (aloc16 <= Mle::kAloc16ServiceEnd)
        {
            uint8_t serviceId = static_cast<uint8_t>(aloc16 - Mle::kAloc16ServiceStart);
            SuccessOrExit(error = AnycastRouteLookup(serviceId, kAnycastService, mMeshDest));
        }
        else if (aloc16 <= Mle::kAloc16CommissionerEnd)
        {
            SuccessOrExit(error = Get<NetworkData::Leader>().FindBorderAgentRloc(mMeshDest));
        }

#if (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
        else if (aloc16 == Mle::kAloc16BackboneRouterPrimary)
        {
            VerifyOrExit(Get<BackboneRouter::Leader>().HasPrimary(), error = kErrorDrop);
            mMeshDest = Get<BackboneRouter::Leader>().GetServer16();
        }
#endif
        else if ((aloc16 >= Mle::kAloc16NeighborDiscoveryAgentStart) &&
                 (aloc16 <= Mle::kAloc16NeighborDiscoveryAgentEnd))
        {
            uint8_t contextId = static_cast<uint8_t>(aloc16 - Mle::kAloc16NeighborDiscoveryAgentStart + 1);
            SuccessOrExit(error = AnycastRouteLookup(contextId, kAnycastNeighborDiscoveryAgent, mMeshDest));
        }
        else
        {
            ExitNow(error = kErrorDrop);
        }
    }
    else if ((neighbor = Get<NeighborTable>().FindNeighbor(aIp6Header.GetDestination())) != nullptr)
    {
        mMeshDest = neighbor->GetRloc16();
    }
    else if (Get<NetworkData::Leader>().IsOnMesh(aIp6Header.GetDestination()))
    {
        SuccessOrExit(error = Get<AddressResolver>().Resolve(aIp6Header.GetDestination(), mMeshDest));
    }
    else
    {
        IgnoreError(
            Get<NetworkData::Leader>().RouteLookup(aIp6Header.GetSource(), aIp6Header.GetDestination(), mMeshDest));
    }

    VerifyOrExit(mMeshDest != Mac::kShortAddrInvalid, error = kErrorDrop);

    mMeshSource = Get<Mac::Mac>().GetShortAddress();

    SuccessOrExit(error = mle.CheckReachability(mMeshDest, aIp6Header));
    aMessage.SetMeshDest(mMeshDest);
    mMacAddrs.mDestination.SetShort(mle.GetNextHop(mMeshDest));

    if (mMacAddrs.mDestination.GetShort() != mMeshDest)
    {
        // destination is not neighbor
        mMacAddrs.mSource.SetShort(mMeshSource);
        mAddMeshHeader = true;
#if OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
        mDelayNextTx = true;
#endif
    }

exit:
    return error;
}

void MeshForwarder::SendIcmpErrorIfDstUnreach(const Message &aMessage, const Mac::Addresses &aMacAddrs)
{
    Error        error;
    Ip6::Headers ip6Headers;
    Child       *child;

    VerifyOrExit(aMacAddrs.mSource.IsShort() && aMacAddrs.mDestination.IsShort());

    child = Get<ChildTable>().FindChild(aMacAddrs.mSource.GetShort(), Child::kInStateAnyExceptInvalid);
    VerifyOrExit((child == nullptr) || child->IsFullThreadDevice());

    SuccessOrExit(ip6Headers.ParseFrom(aMessage));

    VerifyOrExit(!ip6Headers.GetDestinationAddress().IsMulticast() &&
                 Get<NetworkData::Leader>().IsOnMesh(ip6Headers.GetDestinationAddress()));

    error = Get<Mle::MleRouter>().CheckReachability(aMacAddrs.mDestination.GetShort(), ip6Headers.GetIp6Header());

    if (error == kErrorNoRoute)
    {
        SendDestinationUnreachable(aMacAddrs.mSource.GetShort(), ip6Headers);
    }

exit:
    return;
}

Error MeshForwarder::CheckReachability(const FrameData &aFrameData, const Mac::Addresses &aMeshAddrs)
{
    Error        error;
    Ip6::Headers ip6Headers;

    error = ip6Headers.DecompressFrom(aFrameData, aMeshAddrs, GetInstance());

    if (error == kErrorNotFound)
    {
        // Frame may not contain an IPv6 header.
        ExitNow(error = kErrorNone);
    }

    error = Get<Mle::MleRouter>().CheckReachability(aMeshAddrs.mDestination.GetShort(), ip6Headers.GetIp6Header());

    if (error == kErrorNoRoute)
    {
        SendDestinationUnreachable(aMeshAddrs.mSource.GetShort(), ip6Headers);
    }

exit:
    return error;
}

void MeshForwarder::SendDestinationUnreachable(uint16_t aMeshSource, const Ip6::Headers &aIp6Headers)
{
    Ip6::MessageInfo messageInfo;

    messageInfo.GetPeerAddr() = Get<Mle::MleRouter>().GetMeshLocal16();
    messageInfo.GetPeerAddr().GetIid().SetLocator(aMeshSource);

    IgnoreError(Get<Ip6::Icmp>().SendError(Ip6::Icmp::Header::kTypeDstUnreach,
                                           Ip6::Icmp::Header::kCodeDstUnreachNoRoute, messageInfo, aIp6Headers));
}

void MeshForwarder::HandleMesh(FrameData &aFrameData, const Mac::Address &aMacSource, const ThreadLinkInfo &aLinkInfo)
{
    Error              error = kErrorNone;
    Mac::Addresses     meshAddrs;
    Lowpan::MeshHeader meshHeader;

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aLinkInfo.IsLinkSecurityEnabled(), error = kErrorSecurity);

    SuccessOrExit(error = meshHeader.ParseFrom(aFrameData));

    meshAddrs.mSource.SetShort(meshHeader.GetSource());
    meshAddrs.mDestination.SetShort(meshHeader.GetDestination());

    UpdateRoutes(aFrameData, meshAddrs);

    if (meshAddrs.mDestination.GetShort() == Get<Mac::Mac>().GetShortAddress() ||
        Get<Mle::MleRouter>().IsMinimalChild(meshAddrs.mDestination.GetShort()))
    {
        if (Lowpan::FragmentHeader::IsFragmentHeader(aFrameData))
        {
            HandleFragment(aFrameData, meshAddrs, aLinkInfo);
        }
        else if (Lowpan::Lowpan::IsLowpanHc(aFrameData))
        {
            HandleLowpanHC(aFrameData, meshAddrs, aLinkInfo);
        }
        else
        {
            ExitNow(error = kErrorParse);
        }
    }
    else if (meshHeader.GetHopsLeft() > 0)
    {
        OwnedPtr<Message> messagePtr;
        Message::Priority priority = Message::kPriorityNormal;

        Get<Mle::MleRouter>().ResolveRoutingLoops(aMacSource.GetShort(), meshAddrs.mDestination.GetShort());

        SuccessOrExit(error = CheckReachability(aFrameData, meshAddrs));

        meshHeader.DecrementHopsLeft();

        GetForwardFramePriority(aFrameData, meshAddrs, priority);
        messagePtr.Reset(
            Get<MessagePool>().Allocate(Message::kType6lowpan, /* aReserveHeader */ 0, Message::Settings(priority)));
        VerifyOrExit(messagePtr != nullptr, error = kErrorNoBufs);

        SuccessOrExit(error = meshHeader.AppendTo(*messagePtr));
        SuccessOrExit(error = messagePtr->AppendData(aFrameData));

        messagePtr->SetLinkInfo(aLinkInfo);

#if OPENTHREAD_CONFIG_MULTI_RADIO
        // We set the received radio type on the message in order for it
        // to be logged correctly from LogMessage().
        messagePtr->SetRadioType(static_cast<Mac::RadioType>(aLinkInfo.mRadioType));
#endif

        LogMessage(kMessageReceive, *messagePtr, kErrorNone, &aMacSource);

#if OPENTHREAD_CONFIG_MULTI_RADIO
        // Since the message will be forwarded, we clear the radio
        // type on the message to allow the radio type for tx to be
        // selected later (based on the radios supported by the next
        // hop).
        messagePtr->ClearRadioType();
#endif

        SendMessage(messagePtr.PassOwnership());
    }

exit:

    if (error != kErrorNone)
    {
        LogInfo("Dropping rx mesh frame, error:%s, len:%d, src:%s, sec:%s", ErrorToString(error),
                aFrameData.GetLength(), aMacSource.ToString().AsCString(), ToYesNo(aLinkInfo.IsLinkSecurityEnabled()));
    }
}

void MeshForwarder::UpdateRoutes(const FrameData &aFrameData, const Mac::Addresses &aMeshAddrs)
{
    Ip6::Headers ip6Headers;
    Neighbor    *neighbor;

    VerifyOrExit(!aMeshAddrs.mDestination.IsBroadcast() && aMeshAddrs.mSource.IsShort());

    SuccessOrExit(ip6Headers.DecompressFrom(aFrameData, aMeshAddrs, GetInstance()));

    if (!ip6Headers.GetSourceAddress().GetIid().IsLocator() &&
        Get<NetworkData::Leader>().IsOnMesh(ip6Headers.GetSourceAddress()))
    {
        // FTDs MAY add/update EID-to-RLOC Map Cache entries by
        // inspecting packets being received only for on mesh
        // addresses.

        Get<AddressResolver>().UpdateSnoopedCacheEntry(ip6Headers.GetSourceAddress(), aMeshAddrs.mSource.GetShort(),
                                                       aMeshAddrs.mDestination.GetShort());
    }

    neighbor = Get<NeighborTable>().FindNeighbor(ip6Headers.GetSourceAddress());
    VerifyOrExit(neighbor != nullptr && !neighbor->IsFullThreadDevice());

    if (!Mle::RouterIdMatch(aMeshAddrs.mSource.GetShort(), Get<Mac::Mac>().GetShortAddress()))
    {
        Get<Mle::MleRouter>().RemoveNeighbor(*neighbor);
    }

exit:
    return;
}

bool MeshForwarder::FragmentPriorityList::UpdateOnTimeTick(void)
{
    bool continueRxingTicks = false;

    for (Entry &entry : mEntries)
    {
        if (!entry.IsExpired())
        {
            entry.DecrementLifetime();

            if (!entry.IsExpired())
            {
                continueRxingTicks = true;
            }
        }
    }

    return continueRxingTicks;
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
        VerifyOrExit(aFragmentHeader.GetDatagramOffset() == 0);

        mFragmentPriorityList.AllocateEntry(aSrcRloc16, aFragmentHeader.GetDatagramTag(), aPriority);
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMeshForwarder);
        ExitNow();
    }

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    OT_UNUSED_VARIABLE(aFragmentLength);
#else
    // We can clear the entry in `mFragmentPriorityList` if it is the
    // last fragment. But if "delay aware active queue management" is
    // used we need to keep entry until the message is sent.
    if (aFragmentHeader.GetDatagramOffset() + aFragmentLength >= aFragmentHeader.GetDatagramSize())
    {
        entry->Clear();
    }
    else
#endif
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
            entry.Clear();
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

Error MeshForwarder::GetFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                         uint16_t                aSrcRloc16,
                                         Message::Priority      &aPriority)
{
    Error                        error = kErrorNone;
    FragmentPriorityList::Entry *entry;

    entry = mFragmentPriorityList.FindEntry(aSrcRloc16, aFragmentHeader.GetDatagramTag());
    VerifyOrExit(entry != nullptr, error = kErrorNotFound);
    aPriority = entry->GetPriority();

exit:
    return error;
}

void MeshForwarder::GetForwardFramePriority(const FrameData      &aFrameData,
                                            const Mac::Addresses &aMeshAddrs,
                                            Message::Priority    &aPriority)
{
    Error                  error      = kErrorNone;
    FrameData              frameData  = aFrameData;
    bool                   isFragment = false;
    Lowpan::FragmentHeader fragmentHeader;

    if (fragmentHeader.ParseFrom(frameData) == kErrorNone)
    {
        isFragment = true;

        if (fragmentHeader.GetDatagramOffset() > 0)
        {
            // Get priority from the pre-buffered info
            ExitNow(error = GetFragmentPriority(fragmentHeader, aMeshAddrs.mSource.GetShort(), aPriority));
        }
    }

    // Get priority from IPv6 header or UDP destination port directly
    error = GetFramePriority(frameData, aMeshAddrs, aPriority);

exit:
    if (error != kErrorNone)
    {
        LogNote("Failed to get forwarded frame priority, error:%s, len:%d, src:%s, dst:%s", ErrorToString(error),
                frameData.GetLength(), aMeshAddrs.mSource.ToString().AsCString(),
                aMeshAddrs.mDestination.ToString().AsCString());
    }
    else if (isFragment)
    {
        UpdateFragmentPriority(fragmentHeader, frameData.GetLength(), aMeshAddrs.mSource.GetShort(), aPriority);
    }
}

// LCOV_EXCL_START

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

Error MeshForwarder::LogMeshFragmentHeader(MessageAction       aAction,
                                           const Message      &aMessage,
                                           const Mac::Address *aMacAddress,
                                           Error               aError,
                                           uint16_t           &aOffset,
                                           Mac::Addresses     &aMeshAddrs,
                                           LogLevel            aLogLevel)
{
    Error                  error             = kErrorFailed;
    bool                   hasFragmentHeader = false;
    bool                   shouldLogRss;
    Lowpan::MeshHeader     meshHeader;
    Lowpan::FragmentHeader fragmentHeader;
    uint16_t               headerLength;
    bool                   shouldLogRadio = false;
    const char            *radioString    = "";

    SuccessOrExit(meshHeader.ParseFrom(aMessage, headerLength));

    aMeshAddrs.mSource.SetShort(meshHeader.GetSource());
    aMeshAddrs.mDestination.SetShort(meshHeader.GetDestination());

    aOffset = headerLength;

    if (fragmentHeader.ParseFrom(aMessage, aOffset, headerLength) == kErrorNone)
    {
        hasFragmentHeader = true;
        aOffset += headerLength;
    }

    shouldLogRss = (aAction == kMessageReceive) || (aAction == kMessageReassemblyDrop);

#if OPENTHREAD_CONFIG_MULTI_RADIO
    shouldLogRadio = true;
    radioString    = aMessage.IsRadioTypeSet() ? RadioTypeToString(aMessage.GetRadioType()) : "all";
#endif

    LogAt(aLogLevel, "%s mesh frame, len:%d%s%s, msrc:%s, mdst:%s, hops:%d, frag:%s, sec:%s%s%s%s%s%s%s",
          MessageActionToString(aAction, aError), aMessage.GetLength(),
          (aMacAddress == nullptr) ? "" : ((aAction == kMessageReceive) ? ", from:" : ", to:"),
          (aMacAddress == nullptr) ? "" : aMacAddress->ToString().AsCString(),
          aMeshAddrs.mSource.ToString().AsCString(), aMeshAddrs.mDestination.ToString().AsCString(),
          meshHeader.GetHopsLeft() + ((aAction == kMessageReceive) ? 1 : 0), ToYesNo(hasFragmentHeader),
          ToYesNo(aMessage.IsLinkSecurityEnabled()),
          (aError == kErrorNone) ? "" : ", error:", (aError == kErrorNone) ? "" : ErrorToString(aError),
          shouldLogRss ? ", rss:" : "", shouldLogRss ? aMessage.GetRssAverager().ToString().AsCString() : "",
          shouldLogRadio ? ", radio:" : "", radioString);

    if (hasFragmentHeader)
    {
        LogAt(aLogLevel, "    Frag tag:%04x, offset:%d, size:%d", fragmentHeader.GetDatagramTag(),
              fragmentHeader.GetDatagramOffset(), fragmentHeader.GetDatagramSize());

        VerifyOrExit(fragmentHeader.GetDatagramOffset() == 0);
    }

    error = kErrorNone;

exit:
    return error;
}

void MeshForwarder::LogMeshIpHeader(const Message        &aMessage,
                                    uint16_t              aOffset,
                                    const Mac::Addresses &aMeshAddrs,
                                    LogLevel              aLogLevel)
{
    Ip6::Headers headers;

    SuccessOrExit(headers.DecompressFrom(aMessage, aOffset, aMeshAddrs));

    LogAt(aLogLevel, "    IPv6 %s msg, chksum:%04x, ecn:%s, prio:%s", Ip6::Ip6::IpProtoToString(headers.GetIpProto()),
          headers.GetChecksum(), Ip6::Ip6::EcnToString(headers.GetEcn()), MessagePriorityToString(aMessage));

    LogIp6SourceDestAddresses(headers, aLogLevel);

exit:
    return;
}

void MeshForwarder::LogMeshMessage(MessageAction       aAction,
                                   const Message      &aMessage,
                                   const Mac::Address *aMacAddress,
                                   Error               aError,
                                   LogLevel            aLogLevel)
{
    uint16_t       offset;
    Mac::Addresses meshAddrs;

    SuccessOrExit(LogMeshFragmentHeader(aAction, aMessage, aMacAddress, aError, offset, meshAddrs, aLogLevel));

    // When log action is `kMessageTransmit` we do not include
    // the IPv6 header info in the logs, as the same info is
    // logged when the same Mesh Header message was received
    // and info about it was logged.

    VerifyOrExit(aAction != kMessageTransmit);

    LogMeshIpHeader(aMessage, offset, meshAddrs, aLogLevel);

exit:
    return;
}

#endif // #if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_NOTE)

// LCOV_EXCL_STOP

} // namespace ot

#endif // OPENTHREAD_FTD
