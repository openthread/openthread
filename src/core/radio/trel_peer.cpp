/*
 *    Copyright (c) 2025, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements Thread Radio Encapsulation Link (TREL) peer.
 */

#include "trel_peer.hpp"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Trel {

RegisterLogModule("TrelPeerTable");

//---------------------------------------------------------------------------------------------------------------------
// Peer

void Peer::Init(Instance &aInstance)
{
    InstanceLocatorInit::Init(aInstance);

    AsCoreType(&mExtAddress).Clear();
    AsCoreType(&mExtPanId).Clear();
    AsCoreType(&mSockAddr).Clear();

    mState = kStateValid;
}

void Peer::Free(void)
{
    Log(kDeleted);

#if OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
    Heap::Allocatable<Peer>::Free();
#else
    this->~Peer();
    Get<PeerTable>().mPool.Free(*this);
#endif
}

void Peer::ScheduleToRemoveAfter(uint32_t aDelay)
{
    VerifyOrExit(IsStateValid());

    mRemoveTime = TimerMilli::GetNow() + aDelay;
    SetState(kStateRemoving);

    Get<PeerTable>().mTimer.FireAtIfEarlier(mRemoveTime);

    Log(kRemoving);
    LogInfo("   after %u msec", aDelay);

exit:
    return;
}

bool Peer::Matches(const NonNeighborMatcher &aMatcher) const
{
    // Matches only if the peer is not a neighbor. This is used when
    // evicting a peer to make room for a new one, where we search
    // for and remove a non-neighbor from the list.

    bool matches = false;

    VerifyOrExit(aMatcher.mNeighborTable.FindNeighbor(GetExtAddress(), Neighbor::kInStateAny) == nullptr);
#if OPENTHREAD_FTD
    VerifyOrExit(aMatcher.mNeighborTable.FindRxOnlyNeighborRouter(GetExtAddress()) == nullptr);
#endif

    matches = true;

exit:
    return matches;
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void Peer::Log(Action aAction) const
{
    LogInfo("%s peer mac:%s, xpan:%s, %s", ActionToString(aAction), GetExtAddress().ToString().AsCString(),
            GetExtPanId().ToString().AsCString(), GetSockAddr().ToString().AsCString());
}

const char *Peer::ActionToString(Action aAction)
{
    static const char *const kActionStrings[] = {
        "Added",    // (0) kAdded
        "Re-added", // (1) kReAdded,
        "Updated",  // (2) kUpdated
        "Removing", // (3) kRemoving
        "Deleted",  // (4) kDeleted
        "Evicting", // (5) kEvicting
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kAdded);
        ValidateNextEnum(kReAdded);
        ValidateNextEnum(kUpdated);
        ValidateNextEnum(kRemoving);
        ValidateNextEnum(kDeleted);
        ValidateNextEnum(kEvicting);
    };

    return kActionStrings[aAction];
}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

//---------------------------------------------------------------------------------------------------------------------
// PeerTable

PeerTable::PeerTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
{
}

Peer *PeerTable::AllocatePeer(void)
{
    Peer *newPeer;

#if OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
    newPeer = Peer::Allocate();
#else
    newPeer = mPool.Allocate();
#endif

    return newPeer;
}

Peer *PeerTable::AllocateAndAddNewPeer(void)
{
    Peer *newPeer = nullptr;

    do
    {
        newPeer = AllocatePeer();

        if (newPeer != nullptr)
        {
            break;
        }
    } while (EvictPeer() == kErrorNone);

    VerifyOrExit(newPeer != nullptr);

    newPeer->Init(GetInstance());
    Push(*newPeer);

exit:
    return newPeer;
}

Error PeerTable::EvictPeer(void)
{
    Error          error = kErrorNotFound;
    OwnedPtr<Peer> peerToEvict;

    // We first try to evict a peer already scheduled to be removed.
    // Then try to evict a peer belonging to a different PAN. If not
    // found, we evict a non-neighbor peer.

    peerToEvict = RemoveMatching(Peer::kStateRemoving);

    if (peerToEvict == nullptr)
    {
        peerToEvict = RemoveMatching(Peer::OtherExtPanIdMatcher(Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId()));
    }

    if (peerToEvict == nullptr)
    {
        peerToEvict = RemoveMatching(Peer::NonNeighborMatcher(Get<NeighborTable>()));
    }

    VerifyOrExit(peerToEvict != nullptr);

    peerToEvict->Log(Peer::kEvicting);
    error = kErrorNone;

exit:
    return error;
}

void PeerTable::HandleTimer(void)
{
    TimeMilli    now = TimerMilli::GetNow();
    NextFireTime nextFireTime(now);

    RemoveAndFreeAllMatching(Peer::ExpireChecker(now));

    for (const Peer &peer : *this)
    {
        if (peer.IsStateRemoving())
        {
            nextFireTime.UpdateIfEarlier(peer.mRemoveTime);
        }
    }

    mTimer.FireAtIfEarlier(nextFireTime);
}

const Peer *PeerTable::GetNextPeer(PeerIterator &aIterator) const
{
    const Peer *entry = static_cast<const Peer *>(aIterator);

    VerifyOrExit(entry != nullptr);

    while (!entry->IsStateValid())
    {
        entry = entry->GetNext();
        VerifyOrExit(entry != nullptr);
    }

    aIterator = entry->GetNext();

exit:
    return entry;
}

uint16_t PeerTable::GetNumberOfPeers(void) const
{
    uint16_t count = 0;

    for (const Peer &peer : *this)
    {
        if (peer.IsStateValid())
        {
            count++;
        }
    }

    return count;
}

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
