/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes implementation of Thread Direct Peer and Peer List.
 */

#include "td_peer.hpp"

#if OT_CONFIG_THREAD_DIRECT_ENABLE

#include "common/code_utils.hpp"
#include "common/numeric_limits.hpp"
#include "instance/instance.hpp"

namespace ot {
namespace ThreadDirect {

//---------------------------------------------------------------------------------------------------------------------
// Peer

void Peer::Init(Instance &aInstance, const Mac::ExtAddress &aExtAddress, uint32_t aFrameCounter)
{
    InstanceLocatorInit::Init(aInstance);
    mExtAddress   = aExtAddress;
    mFrameCounter = aFrameCounter;
    mLastRecvTime = Get<UptimeTracker>().GetUptimeInSeconds();
}

Error Peer::VerifyAndUpdateFrameCounter(uint32_t aFrameCounter)
{
    Error error = kErrorNone;

    mLastRecvTime = Get<UptimeTracker>().GetUptimeInSeconds();

    VerifyOrExit(aFrameCounter != NumericLimits<uint32_t>::kMax, error = kErrorSecurity);
    VerifyOrExit(aFrameCounter > mFrameCounter, error = kErrorDuplicated);

    mFrameCounter = aFrameCounter;

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// PeerList

void PeerList::Init(Instance &aInstance)
{
    InstanceLocatorInit::Init(aInstance);
    mPeers.Clear();
}

Peer *PeerList::AddNew(const Mac::ExtAddress &aExtAddress, uint32_t aFrameCounter)
{
    Peer *peer;

#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE
    peer = Peer::Allocate();
    VerifyOrExit(peer != nullptr);

    mPeers.Push(*peer);
    Get<KeyManager>().StartPeerTimer();
#else
    peer = mPeers.PushBack();

    if (peer == nullptr)
    {
        for (Peer &entry : mPeers)
        {
            // (For future) Ensure the peer entry is evictable (i.e., not
            // associated with an active Thread Direct Link) before
            // selecting it for eviction.

            if ((peer == nullptr) || (entry.mLastRecvTime < peer->mLastRecvTime))
            {
                peer = &entry;
            }
        }
    }

    VerifyOrExit(peer != nullptr);
#endif

    peer->Init(GetInstance(), aExtAddress, aFrameCounter);

exit:
    return peer;
}

#if OPENTHREAD_CONFIG_TD_PEER_LIST_USE_HEAP_ENABLE

void PeerList::RemoveExpired(UptimeSec aUptimeNow)
{
    Peer::ExpireChecker checker;

    checker.mUptimeNow = aUptimeNow;
    mPeers.RemoveAndFreeAllMatching(checker);
}

#endif

} // namespace ThreadDirect
} // namespace ot

#endif // OT_CONFIG_THREAD_DIRECT_ENABLE
