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

#define WPP_NAME "mesh_forwarder_externmac_ftd.tmh"

#include "mesh_forwarder_externmac.hpp"

#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "net/ip6.hpp"
#include "net/tcp.hpp"
#include "net/udp6.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_USE_EXTERNAL_MAC

namespace ot {

otError MeshForwarder::SendMessage(Message &aMessage)
{
    ThreadNetif &netif      = GetNetif();
    ChildTable & childTable = netif.GetMle().GetChildTable();
    otError      error      = OT_ERROR_NONE;
    Neighbor *   neighbor;

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
                if (ip6Header.GetDestination() == netif.GetMle().GetLinkLocalAllThreadNodesAddress() ||
                    ip6Header.GetDestination() == netif.GetMle().GetRealmLocalAllThreadNodesAddress())
                {
                    // destined for all sleepy children
                    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValidOrRestoring); !iter.IsDone();
                         iter++)
                    {
                        Child &child = *iter.GetChild();

                        if (!child.IsRxOnWhenIdle())
                        {
                            aMessage.SetChildMask(childTable.GetChildIndex(child));
                            mSourceMatchController.IncrementMessageCount(child);
                        }
                    }
                }
                else
                {
                    // destined for some sleepy children which subscribed the multicast address.
                    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateValidOrRestoring); !iter.IsDone();
                         iter++)
                    {
                        Child &child = *iter.GetChild();

                        if (netif.GetMle().IsSleepyChildSubscribed(ip6Header.GetDestination(), child))
                        {
                            aMessage.SetChildMask(childTable.GetChildIndex(child));
                            mSourceMatchController.IncrementMessageCount(child);
                        }
                    }
                }
            }
        }
        else if ((neighbor = netif.GetMle().GetNeighbor(ip6Header.GetDestination())) != NULL &&
                 !neighbor->IsRxOnWhenIdle() && !aMessage.GetDirectTransmission())
        {
            // destined for a sleepy child
            Child &child = *static_cast<Child *>(neighbor);
            aMessage.SetChildMask(childTable.GetChildIndex(child));
            mSourceMatchController.IncrementMessageCount(child);
        }
        else
        {
            // schedule direct transmission
            aMessage.SetDirectTransmission();
        }

        break;
    }

    case Message::kTypeMacDataPoll:
        SendPoll();
        break;

    case Message::kTypeSupervision:
    {
        Child *child = netif.GetChildSupervisor().GetDestination(aMessage);
        VerifyOrExit(child != NULL, error = OT_ERROR_DROP);
        VerifyOrExit(!child->IsRxOnWhenIdle(), error = OT_ERROR_DROP);

        aMessage.SetChildMask(childTable.GetChildIndex(*child));
        mSourceMatchController.IncrementMessageCount(*child);
        break;
    }

    default:
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
                LogIp6Message(kMessageDrop, *cur, NULL, aError);
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

    VerifyOrExit(kNumIndirectSenders > 0);

    // Purge the transaction queue of there are any pending transactions
    for (int i = 0; i < kNumIndirectSenders; i++)
    {
        Child *destChild;

        destChild = mMeshSenders[i].mBoundChild;
        if (destChild != &aChild)
            continue;

        GetNetif().GetMac().PurgeFrameRequest(mMeshSenders[i].mSender);
    }

    VerifyOrExit(aChild.GetIndirectMessageCount() > 0);

    for (Message *message = mSendQueue.GetHead(); message; message = nextMessage)
    {
        nextMessage = message->GetNext();
        message->ClearChildMask(GetNetif().GetMle().GetChildTable().GetChildIndex(aChild));
        if (!message->IsChildPending() && !message->GetDirectTransmission())
        {
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
    for (ChildTable::Iterator iter(GetInstance(), ChildTable::kInStateAnyExceptValidOrRestoring); !iter.IsDone();
         iter++)
    {
        if (iter.GetChild()->GetIndirectMessageCount() == 0)
        {
            continue;
        }

        ClearChildIndirectMessages(*iter.GetChild());
    }
}

otError MeshForwarder::EvictMessage(uint8_t aPriority)
{
    otError  error = OT_ERROR_NOT_FOUND;
    Message *message;

    VerifyOrExit((message = mSendQueue.GetTail()) != NULL);

    if (message->GetPriority() < aPriority)
    {
        RemoveMessage(*message);
        ExitNow(error = OT_ERROR_NONE);
    }
    else
    {
        while (aPriority <= Message::kPriorityHigh)
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

otError MeshForwarder::RemoveMessageFromSleepyChild(Message &aMessage, Child &aChild)
{
    otError error      = OT_ERROR_NONE;
    uint8_t childIndex = GetNetif().GetMle().GetChildTable().GetChildIndex(aChild);

    VerifyOrExit(kNumIndirectSenders > 0, error = OT_ERROR_NOT_CAPABLE);
    VerifyOrExit(aMessage.GetChildMask(childIndex) == true, error = OT_ERROR_NOT_FOUND);

    aMessage.ClearChildMask(childIndex);
    mSourceMatchController.DecrementMessageCount(aChild);

    for (int i = 0; i < kNumIndirectSenders; i++)
    {
        if (mMeshSenders[i].mSendMessage == &aMessage && mMeshSenders[i].mBoundChild == &aChild)
        {
            mMeshSenders[i].mSendMessage = NULL;
            GetNetif().GetMac().PurgeFrameRequest(mMeshSenders[i].mSender);
            break;
        }
    }

exit:
    return error;
}

void MeshForwarder::RemoveMessages(Child &aChild, uint8_t aSubType)
{
    ThreadNetif &netif = GetNetif();
    Message *    nextMessage;

    for (Message *message = mSendQueue.GetHead(); message; message = nextMessage)
    {
        nextMessage = message->GetNext();

        if ((aSubType != Message::kSubTypeNone) && (aSubType != message->GetSubType()))
        {
            continue;
        }

        if (RemoveMessageFromSleepyChild(*message, aChild) != OT_ERROR_NONE)
        {
            switch (message->GetType())
            {
            case Message::kTypeIp6:
            {
                Ip6::Header ip6header;

                IgnoreReturnValue(message->Read(0, sizeof(ip6header), &ip6header));

                if (&aChild == static_cast<Child *>(netif.GetMle().GetNeighbor(ip6header.GetDestination())))
                {
                    message->ClearDirectTransmission();
                }

                break;
            }

            case Message::kType6lowpan:
            {
                Lowpan::MeshHeader meshHeader;

                IgnoreReturnValue(meshHeader.Init(*message));

                if (&aChild == static_cast<Child *>(netif.GetMle().GetNeighbor(meshHeader.GetDestination())))
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
            mSendQueue.Dequeue(*message);
            message->Free();
        }
    }
}

void MeshForwarder::RemoveDataResponseMessages(void)
{
    for (Message *message = mSendQueue.GetHead(); message; message = message->GetNext())
    {
        if (message->GetSubType() != Message::kSubTypeMleDataResponse)
        {
            continue;
        }

        RemoveMessage(*message);
    }
}

Message *MeshForwarder::GetIndirectTransmission(Child &aChild)
{
    Message *message = NULL;
    Message *next;
    uint8_t  childIndex = GetNetif().GetMle().GetChildTable().GetChildIndex(aChild);

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

    aChild.ResetIndirectTxAttempts();

    if (message != NULL)
    {
        Mac::Address macAddr;

        LogIp6Message(kMessagePrepareIndirect, *message, &aChild.GetMacAddress(macAddr), OT_ERROR_NONE);
    }

    return message;
}

void MeshSender::PrepareIndirectTransmission(const Child &aChild)
{
    switch (mSendMessage->GetType())
    {
    case Message::kTypeIp6:
    {
        Ip6::Header ip6Header;

        mSendMessage->Read(0, sizeof(ip6Header), &ip6Header);

        mAddMeshHeader = false;
        mParent->GetMacSourceAddress(ip6Header.GetSource(), mMacSource);

        aChild.GetMacAddress(mMacDest);

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

otError MeshSender::ScheduleIndirectTransmission()
{
    otError      error = OT_ERROR_NONE;
    Mac::Sender *macSender;

    VerifyOrExit(MeshForwarder::kNumIndirectSenders > 0, error = OT_ERROR_NOT_CAPABLE);
    VerifyOrExit(mBoundChild != NULL, error = OT_ERROR_NOT_FOUND);
    VerifyOrExit(mBoundChild->IsStateValidOrRestoring(), error = OT_ERROR_NOT_FOUND);

    if (mIdleMessageSent || !mSender.IsInUse())
    {
        if (mSendMessage == NULL)
        {
            Message *found = mParent->GetIndirectTransmission(*mBoundChild);

            if (found && mIdleMessageSent)
            {
                // Purge the idle message
                VerifyOrExit(mParent->GetNetif().GetMac().PurgeFrameRequest(mSender) == OT_ERROR_NONE);
            }
            mMessageNextOffset = 0;
            mSendMessage       = found;
        }

        mBoundChild->GetMacAddress(mMacDest);

        if (mSendMessage == NULL && mIdleMessageSent)
        {
            ExitNow(error = OT_ERROR_NOT_FOUND);
        }

        SuccessOrExit(error = mParent->GetNetif().GetMac().SendFrameRequest(mSender));
    }

    VerifyOrExit(mSendMessage != NULL);
    while ((macSender = mParent->GetIdleFloatingSender(this)) != NULL && mMessageNextOffset < mSendMessage->GetLength())
    {
        SuccessOrExit(error = mParent->GetNetif().GetMac().SendFrameRequest(*macSender));
    }

exit:
    return error;
}

otError MeshSender::SendMesh(Message &aMessage, otDataRequest &aDataReq)
{
    ThreadNetif &netif = mParent->GetNetif();

    memset(&aDataReq, 0, sizeof(aDataReq));

    aDataReq.mTxOptions = OT_MAC_TX_OPTION_ACK_REQ;
    Encoding::LittleEndian::WriteUint16(netif.GetMac().GetPanId(), aDataReq.mDst.mPanId);
    static_cast<Mac::FullAddr *>(&aDataReq.mDst)->SetAddress(mMacDest);
    aDataReq.mSrcAddrMode             = OT_MAC_ADDRESS_MODE_SHORT;
    aDataReq.mSecurity.mKeyIdMode     = 1;
    aDataReq.mSecurity.mSecurityLevel = Mac::Frame::kSecEncMic32;

    // write payload
    assert(aMessage.GetLength() <= GetMaxMsduSize(aDataReq));
    aMessage.Read(0, aMessage.GetLength(), aDataReq.mMsdu);
    aDataReq.mMsduLength = static_cast<uint8_t>(aMessage.GetLength());

    mMessageNextOffset = aMessage.GetLength();

    return OT_ERROR_NONE;
}

otError MeshForwarder::UpdateMeshRoute(Message &aMessage, MeshSender &aSender)
{
    ThreadNetif &      netif = GetNetif();
    otError            error = OT_ERROR_NONE;
    Lowpan::MeshHeader meshHeader;
    Neighbor *         neighbor;
    uint16_t           nextHop;

    IgnoreReturnValue(meshHeader.Init(aMessage));

    nextHop = netif.GetMle().GetNextHop(meshHeader.GetDestination());

    if (nextHop != Mac::kShortAddrInvalid)
    {
        neighbor = netif.GetMle().GetNeighbor(nextHop);
    }
    else
    {
        neighbor = netif.GetMle().GetNeighbor(meshHeader.GetDestination());
    }

    if (neighbor == NULL)
    {
        ExitNow(error = OT_ERROR_DROP);
    }

    aSender.mMacDest.SetShort(neighbor->GetRloc16());
    aSender.mMacSource.SetShort(netif.GetMac().GetShortAddress());

    aSender.mAddMeshHeader = true;
    aSender.mMeshDest      = meshHeader.GetDestination();
    aSender.mMeshSource    = meshHeader.GetSource();

exit:
    return error;
}

void MeshForwarder::UpdateRoutes(uint8_t *           aFrame,
                                 uint8_t             aFrameLength,
                                 const Mac::Address &aMeshSource,
                                 const Mac::Address &aMeshDest)
{
    ThreadNetif &netif = GetNetif();
    Ip6::Header  ip6Header;
    Neighbor *   neighbor;

    VerifyOrExit(!aMeshDest.IsBroadcast() && aMeshSource.IsShort());
    SuccessOrExit(GetIp6Header(aFrame, aFrameLength, aMeshSource, aMeshDest, ip6Header));

    netif.GetAddressResolver().UpdateCacheEntry(ip6Header.GetSource(), aMeshSource.GetShort());

    neighbor = netif.GetMle().GetNeighbor(ip6Header.GetSource());
    VerifyOrExit(neighbor != NULL && !neighbor->IsFullThreadDevice());

    if (Mle::Mle::GetRouterId(aMeshSource.GetShort()) != Mle::Mle::GetRouterId(GetNetif().GetMac().GetShortAddress()))
    {
        netif.GetMle().RemoveNeighbor(*neighbor);
    }

exit:
    return;
}

otError MeshForwarder::GetIp6Header(const uint8_t *     aFrame,
                                    uint8_t             aFrameLength,
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
                                         uint8_t             aFrameLength,
                                         const Mac::Address &aMeshSource,
                                         const Mac::Address &aMeshDest)
{
    ThreadNetif &netif = GetNetif();
    otError      error = OT_ERROR_NONE;
    Ip6::Header  ip6Header;

    SuccessOrExit(error = GetIp6Header(aFrame, aFrameLength, aMeshSource, aMeshDest, ip6Header));
    error = netif.GetMle().CheckReachability(aMeshSource.GetShort(), aMeshDest.GetShort(), ip6Header);

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
                               uint8_t                 aFrameLength,
                               const Mac::Address &    aMacSource,
                               const otThreadLinkInfo &aLinkInfo)
{
    ThreadNetif &      netif   = GetNetif();
    otError            error   = OT_ERROR_NONE;
    Message *          message = NULL;
    Mac::Address       meshDest;
    Mac::Address       meshSource;
    Lowpan::MeshHeader meshHeader;

    // Check the mesh header
    VerifyOrExit(meshHeader.Init(aFrame, aFrameLength) == OT_ERROR_NONE, error = OT_ERROR_DROP);

    // Security Check: only process Mesh Header frames that had security enabled.
    VerifyOrExit(aLinkInfo.mLinkSecurity && meshHeader.IsValid(), error = OT_ERROR_SECURITY);

    meshSource.SetShort(meshHeader.GetSource());
    meshDest.SetShort(meshHeader.GetDestination());

    UpdateRoutes(aFrame, aFrameLength, meshSource, meshDest);

    if (meshDest.GetShort() == netif.GetMac().GetShortAddress() || netif.GetMle().IsMinimalChild(meshDest.GetShort()))
    {
        aFrame += meshHeader.GetHeaderLength();
        aFrameLength -= meshHeader.GetHeaderLength();

        if (reinterpret_cast<Lowpan::FragmentHeader *>(aFrame)->IsFragmentHeader())
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
        netif.GetMle().ResolveRoutingLoops(aMacSource.GetShort(), meshDest.GetShort());

        SuccessOrExit(error = CheckReachability(aFrame, aFrameLength, meshSource, meshDest));

        meshHeader.SetHopsLeft(meshHeader.GetHopsLeft() - 1);
        meshHeader.AppendTo(aFrame);

        VerifyOrExit((message = GetInstance().GetMessagePool().New(Message::kType6lowpan, 0)) != NULL,
                     error = OT_ERROR_NO_BUFS);
        SuccessOrExit(error = message->SetLength(aFrameLength));
        message->Write(0, aFrameLength, aFrame);
        message->SetLinkSecurityEnabled(aLinkInfo.mLinkSecurity);
        message->SetPanId(aLinkInfo.mPanId);

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

#if OPENTHREAD_ENABLE_SERVICE
otError MeshForwarder::GetDestinationRlocByServiceAloc(uint16_t aServiceAloc, uint16_t &aMeshDest)
{
    otError                  error      = OT_ERROR_NONE;
    ThreadNetif &            netif      = GetNetif();
    uint8_t                  serviceId  = netif.GetMle().GetServiceIdFromAloc(aServiceAloc);
    NetworkData::ServiceTlv *serviceTlv = netif.GetNetworkDataLeader().FindServiceById(serviceId);

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
                curCost = netif.GetMle().GetCost(server->GetServer16());

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
            ExitNow(error = OT_ERROR_DROP);
        }
    }
    else
    {
        // Unknown service, can't forward
        ExitNow(error = OT_ERROR_DROP);
    }

exit:
    return error;
}
#endif // OPENTHREAD_ENABLE_SERVICE

} // namespace ot
#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_USE_EXTERNAL_MAC
