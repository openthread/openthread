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

/**
 * @file
 * @brief
 *   This file defines the OpenThread Border Agent TXT data parsing APIs.
 */

#ifndef OPENTHREAD_BORDER_AGENT_TXT_DATA_H_
#define OPENTHREAD_BORDER_AGENT_TXT_DATA_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/border_agent.h>
#include <openthread/dataset.h>
#include <openthread/error.h>
#include <openthread/ip6.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-border-agent-txt-data
 *
 * @brief
 *   This module includes APIs for parsing the MeshCoP service TXT data of a Border Agent.
 *
 * @{
 */

#define OT_BORDER_AGENT_RECORD_VERSION_SIZE (8)  ///< Max size of Record Version string in `otBorderAgentTxtDataInfo`.
#define OT_BORDER_AGENT_THREAD_VERSION_SIZE (16) ///< Max size of Thread Version string in `otBorderAgentTxtDataInfo`.
#define OT_BORDER_AGENT_VENDOR_NAME_SIZE (32)    ///< Max size of Vendor Name string in `otBorderAgentTxtDataInfo`.
#define OT_BORDER_AGENT_MODEL_NAME_SIZE (32)     ///< Max size of Model Name string in `otBorderAgentTxtDataInfo`.

/**
 * Represents the Connection Mode in a Border Agent State Bitmap.
 */
typedef enum otBorderAgentConnMode
{
    OT_BORDER_AGENT_CONN_MODE_DISABLED = 0, ///< DTLS connection not allowed.
    OT_BORDER_AGENT_CONN_MODE_PSKC     = 1, ///< DTLS connection with PSKc.
    OT_BORDER_AGENT_CONN_MODE_PSKD     = 2, ///< DTLS connection with PSKd.
    OT_BORDER_AGENT_CONN_MODE_VENDOR   = 3, ///< DTLS with vendor defined credential.
    OT_BORDER_AGENT_CONN_MODE_X509     = 4, ///< DTLS with X.509 certificate.
} otBorderAgentConnMode;

/**
 * Represents the Thread Interface Status in a Border Agent State Bitmap.
 */
typedef enum otBorderAgentThreadIfState
{
    OT_BORDER_AGENT_THREAD_IF_NOT_INITIALIZED = 0, ///< Thread interface is not initialized.
    OT_BORDER_AGENT_THREAD_IF_INITIALIZED     = 1, ///< Thread interface is initialized  but is not yet active.
    OT_BORDER_AGENT_THREAD_IF_ACTIVE          = 2, ///< Thread interface is initialized and active.
} otBorderAgentThreadIfState;

/**
 * Represents the Availability Status in a Border Agent State Bitmap.
 */
typedef enum otBorderAgentAvailability
{
    OT_BORDER_AGENT_AVAILABILITY_INFREQUENT = 0, ///< Infrequent availability.
    OT_BORDER_AGENT_AVAILABILITY_HIGH       = 1, ///< High availability.
} otBorderAgentAvailability;

/**
 * Represents the Thread Role in a Border Agent State Bitmap.
 */
typedef enum otBorderAgentThreadRole
{
    OT_BORDER_AGENT_THREAD_ROLE_DISABLED_OR_DETACHED = 0, ///< Detached or disabled.
    OT_BORDER_AGENT_THREAD_ROLE_CHILD                = 1, ///< End device (child).
    OT_BORDER_AGENT_THREAD_ROLE_ROUTER               = 2, ///< Router.
    OT_BORDER_AGENT_THREAD_ROLE_LEADER               = 3, ///< Leader.
} otBorderAgentThreadRole;

/**
 * Represents Border Agent State Bitmap information.
 */
typedef struct otBorderAgentStateBitmap
{
    otBorderAgentConnMode      mConnMode;       ///< Connection Mode.
    otBorderAgentThreadIfState mThreadIfState;  ///< Thread Interface Status.
    otBorderAgentAvailability  mAvailability;   ///< Availability
    otBorderAgentThreadRole    mThreadRole;     ///< Thread Role.
    bool                       mBbrIsActive;    ///< Backbone Router function is active.
    bool                       mBbrIsPrimary;   ///< Device is the Primary Backbone Router.
    bool                       mEpskcSupported; ///< ePSKc Mode is supported.
} otBorderAgentStateBitmap;

/**
 * Represents parsed Border Agent TXT data.
 *
 * The boolean flags indicate whether a specific field is present in the parsed TXT data.
 */
typedef struct otBorderAgentTxtDataInfo
{
    bool                     mHasRecordVersion : 1;   ///< Indicates whether Record Version is present.
    bool                     mHasAgentId : 1;         ///< Indicates whether Agent ID is present.
    bool                     mHasThreadVersion : 1;   ///< Indicates whether Thread Version is present.
    bool                     mHasStateBitmap : 1;     ///< Indicates whether State Bitmap is present.
    bool                     mHasNetworkName : 1;     ///< Indicates whether Network Name is present.
    bool                     mHasExtendedPanId : 1;   ///< Indicates whether Extended PAN ID is present.
    bool                     mHasActiveTimestamp : 1; ///< Indicates whether Active Timestamp is present.
    bool                     mHasPartitionId : 1;     ///< Indicates whether Partition ID is present.
    bool                     mHasDomainName : 1;      ///< Indicates whether Domain Name is present.
    bool                     mHasBbrSeqNum : 1;       ///< Indicates whether BBR Sequence Number is present.
    bool                     mHasBbrPort : 1;         ///< Indicates whether BBR Port is present.
    bool                     mHasOmrPrefix : 1;       ///< Indicates whether OMR Prefix is present.
    bool                     mHasExtAddress : 1;      ///< Indicates whether Extended Address is present.
    bool                     mHasVendorName : 1;      ///< Indicates whether Vendor Name is present.
    bool                     mHasModelName : 1;       ///< Indicates whether Model Name is present.
    char                     mRecordVersion[OT_BORDER_AGENT_RECORD_VERSION_SIZE]; ///< Record Version string.
    otBorderAgentId          mAgentId;                                            ///< Agent ID.
    char                     mThreadVersion[OT_BORDER_AGENT_THREAD_VERSION_SIZE]; ///< Thread Version string.
    otBorderAgentStateBitmap mStateBitmap;                                        ///< State Bitmap.
    otNetworkName            mNetworkName;                                        ///< Network Name.
    otExtendedPanId          mExtendedPanId;                                      ///< Extended PAN ID.
    otTimestamp              mActiveTimestamp;                                    ///< Active Timestamp.
    uint32_t                 mPartitionId;                                        ///< Partition ID.
    otNetworkName            mDomainName;                                         ///< Domain Name.
    uint8_t                  mBbrSeqNum;                                          ///< BBR Sequence Number.
    uint16_t                 mBbrPort;                                            ///< BBR Port.
    otIp6Prefix              mOmrPrefix;                                          ///< OMR Prefix.
    otExtAddress             mExtAddress;                                         ///< Extended Address.
    char                     mVendorName[OT_BORDER_AGENT_VENDOR_NAME_SIZE];       ///< Vendor Name string.
    char                     mModelName[OT_BORDER_AGENT_MODEL_NAME_SIZE];         ///< Model Name string.
} otBorderAgentTxtDataInfo;

/**
 * Parses a Border Agent's MeshCoP service TXT data.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_AGENT_TXT_DATA_PARSER_ENABLE`.
 *
 * @param[in]  aTxtData        A pointer to the buffer containing the TXT data.
 * @param[in]  aTxtDataLength  The length of the TXT data in bytes.
 * @param[out] aInfo           A pointer to a structure to output the parsed information.
 *
 * @retval OT_ERROR_NONE     Successfully parsed the TXT data.
 * @retval OT_ERROR_PARSE    Failed to parse the TXT data.
 */
otError otBorderAgentTxtDataParse(const uint8_t *aTxtData, uint16_t aTxtDataLength, otBorderAgentTxtDataInfo *aInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_BORDER_AGENT_TXT_DATA_H_
