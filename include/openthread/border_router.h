/*
 *  Copyright (c) 2016-2017, The OpenThread Authors.
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
 *  This file defines the OpenThread Border Router API.
 */

#ifndef OPENTHREAD_BORDER_ROUTER_H_
#define OPENTHREAD_BORDER_ROUTER_H_

#include <openthread/border_routing.h>
#include <openthread/ip6.h>
#include <openthread/netdata.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-border-router
 *
 * @brief
 *  This module includes functions to manage local network data with the OpenThread Border Router.
 *
 * @{
 *
 */

/**
 * This method provides a full or stable copy of the local Thread Network Data.
 *
 * @param[in]      aInstance    A pointer to an OpenThread instance.
 * @param[in]      aStable      TRUE when copying the stable version, FALSE when copying the full version.
 * @param[out]     aData        A pointer to the data buffer.
 * @param[in,out]  aDataLength  On entry, size of the data buffer pointed to by @p aData.
 *                              On exit, number of copied bytes.
 */
otError otBorderRouterGetNetData(otInstance *aInstance, bool aStable, uint8_t *aData, uint8_t *aDataLength);

/**
 * Add a border router configuration to the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the border router configuration.
 *
 * @retval OT_ERROR_NONE          Successfully added the configuration to the local network data.
 * @retval OT_ERROR_INVALID_ARGS  One or more configuration parameters were invalid.
 * @retval OT_ERROR_NO_BUFS       Not enough room is available to add the configuration to the local network data.
 *
 * @sa otBorderRouterRemoveOnMeshPrefix
 * @sa otBorderRouterRegister
 */
otError otBorderRouterAddOnMeshPrefix(otInstance *aInstance, const otBorderRouterConfig *aConfig);

/**
 * Remove a border router configuration from the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPrefix   A pointer to an IPv6 prefix.
 *
 * @retval OT_ERROR_NONE       Successfully removed the configuration from the local network data.
 * @retval OT_ERROR_NOT_FOUND  Could not find the Border Router entry.
 *
 * @sa otBorderRouterAddOnMeshPrefix
 * @sa otBorderRouterRegister
 */
otError otBorderRouterRemoveOnMeshPrefix(otInstance *aInstance, const otIp6Prefix *aPrefix);

/**
 * This function gets the next On Mesh Prefix in the local Network Data.
 *
 * @param[in]      aInstance  A pointer to an OpenThread instance.
 * @param[in,out]  aIterator  A pointer to the Network Data iterator context. To get the first on-mesh entry
                              it should be set to OT_NETWORK_DATA_ITERATOR_INIT.
 * @param[out]     aConfig    A pointer to the On Mesh Prefix information.
 *
 * @retval OT_ERROR_NONE       Successfully found the next On Mesh prefix.
 * @retval OT_ERROR_NOT_FOUND  No subsequent On Mesh prefix exists in the Thread Network Data.
 *
 */
otError otBorderRouterGetNextOnMeshPrefix(otInstance *           aInstance,
                                          otNetworkDataIterator *aIterator,
                                          otBorderRouterConfig * aConfig);

/**
 * Add an external route configuration to the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aConfig   A pointer to the external route configuration.
 *
 * @retval OT_ERROR_NONE          Successfully added the configuration to the local network data.
 * @retval OT_ERROR_INVALID_ARGS  One or more configuration parameters were invalid.
 * @retval OT_ERROR_NO_BUFS       Not enough room is available to add the configuration to the local network data.
 *
 * @sa otBorderRouterRemoveRoute
 * @sa otBorderRouterRegister
 */
otError otBorderRouterAddRoute(otInstance *aInstance, const otExternalRouteConfig *aConfig);

/**
 * Remove an external route configuration from the local network data.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aPrefix   A pointer to an IPv6 prefix.
 *
 * @retval OT_ERROR_NONE       Successfully removed the configuration from the local network data.
 * @retval OT_ERROR_NOT_FOUND  Could not find the Border Router entry.
 *
 * @sa otBorderRouterAddRoute
 * @sa otBorderRouterRegister
 */
otError otBorderRouterRemoveRoute(otInstance *aInstance, const otIp6Prefix *aPrefix);

/**
 * This function gets the next external route in the local Network Data.
 *
 * @param[in]      aInstance  A pointer to an OpenThread instance.
 * @param[in,out]  aIterator  A pointer to the Network Data iterator context. To get the first external route entry
                              it should be set to OT_NETWORK_DATA_ITERATOR_INIT.
 * @param[out]     aConfig    A pointer to the External Route information.
 *
 * @retval OT_ERROR_NONE       Successfully found the next External Route.
 * @retval OT_ERROR_NOT_FOUND  No subsequent external route entry exists in the Thread Network Data.
 *
 */
otError otBorderRouterGetNextRoute(otInstance *           aInstance,
                                   otNetworkDataIterator *aIterator,
                                   otExternalRouteConfig *aConfig);

/**
 * Immediately register the local network data with the Leader.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval OT_ERROR_NONE  Successfully queued a Server Data Request message for delivery.
 *
 * @sa otBorderRouterAddOnMeshPrefix
 * @sa otBorderRouterRemoveOnMeshPrefix
 * @sa otBorderRouterAddRoute
 * @sa otBorderRouterRemoveRoute
 */
otError otBorderRouterRegister(otInstance *aInstance);

/**
 * This function sends a datagram via the Thread interface with the logic (like NAT64) for border routers.
 *
 * The caller transfers ownership of @p aMessage when making this call. OpenThread will free @p aMessage when processing
 * is complete, including when a value other than `OT_ERROR_NONE` is returned.
 *
 * The content of @p aMessage can be an IPv6 packet or an IPv4 packet. When @p aMessage contains an IPv4 packet, NAT64
 * will work and translate it into an IPv6 packet. @p aMessage should be allocated by otIp4NewMessage if the
 * content is an IPv4 packet.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aMessage  A pointer to the message buffer containing the IPv6 datagram.
 *
 * @retval OT_ERROR_NONE                    Successfully processed the message.
 * @retval OT_ERROR_DROP                    Message was well-formed but not fully processed due to packet processing
 * rules.
 * @retval OT_ERROR_NO_BUFS                 Could not allocate necessary message buffers when processing the datagram.
 * @retval OT_ERROR_NO_ROUTE                No route to host.
 * @retval OT_ERROR_INVALID_SOURCE_ADDRESS  Source address is invalid, e.g. an anycast address or a multicast address.
 * @retval OT_ERROR_PARSE                   Encountered a malformed header when processing the message.
 *
 * @sa otIp4NewMessage
 * @sa otBorderRouterSetReceiveCallback
 *
 */
otError otBorderRouterSend(otInstance *aInstance, otMessage *aMessage);

/**
 * This function registers a callback to provide received datagrams for border routers.
 *
 * By default, this callback does not pass Thread control traffic.  See otIp6SetReceiveFilterEnabled() to
 * change the Thread control traffic filter setting.
 *
 * The callback registered by this function may receive an IPv4 packet when NAT64 is enabled.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when an IPv6 datagram is received or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @sa otBorderRouterSend
 * @sa otIp6SetReceiveCallback
 * @sa otIp6IsReceiveFilterEnabled
 * @sa otIp6SetReceiveFilterEnabled
 *
 */
void otBorderRouterSetReceiveCallback(otInstance *aInstance, otIp6ReceiveCallback aCallback, void *aCallbackContext);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_BORDER_ROUTER_H_
