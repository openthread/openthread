/* Copyright (c) 2009-2019 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*************************************************************************************************/
/*!
 *  \brief Link layer interface file.
 *
 *  Initialization conditional compilation are used to control LL initialization options.
 *  Define one or more of the following to enable roles and features.
 *
 *    - INIT_BROADCASTER (default)
 *    - INIT_OBSERVER
 *    - INIT_PERIPHERAL
 *    - INIT_CENTRAL
 *    - INIT_ENCRYPTED
 *
 *  \note   Each feature may require additional \ref LlRtCfg_t requirements.
 */
/*************************************************************************************************/

#ifndef LL_API_H
#define LL_API_H

#include "wsf_types.h"
#include "wsf_os.h"
#include "util/bda.h"
#include "cfg_mac_ble.h"
#include "ll_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      The following status values are used in the LL API. */
enum
{
  LL_SUCCESS                                            = 0x00,
  LL_ERROR_CODE_UNKNOWN_HCI_CMD                         = 0x01,
  LL_ERROR_CODE_UNKNOWN_CONN_ID                         = 0x02,
  LL_ERROR_CODE_HW_FAILURE                              = 0x03,
  LL_ERROR_CODE_PAGE_TIMEOUT                            = 0x04,
  LL_ERROR_CODE_AUTH_FAILURE                            = 0x05,
  LL_ERROR_CODE_PIN_KEY_MISSING                         = 0x06,
  LL_ERROR_CODE_MEM_CAP_EXCEEDED                        = 0x07,
  LL_ERROR_CODE_CONN_TIMEOUT                            = 0x08,
  LL_ERROR_CODE_CONN_LIMIT_EXCEEDED                     = 0x09,
  LL_ERROR_CODE_SYNCH_CONN_LIMIT_EXCEEDED               = 0x0A,
  LL_ERROR_CODE_ACL_CONN_ALREADY_EXISTS                 = 0x0B,
  LL_ERROR_CODE_CMD_DISALLOWED                          = 0x0C,
  LL_ERROR_CODE_CONN_REJ_LIMITED_RESOURCES              = 0x0D,
  LL_ERROR_CODE_CONN_REJECTED_SECURITY_REASONS          = 0x0E,
  LL_ERROR_CODE_CONN_REJECTED_UNACCEPTABLE_BDADDR       = 0x0F,
  LL_ERROR_CODE_CONN_ACCEPT_TIMEOUT_EXCEEDED            = 0x10,
  LL_ERROR_CODE_UNSUPPORTED_FEATURE_PARAM_VALUE         = 0x11,
  LL_ERROR_CODE_INVALID_HCI_CMD_PARAMS                  = 0x12,
  LL_ERROR_CODE_REMOTE_USER_TERM_CONN                   = 0x13,
  LL_ERROR_CODE_REMOTE_DEVICE_TERM_CONN_LOW_RESOURCES   = 0x14,
  LL_ERROR_CODE_REMOTE_DEVICE_TERM_CONN_POWER_OFF       = 0x15,
  LL_ERROR_CODE_CONN_TERM_BY_LOCAL_HOST                 = 0x16,
  LL_ERROR_CODE_REPEATED_ATTEMPTS                       = 0x17,
  LL_ERROR_CODE_PAIRING_NOT_ALLOWED                     = 0x18,
  LL_ERROR_CODE_UNKNOWN_LMP_PDU                         = 0x19,
  LL_ERROR_CODE_UNSUPPORTED_REMOTE_FEATURE              = 0x1A,
  LL_ERROR_CODE_SCO_OFFSET_REJ                          = 0x1B,
  LL_ERROR_CODE_SCO_INTERVAL_REJ                        = 0x1C,
  LL_ERROR_CODE_SCO_AIR_MODE_REJ                        = 0x1D,
  LL_ERROR_CODE_INVALID_LMP_PARAMS                      = 0x1E,
  LL_ERROR_CODE_UNSPECIFIED_ERROR                       = 0x1F,
  LL_ERROR_CODE_UNSUPPORTED_LMP_PARAM_VAL               = 0x20,
  LL_ERROR_CODE_ROLE_CHANGE_NOT_ALLOWED                 = 0x21,
  LL_ERROR_CODE_LMP_LL_RESP_TIMEOUT                     = 0x22,
  LL_ERROR_CODE_LMP_ERR_TRANSACTION_COLLISION           = 0x23,
  LL_ERROR_CODE_LMP_PDU_NOT_ALLOWED                     = 0x24,
  LL_ERROR_CODE_ENCRYPT_MODE_NOT_ACCEPTABLE             = 0x25,
  LL_ERROR_CODE_LINK_KEY_CAN_NOT_BE_CHANGED             = 0x26,
  LL_ERROR_CODE_REQ_QOS_NOT_SUPPORTED                   = 0x27,
  LL_ERROR_CODE_INSTANT_PASSED                          = 0x28,
  LL_ERROR_CODE_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED     = 0x29,
  LL_ERROR_CODE_DIFFERENT_TRANSACTION_COLLISION         = 0x2A,
  LL_ERROR_CODE_RESERVED1                               = 0x2B,
  LL_ERROR_CODE_QOS_UNACCEPTABLE_PARAM                  = 0x2C,
  LL_ERROR_CODE_QOS_REJ                                 = 0x2D,
  LL_ERROR_CODE_CHAN_ASSESSMENT_NOT_SUPPORTED           = 0x2E,
  LL_ERROR_CODE_INSUFFICIENT_SECURITY                   = 0x2F,
  LL_ERROR_CODE_PARAM_OUT_OF_MANDATORY_RANGE            = 0x30,
  LL_ERROR_CODE_RESERVED2                               = 0x31,
  LL_ERROR_CODE_ROLE_SWITCH_PENDING                     = 0x32,
  LL_ERROR_CODE_RESERVED3                               = 0x33,
  LL_ERROR_CODE_RESERVED_SLOT_VIOLATION                 = 0x34,
  LL_ERROR_CODE_ROLE_SWITCH_FAILED                      = 0x35,
  LL_ERROR_CODE_EXTENDED_INQUIRY_RESP_TOO_LARGE         = 0x36,
  LL_ERROR_CODE_SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST    = 0x37,
  LL_ERROR_CODE_HOST_BUSY_PAIRING                       = 0x38,
  LL_ERROR_CODE_CONN_REJ_NO_SUITABLE_CHAN_FOUND         = 0x39,
  LL_ERROR_CODE_CONTROLLER_BUSY                         = 0x3A,
  LL_ERROR_CODE_UNACCEPTABLE_CONN_INTERVAL              = 0x3B,
  LL_ERROR_CODE_ADV_TIMEOUT                             = 0x3C,
  LL_ERROR_CODE_CONN_TERM_MIC_FAILURE                   = 0x3D,
  LL_ERROR_CODE_CONN_FAILED_TO_ESTABLISH                = 0x3E,
  LL_ERROR_CODE_MAC_CONN_FAILED                         = 0x3F,
  LL_ERROR_CODE_COARSE_CLK_ADJ_REJ                      = 0x40,
  LL_ERROR_CODE_TYPE0_SUBMAP_NOT_DEF                    = 0x41,
  LL_ERROR_CODE_UNKNOWN_ADV_ID                          = 0x42,
  LL_ERROR_CODE_LIMIT_REACHED                           = 0x43,
  LL_ERROR_CODE_OP_CANCELLED_BY_HOST                    = 0x44
};

/*! \addtogroup LL_API_INIT
 *  \{ */

/*! \brief      LL runtime configuration parameters. */
typedef struct
{
  /* Device */
  uint16_t  compId;                 /*!< Company ID (default to ARM Ltd. ID). */
  uint16_t  implRev;                /*!< Implementation revision number. */
  uint8_t   btVer;                  /*!< Core specification implementation level (LL_VER_BT_CORE_SPEC_4_2). */
  uint32_t  _align32;               /*!< Unused. Align next field to word boundary. */
  /* Advertiser */
  uint8_t   maxAdvSets;             /*!< Maximum number of advertising sets. */
  uint8_t   maxAdvReports;          /*!< Maximum number of pending legacy or extended advertising reports. */
  uint16_t  maxExtAdvDataLen;       /*!< Maximum extended advertising data size. */
  uint8_t   defExtAdvDataFrag;      /*!< Default extended advertising data fragmentation size. */
  uint32_t  auxDelayUsec;           /*!< Auxiliary Offset delay above T_MAFS in microseconds. */
  /* Scanner */
  uint8_t   maxScanReqRcvdEvt;      /*!< Maximum scan request received events. */
  uint16_t  maxExtScanDataLen;      /*!< Maximum extended scan data size. */
  /* Connection */
  uint8_t   maxConn;                /*!< Maximum number of connections. */
  uint8_t   numTxBufs;              /*!< Default number of transmit buffers. */
  uint8_t   numRxBufs;              /*!< Default number of receive buffers. */
  uint16_t  maxAclLen;              /*!< Maximum ACL buffer size. */
  int8_t    defTxPwrLvl;            /*!< Default Tx power level for connections. */
  uint8_t   ceJitterUsec;           /*!< Allowable CE jitter on a slave (account for master's sleep clock resolution). */
  /* DTM */
  uint16_t  dtmRxSyncMs;            /*!< DTM Rx synchronization window in milliseconds. */
  /* PHY */
  bool_t    phy2mSup;               /*!< 2M PHY supported. */
  bool_t    phyCodedSup;            /*!< Coded PHY supported. */
  bool_t    stableModIdxTxSup;      /*!< Tx stable modulation index supported. */
  bool_t    stableModIdxRxSup;      /*!< Rx stable modulation index supported. */
} LlRtCfg_t;

/*! \} */    /* LL_API_INIT */

/*! \addtogroup LL_API_DEVICE
 *  \{ */

/* The supported state bitmask indicates the LE states supported by the LL. */
#define LL_SUP_STATE_NON_CONN_ADV                  (UINT64_C(1) <<  0)    /*!< Non-connectable Advertising State supported. */
#define LL_SUP_STATE_SCAN_ADV                      (UINT64_C(1) <<  1)    /*!< Scannable Advertising State supported. */
#define LL_SUP_STATE_CONN_ADV                      (UINT64_C(1) <<  2)    /*!< Connectable Advertising State supported. */
#define LL_SUP_STATE_HI_DUTY_DIR_ADV               (UINT64_C(1) <<  3)    /*!< High Duty Cycle Directed Advertising State supported. */
#define LL_SUP_STATE_PASS_SCAN                     (UINT64_C(1) <<  4)    /*!< Passive Scanning State supported. */
#define LL_SUP_STATE_ACT_SCAN                      (UINT64_C(1) <<  5)    /*!< Active Scanning State supported. */
#define LL_SUP_STATE_INIT                          (UINT64_C(1) <<  6)    /*!< Initiating State supported. Connection State in the Master Role supported is also supported. */
#define LL_SUP_STATE_CONN_SLV                      (UINT64_C(1) <<  7)    /*!< Connection State in the Slave Role supported. */
#define LL_SUP_STATE_NON_CONN_ADV_AND_PASS_SCAN    (UINT64_C(1) <<  8)    /*!< Non-connectable Advertising State and Passive Scanning State combination supported. */
#define LL_SUP_STATE_SCAN_ADV_AND_PASS_SCAN        (UINT64_C(1) <<  9)    /*!< Scannable Advertising State and Passive Scanning State combination supported. */
#define LL_SUP_STATE_CONN_ADV_AND_PASS_SCAN        (UINT64_C(1) << 10)    /*!< Connectable Advertising State and Passive Scanning State combination supported. */
#define LL_SUP_STATE_HI_DUTY_DIR_ADV_AND_PASS_SCAN (UINT64_C(1) << 11)    /*!< Directed Advertising State and Passive Scanning State combination supported. */
#define LL_SUP_STATE_NON_CONN_ADV_AND_ACT_SCAN     (UINT64_C(1) << 12)    /*!< Non-connectable Advertising State and Active Scanning State combination supported. */
#define LL_SUP_STATE_SCAN_ADV_AND_ACT_SCAN         (UINT64_C(1) << 13)    /*!< Scannable Advertising State and Active Scanning State combination supported. */
#define LL_SUP_STATE_CONN_ADV_AND_ACT_SCAN         (UINT64_C(1) << 14)    /*!< Connectable Advertising State and Active Scanning State combination supported. */
#define LL_SUP_STATE_HI_DUTY_DIR_ADV_ACT_SCAN      (UINT64_C(1) << 15)    /*!< Directed Advertising State and Active Scanning State combination supported. */
#define LL_SUP_STATE_NON_CONN_ADV_AND_INIT         (UINT64_C(1) << 16)    /*!< Non-connectable Advertising State and Initiating State combination supported. */
#define LL_SUP_STATE_SCAN_ADV_AND_INIT             (UINT64_C(1) << 17)    /*!< Scannable Advertising State and Initiating State combination supported */
#define LL_SUP_STATE_NON_CONN_ADV_MST              (UINT64_C(1) << 18)    /*!< Non-connectable Advertising State and Master Role combination supported. */
#define LL_SUP_STATE_SCAN_ADV_MST                  (UINT64_C(1) << 19)    /*!< Scannable Advertising State and Master Role combination supported. */
#define LL_SUP_STATE_NON_CONN_ADV_SLV              (UINT64_C(1) << 20)    /*!< Non-connectable Advertising State and Slave Role combination supported. */
#define LL_SUP_STATE_SCAN_ADV_SLV                  (UINT64_C(1) << 21)    /*!< Scannable Advertising State and Slave Role combination supported. */
#define LL_SUP_STATE_PASS_SCAN_AND_INIT            (UINT64_C(1) << 22)    /*!< Passive Scanning State and Initiating State combination supported. */
#define LL_SUP_STATE_ACT_SCAN_AND_INIT             (UINT64_C(1) << 23)    /*!< Active Scanning State and Initiating State combination supported. */
#define LL_SUP_STATE_PASS_SCAN_MST                 (UINT64_C(1) << 24)    /*!< Passive Scanning State and Master Role combination supported. */
#define LL_SUP_STATE_ACT_SCAN_MST                  (UINT64_C(1) << 25)    /*!< Active Scanning State and Master Role combination supported. */
#define LL_SUP_STATE_PASS_SCAN_SLV                 (UINT64_C(1) << 26)    /*!< Passive Scanning state and Slave Role combination supported. */
#define LL_SUP_STATE_ACT_SCAN_SLV                  (UINT64_C(1) << 27)    /*!< Active Scanning state and Slave Role combination supported. */
#define LL_SUP_STATE_INIT_MST                      (UINT64_C(1) << 28)    /*!< Initiating State and Master Role combination supported. Master Role and Master Role combination is also supported. */
#define LL_SUP_STATE_LO_DUTY_DIR_ADV               (UINT64_C(1) << 29)    /*!< Low Duty Cycle Directed Advertising State. */
#define LL_SUP_STATE_LO_DUTY_DIR_ADV_AND_PASS_SCAN (UINT64_C(1) << 30)    /*!< Low Duty Cycle Directed Advertising and Passive Scanning State combination supported. */
#define LL_SUP_STATE_LO_DUTY_DIR_ADV_AND_ACT_SCAN  (UINT64_C(1) << 31)    /*!< Low Duty Cycle Directed Advertising and Active Scanning State combination supported. */
#define LL_SUP_STATE_CONN_ADV_AND_INIT             (UINT64_C(1) << 32)    /*!< Connectable Advertising State and Initiating State combination supported. */
#define LL_SUP_STATE_HI_DUTY_DIR_ADV_AND_INIT      (UINT64_C(1) << 33)    /*!< High Duty Cycle Directed Advertising and Initiating combination supported. */
#define LL_SUP_STATE_LO_DUTY_DIR_ADV_AND_INIT      (UINT64_C(1) << 34)    /*!< Low Duty Cycle Directed Advertising and Initiating combination supported. */
#define LL_SUP_STATE_CONN_ADV_MST                  (UINT64_C(1) << 35)    /*!< Connectable Advertising State and Master Role combination supported. */
#define LL_SUP_STATE_HI_DUTY_DIR_ADV_MST           (UINT64_C(1) << 36)    /*!< High Duty Cycle Directed Advertising and Master Role combination supported. */
#define LL_SUP_STATE_LO_DUTY_DIR_ADV_MST           (UINT64_C(1) << 37)    /*!< Low Duty Cycle Directed Advertising and Master Role combination supported. */
#define LL_SUP_STATE_CONN_ADV_SLV                  (UINT64_C(1) << 38)    /*!< Connectable Advertising State and Slave Role combination supported. */
#define LL_SUP_STATE_HI_DUTY_DIR_ADV_SLV           (UINT64_C(1) << 39)    /*!< High Duty Cycle Directed Advertising and Slave Role combination supported. */
#define LL_SUP_STATE_LO_DUTY_DIR_ADV_SLV           (UINT64_C(1) << 40)    /*!< Low Duty Cycle Directed Advertising and Slave Role combination supported. */
#define LL_SUP_STATE_INIT_SLV                      (UINT64_C(1) << 41)    /*!< Initiating State and Slave Role combination. */

/*! \brief      The features bitmask indicates the LE features supported by the LL. */
enum
{
  /* --- Core Spec 4.0 --- */
  LL_FEAT_ENCRYPTION                  = (1 <<  0),  /*!< Encryption supported. */
  /* --- Core Spec 4.2 --- */
  LL_FEAT_CONN_PARAM_REQ_PROC         = (1 <<  1),  /*!< Connection Parameters Request Procedure supported. */
  LL_FEAT_EXT_REJECT_IND              = (1 <<  2),  /*!< Extended Reject Indication supported. */
  LL_FEAT_SLV_INIT_FEAT_EXCH          = (1 <<  3),  /*!< Slave-Initiated Features Exchange supported. */
  LL_FEAT_LE_PING                     = (1 <<  4),  /*!< LE Ping supported. */
  LL_FEAT_DATA_LEN_EXT                = (1 <<  5),  /*!< Data Length Extension supported. */
  LL_FEAT_PRIVACY                     = (1 <<  6),  /*!< LL Privacy supported. */
  LL_FEAT_EXT_SCAN_FILT_POLICY        = (1 <<  7),  /*!< Extended Scan Filter Policy supported. */
  /* --- Core Spec 5.0 --- */
  LL_FEAT_LE_2M_PHY                   = (1 <<  8),  /*!< LE 2M PHY supported. */
  LL_FEAT_STABLE_MOD_IDX_TRANSMITTER  = (1 <<  9),  /*!< Stable Modulation Index - Transmitter supported. */
  LL_FEAT_STABLE_MOD_IDX_RECEIVER     = (1 << 10),  /*!< Stable Modulation Index - Receiver supported. */
  LL_FEAT_LE_CODED_PHY                = (1 << 11),  /*!< LE Coded PHY supported. */
  LL_FEAT_LE_EXT_ADV                  = (1 << 12),  /*!< LE Extended Advertising supported. */
  LL_FEAT_LE_PER_ADV                  = (1 << 13),  /*!< LE Periodic Advertising supported. */
  LL_FEAT_CH_SEL_2                    = (1 << 14),  /*!< Channel Selection Algorithm #2 supported. */
  LL_FEAT_LE_POWER_CLASS_1            = (1 << 15),  /*!< LE Power Class 1 supported. */
  LL_FEAT_MIN_NUM_USED_CHAN           = (1 << 16),  /*!< Minimum Number of Used Channels supported. */
  /* --- */
  LL_FEAT_ALL_MASK                    = 0x1FFFF     /*!< All feature mask, need to be updated when new features are added. */
};

/*! \brief      This parameter identifies the device role. */
enum
{
  LL_ROLE_MASTER                = 0,            /*!< Role is master. */
  LL_ROLE_SLAVE                 = 1             /*!< Role is slave. */
};

/*! \brief      Operational mode flags. */
enum
{
  LL_OP_MODE_FLAG_ENA_VER_LLCP_STARTUP   = (1 << 0),  /*!< Perform version exchange LLCP at connection establishment. */
  LL_OP_MODE_FLAG_SLV_REQ_IMMED_ACK      = (1 << 1),  /*!< MD bit set if data transmitted. */
  LL_OP_MODE_FLAG_BYPASS_CE_GUARD        = (1 << 2),  /*!< Bypass end of CE guard. */
  LL_OP_MODE_FLAG_MST_RETX_AFTER_RX_NACK = (1 << 3),  /*!< Master retransmits after receiving NACK. */
  LL_OP_MODE_FLAG_MST_IGNORE_CP_RSP      = (1 << 4),  /*!< Master ignores LL_CONNECTION_PARAM_RSP. */
  LL_OP_MODE_FLAG_MST_UNCOND_CP_RSP      = (1 << 5),  /*!< Master unconditionally accepts LL_CONNECTION_PARAM_RSP. */
                                                      /*!<   (LL_OP_MODE_FLAG_MST_IGNORE_CP_RSP must be cleared). */
  LL_OP_MODE_FLAG_ENA_LEN_LLCP_STARTUP   = (1 << 6),  /*!< Perform data length update LLCP at connection establishment. */
  LL_OP_MODE_FLAG_REQ_SYM_PHY            = (1 << 7),  /*!< Require symmetric PHYs for connection. */
  LL_OP_MODE_FLAG_ENA_FEAT_LLCP_STARTUP  = (1 << 8),  /*!< Perform feature exchange LLCP at connection establishment. */
  LL_OP_MODE_FLAG_SLV_DELAY_LLCP_STARTUP = (1 << 9),  /*!< Slave delays LLCP startup procedures. */
  /* diagnostics only */
  LL_OP_MODE_FLAG_ENA_ADV_DLY            = (1 << 16), /*!< Enable advertising delay. */
  LL_OP_MODE_FLAG_ENA_SCAN_BACKOFF       = (1 << 17), /*!< Enable scan backoff. */
  LL_OP_MODE_FLAG_ENA_WW                 = (1 << 18), /*!< Enable window widening. */
  LL_OP_MODE_FLAG_ENA_SLV_LATENCY        = (1 << 19), /*!< Enable slave latency. */
  LL_OP_MODE_FLAG_ENA_LLCP_TIMER         = (1 << 20)  /*!< Enable LLCP timer. */
};

/*! \} */    /* LL_API_DEVICE */

/*! \addtogroup LL_API_BROADCAST
 *  \{ */

/*! \brief      The advertising type indicates the connectable and discoverable nature of the advertising packets transmitted by a device. */
enum
{
  LL_ADV_CONN_UNDIRECT          = 0,            /*!< Connectable undirected advertising.  Peer devices can scan and connect to this device. */
  LL_ADV_CONN_DIRECT_HIGH_DUTY  = 1,            /*!< Connectable directed advertising, high duty cycle.  Only a specified peer device can connect to this device. */
  LL_ADV_SCAN_UNDIRECT          = 2,            /*!< Scannable undirected advertising.  Peer devices can scan this device but cannot connect. */
  LL_ADV_NONCONN_UNDIRECT       = 3,            /*!< Non-connectable undirected advertising.  Peer devices cannot scan or connect to this device. */
  LL_ADV_CONN_DIRECT_LOW_DUTY   = 4             /*!< Connectable directed advertising, low duty cycle.  Only a specified peer device can connect to this device. */
};

/*! \brief      The address type indicates whether an address is public or random. */
enum
{
  LL_ADDR_PUBLIC                = 0,            /*!< Public address. */
  LL_ADDR_RANDOM                = 1,            /*!< Random address. */
  LL_ADDR_PUBLIC_IDENTITY       = 2,            /*!< Public identity address. */
  LL_ADDR_RANDOM_IDENTITY       = 3,            /*!< Random (static) identity address. */
  LL_ADDR_RANDOM_UNRESOLVABLE   = 0xFE,         /*!< Un-resolvable random address. */
  LL_ADDR_ANONYMOUS             = 0xFF          /*!< Anonymous advertiser. */
};

#define LL_ADDR_RANDOM_BIT      LL_ADDR_RANDOM            /*!< Address type random or public bit. */
#define LL_ADDR_IDENTITY_BIT    LL_ADDR_PUBLIC_IDENTITY   /*!< Address type identity bit. */

/*! \brief      Advertising channel bit. */
enum
{
  LL_ADV_CHAN_37_BIT            = (1 << 0),     /*!< Advertising channel 37. */
  LL_ADV_CHAN_38_BIT            = (1 << 1),     /*!< Advertising channel 38. */
  LL_ADV_CHAN_39_BIT            = (1 << 2),     /*!< Advertising channel 39. */
  LL_ADV_CHAN_ALL               = 0x7,          /*!< All advertising channels. */
};

/*! \brief      Advertising filter policy. */
enum
{
  LL_ADV_FILTER_NONE            = 0,            /*!< Scan from any device. */
  LL_ADV_FILTER_SCAN_WL_BIT     = 1,            /*!< Scan from White List only. */
  LL_ADV_FILTER_CONN_WL_BIT     = 2,            /*!< Connect from While List only. */
  LL_ADV_FILTER_WL_ONLY         = 3             /*!< Scan and connect from While List only. */
};

/*! \brief      Advertising event properties. */
enum
{
  LL_ADV_EVT_PROP_CONN_ADV_BIT      = (1 << 0), /*!< Connectable advertising. */
  LL_ADV_EVT_PROP_SCAN_ADV_BIT      = (1 << 1), /*!< Scannable advertising. */
  LL_ADV_EVT_PROP_DIRECT_ADV_BIT    = (1 << 2), /*!< Directed advertising. */
  LL_ADV_EVT_PROP_HIGH_DUTY_ADV_BIT = (1 << 3), /*!< High Duty Cycle advertising. */
  LL_ADV_EVT_PROP_LEGACY_ADV_BIT    = (1 << 4), /*!< Use legacy advertising PDUs. */
  LL_ADV_EVT_PROP_OMIT_AA_BIT       = (1 << 5), /*!< Omit advertiser's address from all PDUs (anonymous advertising). */
  LL_ADV_EVT_PROP_TX_PWR_BIT        = (1 << 6)  /*!< Include TxPower in the advertising PDU. */
};

#define LL_ADV_EVT_PROP_NON_CONN_NON_SCAN   0   /*!< Non-connectable and non-scannable advertising. */

/*! \brief      Extended advertising parameters. */
typedef struct
{
  uint16_t      advEventProp;       /*!< Advertising Event Properties. */
  uint32_t      priAdvInterMin;     /*!< Primary Advertising Interval Minimum. */
  uint32_t      priAdvInterMax;     /*!< Primary Advertising Interval Maximum. */
  uint8_t       priAdvChanMap;      /*!< Primary Advertising Channel Map. */
  uint8_t       ownAddrType;        /*!< Own Address Type. */
  uint8_t       peerAddrType;       /*!< Peer Address Type. */
  uint8_t       *pPeerAddr;         /*!< Peer Address. */
  uint8_t       advFiltPolicy;      /*!< Advertising Filter Policy. */
  int8_t        advTxPwr;           /*!< Advertising Tx Power. */
  uint8_t       priAdvPhy;          /*!< Primary Advertising PHY. */
  uint8_t       secAdvMaxSkip;      /*!< Secondary Advertising Maximum Skip. */
  uint8_t       secAdvPhy;          /*!< Secondary Advertising PHY. */
  uint8_t       advSID;             /*!< Advertising SID. */
  uint8_t       scanReqNotifEna;    /*!< Scan Request Notification Enable. */
} LlExtAdvParam_t;

/*! \brief      Extended advertising enable parameters. */
typedef struct
{
  uint8_t       handle;             /*!< Advertising handle. */
  uint16_t      duration;           /*!< Duration. */
  uint8_t       numEvents;          /*!< Maximum number of extended advertising events. */
} LlExtAdvEnableParam_t;

/*! \brief      Periodic advertising parameters. */
typedef struct
{
  uint16_t      perAdvInterMin;     /*!< Periodic Advertising Interval Minimum. */
  uint16_t      perAdvInterMax;     /*!< Periodic Advertising Interval Maximum. */
  uint16_t      perAdvProp;         /*!< Periodic Advertising Properties. */
} LlPerAdvParam_t;

/*! \brief       Advertising data operation. */
enum
{
  LL_ADV_DATA_OP_FRAG_INTER     = 0,            /*!< Intermediate fragment. */
  LL_ADV_DATA_OP_FRAG_FIRST     = 1,            /*!< First fragment. */
  LL_ADV_DATA_OP_FRAG_LAST      = 2,            /*!< Last fragment. */
  LL_ADV_DATA_OP_COMP           = 3,            /*!< Complete extended advertising data. */
  LL_ADV_DATA_OP_UNCHANGED      = 4             /*!< Unchanged data (just update the Advertising DID). */
};

/*! \brief       Advertising data fragment preference. */
enum
{
  LL_ADV_DATA_FRAG_ALLOW        = 0,            /*!< Controller may fragment all Host advertising data. */
  LL_ADV_DATA_FRAG_DISALLOW     = 1             /*!< Controller should not fragment nor minimize fragmentation of Host advertising data. */
};

/*! \} */    /* LL_API_BROADCAST */

/*! \addtogroup LL_API_OBSERVER
 *  \{ */

/*! \brief      Type of scan. */
enum
{
  LL_SCAN_PASSIVE               = 0,            /*!< Passive scanning. */
  LL_SCAN_ACTIVE                = 1             /*!< Active scanning. */
};

/*! \brief      Scan filter policy. */
enum
{
  LL_SCAN_FILTER_NONE           = 0,            /*!< Accept all advertising packets. */
  LL_SCAN_FILTER_WL_BIT         = 1,            /*!< Accept from While List only. */
  LL_SCAN_FILTER_RES_INIT_BIT   = 2,            /*!< Accept directed advertisements with RPAs. */
  LL_SCAN_FILTER_WL_OR_RES_INIT = 3             /*!< Accept from White List or directed advertisements with RPAs. */
};

/*! \brief      Periodic scan filter policy. */
enum
{
  LL_PER_SCAN_FILTER_NONE       = 0,            /*!< Use advSID, advAddrType and advAddr to filter. */
  LL_PER_SCAN_FILTER_PL_BIT     = 1,            /*!< Use the periodic advertiser list. */
};

/*! \brief      Scan parameters. */
typedef struct
{
  uint16_t      scanInterval;                   /*!< Scan interval. */
  uint16_t      scanWindow;                     /*!< Scan window. */
  uint8_t       scanType;                       /*!< Scan type. */
  uint8_t       ownAddrType;                    /*!< Address type used by this device. */
  uint8_t       scanFiltPolicy;                 /*!< Scan filter policy. */
} LlScanParam_t;

/*! \brief      Extended scan parameters. */
typedef struct
{
  uint16_t      scanInterval;                   /*!< Scan interval. */
  uint16_t      scanWindow;                     /*!< Scan window. */
  uint8_t       scanType;                       /*!< Scan type. */
} LlExtScanParam_t;

/*! \brief      Scan filter modes for duplicate report. */
enum
{
  LL_SCAN_FILTER_DUP_DISABLE            = 0x00, /*!< Duplicate filtering disabled. */
  LL_SCAN_FILTER_DUP_ENABLE_ONCE        = 0x01, /*!< Duplicate filtering enabled. */
  LL_SCAN_FILTER_DUP_ENABLE_PERIODIC    = 0x02  /*!< Duplicate filtering enabled, reset for each scan period. */
};

/*! \brief       Advertising report event types. */
enum
{
  LL_RPT_TYPE_ADV_IND           = 0x00,         /*!< Connectable undirected advertising (ADV_IND). */
  LL_RPT_TYPE_ADV_DIRECT_IND    = 0x01,         /*!< Connectable directed advertising (ADV_DIRECT_IND). */
  LL_RPT_TYPE_ADV_SCAN_IND      = 0x02,         /*!< Scannable undirected advertising (ADV_SCAN_IND). */
  LL_RPT_TYPE_ADV_NONCONN_IND   = 0x03,         /*!< Non connectable undirected advertising (ADV_NONCONN_IND). */
  LL_RPT_TYPE_SCAN_RSP          = 0x04          /*!< Scan Response (SCAN_RSP). */
};

/*! \brief      Periodic advertising create sync command. */
typedef struct
{
  uint8_t   filterPolicy;   /*!< Filter Policy. */
  uint8_t   advSID;         /*!< Advertising SID. */
  uint8_t   advAddrType;    /*!< Advertiser Address Type. */
  uint8_t   *pAdvAddr;      /*!< Advertiser Address. */
  uint16_t  skip;           /*!< Skip. */
  uint16_t  syncTimeOut;    /*!< Synchronization Timeout. */
} LlPerAdvCreateSyncCmd_t;

/*! \brief      Device in periodic advertiser list */
typedef struct
{
  uint8_t   advAddrType;    /*!< Advertiser Address Type. */
  uint8_t   *pAdvAddr;      /*!< Advertiser Address. */
  uint8_t   advSID;         /*!< Advertising SID. */
} LlDevicePerAdvList_t;

/*! \} */    /* LL_API_OBSERVER */

/*! \addtogroup LL_API_CONN
 *  \{ */

/*! \brief      Master clock accuracy. */
enum
{
  LL_MCA_500_PPM                = 0x00,         /*!< 500 ppm */
  LL_MCA_250_PPM                = 0x01,         /*!< 250 ppm */
  LL_MCA_150_PPM                = 0x02,         /*!< 150 ppm */
  LL_MCA_100_PPM                = 0x03,         /*!< 100 ppm */
  LL_MCA_75_PPM                 = 0x04,         /*!< 75 ppm */
  LL_MCA_50_PPM                 = 0x05,         /*!< 50 ppm */
  LL_MCA_30_PPM                 = 0x06,         /*!< 30 ppm */
  LL_MCA_20_PPM                 = 0x07          /*!< 20 ppm */
};

/*! \brief      PHYS specification. */
enum
{
  LL_PHYS_NONE                  = 0,            /*!< No selected PHY. */
  LL_PHYS_LE_1M_BIT             = (1 << 0),     /*!< LE 1M PHY. */
  LL_PHYS_LE_2M_BIT             = (1 << 1),     /*!< LE 2M PHY. */
  LL_PHYS_LE_CODED_BIT          = (1 << 2),     /*!< LE Coded PHY. */
};

/*! \brief     All PHYs preference. */
enum
{
  LL_ALL_PHY_ALL_PREFERENCES    = 0,            /*!< All PHY preferences. */
  LL_ALL_PHY_TX_PREFERENCE_BIT  = (1 << 0),     /*!< Set if no Tx PHY preference. */
  LL_ALL_PHY_RX_PREFERENCE_BIT  = (1 << 1)      /*!< Set if no Rx PHY preference. */
};

/*! \brief      PHY options. */
enum
{
  LL_PHY_OPTIONS_NONE           = 0,            /*!< No preferences. */
  LL_PHY_OPTIONS_S2_PREFERRED   = 1,            /*!< S=2 coding preferred when transmitting on LE Coded PHY. */
  LL_PHY_OPTIONS_S8_PREFERRED   = 2,            /*!< S=8 coding preferred when transmitting on LE Coded PHY. */
};

/*! \brief      PHY types. */
enum
{
  LL_PHY_NONE                   = 0,            /*!< PHY not selected. */
  LL_PHY_LE_1M                  = 1,            /*!< LE 1M PHY. */
  LL_PHY_LE_2M                  = 2,            /*!< LE 2M PHY. */
  LL_PHY_LE_CODED               = 3             /*!< LE Coded PHY. */
};

/*! \brief      Privacy modes. */
enum
{
  LL_PRIV_MODE_NETWORK          = 0,            /*!< Network privacy mode. */
  LL_PRIV_MODE_DEVICE           = 1,            /*!< Device privacy mode. */
};

/*! \brief      Initiating parameters (\a LlExtCreateConn()). */
typedef struct
{
  uint16_t      scanInterval;                   /*!< Scan interval. */
  uint16_t      scanWindow;                     /*!< Scan window. */
  uint8_t       filterPolicy;                   /*!< Scan filter policy. */
  uint8_t       ownAddrType;                    /*!< Address type used by this device. */
  uint8_t       peerAddrType;                   /*!< Address type used for peer device. */
  const uint8_t *pPeerAddr;                     /*!< Address of peer device. */
} LlInitParam_t;

/*! \brief      Initiating parameters (\a LlExtCreateConn()). */
typedef struct
{
  uint8_t       filterPolicy;                   /*!< Scan filter policy. */
  uint8_t       ownAddrType;                    /*!< Address type used by this device. */
  uint8_t       peerAddrType;                   /*!< Address type used for peer device. */
  const uint8_t *pPeerAddr;                     /*!< Address of peer device. */
  uint8_t       initPhys;                       /*!< Initiating PHYs. */
} LlExtInitParam_t;

/*! \brief      Initiating scan parameters (\a LlExtCreateConn()). */
typedef struct
{
  uint16_t      scanInterval;                   /*!< Scan interval. */
  uint16_t      scanWindow;                     /*!< Scan window. */
} LlExtInitScanParam_t;

/*! \brief      Connection specification (\a LlCreateConn(), \a LlConnUpdate() and \a LlExtCreateConn()). */
typedef struct
{
  uint16_t      connIntervalMin;                /*!< Minimum connection interval. */
  uint16_t      connIntervalMax;                /*!< Maximum connection interval. */
  uint16_t      connLatency;                    /*!< Connection latency. */
  uint16_t      supTimeout;                     /*!< Supervision timeout. */
  uint16_t      minCeLen;                       /*!< Minimum CE length. */
  uint16_t      maxCeLen;                       /*!< Maximum CE length. */
} LlConnSpec_t;

/*! \brief      Channel selection algorithm methods. */
enum
{
  LL_CH_SEL_1                   = 0,            /*!< Channel selection #1. */
  LL_CH_SEL_2                   = 1             /*!< Channel selection #2. */
};

/*! \brief      Tx power level type. */
enum
{
  LL_TX_PWR_LVL_TYPE_CURRENT    = 0x00,         /*!< Current transmit power level. */
  LL_TX_PWR_LVL_TYPE_MAX        = 0x01          /*!< Maximum transmit power level. */
};

/*! \} */    /* LL_API_CONN */

/*! \addtogroup LL_API_ENCRYPT
 *  \{ */

/*! \brief       Nonce mode. */
enum
{
  LL_NONCE_MODE_PKT_CNTR        = 0x00,         /*!< Packet counter nonce mode (default). */
  LL_NONCE_MODE_EVT_CNTR        = 0x01          /*!< Connection event counter mode. */
};

/*! \brief      Encryption mode data structure used in LlGetEncMode() and LlSetEncMode(). */
typedef struct
{
  bool_t        enaAuth;            /*!< Enable authentication. */
  bool_t        nonceMode;          /*!< Nonce mode. */
} LlEncMode_t;

/*! \} */    /* LL_API_ENCRYPT */

/*! \addtogroup LL_API_TEST
 *  \{ */

/*! \brief      Test packet payload type. */
enum
{
  LL_TEST_PKT_TYPE_PRBS9        = 0x00,         /*!< Pseudo-Random bit sequence 9. */
  LL_TEST_PKT_TYPE_0F           = 0x01,         /*!< 00001111'b packet payload type. */
  LL_TEST_PKT_TYPE_55           = 0x02,         /*!< 01010101'b packet payload type. */
  LL_TEST_PKT_TYPE_PRBS15       = 0x03,         /*!< Pseudo-Random bit sequence 15. */
  LL_TEST_PKT_TYPE_FF           = 0x04,         /*!< 11111111'b packet payload type. */
  LL_TEST_PKT_TYPE_00           = 0x05,         /*!< 00000000'b packet payload type. */
  LL_TEST_PKT_TYPE_F0           = 0x06,         /*!< 11110000'b packet payload type. */
  LL_TEST_PKT_TYPE_AA           = 0x07          /*!< 10101010'b packet payload type. */
};

/*! \brief      Test PHY type. */
enum
{
  LL_TEST_PHY_LE_1M             = 0x01,         /*!< LE 1M PHY. */
  LL_TEST_PHY_LE_2M             = 0x02,         /*!< LE 2M PHY. */
  LL_TEST_PHY_LE_CODED          = 0x03,         /*!< LE Coded PHY (data coding unspecified). */
  LL_TEST_PHY_LE_CODED_S8       = 0x03,         /*!< LE Coded PHY with S=8 data coding. */
  LL_TEST_PHY_LE_CODED_S2       = 0x04          /*!< LE Coded PHY with S=2 data coding. */
};

/*! \brief      Test modulation index. */
enum
{
  LL_TEST_MOD_IDX_STANDARD      = 0x00,         /*!< Standard modulation index. */
  LL_TEST_MOD_IDX_STABLE        = 0x01          /*!< Stable modulation index. */
};

/*! \brief      Test report data. */
typedef struct
{
  uint16_t      numTx;          /*!< Total transmit packet count. */
  uint16_t      numRxSuccess;   /*!< Successfully received packet count. */
  uint16_t      numRxCrcError;  /*!< CRC failed packet count. */
  uint16_t      numRxTimeout;   /*!< Receive timeout count. */
} LlTestReport_t;

/*! \} */    /* LL_API_TEST */

/*! \addtogroup LL_API_EVENT
 *  \{ */

/*! \brief      Link control callback interface events */
enum
{
  LL_ERROR_IND,                 /*!< Unrecoverable LL or radio error occurred (vendor specific). */
  /* --- Core Spec 4.0 --- */
  LL_RESET_CNF,                 /*!< Reset complete. */
  LL_ADV_REPORT_IND,            /*!< Advertising report. */
  LL_ADV_ENABLE_CNF,            /*!< Advertising enable/disable complete. */
  LL_SCAN_ENABLE_CNF,           /*!< Scan enable/disable complete. */
  LL_CONN_IND,                  /*!< Connection complete. */
  LL_DISCONNECT_IND,            /*!< Disconnect complete. */
  LL_CONN_UPDATE_IND,           /*!< Connection update complete. */
  LL_CREATE_CONN_CANCEL_CNF,    /*!< Create connection cancel status. */
  LL_READ_REMOTE_VER_INFO_CNF,  /*!< Read remote version information complete. */
  LL_READ_REMOTE_FEAT_CNF,      /*!< Read remote features complete. */
  LL_ENC_CHANGE_IND,            /*!< Encryption change. */
  LL_ENC_KEY_REFRESH_IND,       /*!< Key refresh. */
  LL_LTK_REQ_IND,               /*!< LTK request. */
  LL_LTK_REQ_NEG_REPLY_CNF,     /*!< LTK request negative reply status. */
  LL_LTK_REQ_REPLY_CNF,         /*!< LTK request reply status. */
  /* --- Core Spec 4.2 --- */
  LL_REM_CONN_PARAM_IND,        /*!< Remote connection parameter change. */
  LL_AUTH_PAYLOAD_TIMEOUT_IND,  /*!< Authentication payload timeout expired. */
  LL_DATA_LEN_CHANGE_IND,       /*!< Data length changed. */
  LL_READ_LOCAL_P256_PUB_KEY_CMPL_IND, /*!< Read local P-256 public key complete. */
  LL_GENERATE_DHKEY_CMPL_IND,          /*!< Generate Diffie-Hellman key complete. */
  LL_SCAN_REPORT_IND,           /*!< Legacy scan report (vendor specific). */
  /* --- Core Spec 5.0 --- */
  LL_PHY_UPDATE_IND,            /*!< LE PHY update complete. */
  LL_EXT_ADV_REPORT_IND,        /*!< Extended advertising report. */
  LL_EXT_SCAN_ENABLE_CNF,       /*!< Extended scan enable/disable complete. */
  LL_SCAN_TIMEOUT_IND,          /*!< Scan timeout. */
  LL_SCAN_REQ_RCVD_IND,         /*!< Scan request received. */
  LL_EXT_ADV_ENABLE_CNF,        /*!< Extended advertising enable/disable complete. */
  LL_ADV_SET_TERM_IND,          /*!< Advertising set terminated complete. */
  LL_PER_ADV_ENABLE_CNF,        /*!< Periodic advertising enable/disable complete. */
  LL_PER_ADV_SYNC_ESTD_IND,     /*!< Periodic scanning synchronization established. */
  LL_PER_ADV_SYNC_LOST_IND,     /*!< Periodic scanning synchronization lost. */
  LL_PER_ADV_REPORT_IND,        /*!< Periodic scanning report. */
  LL_CH_SEL_ALGO_IND            /*!< Channel selection algorithm. */
};

/*! \brief      Advertising report indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       *pData;         /*!< Data buffer. */
  uint8_t       len;            /*!< Data buffer length. */
  int8_t        rssi;           /*!< RSSI. */
  uint8_t       eventType;      /*!< Event type. */
  uint8_t       addrType;       /*!< Address type. */
  bdAddr_t      addr;           /*!< Address. */
  /* --- direct fields --- */
  uint8_t       directAddrType; /*!< Direct address type. */
  bdAddr_t      directAddr;     /*!< Direct address. */
} LlAdvReportInd_t;

/*! \brief      Connect indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
  uint8_t       role;           /*!< Role of this device. */
  uint8_t       addrType;       /*!< Address type. */
  bdAddr_t      peerAddr;       /*!< Peer address. */
  uint16_t      connInterval;   /*!< Connection interval. */
  uint16_t      connLatency;    /*!< Connection latency. */
  uint16_t      supTimeout;     /*!< Supervision timeout. */
  uint8_t       clockAccuracy;  /*!< Clock accuracy. */
  /* --- enhanced fields --- */
  bdAddr_t      localRpa;       /*!< Local resolvable private address. */
  bdAddr_t      peerRpa;        /*!< Peer resolvable private address. */
} LlConnInd_t;

/*! \brief      Disconnect indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
  uint8_t       reason;         /*!< Reason code. */
} LlDisconnectInd_t;

/*! \brief      Connect update indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
  uint16_t      connInterval;   /*!< Connection interval. */
  uint16_t      connLatency;    /*!< Connection latency. */
  uint16_t      supTimeout;     /*!< Supervision timeout. */
} LlConnUpdateInd_t;

/*! \brief      Connection parameter change indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint16_t      handle;         /*!< Connection handle. */
  uint16_t      connIntervalMin;/*!< Minimum connection interval. */
  uint16_t      connIntervalMax;/*!< Maximum connection interval. */
  uint16_t      connLatency;    /*!< Connection latency. */
  uint16_t      supTimeout;     /*!< Supervision timeout. */
} LlRemConnParamInd_t;

/*! \brief      Create connection cancel confirm */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
} LlCreateConnCancelCnf_t;

/*! \brief      Read remote version information confirm */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
  uint8_t       version;        /*!< Bluetooth specification version. */
  uint16_t      mfrName;        /*!< Manufacturer ID. */
  uint16_t      subversion;     /*!< Subversion. */
} LlReadRemoteVerInfoCnf_t;

#define LL_FEAT_LEN             8       /*!< Length of features byte array */

/*! \brief      Read remote feature confirm */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
  uint8_t       features[LL_FEAT_LEN];  /*!< Features. */
} LlReadRemoteFeatCnf_t;

/*! \brief      Encryption change indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
  bool_t        enabled;        /*!< Encryption enabled. */
} LlEncChangeInd_t;

/*! \brief      Key refresh indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
} LlEncKeyRefreshInd_t;

/*! \brief      LTK request indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint16_t      handle;         /*!< Connection handle. */
  uint8_t       randNum[LL_RAND_LEN];   /*!< Random number. */
  uint16_t      encDiversifier; /*!< Encryption diversifier. */
} LlLtkReqInd_t;

/*! \brief      LTK request reply confirm */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
} LlLtkReqReplyCnf_t;

/*! \brief      LTK request negative reply */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
} LlLtkReqNegReplyCnf_t;

/*! \brief      Authentication payload timeout expired indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint16_t      handle;         /*!< Connection handle. */
} LlAuthPayloadTimeoutInd_t;

/*! \brief      Data length change */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint16_t      handle;         /*!< Connection handle. */
  uint16_t      maxTxLen;       /*!< Maximum transmit length. */
  uint16_t      maxTxTime;      /*!< Maximum transmit time in microseconds. */
  uint16_t      maxRxLen;       /*!< Maximum receive length. */
  uint16_t      maxRxTime;      /*!< Maximum receive time in microseconds. */
} LlDataLenChangeInd_t;

/*! \brief      Read local P-256 key pair complete */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint8_t       pubKey_x[LL_ECC_KEY_LEN];  /*!< Public key x-coordinate. */
  uint8_t       pubKey_y[LL_ECC_KEY_LEN];  /*!< Public key y-coordinate. */
} LlReadLocalP256PubKeyInd_t;

/*! \brief      Generate Diffie-Hellman key complete */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint8_t       dhKey[LL_ECC_KEY_LEN];     /*!< Diffie-Hellman key. */
} LlGenerateDhKeyInd_t;

/*! \brief      PHY update complete. */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      handle;         /*!< Connection handle. */
  uint8_t       txPhy;          /*!< Transceiver PHY. */
  uint8_t       rxPhy;          /*!< Receiver PHY. */
} LlPhyUpdateInd_t;

/*! \brief      HW error */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       code;           /*!< Code. */
} LlHwErrorInd_t;

/*! \brief      Scan report */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       peerAddrType;   /*!< Peer address type. */
  uint64_t      peerAddr;       /*!< Peer address. */
  uint64_t      peerRpa;        /*!< Peer RPA. */
} LlScanReportInd_t;

/*! \brief      Extended advertising enable */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint8_t       handle;         /*!< Advertising handle. */
} LlExtAdvEnableCnf_t;

/*! \brief      Periodic advertising enable */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint8_t       handle;         /*!< Advertising handle. */
} LlPerAdvEnableCnf_t;

/*! \brief      Extended advertising report event types. */
enum
{
  LL_RPT_EVT_CONN_ADV_BIT       = (1 << 0),     /*!< Connectable advertising event bit. */
  LL_RPT_EVT_SCAN_ADV_BIT       = (1 << 1),     /*!< Scannable advertising event bit. */
  LL_RPT_EVT_DIRECT_ADV_BIT     = (1 << 2),     /*!< Directed advertising event bit. */
  LL_RPT_EVT_SCAN_RSP_BIT       = (1 << 3),     /*!< Scan response event bit. */
  LL_RPT_EVT_LEGACY_ADV_BIT     = (1 << 4),     /*!< Legacy advertising PDU event bit. */
};

/*! \brief      Extended advertising report data status. */
enum
{
  LL_RPT_DATA_CMPL              = 0x00,         /*!< Data complete. */
  LL_RPT_DATA_INC_MORE          = 0x01,         /*!< Data incomplete, more date to come. */
  LL_RPT_DATA_INC_TRUNC         = 0x02          /*!< Data incomplete, data truncated, no more date to come. */
};

/*! \brief      Special SID values. */
enum
{
  LL_SID_NO_ADI                 = 0xFF          /*!< No ADI field in the PDU. */
};

/*! \brief      Extended advertising report */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint16_t      eventType;      /*!< Event type. */
  uint8_t       addrType;       /*!< Address type. */
  bdAddr_t      addr;           /*!< Address. */
  uint8_t       priPhy;         /*!< Primary PHY. */
  uint8_t       secPhy;         /*!< Secondary PHY. */
  uint8_t       advSID;         /*!< Advertising SID. */
  int8_t        txPwr;          /*!< Tx Power. */
  int8_t        rssi;           /*!< RSSI. */
  int16_t       perAdvInter;    /*!< Periodic advertising interval. */
  uint8_t       directAddrType; /*!< Direct address type. */
  bdAddr_t      directAddr;     /*!< Direct address. */
  uint16_t      len;            /*!< Data buffer length. */
  const uint8_t *pData;         /*!< Data buffer. */
} LlExtAdvReportInd_t;

/*! \brief      Extended scan enable confirm */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
} LlExtScanEnableCnf_t;

/*! \brief      Advertising set terminated */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint8_t       advHandle;      /*!< Advertising handle. */
  uint16_t      connHandle;     /*!< Connection handle. */
  uint8_t       numCmplAdvEvt;  /*!< Number of completed advertising events. */
} LlAdvSetTermInd_t;

/*! \brief      Scan request received */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       handle;         /*!< Advertising handle. */
  uint8_t       scanAddrType;   /*!< Scanner address type. */
  bdAddr_t      scanAddr;       /*!< Scanner address. */
} LlScanReqRcvdInd_t;

/*! \brief      Used channel selection indication */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint16_t      handle;         /*!< Connection handle. */
  uint8_t       usedChSel;      /*!< Used channel selection. */
} LlChSelInd_t;

/*! \brief     LE periodic advertising synchronization established */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint8_t       status;         /*!< Status. */
  uint16_t      syncHandle;     /*!< Sync handle. */
  uint8_t       advSID;         /*!< Advertising SID. */
  uint8_t       addrType;       /*!< Advertiser address type. */
  bdAddr_t      addr;           /*!< Advertiser address. */
  uint8_t       advPhy;         /*!< Advertiser PHY. */
  uint16_t      perAdvInterval; /*!< Periodic advertising interval. */
  uint8_t       advClkAccuracy; /*!< Advertiser clock accuracy. */
} LlPerAdvSyncEstdCnf_t;

/*! \brief     LE periodic advertising report */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint16_t      syncHandle;     /*!< Sync handle. */
  int8_t        txPwr;          /*!< Tx Power. */
  int8_t        rssi;           /*!< RSSI. */
  uint8_t       unused;         /*!< Future use. */
  uint8_t       dataStatus;     /*!< Data status. */
  uint16_t      len;            /*!< Data buffer length. */
  const uint8_t *pData;         /*!< Data buffer. */
} LlPerAdvReportInd_t;

/*! \brief     LE periodic advertising sync lost */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< Event header. */
  uint16_t      syncHandle;     /*!< Sync handle. */
} LlPerAdvSyncLostInd_t;

/*! \brief      Union of all event types */
typedef union
{
  wsfMsgHdr_t               hdr;                    /*!< Event header. */
  LlHwErrorInd_t            hwErrorInd;             /*!< Unrecoverable LL or radio error occurred. */
  /* --- Core Spec 4.0 --- */
  LlAdvReportInd_t          advReportInd;           /*!< LE advertising report. */
  LlConnInd_t               connInd;                /*!< LE connection complete. */
  LlDisconnectInd_t         disconnectInd;          /*!< LE disconnect complete. */
  LlConnUpdateInd_t         connUpdateInd;          /*!< LE connection update complete. */
  LlCreateConnCancelCnf_t   createConnCancelCnf;    /*!< LE create connection cancel status. */
  LlReadRemoteVerInfoCnf_t  readRemoteVerInfoCnf;   /*!< Read remote version information complete. */
  LlReadRemoteFeatCnf_t     readRemoteFeatCnf;      /*!< LE read remote features complete. */
  LlEncChangeInd_t          encChangeInd;           /*!< Encryption change. */
  LlEncKeyRefreshInd_t      keyRefreshInd;          /*!< Key refresh. */
  LlLtkReqInd_t             ltkReqInd;              /*!< LE LTK request. */
  LlLtkReqNegReplyCnf_t     ltkReqNegReplyCnf;      /*!< LTK request negative reply status. */
  LlLtkReqReplyCnf_t        ltkReqReplyCnf;         /*!< LTK request reply status. */
  /* --- Core Spec 4.2 --- */
  LlRemConnParamInd_t       remConnParamInd;        /*!< LE remote connection parameter request. */
  LlAuthPayloadTimeoutInd_t authPayloadTimeoutInd;  /*!< Authentication payload timeout. */
  LlDataLenChangeInd_t      dataLenChangeInd;       /*!< Data length changed. */
  LlReadLocalP256PubKeyInd_t  readLocalP256PubKeyInd; /*!< Read local P-256 public key complete. */
  LlGenerateDhKeyInd_t        generateDhKeyInd;       /*!< Generate Diffie-Hellman key complete. */
  LlScanReportInd_t         scanReportInd;          /*!< Scan report. */
  /* --- Core Spec 5.0 --- */
  LlPhyUpdateInd_t          phyUpdateInd;           /*!< PHY update complete. */
  LlExtAdvReportInd_t       extAdvReportInd;        /*!< LE extended advertising report. */
  LlExtScanEnableCnf_t      extScanEnableCnf;       /*!< LE extended scan enable completed. */
  LlScanReqRcvdInd_t        scanReqRcvdInd;         /*!< LE scan request received. */
  LlExtAdvEnableCnf_t       extAdvEnableCnf;        /*!< LE extended advertising enable complete. */
  LlAdvSetTermInd_t         advSetTermInd;          /*!< LE advertising set terminated. */
  LlChSelInd_t              usedChSelInd;           /*!< Used channel selection. */
  LlPerAdvEnableCnf_t       perAdvEnableCnf;        /*!< LE periodic advertising enable complete. */
  LlPerAdvSyncEstdCnf_t     perAdvSyncEstdCnf;      /*!< LE periodic advertising sync established. */
  LlPerAdvReportInd_t       perAdvReportInd;        /*!< LE periodic advertising report. */
  LlPerAdvSyncLostInd_t     perAdvSyncLostInd;      /*!< LE periodic advertising sync lost. */

} LlEvt_t;

/*! \brief      Event callback */
typedef bool_t (*llEvtCback_t)(LlEvt_t *pEvent);

/*! \brief      ACL callback */
typedef void (*llAclCback_t)(uint16_t handle, uint8_t numBufs);

/*! \} */    /* LL_API_EVENT */

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*! \addtogroup LL_API_INIT
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Get default runtime configuration values.
 *
 *  \param      pCfg            Pointer to runtime configuration parameters.
 *
 *  \return     None.
 *
 *  This function returns default value for the LL subsystem's runtime configurations.
 */
/*************************************************************************************************/
void LlGetDefaultRunTimeCfg(LlRtCfg_t *pCfg);

/*************************************************************************************************/
/*!
 *  \brief      Initialize runtime configuration.
 *
 *  \param      pCfg            Pointer to runtime configuration parameters (data must be static).
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem's runtime configuration.
 *
 *  \note       This routine must be called only once before any other initialization routines.
 */
/*************************************************************************************************/
void LlInitRunTimeCfg(const LlRtCfg_t *pCfg);

/*************************************************************************************************/
/*!
 *  \brief      Initialize memory for connections.
 *
 *  \param      pFreeMem        Pointer to free memory.
 *  \param      freeMemSize     Size of pFreeMem.
 *
 *  \return     Amount of free memory consumed.
 *
 *  This function allocates memory for connections.
 *
 *  \note       This routine must be called after LlInitRunTimeCfg() but only once before any
 *              other initialization routines.
 */
/*************************************************************************************************/
uint16_t LlInitConnMem(uint8_t *pFreeMem, uint32_t freeMemSize);

/*************************************************************************************************/
/*!
 *  \brief      Initialize memory for extended advertising.
 *
 *  \param      pFreeMem        Pointer to free memory.
 *  \param      freeMemSize     Size of pFreeMem.
 *
 *  \return     Amount of free memory consumed.
 *
 *  This function allocates memory for extended advertising.
 *
 *  \note       This routine must be called after LlInitRunTimeCfg() but only once before any
 *              other initialization routines.
 */
/*************************************************************************************************/
uint16_t LlInitExtAdvMem(uint8_t *pFreeMem, uint32_t freeMemSize);

/*************************************************************************************************/
/*!
 *  \brief      Initialize memory for extended scanning.
 *
 *  \param      pFreeMem        Pointer to free memory.
 *  \param      freeMemSize     Size of pFreeMem.
 *
 *  \return     Amount of free memory consumed.
 *
 *  This function allocates memory for extended scanning.
 *
 *  \note       This routine must be called after LlInitRunTimeCfg() but only once before any
 *              other initialization routines.
 */
/*************************************************************************************************/
uint16_t LlInitExtScanMem(uint8_t *pFreeMem, uint32_t freeMemSize);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation as an advertising slave.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as an advertising slave.
 */
/*************************************************************************************************/
void LlAdvSlaveInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation for extended advertising slave.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as an extended advertising slave.
 */
/*************************************************************************************************/
void LlExtAdvSlaveInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation as a connectable slave.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as an advertising and connectable slave.
 */
/*************************************************************************************************/
void LlConnSlaveInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation as a encryptable connectable slave.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as an advertising and encryptable
 *  connectable slave.
 */
/*************************************************************************************************/
void LlEncConnSlaveInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation as a scanning master.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as a scanning master.
 */
/*************************************************************************************************/
void LlScanMasterInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation for extended scanning master.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as an extended scanning master.
 */
/*************************************************************************************************/
void LlExtScanMasterInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation as an initiating master.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as an initiating master.
 */
/*************************************************************************************************/
void LlInitMasterInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation as an extended initiating master.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as an initiating master.
 */
/*************************************************************************************************/
void LlExtInitMasterInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation as a connectable master.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as a scanning and initiating master.
 */
/*************************************************************************************************/
void LlConnMasterInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation as a encryptable connectable slave.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use as an advertising and encryptable
 *  connectable slave.
 */
/*************************************************************************************************/
void LlEncConnMasterInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for operation with privacy.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for use with privacy.
 */
/*************************************************************************************************/
void LlPrivInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for secure connections.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for secure connections.
 */
/*************************************************************************************************/
void LlScInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for PHY features (slave).
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for slave PHY features.
 */
/*************************************************************************************************/
void LlPhySlaveInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for PHY features (master).
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for master PHY features.
 */
/*************************************************************************************************/
void LlPhyMasterInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for secure connections.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for secure connections.
 */
/*************************************************************************************************/
void LlChannelSelection2Init(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem for test modes.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem for test modes.
 */
/*************************************************************************************************/
void LlTestInit(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize LL subsystem with task handler.
 *
 *  \param      handlerId  WSF handler ID.
 *
 *  \return     None.
 *
 *  This function initializes the LL subsystem.  It is called once upon system initialization.
 *  It must be called before any other function in the LL API is called.
 */
/*************************************************************************************************/
void LlHandlerInit(wsfHandlerId_t handlerId);

/*************************************************************************************************/
/*!
 *  \brief      LL message dispatch handler.
 *
 *  \param      event       WSF event.
 *  \param      pMsg        WSF message.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LlHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief      Reset LL subsystem.
 *
 *  \return     None.
 *
 * Reset the LL subsystem.  All active connections are closed and all radio procedures such as
 * scanning or advertising are terminated.
 */
/*************************************************************************************************/
void LlReset(void);

/*************************************************************************************************/
/*!
 *  \brief      Register LL event handler.
 *
 *  \param      evtCback        Client callback function.
 *
 *  \return     None.
 *
 *  This function is called by a client to register for LL events.
 */
/*************************************************************************************************/
void LlEvtRegister(llEvtCback_t evtCback);

/*************************************************************************************************/
/*!
 *  \brief      Register ACL handler.
 *
 *  \param      sendCompCback   Client ACL send complete callback function.
 *  \param      recvPendCback   Client ACL receive pending callback function.
 *
 *  \return     None.
 *
 *  This function is called by a client to register for ACL data.
 */
/*************************************************************************************************/
void LlAclRegister(llAclCback_t sendCompCback, llAclCback_t recvPendCback);

/*! \} */    /* LL_API_INIT */

/*! \addtogroup LL_API_DEVICE
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Set Bluetooth device address
 *
 *  \param      pAddr       Bluetooth device address.
 *
 *  \return     None.
 *
 *  Set the BD address to be used by LL.
 */
/*************************************************************************************************/
void LlSetBdAddr(const uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief      Get Bluetooth device address
 *
 *  \param      pAddr       Bluetooth device address.
 *
 *  \return     None.
 *
 *  Get the BD address currently used by LL or all zeros if address is not set.
 */
/*************************************************************************************************/
void LlGetBdAddr(uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief      Set random device address.
 *
 *  \param      pAddr       Random Bluetooth device address.
 *
 *  \return     Status.
 *
 *  Set the random address to be used by LL.
 */
/*************************************************************************************************/
uint8_t LlSetRandAddr(const uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief      Get random device address.
 *
 *  \param      pAddr       Random Bluetooth device address.
 *
 *  \return     Status error code.
 *
 *  Get the random address currently used by LL or all zeros if address is not set.
 */
/*************************************************************************************************/
uint8_t LlGetRandAddr(uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief      Get versions
 *
 *  \param      pCompId     Company ID.
 *  \param      pBtVer      Bluetooth version.
 *  \param      pImplRev    Implementation revision.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LlGetVersion(uint16_t *pCompId, uint8_t *pBtVer, uint16_t *pImplRev);

/*************************************************************************************************/
/*!
 *  \brief      Get supported states.
 *
 *  \param      pStates     Supported states bitmask.
 *
 *  \return     None.
 *
 *  Return the states supported by the LL.
 */
/*************************************************************************************************/
void LlGetSupStates(uint8_t *pStates);

/*************************************************************************************************/
/*!
 *  \brief      Get features.
 *
 *  \param      pFeatures   Supported features bitmask.
 *
 *  \return     None.
 *
 *  Return the LE features supported by the LL.
 */
/*************************************************************************************************/
void LlGetFeatures(uint8_t *pFeatures);

/*************************************************************************************************/
/*!
 *  \brief      Set features.
 *
 *  \param      pFeatures   Supported features bitmask.
 *
 *  \return     Status error code.
 *
 *  \note       This function must only be called when controller is not connected to another
 *              device.
 *
 *  Set the LE features supported by the LL.
 */
/*************************************************************************************************/
uint8_t LlSetFeatures(const uint8_t *pFeatures);

/*************************************************************************************************/
/*!
 *  \brief      Get random number.
 *
 *  \param      pRandNum        Buffer to store 8 bytes random data.
 *
 *  \return     Status error code.
 *
 *  Request the LL to generate a random number.
 */
/*************************************************************************************************/
uint8_t LlGetRandNum(uint8_t *pRandNum);

/*************************************************************************************************/
/*!
 *  \brief      Get white list size.
 *
 *  \return     Total number of white list entries.
 *
 *  Read the white list capacity supported by the LL.
 */
/*************************************************************************************************/
uint8_t LlGetWhitelistSize(void);

/*************************************************************************************************/
/*!
 *  \brief      Clear all white list entries.
 *
 *  \return     Status error code.
 *
 *  Clear all white list entries stored in the LL.
 *
 *  \note       This function must only be called when advertising or scan is disabled
 *              and not initiating.
 */
/*************************************************************************************************/
uint8_t LlClearWhitelist(void);

/*************************************************************************************************/
/*!
 *  \brief      Add device to the white list.
 *
 *  \param      addrType    Address type.
 *  \param      pAddr       Bluetooth device address.
 *
 *  \return     Status error code.
 *
 *  Adds the given address to the white list stored in the LL.
 *
 *  \note       This function must only be called when advertising or scan is disabled
 *              and not initiating.
 */
/*************************************************************************************************/
uint8_t LlAddDeviceToWhitelist(uint8_t addrType, bdAddr_t pAddr);

/*************************************************************************************************/
/*!
 *  \brief      Remove device from the white list.
 *
 *  \param      addrType    Address type.
 *  \param      pAddr       Bluetooth device address.
 *
 *  \return     Status error code.
 *
 *  Removes the given address from the white list stored in the LL.
 *
 *  \note       This function must only be called when advertising or scan is disabled
 *              and not initiating.
 */
/*************************************************************************************************/
uint8_t LlRemoveDeviceFromWhitelist(uint8_t addrType, bdAddr_t pAddr);

/*************************************************************************************************/
/*!
 *  \brief      Add device to resolving list.
 *
 *  \param      peerAddrType        Peer identity address type.
 *  \param      pPeerIdentityAddr   Peer identity address.
 *  \param      pPeerIrk            Peer IRK.
 *  \param      pLocalIrk           Local IRK.
 *
 *  \return     Status error code.
 *
 *  Add device to resolving list.
 */
/*************************************************************************************************/
uint8_t LlAddDeviceToResolvingList(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr, const uint8_t *pPeerIrk, const uint8_t *pLocalIrk);

/*************************************************************************************************/
/*!
 *  \brief      Remove device from resolving list.
 *
 *  \param      peerAddrType        Peer identity address type.
 *  \param      pPeerIdentityAddr   Peer identity address.
 *
 *  \return     Status error code.
 *
 *  Remove device from resolving list.
 */
/*************************************************************************************************/
uint8_t LlRemoveDeviceFromResolvingList(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr);

/*************************************************************************************************/
/*!
 *  \brief      Clear resolving list.
 *
 *  \return     Status error code.
 *
 *  Clear resolving list.
 */
/*************************************************************************************************/
uint8_t LlClearResolvingList(void);

/*************************************************************************************************/
/*!
 *  \brief      Read resolving list size.
 *
 *  \param      pSize             Storage for resolving list size.
 *
 *  \return     Status error code.
 *
 *  Read number of address translation entries that can be stored in the resolving list.
 */
/*************************************************************************************************/
uint8_t LlReadResolvingListSize(uint8_t *pSize);

/*************************************************************************************************/
/*!
 *  \brief      Read peer resolvable address.
 *
 *  \param      addrType        Peer identity address type.
 *  \param      pIdentityAddr   Peer identity address.
 *  \param      pRpa            Storage for peer resolvable private address
 *
 *  \return     Status error code.
 *
 *  Get the peer resolvable private address that is currently being used for the peer identity
 *  address.
 */
/*************************************************************************************************/
uint8_t LlReadPeerResolvableAddr(uint8_t addrType, const uint8_t *pIdentityAddr, uint8_t *pRpa);

/*************************************************************************************************/
/*!
 *  \brief      Read local resolvable address.
 *
 *  \param      addrType        Peer identity address type.
 *  \param      pIdentityAddr   Peer identity address.
 *  \param      pRpa            Storage for peer resolvable private address
 *
 *  \return     Status error code.
 *
 *  Get the local resolvable private address that is currently being used for the peer identity
 *  address.
 */
/*************************************************************************************************/
uint8_t LlReadLocalResolvableAddr(uint8_t addrType, const uint8_t *pIdentityAddr, uint8_t *pRpa);

/*************************************************************************************************/
/*!
 *  \brief      Enable or disable address resolution.
 *
 *  \param      enable      Set to TRUE to enable address resolution or FALSE to disable address
 *                          resolution.
 *
 *  \return     Status error code.
 *
 *  Enable or disable address resolution so that received local or peer resolvable private
 *  addresses are resolved.
 */
/*************************************************************************************************/
uint8_t LlSetAddrResolutionEnable(uint8_t enable);

/*************************************************************************************************/
/*!
 *  \brief      Set resolvable private address timeout.
 *
 *  \param      rpaTimeout    Timeout measured in seconds.
 *
 *  \return     Status error code.
 *
 *  Set the time period between automatic generation of new resolvable private addresses.
 */
/*************************************************************************************************/
uint8_t LlSetResolvablePrivateAddrTimeout(uint16_t rpaTimeout);

/*************************************************************************************************/
/*!
 *  \brief      Set privacy mode.
 *
 *  \param      peerAddrType        Peer identity address type.
 *  \param      pPeerIdentityAddr   Peer identity address.
 *  \param      privMode            Privacy mode.
 *
 *  \return     Status error code.
 *
 *  Allow the host to specify the privacy mode to be used for a given entry on the resolving list.
 */
/*************************************************************************************************/
uint8_t LlSetPrivacyMode(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr, uint8_t privMode);

/*************************************************************************************************/
/*!
 *  \brief      Generate a P-256 public/private key pair.
 *
 *  \return     Status error code.
 *
 *  Generate a P-256 public/private key pair.  If another ECC operation (P-256 key pair or Diffie-
 *  Hellman key generation) is ongoing, an error will be returned.
 */
/*************************************************************************************************/
uint8_t LlGenerateP256KeyPair(void);

/*************************************************************************************************/
/*!
 *  \brief      Generate a Diffie-Hellman key.
 *
 *  \param      pubKey_x  Remote public key x-coordinate.
 *  \param      pubKey_y  Remote public key y-coordinate.
 *
 *  \return     Status error code.
 *
 *  Generate a Diffie-Hellman key from a remote public key and the local private key.  If another
 *  ECC operation (P-256 key pair or Diffie-Hellman key generation) is ongoing, an error will be
 *  returned.
 */
/*************************************************************************************************/
uint8_t LlGenerateDhKey(const uint8_t pubKey_x[LL_ECC_KEY_LEN], const uint8_t pubKey_y[LL_ECC_KEY_LEN]);

/*************************************************************************************************/
/*!
 *  \brief      Set P-256 private key for debug purposes.
 *
 *  \param      privKey   Private key, or all zeros to clear set private key.
 *
 *  \return     Status error code.
 *
 *  Set P-256 private key or clear set private key.  The private key will be used for generate key
 *  pairs and Diffie-Hellman keys until cleared.
 */
/*************************************************************************************************/
uint8_t LlSetP256PrivateKey(const uint8_t privKey[LL_ECC_KEY_LEN]);

/*************************************************************************************************/
/*!
 *  \brief      Set channel class.
 *
 *  \param      pChanMap        Channel map (0=bad, 1=usable).
 *
 *  \return     Status error code.
 *
 *  Set the channel class. At least 2 bits must be set.
 */
/*************************************************************************************************/
uint8_t LlSetChannelClass(const uint8_t *pChanMap);

/*************************************************************************************************/
/*!
 *  \brief      Set operational mode flags.
 *
 *  \param      flags   Flags.
 *  \param      enable  TRUE to set flags or FALSE to clear flags.
 *
 *  \return     Status error code.
 *
 *  Set mode flags governing LL operations.
 */
/*************************************************************************************************/
uint8_t LlSetOpFlags(uint32_t flags, bool_t enable);

/*************************************************************************************************/
/*!
 *  \brief      Set the default Ext adv TX PHY options.
 *
 *  \param      phyOptions  PHY options.
 *
 *  \return     None.
 *
 *  Set the default TX PHY options for extended adv slave primary and secondary channel.
 */
/*************************************************************************************************/
void LlSetDefaultExtAdvTxPhyOptions(const uint8_t phyOptions);

/*! \} */    /* LL_API_DEVICE */

/*! \addtogroup LL_API_BROADCAST
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Set advertising transmit power.
 *
 *  \param      advTxPwr        Advertising transmit power level.
 *
 *  \return     None.
 *
 *  Set the advertising transmit power.
 */
/*************************************************************************************************/
void LlSetAdvTxPower(int8_t advTxPwr);

/*************************************************************************************************/
/*!
 *  \brief      Get advertising transmit power.
 *
 *  \param      pAdvTxPwr       Advertising transmit power level.
 *
 *  \return     Status error code.
 *
 *  Return the advertising transmit power.
 */
/*************************************************************************************************/
uint8_t LlGetAdvTxPower(int8_t *pAdvTxPwr);

/*************************************************************************************************/
/*!
 *  \brief      Set advertising parameter.
 *
 *  \param      advIntervalMin  Minimum advertising interval.
 *  \param      advIntervalMax  Maximum advertising interval.
 *  \param      advType         Advertising type.
 *  \param      ownAddrType     Address type used by this device.
 *  \param      peerAddrType    Address type of peer device.  Only used for directed advertising.
 *  \param      pPeerAddr       Address of peer device.  Only used for directed advertising.
 *  \param      advChanMap      Advertising channel map.
 *  \param      advFiltPolicy   Advertising filter policy.
 *
 *  \return     Status error code.
 *
 *  Set advertising parameters.
 *
 *  \note       This function must only be called when advertising is disabled.
 */
/*************************************************************************************************/
uint8_t LlSetAdvParam(uint16_t advIntervalMin, uint16_t advIntervalMax, uint8_t advType,
                      uint8_t ownAddrType, uint8_t peerAddrType, const uint8_t *pPeerAddr,
                      uint8_t advChanMap, uint8_t advFiltPolicy);

/*************************************************************************************************/
/*!
 *  \brief      Set advertising data.
 *
 *  \param      len     Data buffer length.
 *  \param      pData   Advertising data buffer.
 *
 *  \return     Status error code.
 *
 *  Set advertising data data.
 */
/*************************************************************************************************/
uint8_t LlSetAdvData(uint8_t len, const uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      Set scan response data.
 *
 *  \param      len     Data buffer length.
 *  \param      pData   Scan response data buffer.
 *
 *  \return     Status error code.
 *
 *  Set scan response data.
 */
/*************************************************************************************************/
uint8_t LlSetScanRespData(uint8_t len, const uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      Advertising enable.
 *
 *  \param      enable          Set to TRUE to enable advertising, FALSE to disable advertising.
 *
 *  \return     None.

 *  Enable or disable advertising.
 */
/*************************************************************************************************/
void LlAdvEnable(uint8_t enable);

/*************************************************************************************************/
/*!
 *  \brief      Set advertising set random device address.
 *
 *  \param      handle      Advertising handle.
 *  \param      pAddr       Random Bluetooth device address.
 *
 *  \return     Status error code.
 *
 *  Set the random address to be used by a advertising set.
 */
/*************************************************************************************************/
uint8_t LlSetAdvSetRandAddr(uint8_t handle, const uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief      Get advertising set random device address.
 *
 *  \param      handle      Advertising handle.
 *  \param      pAddr       Random Bluetooth device address.
 *
 *  \return     Status error code.
 *
 *  Get the random address to be used by a advertising set.
 */
/*************************************************************************************************/
uint8_t LlGetAdvSetRandAddr(uint8_t handle, uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief      Set extended advertising parameters.
 *
 *  \param      handle          Advertising handle.
 *  \param      pExtAdvParam    Extended advertising parameters.
 *
 *  \return     Status error code.
 *
 *  Set extended advertising parameters.
 *
 *  \note       This function must only be called when advertising for this set is disabled.
 */
/*************************************************************************************************/
uint8_t LlSetExtAdvParam(uint8_t handle, LlExtAdvParam_t *pExtAdvParam);

/*************************************************************************************************/
/*!
 *  \brief      Get extended advertising TX power level.
 *
 *  \param      handle          Advertising handle.
 *  \param      pLevel          Transmit power level.
 *
 *  \return     Status error code.
 *
 *  Get the TX power of an advertising set.
 */
/*************************************************************************************************/
uint8_t LlGetExtAdvTxPowerLevel(uint16_t handle, int8_t *pLevel);

/*************************************************************************************************/
/*!
 *  \brief      Set extended advertising data.
 *
 *  \param      handle      Advertising handle.
 *  \param      op          Operation.
 *  \param      fragPref    Fragment preference.
 *  \param      len         Data buffer length.
 *  \param      pData       Advertising data buffer.
 *
 *  \return     Status error code.
 *
 *  Set extended advertising data data.
 */
/*************************************************************************************************/
uint8_t LlSetExtAdvData(uint8_t handle, uint8_t op, uint8_t fragPref, uint8_t len, const uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      Set extended scan response data.
 *
 *  \param      handle      Advertising handle.
 *  \param      op          Operation.
 *  \param      fragPref    Fragment preference.
 *  \param      len         Data buffer length.
 *  \param      pData       Scan response data buffer.
 *
 *  \return     Status error code.
 *
 *  Set extended scan response data.
 */
/*************************************************************************************************/
uint8_t LlSetExtScanRespData(uint8_t handle, uint8_t op, uint8_t fragPref, uint8_t len, const uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      Extended advertising enable.
 *
 *  \param      enable      Set to TRUE to enable advertising, FALSE to disable advertising.
 *  \param      numAdvSets  Number of elements in enaParam[].
 *  \param      enaParam    Enable parameter table.
 *
 *  \return     None.
 *
 *  Enable or disable extended advertising.
 */
/*************************************************************************************************/
void LlExtAdvEnable(uint8_t enable, uint8_t numAdvSets, LlExtAdvEnableParam_t enaParam[]);

/*************************************************************************************************/
/*!
 *  \brief      Read maximum advertising data length.
 *
 *  \param      pLen        Return buffer for Maximum data length.
 *
 *  \return     Status error code.
 *
 *  Read maximum advertising data length.
 */
/*************************************************************************************************/
uint8_t LlReadMaxAdvDataLen(uint16_t *pLen);

/*************************************************************************************************/
/*!
 *  \brief      Read number of supported advertising sets.
 *
 *  \param      pNumSets    Return buffer for number of advertising sets.
 *
 *  \return     Status error code.
 *
 *  Read number of supported advertising sets.
 */
/*************************************************************************************************/
uint8_t LlReadNumSupAdvSets(uint8_t *pNumSets);

/*************************************************************************************************/
/*!
 *  \brief      Remove advertising set.
 *
 *  \param      handle      Advertising handle.
 *
 *  \return     Status error code.
 *
 *  Removes the an advertising set from the LL.
 */
/*************************************************************************************************/
uint8_t LlRemoveAdvSet(uint8_t handle);

/*************************************************************************************************/
/*!
 *  \brief      Clear advertising sets.
 *
 *  \return     Status error code.
 *
 *  Remove all existing advertising sets from the LL.
 */
/*************************************************************************************************/
uint8_t LlClearAdvSets(void);

/*************************************************************************************************/
/*!
 *  \brief      Set periodic advertising parameters.
 *
 *  \param      handle          Advertising handle.
 *  \param      pPerAdvParam    Periodic advertising parameters.
 *
 *  \return     Status error code.
 *
 *  Set periodic advertising parameters.
 *
 *  \note       This function must only be called when advertising handle exists.
 */
/*************************************************************************************************/
uint8_t LlSetPeriodicAdvParam(uint8_t handle, LlPerAdvParam_t *pPerAdvParam);

/*************************************************************************************************/
/*!
 *  \brief      Set periodic advertising data.
 *
 *  \param      handle      Advertising handle.
 *  \param      op          Operation.
 *  \param      len         Data buffer length.
 *  \param      pData       Advertising data buffer.
 *
 *  \return     Status error code.
 *
 *  Set periodic advertising data.
 */
/*************************************************************************************************/
uint8_t LlSetPeriodicAdvData(uint8_t handle, uint8_t op, uint8_t len, const uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      Set periodic advertising enable.
 *
 *  \param      enable      TRUE to enable advertising, FALSE to disable advertising.
 *  \param      handle      Advertising handle.
 *
 *  \return     Status error code.
 *
 *  Enable or disable periodic advertising.
 */
/*************************************************************************************************/
void LlSetPeriodicAdvEnable(uint8_t enable, uint8_t handle);

/*************************************************************************************************/
/*!
 *  \brief      Set auxiliary packet offset delay.
 *
 *  \param      handle      Advertising handle.
 *  \param      delayUsec   Additional time in microseconds. "0" to disable.
 *
 *  \return     Status error code.
 *
 *  Additional delay given to auxiliary packets specified by AuxPtr. Offset values are
 *  limited by the advertising interval.
 */
/*************************************************************************************************/
uint8_t LlSetAuxOffsetDelay(uint8_t handle, uint32_t delayUsec);

/*************************************************************************************************/
/*!
 *  \brief      Set extended advertising data fragmentation length.
 *
 *  \param      handle      Advertising handle.
 *  \param      fragLen     Fragmentation length.
 *
 *  \return     Status error code.
 *
 *  Fragmentation size for Advertising Data and Scan Response Data when selected by the host.
 */
/*************************************************************************************************/
uint8_t LlSetExtAdvDataFragLen(uint8_t handle, uint8_t fragLen);

/*************************************************************************************************/
/*!
 *  \brief      Set extended advertising transmit PHY options.
 *
 *  \param      handle      Advertising handle.
 *  \param      priPhyOpts  Primary advertising channel PHY options.
 *  \param      secPhyOpts  Secondary advertising channel PHY options.
 *
 *  \return     Status error code.
 *
 *  PHY options for extended advertising transmissions. New values are applied dynamically.
 */
/*************************************************************************************************/
uint8_t LlSetExtAdvTxPhyOptions(uint8_t handle, uint8_t priPhyOpts, uint8_t secPhyOpts);

/*************************************************************************************************/
/*!
 *  \brief      Read supported transmit power.
 *
 *  \param      pMinTxPwr   Return buffer for minimum transmit power.
 *  \param      pMaxTxPwr   Return buffer for maximum transmit power.
 *
 *  \return     None.
 *
 *  Read the minimum and maximum transmit powers supported by the LL.
 */
/*************************************************************************************************/
void LlReadSupTxPower(int8_t *pMinTxPwr, int8_t *pMaxTxPwr);

/*************************************************************************************************/
/*!
 *  \brief      Read RF path compensation.
 *
 *  \param      pTxPathComp     Return buffer for RF transmit path compensation value.
 *  \param      pRxPathComp     Return buffer for RF receive path compensation value.
 *
 *  \return     None.
 *
 *  Read the RF Path Compensation Values parameter used in the Tx Power Level and RSSI calculation.
 */
/*************************************************************************************************/
void LlReadRfPathComp(int16_t *pTxPathComp, int16_t *pRxPathComp);

/*************************************************************************************************/
/*!
 *  \brief      Write RF path compensation.
 *
 *  \param      txPathComp      RF transmit path compensation value.
 *  \param      rxPathComp      RF receive path compensation value.
 *
 *  \return     Status error code.
 *
 *  Indicate the RF path gain or loss between the RF transceiver and the antenna contributed by
 *  intermediate components.
 */
/*************************************************************************************************/
uint8_t LlWriteRfPathComp(int16_t txPathComp, int16_t rxPathComp);

/*************************************************************************************************/
/*!
 *  \brief      Scan report enable.
 *
 *  \param      enable          Set to TRUE to enable scan reports, FALSE to disable scan reports.
 *
 *  \return     None.
 *
 *  Enable or disable reports about the scanners from which an advertiser receives scan requests.
 */
/*************************************************************************************************/
void LlScanReportEnable(uint8_t enable);

/*! \} */    /* LL_API_BROADCAST */

/*! \addtogroup LL_API_OBSERVER
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Set scan channel map.
 *
 *  \param      chanMap         Scan channel map.
 *
 *  \return     Status error code.
 *
 *  Set scan channel map.
 *
 *  \note       This function must only be called when scanning is disabled.
 */
/*************************************************************************************************/
uint8_t LlSetSetScanChanMap(uint8_t chanMap);

/*************************************************************************************************/
/*!
 *  \brief      Set scan parameters.
 *
 *  \param      pParam          Scan parameters.
 *
 *  \return     Status error code.
 *
 *  Set scan parameters.
 *
 *  \note       This function must only be called when scanning is disabled.
 */
/*************************************************************************************************/
uint8_t LlSetScanParam(const LlScanParam_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief      Scan enable.
 *
 *  \param      enable          Set to TRUE to enable scanning, FALSE to disable scanning.
 *  \param      filterDup       Set to TRUE to filter duplicates.
 *
 *  \return     None.
 *
 *  Enable or disable scanning.  This function is only used when operating in master role.
 */
/*************************************************************************************************/
void LlScanEnable(uint8_t enable, uint8_t filterDup);

/*************************************************************************************************/
/*!
 *  \brief      Set extended scanning parameters.
 *
 *  \param      ownAddrType     Address type used by this device.
 *  \param      scanFiltPolicy  Scan filter policy.
 *  \param      scanPhys        Scanning PHYs bitmask.
 *  \param      param           Scanning parameter table indexed by PHY.
 *
 *  \return     Status error code.
 *
 *  Set the extended scan parameters to be used on the primary advertising channel.
 */
/*************************************************************************************************/
uint8_t LlSetExtScanParam(uint8_t ownAddrType, uint8_t scanFiltPolicy, uint8_t scanPhys, const LlExtScanParam_t param[]);

/*************************************************************************************************/
/*!
 *  \brief      Extended scan enable.
 *
 *  \param      enable          Set to TRUE to enable scanning, FALSE to disable scanning.
 *  \param      filterDup       Set to TRUE to filter duplicates.
 *  \param      duration        Duration.
 *  \param      period          Period.
 *
 *  \return     None.
 *
 *  Enable or disable extended scanning.
 */
/*************************************************************************************************/
void LlExtScanEnable(uint8_t enable, uint8_t filterDup, uint16_t duration, uint16_t period);

/*************************************************************************************************/
/*!
 *  \brief      Create synchronization of periodic advertising.
 *
 *  \param      pParam          Create sync parameters.
 *
 *  \return     Status error code.
 *
 *  Create synchronization of periodic advertising.
 */
/*************************************************************************************************/
uint8_t LlPeriodicAdvCreateSync(const LlPerAdvCreateSyncCmd_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief      Cancel pending synchronization of periodic advertising.
 *
 *  \return     Status error code.
 *
 *  Cancel pending synchronization of periodic advertising.
 */
/*************************************************************************************************/
uint8_t LlPeriodicAdvCreateSyncCancel(void);

/*************************************************************************************************/
/*!
 *  \brief      Stop synchronization of periodic advertising.
 *
 *  \param      syncHandle      Sync handle.
 *
 *  \return     Status error code.
 *
 *  Stop synchronization of periodic advertising.
 */
/*************************************************************************************************/
uint8_t LlPeriodicAdvTerminateSync(uint16_t syncHandle);

/*************************************************************************************************/
/*!
 *  \brief      Add device to periodic advertiser list.
 *
 *  \param      pParam          Add to periodic list parameters.
 *
 *  \return     Status error code.
 *
 *  Add device to periodic advertiser list.
 */
/*************************************************************************************************/
uint8_t LlAddDeviceToPeriodicAdvList(const LlDevicePerAdvList_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief      Remove device from periodic advertiser list command.
 *
 *  \param      pParam          Remove from periodic list parameters.
 *
 *  \return     Status error code.
 *
 *
 */
/*************************************************************************************************/
uint8_t LlRemoveDeviceFromPeriodicAdvList(const LlDevicePerAdvList_t *pParam);

/*************************************************************************************************/
/*!
 *  \brief      Clear all devices in periodic advertiser list command.
 *
 *  \return     Status error code.
 *
 *  Clear all devices in periodic advertiser list command.
 */
/*************************************************************************************************/
uint8_t LlClearPeriodicAdvList(void);

/*************************************************************************************************/
/*!
 *  \brief      Read total number of devices in periodic advertiser list command.
 *
 *  \param      pListSize       Return size value of periodic advertiser list
 *
 *  \return     Status error code.
 *
 *  Read total number of devices in periodic advertiser list command.
 */
/*************************************************************************************************/
uint8_t LlReadPeriodicAdvListSize(uint8_t *pListSize);

/*! \} */    /* LL_API_OBSERVER */

/*! \addtogroup LL_API_CONN
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Disconnect a connection.
 *
 *  \param      handle          Connection handle.
 *  \param      reason          Disconnect reason.
 *
 *  \return     Status error code.
 *
 *  Disconnect a connection.
 */
/*************************************************************************************************/
uint8_t LlDisconnect(uint16_t handle, uint8_t reason);

/*************************************************************************************************/
/*!
 *  \brief      Set connection's operational mode flags.
 *
 *  \param      handle  Connection handle.
 *  \param      flags   Flags.
 *  \param      enable  TRUE to set flags or FALSE to clear flags.
 *
 *  \return     Status error code.
 *
 *  Set connection's operational mode flags governing LL operations.
 */
/*************************************************************************************************/
uint8_t LlSetConnOpFlags(uint16_t handle, uint32_t flags, bool_t enable);

/*************************************************************************************************/
/*!
 *  \brief      Read remote features.
 *
 *  \param      handle          Connection handle.
 *
 *  \return     Status error code.
 *
 *  Read the link layer features of the remote device.
 */
/*************************************************************************************************/
uint8_t LlReadRemoteFeat(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief      Read remote version information.
 *
 *  \param      handle          Connection handle.
 *
 *  \return     Status error code.
 *
 *  Read the version information of the remote device.
 */
/*************************************************************************************************/
uint8_t LlReadRemoteVerInfo(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief      Get RSSI of a connection.
 *
 *  \param      handle          Connection handle.
 *  \param      pRssi           RSSI value.
 *
 *  \return     Status error code.
 *
 *  Get the current RSSI of a connection.
 */
/*************************************************************************************************/
uint8_t LlGetRssi(uint16_t handle, int8_t *pRssi);

/*************************************************************************************************/
/*!
 *  \brief      Get connection's TX power level.
 *
 *  \param      handle          Connection handle.
 *  \param      type            Power level type.
 *  \param      pLevel          Transmit power level.
 *
 *  \return     Status error code.
 *
 *  Get the TX power of a connection.
 */
/*************************************************************************************************/
uint8_t LlGetTxPowerLevel(uint16_t handle, uint8_t type, int8_t *pLevel);

/*************************************************************************************************/
/*!
 *  \brief      Set connection's TX power level.
 *
 *  \param      handle          Connection handle.
 *  \param      level           Transmit power level.
 *
 *  \return     Status error code.
 *
 *  Set the TX power of a connection.
 */
/*************************************************************************************************/
uint8_t LlSetTxPowerLevel(uint16_t handle, int8_t level);

/*************************************************************************************************/
/*!
 *  \brief      Update connection parameters.
 *
 *  \param      handle          Connection handle.
 *  \param      pConnSpec       New connection specification.
 *
 *  \return     Status error code.
 *
 *  Update the connection parameters of a connection.
 */
/*************************************************************************************************/
uint8_t LlConnUpdate(uint16_t handle, const LlConnSpec_t *pConnSpec);

/*************************************************************************************************/
/*!
 *  \brief      Remote connection parameter request reply.
 *
 *  \param      handle          Connection handle.
 *  \param      pConnSpec       New connection specification.
 *
 *  \return     Status error code.
 *
 *  Reply to a connection parameter request.
 */
/*************************************************************************************************/
uint8_t LlRemoteConnParamReqReply(uint16_t handle, const LlConnSpec_t *pConnSpec);

/*************************************************************************************************/
/*!
 *  \brief      Remote connection parameter request negative reply.
 *
 *  \param      handle          Connection handle.
 *  \param      reason          Reason code.
 *
 *  \return     None.
 *
 *  Negative reply to a connection parameter request.
 */
/*************************************************************************************************/
uint8_t LlRemoteConnParamReqNegReply(uint16_t handle, uint8_t reason);

/*************************************************************************************************/
/*!
 *  \brief      Set connection's channel map.
 *
 *  \param      handle          Connection handle.
 *  \param      pChanMap        Channel map.
 *
 *  \return     Status error code.
 *
 *  Set the channel map of a connection.
 */
/*************************************************************************************************/
uint8_t LlSetChannelMap(uint16_t handle, const uint8_t *pChanMap);

/*************************************************************************************************/
/*!
 *  \brief      Get connection's channel map.
 *
 *  \param      handle          Connection handle.
 *  \param      pChanMap        Channel map.
 *
 *  \return     Status error code.
 *
 *  Get the current channel map of a connection.
 */
/*************************************************************************************************/
uint8_t LlGetChannelMap(uint16_t handle, uint8_t *pChanMap);

/*************************************************************************************************/
/*!
 *  \brief      Set data length.
 *
 *  \param      handle          Connection handle.
 *  \param      txLen           Maximum number of payload bytes for a Data PDU
 *  \param      txTime          Maximum microseconds for a Data PDU
 *
 *  \return     Status error code.
 *
 *  Preferred maximum microseconds that the local Controller should use to transmit a
 *  single Link Layer Data Channel PDU.
 */
/*************************************************************************************************/
uint8_t LlSetDataLen(uint16_t handle, uint16_t txLen, uint16_t txTime);

/*************************************************************************************************/
/*!
 *  \brief      Read default data length.
 *
 *  \param      pMaxTxLen       Maximum number of payload bytes for a Data PDU
 *  \param      pMaxTxTime      Maximum microseconds for a Data PDU
 *
 *  \return     None.
 *
 *  Suggested length and microseconds that the local Controller should use to transmit a
 *  single Link Layer Data Channel PDU.
 */
/*************************************************************************************************/
void LlReadDefaultDataLen(uint16_t *pMaxTxLen, uint16_t *pMaxTxTime);

/*************************************************************************************************/
/*!
 *  \brief      Write default data length.
 *
 *  \param      maxTxLen        Maximum number of payload bytes for a Data PDU
 *  \param      maxTxTime       Maximum microseconds for a Data PDU
 *
 *  \return     Status error code.
 *
 *  Suggested length and microseconds that the local Controller should use to transmit a
 *  single Link Layer Data Channel PDU.
 */
/*************************************************************************************************/
uint8_t LlWriteDefaultDataLen(uint16_t maxTxLen, uint16_t maxTxTime);

/*************************************************************************************************/
/*!
 *  \brief      Read maximum data length.
 *
 *  \param      pMaxTxLen       Maximum number of payload bytes for a Tx Data PDU
 *  \param      pMaxTxTime      Maximum microseconds for a Tx Data PDU
 *  \param      pMaxRxLen       Maximum number of payload bytes for a Rx Data PDU
 *  \param      pMaxRxTime      Maximum microseconds for a Rx Data PDU
 *
 *  \return     None.
 *
 *  Read the Controller's maximum supported payload octets and packet duration times for
 *  transmission and reception.
 */
/*************************************************************************************************/
void LlReadMaximumDataLen(uint16_t *pMaxTxLen, uint16_t *pMaxTxTime, uint16_t *pMaxRxLen, uint16_t *pMaxRxTime);

/*************************************************************************************************/
/*!
 *  \brief      Read current transmitter PHY and receive PHY.
 *
 *  \param      handle            Connection handle.
 *  \param      pTxPhy            Storage for transmitter PHY.
 *  \param      pRxPhy            Storage for receiver PHY.
 *
 *  \return     Status error code.
 *
 *  Read current transmitter PHY and receive PHY.
 */
/*************************************************************************************************/
uint8_t LlReadPhy(uint16_t handle, uint8_t *pTxPhy, uint8_t *pRxPhy);

/*************************************************************************************************/
/*!
 *  \brief      Set default PHYs.
 *
 *  \param      allPhys           All PHYs preferences.
 *  \param      txPhys            Preferred transmitter PHYs.
 *  \param      rxPhys            Preferred receiver PHYs.
 *
 *  \return     Status error code.
 *
 *  Specify the preferred values for the transmitter PHY and receiver PHY to be used for all
 *  subsequent connections over the LE transport.
 */
/*************************************************************************************************/
uint8_t LlSetDefaultPhy(uint8_t allPhys, uint8_t txPhys, uint8_t rxPhys);

/*************************************************************************************************/
/*!
 *  \brief      Set PHY for a connection.
 *
 *  \param      handle            Connection handle.
 *  \param      allPhys           All PHYs preferences.
 *  \param      txPhys            Preferred transmitter PHYs.
 *  \param      rxPhys            Preferred receiver PHYs.
 *  \param      phyOptions        PHY options.
 *
 *  \return     Status error code.
 *
 *  Set PHY preferences for a given connection.  The controller might not be able to make the
 *  change (e.g., because the peer does not support the requested PHY) or may decide that the
 *  current PHY is preferable.
 */
/*************************************************************************************************/
uint8_t LlSetPhy(uint16_t handle, uint8_t allPhys, uint8_t txPhys, uint8_t rxPhys, uint16_t phyOptions);

/*************************************************************************************************/
/*!
 *  \brief      Set local minimum number of used channels.
 *
 *  \param      phys            Bitmask for the PHYs.
 *  \param      pwrThres        Power threshold.
 *  \param      minUsedCh       Minimum number of used channels.
 *
 *  \return     Status error code.
 *
 *  Set local minimum number of used channels.
 */
/*************************************************************************************************/
uint8_t LlSetLocalMinUsedChan(uint8_t phys, int8_t pwrThres, uint8_t minUsedCh);

/*************************************************************************************************/
/*!
 *  \brief      Get peer minimum number of used channels.
 *
 *  \param      handle              Connection handle.
 *  \param      pPeerMinUsedChan    Storage for the peer minimum number of used channels.
 *
 *  \return     Status error code.
 *
 *  Get peer minimum number of used channels.
 */
/*************************************************************************************************/
uint8_t LlGetPeerMinUsedChan(uint16_t handle, uint8_t *pPeerMinUsedChan);

/*! \} */    /* LL_API_CONN */

/*! \addtogroup LL_API_CENTRAL
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Create connection.
 *
 *  \param      pInitParam      Initiating parameters.
 *  \param      pConnSpec       Connection specification.
 *
 *  \return     Status error code.
 *
 *  Create a connection to the specified peer address with the specified connection parameters.
 *  This function is only when operating in master role.
 */
/*************************************************************************************************/
uint8_t LlCreateConn(const LlInitParam_t *pInitParam, const LlConnSpec_t *pConnSpec);

/*************************************************************************************************/
/*!
 *  \brief      Extended create connection.
 *
 *  \param      pInitParam      Initiating parameters.
 *  \param      scanParam       Scan parameters table indexed by PHY.
 *  \param      connSpec        Connection specification table indexed by PHY.
 *
 *  \return     Status error code.
 *
 *  Create a connection to the specified peer address with the specified connection parameters.
 *  This function is only when operating in master role.
 */
/*************************************************************************************************/
uint8_t LlExtCreateConn(const LlExtInitParam_t *pInitParam, const LlExtInitScanParam_t scanParam[], const LlConnSpec_t connSpec[]);

/*************************************************************************************************/
/*!
 *  \brief      Cancel a create connection operation.
 *
 *  \return     None.
 *
 *  Cancel a connection before it is established.  This function is only used when operating
 *  in master role.
 */
/*************************************************************************************************/
void LlCreateConnCancel(void);

/*! \} */    /* LL_API_CENTRAL */

/*! \addtogroup LL_API_ENCRYPT
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Encrypt data.
 *
 *  \param      pKey            Encryption key.
 *  \param      pData           16 bytes of plain text data.
 *
 *  \return     Status error code.
 *
 *  Request the LL to encryption a block of data in place.
 */
/*************************************************************************************************/
uint8_t LlEncrypt(uint8_t *pKey, uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      Start encryption.
 *
 *  \param      handle          Connection handle.
 *  \param      pRand           Pointer to the random number.
 *  \param      diversifier     Diversifier value.
 *  \param      pKey            Pointer to the encryption key.
 *
 *  \return     Status error code.
 *
 *  Start or restart link layer encryption on a connection.  This function is only used when
 *  operating in master role.
 */
/*************************************************************************************************/
uint8_t LlStartEncryption(uint16_t handle, const uint8_t *pRand, uint16_t diversifier, const uint8_t *pKey);

/*************************************************************************************************/
/*!
 *  \brief      Reply to a LTK request.
 *
 *  \param      handle          Connection handle.
 *  \param      pKey            Pointer to new key.
 *
 *  \return     Status error code.
 *
 *  Provide the requested LTK encryption key.  This function is only used when operating in
 *  slave mode.
 */
/*************************************************************************************************/
uint8_t LlLtkReqReply(uint16_t handle, const uint8_t *pKey);

/*************************************************************************************************/
/*!
 *  \brief      Negative reply to a LTK request.
 *
 *  \param      handle          Connection handle.
 *
 *  \return     Status error code.
 *
 *  Requested LTK encryption key not available.  This function is only used when operating in
 *  slave mode.
 */
/*************************************************************************************************/
uint8_t LlLtkReqNegReply(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief      Read authenticated payload timeout value.
 *
 *  \param      handle          Connection handle.
 *  \param      pTimeout        Pointer to timeout value.
 *
 *  \return     Status error code.
 *
 *  Read authenticated payload timeout value for the given handle.
 */
/*************************************************************************************************/
uint8_t LlReadAuthPayloadTimeout(uint16_t handle, uint16_t *pTimeout);

/*************************************************************************************************/
/*!
 *  \brief      Write authenticated payload timeout value.
 *
 *  \param      handle          Connection handle.
 *  \param      timeout         New timeout value.
 *
 *  \return     Status error code.
 *
 *  Write new authenticated payload timeout value for the given handle.
 */
/*************************************************************************************************/
uint8_t LlWriteAuthPayloadTimeout(uint16_t handle, uint16_t timeout);

/*************************************************************************************************/
/*!
 *  \brief      Get encryption mode used in a connection.
 *
 *  \param      handle          Connection handle.
 *  \param      pMode           Storage for encryption mode.
 *
 *  \return     Status error code.
 *
 *  Get the encryption mode used by a connection.
 */
/*************************************************************************************************/
uint8_t LlGetEncMode(uint16_t handle, LlEncMode_t *pMode);

/*************************************************************************************************/
/*!
 *  \brief      Set encryption mode used in a connection.
 *
 *  \param      handle          Connection handle.
 *  \param      pMode           New encryption mode.
 *
 *  \return     Status error code.
 *
 *  Set the encryption mode used by a connection. Must be called before encryption is started or
 *  when encryption is paused.
 */
/*************************************************************************************************/
uint8_t LlSetEncMode(uint16_t handle, const LlEncMode_t *pMode);

/*! \} */    /* LL_API_ENCRYPT */

/*! \addtogroup LL_API_DATA
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Get the maximum ACL buffers size.
 *
 *  \return     Maximum buffers size in bytes.
 */
/*************************************************************************************************/
uint16_t LlGetAclMaxSize(void);

/*************************************************************************************************/
/*!
 *  \brief      Get the number of buffers in the LL ACL transmit queue.
 *
 *  \return     Number of buffers.
 */
/*************************************************************************************************/
uint8_t LlGetAclTxBufs(void);

/*************************************************************************************************/
/*!
 *  \brief      Get the number of buffers in the LL ACL receive queue.
 *
 *  \return     Number of buffers.
 */
/*************************************************************************************************/
uint8_t LlGetAclRxBufs(void);

/*************************************************************************************************/
/*!
 *  \brief      Send an ACL data packet.
 *
 *  \param      pData   Data buffer
 *
 *  \return     None.
 *
 *  Send an ACL data packet. pData points to an ACL buffer formatted according to [1]; the host
 *  must set the connection handle, flags, and length fields in the buffer.
 */
/*************************************************************************************************/
void LlSendAclData(uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      Receive an ACL data packet
 *
 *  \return     Data buffer.
 *
 *  Receive an ACL data packet.  This function returns a pointer to an ACL buffer formatted
 *  according to [1].  The host must parse the header to determine the connection handle, flags,
 *  and length fields.  If no ACL buffers are available this function returns NULL.
 *
 *  The host must deallocate the buffer by calling WsfMsgFree() and call LlRecvBufCmpl() to
 *  update LL accounting.
 */
/*************************************************************************************************/
uint8_t *LlRecvAclData(void);

/*************************************************************************************************/
/*!
 *  \brief      Indicate that received ACL data buffer has been deallocated
 *
 *  \param      numBufs     Number of completed packets.
 *
 *  \return     None.
 *
 *  Indicate that received ACL data buffer has been deallocated.
 */
/*************************************************************************************************/
void LlRecvAclDataComplete(uint8_t numBufs);

/*! \} */    /* LL_API_DATA */

/*! \addtogroup LL_API_TEST
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Enter transmit test mode.
 *
 *  \param      rfChan      RF channel number, i.e. "rfChan = (F - 2402) / 2)".
 *  \param      len         Length of test data.
 *  \param      pktType     Test packet payload type.
 *  \param      numPkt      Auto terminate after number of packets, 0 for infinite.
 *
 *  \return     Status error code.
 *
 *  Start the transmit test mode on the given channel.
 */
/*************************************************************************************************/
uint8_t LlTxTest(uint8_t rfChan, uint8_t len, uint8_t pktType, uint16_t numPkt);

/*************************************************************************************************/
/*!
 *  \brief      Enter receive test mode.
 *
 *  \param      rfChan      RF channel number, i.e. "rfChan = (F - 2402) / 2)".
 *  \param      numPkt      Auto terminate after number of successful packets, 0 for infinite.
 *
 *  \return     Status error code.
 *
 *  Start the receive test mode on the given channel.
 */
/*************************************************************************************************/
uint8_t LlRxTest(uint8_t rfChan, uint16_t numPkt);

/*************************************************************************************************/
/*!
 *  \brief      Enter enhanced transmit test mode.
 *
 *  \param      rfChan      RF channel number, i.e. "rfChan = (F - 2402) / 2)".
 *  \param      len         Length of test data.
 *  \param      pktType     Test packet payload type.
 *  \param      phy         Transmitter PHY.
 *  \param      numPkt      Auto terminate after number of packets, 0 for infinite.
 *
 *  \return     Status error code.
 *
 *  Start the transmit test mode on the given channel.
 */
/*************************************************************************************************/
uint8_t LlEnhancedTxTest(uint8_t rfChan, uint8_t len, uint8_t pktType, uint8_t phy, uint16_t numPkt);

/*************************************************************************************************/
/*!
 *  \brief      Enter enhanced receive test mode.
 *
 *  \param      rfChan      RF channel number, i.e. "rfChan = (F - 2402) / 2)".
 *  \param      phy         Receiver PHY.
 *  \param      modIdx      Modulation index.
 *  \param      numPkt      Auto terminate after number of successful packets, 0 for infinite.
 *
 *  \return     Status error code.
 *
 *  Start the receive test mode on the given channel.
 */
/*************************************************************************************************/
uint8_t LlEnhancedRxTest(uint8_t rfChan, uint8_t phy, uint8_t modIdx, uint16_t numPkt);

/*************************************************************************************************/
/*!
 *  \brief      End test mode.
 *
 *  \param      pRpt        Report return buffer.
 *
 *  \return     Status error code.
 *
 *  End test mode and return the report.
 */
/*************************************************************************************************/
uint8_t LlEndTest(LlTestReport_t *pRpt);

/*************************************************************************************************/
/*!
 *  \brief      Set pattern of errors for Tx test mode.
 *
 *  \param      pattern   Error pattern (1s = no error; 0s = CRC failure).
 *
 *  \return     Status error code.
 *
 *  Set pattern of errors for Tx test mode.
 *
 *  \note       The error pattern must be set after the Tx test is started.
 */
/*************************************************************************************************/
uint8_t LlSetTxTestErrorPattern(uint32_t pattern);

/*! \} */    /* LL_API_TEST */

/*! \addtogroup LL_API_DIAG
 *  \{ */

/*************************************************************************************************/
/*!
 *  \brief      Get advertising set context size.
 *
 *  \param      pMaxAdvSets     Buffer to return the maximum number of advertising sets.
 *  \param      pAdvSetCtxSize  Buffer to return the size in bytes of the advertising set context.
 *
 *  Return the advertising set context sizes.
 */
/*************************************************************************************************/
void LlGetAdvSetContextSize(uint8_t *pMaxAdvSets, uint16_t *pAdvSetCtxSize);

/*************************************************************************************************/
/*!
 *  \brief      Get connection context size.
 *
 *  \param      pMaxConn        Buffer to return the maximum number of connections.
 *  \param      pConnCtxSize    Buffer to return the size in bytes of the connection context.
 *
 *  Return the connection context sizes.
 */
/*************************************************************************************************/
void LlGetConnContextSize(uint8_t *pMaxConn, uint16_t *pConnCtxSize);

/*************************************************************************************************/
/*!
 *  \brief      Get the LL handler watermark level.
 *
 *  \return     Watermark level in microseconds.
 */
/*************************************************************************************************/
uint16_t LlStatsGetHandlerWatermarkUsec(void);

/*! \} */    /* LL_API_DIAG */


#ifdef __cplusplus
};
#endif

#endif /* LL_API_H */
