/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#include "frag_cache_retransmit.hpp"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MESH_FRAG_CACHE_RETRANSMIT_ENABLE

#include "instance/instance.hpp"
#include "thread/address_resolver.hpp"
#include "thread/mesh_forwarder.hpp"
#include "thread/mle.hpp"
#include "thread/tmf.hpp"

namespace ot {

RegisterLogModule("FragCache");

FragCacheRetransmit::FragCacheRetransmit(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mCacheTimer(aInstance, FragCacheRetransmit::HandleCacheTimer, this)
    , mRedirectTimer(aInstance, FragCacheRetransmit::HandleRedirectTimer, this)
{
    memset(mCacheEntries, 0, sizeof(mCacheEntries));
    memset(mRedirectEntries, 0, sizeof(mRedirectEntries));
}

bool FragCacheRetransmit::ExtractFlowId(const Message &aMessage, FlowId &aFlowId) const
{
    Ip6::Headers headers;

    SuccessOrExit(headers.ParseFrom(aMessage));
    VerifyOrExit(headers.IsUdp());

    aFlowId.mUdpChecksum = headers.GetChecksum();
    aFlowId.mDstAddr     = headers.GetDestinationAddress();
    aFlowId.mDstPort     = headers.GetDestinationPort();
    aFlowId.mSrcPort     = headers.GetSourcePort();
    aFlowId.mChildRloc16 = 0; // Set by caller

    return true;

exit:
    return false;
}

// --- CachingTable (at parent/failing router) ---

FragCacheRetransmit::CacheEntry *FragCacheRetransmit::FindCacheEntry(const FlowId &aFlowId)
{
    for (CacheEntry &entry : mCacheEntries)
    {
        if (entry.mValid &&
            entry.mFlowId.mUdpChecksum == aFlowId.mUdpChecksum &&
            entry.mFlowId.mDstAddr == aFlowId.mDstAddr &&
            entry.mFlowId.mDstPort == aFlowId.mDstPort &&
            entry.mFlowId.mSrcPort == aFlowId.mSrcPort &&
            (aFlowId.mChildRloc16 == 0 || entry.mFlowId.mChildRloc16 == aFlowId.mChildRloc16))
        {
            return &entry;
        }
    }

    return nullptr;
}

FragCacheRetransmit::CacheEntry *FragCacheRetransmit::AllocateCacheEntry(void)
{
    CacheEntry *oldest = nullptr;

    for (CacheEntry &entry : mCacheEntries)
    {
        if (!entry.mValid)
        {
            return &entry;
        }

        if (oldest == nullptr || entry.mExpireTime < oldest->mExpireTime)
        {
            oldest = &entry;
        }
    }

    // Evict oldest entry
    if (oldest != nullptr && oldest->mMessage != nullptr)
    {
        oldest->mMessage->Free();
        oldest->mMessage = nullptr;
        oldest->mValid   = false;
    }

    return oldest;
}

void FragCacheRetransmit::HandleIndirectTxFailure(Message &aMessage, Child &aChild)
{
    FlowId     flowId;
    CacheEntry *entry;

    VerifyOrExit(ExtractFlowId(aMessage, flowId));

    flowId.mChildRloc16 = aChild.GetRloc16();

    // Don't cache duplicates
    VerifyOrExit(FindCacheEntry(flowId) == nullptr);

    entry = AllocateCacheEntry();
    VerifyOrExit(entry != nullptr);

    entry->mMessage = aMessage.Clone();
    VerifyOrExit(entry->mMessage != nullptr);

    entry->mValid             = true;
    entry->mFlowId            = flowId;
    entry->mExpireTime        = TimerMilli::GetNow() + kCacheTimeoutMs;

    // Resolve mesh source RLOC16 from the IPv6 source address via address cache
    {
        Ip6::Headers headers;
        uint16_t     srcRloc16 = Mle::kInvalidRloc16;

        if (headers.ParseFrom(aMessage) == kErrorNone)
        {
            const Ip6::Address &srcAddr = headers.GetSourceAddress();

            if (srcAddr.GetIid().IsLocator())
            {
                srcRloc16 = srcAddr.GetIid().GetLocator();
            }
            else
            {
                srcRloc16 = Get<AddressResolver>().LookUp(srcAddr);
            }
        }

        VerifyOrExit(srcRloc16 != Mle::kInvalidRloc16);
        entry->mMeshSourceRloc16 = srcRloc16;
    }

    mCacheTimer.FireAtIfEarlier(entry->mExpireTime);

    LogNote("Cached msg for child 0x%04x, checksum 0x%04x, notifying source 0x%04x",
            flowId.mChildRloc16, flowId.mUdpChecksum, entry->mMeshSourceRloc16);

    SendNotification(flowId, entry->mMeshSourceRloc16);

exit:
    return;
}

void FragCacheRetransmit::SendNotification(const FlowId &aFlowId, uint16_t aMeshSourceRloc16)
{
    Error              error;
    Coap::Message     *message;

    message = Get<Tmf::Agent>().AllocateAndInitNonConfirmablePostMessage(kUriFragCacheNotify);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->Append(aFlowId));

    {
        Ip6::Address dest;
        dest.SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), aMeshSourceRloc16);
        SuccessOrExit(error = Get<Tmf::Agent>().SendMessageTo(*message, dest));
    }
    message = nullptr;

    LogNote("Sent FragCacheNotify to 0x%04x", aMeshSourceRloc16);

exit:
    FreeMessage(message);
}

void FragCacheRetransmit::HandleNotify(Coap::Msg &aMsg)
{
    FlowId         flowId;
    RedirectEntry *entry;

    SuccessOrExit(aMsg.mMessage.Read(aMsg.mMessage.GetOffset(), flowId));

    entry = AllocateRedirectEntry();
    VerifyOrExit(entry != nullptr);

    entry->mValid               = true;
    entry->mUdpChecksum         = flowId.mUdpChecksum;
    entry->mDstAddr             = flowId.mDstAddr;
    entry->mDstPort             = flowId.mDstPort;
    entry->mSrcPort             = flowId.mSrcPort;
    entry->mCachingRouterRloc16 = aMsg.mMessageInfo.GetPeerAddr().GetIid().GetLocator();
    entry->mExpireTime          = TimerMilli::GetNow() + kCacheTimeoutMs;

    mRedirectTimer.FireAtIfEarlier(entry->mExpireTime);

    LogNote("Received FragCacheNotify from 0x%04x, checksum 0x%04x",
            entry->mCachingRouterRloc16, flowId.mUdpChecksum);

exit:
    return;
}

// --- RedirectTable (at source router) ---

FragCacheRetransmit::RedirectEntry *FragCacheRetransmit::FindRedirectEntry(uint16_t      aChecksum,
                                                                          const Ip6::Address &aDstAddr,
                                                                          uint16_t      aDstPort,
                                                                          uint16_t      aSrcPort)
{
    for (RedirectEntry &entry : mRedirectEntries)
    {
        if (entry.mValid &&
            entry.mUdpChecksum == aChecksum &&
            entry.mDstAddr == aDstAddr &&
            entry.mDstPort == aDstPort &&
            entry.mSrcPort == aSrcPort)
        {
            return &entry;
        }
    }

    return nullptr;
}

FragCacheRetransmit::RedirectEntry *FragCacheRetransmit::AllocateRedirectEntry(void)
{
    for (RedirectEntry &entry : mRedirectEntries)
    {
        if (!entry.mValid)
        {
            return &entry;
        }
    }

    // Evict oldest
    RedirectEntry *oldest = &mRedirectEntries[0];

    for (RedirectEntry &entry : mRedirectEntries)
    {
        if (entry.mExpireTime < oldest->mExpireTime)
        {
            oldest = &entry;
        }
    }

    oldest->mValid = false;
    return oldest;
}

bool FragCacheRetransmit::ShouldRedirect(const Message &aMessage, uint16_t &aCachingRouterRloc)
{
    Ip6::Headers   headers;
    RedirectEntry *entry;

    SuccessOrExit(headers.ParseFrom(aMessage));
    VerifyOrExit(headers.IsUdp());

    entry = FindRedirectEntry(headers.GetChecksum(), headers.GetDestinationAddress(),
                              headers.GetDestinationPort(), headers.GetSourcePort());
    VerifyOrExit(entry != nullptr);

    aCachingRouterRloc = entry->mCachingRouterRloc16;

    LogNote("Redirect match: checksum 0x%04x → caching router 0x%04x",
            headers.GetChecksum(), aCachingRouterRloc);

    // Remove the entry (one-shot redirect)
    entry->mValid = false;

    return true;

exit:
    return false;
}

void FragCacheRetransmit::SendRedirect(uint16_t aCachingRouterRloc, const Message &aMessage)
{
    Error          error;
    Coap::Message *message = nullptr;
    FlowId         flowId;

    VerifyOrExit(ExtractFlowId(aMessage, flowId));

    message = Get<Tmf::Agent>().AllocateAndInitNonConfirmablePostMessage(kUriFragCacheResend);
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = message->Append(flowId));

    {
        Ip6::Address dest;
        dest.SetToRoutingLocator(Get<Mle::Mle>().GetMeshLocalPrefix(), aCachingRouterRloc);
        SuccessOrExit(error = Get<Tmf::Agent>().SendMessageTo(*message, dest));
    }
    message = nullptr;

    LogNote("Sent FragCacheResend to 0x%04x", aCachingRouterRloc);

exit:
    FreeMessage(message);
}

void FragCacheRetransmit::HandleResend(Coap::Msg &aMsg)
{
    FlowId      flowId;
    CacheEntry *entry;

    SuccessOrExit(aMsg.mMessage.Read(aMsg.mMessage.GetOffset(), flowId));

    entry = FindCacheEntry(flowId);
    VerifyOrExit(entry != nullptr);

    LogNote("Received FragCacheResend, retransmitting cached msg for child 0x%04x", flowId.mChildRloc16);

    RetransmitCachedMessage(*entry);

exit:
    return;
}

void FragCacheRetransmit::RetransmitCachedMessage(CacheEntry &aEntry)
{
    Message *clone;
    Child   *child;

    VerifyOrExit(aEntry.mMessage != nullptr);

    child = Get<ChildTable>().FindChild(aEntry.mFlowId.mChildRloc16, Child::kInStateAnyExceptInvalid);
    VerifyOrExit(child != nullptr);

    clone = aEntry.mMessage->Clone();
    VerifyOrExit(clone != nullptr);

    // Reset for fresh transmission with new datagram tag
    clone->SetOffset(0);
    clone->SetDatagramTag(0);

    Get<MeshForwarder>().SendMessage(OwnedPtr<Message>(clone));

    LogNote("Retransmitted cached msg to child 0x%04x", aEntry.mFlowId.mChildRloc16);

    // Remove cache entry after retransmission
    aEntry.mMessage->Free();
    aEntry.mMessage = nullptr;
    aEntry.mValid   = false;

exit:
    return;
}

// --- Timer handlers ---

void FragCacheRetransmit::HandleCacheTimer(Timer &aTimer)
{
    static_cast<FragCacheRetransmit *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleCacheTimer();
}

void FragCacheRetransmit::HandleCacheTimer(void)
{
    NextFireTime nextTime;

    for (CacheEntry &entry : mCacheEntries)
    {
        if (!entry.mValid)
        {
            continue;
        }

        if (nextTime.GetNow() >= entry.mExpireTime)
        {
            LogNote("Cache entry expired for child 0x%04x, checksum 0x%04x",
                    entry.mFlowId.mChildRloc16, entry.mFlowId.mUdpChecksum);

            if (entry.mMessage != nullptr)
            {
                entry.mMessage->Free();
                entry.mMessage = nullptr;
            }

            entry.mValid = false;
        }
        else
        {
            nextTime.UpdateIfEarlier(entry.mExpireTime);
        }
    }

    mCacheTimer.FireAt(nextTime);
}

void FragCacheRetransmit::HandleRedirectTimer(Timer &aTimer)
{
    static_cast<FragCacheRetransmit *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleRedirectTimer();
}

void FragCacheRetransmit::HandleRedirectTimer(void)
{
    NextFireTime nextTime;

    for (RedirectEntry &entry : mRedirectEntries)
    {
        if (!entry.mValid)
        {
            continue;
        }

        if (nextTime.GetNow() >= entry.mExpireTime)
        {
            entry.mValid = false;
        }
        else
        {
            nextTime.UpdateIfEarlier(entry.mExpireTime);
        }
    }

    mRedirectTimer.FireAt(nextTime);
}

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_MESH_FRAG_CACHE_RETRANSMIT_ENABLE
