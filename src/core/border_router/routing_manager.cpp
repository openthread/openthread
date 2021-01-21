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
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "common/settings.hpp"
#include "net/ip6.hpp"
#include "thread/network_data.hpp"
#include "thread/network_data_leader.hpp"
#include "thread/network_data_local.hpp"
#include "thread/network_data_notifier.hpp"

namespace ot {

namespace BorderRouter {

RoutingManager::RoutingManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mIsRunning(false)
    , mInfraIfIndex(0)
    , mEnabled(true) // The routing manager is by default enabled.
    , mAdvertisedOmrPrefixNum(0)
    , mAdvertisedOnLinkPrefix(nullptr)
    , mDiscoveredPrefixNum(0)
    , mDiscoveredPrefixInvalidTimer(aInstance, HandleDiscoveredPrefixInvalidTimer, this)
    , mRouterAdvertisementTimer(aInstance, HandleRouterAdvertisementTimer, this)
    , mRouterAdvertisementCount(0)
    , mRouterSolicitTimer(aInstance, HandleRouterSolicitTimer, this)
    , mRouterSolicitCount(0)
{
    mLocalOmrPrefix.Clear();
    memset(mAdvertisedOmrPrefixes, 0, sizeof(mAdvertisedOmrPrefixes));

    mLocalOnLinkPrefix.Clear();

    memset(mDiscoveredPrefixes, 0, sizeof(mDiscoveredPrefixes));
}

otError RoutingManager::Init(uint32_t aInfraIfIndex)
{
    otError error;

    OT_ASSERT(!IsInitialized());
    VerifyOrExit(aInfraIfIndex > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = LoadOrGenerateRandomOmrPrefix());
    SuccessOrExit(error = LoadOrGenerateRandomOnLinkPrefix());

    mInfraIfIndex = aInfraIfIndex;

exit:
    return error;
}

otError RoutingManager::SetEnabled(bool aEnabled)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(IsInitialized(), error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(aEnabled != mEnabled);

    mEnabled = aEnabled;

    if (!mEnabled)
    {
        Stop();
    }
    else if (Get<Mle::MleRouter>().IsAttached())
    {
        Start();
    }

exit:
    return error;
}

otError RoutingManager::LoadOrGenerateRandomOmrPrefix(void)
{
    otError error = OT_ERROR_NONE;

    if (Get<Settings>().ReadOmrPrefix(mLocalOmrPrefix) != OT_ERROR_NONE || !IsValidOmrPrefix(mLocalOmrPrefix))
    {
        Ip6::NetworkPrefix randomOmrPrefix;

        otLogNoteBr("no valid OMR prefix found in settings, generating new one");

        error = randomOmrPrefix.GenerateRandomUla();
        if (error != OT_ERROR_NONE)
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

otError RoutingManager::LoadOrGenerateRandomOnLinkPrefix(void)
{
    otError error = OT_ERROR_NONE;

    if (Get<Settings>().ReadOnLinkPrefix(mLocalOnLinkPrefix) != OT_ERROR_NONE ||
        !IsValidOnLinkPrefix(mLocalOnLinkPrefix))
    {
        Ip6::NetworkPrefix randomOnLinkPrefix;

        otLogNoteBr("no valid on-link prefix found in settings, generating new one");

        error = randomOnLinkPrefix.GenerateRandomUla();
        if (error != OT_ERROR_NONE)
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

void RoutingManager::Start(void)
{
    if (!mIsRunning)
    {
        otLogInfoBr("Border Routing manager started");

        mIsRunning = true;
        StartRouterSolicitation();
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
    otError                  error = OT_ERROR_NONE;
    const Ip6::Icmp::Header *icmp6Header;
    const Ip6::Address *     infraLinkLocalAddr;

    VerifyOrExit(IsInitialized() && mIsRunning, error = OT_ERROR_DROP);

    VerifyOrExit(aInfraIfIndex == mInfraIfIndex, error = OT_ERROR_DROP);
    infraLinkLocalAddr = static_cast<const Ip6::Address *>(otPlatInfraIfGetLinkLocalAddress(mInfraIfIndex));

    // Drop any ICMPv6 messages sent from myself.
    VerifyOrExit(infraLinkLocalAddr != nullptr && aSrcAddress != *infraLinkLocalAddr, error = OT_ERROR_DROP);

    VerifyOrExit(aBuffer != nullptr && aBufferLength >= sizeof(*icmp6Header), error = OT_ERROR_PARSE);

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
    if (error != OT_ERROR_NONE)
    {
        otLogDebgBr("drop ICMPv6 message: %s", otThreadErrorToString(error));
    }
}

void RoutingManager::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(IsInitialized() && IsEnabled());

    if (aEvents.Contains(kEventThreadRoleChanged))
    {
        if (Get<Mle::MleRouter>().IsAttached())
        {
            Start();
        }
        else
        {
            Stop();
        }
    }

    if (aEvents.Contains(kEventThreadNetdataChanged))
    {
        EvaluateRoutingPolicy();
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

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, onMeshPrefixConfig) == OT_ERROR_NONE)
    {
        uint8_t newPrefixIndex;

        if (!IsValidOmrPrefix(onMeshPrefixConfig.GetPrefix()) || !onMeshPrefixConfig.mSlaac || onMeshPrefixConfig.mDp)
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
        if (smallestOmrPrefix == nullptr || IsPrefixSmallerThan(onMeshPrefixConfig.GetPrefix(), *smallestOmrPrefix))
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
        if (PublishLocalOmrPrefix() == OT_ERROR_NONE)
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

otError RoutingManager::PublishLocalOmrPrefix(void)
{
    otError                         error = OT_ERROR_NONE;
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
    if (error != OT_ERROR_NONE)
    {
        otLogWarnBr("failed to publish local OMR prefix %s in Thread network: %s",
                    mLocalOmrPrefix.ToString().AsCString(), otThreadErrorToString(error));
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
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mIsRunning);

    SuccessOrExit(error = Get<NetworkData::Local>().RemoveOnMeshPrefix(mLocalOmrPrefix));

    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    otLogInfoBr("unpublished local OMR prefix %s from Thread network", mLocalOmrPrefix.ToString().AsCString());

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnBr("failed to unpublish local OMR prefix %s from Thread network: %s",
                    mLocalOmrPrefix.ToString().AsCString(), otThreadErrorToString(error));
    }
}

otError RoutingManager::AddExternalRoute(const Ip6::Prefix &aPrefix, otRoutePreference aRoutePreference)
{
    otError                          error;
    NetworkData::ExternalRouteConfig routeConfig;

    OT_ASSERT(mIsRunning);

    routeConfig.Clear();
    routeConfig.SetPrefix(aPrefix);
    routeConfig.mStable     = true;
    routeConfig.mPreference = aRoutePreference;

    error = Get<NetworkData::Local>().AddHasRoutePrefix(routeConfig);
    if (error != OT_ERROR_NONE)
    {
        otLogWarnBr("failed to add external route %s: %s", aPrefix.ToString().AsCString(),
                    otThreadErrorToString(error));
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
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mIsRunning);

    SuccessOrExit(error = Get<NetworkData::Local>().RemoveHasRoutePrefix(aPrefix));

    Get<NetworkData::Notifier>().HandleServerDataUpdated();
    otLogInfoBr("removed external route %s", aPrefix.ToString().AsCString());

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnBr("failed to remove external route %s: %s", aPrefix.ToString().AsCString(),
                    otThreadErrorToString(error));
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

        if (smallestOnLinkPrefix == nullptr || IsPrefixSmallerThan(prefix.mPrefix, *smallestOnLinkPrefix))
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
        else if (AddExternalRoute(mLocalOnLinkPrefix, OT_ROUTE_PREFERENCE_MED) == OT_ERROR_NONE)
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
        if (IsPrefixSmallerThan(*mAdvertisedOnLinkPrefix, *smallestOnLinkPrefix))
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
    const Ip6::Prefix *newOnLinkPrefix = nullptr;
    Ip6::Prefix        newOmrPrefixes[kMaxOmrPrefixNum];
    uint8_t            newOmrPrefixNum = 0;

    VerifyOrExit(mIsRunning);

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

exit:
    return;
}

// starts sending Router Solicitations in random delay
// between 0 and kMaxRtrSolicitationDelay.
void RoutingManager::StartRouterSolicitation(void)
{
    uint32_t randomDelay;

    mRouterSolicitCount = 0;

    static_assert(kMaxRtrSolicitationDelay > 0, "invalid maximum Router Solicitation delay");
    randomDelay = Random::NonCrypto::GetUint32InRange(0, Time::SecToMsec(kMaxRtrSolicitationDelay));

    otLogInfoBr("start Router Solicitation, scheduled in %u milliseconds", randomDelay);
    mRouterSolicitTimer.Start(randomDelay);
}

otError RoutingManager::SendRouterSolicitation(void)
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
    uint8_t                     buffer[kMaxRouterAdvMessageLength];
    uint16_t                    bufferLength = 0;
    RouterAdv::RouterAdvMessage routerAdv;

    // Set zero Router Lifetime to indicate that the Border Router is not the default
    // router for infra link so that hosts on infra link will not create default route
    // to the Border Router when received RA.
    routerAdv.SetRouterLifetime(0);

    OT_ASSERT(bufferLength + sizeof(routerAdv) <= sizeof(buffer));
    memcpy(buffer, &routerAdv, sizeof(routerAdv));
    bufferLength += sizeof(routerAdv);

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
    if (bufferLength > sizeof(routerAdv))
    {
        otError      error;
        Ip6::Address destAddress;

        ++mRouterAdvertisementCount;

        destAddress.SetToLinkLocalAllNodesMulticast();
        error = otPlatInfraIfSendIcmp6Nd(mInfraIfIndex, &destAddress, buffer, bufferLength);

        if (error == OT_ERROR_NONE)
        {
            otLogInfoBr("sent Router Advertisement on interface %u", mInfraIfIndex);
        }
        else
        {
            otLogWarnBr("failed to send Router Advertisement on interface %u: %s", mInfraIfIndex,
                        otThreadErrorToString(error));
        }
    }
}

bool RoutingManager::IsPrefixSmallerThan(const Ip6::Prefix &aFirstPrefix, const Ip6::Prefix &aSecondPrefix)
{
    uint8_t matchedLength;

    OT_ASSERT(aFirstPrefix.GetLength() == aSecondPrefix.GetLength());

    matchedLength =
        Ip6::Prefix::MatchLength(aFirstPrefix.GetBytes(), aSecondPrefix.GetBytes(), aFirstPrefix.GetBytesSize());

    return matchedLength < aFirstPrefix.GetLength() &&
           aFirstPrefix.GetBytes()[matchedLength / CHAR_BIT] < aSecondPrefix.GetBytes()[matchedLength / CHAR_BIT];
}

bool RoutingManager::IsValidOmrPrefix(const Ip6::Prefix &aOmrPrefix)
{
    // Accept ULA prefix with length of 64 bits and GUA prefix.
    return (aOmrPrefix.mLength == kOmrPrefixLength && aOmrPrefix.mPrefix.mFields.m8[0] == 0xfd) ||
           (aOmrPrefix.mLength >= 3 && (aOmrPrefix.GetBytes()[0] & 0xE0) == 0x20);
}

bool RoutingManager::IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix)
{
    // Accept ULA prefix with length of 64 bits and GUA prefix.
    return (aOnLinkPrefix.mLength == kOnLinkPrefixLength && aOnLinkPrefix.mPrefix.mFields.m8[0] == 0xfd) ||
           (aOnLinkPrefix.mLength >= 3 && (aOnLinkPrefix.GetBytes()[0] & 0xE0) == 0x20);
}

void RoutingManager::HandleRouterAdvertisementTimer(Timer &aTimer)
{
    aTimer.GetOwner<RoutingManager>().HandleRouterAdvertisementTimer();
}

void RoutingManager::HandleRouterAdvertisementTimer(void)
{
    otLogInfoBr("router advertisement timer triggered");

    EvaluateRoutingPolicy();
}

void RoutingManager::HandleRouterSolicitTimer(Timer &aTimer)
{
    aTimer.GetOwner<RoutingManager>().HandleRouterSolicitTimer();
}

void RoutingManager::HandleRouterSolicitTimer(void)
{
    otLogInfoBr("router solicitation times out");

    if (mRouterSolicitCount < kMaxRtrSolicitations)
    {
        uint32_t nextSolicitationDelay;
        otError  error;

        error = SendRouterSolicitation();
        ++mRouterSolicitCount;

        if (error == OT_ERROR_NONE)
        {
            otLogDebgBr("successfully sent %uth Router Solicitation", mRouterSolicitCount);
        }
        else
        {
            otLogCritBr("failed to send %uth Router Solicitation: %s", mRouterSolicitCount,
                        otThreadErrorToString(error));
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
    aTimer.GetOwner<RoutingManager>().HandleDiscoveredPrefixInvalidTimer();
}

void RoutingManager::HandleDiscoveredPrefixInvalidTimer(void)
{
    InvalidateDiscoveredPrefixes();
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
    OT_UNUSED_VARIABLE(aSrcAddress);

    using RouterAdv::Option;
    using RouterAdv::PrefixInfoOption;
    using RouterAdv::RouteInfoOption;
    using RouterAdv::RouterAdvMessage;

    bool           needReevaluate = false;
    const uint8_t *optionsBegin;
    uint16_t       optionsLength;
    const Option * option;

    VerifyOrExit(aBufferLength >= sizeof(RouterAdvMessage));

    otLogInfoBr("received Router Advertisement from %s on interface %u", aSrcAddress.ToString().AsCString(),
                mInfraIfIndex);

    optionsBegin  = aBuffer + sizeof(RouterAdvMessage);
    optionsLength = aBufferLength - sizeof(RouterAdvMessage);

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
                needReevaluate |= UpdateDiscoveredPrefixes(*pio);
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

    if (needReevaluate)
    {
        EvaluateRoutingPolicy();
    }

exit:
    return;
}

bool RoutingManager::UpdateDiscoveredPrefixes(const RouterAdv::PrefixInfoOption &aPio)
{
    Ip6::Prefix prefix         = aPio.GetPrefix();
    bool        needReevaluate = false;

    if (!IsValidOnLinkPrefix(prefix))
    {
        otLogInfoBr("ignore invalid prefix in PIO: %s", prefix.ToString().AsCString());
        ExitNow();
    }

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
        otLogInfoBr("ignore invalid prefix in RIO: %s", prefix.ToString().AsCString());
        ExitNow();
    }

    // Ignore the OMR prefix that matches what we have advertised.
    VerifyOrExit(!ContainsPrefix(prefix, mAdvertisedOmrPrefixes, mAdvertisedOmrPrefixNum));

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

        // There are no valid on-link prefixes on infra link now, start Router Solicitation
        // To find out more on-link prefixes or timeout to advertise my local on-link prefix.
        StartRouterSolicitation();
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

        if (aIsOnLinkPrefix)
        {
            // Stop Router Solicitation if we discovered a valid on-link prefix.
            // Otherwise, we wait till the Router Solicitation process times out.
            // So the maximum delay before the Border Router starts advertising
            // its own on-link prefix is 9 (4 + 4 + 1) seconds.
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

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
