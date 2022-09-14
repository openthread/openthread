/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes definitions for the RA-based routing management.
 *
 */

#ifndef ROUTING_MANAGER_HPP_
#define ROUTING_MANAGER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#if !OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#error "OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE is required for OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE."
#endif

#if !OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
#error "OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE is required for OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE."
#endif

#include <openthread/nat64.h>
#include <openthread/netdata.h>

#include "border_router/infra_if.hpp"
#include "common/array.hpp"
#include "common/error.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/pool.hpp"
#include "common/string.hpp"
#include "common/timer.hpp"
#include "net/ip6.hpp"
#include "net/nat64_translator.hpp"
#include "net/nd6.hpp"
#include "thread/network_data.hpp"

namespace ot {

namespace BorderRouter {

/**
 * This class implements bi-directional routing between Thread and Infrastructure networks.
 *
 * The Border Routing manager works on both Thread interface and infrastructure interface.
 * All ICMPv6 messages are sent/received on the infrastructure interface.
 *
 */
class RoutingManager : public InstanceLocator
{
    friend class ot::Notifier;
    friend class ot::Instance;

public:
    typedef NetworkData::RoutePreference       RoutePreference;     ///< Route preference (high, medium, low).
    typedef otBorderRoutingPrefixTableIterator PrefixTableIterator; ///< Prefix Table Iterator.
    typedef otBorderRoutingPrefixTableEntry    PrefixTableEntry;    ///< Prefix Table Entry.

    /**
     * This constructor initializes the routing manager.
     *
     * @param[in]  aInstance  A OpenThread instance.
     *
     */
    explicit RoutingManager(Instance &aInstance);

    /**
     * This method initializes the routing manager on given infrastructure interface.
     *
     * @param[in]  aInfraIfIndex      An infrastructure network interface index.
     * @param[in]  aInfraIfIsRunning  A boolean that indicates whether the infrastructure
     *                                interface is running.
     *
     * @retval  kErrorNone         Successfully started the routing manager.
     * @retval  kErrorInvalidArgs  The index of the infra interface is not valid.
     *
     */
    Error Init(uint32_t aInfraIfIndex, bool aInfraIfIsRunning);

    /**
     * This method enables/disables the Border Routing Manager.
     *
     * @note  The Border Routing Manager is enabled by default.
     *
     * @param[in]  aEnabled   A boolean to enable/disable the Border Routing Manager.
     *
     * @retval  kErrorInvalidState   The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone           Successfully enabled/disabled the Border Routing Manager.
     *
     */
    Error SetEnabled(bool aEnabled);

    /**
     * This method requests the Border Routing Manager to stop.
     *
     * If Border Routing Manager is running, calling this method immediately stops it and triggers the preparation
     * and sending of a final Router Advertisement (RA) message on infrastructure interface which deprecates and/or
     * removes any previously advertised PIO/RIO prefixes. If Routing Manager is not running (or not enabled), no
     * action is taken.
     *
     * Note that this method does not change whether the Routing Manager is enabled or disabled (see `SetEnabled()`).
     * It stops the Routing Manager temporarily. After calling this method if the device role gets changes (device
     * gets attached) and/or the infra interface state gets changed, the Routing Manager may be started again.
     *
     */
    void RequestStop(void) { Stop(); }

    /**
     * This method gets the preference used when advertising Route Info Options (e.g., for discovered OMR prefixes) in
     * Router Advertisement messages sent over the infrastructure link.
     *
     * @returns The Route Info Option preference.
     *
     */
    RoutePreference GetRouteInfoOptionPreference(void) const { return mRouteInfoOptionPreference; }

    /**
     * This method sets the preference to use when advertising Route Info Options (e.g., for discovered OMR prefixes)
     * in Router Advertisement messages sent over the infrastructure link.
     *
     * By default BR will use 'medium' preference level but this method allows the default value to be changed. As an
     * example, it can be set to 'low' preference in the case where device is a temporary BR (a mobile BR or a
     * battery-powered BR) to indicate that other BRs (if any) should be preferred over this BR on the infrastructure
     * link.
     *
     * @param[in] aPreference   The route preference to use.
     *
     */
    void SetRouteInfoOptionPreference(RoutePreference aPreference);

    /**
     * This method returns the local off-mesh-routable (OMR) prefix.
     *
     * The randomly generated 64-bit prefix will be added to the Thread Network Data if there isn't already an OMR
     * prefix.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the OMR prefix.
     *
     */
    Error GetOmrPrefix(Ip6::Prefix &aPrefix);

    /**
     * This method returns the currently favored off-mesh-routable (OMR) prefix.
     *
     * The favored OMR prefix can be discovered from Network Data or can be our local OMR prefix.
     *
     * An OMR prefix with higher preference is favored. If the preference is the same, then the smaller prefix (in the
     * sense defined by `Ip6::Prefix`) is favored.
     *
     * @param[out] aPrefix         A reference to output the favored prefix.
     * @param[out] aPreference     A reference to output the preference associated with the favored OMR prefix.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the OMR prefix.
     *
     */
    Error GetFavoredOmrPrefix(Ip6::Prefix &aPrefix, RoutePreference &aPreference);

    /**
     * This method returns the on-link prefix for the adjacent infrastructure link.
     *
     * The randomly generated 64-bit prefix will be advertised
     * on the infrastructure link if there isn't already a usable
     * on-link prefix being advertised on the link.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the on-link prefix.
     *
     */
    Error GetOnLinkPrefix(Ip6::Prefix &aPrefix);

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    /**
     * This method returns the local NAT64 prefix.
     *
     * @param[out]  aPrefix  A reference to where the prefix will be output to.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the NAT64 prefix.
     *
     */
    Error GetNat64Prefix(Ip6::Prefix &aPrefix);

    /**
     * This method returns the currently favored NAT64 prefix.
     *
     * The favored NAT64 prefix can be discovered from infrastructure link or can be the local NAT64 prefix.
     *
     * @param[out] aPrefix         A reference to output the favored prefix.
     * @param[out] aPreference     A reference to output the preference associated with the favored prefix.
     *
     * @retval  kErrorInvalidState  The Border Routing Manager is not initialized yet.
     * @retval  kErrorNone          Successfully retrieved the NAT64 prefix.
     *
     */
    Error GetFavoredNat64Prefix(Ip6::Prefix &aPrefix, RoutePreference &aRoutePreference);

    /**
     * This method updates mInfraIfNat64Prefix to @p aPrefix.
     *
     * @param[in]  aPrefix  A NAT64 prefix on infrastructure link.
     *
     */
    void UpdateInfraIfNat64Prefix(const Ip6::Prefix &aPrefix);
#endif // OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

    /**
     * This method processes a received ICMPv6 message from the infrastructure interface.
     *
     * Malformed or undesired messages are dropped silently.
     *
     * @param[in]  aPacket        The received ICMPv6 packet.
     * @param[in]  aSrcAddress    The source address this message is sent from.
     *
     */
    void HandleReceived(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);

    /**
     * This method handles infrastructure interface state changes.
     *
     */
    void HandleInfraIfStateChanged(void) { EvaluateState(); }

    /**
     * This method checks whether the on-mesh prefix configuration is a valid OMR prefix.
     *
     * @param[in] aOnMeshPrefixConfig  The on-mesh prefix configuration to check.
     *
     * @retval   TRUE    The prefix is a valid OMR prefix.
     * @retval   FALE    The prefix is not a valid OMR prefix.
     *
     */
    static bool IsValidOmrPrefix(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig);

    /**
     * This method checks whether a given prefix is a valid OMR prefix.
     *
     * @param[in]  aPrefix  The prefix to check.
     *
     * @retval   TRUE    The prefix is a valid OMR prefix.
     * @retval   FALE    The prefix is not a valid OMR prefix.
     *
     */
    static bool IsValidOmrPrefix(const Ip6::Prefix &aPrefix);

    /**
     * This method initializes a `PrefixTableIterator`.
     *
     * An iterator can be initialized again to start from the beginning of the table.
     *
     * When iterating over entries in the table, to ensure the entry update times are consistent, they are given
     * relative to the time the iterator was initialized.
     *
     * @param[out] aIterator  The iterator to initialize.
     *
     */
    void InitPrefixTableIterator(PrefixTableIterator &aIterator) const
    {
        mDiscoveredPrefixTable.InitIterator(aIterator);
    }

    /**
     * This method iterates over entries in the discovered prefix table.
     *
     * @param[in,out] aIterator  An iterator.
     * @param[out]    aEntry     A reference to the entry to populate.
     *
     * @retval kErrorNone        Got the next entry, @p aEntry is updated and @p aIterator is advanced.
     * @retval kErrorNotFound    No more entries in the table.
     *
     */
    Error GetNextPrefixTableEntry(PrefixTableIterator &aIterator, PrefixTableEntry &aEntry) const
    {
        return mDiscoveredPrefixTable.GetNextEntry(aIterator, aEntry);
    }

private:
    static constexpr uint8_t kMaxOnMeshPrefixes = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_ON_MESH_PREFIXES;

    static constexpr uint8_t kOmrPrefixLength    = OT_IP6_PREFIX_BITSIZE; // The length of an OMR prefix. In bits.
    static constexpr uint8_t kOnLinkPrefixLength = OT_IP6_PREFIX_BITSIZE; // The length of an On-link prefix. In bits.
    static constexpr uint8_t kBrUlaPrefixLength  = 48;                    // The length of a BR ULA prefix. In bits.
    static constexpr uint8_t kNat64PrefixLength  = 96;                    // The length of a NAT64 prefix. In bits.

    static constexpr uint16_t kOmrPrefixSubnetId   = 1; // The subnet ID of an OMR prefix within a BR ULA prefix.
    static constexpr uint16_t kNat64PrefixSubnetId = 2; // The subnet ID of a NAT64 prefix within a BR ULA prefix.

    // The maximum number of initial Router Advertisements.
    static constexpr uint32_t kMaxInitRtrAdvertisements = 3;

    static constexpr uint32_t kDefaultOmrPrefixLifetime    = 1800; // The default OMR prefix valid lifetime. In sec.
    static constexpr uint32_t kDefaultOnLinkPrefixLifetime = 1800; // The default on-link prefix valid lifetime. In sec.
    static constexpr uint32_t kDefaultNat64PrefixLifetime  = 300;  // The default NAT64 prefix valid lifetime. In sec.
    static constexpr uint32_t kMaxRtrAdvInterval           = 600;  // Max Router Advertisement Interval. In sec.
    static constexpr uint32_t kMinRtrAdvInterval           = kMaxRtrAdvInterval / 3; // Min RA Interval. In sec.
    static constexpr uint32_t kMaxInitRtrAdvInterval       = 16;                     // Max Initial RA Interval. In sec.
    static constexpr uint32_t kRaReplyJitter               = 500;  // Jitter for sending RA after rx RS. In msec.
    static constexpr uint32_t kPolicyEvaluationMinDelay    = 2000; // Min delay for policy evaluation. In msec.
    static constexpr uint32_t kPolicyEvaluationMaxDelay    = 4000; // Max delay for policy evaluation. In msec.
    static constexpr uint32_t kMinDelayBetweenRtrAdvs      = 3000; // Min delay (msec) between consecutive RAs.

    // The STALE_RA_TIME in seconds. The Routing Manager will consider the prefixes
    // and learned RA parameters STALE when they are not refreshed in STALE_RA_TIME
    // seconds. The Routing Manager will then start Router Solicitation to verify
    // that the STALE prefix is not being advertised anymore and remove the STALE
    // prefix.
    // The value is chosen in range of [`kMaxRtrAdvInterval` upper bound (1800s), `kDefaultOnLinkPrefixLifetime`].
    static constexpr uint32_t kRtrAdvStaleTime = 1800;

    static_assert(kMinRtrAdvInterval <= 3 * kMaxRtrAdvInterval / 4, "invalid RA intervals");
    static_assert(kDefaultOmrPrefixLifetime >= kMaxRtrAdvInterval, "invalid default OMR prefix lifetime");
    static_assert(kDefaultOnLinkPrefixLifetime >= kMaxRtrAdvInterval, "invalid default on-link prefix lifetime");
    static_assert(kRtrAdvStaleTime >= 1800 && kRtrAdvStaleTime <= kDefaultOnLinkPrefixLifetime,
                  "invalid RA STALE time");
    static_assert(kPolicyEvaluationMaxDelay > kPolicyEvaluationMinDelay,
                  "kPolicyEvaluationMaxDelay must be larger than kPolicyEvaluationMinDelay");

    enum RouterAdvTxMode : uint8_t // Used in `SendRouterAdvertisement()`
    {
        kInvalidateAllPrevPrefixes,
        kAdvPrefixesFromNetData,
    };

    enum ScheduleMode : uint8_t // Used in `ScheduleRoutingPolicyEvaluation()`
    {
        kImmediately,
        kForNextRa,
        kAfterRandomDelay,
        kToReplyToRs,
    };

    void HandleDiscoveredPrefixTableChanged(void); // Declare early so we can use in `mSignalTask`

    class DiscoveredPrefixTable : public InstanceLocator
    {
        // This class maintains the discovered on-link and route prefixes
        // from the received RA messages by processing PIO and RIO options
        // from the message. It takes care of processing the RA message but
        // delegates the decision whether to include or exclude a prefix to
        // `RoutingManager` by calling its `ShouldProcessPrefixInfoOption()`
        // and `ShouldProcessRouteInfoOption()` methods.
        //
        // It manages the lifetime of the discovered entries and publishes
        // and unpublishes the prefixes in the Network Data (as external
        // route) as they are added or removed.
        //
        // When there is any change in the table (an entry is added, removed,
        // or modified), it signals the change to `RoutingManager` by calling
        // `HandleDiscoveredPrefixTableChanged()` callback. A `Tasklet` is
        // used for signalling which ensures that if there are multiple
        // changes within the same flow of execution, the callback is
        // invoked after all the changes are processed.

    public:
        enum NetDataMode : uint8_t // Used in `Remove{}` methods
        {
            kUnpublishFromNetData, // Unpublish the entry from Network Data if previously published.
            kKeepInNetData,        // Keep entry in Network Data if previously published.
        };

        explicit DiscoveredPrefixTable(Instance &aInstance);

        void ProcessRouterAdvertMessage(const Ip6::Nd::RouterAdvertMessage &aRaMessage,
                                        const Ip6::Address &                aSrcAddress);

        void SetAllowDefaultRouteInNetData(bool aAllow);

        void FindFavoredOnLinkPrefix(Ip6::Prefix &aPrefix) const;
        bool ContainsOnLinkPrefix(const Ip6::Prefix &aPrefix) const;
        void RemoveOnLinkPrefix(const Ip6::Prefix &aPrefix, NetDataMode aNetDataMode);

        bool ContainsRoutePrefix(const Ip6::Prefix &aPrefix) const;
        void RemoveRoutePrefix(const Ip6::Prefix &aPrefix, NetDataMode aNetDataMode);

        void RemoveAllEntries(void);
        void RemoveOrDeprecateOldEntries(TimeMilli aTimeThreshold);

        TimeMilli CalculateNextStaleTime(TimeMilli aNow) const;

        void  InitIterator(PrefixTableIterator &aIterator) const;
        Error GetNextEntry(PrefixTableIterator &aIterator, PrefixTableEntry &aEntry) const;

    private:
        static constexpr uint16_t kMaxRouters = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_ROUTERS;
        static constexpr uint16_t kMaxEntries = OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES;

        class Entry : public LinkedListEntry<Entry>, public Unequatable<Entry>, private Clearable<Entry>
        {
            friend class LinkedListEntry<Entry>;

        public:
            enum Type : uint8_t
            {
                kTypeOnLink,
                kTypeRoute,
            };

            struct Matcher
            {
                Matcher(const Ip6::Prefix &aPrefix, Type aType)
                    : mPrefix(aPrefix)
                    , mType(aType)
                {
                }

                const Ip6::Prefix &mPrefix;
                bool               mType;
            };

            struct ExpirationChecker
            {
                explicit ExpirationChecker(TimeMilli aNow)
                    : mNow(aNow)
                {
                }

                TimeMilli mNow;
            };

            void               SetFrom(const Ip6::Nd::RouterAdvertMessage::Header &aRaHeader);
            void               SetFrom(const Ip6::Nd::PrefixInfoOption &aPio);
            void               SetFrom(const Ip6::Nd::RouteInfoOption &aRio);
            Type               GetType(void) const { return mType; }
            bool               IsOnLinkPrefix(void) const { return (mType == kTypeOnLink); }
            const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }
            const TimeMilli &  GetLastUpdateTime(void) const { return mLastUpdateTime; }
            uint32_t           GetValidLifetime(void) const { return mValidLifetime; }
            void               ClearValidLifetime(void) { mValidLifetime = 0; }
            TimeMilli          GetExpireTime(void) const;
            TimeMilli          GetStaleTime(void) const;
            RoutePreference    GetPreference(void) const;
            bool               operator==(const Entry &aOther) const;
            bool               Matches(const Matcher &aMatcher) const;
            bool               Matches(const ExpirationChecker &aCheker) const;

            // Methods to use when `IsOnLinkPrefix()`
            uint32_t GetPreferredLifetime(void) const { return mShared.mPreferredLifetime; }
            void     ClearPreferredLifetime(void) { mShared.mPreferredLifetime = 0; }
            bool     IsDeprecated(void) const;
            void     AdoptValidAndPreferredLiftimesFrom(const Entry &aEntry);

            // Method to use when `!IsOnlinkPrefix()`
            RoutePreference GetRoutePreference(void) const { return mShared.mRoutePreference; }

        private:
            static uint32_t CalculateExpireDelay(uint32_t aValidLifetime);

            Entry *     mNext;
            Ip6::Prefix mPrefix;
            Type        mType;
            TimeMilli   mLastUpdateTime;
            uint32_t    mValidLifetime;
            union
            {
                uint32_t        mPreferredLifetime; // Applicable when prefix is on-link.
                RoutePreference mRoutePreference;   // Applicable when prefix is not on-link
            } mShared;
        };

        struct Router
        {
            enum EmptyChecker : uint8_t
            {
                kContainsNoEntries
            };

            bool Matches(const Ip6::Address &aAddress) const { return aAddress == mAddress; }
            bool Matches(EmptyChecker) const { return mEntries.IsEmpty(); }

            Ip6::Address      mAddress;
            LinkedList<Entry> mEntries;
        };

        class Iterator : public PrefixTableIterator
        {
        public:
            const Router *GetRouter(void) const { return static_cast<const Router *>(mPtr1); }
            void          SetRouter(const Router *aRouter) { mPtr1 = aRouter; }
            const Entry * GetEntry(void) const { return static_cast<const Entry *>(mPtr2); }
            void          SetEntry(const Entry *aEntry) { mPtr2 = aEntry; }
            TimeMilli     GetInitTime(void) const { return TimeMilli(mData32); }
            void          SetInitTime(void) { mData32 = TimerMilli::GetNow().GetValue(); }
        };

        void        ProcessDefaultRoute(const Ip6::Nd::RouterAdvertMessage::Header &aRaHeader, Router &aRouter);
        void        ProcessPrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio, Router &aRouter);
        void        ProcessRouteInfoOption(const Ip6::Nd::RouteInfoOption &aRio, Router &aRouter);
        bool        ContainsPrefix(const Entry::Matcher &aMatcher) const;
        void        RemovePrefix(const Entry::Matcher &aMatcher, NetDataMode aNetDataMode);
        void        RemoveRoutersWithNoEntries(void);
        Entry *     AllocateEntry(void) { return mEntryPool.Allocate(); }
        void        FreeEntry(Entry &aEntry) { mEntryPool.Free(aEntry); }
        void        FreeEntries(LinkedList<Entry> &aEntries);
        void        UpdateNetworkDataOnChangeTo(Entry &aEntry);
        Entry *     FindFavoredEntryToPublish(const Ip6::Prefix &aPrefix);
        void        PublishEntry(const Entry &aEntry);
        void        UnpublishEntry(const Entry &aEntry);
        static void HandleTimer(Timer &aTimer);
        void        HandleTimer(void);
        void        RemoveExpiredEntries(void);
        void        SignalTableChanged(void);

        using SignalTask = TaskletIn<RoutingManager, &RoutingManager::HandleDiscoveredPrefixTableChanged>;

        Array<Router, kMaxRouters> mRouters;
        Pool<Entry, kMaxEntries>   mEntryPool;
        TimerMilli                 mTimer;
        SignalTask                 mSignalTask;
        bool                       mAllowDefaultRouteInNetData;
    };

    class LocalOmrPrefix;

    class OmrPrefix : public Clearable<OmrPrefix>
    {
    public:
        OmrPrefix(void) { Clear(); }

        bool               IsEmpty(void) const { return (mPrefix.GetLength() == 0); }
        void               SetFrom(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig);
        void               SetFrom(const LocalOmrPrefix &aLocalOmrPrefix);
        const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }
        RoutePreference    GetPreference(void) const { return mPreference; }
        bool               IsFavoredOver(const NetworkData::OnMeshPrefixConfig &aOmrPrefixConfig) const;
        bool               IsDomainPrefix(void) const { return mIsDomainPrefix; }

    private:
        Ip6::Prefix     mPrefix;
        RoutePreference mPreference;
        bool            mIsDomainPrefix;
    };

    class LocalOmrPrefix : public InstanceLocator
    {
    public:
        explicit LocalOmrPrefix(Instance &aInstance);
        void               GenerateFrom(const Ip6::Prefix &aBrUlaPrefix);
        const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }
        RoutePreference    GetPreference(void) const { return NetworkData::kRoutePreferenceLow; }
        Error              AddToNetData(void);
        void               RemoveFromNetData(void);
        bool               IsAddedInNetData(void) const { return mIsAddedInNetData; }

    private:
        Ip6::Prefix mPrefix;
        bool        mIsAddedInNetData;
    };

    class LocalOnLinkPrefix : public InstanceLocator
    {
    public:
        explicit LocalOnLinkPrefix(Instance &aInstance);

        void               Generate(void);
        void               Start(void);
        void               Stop(void);
        Error              Advertise(void);
        void               Deprecate(void);
        void               AppendAsPiosTo(Ip6::Nd::RouterAdvertMessage &aRaMessage);
        const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }
        bool               IsAdvertising(void) const { return (mState == kAdvertising); }
        void               HandleExtPanIdChange(void);

    private:
        enum State : uint8_t
        {
            kIdle,
            kAdvertising,
            kDeprecating,
        };

        void AppendCurPrefix(Ip6::Nd::RouterAdvertMessage &aRaMessage);
        void AppendOldPrefix(Ip6::Nd::RouterAdvertMessage &aRaMessage);
        void Unpublish(const Ip6::Prefix &aPrefix);

        static void HandleTimer(Timer &aTimer);
        void        HandleTimer(void);

        Ip6::Prefix mPrefix;
        State       mState;
        TimeMilli   mExpireTime;
        Ip6::Prefix mOldPrefix;
        TimeMilli   mOldExpireTime;
        TimerMilli  mTimer;
    };

    typedef Ip6::Prefix OnMeshPrefix;

    class OnMeshPrefixArray : public Array<OnMeshPrefix, kMaxOnMeshPrefixes>
    {
    public:
        void Add(const OnMeshPrefix &aPrefix);
        void MarkAsDeleted(const OnMeshPrefix &aPrefix);
    };

    struct RaInfo
    {
        // Tracks info about emitted RA messages: Number of RAs sent,
        // last tx time, header to use and whether the header is
        // discovered from receiving RAs from the host itself. This
        // ensures that if an entity on host is advertising certain
        // info in its RA header (e.g., a default route), the RAs we
        // emit from `RoutingManager` also include the same header.

        RaInfo(void)
            : mHeaderUpdateTime(TimerMilli::GetNow())
            , mIsHeaderFromHost(false)
            , mTxCount(0)
            , mLastTxTime(TimerMilli::GetNow() - kMinDelayBetweenRtrAdvs)
        {
        }

        Ip6::Nd::RouterAdvertMessage::Header mHeader;
        TimeMilli                            mHeaderUpdateTime;
        bool                                 mIsHeaderFromHost;
        uint32_t                             mTxCount;
        TimeMilli                            mLastTxTime;
    };

    class RsSender : public InstanceLocator
    {
    public:
        // This class implements tx of Router Solicitation (RS)
        // messages to discover other routers. `Start()` schedules
        // a cycle of RS transmissions of `kMaxTxCount` separated
        // by `kTxInterval`. At the end of cycle the callback
        // `HandleRsSenderFinished()` is invoked to inform end of
        // the cycle to `RoutingManager`.

        explicit RsSender(Instance &aInstance);

        bool IsInProgress(void) const { return mTimer.IsRunning(); }
        void Start(void);
        void Stop(void);

    private:
        // All time intervals are in msec.
        static constexpr uint32_t kMaxStartDelay     = 1000;        // Max random delay to send the first RS.
        static constexpr uint32_t kTxInterval        = 4000;        // Interval between RS tx.
        static constexpr uint32_t kRetryDelay        = kTxInterval; // Interval to wait to retry a failed RS tx.
        static constexpr uint32_t kWaitOnLastAttempt = 1000;        // Wait interval after last RS tx.
        static constexpr uint8_t  kMaxTxCount        = 3;           // Number of RS tx in one cycle.

        Error       SendRs(void);
        static void HandleTimer(Timer &aTimer);
        void        HandleTimer(void);

        uint8_t    mTxCount;
        TimerMilli mTimer;
        TimeMilli  mStartTime;
    };

    void  EvaluateState(void);
    void  Start(void);
    void  Stop(void);
    void  HandleNotifierEvents(Events aEvents);
    bool  IsInitialized(void) const { return mInfraIf.IsInitialized(); }
    bool  IsEnabled(void) const { return mIsEnabled; }
    Error LoadOrGenerateRandomBrUlaPrefix(void);

    void EvaluateOnLinkPrefix(void);

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    void DiscoverInfraIfNat64Prefix(void);
    void GenerateNat64Prefix(void);
    void EvaluateNat64Prefix(void);
#endif

    void  EvaluateRoutingPolicy(void);
    void  ScheduleRoutingPolicyEvaluation(ScheduleMode aMode);
    void  EvaluateOmrPrefix(void);
    Error PublishExternalRoute(const Ip6::Prefix &aPrefix, RoutePreference aRoutePreference, bool aNat64 = false);
    void  UnpublishExternalRoute(const Ip6::Prefix &aPrefix);
    void  HandleRsSenderFinished(TimeMilli aStartTime);
    void  SendRouterAdvertisement(RouterAdvTxMode aRaTxMode);

    static void HandleDiscoveredPrefixInvalidTimer(Timer &aTimer);
    void        HandleDiscoveredPrefixInvalidTimer(void);
    static void HandleDiscoveredPrefixStaleTimer(Timer &aTimer);
    void        HandleDiscoveredPrefixStaleTimer(void);
    static void HandleRoutingPolicyTimer(Timer &aTimer);
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    static void HandleInfraIfNat64PrefixStaleTimer(Timer &aTimer);
    void        HandleInfraIfNat64PrefixStaleTimer(void);
#endif

    void DeprecateOnLinkPrefix(void);
    void HandleRouterSolicit(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);
    void HandleRouterAdvertisement(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress);
    bool ShouldProcessPrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio, const Ip6::Prefix &aPrefix);
    bool ShouldProcessRouteInfoOption(const Ip6::Nd::RouteInfoOption &aRio, const Ip6::Prefix &aPrefix);
    void UpdateDiscoveredPrefixTableOnNetDataChange(void);
    bool NetworkDataContainsOmrPrefix(const Ip6::Prefix &aPrefix) const;
    void UpdateRouterAdvertHeader(const Ip6::Nd::RouterAdvertMessage *aRouterAdvertMessage);
    bool IsReceivedRouterAdvertFromManager(const Ip6::Nd::RouterAdvertMessage &aRaMessage) const;
    void ResetDiscoveredPrefixStaleTimer(void);

    static bool IsValidBrUlaPrefix(const Ip6::Prefix &aBrUlaPrefix);
    static bool IsValidOnLinkPrefix(const Ip6::Nd::PrefixInfoOption &aPio);
    static bool IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix);

    // Indicates whether the Routing Manager is running (started).
    bool mIsRunning;

    // Indicates whether the Routing manager is enabled. The Routing
    // Manager will be stopped if we are disabled.
    bool mIsEnabled;

    InfraIf mInfraIf;

    // The /48 BR ULA prefix loaded from local persistent storage or
    // randomly generated if none is found in persistent storage.
    Ip6::Prefix mBrUlaPrefix;

    LocalOmrPrefix mLocalOmrPrefix;
    OmrPrefix      mFavoredOmrPrefix;

    // List of on-mesh prefixes (discovered from Network Data) which
    // were advertised as RIO in the last sent RA message.
    OnMeshPrefixArray mAdvertisedPrefixes;

    RoutePreference mRouteInfoOptionPreference;

    // The currently favored (smallest) discovered on-link prefix.
    // Prefix length of zero indicates there is none.
    Ip6::Prefix mFavoredDiscoveredOnLinkPrefix;

    LocalOnLinkPrefix mLocalOnLinkPrefix;

    DiscoveredPrefixTable mDiscoveredPrefixTable;

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    // The latest NAT64 prefix discovered on the infrastructure interface.
    Ip6::Prefix mInfraIfNat64Prefix;
    // The NAT64 prefix allocated from the /48 BR ULA prefix.
    Ip6::Prefix mLocalNat64Prefix;
    // The NAT64 prefix published in Network Data. It can have the following value:
    // - empty: no NAT64 prefix is published from this BR
    // - the local NAT64 prefix
    // - the latest published infrastructure NAT64 prefix, which might differs from mInfraIfNat64Prefix
    Ip6::Prefix mPublishedNat64Prefix;

    TimerMilli mInfraIfNat64PrefixStaleTimer;
#endif

    RaInfo   mRaInfo;
    RsSender mRsSender;

    TimerMilli mDiscoveredPrefixStaleTimer;
    TimerMilli mRoutingPolicyTimer;
};

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // ROUTING_MANAGER_HPP_
