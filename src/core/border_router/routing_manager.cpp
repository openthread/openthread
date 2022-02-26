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
    , mInfraIfIsRunning(false)
    , mInfraIfIndex(0)
    , mIsAdvertisingLocalOnLinkPrefix(false)
    , mOnLinkPrefixDeprecateTimer(aInstance, HandleOnLinkPrefixDeprecateTimer)
    , mIsAdvertisingLocalNat64Prefix(false)
    , mTimeRouterAdvMessageLastUpdate(TimerMilli::GetNow())
    , mLearntRouterAdvMessageFromHost(false)
    , mDiscoveredPrefixInvalidTimer(aInstance, HandleDiscoveredPrefixInvalidTimer)
    , mDiscoveredPrefixStaleTimer(aInstance, HandleDiscoveredPrefixStaleTimer)
    , mRouterAdvertisementCount(0)
#if OPENTHREAD_CONFIG_BORDER_ROUTING_VICARIOUS_RS_ENABLE
    , mVicariousRouterSolicitTimer(aInstance, HandleVicariousRouterSolicitTimer)
#endif
    , mRouterSolicitTimer(aInstance, HandleRouterSolicitTimer)
    , mRouterSolicitCount(0)
    , mRoutingPolicyTimer(aInstance, HandleRoutingPolicyTimer)
{
    mLocalOmrPrefix.Clear();

    mLocalOnLinkPrefix.Clear();

    mLocalNat64Prefix.Clear();
}

Error RoutingManager::Init(uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    Error error;

    VerifyOrExit(!IsInitialized(), error = kErrorInvalidState);
    VerifyOrExit(aInfraIfIndex > 0, error = kErrorInvalidArgs);

    SuccessOrExit(error = LoadOrGenerateRandomOmrPrefix());
    SuccessOrExit(error = LoadOrGenerateRandomOnLinkPrefix());
#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
    SuccessOrExit(error = LoadOrGenerateRandomNat64Prefix());
#endif

    mInfraIfIndex = aInfraIfIndex;

    // Initialize the infra interface status.
    SuccessOrExit(error = HandleInfraIfStateChanged(mInfraIfIndex, aInfraIfIsRunning));

exit:
    if (error != kErrorNone)
    {
        mInfraIfIndex = 0;
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

Error RoutingManager::LoadOrGenerateRandomOmrPrefix(void)
{
    Error error     = kErrorNone;
    bool  generated = false;

    if (Get<Settings>().Read<Settings::OmrPrefix>(mLocalOmrPrefix) != kErrorNone || !IsValidOmrPrefix(mLocalOmrPrefix))
    {
        Ip6::NetworkPrefix randomOmrPrefix;

        LogNote("No valid OMR prefix found in settings, generating new one");

        // TODO: generate OMR prefix from the /48 BR ULA prefix
        error = randomOmrPrefix.GenerateRandomUla();
        if (error != kErrorNone)
        {
            LogCrit("Failed to generate random OMR prefix");
            ExitNow();
        }

        mLocalOmrPrefix.Set(randomOmrPrefix);
        IgnoreError(Get<Settings>().Save<Settings::OmrPrefix>(mLocalOmrPrefix));
        generated = true;
    }

    OT_UNUSED_VARIABLE(generated);

    LogNote("Local OMR prefix: %s (%s)", mLocalOmrPrefix.ToString().AsCString(), generated ? "generated" : "loaded");

exit:
    return error;
}

Error RoutingManager::LoadOrGenerateRandomOnLinkPrefix(void)
{
    Error error     = kErrorNone;
    bool  generated = false;

    if (Get<Settings>().Read<Settings::OnLinkPrefix>(mLocalOnLinkPrefix) != kErrorNone ||
        !mLocalOnLinkPrefix.IsUniqueLocal())
    {
        Ip6::NetworkPrefix randomOnLinkPrefix;

        error = randomOnLinkPrefix.GenerateRandomUla();
        if (error != kErrorNone)
        {
            LogCrit("Failed to generate random on-link prefix");
            ExitNow();
        }

        randomOnLinkPrefix.m8[6] = 0;
        randomOnLinkPrefix.m8[7] = 0;
        mLocalOnLinkPrefix.Set(randomOnLinkPrefix);

        IgnoreError(Get<Settings>().Save<Settings::OnLinkPrefix>(mLocalOnLinkPrefix));
        generated = true;
    }

    OT_UNUSED_VARIABLE(generated);

    LogNote("Local on-link prefix: %s (%s)", mLocalOnLinkPrefix.ToString().AsCString(),
            generated ? "generated" : "loaded");

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
Error RoutingManager::LoadOrGenerateRandomNat64Prefix(void)
{
    Error error = kErrorNone;

    if (Get<Settings>().Read<Settings::Nat64Prefix>(mLocalNat64Prefix) != kErrorNone ||
        !mLocalNat64Prefix.IsValidNat64())
    {
        Ip6::NetworkPrefix randomNat64Prefix;
        constexpr uint8_t  nat64PrefixLength = 96;

        LogNote("No valid NAT64 prefix found in settings, generating new one");

        // TODO: generate NAT64 prefix from the /48 BR ULA prefix
        error = randomNat64Prefix.GenerateRandomUla();
        if (error != kErrorNone)
        {
            LogCrit("Failed to generate random NAT64 prefix");
            ExitNow();
        }

        mLocalNat64Prefix.Clear();
        mLocalNat64Prefix.Set(randomNat64Prefix);
        mLocalNat64Prefix.SetLength(nat64PrefixLength);

        IgnoreError(Get<Settings>().Save<Settings::Nat64Prefix>(mLocalNat64Prefix));
    }

exit:
    return error;
}
#endif

void RoutingManager::EvaluateState(void)
{
    if (mIsEnabled && Get<Mle::MleRouter>().IsAttached() && mInfraIfIsRunning)
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
        RemoveExternalRoute(mLocalOnLinkPrefix);

        // Start deprecating the local on-link prefix to send a PIO
        // with zero preferred lifetime in `SendRouterAdvertisement`.
        DeprecateOnLinkPrefix();
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
    if (mIsAdvertisingLocalNat64Prefix)
    {
        RemoveExternalRoute(mLocalNat64Prefix);
        mIsAdvertisingLocalNat64Prefix = false;
    }
#endif
    // Use empty OMR & on-link prefixes to invalidate possible advertised prefixes.
    SendRouterAdvertisement(OmrPrefixArray(), nullptr);

    mAdvertisedOmrPrefixes.Clear();
    mIsAdvertisingLocalOnLinkPrefix = false;
    mOnLinkPrefixDeprecateTimer.Stop();

    InvalidateAllDiscoveredPrefixes();
    mDiscoveredPrefixes.Clear();
    mDiscoveredPrefixInvalidTimer.Stop();
    mDiscoveredPrefixStaleTimer.Stop();

    mRouterAdvertisementCount = 0;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_VICARIOUS_RS_ENABLE
    mVicariousRouterSolicitTimer.Stop();
#endif
    mRouterSolicitTimer.Stop();
    mRouterSolicitCount = 0;

    mRoutingPolicyTimer.Stop();

    LogInfo("Border Routing manager stopped");

    mIsRunning = false;

exit:
    return;
}

void RoutingManager::RecvIcmp6Message(uint32_t            aInfraIfIndex,
                                      const Ip6::Address &aSrcAddress,
                                      const uint8_t *     aBuffer,
                                      uint16_t            aBufferLength)
{
    Error                    error = kErrorNone;
    const Ip6::Icmp::Header *icmp6Header;

    VerifyOrExit(IsInitialized() && mIsRunning, error = kErrorDrop);
    VerifyOrExit(aInfraIfIndex == mInfraIfIndex, error = kErrorDrop);
    VerifyOrExit(aBuffer != nullptr && aBufferLength >= sizeof(*icmp6Header), error = kErrorParse);

    icmp6Header = reinterpret_cast<const Ip6::Icmp::Header *>(aBuffer);

    switch (icmp6Header->GetType())
    {
    case Ip6::Icmp::Header::kTypeRouterAdvert:
        HandleRouterAdvertisement(aSrcAddress, aBuffer, aBufferLength);
        break;
    case Ip6::Icmp::Header::kTypeRouterSolicit:
        HandleRouterSolicit(aSrcAddress, aBuffer, aBufferLength);
        break;
    default:
        break;
    }

exit:
    if (error != kErrorNone)
    {
        LogDebg("Dropped ICMPv6 message: %s", ErrorToString(error));
    }
}

Error RoutingManager::HandleInfraIfStateChanged(uint32_t aInfraIfIndex, bool aIsRunning)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    VerifyOrExit(aInfraIfIndex == mInfraIfIndex, error = kErrorInvalidArgs);
    VerifyOrExit(aIsRunning != mInfraIfIsRunning);

    LogInfo("Infra interface (%u) state changed: %sRUNNING -> %sRUNNING", aInfraIfIndex,
            (mInfraIfIsRunning ? "" : "NOT "), (aIsRunning ? "" : "NOT "));

    mInfraIfIsRunning = aIsRunning;
    EvaluateState();

exit:
    return error;
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

exit:
    return;
}

void RoutingManager::EvaluateOmrPrefix(OmrPrefixArray &aNewOmrPrefixes)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig onMeshPrefixConfig;
    Ip6::Prefix *                   smallestOmrPrefix       = nullptr;
    Ip6::Prefix *                   publishedLocalOmrPrefix = nullptr;

    OT_ASSERT(mIsRunning);

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, onMeshPrefixConfig) == kErrorNone)
    {
        const Ip6::Prefix &prefix = onMeshPrefixConfig.GetPrefix();

        if (!IsValidOmrPrefix(onMeshPrefixConfig))
        {
            continue;
        }

        if (aNewOmrPrefixes.Contains(prefix))
        {
            // Ignore duplicate prefixes.
            continue;
        }

        if (aNewOmrPrefixes.PushBack(prefix) != kErrorNone)
        {
            LogWarn("EvaluateOmrPrefix: Too many OMR prefixes, ignoring prefix %s", prefix.ToString().AsCString());
            continue;
        }

        if (smallestOmrPrefix == nullptr || (prefix < *smallestOmrPrefix))
        {
            smallestOmrPrefix = aNewOmrPrefixes.Back();
        }

        if (prefix == mLocalOmrPrefix)
        {
            publishedLocalOmrPrefix = aNewOmrPrefixes.Back();
        }
    }

    // Decide if we need to add or remove our local OMR prefix.

    if (aNewOmrPrefixes.IsEmpty())
    {
        LogInfo("EvaluateOmrPrefix: No valid OMR prefixes found in Thread network");

        if (PublishLocalOmrPrefix() == kErrorNone)
        {
            IgnoreError(aNewOmrPrefixes.PushBack(mLocalOmrPrefix));
        }

        // The `aNewOmrPrefixes` remains empty if we fail to publish
        // the local OMR prefix.
    }
    else if (publishedLocalOmrPrefix != nullptr && smallestOmrPrefix != publishedLocalOmrPrefix)
    {
        LogInfo("EvaluateOmrPrefix: There is already a smaller OMR prefix %s in the Thread network",
                smallestOmrPrefix->ToString().AsCString());

        UnpublishLocalOmrPrefix();

        // Remove the local OMR prefix from the list by overwriting it
        // with the last element and then popping it from the list.

        *publishedLocalOmrPrefix = *aNewOmrPrefixes.Back();
        aNewOmrPrefixes.PopBack();
    }
}

Error RoutingManager::PublishLocalOmrPrefix(void)
{
    Error                           error = kErrorNone;
    NetworkData::OnMeshPrefixConfig omrPrefixConfig;

    OT_ASSERT(mIsRunning);

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

    return error;
}

void RoutingManager::UnpublishLocalOmrPrefix(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsRunning);

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

Error RoutingManager::AddExternalRoute(const Ip6::Prefix &aPrefix, RoutePreference aRoutePreference, bool aNat64)
{
    Error                            error;
    NetworkData::ExternalRouteConfig routeConfig;

    OT_ASSERT(mIsRunning);

    routeConfig.Clear();
    routeConfig.SetPrefix(aPrefix);
    routeConfig.mStable     = true;
    routeConfig.mNat64      = aNat64;
    routeConfig.mPreference = aRoutePreference;

    error = Get<NetworkData::Local>().AddHasRoutePrefix(routeConfig);
    if (error != kErrorNone)
    {
        LogWarn("Failed to add external route %s: %s", aPrefix.ToString().AsCString(), ErrorToString(error));
    }
    else
    {
        Get<NetworkData::Notifier>().HandleServerDataUpdated();
        LogInfo("Adding external route %s", aPrefix.ToString().AsCString());
    }

    return error;
}

void RoutingManager::RemoveExternalRoute(const Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsRunning);

    SuccessOrExit(error = Get<NetworkData::Local>().RemoveHasRoutePrefix(aPrefix));

    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    LogInfo("Removing external route %s", aPrefix.ToString().AsCString());

exit:
    if (error != kErrorNone)
    {
        LogWarn("Failed to remove external route %s: %s", aPrefix.ToString().AsCString(), ErrorToString(error));
    }
}

const Ip6::Prefix *RoutingManager::EvaluateOnLinkPrefix(void)
{
    const Ip6::Prefix *newOnLinkPrefix      = nullptr;
    const Ip6::Prefix *smallestOnLinkPrefix = nullptr;

    // We don't evaluate on-link prefix if we are doing Router Solicitation.
    VerifyOrExit(!IsRouterSolicitationInProgress(),
                 newOnLinkPrefix = (mIsAdvertisingLocalOnLinkPrefix ? &mLocalOnLinkPrefix : nullptr));

    for (const ExternalPrefix &prefix : mDiscoveredPrefixes)
    {
        if (!prefix.mIsOnLinkPrefix || prefix.IsDeprecated())
        {
            continue;
        }

        if (smallestOnLinkPrefix == nullptr || (prefix.mPrefix < *smallestOnLinkPrefix))
        {
            smallestOnLinkPrefix = &prefix.mPrefix;
        }
    }

    // We start advertising our local on-link prefix if there is no existing one.
    if (smallestOnLinkPrefix == nullptr)
    {
        if (mIsAdvertisingLocalOnLinkPrefix ||
            (AddExternalRoute(mLocalOnLinkPrefix, NetworkData::kRoutePreferenceMedium) == kErrorNone))
        {
            newOnLinkPrefix = &mLocalOnLinkPrefix;
        }

        mOnLinkPrefixDeprecateTimer.Stop();
    }
    // When an application-specific on-link prefix is received and it is bigger than the
    // advertised prefix, we will not remove the advertised prefix. In this case, there
    // will be two on-link prefixes on the infra link. But all BRs will still converge to
    // the same smallest on-link prefix and the application-specific prefix is not used.
    else if (mIsAdvertisingLocalOnLinkPrefix)
    {
        if (mLocalOnLinkPrefix < *smallestOnLinkPrefix)
        {
            newOnLinkPrefix = &mLocalOnLinkPrefix;
        }
        else
        {
            LogInfo("EvaluateOnLinkPrefix: There is already smaller on-link prefix %s on interface %u",
                    smallestOnLinkPrefix->ToString().AsCString(), mInfraIfIndex);
            DeprecateOnLinkPrefix();
        }
    }

exit:
    return newOnLinkPrefix;
}

void RoutingManager::HandleOnLinkPrefixDeprecateTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().HandleOnLinkPrefixDeprecateTimer();
}

void RoutingManager::HandleOnLinkPrefixDeprecateTimer(void)
{
    LogInfo("Local on-link prefix %s expired", mLocalOnLinkPrefix.ToString().AsCString());
    RemoveExternalRoute(mLocalOnLinkPrefix);
}

void RoutingManager::DeprecateOnLinkPrefix(void)
{
    OT_ASSERT(mIsAdvertisingLocalOnLinkPrefix);

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
            AddExternalRoute(mLocalNat64Prefix, NetworkData::kRoutePreferenceLow, /* aNat64= */ true) == kErrorNone)
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

        RemoveExternalRoute(mLocalNat64Prefix);
        mIsAdvertisingLocalNat64Prefix = false;
    }
}
#endif

// This method evaluate the routing policy depends on prefix and route
// information on Thread Network and infra link. As a result, this
// method May send RA messages on infra link and publish/unpublish
// OMR prefix in the Thread network.
void RoutingManager::EvaluateRoutingPolicy(void)
{
    OT_ASSERT(mIsRunning);

    const Ip6::Prefix *newOnLinkPrefix = nullptr;
    OmrPrefixArray     newOmrPrefixes;

    LogInfo("Evaluating routing policy");

    // 0. Evaluate on-link & OMR prefixes.
    newOnLinkPrefix = EvaluateOnLinkPrefix();
    EvaluateOmrPrefix(newOmrPrefixes);
#if OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
    EvaluateNat64Prefix();
#endif

    // 1. Send Router Advertisement message if necessary.
    SendRouterAdvertisement(newOmrPrefixes, newOnLinkPrefix);

    if (newOmrPrefixes.IsEmpty())
    {
        // This is the very exceptional case and happens only when we failed to publish
        // our local OMR prefix to the Thread network. We schedule the Router Advertisement
        // timer to re-evaluate our routing policy in the future.

        LogWarn("No OMR prefix advertised! Start Router Advertisement timer for future evaluation");
    }

    // 2. Schedule Router Advertisement timer with random interval.
    {
        uint32_t nextSendDelay;

        nextSendDelay = Random::NonCrypto::GetUint32InRange(kMinRtrAdvInterval, kMaxRtrAdvInterval);

        if (mRouterAdvertisementCount <= kMaxInitRtrAdvertisements && nextSendDelay > kMaxInitRtrAdvInterval)
        {
            nextSendDelay = kMaxInitRtrAdvInterval;
        }

        LogInfo("Router advertisement scheduled in %u seconds", nextSendDelay);
        StartRoutingPolicyEvaluationDelay(Time::SecToMsec(nextSendDelay));
    }

    // 3. Update advertised on-link & OMR prefixes information.
    mIsAdvertisingLocalOnLinkPrefix = (newOnLinkPrefix == &mLocalOnLinkPrefix);
    mAdvertisedOmrPrefixes          = newOmrPrefixes;
}

void RoutingManager::StartRoutingPolicyEvaluationJitter(uint32_t aJitterMilli)
{
    OT_ASSERT(mIsRunning);

    StartRoutingPolicyEvaluationDelay(Random::NonCrypto::GetUint32InRange(0, aJitterMilli));
}

void RoutingManager::StartRoutingPolicyEvaluationDelay(uint32_t aDelayMilli)
{
    LogInfo("Start evaluating routing policy, scheduled in %u milliseconds", aDelayMilli);
    mRoutingPolicyTimer.FireAtIfEarlier(TimerMilli::GetNow() + aDelayMilli);
}

// starts sending Router Solicitations in random delay
// between 0 and kMaxRtrSolicitationDelay.
void RoutingManager::StartRouterSolicitationDelay(void)
{
    uint32_t randomDelay;

    VerifyOrExit(!IsRouterSolicitationInProgress());

    OT_ASSERT(mRouterSolicitCount == 0);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_VICARIOUS_RS_ENABLE
    mVicariousRouterSolicitTimer.Stop();
#endif

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
    Ip6::Address                    destAddress;
    RouterAdv::RouterSolicitMessage routerSolicit;

    OT_ASSERT(IsInitialized());

    destAddress.SetToLinkLocalAllRoutersMulticast();
    return otPlatInfraIfSendIcmp6Nd(mInfraIfIndex, &destAddress, reinterpret_cast<const uint8_t *>(&routerSolicit),
                                    sizeof(routerSolicit));
}

// This method sends Router Advertisement messages to advertise on-link prefix and route for OMR prefix.
// @param[in]  aNewOmrPrefixes   An array of the new OMR prefixes to be advertised.
//                               Empty array means we should stop advertising OMR prefixes.
// @param[in]  aOnLinkPrefix     A pointer to the new on-link prefix to be advertised.
//                               `nullptr` means we should stop advertising on-link prefix.
void RoutingManager::SendRouterAdvertisement(const OmrPrefixArray &aNewOmrPrefixes, const Ip6::Prefix *aNewOnLinkPrefix)
{
    uint8_t  buffer[kMaxRouterAdvMessageLength];
    uint16_t bufferLength = 0;

    static_assert(sizeof(mRouterAdvMessage) <= sizeof(buffer), "RA buffer too small");
    memcpy(buffer, &mRouterAdvMessage, sizeof(mRouterAdvMessage));
    bufferLength += sizeof(mRouterAdvMessage);

    if (aNewOnLinkPrefix != nullptr)
    {
        OT_ASSERT(aNewOnLinkPrefix == &mLocalOnLinkPrefix);

        RouterAdv::PrefixInfoOption pio;

        pio.SetOnLink(true);
        pio.SetAutoAddrConfig(true);
        pio.SetValidLifetime(kDefaultOnLinkPrefixLifetime);
        pio.SetPreferredLifetime(kDefaultOnLinkPrefixLifetime);
        pio.SetPrefix(*aNewOnLinkPrefix);

        OT_ASSERT(bufferLength + pio.GetSize() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &pio, pio.GetSize());
        bufferLength += pio.GetSize();

        if (!mIsAdvertisingLocalOnLinkPrefix)
        {
            LogInfo("Start advertising new on-link prefix %s on interface %u", aNewOnLinkPrefix->ToString().AsCString(),
                    mInfraIfIndex);
        }

        LogInfo("Send on-link prefix %s in PIO (preferred lifetime = %u seconds, valid lifetime = %u seconds)",
                aNewOnLinkPrefix->ToString().AsCString(), pio.GetPreferredLifetime(), pio.GetValidLifetime());

        mTimeAdvertisedOnLinkPrefix = TimerMilli::GetNow();
    }
    else if (mOnLinkPrefixDeprecateTimer.IsRunning())
    {
        RouterAdv::PrefixInfoOption pio;

        pio.SetOnLink(true);
        pio.SetAutoAddrConfig(true);
        pio.SetValidLifetime(TimeMilli::MsecToSec(mOnLinkPrefixDeprecateTimer.GetFireTime() - TimerMilli::GetNow()));

        // Set zero preferred lifetime to immediately deprecate the advertised on-link prefix.
        pio.SetPreferredLifetime(0);
        pio.SetPrefix(mLocalOnLinkPrefix);

        OT_ASSERT(bufferLength + pio.GetSize() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &pio, pio.GetSize());
        bufferLength += pio.GetSize();

        LogInfo("Send on-link prefix %s in PIO (preferred lifetime = %u seconds, valid lifetime = %u seconds)",
                mLocalOnLinkPrefix.ToString().AsCString(), pio.GetPreferredLifetime(), pio.GetValidLifetime());
    }

    // Invalidate the advertised OMR prefixes if they are no longer in the new OMR prefix array.

    for (const Ip6::Prefix &advertisedOmrPrefix : mAdvertisedOmrPrefixes)
    {
        if (!aNewOmrPrefixes.Contains(advertisedOmrPrefix))
        {
            RouterAdv::RouteInfoOption rio;

            // Set zero route lifetime to immediately invalidate the advertised OMR prefix.
            rio.SetRouteLifetime(0);
            rio.SetPrefix(advertisedOmrPrefix);

            OT_ASSERT(bufferLength + rio.GetSize() <= sizeof(buffer));
            memcpy(buffer + bufferLength, &rio, rio.GetSize());
            bufferLength += rio.GetSize();

            LogInfo("Stop advertising OMR prefix %s on interface %u", advertisedOmrPrefix.ToString().AsCString(),
                    mInfraIfIndex);
        }
    }

    for (const Ip6::Prefix &newOmrPrefix : aNewOmrPrefixes)
    {
        RouterAdv::RouteInfoOption rio;

        rio.SetRouteLifetime(kDefaultOmrPrefixLifetime);
        rio.SetPrefix(newOmrPrefix);

        OT_ASSERT(bufferLength + rio.GetSize() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &rio, rio.GetSize());
        bufferLength += rio.GetSize();

        LogInfo("Send OMR prefix %s in RIO (valid lifetime = %u seconds)", newOmrPrefix.ToString().AsCString(),
                kDefaultOmrPrefixLifetime);
    }

    // Send the message only when there are options.
    if (bufferLength > sizeof(mRouterAdvMessage))
    {
        Error        error;
        Ip6::Address destAddress;

        ++mRouterAdvertisementCount;

        destAddress.SetToLinkLocalAllNodesMulticast();
        error = otPlatInfraIfSendIcmp6Nd(mInfraIfIndex, &destAddress, buffer, bufferLength);

        if (error == kErrorNone)
        {
            LogInfo("Sent Router Advertisement on interface %u", mInfraIfIndex);
            DumpDebg("[BR-CERT] direction=send | type=RA |", buffer, bufferLength);
        }
        else
        {
            LogWarn("Failed to send Router Advertisement on interface %u: %s", mInfraIfIndex, ErrorToString(error));
        }
    }
}

bool RoutingManager::IsValidOmrPrefix(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig)
{
    return IsValidOmrPrefix(aOnMeshPrefixConfig.GetPrefix()) && aOnMeshPrefixConfig.mSlaac && !aOnMeshPrefixConfig.mDp;
}

bool RoutingManager::IsValidOmrPrefix(const Ip6::Prefix &aOmrPrefix)
{
    // Accept ULA prefix with length of 64 bits and GUA prefix.
    return (aOmrPrefix.mLength == kOmrPrefixLength && aOmrPrefix.mPrefix.mFields.m8[0] == 0xfd) ||
           (aOmrPrefix.mLength >= 3 && (aOmrPrefix.GetBytes()[0] & 0xE0) == 0x20);
}

bool RoutingManager::IsValidOnLinkPrefix(const RouterAdv::PrefixInfoOption &aPio)
{
    return IsValidOnLinkPrefix(aPio.GetPrefix()) && aPio.GetOnLink() && aPio.GetAutoAddrConfig();
}

bool RoutingManager::IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix)
{
    return !aOnLinkPrefix.IsLinkLocal() && !aOnLinkPrefix.IsMulticast();
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_VICARIOUS_RS_ENABLE
void RoutingManager::HandleVicariousRouterSolicitTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().HandleVicariousRouterSolicitTimer();
}

void RoutingManager::HandleVicariousRouterSolicitTimer(void)
{
    LogInfo("Vicarious router solicitation time out");

    for (const ExternalPrefix &prefix : mDiscoveredPrefixes)
    {
        if (prefix.mTimeLastUpdate <= mTimeVicariousRouterSolicitStart)
        {
            StartRouterSolicitationDelay();
            break;
        }
    }
}
#endif

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
            if (prefix.mTimeLastUpdate <= mTimeRouterSolicitStart)
            {
                if (prefix.mIsOnLinkPrefix)
                {
                    prefix.mPreferredLifetime = 0;
                }
                else
                {
                    InvalidateDiscoveredPrefixes(&prefix.mPrefix, prefix.mIsOnLinkPrefix);
                }
            }
        }

        // Invalidate the learned RA message if it is not refreshed during Router Solicitation.
        if (mTimeRouterAdvMessageLastUpdate <= mTimeRouterSolicitStart)
        {
            UpdateRouterAdvMessage(/* aRouterAdvMessage */ nullptr);
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

void RoutingManager::HandleRouterSolicit(const Ip6::Address &aSrcAddress,
                                         const uint8_t *     aBuffer,
                                         uint16_t            aBufferLength)
{
    OT_UNUSED_VARIABLE(aSrcAddress);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aBufferLength);

    LogInfo("Received Router Solicitation from %s on interface %u", aSrcAddress.ToString().AsCString(), mInfraIfIndex);

#if OPENTHREAD_CONFIG_BORDER_ROUTING_VICARIOUS_RS_ENABLE
    if (!mVicariousRouterSolicitTimer.IsRunning())
    {
        mTimeVicariousRouterSolicitStart = TimerMilli::GetNow();
        mVicariousRouterSolicitTimer.Start(Time::SecToMsec(kVicariousSolicitationTime));
    }
#endif

    // Schedule Router Advertisements with random delay.
    StartRoutingPolicyEvaluationJitter(kRaReplyJitter);
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

void RoutingManager::HandleRouterAdvertisement(const Ip6::Address &aSrcAddress,
                                               const uint8_t *     aBuffer,
                                               uint16_t            aBufferLength)
{
    OT_ASSERT(mIsRunning);
    OT_UNUSED_VARIABLE(aSrcAddress);

    using RouterAdv::Option;
    using RouterAdv::PrefixInfoOption;
    using RouterAdv::RouteInfoOption;
    using RouterAdv::RouterAdvMessage;

    bool                    needReevaluate = false;
    const uint8_t *         optionsBegin;
    uint16_t                optionsLength;
    const Option *          option;
    const RouterAdvMessage *routerAdvMessage;

    VerifyOrExit(aBufferLength >= sizeof(RouterAdvMessage));

    LogInfo("Received Router Advertisement from %s on interface %u", aSrcAddress.ToString().AsCString(), mInfraIfIndex);
    DumpDebg("[BR-CERT] direction=recv | type=RA |", aBuffer, aBufferLength);

    routerAdvMessage = reinterpret_cast<const RouterAdvMessage *>(aBuffer);
    optionsBegin     = aBuffer + sizeof(RouterAdvMessage);
    optionsLength    = aBufferLength - sizeof(RouterAdvMessage);

    option = nullptr;
    while ((option = Option::GetNextOption(option, optionsBegin, optionsLength)) != nullptr)
    {
        switch (option->GetType())
        {
        case Option::Type::kPrefixInfo:
        {
            const PrefixInfoOption *pio = static_cast<const PrefixInfoOption *>(option);

            if (pio->IsValid())
            {
                needReevaluate |= UpdateDiscoveredOnLinkPrefix(*pio);
            }
        }
        break;

        case Option::Type::kRouteInfo:
        {
            const RouteInfoOption *rio = static_cast<const RouteInfoOption *>(option);

            if (rio->IsValid())
            {
                UpdateDiscoveredOmrPrefix(*rio);
            }
        }
        break;

        default:
            break;
        }
    }

    // Remember the header and parameters of RA messages which are
    // initiated from the infra interface.
    if (otPlatInfraIfHasAddress(mInfraIfIndex, &aSrcAddress))
    {
        needReevaluate |= UpdateRouterAdvMessage(routerAdvMessage);
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
bool RoutingManager::UpdateDiscoveredOnLinkPrefix(const RouterAdv::PrefixInfoOption &aPio)
{
    Ip6::Prefix     prefix         = aPio.GetPrefix();
    bool            needReevaluate = false;
    ExternalPrefix  onLinkPrefix;
    ExternalPrefix *existingPrefix = nullptr;

    if (!IsValidOnLinkPrefix(aPio))
    {
        LogInfo("Ignore invalid on-link prefix in PIO: %s", prefix.ToString().AsCString());
        ExitNow();
    }

    VerifyOrExit(!mIsAdvertisingLocalOnLinkPrefix || prefix != mLocalOnLinkPrefix);

    LogInfo("Discovered on-link prefix (%s, %u seconds) from interface %u", prefix.ToString().AsCString(),
            aPio.GetValidLifetime(), mInfraIfIndex);

    onLinkPrefix.mIsOnLinkPrefix    = true;
    onLinkPrefix.mPrefix            = prefix;
    onLinkPrefix.mValidLifetime     = aPio.GetValidLifetime();
    onLinkPrefix.mPreferredLifetime = aPio.GetPreferredLifetime();
    onLinkPrefix.mTimeLastUpdate    = TimerMilli::GetNow();

    for (ExternalPrefix &externalPrefix : mDiscoveredPrefixes)
    {
        if (externalPrefix == onLinkPrefix)
        {
            existingPrefix = &externalPrefix;
        }
    }

    if (existingPrefix == nullptr)
    {
        if (onLinkPrefix.mValidLifetime == 0)
        {
            ExitNow();
        }

        if (!mDiscoveredPrefixes.IsFull())
        {
            SuccessOrExit(AddExternalRoute(prefix, NetworkData::kRoutePreferenceMedium));
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
        constexpr uint32_t kTwoHoursInSeconds = 2 * 3600;

        // Per RFC 4862 section 5.5.3.e:
        // 1.  If the received Valid Lifetime is greater than 2 hours or
        //     greater than RemainingLifetime, set the valid lifetime of the
        //     corresponding address to the advertised Valid Lifetime.
        // 2.  If RemainingLifetime is less than or equal to 2 hours, ignore
        //     the Prefix Information option with regards to the valid
        //     lifetime, unless ...
        // 3.  Otherwise, reset the valid lifetime of the corresponding
        //     address to 2 hours.

        if (onLinkPrefix.mValidLifetime > kTwoHoursInSeconds ||
            onLinkPrefix.GetExpireTime() > existingPrefix->GetExpireTime())
        {
            existingPrefix->mValidLifetime = onLinkPrefix.mValidLifetime;
        }
        else if (existingPrefix->GetExpireTime() > TimerMilli::GetNow() + TimeMilli::SecToMsec(kTwoHoursInSeconds))
        {
            existingPrefix->mValidLifetime = kTwoHoursInSeconds;
        }

        // The on-link prefix routing policy may be affected when a
        // discovered on-link prefix becomes deprecated or preferred.
        needReevaluate = (onLinkPrefix.IsDeprecated() != existingPrefix->IsDeprecated());

        existingPrefix->mPreferredLifetime = onLinkPrefix.mPreferredLifetime;
        existingPrefix->mTimeLastUpdate    = onLinkPrefix.mTimeLastUpdate;
    }

    mDiscoveredPrefixInvalidTimer.FireAtIfEarlier(existingPrefix->GetExpireTime());
    ResetDiscoveredPrefixStaleTimer();

exit:
    return needReevaluate;
}

// Adds or removes a discovered OMR prefix (external route will be added to or removed
// from the Thread network).
void RoutingManager::UpdateDiscoveredOmrPrefix(const RouterAdv::RouteInfoOption &aRio)
{
    Ip6::Prefix     prefix = aRio.GetPrefix();
    ExternalPrefix  omrPrefix;
    ExternalPrefix *existingPrefix = nullptr;

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

    VerifyOrExit(!mAdvertisedOmrPrefixes.Contains(prefix));
    VerifyOrExit(!NetworkDataContainsOmrPrefix(prefix));

    LogInfo("Discovered OMR prefix (%s, %u seconds) from interface %u", prefix.ToString().AsCString(),
            aRio.GetRouteLifetime(), mInfraIfIndex);

    if (aRio.GetRouteLifetime() == 0)
    {
        InvalidateDiscoveredPrefixes(&prefix, /* aIsOnLinkPrefix */ false);
        ExitNow();
    }

    omrPrefix.mIsOnLinkPrefix  = false;
    omrPrefix.mPrefix          = prefix;
    omrPrefix.mValidLifetime   = aRio.GetRouteLifetime();
    omrPrefix.mRoutePreference = aRio.GetPreference();
    omrPrefix.mTimeLastUpdate  = TimerMilli::GetNow();

    for (ExternalPrefix &externalPrefix : mDiscoveredPrefixes)
    {
        if (externalPrefix == omrPrefix)
        {
            existingPrefix = &externalPrefix;
        }
    }

    if (existingPrefix == nullptr)
    {
        if (omrPrefix.mValidLifetime == 0)
        {
            ExitNow();
        }

        if (!mDiscoveredPrefixes.IsFull())
        {
            SuccessOrExit(AddExternalRoute(prefix, omrPrefix.mRoutePreference));
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

void RoutingManager::InvalidateDiscoveredPrefixes(const Ip6::Prefix *aPrefix, bool aIsOnLinkPrefix)
{
    TimeMilli           now                      = TimerMilli::GetNow();
    uint8_t             remainingOnLinkPrefixNum = 0;
    ExternalPrefixArray remainingPrefixes;

    mDiscoveredPrefixInvalidTimer.Stop();

    for (const ExternalPrefix &prefix : mDiscoveredPrefixes)
    {
        if (
            // Invalidate specified prefix
            (aPrefix != nullptr && prefix.mPrefix == *aPrefix && prefix.mIsOnLinkPrefix == aIsOnLinkPrefix) ||
            // Invalidate expired prefix
            (prefix.GetExpireTime() <= now) ||
            // Invalidate Local OMR prefixes
            (!prefix.mIsOnLinkPrefix &&
             (mAdvertisedOmrPrefixes.Contains(prefix.mPrefix) || NetworkDataContainsOmrPrefix(prefix.mPrefix))))
        {
            RemoveExternalRoute(prefix.mPrefix);
        }
        else
        {
            mDiscoveredPrefixInvalidTimer.FireAtIfEarlier(prefix.GetExpireTime());

            IgnoreError(remainingPrefixes.PushBack(prefix));

            if (prefix.mIsOnLinkPrefix)
            {
                ++remainingOnLinkPrefixNum;
            }
        }
    }

    mDiscoveredPrefixes = remainingPrefixes;

    if (remainingOnLinkPrefixNum == 0 && !mIsAdvertisingLocalOnLinkPrefix)
    {
        // There are no valid on-link prefixes on infra link now, start Router Solicitation
        // To discover more on-link prefixes or timeout to advertise my local on-link prefix.
        StartRouterSolicitationDelay();
    }
}

void RoutingManager::InvalidateAllDiscoveredPrefixes(void)
{
    for (ExternalPrefix &prefix : mDiscoveredPrefixes)
    {
        prefix.mValidLifetime = 0;
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

// Update the `mRouterAdvMessage` with given Router Advertisement message.
// Returns a boolean which indicates whether there are changes of `mRouterAdvMessage`.
bool RoutingManager::UpdateRouterAdvMessage(const RouterAdv::RouterAdvMessage *aRouterAdvMessage)
{
    RouterAdv::RouterAdvMessage oldRouterAdvMessage;

    oldRouterAdvMessage = mRouterAdvMessage;

    mTimeRouterAdvMessageLastUpdate = TimerMilli::GetNow();
    if (aRouterAdvMessage == nullptr || aRouterAdvMessage->GetRouterLifetime() == 0)
    {
        mRouterAdvMessage.SetToDefault();
        mLearntRouterAdvMessageFromHost = false;
    }
    else
    {
        mRouterAdvMessage               = *aRouterAdvMessage;
        mLearntRouterAdvMessageFromHost = true;
    }

    ResetDiscoveredPrefixStaleTimer();

    return (mRouterAdvMessage != oldRouterAdvMessage);
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

        if (externalPrefix.mIsOnLinkPrefix)
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

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
