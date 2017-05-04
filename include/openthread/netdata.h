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

#include "openthread/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup netdata  Network Data
 *
 * @brief
 *   This module includes functions that control Network Data configuration.
 *
 * @{
 *
 */

/**
 * This method provides a full or stable copy of the Leader's Thread Network Data.
 *
 * @param[in]     aInstance    A pointer to an OpenThread instance.
 * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]    aData        A pointer to the data buffer.
 * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                             On exit, number of copied bytes.
 */
OTAPI ThreadError OTCALL otNetDataGetLeader(otInstance *aInstance, bool aStable, uint8_t *aData,
                                            uint8_t *aDataLength);

/**
 * This method provides a full or stable copy of the local Thread Network Data.
 *
 * @param[in]     aInstance    A pointer to an OpenThread instance.
 * @param[in]     aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]    aData        A pointer to the data buffer.
 * @param[inout]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                             On exit, number of copied bytes.
 */
OTAPI ThreadError OTCALL otNetDataGetLocal(otInstance *aInstance, bool aStable, uint8_t *aData,
                                           uint8_t *aDataLength);

/**
 * This function gets the next On Mesh Prefix in the Network Data.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[in]     aLocal     TRUE to retrieve from the local Network Data, FALSE for partition's Network Data
 * @param[inout]  aIterator  A pointer to the Network Data iterator context. To get the first on-mesh entry
                             it should be set to OT_NETWORK_DATA_ITERATOR_INIT.
 * @param[out]    aConfig    A pointer to where the On Mesh Prefix information will be placed.
 *
 * @retval kThreadError_None      Successfully found the next On Mesh prefix.
 * @retval kThreadError_NotFound  No subsequent On Mesh prefix exists in the Thread Network Data.
 *
 */
OTAPI ThreadError OTCALL otNetDataGetNextPrefixInfo(otInstance *aInstance, bool aLocal,
                                                    otNetworkDataIterator *aIterator, otBorderRouterConfig *aConfig);

/**
 * Add a border router configuration to the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the border router configuration.
 *
 * @retval kThreadErrorNone         Successfully added the configuration to the local network data.
 * @retval kThreadErrorInvalidArgs  One or more configuration parameters were invalid.
 * @retval kThreadErrorSize         Not enough room is available to add the configuration to the local network data.
 *
 * @sa otRemoveBorderRouter
 * @sa otSendServerData
 */
OTAPI ThreadError OTCALL otNetDataAddPrefixInfo(otInstance *aInstance, const otBorderRouterConfig *aConfig);

/**
 * Remove a border router configuration from the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPrefix   A pointer to an IPv6 prefix.
 *
 * @retval kThreadErrorNone  Successfully removed the configuration from the local network data.
 *
 * @sa otAddBorderRouter
 * @sa otSendServerData
 */
OTAPI ThreadError OTCALL otNetDataRemovePrefixInfo(otInstance *aInstance, const otIp6Prefix *aPrefix);

/**
 * Add an external route configuration to the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the external route configuration.
 *
 * @retval kThreadErrorNone         Successfully added the configuration to the local network data.
 * @retval kThreadErrorInvalidArgs  One or more configuration parameters were invalid.
 * @retval kThreadErrorSize         Not enough room is available to add the configuration to the local network data.
 *
 * @sa otRemoveExternalRoute
 * @sa otSendServerData
 */
OTAPI ThreadError OTCALL otNetDataAddRoute(otInstance *aInstance, const otExternalRouteConfig *aConfig);

/**
 * Remove an external route configuration from the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPrefix   A pointer to an IPv6 prefix.
 *
 * @retval kThreadErrorNone  Successfully removed the configuration from the local network data.
 *
 * @sa otAddExternalRoute
 * @sa otSendServerData
 */
OTAPI ThreadError OTCALL otNetDataRemoveRoute(otInstance *aInstance, const otIp6Prefix *aPrefix);

/**
 * This function gets the next external route in the Network Data.
 *
 * @param[in]     aInstance  A pointer to an OpenThread instance.
 * @param[in]     aLocal     TRUE to retrieve from the local Network Data, FALSE for partition's Network Data
 * @param[inout]  aIterator  A pointer to the Network Data iterator context. To get the first external route entry
                             it should be set to OT_NETWORK_DATA_ITERATOR_INIT.
 * @param[out]    aConfig    A pointer to where the External Route information will be placed.
 *
 * @retval kThreadError_None      Successfully found the next External Route.
 * @retval kThreadError_NotFound  No subsequent external route entry exists in the Thread Network Data.
 *
 */
ThreadError otNetDataGetNextRoute(otInstance *aInstance, bool aLocal, otNetworkDataIterator *aIterator,
                                  otExternalRouteConfig *aConfig);

/**
 * Immediately register the local network data with the Leader.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * retval kThreadErrorNone  Successfully queued a Server Data Request message for delivery.
 *
 * @sa otAddBorderRouter
 * @sa otRemoveBorderRouter
 * @sa otAddExternalRoute
 * @sa otRemoveExternalRoute
 */
OTAPI ThreadError OTCALL otNetDataRegister(otInstance *aInstance);

/**
 * Get the Network Data Version.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Network Data Version.
 */
OTAPI uint8_t OTCALL otNetDataGetVersion(otInstance *aInstance);

/**
 * Get the Stable Network Data Version.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns The Stable Network Data Version.
 */
OTAPI uint8_t OTCALL otNetDataGetStableVersion(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_NETDATA_H_
