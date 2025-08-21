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

#ifndef OPENTHREAD_DIAG_SERVER_H_
#define OPENTHREAD_DIAG_SERVER_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/thread.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>
#include "openthread/ip6.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-general
 *
 * @{
 */

#define OT_DIAG_SERVER_DEVICE_HOST 0U     ///< Host Device Context type
#define OT_DIAG_SERVER_DEVICE_CHILD 1U    ///< Child Device Context type
#define OT_DIAG_SERVER_DEVICE_NEIGHBOR 2U ///< Neighbor Device Context type

#define OT_DIAG_SERVER_UPDATE_MODE_ADDED 0U   ///< Added Update Mode
#define OT_DIAG_SERVER_UPDATE_MODE_UPDATE 1U  ///< Update Update Mode
#define OT_DIAG_SERVER_UPDATE_MODE_REMOVED 2U ///< Removed Update Mode

#define OT_DIAG_SERVER_TLV_MAC_ADDRESS 0U     ///< MAC Address TLV
#define OT_DIAG_SERVER_TLV_MODE 1U            ///< Mode TLV
#define OT_DIAG_SERVER_TLV_TIMEOUT 2U         ///< Timeout TLV
#define OT_DIAG_SERVER_TLV_LAST_HEARD 3U      ///< Last Heard TLV
#define OT_DIAG_SERVER_TLV_CONNECTION_TIME 4U ///< Connection Time TLV
#define OT_DIAG_SERVER_TLV_CSL 5U             ///< CSL TLV
#define OT_DIAG_SERVER_TLV_ROUTE64 6U         ///< Route64 TLV
#define OT_DIAG_SERVER_TLV_LINK_MARGIN_IN 7U  ///< Link Margin In TLV

#define OT_DIAG_SERVER_TLV_MAC_LINK_ERROR_RATES_OUT 8U ///< Mac Link Error Rates Out TLV
#define OT_DIAG_SERVER_TLV_MLEID 13U                   ///< MlEid TLV
#define OT_DIAG_SERVER_TLV_IP6_ADDRESS_LIST 14U        ///< Ip6 Address List TLV
#define OT_DIAG_SERVER_TLV_ALOC_LIST 15U               ///< ALOC List TLV

#define OT_DIAG_SERVER_TLV_THREAD_SPEC_VERSION 16U         ///< Thread Spec Version TLV
#define OT_DIAG_SERVER_TLV_THREAD_STACK_VERSION 17U        ///< Thread Stack Version TLV
#define OT_DIAG_SERVER_TLV_VENDOR_NAME 18U                 ///< Vendor Name TLV
#define OT_DIAG_SERVER_TLV_VENDOR_MODEL 19U                ///< Vendor Model TLV
#define OT_DIAG_SERVER_TLV_VENDOR_SW_VERSION 20U           ///< Vendor Software Version TLV
#define OT_DIAG_SERVER_TLV_VENDOR_APP_URL 21U              ///< Vendor App URL TLV
#define OT_DIAG_SERVER_TLV_IP6_LINK_LOCAL_ADDRESS_LIST 22U ///< Ip6 Link Local Address List TLV
#define OT_DIAG_SERVER_TLV_EUI64 23U                       ///< EUI64 TLV

#define OT_DIAG_SERVER_TLV_MAC_COUNTERS 24U            ///< Mac Counters TLV
#define OT_DIAG_SERVER_TLV_MAC_LINK_ERROR_RATES_IN 25U ///< Mac Link Error Rates In TLV
#define OT_DIAG_SERVER_TLV_MLE_COUNTERS 26U            ///< Mle Counters TLV
#define OT_DIAG_SERVER_TLV_LINK_MARGIN_OUT 27U         ///< Link Margin Out TLV

#define OT_DIAG_SERVER_DATA_TLV_MAX 27U ///< The highest known tlv value that can be requested using a request set.

#define OT_DIAG_SERVER_MAX_THREAD_STACK_VERSION_TLV_LENGTH 64 ///< Max length of the Thread Stack Version TLV
#define OT_DIAG_SERVER_MAX_VENDOR_NAME_TLV_LENGTH 32          ///< Max lenght of the Vendor Name TLV
#define OT_DIAG_SERVER_MAX_VENDOR_MODEL_TLV_LENGTH 32         ///< Max langth of the Vendor Model TLV
#define OT_DIAG_SERVER_MAX_VENDOR_SW_VERSION_TLV_LENGTH 32    ///< Max length of the Vendor Software Version TLV
#define OT_DIAG_SERVER_MAX_VENDOR_APP_URL_TLV_LENGTH 96       ///< Max length of the Vendor App URL TLV

#define OT_DIAG_SERVER_ITERATOR_INIT 0 ///< Initializer for `otDiagServerInterator`.

typedef uint16_t otDiagServerIterator; ///< Used to iterate through Device Contexts in a message.

/**
 * The size in bytes of the tlv set bitset.
 *
 * Will be the smallest multiple of 4 that can contain all tlvs.
 */
#define OT_DIAG_SERVER_TLV_SET_SIZE (((OT_DIAG_SERVER_DATA_TLV_MAX + 31U) & ~31U) / 8U)

/**
 * Bitset of Diagnostic Server TLVs.
 *
 * Bit for a TLV can be determined as follows:
 * `m8[tlv / 8] & (1 << (tlv % 8))`
 */
OT_TOOL_PACKED_BEGIN
struct otDiagServerTlvSet
{
    union OT_TOOL_PACKED_FIELD
    {
        uint8_t  m8[OT_DIAG_SERVER_TLV_SET_SIZE];
        uint16_t m16[OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint16_t)];
        uint32_t m32[OT_DIAG_SERVER_TLV_SET_SIZE / sizeof(uint32_t)];
    };
} OT_TOOL_PACKED_END;

typedef struct otDiagServerTlvSet otDiagServerTlvSet;

/**
 * Represents information about a device context.
 */
typedef struct otDiagServerContext
{
    uint16_t mRloc16; ///< The Rloc16 of the device.

    uint8_t mType;       ///< The Type of the device.
    uint8_t mUpdateMode; ///< The update mode of the context. Only valid if `mType` is child or neighbor.
    bool    mLegacy;     ///< The legacy flag of the context. Only valid if `mType` is child.

    uint16_t mTlvIterator;    ///< Iterator state for TLV iteration. DO NOT MODIFY.
    uint16_t mTlvIteratorEnd; ///< Iterator state for TLV iteration. DO NOT MODIFY.
} otDiagServerContext;

/**
 * Represents iterator information for ip6 address tlvs.
 */
typedef struct otDiagServerIp6AddressIterator
{
    uint16_t mOffset;
    uint16_t mEnd;
} otDiagServerIp6AddressIterator;

/**
 * Represents iterator information for the aloc tlv.
 */
typedef struct otDiagServerAlocIterator
{
    uint16_t mOffset;
    uint16_t mEnd;
} otDiagServerAlocIterator;

/**
 * Represents a Diagnostic Server TLV.
 */
typedef struct otDiagServerTlv
{
    uint8_t mType; ///< The Diagnostic Server TLV type.

    union
    {
        otExtAddress     mExtAddress;
        otLinkModeConfig mMode;
        uint32_t         mTimeout;
        uint32_t         mLastHeard;
        uint32_t         mConnectionTime;

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
        char     mThreadStackVersion[OT_DIAG_SERVER_MAX_THREAD_STACK_VERSION_TLV_LENGTH + 1];
        char     mVendorName[OT_DIAG_SERVER_MAX_VENDOR_NAME_TLV_LENGTH + 1];
        char     mVendorModel[OT_DIAG_SERVER_MAX_VENDOR_MODEL_TLV_LENGTH + 1];
        char     mVendorSwVersion[OT_DIAG_SERVER_MAX_VENDOR_SW_VERSION_TLV_LENGTH + 1];
        char     mVendorAppUrl[OT_DIAG_SERVER_MAX_VENDOR_APP_URL_TLV_LENGTH + 1];
        struct
        {
            uint8_t  mCount;
            uint16_t mDataOffset;
        } mIp6LinkLocalAddressList;
        otExtAddress mEui64;

    } mData;
} otDiagServerTlv;

typedef void (*otDiagServerUpdateCallback)(const otMessage *aMessage, uint16_t aRloc16, bool aComplete, void *aContext);

/**
 * Gets the next Device Context in the message.
 *
 * Requires `OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage   A pointer to the message.
 * @param[in,out] aIterator  A pointer to the iterator context. To get the first context it should be set to
 * OT_DIAGNOSTIC_SERVER_ITERATOR_INIT.
 * @param[out]    aContext   A pointer to where the Device Context information will be stored. Also acts as an iterator
 * over TLVs in the context.
 *
 * @retval OT_ERROR_NONE       Successfully found the next Device Context.
 * @retval OT_ERROR_NOT_FOUND  No subsequent Device Context exists in the message.
 * @retval OT_ERROR_PARSE      Parsing the next Device Context failed.
 *
 * @Note A subsequent call to this function is only allowed when the current return value is OT_ERROR_NONE.
 */
otError otDiagServerGetNextContext(const otMessage      *aMessage,
                                   otDiagServerIterator *aIterator,
                                   otDiagServerContext  *aContext);

/**
 * Gets the next Diagnostic Server TLV in a Device Context.
 *
 * Requires `OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE`.
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
otError otDiagServerGetNextTlv(const otMessage *aMessage, otDiagServerContext *aContext, otDiagServerTlv *aTlv);

/**
 * Gets the Ip6 address list for a Ip6AddressList or Ip6LinkLocalAddressList tlv.
 *
 * Requires `OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage     A pointer to the message.
 * @param[in]     aDataOffset  The `mDataOffset` value returned in the `otDiagServerTlv` struct for this tlv.
 * @param[in]     aCount       The number of Ip6Addresses to read. Must be less or equal than `mCount` returned in the
 * `otDiagServerTlv` struct for this tlv.
 * @param[out] aAddresses   A pointer to a `aCount` sized array where the ip6 addresses will be stored in.
 *
 * @retval OT_ERROR_NONE   Successfully read the ip6 addresses.
 * @retval OT_ERROR_PARSE  Parsing the message failed.
 * @retval OT_ERROR_INVALID_ARGS  If `aCount` is not 0 and `aAddresses` is NULL.
 */
otError otDiagServerGetIp6Addresses(const otMessage *aMessage,
                                    uint16_t         aDataOffset,
                                    uint16_t         aCount,
                                    otIp6Address    *aAddresses);

/**
 * Gets the aloc list for a AlocList tlv.
 *
 * Requires `OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE`.
 *
 * @param[in]     aMessage     A pointer to the message.
 * @param[in]     aDataOffset  The `mDataOffset` value returned in the `otDiagServerTlv` struct for this tlv.
 * @param[in]     aCount       The number of Alocs to read. Must be less or equal than `mCount` returned in the
 * `otDiagServerTlv` struct for this tlv.
 * @param[out] aAlocs       A pointer to a `aCount` sized array where the Alocs will be stored in.
 *
 * @retval OT_ERROR_NONE   Successfully read the alocs.
 * @retval OT_ERROR_PARSE  Parsing the message failed.
 * @retval OT_ERROR_INVALID_ARGS  If `aCount` is not 0 and `aAlocs` is NULL.
 */
otError otDiagServerGetAlocs(const otMessage *aMessage, uint16_t aDataOffset, uint16_t aCount, uint8_t *aAlocs);

/**
 * Starts the Diagnostic Client and configures the set of requested TLVs.
 *
 * Requires `OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE`.
 *
 * The diagnostic client will automatically discover the set of servers and
 * register with them. Additionally if a sequence number missmatch occurs the
 * client will automatically request the current server state.
 *
 * No TLV or other message validation is performed. If a consumer of the api
 * suspects a message was corrupted or lost `otDiagServerInvalidateServer` can be
 * used to manually trigger a request of the current server state.
 *
 * @param[in]  aInstance  The openthread instance.
 * @param[in]  aHost      The set of tlvs to request for host contexts. May be NULL
 * @param[in]  aChild     The set of tlvs to request for child contexts. May be NULL
 * @param[in]  aNeighbor  The set of tlvs to request for neighbor contexts. May be NULL
 * @param[in]  aCallback  The callback to use when diagnostic update message are received.
 * @param[in]  aContext   A context provided to the callback.
 */
void otDiagServerStartClient(otInstance                *aInstance,
                             const otDiagServerTlvSet  *aHost,
                             const otDiagServerTlvSet  *aChild,
                             const otDiagServerTlvSet  *aNeighbor,
                             otDiagServerUpdateCallback aCallback,
                             void                      *aContext);

/**
 * Stops the Diagnostic Client and prevents all calls to any previously registered
 * callback.
 *
 * Requires `OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE`.
 */
void otDiagServerStopClient(otInstance *aInstance);

bool otDiagServerGetTlv(const otDiagServerTlvSet *aSet, uint8_t aTlv);

/**
 * Sets the bit for a tlv in the provided tlv set.
 *
 * Requires `OPENTHREAD_CONFIG_DIAG_CLIENT_ENABLE`.
 *
 * @param[in] aSet  The TlvSet to modify.
 * @param[in] aTlv  The Tlv id to set.
 *
 * @retval OT_ERROR_NONE          If the bit was successfully set, including if it was already set.
 * @retval OT_ERROR_INVALID_ARGS  If the tlv specified is not a known tlv or aSet is NULL.
 */
otError otDiagServerSetTlv(otDiagServerTlvSet *aSet, uint8_t aTlv);

void otDiagServerClearTlv(otDiagServerTlvSet *aSet, uint8_t aTlv);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_DIAG_SERVER_H_
