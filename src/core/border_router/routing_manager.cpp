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
    , mAdvertisedOmrPrefixNum(0)
    , mAdvertisedOnLinkPrefix(nullptr)
    , mRouterAdvertisementTimer(aInstance, HandleRouterAdvertisementTimer, this)
    , mRouterAdvertisementCount(0)
    , mRouterSolicitTimer(aInstance, HandleRouterSolicitTimer, this)
    , mRouterSolicitCount(0)
    , mDiscoveredOnLinkPrefixInvalidTimer(aInstance, HandleDiscoveredOnLinkPrefixInvalidTimer, this)
{
    mLocalOmrPrefix.Clear();
    memset(mAdvertisedOmrPrefixes, 0, sizeof(mAdvertisedOmrPrefixes));

    mLocalOnLinkPrefix.Clear();
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
        mIsRunning = true;
        StartRouterSolicitation();
    }
}

void RoutingManager::Stop(void)
{
    VerifyOrExit(mIsRunning);

    UnpublishLocalOmrPrefix();

    // Use empty OMR & on-link prefixes to invalidate possible advertised prefixes.
    SendRouterAdvertisement(nullptr, 0, nullptr);

    memset(mAdvertisedOmrPrefixes, 0, sizeof(mAdvertisedOmrPrefixes));
    mAdvertisedOmrPrefixNum = 0;

    mAdvertisedOnLinkPrefix = nullptr;

    mDiscoveredOnLinkPrefix.Clear();
    mDiscoveredOnLinkPrefixInvalidTimer.Stop();

    mRouterAdvertisementTimer.Stop();
    mRouterAdvertisementCount = 0;

    mRouterSolicitTimer.Stop();
    mRouterSolicitCount = 0;

    mIsRunning = false;

exit:
    return;
}

void RoutingManager::RecvIcmp6Message(uint32_t            aInfraIfIndex,
                                      const Ip6::Address &aSrcAddress,
                                      const uint8_t *     aBuffer,
                                      uint16_t            aBufferLength)
{
    const Ip6::Icmp::Header *icmp6Header;

    VerifyOrExit(mIsRunning);

    VerifyOrExit(aInfraIfIndex == mInfraIfIndex);
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
        break;
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

        if (!IsValidOmrPrefix(onMeshPrefixConfig.GetPrefix()) || !onMeshPrefixConfig.mDefaultRoute ||
            !onMeshPrefixConfig.mSlaac || onMeshPrefixConfig.mDp)
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
    omrPrefixConfig.mDefaultRoute = true;

    error = Get<NetworkData::Local>().AddOnMeshPrefix(omrPrefixConfig);
    if (error != OT_ERROR_NONE)
    {
        otLogInfoBr("failed to publish local OMR prefix %s in Thread network: %s",
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
    VerifyOrExit(mIsRunning);

    IgnoreError(Get<NetworkData::Local>().RemoveOnMeshPrefix(mLocalOmrPrefix));
    Get<NetworkData::Notifier>().HandleServerDataUpdated();

    otLogInfoBr("unpubished local OMR prefix %s from Thread network", mLocalOmrPrefix.ToString().AsCString());

exit:
    return;
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

const Ip6::Prefix *RoutingManager::EvaluateOnLinkPrefix(void) const
{
    const Ip6::Prefix *newOnLinkPrefix = nullptr;

    // We don't evaluate on-link prefix if we are doing
    // Router Discovery or we have already discovered some
    // on-link prefixes.
    VerifyOrExit(!mRouterSolicitTimer.IsRunning());
    if (IsValidOnLinkPrefix(mDiscoveredOnLinkPrefix))
    {
        otLogInfoBr("EvaluateOnLinkPrefix: there is already on-link prefix %s on interface %u",
                    mDiscoveredOnLinkPrefix.ToString().AsCString(), mInfraIfIndex);
        ExitNow();
    }

    newOnLinkPrefix = &mLocalOnLinkPrefix;

exit:
    return newOnLinkPrefix;
}

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
        mRouterAdvertisementTimer.Start(nextSendTime * 1000);
    }

    // 3. Update advertised on-link & OMR prefixes information.
    mAdvertisedOnLinkPrefix = newOnLinkPrefix;
    static_assert(sizeof(mAdvertisedOmrPrefixes) == sizeof(newOmrPrefixes), "invalid new OMR prefix array size");
    memcpy(mAdvertisedOmrPrefixes, newOmrPrefixes, sizeof(newOmrPrefixes[0]) * newOmrPrefixNum);
    mAdvertisedOmrPrefixNum = newOmrPrefixNum;

exit:
    return;
}

void RoutingManager::StartRouterSolicitation(void)
{
    mRouterSolicitCount = 0;
    mRouterSolicitTimer.Start(Random::NonCrypto::GetUint32InRange(0, kMaxRtrSolicitationDelay * 1000));
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

        OT_ASSERT(bufferLength + pio.GetLength() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &pio, pio.GetLength());
        bufferLength += pio.GetLength();

        if (mAdvertisedOnLinkPrefix != nullptr)
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

        // Set zero valid lifetime to immediately invalidate the advertised on-link prefix.
        pio.SetValidLifetime(0);
        pio.SetPreferredLifetime(0);
        pio.SetPrefix(*mAdvertisedOnLinkPrefix);

        OT_ASSERT(bufferLength + pio.GetLength() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &pio, pio.GetLength());
        bufferLength += pio.GetLength();

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

            OT_ASSERT(bufferLength + rio.GetLength() <= sizeof(buffer));
            memcpy(buffer + bufferLength, &rio, rio.GetLength());
            bufferLength += rio.GetLength();

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

        OT_ASSERT(bufferLength + rio.GetLength() <= sizeof(buffer));
        memcpy(buffer + bufferLength, &rio, rio.GetLength());
        bufferLength += rio.GetLength();

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

    otLogInfoBr("received Router Solicitation from %s on interface %u", aSrcAddress.ToString().AsCString(),
                mInfraIfIndex);

    mRouterAdvertisementTimer.Start(Random::NonCrypto::GetUint32InRange(0, kMaxRaDelayTime));
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
        const PrefixInfoOption *pio;
        Ip6::Prefix             prefix;

        if (option->GetType() != Option::Type::kPrefixInfo)
        {
            continue;
        }

        pio = static_cast<const PrefixInfoOption *>(option);

        if (!pio->IsValid())
        {
            continue;
        }

        pio->GetPrefix(prefix);
        if (!IsValidOnLinkPrefix(prefix))
        {
            otLogInfoBr("ignore invalid prefix in PIO: %s", prefix.ToString().AsCString());
            continue;
        }

        if (pio->GetValidLifetime() == 0)
        {
            if (mDiscoveredOnLinkPrefix == prefix)
            {
                otLogInfoBr("invalidate discovered on-link prefix %s", prefix.ToString().AsCString());
                mDiscoveredOnLinkPrefixInvalidTimer.Stop();
                mDiscoveredOnLinkPrefix.Clear();
                needReevaluate = true;
            }
        }
        else
        {
            otLogInfoBr("set discovered on-link prefix to %s, valid lifetime: %u seconds",
                        prefix.ToString().AsCString(), pio->GetValidLifetime());

            // We keep tracking the latest on-link prefix.
            mDiscoveredOnLinkPrefixInvalidTimer.Start(GetPrefixExpireDelay(pio->GetValidLifetime()));
            mDiscoveredOnLinkPrefix = prefix;

            // Stop Router Solicitation if we found a valid on-link prefix.
            // Otherwise, we wait till the Router Solicitation process times out.
            // So the maximum delay before the Border Router starts advertising
            // its own on-link prefix is 9 (4 + 4 + 1) seconds.
            mRouterSolicitTimer.Stop();
            needReevaluate = true;
        }
    }

    if (needReevaluate)
    {
        EvaluateRoutingPolicy();
    }

exit:
    return;
}

} // namespace BorderRouter

} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
