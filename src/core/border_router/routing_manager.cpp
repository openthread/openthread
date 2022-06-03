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
    , mIsAdvertisingLocalOnLinkPrefix(false)
    , mOnLinkPrefixDeprecateTimer(aInstance, HandleOnLinkPrefixDeprecateTimer)
    , mIsAdvertisingLocalNat64Prefix(false)
    , mTimeRouterAdvMessageLastUpdate(TimerMilli::GetNow())
    , mLearntRouterAdvMessageFromHost(false)
    , mDiscoveredPrefixInvalidTimer(aInstance, HandleDiscoveredPrefixInvalidTimer)
    , mDiscoveredPrefixStaleTimer(aInstance, HandleDiscoveredPrefixStaleTimer)
    , mRouterAdvertisementCount(0)
    , mLastRouterAdvertisementSendTime(TimerMilli::GetNow() - kMinDelayBetweenRtrAdvs)
    , mRouterSolicitTimer(aInstance, HandleRouterSolicitTimer)
    , mRouterSolicitCount(0)
    , mRoutingPolicyTimer(aInstance, HandleRoutingPolicyTimer)
{
    mBrUlaPrefix.Clear();

    mLocalOmrPrefix.Clear();

    mLocalOnLinkPrefix.Clear();

    mLocalNat64Prefix.Clear();
}

Error RoutingManager::Init(uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    Error error;

    SuccessOrExit(error = mInfraIf.Init(aInfraIfIndex));

    SuccessOrExit(error = LoadOrGenerateRandomBrUlaPrefix());
    GenerateOmrPrefix();
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

Error RoutingManager::GetOmrPrefix(Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mLocalOmrPrefix;

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

void RoutingManager::GenerateOmrPrefix(void)
{
    mLocalOmrPrefix = mBrUlaPrefix;
    mLocalOmrPrefix.SetSubnetId(kOmrPrefixSubnetId);
    mLocalOmrPrefix.SetLength(kOmrPrefixLength);

    LogInfo("Generated OMR prefix: %s", mLocalOmrPrefix.ToString().AsCString());
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
        StartRouterSolicitationDelay();
    }
}

void RoutingManager::Stop(void)
{
    VerifyOrExit(mIsRunning);

    UnpublishLocalOmrPrefix();

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

    InvalidateAllDiscoveredPrefixes();
    mDiscoveredPrefixes.Clear();
    mDiscoveredPrefixInvalidTimer.Stop();
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
        // Invalidate discovered prefixes because OMR Prefixes in Network Data may change.
        InvalidateDiscoveredPrefixes();
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

        if (entry->GetPrefix() == mLocalOmrPrefix)
        {
            localOmrEntry = entry;
        }
    }

    // Decide if we need to add or remove our local OMR prefix.

    if (favoredOmrEntry == nullptr)
    {
        LogInfo("EvaluateOmrPrefix: No preferred OMR prefixes found in Thread network");

        // The `aNewOmrPrefixes` remains empty if we fail to publish
        // the local OMR prefix.
        SuccessOrExit(PublishLocalOmrPrefix());

        localOmrEntry = aNewOmrPrefixes.PushBack();
        VerifyOrExit(localOmrEntry != nullptr);

        localOmrEntry->Init(mLocalOmrPrefix, NetworkData::kRoutePreferenceMedium);
    }
    else if (favoredOmrEntry == localOmrEntry)
    {
        IgnoreError(PublishLocalOmrPrefix());
    }
    else if (IsOmrPrefixAddedToLocalNetworkData())
    {
        LogInfo("EvaluateOmrPrefix: There is already a preferred OMR prefix %s in the Thread network",
                favoredOmrEntry->ToString().AsCString());

        UnpublishLocalOmrPrefix();

        if (localOmrEntry != nullptr)
        {
            // Remove the local OMR prefix from the list by overwriting it
            // with popped last entry in the list.
            *localOmrEntry = *aNewOmrPrefixes.PopBack();
        }
    }

exit:
    return;
}

Error RoutingManager::PublishLocalOmrPrefix(void)
{
    Error                           error = kErrorNone;
    NetworkData::OnMeshPrefixConfig omrPrefixConfig;

    OT_ASSERT(mIsRunning);

    VerifyOrExit(!IsOmrPrefixAddedToLocalNetworkData());

    omrPrefixConfig.Clear();
    omrPrefixConfig.mPrefix       = mLocalOmrPrefix;
    omrPrefixConfig.mStable       = true;
    omrPrefixConfig.mSlaac        = true;
    omrPrefixConfig.mPreferred    = true;
    omrPrefixConfig.mOnMesh       = true;
    omrPrefixConfig.mDefaultRoute = false;
    omrPrefixConfig.mPreference   = NetworkData::kRoutePreferenceMedium;

    error = Get<NetworkData::Local>().AddOnMeshPrefix(omrPrefixConfig);
    if (error != kErrorNone)
    {
        LogWarn("Failed to publish local OMR prefix %s in Thread network: %s", mLocalOmrPrefix.ToString().AsCString(),
                ErrorToString(error));
    }
    else
    {
        Get<NetworkData::Notifier>().HandleServerDataUpdated();
        LogInfo("Publishing local OMR prefix %s in Thread network", mLocalOmrPrefix.ToString().AsCString());
    }

exit:
    return error;
}

void RoutingManager::UnpublishLocalOmrPrefix(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsRunning);

    VerifyOrExit(IsOmrPrefixAddedToLocalNetworkData());

    SuccessOrExit(error = Get<NetworkData::Local>().RemoveOnMeshPrefix(mLocalOmrPrefix));

    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    LogInfo("Unpublishing local OMR prefix %s from Thread network", mLocalOmrPrefix.ToString().AsCString());

exit:
    if (error != kErrorNone && error != kErrorNotFound)
    {
        LogWarn("Failed to unpublish local OMR prefix %s from Thread network: %s",
                mLocalOmrPrefix.ToString().AsCString(), ErrorToString(error));
    }
}

bool RoutingManager::IsOmrPrefixAddedToLocalNetworkData(void) const
{
    return Get<NetworkData::Local>().ContainsOnMeshPrefix(mLocalOmrPrefix);
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

    return (error == kErrorAlready) ? kErrorNone : error;
}

void RoutingManager::UnpublishExternalRoute(const Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsRunning);

    error = Get<NetworkData::Publisher>().UnpublishPrefix(aPrefix);

    if (error != kErrorNone)
    {
        LogWarn("Failed to unpublish route %s: %s", aPrefix.ToString().AsCString(), ErrorToString(error));
    }

exit:
    return;
}

void RoutingManager::EvaluateOnLinkPrefix(void)
{
    const Ip6::Prefix *smallestOnLinkPrefix = nullptr;

    // We don't evaluate on-link prefix if we are doing Router Solicitation.
    VerifyOrExit(!IsRouterSolicitationInProgress());

    for (const ExternalPrefix &prefix : mDiscoveredPrefixes)
    {
        if (!prefix.IsOnLinkPrefix() || prefix.IsDeprecated())
        {
            continue;
        }

        if (smallestOnLinkPrefix == nullptr || (prefix.GetPrefix() < *smallestOnLinkPrefix))
        {
            smallestOnLinkPrefix = &prefix.GetPrefix();
        }
    }

    // We start advertising our local on-link prefix if there is no existing one.
    if (smallestOnLinkPrefix == nullptr)
    {
        if (!mIsAdvertisingLocalOnLinkPrefix &&
            (PublishExternalRoute(mLocalOnLinkPrefix, NetworkData::kRoutePreferenceMedium) == kErrorNone))
        {
            mIsAdvertisingLocalOnLinkPrefix = true;
            LogInfo("Start advertising on-link prefix %s on %s", mLocalOnLinkPrefix.ToString().AsCString(),
                    mInfraIf.ToString().AsCString());

            // We go through `mDiscoveredPrefixes` list to check if the
            // local on-link prefix was previously discovered and
            // included in the list and if so we remove it from list.
            //
            // Note that `UpdateDiscoveredOnLinkPrefix()` will also
            // check and not add local on-link prefix in the discovered
            // prefix list while we are advertising the local on-link
            // prefix.

            for (ExternalPrefix &prefix : mDiscoveredPrefixes)
            {
                if (prefix.IsOnLinkPrefix() && mLocalOnLinkPrefix == prefix.GetPrefix())
                {
                    // To remove the prefix from the list, we copy the
                    // popped last entry into `prefix` entry.
                    prefix = *mDiscoveredPrefixes.PopBack();
                    break;
                }
            }
        }

        mOnLinkPrefixDeprecateTimer.Stop();
    }
    // When an application-specific on-link prefix is received and it is bigger than the
    // advertised prefix, we will not remove the advertised prefix. In this case, there
    // will be two on-link prefixes on the infra link. But all BRs will still converge to
    // the same smallest on-link prefix and the application-specific prefix is not used.
    else if (mIsAdvertisingLocalOnLinkPrefix)
    {
        if (!(mLocalOnLinkPrefix < *smallestOnLinkPrefix))
        {
            LogInfo("EvaluateOnLinkPrefix: There is already smaller on-link prefix %s on %s",
                    smallestOnLinkPrefix->ToString().AsCString(), mInfraIf.ToString().AsCString());
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
    bool discoveredLocalOnLinkPrefix = false;

    OT_ASSERT(!mIsAdvertisingLocalOnLinkPrefix);

    LogInfo("Local on-link prefix %s expired", mLocalOnLinkPrefix.ToString().AsCString());

    for (const ExternalPrefix &prefix : mDiscoveredPrefixes)
    {
        if (prefix.IsOnLinkPrefix() && prefix.GetPrefix() == mLocalOnLinkPrefix)
        {
            discoveredLocalOnLinkPrefix = true;
            break;
        }
    }

    if (!discoveredLocalOnLinkPrefix)
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
                raMsg.AppendRouteInfoOption(omrPrefix.GetPrefix(), /* aRouteLifetime */ 0, omrPrefix.GetPreference()));

            LogInfo("RouterAdvert: Added RIO for %s (lifetime=0)", omrPrefix.ToString().AsCString());
        }
    }

    for (const OmrPrefix &omrPrefix : aNewOmrPrefixes)
    {
        SuccessOrAssert(
            raMsg.AppendRouteInfoOption(omrPrefix.GetPrefix(), kDefaultOmrPrefixLifetime, omrPrefix.GetPreference()));

        LogInfo("RouterAdvert: Added RIO for %s (lifetime=%u)", omrPrefix.ToString().AsCString(),
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
    return !aOnLinkPrefix.IsLinkLocal() && !aOnLinkPrefix.IsMulticast();
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
        // Invalidate/deprecate all OMR/on-link prefixes that are not refreshed during Router Solicitation.
        for (ExternalPrefix &prefix : mDiscoveredPrefixes)
        {
            if (prefix.GetLastUpdateTime() <= mTimeRouterSolicitStart)
            {
                if (prefix.IsOnLinkPrefix())
                {
                    prefix.ClearPreferredLifetime();
                }
                else
                {
                    prefix.ClearValidLifetime();
                }
            }
        }

        InvalidateDiscoveredPrefixes();

        // Invalidate the learned RA message if it is not refreshed during Router Solicitation.
        if (mTimeRouterAdvMessageLastUpdate <= mTimeRouterSolicitStart)
        {
            UpdateRouterAdvertHeader(/* aRouterAdvertMessage */ nullptr);
        }

        mRouterSolicitCount = 0;

        // Re-evaluate our routing policy and send Router Advertisement if necessary.
        StartRoutingPolicyEvaluationDelay(/* aDelayJitter */ 0);

        // Reset prefix stale timer because `mDiscoveredPrefixes` may change.
        ResetDiscoveredPrefixStaleTimer();
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

void RoutingManager::HandleDiscoveredPrefixInvalidTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().HandleDiscoveredPrefixInvalidTimer();
}

void RoutingManager::HandleDiscoveredPrefixInvalidTimer(void)
{
    InvalidateDiscoveredPrefixes();
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
    OT_UNUSED_VARIABLE(aSrcAddress);

    bool                         needReevaluate = false;
    Ip6::Nd::RouterAdvertMessage routerAdvMessage(aPacket);

    OT_ASSERT(mIsRunning);

    VerifyOrExit(routerAdvMessage.IsValid());

    LogInfo("Received Router Advertisement from %s on %s", aSrcAddress.ToString().AsCString(),
            mInfraIf.ToString().AsCString());
    DumpDebg("[BR-CERT] direction=recv | type=RA |", aPacket.GetBytes(), aPacket.GetLength());

    for (const Ip6::Nd::Option &option : routerAdvMessage)
    {
        switch (option.GetType())
        {
        case Ip6::Nd::Option::kTypePrefixInfo:
            needReevaluate |= UpdateDiscoveredOnLinkPrefix(static_cast<const Ip6::Nd::PrefixInfoOption &>(option));
            break;

        case Ip6::Nd::Option::kTypeRouteInfo:
            UpdateDiscoveredOmrPrefix(static_cast<const Ip6::Nd::RouteInfoOption &>(option));
            break;

        default:
            break;
        }
    }

    // Remember the header and parameters of RA messages which are
    // initiated from the infra interface.
    if (mInfraIf.HasAddress(aSrcAddress))
    {
        needReevaluate |= UpdateRouterAdvertHeader(&routerAdvMessage);
    }

    if (needReevaluate)
    {
        StartRoutingPolicyEvaluationJitter(kRoutingPolicyEvaluationJitter);
    }

exit:
    return;
}

// Adds or deprecates a discovered on-link prefix (new external routes may be added
// to the Thread network). Returns a boolean which indicates whether we need to do
// routing policy evaluation.
bool RoutingManager::UpdateDiscoveredOnLinkPrefix(const Ip6::Nd::PrefixInfoOption &aPio)
{
    Ip6::Prefix     prefix;
    bool            needReevaluate = false;
    ExternalPrefix  onLinkPrefix;
    ExternalPrefix *existingPrefix = nullptr;

    VerifyOrExit(aPio.IsValid());

    aPio.GetPrefix(prefix);

    if (!IsValidOnLinkPrefix(aPio))
    {
        LogInfo("Ignore invalid on-link prefix in PIO: %s", prefix.ToString().AsCString());
        ExitNow();
    }

    VerifyOrExit(!mIsAdvertisingLocalOnLinkPrefix || prefix != mLocalOnLinkPrefix);

    LogInfo("Discovered on-link prefix (%s, %u seconds) from %s", prefix.ToString().AsCString(),
            aPio.GetValidLifetime(), mInfraIf.ToString().AsCString());

    onLinkPrefix.InitFrom(aPio);

    existingPrefix = mDiscoveredPrefixes.Find(onLinkPrefix);

    if (existingPrefix == nullptr)
    {
        if (onLinkPrefix.GetValidLifetime() == 0)
        {
            ExitNow();
        }

        if (!mDiscoveredPrefixes.IsFull())
        {
            SuccessOrExit(PublishExternalRoute(prefix, NetworkData::kRoutePreferenceMedium));
            existingPrefix  = mDiscoveredPrefixes.PushBack();
            *existingPrefix = onLinkPrefix;
            needReevaluate  = true;
        }
        else
        {
            LogWarn("Discovered too many prefixes, ignore new on-link prefix %s", prefix.ToString().AsCString());
            ExitNow();
        }
    }
    else
    {
        // The on-link prefix routing policy may be affected when a
        // discovered on-link prefix becomes deprecated or preferred.
        needReevaluate = (onLinkPrefix.IsDeprecated() != existingPrefix->IsDeprecated());

        existingPrefix->AdoptValidAndPreferredLiftimesFrom(onLinkPrefix);
    }

    mDiscoveredPrefixInvalidTimer.FireAtIfEarlier(existingPrefix->GetExpireTime());
    ResetDiscoveredPrefixStaleTimer();

exit:
    return needReevaluate;
}

// Adds or removes a discovered OMR prefix (external route will be added to or removed
// from the Thread network).
void RoutingManager::UpdateDiscoveredOmrPrefix(const Ip6::Nd::RouteInfoOption &aRio)
{
    Ip6::Prefix     prefix;
    ExternalPrefix  omrPrefix;
    ExternalPrefix *existingPrefix = nullptr;

    VerifyOrExit(aRio.IsValid());

    aRio.GetPrefix(prefix);

    if (!IsValidOmrPrefix(prefix))
    {
        LogInfo("Ignore invalid OMR prefix in RIO: %s", prefix.ToString().AsCString());
        ExitNow();
    }

    // Ignore own OMR prefix.
    VerifyOrExit(mLocalOmrPrefix != prefix);

    // Ignore OMR prefixes advertised by ourselves or in current Thread Network Data.
    // The `mAdvertisedOmrPrefixes` and the OMR prefix set in Network Data should eventually
    // be equal, but there is time that they are not synchronized immediately:
    // 1. Network Data could contain more OMR prefixes than `mAdvertisedOmrPrefixes` because
    //    we added random delay before Evaluating routing policy when Network Data is changed.
    // 2. `mAdvertisedOmrPrefixes` could contain more OMR prefixes than Network Data because
    //    it takes time to sync a new OMR prefix into Network Data (multicast loopback RA
    //    messages are usually faster than Thread Network Data propagation).
    // They are the reasons why we need both the checks.

    VerifyOrExit(!mAdvertisedOmrPrefixes.ContainsMatching(prefix));
    VerifyOrExit(!NetworkDataContainsOmrPrefix(prefix));

    LogInfo("Discovered OMR prefix (%s, %u seconds) from %s", prefix.ToString().AsCString(), aRio.GetRouteLifetime(),
            mInfraIf.ToString().AsCString());

    omrPrefix.InitFrom(aRio);

    existingPrefix = mDiscoveredPrefixes.Find(omrPrefix);

    if (omrPrefix.GetValidLifetime() == 0)
    {
        if (existingPrefix != nullptr)
        {
            existingPrefix->ClearValidLifetime();
            InvalidateDiscoveredPrefixes();
        }

        ExitNow();
    }

    if (existingPrefix == nullptr)
    {
        if (!mDiscoveredPrefixes.IsFull())
        {
            SuccessOrExit(PublishExternalRoute(prefix, omrPrefix.GetRoutePreference()));
            existingPrefix = mDiscoveredPrefixes.PushBack();
        }
        else
        {
            LogWarn("Discovered too many prefixes, ignore new prefix %s", prefix.ToString().AsCString());
            ExitNow();
        }
    }

    *existingPrefix = omrPrefix;

    mDiscoveredPrefixInvalidTimer.FireAtIfEarlier(existingPrefix->GetExpireTime());
    ResetDiscoveredPrefixStaleTimer();

exit:
    return;
}

void RoutingManager::InvalidateDiscoveredPrefixes(void)
{
    TimeMilli now                  = TimerMilli::GetNow();
    TimeMilli nextExpireTime       = now.GetDistantFuture();
    bool      containsOnLinkPrefix = false;

    mDiscoveredPrefixInvalidTimer.Stop();

    for (ExternalPrefixArray::IndexType index = 0; index < mDiscoveredPrefixes.GetLength();)
    {
        ExternalPrefix &prefix = mDiscoveredPrefixes[index];

        // We invalidate expired prefixes, or local OMR prefixes
        // (either in `mAdvertisedOmrPrefixes` or in Thread Network
        // Data).

        if ((prefix.GetExpireTime() <= now) ||
            (!prefix.IsOnLinkPrefix() && (mAdvertisedOmrPrefixes.ContainsMatching(prefix.GetPrefix()) ||
                                          NetworkDataContainsOmrPrefix(prefix.GetPrefix()))))
        {
            UnpublishExternalRoute(prefix.GetPrefix());

            // Remove the prefix from the array by replacing it with
            // last entry in the array (we copy the popped last entry
            // into `prefix` entry at current `index`). Also in this
            // case, the `index` is not incremented.

            prefix = *mDiscoveredPrefixes.PopBack();
        }
        else
        {
            nextExpireTime = OT_MIN(nextExpireTime, prefix.GetExpireTime());
            containsOnLinkPrefix |= prefix.IsOnLinkPrefix();

            index++;
        }
    }

    if (nextExpireTime != now.GetDistantFuture())
    {
        mDiscoveredPrefixInvalidTimer.FireAt(nextExpireTime);
    }

    if (!containsOnLinkPrefix && !mIsAdvertisingLocalOnLinkPrefix)
    {
        // There are no valid on-link prefixes on infra link now, start
        // Router Solicitation to discover more on-link prefixes or
        // time out to advertise the local on-link prefix.
        StartRouterSolicitationDelay();
    }
}

void RoutingManager::InvalidateAllDiscoveredPrefixes(void)
{
    for (ExternalPrefix &prefix : mDiscoveredPrefixes)
    {
        prefix.ClearValidLifetime();
    }

    InvalidateDiscoveredPrefixes();

    OT_ASSERT(mDiscoveredPrefixes.IsEmpty());
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

bool RoutingManager::UpdateRouterAdvertHeader(const Ip6::Nd::RouterAdvertMessage *aRouterAdvertMessage)
{
    // Updates the `mRouterAdvertHeader` from the given RA message.
    // Returns a boolean which indicates whether there was any changes
    // to `mRouterAdvertHeader`.

    Ip6::Nd::RouterAdvertMessage::Header oldHeader;

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

    return (mRouterAdvertHeader != oldHeader);
}

void RoutingManager::ResetDiscoveredPrefixStaleTimer(void)
{
    TimeMilli now                           = TimerMilli::GetNow();
    TimeMilli nextStaleTime                 = now.GetDistantFuture();
    TimeMilli maxOnlinkPrefixStaleTime      = now;
    bool      requireCheckStaleOnlinkPrefix = false;

    OT_ASSERT(mIsRunning);

    // The stale timer triggers sending RS to check the state of On-Link/OMR prefixes and host RA messages.
    // The rules for calculating the next stale time:
    // 1. If BR learns RA header from Host daemons, it should send RS when the RA header is stale.
    // 2. If BR discovered any on-link prefix, it should send RS when all on-link prefixes are stale.
    // 3. If BR discovered any OMR prefix, it should send RS when the first OMR prefix is stale.

    // Check for stale Router Advertisement Message if learnt from Host.
    if (mLearntRouterAdvMessageFromHost)
    {
        TimeMilli routerAdvMessageStaleTime = mTimeRouterAdvMessageLastUpdate + Time::SecToMsec(kRtrAdvStaleTime);

        nextStaleTime = OT_MIN(nextStaleTime, routerAdvMessageStaleTime);
    }

    for (ExternalPrefix &externalPrefix : mDiscoveredPrefixes)
    {
        TimeMilli prefixStaleTime = externalPrefix.GetStaleTime();

        if (externalPrefix.IsOnLinkPrefix())
        {
            if (!externalPrefix.IsDeprecated())
            {
                // Check for least recent stale On-Link Prefixes if BR is not advertising local On-Link Prefix.
                maxOnlinkPrefixStaleTime      = OT_MAX(maxOnlinkPrefixStaleTime, prefixStaleTime);
                requireCheckStaleOnlinkPrefix = true;
            }
        }
        else
        {
            // Check for most recent stale OMR Prefixes
            nextStaleTime = OT_MIN(nextStaleTime, prefixStaleTime);
        }
    }

    if (requireCheckStaleOnlinkPrefix)
    {
        nextStaleTime = OT_MIN(nextStaleTime, maxOnlinkPrefixStaleTime);
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
// ExtneralPrefix

void RoutingManager::ExternalPrefix::InitFrom(const Ip6::Nd::PrefixInfoOption &aPio)
{
    Clear();
    aPio.GetPrefix(mPrefix);
    mIsOnLinkPrefix    = true;
    mValidLifetime     = aPio.GetValidLifetime();
    mPreferredLifetime = aPio.GetPreferredLifetime();
    mLastUpdateTime    = TimerMilli::GetNow();
}

void RoutingManager::ExternalPrefix::InitFrom(const Ip6::Nd::RouteInfoOption &aRio)
{
    Clear();
    aRio.GetPrefix(mPrefix);
    mIsOnLinkPrefix  = false;
    mValidLifetime   = aRio.GetRouteLifetime();
    mRoutePreference = aRio.GetPreference();
    mLastUpdateTime  = TimerMilli::GetNow();
}

bool RoutingManager::ExternalPrefix::operator==(const ExternalPrefix &aPrefix) const
{
    return mIsOnLinkPrefix == aPrefix.mIsOnLinkPrefix && (mPrefix == aPrefix.mPrefix);
}

TimeMilli RoutingManager::ExternalPrefix::GetStaleTime(void) const
{
    uint32_t delay = OT_MIN(kRtrAdvStaleTime, mIsOnLinkPrefix ? mPreferredLifetime : mValidLifetime);

    return mLastUpdateTime + TimeMilli::SecToMsec(delay);
}

bool RoutingManager::ExternalPrefix::IsDeprecated(void) const
{
    OT_ASSERT(mIsOnLinkPrefix);

    return mLastUpdateTime + TimeMilli::SecToMsec(mPreferredLifetime) <= TimerMilli::GetNow();
}

void RoutingManager::ExternalPrefix::AdoptValidAndPreferredLiftimesFrom(const ExternalPrefix &aPrefix)
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

    if (aPrefix.mValidLifetime > kTwoHoursInSeconds || aPrefix.GetExpireTime() > GetExpireTime())
    {
        mValidLifetime = aPrefix.mValidLifetime;
    }
    else if (GetExpireTime() > TimerMilli::GetNow() + TimeMilli::SecToMsec(kTwoHoursInSeconds))
    {
        mValidLifetime = kTwoHoursInSeconds;
    }

    mPreferredLifetime = aPrefix.GetPreferredLifetime();
    mLastUpdateTime    = aPrefix.GetLastUpdateTime();
}

uint32_t RoutingManager::ExternalPrefix::GetPrefixExpireDelay(uint32_t aValidLifetime)
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

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
