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
    SuccessOrExit(error = mSendQueue.Enqueue(aMessage));

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
                    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValidOrRestoring); !iter.IsDone();
                         iter++)
                    {
                        Child &child = *iter.GetChild();

                        if (!child.IsRxOnWhenIdle())
                        {
                            mIndirectSender.AddMessageForSleepyChild(aMessage, child);
                        }
                    }
                }
                else
                {
                    // destined for some sleepy children which subscribed the multicast address.
                    for (ChildTable::Iterator iter(GetInstance(), Child::kInStateValidOrRestoring); !iter.IsDone();
                         iter++)
                    {
                        Child &child = *iter.GetChild();

                        if (mle.IsSleepyChildSubscribed(ip6Header.GetDestination(), child))
                        {
                            mIndirectSender.AddMessageForSleepyChild(aMessage, child);
                        }
                    }
                }
            }
        }
        else if ((neighbor = mle.GetNeighbor(ip6Header.GetDestination())) != NULL && !neighbor->IsRxOnWhenIdle() &&
                 !aMessage.GetDirectTransmission())
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
        assert((child != NULL) && !child->IsRxOnWhenIdle());
        mIndirectSender.AddMessageForSleepyChild(aMessage, *child);
        break;
    }

    default:
        aMessage.SetDirectTransmission();
        break;
    }

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
                LogMessage(kMessageDrop, *cur, NULL, aError);
                cur->Free();
            }
        }
    }

    if (enqueuedMessage)
    {
        mScheduleTransmissionTask.Post();
    }
}

otError MeshForwarder::EvictMessage(uint8_t aPriority)
{
    otError  error = OT_ERROR_NOT_FOUND;
    Message *message;

    VerifyOrExit((message = mSendQueue.GetTail()) != NULL);

    if (message->GetPriority() < aPriority)
    {
        VerifyOrExit(!message->GetDoNotEvict());
        RemoveMessage(*message);
        ExitNow(error = OT_ERROR_NONE);
    }
    else
    {
        while (aPriority <= Message::kPriorityNet)
        {
            for (message = mSendQueue.GetHeadForPriority(aPriority); message && (message->GetPriority() == aPriority);
                 message = message->GetNext())
            {
                if (message->IsChildPending())
                {
                    RemoveMessage(*message);
                    ExitNow(error = OT_ERROR_NONE);
                }
            }

            aPriority++;
        }
    }

exit:
    return error;
}

void MeshForwarder::RemoveMessages(Child &aChild, uint8_t aSubType)
{
    Mle::MleRouter &mle = Get<Mle::MleRouter>();
    Message *       nextMessage;

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

                if (&aChild == static_cast<Child *>(mle.GetNeighbor(ip6header.GetDestination())))
                {
                    message->ClearDirectTransmission();
                }

                break;
            }

            case Message::kType6lowpan:
            {
                Lowpan::MeshHeader meshHeader;

                IgnoreReturnValue(meshHeader.ParseFrom(*message));

                if (&aChild == static_cast<Child *>(mle.GetNeighbor(meshHeader.GetDestination())))
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
            for (ChildTable::Iterator iter(GetInstance(), Child::kInStateAnyExceptInvalid); !iter.IsDone(); iter++)
            {
                IgnoreReturnValue(mIndirectSender.RemoveMessageFromSleepyChild(*message, *iter.GetChild()));
            }
        }

        if (mSendMessage == message)
        {
            mSendMessage = NULL;
        }

        mSendQueue.Dequeue(*message);
        LogMessage(kMessageDrop, *message, NULL, OT_ERROR_NONE);
        message->Free();
    }
}

void MeshForwarder::SendMesh(Message &aMessage, Mac::TxFrame &aFrame)
{
    uint16_t fcf;

    // initialize MAC header
    fcf = Mac::Frame::kFcfFrameData | Mac::Frame::kFcfPanidCompression | Mac::Frame::kFcfFrameVersion2006 |
          Mac::Frame::kFcfDstAddrShort | Mac::Frame::kFcfSrcAddrShort | Mac::Frame::kFcfAckRequest |
          Mac::Frame::kFcfSecurityEnabled;

    aFrame.InitMacHeader(fcf, Mac::Frame::kKeyIdMode1 | Mac::Frame::kSecEncMic32);
    aFrame.SetDstPanId(Get<Mac::Mac>().GetPanId());
    aFrame.SetDstAddr(mMacDest.GetShort());
    aFrame.SetSrcAddr(mMacSource.GetShort());

    // write payload
    assert(aMessage.GetLength() <= aFrame.GetMaxPayloadLength());
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

    IgnoreReturnValue(meshHeader.ParseFrom(aMessage));

    nextHop = Get<Mle::MleRouter>().GetNextHop(meshHeader.GetDestination());

    if (nextHop != Mac::kShortAddrInvalid)
    {
        neighbor = Get<Mle::MleRouter>().GetNeighbor(nextHop);
    }
    else
    {
        neighbor = Get<Mle::MleRouter>().GetNeighbor(meshHeader.GetDestination());
    }

    if (neighbor == NULL)
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

otError MeshForwarder::UpdateIp6RouteFtd(Ip6::Header &ip6Header)
{
    Mle::MleRouter &mle   = Get<Mle::MleRouter>();
    otError         error = OT_ERROR_NONE;
    Neighbor *      neighbor;

    if (mle.IsRoutingLocator(ip6Header.GetDestination()))
    {
        uint16_t rloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);
        VerifyOrExit(mle.IsRouterIdValid(Mle::Mle::RouterIdFromRloc16(rloc16)), error = OT_ERROR_DROP);
        mMeshDest = rloc16;
    }
    else if (mle.IsAnycastLocator(ip6Header.GetDestination()))
    {
        uint16_t aloc16 = HostSwap16(ip6Header.GetDestination().mFields.m16[7]);

        if (aloc16 == Mle::kAloc16Leader)
        {
            mMeshDest = Mle::Mle::Rloc16FromRouterId(mle.GetLeaderId());
        }
        else if ((aloc16 >= Mle::kAloc16CommissionerStart) && (aloc16 <= Mle::kAloc16CommissionerEnd))
        {
            SuccessOrExit(error = MeshCoP::GetBorderAgentRloc(Get<ThreadNetif>(), mMeshDest));
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
        else if ((aloc16 >= Mle::kAloc16ServiceStart) && (aloc16 <= Mle::kAloc16ServiceEnd))
        {
            SuccessOrExit(error = GetDestinationRlocByServiceAloc(aloc16, mMeshDest));
        }

        else
        {
            // TODO: support for Neighbor Discovery Agent ALOC
            ExitNow(error = OT_ERROR_DROP);
        }
    }
    else if ((neighbor = mle.GetNeighbor(ip6Header.GetDestination())) != NULL)
    {
        mMeshDest = neighbor->GetRloc16();
    }
    else if (Get<NetworkData::Leader>().IsOnMesh(ip6Header.GetDestination()))
    {
        SuccessOrExit(error = Get<AddressResolver>().Resolve(ip6Header.GetDestination(), mMeshDest));
    }
    else
    {
        Get<NetworkData::Leader>().RouteLookup(ip6Header.GetSource(), ip6Header.GetDestination(), NULL, &mMeshDest);
    }

    VerifyOrExit(mMeshDest != Mac::kShortAddrInvalid, error = OT_ERROR_DROP);

    mMeshSource = Get<Mac::Mac>().GetShortAddress();

    SuccessOrExit(error = mle.CheckReachability(mMeshSource, mMeshDest, ip6Header));
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

otError MeshForwarder::CheckReachability(uint8_t *           aFrame,
                                         uint16_t            aFrameLength,
                                         const Mac::Address &aMeshSource,
                                         const Mac::Address &aMeshDest)
{
    otError     error = OT_ERROR_NONE;
    Ip6::Header ip6Header;

    SuccessOrExit(error = GetIp6Header(aFrame, aFrameLength, aMeshSource, aMeshDest, ip6Header));
    error = Get<Mle::MleRouter>().CheckReachability(aMeshSource.GetShort(), aMeshDest.GetShort(), ip6Header);

exit:
    // the message may not contain an IPv6 header
    if (error == OT_ERROR_NOT_FOUND)
    {
        error = OT_ERROR_NONE;
    }
    else if (error != OT_ERROR_NONE)
    {
        error = OT_ERROR_DROP;
    }

    return error;
}

void MeshForwarder::HandleMesh(uint8_t *               aFrame,
                               uint16_t                aFrameLength,
                               const Mac::Address &    aMacSource,
                               const otThreadLinkInfo &aLinkInfo)
{
    otError            error   = OT_ERROR_NONE;
    Message *          message = NULL;
    Mac::Address       meshDest;
    Mac::Address       meshSource;
    Lowpan::MeshHeader meshHeader;
    uint16_t           headerLength;

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aLinkInfo.mLinkSecurity, error = OT_ERROR_SECURITY);

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
        uint8_t  priority = kDefaultMsgPriority;
        uint16_t offset   = 0;

        Get<Mle::MleRouter>().ResolveRoutingLoops(aMacSource.GetShort(), meshDest.GetShort());

        SuccessOrExit(error = CheckReachability(aFrame, aFrameLength, meshSource, meshDest));

        meshHeader.DecrementHopsLeft();

        GetForwardFramePriority(aFrame, aFrameLength, meshSource, meshDest, priority);
        message = Get<MessagePool>().New(Message::kType6lowpan, priority);
        VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

        SuccessOrExit(error = message->SetLength(meshHeader.GetHeaderLength() + aFrameLength));
        offset += meshHeader.WriteTo(*message, offset);
        message->Write(offset, aFrameLength, aFrame);
        message->SetLinkSecurityEnabled(aLinkInfo.mLinkSecurity);
        message->SetPanId(aLinkInfo.mPanId);
        message->AddRss(aLinkInfo.mRss);

        LogMessage(kMessageReceive, *message, &aMacSource, OT_ERROR_NONE);

        SendMessage(*message);
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogInfoMac("Dropping rx mesh frame, error:%s, len:%d, src:%s, sec:%s", otThreadErrorToString(error),
                     aFrameLength, aMacSource.ToString().AsCString(), aLinkInfo.mLinkSecurity ? "yes" : "no");

        if (message != NULL)
        {
            message->Free();
        }
    }
}

void MeshForwarder::UpdateRoutes(uint8_t *           aFrame,
                                 uint16_t            aFrameLength,
                                 const Mac::Address &aMeshSource,
                                 const Mac::Address &aMeshDest)
{
    Ip6::Header ip6Header;
    Neighbor *  neighbor;

    VerifyOrExit(!aMeshDest.IsBroadcast() && aMeshSource.IsShort());
    SuccessOrExit(GetIp6Header(aFrame, aFrameLength, aMeshSource, aMeshDest, ip6Header));

    if (!ip6Header.GetSource().IsRoutingLocator() && !ip6Header.GetSource().IsAnycastRoutingLocator() &&
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
                Get<AddressResolver>().AddCacheEntry(ip6Header.GetSource(), aMeshSource.GetShort());
            }
        }
    }

    neighbor = Get<Mle::MleRouter>().GetNeighbor(ip6Header.GetSource());
    VerifyOrExit(neighbor != NULL && !neighbor->IsFullThreadDevice());

    if (!Mle::Mle::RouterIdMatch(aMeshSource.GetShort(), Get<Mac::Mac>().GetShortAddress()))
    {
        Get<Mle::MleRouter>().RemoveNeighbor(*neighbor);
    }

exit:
    return;
}

bool MeshForwarder::UpdateFragmentLifetime(void)
{
    bool shouldRun = false;

    for (FragmentPriorityEntry *entry = &mFragmentEntries[0]; entry < OT_ARRAY_END(mFragmentEntries); entry++)
    {
        if (entry->GetLifetime() != 0)
        {
            entry->DecrementLifetime();

            if (entry->GetLifetime() != 0)
            {
                shouldRun = true;
            }
        }
    }

    return shouldRun;
}

void MeshForwarder::UpdateFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                           uint16_t                aFragmentLength,
                                           uint16_t                aSrcRloc16,
                                           uint8_t                 aPriority)
{
    FragmentPriorityEntry *entry;

    if (aFragmentHeader.GetDatagramOffset() == 0)
    {
        VerifyOrExit((entry = GetUnusedFragmentPriorityEntry()) != NULL);

        entry->SetDatagramTag(aFragmentHeader.GetDatagramTag());
        entry->SetSrcRloc16(aSrcRloc16);
        entry->SetPriority(aPriority);
        entry->SetLifetime(kReassemblyTimeout);

        if (!mUpdateTimer.IsRunning())
        {
            mUpdateTimer.Start(kStateUpdatePeriod);
        }
    }
    else
    {
        VerifyOrExit((entry = FindFragmentPriorityEntry(aFragmentHeader.GetDatagramTag(), aSrcRloc16)) != NULL);

        entry->SetLifetime(kReassemblyTimeout);

        if (aFragmentHeader.GetDatagramOffset() + aFragmentLength >= aFragmentHeader.GetDatagramSize())
        {
            entry->SetLifetime(0);
        }
    }

exit:
    return;
}

FragmentPriorityEntry *MeshForwarder::FindFragmentPriorityEntry(uint16_t aTag, uint16_t aSrcRloc16)
{
    FragmentPriorityEntry *entry;

    for (entry = &mFragmentEntries[0]; entry < OT_ARRAY_END(mFragmentEntries); entry++)
    {
        if ((entry->GetLifetime() != 0) && (entry->GetDatagramTag() == aTag) && (entry->GetSrcRloc16() == aSrcRloc16))
        {
            ExitNow();
        }
    }

    entry = NULL;

exit:
    return entry;
}

FragmentPriorityEntry *MeshForwarder::GetUnusedFragmentPriorityEntry(void)
{
    FragmentPriorityEntry *entry;

    for (entry = &mFragmentEntries[0]; entry < OT_ARRAY_END(mFragmentEntries); entry++)
    {
        if (entry->GetLifetime() == 0)
        {
            ExitNow();
        }
    }

    entry = NULL;

exit:
    return entry;
}

otError MeshForwarder::GetFragmentPriority(Lowpan::FragmentHeader &aFragmentHeader,
                                           uint16_t                aSrcRloc16,
                                           uint8_t &               aPriority)
{
    otError                error = OT_ERROR_NONE;
    FragmentPriorityEntry *entry;

    entry = FindFragmentPriorityEntry(aFragmentHeader.GetDatagramTag(), aSrcRloc16);
    VerifyOrExit(entry != NULL, error = OT_ERROR_NOT_FOUND);
    aPriority = entry->GetPriority();

exit:
    return error;
}

otError MeshForwarder::GetForwardFramePriority(const uint8_t *     aFrame,
                                               uint16_t            aFrameLength,
                                               const Mac::Address &aMeshSource,
                                               const Mac::Address &aMeshDest,
                                               uint8_t &           aPriority)
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

    return error;
}

otError MeshForwarder::GetDestinationRlocByServiceAloc(uint16_t aServiceAloc, uint16_t &aMeshDest)
{
    otError                  error      = OT_ERROR_NONE;
    uint8_t                  serviceId  = Mle::Mle::ServiceIdFromAloc(aServiceAloc);
    NetworkData::ServiceTlv *serviceTlv = Get<NetworkData::Leader>().FindServiceById(serviceId);

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
                curCost = Get<Mle::MleRouter>().GetCost(server->GetServer16());

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
        (aMacAddress == NULL) ? "" : ((aAction == kMessageReceive) ? ", from:" : ", to:"),
        (aMacAddress == NULL) ? "" : aMacAddress->ToString().AsCString(), aMeshSource.ToString().AsCString(),
        aMeshDest.ToString().AsCString(), meshHeader.GetHopsLeft() + ((aAction == kMessageReceive) ? 1 : 0),
        hasFragmentHeader ? "yes" : "no", aMessage.IsLinkSecurityEnabled() ? "yes" : "no",
        (aError == OT_ERROR_NONE) ? "" : ", error:", (aError == OT_ERROR_NONE) ? "" : otThreadErrorToString(aError),
        shouldLogRss ? ", rss:" : "", shouldLogRss ? aMessage.GetRssAverager().ToString().AsCString() : "");

    if (hasFragmentHeader)
    {
        otLogMac(aLogLevel, "    Frag tag:%04x, offset:%d, size:%d", fragmentHeader.GetDatagramTag(),
                 fragmentHeader.GetDatagramOffset(), fragmentHeader.GetDatagramSize());

        VerifyOrExit(fragmentHeader.GetDatagramOffset() == 0);
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
        Ip6::UdpHeader udp;
        Ip6::TcpHeader tcp;
    } header;

    aChecksum   = 0;
    aSourcePort = 0;
    aDestPort   = 0;

    // Read and decompress the IPv6 header

    frameLength = aMessage.Read(aOffset, sizeof(frameBuffer), frameBuffer);

    headerLength = Get<Lowpan::Lowpan>().DecompressBaseHeader(aIp6Header, nextHeaderCompressed, aMeshSource, aMeshDest,
                                                              frameBuffer, frameLength);
    VerifyOrExit(headerLength >= 0);

    aOffset += headerLength;

    // Read and decompress UDP or TCP header

    switch (aIp6Header.GetNextHeader())
    {
    case Ip6::kProtoUdp:
        if (nextHeaderCompressed)
        {
            frameLength  = aMessage.Read(aOffset, sizeof(Ip6::UdpHeader), frameBuffer);
            headerLength = Get<Lowpan::Lowpan>().DecompressUdpHeader(header.udp, frameBuffer, frameLength);
            VerifyOrExit(headerLength >= 0);
        }
        else
        {
            VerifyOrExit(sizeof(Ip6::UdpHeader) == aMessage.Read(aOffset, sizeof(Ip6::UdpHeader), &header.udp));
        }

        aChecksum   = header.udp.GetChecksum();
        aSourcePort = header.udp.GetSourcePort();
        aDestPort   = header.udp.GetDestinationPort();
        break;

    case Ip6::kProtoTcp:
        VerifyOrExit(sizeof(Ip6::TcpHeader) == aMessage.Read(aOffset, sizeof(Ip6::TcpHeader), &header.tcp));
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

    VerifyOrExit(aAction != kMessageTransmit);

    LogMeshIpHeader(aMessage, offset, meshSource, meshDest, aLogLevel);

exit:
    return;
}

#endif // #if (OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_NOTE) && (OPENTHREAD_CONFIG_LOG_MAC == 1)

// LCOV_EXCL_STOP

} // namespace ot

#endif // OPENTHREAD_FTD
