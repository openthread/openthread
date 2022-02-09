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
 * @brief
 *  This file defines the OpenThread Network Data API.
 */

#ifndef OPENTHREAD_NETDATA_H_
#define OPENTHREAD_NETDATA_H_

#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-general
 *
 * @{
 *
 * @cli netdata help
 * @code
 * netdata help
 * help
 * publish
 * register
 * show
 * steeringdata
 * unpublish
 * Done
 * @endcode
 * @par
 * Gets a list of `netdata` CLI commands.
 * @sa @netdata
 *
 *
 */

#define OT_NETWORK_DATA_ITERATOR_INIT 0 ///< Value to initialize `otNetworkDataIterator`.

typedef uint32_t otNetworkDataIterator; ///< Used to iterate through Network Data information.

/**
 * This structure represents a Border Router configuration.
 */
typedef struct otBorderRouterConfig
{
    otIp6Prefix mPrefix;         ///< The IPv6 prefix.
    signed int  mPreference : 2; ///< A 2-bit signed int preference (`OT_ROUTE_PREFERENCE_*` values).
                                 ///< Maps to `high`, `med`, or `low` in OT CLI.
    bool mPreferred : 1;         ///< Whether prefix is preferred.
                                 ///< Maps to `p` in OT CLI.
    bool mSlaac : 1;             ///< Whether prefix can be used for address auto-configuration (SLAAC).
                                 ///< Maps to `a` in OT CLI.
    bool mDhcp : 1;              ///< Whether border router is DHCPv6 Agent.
                                 ///< Maps to `d` in OT CLI.
    bool mConfigure : 1;         ///< Whether DHCPv6 Agent supplying other config data.
                                 ///< Maps to `c` in OT CLI.
    bool mDefaultRoute : 1;      ///< Whether border router is a default router for prefix.
                                 ///< Maps to `r` in OT CLI.
    bool mOnMesh : 1;            ///< Whether this prefix is considered on-mesh.
                                 ///< Maps to `o` in OT CLI.
    bool mStable : 1;            ///< Whether this configuration is considered Stable Network Data.
                                 ///< Maps to`s` in OT CLI.
    bool mNdDns : 1;             ///< Whether this border router can supply DNS information via ND.
                                 ///< Maps to `n` in OT CLI.
    bool mDp : 1;                ///< Whether prefix is a Thread Domain Prefix (added since Thread 1.2).
                                 ///< Maps to `D` in OT CLI.
    uint16_t mRloc16;            ///< The border router's RLOC16 (value ignored on config add).
} otBorderRouterConfig;

/**
 * This structure represents an External Route configuration.
 *
 */
typedef struct otExternalRouteConfig
{
    otIp6Prefix mPrefix;                  ///< The IPv6 prefix.
    uint16_t    mRloc16;                  ///< The border router's RLOC16 (value ignored on config add).
    signed int  mPreference : 2;          ///< A 2-bit signed int preference (`OT_ROUTE_PREFERENCE_*` values).
    bool        mNat64 : 1;               ///< Whether this is a NAT64 prefix.
    bool        mStable : 1;              ///< Whether this configuration is considered Stable Network Data.
    bool        mNextHopIsThisDevice : 1; ///< Whether the next hop is this device (value ignored on config add).
} otExternalRouteConfig;

/**
 * Defines valid values for `mPreference` in `otExternalRouteConfig` and `otBorderRouterConfig`.
 *
 */
typedef enum otRoutePreference
{
    OT_ROUTE_PREFERENCE_LOW  = -1, ///< Low route preference.
    OT_ROUTE_PREFERENCE_MED  = 0,  ///< Medium route preference.
    OT_ROUTE_PREFERENCE_HIGH = 1,  ///< High route preference.
} otRoutePreference;

#define OT_SERVICE_DATA_MAX_SIZE 252 ///< Max size of Service Data in bytes.
#define OT_SERVER_DATA_MAX_SIZE 248  ///< Max size of Server Data in bytes. Theoretical limit, practically much lower.

/**
 * This structure represents a Server configuration.
 *
 */
typedef struct otServerConfig
{
    bool     mStable : 1;                          ///< Whether this config is considered Stable Network Data.
    uint8_t  mServerDataLength;                    ///< Length of server data.
    uint8_t  mServerData[OT_SERVER_DATA_MAX_SIZE]; ///< Server data bytes.
    uint16_t mRloc16;                              ///< The Server RLOC16.
} otServerConfig;

/**
 * This structure represents a Service configuration.
 *
 */
typedef struct otServiceConfig
{
    uint8_t        mServiceId;                             ///< Service ID (when iterating over the  Network Data).
    uint32_t       mEnterpriseNumber;                      ///< IANA Enterprise Number.
    uint8_t        mServiceDataLength;                     ///< Length of service data.
    uint8_t        mServiceData[OT_SERVICE_DATA_MAX_SIZE]; ///< Service data bytes.
    otServerConfig mServerConfig;                          ///< The Server configuration.
} otServiceConfig;

/**
 * Gets a full or stable copy of the Partition's Thread network data, including
 * prefixes, routes, and services.
 *
 * @cli netdata show
 * @code
 * netdata show
 * Prefixes:
 * fd00:dead:beef:cafe::/64 paros med dc00
 * Routes:
 * fd49:7770:7fc5:0::/64 s med 4000
 * Services:
 * 44970 5d c000 s 4000
 * 44970 01 9a04b000000e10 s 4000
 * Done
 * @endcode
 * @code
 * netdata show -x
 * 08040b02174703140040fd00deadbeefcafe0504dc00330007021140
 * Done
 * @endcode
 * @code
 * netdata show local
 * Prefixes:
 * fd00:dead:beef:cafe::/64 paros med dc00
 * Routes:
 * Services:
 * Done
 * @endcode
 * @code
 * netdata show local -x
 * 08040b02174703140040fd00deadbeefcafe0504dc00330007021140
 * Done
 * @endcode
 * @cparam netdata show [@ca{local}] [@ca{-x}]
 * *   The optional `-x` argument gets Network Data as hex-encoded TLVs.
 * *   The optional `local` argument gets local Network Data to sync with Leader.
 * @par
 * `netdata show` from OT CLI gets full Network Data. This command uses several API functions to combine
 * prefixes, routes, and services, including #otNetDataGetNextOnMeshPrefix, #otNetDataGetNextRoute, and
 * #otNetDataGetNextService.
 * @par
 * @moreinfo{@netdata}.
 * @csa{br omrprefix}
 * @csa{br onlinkprefix}
 * @sa [NetworkData::ProcessShow
 * function](https://github.com/openthread/openthread/blob/main/src/cli/cli_network_data.cpp)
 * @sa #otBorderRouterGetNetData
 *
 * @param[in]     aInstance    A pointer to an OpenThread instance.
 * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]    aData        A pointer to the data buffer.
 * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                             On exit, number of copied bytes.
 *
 */
otError otNetDataGet(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength);

/**
 * This function gets the next On Mesh Prefix in the partition's Network Data.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A pointer to the Network Data iterator context. To get the first on-mesh entry
                             it should be set to OT_NETWORK_DATA_ITERATOR_INIT.
 * @param[out]    aConfig    A pointer to where the On Mesh Prefix information will be placed.
 *
 * @retval OT_ERROR_NONE       Successfully found the next On Mesh prefix.
 * @retval OT_ERROR_NOT_FOUND  No subsequent On Mesh prefix exists in the Thread Network Data.
 *
 */
otError otNetDataGetNextOnMeshPrefix(otInstance *           aInstance,
                                     otNetworkDataIterator *aIterator,
                                     otBorderRouterConfig * aConfig);

/**
 * This function gets the next external route in the partition's Network Data.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A pointer to the Network Data iterator context. To get the first external route entry
                             it should be set to OT_NETWORK_DATA_ITERATOR_INIT.
 * @param[out]    aConfig    A pointer to where the External Route information will be placed.
 *
 * @retval OT_ERROR_NONE       Successfully found the next External Route.
 * @retval OT_ERROR_NOT_FOUND  No subsequent external route entry exists in the Thread Network Data.
 *
 */
otError otNetDataGetNextRoute(otInstance *aInstance, otNetworkDataIterator *aIterator, otExternalRouteConfig *aConfig);

/**
 * This function gets the next service in the partition's Network Data.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[inout]  aIterator  A pointer to the Network Data iterator context. To get the first service entry
                             it should be set to OT_NETWORK_DATA_ITERATOR_INIT.
 * @param[out]    aConfig    A pointer to where the service information will be placed.
 *
 * @retval OT_ERROR_NONE       Successfully found the next service.
 * @retval OT_ERROR_NOT_FOUND  No subsequent service exists in the partition's Network Data.
 *
 */
otError otNetDataGetNextService(otInstance *aInstance, otNetworkDataIterator *aIterator, otServiceConfig *aConfig);

/**
 * Get the Network Data Version.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Network Data Version.
 *
 */
uint8_t otNetDataGetVersion(otInstance *aInstance);

/**
 * Get the Stable Network Data Version.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Stable Network Data Version.
 *
 */
uint8_t otNetDataGetStableVersion(otInstance *aInstance);

/**
 * Check whether the steering data includes a Joiner.
 *
 * @cli netdata steeringdata check
 * @code
 * netdata steeringdata check d45e64fa83f81cf7
 * Done
 * @endcode
 * @code
 * netdata steeringdata check 0xabc/12
 * Done
 * @endcode
 * @code
 * netdata steeringdata check 0xdef/12
 * Error 23: NotFound
 * @endcode
 * @cparam netdata steeringdata check {@ca{eui64}|@ca{discerner}}
 * *   `eui64`: The IEEE EUI-64 of the Joiner.
 * *   `discerner`: The Joiner discerner in format `{number}/{length}`.
 * @par
 * @moreinfo{@netdata}.
 * @csa{eui64}
 * @csa{joiner discerner (get)}
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aEui64             A pointer to the Joiner's IEEE EUI-64.
 *
 * @retval OT_ERROR_NONE          @p aEui64 is included in the steering data.
 * @retval OT_ERROR_INVALID_STATE No steering data present.
 * @retval OT_ERROR_NOT_FOUND     @p aEui64 is not included in the steering data.
 *
 */
otError otNetDataSteeringDataCheckJoiner(otInstance *aInstance, const otExtAddress *aEui64);

// Forward declaration
struct otJoinerDiscerner;

/**
 * Check if the steering data includes a Joiner with a given discerner value.
 *
 * @param[in]  aInstance          A pointer to an OpenThread instance.
 * @param[in]  aDiscerner         A pointer to the Joiner Discerner.
 *
 * @retval OT_ERROR_NONE          @p aDiscerner is included in the steering data.
 * @retval OT_ERROR_INVALID_STATE No steering data present.
 * @retval OT_ERROR_NOT_FOUND     @p aDiscerner is not included in the steering data.
 *
 */
otError otNetDataSteeringDataCheckJoinerWithDiscerner(otInstance *                    aInstance,
                                                      const struct otJoinerDiscerner *aDiscerner);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_NETDATA_H_
