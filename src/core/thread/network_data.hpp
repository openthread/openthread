/*
 *  Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for managing Thread Network Data.
 */

#ifndef NETWORK_DATA_HPP_
#define NETWORK_DATA_HPP_

#include "openthread-core-config.h"

#include <openthread/border_router.h>
#include <openthread/server.h>

#include "coap/coap.hpp"
#include "common/clearable.hpp"
#include "common/equatable.hpp"
#include "common/locator.hpp"
#include "common/timer.hpp"
#include "net/udp6.hpp"
#include "thread/lowpan.hpp"
#include "thread/mle_router.hpp"
#include "thread/network_data_tlvs.hpp"

namespace ot {

/**
 * @addtogroup core-netdata
 * @brief
 *   This module includes definitions for generating, processing, and managing Thread Network Data.
 *
 * @{
 *
 * @defgroup core-netdata-core Core
 * @defgroup core-netdata-leader Leader
 * @defgroup core-netdata-local Local
 * @defgroup core-netdata-tlvs TLVs
 *
 * @}
 *
 */

/**
 * @namespace ot::NetworkData
 *
 * @brief
 *   This namespace includes definitions for managing Thread Network Data.
 *
 */
namespace NetworkData {

/**
 * @addtogroup core-netdata-core
 *
 * @brief
 *   This module includes definitions for managing Thread Network Data.
 *
 * @{
 *
 */

enum
{
    kIteratorInit = OT_NETWORK_DATA_ITERATOR_INIT, ///< Initializer for `Iterator` type.
};

/**
 * This type represents a Iterator used to iterate through Network Data info (e.g., see `GetNextOnMeshPrefix()`)
 *
 */
typedef otNetworkDataIterator Iterator;

/**
 * This class represents an On Mesh Prefix (Border Router) configuration.
 *
 */
class OnMeshPrefixConfig : public otBorderRouterConfig,
                           public Clearable<OnMeshPrefixConfig>,
                           public Equatable<OnMeshPrefixConfig>
{
    friend class NetworkData;

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

private:
    void SetFrom(const PrefixTlv &        aPrefixTlv,
                 const BorderRouterTlv &  aBorderRouterTlv,
                 const BorderRouterEntry &aBorderRouterEntry);
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

private:
    void SetFrom(Instance &           aInstance,
                 const PrefixTlv &    aPrefixTlv,
                 const HasRouteTlv &  aHasRouteTlv,
                 const HasRouteEntry &aEntry);
};

/**
 * This type represents a Service configuration.
 *
 */
class ServiceConfig : public otServiceConfig, public Clearable<ServiceConfig>
{
    friend class NetworkData;

public:
    /**
     * This class represents a Server configuration.
     *
     */
    class ServerConfig : public otServerConfig
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

        /**
         * This method overloads operator `!=` to evaluate whether or not two `ServerConfig` instances are unequal.
         *
         * @param[in]  aOther  The other `ServerConfig` instance to compare with.
         *
         * @retval TRUE   If the two `ServerConfig` instances are unequal.
         * @retval FALSE  If the two `ServerConfig` instances are not unequal.
         *
         */
        bool operator!=(const ServerConfig &aOther) const { return !(*this == aOther); }

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

    /**
     * This method overloads operator `!=` to evaluate whether or not two `ServiceConfig` instances are unequal.
     *
     * @param[in]  aOther  The other `ServiceConfig` instance to compare with.
     *
     * @retval TRUE   If the two `ServiceConfig` instances are unequal.
     * @retval FALSE  If the two `ServiceConfig` instances are not unequal.
     *
     */
    bool operator!=(const ServiceConfig &aOther) const { return !(*this == aOther); }

private:
    void SetFrom(const ServiceTlv &aServiceTlv, const ServerTlv &aServerTlv);
};

/**
 * This class implements Network Data processing.
 *
 */
class NetworkData : public InstanceLocator
{
public:
    enum
    {
        kMaxSize = 254, ///< Maximum size of Thread Network Data in bytes.
    };

    /**
     * This enumeration specifies the type of Network Data (local or leader).
     *
     */
    enum Type
    {
        kTypeLocal,  ///< Local Network Data.
        kTypeLeader, ///< Leader Network Data.
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     * @param[in]  aType         Network data type
     *
     */
    NetworkData(Instance &aInstance, Type aType);

    /**
     * This method clears the network data.
     *
     */
    void Clear(void);

    /**
     * This method provides a full or stable copy of the Thread Network Data.
     *
     * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
     * @param[out]    aData        A pointer to the data buffer.
     * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
     *                             On exit, number of copied bytes.
     *
     * @retval OT_ERROR_NONE       Successfully copied full Thread Network Data.
     * @retval OT_ERROR_NO_BUFS    Not enough space to fully copy Thread Network Data.
     *
     */
    otError GetNetworkData(bool aStable, uint8_t *aData, uint8_t &aDataLength) const;

    /**
     * This method provides the next On Mesh prefix in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the On Mesh Prefix information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next On Mesh prefix.
     * @retval OT_ERROR_NOT_FOUND  No subsequent On Mesh prefix exists in the Thread Network Data.
     *
     */
    otError GetNextOnMeshPrefix(Iterator &aIterator, OnMeshPrefixConfig &aConfig) const;

    /**
     * This method provides the next On Mesh prefix in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the On Mesh Prefix information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next On Mesh prefix.
     * @retval OT_ERROR_NOT_FOUND  No subsequent On Mesh prefix exists in the Thread Network Data.
     *
     */
    otError GetNextOnMeshPrefix(Iterator &aIterator, uint16_t aRloc16, OnMeshPrefixConfig &aConfig) const;

    /**
     * This method provides the next external route in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the external route information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next external route.
     * @retval OT_ERROR_NOT_FOUND  No subsequent external route exists in the Thread Network Data.
     *
     */
    otError GetNextExternalRoute(Iterator &aIterator, ExternalRouteConfig &aConfig) const;

    /**
     * This method provides the next external route in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the external route information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next external route.
     * @retval OT_ERROR_NOT_FOUND  No subsequent external route exists in the Thread Network Data.
     *
     */
    otError GetNextExternalRoute(Iterator &aIterator, uint16_t aRloc16, ExternalRouteConfig &aConfig) const;

    /**
     * This method provides the next service in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aConfig    A reference to a config variable where the service information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the Thread Network Data.
     *
     */
    otError GetNextService(Iterator &aIterator, ServiceConfig &aConfig) const;

    /**
     * This method provides the next service in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aConfig    A reference to a config variable where the service information will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the Thread Network Data.
     *
     */
    otError GetNextService(Iterator &aIterator, uint16_t aRloc16, ServiceConfig &aConfig) const;

    /**
     * This method provides the next Service ID in the Thread Network Data for a given RLOC16.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[in]     aRloc16    The RLOC16 value.
     * @param[out]    aServiceId A reference to variable where the Service ID will be placed.
     *
     * @retval OT_ERROR_NONE       Successfully found the next service.
     * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the Thread Network Data.
     *
     */
    otError GetNextServiceId(Iterator &aIterator, uint16_t aRloc16, uint8_t &aServiceId) const;

    /**
     * This method indicates whether or not the Thread Network Data contains all of the on mesh prefix information
     * in @p aCompare associated with @p aRloc16.
     *
     * @param[in]  aCompare  The Network Data to use for the query.
     * @param[in]  aRloc16   The RLOC16 to consider.
     *
     * @returns TRUE if this object contains all on mesh prefix information in @p aCompare associated with @p aRloc16,
     *          FALSE otherwise.
     *
     */
    bool ContainsOnMeshPrefixes(const NetworkData &aCompare, uint16_t aRloc16) const;

    /**
     * This method indicates whether or not the Thread Network Data contains all of the external route information
     * in @p aCompare associated with @p aRloc16.
     *
     * @param[in]  aCompare  The Network Data to use for the query.
     * @param[in]  aRloc16   The RLOC16 to consider.
     *
     * @returns TRUE if this object contains all external route information in @p aCompare associated with @p aRloc16,
     *          FALSE otherwise.
     *
     */
    bool ContainsExternalRoutes(const NetworkData &aCompare, uint16_t aRloc16) const;

    /**
     * This method indicates whether or not the Thread Network Data contains all of the service information
     * in @p aCompare associated with @p aRloc16.
     *
     * @param[in]  aCompare  The Network Data to use for the query.
     * @param[in]  aRloc16   The RLOC16 to consider.
     *
     * @returns TRUE if this object contains all service information in @p aCompare associated with @p aRloc16,
     *          FALSE otherwise.
     *
     */
    bool ContainsServices(const NetworkData &aCompare, uint16_t aRloc16) const;

    /**
     * This method indicates whether or not the Thread Network Data contains the service with given Service ID
     * associated with @p aRloc16.
     *
     * @param[in]  aServiceId The Service ID to search for.
     * @param[in]  aRloc16    The RLOC16 to consider.
     *
     * @returns TRUE if this object contains the service with given ID associated with @p aRloc16,
     *          FALSE otherwise.
     *
     */
    bool ContainsService(uint8_t aServiceId, uint16_t aRloc16) const;

    /**
     * This method provides the next server RLOC16 in the Thread Network Data.
     *
     * @param[inout]  aIterator  A reference to the Network Data iterator.
     * @param[out]    aRloc16    The RLOC16 value.
     *
     * @retval OT_ERROR_NONE       Successfully found the next server.
     * @retval OT_ERROR_NOT_FOUND  No subsequent server exists in the Thread Network Data.
     *
     */
    otError GetNextServer(Iterator &aIterator, uint16_t &aRloc16) const;

protected:
    /**
     * This method returns a pointer to the start of Network Data TLV sequence.
     *
     * @returns A pointer to the start of Network Data TLV sequence.
     *
     */
    NetworkDataTlv *GetTlvsStart(void) { return reinterpret_cast<NetworkDataTlv *>(mTlvs); }

    /**
     * This method returns a pointer to the start of Network Data TLV sequence.
     *
     * @returns A pointer to the start of Network Data TLV sequence.
     *
     */
    const NetworkDataTlv *GetTlvsStart(void) const { return reinterpret_cast<const NetworkDataTlv *>(mTlvs); }

    /**
     * This method returns a pointer to the end of Network Data TLV sequence.
     *
     * @returns A pointer to the end of Network Data TLV sequence.
     *
     */
    NetworkDataTlv *GetTlvsEnd(void) { return reinterpret_cast<NetworkDataTlv *>(mTlvs + mLength); }

    /**
     * This method returns a pointer to the end of Network Data TLV sequence.
     *
     * @returns A pointer to the end of Network Data TLV sequence.
     *
     */
    const NetworkDataTlv *GetTlvsEnd(void) const { return reinterpret_cast<const NetworkDataTlv *>(mTlvs + mLength); }

    /**
     * This method returns a pointer to the Border Router TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Border Router TLV if one is found or nullptr if no Border Router TLV exists.
     *
     */
    static BorderRouterTlv *FindBorderRouter(PrefixTlv &aPrefix)
    {
        return const_cast<BorderRouterTlv *>(FindBorderRouter(const_cast<const PrefixTlv &>(aPrefix)));
    }

    /**
     * This method returns a pointer to the Border Router TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Border Router TLV if one is found or nullptr if no Border Router TLV exists.
     *
     */
    static const BorderRouterTlv *FindBorderRouter(const PrefixTlv &aPrefix);

    /**
     * This method returns a pointer to the stable or non-stable Border Router TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     * @param[in]  aStable  TRUE to find a stable TLV, FALSE to find a TLV not marked as stable..
     *
     * @returns A pointer to the Border Router TLV if one is found or nullptr if no Border Router TLV exists.
     *
     */
    static BorderRouterTlv *FindBorderRouter(PrefixTlv &aPrefix, bool aStable)
    {
        return const_cast<BorderRouterTlv *>(FindBorderRouter(const_cast<const PrefixTlv &>(aPrefix), aStable));
    }

    /**
     * This method returns a pointer to the stable or non-stable Border Router TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     * @param[in]  aStable  TRUE to find a stable TLV, FALSE to find a TLV not marked as stable..
     *
     * @returns A pointer to the Border Router TLV if one is found or nullptr if no Border Router TLV exists.
     *
     */
    static const BorderRouterTlv *FindBorderRouter(const PrefixTlv &aPrefix, bool aStable);

    /**
     * This method returns a pointer to the Has Route TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Has Route TLV if one is found or nullptr if no Has Route TLV exists.
     *
     */
    static HasRouteTlv *FindHasRoute(PrefixTlv &aPrefix)
    {
        return const_cast<HasRouteTlv *>(FindHasRoute(const_cast<const PrefixTlv &>(aPrefix)));
    }

    /**
     * This method returns a pointer to the Has Route TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Has Route TLV if one is found or nullptr if no Has Route TLV exists.
     *
     */
    static const HasRouteTlv *FindHasRoute(const PrefixTlv &aPrefix);

    /**
     * This method returns a pointer to the stable or non-stable Has Route TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     * @param[in]  aStable  TRUE to find a stable TLV, FALSE to find a TLV not marked as stable.
     *
     * @returns A pointer to the Has Route TLV if one is found or nullptr if no Has Route TLV exists.
     *
     */
    static HasRouteTlv *FindHasRoute(PrefixTlv &aPrefix, bool aStable)
    {
        return const_cast<HasRouteTlv *>(FindHasRoute(const_cast<const PrefixTlv &>(aPrefix), aStable));
    }

    /**
     * This method returns a pointer to the stable or non-stable Has Route TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     * @param[in]  aStable  TRUE to find a stable TLV, FALSE to find a TLV not marked as stable.
     *
     * @returns A pointer to the Has Route TLV if one is found or nullptr if no Has Route TLV exists.
     *
     */
    static const HasRouteTlv *FindHasRoute(const PrefixTlv &aPrefix, bool aStable);

    /**
     * This method returns a pointer to the Context TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Context TLV if one is found or nullptr if no Context TLV exists.
     *
     */
    static ContextTlv *FindContext(PrefixTlv &aPrefix)
    {
        return const_cast<ContextTlv *>(FindContext(const_cast<const PrefixTlv &>(aPrefix)));
    }

    /**
     * This method returns a pointer to the Context TLV within a given Prefix TLV.
     *
     * @param[in]  aPrefix  A reference to the Prefix TLV.
     *
     * @returns A pointer to the Context TLV if one is found or nullptr if no Context TLV exists.
     *
     */
    static const ContextTlv *FindContext(const PrefixTlv &aPrefix);

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength)
    {
        return const_cast<PrefixTlv *>(const_cast<const NetworkData *>(this)->FindPrefix(aPrefix, aPrefixLength));
    }

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    const PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength) const;

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        An IPv6 prefix.
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    PrefixTlv *FindPrefix(const Ip6::Prefix &aPrefix) { return FindPrefix(aPrefix.GetBytes(), aPrefix.GetLength()); }

    /**
     * This method returns a pointer to a Prefix TLV.
     *
     * @param[in]  aPrefix        An IPv6 prefix.
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    const PrefixTlv *FindPrefix(const Ip6::Prefix &aPrefix) const
    {
        return FindPrefix(aPrefix.GetBytes(), aPrefix.GetLength());
    }

    /**
     * This method returns a pointer to a Prefix TLV in a specified tlvs buffer.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     * @param[in]  aTlvs          A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength    The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    static PrefixTlv *FindPrefix(const uint8_t *aPrefix, uint8_t aPrefixLength, uint8_t *aTlvs, uint8_t aTlvsLength)
    {
        return const_cast<PrefixTlv *>(
            FindPrefix(aPrefix, aPrefixLength, const_cast<const uint8_t *>(aTlvs), aTlvsLength));
    }

    /**
     * This method returns a pointer to a Prefix TLV in a specified tlvs buffer.
     *
     * @param[in]  aPrefix        A pointer to an IPv6 prefix.
     * @param[in]  aPrefixLength  The prefix length pointed to by @p aPrefix (in bits).
     * @param[in]  aTlvs          A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength    The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Prefix TLV if one is found or nullptr if no matching Prefix TLV exists.
     *
     */
    static const PrefixTlv *FindPrefix(const uint8_t *aPrefix,
                                       uint8_t        aPrefixLength,
                                       const uint8_t *aTlvs,
                                       uint8_t        aTlvsLength);

    /**
     * This method returns a pointer to a matching Service TLV.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to a Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     *
     * @returns A pointer to the Service TLV if one is found or nullptr if no matching Service TLV exists.
     *
     */
    ServiceTlv *FindService(uint32_t aEnterpriseNumber, const uint8_t *aServiceData, uint8_t aServiceDataLength)
    {
        return const_cast<ServiceTlv *>(
            const_cast<const NetworkData *>(this)->FindService(aEnterpriseNumber, aServiceData, aServiceDataLength));
    }

    /**
     * This method returns a pointer to a matching Service TLV.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to a Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     *
     * @returns A pointer to the Service TLV if one is found or nullptr if no matching Service TLV exists.
     *
     */
    const ServiceTlv *FindService(uint32_t       aEnterpriseNumber,
                                  const uint8_t *aServiceData,
                                  uint8_t        aServiceDataLength) const;

    /**
     * This method returns a pointer to a Service TLV in a specified tlvs buffer.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to an Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     * @param[in]  aTlvs              A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength        The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Service TLV if one is found or nullptr if no matching Service TLV exists.
     *
     */
    static ServiceTlv *FindService(uint32_t       aEnterpriseNumber,
                                   const uint8_t *aServiceData,
                                   uint8_t        aServiceDataLength,
                                   uint8_t *      aTlvs,
                                   uint8_t        aTlvsLength)
    {
        return const_cast<ServiceTlv *>(FindService(aEnterpriseNumber, aServiceData, aServiceDataLength,
                                                    const_cast<const uint8_t *>(aTlvs), aTlvsLength));
    }

    /**
     * This method returns a pointer to a Service TLV in a specified tlvs buffer.
     *
     * @param[in]  aEnterpriseNumber  Enterprise Number.
     * @param[in]  aServiceData       A pointer to an Service Data.
     * @param[in]  aServiceDataLength The Service Data length pointed to by @p aServiceData.
     * @param[in]  aTlvs              A pointer to a specified tlvs buffer.
     * @param[in]  aTlvsLength        The specified tlvs buffer length pointed to by @p aTlvs.
     *
     * @returns A pointer to the Service TLV if one is found or nullptr if no matching Service TLV exists.
     *
     */
    static const ServiceTlv *FindService(uint32_t       aEnterpriseNumber,
                                         const uint8_t *aServiceData,
                                         uint8_t        aServiceDataLength,
                                         const uint8_t *aTlvs,
                                         uint8_t        aTlvsLength);

    /**
     * This method indicates whether there is space in Network Data to insert/append new info and grow it by a given
     * number of bytes.
     *
     * @param[in]  aSize  The number of bytes to grow the Network Data.
     *
     * @retval TRUE   There is space to grow Network Data by @p aSize bytes.
     * @retval FALSE  There is no space left to grow Network Data by @p aSize bytes.
     *
     */
    bool CanInsert(uint16_t aSize) const { return (mLength + aSize <= kMaxSize); }

    /**
     * This method grows the Network Data to append a TLV with a requested size.
     *
     * On success, the returned TLV is not initialized (i.e., the TLV Length field is not set) but the requested
     * size for it (@p aTlvSize number of bytes) is reserved in the Network Data.
     *
     * @param[in]  aTlvSize  The size of TLV (total number of bytes including Type, Length, and Value fields)
     *
     * @returns A pointer to the TLV if there is space to grow Network Data, or nullptr if no space to grow the Network
     *          Data with requested @p aTlvSize number of bytes.
     *
     */
    NetworkDataTlv *AppendTlv(uint16_t aTlvSize);

    /**
     * This method inserts bytes into the Network Data.
     *
     * @param[in]  aStart   A pointer to the beginning of the insertion.
     * @param[in]  aLength  The number of bytes to insert.
     *
     */
    void Insert(void *aStart, uint8_t aLength);

    /**
     * This method removes bytes from the Network Data.
     *
     * @param[in]  aRemoveStart   A pointer to the beginning of the removal.
     * @param[in]  aRemoveLength  The number of bytes to remove.
     *
     */
    void Remove(void *aRemoveStart, uint8_t aRemoveLength);

    /**
     * This method removes a TLV from the Network Data.
     *
     * @param[in]  aTlv   The TLV to remove.
     *
     */
    void RemoveTlv(NetworkDataTlv *aTlv);

    /**
     * This method strips non-stable data from the Thread Network Data.
     *
     * @param[inout]  aData        A pointer to the Network Data to modify.
     * @param[inout]  aDataLength  On entry, the size of the Network Data in bytes.  On exit, the size of the
     *                             resulting Network Data in bytes.
     *
     */
    static void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength);

    /**
     * This method sends a Server Data Notification message to the Leader.
     *
     * @param[in]  aRloc16   The old RLOC16 value that was previously registered.
     * @param[in]  aHandler  A function pointer that is called when the transaction ends.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE     Successfully enqueued the notification message.
     * @retval OT_ERROR_NO_BUFS  Insufficient message buffers to generate the notification message.
     *
     */
    otError SendServerDataNotification(uint16_t aRloc16, Coap::ResponseHandler aHandler, void *aContext);

    /**
     * This static method searches in a given sequence of TLVs to find the first TLV with a given TLV Type.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param[in]  aEnd    A pointer to the end of the sequence of TLVs.
     * @param[in]  aType   The TLV type to find.
     *
     * @returns A pointer to the TLV if found, or nullptr if not found.
     *
     */
    static NetworkDataTlv *FindTlv(NetworkDataTlv *aStart, NetworkDataTlv *aEnd, NetworkDataTlv::Type aType)
    {
        return const_cast<NetworkDataTlv *>(
            FindTlv(const_cast<const NetworkDataTlv *>(aStart), const_cast<const NetworkDataTlv *>(aEnd), aType));
    }

    /**
     * This static method searches in a given sequence of TLVs to find the first TLV with a given TLV Type.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param[in]  aEnd    A pointer to the end of the sequence of TLVs.
     * @param[in]  aType   The TLV type to find.
     *
     * @returns A pointer to the TLV if found, or nullptr if not found.
     *
     */
    static const NetworkDataTlv *FindTlv(const NetworkDataTlv *aStart,
                                         const NetworkDataTlv *aEnd,
                                         NetworkDataTlv::Type  aType);

    /**
     * This static template method searches in a given sequence of TLVs to find the first TLV with a give template
     * `TlvType`.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param[in]  aEnd    A pointer to the end of the sequence of TLVs.
     *
     * @returns A pointer to the TLV if found, or nullptr if not found.
     *
     */
    template <typename TlvType> static TlvType *FindTlv(NetworkDataTlv *aStart, NetworkDataTlv *aEnd)
    {
        return static_cast<TlvType *>(FindTlv(aStart, aEnd, static_cast<NetworkDataTlv::Type>(TlvType::kType)));
    }

    /**
     * This static template method searches in a given sequence of TLVs to find the first TLV with a give template
     * `TlvType`.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param[in]  aEnd    A pointer to the end of the sequence of TLVs.
     *
     * @returns A pointer to the TLV if found, or nullptr if not found.
     *
     */
    template <typename TlvType> static const TlvType *FindTlv(const NetworkDataTlv *aStart, const NetworkDataTlv *aEnd)
    {
        return static_cast<const TlvType *>(FindTlv(aStart, aEnd, static_cast<NetworkDataTlv::Type>(TlvType::kType)));
    }

    /**
     * This static method searches in a given sequence of TLVs to find the first TLV with a given TLV Type and stable
     * flag.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param [in] aEnd    A pointer to the end of the sequence of TLVs.
     * @param[in]  aType   The TLV type to find.
     * @param[in]  aStable TRUE to find a stable TLV, FALSE to find a TLV not marked as stable.
     *
     * @returns A pointer to the TLV if found, or nullptr if not found.
     *
     */
    static NetworkDataTlv *FindTlv(NetworkDataTlv *     aStart,
                                   NetworkDataTlv *     aEnd,
                                   NetworkDataTlv::Type aType,
                                   bool                 aStable)
    {
        return const_cast<NetworkDataTlv *>(FindTlv(const_cast<const NetworkDataTlv *>(aStart),
                                                    const_cast<const NetworkDataTlv *>(aEnd), aType, aStable));
    }

    /**
     * This static method searches in a given sequence of TLVs to find the first TLV with a given TLV Type and stable
     * flag.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param [in] aEnd    A pointer to the end of the sequence of TLVs.
     * @param[in]  aType   The TLV type to find.
     * @param[in]  aStable TRUE to find a stable TLV, FALSE to find a TLV not marked as stable.
     *
     * @returns A pointer to the TLV if found, or nullptr if not found.
     *
     */
    static const NetworkDataTlv *FindTlv(const NetworkDataTlv *aStart,
                                         const NetworkDataTlv *aEnd,
                                         NetworkDataTlv::Type  aType,
                                         bool                  aStable);

    /**
     * This template static method searches in a given sequence of TLVs to find the first TLV with a given TLV Type and
     * stable flag.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param [in] aEnd    A pointer to the end of the sequence of TLVs.
     * @param[in]  aStable TRUE to find a stable TLV, FALSE to find a TLV not marked as stable.
     *
     * @returns A pointer to the TLV if found, or nullptr if not found.
     *
     */
    template <typename TlvType> static TlvType *FindTlv(NetworkDataTlv *aStart, NetworkDataTlv *aEnd, bool aStable)
    {
        return static_cast<TlvType *>(
            FindTlv(aStart, aEnd, static_cast<NetworkDataTlv::Type>(TlvType::kType), aStable));
    }

    /**
     * This template static method searches in a given sequence of TLVs to find the first TLV with a given TLV Type and
     * stable flag.
     *
     * @param[in]  aStart  A pointer to the start of the sequence of TLVs to search within.
     * @param [in] aEnd    A pointer to the end of the sequence of TLVs.
     * @param[in]  aStable TRUE to find a stable TLV, FALSE to find a TLV not marked as stable.
     *
     * @returns A pointer to the TLV if found, or nullptr if not found.
     *
     */
    template <typename TlvType>
    static const TlvType *FindTlv(const NetworkDataTlv *aStart, const NetworkDataTlv *aEnd, bool aStable)
    {
        return static_cast<const TlvType *>(
            FindTlv(aStart, aEnd, static_cast<NetworkDataTlv::Type>(TlvType::kType), aStable));
    }

    uint8_t mTlvs[kMaxSize]; ///< The Network Data buffer.
    uint8_t mLength;         ///< The number of valid bytes in @var mTlvs.

private:
    enum
    {
        kDataResubmitDelay  = 300000, ///< DATA_RESUBMIT_DELAY (milliseconds) if the device itself is the server.
        kProxyResubmitDelay = 5000,   ///< Resubmit delay (milliseconds) if deregister as the child server proxy.

    };

    class NetworkDataIterator
    {
    public:
        explicit NetworkDataIterator(Iterator &aIterator)
            : mIteratorBuffer(reinterpret_cast<uint8_t *>(&aIterator))
        {
        }

        const NetworkDataTlv *GetTlv(const uint8_t *aTlvs) const
        {
            return reinterpret_cast<const NetworkDataTlv *>(aTlvs + GetTlvOffset());
        }

        void AdvanceTlv(const uint8_t *aTlvs)
        {
            SaveTlvOffset(GetTlv(aTlvs)->GetNext(), aTlvs);
            SetSubTlvOffset(0);
            SetEntryIndex(0);
        }

        const NetworkDataTlv *GetSubTlv(const NetworkDataTlv *aSubTlvs) const
        {
            return reinterpret_cast<const NetworkDataTlv *>(reinterpret_cast<const uint8_t *>(aSubTlvs) +
                                                            GetSubTlvOffset());
        }

        void AdvaceSubTlv(const NetworkDataTlv *aSubTlvs)
        {
            SaveSubTlvOffset(GetSubTlv(aSubTlvs)->GetNext(), aSubTlvs);
            SetEntryIndex(0);
        }

        uint8_t GetAndAdvanceIndex(void) { return mIteratorBuffer[kEntryPosition]++; }

        bool IsNewEntry(void) const { return GetEntryIndex() == 0; }
        void MarkEntryAsNotNew(void) { SetEntryIndex(1); }

    private:
        enum
        {
            kTlvPosition    = 0,
            kSubTlvPosition = 1,
            kEntryPosition  = 2,
        };

        uint8_t GetTlvOffset(void) const { return mIteratorBuffer[kTlvPosition]; }
        uint8_t GetSubTlvOffset(void) const { return mIteratorBuffer[kSubTlvPosition]; }
        void    SetSubTlvOffset(uint8_t aOffset) { mIteratorBuffer[kSubTlvPosition] = aOffset; }
        void    SetTlvOffset(uint8_t aOffset) { mIteratorBuffer[kTlvPosition] = aOffset; }
        uint8_t GetEntryIndex(void) const { return mIteratorBuffer[kEntryPosition]; }
        void    SetEntryIndex(uint8_t aIndex) { mIteratorBuffer[kEntryPosition] = aIndex; }

        void SaveTlvOffset(const NetworkDataTlv *aTlv, const uint8_t *aTlvs)
        {
            SetTlvOffset(static_cast<uint8_t>(reinterpret_cast<const uint8_t *>(aTlv) - aTlvs));
        }

        void SaveSubTlvOffset(const NetworkDataTlv *aSubTlv, const NetworkDataTlv *aSubTlvs)
        {
            SetSubTlvOffset(static_cast<uint8_t>(reinterpret_cast<const uint8_t *>(aSubTlv) -
                                                 reinterpret_cast<const uint8_t *>(aSubTlvs)));
        }

        uint8_t *mIteratorBuffer;
    };

    struct Config
    {
        OnMeshPrefixConfig * mOnMeshPrefix;
        ExternalRouteConfig *mExternalRoute;
        ServiceConfig *      mService;
    };

    otError Iterate(Iterator &aIterator, uint16_t aRloc16, Config &aConfig) const;

    static void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, PrefixTlv &aPrefix);
    static void RemoveTemporaryData(uint8_t *aData, uint8_t &aDataLength, ServiceTlv &aService);

    static void Remove(uint8_t *aData, uint8_t &aDataLength, uint8_t *aRemoveStart, uint8_t aRemoveLength);
    static void RemoveTlv(uint8_t *aData, uint8_t &aDataLength, NetworkDataTlv *aTlv);

    const Type mType;
};

} // namespace NetworkData

/**
 * @}
 */

} // namespace ot

#endif // NETWORK_DATA_HPP_
