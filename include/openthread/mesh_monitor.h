/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#ifndef OPENTHREAD_MESH_MONITOR_H_
#define OPENTHREAD_MESH_MONITOR_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/message.h>
#include <openthread/netdiag.h>
#include <openthread/thread.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-general
 *
 * @brief
 *   This module includes functions for Mesh Monitor.
 *
 *   The server functions are not explicitly marked and are available when the Mesh Monitor
 *   `OPENTHREAD_CONFIG_MESH_MONITOR_SERVER_ENABLE` is enabled. Client functions are explicitly marked to require
 *   `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE` enabled.
 *   Mesh Monitor is a protocol for collecting and reporting Thread network information. It allows a client to
 *   request Mesh Monitoring data from a server about the server's state and its children's states. The client can
 *   register with the server to be notified about changes of the requested Mesh Monitoring data. The information is
 *   organized in TLVs (Type-Length-Value) and can include details about the devices, their connectivity, and other
 *   network parameters.
 *
 * @{
 */

/**
 * Represents the type of device in a Mesh Monitor device context.
 */
typedef enum otMeshMonDeviceType
{
    OT_MESH_MON_DEVICE_HOST     = 0, ///< Host Device Context type
    OT_MESH_MON_DEVICE_CHILD    = 1, ///< Child Device Context type
    OT_MESH_MON_DEVICE_NEIGHBOR = 2, ///< Neighbor Device Context type
} otMeshMonDeviceType;

/**
 * Represents the update mode in a Mesh Monitor device context.
 */
typedef enum otMeshMonUpdateMode
{
    OT_MESH_MON_UPDATE_MODE_ADDED   = 0, ///< Added
    OT_MESH_MON_UPDATE_MODE_UPDATED = 1, ///< Updated
    OT_MESH_MON_UPDATE_MODE_REMOVED = 2, ///< Removed
} otMeshMonUpdateMode;

#define OT_MESH_MON_TLV_EXT_ADDRESS 0U                  ///< Extended Address TLV
#define OT_MESH_MON_TLV_MODE 1U                         ///< Mode TLV
#define OT_MESH_MON_TLV_TIMEOUT 2U                      ///< Timeout TLV
#define OT_MESH_MON_TLV_LAST_HEARD 3U                   ///< Last Heard TLV
#define OT_MESH_MON_TLV_CONNECTION_TIME 4U              ///< Connection Time TLV
#define OT_MESH_MON_TLV_CSL 5U                          ///< CSL Configuration TLV
#define OT_MESH_MON_TLV_ROUTE64 6U                      ///< Route64 TLV
#define OT_MESH_MON_TLV_LINK_MARGIN_IN 7U               ///< Link Margin In TLV
#define OT_MESH_MON_TLV_MAC_LINK_ERROR_RATES_TX 8U      ///< MAC Link Error Rates Tx TLV
#define OT_MESH_MON_TLV_MLEID 9U                        ///< ML-EID TLV
#define OT_MESH_MON_TLV_IP6_ADDRESS_LIST 10U            ///< IPv6 Address List TLV
#define OT_MESH_MON_TLV_ALOC_LIST 11U                   ///< ALOC List TLV
#define OT_MESH_MON_TLV_THREAD_SPEC_VERSION 12U         ///< Thread Specification Version TLV
#define OT_MESH_MON_TLV_THREAD_STACK_VERSION 13U        ///< Thread Stack Version TLV
#define OT_MESH_MON_TLV_VENDOR_NAME 14U                 ///< Vendor Name TLV
#define OT_MESH_MON_TLV_VENDOR_MODEL 15U                ///< Vendor Model TLV
#define OT_MESH_MON_TLV_VENDOR_SW_VERSION 16U           ///< Vendor Software Version TLV
#define OT_MESH_MON_TLV_VENDOR_APP_URL 17U              ///< Vendor App URL TLV
#define OT_MESH_MON_TLV_IP6_LINK_LOCAL_ADDRESS_LIST 18U ///< IPv6 Link-Local Address List TLV
#define OT_MESH_MON_TLV_EUI64 19U                       ///< EUI-64 TLV
#define OT_MESH_MON_TLV_MAC_COUNTERS 20U                ///< MAC Counters TLV
#define OT_MESH_MON_TLV_MAC_LINK_ERROR_RATES_RX 21U     ///< MAC Link Error Rates Rx TLV
#define OT_MESH_MON_TLV_MLE_COUNTERS 22U                ///< MLE Counters TLV
#define OT_MESH_MON_TLV_LINK_MARGIN_OUT 23U             ///< Link Margin Out TLV

#define OT_MESH_MON_DATA_TLV_MAX 23U ///< The highest known tlv value that can be requested using a request set.

#define OT_MESH_MON_ITERATOR_INIT 0 ///< Initializer for `otMeshMonIterator`.

typedef uint16_t otMeshMonIterator; ///< Used to iterate through Device Contexts in a message.

/**
 * The size in bytes of the tlv set bitset.
 *
 * Will be the smallest multiple of 4 that can contain all tlvs.
 */
#define OT_MESH_MON_TLV_SET_SIZE (((OT_MESH_MON_DATA_TLV_MAX + 31U) & ~31U) / 8U)

/**
 * Represents a set of Mesh Monitor TLV types stored as a bitmask.
 */
typedef struct otMeshMonTlvSet
{
    uint8_t mFields[OT_MESH_MON_TLV_SET_SIZE]; ///< Reserved for internal use. DO NOT MODIFY.
} otMeshMonTlvSet;

/**
 * Represents a device context within a Mesh Monitor update.
 *
 * A server update may contain multiple contexts, one per reported device
 * instance (e.g. one context per child, one per neighbor). Each context
 * groups the set of TLVs that describe that particular device.
 *
 * A context may be flagged as `legacy` (see `mLegacy`). A legacy context
 * describes a child that does not support the Mesh Monitor protocol. Such a
 * child is still reported by its parent router, but the mesh monitoring TLVs
 * it would normally provide may be unavailable. In that case a client may fall
 * back to a Network Diagnostic request/query to obtain the missing data.
 */
typedef struct otMeshMonContext
{
    uint16_t mRloc16;     ///< The Rloc16 of the device.
    uint8_t  mType;       ///< The Type of the device (otMeshMonDeviceType).
    uint8_t  mUpdateMode; ///< The update mode of the context (otMeshMonUpdateMode). Only valid if child or neighbor.
    bool     mLegacy;     ///< TRUE if the child does not support Mesh Monitor (its TLVs may be unavailable). Only valid
                          ///< if `mType` is child.
    uint16_t mData;       ///< Reserved for internal use. DO NOT MODIFY.
    uint16_t mData2;      ///< Reserved for internal use. DO NOT MODIFY.
} otMeshMonContext;

/**
 * Represents the CSL TLV value.
 *
 * This is a compact form of the CSL Channel information: it carries only the
 * 8-bit channel value and assumes the default 2.4 GHz IEEE 802.15.4 channel
 * page (i.e. no separate Channel Page field). This matches the channels used
 * in Thread today. Newer Mesh Monitor CSL TLVs may be defined in the future if
 * other PHYs / channel pages need to be reported.
 */
typedef struct otMeshMonCsl
{
    uint32_t mTimeout; ///< CSL timeout in seconds
    uint16_t mPeriod;  ///< CSL period
    uint8_t  mChannel; ///< CSL channel (2.4 GHz IEEE 802.15.4 channel page assumed)
} otMeshMonCsl;

/**
 * Iterator over an IPv6 address list TLV value (used for `OT_MESH_MON_TLV_IP6_ADDRESS_LIST`
 * and `OT_MESH_MON_TLV_IP6_LINK_LOCAL_ADDRESS_LIST`).
 *
 * Use `otMeshMonGetNextIp6Address()` to walk through the addresses. Treat the
 * fields as opaque.
 */
typedef struct otMeshMonIp6AddrIterator
{
    uint16_t mOffset; ///< Reserved for internal use. DO NOT MODIFY.
    uint16_t mLength; ///< Reserved for internal use. DO NOT MODIFY.
} otMeshMonIp6AddrIterator;

/**
 * Iterator over a list of ALOC16 (anycast locator) of device TLV value (used for `OT_MESH_MON_TLV_ALOC_LIST`).
 *
 * Use `otMeshMonGetNextAloc()` to walk through the entries. Treat the fields
 * as opaque.
 */
typedef struct otMeshMonAlocIterator
{
    uint16_t mOffset; ///< Reserved for internal use. DO NOT MODIFY.
    uint16_t mLength; ///< Reserved for internal use. DO NOT MODIFY.
} otMeshMonAlocIterator;

/**
 * Represents a Link Margin TLV value.
 *
 * - `OT_MESH_MON_TLV_LINK_MARGIN_IN` reports the link margin of frames received by
 *   this device from its peer.
 * - `OT_MESH_MON_TLV_LINK_MARGIN_OUT` reports the link margin of frames transmitted
 *   by this device to its peer, as observed by the peer.
 */
typedef struct otMeshMonLinkMargin
{
    uint8_t mLinkMargin;  ///< Average Link margin in dB
    int8_t  mAverageRssi; ///< Average RSSI in dBm
    int8_t  mLastRssi;    ///< Last RSSI in dBm
} otMeshMonLinkMargin;

/**
 * Represents a MAC Link Error Rates TLV value.
 *
 * - `OT_MESH_MON_TLV_MAC_LINK_ERROR_RATES_TX` reports the error rate of
 *   frames transmitted by this device to its peer.
 * - `OT_MESH_MON_TLV_MAC_LINK_ERROR_RATES_RX` reports the error rate of
 *   frames received by this device from its peer.
 */
typedef struct otMeshMonMacLinkErrorRates
{
    uint16_t mMessageErrorRate; ///< Message error rate (0x0000-0xFFFF = 100%)
    uint16_t mFrameErrorRate;   ///< Frame error rate (0x0000-0xFFFF = 100%)
} otMeshMonMacLinkErrorRates;

/**
 * Represents a Mesh Monitor TLV.
 */
typedef struct otMeshMonTlv
{
    uint8_t mType; ///< The Mesh Monitor TLV type.

    union
    {
        otExtAddress               mExtAddress;
        otLinkModeConfig           mMode;
        uint32_t                   mTimeout;
        uint32_t                   mLastHeard;
        uint32_t                   mConnectionTime;
        otMeshMonCsl               mCsl;
        otNetworkDiagRoute         mRoute;
        otIp6InterfaceIdentifier   mMlEid;
        otMeshMonIp6AddrIterator   mIp6AddressList;
        otMeshMonAlocIterator      mAlocList;
        uint16_t                   mThreadSpecVersion;
        char                       mThreadStackVersion[OT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH + 1];
        char                       mVendorName[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH + 1];
        char                       mVendorModel[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH + 1];
        char                       mVendorSwVersion[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH + 1];
        char                       mVendorAppUrl[OT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH + 1];
        otMeshMonIp6AddrIterator   mIp6LinkLocalAddressList;
        otExtAddress               mEui64;
        otMeshMonLinkMargin        mLinkMarginIn;
        otMeshMonMacLinkErrorRates mMacLinkErrorRatesTx;
        otNetworkDiagMacCounters   mMacCounters;
        otMeshMonMacLinkErrorRates mMacLinkErrorRatesRx;
        otNetworkDiagMleCounters   mMleCounters;
        otMeshMonLinkMargin        mLinkMarginOut;
    } mData;
} otMeshMonTlv;

/**
 * Pointer is called when a Mesh Monitor server update message is received.
 *
 * @param[in] aMessage   A pointer to the message containing the update TLVs.
 * @param[in] aRloc16    The RLOC16 of the device that sent the update.
 * @param[in] aComplete  TRUE if this is the final update in the sequence, FALSE otherwise.
 * @param[in] aContext   A pointer to application-specific context.
 */
typedef void (*otMeshMonServerUpdateCallback)(const otMessage *aMessage,
                                              uint16_t         aRloc16,
                                              bool             aComplete,
                                              void            *aContext);

/**
 * Gets the next Device Context in the message.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage   A pointer to the message.
 * @param[in,out] aIterator  A pointer to the iterator context. To get the first context it should be set to
 * OT_MESH_MON_ITERATOR_INIT.
 * @param[out]    aContext   A pointer to where the Device Context information will be stored. Also acts as an iterator
 * over TLVs in the context.
 *
 * @retval OT_ERROR_NONE       Successfully found the next Device Context.
 * @retval OT_ERROR_NOT_FOUND  No subsequent Device Context exists in the message.
 * @retval OT_ERROR_PARSE      Parsing the next Device Context failed.
 *
 * @Note A subsequent call to this function is only allowed when the current return value is OT_ERROR_NONE.
 */
otError otMeshMonGetNextContext(const otMessage *aMessage, otMeshMonIterator *aIterator, otMeshMonContext *aContext);

/**
 * Gets the next Mesh Monitor TLV in a Device Context.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage   A pointer to the message.
 * @param[in,out] aContext   A pointer to the device context. Only iterator state in the context will be modified.
 * @param[out]    aTlv       A pointer to where TLV information will be stored.
 *
 * @retval OT_ERROR_NONE       Successfully found the next TLV.
 * @retval OT_ERROR_NOT_FOUND  No subsequent TLV exists in the device context.
 * @retval OT_ERROR_PARSE      Parsing the next TLV failed.
 *
 * @Note A subsequent call to this function is only allowed when the current return value is OT_ERROR_NONE.
 */
otError otMeshMonGetNextTlv(const otMessage *aMessage, otMeshMonContext *aContext, otMeshMonTlv *aTlv);

/**
 * Gets the next IPv6 address from an IPv6 address list TLV value.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage   A pointer to the message.
 * @param[in,out] aIterator  A pointer to the iterator state from the TLV value.
 * @param[out]    aAddress   A pointer to where the IPv6 address will be stored.
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the next address.
 * @retval OT_ERROR_NOT_FOUND  No more addresses in the list.
 * @retval OT_ERROR_PARSE      Failed to parse an address.
 *
 * @Note A subsequent call to this function is only allowed when the current return value is OT_ERROR_NONE.
 */
otError otMeshMonGetNextIp6Address(const otMessage          *aMessage,
                                   otMeshMonIp6AddrIterator *aIterator,
                                   otIp6Address             *aAddress);

/**
 * Gets the next ALOC16 entry from an ALOC16 list TLV value.
 *
 * Each entry on the wire is a single byte (the locator portion of the
 * anycast RLOC16). The returned value is the full 16-bit ALOC16.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage   A pointer to the message.
 * @param[in,out] aIterator  A pointer to the iterator state from the TLV value.
 * @param[out]    aAloc      A pointer to where the ALOC16 will be stored.
 *
 * @retval OT_ERROR_NONE       Successfully retrieved the next entry.
 * @retval OT_ERROR_NOT_FOUND  No more entries in the list.
 * @retval OT_ERROR_PARSE      Failed to parse an entry.
 *
 * @Note A subsequent call to this function is only allowed when the current return value is OT_ERROR_NONE.
 */
otError otMeshMonGetNextAloc(const otMessage *aMessage, otMeshMonAlocIterator *aIterator, uint16_t *aAloc);

/**
 * Starts the Mesh Monitor client.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * Collects mesh monitoring TLVs from the Thread network. When @p aDestination is NULL the client
 * registers with all routers across the whole Thread partition. How this is done
 * depends on the device type:
 *  - An FTD client has a router table, so it sends a unicast registration request to each known
 *    router's RLOC.
 *  - An MTD client has no router table, so it sends a single registration request to the
 *    realm-local all-routers multicast address.
 * When @p aDestination is non-NULL, registration and query requests are sent unicast to that
 * specific IPv6 address only.
 *
 * Each of @p aHost, @p aChild and @p aNeighbor may be NULL. A NULL set is treated as an empty set,
 * meaning no TLVs are requested for that device context type.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 * @param[in] aHost         TLV set for host contexts. May be NULL (treated as an empty set).
 * @param[in] aChild        TLV set for child contexts. May be NULL (treated as an empty set).
 * @param[in] aNeighbor     TLV set for neighbor contexts. May be NULL (treated as an empty set).
 * @param[in] aDestination  Optional unicast destination. NULL means realm-local all-routers multicast.
 * @param[in] aCallback     Callback invoked when mesh monitoring information is received.
 * @param[in] aContext      Application-specific context passed to the callback.
 *
 * @retval OT_ERROR_NONE          Successfully started the Mesh Monitor client.
 * @retval OT_ERROR_INVALID_ARGS  @p aCallback is NULL, or @p aDestination is non-NULL but is not a valid
 *                                unicast address.
 */
otError otMeshMonStartClient(otInstance                   *aInstance,
                             const otMeshMonTlvSet        *aHost,
                             const otMeshMonTlvSet        *aChild,
                             const otMeshMonTlvSet        *aNeighbor,
                             const otIp6Address           *aDestination,
                             otMeshMonServerUpdateCallback aCallback,
                             void                         *aContext);

/**
 * Stops the Mesh Monitor Client and prevents all calls to any previously registered
 * callback.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * @param[in]  aInstance  The openthread instance.
 */
void otMeshMonStopClient(otInstance *aInstance);

/**
 * Checks if a specific TLV type is set in the Mesh Monitor TLV set.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * @param[in] aTlvSet  A pointer to the Mesh Monitor TLV set to query.
 *                     Must not be a null pointer.
 * @param[in] aTlv     The TLV type identifier to check for in the set.
 *                     Must be a known/valid TLV type.
 *
 * @retval true   The specified TLV type is set in the TLV set.
 * @retval false  The specified TLV type is not set, the set pointer is a null pointer,
 *                or the TLV type is not recognized.
 */
bool otMeshMonTlvIsSet(const otMeshMonTlvSet *aTlvSet, uint8_t aTlv);

/**
 * Sets the bit for a TLV in the provided TLV set.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * @param[in] aTlvSet  The TlvSet to modify.
 * @param[in] aTlv     The TLV id to set.
 *
 * @retval OT_ERROR_NONE          If the bit was successfully set, including if it was already set.
 * @retval OT_ERROR_INVALID_ARGS  If the TLV specified is not a known TLV or aTlvSet is NULL.
 */
otError otMeshMonSetTlv(otMeshMonTlvSet *aTlvSet, uint8_t aTlv);

/**
 * Clears a specific TLV from the Mesh Monitor TLV set.
 *
 * Requires `OPENTHREAD_CONFIG_MESH_MONITOR_CLIENT_ENABLE`.
 *
 * @param[in,out] aTlvSet  A pointer to the Mesh Monitor TLV set to modify.
 *                         If NULL, the function returns without performing any operation.
 * @param[in]     aTlv     The TLV type to clear from the set. Must be a known TLV type.
 *
 * @note If the TLV set pointer is NULL or the TLV type is unknown, the function
 *       returns early without modifying the TLV set.
 *
 */
void otMeshMonClearTlv(otMeshMonTlvSet *aTlvSet, uint8_t aTlv);

/**
 * Clears all TLVs from a Mesh Monitor TLV set.
 *
 * @param[in,out] aTlvSet  A pointer to the Mesh Monitor TLV set to clear.
 *                         If NULL, the function returns without performing any operation.
 */
void otMeshMonClearTlvSet(otMeshMonTlvSet *aTlvSet);

/**
 * Checks whether a Mesh Monitor TLV set has no TLVs set.
 *
 * @param[in] aTlvSet  A pointer to the Mesh Monitor TLV set to query.
 *
 * @retval true   The set is empty, or @p aTlvSet is NULL.
 * @retval false  The set contains at least one TLV.
 */
bool otMeshMonTlvSetIsEmpty(const otMeshMonTlvSet *aTlvSet);

/**
 * Adds all TLVs from a source set into a destination set (set union, @p aDst |= @p aSrc).
 *
 * @param[in,out] aDst  A pointer to the destination Mesh Monitor TLV set to modify.
 *                      If NULL, the function returns without performing any operation.
 * @param[in]     aSrc  A pointer to the source Mesh Monitor TLV set to add.
 *                      If NULL, the function returns without performing any operation.
 */
void otMeshMonTlvSetUnion(otMeshMonTlvSet *aDst, const otMeshMonTlvSet *aSrc);

/**
 * Checks whether two Mesh Monitor TLV sets contain the same TLVs.
 *
 * @param[in] aFirst   A pointer to the first Mesh Monitor TLV set.
 * @param[in] aSecond  A pointer to the second Mesh Monitor TLV set.
 *
 * @retval true   Both sets contain the same TLVs, or both pointers are NULL.
 * @retval false  The sets differ, or exactly one pointer is NULL.
 */
bool otMeshMonTlvSetsAreEqual(const otMeshMonTlvSet *aFirst, const otMeshMonTlvSet *aSecond);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_MESH_MONITOR_H_
