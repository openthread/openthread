/*
 *  Copyright (c) 2025, The OpenThread Authors.
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

#ifndef OPENTHREAD_EXT_NETWORK_DIAGNOSTIC_H_
#define OPENTHREAD_EXT_NETWORK_DIAGNOSTIC_H_

#include <stdbool.h>
#include <stdint.h>
#include <openthread/thread.h>
#include "openthread/error.h"
#include "openthread/instance.h"
#include "openthread/ip6.h"
#include "openthread/message.h"
#include "openthread/platform/radio.h"
#include "openthread/platform/toolchain.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-general
 *
 * @{
 */

#define OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_HOST 0U     ///< Host Device Context type
#define OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_CHILD 1U    ///< Child Device Context type
#define OT_EXT_NETWORK_DIAGNOSTIC_DEVICE_NEIGHBOR 2U ///< Neighbor Device Context type

#define OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_ADDED 0U   ///< Added Update Mode
#define OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_UPDATE 1U  ///< Update Update Mode
#define OT_EXT_NETWORK_DIAGNOSTIC_UPDATE_MODE_REMOVED 2U ///< Removed Update Mode

#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_MAC_ADDRESS 0U     ///< MAC Address TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_MODE 1U            ///< Mode TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_TIMEOUT 2U         ///< Timeout TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_LAST_HEARD 3U      ///< Last Heard TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_CONNECTION_TIME 4U ///< Connection Time TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_CSL 5U             ///< CSL TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_ROUTE64 6U         ///< Route64 TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_LINK_MARGIN_IN 7U  ///< Link Margin In TLV

#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_MAC_LINK_ERROR_RATES_OUT 8U ///< Mac Link Error Rates Out TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_MLEID 13U                   ///< MlEid TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDRESS_LIST 14U        ///< Ip6 Address List TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_ALOC_LIST 15U               ///< ALOC List TLV

#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_THREAD_SPEC_VERSION 16U         ///< Thread Spec Version TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION 17U        ///< Thread Stack Version TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME 18U                 ///< Vendor Name TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL 19U                ///< Vendor Model TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION 20U           ///< Vendor Software Version TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL 21U              ///< Vendor App URL TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_IP6_LINK_LOCAL_ADDRESS_LIST 22U ///< Ip6 Link Local Address List TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_EUI64 23U                       ///< EUI64 TLV

#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS 24U            ///< Mac Counters TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_MAC_LINK_ERROR_RATES_IN 25U ///< Mac Link Error Rates In TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS 26U            ///< Mle Counters TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_LINK_MARGIN_OUT 27U         ///< Link Margin Out TLV

#define OT_EXT_NETWORK_DIAGNOSTIC_DATA_TLV_MAX \
    27U ///< The highest known tlv value that can be requested using a request set.

#define OT_EXT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH 64 ///< Max length of the Thread Stack Version TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH 32          ///< Max lenght of the Vendor Name TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH 32         ///< Max langth of the Vendor Model TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH 32 ///< Max length of the Vendor Software Version TLV
#define OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH 96    ///< Max length of the Vendor App URL TLV

#define OT_EXT_NETWORK_DIAGNOSTIC_ITERATOR_INIT 0 ///< Initializer for `otExtNetworkDiagnosticIterator`.

typedef uint16_t otExtNetworkDiagnosticIterator; ///< Used to iterate through Device Contexts in a message.

/**
 * The size in bytes of the tlv set bitset.
 *
 * Will be the smallest multiple of 4 that can contain all tlvs.
 */
#define OT_EXT_NETWORK_DIAGNOSTIC_TLV_SET_SIZE (((OT_EXT_NETWORK_DIAGNOSTIC_DATA_TLV_MAX + 31U) & ~31U) / 8U)

/**
 * Bitset of Extended Network Diagnostic TLVs.
 *
 * Bit for a TLV can be determined as follows:
 * `m8[tlv / 8] & (1 << (tlv % 8))`
 */
OT_TOOL_PACKED_BEGIN
struct otExtNetworkDiagnosticTlvSet
{
    union OT_TOOL_PACKED_FIELD
    {
        uint8_t  m8[OT_EXT_NETWORK_DIAGNOSTIC_TLV_SET_SIZE];
        uint16_t m16[OT_EXT_NETWORK_DIAGNOSTIC_TLV_SET_SIZE / sizeof(uint16_t)];
        uint32_t m32[OT_EXT_NETWORK_DIAGNOSTIC_TLV_SET_SIZE / sizeof(uint32_t)];
    };
} OT_TOOL_PACKED_END;

typedef struct otExtNetworkDiagnosticTlvSet otExtNetworkDiagnosticTlvSet;

/**
 * Represents information about a device context.
 */
typedef struct otExtNetworkDiagnosticContext
{
    uint16_t mRloc16; ///< The Rloc16 of the device.

    uint8_t mType;       ///< The Type of the device.
    uint8_t mUpdateMode; ///< The update mode of the context. Only valid if `mType` is child or neighbor.
    bool    mLegacy;     ///< The legacy flag of the context. Only valid if `mType` is child.

    uint16_t mTlvIterator;    ///< Iterator state for TLV iteration. DO NOT MODIFY.
    uint16_t mTlvIteratorEnd; ///< Iterator state for TLV iteration. DO NOT MODIFY.
} otExtNetworkDiagnosticContext;

/**
 * Represents an Extended Network Diagnostic TLV.
 */
typedef struct otExtNetworkDiagnosticTlv
{
    uint8_t mType; ///< The Extended Network Diagnostic TLV type.

    union
    {
        otExtAddress     mExtAddress;
        otLinkModeConfig mMode;
        uint32_t         mTimeout;
        uint32_t         mLastHeard;
        uint32_t         mConnectionTime;
        struct
        {
            uint32_t mTimeout;
            uint16_t mPeriod;
            uint8_t  mChannel;
        } mCsl;

        struct
        {
            uint8_t  mRouterIdSequence;
            uint8_t  mRouterIdMask[8];
            uint8_t  mRouterCount;
            uint16_t mDataOffset;
        } mRoute64;

        otIp6InterfaceIdentifier mMlEid;
        struct
        {
            uint8_t  mCount;
            uint16_t mDataOffset;
        } mIp6AddressList;
        struct
        {
            uint8_t  mCount;
            uint16_t mDataOffset;
        } mAlocList;

        uint16_t mThreadSpecVersion;
        char     mThreadStackVersion[OT_EXT_NETWORK_DIAGNOSTIC_MAX_THREAD_STACK_VERSION_TLV_LENGTH + 1];
        char     mVendorName[OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_NAME_TLV_LENGTH + 1];
        char     mVendorModel[OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_MODEL_TLV_LENGTH + 1];
        char     mVendorSwVersion[OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_SW_VERSION_TLV_LENGTH + 1];
        char     mVendorAppUrl[OT_EXT_NETWORK_DIAGNOSTIC_MAX_VENDOR_APP_URL_TLV_LENGTH + 1];
        struct
        {
            uint8_t  mCount;
            uint16_t mDataOffset;
        } mIp6LinkLocalAddressList;
        otExtAddress mEui64;

        struct
        {
            uint8_t mLinkMargin;
            int8_t  mAverageRssi;
            int8_t  mLastRssi;
        } mLinkMarginIn;

        struct
        {
            uint8_t mMessageErrorRate;
            uint8_t mFrameErrorRate;
        } mMacLinkErrorRatesOut;

        struct
        {
            uint32_t mIfInUnknownProtos;
            uint32_t mIfInErrors;
            uint32_t mIfOutErrors;
            uint32_t mIfInUcastPkts;
            uint32_t mIfInBroadcastPkts;
            uint32_t mIfInDiscards;
            uint32_t mIfOutUcastPkts;
            uint32_t mIfOutBroadcastPkts;
            uint32_t mIfOutDiscards;
        } mMacCounters;

        struct
        {
            uint8_t mMessageErrorRate;
            uint8_t mFrameErrorRate;
        } mMacLinkErrorRatesIn;

        struct
        {
            uint16_t mDisabledRole;
            uint16_t mDetachedRole;
            uint16_t mChildRole;
            uint16_t mRouterRole;
            uint16_t mLeaderRole;
            uint16_t mAttachAttempts;
            uint16_t mPartitionIdChanges;
            uint16_t mBetterPartitionAttachAttempts;
            uint16_t mParentChanges;
            uint64_t mTrackedTime;
            uint64_t mDisabledTime;
            uint64_t mDetachedTime;
            uint64_t mChildTime;
            uint64_t mRouterTime;
            uint64_t mLeaderTime;
        } mMleCounters;

        struct
        {
            uint8_t mLinkMargin;
            int8_t  mAverageRssi;
            int8_t  mLastRssi;
        } mLinkMarginOut;

    } mData;
} otExtNetworkDiagnosticTlv;

typedef void (*otExtNetworkDiagnosticServerUpdateCallback)(const otMessage *aMessage,
                                                           uint16_t         aRloc16,
                                                           bool             aComplete,
                                                           void            *aContext);

/**
 * Represents a single route entry from the Route64 TLV.
 *
 * Each entry corresponds to a router ID that has its bit set in the Router ID Mask.
 */
typedef struct otExtNetworkDiagnosticRouteData
{
    uint8_t mRouterId;
    uint8_t mLinkQualityOut;
    uint8_t mLinkQualityIn;
    uint8_t mRouteCost;
} otExtNetworkDiagnosticRouteData;

/**
 * Gets the route data entries for a Route64 TLV.
 *
 * Requires `OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage     A pointer to the message.
 * @param[in]     aDataOffset  The `mDataOffset` value returned in the `otExtNetworkDiagnosticTlv` struct for this TLV.
 * @param[in]     aRouterIdMask The router ID mask from the TLV (8 bytes).
 * @param[in]     aCount       The number of route entries to read. Must be less or equal than `mRouterCount`.
 * @param[out]    aRouteData   A pointer to an array where the route data entries will be stored.
 *
 * @retval OT_ERROR_NONE          Successfully read the route data.
 * @retval OT_ERROR_PARSE         Parsing the message failed.
 * @retval OT_ERROR_INVALID_ARGS  If `aCount` is not 0 and `aRouteData` is NULL.
 */
otError otExtNetworkDiagnosticGetRouteData(const otMessage                 *aMessage,
                                           uint16_t                         aDataOffset,
                                           const uint8_t                   *aRouterIdMask,
                                           uint8_t                          aCount,
                                           otExtNetworkDiagnosticRouteData *aRouteData);

/**
 * Gets the next Device Context in the message.
 *
 * Requires `OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage   A pointer to the message.
 * @param[in,out] aIterator  A pointer to the iterator context. To get the first context it should be set to
 * OT_EXT_NETWORK_DIAGNOSTIC_ITERATOR_INIT.
 * @param[out]    aContext   A pointer to where the Device Context information will be stored. Also acts as an iterator
 * over TLVs in the context.
 *
 * @retval OT_ERROR_NONE       Successfully found the next Device Context.
 * @retval OT_ERROR_NOT_FOUND  No subsequent Device Context exists in the message.
 * @retval OT_ERROR_PARSE      Parsing the next Device Context failed.
 *
 * @Note A subsequent call to this function is only allowed when the current return value is OT_ERROR_NONE.
 */
otError otExtNetworkDiagnosticGetNextContext(const otMessage                *aMessage,
                                             otExtNetworkDiagnosticIterator *aIterator,
                                             otExtNetworkDiagnosticContext  *aContext);

/**
 * Gets the next Extended Network Diagnostic TLV in a Device Context.
 *
 * Requires `OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE`.
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
otError otExtNetworkDiagnosticGetNextTlv(const otMessage               *aMessage,
                                         otExtNetworkDiagnosticContext *aContext,
                                         otExtNetworkDiagnosticTlv     *aTlv);

/**
 * Gets the Ip6 address list for a Ip6AddressList or Ip6LinkLocalAddressList tlv.
 *
 * Requires `OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage     A pointer to the message.
 * @param[in]     aDataOffset  The `mDataOffset` value returned in the `otExtNetworkDiagnosticTlv` struct for this tlv.
 * @param[in]     aCount       The number of Ip6Addresses to read. Must be less or equal than `mCount` returned in the
 * `otExtNetworkDiagnosticTlv` struct for this tlv.
 * @param[out] aAddresses   A pointer to a `aCount` sized array where the ip6 addresses will be stored in.
 *
 * @retval OT_ERROR_NONE   Successfully read the ip6 addresses.
 * @retval OT_ERROR_PARSE  Parsing the message failed.
 * @retval OT_ERROR_INVALID_ARGS  If `aCount` is not 0 and `aAddresses` is NULL.
 */
otError otExtNetworkDiagnosticGetIp6Addresses(const otMessage *aMessage,
                                              uint16_t         aDataOffset,
                                              uint16_t         aCount,
                                              otIp6Address    *aAddresses);

/**
 * Gets the aloc list for a AlocList tlv.
 *
 * Requires `OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage     A pointer to the message.
 * @param[in]     aDataOffset  The `mDataOffset` value returned in the `otExtNetworkDiagnosticTlv` struct for this tlv.
 * @param[in]     aCount       The number of Alocs to read. Must be less or equal than `mCount` returned in the
 * `otExtNetworkDiagnosticTlv` struct for this tlv.
 * @param[out] aAlocs       A pointer to a `aCount` sized array where the Alocs will be stored in.
 *
 * @retval OT_ERROR_NONE   Successfully read the alocs.
 * @retval OT_ERROR_PARSE  Parsing the message failed.
 * @retval OT_ERROR_INVALID_ARGS  If `aCount` is not 0 and `aAlocs` is NULL.
 */
otError otExtNetworkDiagnosticGetAlocs(const otMessage *aMessage,
                                       uint16_t         aDataOffset,
                                       uint16_t         aCount,
                                       uint8_t         *aAlocs);

/**
 * Starts the Extended Network Diagnostic client.
 *
 * This function initiates the network diagnostic client to collect diagnostic information
 * from the Thread network based on the specified TLV sets for host, child, and neighbor nodes.
 *
 * @param[in] aInstance   A pointer to an OpenThread instance.
 * @param[in] aHost       A pointer to the TLV set for host diagnostic information to collect.
 *                        May be NULL if no host diagnostics are requested.
 * @param[in] aChild      A pointer to the TLV set for child diagnostic information to collect.
 *                        May be NULL if no child diagnostics are requested.
 * @param[in] aNeighbor   A pointer to the TLV set for neighbor diagnostic information to collect.
 *                        May be NULL if no neighbor diagnostics are requested.
 * @param[in] aCallback   A pointer to a callback function that is invoked when diagnostic
 *                        information is received from the server.
 * @param[in] aContext    A pointer to application-specific context information that will be
 *                        passed to the callback function.
 *
 */
void otExtNetworkDiagnosticStartClient(otInstance                                *aInstance,
                                       const otExtNetworkDiagnosticTlvSet        *aHost,
                                       const otExtNetworkDiagnosticTlvSet        *aChild,
                                       const otExtNetworkDiagnosticTlvSet        *aNeighbor,
                                       otExtNetworkDiagnosticServerUpdateCallback aCallback,
                                       void                                      *aContext);

/**
 * Stops the Extended Network Diagnostic Client and prevents all calls to any previously registered
 * callback.
 *
 * @param[in]  aInstance  The openthread instance.
 *
 * Requires `OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE`.
 */
void otExtNetworkDiagnosticStopClient(otInstance *aInstance);

/**
 * Checks if a specific TLV (Type-Length-Value) type is set in the network diagnostic TLV set.
 *
 * This function verifies whether a given TLV type is present in the extended network diagnostic
 * TLV set. It performs validation to ensure the set pointer is valid and the TLV type is known
 * before checking if the TLV is set.
 *
 * @param[in] aTlvSet  A pointer to the extended network diagnostic TLV set to query.
 *                     Must not be nullptr.
 * @param[in] aTlv     The TLV type identifier to check for in the set.
 *                     Must be a known/valid TLV type.
 *
 * @retval true   The specified TLV type is set in the TLV set.
 * @retval false  The specified TLV type is not set, the set pointer is nullptr,
 *                or the TLV type is not recognized.
 */
bool otExtNetworkDiagnosticTlvIsSet(const otExtNetworkDiagnosticTlvSet *aTlvSet, uint8_t aTlv);

/**
 * Sets the bit for a tlv in the provided tlv set.
 *
 * Requires `OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE`.
 *
 * @param[in] aTlvSet  The TlvSet to modify.
 * @param[in] aTlv     The Tlv id to set.
 *
 * @retval OT_ERROR_NONE          If the bit was successfully set, including if it was already set.
 * @retval OT_ERROR_INVALID_ARGS  If the tlv specified is not a known tlv or aTlvSet is NULL.
 */
otError otExtNetworkDiagnosticSetTlv(otExtNetworkDiagnosticTlvSet *aTlvSet, uint8_t aTlv);

/**
 * Clears a specific TLV from the Extended Network Diagnostic TLV set.
 *
 * This function removes a specified TLV type from the given Extended Network Diagnostic TLV set.
 * The function validates that the TLV set pointer is not null and that the TLV type
 * is a known/valid TLV before attempting to clear it.
 *
 * @param[in,out] aTlvSet  A pointer to the Extended Network Diagnostic TLV set to modify.
 *                         If NULL, the function returns without performing any operation.
 * @param[in]     aTlv     The TLV type to clear from the set. Must be a known TLV type.
 *
 * @note If the TLV set pointer is NULL or the TLV type is unknown, the function
 *       returns early without modifying the TLV set.
 *
 */
void otExtNetworkDiagnosticClearTlv(otExtNetworkDiagnosticTlvSet *aTlvSet, uint8_t aTlv);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_EXT_NETWORK_DIAGNOSTIC_H_
