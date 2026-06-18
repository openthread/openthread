/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements the Thread Direct peer table.
 */

#include "direct_peer_table.hpp"

#if OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE

#include "instance/instance.hpp"

namespace ot {

DirectPeerTable::Iterator::Iterator(Instance &aInstance, DirectPeer::StateFilter aFilter)
    : InstanceLocator(aInstance)
    , ItemPtrIterator(nullptr)
    , mFilter(aFilter)
{
    Reset();
}

void DirectPeerTable::Iterator::Reset(void)
{
    mItem = &Get<DirectPeerTable>().mPeers[0];

    if (!mItem->Matches(mFilter))
    {
        Advance();
    }
}

void DirectPeerTable::Iterator::Advance(void)
{
    VerifyOrExit(mItem != nullptr);

    do
    {
        mItem++;
        VerifyOrExit(mItem < &Get<DirectPeerTable>().mPeers[Get<DirectPeerTable>().kMaxPeers], mItem = nullptr);
    } while (!mItem->Matches(mFilter));

exit:
    return;
}

DirectPeerTable::DirectPeerTable(Instance &aInstance)
    : InstanceLocator(aInstance)
{
    for (DirectPeer &peer : mPeers)
    {
        peer.Init(aInstance);
        peer.Clear();
    }
}

void DirectPeerTable::Clear(void)
{
    for (DirectPeer &peer : mPeers)
    {
        peer.Clear();
    }
}

DirectPeer *DirectPeerTable::GetNewPeer(void)
{
    DirectPeer *peer = FindPeer(DirectPeer::AddressMatcher(DirectPeer::kInStateInvalid));

    VerifyOrExit(peer != nullptr);
    peer->Clear();

exit:
    return peer;
}

const DirectPeer *DirectPeerTable::FindPeer(const DirectPeer::AddressMatcher &aMatcher) const
{
    const DirectPeer *peer = mPeers;

    for (uint16_t num = kMaxPeers; num != 0; num--, peer++)
    {
        if (peer->Matches(aMatcher))
        {
            ExitNow();
        }
    }

    peer = nullptr;

exit:
    return peer;
}

DirectPeer *DirectPeerTable::FindPeer(const Mac::ExtAddress &aExtAddress, DirectPeer::StateFilter aFilter)
{
    return FindPeer(DirectPeer::AddressMatcher(aExtAddress, aFilter));
}

bool DirectPeerTable::HasPeers(DirectPeer::StateFilter aFilter) const
{
    return (FindPeer(DirectPeer::AddressMatcher(aFilter)) != nullptr);
}

bool DirectPeerTable::IsFull(void) const
{
    return FindPeer(DirectPeer::AddressMatcher(DirectPeer::kInStateInvalid)) == nullptr;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
