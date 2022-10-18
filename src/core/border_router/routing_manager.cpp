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

#include <openthread/border_router.h>
#include <openthread/platform/infra_if.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "meshcop/extended_panid.hpp"
#include "net/ip6.hpp"
#include "net/nat64_translator.hpp"
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
    , mLocalOnLinkPrefix(aInstance)
    , mDiscoveredPrefixTable(aInstance)
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    , mNat64PrefixManager(aInstance)
#endif
    , mRsSender(aInstance)
    , mDiscoveredPrefixStaleTimer(aInstance)
    , mRoutingPolicyTimer(aInstance)
{
    mFavoredDiscoveredOnLinkPrefix.Clear();

    mBrUlaPrefix.Clear();
}

Error RoutingManager::Init(uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    Error error;

    SuccessOrExit(error = mInfraIf.Init(aInfraIfIndex));

    SuccessOrExit(error = LoadOrGenerateRandomBrUlaPrefix());
    mLocalOmrPrefix.GenerateFrom(mBrUlaPrefix);
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    mNat64PrefixManager.GenerateLocalPrefix(mBrUlaPrefix);
#endif
    mLocalOnLinkPrefix.Generate();

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
    ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);

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

Error RoutingManager::GetFavoredOmrPrefix(Ip6::Prefix &aPrefix, RoutePreference &aPreference)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix     = mFavoredOmrPrefix.GetPrefix();
    aPreference = mFavoredOmrPrefix.GetPreference();

exit:
    return error;
}

Error RoutingManager::GetOnLinkPrefix(Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mLocalOnLinkPrefix.GetPrefix();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
void RoutingManager::SetNat64PrefixManagerEnabled(bool aEnabled)
{
    // PrefixManager will start itself if routing manager is running.
    mNat64PrefixManager.SetEnabled(aEnabled);
}

Error RoutingManager::GetNat64Prefix(Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mNat64PrefixManager.GetLocalPrefix();

exit:
    return error;
}

Error RoutingManager::GetFavoredNat64Prefix(Ip6::Prefix &aPrefix, RoutePreference &aRoutePreference)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mNat64PrefixManager.GetFavoredPrefix(aRoutePreference);

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
        mLocalOnLinkPrefix.Start();
        mRsSender.Start();
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
        mNat64PrefixManager.Start();
#endif
    }
}

void RoutingManager::Stop(void)
{
    VerifyOrExit(mIsRunning);

    mLocalOmrPrefix.RemoveFromNetData();
    mFavoredOmrPrefix.Clear();

    mFavoredDiscoveredOnLinkPrefix.Clear();

    mLocalOnLinkPrefix.Stop();

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    mNat64PrefixManager.Stop();
#endif

    SendRouterAdvertisement(kInvalidateAllPrevPrefixes);

    mAdvertisedPrefixes.Clear();

    mDiscoveredPrefixTable.RemoveAllEntries();
    mDiscoveredPrefixStaleTimer.Stop();

    mRaInfo.mTxCount = 0;

    mRsSender.Stop();

    mRoutingPolicyTimer.Stop();

    LogInfo("Border Routing manager stopped");

    mIsRunning = false;

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    if (Get<Srp::Server>().IsAutoEnableMode())
    {
        Get<Srp::Server>().Disable();
    }
#endif

exit:
    return;
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
void RoutingManager::HandleSrpServerAutoEnableMode(void)
{
    VerifyOrExit(Get<Srp::Server>().IsAutoEnableMode());

    if (IsInitalPolicyEvaluationDone())
    {
        Get<Srp::Server>().Enable();
    }
    else
    {
        Get<Srp::Server>().Disable();
    }

exit:
    return;
}
#endif

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
        mLocalOnLinkPrefix.HandleNetDataChange();
        ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
    }

    if (aEvents.Contains(kEventThreadExtPanIdChanged))
    {
        mLocalOnLinkPrefix.HandleExtPanIdChange();

        if (mIsRunning)
        {
            ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
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

        mDiscoveredPrefixTable.RemoveRoutePrefix(prefixConfig.GetPrefix());

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

void RoutingManager::EvaluateOmrPrefix(void)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig onMeshPrefixConfig;

    OT_ASSERT(mIsRunning);

    mFavoredOmrPrefix.Clear();

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, onMeshPrefixConfig) == kErrorNone)
    {
        if (!IsValidOmrPrefix(onMeshPrefixConfig) || !onMeshPrefixConfig.mPreferred)
        {
            continue;
        }

        if (mFavoredOmrPrefix.IsEmpty() || !mFavoredOmrPrefix.IsFavoredOver(onMeshPrefixConfig))
        {
            mFavoredOmrPrefix.SetFrom(onMeshPrefixConfig);
        }
    }

    // Decide if we need to add or remove our local OMR prefix.

    if (mFavoredOmrPrefix.IsEmpty())
    {
        LogInfo("EvaluateOmrPrefix: No preferred OMR prefix found in Thread network");

        // The `mFavoredOmrPrefix` remains empty if we fail to publish
        // the local OMR prefix.
        SuccessOrExit(mLocalOmrPrefix.AddToNetData());

        mFavoredOmrPrefix.SetFrom(mLocalOmrPrefix);
    }
    else if (mFavoredOmrPrefix.GetPrefix() == mLocalOmrPrefix.GetPrefix())
    {
        IgnoreError(mLocalOmrPrefix.AddToNetData());
    }
    else if (mLocalOmrPrefix.IsAddedInNetData())
    {
        LogInfo("EvaluateOmrPrefix: There is already a preferred OMR prefix %s in the Thread network",
                mFavoredOmrPrefix.GetPrefix().ToString().AsCString());

        mLocalOmrPrefix.RemoveFromNetData();
    }

exit:
    return;
}

void RoutingManager::EvaluatePublishingPrefix(const Ip6::Prefix &aPrefix)
{
    // This method evaluates whether to publish or unpublish a given
    // `aPrefix` as an external route in the Network Data. It makes a
    // collective decision by checking with different sub-components to
    // see whether or not each wants this prefix published and if so
    // at what preference level and flags.
    //
    // Before calling this method, the sub-components need to make sure
    // to update their internal state such that their `ShouldPublish()`
    // provides the correct info.

    bool                             shouldPublish = false;
    NetworkData::ExternalRouteConfig routeConfig;

    routeConfig.Clear();
    routeConfig.SetPrefix(aPrefix);
    routeConfig.mPreference = NetworkData::kRoutePreferenceLow;
    routeConfig.mStable     = true;

    // The order of checks is important. The Discovered Prefix Table is
    // first followed by Local On Link Prefix manager and finally NAT64
    // prefix manager.

    shouldPublish = mDiscoveredPrefixTable.ShouldPublish(routeConfig);

    if (mLocalOnLinkPrefix.ShouldPublish(routeConfig))
    {
        shouldPublish = true;
    }

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    if (mNat64PrefixManager.ShouldPublish(routeConfig))
    {
        shouldPublish = true;
    }
#endif

    if (shouldPublish)
    {
        SuccessOrAssert(Get<NetworkData::Publisher>().PublishExternalRoute(
            routeConfig, NetworkData::Publisher::kFromRoutingManager));
    }
    else
    {
        UnpublishExternalRoute(aPrefix);
    }
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
    VerifyOrExit(!mRsSender.IsInProgress());

    mDiscoveredPrefixTable.FindFavoredOnLinkPrefix(mFavoredDiscoveredOnLinkPrefix);

    if ((mFavoredDiscoveredOnLinkPrefix.GetLength() == 0) ||
        (mFavoredDiscoveredOnLinkPrefix == mLocalOnLinkPrefix.GetPrefix()))
    {
        // We need to advertise our local on-link prefix when there is
        // no discovered on-link prefix. If the favored discovered
        // prefix is the same as our local on-link prefix we also
        // start advertising the local prefix to add redundancy. Note
        // that local on-link prefix is derived from extended PAN ID
        // and therefore is the same for all BRs on the same Thread
        // mesh.

        mLocalOnLinkPrefix.PublishAndAdvertise();

        // We remove the local on-link prefix from discovered prefix
        // table, in case it was previously discovered and included in
        // the table (now as a deprecating entry). We remove it with
        // `kKeepInNetData` flag to ensure that the prefix is not
        // unpublished from network data.
        //
        // Note that `ShouldProcessPrefixInfoOption()` will also check
        // not allow the local on-link prefix to be added in the prefix
        // table while we are advertising it.

        mDiscoveredPrefixTable.RemoveOnLinkPrefix(mLocalOnLinkPrefix.GetPrefix());

        mFavoredDiscoveredOnLinkPrefix.Clear();
    }
    else if (mLocalOnLinkPrefix.IsPublishingOrAdvertising())
    {
        // When an application-specific on-link prefix is received and
        // it is larger than the local prefix, we will not remove the
        // advertised local prefix. In this case, there will be two
        // on-link prefixes on the infra link. But all BRs will still
        // converge to the same smallest/favored on-link prefix and the
        // application-specific prefix is not used.

        if (!(mLocalOnLinkPrefix.GetPrefix() < mFavoredDiscoveredOnLinkPrefix))
        {
            LogInfo("EvaluateOnLinkPrefix: There is already favored on-link prefix %s on %s",
                    mFavoredDiscoveredOnLinkPrefix.ToString().AsCString(), mInfraIf.ToString().AsCString());
            mLocalOnLinkPrefix.Deprecate();
        }
    }

exit:
    return;
}

// This method evaluate the routing policy depends on prefix and route
// information on Thread Network and infra link. As a result, this
// method May send RA messages on infra link and publish/unpublish
// OMR and NAT64 prefix in the Thread network.
void RoutingManager::EvaluateRoutingPolicy(void)
{
    OT_ASSERT(mIsRunning);

    LogInfo("Evaluating routing policy");

    EvaluateOnLinkPrefix();
    EvaluateOmrPrefix();
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    mNat64PrefixManager.Evaluate();
#endif

    SendRouterAdvertisement(kAdvPrefixesFromNetData);

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    if (Get<Srp::Server>().IsAutoEnableMode() && IsInitalPolicyEvaluationDone())
    {
        // If SRP server uses the auto-enable mode, we enable the SRP
        // server on the first RA transmission after we are done with
        // initial prefix/route configurations. Note that if SRP server
        // is already enabled, calling `Enable()` again does nothing.

        Get<Srp::Server>().Enable();
    }
#endif

    ScheduleRoutingPolicyEvaluation(kForNextRa);
}

bool RoutingManager::IsInitalPolicyEvaluationDone(void) const
{
    // This method indicates whether or not we are done with the
    // initial policy evaluation and prefix and route setup, i.e.,
    // the OMR and on-link prefixes are determined, advertised in
    // the emitted Router Advert message on infrastructure side
    // and published in the Thread Network Data.

    return mIsRunning && !mFavoredOmrPrefix.IsEmpty() &&
           (mFavoredDiscoveredOnLinkPrefix.GetLength() != 0 || mLocalOnLinkPrefix.IsPublishingOrAdvertising());
}

void RoutingManager::ScheduleRoutingPolicyEvaluation(ScheduleMode aMode)
{
    TimeMilli now   = TimerMilli::GetNow();
    uint32_t  delay = 0;
    TimeMilli evaluateTime;

    switch (aMode)
    {
    case kImmediately:
        break;

    case kForNextRa:
        delay = Random::NonCrypto::GetUint32InRange(Time::SecToMsec(kMinRtrAdvInterval),
                                                    Time::SecToMsec(kMaxRtrAdvInterval));

        if (mRaInfo.mTxCount <= kMaxInitRtrAdvertisements && delay > Time::SecToMsec(kMaxInitRtrAdvInterval))
        {
            delay = Time::SecToMsec(kMaxInitRtrAdvInterval);
        }
        break;

    case kAfterRandomDelay:
        delay = Random::NonCrypto::GetUint32InRange(kPolicyEvaluationMinDelay, kPolicyEvaluationMaxDelay);
        break;

    case kToReplyToRs:
        delay = Random::NonCrypto::GetUint32InRange(0, kRaReplyJitter);
        break;
    }

    // Ensure we wait a min delay after last RA tx
    evaluateTime = Max(now + delay, mRaInfo.mLastTxTime + kMinDelayBetweenRtrAdvs);

    LogInfo("Start evaluating routing policy, scheduled in %u milliseconds", evaluateTime - now);

    mRoutingPolicyTimer.FireAtIfEarlier(evaluateTime);
}

void RoutingManager::SendRouterAdvertisement(RouterAdvTxMode aRaTxMode)
{
    // RA message max length is derived to accommodate:
    //
    // - The RA header,
    // - At most two PIOs (for current and old local on-link prefixes),
    // - At most twice `kMaxOnMeshPrefixes` RIO for on-mesh prefixes.
    //   Factor two is used for RIO to account for entries invalidating
    //   previous prefixes while adding new ones.

    static constexpr uint16_t kMaxRaLength =
        sizeof(Ip6::Nd::RouterAdvertMessage::Header) + sizeof(Ip6::Nd::PrefixInfoOption) * 2 +
        2 * kMaxOnMeshPrefixes * (sizeof(Ip6::Nd::RouteInfoOption) + sizeof(Ip6::Prefix));

    uint8_t                         buffer[kMaxRaLength];
    Ip6::Nd::RouterAdvertMessage    raMsg(mRaInfo.mHeader, buffer);
    NetworkData::Iterator           iterator;
    NetworkData::OnMeshPrefixConfig prefixConfig;

    // Append PIO for local on-link prefix if is either being
    // advertised or deprecated and for old prefix if is being
    // deprecated.

    mLocalOnLinkPrefix.AppendAsPiosTo(raMsg);

    // Determine which previously advertised prefixes need to be
    // invalidated. Under `kInvalidateAllPrevPrefixes` mode we need
    // to invalidate all. Under `kAdvPrefixesFromNetData` mode, we
    // check Network Data entries and invalidate any previously
    // advertised prefix that is no longer present in the Network
    // Data. We go through all Network Data prefixes and mark the
    // ones we find in `mAdvertisedPrefixes` as deleted by setting
    // the prefix length to zero). By the end, the remaining entries
    // in the array with a non-zero prefix length are invalidated.

    if (aRaTxMode != kInvalidateAllPrevPrefixes)
    {
        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
        {
            if (!prefixConfig.mOnMesh || prefixConfig.mDp || (prefixConfig.GetPrefix() == mLocalOmrPrefix.GetPrefix()))
            {
                continue;
            }

            mAdvertisedPrefixes.MarkAsDeleted(prefixConfig.GetPrefix());
        }

        if (mLocalOmrPrefix.IsAddedInNetData())
        {
            mAdvertisedPrefixes.MarkAsDeleted(mLocalOmrPrefix.GetPrefix());
        }
    }

    for (const OnMeshPrefix &prefix : mAdvertisedPrefixes)
    {
        if (prefix.GetLength() != 0)
        {
            SuccessOrAssert(raMsg.AppendRouteInfoOption(prefix, /* aRouteLifetime */ 0, mRouteInfoOptionPreference));
            LogInfo("RouterAdvert: Added RIO for %s (lifetime=0)", prefix.ToString().AsCString());
        }
    }

    // Discover and add prefixes from Network Data to advertise as
    // RIO in the Router Advertisement message.

    mAdvertisedPrefixes.Clear();

    if (aRaTxMode == kAdvPrefixesFromNetData)
    {
        // `mAdvertisedPrefixes` array has a limited size. We add more
        // important prefixes first in the array to ensure they are
        // advertised in the RA message. Note that `Add()` method
        // will ensure to add a prefix only once (will check if
        // prefix is already present in the array).

        // (1) Local OMR prefix.

        if (mLocalOmrPrefix.IsAddedInNetData())
        {
            mAdvertisedPrefixes.Add(mLocalOmrPrefix.GetPrefix());
        }

        // (2) Favored OMR prefix.

        if (!mFavoredOmrPrefix.IsEmpty() && !mFavoredOmrPrefix.IsDomainPrefix())
        {
            mAdvertisedPrefixes.Add(mFavoredOmrPrefix.GetPrefix());
        }

        // (3) All other OMR prefixes.

        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
        {
            // Local OMR prefix is added to the array depending on
            // `mLocalOmrPrefix.IsAddedInNetData()` at step (1). As
            // we iterate through the Network Data prefixes, we skip
            // over entries matching the local OMR prefix. This
            // ensures that we stop including it in emitted RA
            // message as soon as we decide to remove it from Network
            // Data. Note that upon requesting it to be removed from
            // Network Data the change needs to be registered with
            // leader and can take some time to be updated in Network
            // Data.

            if (prefixConfig.mDp)
            {
                continue;
            }

            if (IsValidOmrPrefix(prefixConfig) && (prefixConfig.GetPrefix() != mLocalOmrPrefix.GetPrefix()))
            {
                mAdvertisedPrefixes.Add(prefixConfig.GetPrefix());
            }
        }

        // (4) All other on-mesh prefixes (excluding Domain Prefix).

        iterator = NetworkData::kIteratorInit;

        while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
        {
            if (prefixConfig.mOnMesh && !prefixConfig.mDp && !IsValidOmrPrefix(prefixConfig))
            {
                mAdvertisedPrefixes.Add(prefixConfig.GetPrefix());
            }
        }

        for (const OnMeshPrefix &prefix : mAdvertisedPrefixes)
        {
            SuccessOrAssert(raMsg.AppendRouteInfoOption(prefix, kDefaultOmrPrefixLifetime, mRouteInfoOptionPreference));
            LogInfo("RouterAdvert: Added RIO for %s (lifetime=%u)", prefix.ToString().AsCString(),
                    kDefaultOmrPrefixLifetime);
        }
    }

    if (raMsg.ContainsAnyOptions())
    {
        Error        error;
        Ip6::Address destAddress;

        ++mRaInfo.mTxCount;

        destAddress.SetToLinkLocalAllNodesMulticast();

        error = mInfraIf.Send(raMsg.GetAsPacket(), destAddress);

        if (error == kErrorNone)
        {
            mRaInfo.mLastTxTime = TimerMilli::GetNow();
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

bool RoutingManager::IsReceivedRouterAdvertFromManager(const Ip6::Nd::RouterAdvertMessage &aRaMessage) const
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
            const Ip6::Nd::PrefixInfoOption &pio = static_cast<const Ip6::Nd::PrefixInfoOption &>(option);

            VerifyOrExit(pio.IsValid());
            pio.GetPrefix(prefix);

            // If it is a non-deprecated PIO, it should match the
            // local on-link prefix.

            if (pio.GetPreferredLifetime() > 0)
            {
                VerifyOrExit(prefix == mLocalOnLinkPrefix.GetPrefix());
            }

            break;
        }

        case Ip6::Nd::Option::kTypeRouteInfo:
        {
            // RIO (with non-zero lifetime) should match entries from
            // `mAdvertisedPrefixes`. We keep track of the number
            // of matched RIOs and check after the loop ends that all
            // entries were seen.

            const Ip6::Nd::RouteInfoOption &rio = static_cast<const Ip6::Nd::RouteInfoOption &>(option);

            VerifyOrExit(rio.IsValid());
            rio.GetPrefix(prefix);

            if (rio.GetRouteLifetime() != 0)
            {
                VerifyOrExit(mAdvertisedPrefixes.Contains(prefix));
                rioCount++;
            }

            break;
        }

        default:
            ExitNow();
        }
    }

    VerifyOrExit(rioCount == mAdvertisedPrefixes.GetLength());

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
           aOnMeshPrefixConfig.mSlaac && aOnMeshPrefixConfig.mStable;
}

bool RoutingManager::IsValidOmrPrefix(const Ip6::Prefix &aPrefix)
{
    // Accept ULA/GUA prefixes with 64-bit length.
    return (aPrefix.GetLength() == kOmrPrefixLength) && !aPrefix.IsLinkLocal() && !aPrefix.IsMulticast();
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

void RoutingManager::HandleRsSenderFinished(TimeMilli aStartTime)
{
    // This is a callback from `RsSender` and is invoked when it
    // finishes a cycle of sending Router Solicitations. `aStartTime`
    // specifies the start time of the RS transmission cycle.
    //
    // We remove or deprecate old entries in discovered table that are
    // not refreshed during Router Solicitation. We also invalidate
    // the learned RA header if it is not refreshed during Router
    // Solicitation.

    mDiscoveredPrefixTable.RemoveOrDeprecateOldEntries(aStartTime);

    if (mRaInfo.mHeaderUpdateTime <= aStartTime)
    {
        UpdateRouterAdvertHeader(/* aRouterAdvertMessage */ nullptr);
    }

    ScheduleRoutingPolicyEvaluation(kImmediately);
}

void RoutingManager::HandleDiscoveredPrefixStaleTimer(void)
{
    LogInfo("Stale On-Link or OMR Prefixes or RA messages are detected");
    mRsSender.Start();
}

void RoutingManager::HandleRouterSolicit(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress)
{
    OT_UNUSED_VARIABLE(aPacket);
    OT_UNUSED_VARIABLE(aSrcAddress);

    LogInfo("Received Router Solicitation from %s on %s", aSrcAddress.ToString().AsCString(),
            mInfraIf.ToString().AsCString());

    ScheduleRoutingPolicyEvaluation(kToReplyToRs);
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

    if (mLocalOnLinkPrefix.IsPublishingOrAdvertising())
    {
        VerifyOrExit(aPrefix != mLocalOnLinkPrefix.GetPrefix());
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
    // The `mAdvertisedPrefixes` and the OMR prefix set in Network Data should eventually
    // be equal, but there is time that they are not synchronized immediately:
    // 1. Network Data could contain more OMR prefixes than `mAdvertisedPrefixes` because
    //    we added random delay before Evaluating routing policy when Network Data is changed.
    // 2. `mAdvertisedPrefixes` could contain more OMR prefixes than Network Data because
    //    it takes time to sync a new OMR prefix into Network Data (multicast loopback RA
    //    messages are usually faster than Thread Network Data propagation).
    // They are the reasons why we need both the checks.

    VerifyOrExit(!mAdvertisedPrefixes.Contains(aPrefix));
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
        ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
    }

exit:
    return;
}

bool RoutingManager::NetworkDataContainsOmrPrefix(const Ip6::Prefix &aPrefix) const
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig onMeshPrefixConfig;
    bool                            contains = false;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, onMeshPrefixConfig) == kErrorNone)
    {
        if (IsValidOmrPrefix(onMeshPrefixConfig) && onMeshPrefixConfig.GetPrefix() == aPrefix)
        {
            contains = true;
            break;
        }
    }

    return contains;
}

bool RoutingManager::NetworkDataContainsExternalRoute(const Ip6::Prefix &aPrefix) const
{
    NetworkData::Iterator            iterator = NetworkData::kIteratorInit;
    NetworkData::ExternalRouteConfig routeConfig;
    bool                             contains = false;

    while (Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig) == kErrorNone)
    {
        if (routeConfig.mStable && routeConfig.GetPrefix() == aPrefix)
        {
            contains = true;
            break;
        }
    }

    return contains;
}

void RoutingManager::UpdateRouterAdvertHeader(const Ip6::Nd::RouterAdvertMessage *aRouterAdvertMessage)
{
    // Updates the `mRaInfo` from the given RA message.

    Ip6::Nd::RouterAdvertMessage::Header oldHeader;

    if (aRouterAdvertMessage != nullptr)
    {
        // We skip and do not update RA header if the received RA message
        // was not prepared and sent by `RoutingManager` itself.

        VerifyOrExit(!IsReceivedRouterAdvertFromManager(*aRouterAdvertMessage));
    }

    oldHeader                 = mRaInfo.mHeader;
    mRaInfo.mHeaderUpdateTime = TimerMilli::GetNow();

    if (aRouterAdvertMessage == nullptr || aRouterAdvertMessage->GetHeader().GetRouterLifetime() == 0)
    {
        mRaInfo.mHeader.SetToDefault();
        mRaInfo.mIsHeaderFromHost = false;
    }
    else
    {
        // The checksum is set to zero in `mRaInfo.mHeader`
        // which indicates to platform that it needs to do the
        // calculation and update it.

        mRaInfo.mHeader = aRouterAdvertMessage->GetHeader();
        mRaInfo.mHeader.SetChecksum(0);
        mRaInfo.mIsHeaderFromHost = true;
    }

    ResetDiscoveredPrefixStaleTimer();

    if (mRaInfo.mHeader != oldHeader)
    {
        // If there was a change to the header, start timer to
        // reevaluate routing policy and send RA message with new
        // header.

        ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
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
    if (mRaInfo.mIsHeaderFromHost)
    {
        TimeMilli raStaleTime = Max(now, mRaInfo.mHeaderUpdateTime + Time::SecToMsec(kRtrAdvStaleTime));

        nextStaleTime = Min(nextStaleTime, raStaleTime);
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
    , mTimer(aInstance)
    , mSignalTask(aInstance)
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

        entry->SetFrom(aRaHeader);
        aRouter.mEntries.Push(*entry);
    }
    else
    {
        entry->SetFrom(aRaHeader);
    }

    mTimer.FireAtIfEarlier(entry->GetExpireTime());
    Get<RoutingManager>().EvaluatePublishingPrefix(entry->GetPrefix());

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

        entry->SetFrom(aPio);
        aRouter.mEntries.Push(*entry);
    }
    else
    {
        Entry newEntry;

        newEntry.SetFrom(aPio);
        entry->AdoptValidAndPreferredLiftimesFrom(newEntry);
    }

    mTimer.FireAtIfEarlier(entry->GetExpireTime());
    Get<RoutingManager>().EvaluatePublishingPrefix(entry->GetPrefix());

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

        entry->SetFrom(aRio);
        aRouter.mEntries.Push(*entry);
    }
    else
    {
        entry->SetFrom(aRio);
    }

    mTimer.FireAtIfEarlier(entry->GetExpireTime());
    Get<RoutingManager>().EvaluatePublishingPrefix(entry->GetPrefix());

    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::SetAllowDefaultRouteInNetData(bool aAllow)
{
    Ip6::Prefix prefix;

    VerifyOrExit(aAllow != mAllowDefaultRouteInNetData);

    LogInfo("Allow default route in netdata: %s -> %s", ToYesNo(mAllowDefaultRouteInNetData), ToYesNo(aAllow));

    mAllowDefaultRouteInNetData = aAllow;

    prefix.Clear();
    Get<RoutingManager>().EvaluatePublishingPrefix(prefix);

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

void RoutingManager::DiscoveredPrefixTable::RemoveOnLinkPrefix(const Ip6::Prefix &aPrefix)
{
    RemovePrefix(Entry::Matcher(aPrefix, Entry::kTypeOnLink));
}

void RoutingManager::DiscoveredPrefixTable::RemoveRoutePrefix(const Ip6::Prefix &aPrefix)
{
    RemovePrefix(Entry::Matcher(aPrefix, Entry::kTypeRoute));
}

void RoutingManager::DiscoveredPrefixTable::RemovePrefix(const Entry::Matcher &aMatcher)
{
    // Removes all entries matching a given prefix from the table.

    LinkedList<Entry> removedEntries;

    for (Router &router : mRouters)
    {
        router.mEntries.RemoveAllMatching(aMatcher, removedEntries);
    }

    VerifyOrExit(!removedEntries.IsEmpty());

    FreeEntries(removedEntries);
    RemoveRoutersWithNoEntries();

    Get<RoutingManager>().EvaluatePublishingPrefix(aMatcher.mPrefix);

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
            Get<RoutingManager>().UnpublishExternalRoute(entry->GetPrefix());
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
            TimeMilli entryStaleTime = Max(aNow, entry.GetStaleTime());

            if (entry.IsOnLinkPrefix() && !entry.IsDeprecated())
            {
                onLinkStaleTime = Max(onLinkStaleTime, entryStaleTime);
                foundOnLink     = true;
            }

            if (!entry.IsOnLinkPrefix())
            {
                routeStaleTime = Min(routeStaleTime, entryStaleTime);
            }
        }
    }

    return foundOnLink ? Min(onLinkStaleTime, routeStaleTime) : routeStaleTime;
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

const RoutingManager::DiscoveredPrefixTable::Entry *RoutingManager::DiscoveredPrefixTable::FindFavoredEntryToPublish(
    const Ip6::Prefix &aPrefix) const
{
    // Finds the favored entry matching a given `aPrefix` in the table
    // to publish in the Network Data. We can have multiple entries
    // in the table matching the same `aPrefix` from different
    // routers and potentially with different preference values. We
    // select the one with the highest preference as the favored
    // entry to publish.

    const Entry *favoredEntry = nullptr;

    for (const Router &router : mRouters)
    {
        for (const Entry &entry : router.mEntries)
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

bool RoutingManager::DiscoveredPrefixTable::ShouldPublish(NetworkData::ExternalRouteConfig &aRouteConfig) const
{
    bool         shouldPublish = false;
    const Entry *favoredEntry;

    if (aRouteConfig.GetPrefix().GetLength() == 0)
    {
        // If the change is to default route ::/0 prefix, make sure we
        // are allowed to publish default route in Network Data.

        VerifyOrExit(mAllowDefaultRouteInNetData);
    }

    favoredEntry = FindFavoredEntryToPublish(aRouteConfig.GetPrefix());
    VerifyOrExit(favoredEntry != nullptr);

    shouldPublish            = true;
    aRouteConfig.mPreference = favoredEntry->GetPreference();

exit:
    return shouldPublish;
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
        Get<RoutingManager>().EvaluatePublishingPrefix(expiredEntry.GetPrefix());
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
            nextExpireTime = Min(nextExpireTime, entry.GetExpireTime());
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

void RoutingManager::DiscoveredPrefixTable::Entry::SetFrom(const Ip6::Nd::RouterAdvertMessage::Header &aRaHeader)
{
    mPrefix.Clear();
    mType                    = kTypeRoute;
    mValidLifetime           = aRaHeader.GetRouterLifetime();
    mShared.mRoutePreference = aRaHeader.GetDefaultRouterPreference();
    mLastUpdateTime          = TimerMilli::GetNow();
}

void RoutingManager::DiscoveredPrefixTable::Entry::SetFrom(const Ip6::Nd::PrefixInfoOption &aPio)
{
    aPio.GetPrefix(mPrefix);
    mType                      = kTypeOnLink;
    mValidLifetime             = aPio.GetValidLifetime();
    mShared.mPreferredLifetime = aPio.GetPreferredLifetime();
    mLastUpdateTime            = TimerMilli::GetNow();
}

void RoutingManager::DiscoveredPrefixTable::Entry::SetFrom(const Ip6::Nd::RouteInfoOption &aRio)
{
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
    uint32_t delay = Min(kRtrAdvStaleTime, IsOnLinkPrefix() ? GetPreferredLifetime() : mValidLifetime);

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

void RoutingManager::OmrPrefix::SetFrom(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig)
{
    mPrefix         = aOnMeshPrefixConfig.GetPrefix();
    mPreference     = aOnMeshPrefixConfig.GetPreference();
    mIsDomainPrefix = aOnMeshPrefixConfig.mDp;
}

void RoutingManager::OmrPrefix::SetFrom(const LocalOmrPrefix &aLocalOmrPrefix)
{
    mPrefix         = aLocalOmrPrefix.GetPrefix();
    mPreference     = aLocalOmrPrefix.GetPreference();
    mIsDomainPrefix = false;
}

bool RoutingManager::OmrPrefix::IsFavoredOver(const NetworkData::OnMeshPrefixConfig &aOmrPrefixConfig) const
{
    // This method determines whether this OMR prefix is favored
    // over another prefix. A prefix with higher preference is
    // favored. If the preference is the same, then the smaller
    // prefix (in the sense defined by `Ip6::Prefix`) is favored.

    bool isFavored = (mPreference > aOmrPrefixConfig.GetPreference());

    OT_ASSERT(IsValidOmrPrefix(aOmrPrefixConfig));

    if (mPreference == aOmrPrefixConfig.GetPreference())
    {
        isFavored = (mPrefix < aOmrPrefixConfig.GetPrefix());
    }

    return isFavored;
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
    config.mPreference   = GetPreference();

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

//---------------------------------------------------------------------------------------------------------------------
// LocalOnLinkPrefix

RoutingManager::LocalOnLinkPrefix::LocalOnLinkPrefix(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kIdle)
    , mTimer(aInstance)
{
    mPrefix.Clear();
    mOldPrefix.Clear();
}

void RoutingManager::LocalOnLinkPrefix::Generate(void)
{
    MeshCoP::ExtendedPanId extPanId = Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId();

    mPrefix.mPrefix.mFields.m8[0] = 0xfd;
    // Global ID: 40 most significant bits of Extended PAN ID
    memcpy(mPrefix.mPrefix.mFields.m8 + 1, extPanId.m8, 5);
    // Subnet ID: 16 least significant bits of Extended PAN ID
    memcpy(mPrefix.mPrefix.mFields.m8 + 6, extPanId.m8 + 6, 2);
    mPrefix.SetLength(kOnLinkPrefixLength);

    LogNote("Local on-link prefix: %s", mPrefix.ToString().AsCString());
}

void RoutingManager::LocalOnLinkPrefix::Start(void)
{
    mState = kIdle;
}

void RoutingManager::LocalOnLinkPrefix::Stop(void)
{
    mTimer.Stop();

    if (mOldPrefix.GetLength() != 0)
    {
        Get<RoutingManager>().UnpublishExternalRoute(mOldPrefix);
        mOldPrefix.Clear();
    }

    VerifyOrExit(mState != kIdle);

    Get<RoutingManager>().UnpublishExternalRoute(mPrefix);

    if (mState == kPublishing)
    {
        // If we are waiting for prefix to be added in Network Data
        // and not yet advertised it in any RA, there is no need to
        // deprecate it and we can directly go to `kIdle` state.
        mState = kIdle;
        ExitNow();
    }

    mState = kDeprecating;

    // Start deprecating the local on-link prefix to send a PIO
    // with zero preferred lifetime in the next call to
    // `SendRouterAdvertisement()`.

exit:
    return;
}

void RoutingManager::LocalOnLinkPrefix::PublishAndAdvertise(void)
{
    // Start publishing and advertising the local on-link prefix if
    // not already.

    switch (mState)
    {
    case kIdle:
    case kDeprecating:
        break;

    case kPublishing:
    case kAdvertising:
        ExitNow();
    }

    mState = kPublishing;
    LogInfo("Publishing local on-link prefix %s in netdata", mPrefix.ToString().AsCString());

    Get<RoutingManager>().EvaluatePublishingPrefix(mPrefix);

    // We wait for the prefix to be added in Network Data before
    // starting to advertise it in RAs. However, if it is already
    // present in Network Data (e.g., added by another BR on the same
    // Thread mesh), we can immediately start advertising it.

    if (Get<RoutingManager>().NetworkDataContainsExternalRoute(mPrefix))
    {
        EnterAdvertisingState();
    }

exit:
    return;
}

void RoutingManager::LocalOnLinkPrefix::Deprecate(void)
{
    // Deprecate the local on-link prefix if it was being advertised
    // before. While depreciating the prefix, we wait for the lifetime
    // timer to expire before unpublishing the prefix from the Network
    // Data. We also continue to include it as a PIO in the RA message
    // with zero preferred lifetime and the remaining valid lifetime
    // until the timer expires.

    switch (mState)
    {
    case kPublishing:
        mState = kIdle;
        Get<RoutingManager>().EvaluatePublishingPrefix(mPrefix);
        mState = kIdle;
        break;

    case kAdvertising:
        mState = kDeprecating;
        mTimer.FireAtIfEarlier(mExpireTime);
        LogInfo("Deprecate local on-link prefix %s", mPrefix.ToString().AsCString());
        break;

    case kIdle:
    case kDeprecating:
        ExitNow();
    }

exit:
    return;
}

bool RoutingManager::LocalOnLinkPrefix::ShouldPublish(NetworkData::ExternalRouteConfig &aRouteConfig) const
{
    bool shouldPublish = false;

    if (aRouteConfig.GetPrefix() == mPrefix)
    {
        switch (mState)
        {
        case kIdle:
            break;
        case kPublishing:
        case kAdvertising:
        case kDeprecating:
            shouldPublish            = true;
            aRouteConfig.mPreference = NetworkData::kRoutePreferenceMedium;
            break;
        }
    }
    else if ((mOldPrefix.GetLength() != 0) && (aRouteConfig.GetPrefix() == mOldPrefix))
    {
        shouldPublish            = true;
        aRouteConfig.mPreference = NetworkData::kRoutePreferenceMedium;
    }

    return shouldPublish;
}

void RoutingManager::LocalOnLinkPrefix::EnterAdvertisingState(void)
{
    mState      = kAdvertising;
    mExpireTime = TimerMilli::GetNow() + TimeMilli::SecToMsec(kDefaultOnLinkPrefixLifetime);

    LogInfo("Start advertising local on-link prefix %s", mPrefix.ToString().AsCString());
}

bool RoutingManager::LocalOnLinkPrefix::IsPublishingOrAdvertising(void) const
{
    return (mState == kPublishing) || (mState == kAdvertising);
}

void RoutingManager::LocalOnLinkPrefix::AppendAsPiosTo(Ip6::Nd::RouterAdvertMessage &aRaMessage)
{
    AppendCurPrefix(aRaMessage);
    AppendOldPrefix(aRaMessage);
}

void RoutingManager::LocalOnLinkPrefix::AppendCurPrefix(Ip6::Nd::RouterAdvertMessage &aRaMessage)
{
    // Append the local on-link prefix to the `aRaMessage` as a PIO
    // only if it is being advertised or deprecated.
    //
    // If in `kAdvertising` state, we reset the expire time.
    // If in `kDeprecating` state, we include it as PIO with zero
    // preferred lifetime and the remaining valid lifetime.

    uint32_t  validLifetime     = kDefaultOnLinkPrefixLifetime;
    uint32_t  preferredLifetime = kDefaultOnLinkPrefixLifetime;
    TimeMilli now               = TimerMilli::GetNow();

    switch (mState)
    {
    case kAdvertising:
        mExpireTime = now + TimeMilli::SecToMsec(kDefaultOnLinkPrefixLifetime);
        break;

    case kDeprecating:
        VerifyOrExit(mExpireTime > now);
        validLifetime     = TimeMilli::MsecToSec(mExpireTime - now);
        preferredLifetime = 0;
        break;

    case kIdle:
    case kPublishing:
        ExitNow();
    }

    SuccessOrAssert(aRaMessage.AppendPrefixInfoOption(mPrefix, validLifetime, preferredLifetime));

    LogInfo("RouterAdvert: Added PIO for %s (valid=%u, preferred=%u)", mPrefix.ToString().AsCString(), validLifetime,
            preferredLifetime);

exit:
    return;
}

void RoutingManager::LocalOnLinkPrefix::AppendOldPrefix(Ip6::Nd::RouterAdvertMessage &aRaMessage)
{
    TimeMilli now = TimerMilli::GetNow();
    uint32_t  validLifetime;

    VerifyOrExit((mOldPrefix.GetLength() != 0) && (mOldExpireTime > now));

    validLifetime = TimeMilli::MsecToSec(mOldExpireTime - now);
    SuccessOrAssert(aRaMessage.AppendPrefixInfoOption(mOldPrefix, validLifetime, 0));

    LogInfo("RouterAdvert: Added PIO for %s (valid=%u, preferred=0)", mOldPrefix.ToString().AsCString(), validLifetime);

exit:
    return;
}

void RoutingManager::LocalOnLinkPrefix::HandleNetDataChange(void)
{
    VerifyOrExit(mState == kPublishing);

    if (Get<RoutingManager>().NetworkDataContainsExternalRoute(mPrefix))
    {
        EnterAdvertisingState();
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
    }

exit:
    return;
}

void RoutingManager::LocalOnLinkPrefix::HandleExtPanIdChange(void)
{
    // If the prefix is advertised or being deprecated we remember it
    // as `mOldPrefix` and deprecate it. It will be included in
    // emitted RAs as PIO with zero preferred lifetime. It will still
    // be present in Network Data until its expire time so to allow
    // Thread nodes to continue to communicate with `InfraIf` devices
    // using addresses based on this prefix.

    switch (mState)
    {
    case kIdle:
        break;

    case kPublishing:
        Get<RoutingManager>().UnpublishExternalRoute(mPrefix);
        break;

    case kAdvertising:
    case kDeprecating:
        if (mOldPrefix.GetLength() != 0)
        {
            Ip6::Prefix prevPrefix = mOldPrefix;

            mOldPrefix.Clear();
            Get<RoutingManager>().EvaluatePublishingPrefix(prevPrefix);
        }

        mOldPrefix     = mPrefix;
        mOldExpireTime = mExpireTime;
        mTimer.FireAtIfEarlier(mOldExpireTime);
        break;
    }

    mState = kIdle;
    Generate();
}

void RoutingManager::LocalOnLinkPrefix::HandleTimer(void)
{
    TimeMilli now = TimerMilli::GetNow();

    if ((mState == kDeprecating) && (now >= mExpireTime))
    {
        LogInfo("Local on-link prefix %s expired", mPrefix.ToString().AsCString());
        mState = kIdle;
        Get<RoutingManager>().EvaluatePublishingPrefix(mPrefix);
    }

    if ((mOldPrefix.GetLength() != 0) && (now >= mOldExpireTime))
    {
        Ip6::Prefix oldPrefix = mOldPrefix;

        LogInfo("Old local on-link prefix %s expired", mOldPrefix.ToString().AsCString());
        mOldPrefix.Clear();
        Get<RoutingManager>().EvaluatePublishingPrefix(oldPrefix);
    }

    // Re-schedule the timer

    if (mState == kDeprecating)
    {
        mTimer.FireAt(mExpireTime);
    }

    if (mOldPrefix.GetLength() != 0)
    {
        mTimer.FireAtIfEarlier(mOldExpireTime);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// OnMeshPrefixArray

void RoutingManager::OnMeshPrefixArray::Add(const OnMeshPrefix &aPrefix)
{
    // Checks if `aPrefix` is already present in the array and if not
    // adds it as new entry.

    Error error;

    VerifyOrExit(!Contains(aPrefix));

    error = PushBack(aPrefix);

    if (error != kErrorNone)
    {
        LogWarn("Too many on-mesh prefixes in net data, ignoring prefix %s", aPrefix.ToString().AsCString());
    }

exit:
    return;
}

void RoutingManager::OnMeshPrefixArray::MarkAsDeleted(const OnMeshPrefix &aPrefix)
{
    // Searches for a matching entry to `aPrefix` and if found marks
    // it as deleted by setting prefix length to zero.

    OnMeshPrefix *entry = Find(aPrefix);

    if (entry != nullptr)
    {
        entry->SetLength(0);
    }
}

#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// Nat64PrefixManager

RoutingManager::Nat64PrefixManager::Nat64PrefixManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
    , mTimer(aInstance)
{
    mInfraIfPrefix.Clear();
    mLocalPrefix.Clear();
    mPublishedPrefix.Clear();
}

void RoutingManager::Nat64PrefixManager::SetEnabled(bool aEnabled)
{
    VerifyOrExit(mEnabled != aEnabled);
    mEnabled = aEnabled;

    if (aEnabled)
    {
        if (Get<RoutingManager>().IsRunning())
        {
            Start();
        }
    }
    else
    {
        Stop();
    }

exit:
    return;
}

void RoutingManager::Nat64PrefixManager::Start(void)
{
    VerifyOrExit(mEnabled);
    LogInfo("Starting Nat64PrefixManager");
    mTimer.Start(0);

exit:
    return;
}

void RoutingManager::Nat64PrefixManager::Stop(void)
{
    LogInfo("Stopping Nat64PrefixManager");

    if (mPublishedPrefix.IsValidNat64())
    {
        Get<RoutingManager>().UnpublishExternalRoute(mPublishedPrefix);
    }

    mPublishedPrefix.Clear();
    mInfraIfPrefix.Clear();
    mTimer.Stop();

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    Get<Nat64::Translator>().SetNat64Prefix(mPublishedPrefix);
#endif
}

void RoutingManager::Nat64PrefixManager::GenerateLocalPrefix(const Ip6::Prefix &aBrUlaPrefix)
{
    mLocalPrefix = aBrUlaPrefix;
    mLocalPrefix.SetSubnetId(kNat64PrefixSubnetId);
    mLocalPrefix.mPrefix.mFields.m32[2] = 0;
    mLocalPrefix.SetLength(kNat64PrefixLength);

    LogInfo("Generated local NAT64 prefix: %s", mLocalPrefix.ToString().AsCString());
}

const Ip6::Prefix &RoutingManager::Nat64PrefixManager::GetFavoredPrefix(RoutePreference &aPreference) const
{
    const Ip6::Prefix *favoredPrefix = &mInfraIfPrefix;

    if (mInfraIfPrefix.IsValidNat64())
    {
        aPreference = NetworkData::kRoutePreferenceMedium;
    }
    else
    {
        favoredPrefix = &mLocalPrefix;
        aPreference   = NetworkData::kRoutePreferenceLow;
    }

    return *favoredPrefix;
}

void RoutingManager::Nat64PrefixManager::Evaluate(void)
{
    Error                            error;
    Ip6::Prefix                      prefix;
    RoutePreference                  preference;
    NetworkData::ExternalRouteConfig netdataPrefixConfig;
    bool                             shouldPublish;

    VerifyOrExit(mEnabled);

    LogInfo("Evaluating NAT64 prefix");

    prefix = GetFavoredPrefix(preference);

    error = Get<NetworkData::Leader>().GetPreferredNat64Prefix(netdataPrefixConfig);

    // NAT64 prefix is expected to be published from this BR
    // when one of the following is true:
    //
    // - No NAT64 prefix in Network Data.
    // - The preferred NAT64 prefix in Network Data has lower
    //   preference than this BR's prefix.
    // - The preferred NAT64 prefix in Network Data was published
    //   by this BR.
    // - The preferred NAT64 prefix in Network Data is same as the
    //   discovered infrastructure prefix.
    //
    // TODO: change to check RLOC16 to determine if the NAT64 prefix
    // was published by this BR.

    shouldPublish =
        ((error == kErrorNotFound) || (netdataPrefixConfig.mPreference < preference) ||
         (netdataPrefixConfig.GetPrefix() == mPublishedPrefix) || (netdataPrefixConfig.GetPrefix() == mInfraIfPrefix));

    if (mPublishedPrefix.IsValidNat64() && (!shouldPublish || (prefix != mPublishedPrefix)))
    {
        Ip6::Prefix prevPrefix;

        prevPrefix = mPublishedPrefix;
        mPublishedPrefix.Clear();
        Get<RoutingManager>().EvaluatePublishingPrefix(prevPrefix);
    }

    if (shouldPublish && ((prefix != mPublishedPrefix) || (preference != mPublishedPreference)))
    {
        mPublishedPrefix     = prefix;
        mPublishedPreference = preference;
        Get<RoutingManager>().EvaluatePublishingPrefix(mPublishedPrefix);
    }

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    Get<Nat64::Translator>().SetNat64Prefix(mPublishedPrefix);
#endif

exit:
    return;
}

bool RoutingManager::Nat64PrefixManager::ShouldPublish(NetworkData::ExternalRouteConfig &aRouteConfig) const
{
    bool shouldPublish = false;

    VerifyOrExit(mPublishedPrefix.IsValidNat64());
    VerifyOrExit(mPublishedPrefix == aRouteConfig.GetPrefix());

    shouldPublish            = true;
    aRouteConfig.mPreference = mPublishedPreference;
    aRouteConfig.mNat64      = true;

exit:
    return shouldPublish;
}

void RoutingManager::Nat64PrefixManager::HandleTimer(void)
{
    OT_ASSERT(mEnabled);

    Discover();

    mTimer.Start(TimeMilli::SecToMsec(kDefaultNat64PrefixLifetime));
    LogInfo("NAT64 prefix timer scheduled in %u seconds", kDefaultNat64PrefixLifetime);
}

void RoutingManager::Nat64PrefixManager::Discover(void)
{
    Error error = Get<RoutingManager>().mInfraIf.DiscoverNat64Prefix();

    if (error == kErrorNone)
    {
        LogInfo("Discovering infraif NAT64 prefix");
    }
    else
    {
        LogWarn("Failed to discover infraif NAT64 prefix: %s", ErrorToString(error));
    }
}

void RoutingManager::Nat64PrefixManager::HandleDiscoverDone(const Ip6::Prefix &aPrefix)
{
    mInfraIfPrefix = aPrefix;

    LogInfo("Infraif NAT64 prefix: %s", mInfraIfPrefix.IsValidNat64() ? mInfraIfPrefix.ToString().AsCString() : "none");

    if (Get<RoutingManager>().mIsRunning)
    {
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
    }
}

Nat64::State RoutingManager::Nat64PrefixManager::GetState(void) const
{
    Nat64::State state = Nat64::kStateDisabled;

    VerifyOrExit(mEnabled);
    VerifyOrExit(Get<RoutingManager>().IsRunning(), state = Nat64::kStateNotRunning);
    VerifyOrExit(mPublishedPrefix.IsValidNat64(), state = Nat64::kStateIdle);
    state = Nat64::kStateActive;

exit:
    return state;
}

#endif // OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// RsSender

RoutingManager::RsSender::RsSender(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTxCount(0)
    , mTimer(aInstance)
{
}

void RoutingManager::RsSender::Start(void)
{
    uint32_t delay;

    VerifyOrExit(!IsInProgress());

    delay = Random::NonCrypto::GetUint32InRange(0, kMaxStartDelay);
    LogInfo("Scheduled Router Solicitation in %u milliseconds", delay);

    mTxCount   = 0;
    mStartTime = TimerMilli::GetNow();
    mTimer.Start(delay);

exit:
    return;
}

void RoutingManager::RsSender::Stop(void)
{
    mTimer.Stop();
}

Error RoutingManager::RsSender::SendRs(void)
{
    Ip6::Address                  destAddress;
    Ip6::Nd::RouterSolicitMessage routerSolicit;
    InfraIf::Icmp6Packet          packet;

    packet.InitFrom(routerSolicit);
    destAddress.SetToLinkLocalAllRoutersMulticast();

    return Get<RoutingManager>().mInfraIf.Send(packet, destAddress);
}

void RoutingManager::RsSender::HandleTimer(void)
{
    Error    error;
    uint32_t delay;

    if (mTxCount >= kMaxTxCount)
    {
        Get<RoutingManager>().HandleRsSenderFinished(mStartTime);
        ExitNow();
    }

    error = SendRs();

    if (error == kErrorNone)
    {
        mTxCount++;
        LogInfo("Successfully sent RS %d/%d", mTxCount, kMaxTxCount);
        delay = (mTxCount == kMaxTxCount) ? kWaitOnLastAttempt : kTxInterval;
    }
    else
    {
        LogCrit("Failed to send RS %d, error:%s", mTxCount + 1, ErrorToString(error));

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
