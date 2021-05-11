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
#include "common/logging.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "net/ip6.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"

namespace ot {

namespace BorderRouter {

RoutingManager::RoutingManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsRunning(false)
    , mIsEnabled(false)
    , mInfraIfIsRunning(false)
    , mInfraIfIndex(0)
    , mAdvertisedOmrPrefixNum(0)
    , mAdvertisedOnLinkPrefix(nullptr)
    , mDiscoveredPrefixNum(0)
    , mDiscoveredPrefixInvalidTimer(aInstance, HandleDiscoveredPrefixInvalidTimer)
    , mRouterAdvertisementTimer(aInstance, HandleRouterAdvertisementTimer)
    , mRouterAdvertisementCount(0)
    , mRouterSolicitTimer(aInstance, HandleRouterSolicitTimer)
    , mRouterSolicitCount(0)
    , mRoutingPolicyTimer(aInstance, HandleRoutingPolicyTimer)
{
    mLocalOmrPrefix.Clear();
    memset(mAdvertisedOmrPrefixes, 0, sizeof(mAdvertisedOmrPrefixes));

    mLocalOnLinkPrefix.Clear();

    memset(mDiscoveredPrefixes, 0, sizeof(mDiscoveredPrefixes));
}

Error RoutingManager::Init(uint32_t aInfraIfIndex, bool aInfraIfIsRunning)
{
    Error error;

    VerifyOrExit(!IsInitialized(), error = kErrorInvalidState);
    VerifyOrExit(aInfraIfIndex > 0, error = kErrorInvalidArgs);

    SuccessOrExit(error = LoadOrGenerateRandomOmrPrefix());
    SuccessOrExit(error = LoadOrGenerateRandomOnLinkPrefix());

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

Error RoutingManager::LoadOrGenerateRandomOmrPrefix(void)
{
    Error error = kErrorNone;

    if (Get<Settings>().ReadOmrPrefix(mLocalOmrPrefix) != kErrorNone || !IsValidOmrPrefix(mLocalOmrPrefix))
    {
        Ip6::NetworkPrefix randomOmrPrefix;

        otLogNoteBr("no valid OMR prefix found in settings, generating new one");

        error = randomOmrPrefix.GenerateRandomUla();
        if (error != kErrorNone)
        {
            otLogCritBr("failed to generate random OMR prefix");
            ExitNow();
        }

        mLocalOmrPrefix.Set(randomOmrPrefix);
        IgnoreError(Get<Settings>().SaveOmrPrefix(mLocalOmrPrefix));
    }

exit:
    return error;
}

Error RoutingManager::LoadOrGenerateRandomOnLinkPrefix(void)
{
    Error error = kErrorNone;

    if (Get<Settings>().ReadOnLinkPrefix(mLocalOnLinkPrefix) != kErrorNone || !IsValidOnLinkPrefix(mLocalOnLinkPrefix))
    {
        Ip6::NetworkPrefix randomOnLinkPrefix;

        otLogNoteBr("no valid on-link prefix found in settings, generating new one");

        error = randomOnLinkPrefix.GenerateRandomUla();
        if (error != kErrorNone)
        {
            otLogCritBr("failed to generate random on-link prefix");
            ExitNow();
        }

        randomOnLinkPrefix.m8[6] = 0;
        randomOnLinkPrefix.m8[7] = 0;
        mLocalOnLinkPrefix.Set(randomOnLinkPrefix);

        IgnoreError(Get<Settings>().SaveOnLinkPrefix(mLocalOnLinkPrefix));
    }

exit:
    return error;
}

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
        otLogInfoBr("Border Routing manager started");

        mIsRunning = true;
        StartRouterSolicitationDelay();
    }
}

void RoutingManager::Stop(void)
{
    VerifyOrExit(mIsRunning);

    UnpublishLocalOmrPrefix();
    if (mAdvertisedOnLinkPrefix != nullptr)
    {
        RemoveExternalRoute(*mAdvertisedOnLinkPrefix);
    }
    InvalidateAllDiscoveredPrefixes();

    // Use empty OMR & on-link prefixes to invalidate possible advertised prefixes.
    SendRouterAdvertisement(nullptr, 0, nullptr);

    memset(mAdvertisedOmrPrefixes, 0, sizeof(mAdvertisedOmrPrefixes));
    mAdvertisedOmrPrefixNum = 0;

    mAdvertisedOnLinkPrefix = nullptr;

    memset(mDiscoveredPrefixes, 0, sizeof(mDiscoveredPrefixes));
    mDiscoveredPrefixNum = 0;
    mDiscoveredPrefixInvalidTimer.Stop();

    mRouterAdvertisementTimer.Stop();
    mRouterAdvertisementCount = 0;

    mRouterSolicitTimer.Stop();
    mRouterSolicitCount = 0;

    mRoutingPolicyTimer.Stop();

    otLogInfoBr("Border Routing manager stopped");

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
        otLogDebgBr("drop ICMPv6 message: %s", ErrorToString(error));
    }
}

Error RoutingManager::HandleInfraIfStateChanged(uint32_t aInfraIfIndex, bool aIsRunning)
{
    Error error = kErrorNone;

    VerifyOrExit(IsInitialized(), error = kErrorInvalidState);
    VerifyOrExit(aInfraIfIndex == mInfraIfIndex, error = kErrorInvalidArgs);
    VerifyOrExit(aIsRunning != mInfraIfIsRunning);

    otLogInfoBr("infra interface (%u) state changed: %sRUNNING -> %sRUNNING", aInfraIfIndex,
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
        StartRoutingPolicyEvaluationDelay();
    }

exit:
    return;
}

uint8_t RoutingManager::EvaluateOmrPrefix(Ip6::Prefix *aNewOmrPrefixes, uint8_t aMaxOmrPrefixNum)
{
    uint8_t                         newOmrPrefixNum = 0;
    NetworkData::Iterator           iterator        = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig onMeshPrefixConfig;
    Ip6::Prefix *                   smallestOmrPrefix       = nullptr;
    Ip6::Prefix *                   publishedLocalOmrPrefix = nullptr;

    OT_ASSERT(mIsRunning);

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, onMeshPrefixConfig) == kErrorNone)
    {
        uint8_t newPrefixIndex;

        if (!IsValidOmrPrefix(onMeshPrefixConfig))
        {
            continue;
        }

        newPrefixIndex = 0;
        while (newPrefixIndex < newOmrPrefixNum && onMeshPrefixConfig.GetPrefix() != aNewOmrPrefixes[newPrefixIndex])
        {
            ++newPrefixIndex;
        }

        if (newPrefixIndex != newOmrPrefixNum)
        {
            // Ignore duplicate prefixes.
            continue;
        }

        if (newOmrPrefixNum >= aMaxOmrPrefixNum)
        {
            otLogWarnBr("EvaluateOmrPrefix: too many OMR prefixes, ignoring prefix %s",
                        onMeshPrefixConfig.GetPrefix().ToString().AsCString());
            continue;
        }

        aNewOmrPrefixes[newOmrPrefixNum] = onMeshPrefixConfig.GetPrefix();
        if (smallestOmrPrefix == nullptr || (onMeshPrefixConfig.GetPrefix() < *smallestOmrPrefix))
        {
            smallestOmrPrefix = &aNewOmrPrefixes[newOmrPrefixNum];
        }
        if (aNewOmrPrefixes[newOmrPrefixNum] == mLocalOmrPrefix)
        {
            publishedLocalOmrPrefix = &aNewOmrPrefixes[newOmrPrefixNum];
        }
        ++newOmrPrefixNum;
    }

    // Decide if we need to add or remove my local OMR prefix.
    if (newOmrPrefixNum == 0)
    {
        otLogInfoBr("EvaluateOmrPrefix: no valid OMR prefixes found in Thread network");
        if (PublishLocalOmrPrefix() == kErrorNone)
        {
            aNewOmrPrefixes[newOmrPrefixNum++] = mLocalOmrPrefix;
        }

        // The `newOmrPrefixNum` is zero when we failed to publish the local OMR prefix.
    }
    else if (publishedLocalOmrPrefix != nullptr && smallestOmrPrefix != publishedLocalOmrPrefix)
    {
        otLogInfoBr("EvaluateOmrPrefix: there is already a smaller OMR prefix %s in the Thread network",
                    smallestOmrPrefix->ToString().AsCString());
        UnpublishLocalOmrPrefix();

        // Remove the local OMR prefix from the list by overwriting it with the last one.
        *publishedLocalOmrPrefix = aNewOmrPrefixes[--newOmrPrefixNum];
    }

    return newOmrPrefixNum;
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
    omrPrefixConfig.mPreference   = OT_ROUTE_PREFERENCE_MED;

    error = Get<NetworkData::Local>().AddOnMeshPrefix(omrPrefixConfig);
    if (error != kErrorNone)
    {
        otLogWarnBr("failed to publish local OMR prefix %s in Thread network: %s",
                    mLocalOmrPrefix.ToString().AsCString(), ErrorToString(error));
    }
    else
    {
        Get<NetworkData::Notifier>().HandleServerDataUpdated();
        otLogInfoBr("published local OMR prefix %s in Thread network", mLocalOmrPrefix.ToString().AsCString());
    }

    return error;
}

void RoutingManager::UnpublishLocalOmrPrefix(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsRunning);

    SuccessOrExit(error = Get<NetworkData::Local>().RemoveOnMeshPrefix(mLocalOmrPrefix));

    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    otLogInfoBr("unpublished local OMR prefix %s from Thread network", mLocalOmrPrefix.ToString().AsCString());

exit:
    if (error != kErrorNone)
    {
        otLogWarnBr("failed to unpublish local OMR prefix %s from Thread network: %s",
                    mLocalOmrPrefix.ToString().AsCString(), ErrorToString(error));
    }
}

Error RoutingManager::AddExternalRoute(const Ip6::Prefix &aPrefix, otRoutePreference aRoutePreference)
{
    Error                            error;
    NetworkData::ExternalRouteConfig routeConfig;

    OT_ASSERT(mIsRunning);

    routeConfig.Clear();
    routeConfig.SetPrefix(aPrefix);
    routeConfig.mStable     = true;
    routeConfig.mPreference = aRoutePreference;

    error = Get<NetworkData::Local>().AddHasRoutePrefix(routeConfig);
    if (error != kErrorNone)
    {
        otLogWarnBr("failed to add external route %s: %s", aPrefix.ToString().AsCString(), ErrorToString(error));
    }
    else
    {
        Get<NetworkData::Notifier>().HandleServerDataUpdated();
        otLogInfoBr("added external route %s", aPrefix.ToString().AsCString());
    }

    return error;
}

void RoutingManager::RemoveExternalRoute(const Ip6::Prefix &aPrefix)
{
    Error error = kErrorNone;

    VerifyOrExit(mIsRunning);

    SuccessOrExit(error = Get<NetworkData::Local>().RemoveHasRoutePrefix(aPrefix));

    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    otLogInfoBr("removed external route %s", aPrefix.ToString().AsCString());

exit:
    if (error != kErrorNone)
    {
        otLogWarnBr("failed to remove external route %s: %s", aPrefix.ToString().AsCString(), ErrorToString(error));
    }
}

bool RoutingManager::ContainsPrefix(const Ip6::Prefix &aPrefix, const Ip6::Prefix *aPrefixList, uint8_t aPrefixNum)
{
    bool ret = false;

    for (uint8_t i = 0; i < aPrefixNum; ++i)
    {
        if (aPrefixList[i] == aPrefix)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

const Ip6::Prefix *RoutingManager::EvaluateOnLinkPrefix(void)
{
    const Ip6::Prefix *newOnLinkPrefix      = nullptr;
    const Ip6::Prefix *smallestOnLinkPrefix = nullptr;

    // We don't evaluate on-link prefix if we are doing Router Solicitation.
    VerifyOrExit(!mRouterSolicitTimer.IsRunning());

    for (uint8_t i = 0; i < mDiscoveredPrefixNum; ++i)
    {
        ExternalPrefix &prefix = mDiscoveredPrefixes[i];

        if (!prefix.mIsOnLinkPrefix)
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
        if (mAdvertisedOnLinkPrefix != nullptr)
        {
            newOnLinkPrefix = mAdvertisedOnLinkPrefix;
        }
        else if (AddExternalRoute(mLocalOnLinkPrefix, OT_ROUTE_PREFERENCE_MED) == kErrorNone)
        {
            newOnLinkPrefix = &mLocalOnLinkPrefix;
        }
    }
    // When an application-specific on-link prefix is received and it is bigger than the
    // advertised prefix, we will not remove the advertised prefix. In this case, there
    // will be two on-link prefixes on the infra link. But all BRs will still converge to
    // the same smallest on-link prefix and the application-specific prefix is not used.
    else if (mAdvertisedOnLinkPrefix != nullptr)
    {
        if (*mAdvertisedOnLinkPrefix < *smallestOnLinkPrefix)
        {
            newOnLinkPrefix = mAdvertisedOnLinkPrefix;
        }
        else
        {
            otLogInfoBr("EvaluateOnLinkPrefix: there is already smaller on-link prefix %s on interface %u",
                        smallestOnLinkPrefix->ToString().AsCString(), mInfraIfIndex);

            // TODO: we removes the advertised on-link prefix by setting valid lifetime for PIO.
            // But SLAAC addresses configured by PIO prefix will not be removed immediately (
            // https://tools.ietf.org/html/rfc4862#section-5.5.3). This leads to a situation that
            // a WiFi device keeps using the old SLAAC address but a Thread device cannot reach to
            // it. One solution is to delay removing external route until the SLAAC addresses are
            // actually expired/deprecated.
            RemoveExternalRoute(*mAdvertisedOnLinkPrefix);
        }
    }

exit:
    return newOnLinkPrefix;
}

// This method evaluate the routing policy depends on prefix and route
// information on Thread Network and infra link. As a result, this
// method May send RA messages on infra link and publish/unpublish
// OMR prefix in the Thread network.
void RoutingManager::EvaluateRoutingPolicy(void)
{
    OT_ASSERT(mIsRunning);

    const Ip6::Prefix *newOnLinkPrefix = nullptr;
    Ip6::Prefix        newOmrPrefixes[kMaxOmrPrefixNum];
    uint8_t            newOmrPrefixNum = 0;

    otLogInfoBr("evaluating routing policy");

    // 0. Evaluate on-link & OMR prefixes.
    newOnLinkPrefix = EvaluateOnLinkPrefix();
    newOmrPrefixNum = EvaluateOmrPrefix(newOmrPrefixes, kMaxOmrPrefixNum);

    // 1. Send Router Advertisement message if necessary.
    SendRouterAdvertisement(newOmrPrefixes, newOmrPrefixNum, newOnLinkPrefix);
    if (newOmrPrefixNum == 0)
    {
        // This is the very exceptional case and happens only when we failed to publish
        // our local OMR prefix to the Thread network. We schedule the Router Advertisement
        // timer to re-evaluate our routing policy in the future.
        otLogWarnBr("no OMR prefix advertised! Start Router Advertisement timer for future evaluation");
    }

    // 2. Schedule Router Advertisement timer with random interval.
    {
        uint32_t nextSendTime;

        nextSendTime = Random::NonCrypto::GetUint32InRange(kMinRtrAdvInterval, kMaxRtrAdvInterval);

        if (mRouterAdvertisementCount <= kMaxInitRtrAdvertisements && nextSendTime > kMaxInitRtrAdvInterval)
        {
            nextSendTime = kMaxInitRtrAdvInterval;
        }

        otLogInfoBr("router advertisement scheduled in %u seconds", nextSendTime);
        mRouterAdvertisementTimer.Start(Time::SecToMsec(nextSendTime));
    }

    // 3. Update advertised on-link & OMR prefixes information.
    mAdvertisedOnLinkPrefix = newOnLinkPrefix;
    static_assert(sizeof(mAdvertisedOmrPrefixes) == sizeof(newOmrPrefixes), "invalid new OMR prefix array size");
    memcpy(mAdvertisedOmrPrefixes, newOmrPrefixes, sizeof(newOmrPrefixes[0]) * newOmrPrefixNum);
    mAdvertisedOmrPrefixNum = newOmrPrefixNum;
}

void RoutingManager::StartRoutingPolicyEvaluationDelay(void)
{
    OT_ASSERT(mIsRunning);

    uint32_t randomDelay;

    static_assert(kMaxRoutingPolicyDelay > 0, "invalid maximum routing policy evaluation delay");
    randomDelay = Random::NonCrypto::GetUint32InRange(0, Time::SecToMsec(kMaxRoutingPolicyDelay));

    otLogInfoBr("start evaluating routing policy, scheduled in %u milliseconds", randomDelay);
    mRoutingPolicyTimer.Start(randomDelay);
}

// starts sending Router Solicitations in random delay
// between 0 and kMaxRtrSolicitationDelay.
void RoutingManager::StartRouterSolicitationDelay(void)
{
    OT_ASSERT(mAdvertisedOnLinkPrefix == nullptr);

    uint32_t randomDelay;

    mRouterAdvMessage.SetToDefault();

    mRouterSolicitCount = 0;

    static_assert(kMaxRtrSolicitationDelay > 0, "invalid maximum Router Solicitation delay");
    randomDelay = Random::NonCrypto::GetUint32InRange(0, Time::SecToMsec(kMaxRtrSolicitationDelay));

    otLogInfoBr("start Router Solicitation, scheduled in %u milliseconds", randomDelay);
    mRouterSolicitTimer.Start(randomDelay);
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
// @param[in]  aNewOmrPrefixes   A pointer to an array of the new OMR prefixes to be advertised.
//                               @p aNewOmrPrefixNum must be zero if this argument is nullptr.
// @param[in]  aNewOmrPrefixNum  The number of the new OMR prefixes to be advertised.
//                               Zero means we should stop advertising OMR prefixes.
// @param[in]  aOnLinkPrefix     A pointer to the new on-link prefix to be advertised.
//                               nullptr means we should stop advertising on-link prefix.
void RoutingManager::SendRouterAdvertisement(const Ip6::Prefix *aNewOmrPrefixes,
                                             uint8_t            aNewOmrPrefixNum,
                                             const Ip6::Prefix *aNewOnLinkPrefix)
{
    uint8_t  buffer[kMaxRouterAdvMessageLength];
    uint16_t bufferLength = 0;

    static_assert(sizeof(mRouterAdvMessage) <= sizeof(buffer), "RA buffer too small");
    memcpy(buffer, &mRouterAdvMessage, sizeof(mRouterAdvMessage));
    bufferLength += sizeof(mRouterAdvMessage);

    if (aNewOnLinkPrefix != nullptr)
    {
        RouterAdv::PrefixInfoOption pio;

        pio.SetOnLink(true);
        pio.SetAutoAddrConfig(true);
        pio.SetValidLifetime(kDefaultOnLinkPrefixLifetime);
        pio.SetPreferredLifetime(kDefaultOnLinkPrefixLifetime);
        pio.SetPrefix(*aNewOnLinkPrefix);

        OT_ASSERT(bufferLength + pio.GetSize() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &pio, pio.GetSize());
        bufferLength += pio.GetSize();

        if (mAdvertisedOnLinkPrefix == nullptr)
        {
            otLogInfoBr("start advertising new on-link prefix %s on interface %u",
                        aNewOnLinkPrefix->ToString().AsCString(), mInfraIfIndex);
        }

        otLogInfoBr("send on-link prefix %s in PIO (valid lifetime = %u seconds)",
                    aNewOnLinkPrefix->ToString().AsCString(), kDefaultOnLinkPrefixLifetime);
    }
    else if (mAdvertisedOnLinkPrefix != nullptr)
    {
        RouterAdv::PrefixInfoOption pio;

        pio.SetOnLink(true);
        pio.SetAutoAddrConfig(true);

        // Set zero valid lifetime to immediately invalidate the advertised on-link prefix.
        pio.SetValidLifetime(0);
        pio.SetPreferredLifetime(0);
        pio.SetPrefix(*mAdvertisedOnLinkPrefix);

        OT_ASSERT(bufferLength + pio.GetSize() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &pio, pio.GetSize());
        bufferLength += pio.GetSize();

        otLogInfoBr("stop advertising on-link prefix %s on interface %u",
                    mAdvertisedOnLinkPrefix->ToString().AsCString(), mInfraIfIndex);
    }

    // Invalidate the advertised OMR prefixes if they are no longer in the new OMR prefix array.
    for (uint8_t i = 0; i < mAdvertisedOmrPrefixNum; ++i)
    {
        const Ip6::Prefix &advertisedOmrPrefix = mAdvertisedOmrPrefixes[i];

        if (!ContainsPrefix(advertisedOmrPrefix, aNewOmrPrefixes, aNewOmrPrefixNum))
        {
            RouterAdv::RouteInfoOption rio;

            // Set zero route lifetime to immediately invalidate the advertised OMR prefix.
            rio.SetRouteLifetime(0);
            rio.SetPrefix(advertisedOmrPrefix);

            OT_ASSERT(bufferLength + rio.GetSize() <= sizeof(buffer));
            memcpy(buffer + bufferLength, &rio, rio.GetSize());
            bufferLength += rio.GetSize();

            otLogInfoBr("stop advertising OMR prefix %s on interface %u", advertisedOmrPrefix.ToString().AsCString(),
                        mInfraIfIndex);
        }
    }

    for (uint8_t i = 0; i < aNewOmrPrefixNum; ++i)
    {
        const Ip6::Prefix &        newOmrPrefix = aNewOmrPrefixes[i];
        RouterAdv::RouteInfoOption rio;

        rio.SetRouteLifetime(kDefaultOmrPrefixLifetime);
        rio.SetPrefix(newOmrPrefix);

        OT_ASSERT(bufferLength + rio.GetSize() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &rio, rio.GetSize());
        bufferLength += rio.GetSize();

        otLogInfoBr("send OMR prefix %s in RIO (valid lifetime = %u seconds)", newOmrPrefix.ToString().AsCString(),
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
            otLogInfoBr("sent Router Advertisement on interface %u", mInfraIfIndex);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
            otDumpCertBr("[BR-CERT] direction=send | type=RA |", buffer, bufferLength);
#endif
        }
        else
        {
            otLogWarnBr("failed to send Router Advertisement on interface %u: %s", mInfraIfIndex, ErrorToString(error));
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

bool RoutingManager::IsValidOnLinkPrefix(const RouterAdv::PrefixInfoOption &aPio, bool aManagedAddrConfig)
{
    return IsValidOnLinkPrefix(aPio.GetPrefix()) && aPio.GetOnLink() &&
           (aPio.GetAutoAddrConfig() || aManagedAddrConfig);
}

bool RoutingManager::IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix)
{
    // Accept ULA prefix with length of 64 bits and GUA prefix.
    return (aOnLinkPrefix.mLength == kOnLinkPrefixLength && aOnLinkPrefix.mPrefix.mFields.m8[0] == 0xfd) ||
           (aOnLinkPrefix.mLength >= 3 && (aOnLinkPrefix.GetBytes()[0] & 0xE0) == 0x20);
}

void RoutingManager::HandleRouterAdvertisementTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().HandleRouterAdvertisementTimer();
}

void RoutingManager::HandleRouterAdvertisementTimer(void)
{
    otLogInfoBr("router advertisement timer triggered");

    EvaluateRoutingPolicy();
}

void RoutingManager::HandleRouterSolicitTimer(Timer &aTimer)
{
    aTimer.Get<RoutingManager>().HandleRouterSolicitTimer();
}

void RoutingManager::HandleRouterSolicitTimer(void)
{
    otLogInfoBr("router solicitation times out");

    if (mRouterSolicitCount < kMaxRtrSolicitations)
    {
        uint32_t nextSolicitationDelay;
        Error    error;

        error = SendRouterSolicitation();
        ++mRouterSolicitCount;

        if (error == kErrorNone)
        {
            otLogDebgBr("successfully sent %uth Router Solicitation", mRouterSolicitCount);
        }
        else
        {
            otLogCritBr("failed to send %uth Router Solicitation: %s", mRouterSolicitCount, ErrorToString(error));
        }

        nextSolicitationDelay =
            (mRouterSolicitCount == kMaxRtrSolicitations) ? kMaxRtrSolicitationDelay : kRtrSolicitationInterval;

        otLogDebgBr("router solicitation timer scheduled in %u seconds", nextSolicitationDelay);
        mRouterSolicitTimer.Start(Time::SecToMsec(nextSolicitationDelay));
    }
    else
    {
        // Re-evaluate our routing policy and send Router Advertisement if necessary.
        EvaluateRoutingPolicy();
    }
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
    uint32_t randomDelay;

    OT_UNUSED_VARIABLE(aSrcAddress);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aBufferLength);

    VerifyOrExit(!mRouterSolicitTimer.IsRunning());

    otLogInfoBr("received Router Solicitation from %s on interface %u", aSrcAddress.ToString().AsCString(),
                mInfraIfIndex);

    randomDelay = Random::NonCrypto::GetUint32InRange(0, kMaxRaDelayTime);

    otLogInfoBr("Router Advertisement scheduled in %u milliseconds", randomDelay);
    mRouterAdvertisementTimer.Start(randomDelay);

exit:
    return;
}

uint32_t RoutingManager::GetPrefixExpireDelay(uint32_t aValidLifetime)
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

    otLogInfoBr("received Router Advertisement from %s on interface %u", aSrcAddress.ToString().AsCString(),
                mInfraIfIndex);
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    otDumpCertBr("[BR-CERT] direction=recv | type=RA |", aBuffer, aBufferLength);
#endif

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
                needReevaluate |= UpdateDiscoveredPrefixes(*pio, routerAdvMessage->GetManagedAddrConfig());
            }
        }
        break;

        case Option::Type::kRouteInfo:
        {
            const RouteInfoOption *rio = static_cast<const RouteInfoOption *>(option);

            if (rio->IsValid())
            {
                needReevaluate |= UpdateDiscoveredPrefixes(*rio);
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
        needReevaluate |= UpdateRouterAdvMessage(*routerAdvMessage);
    }

    if (needReevaluate)
    {
        StartRoutingPolicyEvaluationDelay();
    }

exit:
    return;
}

bool RoutingManager::UpdateDiscoveredPrefixes(const RouterAdv::PrefixInfoOption &aPio, bool aManagedAddrConfig)
{
    Ip6::Prefix prefix         = aPio.GetPrefix();
    bool        needReevaluate = false;

    if (!IsValidOnLinkPrefix(aPio, aManagedAddrConfig))
    {
        otLogInfoBr("ignore invalid on-link prefix in PIO: %s", prefix.ToString().AsCString());
        ExitNow();
    }

    VerifyOrExit(mAdvertisedOnLinkPrefix == nullptr || prefix != *mAdvertisedOnLinkPrefix);

    otLogInfoBr("discovered on-link prefix (%s, %u seconds) from interface %u", prefix.ToString().AsCString(),
                aPio.GetValidLifetime(), mInfraIfIndex);

    if (aPio.GetValidLifetime() == 0)
    {
        needReevaluate = InvalidateDiscoveredPrefixes(&prefix, /* aIsOnLinkPrefix */ true) > 0;
    }
    else
    {
        needReevaluate = AddDiscoveredPrefix(prefix, /* aIsOnLinkPrefix */ true, aPio.GetValidLifetime());
    }

exit:
    return needReevaluate;
}

bool RoutingManager::UpdateDiscoveredPrefixes(const RouterAdv::RouteInfoOption &aRio)
{
    Ip6::Prefix prefix         = aRio.GetPrefix();
    bool        needReevaluate = false;

    if (!IsValidOmrPrefix(prefix))
    {
        otLogInfoBr("ignore invalid OMR prefix in RIO: %s", prefix.ToString().AsCString());
        ExitNow();
    }

    // Ignore OMR prefixes advertised by ourselves or in current Thread Network Data.
    // The `mAdvertisedOmrPrefixes` and the OMR prefix set in Network Data should eventually
    // be equal, but there is time that they are not synchronized immediately:
    // 1. Network Data could contain more OMR prefixes than `mAdvertisedOmrPrefixes` because
    //    we added random delay before Evaluating routing policy when Network Data is changed.
    // 2. `mAdvertisedOmrPrefixes` could contain more OMR prefixes than Network Data because
    //    it takes time to sync a new OMR prefix into Network Data (multicast loopback RA
    //    messages are usually faster than Thread Network Data propagation).
    // They are the reasons why we need both the checks.
    VerifyOrExit(!ContainsPrefix(prefix, mAdvertisedOmrPrefixes, mAdvertisedOmrPrefixNum));
    VerifyOrExit(!NetworkDataContainsOmrPrefix(prefix));

    otLogInfoBr("discovered OMR prefix (%s, %u seconds) from interface %u", prefix.ToString().AsCString(),
                aRio.GetRouteLifetime(), mInfraIfIndex);

    if (aRio.GetRouteLifetime() == 0)
    {
        needReevaluate = (InvalidateDiscoveredPrefixes(&prefix, /* aIsOnLinkPrefix */ false) > 0);
    }
    else
    {
        needReevaluate =
            AddDiscoveredPrefix(prefix, /* aIsOnLinkPrefix */ false, aRio.GetRouteLifetime(), aRio.GetPreference());
    }

exit:
    return needReevaluate;
}

bool RoutingManager::InvalidateDiscoveredPrefixes(const Ip6::Prefix *aPrefix, bool aIsOnLinkPrefix)
{
    uint8_t         removedNum          = 0;
    ExternalPrefix *keptPrefix          = mDiscoveredPrefixes;
    TimeMilli       now                 = TimerMilli::GetNow();
    TimeMilli       earliestExpireTime  = now.GetDistantFuture();
    uint8_t         keptOnLinkPrefixNum = 0;

    for (uint8_t i = 0; i < mDiscoveredPrefixNum; ++i)
    {
        ExternalPrefix &prefix = mDiscoveredPrefixes[i];

        if ((aPrefix != nullptr && prefix.mPrefix == *aPrefix && prefix.mIsOnLinkPrefix == aIsOnLinkPrefix) ||
            (prefix.mExpireTime <= now))
        {
            RemoveExternalRoute(prefix.mPrefix);
            ++removedNum;
        }
        else
        {
            earliestExpireTime = OT_MIN(earliestExpireTime, prefix.mExpireTime);
            *keptPrefix        = prefix;
            ++keptPrefix;
            if (prefix.mIsOnLinkPrefix)
            {
                ++keptOnLinkPrefixNum;
            }
        }
    }

    mDiscoveredPrefixNum -= removedNum;

    if (keptOnLinkPrefixNum == 0)
    {
        mDiscoveredPrefixInvalidTimer.Stop();

        if (mAdvertisedOnLinkPrefix == nullptr)
        {
            // There are no valid on-link prefixes on infra link now, start Router Solicitation
            // To find out more on-link prefixes or timeout to advertise my local on-link prefix.
            StartRouterSolicitationDelay();
        }
    }
    else
    {
        mDiscoveredPrefixInvalidTimer.FireAt(earliestExpireTime);
    }

    return (removedNum != 0); // If anything was removed we need to reevaluate.
}

void RoutingManager::InvalidateAllDiscoveredPrefixes(void)
{
    TimeMilli past = TimerMilli::GetNow();

    for (uint8_t i = 0; i < mDiscoveredPrefixNum; ++i)
    {
        mDiscoveredPrefixes[i].mExpireTime = past;
    }

    InvalidateDiscoveredPrefixes();

    OT_ASSERT(mDiscoveredPrefixNum == 0);
}

// Adds a discovered prefix on infra link. If the same prefix already exists,
// only the lifetime will be updated. Returns a boolean which indicates whether
// a new prefix is added.
bool RoutingManager::AddDiscoveredPrefix(const Ip6::Prefix &aPrefix,
                                         bool               aIsOnLinkPrefix,
                                         uint32_t           aLifetime,
                                         otRoutePreference  aRoutePreference)
{
    OT_ASSERT(aIsOnLinkPrefix ? IsValidOmrPrefix(aPrefix) : IsValidOnLinkPrefix(aPrefix));
    OT_ASSERT(aLifetime > 0);

    bool added = false;

    for (uint8_t i = 0; i < mDiscoveredPrefixNum; ++i)
    {
        ExternalPrefix &prefix = mDiscoveredPrefixes[i];

        if (aPrefix == prefix.mPrefix && aIsOnLinkPrefix == prefix.mIsOnLinkPrefix)
        {
            prefix.mExpireTime = TimerMilli::GetNow() + GetPrefixExpireDelay(aLifetime);
            mDiscoveredPrefixInvalidTimer.FireAtIfEarlier(prefix.mExpireTime);

            otLogInfoBr("discovered prefix %s refreshed lifetime: %u seconds", aPrefix.ToString().AsCString(),
                        aLifetime);
            ExitNow();
        }
    }

    if (mDiscoveredPrefixNum < kMaxDiscoveredPrefixNum)
    {
        ExternalPrefix &newPrefix = mDiscoveredPrefixes[mDiscoveredPrefixNum];

        SuccessOrExit(AddExternalRoute(aPrefix, aRoutePreference));

        if (aIsOnLinkPrefix && mRouterSolicitCount > 0)
        {
            // Stop Router Solicitation if we discovered a valid on-link prefix.
            // Otherwise, we wait till the Router Solicitation process times out.
            // So the maximum delay before the Border Router starts advertising
            // its own on-link prefix is 10 seconds = RS_DELAY (1) + RS_INTERNAL (4)
            // + RS_INTERVAL (4) + RS_DELAY (1).
            //
            // Always send at least one RS message so that all BRs will respond.
            // Consider that there are multiple BRs on the infra link, we may not learn
            // RIOs of other BRs if the router discovery process is stopped immediately
            // before sending any RS messages because of receiving an unsolicited RA.
            mRouterSolicitTimer.Stop();
        }

        newPrefix.mPrefix         = aPrefix;
        newPrefix.mIsOnLinkPrefix = aIsOnLinkPrefix;
        newPrefix.mExpireTime     = TimerMilli::GetNow() + GetPrefixExpireDelay(aLifetime);
        mDiscoveredPrefixInvalidTimer.FireAtIfEarlier(newPrefix.mExpireTime);

        ++mDiscoveredPrefixNum;
        added = true;
    }
    else
    {
        otLogWarnBr("discovered too many prefixes, ignore new prefix %s", aPrefix.ToString().AsCString());
    }

exit:
    return added;
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
bool RoutingManager::UpdateRouterAdvMessage(const RouterAdv::RouterAdvMessage &aRouterAdvMessage)
{
    RouterAdv::RouterAdvMessage oldRouterAdvMessage;

    oldRouterAdvMessage = mRouterAdvMessage;
    if (aRouterAdvMessage.GetRouterLifetime() == 0)
    {
        mRouterAdvMessage.SetToDefault();
    }
    else
    {
        mRouterAdvMessage = aRouterAdvMessage;
        // TODO: add a timer for invalidating the learned RA parameters
        // for cases that the other RA daemon crashed or is force killed.
    }

    return (mRouterAdvMessage != oldRouterAdvMessage);
}

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
