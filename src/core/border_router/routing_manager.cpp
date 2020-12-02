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

RoutingManager::RoutingManager(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mInfraIfIndex(0)
    , mRouterAdvertisementTimer(aInstance, HandleRouterAdvertisementTimer, this)
    , mRouterAdvertisementCount(0)
    , mRouterSolicitTimer(aInstance, HandleRouterSolicitTimer, this)
    , mRouterSolicitCount(0)
    , mDiscoveredOnLinkPrefixInvalidTimer(aInstance, HandleDiscoveredOnLinkPrefixInvalidTimer, this)
{
    mLocalOmrPrefix.Clear();
    mAdvertisedOmrPrefix.Clear();

    mLocalOnLinkPrefix.Clear();
    mAdvertisedOnLinkPrefix.Clear();
    mDiscoveredOnLinkPrefix.Clear();
}

otError RoutingManager::Init(uint32_t aInfraIfIndex)
{
    otError error;

    OT_ASSERT(!IsInitialized() && !Get<Mle::MleRouter>().IsAttached());
    VerifyOrExit(aInfraIfIndex > 0, error = OT_ERROR_INVALID_ARGS);

    SuccessOrExit(error = LoadOrGenerateRandomOmrPrefix());
    SuccessOrExit(error = LoadOrGenerateRandomOnLinkPrefix());

    mInfraIfIndex = aInfraIfIndex;

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

        if (randomOmrPrefix.GenerateRandomUla() != OT_ERROR_NONE)
        {
            otLogCritBr("failed to generate random OMR prefix");
            ExitNow(error = OT_ERROR_FAILED);
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

        if (randomOnLinkPrefix.GenerateRandomUla() != OT_ERROR_NONE)
        {
            otLogCritBr("failed to generate random on-link prefix");
            ExitNow(error = OT_ERROR_FAILED);
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
    StartRouterSolicitation();
}

void RoutingManager::Stop(void)
{
    Ip6::Prefix invalidOmrPrefix;
    Ip6::Prefix invalidOnLinkPrefix;

    if (mAdvertisedOmrPrefix == mLocalOmrPrefix)
    {
        UnpublishOmrPrefix(mAdvertisedOmrPrefix);
    }

    invalidOmrPrefix.Clear();
    invalidOnLinkPrefix.Clear();

    // Use invalid OMR & on-link prefixes to invalidate possiblely advertised prefixes.
    SendRouterAdvertisement(invalidOmrPrefix, invalidOnLinkPrefix);

    mAdvertisedOmrPrefix.Clear();
    mAdvertisedOnLinkPrefix.Clear();

    mDiscoveredOnLinkPrefix.Clear();
    mDiscoveredOnLinkPrefixInvalidTimer.Stop();

    mRouterAdvertisementTimer.Stop();
    mRouterAdvertisementCount = 0;

    mRouterSolicitTimer.Stop();
    mRouterSolicitCount = 0;
}

void RoutingManager::RecvIcmp6Message(uint32_t            aInfraIfIndex,
                                      const Ip6::Address &aSrcAddress,
                                      const uint8_t *     aBuffer,
                                      uint16_t            aBufferLength)
{
    const Ip6::Icmp::Header *icmp6Header;

    VerifyOrExit(IsInitialized() && aInfraIfIndex == mInfraIfIndex);
    VerifyOrExit(aBuffer != nullptr && aBufferLength >= sizeof(*icmp6Header));

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
        ExitNow();
    }

exit:
    return;
}

void RoutingManager::HandleNotifierEvents(Events aEvents)
{
    VerifyOrExit(IsInitialized());

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

Ip6::Prefix RoutingManager::EvaluateOmrPrefix(void)
{
    Ip6::Prefix                     lowestOmrPrefix;
    Ip6::Prefix                     newOmrPrefix;
    NetworkData::Iterator           iterator = NetworkData::kIteratorInit;
    NetworkData::OnMeshPrefixConfig onMeshPrefixConfig;

    lowestOmrPrefix.Clear();
    newOmrPrefix.Clear();

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached());

    while (Get<NetworkData::Leader>().GetNextOnMeshPrefix(iterator, onMeshPrefixConfig) == OT_ERROR_NONE)
    {
        if (!IsValidOmrPrefix(onMeshPrefixConfig.GetPrefix()) || !onMeshPrefixConfig.mDefaultRoute ||
            !onMeshPrefixConfig.mSlaac)
        {
            continue;
        }

        if (!IsValidOmrPrefix(lowestOmrPrefix) || onMeshPrefixConfig.GetPrefix() < lowestOmrPrefix)
        {
            lowestOmrPrefix = onMeshPrefixConfig.GetPrefix();
        }
    }

    if (IsValidOmrPrefix(lowestOmrPrefix) && lowestOmrPrefix != mLocalOmrPrefix)
    {
        otLogInfoBr("EvaluateOmrPrefix: adopt existing OMR prefix %s in Thread network",
                    lowestOmrPrefix.ToString().AsCString());
        newOmrPrefix = lowestOmrPrefix;
    }
    else
    {
        newOmrPrefix = mLocalOmrPrefix;
    }

exit:
    return newOmrPrefix;
}

void RoutingManager::PublishOmrPrefix(const Ip6::Prefix &aOmrPrefix)
{
    otError                         error;
    NetworkData::OnMeshPrefixConfig omrPrefixConfig;

    VerifyOrExit(Get<Mle::MleRouter>().IsAttached());
    VerifyOrExit(IsValidOmrPrefix(aOmrPrefix));

    omrPrefixConfig.Clear();
    omrPrefixConfig.mPrefix       = aOmrPrefix;
    omrPrefixConfig.mStable       = true;
    omrPrefixConfig.mSlaac        = true;
    omrPrefixConfig.mPreferred    = true;
    omrPrefixConfig.mOnMesh       = true;
    omrPrefixConfig.mDefaultRoute = true;

    error = Get<NetworkData::Local>().AddOnMeshPrefix(omrPrefixConfig);
    if (error != OT_ERROR_NONE)
    {
        otLogInfoBr("failed to publish OMR prefix %s in Thread network: %s", aOmrPrefix.ToString().AsCString(),
                    otThreadErrorToString(error));
    }
    else
    {
        Get<NetworkData::Notifier>().HandleServerDataUpdated();
        otLogInfoBr("published OMR prefix %s in Thread network", aOmrPrefix.ToString().AsCString());
    }

exit:
    return;
}

void RoutingManager::UnpublishOmrPrefix(const Ip6::Prefix &aOmrPrefix)
{
    VerifyOrExit(Get<Mle::MleRouter>().IsAttached());
    VerifyOrExit(IsValidOmrPrefix(aOmrPrefix));

    IgnoreError(Get<NetworkData::Local>().RemoveOnMeshPrefix(aOmrPrefix));
    Get<NetworkData::Notifier>().HandleServerDataUpdated();

exit:
    return;
}

Ip6::Prefix RoutingManager::EvaluateOnLinkPrefix(void)
{
    Ip6::Prefix newOnLinkPrefix;

    newOnLinkPrefix.Clear();

    // We don't evalute on-link prefix if we are doing
    // Router Discovery or we have already discovered some
    // on-link prefixes.
    VerifyOrExit(!mRouterSolicitTimer.IsRunning());
    if (IsValidOnLinkPrefix(mDiscoveredOnLinkPrefix))
    {
        otLogInfoBr("EvaluateOnLinkPrefix: there is already on-link prefix %s on interface %s",
                    mDiscoveredOnLinkPrefix.ToString().AsCString(), GetInfraIfName());
        ExitNow();
    }

    if (IsValidOnLinkPrefix(mAdvertisedOnLinkPrefix))
    {
        newOnLinkPrefix = mAdvertisedOnLinkPrefix;
    }
    else
    {
        newOnLinkPrefix = mLocalOnLinkPrefix;
    }

exit:
    return newOnLinkPrefix;
}

void RoutingManager::EvaluateRoutingPolicy(void)
{
    Ip6::Prefix newOnLinkPrefix;
    Ip6::Prefix newOmrPrefix;

    OT_ASSERT(IsInitialized());

    otLogInfoBr("evaluating routing policy");

    newOnLinkPrefix = EvaluateOnLinkPrefix();
    newOmrPrefix    = EvaluateOmrPrefix();

    if (newOmrPrefix == mLocalOmrPrefix)
    {
        if (!IsValidOmrPrefix(mAdvertisedOmrPrefix))
        {
            otLogInfoBr("publish new OMR prefix in Thread network");
            PublishOmrPrefix(newOmrPrefix);
        }
    }
    else
    {
        if (IsValidOmrPrefix(mAdvertisedOmrPrefix))
        {
            otLogInfoBr("there is already OMR prefix in the Thread network, stop publishing");
            UnpublishOmrPrefix(mAdvertisedOmrPrefix);
            mAdvertisedOmrPrefix.Clear();
        }
    }

    SendRouterAdvertisement(newOmrPrefix, newOnLinkPrefix);

    mAdvertisedOnLinkPrefix = newOnLinkPrefix;
    mAdvertisedOmrPrefix    = newOmrPrefix;

    return;
}

void RoutingManager::StartRouterSolicitation()
{
    mRouterSolicitCount = 0;
    mRouterSolicitTimer.Start(Random::NonCrypto::GetUint32InRange(0, kMaxRtrSolicitationDelay * 1000));
}

otError RoutingManager::SendRouterSolicit(void)
{
    Ip6::Address                    destAddress;
    RouterAdv::RouterSolicitMessage routerSolicit;

    OT_ASSERT(IsInitialized());

    destAddress.SetToLinkLocalAllRoutersMulticast();
    return otPlatInfraIfSendIcmp6(mInfraIfIndex, &destAddress, reinterpret_cast<const uint8_t *>(&routerSolicit),
                                  sizeof(routerSolicit));
}

void RoutingManager::SendRouterAdvertisement(const Ip6::Prefix &aNewOmrPrefix, const Ip6::Prefix &aNewOnLinkPrefix)
{
    uint8_t                     buffer[kMaxRouterAdvMessageLength];
    uint16_t                    bufferLength = 0;
    bool                        startTimer   = false;
    RouterAdv::RouterAdvMessage routerAdv;

    // Set zero Router Lifetime to indicate that the Border Router is not the default
    // router for infra link so that hosts on infra link will not create default route
    // to the Border Router when received RA.
    routerAdv.SetRouterLifetime(0);

    OT_ASSERT(bufferLength + sizeof(routerAdv) <= sizeof(buffer));
    memcpy(buffer, &routerAdv, sizeof(routerAdv));
    bufferLength += sizeof(routerAdv);

    // Invalidate the advertised on-link prefix if we are advertising a new prefix.
    if (IsValidOnLinkPrefix(mAdvertisedOnLinkPrefix) && aNewOnLinkPrefix != mAdvertisedOnLinkPrefix)
    {
        RouterAdv::PrefixInfoOption pio;

        // Set zero valid lifetime to immediately invalidate the previous on-link prefix.
        pio.SetValidLifetime(0);
        pio.SetPreferredLifetime(0);
        pio.SetPrefix(mAdvertisedOnLinkPrefix);

        OT_ASSERT(bufferLength + pio.GetLength() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &pio, pio.GetLength());
        bufferLength += pio.GetLength();

        otLogInfoBr("stop advertising on-link prefix %s on interface %s",
                    mAdvertisedOnLinkPrefix.ToString().AsCString(), GetInfraIfName());
    }

    if (IsValidOnLinkPrefix(aNewOnLinkPrefix))
    {
        RouterAdv::PrefixInfoOption pio;

        pio.SetOnLink(true);
        pio.SetAutoAddrConfig(true);
        pio.SetValidLifetime(kDefaultOnLinkPrefixLifetime);
        pio.SetPreferredLifetime(kDefaultOnLinkPrefixLifetime);
        pio.SetPrefix(aNewOnLinkPrefix);

        OT_ASSERT(bufferLength + pio.GetLength() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &pio, pio.GetLength());
        bufferLength += pio.GetLength();

        startTimer = true;

        if (!IsValidOnLinkPrefix(mAdvertisedOnLinkPrefix))
        {
            otLogInfoBr("start advertising new on-link prefix %s on interface %s",
                        aNewOnLinkPrefix.ToString().AsCString(), GetInfraIfName());
        }

        otLogInfoBr("send on-link prefix %s in PIO (valid lifetime = %u)", aNewOnLinkPrefix.ToString().AsCString(),
                    kDefaultOnLinkPrefixLifetime);
    }

    // Invalidate the advertised OMR prefix if we are advertising a new prefix.
    if (IsValidOmrPrefix(mAdvertisedOmrPrefix) && aNewOmrPrefix != mAdvertisedOmrPrefix)
    {
        RouterAdv::RouteInfoOption rio;

        // Set zero route lifetime to immediately invalidate the previous OMR prefix.
        rio.SetRouteLifetime(0);
        rio.SetPrefix(mAdvertisedOmrPrefix);

        OT_ASSERT(bufferLength + rio.GetLength() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &rio, rio.GetLength());
        bufferLength += rio.GetLength();

        otLogInfoBr("stop advertising OMR prefix %s on interface %s", mAdvertisedOmrPrefix.ToString().AsCString(),
                    GetInfraIfName());
    }

    if (IsValidOmrPrefix(aNewOmrPrefix))
    {
        RouterAdv::RouteInfoOption rio;

        rio.SetRouteLifetime(kDefaultOmrPrefixLifetime);
        rio.SetPrefix(aNewOmrPrefix);

        OT_ASSERT(bufferLength + rio.GetLength() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &rio, rio.GetLength());
        bufferLength += rio.GetLength();

        startTimer = true;

        otLogInfoBr("send OMR prefix %s in RIO (valid lifetime = %u)", aNewOmrPrefix.ToString().AsCString(),
                    kDefaultOmrPrefixLifetime);
    }

    // Send the message only when there are options.
    if (bufferLength > sizeof(routerAdv))
    {
        otError      error;
        Ip6::Address destAddress;

        destAddress.SetToLinkLocalAllNodesMulticast();
        error = otPlatInfraIfSendIcmp6(mInfraIfIndex, &destAddress, buffer, bufferLength);

        if (error == OT_ERROR_NONE)
        {
            otLogInfoBr("sent Router Advertisement on interface %s", GetInfraIfName());
        }
        else
        {
            otLogWarnBr("failed to send Router Advertisement on interface %s: %s", GetInfraIfName(),
                        otThreadErrorToString(error));
        }
    }

    if (startTimer)
    {
        uint32_t nextSendTime;

        ++mRouterAdvertisementCount;

        nextSendTime = Random::NonCrypto::GetUint32InRange(kMinRtrAdvInterval, kMaxRtrAdvInterval);

        if (mRouterAdvertisementCount <= kMaxInitRtrAdvertisements && nextSendTime > kMaxInitRtrAdvInterval)
        {
            nextSendTime = kMaxInitRtrAdvInterval;
        }

        otLogInfoBr("Router Advertisement scheduled in %u Seconds", nextSendTime);
        mRouterAdvertisementTimer.Start(nextSendTime * 1000);
    }
    else
    {
        mRouterAdvertisementTimer.Stop();
    }
}

bool RoutingManager::IsValidOmrPrefix(const Ip6::Prefix &aOmrPrefix)
{
    return (aOmrPrefix.mLength == kOmrPrefixLength && aOmrPrefix.mPrefix.mFields.m8[0] == 0xfd);
}

bool RoutingManager::IsValidOnLinkPrefix(const Ip6::Prefix &aOnLinkPrefix)
{
    return (aOnLinkPrefix.mLength == kOnLinkPrefixLength && aOnLinkPrefix.mPrefix.mFields.m8[0] == 0xfd);
}

void RoutingManager::HandleRouterAdvertisementTimer(Timer &aTimer)
{
    aTimer.GetOwner<RoutingManager>().HandleRouterAdvertisementTimer();
}

void RoutingManager::HandleRouterAdvertisementTimer(void)
{
    otLogInfoBr("Router Advertisement timer triggered");

    SendRouterAdvertisement(mAdvertisedOmrPrefix, mAdvertisedOnLinkPrefix);
}

void RoutingManager::HandleRouterSolicitTimer(Timer &aTimer)
{
    aTimer.GetOwner<RoutingManager>().HandleRouterSolicitTimer();
}

void RoutingManager::HandleRouterSolicitTimer(void)
{
    otLogInfoBr("Router Solicit timeouted");

    if (mRouterSolicitCount < kMaxRtrSolicitations)
    {
        uint32_t nextSolicitationDelay;
        otError  error;

        error = SendRouterSolicit();
        ++mRouterSolicitCount;

        if (error == OT_ERROR_NONE)
        {
            otLogDebgBr("Successfully sent %uth Router Solicitation", mRouterSolicitCount);
        }
        else
        {
            otLogCritBr("failed to send %uth Router Solicitation: %s", mRouterSolicitCount,
                        otThreadErrorToString(error));
        }

        nextSolicitationDelay =
            (mRouterSolicitCount == kMaxRtrSolicitations) ? kMaxRtrSolicitationDelay : kRtrSolicitationInterval;

        otLogDebgBr("Router Solicitation timer scheduled in %u seconds", nextSolicitationDelay);
        mRouterSolicitTimer.Start(nextSolicitationDelay * 1000);
    }
    else
    {
        // Re-evaluate our routing policy and send Router Advertisement if necessary.
        EvaluateRoutingPolicy();
    }
}

void RoutingManager::HandleDiscoveredOnLinkPrefixInvalidTimer(Timer &aTimer)
{
    aTimer.GetOwner<RoutingManager>().HandleDiscoveredOnLinkPrefixInvalidTimer();
}

void RoutingManager::HandleDiscoveredOnLinkPrefixInvalidTimer(void)
{
    otLogInfoBr("invalidate discovered on-link prefix: %s", mDiscoveredOnLinkPrefix.ToString().AsCString());
    mDiscoveredOnLinkPrefix.Clear();

    // The discovered on-link prefix becomes invalid, start Router Solicitation
    // to discover new one.
    StartRouterSolicitation();
}

void RoutingManager::HandleRouterSolicit(const Ip6::Address &aSrcAddress,
                                         const uint8_t *     aBuffer,
                                         uint16_t            aBufferLength)
{
    OT_UNUSED_VARIABLE(aSrcAddress);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aBufferLength);

    otLogInfoBr("received Router Solicitation from %s on interface %s", aSrcAddress.ToString().AsCString(),
                GetInfraIfName());

    mRouterAdvertisementTimer.Start(Random::NonCrypto::GetUint32InRange(0, kMaxRaDelayTime));
}

TimeMilli RoutingManager::GetPrefixExpireTime(uint32_t aValidLifetime)
{
    if (aValidLifetime * 1000ULL + TimerMilli::GetNow().GetValue() < TimeMilli::kMaxDuration)
    {
        return Time(aValidLifetime * 1000 + TimerMilli::GetNow().GetValue());
    }
    else
    {
        return Time(Time::kMaxDuration);
    }
}

void RoutingManager::HandleRouterAdvertisement(const Ip6::Address &aSrcAddress,
                                               const uint8_t *     aBuffer,
                                               uint16_t            aBufferLength)
{
    OT_UNUSED_VARIABLE(aSrcAddress);

    using RouterAdv::Option;
    using RouterAdv::PrefixInfoOption;
    using RouterAdv::RouterAdvMessage;

    bool           needReevaluate = false;
    const uint8_t *optionsBegin;
    uint16_t       optionsLength;
    const Option * option;

    VerifyOrExit(aBufferLength >= sizeof(RouterAdvMessage));

    otLogInfoBr("received Router Advertisement from %s on interface %s", aSrcAddress.ToString().AsCString(),
                GetInfraIfName());

    optionsBegin  = aBuffer + sizeof(RouterAdvMessage);
    optionsLength = aBufferLength - sizeof(RouterAdvMessage);

    option = Option::GetNextOption(nullptr, optionsBegin, optionsLength);
    while (option != nullptr)
    {
        if (option->GetType() == Option::Type::kPrefixInfo)
        {
            auto &      pio = *static_cast<const PrefixInfoOption *>(option);
            Ip6::Prefix prefix;

            prefix.Set(pio.GetPrefix(), pio.GetPrefixLength());
            if (IsValidOnLinkPrefix(prefix))
            {
                TimeMilli expireTime;

                // We tracks the latest on-link prefix.
                expireTime = GetPrefixExpireTime(pio.GetValidLifetime());
                mDiscoveredOnLinkPrefixInvalidTimer.FireAt(expireTime);
                mDiscoveredOnLinkPrefix = prefix;

                otLogInfoBr("set discovered on-link prefix to %s, valid lifetime: %u seconds",
                            mDiscoveredOnLinkPrefix.ToString().AsCString(), pio.GetValidLifetime());

                // Stop Router Solicitation if we found a valid on-link prefix.
                // Otherwise, we wait till the Router Solicitation process timeouted.
                // So the maximum delay before the Border Router starts advertising
                // its own on-link prefix is 9 (4 + 4 + 1) seconds.
                mRouterSolicitTimer.Stop();
                needReevaluate = true;
            }
            else
            {
                otLogInfoBr("ignore invalid prefix in PIO: %s", prefix.ToString().AsCString());
            }
        }
        option = Option::GetNextOption(option, optionsBegin, optionsLength);
    }

    if (needReevaluate)
    {
        EvaluateRoutingPolicy();
    }

exit:
    return;
}

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
