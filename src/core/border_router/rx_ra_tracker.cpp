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
 *   This file implements the Received RA Tracker.
 */

#include "rx_ra_tracker.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include "border_router/br_log.hpp"
#include "instance/instance.hpp"

namespace ot {
namespace BorderRouter {

RegisterLogModule("BorderRouting");

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker

RxRaTracker::RxRaTracker(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mRsSender(aInstance)
    , mExpirationTimer(aInstance)
    , mStaleTimer(aInstance)
    , mRouterTimer(aInstance)
    , mRdnssAddrTimer(aInstance)
    , mSignalTask(aInstance)
    , mRdnssAddrTask(aInstance)
{
    mLocalRaHeader.Clear();
}

void RxRaTracker::Start(void)
{
    mRsSender.Start();
    HandleNetDataChange();
}

void RxRaTracker::Stop(void)
{
    mRouters.Free();
    mIfAddresses.Free();
    mLocalRaHeader.Clear();
    mDecisionFactors.Clear();

    mExpirationTimer.Stop();
    mStaleTimer.Stop();
    mRouterTimer.Stop();
    mRdnssAddrTimer.Stop();

    mRsSender.Stop();
}

void RxRaTracker::HandleRsSenderFinished(TimeMilli aStartTime)
{
    // This is a callback from `RsSender` and is invoked when it
    // finishes a cycle of sending Router Solicitations. `aStartTime`
    // specifies the start time of the RS transmission cycle.
    //
    // We remove or deprecate old entries in discovered table that are
    // not refreshed during Router Solicitation. We also invalidate
    // the learned RA header if it is not refreshed during Router
    // Solicitation.

    RemoveOrDeprecateOldEntries(aStartTime);
    Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(RoutingManager::kImmediately);
}

void RxRaTracker::ProcessRouterAdvertMessage(const RouterAdvert::RxMessage &aRaMessage,
                                             const Ip6::Address            &aSrcAddress,
                                             RouterAdvOrigin                aRaOrigin)
{
    // Process a received RA message and update the prefix table.

    Router *router;

    switch (aRaOrigin)
    {
    case kThisBrOtherEntity:
    case kThisBrRoutingManager:
        UpdateIfAddresses(aSrcAddress);
        break;
    case kAnotherRouter:
        break;
    }

    VerifyOrExit(aRaOrigin != kThisBrRoutingManager);

    router = mRouters.FindMatching(aSrcAddress);

    if (router == nullptr)
    {
        Entry<Router> *newEntry = AllocateEntry<Router>();

        if (newEntry == nullptr)
        {
            LogWarn("Received RA from too many routers, ignore RA from %s", aSrcAddress.ToString().AsCString());
            ExitNow();
        }

        router = newEntry;
        router->Clear();
        router->mDiscoverTime = Get<Uptime>().GetUptimeInSeconds();
        router->mAddress      = aSrcAddress;

        mRouters.Push(*newEntry);
    }

    // RA message can indicate router provides default route in the RA
    // message header and can also include an RIO for `::/0`. When
    // processing an RA message, the preference and lifetime values
    // in a `::/0` RIO override the preference and lifetime values in
    // the RA header (per RFC 4191 section 3.1).

    ProcessRaHeader(aRaMessage.GetHeader(), *router, aRaOrigin);

    for (const Option &option : aRaMessage)
    {
        switch (option.GetType())
        {
        case Option::kTypePrefixInfo:
            ProcessPrefixInfoOption(static_cast<const PrefixInfoOption &>(option), *router);
            break;

        case Option::kTypeRouteInfo:
            ProcessRouteInfoOption(static_cast<const RouteInfoOption &>(option), *router);
            break;

        case Option::kTypeRecursiveDnsServer:
            ProcessRecursiveDnsServerOption(static_cast<const RecursiveDnsServerOption &>(option), *router);
            break;

        default:
            break;
        }
    }

    router->mIsLocalDevice = (aRaOrigin == kThisBrOtherEntity);

    router->ResetReachabilityState();

    Evaluate();

exit:
    return;
}

void RxRaTracker::ProcessRaHeader(const RouterAdvert::Header &aRaHeader, Router &aRouter, RouterAdvOrigin aRaOrigin)
{
    Entry<RoutePrefix> *entry;
    Ip6::Prefix         prefix;

    LogRaHeader(aRaHeader);

    aRouter.mManagedAddressConfigFlag = aRaHeader.IsManagedAddressConfigFlagSet();
    aRouter.mOtherConfigFlag          = aRaHeader.IsOtherConfigFlagSet();
    aRouter.mSnacRouterFlag           = aRaHeader.IsSnacRouterFlagSet();

    if (aRaOrigin == kThisBrOtherEntity)
    {
        // Update `mLocalRaHeader`, which tracks the RA header of
        // locally generated RA by another sw entity running on this
        // device.

        RouterAdvert::Header oldHeader = mLocalRaHeader;

        if (aRaHeader.GetRouterLifetime() == 0)
        {
            mLocalRaHeader.Clear();
        }
        else
        {
            mLocalRaHeader           = aRaHeader;
            mLocalRaHeaderUpdateTime = TimerMilli::GetNow();

            // The checksum is set to zero which indicates to platform
            // that it needs to do the calculation and update it.

            mLocalRaHeader.SetChecksum(0);
        }

        if (mLocalRaHeader != oldHeader)
        {
            Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(RoutingManager::kAfterRandomDelay);
        }
    }

    prefix.Clear();
    entry = aRouter.mRoutePrefixes.FindMatching(prefix);

    if (entry == nullptr)
    {
        VerifyOrExit(aRaHeader.GetRouterLifetime() != 0);

        entry = AllocateEntry<RoutePrefix>();

        if (entry == nullptr)
        {
            LogWarn("Discovered too many prefixes, ignore default route from RA header");
            ExitNow();
        }

        entry->SetFrom(aRaHeader);
        aRouter.mRoutePrefixes.Push(*entry);
    }
    else
    {
        entry->SetFrom(aRaHeader);
    }

exit:
    return;
}

void RxRaTracker::ProcessPrefixInfoOption(const PrefixInfoOption &aPio, Router &aRouter)
{
    Ip6::Prefix          prefix;
    Entry<OnLinkPrefix> *entry;
    bool                 disregard;

    // We track all valid PIO prefixes with the on-link (`L`) flag. The
    // `OnLinkPrefix` entries store other PIO flags and are used by
    // `DecisionFactors` to determine if a ULA or non-ULA on-link
    // prefix has been observed. This decision then guides
    // `RoutePublisher` on which route to publish. For determining the
    // favored on-link prefix, only eligible `OnLinkPrefix` entries are
    // considered. These entries must meet specific conditions, such as
    // having a valid 64-bit length and either the AutoAddrConfig
    // (`A`) and/or Dhcp6PdPreferred (`P`) flag set. The full set of
    // conditions is covered in `OnLinkPrefix::IsFavoredOver()`.

    VerifyOrExit(aPio.IsValid());
    aPio.GetPrefix(prefix);
    VerifyOrExit(!prefix.IsLinkLocal() && !prefix.IsMulticast());

    if (!aPio.IsOnLinkFlagSet())
    {
        aRouter.mOnLinkPrefixes.RemoveMatching(prefix);
        ExitNow();
    }

    // Disregard the PIO prefix if it matches our local on-link prefix,
    // as this indicates it's likely from a peer Border Router connected
    // to the same Thread mesh.

    disregard = (prefix == Get<RoutingManager>().mOnLinkPrefixManager.GetLocalPrefix());

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
    VerifyOrExit(!disregard);
#endif

    LogPrefixInfoOption(prefix, aPio.GetValidLifetime(), aPio.GetPreferredLifetime(), aPio.GetFlags());

    entry = aRouter.mOnLinkPrefixes.FindMatching(prefix);

    if (entry == nullptr)
    {
        VerifyOrExit(aPio.GetValidLifetime() != 0);

        entry = AllocateEntry<OnLinkPrefix>();

        if (entry == nullptr)
        {
            LogWarn("Discovered too many prefixes, ignore on-link prefix %s", prefix.ToString().AsCString());
            ExitNow();
        }

        entry->SetFrom(aPio);
        aRouter.mOnLinkPrefixes.Push(*entry);
    }
    else
    {
        OnLinkPrefix newPrefix;

        newPrefix.SetFrom(aPio);
        entry->AdoptFlagsAndValidAndPreferredLifetimesFrom(newPrefix);
    }

    entry->SetDisregardFlag(disregard);

exit:
    return;
}

void RxRaTracker::ProcessRouteInfoOption(const RouteInfoOption &aRio, Router &aRouter)
{
    Ip6::Prefix         prefix;
    Entry<RoutePrefix> *entry;
    bool                disregard;

    VerifyOrExit(aRio.IsValid());
    aRio.GetPrefix(prefix);

    VerifyOrExit(!prefix.IsLinkLocal() && !prefix.IsMulticast());

    // Disregard our own advertised OMR prefixes and those currently
    // present in the Thread Network Data. This implies it is likely
    // from a peer Thread BR connected to the same Thread mesh.
    //
    // There should be eventual parity between the `RioAdvertiser`
    // prefixes and the OMR prefixes in Network Data, but temporary
    // discrepancies can occur due to the tx timing of RAs and time
    // required to update Network Data (registering with leader). So
    // both checks are necessary.

    disregard = (Get<RoutingManager>().mOmrPrefixManager.GetLocalPrefix().GetPrefix() == prefix) ||
                Get<RoutingManager>().mRioAdvertiser.HasAdvertised(prefix) ||
                Get<NetworkData::Leader>().ContainsOmrPrefix(prefix);

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
    VerifyOrExit(!disregard);
#endif

    LogRouteInfoOption(prefix, aRio.GetRouteLifetime(), aRio.GetPreference());

    entry = aRouter.mRoutePrefixes.FindMatching(prefix);

    if (entry == nullptr)
    {
        VerifyOrExit(aRio.GetRouteLifetime() != 0);

        entry = AllocateEntry<RoutePrefix>();

        if (entry == nullptr)
        {
            LogWarn("Discovered too many prefixes, ignore route prefix %s", prefix.ToString().AsCString());
            ExitNow();
        }

        entry->SetFrom(aRio);
        aRouter.mRoutePrefixes.Push(*entry);
    }
    else
    {
        entry->SetFrom(aRio);
    }

    entry->SetDisregardFlag(disregard);

exit:
    return;
}

void RxRaTracker::ProcessRecursiveDnsServerOption(const RecursiveDnsServerOption &aRdnss, Router &aRouter)
{
    Entry<RdnssAddress> *entry;
    bool                 didChange = false;
    uint32_t             lifetime;

    VerifyOrExit(aRdnss.IsValid());

    lifetime = aRdnss.GetLifetime();

    for (uint16_t index = 0; index < aRdnss.GetNumAddresses(); index++)
    {
        const Ip6::Address &address = aRdnss.GetAddressAt(index);

        LogRecursiveDnsServerOption(address, lifetime);

        if (lifetime == 0)
        {
            didChange |= (aRouter.mRdnssAddresses.RemoveAndFreeAllMatching(address));
            continue;
        }

        entry = aRouter.mRdnssAddresses.FindMatching(address);

        if (entry != nullptr)
        {
            entry->SetFrom(aRdnss, index);
        }
        else
        {
            entry = AllocateEntry<RdnssAddress>();

            if (entry == nullptr)
            {
                LogWarn("Discovered too many entries, ignore RDNSS address %s", address.ToString().AsCString());
                ExitNow();
            }

            entry->SetFrom(aRdnss, index);
            aRouter.mRdnssAddresses.Push(*entry);
            didChange = true;
        }
    }

exit:
    if (didChange)
    {
        mRdnssAddrTask.Post();
    }
}

void RxRaTracker::UpdateIfAddresses(const Ip6::Address &aAddress)
{
    Entry<IfAddress> *entry;

    mIfAddresses.RemoveAndFreeAllMatching(IfAddress::InvalidChecker(GetInstance()));

    entry = mIfAddresses.FindMatching(aAddress);

    if (entry == nullptr)
    {
        entry = AllocateEntry<IfAddress>();
        VerifyOrExit(entry != nullptr);
        mIfAddresses.Push(*entry);
    }

    entry->SetFrom(aAddress, Get<Uptime>().GetUptimeInSeconds());

exit:
    return;
}

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE

template <> RxRaTracker::Entry<RxRaTracker::Router> *RxRaTracker::AllocateEntry(void)
{
    Entry<Router> *router = mRouterPool.Allocate();

    VerifyOrExit(router != nullptr);
    router->Init(GetInstance());

exit:
    return router;
}

template <class Type> RxRaTracker::Entry<Type> *RxRaTracker::AllocateEntry(void)
{
    static_assert(TypeTraits::IsSame<Type, OnLinkPrefix>::kValue || TypeTraits::IsSame<Type, RoutePrefix>::kValue ||
                      TypeTraits::IsSame<Type, RdnssAddress>::kValue || TypeTraits::IsSame<Type, IfAddress>::kValue,
                  "Type MUST be RoutePrefix, OnLinkPrefix, RdnssAddress, or IfAddress");

    Entry<Type> *entry       = nullptr;
    SharedEntry *sharedEntry = mEntryPool.Allocate();

    VerifyOrExit(sharedEntry != nullptr);
    entry = &sharedEntry->GetEntry<Type>();
    entry->Init(GetInstance());

exit:
    return entry;
}

template <> void RxRaTracker::Entry<RxRaTracker::Router>::Free(void)
{
    mOnLinkPrefixes.Free();
    mRoutePrefixes.Free();
    mRdnssAddresses.Free();
    Get<RxRaTracker>().mRouterPool.Free(*this);
}

template <class Type> void RxRaTracker::Entry<Type>::Free(void)
{
    static_assert(TypeTraits::IsSame<Type, OnLinkPrefix>::kValue || TypeTraits::IsSame<Type, RoutePrefix>::kValue ||
                      TypeTraits::IsSame<Type, RdnssAddress>::kValue || TypeTraits::IsSame<Type, IfAddress>::kValue,
                  "Type MUST be RoutePrefix, OnLinkPrefix, RdnssAddress, or IfAddress");

    Get<RxRaTracker>().mEntryPool.Free(*reinterpret_cast<SharedEntry *>(this));
}

#endif // !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE

void RxRaTracker::HandleLocalOnLinkPrefixChanged(void)
{
    const Ip6::Prefix &prefix    = Get<RoutingManager>().mOnLinkPrefixManager.GetLocalPrefix();
    bool               didChange = false;

    // When `TRACK_PEER_BR_INFO_ENABLE` is enabled, we mark
    // to disregard any on-link prefix entries matching the new
    // local on-link prefix. Otherwise, we can remove and free
    // them.

    for (Router &router : mRouters)
    {
#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
        OnLinkPrefix *entry = router.mOnLinkPrefixes.FindMatching(prefix);

        if ((entry != nullptr) && !entry->ShouldDisregard())
        {
            entry->SetDisregardFlag(true);
            didChange = true;
        }
#else
        didChange |= router.mOnLinkPrefixes.RemoveAndFreeAllMatching(prefix);
#endif
    }

    VerifyOrExit(didChange);

    Evaluate();

exit:
    return;
}

void RxRaTracker::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        HandleNetDataChange();
    }
}

void RxRaTracker::HandleNetDataChange(void)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;
    bool                            didChange = false;

    while (Get<NetworkData::Leader>().GetNext(iterator, prefixConfig) == kErrorNone)
    {
        if (!IsValidOmrPrefix(prefixConfig))
        {
            continue;
        }

        for (Router &router : mRouters)
        {
#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
            RoutePrefix *entry = router.mRoutePrefixes.FindMatching(prefixConfig.GetPrefix());

            if ((entry != nullptr) && !entry->ShouldDisregard())
            {
                entry->SetDisregardFlag(true);
                didChange = true;
            }
#else
            didChange |= router.mRoutePrefixes.RemoveAndFreeAllMatching(prefixConfig.GetPrefix());
#endif
        }
    }

    if (didChange)
    {
        Evaluate();
    }
}

void RxRaTracker::RemoveOrDeprecateOldEntries(TimeMilli aTimeThreshold)
{
    // Remove route prefix entries and deprecate on-link entries in
    // the table that are old (not updated since `aTimeThreshold`).

    for (Router &router : mRouters)
    {
        for (OnLinkPrefix &entry : router.mOnLinkPrefixes)
        {
            if (entry.GetLastUpdateTime() <= aTimeThreshold)
            {
                entry.ClearPreferredLifetime();
            }
        }

        for (RoutePrefix &entry : router.mRoutePrefixes)
        {
            if (entry.GetLastUpdateTime() <= aTimeThreshold)
            {
                entry.ClearValidLifetime();
            }
        }

        for (RdnssAddress &entry : router.mRdnssAddresses)
        {
            if (entry.GetLastUpdateTime() <= aTimeThreshold)
            {
                entry.ClearLifetime();
            }
        }
    }

    if (mLocalRaHeader.IsValid() && (mLocalRaHeaderUpdateTime <= aTimeThreshold))
    {
        mLocalRaHeader.Clear();
    }

    Evaluate();
}

void RxRaTracker::Evaluate(void)
{
    DecisionFactors oldFactors = mDecisionFactors;
    TimeMilli       now        = TimerMilli::GetNow();
    NextFireTime    routerTimeoutTime(now);
    NextFireTime    entryExpireTime(now);
    NextFireTime    staleTime(now);
    NextFireTime    rdnsssAddrExpireTime(now);
    RouterList      removedRouters;

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove expired entries associated with each router

    for (Router &router : mRouters)
    {
        ExpirationChecker expirationChecker(now);

        router.mOnLinkPrefixes.RemoveAndFreeAllMatching(expirationChecker);
        router.mRoutePrefixes.RemoveAndFreeAllMatching(expirationChecker);

        if (router.mRdnssAddresses.RemoveAndFreeAllMatching(expirationChecker))
        {
            mRdnssAddrTask.Post();
        }
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove any router entry that no longer has any valid on-link
    // or route prefixes, RDNSS addresses, or other relevant flags set.

    mRouters.RemoveAllMatching(removedRouters, Router::EmptyChecker());

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    for (Router &router : removedRouters)
    {
        ReportChangesToHistoryTracker(router, /* aRemoved */ true);
    }
#endif

    removedRouters.Free();

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Determine decision factors (favored on-link prefix, has any
    // ULA/non-ULA on-link/route prefix, M/O flags).

    mDecisionFactors.Clear();

    for (Router &router : mRouters)
    {
        router.mAllEntriesDisregarded = true;

        mDecisionFactors.UpdateFlagsFrom(router);

        for (OnLinkPrefix &entry : router.mOnLinkPrefixes)
        {
            mDecisionFactors.UpdateFrom(entry);
            entry.SetStaleTimeCalculated(false);

            router.mAllEntriesDisregarded &= entry.ShouldDisregard();
        }

        for (RoutePrefix &entry : router.mRoutePrefixes)
        {
            mDecisionFactors.UpdateFrom(entry);
            entry.SetStaleTimeCalculated(false);

            router.mAllEntriesDisregarded &= entry.ShouldDisregard();
        }
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE
    mDecisionFactors.mReachablePeerBrCount = CountReachablePeerBrs();
#endif

    if (oldFactors != mDecisionFactors)
    {
        mSignalTask.Post();
    }

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Schedule timers

    // If multiple routers advertise the same on-link or route prefix,
    // the stale time for the prefix is determined by the latest stale
    // time among all corresponding entries.
    //
    // The "StaleTimeCalculated" flag is used to ensure stale time is
    // calculated only once for each unique prefix. Initially, this
    // flag is cleared on all entries. As we iterate over routers and
    // their entries, `DetermineStaleTimeFor()` will consider all
    // matching entries and mark "StaleTimeCalculated" flag on them.

    for (Router &router : mRouters)
    {
        if (router.ShouldCheckReachability())
        {
            router.DetermineReachabilityTimeout();
            routerTimeoutTime.UpdateIfEarlier(router.mTimeoutTime);
        }

        for (const OnLinkPrefix &entry : router.mOnLinkPrefixes)
        {
            entryExpireTime.UpdateIfEarlier(entry.GetExpireTime());

            if (!entry.IsStaleTimeCalculated())
            {
                DetermineStaleTimeFor(entry, staleTime);
            }
        }

        for (const RoutePrefix &entry : router.mRoutePrefixes)
        {
            entryExpireTime.UpdateIfEarlier(entry.GetExpireTime());

            if (!entry.IsStaleTimeCalculated())
            {
                DetermineStaleTimeFor(entry, staleTime);
            }
        }

        for (const RdnssAddress &entry : router.mRdnssAddresses)
        {
            rdnsssAddrExpireTime.UpdateIfEarlier(entry.GetExpireTime());
        }
    }

    if (mLocalRaHeader.IsValid())
    {
        uint16_t interval = kStaleTime;

        if (mLocalRaHeader.GetRouterLifetime() > 0)
        {
            interval = Min(interval, mLocalRaHeader.GetRouterLifetime());
        }

        staleTime.UpdateIfEarlier(CalculateClampedExpirationTime(mLocalRaHeaderUpdateTime, interval));
    }

    mRouterTimer.FireAt(routerTimeoutTime);
    mExpirationTimer.FireAt(entryExpireTime);
    mStaleTimer.FireAt(staleTime);
    mRdnssAddrTimer.FireAt(rdnsssAddrExpireTime);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Report any changes to history tracker.

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE
    for (Router &router : mRouters)
    {
        ReportChangesToHistoryTracker(router, /* aRemoved */ false);
    }

#endif
}

void RxRaTracker::DetermineStaleTimeFor(const OnLinkPrefix &aPrefix, NextFireTime &aStaleTime)
{
    TimeMilli prefixStaleTime = aStaleTime.GetNow();
    bool      found           = false;

    for (Router &router : mRouters)
    {
        for (OnLinkPrefix &entry : router.mOnLinkPrefixes)
        {
            if (!entry.Matches(aPrefix.GetPrefix()))
            {
                continue;
            }

            entry.SetStaleTimeCalculated(true);

            if (entry.IsDeprecated())
            {
                continue;
            }

            prefixStaleTime = Max(prefixStaleTime, Max(aStaleTime.GetNow(), entry.GetStaleTime()));
            found           = true;
        }
    }

    if (found)
    {
        aStaleTime.UpdateIfEarlier(prefixStaleTime);
    }
}

void RxRaTracker::DetermineStaleTimeFor(const RoutePrefix &aPrefix, NextFireTime &aStaleTime)
{
    TimeMilli prefixStaleTime = aStaleTime.GetNow();
    bool      found           = false;

    for (Router &router : mRouters)
    {
        for (RoutePrefix &entry : router.mRoutePrefixes)
        {
            if (!entry.Matches(aPrefix.GetPrefix()))
            {
                continue;
            }

            entry.SetStaleTimeCalculated(true);

            prefixStaleTime = Max(prefixStaleTime, Max(aStaleTime.GetNow(), entry.GetStaleTime()));
            found           = true;
        }
    }

    if (found)
    {
        aStaleTime.UpdateIfEarlier(prefixStaleTime);
    }
}

void RxRaTracker::HandleStaleTimer(void)
{
    VerifyOrExit(Get<RoutingManager>().IsRunning());

    LogInfo("Stale timer expired");
    mRsSender.Start();

exit:
    return;
}

void RxRaTracker::HandleExpirationTimer(void) { Evaluate(); }

void RxRaTracker::HandleSignalTask(void) { Get<RoutingManager>().HandleRxRaTrackerDecisionFactorChanged(); }

void RxRaTracker::HandleRdnssAddrTask(void) { mRdnssCallback.InvokeIfSet(); }

void RxRaTracker::ProcessNeighborAdvertMessage(const NeighborAdvertMessage &aNaMessage)
{
    Router *router;

    VerifyOrExit(aNaMessage.IsValid());

    router = mRouters.FindMatching(aNaMessage.GetTargetAddress());
    VerifyOrExit(router != nullptr);

    LogInfo("Received NA from router %s", router->mAddress.ToString().AsCString());

    router->ResetReachabilityState();

    Evaluate();

exit:
    return;
}

void RxRaTracker::HandleRouterTimer(void)
{
    TimeMilli now = TimerMilli::GetNow();

    for (Router &router : mRouters)
    {
        if (!router.ShouldCheckReachability() || (router.mTimeoutTime > now))
        {
            continue;
        }

        router.mNsProbeCount++;

        if (router.IsReachable())
        {
            router.mTimeoutTime = now + ((router.mNsProbeCount < Router::kMaxNsProbes) ? Router::kNsProbeRetryInterval
                                                                                       : Router::kNsProbeTimeout);
            SendNeighborSolicitToRouter(router);
        }
        else
        {
            LogInfo("No response to all Neighbor Solicitations attempts from router %s - marking it unreachable",
                    router.mAddress.ToString().AsCString());

            // Remove route prefix entries and deprecate on-link prefix entries
            // of the unreachable router.

            for (OnLinkPrefix &entry : router.mOnLinkPrefixes)
            {
                if (!entry.IsDeprecated())
                {
                    entry.ClearPreferredLifetime();
                }
            }

            for (RoutePrefix &entry : router.mRoutePrefixes)
            {
                entry.ClearValidLifetime();
            }

            for (RdnssAddress &entry : router.mRdnssAddresses)
            {
                entry.ClearLifetime();
            }
        }
    }

    Evaluate();
}

void RxRaTracker::HandleRdnssAddrTimer(void) { Evaluate(); }

void RxRaTracker::SendNeighborSolicitToRouter(const Router &aRouter)
{
    InfraIf::Icmp6Packet      packet;
    NeighborSolicitHeader     nsHdr;
    TxMessage                 nsMsg;
    InfraIf::LinkLayerAddress linkAddr;

    VerifyOrExit(!mRsSender.IsInProgress());

    nsHdr.SetTargetAddress(aRouter.mAddress);
    SuccessOrExit(nsMsg.Append(nsHdr));

    if (Get<InfraIf>().GetLinkLayerAddress(linkAddr) == kErrorNone)
    {
        SuccessOrExit(nsMsg.AppendLinkLayerOption(linkAddr, Option::kSourceLinkLayerAddr));
    }

    nsMsg.GetAsPacket(packet);

    IgnoreError(Get<InfraIf>().Send(packet, aRouter.mAddress));

    LogInfo("Sent Neighbor Solicitation to %s - attempt:%u/%u", aRouter.mAddress.ToString().AsCString(),
            aRouter.mNsProbeCount, Router::kMaxNsProbes);

exit:
    return;
}

void RxRaTracker::SetHeaderFlagsOn(RouterAdvert::Header &aHeader) const
{
    if (mDecisionFactors.mHeaderManagedAddressConfigFlag)
    {
        aHeader.SetManagedAddressConfigFlag();
    }

    if (mDecisionFactors.mHeaderOtherConfigFlag)
    {
        aHeader.SetOtherConfigFlag();
    }
}

bool RxRaTracker::IsAddressOnLink(const Ip6::Address &aAddress) const
{
    bool isOnLink = Get<RoutingManager>().mOnLinkPrefixManager.AddressMatchesLocalPrefix(aAddress);

    VerifyOrExit(!isOnLink);

    for (const Router &router : mRouters)
    {
        for (const OnLinkPrefix &onLinkPrefix : router.mOnLinkPrefixes)
        {
            isOnLink = aAddress.MatchesPrefix(onLinkPrefix.GetPrefix());
            VerifyOrExit(!isOnLink);
        }
    }

exit:
    return isOnLink;
}

bool RxRaTracker::IsAddressReachableThroughExplicitRoute(const Ip6::Address &aAddress) const
{
    // Checks whether the `aAddress` matches any discovered route
    // prefix excluding `::/0`.

    bool isReachable = false;

    for (const Router &router : mRouters)
    {
        for (const RoutePrefix &routePrefix : router.mRoutePrefixes)
        {
            if (routePrefix.GetPrefix().GetLength() == 0)
            {
                continue;
            }

            isReachable = aAddress.MatchesPrefix(routePrefix.GetPrefix());
            VerifyOrExit(!isReachable);
        }
    }

exit:
    return isReachable;
}

void RxRaTracker::InitIterator(PrefixTableIterator &aIterator) const
{
    static_cast<Iterator &>(aIterator).Init(mRouters.GetHead(), Get<Uptime>().GetUptimeInSeconds());
}

Error RxRaTracker::GetNextPrefixTableEntry(PrefixTableIterator &aIterator, PrefixTableEntry &aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    ClearAllBytes(aEntry);

    SuccessOrExit(error = iterator.AdvanceToNextPrefixEntry());

    iterator.GetRouter()->CopyInfoTo(aEntry.mRouter, iterator.GetInitTime(), iterator.GetInitUptime());

    switch (iterator.GetPrefixType())
    {
    case Iterator::kOnLinkPrefix:
        iterator.GetEntry<OnLinkPrefix>()->CopyInfoTo(aEntry, iterator.GetInitTime());
        break;
    case Iterator::kRoutePrefix:
        iterator.GetEntry<RoutePrefix>()->CopyInfoTo(aEntry, iterator.GetInitTime());
        break;
    }

exit:
    return error;
}

Error RxRaTracker::GetNextRouterEntry(PrefixTableIterator &aIterator, RouterEntry &aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    ClearAllBytes(aEntry);

    SuccessOrExit(error = iterator.AdvanceToNextRouter(Iterator::kRouterIterator));
    iterator.GetRouter()->CopyInfoTo(aEntry, iterator.GetInitTime(), iterator.GetInitUptime());

exit:
    return error;
}

Error RxRaTracker::GetNextRdnssAddrEntry(PrefixTableIterator &aIterator, RdnssAddrEntry &aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    ClearAllBytes(aEntry);

    SuccessOrExit(error = iterator.AdvanceToNextRdnssAddrEntry());

    iterator.GetRouter()->CopyInfoTo(aEntry.mRouter, iterator.GetInitTime(), iterator.GetInitUptime());
    iterator.GetEntry<RdnssAddress>()->CopyInfoTo(aEntry, iterator.GetInitTime());

exit:
    return error;
}

Error RxRaTracker::GetNextIfAddrEntry(PrefixTableIterator &aIterator, IfAddrEntry &aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    ClearAllBytes(aEntry);

    SuccessOrExit(error = iterator.AdvanceToNextIfAddrEntry(mIfAddresses.GetHead()));

    iterator.GetEntry<IfAddress>()->CopyInfoTo(aEntry, iterator.GetInitUptime());

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_MULTI_AIL_DETECTION_ENABLE
uint16_t RxRaTracker::CountReachablePeerBrs(void) const
{
    uint16_t count = 0;

    for (const Router &router : mRouters)
    {
        if (!router.mIsLocalDevice && router.IsPeerBr() && router.IsReachable())
        {
            count++;
        }
    }

    return count;
}
#endif

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

void RxRaTracker::ReportChangesToHistoryTracker(Router &aRouter, bool aRemoved)
{
    // Report any changes in the `aRouter` to `HistoryTracker` only if
    // something has changed since the last recorded event.

    Router::HistoryInfo        oldInfo;
    HistoryTracker::AilRouter *entry;

    if (aRemoved)
    {
        // If we have never recorded this router entry in the
        // `HistoryTracker`, there is no point in reporting its
        // removal. This can happen if we receive an RA from a router
        // with no useful information that we want to track. In this
        // case, the router entry is removed immediately during
        // `Evaluate()`.

        VerifyOrExit(aRouter.mHistoryInfo.mHistoryRecorded);
    }
    else
    {
        oldInfo = aRouter.mHistoryInfo;
    }

    aRouter.mHistoryInfo.DetermineFrom(aRouter);

    VerifyOrExit(aRemoved || (aRouter.mHistoryInfo != oldInfo));

    // Allocate and populate the new `HistoryTracker::AilRouter` entry.

    entry = Get<HistoryTracker::Local>().RecordAilRouterEvent();
    VerifyOrExit(entry != nullptr);

    entry->mEvent = aRemoved ? HistoryTracker::Local::kAilRouterRemoved
                             : (oldInfo.mHistoryRecorded ? HistoryTracker::Local::kAilRouterChanged
                                                         : HistoryTracker::Local::kAilRouterAdded);

    entry->mAddress                  = aRouter.mAddress;
    entry->mDefRoutePreference       = static_cast<int8_t>(aRouter.mHistoryInfo.mDefRoutePreference);
    entry->mFavoredOnLinkPrefix      = aRouter.mHistoryInfo.mFavoredOnLinkPrefix;
    entry->mProvidesDefaultRoute     = aRouter.mHistoryInfo.mProvidesDefaultRoute;
    entry->mManagedAddressConfigFlag = aRouter.mHistoryInfo.mManagedAddressConfigFlag;
    entry->mOtherConfigFlag          = aRouter.mHistoryInfo.mOtherConfigFlag;
    entry->mSnacRouterFlag           = aRouter.mHistoryInfo.mSnacRouterFlag;
    entry->mIsLocalDevice            = aRouter.mHistoryInfo.mIsLocalDevice;
    entry->mIsReachable              = aRouter.mHistoryInfo.mIsReachable;
    entry->mIsPeerBr                 = aRouter.mHistoryInfo.mIsPeerBr;

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker::Iterator

void RxRaTracker::Iterator::Init(const Entry<Router> *aRoutersHead, uint32_t aUptime)
{
    SetInitUptime(aUptime);
    SetInitTime();
    SetType(kUnspecified);
    SetRouter(aRoutersHead);
    SetEntry(nullptr);
    SetPrefixType(kRoutePrefix);
}

Error RxRaTracker::Iterator::AdvanceToNextRouter(Type aType)
{
    Error error = kErrorNone;

    if (GetType() == kUnspecified)
    {
        // On the first call, when iterator type is `kUnspecified`, we
        // set the type, and keep the `GetRouter()` as is so to start
        // from the first router in the list.

        SetType(aType);
    }
    else
    {
        // On subsequent call, we ensure that the iterator type
        // matches what we expect and advance to the next router on
        // the list.

        VerifyOrExit(GetType() == aType, error = kErrorInvalidArgs);
        VerifyOrExit(GetRouter() != nullptr, error = kErrorNone);
        SetRouter(GetRouter()->GetNext());
    }

    VerifyOrExit(GetRouter() != nullptr, error = kErrorNotFound);

exit:
    return error;
}

Error RxRaTracker::Iterator::AdvanceToNextPrefixEntry(void)
{
    Error error = kErrorNone;

    VerifyOrExit(GetRouter() != nullptr, error = kErrorNotFound);

    if (HasEntry())
    {
        switch (GetPrefixType())
        {
        case kOnLinkPrefix:
            SetEntry(GetEntry<OnLinkPrefix>()->GetNext());
            break;
        case kRoutePrefix:
            SetEntry(GetEntry<RoutePrefix>()->GetNext());
            break;
        }
    }

    while (!HasEntry())
    {
        switch (GetPrefixType())
        {
        case kOnLinkPrefix:

            // Transition from on-link prefixes to route prefixes of
            // the current router.

            SetEntry(GetRouter()->mRoutePrefixes.GetHead());
            SetPrefixType(kRoutePrefix);
            break;

        case kRoutePrefix:

            // Transition to the next router and start with its on-link
            // prefixes.
            //
            // On the first call when iterator type is `kUnspecified`,
            // `AdvanceToNextRouter()` sets the type and starts from
            // the first router.

            SuccessOrExit(error = AdvanceToNextRouter(kPrefixIterator));
            SetEntry(GetRouter()->mOnLinkPrefixes.GetHead());
            SetPrefixType(kOnLinkPrefix);
            break;
        }
    }

exit:
    return error;
}

Error RxRaTracker::Iterator::AdvanceToNextRdnssAddrEntry(void)
{
    Error error = kErrorNone;

    VerifyOrExit(GetRouter() != nullptr, error = kErrorNotFound);

    if (HasEntry())
    {
        VerifyOrExit(GetType() == kRdnssAddrIterator, error = kErrorInvalidArgs);
        SetEntry(GetEntry<RdnssAddress>()->GetNext());
    }

    while (!HasEntry())
    {
        SuccessOrExit(error = AdvanceToNextRouter(kRdnssAddrIterator));
        SetEntry(GetRouter()->mRdnssAddresses.GetHead());
    }

exit:
    return error;
}

Error RxRaTracker::Iterator::AdvanceToNextIfAddrEntry(const Entry<IfAddress> *aListHead)
{
    Error error = kErrorNone;

    if (GetType() == kUnspecified)
    {
        SetType(kIfAddrIterator);
        SetEntry(aListHead);
    }
    else
    {
        VerifyOrExit(GetType() == kIfAddrIterator, error = kErrorInvalidArgs);
        VerifyOrExit(HasEntry(), error = kErrorNotFound);
        SetEntry(GetEntry<IfAddress>()->GetNext());
    }

    VerifyOrExit(HasEntry(), error = kErrorNotFound);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker::Router

bool RxRaTracker::Router::ShouldCheckReachability(void) const
{
    // Perform reachability check (send NS probes) only if the router:
    // - Is not already marked as unreachable (due to failed NS probes)
    // - Is not the local device itself (to avoid potential issues with
    //   the platform receiving/processing NAs from itself).

    return IsReachable() && !mIsLocalDevice;
}

void RxRaTracker::Router::ResetReachabilityState(void)
{
    // Called when an RA or NA is received and processed.

    mNsProbeCount   = 0;
    mLastUpdateTime = TimerMilli::GetNow();
    mTimeoutTime    = mLastUpdateTime + Random::NonCrypto::AddJitter(kReachableInterval, kJitter);
}

void RxRaTracker::Router::DetermineReachabilityTimeout(void)
{
    uint32_t interval;

    VerifyOrExit(ShouldCheckReachability());
    VerifyOrExit(mNsProbeCount == 0);

    // If all of the router's prefix entries are marked as
    // disregarded (excluded from any decisions), it indicates that
    // this router is likely a peer BR connected to the same Thread
    // mesh. We use a longer reachability check interval for such
    // peer BRs.

    interval     = mAllEntriesDisregarded ? kPeerBrReachableInterval : kReachableInterval;
    mTimeoutTime = mLastUpdateTime + Random::NonCrypto::AddJitter(interval, kJitter);

exit:
    return;
}

bool RxRaTracker::Router::Matches(const EmptyChecker &aChecker)
{
    OT_UNUSED_VARIABLE(aChecker);

    bool hasFlags = false;

    // Router can be removed if it does not advertise M or O flags and
    // also does not have any advertised prefix entries (RIO/PIO) or
    // RDNSS address entries. If the router already failed to respond
    // to max NS probe attempts, we consider it as offline and
    // therefore do not consider its flags anymore.

    if (IsReachable())
    {
        hasFlags = (mManagedAddressConfigFlag || mOtherConfigFlag);
    }

    return !hasFlags && mOnLinkPrefixes.IsEmpty() && mRoutePrefixes.IsEmpty() && mRdnssAddresses.IsEmpty();
}

bool RxRaTracker::Router::IsPeerBr(void) const
{
    // Determines whether the router is a peer BR (connected to the
    // same Thread mesh network). It must have at least one entry
    // (on-link or route) and all entries should be marked to be
    // disregarded. While this model is generally effective to detect
    // peer BRs, it may not be 100% accurate in all scenarios.

    return mAllEntriesDisregarded && !(mOnLinkPrefixes.IsEmpty() && mRoutePrefixes.IsEmpty());
}

void RxRaTracker::Router::CopyInfoTo(RouterEntry &aEntry, TimeMilli aNow, uint32_t aUptime) const
{
    aEntry.mAddress                  = mAddress;
    aEntry.mMsecSinceLastUpdate      = aNow - mLastUpdateTime;
    aEntry.mAge                      = aUptime - mDiscoverTime;
    aEntry.mManagedAddressConfigFlag = mManagedAddressConfigFlag;
    aEntry.mOtherConfigFlag          = mOtherConfigFlag;
    aEntry.mSnacRouterFlag           = mSnacRouterFlag;
    aEntry.mIsLocalDevice            = mIsLocalDevice;
    aEntry.mIsReachable              = IsReachable();
    aEntry.mIsPeerBr                 = IsPeerBr();
}

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker::Router::HistoryInfo

#if OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

void RxRaTracker::Router::HistoryInfo::DetermineFrom(const Router &aRouter)
{
    Ip6::Prefix        emptyPrefix;
    const RoutePrefix *defRoute;

    Clear();

    mHistoryRecorded          = true;
    mManagedAddressConfigFlag = aRouter.mManagedAddressConfigFlag;
    mOtherConfigFlag          = aRouter.mOtherConfigFlag;
    mSnacRouterFlag           = aRouter.mSnacRouterFlag;
    mIsLocalDevice            = aRouter.mIsLocalDevice;
    mIsReachable              = aRouter.IsReachable();
    mIsPeerBr                 = aRouter.IsPeerBr();

    emptyPrefix.Clear();
    defRoute = aRouter.mRoutePrefixes.FindMatching(emptyPrefix);

    if ((defRoute != nullptr) && (defRoute->GetValidLifetime() > 0))
    {
        mProvidesDefaultRoute = true;
        mDefRoutePreference   = defRoute->GetRoutePreference();
    }

    for (const OnLinkPrefix &onLinkPrefix : aRouter.mOnLinkPrefixes)
    {
        if (onLinkPrefix.IsFavoredOver(mFavoredOnLinkPrefix))
        {
            mFavoredOnLinkPrefix = onLinkPrefix.GetPrefix();
        }
    }
}

#endif // OPENTHREAD_CONFIG_HISTORY_TRACKER_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker::DecisionFactors

void RxRaTracker::DecisionFactors::UpdateFlagsFrom(const Router &aRouter)
{
    // Determine the `M` and `O` flags to include in the RA message
    // header to be emitted.
    //
    // If any discovered router on infrastructure which is not itself a
    // stub router (e.g., another Thread BR) includes the `M` or `O`
    // flag, we also include the same flag.

    VerifyOrExit(!aRouter.mSnacRouterFlag);
    VerifyOrExit(aRouter.IsReachable());

    if (aRouter.mManagedAddressConfigFlag)
    {
        mHeaderManagedAddressConfigFlag = true;
    }

    if (aRouter.mOtherConfigFlag)
    {
        mHeaderOtherConfigFlag = true;
    }

exit:
    return;
}

void RxRaTracker::DecisionFactors::UpdateFrom(const OnLinkPrefix &aOnLinkPrefix)
{
    VerifyOrExit(!aOnLinkPrefix.ShouldDisregard());

    if (aOnLinkPrefix.GetPrefix().IsUniqueLocal())
    {
        mHasUlaOnLink = true;
    }
    else
    {
        mHasNonUlaOnLink = true;
    }

    if (aOnLinkPrefix.IsFavoredOver(mFavoredOnLinkPrefix))
    {
        mFavoredOnLinkPrefix = aOnLinkPrefix.GetPrefix();
    }

exit:
    return;
}

void RxRaTracker::DecisionFactors::UpdateFrom(const RoutePrefix &aRoutePrefix)
{
    VerifyOrExit(!aRoutePrefix.ShouldDisregard());

    if (!mHasNonUlaRoute)
    {
        mHasNonUlaRoute = !aRoutePrefix.GetPrefix().IsUniqueLocal();
    }

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker::RsSender

RxRaTracker::RsSender::RsSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTxCount(0)
    , mTimer(aInstance)
{
}

void RxRaTracker::RsSender::Start(void)
{
    uint32_t delay;

    VerifyOrExit(!IsInProgress());

    delay = Random::NonCrypto::GetUint32InRange(0, kMaxStartDelay);

    LogInfo("RsSender: Starting - will send first RS in %lu msec", ToUlong(delay));

    mTxCount   = 0;
    mStartTime = TimerMilli::GetNow();
    mTimer.Start(delay);

exit:
    return;
}

void RxRaTracker::RsSender::Stop(void) { mTimer.Stop(); }

Error RxRaTracker::RsSender::SendRs(void)
{
    Ip6::Address              destAddress;
    RouterSolicitHeader       rsHdr;
    TxMessage                 rsMsg;
    InfraIf::LinkLayerAddress linkAddr;
    InfraIf::Icmp6Packet      packet;
    Error                     error;

    SuccessOrExit(error = rsMsg.Append(rsHdr));

    if (Get<InfraIf>().GetLinkLayerAddress(linkAddr) == kErrorNone)
    {
        SuccessOrExit(error = rsMsg.AppendLinkLayerOption(linkAddr, Option::kSourceLinkLayerAddr));
    }

    rsMsg.GetAsPacket(packet);
    destAddress.SetToLinkLocalAllRoutersMulticast();

    error = Get<InfraIf>().Send(packet, destAddress);

    if (error == kErrorNone)
    {
        Get<Ip6::Ip6>().GetBorderRoutingCounters().mRsTxSuccess++;
    }
    else
    {
        Get<Ip6::Ip6>().GetBorderRoutingCounters().mRsTxFailure++;
    }
exit:
    return error;
}

void RxRaTracker::RsSender::HandleTimer(void)
{
    Error    error;
    uint32_t delay;

    if (mTxCount >= kMaxTxCount)
    {
        LogInfo("RsSender: Finished sending RS msgs and waiting for RAs");
        Get<RxRaTracker>().HandleRsSenderFinished(mStartTime);
        ExitNow();
    }

    error = SendRs();

    if (error == kErrorNone)
    {
        mTxCount++;
        delay = (mTxCount == kMaxTxCount) ? kWaitOnLastAttempt : kTxInterval;
        LogInfo("RsSender: Sent RS %u/%u", mTxCount, kMaxTxCount);
    }
    else
    {
        LogCrit("RsSender: Failed to send RS %u/%u: %s", mTxCount + 1, kMaxTxCount, ErrorToString(error));

        // Note that `mTxCount` is intentionally not incremented
        // if the tx fails.
        delay = kRetryDelay;
    }

    mTimer.Start(delay);

exit:
    return;
}

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
