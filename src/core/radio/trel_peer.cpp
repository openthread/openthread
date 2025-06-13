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

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    mPort                     = 0;
    mExtAddressSet            = false;
    mResolvingService         = false;
    mResolvingHost            = false;
    mTxtDataValidated         = false;
    mSockAddrUpdatedBasedOnRx = false;
    mState                    = kStateResolving;
#else
    mState = kStateValid;
#endif
}

void Peer::Free(void)
{
    SignalPeerRemoval();

    Log(kDeleted);

#if OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
    Heap::Allocatable<Peer>::Free();
#else
    this->~Peer();
    Get<PeerTable>().mPool.Free(*this);
#endif
}

void Peer::UpdateSockAddrBasedOnRx(const Ip6::SockAddr &aSockAddr)
{
    VerifyOrExit(GetSockAddr() != aSockAddr);

    SetSockAddr(aSockAddr);
#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    mSockAddrUpdatedBasedOnRx = true;
#endif
    Log(kUpdated);

exit:
    return;
}

void Peer::ScheduleToRemoveAfter(uint32_t aDelay)
{
    VerifyOrExit(!IsStateRemoving());

    mRemoveTime = TimerMilli::GetNow() + aDelay;

    Log(kRemoving);
    LogInfo("    after %lu msec", ToUlong(aDelay));

    SetState(kStateRemoving);

    Get<PeerTable>().mTimer.FireAtIfEarlier(mRemoveTime);

    SignalPeerRemoval();

exit:
    return;
}

void Peer::SetExtAddress(const Mac::ExtAddress &aExtAddress)
{
    mExtAddress = aExtAddress;
#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    mExtAddressSet = true;
#endif
}

bool Peer::Matches(const Mac::ExtAddress &aExtAddress) const
{
    bool matches = false;

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    VerifyOrExit(mExtAddressSet);
#endif

    VerifyOrExit(GetExtAddress() == aExtAddress);
    matches = true;

exit:
    return matches;
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

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

void Peer::SignalPeerRemoval(void)
{
    // Signal to `PeerDiscoverer` that peer is removed and/or about to be
    // freed. The `PeerDiscoverer` will then stop any active resolvers
    // associated with this peer.

    Get<PeerDiscoverer>().HandlePeerRemoval(*this);
}

void Peer::SetPort(uint16_t aPort)
{
    VerifyOrExit(mPort != aPort);

    mPort = aPort;

    if (mPort != 0)
    {
        AsCoreType(&mSockAddr).SetPort(mPort);
    }

exit:
    return;
}

bool Peer::Matches(const ServiceNameMatcher &aMatcher) const { return NameMatch(mServiceName, aMatcher.mServiceName); }

bool Peer::Matches(const HostNameMatcher &aMatcher) const
{
    return mResolvingHost && NameMatch(mHostName, aMatcher.mHostName);
}

bool Peer::NameMatch(const Heap::String &aHeapString, const char *aName)
{
    // Compares a DNS name given as a `Heap::String` with a
    // `aName` C string.
    return !aHeapString.IsNull() && StringMatch(aHeapString.AsCString(), aName, kStringCaseInsensitiveMatch);
}

#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

const char *Peer::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "valid",     // (0) kStateValid
        "removing",  // (1) kStateRemoving
        "resolving", // (2) kStateResolving
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kStateValid);
        ValidateNextEnum(kStateRemoving);
        ValidateNextEnum(kStateResolving);
    };

    return kStateStrings[aState];
}

void Peer::Log(Action aAction) const
{
    LogInfo("%s peer %s, state:%s", ActionToString(aAction),
            mServiceName.IsNull() ? "(null)" : mServiceName.AsCString(), StateToString(mState));

    if (!mHostName.IsNull())
    {
        LogInfo("    host:%s, port:%u", mHostName.AsCString(), mPort);

        if (mHostAddresses.GetLength() > 0)
        {
            LogInfo("    num-addrs:%u, %s", mHostAddresses.GetLength(), mHostAddresses[0].ToString().AsCString());
        }
    }

    if (mExtAddressSet)
    {
        LogInfo("    ext-addr:%s, ext-panid:%s", GetExtAddress().ToString().AsCString(),
                GetExtPanId().ToString().AsCString());
    }

    if (!GetSockAddr().GetAddress().IsUnspecified())
    {
        LogInfo("    sock-addr:%s, based-on-rx:%s", GetSockAddr().ToString().AsCString(),
                ToYesNo(mSockAddrUpdatedBasedOnRx));
    }
}

#else

void Peer::Log(Action aAction) const
{
    LogInfo("%s peer mac:%s, xpan:%s, %s", ActionToString(aAction), GetExtAddress().ToString().AsCString(),
            GetExtPanId().ToString().AsCString(), GetSockAddr().ToString().AsCString());
}

#endif

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

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Peer::AddressArray

void Peer::AddressArray::CloneFrom(const AddressArray &aOtherArray)
{
    Free();

    VerifyOrExit(!aOtherArray.IsEmpty());
    SuccessOrAssert(ReserveCapacity(aOtherArray.GetLength()));

    for (const Ip6::Address &addr : aOtherArray)
    {
        SuccessOrAssert(PushBack(addr));
    }

exit:
    return;
}

#endif

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
