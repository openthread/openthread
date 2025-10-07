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
 *   This file includes common type definitions for Border Router modules.
 */

#ifndef BR_TYPES_HPP_
#define BR_TYPES_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#include <openthread/border_routing.h>

#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/string.hpp"
#include "common/time.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "net/nd6.hpp"
#include "thread/network_data.hpp"

namespace ot {
namespace BorderRouter {

typedef NetworkData::RoutePreference          RoutePreference;     ///< Route preference (high, medium, low).
typedef otBorderRoutingPrefixTableIterator    PrefixTableIterator; ///< Prefix Table Iterator.
typedef otBorderRoutingPrefixTableEntry       PrefixTableEntry;    ///< Prefix Table Entry.
typedef otBorderRoutingRouterEntry            RouterEntry;         ///< Router Entry.
typedef otBorderRoutingRdnssAddrEntry         RdnssAddrEntry;      ///< RDNSS Address Entry.
typedef otBorderRoutingRdnssAddrCallback      RdnssAddrCallback;   ///< RDNS Address changed callback.
typedef otBorderRoutingIfAddrEntry            IfAddrEntry;         ///< Infra-if IPv6 Address Entry.
typedef otBorderRoutingPeerBorderRouterEntry  PeerBrEntry;         ///< Peer Border Router Entry.
typedef otBorderRoutingPrefixTableEntry       Dhcp6PdPrefix;       ///< DHCPv6 PD prefix.
typedef otPdProcessedRaInfo                   Dhcp6PdCounters;     ///< DHCPv6 PD counters.
typedef otBorderRoutingRequestDhcp6PdCallback Dhcp6PdCallback;     ///< DHCPv6 PD callback.
typedef otBorderRoutingMultiAilCallback       MultiAilCallback;    ///< Multi AIL detection callback.

// IPv6 Neighbor Discovery (ND) types
typedef Ip6::Nd::PrefixInfoOption         PrefixInfoOption;         ///< Prefix Info Option (PIO).
typedef Ip6::Nd::RouteInfoOption          RouteInfoOption;          ///< Route Info Option (RIO).
typedef Ip6::Nd::RaFlagsExtOption         RaFlagsExtOption;         ///< RA Flags Extension Option.
typedef Ip6::Nd::RecursiveDnsServerOption RecursiveDnsServerOption; ///< Recursive DNS Server (RDNSS) Option.
typedef Ip6::Nd::RouterAdvert             RouterAdvert;             ///< Router Advertisement (RA).
typedef Ip6::Nd::NeighborAdvertMessage    NeighborAdvertMessage;    ///< Neighbor Advertisement message.
typedef Ip6::Nd::NeighborSolicitHeader    NeighborSolicitHeader;    ///< Neighbor Solicitation (NS) message.
typedef Ip6::Nd::RouterSolicitHeader      RouterSolicitHeader;      ///< Router Solicitation (RS) message.

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * Represents an IPv6 prefix with an associated lifetime.
 *
 * This class serves as a base for other prefix types like `OnLinkPrefix` or `RoutePrefix`.
 */
class LifetimedPrefix
{
public:
    /**
     * Gets the IPv6 prefix.
     *
     * @returns A const reference to the IPv6 prefix.
     */
    const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }

    /**
     * Gets the IPv6 prefix.
     *
     * @returns A reference to the IPv6 prefix.
     */
    Ip6::Prefix &GetPrefix(void) { return mPrefix; }

    /**
     * Gets the time when this prefix was last updated.
     *
     * @returns The last update time.
     */
    const TimeMilli &GetLastUpdateTime(void) const { return mLastUpdateTime; }

    /**
     * Gets the valid lifetime of the prefix.
     *
     * @returns The valid lifetime in seconds.
     */
    uint32_t GetValidLifetime(void) const { return mValidLifetime; }

    /**
     * Gets the expiration time of the prefix.
     *
     * @returns The expiration time.
     */
    TimeMilli GetExpireTime(void) const { return CalculateExpirationTime(mValidLifetime); }

    /**
     * Indicates whether the prefix matches a given IPv6 prefix.
     *
     * @param[in] aPrefix  The IPv6 prefix to match against.
     *
     * @retval TRUE   If the prefix matches @p aPrefix.
     * @retval FALSE  If the prefix does not match @p aPrefix.
     */
    bool Matches(const Ip6::Prefix &aPrefix) const { return (mPrefix == aPrefix); }

    /**
     * Indicates whether the prefix is considered expired by a given `ExpirationChecker`.
     *
     * @param[in] aChecker  An `ExpirationChecker` to use for checking.
     *
     * @retval TRUE   If the prefix is expired.
     * @retval FALSE  If the prefix is not expired.
     */
    bool Matches(const ExpirationChecker &aChecker) const { return aChecker.IsExpired(GetExpireTime()); }

    /**
     * Sets the flag indicating whether the stale time for this prefix has been calculated.
     *
     * @param[in] aFlag  `true` to indicate that stale time has been calculated, `false` otherwise.
     */
    void SetStaleTimeCalculated(bool aFlag) { mStaleTimeCalculated = aFlag; }

    /**
     * Indicates whether the stale time for this prefix has been calculated.
     *
     * @returns `true` if the stale time has been calculated, `false` otherwise.
     */
    bool IsStaleTimeCalculated(void) const { return mStaleTimeCalculated; }

    /**
     * Sets the flag indicating that this prefix entry should be disregarded.
     *
     * @param[in] aFlag  `true` to disregard the prefix, `false` otherwise.
     */
    void SetDisregardFlag(bool aFlag) { mDisregard = aFlag; }

    /**
     * Indicates whether this prefix entry should be disregarded.
     *
     * @returns `true` if the prefix should be disregarded, `false` otherwise.
     */
    bool ShouldDisregard(void) const { return mDisregard; }

protected:
    // The stale time in seconds.
    //
    // The amount of time that can pass after the last time an RA from
    // a particular router has been received advertising an on-link
    // or route prefix before we assume the prefix entry is stale.
    //
    // If multiple routers advertise the same on-link or route prefix,
    // the stale time for the prefix is determined by the latest
    // stale time among all corresponding entries. Stale time
    // expiration triggers tx of Router Solicitation (RS) messages

    static constexpr uint32_t kStaleTime = 600; // 10 minutes.

    LifetimedPrefix(void) = default;

    TimeMilli CalculateExpirationTime(uint32_t aLifetime) const;

    Ip6::Prefix mPrefix;
    bool        mDisregard : 1;
    bool        mStaleTimeCalculated : 1;
    uint32_t    mValidLifetime;
    TimeMilli   mLastUpdateTime;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * Represents an on-link prefix.
 */
class OnLinkPrefix : public LifetimedPrefix, public Clearable<OnLinkPrefix>
{
public:
    /**
     * Sets the on-link prefix information from a Prefix Information Option (PIO).
     *
     * @param[in] aPio  The Prefix Information Option to set from.
     */
    void SetFrom(const PrefixInfoOption &aPio);

    /**
     * Sets the on-link prefix information from a `PrefixTableEntry`.
     *
     * @param[in] aPrefixTableEntry  The `PrefixTableEntry` to set from.
     */
    void SetFrom(const PrefixTableEntry &aPrefixTableEntry);

    /**
     * Gets the preferred lifetime of the prefix.
     *
     * @returns The preferred lifetime in seconds.
     */
    uint32_t GetPreferredLifetime(void) const { return mPreferredLifetime; }

    /**
     * Clears (sets to zero) the preferred lifetime of the prefix.
     */
    void ClearPreferredLifetime(void) { mPreferredLifetime = 0; }

    /**
     * Indicates whether the on-link prefix is deprecated.
     *
     * @retval TRUE   If the prefix is deprecated.
     * @retval FALSE  If the prefix is not deprecated.
     */
    bool IsDeprecated(void) const;

    /**
     * Gets the time when the on-link prefix will be deprecated.
     *
     * @returns The deprecation time.
     */
    TimeMilli GetDeprecationTime(void) const;

    /**
     * Gets the time when the on-link prefix will become stale.
     *
     * @returns The stale time.
     */
    TimeMilli GetStaleTime(void) const;

    /**
     * Adopts flags, valid lifetime, and preferred lifetime from another `OnLinkPrefix` object.
     *
     * @param[in] aPrefix  The `OnLinkPrefix` to adopt information from.
     */
    void AdoptFlagsAndValidAndPreferredLifetimesFrom(const OnLinkPrefix &aPrefix);

    /**
     * Copies the on-link prefix information to a `PrefixTableEntry`.
     *
     * @param[out] aEntry  The `PrefixTableEntry` to copy information to.
     * @param[in]  aNow    The current time.
     */
    void CopyInfoTo(PrefixTableEntry &aEntry, TimeMilli aNow) const;

    /**
     * Indicates whether this on-link prefix is favored over another IPv6 prefix.
     *
     * @param[in] aPrefix  The IPv6 prefix to compare against.
     *
     * @retval TRUE   If this prefix is favored over @p aPrefix.
     * @retval FALSE  If this prefix is not favored over @p aPrefix.
     */
    bool IsFavoredOver(const Ip6::Prefix &aPrefix) const;

private:
    static constexpr uint32_t kFavoredMinPreferredLifetime = 1800; // In sec.
    static constexpr uint8_t  kExpectedFavoredPrefixLength = 64;

    uint32_t mPreferredLifetime;
    bool     mAutoAddrConfigFlag : 1;
    bool     mDhcp6PdPreferredFlag : 1;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * Represents a route prefix.
 */
class RoutePrefix : public LifetimedPrefix, public Clearable<RoutePrefix>
{
public:
    /**
     * Sets the route prefix information from a Route Information Option (RIO).
     *
     * @param[in] aRio  The Route Information Option to set from.
     */
    void SetFrom(const RouteInfoOption &aRio);

    /**
     * Sets the route prefix information from a Router Advertisement (RA) header.
     *
     * @param[in] aRaHeader  The Router Advertisement header to set from.
     */
    void SetFrom(const RouterAdvert::Header &aRaHeader);

    /**
     * Clears (sets to zero) the valid lifetime of the route prefix.
     */
    void ClearValidLifetime(void) { mValidLifetime = 0; }

    /**
     * Gets the time when the route prefix will become stale.
     *
     * @returns The stale time.
     */
    TimeMilli GetStaleTime(void) const;

    /**
     * Gets the route preference of the prefix.
     *
     * @returns The route preference.
     */
    RoutePreference GetRoutePreference(void) const { return mRoutePreference; }

    /**
     * Copies the route prefix information to a `PrefixTableEntry`.
     *
     * @param[out] aEntry  The `PrefixTableEntry` to copy information to.
     * @param[in]  aNow    The current time.
     */
    void CopyInfoTo(PrefixTableEntry &aEntry, TimeMilli aNow) const;

private:
    RoutePreference mRoutePreference;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * Represents an RDNSS (Recursive DNS Server) address.
 */
class RdnssAddress
{
public:
    /**
     * Sets the RDNSS address information from a Recursive DNS Server Option (RDNSS) and an address index.
     *
     * @param[in] aRdnss         The Recursive DNS Server Option to set from.
     * @param[in] aAddressIndex  The index of the address within the RDNSS option.
     */
    void SetFrom(const RecursiveDnsServerOption &aRdnss, uint16_t aAddressIndex);

    /**
     * Gets the IPv6 address of the RDNSS server.
     *
     * @returns A const reference to the IPv6 address.
     */
    const Ip6::Address &GetAddress(void) const { return mAddress; }

    /**
     * Gets the time when this RDNSS address was last updated.
     *
     * @returns The last update time.
     */
    const TimeMilli &GetLastUpdateTime(void) const { return mLastUpdateTime; }

    /**
     * Gets the lifetime of the RDNSS address.
     *
     * @returns The lifetime in seconds.
     */
    uint32_t GetLifetime(void) const { return mLifetime; }

    /**
     * Gets the expiration time of the RDNSS address.
     *
     * @returns The expiration time.
     */
    TimeMilli GetExpireTime(void) const;

    /**
     * Clears (sets to zero) the lifetime of the RDNSS address.
     */
    void ClearLifetime(void) { mLifetime = 0; }

    /**
     * Copies the RDNSS address information to an `RdnssAddrEntry`.
     *
     * @param[out] aEntry  The `RdnssAddrEntry` to copy information to.
     * @param[in]  aNow    The current time.
     */
    void CopyInfoTo(RdnssAddrEntry &aEntry, TimeMilli aNow) const;

    /**
     * Indicates whether this RDNSS address entry matches a given IPv6 address.
     *
     * @param[in] aAddress  The IPv6 address to match against.
     *
     * @retval TRUE   If the RDNSS address matches @p aAddress.
     * @retval FALSE  If the RDNSS address does not match @p aAddress.
     */
    bool Matches(const Ip6::Address &aAddress) const { return (mAddress == aAddress); }

    /**
     * Indicates whether the RDNSS address is considered expired by a given `ExpirationChecker`.
     *
     * @param[in] aChecker  An `ExpirationChecker` to use for checking.
     *
     * @retval TRUE   If the RDNSS address is expired.
     * @retval FALSE  If the RDNSS address is not expired.
     */
    bool Matches(const ExpirationChecker &aChecker) const { return aChecker.IsExpired(GetExpireTime()); }

private:
    Ip6::Address mAddress;
    uint32_t     mLifetime;
    TimeMilli    mLastUpdateTime;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * Represents an interface address used by this BR itself (e.g. for sending RA).
 */
class IfAddress
{
public:
    /**
     * Represents an `InvalidChecker` used to check if an interface address is invalid.
     */
    struct InvalidChecker : public InstanceLocator
    {
        /**
         * Initializes the `InvalidChecker`.
         *
         * @param[in] aInstance  The OpenThread instance.
         */
        explicit InvalidChecker(Instance &aInstance)
            : InstanceLocator(aInstance)
        {
        }
    };

    /**
     * Sets the interface address.
     *
     * @param[in] aAddress    The IPv6 address.
     * @param[in] aUptimeNow  The current uptime (in seconds).
     */
    void SetFrom(const Ip6::Address &aAddress, uint32_t aUptimeNow);

    /**
     * Indicates whether this interface address entry matches a given IPv6 address.
     *
     * @param[in] aAddress  The IPv6 address to match against.
     *
     * @retval TRUE   If the interface address matches @p aAddress.
     * @retval FALSE  If the interface address does not match @p aAddress.
     */
    bool Matches(const Ip6::Address &aAddress) const { return (mAddress == aAddress); }

    /**
     * Indicates whether the interface address is considered invalid by a given `InvalidChecker`.
     *
     * @param[in] aChecker  An `InvalidChecker` to use for checking.
     *
     * @retval TRUE   If the interface address is invalid.
     * @retval FALSE  If the interface address is valid.
     */
    bool Matches(const InvalidChecker &aChecker) const;

    /**
     * Copies the interface address information to an `IfAddrEntry`.
     *
     * @param[out] aEntry      The `IfAddrEntry` to copy information to.
     * @param[in]  aUptimeNow  The current uptime.
     */
    void CopyInfoTo(IfAddrEntry &aEntry, uint32_t aUptimeNow) const;

private:
    Ip6::Address mAddress;
    uint32_t     mLastUseUptime;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * Represents an OMR (Off-Mesh Routable) prefix.
 */
class OmrPrefix : public Clearable<OmrPrefix>, public Equatable<OmrPrefix>
{
public:
    static constexpr uint8_t kPrefixLength = 64;

    /**
     * Constructor for `OmrPrefix`.
     */
    OmrPrefix(void) { Clear(); }

    /**
     * Indicates whether the OMR prefix is empty.
     *
     * @retval TRUE   If the OMR prefix is empty.
     * @retval FALSE  If the OMR prefix is not empty.
     */
    bool IsEmpty(void) const { return (mPrefix.GetLength() == 0); }

    /**
     * Gets the IPv6 prefix.
     *
     * @returns The IPv6 prefix.
     */
    const Ip6::Prefix &GetPrefix(void) const { return mPrefix; }

    /**
     * Gets the preference of the OMR prefix.
     *
     * @returns The preference.
     */
    RoutePreference GetPreference(void) const { return mPreference; }

    /**
     * Indicates whether the OMR prefix is a domain prefix.
     *
     * @retval TRUE   If the OMR prefix is a domain prefix.
     * @retval FALSE  If the OMR prefix is not a domain prefix.
     */
    bool IsDomainPrefix(void) const { return mIsDomainPrefix; }

    /**
     * Sets the OMR prefix and its preference.
     *
     * @param[in] aPrefix      The IPv6 prefix to set.
     * @param[in] aPreference  The preference to set.
     */
    void SetPrefix(const Ip6::Prefix &aPrefix, RoutePreference aPreference);

protected:
    Ip6::Prefix     mPrefix;
    RoutePreference mPreference;
    bool            mIsDomainPrefix;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class FavoredOmrPrefix : public OmrPrefix
{
public:
    /**
     * Indicates whether the favored OMR prefix is derived from the infrastructure.
     *
     * This is identified as a valid OMR prefix with preference of medium or higher.
     *
     * @retval TRUE   If the prefix is infrastructure-derived.
     * @retval FALSE  If the prefix is not infrastructure-derived.
     */
    bool IsInfrastructureDerived(void) const;

    /**
     * Sets the favored OMR prefix from an on-mesh prefix configuration.
     *
     * @param[in] aOnMeshPrefixConfig  The on-mesh prefix configuration to set from.
     */
    void SetFrom(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig);

    /**
     * Sets the favored OMR prefix from an `OmrPrefix` object.
     *
     * @param[in] aOmrPrefix  The `OmrPrefix` object to set from.
     */
    void SetFrom(const OmrPrefix &aOmrPrefix);

    /**
     * Indicates whether this favored OMR prefix is favored over another on-mesh prefix configuration.
     *
     * @param[in] aOmrPrefixConfig  The on-mesh prefix configuration to compare against.
     *
     * @retval TRUE   If this prefix is favored over @p aOmrPrefixConfig.
     * @retval FALSE  If this prefix is not favored over @p aOmrPrefixConfig.
     */
    bool IsFavoredOver(const NetworkData::OnMeshPrefixConfig &aOmrPrefixConfig) const;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper functions

/**
 * Checks whether the on-mesh prefix configuration is a valid OMR prefix.
 *
 * @param[in] aOnMeshPrefixConfig  The on-mesh prefix configuration to check.
 *
 * @retval   TRUE    The prefix is a valid OMR prefix.
 * @retval   FALSE   The prefix is not a valid OMR prefix.
 */
bool IsValidOmrPrefix(const NetworkData::OnMeshPrefixConfig &aOnMeshPrefixConfig);

/**
 * Checks whether a given prefix is a valid OMR prefix.
 *
 * @param[in]  aPrefix  The prefix to check.
 *
 * @retval   TRUE    The prefix is a valid OMR prefix.
 * @retval   FALSE   The prefix is not a valid OMR prefix.
 */
bool IsValidOmrPrefix(const Ip6::Prefix &aPrefix);

/**
 * Calculates the expiration time based on an update time and a lifetime, ensuring it is clamped to fit within the
 * `TimerMilli` range.
 *
 * The @p aLifetime` is provided in seconds. The calculated expiration time is clamped to the maximum interval
 * supported by `Timer` (`2^31` msec or approximately 24.8 days). This clamping ensures that the time calculation
 * fits within the `TimeMilli` range, preventing overflow issues for very long lifetimes.
 *
 * @param[in] aUpdateTime The base time from which the lifetime is calculated.
 * @param[in] aLifetime   The lifetime in seconds.
 *
 * @returns The expiration time.
 */
TimeMilli CalculateClampedExpirationTime(TimeMilli aUpdateTime, uint32_t aLifetime);

} // namespace BorderRouter
} // namespace ot

#endif // OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE

#endif // BR_TYPES_HPP_
