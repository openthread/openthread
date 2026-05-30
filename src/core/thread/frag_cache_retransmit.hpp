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

#ifndef FRAG_CACHE_RETRANSMIT_HPP_
#define FRAG_CACHE_RETRANSMIT_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MESH_FRAG_CACHE_RETRANSMIT_ENABLE

#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "coap/coap.hpp"
#include "net/ip6.hpp"
#include "thread/child.hpp"

namespace ot {

class FragCacheRetransmit : public InstanceLocator, private NonCopyable
{
public:
    explicit FragCacheRetransmit(Instance &aInstance);

    void HandleIndirectTxFailure(Message &aMessage, Child &aChild);

    bool ShouldRedirect(const Message &aMessage, uint16_t &aCachingRouterRloc);

    void SendRedirect(uint16_t aCachingRouterRloc, const Message &aMessage);

    void HandleNotify(Coap::Msg &aMsg);

    void HandleResend(Coap::Msg &aMsg);

private:
    static constexpr uint16_t kMaxCacheEntries    = OPENTHREAD_CONFIG_MESH_FRAG_CACHE_MAX_ENTRIES;
    static constexpr uint32_t kCacheTimeoutMs     = OPENTHREAD_CONFIG_MESH_FRAG_CACHE_TIMEOUT * 1000;
    static constexpr uint16_t kMaxRedirectEntries = 8;

    OT_TOOL_PACKED_BEGIN
    struct FlowId
    {
        uint16_t       mUdpChecksum;
        Ip6::Address   mDstAddr;
        uint16_t       mDstPort;
        uint16_t       mSrcPort;
        uint16_t       mChildRloc16;
    } OT_TOOL_PACKED_END;

    struct CacheEntry
    {
        bool           mValid;
        FlowId         mFlowId;
        uint16_t       mMeshSourceRloc16;
        Message       *mMessage;
        TimeMilli      mExpireTime;
    };

    struct RedirectEntry
    {
        bool           mValid;
        uint16_t       mUdpChecksum;
        Ip6::Address   mDstAddr;
        uint16_t       mDstPort;
        uint16_t       mSrcPort;
        uint16_t       mCachingRouterRloc16;
        TimeMilli      mExpireTime;
    };

    bool ExtractFlowId(const Message &aMessage, FlowId &aFlowId) const;
    void SendNotification(const FlowId &aFlowId, uint16_t aMeshSourceRloc16);
    void RetransmitCachedMessage(CacheEntry &aEntry);

    CacheEntry    *FindCacheEntry(const FlowId &aFlowId);
    CacheEntry    *AllocateCacheEntry(void);
    RedirectEntry *FindRedirectEntry(uint16_t aChecksum, const Ip6::Address &aDstAddr, uint16_t aDstPort, uint16_t aSrcPort);
    RedirectEntry *AllocateRedirectEntry(void);

    static void HandleCacheTimer(Timer &aTimer);
    void        HandleCacheTimer(void);
    static void HandleRedirectTimer(Timer &aTimer);
    void        HandleRedirectTimer(void);

    CacheEntry        mCacheEntries[kMaxCacheEntries];
    RedirectEntry     mRedirectEntries[kMaxRedirectEntries];
    TimerMilliContext  mCacheTimer;
    TimerMilliContext  mRedirectTimer;
};

} // namespace ot

#endif // OPENTHREAD_FTD && OPENTHREAD_CONFIG_MESH_FRAG_CACHE_RETRANSMIT_ENABLE

#endif // FRAG_CACHE_RETRANSMIT_HPP_
