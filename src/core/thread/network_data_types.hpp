/*
 *  Copyright (c) 2016-21, The OpenThread Authors.
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
 *   This file includes definitions for Network Data types and constants.
 */

#ifndef NETWORK_DATA_TYPES_HPP_
#define NETWORK_DATA_TYPES_HPP_

#include "openthread-core-config.h"

#include <openthread/netdata.h>

#include "common/as_core_type.hpp"
#include "common/clearable.hpp"
#include "common/data.hpp"
#include "common/debug.hpp"
#include "common/equatable.hpp"
#include "common/preference.hpp"
#include "net/ip6_address.hpp"

namespace ot {

class Instance;

namespace NetworkData {

/**
 * @addtogroup core-netdata-core
 *
 */

// Forward declarations
class NetworkData;
class Local;
class Publisher;
class PrefixTlv;
class BorderRouterTlv;
class BorderRouterEntry;
class HasRouteTlv;
class HasRouteEntry;
class ServiceTlv;
class ServerTlv;
class ContextTlv;

/**
 * Represents the Network Data type.
 *
 */
enum Type : uint8_t
{
    kFullSet,      ///< Full Network Data set.
    kStableSubset, ///< Stable Network Data subset.
};

/**
 * Type represents the route preference values as a signed integer (per RFC-4191).
 *
 */
enum RoutePreference : int8_t
{
    kRoutePreferenceLow    = OT_ROUTE_PREFERENCE_LOW,  ///< Low route preference.
    kRoutePreferenceMedium = OT_ROUTE_PREFERENCE_MED,  ///< Medium route preference.
    kRoutePreferenceHigh   = OT_ROUTE_PREFERENCE_HIGH, ///< High route preference.
};

static_assert(kRoutePreferenceHigh == Preference::kHigh, "kRoutePreferenceHigh is not valid");
static_assert(kRoutePreferenceMedium == Preference::kMedium, "kRoutePreferenceMedium is not valid");
static_assert(kRoutePreferenceLow == Preference::kLow, "kRoutePreferenceLow is not valid");

/**
 * Represents the border router RLOC role filter used when searching for border routers in the Network
 * Data.
 *
 */
enum RoleFilter : uint8_t
{
    kAnyRole,        ///< Include devices in any role.
    kRouterRoleOnly, ///< Include devices that act as Thread router.
    kChildRoleOnly,  ///< Include devices that act as Thread child (end-device).
};

/**
 * Indicates whether a given `int8_t` preference value is a valid route preference (i.e., one of the
 * values from `RoutePreference` enumeration).
 *
 * @param[in] aPref  The signed route preference value.
 *
 * @retval TRUE   if @p aPref is valid.
 * @retval FALSE  if @p aPref is not valid
 *
 */
inline bool IsRoutePreferenceValid(int8_t aPref) { return Preference::IsValid(aPref); }

/**
 * Coverts a route preference to a 2-bit unsigned value.
 *
 * The @p aPref MUST be valid (value from `RoutePreference` enumeration), or the behavior is undefined.
 *
 * @param[in] aPref   The route preference to convert.
 *
 * @returns The 2-bit unsigned value representing @p aPref.
 *
 */
inline uint8_t RoutePreferenceToValue(int8_t aPref) { return Preference::To2BitUint(aPref); }

/**
 * Coverts a 2-bit unsigned value to a route preference.
 *
 * @param[in] aValue   The 2-bit unsigned value to convert from. Note that only the first two bits of @p aValue
 *                     are used and the rest of bits are ignored.
 *
 * @returns The route preference corresponding to @p aValue.
 *
 */
inline RoutePreference RoutePreferenceFromValue(uint8_t aValue)
{
    return static_cast<RoutePreference>(Preference::From2BitUint(aValue));
}

/**
 * Converts a router preference to a human-readable string.
 *
 * @param[in] aPreference  The preference to convert
 *
 * @returns The string representation of @p aPreference.
 *
 */
inline const char *RoutePreferenceToString(RoutePreference aPreference) { return Preference::ToString(aPreference); }

/**
 * Represents an On-mesh Prefix (Border Router) configuration.
 *
 */
class OnMeshPrefixConfig : public otBorderRouterConfig,
                           public Clearable<OnMeshPrefixConfig>,
                           public Equatable<OnMeshPrefixConfig>
{
    friend class NetworkData;
    friend class Leader;
    friend class Local;
    friend class Publisher;

public:
    /**
     * Gets the prefix.
     *
     * @return The prefix.
     *
     */
    const Ip6::Prefix &GetPrefix(void) const { return AsCoreType(&mPrefix); }

    /**
     * Gets the prefix.
     *
     * @return The prefix.
     *
     */
    Ip6::Prefix &GetPrefix(void) { return AsCoreType(&mPrefix); }

    /**
     * Gets the preference.
     *
     * @return The preference.
     *
     */
    RoutePreference GetPreference(void) const { return RoutePreferenceFromValue(RoutePreferenceToValue(mPreference)); }

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    /**
     * Indicates whether or not the prefix configuration is valid.
     *
     * @param[in] aInstance   A reference to the OpenThread instance.
     *
     * @retval TRUE   The config is a valid on-mesh prefix.
     * @retval FALSE  The config is not a valid on-mesh prefix.
     *
     */
    bool IsValid(Instance &aInstance) const;
#endif

private:
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    uint16_t ConvertToTlvFlags(void) const;
#endif
    void SetFrom(const PrefixTlv         &aPrefixTlv,
                 const BorderRouterTlv   &aBorderRouterTlv,
                 const BorderRouterEntry &aBorderRouterEntry);
    void SetFromTlvFlags(uint16_t aFlags);
};

/**
 * Represents an External Route configuration.
 *
 */
class ExternalRouteConfig : public otExternalRouteConfig,
                            public Clearable<ExternalRouteConfig>,
                            public Equatable<ExternalRouteConfig>
{
    friend class NetworkData;
    friend class Local;
    friend class Publisher;

public:
    /**
     * Gets the prefix.
     *
     * @return The prefix.
     *
     */
    const Ip6::Prefix &GetPrefix(void) const { return AsCoreType(&mPrefix); }

    /**
     * Gets the prefix.
     *
     * @return The prefix.
     *
     */
    Ip6::Prefix &GetPrefix(void) { return AsCoreType(&mPrefix); }

    /**
     * Sets the prefix.
     *
     * @param[in]  aPrefix  The prefix to set to.
     *
     */
    void SetPrefix(const Ip6::Prefix &aPrefix) { mPrefix = aPrefix; }

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    /**
     * Indicates whether or not the external route configuration is valid.
     *
     * @param[in] aInstance   A reference to the OpenThread instance.
     *
     * @retval TRUE   The config is a valid external route.
     * @retval FALSE  The config is not a valid extern route.
     *
     */
    bool IsValid(Instance &aInstance) const;
#endif

private:
#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    uint8_t ConvertToTlvFlags(void) const;
#endif
    void SetFrom(Instance            &aInstance,
                 const PrefixTlv     &aPrefixTlv,
                 const HasRouteTlv   &aHasRouteTlv,
                 const HasRouteEntry &aHasRouteEntry);
    void SetFromTlvFlags(uint8_t aFlags);
};

/**
 * Represents 6LoWPAN Context ID information associated with a prefix in Network Data.
 *
 */
class LowpanContextInfo : public otLowpanContextInfo, public Clearable<LowpanContextInfo>
{
    friend class NetworkData;

public:
    /**
     * Gets the prefix.
     *
     * @return The prefix.
     *
     */
    const Ip6::Prefix &GetPrefix(void) const { return AsCoreType(&mPrefix); }

private:
    Ip6::Prefix &GetPrefix(void) { return AsCoreType(&mPrefix); }
    void         SetFrom(const PrefixTlv &aPrefixTlv, const ContextTlv &aContextTlv);
};

/**
 * Represents a Service Data.
 *
 */
class ServiceData : public Data<kWithUint8Length>
{
};

/**
 * Represents a Server Data.
 *
 */
class ServerData : public Data<kWithUint8Length>
{
};

/**
 * Represents a Service configuration.
 *
 */
class ServiceConfig : public otServiceConfig, public Clearable<ServiceConfig>, public Unequatable<ServiceConfig>
{
    friend class NetworkData;

public:
    /**
     * Represents a Server configuration.
     *
     */
    class ServerConfig : public otServerConfig, public Unequatable<ServerConfig>
    {
        friend class ServiceConfig;

    public:
        /**
         * Gets the Server Data.
         *
         * @param[out] aServerData   A reference to a`ServerData` to return the data.
         *
         */
        void GetServerData(ServerData &aServerData) const { aServerData.Init(mServerData, mServerDataLength); }

        /**
         * Overloads operator `==` to evaluate whether or not two `ServerConfig` instances are equal.
         *
         * @param[in]  aOther  The other `ServerConfig` instance to compare with.
         *
         * @retval TRUE   If the two `ServerConfig` instances are equal.
         * @retval FALSE  If the two `ServerConfig` instances are not equal.
         *
         */
        bool operator==(const ServerConfig &aOther) const;

    private:
        void SetFrom(const ServerTlv &aServerTlv);
    };

    /**
     * Gets the Service Data.
     *
     * @param[out] aServiceData   A reference to a `ServiceData` to return the data.
     *
     */
    void GetServiceData(ServiceData &aServiceData) const { aServiceData.Init(mServiceData, mServiceDataLength); }

    /**
     * Gets the Server configuration.
     *
     * @returns The Server configuration.
     *
     */
    const ServerConfig &GetServerConfig(void) const { return static_cast<const ServerConfig &>(mServerConfig); }

    /**
     * Gets the Server configuration.
     *
     * @returns The Server configuration.
     *
     */
    ServerConfig &GetServerConfig(void) { return static_cast<ServerConfig &>(mServerConfig); }

    /**
     * Overloads operator `==` to evaluate whether or not two `ServiceConfig` instances are equal.
     *
     * @param[in]  aOther  The other `ServiceConfig` instance to compare with.
     *
     * @retval TRUE   If the two `ServiceConfig` instances are equal.
     * @retval FALSE  If the two `ServiceConfig` instances are not equal.
     *
     */
    bool operator==(const ServiceConfig &aOther) const;

private:
    void SetFrom(const ServiceTlv &aServiceTlv, const ServerTlv &aServerTlv);
};

} // namespace NetworkData

DefineCoreType(otBorderRouterConfig, NetworkData::OnMeshPrefixConfig);
DefineCoreType(otExternalRouteConfig, NetworkData::ExternalRouteConfig);
DefineCoreType(otLowpanContextInfo, NetworkData::LowpanContextInfo);
DefineCoreType(otServiceConfig, NetworkData::ServiceConfig);
DefineCoreType(otServerConfig, NetworkData::ServiceConfig::ServerConfig);

/**
 * @}
 */

} // namespace ot

#endif // NETWORK_DATA_TYPES_HPP_
