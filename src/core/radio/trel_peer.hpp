/*
 *  Copyright (c) 2019-2025, The OpenThread Authors.
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
 *   This file includes definitions for Thread Radio Encapsulation Link (TREL) peer.
 */

#ifndef TREL_PEER_HPP_
#define TREL_PEER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#include <openthread/trel.h>
#include <openthread/platform/trel.h>

#include "common/as_core_type.hpp"
#include "common/heap_allocatable.hpp"
#include "common/heap_array.hpp"
#include "common/heap_data.hpp"
#include "common/heap_string.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/non_copyable.hpp"
#include "common/owning_list.hpp"
#include "common/pool.hpp"
#include "common/timer.hpp"
#include "mac/mac_types.hpp"
#include "meshcop/extended_panid.hpp"
#include "net/socket.hpp"
#include "thread/mle_types.hpp"

namespace ot {

class NeighborTable;

namespace Trel {

class PeerTable;
class PeerDiscoverer;

/**
 * Represents a discovered TREL peer.
 */
class Peer : public InstanceLocatorInit,
             public otTrelPeer,
             public LinkedListEntry<Peer>,
#if OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
             public Heap::Allocatable<Peer>,
#endif
             private NonCopyable

{
    friend class PeerTable;
    friend class PeerDiscoverer;
    friend class OwnedPtr<Peer>;
    friend class OwningList<Peer>;
    friend class LinkedList<Peer>;
    friend class LinkedListEntry<Peer>;

public:
    /**
     * Represents the DNS-SD (mDNS) service discovery state of the peer.
     */
    enum DnssdState : uint8_t
    {
        kDnssdResolved,  ///< Peer is resolved on DNS-SD (mDNS), i.e., service and host resolution are done.
        kDnssdRemoved,   ///< DNS-SD (mDNS) has notified that the peer service registration is removed or timed out.
        kDnssdResolving, ///< Ongoing service/host resolution for the peer service or host name.
    };

    /**
     * Gets the current DNS-SD service discovery state of the peer.
     *
     * @returns The DNS-SD state.
     */
    DnssdState GetDnssdState(void) const { return mDnssdState; }

    /**
     * Returns the Extended MAC Address of the discovered TREL peer.
     *
     * @returns The Extended MAC Address of the TREL peer.
     */
    const Mac::ExtAddress &GetExtAddress(void) const { return AsCoreType(&mExtAddress); }

    /**
     * Returns the Extended PAN Identifier of the discovered TREL peer.
     *
     * @returns The Extended PAN Identifier of the TREL peer.
     */
    const MeshCoP::ExtendedPanId &GetExtPanId(void) const { return AsCoreType(&mExtPanId); }

    /**
     * Returns the IPv6 socket address of the discovered TREL peer.
     *
     * @returns The IPv6 socket address of the TREL peer.
     */
    const Ip6::SockAddr &GetSockAddr(void) const { return AsCoreType(&mSockAddr); }

    /**
     * Indicates whether or not the IPv6 socket address associated with the TREL peer is valid.
     *
     * During peer discovery (mDNS service and host resolution), the peer address may not yet be known and can be
     * invalid (i.e., set to the unspecified IPv6 address `::`).
     *
     * @retval TRUE   If the peer socket address is valid.
     * @retval FALSE  If the peer socket address is not valid.
     */
    bool HasValidSockAddr(void) const { return !GetSockAddr().GetAddress().IsUnspecified(); }

    /**
     * Updates the IPv6 socket address of the discovered TREL peer based on a received message from peer.
     *
     * @param[in] aSockAddr   The IPv6 socket address discovered based on received message from the peer
     */
    void UpdateSockAddrBasedOnRx(const Ip6::SockAddr &aSockAddr);

    /**
     * Updates the last interaction time (rx or tx) of the TREL peer to now.
     *
     * This is called after sending (unicast/ack TREL packet excluding a broadcast tx) or receiving from the peer.
     */
    void UpdateLastInteractionTime(void);

    /**
     * Determine number of seconds since last interaction with the TREL peer.
     *
     * @returns Duration (in seconds) since last interaction with the peer.
     */
    uint32_t DetermineSecondsSinceLastInteraction(void) const;

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

    /**
     * Returns the TREL peer service name (service instance label).
     *
     * @returns The peer service name, or `nullptr` if not yet known.
     */
    const char *GetServiceName(void) const { return mServiceName.AsCString(); }

    /**
     * Returns the host name of the discovered TREL peer.
     *
     * @returns The host name as a null-terminated C string, or `nullptr` if not yet known.
     */
    const char *GetHostName(void) const { return mHostName.AsCString(); }

    /**
     * Returns the list of discovered IPv6 host addresses for the TREL peer.
     *
     * The array is sorted to place preferred addresses at the top of the list.
     *
     * @returns An array of IPv6 addresses.
     */
    const Heap::Array<Ip6::Address> &GetHostAddresses(void) const { return mHostAddresses; }

#endif // OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE

private:
    static constexpr uint32_t kExpirationDelay = 450; // (in second) - 7.5 minutes

    enum Action : uint8_t
    {
        kAdded,    // Added a new peer.
        kReAdded,  // Re-added a peer (discovered again) after DNSSD removal.
        kUpdated,  // Updated an existing peer.
        kDeleted,  // Fully removing and deleting the peer from the table.
        kEvicting, // Evicting the peer to make space for new one.
    };

    struct OtherExtPanIdMatcher // Matches if Ext PAN ID is different.
    {
        explicit OtherExtPanIdMatcher(const MeshCoP::ExtendedPanId &aExtPanId)
            : mExtPanId(aExtPanId)
        {
        }

        const MeshCoP::ExtendedPanId &mExtPanId;
    };

    struct NonNeighborMatcher
    {
        explicit NonNeighborMatcher(NeighborTable &aNeighborTable)
            : mNeighborTable(aNeighborTable)
        {
        }

        NeighborTable &mNeighborTable;
    };

    struct ExpireChecker // Matches if the peer is in `kDnssdRemoved` and already expired.
    {
        explicit ExpireChecker(uint32_t aUptimeNow)
            : mUptimeNow(aUptimeNow)
        {
        }

        uint32_t mUptimeNow;
    };

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    struct ServiceNameMatcher
    {
        explicit ServiceNameMatcher(const char *SrerviceName)
            : mServiceName(SrerviceName)
        {
        }

        const char *mServiceName;
    };

    struct HostNameMatcher
    {
        explicit HostNameMatcher(const char *aHostName)
            : mHostName(aHostName)
        {
        }

        const char *mHostName;
    };

    class AddressArray : public Heap::Array<Ip6::Address>
    {
    public:
        bool IsEmpty(void) const { return (GetLength() == 0); }
        void CloneFrom(const AddressArray &aOtherArray);
    };

#endif

    void     Init(Instance &aInstance);
    void     Free(void);
    void     SetDnssdState(DnssdState aState);
    void     SetExtAddress(const Mac::ExtAddress &aExtAddress);
    void     SetExtPanId(const MeshCoP::ExtendedPanId &aExtPanId) { mExtPanId = aExtPanId; }
    void     SetSockAddr(const Ip6::SockAddr &aSockAddr) { mSockAddr = aSockAddr; }
    bool     Matches(const Mac::ExtAddress &aExtAddress) const;
    bool     Matches(const Ip6::SockAddr &aSockAddr) const { return GetSockAddr() == aSockAddr; }
    bool     Matches(const Peer &aPeer) const { return this == &aPeer; }
    bool     Matches(const OtherExtPanIdMatcher &aMatcher) const { return GetExtPanId() != aMatcher.mExtPanId; }
    bool     Matches(const NonNeighborMatcher &aMatcher) const;
    bool     Matches(const ExpireChecker &aChecker) const;
    uint32_t DetermineExpirationDelay(uint32_t aUptimeNow) const;

#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    void SetPort(uint16_t aPort);
    bool Matches(const ServiceNameMatcher &aMatcher) const;
    bool Matches(const HostNameMatcher &aMatcher) const;
    void SignalPeerRemoval(void);

    static bool NameMatch(const Heap::String &aHeapString, const char *aName);
#else
    void SignalPeerRemoval(void) {}
#endif

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    void               Log(Action aAction) const;
    static const char *ActionToString(Action aAction);
    static const char *DnssdStateToString(DnssdState aState);
#else
    void Log(Action) const {}
#endif

    Peer      *mNext;
    DnssdState mDnssdState;
    uint32_t   mLastInteractionTime;
#if OPENTHREAD_CONFIG_TREL_MANAGE_DNSSD_ENABLE
    bool         mExtAddressSet : 1;
    bool         mResolvingService : 1;
    bool         mResolvingHost : 1;
    bool         mTxtDataValidated : 1;
    bool         mSockAddrUpdatedBasedOnRx : 1;
    uint16_t     mPort;
    Heap::String mServiceName;
    Heap::String mHostName;
    AddressArray mHostAddresses;
#endif
};

//---------------------------------------------------------------------------------------------------------------------

/**
 * Represents the TREL peer table.
 */
class PeerTable : public InstanceLocator, public OwningList<Peer>
{
    friend class Peer;

public:
    /**
     * Represents an iterator for iterating over TREL peer table entries.
     */
    typedef otTrelPeerIterator PeerIterator;

    /**
     * Initializes the TREL peer table.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    explicit PeerTable(Instance &aInstance);

    /**
     * Allocates a new peer entry and adds it to the table.
     *
     * This method attempts to allocate a new `Peer`. If successful, the new peer is added to the peer table and
     * becomes owned by it.
     *
     * If the initial allocation fail, the method will then attempt to make space by evicting an existing peer from the
     * table. The eviction strategy prioritizes removing a peer associated with a different Extended PAN ID. If no
     * such suitable peer is found, a non-neighbor peer will be targeted for eviction. If an existing peer is
     * successfully evicted, allocation for the new `Peer` object is re-attempted.
     *
     * @returns A pointer to the newly allocated and added `Peer`, or `nullptr` if the allocation fails.
     */
    Peer *AllocateAndAddNewPeer(void);

    /**
     * Initializes a peer table iterator.
     *
     * @param[in] aIterator  The iterator to initialize.
     */
    void InitIterator(PeerIterator &aIterator) const { aIterator = GetHead(); }

    /**
     * Iterates over the peer table entries.
     *
     * @param[in] aIterator  The iterator. MUST be initialized.
     *
     * @returns A pointer to the next `Peer` entry or `nullptr` if no more entries in the table.
     */
    const Peer *GetNextPeer(PeerIterator &aIterator) const;

    /**
     * Returns the number of TREL peers.
     *
     * @returns  The number of TREL peers.
     */
    uint16_t GetNumberOfPeers(void) const;

private:
#if !OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
#if OPENTHREAD_CONFIG_TREL_PEER_TABLE_SIZE != 0
    static constexpr uint16_t PoolSize = OPENTHREAD_CONFIG_TREL_PEER_TABLE_SIZE;
#else
    static constexpr uint16_t kExtraEntries = 32;
    static constexpr uint16_t PoolSize      = Mle::kMaxRouters + Mle::kMaxChildren + kExtraEntries;
#endif
#endif

    Peer *AllocatePeer(void);
    Error EvictPeer(void);
    void  HandleTimer(void);

    using PeerTimer = TimerMilliIn<PeerTable, &PeerTable::HandleTimer>;

    PeerTimer mTimer;
#if !OPENTHREAD_CONFIG_TREL_USE_HEAP_ENABLE
    Pool<Peer, PoolSize> mPool;
#endif
};

} // namespace Trel
} // namespace ot

#endif // #if OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE

#endif // TREL_PEER_HPP_
