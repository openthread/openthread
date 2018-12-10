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
 *  This file defines the OpenThread Operational Dataset API (for both FTD and MTD).
 */

#ifndef OPENTHREAD_DATASET_H_
#define OPENTHREAD_DATASET_H_

#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-thread-general
 *
 * @{
 *
 */

#define OT_MASTER_KEY_SIZE 16 ///< Size of the Thread Master Key (bytes)

/**
 * @struct otMasterKey
 *
 * This structure represents a Thread Master Key.
 *
 */
OT_TOOL_PACKED_BEGIN
struct otMasterKey
{
    uint8_t m8[OT_MASTER_KEY_SIZE]; ///< Byte values
} OT_TOOL_PACKED_END;

/**
 * This structure represents a Thread Master Key.
 *
 */
typedef struct otMasterKey otMasterKey;

#define OT_NETWORK_NAME_MAX_SIZE 16 ///< Maximum size of the Thread Network Name field (bytes)

/**
 * This structure represents a Network Name.
 *
 */
typedef struct otNetworkName
{
    char m8[OT_NETWORK_NAME_MAX_SIZE + 1]; ///< Byte values
} otNetworkName;

#define OT_EXT_PAN_ID_SIZE 8 ///< Size of a Thread PAN ID (bytes)

/**
 * This structure represents an Extended PAN ID.
 *
 */
OT_TOOL_PACKED_BEGIN
struct otExtendedPanId
{
    uint8_t m8[OT_EXT_PAN_ID_SIZE]; ///< Byte values
} OT_TOOL_PACKED_END;

/**
 * This structure represents an Extended PAN ID.
 *
 */
typedef struct otExtendedPanId otExtendedPanId;

#define OT_MESH_LOCAL_PREFIX_SIZE 8 ///< Size of the Mesh Local Prefix (bytes)

/**
 * This structure represents a Mesh Local Prefix.
 *
 */
OT_TOOL_PACKED_BEGIN
struct otMeshLocalPrefix
{
    uint8_t m8[OT_MESH_LOCAL_PREFIX_SIZE]; ///< Byte values
} OT_TOOL_PACKED_END;

/**
 * This structure represents a Mesh Local Prefix
 *
 */
typedef struct otMeshLocalPrefix otMeshLocalPrefix;

#define OT_PSKC_MAX_SIZE 16 ///< Maximum size of the PSKc (bytes)

/**
 * This structure represents PSKc.
 *
 */
typedef struct otPSKc
{
    uint8_t m8[OT_PSKC_MAX_SIZE]; ///< Byte values
} otPSKc;

/**
 * This structure represent Security Policy.
 *
 */
typedef struct otSecurityPolicy
{
    uint16_t mRotationTime; ///< The value for thrKeyRotation in units of hours
    uint8_t  mFlags;        ///< Flags as defined in Thread 1.1 Section 8.10.1.15
} otSecurityPolicy;

/**
 * This enumeration defines the Security Policy TLV flags.
 *
 */
enum
{
    OT_SECURITY_POLICY_OBTAIN_MASTER_KEY     = 1 << 7, ///< Obtaining the Master Key
    OT_SECURITY_POLICY_NATIVE_COMMISSIONING  = 1 << 6, ///< Native Commissioning
    OT_SECURITY_POLICY_ROUTERS               = 1 << 5, ///< Routers enabled
    OT_SECURITY_POLICY_EXTERNAL_COMMISSIONER = 1 << 4, ///< External Commissioner allowed
    OT_SECURITY_POLICY_BEACONS               = 1 << 3, ///< Beacons enabled
};

/**
 * This type represents Channel Mask Page 0.
 *
 */
typedef uint32_t otChannelMaskPage0;

#define OT_CHANNEL_11_MASK (1 << 11) ///< Channel 11
#define OT_CHANNEL_12_MASK (1 << 12) ///< Channel 12
#define OT_CHANNEL_13_MASK (1 << 13) ///< Channel 13
#define OT_CHANNEL_14_MASK (1 << 14) ///< Channel 14
#define OT_CHANNEL_15_MASK (1 << 15) ///< Channel 15
#define OT_CHANNEL_16_MASK (1 << 16) ///< Channel 16
#define OT_CHANNEL_17_MASK (1 << 17) ///< Channel 17
#define OT_CHANNEL_18_MASK (1 << 18) ///< Channel 18
#define OT_CHANNEL_19_MASK (1 << 19) ///< Channel 19
#define OT_CHANNEL_20_MASK (1 << 20) ///< Channel 20
#define OT_CHANNEL_21_MASK (1 << 21) ///< Channel 21
#define OT_CHANNEL_22_MASK (1 << 22) ///< Channel 22
#define OT_CHANNEL_23_MASK (1 << 23) ///< Channel 23
#define OT_CHANNEL_24_MASK (1 << 24) ///< Channel 24
#define OT_CHANNEL_25_MASK (1 << 25) ///< Channel 25
#define OT_CHANNEL_26_MASK (1 << 26) ///< Channel 26

/**
 * This structure represents presence of different components in Active or Pending Operational Dataset.
 *
 */
typedef struct otOperationalDatasetComponents
{
    bool mIsActiveTimestampPresent : 1;  ///< TRUE if Active Timestamp is present, FALSE otherwise.
    bool mIsPendingTimestampPresent : 1; ///< TRUE if Pending Timestamp is present, FALSE otherwise.
    bool mIsMasterKeyPresent : 1;        ///< TRUE if Network Master Key is present, FALSE otherwise.
    bool mIsNetworkNamePresent : 1;      ///< TRUE if Network Name is present, FALSE otherwise.
    bool mIsExtendedPanIdPresent : 1;    ///< TRUE if Extended PAN ID is present, FALSE otherwise.
    bool mIsMeshLocalPrefixPresent : 1;  ///< TRUE if Mesh Local Prefix is present, FALSE otherwise.
    bool mIsDelayPresent : 1;            ///< TRUE if Delay Timer is present, FALSE otherwise.
    bool mIsPanIdPresent : 1;            ///< TRUE if PAN ID is present, FALSE otherwise.
    bool mIsChannelPresent : 1;          ///< TRUE if Channel is present, FALSE otherwise.
    bool mIsPSKcPresent : 1;             ///< TRUE if PSKc is present, FALSE otherwise.
    bool mIsSecurityPolicyPresent : 1;   ///< TRUE if Security Policy is present, FALSE otherwise.
    bool mIsChannelMaskPage0Present : 1; ///< TRUE if Channel Mask Page 0 is present, FALSE otherwise.
} otOperationalDatasetComponents;

/**
 * This structure represents an Active or Pending Operational Dataset.
 *
 * Components in Dataset are optional. `mComponets` structure specifies which components are present in the Dataset.
 *
 */
typedef struct otOperationalDataset
{
    uint64_t                       mActiveTimestamp;  ///< Active Timestamp
    uint64_t                       mPendingTimestamp; ///< Pending Timestamp
    otMasterKey                    mMasterKey;        ///< Network Master Key
    otNetworkName                  mNetworkName;      ///< Network Name
    otExtendedPanId                mExtendedPanId;    ///< Extended PAN ID
    otMeshLocalPrefix              mMeshLocalPrefix;  ///< Mesh Local Prefix
    uint32_t                       mDelay;            ///< Delay Timer
    otPanId                        mPanId;            ///< PAN ID
    uint16_t                       mChannel;          ///< Channel
    otPSKc                         mPSKc;             ///< PSKc
    otSecurityPolicy               mSecurityPolicy;   ///< Security Policy
    otChannelMaskPage0             mChannelMaskPage0; ///< Channel Mask Page 0
    otOperationalDatasetComponents mComponents;       ///< Specifies which components are set in the Dataset.
} otOperationalDataset;

/**
 * This enumeration represents meshcop TLV types.
 *
 */
typedef enum otMeshcopTlvType
{
    OT_MESHCOP_TLV_CHANNEL                  = 0,   ///< meshcop Channel TLV
    OT_MESHCOP_TLV_PANID                    = 1,   ///< meshcop Pan Id TLV
    OT_MESHCOP_TLV_EXTPANID                 = 2,   ///< meshcop Extended Pan Id TLV
    OT_MESHCOP_TLV_NETWORKNAME              = 3,   ///< meshcop Network Name TLV
    OT_MESHCOP_TLV_PSKC                     = 4,   ///< meshcop PSKc TLV
    OT_MESHCOP_TLV_MASTERKEY                = 5,   ///< meshcop Network Master Key TLV
    OT_MESHCOP_TLV_NETWORK_KEY_SEQUENCE     = 6,   ///< meshcop Network Key Sequence TLV
    OT_MESHCOP_TLV_MESHLOCALPREFIX          = 7,   ///< meshcop Mesh Local Prefix TLV
    OT_MESHCOP_TLV_STEERING_DATA            = 8,   ///< meshcop Steering Data TLV
    OT_MESHCOP_TLV_BORDER_AGENT_RLOC        = 9,   ///< meshcop Border Agent Locator TLV
    OT_MESHCOP_TLV_COMMISSIONER_ID          = 10,  ///< meshcop Commissioner ID TLV
    OT_MESHCOP_TLV_COMM_SESSION_ID          = 11,  ///< meshcop Commissioner Session ID TLV
    OT_MESHCOP_TLV_SECURITYPOLICY           = 12,  ///< meshcop Security Policy TLV
    OT_MESHCOP_TLV_GET                      = 13,  ///< meshcop Get TLV
    OT_MESHCOP_TLV_ACTIVETIMESTAMP          = 14,  ///< meshcop Active Timestamp TLV
    OT_MESHCOP_TLV_STATE                    = 16,  ///< meshcop State TLV
    OT_MESHCOP_TLV_JOINER_DTLS              = 17,  ///< meshcop Joiner DTLS Encapsulation TLV
    OT_MESHCOP_TLV_JOINER_UDP_PORT          = 18,  ///< meshcop Joiner UDP Port TLV
    OT_MESHCOP_TLV_JOINER_IID               = 19,  ///< meshcop Joiner IID TLV
    OT_MESHCOP_TLV_JOINER_RLOC              = 20,  ///< meshcop Joiner Router Locator TLV
    OT_MESHCOP_TLV_JOINER_ROUTER_KEK        = 21,  ///< meshcop Joiner Router KEK TLV
    OT_MESHCOP_TLV_PROVISIONING_URL         = 32,  ///< meshcop Provisioning URL TLV
    OT_MESHCOP_TLV_VENDOR_NAME_TLV          = 33,  ///< meshcop Vendor Name TLV
    OT_MESHCOP_TLV_VENDOR_MODEL_TLV         = 34,  ///< meshcop Vendor Model TLV
    OT_MESHCOP_TLV_VENDOR_SW_VERSION_TLV    = 35,  ///< meshcop Vendor SW Version TLV
    OT_MESHCOP_TLV_VENDOR_DATA_TLV          = 36,  ///< meshcop Vendor Data TLV
    OT_MESHCOP_TLV_VENDOR_STACK_VERSION_TLV = 37,  ///< meshcop Vendor Stack Version TLV
    OT_MESHCOP_TLV_UDP_ENCAPSULATION_TLV    = 48,  ///< meshcop UDP encapsulation TLV
    OT_MESHCOP_TLV_IPV6_ADDRESS_TLV         = 49,  ///< meshcop IPv6 address TLV
    OT_MESHCOP_TLV_PENDINGTIMESTAMP         = 51,  ///< meshcop Pending Timestamp TLV
    OT_MESHCOP_TLV_DELAYTIMER               = 52,  ///< meshcop Delay Timer TLV
    OT_MESHCOP_TLV_CHANNELMASK              = 53,  ///< meshcop Channel Mask TLV
    OT_MESHCOP_TLV_COUNT                    = 54,  ///< meshcop Count TLV
    OT_MESHCOP_TLV_PERIOD                   = 55,  ///< meshcop Period TLV
    OT_MESHCOP_TLV_SCAN_DURATION            = 56,  ///< meshcop Scan Duration TLV
    OT_MESHCOP_TLV_ENERGY_LIST              = 57,  ///< meshcop Energy List TLV
    OT_MESHCOP_TLV_DISCOVERYREQUEST         = 128, ///< meshcop Discovery Request TLV
    OT_MESHCOP_TLV_DISCOVERYRESPONSE        = 129, ///< meshcop Discovery Response TLV
} otMeshcopTlvType;

/**
 * This function indicates whether a valid network is present in the Active Operational Dataset or not.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns TRUE if a valid network is present in the Active Operational Dataset, FALSE otherwise.
 *
 */
OTAPI bool OTCALL otDatasetIsCommissioned(otInstance *aInstance);

/**
 * This function gets the Active Operational Dataset.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[out]  aDataset  A pointer to where the Active Operational Dataset will be placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the Active Operational Dataset.
 * @retval OT_ERROR_INVALID_ARGS  @p aDataset was NULL.
 *
 */
OTAPI otError OTCALL otDatasetGetActive(otInstance *aInstance, otOperationalDataset *aDataset);

/**
 * This function sets the Active Operational Dataset.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDataset  A pointer to the Active Operational Dataset.
 *
 * @retval OT_ERROR_NONE          Successfully set the Active Operational Dataset.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to set the Active Operational Dataset.
 * @retval OT_ERROR_INVALID_ARGS  @p aDataset was NULL.
 *
 */
OTAPI otError OTCALL otDatasetSetActive(otInstance *aInstance, const otOperationalDataset *aDataset);

/**
 * This function gets the Pending Operational Dataset.
 *
 * @param[in]   aInstance A pointer to an OpenThread instance.
 * @param[out]  aDataset  A pointer to where the Pending Operational Dataset will be placed.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved the Pending Operational Dataset.
 * @retval OT_ERROR_INVALID_ARGS  @p aDataset was NULL.
 *
 */
OTAPI otError OTCALL otDatasetGetPending(otInstance *aInstance, otOperationalDataset *aDataset);

/**
 * This function sets the Pending Operational Dataset.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aDataset  A pointer to the Pending Operational Dataset.
 *
 * @retval OT_ERROR_NONE          Successfully set the Pending Operational Dataset.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to set the Pending Operational Dataset.
 * @retval OT_ERROR_INVALID_ARGS  @p aDataset was NULL.
 *
 */
OTAPI otError OTCALL otDatasetSetPending(otInstance *aInstance, const otOperationalDataset *aDataset);

/**
 * This function sends MGMT_ACTIVE_GET.
 *
 * @param[in]  aInstance           A pointer to an OpenThread instance.
 * @param[in]  aDatasetComponents  A pointer to a Dataset Components structure specifying which components to request.
 * @param[in]  aTlvTypes           A pointer to array containing additional raw TLV types to be requested.
 * @param[in]  aLength             The length of @p aTlvTypes.
 * @param[in]  aAddress            A pointer to the IPv6 destination, if it is NULL, will use Leader ALOC as default.
 *
 * @retval OT_ERROR_NONE          Successfully send the meshcop dataset command.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to send.
 *
 */
OTAPI otError OTCALL otDatasetSendMgmtActiveGet(otInstance *                          aInstance,
                                                const otOperationalDatasetComponents *aDatasetComponents,
                                                const uint8_t *                       aTlvTypes,
                                                uint8_t                               aLength,
                                                const otIp6Address *                  aAddress);

/**
 * This function sends MGMT_ACTIVE_SET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDataset   A pointer to operational dataset.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval OT_ERROR_NONE          Successfully send the meshcop dataset command.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to send.
 *
 */
OTAPI otError OTCALL otDatasetSendMgmtActiveSet(otInstance *                aInstance,
                                                const otOperationalDataset *aDataset,
                                                const uint8_t *             aTlvs,
                                                uint8_t                     aLength);

/**
 * This function sends MGMT_PENDING_GET.
 *
 * @param[in]  aInstance           A pointer to an OpenThread instance.
 * @param[in]  aDatasetComponents  A pointer to a Dataset Components structure specifying which components to request.
 * @param[in]  aTlvTypes           A pointer to array containing additional raw TLV types to be requested.
 * @param[in]  aLength             The length of @p aTlvTypes.
 * @param[in]  aAddress            A pointer to the IPv6 destination, if it is NULL, will use Leader ALOC as default.
 *
 * @retval OT_ERROR_NONE          Successfully send the meshcop dataset command.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to send.
 *
 */
OTAPI otError OTCALL otDatasetSendMgmtPendingGet(otInstance *                          aInstance,
                                                 const otOperationalDatasetComponents *aDatasetComponents,
                                                 const uint8_t *                       aTlvTypes,
                                                 uint8_t                               aLength,
                                                 const otIp6Address *                  aAddress);

/**
 * This function sends MGMT_PENDING_SET.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aDataset   A pointer to operational dataset.
 * @param[in]  aTlvs      A pointer to TLVs.
 * @param[in]  aLength    The length of TLVs.
 *
 * @retval OT_ERROR_NONE          Successfully send the meshcop dataset command.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to send.
 *
 */
OTAPI otError OTCALL otDatasetSendMgmtPendingSet(otInstance *                aInstance,
                                                 const otOperationalDataset *aDataset,
                                                 const uint8_t *             aTlvs,
                                                 uint8_t                     aLength);

/**
 * Get minimal delay timer.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval the value of minimal delay timer (in ms).
 *
 */
OTAPI uint32_t OTCALL otDatasetGetDelayTimerMinimal(otInstance *aInstance);

/**
 * Set minimal delay timer.
 *
 * @note This API is reserved for testing and demo purposes only. Changing settings with
 * this API will render a production application non-compliant with the Thread Specification.
 *
 * @param[in]  aInstance           A pointer to an OpenThread instance.
 * @param[in]  aDelayTimerMinimal  The value of minimal delay timer (in ms).
 *
 * @retval  OT_ERROR_NONE          Successfully set minimal delay timer.
 * @retval  OT_ERROR_INVALID_ARGS  If @p aDelayTimerMinimal is not valid.
 *
 */
OTAPI otError OTCALL otDatasetSetDelayTimerMinimal(otInstance *aInstance, uint32_t aDelayTimerMinimal);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_DATASET_H_
