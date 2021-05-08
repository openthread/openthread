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

#include "common/clearable.hpp"
#include "common/equatable.hpp"
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
class PrefixTlv;
class BorderRouterTlv;
class BorderRouterEntry;
class HasRouteTlv;
class HasRouteEntry;
class ServiceTlv;
class ServerTlv;

/**
 * This class represents an On-mesh Prefix (Border Router) configuration.
 *
 */
class OnMeshPrefixConfig : public otBorderRouterConfig,
                           public Clearable<OnMeshPrefixConfig>,
                           public Equatable<OnMeshPrefixConfig>
{
    friend class NetworkData;
    friend class Local;

public:
    /**
     * This method gets the prefix.
     *
     * @return The prefix.
     *
     */
    const Ip6::Prefix &GetPrefix(void) const { return static_cast<const Ip6::Prefix &>(mPrefix); }

    /**
     * This method gets the prefix.
     *
     * @return The prefix.
     *
     */
    Ip6::Prefix &GetPrefix(void) { return static_cast<Ip6::Prefix &>(mPrefix); }

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    /**
     * This method indicates whether or not the prefix configuration is valid.
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
    void SetFrom(const PrefixTlv &        aPrefixTlv,
                 const BorderRouterTlv &  aBorderRouterTlv,
                 const BorderRouterEntry &aBorderRouterEntry);
    void SetFromTlvFlags(uint16_t aFlags);
};

/**
 * This class represents an External Route configuration.
 *
 */
class ExternalRouteConfig : public otExternalRouteConfig,
                            public Clearable<ExternalRouteConfig>,
                            public Equatable<ExternalRouteConfig>
{
    friend class NetworkData;
    friend class Local;

public:
    /**
     * This method gets the prefix.
     *
     * @return The prefix.
     *
     */
    const Ip6::Prefix &GetPrefix(void) const { return static_cast<const Ip6::Prefix &>(mPrefix); }

    /**
     * This method gets the prefix.
     *
     * @return The prefix.
     *
     */
    Ip6::Prefix &GetPrefix(void) { return static_cast<Ip6::Prefix &>(mPrefix); }

    /**
     * This method sets the prefix.
     *
     * @param[in]  aPrefix  The prefix to set to.
     *
     */
    void SetPrefix(const Ip6::Prefix &aPrefix) { mPrefix = aPrefix; }

#if OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
    /**
     * This method indicates whether or not the external route configuration is valid.
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
    void SetFrom(Instance &           aInstance,
                 const PrefixTlv &    aPrefixTlv,
                 const HasRouteTlv &  aHasRouteTlv,
                 const HasRouteEntry &aHasRouteEntry);
    void SetFromTlvFlags(uint8_t aFlags);
};

/**
 * This type represents a Service configuration.
 *
 */
class ServiceConfig : public otServiceConfig, public Clearable<ServiceConfig>, public Unequatable<ServiceConfig>
{
    friend class NetworkData;

public:
    /**
     * This class represents a Server configuration.
     *
     */
    class ServerConfig : public otServerConfig, public Unequatable<ServerConfig>
    {
        friend class ServiceConfig;

    public:
        /**
         * This method overloads operator `==` to evaluate whether or not two `ServerConfig` instances are equal.
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
     * This method gets the Server configuration.
     *
     * @returns The Server configuration.
     *
     */
    const ServerConfig &GetServerConfig(void) const { return static_cast<const ServerConfig &>(mServerConfig); }

    /**
     * This method gets the Server configuration.
     *
     * @returns The Server configuration.
     *
     */
    ServerConfig &GetServerConfig(void) { return static_cast<ServerConfig &>(mServerConfig); }

    /**
     * This method overloads operator `==` to evaluate whether or not two `ServiceConfig` instances are equal.
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

/**
 * @}
 */

} // namespace ot

#endif // NETWORK_DATA_TYPES_HPP_
