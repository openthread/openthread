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
 *   This file defines the phy radio interface for OpenThread.
 *
 */

#ifndef INCLUDE_OPENTHREAD_PLATFORM_RADIO_MAC_H_
#define INCLUDE_OPENTHREAD_PLATFORM_RADIO_MAC_H_

#include <stdint.h>

#include <openthread-core-config.h>
#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-radio
 *
 * @brief
 *   This module includes the platform abstraction for radio communication.
 *
 * @{
 *
 */

/**
 * @defgroup radio-mac-config Configuration
 *
 * @brief
 *   This module includes the platform abstraction for radio mac configuration.
 *
 * @{
 *
 */

enum otMacStatus
{
    OT_MAC_STATUS_SUCCESS                 = 0x00,
    OT_MAC_STATUS_ERROR                   = 0x01,
    OT_MAC_STATUS_CANCELLED               = 0x02,
    OT_MAC_STATUS_READY_FOR_POLL          = 0x03,
    OT_MAC_STATUS_COUNTER_ERROR           = 0xDB,
    OT_MAC_STATUS_IMPROPER_KEY_TYPE       = 0xDC,
    OT_MAC_STATUS_IMPROPER_SECURITY_LEVEL = 0xDD,
    OT_MAC_STATUS_UNSUPPORTED_LEGACY      = 0xDE,
    OT_MAC_STATUS_UNSUPPORTED_SECURITY    = 0xDF,
    OT_MAC_STATUS_BEACON_LOST             = 0xE0,
    OT_MAC_STATUS_CHANNEL_ACCESS_FAILURE  = 0xE1,
    OT_MAC_STATUS_DENIED                  = 0xE2,
    OT_MAC_STATUS_DISABLE_TRX_FAILURE     = 0xE3,
    OT_MAC_STATUS_SECURITY_ERROR          = 0xE4,
    OT_MAC_STATUS_FRAME_TOO_LONG          = 0xE5,
    OT_MAC_STATUS_INVALID_GTS             = 0xE6,
    OT_MAC_STATUS_INVALID_HANDLE          = 0xE7,
    OT_MAC_STATUS_INVALID_PARAMETER       = 0xE8,
    OT_MAC_STATUS_NO_ACK                  = 0xE9,
    OT_MAC_STATUS_NO_BEACON               = 0xEA,
    OT_MAC_STATUS_NO_DATA                 = 0xEB,
    OT_MAC_STATUS_NO_SHORT_ADDRESS        = 0xEC,
    OT_MAC_STATUS_OUT_OF_CAP              = 0xED,
    OT_MAC_STATUS_PAN_ID_CONFLICT         = 0xEE,
    OT_MAC_STATUS_REALIGNMENT             = 0xEF,
    OT_MAC_STATUS_TRANSACTION_EXPIRED     = 0xF0,
    OT_MAC_STATUS_TRANSACTION_OVERFLOW    = 0xF1,
    OT_MAC_STATUS_TX_ACTIVE               = 0xF2,
    OT_MAC_STATUS_UNAVAILABLE_KEY         = 0xF3,
    OT_MAC_STATUS_UNSUPPORTED_ATTRIBUTE   = 0xF4,
    OT_MAC_STATUS_INVALID_ADDRESS         = 0xF5,
    OT_MAC_STATUS_ON_TIME_TOO_LONG        = 0xF6,
    OT_MAC_STATUS_PAST_TIME               = 0xF7,
    OT_MAC_STATUS_TRACKING_OFF            = 0xF8,
    OT_MAC_STATUS_INVALID_INDEX           = 0xF9,
    OT_MAC_STATUS_LIMIT_REACHED           = 0xFA,
    OT_MAC_STATUS_READ_ONLY               = 0xFB,
    OT_MAC_STATUS_SCAN_IN_PROGRESS        = 0xFC,
    OT_MAC_STATUS_SUPERFRAME_OVERLAP      = 0xFD,
    OT_MAC_STATUS_SYSTEM_ERROR            = 0xFF
};

/**
 * This enum represents the indexes for each PIB attribute as defined in IEEE 802.15.4-2006
 *
 */
typedef enum otPibAttr
{
    OT_PIB_PHY_CURRENT_CHANNEL    = 0x00,
    OT_PIB_PHY_CHANNELS_SUPPORT   = 0x01,
    OT_PIB_PHY_TRANSMIT_POWER     = 0x02,
    OT_PIB_PHY_CCA_MODE           = 0x03,
    OT_PIB_PHY_CURRENT_PAGE       = 0x04,
    OT_PIB_PHY_MAX_FRAME_DURATION = 0x05,
    OT_PIB_PHY_SHR_DURATION       = 0x06,
    OT_PIB_PHY_SYMBOLS_PER_OCTET  = 0x07,

    OT_PIB_PHY_PIB_FIRST = OT_PIB_PHY_CURRENT_CHANNEL,
    OT_PIB_PHY_PIB_LAST  = OT_PIB_PHY_SYMBOLS_PER_OCTET,

    OT_PIB_MAC_ACK_WAIT_DURATION            = 0x40,
    OT_PIB_MAC_ASSOCIATION_PERMIT           = 0x41,
    OT_PIB_MAC_AUTO_REQUEST                 = 0x42,
    OT_PIB_MAC_BATT_LIFE_EXT                = 0x43,
    OT_PIB_MAC_BATT_LIFE_EXT_PERIODS        = 0x44,
    OT_PIB_MAC_BEACON_PAYLOAD               = 0x45,
    OT_PIB_MAC_BEACON_PAYLOAD_LENGTH        = 0x46,
    OT_PIB_MAC_BEACON_ORDER                 = 0x47,
    OT_PIB_MAC_BEACON_TX_TIME               = 0x48,
    OT_PIB_MAC_BSN                          = 0x49,
    OT_PIB_MAC_COORD_EXTENDED_ADDRESS       = 0x4a,
    OT_PIB_MAC_COORD_SHORT_ADDRESS          = 0x4b,
    OT_PIB_MAC_DSN                          = 0x4c,
    OT_PIB_MAC_GTS_PERMIT                   = 0x4d,
    OT_PIB_MAC_MAX_CSMA_BACKOFFS            = 0x4e,
    OT_PIB_MAC_MIN_BE                       = 0x4f,
    OT_PIB_MAC_PAN_ID                       = 0x50,
    OT_PIB_MAC_PROMISCUOUS_MODE             = 0x51,
    OT_PIB_MAC_RX_ON_WHEN_IDLE              = 0x52,
    OT_PIB_MAC_SHORT_ADDRESS                = 0x53,
    OT_PIB_MAC_SUPERFRAME_ORDER             = 0x54,
    OT_PIB_MAC_TRANSACTION_PERSISTENCE_TIME = 0x55,
    OT_PIB_MAC_ASSOCIATED_PAN_COORD         = 0x56,
    OT_PIB_MAC_MAX_BE                       = 0x57,
    OT_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME    = 0x58,
    OT_PIB_MAC_MAX_FRAME_RETRIES            = 0x59,
    OT_PIB_MAC_RESPONSE_WAIT_TIME           = 0x5A,
    OT_PIB_MAC_SYNC_SYMBOL_OFFSET           = 0x5B,
    OT_PIB_MAC_TIMESTAMP_SUPPORTED          = 0x5C,
    OT_PIB_MAC_SECURITY_ENABLED             = 0x5D,

    OT_PIB_MAC_PIB_FIRST = OT_PIB_MAC_ACK_WAIT_DURATION,
    OT_PIB_MAC_PIB_LAST  = OT_PIB_MAC_SECURITY_ENABLED,

    OT_PIB_MAC_KEY_TABLE                    = 0x71,
    OT_PIB_MAC_KEY_TABLE_ENTRIES            = 0x72,
    OT_PIB_MAC_DEVICE_TABLE                 = 0x73,
    OT_PIB_MAC_DEVICE_TABLE_ENTRIES         = 0x74,
    OT_PIB_MAC_SECURITY_LEVEL_TABLE         = 0x75,
    OT_PIB_MAC_SECURITY_LEVEL_TABLE_ENTRIES = 0x76,
    OT_PIB_MAC_FRAME_COUNTER                = 0x77,
    OT_PIB_MAC_AUTO_REQUEST_SECURITY_LEVEL  = 0x78,
    OT_PIB_MAC_AUTO_REQUEST_KEY_ID_MODE     = 0x79,
    OT_PIB_MAC_AUTO_REQUEST_KEY_SOURCE      = 0x7A,
    OT_PIB_MAC_AUTO_REQUEST_KEY_INDEX       = 0x7B,
    OT_PIB_MAC_DEFAULT_KEY_SOURCE           = 0x7C,
    OT_PIB_MAC_PAN_COORD_EXTENDED_ADDRESS   = 0x7D,
    OT_PIB_MAC_PAN_COORD_SHORT_ADDRESS      = 0x7E,

    OT_PIB_MAC_SEC_PIB_FIRST = OT_PIB_MAC_KEY_TABLE,
    OT_PIB_MAC_SEC_PIB_LAST  = OT_PIB_MAC_PAN_COORD_SHORT_ADDRESS,

    OT_PIB_MAC_IEEE_ADDRESS = 0xFF /* Non-standard, writeable IEEE address */
} otPibAttr;

/**
 * This enum represents the TxOpt bits as defined in IEEE 802.15.4-2006.
 * There are also some additional TxOptions required for thread compliance.
 *
 */
enum otTxOption
{
    OT_MAC_TX_OPTION_ACK_REQ  = (1 << 0),
    OT_MAC_TX_OPTION_GTS      = (1 << 1), // Unset bit for CAP
    OT_MAC_TX_OPTION_INDIRECT = (1 << 2),
    OT_MAC_TX_OPTION_NS_NONCE = (1 << 7), // Nonstandard flag to indicate nonce should be constructed with mode2 extaddr
    OT_MAC_TX_OPTION_NS_FPEND = (1 << 6)  // Nonstandard flag to indicate FPEND bit should be set (optional hint)
};

/**
 * This enum represents the address modes as defined in IEEE 802.15.4-2006.
 *
 */
enum otAddressMode
{
    OT_MAC_ADDRESS_MODE_NONE  = 0x00,
    OT_MAC_ADDRESS_MODE_SHORT = 0x02,
    OT_MAC_ADDRESS_MODE_EXT   = 0x03
};

/**
 * This enum represents the scan types as defined in IEEE 802.15.4-2006.
 *
 */
enum otScanType
{
    OT_MAC_SCAN_TYPE_ENERGY = 0,
    OT_MAC_SCAN_TYPE_ACTIVE = 1
};

/**
 * This enum represents the lookup data size codes as defined in IEEE 802.15.4-2006.
 *
 */
enum otLookupDataSizeCode
{
    OT_MAC_LOOKUP_DATA_SIZE_CODE_5_OCTETS = 0,
    OT_MAC_LOOKUP_DATA_SIZE_CODE_9_OCTETS = 1
};

/**
 * This structure represents a full set of addressing data.
 *
 */
struct otFullAddr
{
    uint8_t mAddressMode; ///< Address Mode of contained address
    uint8_t mPanId[2];    ///< Pan Id field
    uint8_t mAddress[8];  ///< Address data with length dependant on address mode
};

/**
 * This structure represents the security information required for various primitives as defined in IEEE 802.15.4-2006.
 *
 */
struct otSecSpec
{
    uint8_t mSecurityLevel; ///< 802.15.4 Security level
    uint8_t mKeyIdMode;     ///< 802.15.4 Key Id Mode
    uint8_t mKeySource[8];  ///< 802.15.4 Key Source
    uint8_t mKeyIndex;      ///< 802.15.4 Key Index
};

/**
 * This structure represents the pan descriptor as defined in IEEE 802.15.4-2006.
 *
 */
struct otPanDescriptor
{
    struct otFullAddr Coord;             ///< Address of coordinator in otFullAddr format
    uint8_t           LogicalChannel;    ///< Logical channel of the network
    uint8_t           SuperframeSpec[2]; ///< Superframe specification from received beacon frame
    uint8_t           GTSPermit;    ///< Boolean value specifying whether or not coordinator is accepting GTS requests
    uint8_t           LinkQuality;  ///< Link quality of received beacon
    uint8_t           TimeStamp[4]; ///< Timestamp of received beacon (optional)
    uint8_t           SecurityFailure; ///< OT_MAC_STATUS_SUCCESS if security processing was successful
    struct otSecSpec  Security;        ///< Security information of the received beacon
};

/**
 * This structure represents the MCPS-Data.Request as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otDataRequest
{
    uint8_t           mSrcAddrMode;                  ///< Source addressing mode
    struct otFullAddr mDst;                          ///< Destination addressing information
    uint8_t           mMsduLength;                   ///< Length of Data
    uint8_t           mMsduHandle;                   ///< Handle of Data
    uint8_t           mTxOptions;                    ///< Tx options bit field
    uint8_t           mMsdu[OT_RADIO_MSDU_MAX_SIZE]; ///< Data
    struct otSecSpec  mSecurity;                     ///< Security information to be used for the generated frame
} otDataRequest;

/**
 * This structure represents the MCPS-Data.Indication as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otDataIndication
{
    struct otFullAddr mSrc;                          ///< Src Address information of received frame
    struct otFullAddr mDst;                          ///< Dst Address information of received frame
    uint8_t           mMsduLength;                   ///< Length of the received MSDU
    int8_t            mMpduLinkQuality;              ///< LQI of received frame - MUST be RSSI for openthread
    uint8_t           mDSN;                          ///< DSN of received frame
    uint8_t           mTimeStamp[4];                 ///< Timestamp of received frame (optional)
    uint8_t           mMsdu[OT_RADIO_MSDU_MAX_SIZE]; ///< Unsecured MSDU from received frame
    struct otSecSpec  mSecurity;                     ///< Security information of received frame
} otDataIndication;

/**
 * This structure represents the MLME-COMM-STATUS.Indication as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otCommStatusIndication
{
    uint8_t          mPanId[2];    ///< Pan Id of indicated frame
    uint8_t          mSrcAddrMode; ///< SrcAddrMode of indicated frame
    uint8_t          mSrcAddr[8];  ///< SrcAddr of indicated frame
    uint8_t          mDstAddrMode; ///< DstAddrMode of indicated frame
    uint8_t          mDstAddr[8];  ///< DstAddr of indicated frame
    uint8_t          mStatus;      ///< Status indicating why the MLME-COMM-STATUS.Indication was generated
    struct otSecSpec mSecurity;    ///< Security information for indicated frame
} otCommStatusIndication;

/**
 * This structure represents the MLME-Poll.Request as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otPollRequest
{
    struct otFullAddr mCoordAddress; ///< Destination of the requested poll
    struct otSecSpec  mSecurity;     ///< Security information to be used to generate the poll
} otPollRequest;

/**
 * This structure represents the MLME-Scan.Request as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otScanRequest
{
    enum otScanType  mScanType;        ///< Enumerated value to choose the scan type
    uint32_t         mScanChannelMask; ///< Mask of channels to scan eg channel 11 = (1<<11)
    uint8_t          mScanDuration;    ///< Scan duration in symbols = (aBaseSuperframeDuration * (2^aScanDuration + 1))
    struct otSecSpec mSecSpec;         ///< Security information for the scan
} otScanRequest;

/**
 * This structure represents the MLME-Scan.Confirm as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otScanConfirm
{
    uint8_t mStatus;               ///< Status of the scan request
    uint8_t mScanType;             ///< Type of scan that was performed
    uint8_t mUnscannedChannels[4]; ///< Indicates which channels given in the request were not scanned
    uint8_t mResultListSize;       ///< Number of elements in the result list
    uint8_t mResultList[16];       ///< Used only for energy scans - List of energy measurements
} otScanConfirm;

/**
 * This structure represents the MLME-Beacon.Notify as defined in IEEE 802.15.4-2006.
 * Not full beacon notify from spec - the variable size part has been omitted for simplicity
 */
typedef struct otBeaconNotify
{
    uint8_t                BSN;                               ///< BSN of received beacon
    struct otPanDescriptor mPanDescriptor;                    ///< PanDescriptor generated from received beacon
    uint8_t                mSduLength;                        ///< Length of the received beacon payload
    uint8_t                mSdu[OT_RADIO_BEACON_MAX_PAYLOAD]; ///< Received beacon payload
} otBeaconNotify;

/**
 * This structure represents the MLME-Start.Request as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otStartRequest
{
    uint16_t mPanId;                ///< Pan id of the network
    uint8_t  mLogicalChannel;       ///< Logical channel of the network
    uint8_t  mBeaconOrder;          ///< Beacon order - always 15 (disabled)
    uint8_t  mSuperframeOrder;      ///< Superframe order - always 15 (disabled)
    uint8_t  mPanCoordinator;       ///< Boolean value indicating whether the MAC should act as a coordinator
    uint8_t  mBatteryLifeExtension; ///< Boolean value indicating whether BLE should be used by the mac (Always false)
    uint8_t  mCoordRealignment;     ///< Boolean value for generating a coordinator realignment command (Always false)
    struct otSecSpec mCoordRealignSecurity; ///< Security used for realignment command (empty)
    struct otSecSpec mBeaconSecurity;       ///< Security to be used in generated beacons
} otStartRequest;

/**
 * This structure represents the Device Descriptor as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otPibDeviceDescriptor
{
    uint8_t mPanId[2];        ///< Pan Id of the represented device
    uint8_t mShortAddress[2]; ///< Short address of the represented device
    uint8_t mExtAddress[8];   ///< ExtAddress of the represented device
    uint8_t mFrameCounter[4]; ///< Current frame counter of the represented device
    uint8_t mExempt;          ///< Exempt bit of the represented device to allow bypass of minimum security
} otPibDeviceDescriptor;

/**
 * This structure represents the Key Id Lookup Descriptor as defined in IEEE 802.15.4-2006.
 *
 */
struct otKeyIdLookupDesc
{
    uint8_t mLookupData[9];      ///< 802.15.4 lookupdata
    uint8_t mLookupDataSizeCode; ///< 802.15.4 size code for the lookupdata (see otLookupDataSizeCode)
};

/**
 * This structure represents the Key Device Descriptor as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otKeyDeviceDesc
{
    uint8_t mDeviceDescriptorHandle; ///< 802.15.4 Device descriptor handle
    bool    mUniqueDevice : 1;       ///< 802.15.4 Unique Device flag
    bool    mBlacklisted : 1;        ///< 802.15.4 Blacklist Flag
#if OPENTHREAD_CONFIG_EXTERNAL_MAC_SHARED_DD
    bool mNew : 1; // Optional memory-saving Extension to minimise number of saved frame counter
#endif
} otKeyDeviceDesc;

/**
 * This structure represents the Key Usage Descriptor as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otKeyUsageDesc
{
    unsigned int mFrameType : 2;      ///< Frame type of allowed frame
    unsigned int mCommandFrameId : 4; ///< Command frame type of allowed frame type if mFrameType is Command type
} otKeyUsageDesc;

/**
 * This structure represents a Key Descriptor Key Table Entry as defined in IEEE 802.15.4-2006.
 *
 */
typedef struct otKeyTableEntry
{
    uint8_t                  mKeyIdLookupListEntries; ///< Number of entries in the Key Id Lookup List
    uint8_t                  mKeyDeviceListEntries;   ///< Number of entries in the Key Device List
    uint8_t                  mKeyUsageListEntries;    ///< Number of entries in the Key Usage List
    uint8_t                  mKey[16];                ///< Key Data
    struct otKeyIdLookupDesc mKeyIdLookupDesc[1];     ///< List of the lookup descriptors for this key
    otKeyDeviceDesc mKeyDeviceDesc[OPENTHREAD_CONFIG_EXTERNAL_MAC_DEVICE_TABLE_SIZE]; ///< Key Device Descriptors
                                                                                      ///< enabled for this key
    otKeyUsageDesc mKeyUsageDesc[2]; ///< Key Usage Descriptors for this key
} otKeyTableEntry;

/**
 * The following are valid radio state transitions:
 *
 *
 *  +----------+  Enable()  +---------+
 *  |          |----------->|         |
 *  | Disabled |            | Enabled |
 *  |          |<-----------|         |
 *  +----------+  Disable() +---------+
 *
 *
 */

/**
 * @}
 *
 */

/**
 * @defgroup radio-config Configuration
 *
 * @brief
 *   This module includes the platform abstraction for radio configuration.
 *
 * @{
 *
 */

/**
 * Get the factory-assigned IEEE EUI-64 for this interface.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[out] aIeeeEui64  A pointer to the factory-assigned IEEE EUI-64.
 *
 */
void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64);

/**
 * Use the MLME_GET SAP to get an attribute from the MAC PiB
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aAttr       The attribute reference for the attribute to be got
 * @param[in] aIndex      The index for the attribute to be got (0 for non-indexed attributes)
 * @param[out] aLen       The length of the data in the buffer
 * @param[out] aBuf       A buffer to store the data from the Pib
 *
 * @returns an otError code (executes synchronously)
 *
 */
otError otPlatMlmeGet(otInstance *aInstance, otPibAttr aAttr, uint8_t aIndex, uint8_t *aLen, uint8_t *aBuf);

/**
 * Use the MLME_SET SAP to set an attribute in the MAC PiB
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aAttr       The attribute reference for the attribute to be set
 * @param[in] aIndex      The index for the attribute to be set (0 for non-indexed attributes)
 * @param[in] aLen        The length of the data in the buffer
 * @param[in] aBuf        A buffer containing the data to be entered into the Pib
 *
 * @returns an otError code (executes synchronously)
 *
 */
otError otPlatMlmeSet(otInstance *aInstance, otPibAttr aAttr, uint8_t aIndex, uint8_t aLen, const uint8_t *aBuf);

/**
 * Use the MLME_RESET SAP to reset the MAC
 *
 * @param[in] aInstance      The OpenThread instance structure.
 * @param[in] setDefaultPib  Flag indicating whether or not the PIB should be reset to default values
 *
 * @returns an otError code (executes synchronously)
 */
otError otPlatMlmeReset(otInstance *aInstance, bool setDefaultPib);

/**
 * Use the MLME_START SAP. In openthread this is only used to enable beacon response behaviour.
 *
 * @param[in] aInstance       The OpenThread instance structure.
 * @param[in] aStartReq       Struct with configuration values for the start SAP
 *
 * @returns an otError code (executes synchronously)
 */
otError otPlatMlmeStart(otInstance *aInstance, otStartRequest *aStartReq);

/**
 * Use the MLME_SET SAP to set an attribute in the MAC PiB
 *
 * @param[in] aInstance    The OpenThread instance structure.
 * @param[in] aScanRequest A pointer to a scan request struct
 *
 * @returns an otError code
 *
 */
otError     otPlatMlmeScan(otInstance *aInstance, otScanRequest *aScanRequest);
extern void otPlatMlmeBeaconNotifyIndication(otInstance *aInstance, otBeaconNotify *aBeaconNotify);
extern void otPlatMlmeScanConfirm(otInstance *aInstance, otScanConfirm *aScanConfirm);

/**
 * Enable the radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE     Successfully enabled.
 * @retval OT_ERROR_FAILED   The radio could not be enabled.
 */
otError otPlatRadioEnable(otInstance *aInstance);

/**
 * Disable the radio.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @retval OT_ERROR_NONE  Successfully transitioned to Disabled.
 */
otError otPlatRadioDisable(otInstance *aInstance);

/**
 * Check whether radio is enabled or not.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns TRUE if the radio is enabled, FALSE otherwise.
 *
 */
bool otPlatRadioIsEnabled(otInstance *aInstance);

/**
 * The radio driver calls this method to notify OpenThread of a received frame.
 *
 * @param[in]  aInstance        The OpenThread instance structure.
 * @param[in]  aDataIndication  A pointer to the filled otDataIndication struct
 *
 */
extern void otPlatMcpsDataIndication(otInstance *aInstance, otDataIndication *aDataIndication);

/**
 * The radio driver calls this method to notify OpenThread of a failed-security receive
 *
 * @param[in]  aInstance        The OpenThread instance structure.
 * @param[in]  aDataIndication  A pointer to the filled otCommStatusIndication struct
 *
 */
extern void otPlatMlmeCommStatusIndication(otInstance *aInstance, otCommStatusIndication *aCommStatusIndication);

/**
 * This method synch
 *
 * @param[in] aInstance    The OpenThread instance structure.
 * @param[in] aPollRequest Poll request SAP.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the Receive state.
 */
otError otPlatMlmePollRequest(otInstance *aInstance, otPollRequest *aPollRequest);

/**
 * This method begins the transmit sequence on the radio.
 *
 * The caller must form the IEEE 802.15.4 frame in the buffer provided by otPlatRadioGetTransmitBuffer() before
 * requesting transmission.  The channel and transmit power are also included in the otRadioFrame structure.
 *
 * The transmit sequence consists of:
 * 1. Transitioning the radio to Transmit from Receive.
 * 2. Transmits the psdu on the given channel and at the given transmit power.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aFrame     A pointer to the transmitted frame.
 *
 * @retval OT_ERROR_NONE          Successfully transitioned to Transmit.
 * @retval OT_ERROR_INVALID_STATE The radio was not in the Receive state.
 */
otError otPlatMcpsDataRequest(otInstance *aInstance, otDataRequest *aDataRequest);

/**
 * This method removes an indirect data request from the transaction queue (Cancelling it)
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aMsduHandle The application-defined MSDU handle for the frame to be purged
 *
 */
otError otPlatMcpsPurge(otInstance *aInstance, uint8_t aMsduHandle);

/**
 * The radio driver calls this function to notify OpenThread that the transmit operation has completed,
 * providing both the transmitted frame and, if applicable, the received ack frame.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aMsduHandle The application-defined MSDU handle for the sent frame
 * @param[in]  aError      Error code from the IEEE 802.15.4 spec
 *
 */
extern void otPlatMcpsDataConfirm(otInstance *aInstance, uint8_t aMsduHandle, int aMacError);

/**
 * Get the radio's transmit power in dBm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[out] aPower    The transmit power in dBm.
 *
 * @retval OT_ERROR_NONE             Successfully retrieved the transmit power.
 * @retval OT_ERROR_INVALID_ARGS     @p aPower was NULL.
 * @retval OT_ERROR_NOT_IMPLEMENTED  Transmit power configuration via dBm is not implemented.
 *
 */
extern otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower);

/**
 * Set the radio's transmit power in dBm.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 * @param[in] aPower     The transmit power in dBm.
 *
 * @retval OT_ERROR_NONE             Successfully set the transmit power.
 * @retval OT_ERROR_NOT_IMPLEMENTED  Transmit power configuration via dBm is not implemented.
 *
 */
extern otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower);

/**
 * Get the most recent RSSI measurement.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns The RSSI in dBm when it is valid.  127 when RSSI is invalid.
 */
int8_t otPlatRadioGetRssi(otInstance *aInstance);

/**
 * Get the radio receive sensitivity value.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns The radio receive sensitivity value in dBm.
 */
int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance);

/**
 * @}
 *
 */

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_OPENTHREAD_PLATFORM_RADIO_MAC_H_ */
