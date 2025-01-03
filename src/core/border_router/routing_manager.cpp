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
 */

#include "border_router/routing_manager.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <string.h>

#include <openthread/border_router.h>
#include <openthread/platform/border_routing.h>
#include <openthread/platform/infra_if.h>

#include "instance/instance.hpp"

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
#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
    , mNetDataPeerBrTracker(aInstance)
#endif
    , mRxRaTracker(aInstance)
    , mRoutePublisher(aInstance)
#if OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
    , mNat64PrefixManager(aInstance)
#endif
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    , mPdPrefixManager(aInstance)
#endif
    , mRsSender(aInstance)
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

#if OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE && OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF
        IgnoreError(Get<Dns::Multicast::Core>().SetEnabled(false, mInfraIf.GetIfIndex()));
#endif

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
        mRxRaTracker.Start();
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

    mRxRaTracker.Stop();

    mTxRaInfo.mTxCount = 0;

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

Error RoutingManager::SetExtraRouterAdvertOptions(const uint8_t *aOptions, uint16_t aLength)
{
    Error error = kErrorNone;

    if (aOptions == nullptr)
    {
        mExtraRaOptions.Free();
    }
    else
    {
        error = mExtraRaOptions.SetFrom(aOptions, aLength);
    }

    return error;
}

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
void RoutingManager::HandleSrpServerAutoEnableMode(void)
{
    VerifyOrExit(Get<Srp::Server>().IsAutoEnableMode());

    if (IsInitialPolicyEvaluationDone())
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

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
    mNetDataPeerBrTracker.HandleNotifierEvents(aEvents);
#endif

    VerifyOrExit(IsInitialized() && IsEnabled());

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        EvaluateState();
    }

    if (mIsRunning && aEvents.Contains(kEventThreadNetdataChanged))
    {
        mRxRaTracker.HandleNetDataChange();
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
#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE
    mPdPrefixManager.Evaluate();
#endif

    if (IsInitialPolicyEvaluationDone())
    {
        SendRouterAdvertisement(kAdvPrefixesFromNetData);
    }

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
    if (Get<Srp::Server>().IsAutoEnableMode() && IsInitialPolicyEvaluationDone())
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

bool RoutingManager::IsInitialPolicyEvaluationDone(void) const
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
    TimeMilli now      = TimerMilli::GetNow();
    uint32_t  delay    = 0;
    uint32_t  interval = 0;
    uint16_t  jitter   = 0;
    TimeMilli evaluateTime;

    VerifyOrExit(mIsRunning);

    switch (aMode)
    {
    case kImmediately:
        break;

    case kForNextRa:
        if (mTxRaInfo.mTxCount <= kInitalRaTxCount)
        {
            interval = kInitalRaInterval;
            jitter   = kInitialRaJitter;
        }
        else
        {
            interval = kRaBeaconInterval;
            jitter   = kRaBeaconJitter;
        }

        break;

    case kAfterRandomDelay:
        interval = kEvaluationInterval;
        jitter   = kEvaluationJitter;
        break;

    case kToReplyToRs:
        interval = kRsReplyInterval;
        jitter   = kRsReplyJitter;
        break;
    }

    delay = Random::NonCrypto::AddJitter(interval, jitter);

    // Ensure we wait a min delay after last RA tx
    evaluateTime = Max(now + delay, mTxRaInfo.mLastTxTime + kMinDelayBetweenRas);

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
    Error                   error = kErrorNone;
    RouterAdvert::TxMessage raMsg;
    RouterAdvert::Header    header;
    Ip6::Address            destAddress;
    InfraIf::Icmp6Packet    packet;
    LinkLayerAddress        linkAddr;

    LogInfo("Preparing RA");

    if (mRxRaTracker.GetLocalRaHeaderToMirror().IsValid())
    {
        header = mRxRaTracker.GetLocalRaHeaderToMirror();
    }
    else
    {
        header.SetToDefault();
    }

    mRxRaTracker.SetHeaderFlagsOn(header);
    header.SetSnacRouterFlag();

    SuccessOrExit(error = raMsg.Append(header));

    LogRaHeader(header);

    // Append PIO for local on-link prefix if is either being
    // advertised or deprecated and for old prefix if is being
    // deprecated.

    SuccessOrExit(error = mOnLinkPrefixManager.AppendAsPiosTo(raMsg));

    if (aRaTxMode == kInvalidateAllPrevPrefixes)
    {
        SuccessOrExit(error = mRioAdvertiser.InvalidatPrevRios(raMsg));
    }
    else
    {
        SuccessOrExit(error = mRioAdvertiser.AppendRios(raMsg));
    }

    if (Get<InfraIf>().GetLinkLayerAddress(linkAddr) == kErrorNone)
    {
        SuccessOrExit(error = raMsg.AppendLinkLayerOption(linkAddr, Option::kSourceLinkLayerAddr));
    }

    if (mExtraRaOptions.GetLength() > 0)
    {
        SuccessOrExit(error = raMsg.AppendBytes(mExtraRaOptions.GetBytes(), mExtraRaOptions.GetLength()));
    }

    // A valid RA message should contain at lease one option.
    // Exit when the size of packet is less than the size of header.
    VerifyOrExit(raMsg.ContainsAnyOptions());

    destAddress.SetToLinkLocalAllNodesMulticast();
    raMsg.GetAsPacket(packet);

    mTxRaInfo.IncrementTxCountAndSaveHash(packet);

    SuccessOrExit(error = mInfraIf.Send(packet, destAddress));

    mTxRaInfo.mLastTxTime = TimerMilli::GetNow();
    Get<Ip6::Ip6>().GetBorderRoutingCounters().mRaTxSuccess++;
    LogInfo("Sent RA on %s", mInfraIf.ToString().AsCString());
    DumpDebg("[BR-CERT] direction=send | type=RA |", packet.GetBytes(), packet.GetLength());

exit:
    if (error != kErrorNone)
    {
        Get<Ip6::Ip6>().GetBorderRoutingCounters().mRaTxFailure++;
        LogWarn("Failed to send RA on %s: %s", mInfraIf.ToString().AsCString(), ErrorToString(error));
    }
}

TimeMilli RoutingManager::CalculateExpirationTime(TimeMilli aUpdateTime, uint32_t aLifetime)
{
    // `aLifetime` is in unit of seconds. We clamp the lifetime to max
    // interval supported by `Timer` (`2^31` msec or ~24.8 days).
    // This ensures that the time calculation fits within `TimeMilli`
    // range.

    static constexpr uint32_t kMaxLifetime = Time::MsecToSec(Timer::kMaxDelay);

    return aUpdateTime + Time::SecToMsec(Min(aLifetime, kMaxLifetime));
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

bool RoutingManager::IsValidOnLinkPrefix(const PrefixInfoOption &aPio)
{
    Ip6::Prefix prefix;

    aPio.GetPrefix(prefix);

    return IsValidOnLinkPrefix(prefix) && aPio.IsOnLinkFlagSet() &&
           (aPio.IsAutoAddrConfigFlagSet() || aPio.IsDhcp6PdPreferredFlagSet());
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

    mRxRaTracker.RemoveOrDeprecateOldEntries(aStartTime);
    ScheduleRoutingPolicyEvaluation(kImmediately);
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
    const NeighborAdvertMessage *naMsg;

    VerifyOrExit(aPacket.GetLength() >= sizeof(NeighborAdvertMessage));
    naMsg = reinterpret_cast<const NeighborAdvertMessage *>(aPacket.GetBytes());

    mRxRaTracker.ProcessNeighborAdvertMessage(*naMsg);

exit:
    return;
}

void RoutingManager::HandleRouterAdvertisement(const InfraIf::Icmp6Packet &aPacket, const Ip6::Address &aSrcAddress)
{
    RouterAdvert::RxMessage raMsg(aPacket);
    RouterAdvOrigin         raOrigin = kAnotherRouter;

    OT_ASSERT(mIsRunning);

    VerifyOrExit(raMsg.IsValid());

    Get<Ip6::Ip6>().GetBorderRoutingCounters().mRaRx++;

    if (mInfraIf.HasAddress(aSrcAddress))
    {
        raOrigin = mTxRaInfo.IsRaFromManager(raMsg) ? kThisBrRoutingManager : kThisBrOtherEntity;
    }

    LogInfo("Received RA from %s on %s %s", aSrcAddress.ToString().AsCString(), mInfraIf.ToString().AsCString(),
            RouterAdvOriginToString(raOrigin));

    DumpDebg("[BR-CERT] direction=recv | type=RA |", aPacket.GetBytes(), aPacket.GetLength());

    mRxRaTracker.ProcessRouterAdvertMessage(raMsg, aSrcAddress, raOrigin);

exit:
    return;
}

void RoutingManager::HandleRaPrefixTableChanged(void)
{
    // This is a callback from `mRxRaTracker` indicating that
    // there has been a change in the table.

    VerifyOrExit(mIsRunning);

    mOnLinkPrefixManager.HandleRaPrefixTableChanged();
    mRoutePublisher.Evaluate();

exit:
    return;
}

void RoutingManager::HandleLocalOnLinkPrefixChanged(void)
{
    // This is a callback from `OnLinkPrefixManager` indicating
    // that the local on-link prefix is changed. The local on-link
    // prefix is derived from extended PAN ID.

    VerifyOrExit(mIsRunning);

    mRoutePublisher.Evaluate();
    mRxRaTracker.HandleLocalOnLinkPrefixChanged();
    ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);

exit:
    return;
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

#if OPENTHREAD_CONFIG_BORDER_ROUTING_REACHABILITY_CHECK_ICMP6_ERROR_ENABLE

void RoutingManager::CheckReachabilityToSendIcmpError(const Message &aMessage, const Ip6::Header &aIp6Header)
{
    bool                            matchesUlaOmrLowPrf = false;
    NetworkData::Iterator           iterator            = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;
    Ip6::MessageInfo                messageInfo;

    VerifyOrExit(IsRunning() && IsInitialPolicyEvaluationDone());

    VerifyOrExit(!aIp6Header.GetDestination().IsMulticast());

    // Validate that source matches a ULA OMR prefix with low preference
    // (indicating it is not infrastructure-derived).

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
    {
        if (IsValidOmrPrefix(prefixConfig) && prefixConfig.GetPrefix().IsUniqueLocal() &&
            aIp6Header.GetSource().MatchesPrefix(prefixConfig.GetPrefix()))
        {
            if (prefixConfig.GetPreference() >= NetworkData::kRoutePreferenceMedium)
            {
                matchesUlaOmrLowPrf = false;
                break;
            }

            matchesUlaOmrLowPrf = true;

            // Keep checking other prefixes, as the same prefix might
            // be added with a higher preference by another BR.
        }
    }

    VerifyOrExit(matchesUlaOmrLowPrf);

    VerifyOrExit(!mRxRaTracker.IsAddressOnLink(aIp6Header.GetDestination()));
    VerifyOrExit(!mRxRaTracker.IsAddressReachableThroughExplicitRoute(aIp6Header.GetDestination()));
    VerifyOrExit(!Get<NetworkData::Leader>().IsNat64(aIp6Header.GetDestination()));

    LogInfo("Send ICMP unreachable for fwd msg with local ULA src and non-local dst");
    LogInfo("   src: %s", aIp6Header.GetSource().ToString().AsCString());
    LogInfo("   dst: %s", aIp6Header.GetDestination().ToString().AsCString());

    messageInfo.Clear();
    messageInfo.SetPeerAddr(aIp6Header.GetSource());

    IgnoreError(Get<Ip6::Icmp>().SendError(Ip6::Icmp::Header::kTypeDstUnreach,
                                           Ip6::Icmp::Header::kCodeDstUnreachProhibited, messageInfo, aMessage));

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_REACHABILITY_CHECK_ICMP6_ERROR_ENABLE

#if OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

void RoutingManager::LogRaHeader(const RouterAdvert::Header &aRaHeader)
{
    LogInfo("- RA Header - flags - M:%u O:%u S:%u", aRaHeader.IsManagedAddressConfigFlagSet(),
            aRaHeader.IsOtherConfigFlagSet(), aRaHeader.IsSnacRouterFlagSet());
    LogInfo("- RA Header - default route - lifetime:%u", aRaHeader.GetRouterLifetime());
}

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

const char *RoutingManager::RouterAdvOriginToString(RouterAdvOrigin aRaOrigin)
{
    static const char *const kOriginStrings[] = {
        "",                          // (0) kAnotherRouter
        "(this BR routing-manager)", // (1) kThisBrRoutingManager
        "(this BR other sw entity)", // (2) kThisBrOtherEntity
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kAnotherRouter);
        ValidateNextEnum(kThisBrRoutingManager);
        ValidateNextEnum(kThisBrOtherEntity);
    };

    return kOriginStrings[aRaOrigin];
}

#else

void RoutingManager::LogRaHeader(const RouterAdvert::Header &) {}
void RoutingManager::LogPrefixInfoOption(const Ip6::Prefix &, uint32_t, uint32_t) {}
void RoutingManager::LogRouteInfoOption(const Ip6::Prefix &, uint32_t, RoutePreference) {}

#endif // OT_SHOULD_LOG_AT(OT_LOG_LEVEL_INFO)

//---------------------------------------------------------------------------------------------------------------------
// LifetimedPrefix

TimeMilli RoutingManager::LifetimedPrefix::CalculateExpirationTime(uint32_t aLifetime) const
{
    // `aLifetime` is in unit of seconds. This method ensures
    // that the time calculation fits with `TimeMilli` range.

    return RoutingManager::CalculateExpirationTime(mLastUpdateTime, aLifetime);
}

//---------------------------------------------------------------------------------------------------------------------
// OnLinkPrefix

void RoutingManager::OnLinkPrefix::SetFrom(const PrefixInfoOption &aPio)
{
    aPio.GetPrefix(mPrefix);
    mValidLifetime     = aPio.GetValidLifetime();
    mPreferredLifetime = aPio.GetPreferredLifetime();
    mLastUpdateTime    = TimerMilli::GetNow();
}

void RoutingManager::OnLinkPrefix::SetFrom(const PrefixTableEntry &aPrefixTableEntry)
{
    mPrefix            = AsCoreType(&aPrefixTableEntry.mPrefix);
    mValidLifetime     = aPrefixTableEntry.mValidLifetime;
    mPreferredLifetime = aPrefixTableEntry.mPreferredLifetime;
    mLastUpdateTime    = TimerMilli::GetNow();
}

bool RoutingManager::OnLinkPrefix::IsDeprecated(void) const { return GetDeprecationTime() <= TimerMilli::GetNow(); }

TimeMilli RoutingManager::OnLinkPrefix::GetDeprecationTime(void) const
{
    return CalculateExpirationTime(mPreferredLifetime);
}

TimeMilli RoutingManager::OnLinkPrefix::GetStaleTime(void) const
{
    return CalculateExpirationTime(Min(kStaleTime, mPreferredLifetime));
}

void RoutingManager::OnLinkPrefix::AdoptValidAndPreferredLifetimesFrom(const OnLinkPrefix &aPrefix)
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

void RoutingManager::OnLinkPrefix::CopyInfoTo(PrefixTableEntry &aEntry, TimeMilli aNow) const
{
    aEntry.mPrefix              = GetPrefix();
    aEntry.mIsOnLink            = true;
    aEntry.mMsecSinceLastUpdate = aNow - GetLastUpdateTime();
    aEntry.mValidLifetime       = GetValidLifetime();
    aEntry.mPreferredLifetime   = GetPreferredLifetime();
}

bool RoutingManager::OnLinkPrefix::IsFavoredOver(const Ip6::Prefix &aPrefix) const
{
    bool isFavored = false;

    // Validate that the `OnLinkPrefix` is eligible to be considered a
    // favored on-link prefix. It must not be deprecated and have a
    // preferred lifetime exceeding a minimum (1800 seconds).

    VerifyOrExit(!IsDeprecated());
    VerifyOrExit(GetPreferredLifetime() >= kFavoredMinPreferredLifetime);

    // Numerically smaller prefix is favored (unless `aPrefix` is empty).

    VerifyOrExit(aPrefix.GetLength() != 0, isFavored = true);

    isFavored = GetPrefix() < aPrefix;

exit:
    return isFavored;
}

//---------------------------------------------------------------------------------------------------------------------
// RoutePrefix

void RoutingManager::RoutePrefix::SetFrom(const RouteInfoOption &aRio)
{
    aRio.GetPrefix(mPrefix);
    mValidLifetime   = aRio.GetRouteLifetime();
    mRoutePreference = aRio.GetPreference();
    mLastUpdateTime  = TimerMilli::GetNow();
}

void RoutingManager::RoutePrefix::SetFrom(const RouterAdvert::Header &aRaHeader)
{
    mPrefix.Clear();
    mValidLifetime   = aRaHeader.GetRouterLifetime();
    mRoutePreference = aRaHeader.GetDefaultRouterPreference();
    mLastUpdateTime  = TimerMilli::GetNow();
}

TimeMilli RoutingManager::RoutePrefix::GetStaleTime(void) const
{
    return CalculateExpirationTime(Min(kStaleTime, mValidLifetime));
}

void RoutingManager::RoutePrefix::CopyInfoTo(PrefixTableEntry &aEntry, TimeMilli aNow) const
{
    aEntry.mPrefix              = GetPrefix();
    aEntry.mIsOnLink            = false;
    aEntry.mMsecSinceLastUpdate = aNow - GetLastUpdateTime();
    aEntry.mValidLifetime       = GetValidLifetime();
    aEntry.mPreferredLifetime   = 0;
    aEntry.mRoutePreference     = static_cast<otRoutePreference>(GetRoutePreference());
}

//---------------------------------------------------------------------------------------------------------------------
// NetDataPeerBrTracker

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
RoutingManager::NetDataPeerBrTracker::NetDataPeerBrTracker(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

uint16_t RoutingManager::NetDataPeerBrTracker::CountPeerBrs(uint32_t &aMinAge) const
{
    uint32_t uptime = Uptime::MsecToSec(Get<Uptime>().GetUptime());
    uint16_t count  = 0;

    aMinAge = NumericLimits<uint16_t>::kMax;

    for (const PeerBr &peerBr : mPeerBrs)
    {
        count++;
        aMinAge = Min(aMinAge, peerBr.GetAge(uptime));
    }

    if (count == 0)
    {
        aMinAge = 0;
    }

    return count;
}

Error RoutingManager::NetDataPeerBrTracker::GetNext(PrefixTableIterator &aIterator, PeerBrEntry &aEntry) const
{
    using Iterator = RxRaTracker::Iterator;

    Iterator &iterator = static_cast<Iterator &>(aIterator);
    Error     error;

    SuccessOrExit(error = iterator.AdvanceToNextPeerBr(mPeerBrs.GetHead()));

    aEntry.mRloc16 = iterator.GetPeerBrEntry()->mRloc16;
    aEntry.mAge    = iterator.GetPeerBrEntry()->GetAge(iterator.GetInitUptime());

exit:
    return error;
}

void RoutingManager::NetDataPeerBrTracker::HandleNotifierEvents(Events aEvents)
{
    NetworkData::Rlocs rlocs;

    VerifyOrExit(aEvents.ContainsAny(kEventThreadNetdataChanged | kEventThreadRoleChanged));

    Get<NetworkData::Leader>().FindRlocs(NetworkData::kBrProvidingExternalIpConn, NetworkData::kAnyRole, rlocs);

    // Remove `PeerBr` entries no longer found in Network Data,
    // or they match the device RLOC16. Then allocate and add
    // entries for newly discovered peers.

    mPeerBrs.RemoveAndFreeAllMatching(PeerBr::Filter(rlocs));
    mPeerBrs.RemoveAndFreeAllMatching(Get<Mle::Mle>().GetRloc16());

    for (uint16_t rloc16 : rlocs)
    {
        PeerBr *newEntry;

        if (Get<Mle::Mle>().HasRloc16(rloc16) || mPeerBrs.ContainsMatching(rloc16))
        {
            continue;
        }

        newEntry = PeerBr::Allocate();
        VerifyOrExit(newEntry != nullptr, LogWarn("Failed to allocate `PeerBr` entry"));

        newEntry->mRloc16       = rloc16;
        newEntry->mDiscoverTime = Uptime::MsecToSec(Get<Uptime>().GetUptime());

        mPeerBrs.Push(*newEntry);
    }

exit:
    return;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker

RoutingManager::RxRaTracker::RxRaTracker(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mExpirationTimer(aInstance)
    , mStaleTimer(aInstance)
    , mRouterTimer(aInstance)
    , mSignalTask(aInstance)
{
    mLocalRaHeader.Clear();
}

void RoutingManager::RxRaTracker::Start(void) { HandleNetDataChange(); }

void RoutingManager::RxRaTracker::Stop(void)
{
    mRouters.Free();
    mLocalRaHeader.Clear();
    mDecisionFactors.Clear();

    mExpirationTimer.Stop();
    mStaleTimer.Stop();
    mRouterTimer.Stop();
}

void RoutingManager::RxRaTracker::ProcessRouterAdvertMessage(const RouterAdvert::RxMessage &aRaMessage,
                                                             const Ip6::Address            &aSrcAddress,
                                                             RouterAdvOrigin                aRaOrigin)
{
    // Process a received RA message and update the prefix table.

    Router *router;

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
        router->mDiscoverTime = Uptime::MsecToSec(Get<Uptime>().GetUptime());
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

void RoutingManager::RxRaTracker::ProcessRaHeader(const RouterAdvert::Header &aRaHeader,
                                                  Router                     &aRouter,
                                                  RouterAdvOrigin             aRaOrigin)
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
            Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kAfterRandomDelay);
        }
    }

    prefix.Clear();
    entry = aRouter.mRoutePrefixes.FindMatching(prefix);

    LogInfo("- RA Header - default route - lifetime:%u", aRaHeader.GetRouterLifetime());

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

void RoutingManager::RxRaTracker::ProcessPrefixInfoOption(const PrefixInfoOption &aPio, Router &aRouter)
{
    Ip6::Prefix          prefix;
    Entry<OnLinkPrefix> *entry;
    bool                 disregard;

    VerifyOrExit(aPio.IsValid());
    aPio.GetPrefix(prefix);

    if (!IsValidOnLinkPrefix(aPio))
    {
        LogInfo("- PIO %s - ignore since not a valid on-link prefix", prefix.ToString().AsCString());
        ExitNow();
    }

    // Disregard the PIO prefix if it matches our local on-link prefix,
    // as this indicates it's likely from a peer Border Router connected
    // to the same Thread mesh.

    disregard = (prefix == Get<RoutingManager>().mOnLinkPrefixManager.GetLocalPrefix());

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE
    VerifyOrExit(!disregard);
#endif

    LogPrefixInfoOption(prefix, aPio.GetValidLifetime(), aPio.GetPreferredLifetime());

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
        entry->AdoptValidAndPreferredLifetimesFrom(newPrefix);
    }

    entry->SetDisregardFlag(disregard);

exit:
    return;
}

void RoutingManager::RxRaTracker::ProcessRouteInfoOption(const RouteInfoOption &aRio, Router &aRouter)
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

#if !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE

template <>
RoutingManager::RxRaTracker::Entry<RoutingManager::RxRaTracker::Router> *RoutingManager::RxRaTracker::AllocateEntry(
    void)
{
    Entry<Router> *router = mRouterPool.Allocate();

    VerifyOrExit(router != nullptr);
    router->Init(GetInstance());

exit:
    return router;
}

template <class PrefixType>
RoutingManager::RxRaTracker::Entry<PrefixType> *RoutingManager::RxRaTracker::AllocateEntry(void)
{
    static_assert(TypeTraits::IsSame<PrefixType, OnLinkPrefix>::kValue ||
                      TypeTraits::IsSame<PrefixType, RoutePrefix>::kValue,
                  "PrefixType MSUT be either RoutePrefix or OnLinkPrefix");

    Entry<PrefixType> *entry       = nullptr;
    SharedEntry       *sharedEntry = mEntryPool.Allocate();

    VerifyOrExit(sharedEntry != nullptr);
    entry = &sharedEntry->GetEntry<PrefixType>();
    entry->Init(GetInstance());

exit:
    return entry;
}

template <> void RoutingManager::RxRaTracker::Entry<RoutingManager::RxRaTracker::Router>::Free(void)
{
    mOnLinkPrefixes.Free();
    mRoutePrefixes.Free();
    Get<RoutingManager>().mRxRaTracker.mRouterPool.Free(*this);
}

template <class PrefixType> void RoutingManager::RxRaTracker::Entry<PrefixType>::Free(void)
{
    static_assert(TypeTraits::IsSame<PrefixType, OnLinkPrefix>::kValue ||
                      TypeTraits::IsSame<PrefixType, RoutePrefix>::kValue,
                  "PrefixType MSUT be either RoutePrefix or OnLinkPrefix");

    Get<RoutingManager>().mRxRaTracker.mEntryPool.Free(*reinterpret_cast<SharedEntry *>(this));
}

#endif // !OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE

void RoutingManager::RxRaTracker::HandleLocalOnLinkPrefixChanged(void)
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

void RoutingManager::RxRaTracker::HandleNetDataChange(void)
{
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig prefixConfig;
    bool                            didChange = false;

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, prefixConfig) == kErrorNone)
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

void RoutingManager::RxRaTracker::RemoveOrDeprecateOldEntries(TimeMilli aTimeThreshold)
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
    }

    if (mLocalRaHeader.IsValid() && (mLocalRaHeaderUpdateTime <= aTimeThreshold))
    {
        mLocalRaHeader.Clear();
    }

    Evaluate();
}

void RoutingManager::RxRaTracker::Evaluate(void)
{
    DecisionFactors oldFactors = mDecisionFactors;
    TimeMilli       now        = TimerMilli::GetNow();
    NextFireTime    routerTimeoutTime(now);
    NextFireTime    entryExpireTime(now);
    NextFireTime    staleTime(now);

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Remove expired prefix entries in routers and then remove any
    // router that has no prefix entries or flags.

    mRouters.RemoveAndFreeAllMatching(Router::EmptyChecker(now));

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
    }

    if (mLocalRaHeader.IsValid())
    {
        uint16_t interval = kStaleTime;

        if (mLocalRaHeader.GetRouterLifetime() > 0)
        {
            interval = Min(interval, mLocalRaHeader.GetRouterLifetime());
        }

        staleTime.UpdateIfEarlier(CalculateExpirationTime(mLocalRaHeaderUpdateTime, interval));
    }

    mRouterTimer.FireAt(routerTimeoutTime);
    mExpirationTimer.FireAt(entryExpireTime);
    mStaleTimer.FireAt(staleTime);
}

void RoutingManager::RxRaTracker::DetermineStaleTimeFor(const OnLinkPrefix &aPrefix, NextFireTime &aStaleTime)
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

void RoutingManager::RxRaTracker::DetermineStaleTimeFor(const RoutePrefix &aPrefix, NextFireTime &aStaleTime)
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

void RoutingManager::RxRaTracker::HandleStaleTimer(void)
{
    VerifyOrExit(Get<RoutingManager>().IsRunning());

    LogInfo("Stale timer expired");
    Get<RoutingManager>().mRsSender.Start();

exit:
    return;
}

void RoutingManager::RxRaTracker::HandleExpirationTimer(void) { Evaluate(); }

void RoutingManager::RxRaTracker::HandleSignalTask(void) { Get<RoutingManager>().HandleRaPrefixTableChanged(); }

void RoutingManager::RxRaTracker::ProcessNeighborAdvertMessage(const NeighborAdvertMessage &aNaMessage)
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

void RoutingManager::RxRaTracker::HandleRouterTimer(void)
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
        }
    }

    Evaluate();
}

void RoutingManager::RxRaTracker::SendNeighborSolicitToRouter(const Router &aRouter)
{
    InfraIf::Icmp6Packet  packet;
    NeighborSolicitHeader nsHdr;
    TxMessage             nsMsg;
    LinkLayerAddress      linkAddr;

    VerifyOrExit(!Get<RoutingManager>().mRsSender.IsInProgress());

    nsHdr.SetTargetAddress(aRouter.mAddress);
    SuccessOrExit(nsMsg.Append(nsHdr));

    if (Get<InfraIf>().GetLinkLayerAddress(linkAddr) == kErrorNone)
    {
        SuccessOrExit(nsMsg.AppendLinkLayerOption(linkAddr, Option::kSourceLinkLayerAddr));
    }

    nsMsg.GetAsPacket(packet);

    IgnoreError(Get<RoutingManager>().mInfraIf.Send(packet, aRouter.mAddress));

    LogInfo("Sent Neighbor Solicitation to %s - attempt:%u/%u", aRouter.mAddress.ToString().AsCString(),
            aRouter.mNsProbeCount, Router::kMaxNsProbes);

exit:
    return;
}

void RoutingManager::RxRaTracker::SetHeaderFlagsOn(RouterAdvert::Header &aHeader) const
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

bool RoutingManager::RxRaTracker::IsAddressOnLink(const Ip6::Address &aAddress) const
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

bool RoutingManager::RxRaTracker::IsAddressReachableThroughExplicitRoute(const Ip6::Address &aAddress) const
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

void RoutingManager::RxRaTracker::InitIterator(PrefixTableIterator &aIterator) const
{
    static_cast<Iterator &>(aIterator).Init(mRouters.GetHead(), Uptime::MsecToSec(Get<Uptime>().GetUptime()));
}

Error RoutingManager::RxRaTracker::GetNextEntry(PrefixTableIterator &aIterator, PrefixTableEntry &aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    ClearAllBytes(aEntry);

    SuccessOrExit(error = iterator.AdvanceToNextEntry());

    iterator.GetRouter()->CopyInfoTo(aEntry.mRouter, iterator.GetInitTime(), iterator.GetInitUptime());

    switch (iterator.GetEntryType())
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

Error RoutingManager::RxRaTracker::GetNextRouter(PrefixTableIterator &aIterator, RouterEntry &aEntry) const
{
    Error     error    = kErrorNone;
    Iterator &iterator = static_cast<Iterator &>(aIterator);

    ClearAllBytes(aEntry);

    SuccessOrExit(error = iterator.AdvanceToNextRouter(Iterator::kRouterIterator));
    iterator.GetRouter()->CopyInfoTo(aEntry, iterator.GetInitTime(), iterator.GetInitUptime());

exit:
    return error;
}

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker::Iterator

void RoutingManager::RxRaTracker::Iterator::Init(const Entry<Router> *aRoutersHead, uint32_t aUptime)
{
    SetInitUptime(aUptime);
    SetInitTime();
    SetType(kUnspecified);
    SetRouter(aRoutersHead);
    SetEntry(nullptr);
    SetEntryType(kRoutePrefix);
}

Error RoutingManager::RxRaTracker::Iterator::AdvanceToNextRouter(Type aType)
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

Error RoutingManager::RxRaTracker::Iterator::AdvanceToNextEntry(void)
{
    Error error = kErrorNone;

    VerifyOrExit(GetRouter() != nullptr, error = kErrorNotFound);

    if (HasEntry())
    {
        switch (GetEntryType())
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
        switch (GetEntryType())
        {
        case kOnLinkPrefix:

            // Transition from on-link prefixes to route prefixes of
            // the current router.

            SetEntry(GetRouter()->mRoutePrefixes.GetHead());
            SetEntryType(kRoutePrefix);
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
            SetEntryType(kOnLinkPrefix);
            break;
        }
    }

exit:
    return error;
}

#if OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

Error RoutingManager::RxRaTracker::Iterator::AdvanceToNextPeerBr(const PeerBr *aPeerBrsHead)
{
    Error error = kErrorNone;

    if (GetType() == kUnspecified)
    {
        SetType(kPeerBrIterator);
        SetEntry(aPeerBrsHead);
    }
    else
    {
        VerifyOrExit(GetType() == kPeerBrIterator, error = kErrorInvalidArgs);
        VerifyOrExit(GetPeerBrEntry() != nullptr, error = kErrorNotFound);
        SetEntry(GetPeerBrEntry()->GetNext());
    }

    VerifyOrExit(GetPeerBrEntry() != nullptr, error = kErrorNotFound);

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_TRACK_PEER_BR_INFO_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// RxRaTracker::Router

bool RoutingManager::RxRaTracker::Router::ShouldCheckReachability(void) const
{
    // Perform reachability check (send NS probes) only if the router:
    // - Is not already marked as unreachable (due to failed NS probes)
    // - Is not the local device itself (to avoid potential issues with
    //   the platform receiving/processing NAs from itself).

    return IsReachable() && !mIsLocalDevice;
}

void RoutingManager::RxRaTracker::Router::ResetReachabilityState(void)
{
    // Called when an RA or NA is received and processed.

    mNsProbeCount   = 0;
    mLastUpdateTime = TimerMilli::GetNow();
    mTimeoutTime    = mLastUpdateTime + Random::NonCrypto::AddJitter(kReachableInterval, kJitter);
}

void RoutingManager::RxRaTracker::Router::DetermineReachabilityTimeout(void)
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

bool RoutingManager::RxRaTracker::Router::Matches(const EmptyChecker &aChecker)
{
    // First removes all expired on-link or router prefixes. Then
    // checks whether or not the router has any useful info.

    bool hasFlags = false;

    mOnLinkPrefixes.RemoveAndFreeAllMatching(aChecker);
    mRoutePrefixes.RemoveAndFreeAllMatching(aChecker);

    // Router can be removed if it does not advertise M or O flags and
    // also does not have any advertised prefix entries (RIO/PIO). If
    // the router already failed to respond to max NS probe attempts,
    // we consider it as offline and therefore do not consider its
    // flags anymore.

    if (IsReachable())
    {
        hasFlags = (mManagedAddressConfigFlag || mOtherConfigFlag);
    }

    return !hasFlags && mOnLinkPrefixes.IsEmpty() && mRoutePrefixes.IsEmpty();
}

bool RoutingManager::RxRaTracker::Router::IsPeerBr(void) const
{
    // Determines whether the router is a peer BR (connected to the
    // same Thread mesh network). It must have at least one entry
    // (on-link or route) and all entries should be marked to be
    // disregarded. While this model is generally effective to detect
    // peer BRs, it may not be 100% accurate in all scenarios.

    return mAllEntriesDisregarded && !(mOnLinkPrefixes.IsEmpty() && mRoutePrefixes.IsEmpty());
}

void RoutingManager::RxRaTracker::Router::CopyInfoTo(RouterEntry &aEntry, TimeMilli aNow, uint32_t aUptime) const
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
// RxRaTracker::DecisionFactors

void RoutingManager::RxRaTracker::DecisionFactors::UpdateFlagsFrom(const Router &aRouter)
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

void RoutingManager::RxRaTracker::DecisionFactors::UpdateFrom(const OnLinkPrefix &aOnLinkPrefix)
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

void RoutingManager::RxRaTracker::DecisionFactors::UpdateFrom(const RoutePrefix &aRoutePrefix)
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
            mLocalPrefix.mPreference     = PdPrefixManager::kPdRoutePreference;
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

    return mIsLocalAddedInNetData && Get<NetworkData::Leader>().ContainsOmrPrefix(mLocalPrefix.GetPrefix());
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

bool RoutingManager::OnLinkPrefixManager::AddressMatchesLocalPrefix(const Ip6::Address &aAddress) const
{
    bool matches = false;

    VerifyOrExit(GetState() != kIdle);
    matches = aAddress.MatchesPrefix(mLocalPrefix);

exit:
    return matches;
}

void RoutingManager::OnLinkPrefixManager::Evaluate(void)
{
    VerifyOrExit(!Get<RoutingManager>().mRsSender.IsInProgress());

    mFavoredDiscoveredPrefix = Get<RoutingManager>().mRxRaTracker.GetFavoredOnLinkPrefix();

    if ((mFavoredDiscoveredPrefix.GetLength() == 0) || (mFavoredDiscoveredPrefix == mLocalPrefix))
    {
        // We advertise the local on-link prefix if no other prefix is
        // discovered, or if the favored discovered prefix is the
        // same as the local prefix (for redundancy). Note that the
        // local on-link prefix, derived from the extended PAN ID, is
        // identical for all BRs on the same Thread mesh.

        PublishAndAdvertise();

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

void RoutingManager::OnLinkPrefixManager::HandleRaPrefixTableChanged(void)
{
    // This is a callback from `mRxRaTracker` indicating that
    // there has been a change in the table. If the favored on-link
    // prefix has changed, we trigger a re-evaluation of the routing
    // policy.

    if (Get<RoutingManager>().mRxRaTracker.GetFavoredOnLinkPrefix() != mFavoredDiscoveredPrefix)
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

Error RoutingManager::OnLinkPrefixManager::AppendAsPiosTo(RouterAdvert::TxMessage &aRaMessage)
{
    Error error;

    SuccessOrExit(error = AppendCurPrefix(aRaMessage));
    error = AppendOldPrefixes(aRaMessage);

exit:
    return error;
}

Error RoutingManager::OnLinkPrefixManager::AppendCurPrefix(RouterAdvert::TxMessage &aRaMessage)
{
    // Append the local on-link prefix to the `aRaMessage` as a PIO
    // only if it is being advertised or deprecated.
    //
    // If in `kAdvertising` state, we reset the expire time.
    // If in `kDeprecating` state, we include it as PIO with zero
    // preferred lifetime and the remaining valid lifetime.

    Error     error             = kErrorNone;
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

    SuccessOrExit(error = aRaMessage.AppendPrefixInfoOption(mLocalPrefix, validLifetime, preferredLifetime));

    LogPrefixInfoOption(mLocalPrefix, validLifetime, preferredLifetime);

exit:
    return error;
}

Error RoutingManager::OnLinkPrefixManager::AppendOldPrefixes(RouterAdvert::TxMessage &aRaMessage)
{
    Error     error = kErrorNone;
    TimeMilli now   = TimerMilli::GetNow();
    uint32_t  validLifetime;

    for (const OldPrefix &oldPrefix : mOldLocalPrefixes)
    {
        if (oldPrefix.mExpireTime < now)
        {
            continue;
        }

        validLifetime = TimeMilli::MsecToSec(oldPrefix.mExpireTime - now);
        SuccessOrExit(error = aRaMessage.AppendPrefixInfoOption(oldPrefix.mPrefix, validLifetime, 0));

        LogPrefixInfoOption(oldPrefix.mPrefix, validLifetime, 0);
    }

exit:
    return error;
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

    Get<RoutingManager>().HandleLocalOnLinkPrefixChanged();

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
    NextFireTime nextExpireTime;

    Array<Ip6::Prefix, kMaxOldPrefixes> expiredPrefixes;

    switch (GetState())
    {
    case kIdle:
        break;
    case kPublishing:
    case kAdvertising:
    case kDeprecating:
        if (nextExpireTime.GetNow() >= mExpireTime)
        {
            IgnoreError(Get<Settings>().RemoveBrOnLinkPrefix(mLocalPrefix));
            SetState(kIdle);
        }
        else
        {
            nextExpireTime.UpdateIfEarlier(mExpireTime);
        }
        break;
    }

    for (OldPrefix &entry : mOldLocalPrefixes)
    {
        if (nextExpireTime.GetNow() >= entry.mExpireTime)
        {
            SuccessOrAssert(expiredPrefixes.PushBack(entry.mPrefix));
        }
        else
        {
            nextExpireTime.UpdateIfEarlier(entry.mExpireTime);
        }
    }

    for (const Ip6::Prefix &prefix : expiredPrefixes)
    {
        LogInfo("Old local on-link prefix %s expired", prefix.ToString().AsCString());
        IgnoreError(Get<Settings>().RemoveBrOnLinkPrefix(prefix));
        mOldLocalPrefixes.RemoveMatching(prefix);
    }

    mTimer.FireAtIfEarlier(nextExpireTime);

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

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kIdle);
        ValidateNextEnum(kPublishing);
        ValidateNextEnum(kAdvertising);
        ValidateNextEnum(kDeprecating);
    };

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

Error RoutingManager::RioAdvertiser::InvalidatPrevRios(RouterAdvert::TxMessage &aRaMessage)
{
    Error error = kErrorNone;

    for (const RioPrefix &prefix : mPrefixes)
    {
        RoutePreference preference = prefix.mIsDeprecating ? NetworkData::kRoutePreferenceLow : mPreference;

        SuccessOrExit(error = AppendRio(prefix.mPrefix, /* aRouteLifetime */ 0, preference, aRaMessage));
    }

#if OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
    mPrefixes.Free();
#endif

    mPrefixes.Clear();
    mTimer.Stop();

exit:
    return error;
}

Error RoutingManager::RioAdvertiser::AppendRios(RouterAdvert::TxMessage &aRaMessage)
{
    Error                           error = kErrorNone;
    NextFireTime                    nextTime;
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
        // The decision to include the local OMR prefix as a RIO is
        // delegated to `OmrPrefixManager.ShouldAdvertiseLocalAsRio()`
        // at step (1). Here, as we iterate over Network Data prefixes,
        // we exclude entries matching the local OMR prefix. This is
        // because `OmrPrefixManager` may have decided to stop advertising
        // it, while it might still be present in the Network Data due to
        // delays in registering changes with the leader.

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
            if (nextTime.GetNow() >= prefix.mExpirationTime)
            {
                SuccessOrExit(error = AppendRio(prefix.mPrefix, /* aRouteLifetime */ 0,
                                                NetworkData::kRoutePreferenceLow, aRaMessage));
                continue;
            }
        }
        else
        {
            prefix.mIsDeprecating  = true;
            prefix.mExpirationTime = nextTime.GetNow() + kDeprecationTime;
        }

        if (mPrefixes.PushBack(prefix) != kErrorNone)
        {
            LogWarn("Too many deprecating on-mesh prefixes, removing %s", prefix.mPrefix.ToString().AsCString());
            SuccessOrExit(error = AppendRio(prefix.mPrefix, /* aRouteLifetime */ 0, NetworkData::kRoutePreferenceLow,
                                            aRaMessage));
        }

        nextTime.UpdateIfEarlier(prefix.mExpirationTime);
    }

    // Advertise all prefixes in `mPrefixes`

    for (const RioPrefix &prefix : mPrefixes)
    {
        uint32_t        lifetime   = kDefaultOmrPrefixLifetime;
        RoutePreference preference = mPreference;

        if (prefix.mIsDeprecating)
        {
            lifetime   = TimeMilli::MsecToSec(prefix.mExpirationTime - nextTime.GetNow());
            preference = NetworkData::kRoutePreferenceLow;
        }

        SuccessOrExit(error = AppendRio(prefix.mPrefix, lifetime, preference, aRaMessage));
    }

    mTimer.FireAtIfEarlier(nextTime);

exit:
    return error;
}

Error RoutingManager::RioAdvertiser::AppendRio(const Ip6::Prefix       &aPrefix,
                                               uint32_t                 aRouteLifetime,
                                               RoutePreference          aPreference,
                                               RouterAdvert::TxMessage &aRaMessage)
{
    Error error;

    SuccessOrExit(error = aRaMessage.AppendRouteInfoOption(aPrefix, aRouteLifetime, aPreference));
    LogRouteInfoOption(aPrefix, aRouteLifetime, aPreference);

exit:
    return error;
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
        Get<RoutingManager>().mRxRaTracker.ContainsDefaultOrNonUlaRoutePrefix())
    {
        newState = kPublishDefault;
    }
    else if (Get<RoutingManager>().mRxRaTracker.ContainsNonUlaOnLinkPrefix())
    {
        newState = kPublishDefault;
    }
    else if (Get<RoutingManager>().mRxRaTracker.ContainsUlaOnLinkPrefix() ||
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
        // `Clear()` will set the prefix to `::/0`.
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

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kDoNotPublish);
        ValidateNextEnum(kPublishDefault);
        ValidateNextEnum(kPublishUla);
    };

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

    // If a prefix other than `mLocalPrefix` is present, an external
    // translator is available. To bypass the NAT64 translator, we
    // clear its NAT64 prefix.

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
// RaInfo

void RoutingManager::TxRaInfo::IncrementTxCountAndSaveHash(const InfraIf::Icmp6Packet &aRaMessage)
{
    mTxCount++;
    mLastHashIndex++;

    if (mLastHashIndex == kNumHashEntries)
    {
        mLastHashIndex = 0;
    }

    CalculateHash(RouterAdvert::RxMessage(aRaMessage), mHashes[mLastHashIndex]);
}

bool RoutingManager::TxRaInfo::IsRaFromManager(const RouterAdvert::RxMessage &aRaMessage) const
{
    // Determines whether or not a received RA message was prepared by
    // by `RoutingManager` itself (is present in the saved `mHashes`).

    bool     isFromManager = false;
    uint16_t hashIndex     = mLastHashIndex;
    uint32_t count         = Min<uint32_t>(mTxCount, kNumHashEntries);
    Hash     hash;

    CalculateHash(aRaMessage, hash);

    for (; count > 0; count--)
    {
        if (mHashes[hashIndex] == hash)
        {
            isFromManager = true;
            break;
        }

        // Go to the previous index (ring buffer)

        if (hashIndex == 0)
        {
            hashIndex = kNumHashEntries - 1;
        }
        else
        {
            hashIndex--;
        }
    }

    return isFromManager;
}

void RoutingManager::TxRaInfo::CalculateHash(const RouterAdvert::RxMessage &aRaMessage, Hash &aHash)
{
    RouterAdvert::Header header;
    Crypto::Sha256       sha256;

    header = aRaMessage.GetHeader();
    header.SetChecksum(0);

    sha256.Start();
    sha256.Update(header);
    sha256.Update(aRaMessage.GetOptionStart(), aRaMessage.GetOptionLength());
    sha256.Finish(aHash);
}

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
    Ip6::Address         destAddress;
    RouterSolicitHeader  rsHdr;
    TxMessage            rsMsg;
    LinkLayerAddress     linkAddr;
    InfraIf::Icmp6Packet packet;
    Error                error;

    SuccessOrExit(error = rsMsg.Append(rsHdr));

    if (Get<InfraIf>().GetLinkLayerAddress(linkAddr) == kErrorNone)
    {
        SuccessOrExit(error = rsMsg.AppendLinkLayerOption(linkAddr, Option::kSourceLinkLayerAddr));
    }

    rsMsg.GetAsPacket(packet);
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
exit:
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

//---------------------------------------------------------------------------------------------------------------------
// PdPrefixManager

#if OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

RoutingManager::PdPrefixManager::PdPrefixManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mEnabled(false)
    , mIsStarted(false)
    , mIsPaused(false)
    , mNumPlatformPioProcessed(0)
    , mNumPlatformRaReceived(0)
    , mLastPlatformRaTime(0)
    , mTimer(aInstance)
{
}

void RoutingManager::PdPrefixManager::SetEnabled(bool aEnabled)
{
    State oldState = GetState();

    VerifyOrExit(mEnabled != aEnabled);
    mEnabled = aEnabled;
    EvaluateStateChange(oldState);

exit:
    return;
}

void RoutingManager::PdPrefixManager::StartStop(bool aStart)
{
    State oldState = GetState();

    VerifyOrExit(aStart != mIsStarted);
    mIsStarted = aStart;
    EvaluateStateChange(oldState);

exit:
    return;
}

void RoutingManager::PdPrefixManager::PauseResume(bool aPause)
{
    State oldState = GetState();

    VerifyOrExit(aPause != mIsPaused);
    mIsPaused = aPause;
    EvaluateStateChange(oldState);

exit:
    return;
}

RoutingManager::PdPrefixManager::State RoutingManager::PdPrefixManager::GetState(void) const
{
    State state = kDhcp6PdStateDisabled;

    if (mEnabled)
    {
        state = mIsStarted ? (mIsPaused ? kDhcp6PdStateIdle : kDhcp6PdStateRunning) : kDhcp6PdStateStopped;
    }

    return state;
}

void RoutingManager::PdPrefixManager::Evaluate(void)
{
    const FavoredOmrPrefix &favoredPrefix = Get<RoutingManager>().mOmrPrefixManager.GetFavoredPrefix();

    bool shouldPause = !favoredPrefix.IsEmpty() && favoredPrefix.GetPrefix() != mPrefix.GetPrefix() &&
                       favoredPrefix.GetPreference() >= kPdRoutePreference;

    PauseResume(/* aPause */ shouldPause);
}

void RoutingManager::PdPrefixManager::EvaluateStateChange(Dhcp6PdState aOldState)
{
    State newState = GetState();

    VerifyOrExit(aOldState != newState);
    LogInfo("PdPrefixManager: %s -> %s", StateToString(aOldState), StateToString(newState));

    switch (newState)
    {
    case kDhcp6PdStateDisabled:
    case kDhcp6PdStateStopped:
    case kDhcp6PdStateIdle:
        WithdrawPrefix();
        break;
    case kDhcp6PdStateRunning:
        break;
    }

    // When the prefix is replaced, there will be a short period when the old prefix is still in the netdata, and PD
    // manager will refuse to request the prefix.
    // TODO: Either update the comment for the state callback or add a random delay when notifing the upper layer for
    // state change.
    mStateCallback.InvokeIfSet(static_cast<otBorderRoutingDhcp6PdState>(newState));

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

    LogInfo("Withdrew DHCPv6 PD prefix %s", mPrefix.GetPrefix().ToString().AsCString());

    mPrefix.Clear();
    mTimer.Stop();

    Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kImmediately);

exit:
    return;
}

void RoutingManager::PdPrefixManager::ProcessRa(const uint8_t *aRouterAdvert, const uint16_t aLength)
{
    // Processes a Router Advertisement (RA) message received on the
    // platform's Thread interface. This RA message, generated by
    // software entities like dnsmasq, radvd, or systemd-networkd, is
    // part of the DHCPv6 prefix delegation process for distributing
    // prefixes to interfaces.

    InfraIf::Icmp6Packet packet;

    packet.Init(aRouterAdvert, aLength);
    Process(&packet, nullptr);
}

void RoutingManager::PdPrefixManager::ProcessPrefix(const PrefixTableEntry &aPrefixTableEntry)
{
    // Processes a prefix delegated by a DHCPv6 Prefix Delegation
    // (PD) server.  Similar to `ProcessRa()`, but sets the prefix
    // directly instead of parsing an RA message. Calling this method
    // again with new values can update the prefix's lifetime.

    Process(nullptr, &aPrefixTableEntry);
}

void RoutingManager::PdPrefixManager::Process(const InfraIf::Icmp6Packet *aRaPacket,
                                              const PrefixTableEntry     *aPrefixTableEntry)
{
    // Processes DHCPv6 Prefix Delegation (PD) prefixes, either from
    // an RA message or directly set. Requires either `aRaPacket` or
    // `aPrefixTableEntry` to be non-null.

    bool        currentPrefixUpdated = false;
    Error       error                = kErrorNone;
    PrefixEntry favoredEntry;
    PrefixEntry entry;

    VerifyOrExit(mEnabled, error = kErrorInvalidState);

    if (aRaPacket != nullptr)
    {
        RouterAdvert::RxMessage raMsg = RouterAdvert::RxMessage(*aRaPacket);

        VerifyOrExit(raMsg.IsValid(), error = kErrorParse);

        for (const Option &option : raMsg)
        {
            if (option.GetType() != Option::kTypePrefixInfo || !static_cast<const PrefixInfoOption &>(option).IsValid())
            {
                continue;
            }

            mNumPlatformPioProcessed++;
            entry.SetFrom(static_cast<const PrefixInfoOption &>(option));
            currentPrefixUpdated |= ProcessPrefixEntry(entry, favoredEntry);
        }

        mNumPlatformRaReceived++;
        mLastPlatformRaTime = TimerMilli::GetNow();
    }
    else // aPrefixTableEntry != nullptr
    {
        entry.SetFrom(*aPrefixTableEntry);
        currentPrefixUpdated = ProcessPrefixEntry(entry, favoredEntry);
    }

    if (currentPrefixUpdated && mPrefix.IsDeprecated())
    {
        LogInfo("DHCPv6 PD prefix %s is deprecated", mPrefix.GetPrefix().ToString().AsCString());
        mPrefix.Clear();
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kImmediately);
    }

    if (favoredEntry.IsFavoredOver(mPrefix))
    {
        mPrefix              = favoredEntry;
        currentPrefixUpdated = true;
        LogInfo("DHCPv6 PD prefix set to %s", mPrefix.GetPrefix().ToString().AsCString());
        Get<RoutingManager>().ScheduleRoutingPolicyEvaluation(kImmediately);
    }

    if (HasPrefix())
    {
        mTimer.FireAt(mPrefix.GetDeprecationTime());
    }
    else
    {
        mTimer.Stop();
    }

exit:
    LogWarnOnError(error, "process DHCPv6 delegated prefix");
    OT_UNUSED_VARIABLE(error);
}

bool RoutingManager::PdPrefixManager::ProcessPrefixEntry(PrefixEntry &aEntry, PrefixEntry &aFavoredEntry)
{
    bool currentPrefixUpdated = false;

    if (!aEntry.IsValidPdPrefix())
    {
        LogWarn("Ignore invalid DHCPv6 PD prefix %s", aEntry.GetPrefix().ToString().AsCString());
        ExitNow();
    }

    aEntry.GetPrefix().Tidy();
    aEntry.GetPrefix().SetLength(kOmrPrefixLength);

    // Check if there is an update to the current prefix. The valid or
    // preferred lifetime may have changed.

    if (HasPrefix() && (mPrefix.GetPrefix() == aEntry.GetPrefix()))
    {
        currentPrefixUpdated = true;
        mPrefix              = aEntry;
    }

    VerifyOrExit(!aEntry.IsDeprecated());

    // Some platforms may delegate multiple prefixes. We'll select the
    // smallest one, as GUA prefixes (`2000::/3`) are inherently
    // smaller than ULA prefixes (`fc00::/7`). This rule prefers GUA
    // prefixes over ULA.

    if (aEntry.IsFavoredOver(aFavoredEntry))
    {
        aFavoredEntry = aEntry;
    }

exit:
    return currentPrefixUpdated;
}

bool RoutingManager::PdPrefixManager::PrefixEntry::IsValidPdPrefix(void) const
{
    // We should accept ULA prefix since it could be used by the internet infrastructure like NAT64.

    return !IsEmpty() && (GetPrefix().GetLength() <= kOmrPrefixLength) && !GetPrefix().IsLinkLocal() &&
           !GetPrefix().IsMulticast();
}

bool RoutingManager::PdPrefixManager::PrefixEntry::IsFavoredOver(const PrefixEntry &aOther) const
{
    bool isFavored;

    if (IsEmpty())
    {
        // Empty prefix is not favored over any (including another
        // empty prefix).
        isFavored = false;
        ExitNow();
    }

    if (aOther.IsEmpty())
    {
        // A non-empty prefix is favored over an empty one.
        isFavored = true;
        ExitNow();
    }

    // Numerically smaller prefix is favored.

    isFavored = GetPrefix() < aOther.GetPrefix();

exit:
    return isFavored;
}

const char *RoutingManager::PdPrefixManager::StateToString(State aState)
{
    static const char *const kStateStrings[] = {
        "Disabled", // (0) kDisabled
        "Stopped",  // (1) kStopped
        "Running",  // (2) kRunning
        "Idle",     // (3) kIdle
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kDhcp6PdStateDisabled);
        ValidateNextEnum(kDhcp6PdStateStopped);
        ValidateNextEnum(kDhcp6PdStateRunning);
        ValidateNextEnum(kDhcp6PdStateIdle);
    };

    return kStateStrings[aState];
}

extern "C" void otPlatBorderRoutingProcessIcmp6Ra(otInstance *aInstance, const uint8_t *aMessage, uint16_t aLength)
{
    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().mPdPrefixManager.ProcessRa(aMessage, aLength);
}

extern "C" void otPlatBorderRoutingProcessDhcp6PdPrefix(otInstance                            *aInstance,
                                                        const otBorderRoutingPrefixTableEntry *aPrefixInfo)
{
    AssertPointerIsNotNull(aPrefixInfo);

    AsCoreType(aInstance).Get<BorderRouter::RoutingManager>().mPdPrefixManager.ProcessPrefix(*aPrefixInfo);
}
#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
