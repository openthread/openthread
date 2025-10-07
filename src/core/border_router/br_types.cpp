/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements common type definitions for Border Router modules.
 */

#include "br_types.hpp"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace BorderRouter {

//---------------------------------------------------------------------------------------------------------------------
// LifetimedPrefix

TimeMilli LifetimedPrefix::CalculateExpirationTime(uint32_t aLifetime) const
{
    // `aLifetime` is in unit of seconds. This method ensures
    // that the time calculation fits with `TimeMilli` range.

    return CalculateClampedExpirationTime(mLastUpdateTime, aLifetime);
}

//---------------------------------------------------------------------------------------------------------------------
// OnLinkPrefix

void OnLinkPrefix::SetFrom(const PrefixInfoOption &aPio)
{
    aPio.GetPrefix(mPrefix);
    mValidLifetime        = aPio.GetValidLifetime();
    mPreferredLifetime    = aPio.GetPreferredLifetime();
    mAutoAddrConfigFlag   = aPio.IsAutoAddrConfigFlagSet();
    mDhcp6PdPreferredFlag = aPio.IsDhcp6PdPreferredFlagSet();
    mLastUpdateTime       = TimerMilli::GetNow();
}

void OnLinkPrefix::SetFrom(const PrefixTableEntry &aPrefixTableEntry)
{
    mPrefix            = AsCoreType(&aPrefixTableEntry.mPrefix);
    mValidLifetime     = aPrefixTableEntry.mValidLifetime;
    mPreferredLifetime = aPrefixTableEntry.mPreferredLifetime;
    mLastUpdateTime    = TimerMilli::GetNow();
}

bool OnLinkPrefix::IsDeprecated(void) const { return GetDeprecationTime() <= TimerMilli::GetNow(); }

TimeMilli OnLinkPrefix::GetDeprecationTime(void) const { return CalculateExpirationTime(mPreferredLifetime); }

TimeMilli OnLinkPrefix::GetStaleTime(void) const
{
    return CalculateExpirationTime(Min(kStaleTime, mPreferredLifetime));
}

void OnLinkPrefix::AdoptFlagsAndValidAndPreferredLifetimesFrom(const OnLinkPrefix &aPrefix)
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

    mPreferredLifetime    = aPrefix.GetPreferredLifetime();
    mAutoAddrConfigFlag   = aPrefix.mAutoAddrConfigFlag;
    mDhcp6PdPreferredFlag = aPrefix.mDhcp6PdPreferredFlag;
    mLastUpdateTime       = aPrefix.GetLastUpdateTime();
}

void OnLinkPrefix::CopyInfoTo(PrefixTableEntry &aEntry, TimeMilli aNow) const
{
    aEntry.mPrefix              = GetPrefix();
    aEntry.mIsOnLink            = true;
    aEntry.mMsecSinceLastUpdate = aNow - GetLastUpdateTime();
    aEntry.mValidLifetime       = GetValidLifetime();
    aEntry.mPreferredLifetime   = GetPreferredLifetime();
}

bool OnLinkPrefix::IsFavoredOver(const Ip6::Prefix &aPrefix) const
{
    bool isFavored = false;

    // Validate that the `OnLinkPrefix` is eligible to be a favored
    // on-link prefix. It must have a valid length and either the
    // AutoAddrConfig (`A`) or Dhcp6PdPreferred (`P`) flag.
    // Additionally, it must not be deprecated and its preferred
    // lifetime must be at least `kFavoredMinPreferredLifetime`
    // (1800) seconds.

    VerifyOrExit(mPrefix.GetLength() == kExpectedFavoredPrefixLength);
    VerifyOrExit(mAutoAddrConfigFlag || mDhcp6PdPreferredFlag);
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

void RoutePrefix::SetFrom(const RouteInfoOption &aRio)
{
    aRio.GetPrefix(mPrefix);
    mValidLifetime   = aRio.GetRouteLifetime();
    mRoutePreference = aRio.GetPreference();
    mLastUpdateTime  = TimerMilli::GetNow();
}

void RoutePrefix::SetFrom(const RouterAdvert::Header &aRaHeader)
{
    mPrefix.Clear();
    mValidLifetime   = aRaHeader.GetRouterLifetime();
    mRoutePreference = aRaHeader.GetDefaultRouterPreference();
    mLastUpdateTime  = TimerMilli::GetNow();
}

TimeMilli RoutePrefix::GetStaleTime(void) const { return CalculateExpirationTime(Min(kStaleTime, mValidLifetime)); }

void RoutePrefix::CopyInfoTo(PrefixTableEntry &aEntry, TimeMilli aNow) const
{
    aEntry.mPrefix              = GetPrefix();
    aEntry.mIsOnLink            = false;
    aEntry.mMsecSinceLastUpdate = aNow - GetLastUpdateTime();
    aEntry.mValidLifetime       = GetValidLifetime();
    aEntry.mPreferredLifetime   = 0;
    aEntry.mRoutePreference     = static_cast<otRoutePreference>(GetRoutePreference());
}

//---------------------------------------------------------------------------------------------------------------------
// RdnssAddress

void RdnssAddress::SetFrom(const RecursiveDnsServerOption &aRdnss, uint16_t aAddressIndex)
{
    mAddress        = aRdnss.GetAddressAt(aAddressIndex);
    mLifetime       = aRdnss.GetLifetime();
    mLastUpdateTime = TimerMilli::GetNow();
}

TimeMilli RdnssAddress::GetExpireTime(void) const { return CalculateClampedExpirationTime(mLastUpdateTime, mLifetime); }

void RdnssAddress::CopyInfoTo(RdnssAddrEntry &aEntry, TimeMilli aNow) const
{
    aEntry.mAddress             = GetAddress();
    aEntry.mMsecSinceLastUpdate = aNow - GetLastUpdateTime();
    aEntry.mLifetime            = GetLifetime();
}

//---------------------------------------------------------------------------------------------------------------------
// IfAddress

void IfAddress::SetFrom(const Ip6::Address &aAddress, uint32_t aUptimeNow)
{
    mAddress       = aAddress;
    mLastUseUptime = aUptimeNow;
}

bool IfAddress::Matches(const InvalidChecker &aChecker) const { return !aChecker.Get<InfraIf>().HasAddress(mAddress); }

void IfAddress::CopyInfoTo(IfAddrEntry &aEntry, uint32_t aUptimeNow) const
{
    aEntry.mAddress         = mAddress;
    aEntry.mSecSinceLastUse = aUptimeNow - mLastUseUptime;
}

//---------------------------------------------------------------------------------------------------------------------
// OmrPrefix

void OmrPrefix::SetPrefix(const Ip6::Prefix &aPrefix, RoutePreference aPreference)
{
    Clear();
    mPrefix     = aPrefix;
    mPreference = aPreference;
}

//---------------------------------------------------------------------------------------------------------------------
// FavoredOmrPrefix

bool FavoredOmrPrefix::IsInfrastructureDerived(void) const
{
    // Indicate whether the OMR prefix is infrastructure-derived which
    // can be identified as a valid OMR prefix with preference of
    // medium or higher.

    return !IsEmpty() && (mPreference >= NetworkData::kRoutePreferenceMedium);
}

void FavoredOmrPrefix::SetFrom(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig)
{
    mPrefix         = aOnMeshPrefixConfig.GetPrefix();
    mPreference     = aOnMeshPrefixConfig.GetPreference();
    mIsDomainPrefix = aOnMeshPrefixConfig.mDp;
}

void FavoredOmrPrefix::SetFrom(const OmrPrefix &aOmrPrefix)
{
    mPrefix         = aOmrPrefix.GetPrefix();
    mPreference     = aOmrPrefix.GetPreference();
    mIsDomainPrefix = aOmrPrefix.IsDomainPrefix();
}

bool FavoredOmrPrefix::IsFavoredOver(const NetworkData::OnMeshPrefixConfig &aOmrPrefixConfig) const
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

bool IsValidOmrPrefix(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig)
{
    return IsValidOmrPrefix(aOnMeshPrefixConfig.GetPrefix()) && aOnMeshPrefixConfig.mOnMesh &&
           aOnMeshPrefixConfig.mSlaac && aOnMeshPrefixConfig.mStable;
}

bool IsValidOmrPrefix(const Ip6::Prefix &aPrefix)
{
    // Accept ULA/GUA prefixes with 64-bit length.
    return (aPrefix.GetLength() == OmrPrefix::kPrefixLength) && !aPrefix.IsLinkLocal() && !aPrefix.IsMulticast();
}

TimeMilli CalculateClampedExpirationTime(TimeMilli aUpdateTime, uint32_t aLifetime)
{
    static constexpr uint32_t kMaxLifetime = Time::MsecToSec(Timer::kMaxDelay);

    return aUpdateTime + Time::SecToMsec(Min(aLifetime, kMaxLifetime));
}

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
