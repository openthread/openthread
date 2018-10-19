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
 */

#define OT_NETWORK_DATA_ITERATOR_INIT 0 ///< Initializer for otNetworkDataIterator.

typedef uint32_t otNetworkDataIterator; ///< Used to iterate through Network Data information.

/**
 * This structure represents a Border Router configuration.
 */
typedef struct otBorderRouterConfig
{
    /**
     * The IPv6 prefix.
     */
    otIp6Prefix mPrefix;

    /**
     * A 2-bit signed integer indicating router preference as defined in RFC 4191.
     */
    int mPreference : 2;

    /**
     * TRUE, if @p mPrefix is preferred.  FALSE, otherwise.
     */
    bool mPreferred : 1;

    /**
     * TRUE, if @p mPrefix should be used for address autoconfiguration.  FALSE, otherwise.
     */
    bool mSlaac : 1;

    /**
     * TRUE, if this border router is a DHCPv6 Agent that supplies IPv6 address configuration.  FALSE, otherwise.
     */
    bool mDhcp : 1;

    /**
     * TRUE, if this border router is a DHCPv6 Agent that supplies other configuration data.  FALSE, otherwise.
     */
    bool mConfigure : 1;

    /**
     * TRUE, if this border router is a default route for @p mPrefix.  FALSE, otherwise.
     */
    bool mDefaultRoute : 1;

    /**
     * TRUE, if this prefix is considered on-mesh.  FALSE, otherwise.
     */
    bool mOnMesh : 1;

    /**
     * TRUE, if this configuration is considered Stable Network Data.  FALSE, otherwise.
     */
    bool mStable : 1;

    /**
     * The Border Agent Rloc.
     */
    uint16_t mRloc16;
} otBorderRouterConfig;

/**
 * This structure represents an External Route configuration.
 *
 */
typedef struct otExternalRouteConfig
{
    /**
     * The prefix for the off-mesh route.
     */
    otIp6Prefix mPrefix;

    /**
     * The Rloc associated with the external route entry.
     *
     * This value is ignored when adding an external route. For any added route, the device's Rloc is used.
     */
    uint16_t mRloc16;

    /**
     * A 2-bit signed integer indicating router preference as defined in RFC 4191.
     */
    int mPreference : 2;

    /**
     * TRUE, if this configuration is considered Stable Network Data.  FALSE, otherwise.
     */
    bool mStable : 1;

    /**
     * TRUE if the external route entry's next hop is this device itself (i.e., the route was added earlier by this
     * device). FALSE otherwise.
     *
     * This value is ignored when adding an external route. For any added route the next hop is this device.
     */
    bool mNextHopIsThisDevice : 1;

} otExternalRouteConfig;

/**
 * Defines valid values for member mPreference in otExternalRouteConfig and otBorderRouterConfig.
 *
 */
typedef enum otRoutePreference
{
    OT_ROUTE_PREFERENCE_LOW  = -1, ///< Low route preference.
    OT_ROUTE_PREFERENCE_MED  = 0,  ///< Medium route preference.
    OT_ROUTE_PREFERENCE_HIGH = 1,  ///< High route preference.
} otRoutePreference;

/**
 * This method provides a full or stable copy of the Leader's Thread Network Data.
 *
 * @param[in]     aInstance    A pointer to an OpenThread instance.
 * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]    aData        A pointer to the data buffer.
 * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                             On exit, number of copied bytes.
 *
 */
OTAPI otError OTCALL otNetDataGet(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength);

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
OTAPI otError OTCALL otNetDataGetNextOnMeshPrefix(otInstance *           aInstance,
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
 * Get the Network Data Version.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Network Data Version.
 *
 */
OTAPI uint8_t OTCALL otNetDataGetVersion(otInstance *aInstance);

/**
 * Get the Stable Network Data Version.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Stable Network Data Version.
 *
 */
OTAPI uint8_t OTCALL otNetDataGetStableVersion(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_NETDATA_H_
