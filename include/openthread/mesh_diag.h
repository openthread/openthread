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
 */

/**
 * Represents the set of configurations used when discovering mesh topology indicating which items to
 * discover.
 */
typedef struct otMeshDiagDiscoverConfig
{
    bool mDiscoverIp6Addresses : 1; ///< Whether or not to discover IPv6 addresses of every router.
    bool mDiscoverChildTable : 1;   ///< Whether or not to discover children of every router.
} otMeshDiagDiscoverConfig;

/**
 * An opaque iterator to iterate over list of IPv6 addresses of a router.
 *
 * Pointers to instance of this type are provided in `otMeshDiagRouterInfo`.
 */
typedef struct otMeshDiagIp6AddrIterator otMeshDiagIp6AddrIterator;

/**
 * An opaque iterator to iterate over list of children of a router.
 *
 * Pointers to instance of this type are provided in `otMeshDiagRouterInfo`.
 */
typedef struct otMeshDiagChildIterator otMeshDiagChildIterator;

/**
 * Specifies that Thread Version is unknown.
 *
 * This is used in `otMeshDiagRouterInfo` for `mVersion` property when device does not provide its version. This
 * indicates that device is likely running 1.3.0 (version value 4) or earlier.
 */
#define OT_MESH_DIAG_VERSION_UNKNOWN 0xffff

/**
 * Represents information about a router in Thread mesh discovered using `otMeshDiagDiscoverTopology()`.
 */
typedef struct otMeshDiagRouterInfo
{
    otExtAddress mExtAddress;             ///< Extended MAC address.
    uint16_t     mRloc16;                 ///< RLOC16.
    uint8_t      mRouterId;               ///< Router ID.
    uint16_t     mVersion;                ///< Thread Version. `OT_MESH_DIAG_VERSION_UNKNOWN` if unknown.
    bool         mIsThisDevice : 1;       ///< Whether router is this device itself.
    bool         mIsThisDeviceParent : 1; ///< Whether router is parent of this device (when device is a child).
    bool         mIsLeader : 1;           ///< Whether router is leader.
    bool         mIsBorderRouter : 1;     ///< Whether router acts as a border router providing ext connectivity.

    /**
     * Provides the link quality from this router to other routers, also indicating whether a link is established
     * between the routers.
     *
     * The array is indexed based on Router ID. `mLinkQualities[routerId]` indicates the incoming link quality, the
     * router sees to the router with `routerId`. Link quality is a value in [0, 3]. Value zero indicates no link.
     * Larger value indicate better link quality (as defined by Thread specification).
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
     */
    otMeshDiagChildIterator *mChildIterator;
} otMeshDiagRouterInfo;

/**
 * Represents information about a discovered child in Thread mesh using `otMeshDiagDiscoverTopology()`.
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
 * Pointer type represents the callback used by `otMeshDiagDiscoverTopology()` to provide information
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
 */
typedef void (*otMeshDiagDiscoverCallback)(otError aError, otMeshDiagRouterInfo *aRouterInfo, void *aContext);

/**
 * Starts network topology discovery.
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
 */
otError otMeshDiagDiscoverTopology(otInstance                     *aInstance,
                                   const otMeshDiagDiscoverConfig *aConfig,
                                   otMeshDiagDiscoverCallback      aCallback,
                                   void                           *aContext);

/**
 * Cancels an ongoing topology discovery if there is one, otherwise no action.
 *
 * When ongoing discovery is cancelled, the callback from `otMeshDiagDiscoverTopology()` will not be called anymore.
 */
void otMeshDiagCancel(otInstance *aInstance);

/**
 * Iterates through the discovered IPv6 addresses of a router or an MTD child.
 *
 * MUST be used
 * - from the callback `otMeshDiagDiscoverCallback()` and use the `mIp6AddrIterator` from the `aRouterInfo` struct that
 *   is provided as input to the callback, or
 * - from the callback `otMeshDiagChildIp6AddrsCallback()` along with provided `aIp6AddrIterator`.
 *
 * @param[in,out]  aIterator    The address iterator to use.
 * @param[out]     aIp6Address  A pointer to return the next IPv6 address (if any).
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the next address. @p aIp6Address and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND  No more address. Reached the end of the list.
 */
otError otMeshDiagGetNextIp6Address(otMeshDiagIp6AddrIterator *aIterator, otIp6Address *aIp6Address);

/**
 * Iterates through the discovered children of a router.
 *
 * This function MUST be used from the callback `otMeshDiagDiscoverCallback()` and use the `mChildIterator` from the
 * `aRouterInfo` struct that is provided as input to the callback.
 *
 * @param[in,out]  aIterator    The address iterator to use.
 * @param[out]     aChildInfo   A pointer to return the child info (if any).
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the next child. @p aChildInfo and @p aIterator are updated.
 * @retval OT_ERROR_NOT_FOUND  No more child. Reached the end of the list.
 */
otError otMeshDiagGetNextChildInfo(otMeshDiagChildIterator *aIterator, otMeshDiagChildInfo *aChildInfo);

/**
 * Represents information about a child entry from `otMeshDiagQueryChildTable()`.
 *
 * `mSupportsErrRate` indicates whether or not the error tracking feature is supported and `mFrameErrorRate` and
 * `mMessageErrorRate` values are valid. The frame error rate tracks frame tx errors (towards the child) at MAC
 * layer,  while `mMessageErrorRate` tracks the IPv6 message error rate (above MAC layer and after MAC retries) when
 * an IPv6 message is dropped. For example, if the message is large and requires 6LoWPAN fragmentation, message tx is
 * considered as failed if one of its fragment frame tx fails (for example, never acked).
 */
typedef struct otMeshDiagChildEntry
{
    bool         mRxOnWhenIdle : 1;    ///< Is rx-on when idle (vs sleepy).
    bool         mDeviceTypeFtd : 1;   ///< Is device FTD (vs MTD).
    bool         mFullNetData : 1;     ///< Whether device gets full Network Data (vs stable sub-set).
    bool         mCslSynchronized : 1; ///< Is CSL capable and CSL synchronized.
    bool         mSupportsErrRate : 1; ///< `mFrameErrorRate` and `mMessageErrorRate` values are valid.
    uint16_t     mRloc16;              ///< RLOC16.
    otExtAddress mExtAddress;          ///< Extended Address.
    uint16_t     mVersion;             ///< Version.
    uint32_t     mTimeout;             ///< Timeout in seconds.
    uint32_t     mAge;                 ///< Seconds since last heard from the child.
    uint32_t     mConnectionTime;      ///< Seconds since child attach.
    uint16_t     mSupervisionInterval; ///< Supervision interval in seconds. Zero to indicate not used.
    uint8_t      mLinkMargin;          ///< Link Margin in dB.
    int8_t       mAverageRssi;         ///< Average RSSI.
    int8_t       mLastRssi;            ///< RSSI of last received frame.
    uint16_t     mFrameErrorRate;      ///< Frame error rate (0x0000->0%, 0xffff->100%).
    uint16_t     mMessageErrorRate;    ///< (IPv6) msg error rate (0x0000->0%, 0xffff->100%).
    uint16_t     mQueuedMessageCount;  ///< Number of queued messages for indirect tx to child.
    uint16_t     mCslPeriod;           ///< CSL Period in unit of 10-symbols-time. Zero indicates CSL is disabled.
    uint32_t     mCslTimeout;          ///< CSL Timeout in seconds.
    uint8_t      mCslChannel;          ///< CSL channel.
} otMeshDiagChildEntry;

/**
 * Represents the callback used by `otMeshDiagQueryChildTable()` to provide information about child table entries.
 *
 * When @p aError is `OT_ERROR_PENDING`, it indicates that the table still has more entries and the callback will be
 * invoked again.
 *
 * @param[in] aError       OT_ERROR_PENDING            Indicates there are more entries in the table.
 *                         OT_ERROR_NONE               Indicates the table is finished.
 *                         OT_ERROR_RESPONSE_TIMEOUT   Timed out waiting for response.
 * @param[in] aChildEntry  The child entry (can be null if `aError` is OT_ERROR_RESPONSE_TIMEOUT or OT_ERROR_NONE).
 * @param[in] aContext     Application-specific context.
 */
typedef void (*otMeshDiagQueryChildTableCallback)(otError                     aError,
                                                  const otMeshDiagChildEntry *aChildEntry,
                                                  void                       *aContext);

/**
 * Starts query for child table for a given router.
 *
 * @param[in] aInstance        The OpenThread instance.
 * @param[in] aRloc16          The RLOC16 of router to query.
 * @param[in] aCallback        The callback to report the queried child table.
 * @param[in] aContext         A context to pass in @p aCallback.
 *
 * @retval OT_ERROR_NONE           The query started successfully.
 * @retval OT_ERROR_BUSY           A previous discovery or query request is still ongoing.
 * @retval OT_ERROR_INVALID_ARGS   The @p aRloc16 is not a valid router RLOC16.
 * @retval OT_ERROR_INVALID_STATE  Device is not attached.
 * @retval OT_ERROR_NO_BUFS        Could not allocate buffer to send query messages.
 */
otError otMeshDiagQueryChildTable(otInstance                       *aInstance,
                                  uint16_t                          aRloc16,
                                  otMeshDiagQueryChildTableCallback aCallback,
                                  void                             *aContext);

/**
 * Represents the callback used by `otMeshDiagQueryChildrenIp6Addrs()` to provide information about an MTD child and
 * its list of IPv6 addresses.
 *
 * When @p aError is `OT_ERROR_PENDING`, it indicates that there are more children and the callback will be invoked
 * again.
 *
 * @param[in] aError            OT_ERROR_PENDING            Indicates there are more children in the table.
 *                              OT_ERROR_NONE               Indicates the table is finished.
 *                              OT_ERROR_RESPONSE_TIMEOUT   Timed out waiting for response.
 * @param[in] aChildRloc16      The RLOC16 of the child. `0xfffe` is used on `OT_ERROR_RESPONSE_TIMEOUT`.
 * @param[in] aIp6AddrIterator  An iterator to go through the IPv6 addresses of the child with @p aRloc using
 *                              `otMeshDiagGetNextIp6Address()`. Set to NULL on `OT_ERROR_RESPONSE_TIMEOUT`.
 * @param[in] aContext          Application-specific context.
 */
typedef void (*otMeshDiagChildIp6AddrsCallback)(otError                    aError,
                                                uint16_t                   aChildRloc16,
                                                otMeshDiagIp6AddrIterator *aIp6AddrIterator,
                                                void                      *aContext);

/**
 * Sends a query to a parent to retrieve the IPv6 addresses of all its MTD children.
 *
 * @param[in] aInstance        The OpenThread instance.
 * @param[in] aRloc16          The RLOC16 of parent to query.
 * @param[in] aCallback        The callback to report the queried child IPv6 address list.
 * @param[in] aContext         A context to pass in @p aCallback.
 *
 * @retval OT_ERROR_NONE           The query started successfully.
 * @retval OT_ERROR_BUSY           A previous discovery or query request is still ongoing.
 * @retval OT_ERROR_INVALID_ARGS   The @p aRloc16 is not a valid  RLOC16.
 * @retval OT_ERROR_INVALID_STATE  Device is not attached.
 * @retval OT_ERROR_NO_BUFS        Could not allocate buffer to send query messages.
 */
otError otMeshDiagQueryChildrenIp6Addrs(otInstance                     *aInstance,
                                        uint16_t                        aRloc16,
                                        otMeshDiagChildIp6AddrsCallback aCallback,
                                        void                           *aContext);

/**
 * Represents information about a router neighbor entry from `otMeshDiagQueryRouterNeighborTable()`.
 *
 * `mSupportsErrRate` indicates whether or not the error tracking feature is supported and `mFrameErrorRate` and
 * `mMessageErrorRate` values are valid. The frame error rate tracks frame tx errors (towards the child) at MAC
 * layer,  while `mMessageErrorRate` tracks the IPv6 message error rate (above MAC layer and after MAC retries) when
 * an IPv6 message is dropped. For example, if the message is large and requires 6LoWPAN fragmentation, message tx is
 * considered as failed if one of its fragment frame tx fails (for example, never acked).
 */
typedef struct otMeshDiagRouterNeighborEntry
{
    bool         mSupportsErrRate : 1; ///< `mFrameErrorRate` and `mMessageErrorRate` values are valid.
    uint16_t     mRloc16;              ///< RLOC16.
    otExtAddress mExtAddress;          ///< Extended Address.
    uint16_t     mVersion;             ///< Version.
    uint32_t     mConnectionTime;      ///< Seconds since link establishment.
    uint8_t      mLinkMargin;          ///< Link Margin in dB.
    int8_t       mAverageRssi;         ///< Average RSSI.
    int8_t       mLastRssi;            ///< RSSI of last received frame.
    uint16_t     mFrameErrorRate;      ///< Frame error rate (0x0000->0%, 0xffff->100%).
    uint16_t     mMessageErrorRate;    ///< (IPv6) msg error rate (0x0000->0%, 0xffff->100%).
} otMeshDiagRouterNeighborEntry;

/**
 * Represents the callback used by `otMeshDiagQueryRouterNeighborTable()` to provide information about neighbor router
 * table entries.
 *
 * When @p aError is `OT_ERROR_PENDING`, it indicates that the table still has more entries and the callback will be
 * invoked again.
 *
 * @param[in] aError          OT_ERROR_PENDING            Indicates there are more entries in the table.
 *                            OT_ERROR_NONE               Indicates the table is finished.
 *                            OT_ERROR_RESPONSE_TIMEOUT   Timed out waiting for response.
 * @param[in] aNeighborEntry  The neighbor entry (can be null if `aError` is RESPONSE_TIMEOUT or NONE).
 * @param[in] aContext        Application-specific context.
 */
typedef void (*otMeshDiagQueryRouterNeighborTableCallback)(otError                              aError,
                                                           const otMeshDiagRouterNeighborEntry *aNeighborEntry,
                                                           void                                *aContext);

/**
 * Starts query for router neighbor table for a given router.
 *
 * @param[in] aInstance        The OpenThread instance.
 * @param[in] aRloc16          The RLOC16 of router to query.
 * @param[in] aCallback        The callback to report the queried table.
 * @param[in] aContext         A context to pass in @p aCallback.
 *
 * @retval OT_ERROR_NONE           The query started successfully.
 * @retval OT_ERROR_BUSY           A previous discovery or query request is still ongoing.
 * @retval OT_ERROR_INVALID_ARGS   The @p aRloc16 is not a valid router RLOC16.
 * @retval OT_ERROR_INVALID_STATE  Device is not attached.
 * @retval OT_ERROR_NO_BUFS        Could not allocate buffer to send query messages.
 */
otError otMeshDiagQueryRouterNeighborTable(otInstance                                *aInstance,
                                           uint16_t                                   aRloc16,
                                           otMeshDiagQueryRouterNeighborTableCallback aCallback,
                                           void                                      *aContext);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_MESH_DIAG_H_
