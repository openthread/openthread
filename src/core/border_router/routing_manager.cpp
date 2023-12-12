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
#include <openthread/platform/border_routing.h>
#include <openthread/platform/infra_if.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "instance/instance.hpp"
#include "meshcop/extended_panid.hpp"
#include "net/ip6.hpp"
#include "net/nat64_translator.hpp"
#include "net/nd6.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"

namespace ot {

namespace BorderRouter {

RegisterLogModule("RoutingManager");

RoutingManager::RoutingManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsRunning(false)
    , mIsEnabled(false)
    , mInfraIf(aInstance)
    , mOmrPrefixManager(aInstance)
    , mRioAdvertiser(aInstance)
    , mOnLinkPrefixManager(aInstance)
    , mDiscoveredPrefixTable(aInstance)
    , mRoutePublisher(aInstance)
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    , mNat64PrefixManager(aInstance)
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    , mPdPrefixManager(aInstance)
#endif
    , mRsSender(aInstance)
    , mDiscoveredPrefixStaleTimer(aInstance)
    , mRoutingPolicyTimer(aInstance)
{
    mBrUlaPrefix.Clear();
}

Error RoutingManager::Init(uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    Error error;

    VerifyOrExit(GetState() == kStateUninitialized || GetState() == kStateDisabled, error = kErrorInvalidState);

    if (!mInfraIf.IsInitialized())
    {
        LogInfo("Initializing - InfraIfIndex:%lu", ToUlong(aInfraIfIndex));
        SuccessOrExit(error = mInfraIf.Init(aInfraIfIndex));
        SuccessOrExit(error = LoadOrGenerateRandomBrUlaPrefix());
        mOmrPrefixManager.Init(mBrUlaPrefix);
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
        mNat64PrefixManager.GenerateLocalPrefix(mBrUlaPrefix);
#endif
        mOnLinkPrefixManager.Init();
    }
    else if (aInfraIfIndex != mInfraIf.GetIfIndex())
    {
        LogInfo("Reinitializing - InfraIfIndex:%lu -> %lu", ToUlong(mInfraIf.GetIfIndex()), ToUlong(aInfraIfIndex));
        mInfraIf.SetIfIndex(aInfraIfIndex);
    }

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
    LogInfo("%s", mIsEnabled ? "Enabling" : "Disabling");
    EvaluateState();

exit:
    return error;
}

RoutingManager::State RoutingManager::GetState(void) const
{
    State state = kStateUninitialized;

    VerifyOrExit(IsInitialized());
    VerifyOrExit(IsEnabled(), state = kStateDisabled);

    state = IsRunning() ? kStateRunning : kStateStopped;

exit:
    return state;
}

Error RoutingManager::GetOmrPrefix(Ip6::Prefix &aPrefix) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mOmrPrefixManager.GetGeneratedPrefix();

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
Error RoutingManager::GetPdOmrPrefix(PrefixTableEntry &aPrefixInfo) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    error = mPdPrefixManager.GetPrefixInfo(aPrefixInfo);

exit:
    return error;
}

Error RoutingManager::GetPdProcessedRaInfo(PdProcessedRaInfo &aPdProcessedRaInfo)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    error = mPdPrefixManager.GetProcessedRaInfo(aPdProcessedRaInfo);

exit:
    return error;
}
#endif

Error RoutingManager::GetFavoredOmrPrefix(Ip6::Prefix &aPrefix, RoutePreference &aPreference) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsRunning(), error = kErrorInvalidState);
    aPrefix     = mOmrPrefixManager.GetFavoredPrefix().GetPrefix();
    aPreference = mOmrPrefixManager.GetFavoredPrefix().GetPreference();

exit:
    return error;
}

Error RoutingManager::GetOnLinkPrefix(Ip6::Prefix &aPrefix) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mOnLinkPrefixManager.GetLocalPrefix();

exit:
    return error;
}

Error RoutingManager::GetFavoredOnLinkPrefix(Ip6::Prefix &aPrefix) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    aPrefix = mOnLinkPrefixManager.GetFavoredDiscoveredPrefix();

    if (aPrefix.GetLength() == 0)
    {
        aPrefix = mOnLinkPrefixManager.GetLocalPrefix();
    }

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
        LogInfo("Starting");

        mIsRunning = true;
        UpdateDiscoveredPrefixTableOnNetDataChange();
        mOnLinkPrefixManager.Start();
        mOmrPrefixManager.Start();
        mRoutePublisher.Start();
        mRsSender.Start();
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
        mPdPrefixManager.Start();
#endif
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
        mNat64PrefixManager.Start();
#endif
    }
}

void RoutingManager::Stop(void)
{
    VerifyOrExit(mIsRunning);

    mOmrPrefixManager.Stop();
    mOnLinkPrefixManager.Stop();
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    mPdPrefixManager.Stop();
#endif
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    mNat64PrefixManager.Stop();
#endif

    SendRouterAdvertisement(kInvalidateAllPrevPrefixes);

    mDiscoveredPrefixTable.RemoveAllEntries();
    mDiscoveredPrefixStaleTimer.Stop();

    mRaInfo.mTxCount = 0;

    mRsSender.Stop();

    mRoutingPolicyTimer.Stop();

    mRoutePublisher.Stop();

    LogInfo("Stopped");

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
    case Ip6::Icmp::Header::kTypeNeighborAdvert:
        HandleNeighborAdvertisement(aPacket);
        break;
    default:
        break;
    }

exit:
    return;
}

void RoutingManager::HandleNotifierEvents(Events aEvents)
{
    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        mRioAdvertiser.HandleRoleChanged();
    }

    mRoutePublisher.HandleNotifierEvents(aEvents);

    VerifyOrExit(IsInitialized() && IsEnabled());

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        EvaluateState();
    }

    if (mIsRunning && aEvents.Contains(kEventThreadNetdataChanged))
    {
        UpdateDiscoveredPrefixTableOnNetDataChange();
        mOnLinkPrefixManager.HandleNetDataChange();
        ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
    }

    if (aEvents.Contains(kEventThreadExtPanIdChanged))
    {
        mOnLinkPrefixManager.HandleExtPanIdChange();
    }

exit:
    return;
}

void RoutingManager::UpdateDiscoveredPrefixTableOnNetDataChange(void)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;

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
    }
}

// This method evaluate the routing policy depends on prefix and route
// information on Thread Network and infra link. As a result, this
// method May send RA messages on infra link and publish/unpublish
// OMR and NAT64 prefix in the Thread network.
void RoutingManager::EvaluateRoutingPolicy(void)
{
    OT_ASSERT(mIsRunning);

    LogInfo("Evaluating routing policy");

    mOnLinkPrefixManager.Evaluate();
    mOmrPrefixManager.Evaluate();
    mRoutePublisher.Evaluate();
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

    return mIsRunning && !mOmrPrefixManager.GetFavoredPrefix().IsEmpty() &&
           mOnLinkPrefixManager.IsInitalEvaluationDone();
}

void RoutingManager::ScheduleRoutingPolicyEvaluation(ScheduleMode aMode)
{
    TimeMilli now   = TimerMilli::GetNow();
    uint32_t  delay = 0;
    TimeMilli evaluateTime;

    VerifyOrExit(mIsRunning);

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

    mRoutingPolicyTimer.FireAtIfEarlier(evaluateTime);

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
    {
        uint32_t duration = evaluateTime - now;

        if (duration == 0)
        {
            LogInfo("Will evaluate routing policy immediately");
        }
        else
        {
            String<Uptime::kStringSize> string;

            Uptime::UptimeToString(duration, string, /* aIncludeMsec */ true);
            LogInfo("Will evaluate routing policy in %s (%lu msec)", string.AsCString() + 3, ToUlong(duration));
        }
    }
#endif

exit:
    return;
}

void RoutingManager::SendRouterAdvertisement(RouterAdvTxMode aRaTxMode)
{
    // RA message max length is derived to accommodate:
    //
    // - The RA header.
    // - One RA Flags Extensions Option (with stub router flag).
    // - One PIO for current local on-link prefix.
    // - At most `kMaxOldPrefixes` for old deprecating on-link prefixes.
    // - At most 3 times `kMaxOnMeshPrefixes` RIO for on-mesh prefixes.
    //   Factor three is used for RIOs to account for any new prefix
    //   with older prefixes entries being deprecated and prefixes
    //   being invalidated.

    static constexpr uint16_t kMaxRaLength =
        sizeof(Ip6::Nd::RouterAdvertMessage::Header) + sizeof(Ip6::Nd::RaFlagsExtOption) +
        sizeof(Ip6::Nd::PrefixInfoOption) + sizeof(Ip6::Nd::PrefixInfoOption) * OnLinkPrefixManager::kMaxOldPrefixes +
        3 * kMaxOnMeshPrefixes * (sizeof(Ip6::Nd::RouteInfoOption) + sizeof(Ip6::Prefix));

    uint8_t                      buffer[kMaxRaLength];
    Ip6::Nd::RouterAdvertMessage raMsg(mRaInfo.mHeader, buffer);

    LogInfo("Preparing RA");

    mDiscoveredPrefixTable.DetermineAndSetFlags(raMsg);

    LogInfo("- RA Header - flags - M:%u O:%u", raMsg.GetHeader().IsManagedAddressConfigFlagSet(),
            raMsg.GetHeader().IsOtherConfigFlagSet());
    LogInfo("- RA Header - default route - lifetime:%u", raMsg.GetHeader().GetRouterLifetime());

#if OPENTHREAD_CONFIG_BORDER_ROUTING_STUB_ROUTER_FLAG_IN_EMITTED_RA_ENABLE
    SuccessOrAssert(raMsg.AppendFlagsExtensionOption(/* aStubRouterFlag */ true));
    LogInfo("- FlagsExt - StubRouter:1");
#endif

    // Append PIO for local on-link prefix if is either being
    // advertised or deprecated and for old prefix if is being
    // deprecated.

    mOnLinkPrefixManager.AppendAsPiosTo(raMsg);

    if (aRaTxMode == kInvalidateAllPrevPrefixes)
    {
        mRioAdvertiser.InvalidatPrevRios(raMsg);
    }
    else
    {
        mRioAdvertiser.AppendRios(raMsg);
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
            Get<Ip6::Ip6>().GetBorderRoutingCounters().mRaTxSuccess++;
            LogInfo("Sent RA on %s", mInfraIf.ToString().AsCString());
            DumpDebg("[BR-CERT] direction=send | type=RA |", raMsg.GetAsPacket().GetBytes(),
                     raMsg.GetAsPacket().GetLength());
        }
        else
        {
            Get<Ip6::Ip6>().GetBorderRoutingCounters().mRaTxFailure++;
            LogWarn("Failed to send RA on %s: %s", mInfraIf.ToString().AsCString(), ErrorToString(error));
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
                VerifyOrExit(prefix == mOnLinkPrefixManager.GetLocalPrefix());
            }

            break;
        }

        case Ip6::Nd::Option::kTypeRouteInfo:
        {
            // RIO (with non-zero lifetime) should match entries from
            // `mRioAdvertiser`. We keep track of the number of matched
            // RIOs and check after the loop ends that all entries were
            // seen.

            const Ip6::Nd::RouteInfoOption &rio = static_cast<const Ip6::Nd::RouteInfoOption &>(option);

            VerifyOrExit(rio.IsValid());
            rio.GetPrefix(prefix);

            if (rio.GetRouteLifetime() != 0)
            {
                VerifyOrExit(mRioAdvertiser.HasAdvertised(prefix));
                rioCount++;
            }

            break;
        }

        default:
            ExitNow();
        }
    }

    VerifyOrExit(rioCount == mRioAdvertiser.GetAdvertisedRioCount());

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
    return (aOnLinkPrefix.GetLength() == kOnLinkPrefixLength) && !aOnLinkPrefix.IsLinkLocal() &&
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

    Get<Ip6::Ip6>().GetBorderRoutingCounters().mRsRx++;
    LogInfo("Received RS from %s on %s", aSrcAddress.ToString().AsCString(), mInfraIf.ToString().AsCString());

    ScheduleRoutingPolicyEvaluation(kToReplyToRs);
}

void RoutingManager::HandleNeighborAdvertisement(const InfraIf::Icmp6Packet &aPacket)
{
    const Ip6::Nd::NeighborAdvertMessage *naMsg;

    VerifyOrExit(aPacket.GetLength() >= sizeof(naMsg));
    naMsg = reinterpret_cast<const Ip6::Nd::NeighborAdvertMessage *>(aPacket.GetBytes());

    mDiscoveredPrefixTable.ProcessNeighborAdvertMessage(*naMsg);

exit:
    return;
}

void RoutingManager::HandleRouterAdvertisement(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress)
{
    Ip6::Nd::RouterAdvertMessage routerAdvMessage(aPacket);

    OT_ASSERT(mIsRunning);

    VerifyOrExit(routerAdvMessage.IsValid());

    Get<Ip6::Ip6>().GetBorderRoutingCounters().mRaRx++;

    LogInfo("Received RA from %s on %s", aSrcAddress.ToString().AsCString(), mInfraIf.ToString().AsCString());
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
        LogInfo("- PIO %s - ignore since not a valid on-link prefix", aPrefix.ToString().AsCString());
        ExitNow();
    }

    if (mOnLinkPrefixManager.IsPublishingOrAdvertising())
    {
        VerifyOrExit(aPrefix != mOnLinkPrefixManager.GetLocalPrefix());
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
        LogInfo("- RIO %s - ignore since not a valid OMR prefix", aPrefix.ToString().AsCString());
        ExitNow();
    }

    VerifyOrExit(mOmrPrefixManager.GetLocalPrefix().GetPrefix() != aPrefix);

    // Ignore OMR prefixes advertised by ourselves or in current Thread Network Data.
    // The `RioAdvertiser` prefixes and the OMR prefix set in Network Data should eventually
    // be equal, but there is time that they are not synchronized immediately:
    // 1. Network Data could contain more OMR prefixes than `RioAdvertiser` because
    //    we added random delay before Evaluating routing policy when Network Data is changed.
    // 2. `RioAdvertiser` prefixes could contain more OMR prefixes than Network Data because
    //    it takes time to sync a new OMR prefix into Network Data (multicast loopback RA
    //    messages are usually faster than Thread Network Data propagation).
    // They are the reasons why we need both the checks.

    VerifyOrExit(!mRioAdvertiser.HasAdvertised(aPrefix));
    VerifyOrExit(!Get<RoutingManager>().NetworkDataContainsOmrPrefix(aPrefix));

    shouldProcess = true;

exit:
    return shouldProcess;
}

void RoutingManager::HandleDiscoveredPrefixTableChanged(void)
{
    // This is a callback from `mDiscoveredPrefixTable` indicating that
    // there has been a change in the table.

    VerifyOrExit(mIsRunning);

    ResetDiscoveredPrefixStaleTimer();
    mOnLinkPrefixManager.HandleDiscoveredPrefixTableChanged();
    mRoutePublisher.Evaluate();

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

bool RoutingManager::NetworkDataContainsUlaRoute(void) const
{
    // Determine whether leader Network Data contains a route
    // prefix which is either the ULA prefix `fc00::/7` or
    // a sub-prefix of it (e.g., default route).

    NetworkData::Iterator            iterator = NetworkData::kIteratorInit;
    NetworkData::ExternalRouteConfig routeConfig;
    bool                             contains = false;

    while (Get<NetworkData::Leader>().GetNextExternalRoute(iterator, routeConfig) == kErrorNone)
    {
        if (routeConfig.mStable && RoutePublisher::GetUlaPrefix().ContainsPrefix(routeConfig.GetPrefix()))
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
        LogDebg("Prefix stale timer scheduled in %lu ms", ToUlong(nextStaleTime - now));
    }
}

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)
void RoutingManager::LogPrefixInfoOption(const Ip6::Prefix &aPrefix,
                                         uint32_t           aValidLifetime,
                                         uint32_t           aPreferredLifetime)
{
    LogInfo("- PIO %s (valid:%lu, preferred:%lu)", aPrefix.ToString().AsCString(), ToUlong(aValidLifetime),
            ToUlong(aPreferredLifetime));
}

void RoutingManager::LogRouteInfoOption(const Ip6::Prefix &aPrefix, uint32_t aLifetime, RoutePreference aPreference)
{
    LogInfo("- RIO %s (lifetime:%lu, prf:%s)", aPrefix.ToString().AsCString(), ToUlong(aLifetime),
            RoutePreferenceToString(aPreference));
}
#else
void RoutingManager::LogPrefixInfoOption(const Ip6::Prefix &, uint32_t, uint32_t) {}
void RoutingManager::LogRouteInfoOption(const Ip6::Prefix &, uint32_t, RoutePreference) {}
#endif

//---------------------------------------------------------------------------------------------------------------------
// DiscoveredPrefixTable

RoutingManager::DiscoveredPrefixTable::DiscoveredPrefixTable(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEntryTimer(aInstance)
    , mRouterTimer(aInstance)
    , mSignalTask(aInstance)
{
}

void RoutingManager::DiscoveredPrefixTable::ProcessRouterAdvertMessage(const Ip6::Nd::RouterAdvertMessage &aRaMessage,
                                                                       const Ip6::Address                 &aSrcAddress)
{
    // Process a received RA message and update the prefix table.

    Router *router = mRouters.FindMatching(aSrcAddress);

    if (router == nullptr)
    {
        router = AllocateRouter();

        if (router == nullptr)
        {
            LogWarn("Received RA from too many routers, ignore RA from %s", aSrcAddress.ToString().AsCString());
            ExitNow();
        }

        router->Clear();
        router->mAddress = aSrcAddress;

        mRouters.Push(*router);
    }

    // RA message can indicate router provides default route in the RA
    // message header and can also include an RIO for `::/0`. When
    // processing an RA message, the preference and lifetime values
    // in a `::/0` RIO override the preference and lifetime values in
    // the RA header (per RFC 4191 section 3.1).

    ProcessRaHeader(aRaMessage.GetHeader(), *router);

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

        case Ip6::Nd::Option::kTypeRaFlagsExtension:
            ProcessRaFlagsExtOption(static_cast<const Ip6::Nd::RaFlagsExtOption &>(option), *router);
            break;

        default:
            break;
        }
    }

    UpdateRouterOnRx(*router);

    RemoveRoutersWithNoEntriesOrFlags();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::ProcessRaHeader(const Ip6::Nd::RouterAdvertMessage::Header &aRaHeader,
                                                            Router                                     &aRouter)
{
    Entry      *entry;
    Ip6::Prefix prefix;

    aRouter.mManagedAddressConfigFlag = aRaHeader.IsManagedAddressConfigFlagSet();
    aRouter.mOtherConfigFlag          = aRaHeader.IsOtherConfigFlagSet();
    LogInfo("- RA Header - flags - M:%u O:%u", aRouter.mManagedAddressConfigFlag, aRouter.mOtherConfigFlag);

    prefix.Clear();
    entry = aRouter.mEntries.FindMatching(Entry::Matcher(prefix, Entry::kTypeRoute));

    LogInfo("- RA Header - default route - lifetime:%u", aRaHeader.GetRouterLifetime());

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

    mEntryTimer.FireAtIfEarlier(entry->GetExpireTime());

    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::ProcessPrefixInfoOption(const Ip6::Nd::PrefixInfoOption &aPio,
                                                                    Router                          &aRouter)
{
    Ip6::Prefix prefix;
    Entry      *entry;

    VerifyOrExit(aPio.IsValid());
    aPio.GetPrefix(prefix);

    VerifyOrExit(Get<RoutingManager>().ShouldProcessPrefixInfoOption(aPio, prefix));

    LogPrefixInfoOption(prefix, aPio.GetValidLifetime(), aPio.GetPreferredLifetime());

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
        entry->AdoptValidAndPreferredLifetimesFrom(newEntry);
    }

    mEntryTimer.FireAtIfEarlier(entry->GetExpireTime());

    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::ProcessRouteInfoOption(const Ip6::Nd::RouteInfoOption &aRio,
                                                                   Router                         &aRouter)
{
    Ip6::Prefix prefix;
    Entry      *entry;

    VerifyOrExit(aRio.IsValid());
    aRio.GetPrefix(prefix);

    VerifyOrExit(Get<RoutingManager>().ShouldProcessRouteInfoOption(aRio, prefix));

    LogRouteInfoOption(prefix, aRio.GetRouteLifetime(), aRio.GetPreference());

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

    mEntryTimer.FireAtIfEarlier(entry->GetExpireTime());

    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::ProcessRaFlagsExtOption(const Ip6::Nd::RaFlagsExtOption &aRaFlagsOption,
                                                                    Router                          &aRouter)
{
    VerifyOrExit(aRaFlagsOption.IsValid());
    aRouter.mStubRouterFlag = aRaFlagsOption.IsStubRouterFlagSet();

    LogInfo("- FlagsExt - StubRouter:%u", aRouter.mStubRouterFlag);

exit:
    return;
}

bool RoutingManager::DiscoveredPrefixTable::Contains(const Entry::Checker &aChecker) const
{
    bool contains = false;

    for (const Router &router : mRouters)
    {
        if (router.mEntries.ContainsMatching(aChecker))
        {
            contains = true;
            break;
        }
    }

    return contains;
}

bool RoutingManager::DiscoveredPrefixTable::ContainsDefaultOrNonUlaRoutePrefix(void) const
{
    return Contains(Entry::Checker(Entry::Checker::kIsNotUla, Entry::kTypeRoute));
}

bool RoutingManager::DiscoveredPrefixTable::ContainsNonUlaOnLinkPrefix(void) const
{
    return Contains(Entry::Checker(Entry::Checker::kIsNotUla, Entry::kTypeOnLink));
}

bool RoutingManager::DiscoveredPrefixTable::ContainsUlaOnLinkPrefix(void) const
{
    return Contains(Entry::Checker(Entry::Checker::kIsUla, Entry::kTypeOnLink));
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
    RemoveRoutersWithNoEntriesOrFlags();

    SignalTableChanged();

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::RemoveAllEntries(void)
{
    // Remove all entries from the table.

    for (Router &router : mRouters)
    {
        FreeEntries(router.mEntries);
    }

    FreeRouters(mRouters);
    mEntryTimer.Stop();

    SignalTableChanged();
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

void RoutingManager::DiscoveredPrefixTable::RemoveOrDeprecateEntriesFromInactiveRouters(void)
{
    // Remove route prefix entries and deprecate on-link prefix entries
    // in the table for routers that have reached the max NS probe
    // attempts and considered as inactive.

    for (Router &router : mRouters)
    {
        if (router.mNsProbeCount <= Router::kMaxNsProbes)
        {
            continue;
        }

        for (Entry &entry : router.mEntries)
        {
            if (entry.IsOnLinkPrefix())
            {
                if (!entry.IsDeprecated())
                {
                    entry.ClearPreferredLifetime();
                    SignalTableChanged();
                }
            }
            else
            {
                entry.ClearValidLifetime();
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

void RoutingManager::DiscoveredPrefixTable::RemoveRoutersWithNoEntriesOrFlags(void)
{
    LinkedList<Router> routersToFree;

    mRouters.RemoveAllMatching(Router::kContainsNoEntriesOrFlags, routersToFree);
    FreeRouters(routersToFree);
}

void RoutingManager::DiscoveredPrefixTable::FreeRouters(LinkedList<Router> &aRouters)
{
    // Frees all routers in the given list `aRouters`

    Router *router;

    while ((router = aRouters.Pop()) != nullptr)
    {
        FreeRouter(*router);
    }
}

void RoutingManager::DiscoveredPrefixTable::FreeEntries(LinkedList<Entry> &aEntries)
{
    // Frees all entries in the given list `aEntries`.

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

void RoutingManager::DiscoveredPrefixTable::HandleEntryTimer(void) { RemoveExpiredEntries(); }

void RoutingManager::DiscoveredPrefixTable::RemoveExpiredEntries(void)
{
    TimeMilli         now            = TimerMilli::GetNow();
    TimeMilli         nextExpireTime = now.GetDistantFuture();
    LinkedList<Entry> expiredEntries;

    for (Router &router : mRouters)
    {
        router.mEntries.RemoveAllMatching(Entry::ExpirationChecker(now), expiredEntries);
    }

    RemoveRoutersWithNoEntriesOrFlags();

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
        mEntryTimer.FireAt(nextExpireTime);
    }
}

void RoutingManager::DiscoveredPrefixTable::SignalTableChanged(void) { mSignalTask.Post(); }

void RoutingManager::DiscoveredPrefixTable::ProcessNeighborAdvertMessage(
    const Ip6::Nd::NeighborAdvertMessage &aNaMessage)
{
    Router *router;

    VerifyOrExit(aNaMessage.IsValid());

    router = mRouters.FindMatching(aNaMessage.GetTargetAddress());
    VerifyOrExit(router != nullptr);

    LogInfo("Received NA from router %s", router->mAddress.ToString().AsCString());

    UpdateRouterOnRx(*router);

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::UpdateRouterOnRx(Router &aRouter)
{
    aRouter.mNsProbeCount = 0;
    aRouter.mTimeout = TimerMilli::GetNow() + Random::NonCrypto::AddJitter(Router::kActiveTimeout, Router::kJitter);

    mRouterTimer.FireAtIfEarlier(aRouter.mTimeout);
}

void RoutingManager::DiscoveredPrefixTable::HandleRouterTimer(void)
{
    TimeMilli now      = TimerMilli::GetNow();
    TimeMilli nextTime = now.GetDistantFuture();

    for (Router &router : mRouters)
    {
        if (router.mNsProbeCount > Router::kMaxNsProbes)
        {
            continue;
        }

        // If the `router` emitting RA has an address belonging to
        // infra interface, it indicates that the RAs are from
        // same device. In this case we skip performing NS probes.
        // This addresses situation where platform may not be
        // be able to receive and pass the NA message response
        // from device itself.

        if (Get<RoutingManager>().mInfraIf.HasAddress(router.mAddress))
        {
            continue;
        }

        if (router.mTimeout <= now)
        {
            router.mNsProbeCount++;

            if (router.mNsProbeCount > Router::kMaxNsProbes)
            {
                LogInfo("No response to all Neighbor Solicitations attempts from router %s",
                        router.mAddress.ToString().AsCString());
                continue;
            }

            router.mTimeout = now + ((router.mNsProbeCount < Router::kMaxNsProbes) ? Router::kNsProbeRetryInterval
                                                                                   : Router::kNsProbeTimeout);

            SendNeighborSolicitToRouter(router);
        }

        nextTime = Min(nextTime, router.mTimeout);
    }

    RemoveOrDeprecateEntriesFromInactiveRouters();

    if (nextTime != now.GetDistantFuture())
    {
        mRouterTimer.FireAtIfEarlier(nextTime);
    }
}

void RoutingManager::DiscoveredPrefixTable::SendNeighborSolicitToRouter(const Router &aRouter)
{
    InfraIf::Icmp6Packet            packet;
    Ip6::Nd::NeighborSolicitMessage neighborSolicitMsg;

    VerifyOrExit(!Get<RoutingManager>().mRsSender.IsInProgress());

    neighborSolicitMsg.SetTargetAddress(aRouter.mAddress);
    packet.InitFrom(neighborSolicitMsg);

    IgnoreError(Get<RoutingManager>().mInfraIf.Send(packet, aRouter.mAddress));

    LogInfo("Sent Neighbor Solicitation to %s - attempt:%u/%u", aRouter.mAddress.ToString().AsCString(),
            aRouter.mNsProbeCount, Router::kMaxNsProbes);

exit:
    return;
}

void RoutingManager::DiscoveredPrefixTable::DetermineAndSetFlags(Ip6::Nd::RouterAdvertMessage &aRaMessage) const
{
    // Determine the `M` and `O` flags to include in the RA message
    // header `aRaMessage` to be emitted.
    //
    // If any discovered router on infrastructure which is not itself a
    // stub router (e.g., another Thread BR) includes the `M` or `O`
    // flag, we also include the same flag.
    //
    // If a router has failed to respond to max number of NS probe
    // attempts, we consider it as offline and ignore its flags.

    for (const Router &router : mRouters)
    {
        if (router.mStubRouterFlag)
        {
            continue;
        }

        if (router.mNsProbeCount > Router::kMaxNsProbes)
        {
            continue;
        }

        if (router.mManagedAddressConfigFlag)
        {
            aRaMessage.GetHeader().SetManagedAddressConfigFlag();
        }

        if (router.mOtherConfigFlag)
        {
            aRaMessage.GetHeader().SetOtherConfigFlag();
        }
    }
}

void RoutingManager::DiscoveredPrefixTable::InitIterator(PrefixTableIterator &aIterator) const
{
    static_cast<Iterator &>(aIterator).Init(mRouters);
}

Error RoutingManager::DiscoveredPrefixTable::GetNextEntry(PrefixTableIterator &aIterator,
                                                          PrefixTableEntry    &aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    VerifyOrExit(iterator.GetRouter() != nullptr, error = kErrorNotFound);
    OT_ASSERT(iterator.GetEntry() != nullptr);

    iterator.GetRouter()->CopyInfoTo(aEntry.mRouter);
    aEntry.mPrefix              = iterator.GetEntry()->GetPrefix();
    aEntry.mIsOnLink            = iterator.GetEntry()->IsOnLinkPrefix();
    aEntry.mMsecSinceLastUpdate = iterator.GetInitTime() - iterator.GetEntry()->GetLastUpdateTime();
    aEntry.mValidLifetime       = iterator.GetEntry()->GetValidLifetime();
    aEntry.mPreferredLifetime   = aEntry.mIsOnLink ? iterator.GetEntry()->GetPreferredLifetime() : 0;
    aEntry.mRoutePreference =
        static_cast<otRoutePreference>(aEntry.mIsOnLink ? 0 : iterator.GetEntry()->GetRoutePreference());

    iterator.Advance(Iterator::kToNextEntry);

exit:
    return error;
}

Error RoutingManager::DiscoveredPrefixTable::GetNextRouter(PrefixTableIterator &aIterator, RouterEntry &aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    VerifyOrExit(iterator.GetRouter() != nullptr, error = kErrorNotFound);

    iterator.GetRouter()->CopyInfoTo(aEntry);
    iterator.Advance(Iterator::kToNextRouter);

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// DiscoveredPrefixTable::Iterator

void RoutingManager::DiscoveredPrefixTable::Iterator::Init(const LinkedList<Router> &aRouters)
{
    SetInitTime();
    SetRouter(aRouters.GetHead());
    SetEntry(aRouters.IsEmpty() ? nullptr : aRouters.GetHead()->mEntries.GetHead());
}

void RoutingManager::DiscoveredPrefixTable::Iterator::Advance(AdvanceMode aMode)
{
    switch (aMode)
    {
    case kToNextEntry:
        SetEntry(GetEntry()->GetNext());

        if (GetEntry() != nullptr)
        {
            break;
        }

        OT_FALL_THROUGH;

    case kToNextRouter:
        SetRouter(GetRouter()->GetNext());

        if (GetRouter() != nullptr)
        {
            SetEntry(GetRouter()->mEntries.GetHead());
        }

        break;
    }
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

bool RoutingManager::DiscoveredPrefixTable::Entry::Matches(const Checker &aChecker) const
{
    return (mType == aChecker.mType) && (mPrefix.IsUniqueLocal() == (aChecker.mMode == Checker::kIsUla));
}

bool RoutingManager::DiscoveredPrefixTable::Entry::Matches(const ExpirationChecker &aChecker) const
{
    return GetExpireTime() <= aChecker.mNow;
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

void RoutingManager::DiscoveredPrefixTable::Entry::AdoptValidAndPreferredLifetimesFrom(const Entry &aEntry)
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
// DiscoveredPrefixTable::Router

bool RoutingManager::DiscoveredPrefixTable::Router::Matches(EmptyChecker aChecker) const
{
    // Checks whether or not a `Router` instance has any useful info. An
    // entry can be removed if it does not advertise M or O flags and
    // also does not have any advertised prefix entries (RIO/PIO). If
    // the router already failed to respond to max NS probe attempts,
    // we consider it as offline and therefore do not consider its
    // flags anymore.

    OT_UNUSED_VARIABLE(aChecker);

    bool hasFlags = false;

    if (mNsProbeCount <= kMaxNsProbes)
    {
        hasFlags = (mManagedAddressConfigFlag || mOtherConfigFlag);
    }

    return !hasFlags && mEntries.IsEmpty();
}

void RoutingManager::DiscoveredPrefixTable::Router::CopyInfoTo(RouterEntry &aEntry) const
{
    aEntry.mAddress                  = mAddress;
    aEntry.mManagedAddressConfigFlag = mManagedAddressConfigFlag;
    aEntry.mOtherConfigFlag          = mOtherConfigFlag;
    aEntry.mStubRouterFlag           = mStubRouterFlag;
}

//---------------------------------------------------------------------------------------------------------------------
// FavoredOmrPrefix

bool RoutingManager::FavoredOmrPrefix::IsInfrastructureDerived(void) const
{
    // Indicate whether the OMR prefix is infrastructure-derived which
    // can be identified as a valid OMR prefix with preference of
    // medium or higher.

    return !IsEmpty() && (mPreference >= NetworkData::kRoutePreferenceMedium);
}

void RoutingManager::FavoredOmrPrefix::SetFrom(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig)
{
    mPrefix         = aOnMeshPrefixConfig.GetPrefix();
    mPreference     = aOnMeshPrefixConfig.GetPreference();
    mIsDomainPrefix = aOnMeshPrefixConfig.mDp;
}

void RoutingManager::FavoredOmrPrefix::SetFrom(const OmrPrefix &aOmrPrefix)
{
    mPrefix         = aOmrPrefix.GetPrefix();
    mPreference     = aOmrPrefix.GetPreference();
    mIsDomainPrefix = aOmrPrefix.IsDomainPrefix();
}

bool RoutingManager::FavoredOmrPrefix::IsFavoredOver(const NetworkData::OnMeshPrefixConfig &aOmrPrefixConfig) const
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
// OmrPrefixManager

RoutingManager::OmrPrefixManager::OmrPrefixManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsLocalAddedInNetData(false)
    , mDefaultRoute(false)
{
}

void RoutingManager::OmrPrefixManager::Init(const Ip6::Prefix &aBrUlaPrefix)
{
    mGeneratedPrefix = aBrUlaPrefix;
    mGeneratedPrefix.SetSubnetId(kOmrPrefixSubnetId);
    mGeneratedPrefix.SetLength(kOmrPrefixLength);

    LogInfo("Generated local OMR prefix: %s", mGeneratedPrefix.ToString().AsCString());
}

void RoutingManager::OmrPrefixManager::Start(void) { DetermineFavoredPrefix(); }

void RoutingManager::OmrPrefixManager::Stop(void)
{
    RemoveLocalFromNetData();
    mFavoredPrefix.Clear();
}

void RoutingManager::OmrPrefixManager::DetermineFavoredPrefix(void)
{
    // Determine the favored OMR prefix present in Network Data.

    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;

    mFavoredPrefix.Clear();

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
    {
        if (!IsValidOmrPrefix(prefixConfig) || !prefixConfig.mPreferred)
        {
            continue;
        }

        if (mFavoredPrefix.IsEmpty() || !mFavoredPrefix.IsFavoredOver(prefixConfig))
        {
            mFavoredPrefix.SetFrom(prefixConfig);
        }
    }
}

void RoutingManager::OmrPrefixManager::Evaluate(void)
{
    OT_ASSERT(Get<RoutingManager>().IsRunning());

    DetermineFavoredPrefix();

    // Determine the local prefix and remove outdated prefix published by us.
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    if (Get<RoutingManager>().mPdPrefixManager.HasPrefix())
    {
        if (mLocalPrefix.GetPrefix() != Get<RoutingManager>().mPdPrefixManager.GetPrefix())
        {
            RemoveLocalFromNetData();
            mLocalPrefix.mPrefix         = Get<RoutingManager>().mPdPrefixManager.GetPrefix();
            mLocalPrefix.mPreference     = RoutePreference::kRoutePreferenceMedium;
            mLocalPrefix.mIsDomainPrefix = false;
            LogInfo("Setting local OMR prefix to PD prefix: %s", mLocalPrefix.GetPrefix().ToString().AsCString());
        }
    }
    else
#endif
        if (mLocalPrefix.GetPrefix() != mGeneratedPrefix)
    {
        RemoveLocalFromNetData();
        mLocalPrefix.mPrefix         = mGeneratedPrefix;
        mLocalPrefix.mPreference     = RoutePreference::kRoutePreferenceLow;
        mLocalPrefix.mIsDomainPrefix = false;
        LogInfo("Setting local OMR prefix to generated prefix: %s", mLocalPrefix.GetPrefix().ToString().AsCString());
    }

    // Decide if we need to add or remove our local OMR prefix.
    if (mFavoredPrefix.IsEmpty() || mFavoredPrefix.GetPreference() < mLocalPrefix.GetPreference())
    {
        if (mFavoredPrefix.IsEmpty())
        {
            LogInfo("No favored OMR prefix found in Thread network.");
        }
        else
        {
            LogInfo("Replacing favored OMR prefix %s with higher preference local prefix %s.",
                    mFavoredPrefix.GetPrefix().ToString().AsCString(), mLocalPrefix.GetPrefix().ToString().AsCString());
        }

        // The `mFavoredPrefix` remains empty if we fail to publish
        // the local OMR prefix.
        SuccessOrExit(AddLocalToNetData());

        mFavoredPrefix.SetFrom(mLocalPrefix);
    }
    else if (mFavoredPrefix.GetPrefix() == mLocalPrefix.GetPrefix())
    {
        IgnoreError(AddLocalToNetData());
    }
    else if (mIsLocalAddedInNetData)
    {
        LogInfo("There is already a favored OMR prefix %s in the Thread network",
                mFavoredPrefix.GetPrefix().ToString().AsCString());

        RemoveLocalFromNetData();
    }

exit:
    return;
}

bool RoutingManager::OmrPrefixManager::ShouldAdvertiseLocalAsRio(void) const
{
    // Determines whether the local OMR prefix should be advertised as
    // RIO in emitted RAs. To advertise, we must have decided to
    // publish it, and it must already be added and present in the
    // Network Data. This ensures that we only advertise the local
    // OMR prefix in emitted RAs when, as a Border Router, we can
    // accept and route messages using an OMR-based address
    // destination, which requires the prefix to be present in
    // Network Data. Similarly, we stop advertising (and start
    // deprecating) the OMR prefix in RAs as soon as we decide to
    // remove it. After requesting its removal from Network Data, it
    // may still be present in Network Data for a short interval due
    // to delays in registering changes with the leader.

    bool                            shouldAdvertise = false;
    NetworkData::Iterator           iterator        = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;

    VerifyOrExit(mIsLocalAddedInNetData);

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
    {
        if (!IsValidOmrPrefix(prefixConfig))
        {
            continue;
        }

        if (prefixConfig.GetPrefix() == mLocalPrefix.GetPrefix())
        {
            shouldAdvertise = true;
            break;
        }
    }

exit:
    return shouldAdvertise;
}

Error RoutingManager::OmrPrefixManager::AddLocalToNetData(void)
{
    Error error = kErrorNone;

    VerifyOrExit(!mIsLocalAddedInNetData);
    SuccessOrExit(error = AddOrUpdateLocalInNetData());
    mIsLocalAddedInNetData = true;

exit:
    return error;
}

Error RoutingManager::OmrPrefixManager::AddOrUpdateLocalInNetData(void)
{
    // Add the local OMR prefix in Thread Network Data or update it
    // (e.g., change default route flag) if it is already added.

    Error                           error;
    NetworkData::OnMeshPrefixConfig config;

    config.Clear();
    config.mPrefix       = mLocalPrefix.GetPrefix();
    config.mStable       = true;
    config.mSlaac        = true;
    config.mPreferred    = true;
    config.mOnMesh       = true;
    config.mDefaultRoute = mDefaultRoute;
    config.mPreference   = mLocalPrefix.GetPreference();

    error = Get<NetworkData::Local>().AddOnMeshPrefix(config);

    if (error != kErrorNone)
    {
        LogWarn("Failed to %s %s in Thread Network Data: %s", !mIsLocalAddedInNetData ? "add" : "update",
                LocalToString().AsCString(), ErrorToString(error));
        ExitNow();
    }

    Get<NetworkData::Notifier>().HandleServerDataUpdated();

    LogInfo("%s %s in Thread Network Data", !mIsLocalAddedInNetData ? "Added" : "Updated", LocalToString().AsCString());

exit:
    return error;
}

void RoutingManager::OmrPrefixManager::RemoveLocalFromNetData(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsLocalAddedInNetData);

    error = Get<NetworkData::Local>().RemoveOnMeshPrefix(mLocalPrefix.GetPrefix());

    if (error != kErrorNone)
    {
        LogWarn("Failed to remove %s from Thread Network Data: %s", LocalToString().AsCString(), ErrorToString(error));
        ExitNow();
    }

    mIsLocalAddedInNetData = false;
    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    LogInfo("Removed %s from Thread Network Data", LocalToString().AsCString());

exit:
    return;
}

void RoutingManager::OmrPrefixManager::UpdateDefaultRouteFlag(bool aDefaultRoute)
{
    VerifyOrExit(aDefaultRoute != mDefaultRoute);

    mDefaultRoute = aDefaultRoute;

    VerifyOrExit(mIsLocalAddedInNetData);
    IgnoreError(AddOrUpdateLocalInNetData());

exit:
    return;
}

RoutingManager::OmrPrefixManager::InfoString RoutingManager::OmrPrefixManager::LocalToString(void) const
{
    InfoString string;

    string.Append("local OMR prefix %s (def-route:%s)", mLocalPrefix.GetPrefix().ToString().AsCString(),
                  ToYesNo(mDefaultRoute));
    return string;
}

//---------------------------------------------------------------------------------------------------------------------
// OnLinkPrefixManager

RoutingManager::OnLinkPrefixManager::OnLinkPrefixManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kIdle)
    , mTimer(aInstance)
{
    mLocalPrefix.Clear();
    mFavoredDiscoveredPrefix.Clear();
    mOldLocalPrefixes.Clear();
}

void RoutingManager::OnLinkPrefixManager::SetState(State aState)
{
    VerifyOrExit(mState != aState);

    LogInfo("Local on-link prefix state: %s -> %s (%s)", StateToString(mState), StateToString(aState),
            mLocalPrefix.ToString().AsCString());
    mState = aState;

    // Mark the Advertising PIO (AP) flag in the published route, when
    // the local on-link prefix is being published, advertised, or
    // deprecated.

    Get<RoutingManager>().mRoutePublisher.UpdateAdvPioFlags(aState != kIdle);

exit:
    return;
}

void RoutingManager::OnLinkPrefixManager::Init(void)
{
    TimeMilli                now = TimerMilli::GetNow();
    Settings::BrOnLinkPrefix savedPrefix;
    bool                     refreshStoredPrefixes = false;

    // Restore old prefixes from `Settings`

    for (int index = 0; Get<Settings>().ReadBrOnLinkPrefix(index, savedPrefix) == kErrorNone; index++)
    {
        uint32_t   lifetime;
        OldPrefix *entry;

        if (mOldLocalPrefixes.ContainsMatching(savedPrefix.GetPrefix()))
        {
            // We should not see duplicate entries in `Settings`
            // but if we do we refresh the stored prefixes to make
            // it consistent.
            refreshStoredPrefixes = true;
            continue;
        }

        entry = mOldLocalPrefixes.PushBack();

        if (entry == nullptr)
        {
            // If there are more stored prefixes, we refresh the
            // prefixes in `Settings` to remove the ones we cannot
            // handle.

            refreshStoredPrefixes = true;
            break;
        }

        lifetime = Min(savedPrefix.GetLifetime(), Time::MsecToSec(TimerMilli::kMaxDelay));

        entry->mPrefix     = savedPrefix.GetPrefix();
        entry->mExpireTime = now + Time::SecToMsec(lifetime);

        LogInfo("Restored old prefix %s, lifetime:%lu", entry->mPrefix.ToString().AsCString(), ToUlong(lifetime));

        mTimer.FireAtIfEarlier(entry->mExpireTime);
    }

    if (refreshStoredPrefixes)
    {
        // We clear the entries in `Settings` and re-write the entries
        // from `mOldLocalPrefixes` array.

        IgnoreError(Get<Settings>().DeleteAllBrOnLinkPrefixes());

        for (OldPrefix &oldPrefix : mOldLocalPrefixes)
        {
            SavePrefix(oldPrefix.mPrefix, oldPrefix.mExpireTime);
        }
    }

    GenerateLocalPrefix();
}

void RoutingManager::OnLinkPrefixManager::GenerateLocalPrefix(void)
{
    const MeshCoP::ExtendedPanId &extPanId = Get<MeshCoP::ExtendedPanIdManager>().GetExtPanId();
    OldPrefix                    *entry;
    Ip6::Prefix                   oldLocalPrefix = mLocalPrefix;

    // Global ID: 40 most significant bits of Extended PAN ID
    // Subnet ID: 16 least significant bits of Extended PAN ID

    mLocalPrefix.mPrefix.mFields.m8[0] = 0xfd;
    memcpy(mLocalPrefix.mPrefix.mFields.m8 + 1, extPanId.m8, 5);
    memcpy(mLocalPrefix.mPrefix.mFields.m8 + 6, extPanId.m8 + 6, 2);

    mLocalPrefix.SetLength(kOnLinkPrefixLength);

    // We ensure that the local prefix did change, since not all the
    // bytes in Extended PAN ID are used in derivation of the local prefix.

    VerifyOrExit(mLocalPrefix != oldLocalPrefix);

    LogNote("Local on-link prefix: %s", mLocalPrefix.ToString().AsCString());

    // Check if the new local prefix happens to be in `mOldLocalPrefixes` array.
    // If so, we remove it from the array and update the state accordingly.

    entry = mOldLocalPrefixes.FindMatching(mLocalPrefix);

    if (entry != nullptr)
    {
        SetState(kDeprecating);
        mExpireTime = entry->mExpireTime;
        mOldLocalPrefixes.Remove(*entry);
    }
    else
    {
        SetState(kIdle);
    }

exit:
    return;
}

void RoutingManager::OnLinkPrefixManager::Start(void) {}

void RoutingManager::OnLinkPrefixManager::Stop(void)
{
    mFavoredDiscoveredPrefix.Clear();

    switch (GetState())
    {
    case kIdle:
        break;

    case kPublishing:
    case kAdvertising:
    case kDeprecating:
        SetState(kDeprecating);
        break;
    }
}

void RoutingManager::OnLinkPrefixManager::Evaluate(void)
{
    VerifyOrExit(!Get<RoutingManager>().mRsSender.IsInProgress());

    Get<RoutingManager>().mDiscoveredPrefixTable.FindFavoredOnLinkPrefix(mFavoredDiscoveredPrefix);

    if ((mFavoredDiscoveredPrefix.GetLength() == 0) || (mFavoredDiscoveredPrefix == mLocalPrefix))
    {
        // We need to advertise our local on-link prefix when there is
        // no discovered on-link prefix. If the favored discovered
        // prefix is the same as our local on-link prefix we also
        // start advertising the local prefix to add redundancy. Note
        // that local on-link prefix is derived from extended PAN ID
        // and therefore is the same for all BRs on the same Thread
        // mesh.

        PublishAndAdvertise();

        // We remove the local on-link prefix from discovered prefix
        // table, in case it was previously discovered and included in
        // the table (now as a deprecating entry). We remove it with
        // `kKeepInNetData` flag to ensure that the prefix is not
        // unpublished from network data.
        //
        // Note that `ShouldProcessPrefixInfoOption()` will also check
        // not allow the local on-link prefix to be added in the prefix
        // table while we are advertising it.

        Get<RoutingManager>().mDiscoveredPrefixTable.RemoveOnLinkPrefix(mLocalPrefix);

        mFavoredDiscoveredPrefix.Clear();
    }
    else if (IsPublishingOrAdvertising())
    {
        // When an application-specific on-link prefix is received and
        // it is larger than the local prefix, we will not remove the
        // advertised local prefix. In this case, there will be two
        // on-link prefixes on the infra link. But all BRs will still
        // converge to the same smallest/favored on-link prefix and the
        // application-specific prefix is not used.

        if (!(mLocalPrefix < mFavoredDiscoveredPrefix))
        {
            LogInfo("Found a favored on-link prefix %s", mFavoredDiscoveredPrefix.ToString().AsCString());
            Deprecate();
        }
    }

exit:
    return;
}

bool RoutingManager::OnLinkPrefixManager::IsInitalEvaluationDone(void) const
{
    // This method indicates whether or not we are done with the
    // initial policy evaluation of the on-link prefixes, i.e., either
    // we have discovered a favored on-link prefix (being advertised by
    // another router on infra link) or we are advertising our local
    // on-link prefix.

    return (mFavoredDiscoveredPrefix.GetLength() != 0 || IsPublishingOrAdvertising());
}

void RoutingManager::OnLinkPrefixManager::HandleDiscoveredPrefixTableChanged(void)
{
    // This is a callback from `mDiscoveredPrefixTable` indicating that
    // there has been a change in the table. If the favored on-link
    // prefix has changed, we trigger a re-evaluation of the routing
    // policy.

    Ip6::Prefix newFavoredPrefix;

    Get<RoutingManager>().mDiscoveredPrefixTable.FindFavoredOnLinkPrefix(newFavoredPrefix);

    if (newFavoredPrefix != mFavoredDiscoveredPrefix)
    {
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
    }
}

void RoutingManager::OnLinkPrefixManager::PublishAndAdvertise(void)
{
    // Start publishing and advertising the local on-link prefix if
    // not already.

    switch (GetState())
    {
    case kIdle:
    case kDeprecating:
        break;

    case kPublishing:
    case kAdvertising:
        ExitNow();
    }

    SetState(kPublishing);
    ResetExpireTime(TimerMilli::GetNow());

    // We wait for the ULA `fc00::/7` route or a sub-prefix of it (e.g.,
    // default route) to be added in Network Data before
    // starting to advertise the local on-link prefix in RAs.
    // However, if it is already present in Network Data (e.g.,
    // added by another BR on the same Thread mesh), we can
    // immediately start advertising it.

    if (Get<RoutingManager>().NetworkDataContainsUlaRoute())
    {
        SetState(kAdvertising);
    }

exit:
    return;
}

void RoutingManager::OnLinkPrefixManager::Deprecate(void)
{
    // Deprecate the local on-link prefix if it was being advertised
    // before. While depreciating the prefix, we wait for the lifetime
    // timer to expire before unpublishing the prefix from the Network
    // Data. We also continue to include it as a PIO in the RA message
    // with zero preferred lifetime and the remaining valid lifetime
    // until the timer expires.

    switch (GetState())
    {
    case kPublishing:
    case kAdvertising:
        SetState(kDeprecating);
        break;

    case kIdle:
    case kDeprecating:
        break;
    }
}

bool RoutingManager::OnLinkPrefixManager::ShouldPublishUlaRoute(void) const
{
    // Determine whether or not we should publish ULA prefix. We need
    // to publish if we are in any of `kPublishing`, `kAdvertising`,
    // or `kDeprecating` states, or if there is at least one old local
    // prefix being deprecated.

    return (GetState() != kIdle) || !mOldLocalPrefixes.IsEmpty();
}

void RoutingManager::OnLinkPrefixManager::ResetExpireTime(TimeMilli aNow)
{
    mExpireTime = aNow + TimeMilli::SecToMsec(kDefaultOnLinkPrefixLifetime);
    mTimer.FireAtIfEarlier(mExpireTime);
    SavePrefix(mLocalPrefix, mExpireTime);
}

bool RoutingManager::OnLinkPrefixManager::IsPublishingOrAdvertising(void) const
{
    return (GetState() == kPublishing) || (GetState() == kAdvertising);
}

void RoutingManager::OnLinkPrefixManager::AppendAsPiosTo(Ip6::Nd::RouterAdvertMessage &aRaMessage)
{
    AppendCurPrefix(aRaMessage);
    AppendOldPrefixes(aRaMessage);
}

void RoutingManager::OnLinkPrefixManager::AppendCurPrefix(Ip6::Nd::RouterAdvertMessage &aRaMessage)
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

    switch (GetState())
    {
    case kAdvertising:
        ResetExpireTime(now);
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

    SuccessOrAssert(aRaMessage.AppendPrefixInfoOption(mLocalPrefix, validLifetime, preferredLifetime));

    LogPrefixInfoOption(mLocalPrefix, validLifetime, preferredLifetime);

exit:
    return;
}

void RoutingManager::OnLinkPrefixManager::AppendOldPrefixes(Ip6::Nd::RouterAdvertMessage &aRaMessage)
{
    TimeMilli now = TimerMilli::GetNow();
    uint32_t  validLifetime;

    for (const OldPrefix &oldPrefix : mOldLocalPrefixes)
    {
        if (oldPrefix.mExpireTime < now)
        {
            continue;
        }

        validLifetime = TimeMilli::MsecToSec(oldPrefix.mExpireTime - now);
        SuccessOrAssert(aRaMessage.AppendPrefixInfoOption(oldPrefix.mPrefix, validLifetime, 0));

        LogPrefixInfoOption(oldPrefix.mPrefix, validLifetime, 0);
    }
}

void RoutingManager::OnLinkPrefixManager::HandleNetDataChange(void)
{
    VerifyOrExit(GetState() == kPublishing);

    if (Get<RoutingManager>().NetworkDataContainsUlaRoute())
    {
        SetState(kAdvertising);
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
    }

exit:
    return;
}

void RoutingManager::OnLinkPrefixManager::HandleExtPanIdChange(void)
{
    // If the current local prefix is being advertised or deprecated,
    // we save it in `mOldLocalPrefixes` and keep deprecating it. It will
    // be included in emitted RAs as PIO with zero preferred lifetime.
    // It will still be present in Network Data until its expire time
    // so to allow Thread nodes to continue to communicate with `InfraIf`
    // device using addresses based on this prefix.

    uint16_t    oldState  = GetState();
    Ip6::Prefix oldPrefix = mLocalPrefix;

    GenerateLocalPrefix();

    VerifyOrExit(oldPrefix != mLocalPrefix);

    switch (oldState)
    {
    case kIdle:
    case kPublishing:
        break;

    case kAdvertising:
    case kDeprecating:
        DeprecateOldPrefix(oldPrefix, mExpireTime);
        break;
    }

    if (Get<RoutingManager>().mIsRunning)
    {
        Get<RoutingManager>().mRoutePublisher.Evaluate();
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
    }

exit:
    return;
}

void RoutingManager::OnLinkPrefixManager::DeprecateOldPrefix(const Ip6::Prefix &aPrefix, TimeMilli aExpireTime)
{
    OldPrefix  *entry = nullptr;
    Ip6::Prefix removedPrefix;

    removedPrefix.Clear();

    VerifyOrExit(!mOldLocalPrefixes.ContainsMatching(aPrefix));

    LogInfo("Deprecating old on-link prefix %s", aPrefix.ToString().AsCString());

    if (!mOldLocalPrefixes.IsFull())
    {
        entry = mOldLocalPrefixes.PushBack();
    }
    else
    {
        // If there is no more room in `mOldLocalPrefixes` array
        // we evict the entry with the earliest expiration time.

        entry = &mOldLocalPrefixes[0];

        for (OldPrefix &oldPrefix : mOldLocalPrefixes)
        {
            if ((oldPrefix.mExpireTime < entry->mExpireTime))
            {
                entry = &oldPrefix;
            }
        }

        removedPrefix = entry->mPrefix;

        IgnoreError(Get<Settings>().RemoveBrOnLinkPrefix(removedPrefix));
    }

    entry->mPrefix     = aPrefix;
    entry->mExpireTime = aExpireTime;
    mTimer.FireAtIfEarlier(aExpireTime);

    SavePrefix(aPrefix, aExpireTime);

exit:
    return;
}

void RoutingManager::OnLinkPrefixManager::SavePrefix(const Ip6::Prefix &aPrefix, TimeMilli aExpireTime)
{
    Settings::BrOnLinkPrefix savedPrefix;

    savedPrefix.SetPrefix(aPrefix);
    savedPrefix.SetLifetime(TimeMilli::MsecToSec(aExpireTime - TimerMilli::GetNow()));
    IgnoreError(Get<Settings>().AddOrUpdateBrOnLinkPrefix(savedPrefix));
}

void RoutingManager::OnLinkPrefixManager::HandleTimer(void)
{
    TimeMilli                           now            = TimerMilli::GetNow();
    TimeMilli                           nextExpireTime = now.GetDistantFuture();
    Array<Ip6::Prefix, kMaxOldPrefixes> expiredPrefixes;

    switch (GetState())
    {
    case kIdle:
        break;
    case kPublishing:
    case kAdvertising:
    case kDeprecating:
        if (now >= mExpireTime)
        {
            IgnoreError(Get<Settings>().RemoveBrOnLinkPrefix(mLocalPrefix));
            SetState(kIdle);
        }
        else
        {
            nextExpireTime = mExpireTime;
        }
        break;
    }

    for (OldPrefix &entry : mOldLocalPrefixes)
    {
        if (now >= entry.mExpireTime)
        {
            SuccessOrAssert(expiredPrefixes.PushBack(entry.mPrefix));
        }
        else
        {
            nextExpireTime = Min(nextExpireTime, entry.mExpireTime);
        }
    }

    for (const Ip6::Prefix &prefix : expiredPrefixes)
    {
        LogInfo("Old local on-link prefix %s expired", prefix.ToString().AsCString());
        IgnoreError(Get<Settings>().RemoveBrOnLinkPrefix(prefix));
        mOldLocalPrefixes.RemoveMatching(prefix);
    }

    if (nextExpireTime != now.GetDistantFuture())
    {
        mTimer.FireAtIfEarlier(nextExpireTime);
    }

    Get<RoutingManager>().mRoutePublisher.Evaluate();
}

const char *RoutingManager::OnLinkPrefixManager::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Removed",     // (0) kIdle
        "Publishing",  // (1) kPublishing
        "Advertising", // (2) kAdvertising
        "Deprecating", // (3) kDeprecating
    };

    static_assert(0 == kIdle, "kIdle value is incorrect");
    static_assert(1 == kPublishing, "kPublishing value is incorrect");
    static_assert(2 == kAdvertising, "kAdvertising value is incorrect");
    static_assert(3 == kDeprecating, "kDeprecating value is incorrect");

    return kStateStrings[aState];
}

//---------------------------------------------------------------------------------------------------------------------
// RioAdvertiser

RoutingManager::RioAdvertiser::RioAdvertiser(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mTimer(aInstance)
    , mPreference(NetworkData::kRoutePreferenceLow)
    , mUserSetPreference(false)
{
}

void RoutingManager::RioAdvertiser::SetPreference(RoutePreference aPreference)
{
    LogInfo("User explicitly set RIO Preference to %s", RoutePreferenceToString(aPreference));
    mUserSetPreference = true;
    UpdatePreference(aPreference);
}

void RoutingManager::RioAdvertiser::ClearPreference(void)
{
    VerifyOrExit(mUserSetPreference);

    LogInfo("User cleared explicitly set RIO Preference");
    mUserSetPreference = false;
    SetPreferenceBasedOnRole();

exit:
    return;
}

void RoutingManager::RioAdvertiser::HandleRoleChanged(void)
{
    if (!mUserSetPreference)
    {
        SetPreferenceBasedOnRole();
    }
}

void RoutingManager::RioAdvertiser::SetPreferenceBasedOnRole(void)
{
    UpdatePreference(Get<Mle::Mle>().IsRouterOrLeader() ? NetworkData::kRoutePreferenceMedium
                                                        : NetworkData::kRoutePreferenceLow);
}

void RoutingManager::RioAdvertiser::UpdatePreference(RoutePreference aPreference)
{
    VerifyOrExit(mPreference != aPreference);

    LogInfo("RIO Preference changed: %s -> %s", RoutePreferenceToString(mPreference),
            RoutePreferenceToString(aPreference));
    mPreference = aPreference;

    Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);

exit:
    return;
}

void RoutingManager::RioAdvertiser::InvalidatPrevRios(Ip6::Nd::RouterAdvertMessage &aRaMessage)
{
    for (const RioPrefix &prefix : mPrefixes)
    {
        AppendRio(prefix.mPrefix, /* aRouteLifetime */ 0, aRaMessage);
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
    mPrefixes.Free();
#endif

    mPrefixes.Clear();
    mTimer.Stop();
}

void RoutingManager::RioAdvertiser::AppendRios(Ip6::Nd::RouterAdvertMessage &aRaMessage)
{
    TimeMilli                       now      = TimerMilli::GetNow();
    TimeMilli                       nextTime = now.GetDistantFuture();
    RioPrefixArray                  oldPrefixes;
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;
    const OmrPrefixManager         &omrPrefixManager = Get<RoutingManager>().mOmrPrefixManager;

#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
    oldPrefixes.TakeFrom(static_cast<RioPrefixArray &&>(mPrefixes));
#else
    oldPrefixes = mPrefixes;
#endif

    mPrefixes.Clear();

    // `mPrefixes` array can have a limited size. We add more
    // important prefixes first in the array to ensure they are
    // advertised in the RA message. Note that `Add()` method
    // will ensure to add a prefix only once (will check if
    // prefix is already present in the array).

    // (1) Local OMR prefix.

    if (omrPrefixManager.ShouldAdvertiseLocalAsRio())
    {
        mPrefixes.Add(omrPrefixManager.GetLocalPrefix().GetPrefix());
    }

    // (2) Favored OMR prefix.

    if (!omrPrefixManager.GetFavoredPrefix().IsEmpty() && !omrPrefixManager.GetFavoredPrefix().IsDomainPrefix())
    {
        mPrefixes.Add(omrPrefixManager.GetFavoredPrefix().GetPrefix());
    }

    // (3) All other OMR prefixes.

    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
    {
        // Decision on whether or not to include the local OMR prefix is
        // delegated to `OmrPrefixManager.ShouldAdvertiseLocalAsRio()`
        // at step (1). Here as we iterate over the Network Data
        // prefixes, we skip entries matching the local OMR prefix.
        // In particular, `OmrPrefixManager` may have decided to remove the
        // local prefix and not advertise it anymore, but it may still be
        // present in the Network Data (due to delay of registering changes
        // with leader).

        if (prefixConfig.mDp)
        {
            continue;
        }

        if (IsValidOmrPrefix(prefixConfig) &&
            (prefixConfig.GetPrefix() != omrPrefixManager.GetLocalPrefix().GetPrefix()))
        {
            mPrefixes.Add(prefixConfig.GetPrefix());
        }
    }

    // (4) All other on-mesh prefixes (excluding Domain Prefix).

    iterator = NetworkData::kIteratorInit;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
    {
        if (prefixConfig.mOnMesh && !prefixConfig.mDp && !IsValidOmrPrefix(prefixConfig))
        {
            mPrefixes.Add(prefixConfig.GetPrefix());
        }
    }

    // Determine deprecating prefixes

    for (RioPrefix &prefix : oldPrefixes)
    {
        if (mPrefixes.ContainsMatching(prefix.mPrefix))
        {
            continue;
        }

        if (prefix.mIsDeprecating)
        {
            if (now >= prefix.mExpirationTime)
            {
                AppendRio(prefix.mPrefix, /* aRouteLifetime */ 0, aRaMessage);
                continue;
            }
        }
        else
        {
            prefix.mIsDeprecating  = true;
            prefix.mExpirationTime = now + kDeprecationTime;
        }

        if (mPrefixes.PushBack(prefix) != kErrorNone)
        {
            LogWarn("Too many deprecating on-mesh prefixes, removing %s", prefix.mPrefix.ToString().AsCString());
            AppendRio(prefix.mPrefix, /* aRouteLifetime */ 0, aRaMessage);
        }

        nextTime = Min(nextTime, prefix.mExpirationTime);
    }

    // Advertise all prefixes in `mPrefixes`

    for (const RioPrefix &prefix : mPrefixes)
    {
        uint32_t lifetime = kDefaultOmrPrefixLifetime;

        if (prefix.mIsDeprecating)
        {
            lifetime = TimeMilli::MsecToSec(prefix.mExpirationTime - now);
        }

        AppendRio(prefix.mPrefix, lifetime, aRaMessage);
    }

    if (nextTime != now.GetDistantFuture())
    {
        mTimer.FireAtIfEarlier(nextTime);
    }
}

void RoutingManager::RioAdvertiser::AppendRio(const Ip6::Prefix            &aPrefix,
                                              uint32_t                      aRouteLifetime,
                                              Ip6::Nd::RouterAdvertMessage &aRaMessage)
{
    SuccessOrAssert(aRaMessage.AppendRouteInfoOption(aPrefix, aRouteLifetime, mPreference));
    LogRouteInfoOption(aPrefix, aRouteLifetime, mPreference);
}

void RoutingManager::RioAdvertiser::HandleTimer(void)
{
    Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kImmediately);
}

void RoutingManager::RioAdvertiser::RioPrefixArray::Add(const Ip6::Prefix &aPrefix)
{
    // Checks if `aPrefix` is already present in the array and if not
    // adds it as a new entry.

    Error     error;
    RioPrefix newEntry;

    VerifyOrExit(!ContainsMatching(aPrefix));

    newEntry.Clear();
    newEntry.mPrefix = aPrefix;

    error = PushBack(newEntry);

    if (error != kErrorNone)
    {
        LogWarn("Too many on-mesh prefixes in net data, ignoring prefix %s", aPrefix.ToString().AsCString());
    }

exit:
    return;
}

//---------------------------------------------------------------------------------------------------------------------
// RoutePublisher

const otIp6Prefix RoutingManager::RoutePublisher::kUlaPrefix = {
    {{{0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}},
    7,
};

RoutingManager::RoutePublisher::RoutePublisher(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mState(kDoNotPublish)
    , mPreference(NetworkData::kRoutePreferenceMedium)
    , mUserSetPreference(false)
    , mAdvPioFlag(false)
    , mTimer(aInstance)
{
}

void RoutingManager::RoutePublisher::Evaluate(void)
{
    State newState = kDoNotPublish;

    VerifyOrExit(Get<RoutingManager>().IsRunning());

    if (Get<RoutingManager>().mOmrPrefixManager.GetFavoredPrefix().IsInfrastructureDerived() &&
        Get<RoutingManager>().mDiscoveredPrefixTable.ContainsDefaultOrNonUlaRoutePrefix())
    {
        newState = kPublishDefault;
    }
    else if (Get<RoutingManager>().mDiscoveredPrefixTable.ContainsNonUlaOnLinkPrefix())
    {
        newState = kPublishDefault;
    }
    else if (Get<RoutingManager>().mDiscoveredPrefixTable.ContainsUlaOnLinkPrefix() ||
             Get<RoutingManager>().mOnLinkPrefixManager.ShouldPublishUlaRoute())
    {
        newState = kPublishUla;
    }

exit:
    if (newState != mState)
    {
        LogInfo("RoutePublisher state: %s -> %s", StateToString(mState), StateToString(newState));
        UpdatePublishedRoute(newState);
        Get<RoutingManager>().mOmrPrefixManager.UpdateDefaultRouteFlag(newState == kPublishDefault);
    }
}

void RoutingManager::RoutePublisher::DeterminePrefixFor(State aState, Ip6::Prefix &aPrefix) const
{
    aPrefix.Clear();

    switch (aState)
    {
    case kDoNotPublish:
    case kPublishDefault:
        // `Clear()` will set the prefix `::/0`.
        break;
    case kPublishUla:
        aPrefix = GetUlaPrefix();
        break;
    }
}

void RoutingManager::RoutePublisher::UpdatePublishedRoute(State aNewState)
{
    // Updates the published route entry in Network Data, transitioning
    // from current `mState` to new `aNewState`. This method can be used
    // when there is no change to `mState` but a change to `mPreference`
    // or `mAdvPioFlag`.

    Ip6::Prefix                      oldPrefix;
    NetworkData::ExternalRouteConfig routeConfig;

    DeterminePrefixFor(mState, oldPrefix);

    if (aNewState == kDoNotPublish)
    {
        VerifyOrExit(mState != kDoNotPublish);
        IgnoreError(Get<NetworkData::Publisher>().UnpublishPrefix(oldPrefix));
        ExitNow();
    }

    routeConfig.Clear();
    routeConfig.mPreference = mPreference;
    routeConfig.mAdvPio     = mAdvPioFlag;
    routeConfig.mStable     = true;
    DeterminePrefixFor(aNewState, routeConfig.GetPrefix());

    // If we were not publishing a route prefix before, publish the new
    // `routeConfig`. Otherwise, use `ReplacePublishedExternalRoute()` to
    // replace the previously published prefix entry. This ensures that we do
    // not have a situation where the previous route is removed while the new
    // one is not yet added in the Network Data.

    if (mState == kDoNotPublish)
    {
        SuccessOrAssert(Get<NetworkData::Publisher>().PublishExternalRoute(
            routeConfig, NetworkData::Publisher::kFromRoutingManager));
    }
    else
    {
        SuccessOrAssert(Get<NetworkData::Publisher>().ReplacePublishedExternalRoute(
            oldPrefix, routeConfig, NetworkData::Publisher::kFromRoutingManager));
    }

exit:
    mState = aNewState;
}

void RoutingManager::RoutePublisher::Unpublish(void)
{
    // Unpublish the previously published route based on `mState`
    // and update `mState`.

    Ip6::Prefix prefix;

    VerifyOrExit(mState != kDoNotPublish);
    DeterminePrefixFor(mState, prefix);
    IgnoreError(Get<NetworkData::Publisher>().UnpublishPrefix(prefix));
    mState = kDoNotPublish;

exit:
    return;
}

void RoutingManager::RoutePublisher::UpdateAdvPioFlags(bool aAdvPioFlag)
{
    VerifyOrExit(mAdvPioFlag != aAdvPioFlag);
    mAdvPioFlag = aAdvPioFlag;
    UpdatePublishedRoute(mState);

exit:
    return;
}

void RoutingManager::RoutePublisher::SetPreference(RoutePreference aPreference)
{
    LogInfo("User explicitly set published route preference to %s", RoutePreferenceToString(aPreference));
    mUserSetPreference = true;
    mTimer.Stop();
    UpdatePreference(aPreference);
}

void RoutingManager::RoutePublisher::ClearPreference(void)
{
    VerifyOrExit(mUserSetPreference);

    LogInfo("User cleared explicitly set published route preference - set based on role");
    mUserSetPreference = false;
    SetPreferenceBasedOnRole();

exit:
    return;
}

void RoutingManager::RoutePublisher::SetPreferenceBasedOnRole(void)
{
    RoutePreference preference = NetworkData::kRoutePreferenceMedium;

    if (Get<Mle::Mle>().IsChild() && (Get<Mle::Mle>().GetParent().GetTwoWayLinkQuality() != kLinkQuality3))
    {
        preference = NetworkData::kRoutePreferenceLow;
    }

    UpdatePreference(preference);
    mTimer.Stop();
}

void RoutingManager::RoutePublisher::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(!mUserSetPreference);

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        SetPreferenceBasedOnRole();
    }

    if (aEvents.Contains(kEventParentLinkQualityChanged))
    {
        VerifyOrExit(Get<Mle::Mle>().IsChild());

        if (Get<Mle::Mle>().GetParent().GetTwoWayLinkQuality() == kLinkQuality3)
        {
            VerifyOrExit(!mTimer.IsRunning());
            mTimer.Start(kDelayBeforePrfUpdateOnLinkQuality3);
        }
        else
        {
            UpdatePreference(NetworkData::kRoutePreferenceLow);
            mTimer.Stop();
        }
    }

exit:
    return;
}

void RoutingManager::RoutePublisher::HandleTimer(void) { SetPreferenceBasedOnRole(); }

void RoutingManager::RoutePublisher::UpdatePreference(RoutePreference aPreference)
{
    VerifyOrExit(mPreference != aPreference);

    LogInfo("Published route preference changed: %s -> %s", RoutePreferenceToString(mPreference),
            RoutePreferenceToString(aPreference));
    mPreference = aPreference;
    UpdatePublishedRoute(mState);

exit:
    return;
}

const char *RoutingManager::RoutePublisher::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "none",      // (0) kDoNotPublish
        "def-route", // (1) kPublishDefault
        "ula",       // (2) kPublishUla
    };

    static_assert(0 == kDoNotPublish, "kDoNotPublish value is incorrect");
    static_assert(1 == kPublishDefault, "kPublishDefault value is incorrect");
    static_assert(2 == kPublishUla, "kPublishUla value is incorrect");

    return kStateStrings[aState];
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
        IgnoreError(Get<NetworkData::Publisher>().UnpublishPrefix(mPublishedPrefix));
    }

    mPublishedPrefix.Clear();
    mInfraIfPrefix.Clear();
    mTimer.Stop();

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    Get<Nat64::Translator>().ClearNat64Prefix();
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
    const Ip6::Prefix *favoredPrefix = &mLocalPrefix;

    aPreference = NetworkData::kRoutePreferenceLow;

    if (mInfraIfPrefix.IsValidNat64() &&
        Get<RoutingManager>().mOmrPrefixManager.GetFavoredPrefix().IsInfrastructureDerived())
    {
        favoredPrefix = &mInfraIfPrefix;
        aPreference   = NetworkData::kRoutePreferenceMedium;
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
        IgnoreError(Get<NetworkData::Publisher>().UnpublishPrefix(mPublishedPrefix));
        mPublishedPrefix.Clear();
    }

    if (shouldPublish && ((prefix != mPublishedPrefix) || (preference != mPublishedPreference)))
    {
        mPublishedPrefix     = prefix;
        mPublishedPreference = preference;
        Publish();
    }

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    // When there is an prefix other than mLocalPrefix, means there is an external translator available. So we bypass
    // the NAT64 translator by clearing the NAT64 prefix in the translator.
    if (mPublishedPrefix == mLocalPrefix)
    {
        Get<Nat64::Translator>().SetNat64Prefix(mLocalPrefix);
    }
    else
    {
        Get<Nat64::Translator>().ClearNat64Prefix();
    }
#endif

exit:
    return;
}

void RoutingManager::Nat64PrefixManager::Publish(void)
{
    NetworkData::ExternalRouteConfig routeConfig;

    routeConfig.Clear();
    routeConfig.SetPrefix(mPublishedPrefix);
    routeConfig.mPreference = mPublishedPreference;
    routeConfig.mStable     = true;
    routeConfig.mNat64      = true;

    SuccessOrAssert(
        Get<NetworkData::Publisher>().PublishExternalRoute(routeConfig, NetworkData::Publisher::kFromRoutingManager));
}

void RoutingManager::Nat64PrefixManager::HandleTimer(void)
{
    OT_ASSERT(mEnabled);

    Discover();

    mTimer.Start(TimeMilli::SecToMsec(kDefaultNat64PrefixLifetime));
    LogInfo("NAT64 prefix timer scheduled in %lu seconds", ToUlong(kDefaultNat64PrefixLifetime));
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
    Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
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

    LogInfo("RsSender: Starting - will send first RS in %lu msec", ToUlong(delay));

    mTxCount   = 0;
    mStartTime = TimerMilli::GetNow();
    mTimer.Start(delay);

exit:
    return;
}

void RoutingManager::RsSender::Stop(void) { mTimer.Stop(); }

Error RoutingManager::RsSender::SendRs(void)
{
    Ip6::Address                  destAddress;
    Ip6::Nd::RouterSolicitMessage routerSolicit;
    InfraIf::Icmp6Packet          packet;
    Error                         error;

    packet.InitFrom(routerSolicit);
    destAddress.SetToLinkLocalAllRoutersMulticast();

    error = Get<RoutingManager>().mInfraIf.Send(packet, destAddress);

    if (error == kErrorNone)
    {
        Get<Ip6::Ip6>().GetBorderRoutingCounters().mRsTxSuccess++;
    }
    else
    {
        Get<Ip6::Ip6>().GetBorderRoutingCounters().mRsTxFailure++;
    }
    return error;
}

void RoutingManager::RsSender::HandleTimer(void)
{
    Error    error;
    uint32_t delay;

    if (mTxCount >= kMaxTxCount)
    {
        LogInfo("RsSender: Finished sending RS msgs and waiting for RAs");
        Get<RoutingManager>().HandleRsSenderFinished(mStartTime);
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

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
const char *RoutingManager::PdPrefixManager::StateToString(Dhcp6PdState aState)
{
    static const char *const kStateStrings[] = {
        "Disabled", // (0) kDisabled
        "Stopped",  // (1) kStopped
        "Running",  // (2) kRunning
    };

    static_assert(0 == kDhcp6PdStateDisabled, "kDhcp6PdStateDisabled value is incorrect");
    static_assert(1 == kDhcp6PdStateStopped, "kDhcp6PdStateStopped value is incorrect");
    static_assert(2 == kDhcp6PdStateRunning, "kDhcp6PdStateRunning value is incorrect");

    return kStateStrings[aState];
}

RoutingManager::PdPrefixManager::PdPrefixManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
    , mIsRunning(false)
    , mNumPlatformPioProcessed(0)
    , mNumPlatformRaReceived(0)
    , mLastPlatformRaTime(0)
    , mTimer(aInstance)
{
    mPrefix.Clear();
}

void RoutingManager::PdPrefixManager::StartStop(bool aStart)
{
    Dhcp6PdState oldState = GetState();

    VerifyOrExit(aStart != mIsRunning);
    mIsRunning = aStart;
    EvaluateStateChange(oldState);

exit:
    return;
}

RoutingManager::Dhcp6PdState RoutingManager::PdPrefixManager::GetState(void) const
{
    Dhcp6PdState state = kDhcp6PdStateDisabled;

    if (mEnabled)
    {
        state = mIsRunning ? kDhcp6PdStateRunning : kDhcp6PdStateStopped;
    }

    return state;
}

void RoutingManager::PdPrefixManager::EvaluateStateChange(Dhcp6PdState aOldState)
{
    Dhcp6PdState newState = GetState();

    VerifyOrExit(aOldState != newState);
    LogInfo("PdPrefixManager: %s -> %s", StateToString(aOldState), StateToString(newState));

    // TODO: We may also want to inform the platform that PD is stopped.
    switch (newState)
    {
    case kDhcp6PdStateDisabled:
    case kDhcp6PdStateStopped:
        WithdrawPrefix();
        break;
    case kDhcp6PdStateRunning:
        break;
    }

exit:
    return;
}

Error RoutingManager::PdPrefixManager::GetPrefixInfo(PrefixTableEntry &aInfo) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsRunning() && HasPrefix(), error = kErrorNotFound);

    aInfo.mPrefix              = mPrefix.GetPrefix();
    aInfo.mValidLifetime       = mPrefix.GetValidLifetime();
    aInfo.mPreferredLifetime   = mPrefix.GetPreferredLifetime();
    aInfo.mMsecSinceLastUpdate = TimerMilli::GetNow() - mPrefix.GetLastUpdateTime();

exit:
    return error;
}

Error RoutingManager::PdPrefixManager::GetProcessedRaInfo(PdProcessedRaInfo &aPdProcessedRaInfo) const
{
    Error error = kErrorNone;

    VerifyOrExit(IsRunning() && HasPrefix(), error = kErrorNotFound);

    aPdProcessedRaInfo.mNumPlatformRaReceived   = mNumPlatformRaReceived;
    aPdProcessedRaInfo.mNumPlatformPioProcessed = mNumPlatformPioProcessed;
    aPdProcessedRaInfo.mLastPlatformRaMsec      = TimerMilli::GetNow() - mLastPlatformRaTime;

exit:
    return error;
}

void RoutingManager::PdPrefixManager::WithdrawPrefix(void)
{
    VerifyOrExit(HasPrefix());

    LogInfo("Withdrew platform provided outdated prefix: %s", mPrefix.GetPrefix().ToString().AsCString());

    mPrefix.Clear();
    mTimer.Stop();

    Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kImmediately);

exit:
    return;
}

void RoutingManager::PdPrefixManager::ProcessPlatformGeneratedRa(const uint8_t *aRouterAdvert, const uint16_t aLength)
{
    Error                                     error = kErrorNone;
    Ip6::Nd::RouterAdvertMessage::Icmp6Packet packet;

    VerifyOrExit(IsRunning(), LogWarn("Ignore platform generated RA since PD is disabled or not running."));
    packet.Init(aRouterAdvert, aLength);
    error = Process(Ip6::Nd::RouterAdvertMessage(packet));
    mNumPlatformRaReceived++;
    mLastPlatformRaTime = TimerMilli::GetNow();

exit:
    if (error != kErrorNone)
    {
        LogCrit("Failed to process platform generated ND OnMeshPrefix: %s", ErrorToString(error));
    }
}

Error RoutingManager::PdPrefixManager::Process(const Ip6::Nd::RouterAdvertMessage &aMessage)
{
    Error                        error = kErrorNone;
    DiscoveredPrefixTable::Entry favoredEntry;
    bool                         currentPrefixUpdated = false;

    VerifyOrExit(aMessage.IsValid(), error = kErrorParse);
    favoredEntry.Clear();

    for (const Ip6::Nd::Option &option : aMessage)
    {
        DiscoveredPrefixTable::Entry entry;

        if (option.GetType() != Ip6::Nd::Option::Type::kTypePrefixInfo ||
            !static_cast<const Ip6::Nd::PrefixInfoOption &>(option).IsValid())
        {
            continue;
        }
        mNumPlatformPioProcessed++;
        entry.SetFrom(static_cast<const Ip6::Nd::PrefixInfoOption &>(option));

        if (!IsValidPdPrefix(entry.GetPrefix()))
        {
            LogWarn("PdPrefixManager: Ignore invalid PIO entry %s", entry.GetPrefix().ToString().AsCString());
            continue;
        }

        entry.mPrefix.Tidy();
        entry.mPrefix.SetLength(kOmrPrefixLength);

        // The platform may send another RA message to announce that the current prefix we are using is no longer
        // preferred or no longer valid.
        if (entry.GetPrefix() == GetPrefix())
        {
            currentPrefixUpdated = true;
            mPrefix              = entry;
        }

        if (entry.IsDeprecated())
        {
            continue;
        }

        // Some platforms may delegate us more than one prefixes. We will pick the smallest one. This is a simple rule
        // to pick the GUA prefix from the RA messages since GUA prefixes (2000::/3) are always smaller than ULA
        // prefixes (fc00::/7).
        if (favoredEntry.GetPrefix().GetLength() == 0 || entry.GetPrefix() < favoredEntry.GetPrefix())
        {
            favoredEntry = entry;
        }
    }

    if (currentPrefixUpdated && mPrefix.IsDeprecated())
    {
        LogInfo("PdPrefixManager: Prefix %s is deprecated", mPrefix.GetPrefix().ToString().AsCString());
        mPrefix.Clear();
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kImmediately);
    }

    if (!HasPrefix() || (favoredEntry.GetPrefix().GetLength() != 0 && favoredEntry.GetPrefix() < mPrefix.GetPrefix()))
    {
        mPrefix = favoredEntry;
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kImmediately);
    }

exit:
    if (HasPrefix())
    {
        mTimer.FireAt(mPrefix.GetStaleTime());
    }
    else
    {
        mTimer.Stop();
    }

    return error;
}

void RoutingManager::PdPrefixManager::SetEnabled(bool aEnabled)
{
    Dhcp6PdState oldState = GetState();

    VerifyOrExit(mEnabled != aEnabled);
    mEnabled = aEnabled;
    EvaluateStateChange(oldState);

exit:
    return;
}

extern "C" void otPlatBorderRoutingProcessIcmp6Ra(otInstance *aInstance, const uint8_t *aMessage, uint16_t aLength)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().ProcessPlatformGeneratedRa(aMessage, aLength);
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
