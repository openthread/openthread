/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *  This file defines the OpenThread Mesh Diagnostic APIs.
 */

#ifndef OPENTHREAD_MESH_DIAG_H_
#define OPENTHREAD_MESH_DIAG_H_

#include <openthread/instance.h>
#include <openthread/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-mesh-diag
 *
 * @brief
 *   This module includes definitions and functions for Mesh Diagnostics.
 *
 *   The Mesh Diagnostics APIs require `OPENTHREAD_CONFIG_MESH_DIAG_ENABLE` and `OPENTHREAD_FTD`.
 *
 * @{
 *
 */

/**
 * This structure represents the set of configurations used when discovering mesh topology indicating which items to
 * discover.
 *
 */
typedef struct otMeshDiagDiscoverConfig
{
    bool mDiscoverIp6Addresses : 1; ///< Whether or not to discover IPv6 addresses of every router.
    bool mDiscoverChildTable : 1;   ///< Whether or not to discover children of every router.
} otMeshDiagDiscoverConfig;

/**
 * This type is an opaque iterator to iterate over list of IPv6 addresses of a router.
 *
 * Pointers to instance of this type are provided in `otMeshDiagRouterInfo`.
 *
 */
typedef struct otMeshDiagIp6AddrIterator otMeshDiagIp6AddrIterator;

/**
 * This type is an opaque iterator to iterate over list of children of a router.
 *
 * Pointers to instance of this type are provided in `otMeshDiagRouterInfo`.
 *
 */
typedef struct otMeshDiagChildIterator otMeshDiagChildIterator;

/**
 * This type represents information about a router in Thread mesh.
 *
 */
typedef struct otMeshDiagRouterInfo
{
    otExtAddress mExtAddress;             ///< Extended MAC address.
    uint16_t     mRloc16;                 ///< RLOC16.
    uint8_t      mRouterId;               ///< Router ID.
    bool         mIsThisDevice : 1;       ///< Whether router is this device itself.
    bool         mIsThisDeviceParent : 1; ///< Whether router is parent of this device (when device is a child).
    bool         mIsLeader : 1;           ///< Whether router is leader.
    bool         mIsBorderRouter : 1;     ///< Whether router acts as a border router providing ext connectivity.

    /**
     * This array provides the link quality from this router to other routers, also indicating whether a link is
     * established between the routers.
     *
     * The array is indexed based on Router ID. `mLinkQualities[routerId]` indicates the incoming link quality, the
     * router sees to the router with `routerId`. Link quality is a value in [0, 3]. Value zero indicates no link.
     * Larger value indicate better link quality (as defined by Thread specification).
     *
     */
    uint8_t mLinkQualities[OT_NETWORK_MAX_ROUTER_ID + 1];

    /**
     * A pointer to an iterator to go through the list of IPv6 addresses of the router.
     *
     * The pointer is valid only while `otMeshDiagRouterInfo` is valid. It can be used in `otMeshDiagGetNextIp6Address`
     * to iterate through the IPv6 addresses.
     *
     * The pointer can be NULL when there was no request to discover IPv6 addresses (in `otMeshDiagDiscoverConfig`) or
     * if the router did not provide the list.
     *
     */
    otMeshDiagIp6AddrIterator *mIp6AddrIterator;

    /**
     * A pointer to an iterator to go through the list of children of the router.
     *
     * The pointer is valid only while `otMeshDiagRouterInfo` is valid. It can be used in `otMeshDiagGetNextChildInfo`
     * to iterate through the children of the router.
     *
     * The pointer can be NULL when there was no request to discover children (in `otMeshDiagDiscoverConfig`) or
     * if the router did not provide the list.
     *
     */
    otMeshDiagChildIterator *mChildIterator;
} otMeshDiagRouterInfo;

/**
 * This type represents information about a discovered child in Thread mesh.
 *
 */
typedef struct otMeshDiagChildInfo
{
    uint16_t         mRloc16;             ///< RLOC16.
    otLinkModeConfig mMode;               ///< Device mode.
    uint8_t          mLinkQuality;        ///< Incoming link quality to child from parent.
    bool             mIsThisDevice : 1;   ///< Whether child is this device itself.
    bool             mIsBorderRouter : 1; ///< Whether child acts as a border router providing ext connectivity.
} otMeshDiagChildInfo;

/**
 * This function pointer type represents the callback used by `otMeshDiagDiscoverTopology()` to provide information
 * about a discovered router.
 *
 * When @p aError is `OT_ERROR_PENDING`, it indicates that the discovery is not yet finished and there will be more
 * routers to discover and the callback will be invoked again.
 *
 * @param[in] aError       OT_ERROR_PENDING            Indicates there are more routers to be discovered.
 *                         OT_ERROR_NONE               Indicates this is the last router and mesh discovery is done.
 *                         OT_ERROR_RESPONSE_TIMEOUT   Timed out waiting for response from one or more routers.
 * @param[in] aRouterInfo  The discovered router info (can be null if `aError` is OT_ERROR_RESPONSE_TIMEOUT).
 * @param[in] aContext     Application-specific context.
 *
 */
typedef void (*otMeshDiagDiscoverCallback)(otError aError, otMeshDiagRouterInfo *aRouterInfo, void *aContext);

/**
 * This function starts network topology discovery.
 *
 * @param[in] aInstance        The OpenThread instance.
 * @param[in] aConfig          The configuration to use for discovery (e.g., which items to discover).
 * @param[in] aCallback        The callback to report the discovered routers.
 * @param[in] aContext         A context to pass in @p aCallback.
 *
 * @retval OT_ERROR_NONE            The network topology discovery started successfully.
 * @retval OT_ERROR_BUSY            A previous discovery request is still ongoing.
 * @retval OT_ERROR_INVALID_STATE   Device is not attached.
 * @retval OT_ERROR_NO_BUFS         Could not allocate buffer to send discovery messages.
 *
 */
otError otMeshDiagDiscoverTopology(otInstance                     *aInstance,
                                   const otMeshDiagDiscoverConfig *aConfig,
                                   otMeshDiagDiscoverCallback      aCallback,
                                   void                           *aContext);

/**
 * This function cancels an ongoing topology discovery if there is one, otherwise no action.
 *
 * When ongoing discovery is cancelled, the callback from `otMeshDiagDiscoverTopology()` will not be called anymore.
 *
 */
void otMeshDiagCancel(otInstance *aInstance);

/**
 * This function iterates through the discovered IPv6 address of a router.
 *
 * @param[in,out]  aIterator    The address iterator to use.
 * @param[out]     aIp6Address  A pointer to return the next IPv6 address (if any).
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the next address. @p aIp6Address and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND  No more address. Reached the end of the list.
 *
 */
otError otMeshDiagGetNextIp6Address(otMeshDiagIp6AddrIterator *aIterator, otIp6Address *aIp6Address);

/**
 * This function iterates through the discovered children of a router.
 *
 * @param[in,out]  aIterator    The address iterator to use.
 * @param[out]     aChildInfo   A pointer to return the child info (if any).
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the next child. @p aChildInfo and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND  No more child. Reached the end of the list.
 *
 */
otError otMeshDiagGetNextChildInfo(otMeshDiagChildIterator *aIterator, otMeshDiagChildInfo *aChildInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_MESH_DIAG_H_
