/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI subsystem API.
 *
 *  Copyright (c) 2009-2018 Arm Ltd. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/
#ifndef HCI_API_H
#define HCI_API_H

#include "wsf_types.h"
#include "hci_defs.h"
#include "wsf_os.h"
#include "util/bda.h"

#ifdef __cplusplus
extern "C" {
#endif



/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \addtogroup STACK_HCI_EVT_API
 *  \{ */

/** \name HCI Internal Event Codes
 * Proprietary HCI event codes for handling HCI events in callbacks.
 */
/**@{*/
#define HCI_RESET_SEQ_CMPL_CBACK_EVT                     0   /*!< \brief Reset sequence complete */
#define HCI_LE_CONN_CMPL_CBACK_EVT                       1   /*!< \brief LE connection complete */
#define HCI_LE_ENHANCED_CONN_CMPL_CBACK_EVT              2   /*!< \brief LE enhanced connection complete */
#define HCI_DISCONNECT_CMPL_CBACK_EVT                    3   /*!< \brief LE disconnect complete */
#define HCI_LE_CONN_UPDATE_CMPL_CBACK_EVT                4   /*!< \brief LE connection update complete */
#define HCI_LE_CREATE_CONN_CANCEL_CMD_CMPL_CBACK_EVT     5   /*!< \brief LE create connection cancel command complete */
#define HCI_LE_ADV_REPORT_CBACK_EVT                      6   /*!< \brief LE advertising report */
#define HCI_READ_RSSI_CMD_CMPL_CBACK_EVT                 7   /*!< \brief Read RSSI command complete */
#define HCI_LE_READ_CHAN_MAP_CMD_CMPL_CBACK_EVT          8   /*!< \brief LE Read channel map command complete */
#define HCI_READ_TX_PWR_LVL_CMD_CMPL_CBACK_EVT           9   /*!< \brief Read transmit power level command complete */
#define HCI_READ_REMOTE_VER_INFO_CMPL_CBACK_EVT          10  /*!< \brief Read remote version information complete */
#define HCI_LE_READ_REMOTE_FEAT_CMPL_CBACK_EVT           11  /*!< \brief LE read remote features complete */
#define HCI_LE_LTK_REQ_REPL_CMD_CMPL_CBACK_EVT           12  /*!< \brief LE LTK request reply command complete */
#define HCI_LE_LTK_REQ_NEG_REPL_CMD_CMPL_CBACK_EVT       13  /*!< \brief LE LTK request negative reply command complete */
#define HCI_ENC_KEY_REFRESH_CMPL_CBACK_EVT               14  /*!< \brief Encryption key refresh complete */
#define HCI_ENC_CHANGE_CBACK_EVT                         15  /*!< \brief Encryption change */
#define HCI_LE_LTK_REQ_CBACK_EVT                         16  /*!< \brief LE LTK request */
#define HCI_VENDOR_SPEC_CMD_STATUS_CBACK_EVT             17  /*!< \brief Vendor specific command status */
#define HCI_VENDOR_SPEC_CMD_CMPL_CBACK_EVT               18  /*!< \brief Vendor specific command complete */
#define HCI_VENDOR_SPEC_CBACK_EVT                        19  /*!< \brief Vendor specific */
#define HCI_HW_ERROR_CBACK_EVT                           20  /*!< \brief Hardware error */
#define HCI_LE_ADD_DEV_TO_RES_LIST_CMD_CMPL_CBACK_EVT    21  /*!< \brief LE add device to resolving list command complete */
#define HCI_LE_REM_DEV_FROM_RES_LIST_CMD_CMPL_CBACK_EVT  22  /*!< \brief LE remove device from resolving command complete */
#define HCI_LE_CLEAR_RES_LIST_CMD_CMPL_CBACK_EVT         23  /*!< \brief LE clear resolving list command complete */
#define HCI_LE_READ_PEER_RES_ADDR_CMD_CMPL_CBACK_EVT     24  /*!< \brief LE read peer resolving address command complete */
#define HCI_LE_READ_LOCAL_RES_ADDR_CMD_CMPL_CBACK_EVT    25  /*!< \brief LE read local resolving address command complete */
#define HCI_LE_SET_ADDR_RES_ENABLE_CMD_CMPL_CBACK_EVT    26  /*!< \brief LE set address resolving enable command complete */
#define HCI_LE_ENCRYPT_CMD_CMPL_CBACK_EVT                27  /*!< \brief LE encrypt command complete */
#define HCI_LE_RAND_CMD_CMPL_CBACK_EVT                   28  /*!< \brief LE rand command complete */
#define HCI_LE_REM_CONN_PARAM_REP_CMD_CMPL_CBACK_EVT     29  /*!< \brief LE remote connection parameter request reply complete */
#define HCI_LE_REM_CONN_PARAM_NEG_REP_CMD_CMPL_CBACK_EVT 30  /*!< \brief LE remote connection parameter request negative reply complete */
#define HCI_LE_READ_DEF_DATA_LEN_CMD_CMPL_CBACK_EVT      31  /*!< \brief LE read suggested default data length command complete */
#define HCI_LE_WRITE_DEF_DATA_LEN_CMD_CMPL_CBACK_EVT     32  /*!< \brief LE write suggested default data length command complete */
#define HCI_LE_SET_DATA_LEN_CMD_CMPL_CBACK_EVT           33  /*!< \brief LE set data length command complete */
#define HCI_LE_READ_MAX_DATA_LEN_CMD_CMPL_CBACK_EVT      34  /*!< \brief LE read maximum data length command complete */
#define HCI_LE_REM_CONN_PARAM_REQ_CBACK_EVT              35  /*!< \brief LE remote connection parameter request */
#define HCI_LE_DATA_LEN_CHANGE_CBACK_EVT                 36  /*!< \brief LE data length change */
#define HCI_LE_READ_LOCAL_P256_PUB_KEY_CMPL_CBACK_EVT    37  /*!< \brief LE read local P-256 public key */
#define HCI_LE_GENERATE_DHKEY_CMPL_CBACK_EVT             38  /*!< \brief LE generate DHKey complete */
#define HCI_WRITE_AUTH_PAYLOAD_TO_CMD_CMPL_CBACK_EVT     39  /*!< \brief Write authenticated payload timeout command complete */
#define HCI_AUTH_PAYLOAD_TO_EXPIRED_CBACK_EVT            40  /*!< \brief Authenticated payload timeout expired event */
#define HCI_LE_READ_PHY_CMD_CMPL_CBACK_EVT               41  /*!< \brief LE read phy command complete */
#define HCI_LE_SET_DEF_PHY_CMD_CMPL_CBACK_EVT            42  /*!< \brief LE set default phy command complete */
#define HCI_LE_PHY_UPDATE_CMPL_CBACK_EVT                 43  /*!< \brief LE phy update complete */
#define HCI_LE_EXT_ADV_REPORT_CBACK_EVT                  44  /*!< \brief LE extended advertising report */
#define HCI_LE_SCAN_TIMEOUT_CBACK_EVT                    45  /*!< \brief LE scan timeout event */
#define HCI_LE_ADV_SET_TERM_CBACK_EVT                    46  /*!< \brief LE advertising set terminated event */
#define HCI_LE_SCAN_REQ_RCVD_CBACK_EVT                   47  /*!< \brief LE scan request received event */
#define HCI_LE_PER_ADV_SYNC_EST_CBACK_EVT                48  /*!< \brief LE periodic advertising sync established event */
#define HCI_LE_PER_ADV_REPORT_CBACK_EVT                  49  /*!< \brief LE periodic advertising report event */
#define HCI_LE_PER_ADV_SYNC_LOST_CBACK_EVT               50  /*!< \brief LE periodic advertising synch lost event */
#define HCI_LE_CH_SEL_ALGO_CBACK_EVT                     51  /*!< \brief LE channel selection algorithm event */
#define HCI_LE_SCAN_ENABLE_CMPL_CBACK_EVT                52  /*!< \brief LE scan enable complete event */
#define HCI_LE_ADV_ENABLE_CMPL_CBACK_EVT                 53  /*!< \brief LE advertise enable complete event */
#define HCI_LE_EXT_SCAN_ENABLE_CMPL_CBACK_EVT            54  /*!< \brief LE extended scan enable complete event */
#define HCI_LE_EXT_ADV_ENABLE_CMPL_CBACK_EVT             55  /*!< \brief LE extended advertise enable complete event */
#define HCI_LE_PER_ADV_ENABLE_CMPL_CBACK_EVT             56  /*!< \brief LE periodic advertise enable complete event */
#define HCI_READ_LOCAL_VER_INFO_CMPL_CBACK_EVT           57  /*!< \brief Read local version info complete event */
/**@}*/

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief LE connection complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;           /*!< \brief Event header */
  uint8_t             status;        /*!< \brief Status. */
  uint16_t            handle;        /*!< \brief Connection handle. */
  uint8_t             role;          /*!< \brief Local connection role. */
  uint8_t             addrType;      /*!< \brief Peer address type. */
  bdAddr_t            peerAddr;      /*!< \brief Peer address. */
  uint16_t            connInterval;  /*!< \brief Connection interval */
  uint16_t            connLatency;   /*!< \brief Connection latency. */
  uint16_t            supTimeout;    /*!< \brief Supervision timeout. */
  uint8_t             clockAccuracy; /*!< \brief Clock accuracy. */

  /* \brief enhanced fields */
  bdAddr_t            localRpa;      /*!< \brief Local RPA. */
  bdAddr_t            peerRpa;       /*!< \brief Peer RPA. */
} hciLeConnCmplEvt_t;

/*! \brief Disconnect complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;           /*!< \brief Event header. */
  uint8_t             status;        /*!< \brief Disconnect complete status. */
  uint16_t            handle;        /*!< \brief Connect handle. */
  uint8_t             reason;        /*!< \brief Reason. */
} hciDisconnectCmplEvt_t;

/*! \brief LE connection update complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;          /*!< \brief Event header. */
  uint8_t             status;       /*!< \brief Status. */
  uint16_t            handle;       /*!< \brief Connection handle. */
  uint16_t            connInterval; /*!< \brief Connection interval. */
  uint16_t            connLatency;  /*!< \brief Connection latency. */
  uint16_t            supTimeout;   /*!< \brief Supervision timeout. */
} hciLeConnUpdateCmplEvt_t;

/*! \brief LE create connection cancel command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
} hciLeCreateConnCancelCmdCmplEvt_t;

/*! \brief LE advertising report event */
typedef struct
{
  wsfMsgHdr_t         hdr;            /*!< \brief Event header. */
  uint8_t             *pData;         /*!< \brief advertising or scan response data. */
  uint8_t             len;            /*!< \brief length of advertising or scan response data. */
  int8_t              rssi;           /*!< \brief RSSI. */
  uint8_t             eventType;      /*!< \brief Advertising event type. */
  uint8_t             addrType;       /*!< \brief Address type. */
  bdAddr_t            addr;           /*!< \brief Device address. */

  /* \brief direct fields */
  uint8_t             directAddrType; /*!< \brief Direct advertising address type. */
  bdAddr_t            directAddr;     /*!< \brief Direct advertising address. */
} hciLeAdvReportEvt_t;

/*! \brief LE extended advertising report */
typedef struct
{
  wsfMsgHdr_t         hdr;            /*!< \brief Event header. */
  uint16_t            eventType;      /*!< \brief Event type. */
  uint8_t             addrType;       /*!< \brief Address type. */
  bdAddr_t            addr;           /*!< \brief Address. */
  uint8_t             priPhy;         /*!< \brief Primary PHY. */
  uint8_t             secPhy;         /*!< \brief Secondary PHY. */
  uint8_t             advSid;         /*!< \brief Advertising SID. */
  int8_t              txPower;        /*!< \brief Tx Power. */
  int8_t              rssi;           /*!< \brief RSSI. */
  int16_t             perAdvInter;    /*!< \brief Periodic advertising interval. */
  uint8_t             directAddrType; /*!< \brief Directed address type. */
  bdAddr_t            directAddr;     /*!< \brief Directed address. */
  uint16_t            len;            /*!< \brief Data buffer length. */
  uint8_t             *pData;         /*!< \brief Data buffer. */
} hciLeExtAdvReportEvt_t;

/*! \brief LE scan timeout */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< \brief Event header. */
} hciLeScanTimeoutEvt_t;

/*! \brief LE advertising set terminated */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< \brief Event header. */
  uint8_t       status;         /*!< \brief Status. */
  uint8_t       advHandle;      /*!< \brief Advertising handle. */
  uint16_t      handle;         /*!< \brief Connection handle. */
  uint8_t       numComplEvts;   /*!< \brief Number of completed extended advertising events. */
} hciLeAdvSetTermEvt_t;

/*! \brief LE scan request received */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< \brief Event header. */
  uint8_t       advHandle;      /*!< \brief Advertising handle. */
  uint8_t       scanAddrType;   /*!< \brief Scanner address type. */
  bdAddr_t      scanAddr;       /*!< \brief Scanner address. */
} hciLeScanReqRcvdEvt_t;

/*! \brief LE periodic advertising sync established */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< \brief Event header. */
  uint8_t       status;         /*!< \brief Status. */
  uint16_t      syncHandle;     /*!< \brief Sync handle. */
  uint8_t       advSid;         /*!< \brief Advertising SID. */
  uint8_t       advAddrType;    /*!< \brief Advertiser address type. */
  bdAddr_t      advAddr;        /*!< \brief Advertiser address. */
  uint8_t       advPhy;         /*!< \brief Advertiser PHY. */
  uint16_t      perAdvInterval; /*!< \brief Periodic advertising interval. */
  uint8_t       clockAccuracy;  /*!< \brief Advertiser clock accuracy. */
} hciLePerAdvSyncEstEvt_t;

/*! \brief LE periodic advertising report */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< \brief Event header. */
  uint16_t      syncHandle;     /*!< \brief Sync handle. */
  uint8_t       txPower;        /*!< \brief Tx power. */
  uint8_t       rssi;           /*!< \brief RSSI. */
  uint8_t       unused;         /*!< \brief Intended to be used in a future feature. */
  uint8_t       status;         /*!< \brief Data status. */
  uint16_t      len;            /*!< \brief Data buffer length. */
  uint8_t       *pData;         /*!< \brief Data buffer. */
} hciLePerAdvReportEvt_t;

/*! \brief LE periodic advertising synch lost */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< \brief Event header. */
  uint16_t      syncHandle;     /*!< \brief Sync handle. */
} hciLePerAdvSyncLostEvt_t;

/*! \brief LE channel selection algorithm */
typedef struct
{
  wsfMsgHdr_t   hdr;            /*!< \brief Event header. */
  uint16_t      handle;         /*!< \brief Connection handle. */
  uint8_t       chSelAlgo;      /*!< \brief Channel selection algorithm */
} hciLeChSelAlgoEvt_t;

/*! \brief Read RSSI command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
  int8_t              rssi;   /*!< \brief RSSI. */
} hciReadRssiCmdCmplEvt_t;

/*! \brief LE Read channel map command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;                       /*!< \brief Event header. */
  uint8_t             status;                    /*!< \brief Status. */
  uint16_t            handle;                    /*!< \brief Connection handle. */
  uint8_t             chanMap[HCI_CHAN_MAP_LEN]; /*!< \brief channel map. */
} hciReadChanMapCmdCmplEvt_t;

/*! \brief Read transmit power level command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint8_t             handle; /*!< \brief Connection handle. */
  int8_t              pwrLvl; /*!< \brief Tx power level. */
} hciReadTxPwrLvlCmdCmplEvt_t;

/*! \brief Read remote version information complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;        /*!< \brief Event header. */
  uint8_t             status;     /*!< \brief Status. */
  uint16_t            handle;     /*!< \brief Connection handle. */
  uint8_t             version;    /*!< \brief Version. */
  uint16_t            mfrName;    /*!< \brief Manufacturer name. */
  uint16_t            subversion; /*!< \brief Sub-version. */
} hciReadRemoteVerInfoCmplEvt_t;

/*! \brief LE read remote features complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;                    /*!< \brief Event header. */
  uint8_t             status;                 /*!< \brief Status. */
  uint16_t            handle;                 /*!< \brief Connection handle. */
  uint8_t             features[HCI_FEAT_LEN]; /*!< \brief Remote features buffer. */
} hciLeReadRemoteFeatCmplEvt_t;

/*! \brief LE LTK request reply command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
} hciLeLtkReqReplCmdCmplEvt_t;

/*! \brief LE LTK request negative reply command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
} hciLeLtkReqNegReplCmdCmplEvt_t;

/*! \brief Encryption key refresh complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
} hciEncKeyRefreshCmpl_t;

/*! \brief Encryption change event */
typedef struct
{
  wsfMsgHdr_t         hdr;     /*!< \brief Event header. */
  uint8_t             status;  /*!< \brief Status. */
  uint16_t            handle;  /*!< \brief Connection handle. */
  uint8_t             enabled; /*!< \brief Encryption enabled flag. */
} hciEncChangeEvt_t;

/*! \brief LE LTK request event */
typedef struct
{
  wsfMsgHdr_t         hdr;                   /*!< \brief Event header. */
  uint16_t            handle;                /*!< \brief Connection handle. */
  uint8_t             randNum[HCI_RAND_LEN]; /*!< \brief LTK random number. */
  uint16_t            encDiversifier;        /*!< \brief LTK encryption diversifier. */
} hciLeLtkReqEvt_t;

/*! \brief Vendor specific command status event */
typedef struct
{
  wsfMsgHdr_t        hdr;    /*!< \brief Event header. */
  uint16_t           opcode; /*!< \brief Opcode. */
} hciVendorSpecCmdStatusEvt_t;

/*! \brief Vendor specific command complete event */
typedef struct
{
  wsfMsgHdr_t        hdr;      /*!< \brief Event header. */
  uint16_t           opcode;   /*!< \brief Opcode. */
  uint8_t            param[1]; /*!< \brief Operation parameter. */
} hciVendorSpecCmdCmplEvt_t;

/*! \brief Vendor specific event */
typedef struct
{
  wsfMsgHdr_t        hdr;      /*!< \brief Event header. */
  uint8_t            param[1]; /*!< \brief Vendor specific event. */
} hciVendorSpecEvt_t;

/*! \brief Hardware error event */
typedef struct
{
  wsfMsgHdr_t        hdr;  /*!< \brief Event header. */
  uint8_t            code; /*!< \brief Error code. */
} hciHwErrorEvt_t;

/*! \brief LE encrypt command complete event */
typedef struct
{
  wsfMsgHdr_t        hdr;                        /*!< \brief Event header. */
  uint8_t            status;                     /*!< \brief Status. */
  uint8_t            data[HCI_ENCRYPT_DATA_LEN]; /*!< \brief Data. */
} hciLeEncryptCmdCmplEvt_t;

/*! \brief LE rand command complete event */
typedef struct
{
  wsfMsgHdr_t        hdr;                   /*!< \brief Event header. */
  uint8_t            status;                /*!< \brief Status. */
  uint8_t            randNum[HCI_RAND_LEN]; /*!< \brief Random number buffer. */
} hciLeRandCmdCmplEvt_t;

/*! \brief LE remote connection parameter request reply command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
} hciLeRemConnParamRepEvt_t;

/*! \brief LE remote connection parameter request negative reply command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
} hciLeRemConnParamNegRepEvt_t;

/*! \brief LE read suggested default data len command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;                  /*!< \brief Event header. */
  uint8_t             status;               /*!< \brief Status. */
  uint16_t            suggestedMaxTxOctets; /*!< \brief Suggested maximum Tx octets. */
  uint16_t            suggestedMaxTxTime;   /*!< \brief Suggested maximum Tx time. */
} hciLeReadDefDataLenEvt_t;

/*! \brief LE write suggested default data len command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
} hciLeWriteDefDataLenEvt_t;

/*! \brief LE set data len command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
} hciLeSetDataLenEvt_t;

/*! \brief LE read maximum data len command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;                  /*!< \brief Event header. */
  uint8_t             status;               /*!< \brief Status. */
  uint16_t            supportedMaxTxOctets; /*!< \brief Supported maximum Tx octets. */
  uint16_t            supportedMaxTxTime;   /*!< \brief Supported maximum Tx time. */
  uint16_t            supportedMaxRxOctets; /*!< \brief Supported maximum Rx octets. */
  uint16_t            supportedMaxRxTime;   /*!< \brief Supported maximum Rx time. */
} hciLeReadMaxDataLenEvt_t;

/*! \brief LE remote connetion parameter request event */
typedef struct
{
  wsfMsgHdr_t         hdr;         /*!< \brief Event header. */
  uint16_t            handle;      /*!< \brief Connection handle. */
  uint16_t            intervalMin; /*!< \brief Interval minimum. */
  uint16_t            intervalMax; /*!< \brief Interval maximum. */
  uint16_t            latency;     /*!< \brief Connection latency. */
  uint16_t            timeout;     /*!< \brief Connection timeout. */
} hciLeRemConnParamReqEvt_t;

/*! \brief LE data length change event */
typedef struct
{
  wsfMsgHdr_t        hdr;         /*!< \brief Event header. */
  uint16_t           handle;      /*!< \brief Connection handle. */
  uint16_t           maxTxOctets; /*!< \brief Maximum Tx octets. */
  uint16_t           maxTxTime;   /*!< \brief Maximum Tx time. */
  uint16_t           maxRxOctets; /*!< \brief Maximum Rx octets. */
  uint16_t           maxRxTime;   /*!< \brief Maximum Rx time. */
} hciLeDataLenChangeEvt_t;

/*! \brief LE local  p256 ecc key command complete event */
typedef struct
{
  wsfMsgHdr_t        hdr;                   /*!< \brief Event header. */
  uint8_t            status;                /*!< \brief Status. */
  uint8_t            key[HCI_P256_KEY_LEN]; /*!< \brief P-256 public keys. */
} hciLeP256CmplEvt_t;

/*! \brief LE generate DH key command complete event */
typedef struct
{
  wsfMsgHdr_t        hdr;                 /*!< \brief Event header. */
  uint8_t            status;              /*!< \brief Status. */
  uint8_t            key[HCI_DH_KEY_LEN]; /*!< \brief Diffie-Hellman (Share Secret) key. */
} hciLeGenDhKeyEvt_t;

/*! \brief LE read peer resolving address command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;                   /*!< \brief Event header. */
  uint8_t             status;                /*!< \brief Status. */
  uint8_t             peerRpa[BDA_ADDR_LEN]; /*!< \brief Peer RPA. */
} hciLeReadPeerResAddrCmdCmplEvt_t;

/*! \brief LE read local resolving address command complete event */
typedef struct
{
  wsfMsgHdr_t        hdr;                    /*!< \brief Event header. */
  uint8_t            status;                 /*!< \brief Status. */
  uint8_t            localRpa[BDA_ADDR_LEN]; /*!< \brief Local RPA. */
} hciLeReadLocalResAddrCmdCmplEvt_t;

/*! \brief LE set address resolving enable command complete event */
typedef struct
{
  wsfMsgHdr_t        hdr;    /*!< \brief Event header. */
  uint8_t            status; /*!< \brief Status. */
} hciLeSetAddrResEnableCmdCmplEvt_t;

/*! \brief LE add device to resolving list command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
} hciLeAddDevToResListCmdCmplEvt_t;

/*! \brief LE remove device from resolving list command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
} hciLeRemDevFromResListCmdCmplEvt_t;

/*! \brief LE clear resolving list command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
} hciLeClearResListCmdCmplEvt_t;

/*! \brief Write authenticated payload to command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
} hciWriteAuthPayloadToCmdCmplEvt_t;

/*! \brief Authenticated payload to expire event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint16_t            handle; /*!< \brief Connection handle. */
} hciAuthPayloadToExpiredEvt_t;

/*! \brief LE read PHY command complete event */
  typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Connection handle. */
  uint8_t             txPhy;  /*!< \brief Tx PHY. */
  uint8_t             rxPhy;  /*!< \brief Rx PHY. */
} hciLeReadPhyCmdCmplEvt_t;

/*! \brief LE set default PHY command complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
} hciLeSetDefPhyCmdCmplEvt_t;

/*! \brief LE PHY update complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;    /*!< \brief Event header. */
  uint8_t             status; /*!< \brief Status. */
  uint16_t            handle; /*!< \brief Handle. */
  uint8_t             txPhy;  /*!< \brief Tx PHY. */
  uint8_t             rxPhy;  /*!< \brief Rx PHY. */
} hciLePhyUpdateEvt_t;

/*! \brief Read local version information complete event */
typedef struct
{
  wsfMsgHdr_t         hdr;              /*!< \brief Event header. */
  uint8_t             status;           /*!< \brief Status. */
  uint8_t             hciVersion;       /*!< \brief HCI version. */
  uint16_t            hciRevision;      /*!< \brief HCI revision. */
  uint8_t             lmpVersion;       /*!< \brief LMP version. */
  uint16_t            manufacturerName; /*!< \brief Manufacturer name. */
  uint16_t            lmpSubversion;    /*!< \brief LMP Sub-version. */
} hciReadLocalVerInfo_t;

/*! \brief Union of all event types */
typedef union
{
  wsfMsgHdr_t                        hdr;                         /*!< \brief Event header. */
  wsfMsgHdr_t                        resetSeqCmpl;                /*!< \brief Reset sequence complete. */
  hciLeConnCmplEvt_t                 leConnCmpl;                  /*!< \brief LE connection complete. */
  hciDisconnectCmplEvt_t             disconnectCmpl;              /*!< \brief Disconnect complete. */
  hciLeConnUpdateCmplEvt_t           leConnUpdateCmpl;            /*!< \brief LE connection update complete. */
  hciLeCreateConnCancelCmdCmplEvt_t  leCreateConnCancelCmdCmpl;   /*!< \brief LE create connection cancel command complete. */
  hciLeAdvReportEvt_t                leAdvReport;                 /*!< \brief LE advertising report. */
  hciReadRssiCmdCmplEvt_t            readRssiCmdCmpl;             /*!< \brief Read RSSI command complete. */
  hciReadChanMapCmdCmplEvt_t         readChanMapCmdCmpl;          /*!< \brief Read channel map command complete. */
  hciReadTxPwrLvlCmdCmplEvt_t        readTxPwrLvlCmdCmpl;         /*!< \brief Read Tx power level command complete. */
  hciReadRemoteVerInfoCmplEvt_t      readRemoteVerInfoCmpl;       /*!< \brief Read remote version information complete. */
  hciLeReadRemoteFeatCmplEvt_t       leReadRemoteFeatCmpl;        /*!< \brief LE read remote feature complete. */
  hciLeLtkReqReplCmdCmplEvt_t        leLtkReqReplCmdCmpl;         /*!< \brief LE LTK request reply command complete. */
  hciLeLtkReqNegReplCmdCmplEvt_t     leLtkReqNegReplCmdCmpl;      /*!< \brief LE LT request negative reply command complete. */
  hciEncKeyRefreshCmpl_t             encKeyRefreshCmpl;           /*!< \brief Encryption key refresh complete. */
  hciEncChangeEvt_t                  encChange;                   /*!< \brief Encryption change. */
  hciLeLtkReqEvt_t                   leLtkReq;                    /*!< \brief LE LTK request. */
  hciVendorSpecCmdStatusEvt_t        vendorSpecCmdStatus;         /*!< \brief Vendor specific command status. */
  hciVendorSpecCmdCmplEvt_t          vendorSpecCmdCmpl;           /*!< \brief Vendor specific command complete. */
  hciVendorSpecEvt_t                 vendorSpec;                  /*!< \brief Vendor specific. */
  hciHwErrorEvt_t                    hwError;                     /*!< \brief Hardware error. */
  hciLeEncryptCmdCmplEvt_t           leEncryptCmdCmpl;            /*!< \brief LE encrypt command complete. */
  hciLeRandCmdCmplEvt_t              leRandCmdCmpl;               /*!< \brief LE random command complete. */
  hciLeReadPeerResAddrCmdCmplEvt_t   leReadPeerResAddrCmdCmpl;    /*!< \brief LE read peer resolvable address command complete. */
  hciLeReadLocalResAddrCmdCmplEvt_t  leReadLocalResAddrCmdCmpl;   /*!< \brief LE read local resolvable address command complete. */
  hciLeSetAddrResEnableCmdCmplEvt_t  leSetAddrResEnableCmdCmpl;   /*!< \brief LE set address resolution enable command complete. */
  hciLeAddDevToResListCmdCmplEvt_t   leAddDevToResListCmdCmpl;    /*!< \brief LE add device to resolving list command complete. */
  hciLeRemDevFromResListCmdCmplEvt_t leRemDevFromResListCmdCmpl;  /*!< \brief LE remove device from resolving list command complete. */
  hciLeClearResListCmdCmplEvt_t      leClearResListCmdCmpl;       /*!< \brief LE clear resolving list command complete. */
  hciLeRemConnParamRepEvt_t          leRemConnParamRepCmdCmpl;    /*!< \brief LE Remo Connection Parameter Reply Command Complete. */
  hciLeRemConnParamNegRepEvt_t       leRemConnParamNegRepCmdCmpl; /*!< \brief LE Remote Connection Parameter Negative Reply Command Complete. */
  hciLeReadDefDataLenEvt_t           leReadDefDataLenCmdCmpl;     /*!< \brief LE read default data length command complete. */
  hciLeWriteDefDataLenEvt_t          leWriteDefDataLenCmdCmpl;    /*!< \brief LE write default data length command complete. */
  hciLeSetDataLenEvt_t               leSetDataLenCmdCmpl;         /*!< \brief LE set data length command complete. */
  hciLeReadMaxDataLenEvt_t           leReadMaxDataLenCmdCmpl;     /*!< \brief LE read max data length command complete. */
  hciLeRemConnParamReqEvt_t          leRemConnParamReq;           /*!< \brief LE remote connection parameter request. */
  hciLeDataLenChangeEvt_t            leDataLenChange;             /*!< \brief LE data length change. */
  hciLeP256CmplEvt_t                 leP256;                      /*!< \brief LE P-256 */
  hciLeGenDhKeyEvt_t                 leGenDHKey;                  /*!< \brief LE generate Diffie-Hellman key. */
  hciWriteAuthPayloadToCmdCmplEvt_t  writeAuthPayloadToCmdCmpl;   /*!< \brief Write authenticated payload to command complete. */
  hciAuthPayloadToExpiredEvt_t       authPayloadToExpired;        /*!< \brief Authenticated payload to expired. */
  hciLeReadPhyCmdCmplEvt_t           leReadPhyCmdCmpl;            /*!< \brief LE read PHY command complete. */
  hciLeSetDefPhyCmdCmplEvt_t         leSetDefPhyCmdCmpl;          /*!< \brief LE set default PHY command complete. */
  hciLePhyUpdateEvt_t                lePhyUpdate;                 /*!< \brief LE PHY update. */
  hciLeExtAdvReportEvt_t             leExtAdvReport;              /*!< \brief LE extended advertising report. */
  hciLeScanTimeoutEvt_t              leScanTimeout;               /*!< \brief LE scan timeout. */
  hciLeAdvSetTermEvt_t               leAdvSetTerm;                /*!< \brief LE advertising set terminated. */
  hciLeScanReqRcvdEvt_t              leScanReqRcvd;               /*!< \brief LE scan request received. */
  hciLePerAdvSyncEstEvt_t            lePerAdvSyncEst;             /*!< \brief LE periodic advertising synchronization established. */
  hciLePerAdvReportEvt_t             lePerAdvReport;              /*!< \brief LE periodic advertising report. */
  hciLePerAdvSyncLostEvt_t           lePerAdvSyncLost;            /*!< \brief LE periodic advertising synchronization lost. */
  hciLeChSelAlgoEvt_t                leChSelAlgo;                 /*!< \brief LE channel select algorithm. */
  hciReadLocalVerInfo_t              readLocalVerInfo;            /*!< \brief Read local version information. */
} hciEvt_t;

/*! \} */    /* STACK_HCI_EVT_API */

/*! \addtogroup STACK_HCI_CMD_API
 * \{ */

/*! \brief Connection specification type */
typedef struct
{
  uint16_t            connIntervalMin; /*!< \brief Minimum connection interval. */
  uint16_t            connIntervalMax; /*!< \brief Maximum connection interval. */
  uint16_t            connLatency;     /*!< \brief Connection latency. */
  uint16_t            supTimeout;      /*!< \brief Supervision timeout. */
  uint16_t            minCeLen;        /*!< \brief Minimum CE length. */
  uint16_t            maxCeLen;        /*!< \brief Maximum CE length. */
} hciConnSpec_t;

/*! \brief Initiating parameters */
typedef struct
{
  uint8_t             filterPolicy;    /*!< \brief Scan filter policy. */
  uint8_t             ownAddrType;     /*!< \brief Address type used by this device. */
  uint8_t             peerAddrType;    /*!< \brief Address type used for peer device. */
  const uint8_t       *pPeerAddr;      /*!< \brief Address of peer device. */
  uint8_t             initPhys;        /*!< \brief Initiating PHYs. */
} hciExtInitParam_t;

/*! \brief Initiating scan parameters */
typedef struct
{
  uint16_t            scanInterval;    /*!< \brief Scan interval. */
  uint16_t            scanWindow;      /*!< \brief Scan window. */
} hciExtInitScanParam_t;

/*! \brief Extended advertising parameters */
typedef struct
{
  uint16_t            advEventProp;    /*!< \brief Advertising Event Properties. */
  uint32_t            priAdvInterMin;  /*!< \brief Primary Advertising Interval Minimum. */
  uint32_t            priAdvInterMax;  /*!< \brief Primary Advertising Interval Maximum. */
  uint8_t             priAdvChanMap;   /*!< \brief Primary Advertising Channel Map. */
  uint8_t             ownAddrType;     /*!< \brief Own Address Type. */
  uint8_t             peerAddrType;    /*!< \brief Peer Address Type. */
  uint8_t             *pPeerAddr;      /*!< \brief Peer Address. */
  uint8_t             advFiltPolicy;   /*!< \brief Advertising Filter Policy. */
  int8_t              advTxPwr;        /*!< \brief Advertising Tx Power. */
  uint8_t             priAdvPhy;       /*!< \brief Primary Advertising PHY. */
  uint8_t             secAdvMaxSkip;   /*!< \brief Secondary Advertising Maximum Skip. */
  uint8_t             secAdvPhy;       /*!< \brief Secondary Advertising PHY. */
  uint8_t             advSetId;        /*!< \brief Advertising set ID. */
  uint8_t             scanReqNotifEna; /*!< \brief Scan Request Notification Enable. */
} hciExtAdvParam_t;

/*! \brief Extended advertising enable parameters */
typedef struct
{
  uint8_t             advHandle;       /*!< \brief Advertising handle. */
  uint16_t            duration;        /*!< \brief Advertising duration in 10 ms units. */
  uint8_t             maxEaEvents;     /*!< \brief Maximum number of extended advertising events. */
} hciExtAdvEnableParam_t;

/*! \brief Extended scanning parameters */
typedef struct
{
  uint16_t            scanInterval;    /*!< \brief Scan interval. */
  uint16_t            scanWindow;      /*!< \brief Scan window. */
  uint8_t             scanType;        /*!< \brief Scan type. */
} hciExtScanParam_t;

/*! \} */    /* STACK_HCI_CMD_API */

/**************************************************************************************************
  Callback Function Types
**************************************************************************************************/

/*! \addtogroup STACK_HCI_EVT_API
 * \{ */

/*! \brief HCI event callback type.
 *
 *  This callback function sends events from HCI to the stack.
 *
 *  \param  pEvent    Pointer to HCI callback event structure.
 *
 *  \return None.
 */
typedef void (*hciEvtCback_t)(hciEvt_t *pEvent);

/*! \brief HCI security callback type
 *
 *  This callback function sends certain security events from HCI to the stack.
 *  The security events passed in this callback are the LE Rand Command Complete event and the
 *  LE Encrypt Command Complete event.
 *
 *  \param  pEvt    Pointer to HCK callback event structure.
 *
 *  \return None.
 */
typedef void (*hciSecCback_t)(hciEvt_t *pEvent);

/*! \} */    /* STACK_HCI_EVT_API */

/*! \addtogroup STACK_HCI_ACL_API
 * \{ */

/*! \brief HCI ACL callback type
 *
 *  This callback function sends a data from HCI to the stack.
 *
 *  \param  pData    WSF buffer containing an ACL packet.
 *
 *  \return None.
 */
typedef void (*hciAclCback_t)(uint8_t *pData);

/*! \brief HCI flow control callback type
 *
 *  This callback function manages flow control in the TX path betrween the stack and HCI.
 *
 *  \param  connId          Connection handle.
 *  \param  flowDisabled    TRUE if flow is disabled.
 *
 *  \return None.
 */
typedef void (*hciFlowCback_t)(uint16_t handle, bool_t flowDisabled);

/*! \} */    /* STACK_HCI_ACL_API */

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*! \addtogroup STACK_HCI_INIT_API
 *  \{ */

/** \name HCI Initialization, Registration, Reset
 *
 */
/**@{*/
/*************************************************************************************************/
/*!
 *  \brief  Register a callback for HCI events.
 *
 *  \param  evtCback  Callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciEvtRegister(hciEvtCback_t evtCback);

/*************************************************************************************************/
/*!
 *  \brief  Register a callback for certain HCI security events.
 *
 *  \param  secCback  Callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSecRegister(hciSecCback_t secCback);

/*************************************************************************************************/
/*!
 *  \brief  Register callbacks for the HCI data path.
 *
 *  \param  aclCback  ACL data callback function.
 *  \param  flowCback Flow control callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciAclRegister(hciAclCback_t aclCback, hciFlowCback_t flowCback);

/*************************************************************************************************/
/*!
 *  \brief  Initiate an HCI reset sequence.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciResetSequence(void);

/*************************************************************************************************/
/*!
 *  \brief  Vendor-specific controller initialization function.
 *
 *  \param  param    Vendor-specific parameter.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciVsInit(uint8_t param);

/*************************************************************************************************/
/*!
 *  \brief  HCI core initialization.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciCoreInit(void);

/*************************************************************************************************/
/*!
 *  \brief  WSF event handler for core HCI.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciCoreHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

/*************************************************************************************************/
/*!
 *  \brief  Set the maximum reassembled RX ACL packet length.  Minimum value is 27.
 *
 *  \param  len     ACL packet length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSetMaxRxAclLen(uint16_t len);

/*************************************************************************************************/
/*!
 *  \brief  Set TX ACL queue high and low watermarks.
 *
 *  \param  queueHi   Disable flow on a connection when this many ACL buffers are queued.
 *  \param  queueLo   Disable flow on a connection when this many ACL buffers are queued.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSetAclQueueWatermarks(uint8_t queueHi, uint8_t queueLo);

/*************************************************************************************************/
/*!
*  \brief   Set LE supported features configuration mask.
*
*  \param   feat    Feature bit to set or clear
*  \param   flag    TRUE to set feature bit and FALSE to clear it
*
*  \return None.
*/
/*************************************************************************************************/
void HciSetLeSupFeat(uint16_t feat, bool_t flag);
/**@}*/

/*************************************************************************************************/
/*!
 *  \brief  Vendor-specific controller AE initialization function.
 *
 *  \param  param    Vendor-specific parameter.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciVsAeInit(uint8_t param);

/*! \} */    /* STACK_HCI_INIT_API */

/*! \addtogroup STACK_HCI_OPT_API
 * \{ */

/** \name HCI Optimization Interface Functions
 * This is an optimized interface for certain HCI commands that simply read a
 * value.  The stack uses these functions rather than their corresponding
 * functions in the command interface.
 * These functions can only be called after the reset sequence has been completed.
 */
/**@{*/
/*************************************************************************************************/
/*!
 *  \brief  Return a pointer to the BD address of this device.
 *
 *  \return Pointer to the BD address.
 */
/*************************************************************************************************/
uint8_t *HciGetBdAddr(void);

/*************************************************************************************************/
/*!
 *  \brief  Return the white list size.
 *
 *  \return White list size.
 */
/*************************************************************************************************/
uint8_t HciGetWhiteListSize(void);

/*************************************************************************************************/
/*!
 *  \brief  Return the advertising transmit power.
 *
 *  \return Advertising transmit power.
 */
/*************************************************************************************************/
int8_t HciGetAdvTxPwr(void);

/*************************************************************************************************/
/*!
 *  \brief  Return the ACL buffer size supported by the controller.
 *
 *  \return ACL buffer size.
 */
/*************************************************************************************************/
uint16_t HciGetBufSize(void);

/*************************************************************************************************/
/*!
 *  \brief  Return the number of ACL buffers supported by the controller.
 *
 *  \return Number of ACL buffers.
 */
/*************************************************************************************************/
uint8_t HciGetNumBufs(void);

/*************************************************************************************************/
/*!
 *  \brief  Return the states supported by the controller.
 *
 *  \return Pointer to the supported states array.
 */
/*************************************************************************************************/
uint8_t *HciGetSupStates(void);

/*************************************************************************************************/
/*!
 *  \brief  Return the LE supported features supported by the controller.
 *
 *  \return Supported features.
 */
/*************************************************************************************************/
uint16_t HciGetLeSupFeat(void);

/*************************************************************************************************/
/*!
 *  \brief  Get the maximum reassembled RX ACL packet length.
 *
 *  \return ACL packet length.
 */
/*************************************************************************************************/
uint16_t HciGetMaxRxAclLen(void);

/*************************************************************************************************/
/*!
 *  \brief  Return the resolving list size.
 *
 *  \return resolving list size.
 */
/*************************************************************************************************/
uint8_t HciGetResolvingListSize(void);

/*************************************************************************************************/
/*!
*  \brief  Whether LL Privacy is supported.
*
*  \return TRUE if LL Privacy is supported. FALSE, otherwise.
*/
/*************************************************************************************************/
bool_t HciLlPrivacySupported(void);

/*************************************************************************************************/
/*!
*  \brief  Get the maximum advertisement (or scan response) data length supported by the Controller.
*
*  \return Maximum advertisement data length.
*/
/*************************************************************************************************/
uint16_t HciGetMaxAdvDataLen(void);

/*************************************************************************************************/
/*!
*  \brief  Get the maximum number of advertising sets supported by the Controller.
*
*  \return Maximum number of advertising sets.
*/
/*************************************************************************************************/
uint8_t HciGetNumSupAdvSets(void);

/*************************************************************************************************/
/*!
*  \brief  Whether LE Advertising Extensions is supported.
*
*  \return TRUE if LE Advertising Extensions is supported. FALSE, otherwise.
*/
/*************************************************************************************************/
bool_t HciLeAdvExtSupported(void);

/*************************************************************************************************/
/*!
 *  \brief  Return the periodic advertising list size.
 *
 *  \return periodic advertising list size.
 */
/*************************************************************************************************/
uint8_t HciGetPerAdvListSize(void);
/**@}*/

/*! \} */    /* STACK_HCI_OPT_API */

/*! \addtogroup STACK_HCI_ACL_API
 * \{ */

/** \name HCI ACL Data Functions
 * HCI ACL data interface
 */
/**@{*/

/*************************************************************************************************/
/*!
 *  \brief  Send ACL Data from the stack to HCI.
 *
 *  \param  pAclData    WSF buffer containing an ACL packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSendAclData(uint8_t *pAclData);
/**@}*/

/*! \} */    /* STACK_HCI_ACL_API */

/*! \addtogroup STACK_HCI_CMD_API
 * \{ */

/** \name HCI Command Interface Functions
 * HCI commands
 */
/**@{*/
/*************************************************************************************************/
/*!
 *  \brief  HCI disconnect command.
 *
 *  \param  handle    Connection handle.
 *  \param  reason    Reason for disconnect.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciDisconnectCmd(uint16_t handle, uint8_t reason);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE add device white list command.
 *
 *  \param  addrType    Address type.
 *  \param  pAddr       Peer address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeAddDevWhiteListCmd(uint8_t addrType, uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE clear white list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeClearWhiteListCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI connection update command.
 *
 *  \param  handle       Connection handle.
 *  \param  pConnSpec    Update connection parameters.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeConnUpdateCmd(uint16_t handle, hciConnSpec_t *pConnSpec);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE create connection command.
 *
 *  \param  scanInterval    Scan interval.
 *  \param  scanWindow      Scan window.
 *  \param  filterPolicy    Filter policy.
 *  \param  peerAddrType    Peer address type.
 *  \param  pPeerAddr       Peer address.
 *  \param  ownAddrType     Own address type.
 *  \param  pConnSpec       Connecdtion parameters.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeCreateConnCmd(uint16_t scanInterval, uint16_t scanWindow, uint8_t filterPolicy,
                        uint8_t peerAddrType, uint8_t *pPeerAddr, uint8_t ownAddrType,
                        hciConnSpec_t *pConnSpec);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE create connection cancel command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeCreateConnCancelCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE encrypt command.
 *
 *  \param  pKey     Encryption key.
 *  \param  pData    Data to encrypt.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeEncryptCmd(uint8_t *pKey, uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE long term key request negative reply command.
 *
 *  \param  handle    Connection handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeLtkReqNegReplCmd(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE long term key request reply command.
 *
 *  \param  handle    Connection handle.
 *  \param  pKey      LTK.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeLtkReqReplCmd(uint16_t handle, uint8_t *pKey);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE random command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRandCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read advertising TX power command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadAdvTXPowerCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read buffer size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadBufSizeCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read channel map command.
 *
 *  \param  handle    Connection handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadChanMapCmd(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read local supported feautre command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalSupFeatCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read remote feature command.
 *
 *  \param  handle    Connection handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadRemoteFeatCmd(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read supported states command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadSupStatesCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE read white list size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadWhiteListSizeCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE remove device white list command.
 *
 *  \param  addrType    Address type.
 *  \param  pAddr       Peer address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoveDevWhiteListCmd(uint8_t addrType, uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advanced enable command.
 *
 *  \param  enable    Enable.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvEnableCmd(uint8_t enable);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advertising data command.
 *
 *  \param  len      Length of advertising data.
 *  \param  pData    Advertising data.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvDataCmd(uint8_t len, uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set advertising parameters command.
 *
 *  \param  advIntervalMin    Adveritsing minimum interval.
 *  \param  advIntervalMax    Advertising maximum interval.
 *  \param  advType           Advertising type.
 *  \param  ownAddrType       Own address type.
 *  \param  peerAddrType      Peer address type.
 *  \param  pPeerAddr         Peer address.
 *  \param  advChanMap        Advertising channel map.
 *  \param  advFiltPolicy     Advertising filter policy.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAdvParamCmd(uint16_t advIntervalMin, uint16_t advIntervalMax, uint8_t advType,
                         uint8_t ownAddrType, uint8_t peerAddrType, uint8_t *pPeerAddr,
                         uint8_t advChanMap, uint8_t advFiltPolicy);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set event mask command.
 *
 *  \param  pLeEventMask    LE Event mask.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetEventMaskCmd(uint8_t *pLeEventMask);

/*************************************************************************************************/
/*!
 *  \brief  HCI set host channel class command.
 *
 *  \param  pChanMap    Channel map.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetHostChanClassCmd(uint8_t *pChanMap);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set random address command.
 *
 *  \param  pAddr    Randon address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetRandAddrCmd(uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set scan enable command.
 *
 *  \param  enable       Enable.
 *  \param  filterDup    Filter duplicates.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetScanEnableCmd(uint8_t enable, uint8_t filterDup);

/*************************************************************************************************/
/*!
 *  \brief  HCI set scan parameters command.
 *
 *  \param  scanType          Scan type.
 *  \param  scanInterval      Scan interval.
 *  \param  scanWindow        Scan window.
 *  \param  ownAddrType       Own address type.
 *  \param  scanFiltPolicy    Scanning filter policy.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetScanParamCmd(uint8_t scanType, uint16_t scanInterval, uint16_t scanWindow,
                          uint8_t ownAddrType, uint8_t scanFiltPolicy);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set scan response data.
 *
 *  \param  len      Scan response data length.
 *  \param  pData    Scan response data.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetScanRespDataCmd(uint8_t len, uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE start encryption command.
 *
 *  \param  handle         Connection handle.
 *  \param  pRand          Random number.
 *  \param  diversifier    Diversifier.
 *  \param  pKey           Encryption key.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeStartEncryptionCmd(uint16_t handle, uint8_t *pRand, uint16_t diversifier, uint8_t *pKey);

/*************************************************************************************************/
/*!
 *  \brief  HCI read BD address command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadBdAddrCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI read buffer size command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadBufSizeCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI read local supported feature command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadLocalSupFeatCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI read local version info command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadLocalVerInfoCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI read remote version info command.
 *
 *  \param  handle    Connection handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadRemoteVerInfoCmd(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief  HCI read RSSI command.
 *
 *  \param  handle    Connection handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadRssiCmd(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief  HCI read Tx power level command.
 *
 *  \param  handle    Connection handle.
 *  \param  type      Type.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadTxPwrLvlCmd(uint16_t handle, uint8_t type);

/*************************************************************************************************/
/*!
 *  \brief  HCI reset command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciResetCmd(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI set event mask command.
 *
 *  \param  pEventMask    Page 1 of the event mask.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciSetEventMaskCmd(uint8_t *pEventMask);

/*************************************************************************************************/
/*!
 *  \brief  HCI set event page 2 mask command.
 *
 *  \param  pEventMask    Page 2 of the event mask.
 *  \return None.
 */
/*************************************************************************************************/
void HciSetEventMaskPage2Cmd(uint8_t *pEventMask);

/*************************************************************************************************/
/*!
 *  \brief  HCI read authenticated payload timeout command.
 *
 *  \param  handle    Connection handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciReadAuthPayloadTimeout(uint16_t handle);

/*************************************************************************************************/
/*!
 *  \brief  HCI write authenticated payload timeout command.
 *
 *  \param  handle     Connection handle.
 *  \param  timeout    Timeout value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciWriteAuthPayloadTimeout(uint16_t handle, uint16_t timeout);

/*************************************************************************************************/
/*!
 *  \brief  HCI add device to resolving list command.
 *
 *  \param  peerAddrType         Peer identity address type.
 *  \param  pPeerIdentityAddr    Peer identity address.
 *  \param  pPeerIrk             Peer IRK.
 *  \param  pLocalIrk            Local IRK.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeAddDeviceToResolvingListCmd(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr,
                                      const uint8_t *pPeerIrk, const uint8_t *pLocalIrk);

/*************************************************************************************************/
/*!
 *  \brief  HCI remove device from resolving list command.
 *
 *  \param  peerAddrType         Peer identity address type.
 *  \param  pPeerIdentityAddr    Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoveDeviceFromResolvingList(uint8_t peerAddrType, const uint8_t *pPeerIdentityAddr);

/*************************************************************************************************/
/*!
 *  \brief  HCI clear resolving list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeClearResolvingList(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI read resolving list command.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadResolvingListSize(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI read peer resolvable address command.
 *
 *  \param  addrType         Peer identity address type.
 *  \param  pIdentityAddr    Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadPeerResolvableAddr(uint8_t addrType, const uint8_t *pIdentityAddr);

/*************************************************************************************************/
/*!
 *  \brief  HCI read local resolvable address command.
 *
 *  \param  addrType         Peer identity address type.
 *  \param  pIdentityAddr    Peer identity address.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalResolvableAddr(uint8_t addrType, const uint8_t *pIdentityAddr);

/*************************************************************************************************/
/*!
 *  \brief  HCI enable or disable address resolution command.
 *
 *  \param  enable    Set to TRUE to enable address resolution or FALSE to disable address
 *                    resolution.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetAddrResolutionEnable(uint8_t enable);

/*************************************************************************************************/
/*!
 *  \brief  HCI set resolvable private address timeout command.
 *
 *  \param  rpaTimeout    Timeout measured in seconds.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetResolvablePrivateAddrTimeout(uint16_t rpaTimeout);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE set privacy mode command.
 *
 *  \param  addrType    Peer identity address type.
 *  \param  pAddr       Peer identity address.
 *  \param  mode        Privacy mode.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetPrivacyModeCmd(uint8_t addrType, uint8_t *pAddr, uint8_t mode);

/*************************************************************************************************/
/*!
*  \brief  HCI read PHY command.
*
*  \param  handle    Connection handle.
*
*  \return None.
*/
/*************************************************************************************************/
void HciLeReadPhyCmd(uint16_t handle);

/*************************************************************************************************/
/*!
*  \brief  HCI set default PHY command.
*
*  \param  allPhys    All PHYs.
*  \param  txPhys     Tx PHYs.
*  \param  rxPhys     Rx PHYs.
*
*  \return None.
*/
/*************************************************************************************************/
void HciLeSetDefaultPhyCmd(uint8_t allPhys, uint8_t txPhys, uint8_t rxPhys);

/*************************************************************************************************/
/*!
*  \brief  HCI set PHY command.
*
*  \param  handle        Connection handle.
*  \param  allPhys       All PHYs.
*  \param  txPhys        Tx PHYs.
*  \param  rxPhys        Rx PHYs.
*  \param  phyOptions    PHY options.
*
*  \return None.
*/
/*************************************************************************************************/
void HciLeSetPhyCmd(uint16_t handle, uint8_t allPhys, uint8_t txPhys, uint8_t rxPhys, uint16_t phyOptions);

/*************************************************************************************************/
/*!
 *  \brief  HCI vencor specific command.
 *
 *  \param  opcode    Opcode.
 *  \param  len       Length of pData.
 *  \param  pData     Command data.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciVendorSpecificCmd(uint16_t opcode, uint8_t len, uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief  HCI Remote Connection Parameter Request Reply.
 *
 *  \param  handle         Connection handle.
 *  \param  intervalMin    Interval minimum.
 *  \param  intervalMax    Interval maximum.
 *  \param  latency        Connection latency.
 *  \param  timeout        Connection timeout.
 *  \param  minCeLen       Minimum connection event length.
 *  \param  maxCeLen       Maximum connection event length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoteConnParamReqReply(uint16_t handle, uint16_t intervalMin, uint16_t intervalMax, uint16_t latency,
                                  uint16_t timeout, uint16_t minCeLen, uint16_t maxCeLen);

/*************************************************************************************************/
/*!
 *  \brief  HCI Remote Connection Parameter Request Negative Reply.
 *
 *  \param  handle    Connection handle.
 *  \param  reason    Negative reply reason.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeRemoteConnParamReqNegReply(uint16_t handle, uint8_t reason);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Set Data Length.
 *
 *  \param  handle      Connection handle.
 *  \param  txOctets    Tx octets.
 *  \param  txTime      Tx time.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetDataLen(uint16_t handle, uint16_t txOctets, uint16_t txTime);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Read Default Data Length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadDefDataLen(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Write Default Data Length.
 *
 *  \param  suggestedMaxTxOctets    Suggested maximum Tx octets.
 *  \param  suggestedMaxTxTime      Suggested maximum Tx time.
 *
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeWriteDefDataLen(uint16_t suggestedMaxTxOctets, uint16_t suggestedMaxTxTime);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Read Local P-256 Public Key.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadLocalP256PubKey(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Generate DH Key.
 *
 *  \param  pPubKeyX    Public key X-coordinate.
 *  \param  pPubKeyY    Public key Y-coordinate.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeGenerateDHKey(uint8_t *pPubKeyX, uint8_t *pPubKeyY);

/*************************************************************************************************/
/*!
 *  \brief  HCI LE Read Maximum Data Length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeReadMaxDataLen(void);

/*************************************************************************************************/
/*!
 *  \brief  HCI write authenticated payload timeout command.
 *
 *  \param  handle     Connection handle.
 *  \param  timeout    Timeout value.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciWriteAuthPayloadTimeout(uint16_t handle, uint16_t timeout);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE read transmit power command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeReadTxPower(void);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE read RF path compensation command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeReadRfPathComp(void);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE write RF path compensation command.
 *
 *  \param      txPathComp    RF transmit path compensation value.
 *  \param      rxPathComp    RF receive path compensation value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeWriteRfPathComp(int16_t txPathComp, int16_t rxPathComp);
/**@}*/

/** \name HCI AE Advertiser Interface
 * HCI Advertising Extension functions used by the Advertiser role.
 */
/**@{*/
/*************************************************************************************************/
/*!
 *  \brief      HCI LE set advertising set random device address command.
 *
 *  \param      advHandle    Advertising handle.
 *  \param      pAddr        Random device address.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetAdvSetRandAddrCmd(uint8_t advHandle, const uint8_t *pAddr);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set extended advertising parameters command.
 *
 *  \param      advHandle       Advertising handle.
 *  \param      pExtAdvParam    Extended advertising parameters.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetExtAdvParamCmd(uint8_t advHandle, hciExtAdvParam_t *pExtAdvParam);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set extended advertising data command.
 *
 *  \param      advHandle    Advertising handle.
 *  \param      op           Operation.
 *  \param      fragPref     Fragment preference.
 *  \param      len          Data buffer length.
 *  \param      pData        Advertising data buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetExtAdvDataCmd(uint8_t advHandle, uint8_t op, uint8_t fragPref, uint8_t len,
                           const uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set extended scan response data command.
 *
 *  \param      advHandle    Advertising handle.
 *  \param      op           Operation.
 *  \param      fragPref     Fragment preference.
 *  \param      len          Data buffer length.
 *  \param      pData        Scan response data buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetExtScanRespDataCmd(uint8_t advHandle, uint8_t op, uint8_t fragPref, uint8_t len,
                                const uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set extended advertising enable command.
 *
 *  \param      enable          Set to TRUE to enable advertising, FALSE to disable advertising.
 *  \param      numSets         Number of advertising sets.
 *  \param      pEnableParam    Advertising enable parameter array.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetExtAdvEnableCmd(uint8_t enable, uint8_t numSets, hciExtAdvEnableParam_t *pEnableParam);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE read maximum advertising data length command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeReadMaxAdvDataLen(void);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE read number of supported advertising sets command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeReadNumSupAdvSets(void);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE remove advertising set command.
 *
 *  \param      advHandle    Advertising handle.
 *
 *  \return     Status error code.
 */
/*************************************************************************************************/
void HciLeRemoveAdvSet(uint8_t advHandle);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE clear advertising sets command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeClearAdvSets(void);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set periodic advertising parameters command.
 *
 *  \param      advHandle         Advertising handle.
 *  \param      advIntervalMin    Periodic advertising interval minimum.
 *  \param      advIntervalMax    Periodic advertising interval maximum.
 *  \param      advProps          Periodic advertising properties.
 *
 *  \return None.
 */
/*************************************************************************************************/
void HciLeSetPerAdvParamCmd(uint8_t advHandle, uint16_t advIntervalMin, uint16_t advIntervalMax,
                            uint16_t advProps);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set periodic advertising data command.
 *
 *  \param      advHandle    Advertising handle.
 *  \param      op           Operation.
 *  \param      len          Data buffer length.
 *  \param      pData        Advertising data buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetPerAdvDataCmd(uint8_t advHandle, uint8_t op, uint8_t len, const uint8_t *pData);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE set periodic advertising enable command.
 *
 *  \param      enable       Set to TRUE to enable advertising, FALSE to disable advertising.
 *  \param      advHandle    Advertising handle.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetPerAdvEnableCmd(uint8_t enable, uint8_t advHandle);
/**@}*/

/** \name HCI AE Scanner Interface
 * HCI Advertising Extension functions used in the Scanner role.
 */
/**@{*/
/*************************************************************************************************/
/*!
 *  \brief      HCI LE set extended scanning parameters command.
 *
 *  \param      ownAddrType       Address type used by this device.
 *  \param      scanFiltPolicy    Scan filter policy.
 *  \param      scanPhys          Scanning PHYs.
 *  \param      pScanParam        Scanning parameter array.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeSetExtScanParamCmd(uint8_t ownAddrType, uint8_t scanFiltPolicy, uint8_t scanPhys,
                             hciExtScanParam_t *pScanParam);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE extended scan enable command.
 *
 *  \param      enable          Set to TRUE to enable scanning, FALSE to disable scanning.
 *  \param      filterDup       Set to TRUE to filter duplicates.
 *  \param      duration        Duration.
 *  \param      period          Period.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeExtScanEnableCmd(uint8_t enable, uint8_t filterDup, uint16_t duration, uint16_t period);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE extended create connection command.
 *
 *  \param      pInitParam      Initiating parameters.
 *  \param      pScanParam      Initiating scan parameters.
 *  \param      pConnSpec       Connection specification.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeExtCreateConnCmd(hciExtInitParam_t *pInitParam, hciExtInitScanParam_t *pScanParam,
                           hciConnSpec_t *pConnSpec);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE periodic advertising create sync command.
 *
 *  \param      filterPolicy    Filter policy.
 *  \param      advSid          Advertising SID.
 *  \param      advAddrType     Advertiser address type.
 *  \param      pAdvAddr        Advertiser address.
 *  \param      skip            Number of periodic advertising packets that can be skipped after
 *                              successful receive.
 *  \param      syncTimeout     Synchronization timeout.
 *  \param      unused          Reserved for future use (must be zero).
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLePerAdvCreateSyncCmd(uint8_t filterPolicy, uint8_t advSid, uint8_t advAddrType,
                              uint8_t *pAdvAddr, uint16_t skip, uint16_t syncTimeout, uint8_t unused);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE periodic advertising create sync cancel command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLePerAdvCreateSyncCancelCmd(void);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE periodic advertising terminate sync command.
 *
 *  \param      syncHandle      Sync handle.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLePerAdvTerminateSyncCmd(uint16_t syncHandle);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE add device to periodic advertiser list command.
 *
 *  \param      advAddrType     Advertiser address type.
 *  \param      pAdvAddr        Advertiser address.
 *  \param      advSid          Advertising SID.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeAddDeviceToPerAdvListCmd(uint8_t advAddrType, uint8_t *pAdvAddr, uint8_t advSid);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE remove device from periodic advertiser list command.
 *
 *  \param      advAddrType     Advertiser address type.
 *  \param      pAdvAddr        Advertiser address.
 *  \param      advSid          Advertising SID.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeRemoveDeviceFromPerAdvListCmd(uint8_t advAddrType, uint8_t *pAdvAddr, uint8_t advSid);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE clear periodic advertiser list command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeClearPerAdvListCmd(void);

/*************************************************************************************************/
/*!
 *  \brief      HCI LE read periodic advertiser size command.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void HciLeReadPerAdvListSizeCmd(void);

/**@}*/

/*! \} */    /* STACK_HCI_CMD_API */

#ifdef __cplusplus
};
#endif

#endif /* HCI_API_H */
