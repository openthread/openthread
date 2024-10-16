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

#include "instance/instance.hpp"

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
            RemoveMessageIfNoPendingTx(message);
            continue;
        }

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        // Pass back to IPv6 layer for DUA destination resolved
        // by Backbone Query
        if (Get<BackboneRouter::Local>().IsPrimary() && Get<BackboneRouter::Leader>().IsDomainUnicast(ip6Dst) &&
            Get<Mle::Mle>().HasRloc16(Get<AddressResolver>().LookUp(ip6Dst)))
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

            if (!message->GetIndirectTxChildMask().IsEmpty())
            {
                evict = message;
                ExitNow(error = kErrorNone);
            }
        }
    }

exit:
    if ((error == kErrorNone) && (evict != nullptr))
    {
        FinalizeAndRemoveMessage(*evict, kErrorNoBufs, kMessageEvict);
    }

    return error;
}

void MeshForwarder::RemoveMessagesForChild(Child &aChild, MessageChecker &aMessageChecker)
{
    for (Message &message : mSendQueue)
    {
        if (!aMessageChecker(message))
        {
            continue;
        }

        if (mIndirectSender.RemoveMessageFromSleepyChild(message, aChild) != kErrorNone)
        {
            const Neighbor *neighbor = nullptr;

            if (message.GetType() == Message::kTypeIp6)
            {
                Ip6::Header ip6header;

                IgnoreError(message.Read(0, ip6header));
                neighbor = Get<NeighborTable>().FindNeighbor(ip6header.GetDestination());
            }
            else if (message.GetType() == Message::kType6lowpan)
            {
                Lowpan::MeshHeader meshHeader;

                IgnoreError(meshHeader.ParseFrom(message));
                neighbor = Get<NeighborTable>().FindNeighbor(meshHeader.GetDestination());
            }

            if (&aChild == neighbor)
            {
                message.ClearDirectTransmission();
            }
        }

        RemoveMessageIfNoPendingTx(message);
    }
}

void MeshForwarder::FinalizeMessageIndirectTxs(Message &aMessage)
{
    VerifyOrExit(!aMessage.GetIndirectTxChildMask().IsEmpty());

    for (Child &child : Get<ChildTable>().Iterate(Child::kInStateAnyExceptInvalid))
    {
        IgnoreError(mIndirectSender.RemoveMessageFromSleepyChild(aMessage, child));
        VerifyOrExit(!aMessage.GetIndirectTxChildMask().IsEmpty());
    }

exit:
    return;
}

void MeshForwarder::RemoveDataResponseMessages(void)
{
    for (Message &message : mSendQueue)
    {
        if (message.IsMleCommand(Mle::kCommandDataResponse))
        {
            FinalizeAndRemoveMessage(message, kErrorDrop, kMessageDrop);
        }
    }
}

void MeshForwarder::SendMesh(Message &aMessage, Mac::TxFrame &aFrame)
{
    Mac::TxFrame::Info frameInfo;

    frameInfo.mType          = Mac::Frame::kTypeData;
    frameInfo.mAddrs         = mMacAddrs;
    frameInfo.mSecurityLevel = Mac::Frame::kSecurityEncMic32;
    frameInfo.mKeyIdMode     = Mac::Frame::kKeyIdMode1;
    frameInfo.mPanIds.SetBothSourceDestination(Get<Mac::Mac>().GetPanId());

    PrepareMacHeaders(aFrame, frameInfo, &aMessage);

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

    nextHop = Get<RouterTable>().GetNextHop(meshHeader.GetDestination());

    if (nextHop != Mle::kInvalidRloc16)
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
    mMacAddrs.mSource.SetShort(Get<Mle::Mle>().GetRloc16());

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

        SuccessOrExit(error = Get<NetworkData::Leader>().AnycastLookup(aloc16, mMeshDest));

        // If the selected ALOC destination, `mMeshDest`, is a sleepy
        // child of this device, prepare the message for indirect tx
        // to the sleepy child and un-mark message for direct tx.

        if (mle.IsRouterOrLeader() && Mle::IsChildRloc16(mMeshDest) && mle.HasMatchingRouterIdWith(mMeshDest))
        {
            Child *child = Get<ChildTable>().FindChild(mMeshDest, Child::kInStateValid);

            VerifyOrExit(child != nullptr, error = kErrorDrop);

            if (!child->IsRxOnWhenIdle())
            {
                mIndirectSender.AddMessageForSleepyChild(aMessage, *child);
                aMessage.ClearDirectTransmission();
            }
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

    VerifyOrExit(mMeshDest != Mle::kInvalidRloc16, error = kErrorDrop);

    mMeshSource = Get<Mle::Mle>().GetRloc16();

    SuccessOrExit(error = CheckReachability(mMeshDest, aIp6Header));
    aMessage.SetMeshDest(mMeshDest);
    mMacAddrs.mDestination.SetShort(Get<RouterTable>().GetNextHop(mMeshDest));

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

    error = CheckReachability(aMacAddrs.mDestination.GetShort(), ip6Headers.GetIp6Header());

    if (error == kErrorNoRoute)
    {
        SendDestinationUnreachable(aMacAddrs.mSource.GetShort(), ip6Headers);
    }

exit:
    return;
}

Error MeshForwarder::CheckReachability(RxInfo &aRxInfo)
{
    Error error;

    error = aRxInfo.ParseIp6Headers();

    switch (error)
    {
    case kErrorNone:
        break;
    case kErrorNotFound:
        // Frame may not contain an IPv6 header.
        error = kErrorNone;
        OT_FALL_THROUGH;
    default:
        ExitNow();
    }

    error = CheckReachability(aRxInfo.GetDstAddr().GetShort(), aRxInfo.mIp6Headers.GetIp6Header());

    if (error == kErrorNoRoute)
    {
        SendDestinationUnreachable(aRxInfo.GetSrcAddr().GetShort(), aRxInfo.mIp6Headers);
    }

exit:
    return error;
}

Error MeshForwarder::CheckReachability(uint16_t aMeshDest, const Ip6::Header &aIp6Header)
{
    bool isReachable = false;

    if (Get<Mle::Mle>().IsChild())
    {
        if (Get<Mle::Mle>().HasRloc16(aMeshDest))
        {
            isReachable = Get<ThreadNetif>().HasUnicastAddress(aIp6Header.GetDestination());
        }
        else
        {
            isReachable = true;
        }

        ExitNow();
    }

    if (Get<Mle::Mle>().HasRloc16(aMeshDest))
    {
        isReachable = Get<ThreadNetif>().HasUnicastAddress(aIp6Header.GetDestination()) ||
                      (Get<NeighborTable>().FindNeighbor(aIp6Header.GetDestination()) != nullptr);
        ExitNow();
    }

    if (Get<Mle::Mle>().HasMatchingRouterIdWith(aMeshDest))
    {
        isReachable = (Get<ChildTable>().FindChild(aMeshDest, Child::kInStateValidOrRestoring) != nullptr);
        ExitNow();
    }

    isReachable = (Get<RouterTable>().GetNextHop(aMeshDest) != Mle::kInvalidRloc16);

exit:
    return isReachable ? kErrorNone : kErrorNoRoute;
}

void MeshForwarder::SendDestinationUnreachable(uint16_t aMeshSource, const Ip6::Headers &aIp6Headers)
{
    Ip6::MessageInfo messageInfo;

    messageInfo.GetPeerAddr().SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), aMeshSource);

    IgnoreError(Get<Ip6::Icmp>().SendError(Ip6::Icmp::Header::kTypeDstUnreach,
                                           Ip6::Icmp::Header::kCodeDstUnreachNoRoute, messageInfo, aIp6Headers));
}

void MeshForwarder::HandleMesh(RxInfo &aRxInfo)
{
    Error              error = kErrorNone;
    Lowpan::MeshHeader meshHeader;
    Mac::Address       neighborMacSource;

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aRxInfo.IsLinkSecurityEnabled(), error = kErrorSecurity);

    SuccessOrExit(error = meshHeader.ParseFrom(aRxInfo.mFrameData));

    neighborMacSource = aRxInfo.GetSrcAddr();

    // Switch the `aRxInfo.mMacAddrs` to the mesh header source/destination

    aRxInfo.mMacAddrs.mSource.SetShort(meshHeader.GetSource());
    aRxInfo.mMacAddrs.mDestination.SetShort(meshHeader.GetDestination());

    UpdateRoutes(aRxInfo);

    if (Get<Mle::Mle>().HasRloc16(aRxInfo.GetDstAddr().GetShort()) ||
        Get<ChildTable>().HasMinimalChild(aRxInfo.GetDstAddr().GetShort()))
    {
        if (Lowpan::FragmentHeader::IsFragmentHeader(aRxInfo.mFrameData))
        {
            HandleFragment(aRxInfo);
        }
        else if (Lowpan::Lowpan::IsLowpanHc(aRxInfo.mFrameData))
        {
            HandleLowpanHc(aRxInfo);
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

        ResolveRoutingLoops(neighborMacSource.GetShort(), aRxInfo.GetDstAddr().GetShort());

        SuccessOrExit(error = CheckReachability(aRxInfo));

        meshHeader.DecrementHopsLeft();

        GetForwardFramePriority(aRxInfo, priority);
        messagePtr.Reset(
            Get<MessagePool>().Allocate(Message::kType6lowpan, /* aReserveHeader */ 0, Message::Settings(priority)));
        VerifyOrExit(messagePtr != nullptr, error = kErrorNoBufs);

        SuccessOrExit(error = meshHeader.AppendTo(*messagePtr));
        SuccessOrExit(error = messagePtr->AppendData(aRxInfo.mFrameData));

        messagePtr->UpdateLinkInfoFrom(aRxInfo.mLinkInfo);

        LogMessage(kMessageReceive, *messagePtr, kErrorNone, &neighborMacSource);

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
                aRxInfo.mFrameData.GetLength(), neighborMacSource.ToString().AsCString(),
                ToYesNo(aRxInfo.IsLinkSecurityEnabled()));
    }
}

void MeshForwarder::ResolveRoutingLoops(uint16_t aSourceRloc16, uint16_t aDestRloc16)
{
    // Resolves 2-hop routing loops.

    Router *router;

    if (aSourceRloc16 != Get<RouterTable>().GetNextHop(aDestRloc16))
    {
        ExitNow();
    }

    router = Get<RouterTable>().FindRouterByRloc16(aDestRloc16);
    VerifyOrExit(router != nullptr);

    router->SetNextHopToInvalid();
    Get<Mle::MleRouter>().ResetAdvertiseInterval();

exit:
    return;
}

void MeshForwarder::UpdateRoutes(RxInfo &aRxInfo)
{
    Neighbor *neighbor;

    VerifyOrExit(!aRxInfo.GetDstAddr().IsBroadcast() && aRxInfo.GetSrcAddr().IsShort());

    SuccessOrExit(aRxInfo.ParseIp6Headers());

    if (!aRxInfo.mIp6Headers.GetSourceAddress().GetIid().IsLocator() &&
        Get<NetworkData::Leader>().IsOnMesh(aRxInfo.mIp6Headers.GetSourceAddress()))
    {
        // FTDs MAY add/update EID-to-RLOC Map Cache entries by
        // inspecting packets being received only for on mesh
        // addresses.

        Get<AddressResolver>().UpdateSnoopedCacheEntry(
            aRxInfo.mIp6Headers.GetSourceAddress(), aRxInfo.GetSrcAddr().GetShort(), aRxInfo.GetDstAddr().GetShort());
    }

    neighbor = Get<NeighborTable>().FindNeighbor(aRxInfo.mIp6Headers.GetSourceAddress());
    VerifyOrExit(neighbor != nullptr && !neighbor->IsFullThreadDevice());

    if (!Get<Mle::Mle>().HasMatchingRouterIdWith(aRxInfo.GetSrcAddr().GetShort()))
    {
        Get<Mle::MleRouter>().RemoveNeighbor(*neighbor);
    }

exit:
    return;
}

void MeshForwarder::UpdateFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                           uint16_t                aFragmentLength,
                                           uint16_t                aSrcRloc16,
                                           Message::Priority       aPriority)
{
    FwdFrameInfo *entry;

    entry = FindFwdFrameInfoEntry(aSrcRloc16, aFragmentHeader.GetDatagramTag());

    if (entry == nullptr)
    {
        VerifyOrExit(aFragmentHeader.GetDatagramOffset() == 0);

        entry = mFwdFrameInfoArray.PushBack();
        VerifyOrExit(entry != nullptr);

        entry->Init(aSrcRloc16, aFragmentHeader.GetDatagramTag(), aPriority);
        Get<TimeTicker>().RegisterReceiver(TimeTicker::kMeshForwarder);

        ExitNow();
    }

#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    OT_UNUSED_VARIABLE(aFragmentLength);
#else
    // We can remove the entry in `mFwdFrameInfoArray` if it is the
    // last fragment. But if "delay aware active queue management" is
    // used we need to keep entry until the message is sent.
    if (aFragmentHeader.GetDatagramOffset() + aFragmentLength >= aFragmentHeader.GetDatagramSize())
    {
        mFwdFrameInfoArray.Remove(*entry);
    }
    else
#endif
    {
        entry->ResetLifetime();
    }

exit:
    return;
}

void MeshForwarder::FwdFrameInfo::Init(uint16_t aSrcRloc16, uint16_t aDatagramTag, Message::Priority aPriority)
{
    mSrcRloc16   = aSrcRloc16;
    mDatagramTag = aDatagramTag;
    mLifetime    = kLifetime;
    mPriority    = aPriority;
#if OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_ENABLE
    mShouldDrop = false;
#endif
}

bool MeshForwarder::FwdFrameInfo::Matches(const Info &aInfo) const
{
    return (mSrcRloc16 == aInfo.mSrcRloc16) && (mDatagramTag == aInfo.mDatagramTag);
}

MeshForwarder::FwdFrameInfo *MeshForwarder::FindFwdFrameInfoEntry(uint16_t aSrcRloc16, uint16_t aDatagramTag)
{
    FwdFrameInfo::Info info;

    info.mSrcRloc16   = aSrcRloc16;
    info.mDatagramTag = aDatagramTag;

    return mFwdFrameInfoArray.FindMatching(info);
}

bool MeshForwarder::UpdateFwdFrameInfoArrayOnTimeTick(void)
{
    for (FwdFrameInfo &entry : mFwdFrameInfoArray)
    {
        entry.DecrementLifetime();
    }

    mFwdFrameInfoArray.RemoveAllMatching(FwdFrameInfo::kIsExpired);

    return !mFwdFrameInfoArray.IsEmpty();
}

Error MeshForwarder::GetFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                         uint16_t                aSrcRloc16,
                                         Message::Priority      &aPriority)
{
    Error               error = kErrorNone;
    const FwdFrameInfo *entry = FindFwdFrameInfoEntry(aSrcRloc16, aFragmentHeader.GetDatagramTag());

    VerifyOrExit(entry != nullptr, error = kErrorNotFound);
    aPriority = entry->GetPriority();

exit:
    return error;
}

void MeshForwarder::GetForwardFramePriority(RxInfo &aRxInfo, Message::Priority &aPriority)
{
    // Determines the message priority to use for forwarding a
    // received mesh-header LowPAN frame towards its final
    // destination.

    Error                  error      = kErrorNone;
    bool                   isFragment = false;
    Lowpan::FragmentHeader fragmentHeader;
    FrameData              savedFrameData;

    // We save the `aRxInfo.mFrameData` before parsing the fragment
    // header which may update it to skip over the parsed header. We
    // restore the original frame data on `aRxInfo` before
    // returning.

    savedFrameData = aRxInfo.mFrameData;

    if (fragmentHeader.ParseFrom(aRxInfo.mFrameData) == kErrorNone)
    {
        isFragment = true;

        if (fragmentHeader.GetDatagramOffset() > 0)
        {
            // Get priority from the pre-buffered info
            ExitNow(error = GetFragmentPriority(fragmentHeader, aRxInfo.GetSrcAddr().GetShort(), aPriority));
        }
    }

    // Get priority from IPv6 header or UDP destination port directly
    error = GetFramePriority(aRxInfo, aPriority);

exit:
    if (error != kErrorNone)
    {
        LogNote("Failed to get forwarded frame priority, error:%s, %s", ErrorToString(error),
                aRxInfo.ToString().AsCString());
    }
    else if (isFragment)
    {
        UpdateFragmentPriority(fragmentHeader, aRxInfo.mFrameData.GetLength(), aRxInfo.GetSrcAddr().GetShort(),
                               aPriority);
    }

    aRxInfo.mFrameData = savedFrameData;
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
    Error                     error             = kErrorFailed;
    bool                      hasFragmentHeader = false;
    Lowpan::MeshHeader        meshHeader;
    Lowpan::FragmentHeader    fragmentHeader;
    uint16_t                  headerLength;
    String<kMaxLogStringSize> string;

    SuccessOrExit(meshHeader.ParseFrom(aMessage, headerLength));

    aMeshAddrs.mSource.SetShort(meshHeader.GetSource());
    aMeshAddrs.mDestination.SetShort(meshHeader.GetDestination());

    aOffset = headerLength;

    if (fragmentHeader.ParseFrom(aMessage, aOffset, headerLength) == kErrorNone)
    {
        hasFragmentHeader = true;
        aOffset += headerLength;
    }

    string.Append("%s mesh frame, len:%u, ", MessageActionToString(aAction, aError), aMessage.GetLength());

    AppendMacAddrToLogString(string, aAction, aMacAddress);

    string.Append("msrc:%s, mdst:%s, hops:%d, frag:%s, ", aMeshAddrs.mSource.ToString().AsCString(),
                  aMeshAddrs.mDestination.ToString().AsCString(),
                  meshHeader.GetHopsLeft() + ((aAction == kMessageReceive) ? 1 : 0), ToYesNo(hasFragmentHeader));

    AppendSecErrorPrioRssRadioLabelsToLogString(string, aAction, aMessage, aError);

    LogAt(aLogLevel, "%s", string.AsCString());

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
