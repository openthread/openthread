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
 *   This file includes implementation for the RA-based routing management.
 *
 */

#include "border_router/routing_manager.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <string.h>

#include <openthread/platform/infra_if.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "meshcop/extended_panid.hpp"
#include "net/ip6.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"

namespace ot {

namespace BorderRouter {

RegisterLogModule("BorderRouter");

RoutingManager::RoutingManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsRunning(false)
    , mIsEnabled(false)
    , mInfraIf(aInstance)
    , mLocalOmrPrefix(aInstance)
    , mRouteInfoOptionPreference(NetworkData::kRoutePreferenceMedium)
    , mIsAdvertisingLocalOnLinkPrefix(false)
    , mOnLinkPrefixDeprecateTimer(aInstance, HandleOnLinkPrefixDeprecateTimer)
    , mIsAdvertisingLocalNat64Prefix(false)
    , mDiscoveredPrefixTable(aInstance)
    , mTimeRouterAdvMessageLastUpdate(TimerMilli::GetNow())
    , mLearntRouterAdvMessageFromHost(false)
    , mDiscoveredPrefixStaleTimer(aInstance, HandleDiscoveredPrefixStaleTimer)
    , mRouterAdvertisementCount(0)
    , mLastRouterAdvertisementSendTime(TimerMilli::GetNow() - kMinDelayBetweenRtrAdvs)
    , mRouterSolicitTimer(aInstance, HandleRouterSolicitTimer)
    , mRouterSolicitCount(0)
    , mRoutingPolicyTimer(aInstance, HandleRoutingPolicyTimer)
{
    mFavoredDiscoveredOnLinkPrefix.Clear();

    mBrUlaPrefix.Clear();

    mLocalOnLinkPrefix.Clear();

    mLocalNat64Prefix.Clear();
}

Error RoutingManager::Init(uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    Error error;

    SuccessOrExit(error = mInfraIf.Init(aInfraIfIndex));

    SuccessOrExit(error = LoadOrGenerateRandomBrUlaPrefix());
    mLocalOmrPrefix.GenerateFrom(mBrUlaPrefix);
#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
    GenerateNat64Prefix();
#endif
    GenerateOnLinkPrefix();

    error = mInfraIf.HandleStateChanged(mInfraIf.GetIfIndex(), aInfraIfIsRunning);

exit:
    if (error != kErrorNone)
    {
        mInfraIf.Deinit();
    }

    return error;
}

Error RoutingManager::SetEnabled(bool aEnabled)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);

    VerifyOrExit(aEnabled != mIsEnabled);

    mIsEnabled = aEnabled;
    EvaluateState();

exit:
    return error;
}

void RoutingManager::SetRouteInfoOptionPreference(RoutePreference aPreference)
{
    VerifyOrExit(mRouteInfoOptionPreference != aPreference);

    mRouteInfoOptionPreference = aPreference;

    VerifyOrExit(mIsRunning);
    StartRoutingPolicyEvaluationJitter(kRoutingPolicyEvaluationJitter);

exit:
    return;
}

Error RoutingManager::GetOmrPrefix(Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mLocalOmrPrefix.GetPrefix();

exit:
    return error;
}

Error RoutingManager::GetOnLinkPrefix(Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mLocalOnLinkPrefix;

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
Error RoutingManager::GetNat64Prefix(Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mLocalNat64Prefix;

exit:
    return error;
}
#endif

Error RoutingManager::LoadOrGenerateRandomBrUlaPrefix(void)
{
    Error error     = kErrorNone;
    bool  generated = false;

    if (Get<Settings>().Read<Settings::BrUlaPrefix>(mBrUlaPrefix) != kErrorNone || !IsValidBrUlaPrefix(mBrUlaPrefix))
    {
        Ip6::NetworkPrefix randomUlaPrefix;

        LogNote("No valid /48 BR ULA prefix found in settings, generating new one");

        SuccessOrExit(error = randomUlaPrefix.GenerateRandomUla());

        mBrUlaPrefix.Set(randomUlaPrefix);
        mBrUlaPrefix.SetSubnetId(0);
        mBrUlaPrefix.SetLength(kBrUlaPrefixLength);

        IgnoreError(Get<Settings>().Save<Settings::BrUlaPrefix>(mBrUlaPrefix));
        generated = true;
    }

    OT_UNUSED_VARIABLE(generated);

    LogNote("BR ULA prefix: %s (%s)", mBrUlaPrefix.ToString().AsCString(), generated ? "generated" : "loaded");

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to generate random /48 BR ULA prefix");
    }
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
void RoutingManager::GenerateNat64Prefix(void)
{
    mLocalNat64Prefix = mBrUlaPrefix;
    mLocalNat64Prefix.SetSubnetId(kNat64PrefixSubnetId);
    mLocalNat64Prefix.mPrefix.mFields.m32[2] = 0;
    mLocalNat64Prefix.SetLength(kNat64PrefixLength);

    LogInfo("Generated NAT64 prefix: %s", mLocalNat64Prefix.ToString().AsCString());
}
#endif

void RoutingManager::GenerateOnLinkPrefix(void)
{
    MeshCoP::ExtendedPanId extPanId = Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId();

    mLocalOnLinkPrefix.mPrefix.mFields.m8[0] = 0xfd;
    // Global ID: 40 most significant bits of Extended PAN ID
    memcpy(mLocalOnLinkPrefix.mPrefix.mFields.m8 + 1, extPanId.m8, 5);
    // Subnet ID: 16 least significant bits of Extended PAN ID
    memcpy(mLocalOnLinkPrefix.mPrefix.mFields.m8 + 6, extPanId.m8 + 6, 2);
    mLocalOnLinkPrefix.SetLength(kOnLinkPrefixLength);

    LogNote("Local on-link prefix: %s", mLocalOnLinkPrefix.ToString().AsCString());
}

void RoutingManager::EvaluateState(void)
{
    if (mIsEnabled && Get<Mle::MleRouter>().IsAttached() && mInfraIf.IsRunning())
    {
        Start();
    }
    else
    {
        Stop();
    }
}

void RoutingManager::Start(void)
{
    if (!mIsRunning)
    {
        LogInfo("Border Routing manager started");

        mIsRunning = true;
        UpdateDiscoveredPrefixTableOnNetDataChange();
        StartRouterSolicitationDelay();
    }
}

void RoutingManager::Stop(void)
{
    VerifyOrExit(mIsRunning);

    mLocalOmrPrefix.RemoveFromNetData();

    mFavoredDiscoveredOnLinkPrefix.Clear();

    if (mIsAdvertisingLocalOnLinkPrefix)
    {
        UnpublishExternalRoute(mLocalOnLinkPrefix);

        // Start deprecating the local on-link prefix to send a PIO
        // with zero preferred lifetime in `SendRouterAdvertisement`.
        DeprecateOnLinkPrefix();
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
    if (mIsAdvertisingLocalNat64Prefix)
    {
        UnpublishExternalRoute(mLocalNat64Prefix);
        mIsAdvertisingLocalNat64Prefix = false;
    }
#endif
    // Use empty OMR & on-link prefixes to invalidate possible advertised prefixes.
    SendRouterAdvertisement(OmrPrefixArray());

    mAdvertisedOmrPrefixes.Clear();
    mOnLinkPrefixDeprecateTimer.Stop();

    mDiscoveredPrefixTable.RemoveAllEntries();
    mDiscoveredPrefixStaleTimer.Stop();

    mRouterAdvertisementCount = 0;

    mRouterSolicitTimer.Stop();
    mRouterSolicitCount = 0;

    mRoutingPolicyTimer.Stop();

    LogInfo("Border Routing manager stopped");

    mIsRunning = false;

exit:
    return;
}

void RoutingManager::HandleReceived(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress)
{
    const Ip6::Icmp::Header *icmp6Header;

    VerifyOrExit(mIsRunning);

    icmp6Header = reinterpret_cast<const Ip6::Icmp::Header *>(aPacket.GetBytes());

    switch (icmp6Header->GetType())
    {
    case Ip6::Icmp::Header::kTypeRouterAdvert:
        HandleRouterAdvertisement(aPacket, aSrcAddress);
        break;
    case Ip6::Icmp::Header::kTypeRouterSolicit:
        HandleRouterSolicit(aPacket, aSrcAddress);
        break;
    default:
        break;
    }

exit:
    return;
}

void RoutingManager::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(IsInitialized() && IsEnabled());

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        EvaluateState();
    }

    if (mIsRunning && aEvents.Contains(kEventThreadNetdataChanged))
    {
        UpdateDiscoveredPrefixTableOnNetDataChange();
        StartRoutingPolicyEvaluationJitter(kRoutingPolicyEvaluationJitter);
    }

    if (aEvents.Contains(kEventThreadExtPanIdChanged))
    {
        if (mIsAdvertisingLocalOnLinkPrefix)
        {
            UnpublishExternalRoute(mLocalOnLinkPrefix);
            // TODO: consider deprecating/invalidating existing
            // on-link prefix
            mIsAdvertisingLocalOnLinkPrefix = false;
        }

        GenerateOnLinkPrefix();

        if (mIsRunning)
        {
            StartRoutingPolicyEvaluationJitter(kRoutingPolicyEvaluationJitter);
        }
    }

exit:
    return;
}

void RoutingManager::UpdateDiscoveredPrefixTableOnNetDataChange(void)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;
    bool                            foundDefRouteOmrPrefix = false;

    // Remove all OMR prefixes in Network Data from the
    // discovered prefix table. Also check if we have
    // an OMR prefix with default route flag.

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
    {
        if (!IsValidOmrPrefix(prefixConfig))
        {
            continue;
        }

        mDiscoveredPrefixTable.RemoveRoutePrefix(prefixConfig.GetPrefix(),
                                                 DiscoveredPrefixTable::kUnpublishFromNetData);

        if (prefixConfig.mDefaultRoute)
        {
            foundDefRouteOmrPrefix = true;
        }
    }

    // If we find an OMR prefix with default route flag, it indicates
    // that this prefix can be used with default route (routable beyond
    // infra link).
    //
    // `DiscoveredPrefixTable` will always track which routers provide
    // default route when processing received RA messages, but only
    // if we see an OMR prefix with default route flag, we allow it
    // to publish the discovered default route (as ::/0 external
    // route) in Network Data.

    mDiscoveredPrefixTable.SetAllowDefaultRouteInNetData(foundDefRouteOmrPrefix);
}

void RoutingManager::EvaluateOmrPrefix(OmrPrefixArray &aNewOmrPrefixes)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig onMeshPrefixConfig;
    OmrPrefix *                     favoredOmrEntry = nullptr;
    OmrPrefix *                     localOmrEntry   = nullptr;

    OT_ASSERT(mIsRunning);

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, onMeshPrefixConfig) == kErrorNone)
    {
        OmrPrefix *entry;

        if (!IsValidOmrPrefix(onMeshPrefixConfig))
        {
            continue;
        }

        entry = aNewOmrPrefixes.FindMatching(onMeshPrefixConfig.GetPrefix());

        if (entry != nullptr)
        {
            // Update the entry if we find the same prefix with higher
            // preference in network data

            if (onMeshPrefixConfig.GetPreference() <= entry->GetPreference())
            {
                continue;
            }

            entry->SetPreference(onMeshPrefixConfig.GetPreference());
        }
        else
        {
            entry = aNewOmrPrefixes.PushBack();

            if (entry == nullptr)
            {
                LogWarn("EvaluateOmrPrefix: Too many OMR prefixes, ignoring prefix %s",
                        onMeshPrefixConfig.GetPrefix().ToString().AsCString());
                continue;
            }

            entry->InitFrom(onMeshPrefixConfig);
        }

        if (onMeshPrefixConfig.mPreferred)
        {
            if ((favoredOmrEntry == nullptr) || (entry->IsFavoredOver(*favoredOmrEntry)))
            {
                favoredOmrEntry = entry;
            }
        }

        if (entry->GetPrefix() == mLocalOmrPrefix.GetPrefix())
        {
            localOmrEntry = entry;
        }
    }

    // Decide if we need to add or remove our local OMR prefix.

    if (favoredOmrEntry == nullptr)
    {
        LogInfo("EvaluateOmrPrefix: No preferred OMR prefix found in Thread network");

        // The `aNewOmrPrefixes` remains empty if we fail to publish
        // the local OMR prefix.
        SuccessOrExit(mLocalOmrPrefix.AddToNetData());

        if (localOmrEntry == nullptr)
        {
            localOmrEntry = aNewOmrPrefixes.PushBack();
            VerifyOrExit(localOmrEntry != nullptr);

            localOmrEntry->Init(mLocalOmrPrefix.GetPrefix(), NetworkData::kRoutePreferenceLow);
        }
    }
    else if (favoredOmrEntry == localOmrEntry)
    {
        IgnoreError(mLocalOmrPrefix.AddToNetData());
    }
    else if (mLocalOmrPrefix.IsAddedInNetData())
    {
        LogInfo("EvaluateOmrPrefix: There is already a preferred OMR prefix %s in the Thread network",
                favoredOmrEntry->ToString().AsCString());

        mLocalOmrPrefix.RemoveFromNetData();

        if (localOmrEntry != nullptr)
        {
            aNewOmrPrefixes.Remove(*localOmrEntry);
        }
    }

exit:
    return;
}

Error RoutingManager::PublishExternalRoute(const Ip6::Prefix &aPrefix, RoutePreference aRoutePreference, bool aNat64)
{
    Error                            error;
    NetworkData::ExternalRouteConfig routeConfig;

    OT_ASSERT(mIsRunning);

    routeConfig.Clear();
    routeConfig.SetPrefix(aPrefix);
    routeConfig.mStable     = true;
    routeConfig.mNat64      = aNat64;
    routeConfig.mPreference = aRoutePreference;

    error = Get<NetworkData::Publisher>().PublishExternalRoute(routeConfig);

    if (error != kErrorNone)
    {
        LogWarn("Failed to publish external route %s: %s", aPrefix.ToString().AsCString(), ErrorToString(error));
    }

    return error;
}

void RoutingManager::UnpublishExternalRoute(const Ip6::Prefix &aPrefix)
{
    VerifyOrExit(mIsRunning);
    IgnoreError(Get<NetworkData::Publisher>().UnpublishPrefix(aPrefix));

exit:
    return;
}

void RoutingManager::EvaluateOnLinkPrefix(void)
{
    VerifyOrExit(!IsRouterSolicitationInProgress());

    mDiscoveredPrefixTable.FindFavoredOnLinkPrefix(mFavoredDiscoveredOnLinkPrefix);

    if (mFavoredDiscoveredOnLinkPrefix.GetLength() == 0)
    {
        // We need to advertise our local on-link prefix since there is
        // no discovered on-link prefix.

        mOnLinkPrefixDeprecateTimer.Stop();
        VerifyOrExit(!mIsAdvertisingLocalOnLinkPrefix);

        SuccessOrExit(PublishExternalRoute(mLocalOnLinkPrefix, NetworkData::kRoutePreferenceMedium));

        mIsAdvertisingLocalOnLinkPrefix = true;
        LogInfo("Start advertising on-link prefix %s on %s", mLocalOnLinkPrefix.ToString().AsCString(),
                mInfraIf.ToString().AsCString());

        // We remove the local on-link prefix from discovered prefix
        // table, in case it was previously discovered and included in
        // the table (now as a deprecating entry). We remove it with
        // `kKeepInNetData` flag to ensure that the prefix is not
        // unpublished from network data.
        //
        // Note that `ShouldProcessPrefixInfoOption()` will also check
        // not allow the local on-link prefix to be added in the prefix
        // table while we are advertising it.

        mDiscoveredPrefixTable.RemoveOnLinkPrefix(mLocalOnLinkPrefix, DiscoveredPrefixTable::kKeepInNetData);
    }
    else
    {
        VerifyOrExit(mIsAdvertisingLocalOnLinkPrefix);

        // When an application-specific on-link prefix is received and
        // it is larger than the local prefix, we will not remove the
        // advertised local prefix. In this case, there will be two
        // on-link prefixes on the infra link. But all BRs will still
        // converge to the same smallest/favored on-link prefix and the
        // application-specific prefix is not used.

        if (!(mLocalOnLinkPrefix < mFavoredDiscoveredOnLinkPrefix))
        {
            LogInfo("EvaluateOnLinkPrefix: There is already favored on-link prefix %s on %s",
                    mFavoredDiscoveredOnLinkPrefix.ToString().AsCString(), mInfraIf.ToString().AsCString());
            DeprecateOnLinkPrefix();
        }
    }

exit:
    return;
}

void RoutingManager::HandleOnLinkPrefixDeprecateTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().HandleOnLinkPrefixDeprecateTimer();
}

void RoutingManager::HandleOnLinkPrefixDeprecateTimer(void)
{
    OT_ASSERT(!mIsAdvertisingLocalOnLinkPrefix);

    LogInfo("Local on-link prefix %s expired", mLocalOnLinkPrefix.ToString().AsCString());

    if (!mDiscoveredPrefixTable.ContainsOnLinkPrefix(mLocalOnLinkPrefix))
    {
        UnpublishExternalRoute(mLocalOnLinkPrefix);
    }
}

void RoutingManager::DeprecateOnLinkPrefix(void)
{
    OT_ASSERT(mIsAdvertisingLocalOnLinkPrefix);

    mIsAdvertisingLocalOnLinkPrefix = false;

    LogInfo("Deprecate local on-link prefix %s", mLocalOnLinkPrefix.ToString().AsCString());
    mOnLinkPrefixDeprecateTimer.StartAt(mTimeAdvertisedOnLinkPrefix,
                                        TimeMilli::SecToMsec(kDefaultOnLinkPrefixLifetime));
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
void RoutingManager::EvaluateNat64Prefix(void)
{
    OT_ASSERT(mIsRunning);

    NetworkData::Iterator            iterator = NetworkData::kIteratorInit;
    NetworkData::ExternalRouteConfig config;
    Ip6::Prefix                      smallestNat64Prefix;

    LogInfo("Evaluating NAT64 prefix");

    smallestNat64Prefix.Clear();
    while (Get<NetworkData::Leader>().GetNextExternalRoute(iterator, config) == kErrorNone)
    {
        const Ip6::Prefix &prefix = config.GetPrefix();

        if (config.mNat64 && prefix.IsValidNat64())
        {
            if (smallestNat64Prefix.GetLength() == 0 || prefix < smallestNat64Prefix)
            {
                smallestNat64Prefix = prefix;
            }
        }
    }

    if (smallestNat64Prefix.GetLength() == 0 || smallestNat64Prefix == mLocalNat64Prefix)
    {
        LogInfo("No NAT64 prefix in Network Data is smaller than the local NAT64 prefix %s",
                mLocalNat64Prefix.ToString().AsCString());

        // Advertise local NAT64 prefix.
        if (!mIsAdvertisingLocalNat64Prefix &&
            PublishExternalRoute(mLocalNat64Prefix, NetworkData::kRoutePreferenceLow, /* aNat64= */ true) == kErrorNone)
        {
            mIsAdvertisingLocalNat64Prefix = true;
        }
    }
    else if (mIsAdvertisingLocalNat64Prefix && smallestNat64Prefix < mLocalNat64Prefix)
    {
        // Withdraw local NAT64 prefix if it's not the smallest one in Network Data.
        // TODO: remove the prefix with lower preference after discovering upstream NAT64 prefix is supported
        LogNote("Withdrawing local NAT64 prefix since a smaller one %s exists.",
                smallestNat64Prefix.ToString().AsCString());

        UnpublishExternalRoute(mLocalNat64Prefix);
        mIsAdvertisingLocalNat64Prefix = false;
    }
}
#endif

// This method evaluate the routing policy depends on prefix and route
// information on Thread Network and infra link. As a result, this
// method May send RA messages on infra link and publish/unpublish
// OMR and NAT64 prefix in the Thread network.
void RoutingManager::EvaluateRoutingPolicy(void)
{
    OT_ASSERT(mIsRunning);

    OmrPrefixArray newOmrPrefixes;

    LogInfo("Evaluating routing policy");

    // 0. Evaluate on-link, OMR and NAT64 prefixes.
    EvaluateOnLinkPrefix();
    EvaluateOmrPrefix(newOmrPrefixes);
#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
    EvaluateNat64Prefix();
#endif

    // 1. Send Router Advertisement message if necessary.
    SendRouterAdvertisement(newOmrPrefixes);

    if (newOmrPrefixes.IsEmpty())
    {
        // This is the very exceptional case and happens only when we failed to publish
        // our local OMR prefix to the Thread network. We schedule the routing policy
        // timer to re-evaluate our routing policy in the future.

        LogWarn("No OMR prefix advertised! Start Routing Policy timer for future evaluation");
    }

    // 2. Schedule routing policy timer with random interval for the next Router Advertisement.
    {
        uint32_t nextSendDelay;

        nextSendDelay = Random::NonCrypto::GetUint32InRange(kMinRtrAdvInterval, kMaxRtrAdvInterval);

        if (mRouterAdvertisementCount <= kMaxInitRtrAdvertisements && nextSendDelay > kMaxInitRtrAdvInterval)
        {
            nextSendDelay = kMaxInitRtrAdvInterval;
        }

        StartRoutingPolicyEvaluationDelay(Time::SecToMsec(nextSendDelay));
    }

    // 3. Update OMR prefixes information.
    mAdvertisedOmrPrefixes = newOmrPrefixes;
}

void RoutingManager::StartRoutingPolicyEvaluationJitter(uint32_t aJitterMilli)
{
    OT_ASSERT(mIsRunning);

    StartRoutingPolicyEvaluationDelay(Random::NonCrypto::GetUint32InRange(0, aJitterMilli));
}

void RoutingManager::StartRoutingPolicyEvaluationDelay(uint32_t aDelayMilli)
{
    TimeMilli now          = TimerMilli::GetNow();
    TimeMilli evaluateTime = now + aDelayMilli;
    TimeMilli earlestTime  = mLastRouterAdvertisementSendTime + kMinDelayBetweenRtrAdvs;

    evaluateTime = OT_MAX(evaluateTime, earlestTime);

    LogInfo("Start evaluating routing policy, scheduled in %u milliseconds", evaluateTime - now);

    mRoutingPolicyTimer.FireAtIfEarlier(evaluateTime);
}

// starts sending Router Solicitations in random delay
// between 0 and kMaxRtrSolicitationDelay.
void RoutingManager::StartRouterSolicitationDelay(void)
{
    uint32_t randomDelay;

    VerifyOrExit(!IsRouterSolicitationInProgress());

    OT_ASSERT(mRouterSolicitCount == 0);

    static_assert(kMaxRtrSolicitationDelay > 0, "invalid maximum Router Solicitation delay");
    randomDelay = Random::NonCrypto::GetUint32InRange(0, Time::SecToMsec(kMaxRtrSolicitationDelay));

    LogInfo("Start Router Solicitation, scheduled in %u milliseconds", randomDelay);
    mTimeRouterSolicitStart = TimerMilli::GetNow();
    mRouterSolicitTimer.Start(randomDelay);

exit:
    return;
}

bool RoutingManager::IsRouterSolicitationInProgress(void) const
{
    return mRouterSolicitTimer.IsRunning() || mRouterSolicitCount > 0;
}

Error RoutingManager::SendRouterSolicitation(void)
{
    Ip6::Address                  destAddress;
    Ip6::Nd::RouterSolicitMessage routerSolicit;
    InfraIf::Icmp6Packet          packet;

    OT_ASSERT(IsInitialized());

    packet.InitFrom(routerSolicit);
    destAddress.SetToLinkLocalAllRoutersMulticast();

    return mInfraIf.Send(packet, destAddress);
}

void RoutingManager::SendRouterAdvertisement(const OmrPrefixArray &aNewOmrPrefixes)
{
    uint8_t                      buffer[kMaxRouterAdvMessageLength];
    Ip6::Nd::RouterAdvertMessage raMsg(mRouterAdvertHeader, buffer);

    // Append PIO for local on-link prefix. Ensure it is either being
    // advertised or deprecated.

    if (mIsAdvertisingLocalOnLinkPrefix || mOnLinkPrefixDeprecateTimer.IsRunning())
    {
        uint32_t validLifetime     = kDefaultOnLinkPrefixLifetime;
        uint32_t preferredLifetime = kDefaultOnLinkPrefixLifetime;

        if (mOnLinkPrefixDeprecateTimer.IsRunning())
        {
            validLifetime     = TimeMilli::MsecToSec(mOnLinkPrefixDeprecateTimer.GetFireTime() - TimerMilli::GetNow());
            preferredLifetime = 0;
        }

        SuccessOrAssert(raMsg.AppendPrefixInfoOption(mLocalOnLinkPrefix, validLifetime, preferredLifetime));

        if (mIsAdvertisingLocalOnLinkPrefix)
        {
            mTimeAdvertisedOnLinkPrefix = TimerMilli::GetNow();
        }

        LogInfo("RouterAdvert: Added PIO for %s (valid=%u, preferred=%u)", mLocalOnLinkPrefix.ToString().AsCString(),
                validLifetime, preferredLifetime);
    }

    // Invalidate previously advertised OMR prefixes if they are no
    // longer in the new OMR prefix array.

    for (const OmrPrefix &omrPrefix : mAdvertisedOmrPrefixes)
    {
        if (!aNewOmrPrefixes.ContainsMatching(omrPrefix.GetPrefix()))
        {
            SuccessOrAssert(
                raMsg.AppendRouteInfoOption(omrPrefix.GetPrefix(), /* aRouteLifetime */ 0, mRouteInfoOptionPreference));

            LogInfo("RouterAdvert: Added RIO for %s (lifetime=0)", omrPrefix.GetPrefix().ToString().AsCString());
        }
    }

    for (const OmrPrefix &omrPrefix : aNewOmrPrefixes)
    {
        SuccessOrAssert(
            raMsg.AppendRouteInfoOption(omrPrefix.GetPrefix(), kDefaultOmrPrefixLifetime, mRouteInfoOptionPreference));

        LogInfo("RouterAdvert: Added RIO for %s (lifetime=%u)", omrPrefix.GetPrefix().ToString().AsCString(),
                kDefaultOmrPrefixLifetime);
    }

    if (raMsg.ContainsAnyOptions())
    {
        Error        error;
        Ip6::Address destAddress;

        ++mRouterAdvertisementCount;

        destAddress.SetToLinkLocalAllNodesMulticast();

        error = mInfraIf.Send(raMsg.GetAsPacket(), destAddress);

        if (error == kErrorNone)
        {
            mLastRouterAdvertisementSendTime = TimerMilli::GetNow();
            LogInfo("Sent Router Advertisement on %s", mInfraIf.ToString().AsCString());
            DumpDebg("[BR-CERT] direction=send | type=RA |", raMsg.GetAsPacket().GetBytes(),
                     raMsg.GetAsPacket().GetLength());
        }
        else
        {
            LogWarn("Failed to send Router Advertisement on %s: %s", mInfraIf.ToString().AsCString(),
                    ErrorToString(error));
        }
    }
}

bool RoutingManager::IsReceivdRouterAdvertFromManager(const Ip6::Nd::RouterAdvertMessage &aRaMessage) const
{
    // Determines whether or not a received RA message was prepared by
    // by `RoutingManager` itself.

    bool        isFromManager = false;
    uint16_t    rioCount      = 0;
    Ip6::Prefix prefix;

    VerifyOrExit(aRaMessage.ContainsAnyOptions());

    for (const Ip6::Nd::Option &option : aRaMessage)
    {
        switch (option.GetType())
        {
        case Ip6::Nd::Option::kTypePrefixInfo:
        {
            // PIO should match `mLocalOnLinkPrefix`.

            const Ip6::Nd::PrefixInfoOption &pio = static_cast<const Ip6::Nd::PrefixInfoOption &>(option);

            VerifyOrExit(pio.IsValid());
            pio.GetPrefix(prefix);

            VerifyOrExit(prefix == mLocalOnLinkPrefix);
            break;
        }

        case Ip6::Nd::Option::kTypeRouteInfo:
        {
            // RIO (with non-zero lifetime) should match entries from
            // `mAdvertisedOmrPrefixes`. We keep track of the number
            // of matched RIOs and check after the loop ends that all
            // entries were seen.

            const Ip6::Nd::RouteInfoOption &rio = static_cast<const Ip6::Nd::RouteInfoOption &>(option);

            VerifyOrExit(rio.IsValid());
            rio.GetPrefix(prefix);

            if (rio.GetRouteLifetime() != 0)
            {
                VerifyOrExit(mAdvertisedOmrPrefixes.ContainsMatching(prefix));
                rioCount++;
            }

            break;
        }

        default:
            ExitNow();
        }
    }

    VerifyOrExit(rioCount == mAdvertisedOmrPrefixes.GetLength());

    isFromManager = true;

exit:
    return isFromManager;
}

bool RoutingManager::IsValidBrUlaPrefix(const Ip6::Prefix &aBrUlaPrefix)
{
    return aBrUlaPrefix.mLength == kBrUlaPrefixLength && aBrUlaPrefix.mPrefix.mFields.m8[0] == 0xfd;
}

bool RoutingManager::IsValidOmrPrefix(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig)
{
    return IsValidOmrPrefix(aOnMeshPrefixConfig.GetPrefix()) && aOnMeshPrefixConfig.mOnMesh &&
           aOnMeshPrefixConfig.mSlaac && aOnMeshPrefixConfig.mStable && !aOnMeshPrefixConfig.mDp;
}

bool RoutingManager::IsValidOmrPrefix(const Ip6::Prefix &aOmrPrefix)
{
    // Accept ULA prefix with length of 64 bits and GUA prefix.
    return (aOmrPrefix.mLength == kOmrPrefixLength && aOmrPrefix.mPrefix.mFields.m8[0] == 0xfd) ||
           (aOmrPrefix.mLength >= 3 && (aOmrPrefix.GetBytes()[0] & 0xE0) == 0x20);
}

bool RoutingManager::IsValidOnLinkPrefix(const Ip6::Nd::PrefixInfoOption &aPio)
{
    Ip6::Prefix prefix;

    aPio.GetPrefix(prefix);

    return IsValidOnLinkPrefix(prefix) && aPio.IsOnLinkFlagSet() && aPio.IsAutoAddrConfigFlagSet();
}

bool RoutingManager::IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix)
{
    return aOnLinkPrefix.IsValid() && (aOnLinkPrefix.GetLength() > 0) && !aOnLinkPrefix.IsLinkLocal() &&
           !aOnLinkPrefix.IsMulticast();
}

void RoutingManager::HandleRouterSolicitTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().HandleRouterSolicitTimer();
}

void RoutingManager::HandleRouterSolicitTimer(void)
{
    LogInfo("Router solicitation times out");

    if (mRouterSolicitCount < kMaxRtrSolicitations)
    {
        uint32_t nextSolicitationDelay;
        Error    error;

        error = SendRouterSolicitation();

        if (error == kErrorNone)
        {
            LogDebg("Successfully sent %uth Router Solicitation", mRouterSolicitCount);
            ++mRouterSolicitCount;
            nextSolicitationDelay =
                (mRouterSolicitCount == kMaxRtrSolicitations) ? kMaxRtrSolicitationDelay : kRtrSolicitationInterval;
        }
        else
        {
            LogCrit("Failed to send %uth Router Solicitation: %s", mRouterSolicitCount, ErrorToString(error));

            // It's unexpected that RS will fail and we will retry sending RS messages in 60 seconds.
            // Notice that `mRouterSolicitCount` is not incremented for failed RS and thus we will
            // not start configuring on-link prefixes before `kMaxRtrSolicitations` successful RS
            // messages have been sent.
            nextSolicitationDelay = kRtrSolicitationRetryDelay;
            mRouterSolicitCount   = 0;
        }

        LogDebg("Router solicitation timer scheduled in %u seconds", nextSolicitationDelay);
        mRouterSolicitTimer.Start(Time::SecToMsec(nextSolicitationDelay));
    }
    else
    {
        // Remove route prefixes and deprecate on-link prefixes that
        // are not refreshed during Router Solicitation.
        mDiscoveredPrefixTable.RemoveOrDeprecateOldEntries(mTimeRouterSolicitStart);

        // Invalidate the learned RA message if it is not refreshed during Router Solicitation.
        if (mTimeRouterAdvMessageLastUpdate <= mTimeRouterSolicitStart)
        {
            UpdateRouterAdvertHeader(/* aRouterAdvertMessage */ nullptr);
        }

        mRouterSolicitCount = 0;

        // Re-evaluate our routing policy and send Router Advertisement if necessary.
        StartRoutingPolicyEvaluationDelay(/* aDelayJitter */ 0);
    }
}

void RoutingManager::HandleDiscoveredPrefixStaleTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().HandleDiscoveredPrefixStaleTimer();
}

void RoutingManager::HandleDiscoveredPrefixStaleTimer(void)
{
    LogInfo("Stale On-Link or OMR Prefixes or RA messages are detected");
    StartRouterSolicitationDelay();
}

void RoutingManager::HandleRoutingPolicyTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().EvaluateRoutingPolicy();
}

void RoutingManager::HandleRouterSolicit(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress)
{
    OT_UNUSED_VARIABLE(aPacket);
    OT_UNUSED_VARIABLE(aSrcAddress);

    LogInfo("Received Router Solicitation from %s on %s", aSrcAddress.ToString().AsCString(),
            mInfraIf.ToString().AsCString());

    // Schedule routing policy evaluation with random jitter to respond with Router Advertisement.
    StartRoutingPolicyEvaluationJitter(kRaReplyJitter);
}

void RoutingManager::HandleRouterAdvertisement(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress)
{
    Ip6::Nd::RouterAdvertMessage routerAdvMessage(aPacket);

    OT_ASSERT(mIsRunning);

    VerifyOrExit(routerAdvMessage.IsValid());

    LogInfo("Received Router Advertisement from %s on %s", aSrcAddress.ToString().AsCString(),
            mInfraIf.ToString().AsCString());
    DumpDebg("[BR-CERT] direction=recv | type=RA |", aPacket.GetBytes(), aPacket.GetLength());

    mDiscoveredPrefixTable.ProcessRouterAdvertMessage(routerAdvMessage, aSrcAddress);

    // Remember the header and parameters of RA messages which are
    // initiated from the infra interface.
    if (mInfraIf.HasAddress(aSrcAddress))
    {
        UpdateRouterAdvertHeader(&routerAdvMessage);
    }

exit:
    return;
}

bool RoutingManager::ShouldProcessPrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio, const Ip6::Prefix &aPrefix)
{
    // Indicate whether to process or skip a given prefix
    // from a PIO (from received RA message).

    bool shouldProcess = false;

    VerifyOrExit(mIsRunning);

    if (!IsValidOnLinkPrefix(aPio))
    {
        LogInfo("Ignore invalid on-link prefix in PIO: %s", aPrefix.ToString().AsCString());
        ExitNow();
    }

    if (mIsAdvertisingLocalOnLinkPrefix)
    {
        VerifyOrExit(aPrefix != mLocalOnLinkPrefix);
    }

    shouldProcess = true;

exit:
    return shouldProcess;
}

bool RoutingManager::ShouldProcessRouteInfoOption(const Ip6::Nd::RouteInfoOption &aRio, const Ip6::Prefix &aPrefix)
{
    // Indicate whether to process or skip a given prefix
    // from a RIO (from received RA message).

    OT_UNUSED_VARIABLE(aRio);

    bool shouldProcess = false;

    VerifyOrExit(mIsRunning);

    if (aPrefix.GetLength() == 0)
    {
        // Always process default route ::/0 prefix.
        ExitNow(shouldProcess = true);
    }

    if (!IsValidOmrPrefix(aPrefix))
    {
        LogInfo("Ignore RIO prefix %s since not a valid OMR prefix", aPrefix.ToString().AsCString());
        ExitNow();
    }

    VerifyOrExit(mLocalOmrPrefix.GetPrefix() != aPrefix);

    // Ignore OMR prefixes advertised by ourselves or in current Thread Network Data.
    // The `mAdvertisedOmrPrefixes` and the OMR prefix set in Network Data should eventually
    // be equal, but there is time that they are not synchronized immediately:
    // 1. Network Data could contain more OMR prefixes than `mAdvertisedOmrPrefixes` because
    //    we added random delay before Evaluating routing policy when Network Data is changed.
    // 2. `mAdvertisedOmrPrefixes` could contain more OMR prefixes than Network Data because
    //    it takes time to sync a new OMR prefix into Network Data (multicast loopback RA
    //    messages are usually faster than Thread Network Data propagation).
    // They are the reasons why we need both the checks.

    VerifyOrExit(!mAdvertisedOmrPrefixes.ContainsMatching(aPrefix));
    VerifyOrExit(!Get<RoutingManager>().NetworkDataContainsOmrPrefix(aPrefix));

    shouldProcess = true;

exit:
    return shouldProcess;
}

void RoutingManager::HandleDiscoveredPrefixTableChanged(void)
{
    // This is a callback from `mDiscoveredPrefixTable` indicating that
    // there has been a change in the table. If the favored on-link
    // prefix has changed, we trigger a re-evaluation of the routing
    // policy.

    Ip6::Prefix newFavoredPrefix;

    VerifyOrExit(mIsRunning);

    ResetDiscoveredPrefixStaleTimer();

    mDiscoveredPrefixTable.FindFavoredOnLinkPrefix(newFavoredPrefix);

    if (newFavoredPrefix != mFavoredDiscoveredOnLinkPrefix)
    {
        StartRoutingPolicyEvaluationJitter(kRoutingPolicyEvaluationJitter);
    }

exit:
    return;
}

bool RoutingManager::NetworkDataContainsOmrPrefix(const Ip6::Prefix &aPrefix) const
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig onMeshPrefixConfig;
    bool                            contain = false;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, onMeshPrefixConfig) == OT_ERROR_NONE)
    {
        if (IsValidOmrPrefix(onMeshPrefixConfig) && onMeshPrefixConfig.GetPrefix() == aPrefix)
        {
            contain = true;
            break;
        }
    }

    return contain;
}

void RoutingManager::UpdateRouterAdvertHeader(const Ip6::Nd::RouterAdvertMessage *aRouterAdvertMessage)
{
    // Updates the `mRouterAdvertHeader` from the given RA message.

    Ip6::Nd::RouterAdvertMessage::Header oldHeader;

    if (aRouterAdvertMessage != nullptr)
    {
        // We skip and do not update RA header if the received RA message
        // was not prepared and sent by `RoutingManager` itself.

        VerifyOrExit(!IsReceivdRouterAdvertFromManager(*aRouterAdvertMessage));
    }

    oldHeader                       = mRouterAdvertHeader;
    mTimeRouterAdvMessageLastUpdate = TimerMilli::GetNow();

    if (aRouterAdvertMessage == nullptr || aRouterAdvertMessage->GetHeader().GetRouterLifetime() == 0)
    {
        mRouterAdvertHeader.SetToDefault();
        mLearntRouterAdvMessageFromHost = false;
    }
    else
    {
        // The checksum is set to zero in `mRouterAdvertHeader`
        // which indicates to platform that it needs to do the
        // calculation and update it.

        mRouterAdvertHeader = aRouterAdvertMessage->GetHeader();
        mRouterAdvertHeader.SetChecksum(0);
        mLearntRouterAdvMessageFromHost = true;
    }

    ResetDiscoveredPrefixStaleTimer();

    if (mRouterAdvertHeader != oldHeader)
    {
        // If there was a change to the header, start timer to
        // reevaluate routing policy and send RA message with new
        // header.

        StartRoutingPolicyEvaluationJitter(kRoutingPolicyEvaluationJitter);
    }

exit:
    return;
}

void RoutingManager::ResetDiscoveredPrefixStaleTimer(void)
{
    TimeMilli now = TimerMilli::GetNow();
    TimeMilli nextStaleTime;

    OT_ASSERT(mIsRunning);

    // The stale timer triggers sending RS to check the state of
    // discovered prefixes and host RA messages.

    nextStaleTime = mDiscoveredPrefixTable.CalculateNextStaleTime(now);

    // Check for stale Router Advertisement Message if learnt from Host.
    if (mLearntRouterAdvMessageFromHost)
    {
        TimeMilli raStaleTime = OT_MAX(now, mTimeRouterAdvMessageLastUpdate + Time::SecToMsec(kRtrAdvStaleTime));

        nextStaleTime = OT_MIN(nextStaleTime, raStaleTime);
    }

    if (nextStaleTime == now.GetDistantFuture())
    {
        if (mDiscoveredPrefixStaleTimer.IsRunning())
        {
            LogDebg("Prefix stale timer stopped");
        }

        mDiscoveredPrefixStaleTimer.Stop();
    }
    else
    {
        mDiscoveredPrefixStaleTimer.FireAt(nextStaleTime);
        LogDebg("Prefix stale timer scheduled in %lu ms", nextStaleTime - now);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// DiscoveredPrefixTable

RoutingManager::DiscoveredPrefixTable::DiscoveredPrefixTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance, HandleTimer)
    , mSignalTask(aInstance, HandleSignalTask)
    , mAllowDefaultRouteInNetData(false)
{
}

void RoutingManager::DiscoveredPrefixTable::ProcessRouterAdvertMessage(const Ip6::Nd::RouterAdvertMessage &aRaMessage,
                                                                       const Ip6::Address &                aSrcAddress)
{
    // Process a received RA message and update the prefix table.

    Router *router = mRouters.FindMatching(aSrcAddress);

    if (router == nullptr)
    {
        router = mRouters.PushBack();

        if (router == nullptr)
        {
            LogWarn("Received RA from too many routers, ignore RA from %s", aSrcAddress.ToString().AsCString());
            ExitNow();
        }

        router->mAddress = aSrcAddress;
        router->mEntries.Clear();
    }

    // RA message can indicate router provides default route in the RA
    // message header and can also include an RIO for `::/0`. When
    // processing an RA message, the preference and lifetime values
    // in a `::/0` RIO override the preference and lifetime values in
    // the RA header (per RFC 4191 section 3.1).

    ProcessDefaultRoute(aRaMessage.GetHeader(), *router);

    for (const Ip6::Nd::Option &option : aRaMessage)
    {
        switch (option.GetType())
        {
        case Ip6::Nd::Option::kTypePrefixInfo:
            ProcessPrefixInfoOption(static_cast<const Ip6::Nd::PrefixInfoOption &>(option), *router);
            break;

        case Ip6::Nd::Option::kTypeRouteInfo:
            ProcessRouteInfoOption(static_cast<const Ip6::Nd::RouteInfoOption &>(option), *router);
            break;

        default:
            break;
        }
    }

    RemoveRoutersWithNoEntries();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::ProcessDefaultRoute(const Ip6::Nd::RouterAdvertMessage::Header &aRaHeader,
                                                                Router &                                    aRouter)
{
    Entry *     entry;
    Ip6::Prefix prefix;

    prefix.Clear();
    entry = aRouter.mEntries.FindMatching(Entry::Matcher(prefix, Entry::kTypeRoute));

    if (entry == nullptr)
    {
        VerifyOrExit(aRaHeader.GetRouterLifetime() != 0);

        entry = AllocateEntry();

        if (entry == nullptr)
        {
            LogWarn("Discovered too many prefixes, ignore default route from RA header");
            ExitNow();
        }

        entry->InitFrom(aRaHeader);
        aRouter.mEntries.Push(*entry);
    }
    else
    {
        entry->InitFrom(aRaHeader);
    }

    UpdateNetworkDataOnChangeTo(*entry);
    mTimer.FireAtIfEarlier(entry->GetExpireTime());
    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::ProcessPrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio,
                                                                    Router &                         aRouter)
{
    Ip6::Prefix prefix;
    Entry *     entry;

    VerifyOrExit(aPio.IsValid());
    aPio.GetPrefix(prefix);

    VerifyOrExit(Get<RoutingManager>().ShouldProcessPrefixInfoOption(aPio, prefix));

    LogInfo("Processing PIO (%s, %u seconds)", prefix.ToString().AsCString(), aPio.GetValidLifetime());

    entry = aRouter.mEntries.FindMatching(Entry::Matcher(prefix, Entry::kTypeOnLink));

    if (entry == nullptr)
    {
        VerifyOrExit(aPio.GetValidLifetime() != 0);

        entry = AllocateEntry();

        if (entry == nullptr)
        {
            LogWarn("Discovered too many prefixes, ignore on-link prefix %s", prefix.ToString().AsCString());
            ExitNow();
        }

        entry->InitFrom(aPio);
        aRouter.mEntries.Push(*entry);
    }
    else
    {
        Entry newEntry;

        newEntry.InitFrom(aPio);
        entry->AdoptValidAndPreferredLiftimesFrom(newEntry);
    }

    UpdateNetworkDataOnChangeTo(*entry);
    mTimer.FireAtIfEarlier(entry->GetExpireTime());
    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::ProcessRouteInfoOption(const Ip6::Nd::RouteInfoOption &aRio,
                                                                   Router &                        aRouter)
{
    Ip6::Prefix prefix;
    Entry *     entry;

    VerifyOrExit(aRio.IsValid());
    aRio.GetPrefix(prefix);

    VerifyOrExit(Get<RoutingManager>().ShouldProcessRouteInfoOption(aRio, prefix));

    LogInfo("Processing RIO (%s, %u seconds)", prefix.ToString().AsCString(), aRio.GetRouteLifetime());

    entry = aRouter.mEntries.FindMatching(Entry::Matcher(prefix, Entry::kTypeRoute));

    if (entry == nullptr)
    {
        VerifyOrExit(aRio.GetRouteLifetime() != 0);

        entry = AllocateEntry();

        if (entry == nullptr)
        {
            LogWarn("Discovered too many prefixes, ignore route prefix %s", prefix.ToString().AsCString());
            ExitNow();
        }

        entry->InitFrom(aRio);
        aRouter.mEntries.Push(*entry);
    }
    else
    {
        entry->InitFrom(aRio);
    }

    UpdateNetworkDataOnChangeTo(*entry);
    mTimer.FireAtIfEarlier(entry->GetExpireTime());
    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::SetAllowDefaultRouteInNetData(bool aAllow)
{
    Entry *     favoredEntry;
    Ip6::Prefix prefix;

    VerifyOrExit(aAllow != mAllowDefaultRouteInNetData);

    LogInfo("Allow default route in netdata: %s -> %s", ToYesNo(mAllowDefaultRouteInNetData), ToYesNo(aAllow));

    mAllowDefaultRouteInNetData = aAllow;

    prefix.Clear();
    favoredEntry = FindFavoredEntryToPublish(prefix);
    VerifyOrExit(favoredEntry != nullptr);

    if (mAllowDefaultRouteInNetData)
    {
        PublishEntry(*favoredEntry);
    }
    else
    {
        UnpublishEntry(*favoredEntry);
    }

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::FindFavoredOnLinkPrefix(Ip6::Prefix &aPrefix) const
{
    // Find the smallest preferred on-link prefix entry in the table
    // and return it in `aPrefix`. If there is none, `aPrefix` is
    // cleared (prefix length is set to zero).

    aPrefix.Clear();

    for (const Router &router : mRouters)
    {
        for (const Entry &entry : router.mEntries)
        {
            if (!entry.IsOnLinkPrefix() || entry.IsDeprecated())
            {
                continue;
            }

            if ((aPrefix.GetLength() == 0) || (entry.GetPrefix() < aPrefix))
            {
                aPrefix = entry.GetPrefix();
            }
        }
    }
}

bool RoutingManager::DiscoveredPrefixTable::ContainsOnLinkPrefix(const Ip6::Prefix &aPrefix) const
{
    return ContainsPrefix(Entry::Matcher(aPrefix, Entry::kTypeOnLink));
}

bool RoutingManager::DiscoveredPrefixTable::ContainsRoutePrefix(const Ip6::Prefix &aPrefix) const
{
    return ContainsPrefix(Entry::Matcher(aPrefix, Entry::kTypeRoute));
}

bool RoutingManager::DiscoveredPrefixTable::ContainsPrefix(const Entry::Matcher &aMatcher) const
{
    bool contains = false;

    for (const Router &router : mRouters)
    {
        if (router.mEntries.ContainsMatching(aMatcher))
        {
            contains = true;
            break;
        }
    }

    return contains;
}

void RoutingManager::DiscoveredPrefixTable::RemoveOnLinkPrefix(const Ip6::Prefix &aPrefix, NetDataMode aNetDataMode)
{
    RemovePrefix(Entry::Matcher(aPrefix, Entry::kTypeOnLink), aNetDataMode);
}

void RoutingManager::DiscoveredPrefixTable::RemoveRoutePrefix(const Ip6::Prefix &aPrefix, NetDataMode aNetDataMode)
{
    RemovePrefix(Entry::Matcher(aPrefix, Entry::kTypeRoute), aNetDataMode);
}

void RoutingManager::DiscoveredPrefixTable::RemovePrefix(const Entry::Matcher &aMatcher, NetDataMode aNetDataMode)
{
    // Removes all entries matching a given prefix from the table.
    // `aNetDataMode` specifies behavior when a match is found and
    // removed. It indicates whether or not to unpublish it from
    // Network Data.

    LinkedList<Entry> removedEntries;

    for (Router &router : mRouters)
    {
        router.mEntries.RemoveAllMatching(aMatcher, removedEntries);
    }

    VerifyOrExit(!removedEntries.IsEmpty());

    if (aNetDataMode == kUnpublishFromNetData)
    {
        UnpublishEntry(*removedEntries.GetHead());
    }

    FreeEntries(removedEntries);
    RemoveRoutersWithNoEntries();

    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::RemoveAllEntries(void)
{
    // Remove all entries from the table and unpublish them
    // from Network Data.

    for (Router &router : mRouters)
    {
        Entry *entry;

        while ((entry = router.mEntries.Pop()) != nullptr)
        {
            UnpublishEntry(*entry);
            FreeEntry(*entry);
            SignalTableChanged();
        }
    }

    RemoveRoutersWithNoEntries();
    mTimer.Stop();
}

void RoutingManager::DiscoveredPrefixTable::RemoveOrDeprecateOldEntries(TimeMilli aTimeThreshold)
{
    // Remove route prefix entries and deprecate on-link entries in
    // the table that are old (not updated since `aTimeThreshold`).

    for (Router &router : mRouters)
    {
        for (Entry &entry : router.mEntries)
        {
            if (entry.GetLastUpdateTime() <= aTimeThreshold)
            {
                if (entry.IsOnLinkPrefix())
                {
                    entry.ClearPreferredLifetime();
                }
                else
                {
                    entry.ClearValidLifetime();
                }

                SignalTableChanged();
            }
        }
    }

    RemoveExpiredEntries();
}

TimeMilli RoutingManager::DiscoveredPrefixTable::CalculateNextStaleTime(TimeMilli aNow) const
{
    TimeMilli onLinkStaleTime = aNow;
    TimeMilli routeStaleTime  = aNow.GetDistantFuture();
    bool      foundOnLink     = false;

    // For on-link prefixes, we consider stale time as when all on-link
    // prefixes become stale (the latest stale time) but for route
    // prefixes we consider the earliest stale time.

    for (const Router &router : mRouters)
    {
        for (const Entry &entry : router.mEntries)
        {
            TimeMilli entryStaleTime = OT_MAX(aNow, entry.GetStaleTime());

            if (entry.IsOnLinkPrefix() && !entry.IsDeprecated())
            {
                onLinkStaleTime = OT_MAX(onLinkStaleTime, entryStaleTime);
                foundOnLink     = true;
            }

            if (!entry.IsOnLinkPrefix())
            {
                routeStaleTime = OT_MIN(routeStaleTime, entryStaleTime);
            }
        }
    }

    return foundOnLink ? OT_MIN(onLinkStaleTime, routeStaleTime) : routeStaleTime;
}

void RoutingManager::DiscoveredPrefixTable::RemoveRoutersWithNoEntries(void)
{
    mRouters.RemoveAllMatching(Router::kContainsNoEntries);
}

void RoutingManager::DiscoveredPrefixTable::FreeEntries(LinkedList<Entry> &aEntries)
{
    // Frees all entries in the given list `aEntries` (put them back
    // in the entry pool).

    Entry *entry;

    while ((entry = aEntries.Pop()) != nullptr)
    {
        FreeEntry(*entry);
    }
}

RoutingManager::DiscoveredPrefixTable::Entry *RoutingManager::DiscoveredPrefixTable::FindFavoredEntryToPublish(
    const Ip6::Prefix &aPrefix)
{
    // Finds the favored entry matching a given `aPrefix` in the table
    // to publish in the Network Data. We can have multiple entries
    // in the table matching the same `aPrefix` from different
    // routers and potentially with different preference values. We
    // select the one with the highest preference as the favored
    // entry to publish.

    Entry *favoredEntry = nullptr;

    for (Router &router : mRouters)
    {
        for (Entry &entry : router.mEntries)
        {
            if (entry.GetPrefix() != aPrefix)
            {
                continue;
            }

            if ((favoredEntry == nullptr) || (entry.GetPreference() > favoredEntry->GetPreference()))
            {
                favoredEntry = &entry;
            }
        }
    }

    return favoredEntry;
}

void RoutingManager::DiscoveredPrefixTable::UpdateNetworkDataOnChangeTo(Entry &aEntry)
{
    // Updates Network Data when there is a change to `aEntry` which
    // can be a newly added entry or an existing entry that is
    // modified due to processing of a received RA message.

    Entry *favoredEntry;

    if (aEntry.GetPrefix().GetLength() == 0)
    {
        // If the change is to default route ::/0 prefix, make sure we
        // are allowed to publish default route in Network Data.

        VerifyOrExit(mAllowDefaultRouteInNetData);
    }

    favoredEntry = FindFavoredEntryToPublish(aEntry.GetPrefix());

    OT_ASSERT(favoredEntry != nullptr);
    PublishEntry(*favoredEntry);

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::PublishEntry(const Entry &aEntry)
{
    IgnoreError(Get<RoutingManager>().PublishExternalRoute(aEntry.GetPrefix(), aEntry.GetPreference()));
}

void RoutingManager::DiscoveredPrefixTable::UnpublishEntry(const Entry &aEntry)
{
    Get<RoutingManager>().UnpublishExternalRoute(aEntry.GetPrefix());
}

void RoutingManager::DiscoveredPrefixTable::HandleTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().mDiscoveredPrefixTable.HandleTimer();
}

void RoutingManager::DiscoveredPrefixTable::HandleTimer(void)
{
    RemoveExpiredEntries();
}

void RoutingManager::DiscoveredPrefixTable::RemoveExpiredEntries(void)
{
    TimeMilli         now            = TimerMilli::GetNow();
    TimeMilli         nextExpireTime = now.GetDistantFuture();
    LinkedList<Entry> expiredEntries;

    for (Router &router : mRouters)
    {
        router.mEntries.RemoveAllMatching(Entry::ExpirationChecker(now), expiredEntries);
    }

    RemoveRoutersWithNoEntries();

    // Determine if we need to publish/unpublish any prefixes in
    // the Network Data.

    for (const Entry &expiredEntry : expiredEntries)
    {
        Entry *favoredEntry = FindFavoredEntryToPublish(expiredEntry.GetPrefix());

        if (favoredEntry == nullptr)
        {
            UnpublishEntry(expiredEntry);
        }
        else
        {
            PublishEntry(*favoredEntry);
        }
    }

    if (!expiredEntries.IsEmpty())
    {
        SignalTableChanged();
    }

    FreeEntries(expiredEntries);

    // Determine the next expire time and schedule timer.

    for (const Router &router : mRouters)
    {
        for (const Entry &entry : router.mEntries)
        {
            nextExpireTime = OT_MIN(nextExpireTime, entry.GetExpireTime());
        }
    }

    if (nextExpireTime != now.GetDistantFuture())
    {
        mTimer.FireAt(nextExpireTime);
    }
}

void RoutingManager::DiscoveredPrefixTable::SignalTableChanged(void)
{
    mSignalTask.Post();
}

void RoutingManager::DiscoveredPrefixTable::HandleSignalTask(Tasklet &aTasklet)
{
    aTasklet.Get<RoutingManager>().HandleDiscoveredPrefixTableChanged();
}

void RoutingManager::DiscoveredPrefixTable::InitIterator(PrefixTableIterator &aIterator) const
{
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    iterator.SetInitTime();
    iterator.SetRouter(mRouters.Front());
    iterator.SetEntry(mRouters.IsEmpty() ? nullptr : mRouters[0].mEntries.GetHead());
}

Error RoutingManager::DiscoveredPrefixTable::GetNextEntry(PrefixTableIterator &aIterator,
                                                          PrefixTableEntry &   aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    VerifyOrExit(iterator.GetRouter() != nullptr, error = kErrorNotFound);
    OT_ASSERT(iterator.GetEntry() != nullptr);

    aEntry.mRouterAddress       = iterator.GetRouter()->mAddress;
    aEntry.mPrefix              = iterator.GetEntry()->GetPrefix();
    aEntry.mIsOnLink            = iterator.GetEntry()->IsOnLinkPrefix();
    aEntry.mMsecSinceLastUpdate = iterator.GetInitTime() - iterator.GetEntry()->GetLastUpdateTime();
    aEntry.mValidLifetime       = iterator.GetEntry()->GetValidLifetime();
    aEntry.mPreferredLifetime   = aEntry.mIsOnLink ? iterator.GetEntry()->GetPreferredLifetime() : 0;
    aEntry.mRoutePreference =
        static_cast<otRoutePreference>(aEntry.mIsOnLink ? 0 : iterator.GetEntry()->GetRoutePreference());

    // Advance the iterator
    iterator.SetEntry(iterator.GetEntry()->GetNext());

    if (iterator.GetEntry() == nullptr)
    {
        if (iterator.GetRouter() != mRouters.Back())
        {
            iterator.SetRouter(iterator.GetRouter() + 1);
            iterator.SetEntry(iterator.GetRouter()->mEntries.GetHead());
        }
        else
        {
            iterator.SetRouter(nullptr);
        }
    }

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// DiscoveredPrefixTable::Entry

void RoutingManager::DiscoveredPrefixTable::Entry::InitFrom(const Ip6::Nd::RouterAdvertMessage::Header &aRaHeader)
{
    Clear();
    mType                    = kTypeRoute;
    mValidLifetime           = aRaHeader.GetRouterLifetime();
    mShared.mRoutePreference = aRaHeader.GetDefaultRouterPreference();
    mLastUpdateTime          = TimerMilli::GetNow();
}

void RoutingManager::DiscoveredPrefixTable::Entry::InitFrom(const Ip6::Nd::PrefixInfoOption &aPio)
{
    Clear();
    aPio.GetPrefix(mPrefix);
    mType                      = kTypeOnLink;
    mValidLifetime             = aPio.GetValidLifetime();
    mShared.mPreferredLifetime = aPio.GetPreferredLifetime();
    mLastUpdateTime            = TimerMilli::GetNow();
}

void RoutingManager::DiscoveredPrefixTable::Entry::InitFrom(const Ip6::Nd::RouteInfoOption &aRio)
{
    Clear();
    aRio.GetPrefix(mPrefix);
    mType                    = kTypeRoute;
    mValidLifetime           = aRio.GetRouteLifetime();
    mShared.mRoutePreference = aRio.GetPreference();
    mLastUpdateTime          = TimerMilli::GetNow();
}

bool RoutingManager::DiscoveredPrefixTable::Entry::operator==(const Entry &aOther) const
{
    return (mType == aOther.mType) && (mPrefix == aOther.mPrefix);
}

bool RoutingManager::DiscoveredPrefixTable::Entry::Matches(const Matcher &aMatcher) const
{
    return (mType == aMatcher.mType) && (mPrefix == aMatcher.mPrefix);
}

bool RoutingManager::DiscoveredPrefixTable::Entry::Matches(const ExpirationChecker &aCheker) const
{
    return GetExpireTime() <= aCheker.mNow;
}

TimeMilli RoutingManager::DiscoveredPrefixTable::Entry::GetExpireTime(void) const
{
    return mLastUpdateTime + CalculateExpireDelay(mValidLifetime);
}

TimeMilli RoutingManager::DiscoveredPrefixTable::Entry::GetStaleTime(void) const
{
    uint32_t delay = OT_MIN(kRtrAdvStaleTime, IsOnLinkPrefix() ? GetPreferredLifetime() : mValidLifetime);

    return mLastUpdateTime + TimeMilli::SecToMsec(delay);
}

bool RoutingManager::DiscoveredPrefixTable::Entry::IsDeprecated(void) const
{
    OT_ASSERT(IsOnLinkPrefix());

    return mLastUpdateTime + TimeMilli::SecToMsec(GetPreferredLifetime()) <= TimerMilli::GetNow();
}

RoutingManager::RoutePreference RoutingManager::DiscoveredPrefixTable::Entry::GetPreference(void) const
{
    // Returns the preference level to use when we publish
    // the prefix entry in Network Data.

    return IsOnLinkPrefix() ? NetworkData::kRoutePreferenceMedium : GetRoutePreference();
}

void RoutingManager::DiscoveredPrefixTable::Entry::AdoptValidAndPreferredLiftimesFrom(const Entry &aEntry)
{
    constexpr uint32_t kTwoHoursInSeconds = 2 * 3600;

    // Per RFC 4862 section 5.5.3.e:
    //
    // 1.  If the received Valid Lifetime is greater than 2 hours or
    //     greater than RemainingLifetime, set the valid lifetime of the
    //     corresponding address to the advertised Valid Lifetime.
    // 2.  If RemainingLifetime is less than or equal to 2 hours, ignore
    //     the Prefix Information option with regards to the valid
    //     lifetime, unless ...
    // 3.  Otherwise, reset the valid lifetime of the corresponding
    //     address to 2 hours.

    if (aEntry.mValidLifetime > kTwoHoursInSeconds || aEntry.GetExpireTime() > GetExpireTime())
    {
        mValidLifetime = aEntry.mValidLifetime;
    }
    else if (GetExpireTime() > TimerMilli::GetNow() + TimeMilli::SecToMsec(kTwoHoursInSeconds))
    {
        mValidLifetime = kTwoHoursInSeconds;
    }

    mShared.mPreferredLifetime = aEntry.GetPreferredLifetime();
    mLastUpdateTime            = aEntry.GetLastUpdateTime();
}

uint32_t RoutingManager::DiscoveredPrefixTable::Entry::CalculateExpireDelay(uint32_t aValidLifetime)
{
    uint32_t delay;

    if (aValidLifetime * static_cast<uint64_t>(1000) > Timer::kMaxDelay)
    {
        delay = Timer::kMaxDelay;
    }
    else
    {
        delay = aValidLifetime * 1000;
    }

    return delay;
}

//---------------------------------------------------------------------------------------------------------------------
// OmrPrefix

void RoutingManager::OmrPrefix::Init(const Ip6::Prefix &aPrefix, RoutePreference aPreference)
{
    mPrefix     = aPrefix;
    mPreference = aPreference;
}

void RoutingManager::OmrPrefix::InitFrom(NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig)
{
    Init(aOnMeshPrefixConfig.GetPrefix(), aOnMeshPrefixConfig.GetPreference());
}

bool RoutingManager::OmrPrefix::IsFavoredOver(const OmrPrefix &aOther) const
{
    // This method determines whether this OMR prefix is favored
    // over `aOther` prefix. A prefix with higher preference is
    // favored. If the preference is the same, then the smaller
    // prefix (in the sense defined by `Ip6::Prefix`) is favored.

    return (mPreference > aOther.mPreference) || ((mPreference == aOther.mPreference) && (mPrefix < aOther.mPrefix));
}

RoutingManager::OmrPrefix::InfoString RoutingManager::OmrPrefix::ToString(void) const
{
    InfoString string;

    string.Append("%s (prf:", mPrefix.ToString().AsCString());

    switch (mPreference)
    {
    case NetworkData::kRoutePreferenceHigh:
        string.Append("high)");
        break;
    case NetworkData::kRoutePreferenceMedium:
        string.Append("med)");
        break;
    case NetworkData::kRoutePreferenceLow:
        string.Append("low)");
        break;
    }

    return string;
}

//---------------------------------------------------------------------------------------------------------------------
// LocalOmrPrefix

RoutingManager::LocalOmrPrefix::LocalOmrPrefix(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsAddedInNetData(false)
{
}

void RoutingManager::LocalOmrPrefix::GenerateFrom(const Ip6::Prefix &aBrUlaPrefix)
{
    mPrefix = aBrUlaPrefix;
    mPrefix.SetSubnetId(kOmrPrefixSubnetId);
    mPrefix.SetLength(kOmrPrefixLength);

    LogInfo("Generated OMR prefix: %s", mPrefix.ToString().AsCString());
}

Error RoutingManager::LocalOmrPrefix::AddToNetData(void)
{
    Error                           error = kErrorNone;
    NetworkData::OnMeshPrefixConfig config;

    VerifyOrExit(!mIsAddedInNetData);

    config.Clear();
    config.mPrefix       = mPrefix;
    config.mStable       = true;
    config.mSlaac        = true;
    config.mPreferred    = true;
    config.mOnMesh       = true;
    config.mDefaultRoute = false;
    config.mPreference   = NetworkData::kRoutePreferenceLow;

    error = Get<NetworkData::Local>().AddOnMeshPrefix(config);

    if (error != kErrorNone)
    {
        LogWarn("Failed to add local OMR prefix %s in Thread Network Data: %s", mPrefix.ToString().AsCString(),
                ErrorToString(error));
        ExitNow();
    }

    mIsAddedInNetData = true;
    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    LogInfo("Added local OMR prefix %s in Thread Network Data", mPrefix.ToString().AsCString());

exit:
    return error;
}

void RoutingManager::LocalOmrPrefix::RemoveFromNetData(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsAddedInNetData);

    error = Get<NetworkData::Local>().RemoveOnMeshPrefix(mPrefix);

    if (error != kErrorNone)
    {
        LogWarn("Failed to remove local OMR prefix %s from Thread Network Data: %s", mPrefix.ToString().AsCString(),
                ErrorToString(error));
        ExitNow();
    }

    mIsAddedInNetData = false;
    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    LogInfo("Removed local OMR prefix %s from Thread Network Data", mPrefix.ToString().AsCString());

exit:
    return;
}

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
