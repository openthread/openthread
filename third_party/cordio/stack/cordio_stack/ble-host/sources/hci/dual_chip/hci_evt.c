/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  HCI event module.
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

#include <string.h>
#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "hci_api.h"
#include "hci_main.h"
#include "hci_evt.h"
#include "hci_cmd.h"
#include "hci_core.h"

#ifdef WSF_DETOKEN_TRACE
#include "wsf_detoken.h"
#endif /* LL_DETOKEN_TRACE */

/**************************************************************************************************
  Macros
**************************************************************************************************/

/* Maximum number of reports that can fit in an advertising report event */
#define HCI_MAX_REPORTS               15

/* Length of fixed parameters in an advertising report event */
#define HCI_LE_ADV_REPORT_FIXED_LEN   2

/* Length of fixed parameters in each individual report */
#define HCI_LE_ADV_REPORT_INDIV_LEN   10

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/* Event parsing function type */
typedef void (*hciEvtParse_t)(hciEvt_t *pMsg, uint8_t *p, uint8_t len);

/**************************************************************************************************
  Local Declarations
**************************************************************************************************/

/* Event parsing functions */
static void hciEvtParseLeConnCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeEnhancedConnCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseDisconnectCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeConnUpdateCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeCreateConnCancelCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadRssiCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadChanMapCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadTxPwrLvlCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadRemoteVerInfoCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadLeRemoteFeatCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeLtkReqReplCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeLtkReqNegReplCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseEncKeyRefreshCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseEncChange(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeLtkReq(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseVendorSpecCmdStatus(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseVendorSpecCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseVendorSpec(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseHwError(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeEncryptCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeRandCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeAddDevToResListCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeRemDevFromResListCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeClearResListCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeReadPeerResAddrCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeReadLocalResAddrCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeSetAddrResEnableCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseRemConnParamRepCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseRemConnParamNegRepCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadDefDataLenCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseWriteDefDataLenCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseSetDataLenCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadMaxDataLenCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseRemConnParamReq(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseDataLenChange(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadPubKeyCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseGenDhKeyCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseWriteAuthTimeoutCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseAuthTimeoutExpiredEvt(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadPhyCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseSetDefPhyCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParsePhyUpdateCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeScanTimeout(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeAdvSetTerm(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeScanReqRcvd(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLePerAdvSyncEst(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLePerAdvSyncLost(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeChSelAlgo(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseLeCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len);
static void hciEvtParseReadLocalVerInfo(hciEvt_t *pMsg, uint8_t *p, uint8_t len);

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/* Event parsing function lookup table, indexed by internal callback event value */
static const hciEvtParse_t hciEvtParseFcnTbl[] =
{
  NULL,
  hciEvtParseLeConnCmpl,
  hciEvtParseLeEnhancedConnCmpl,
  hciEvtParseDisconnectCmpl,
  hciEvtParseLeConnUpdateCmpl,
  hciEvtParseLeCreateConnCancelCmdCmpl,
  NULL,
  hciEvtParseReadRssiCmdCmpl,
  hciEvtParseReadChanMapCmdCmpl,
  hciEvtParseReadTxPwrLvlCmdCmpl,
  hciEvtParseReadRemoteVerInfoCmpl,
  hciEvtParseReadLeRemoteFeatCmpl,
  hciEvtParseLeLtkReqReplCmdCmpl,
  hciEvtParseLeLtkReqNegReplCmdCmpl,
  hciEvtParseEncKeyRefreshCmpl,
  hciEvtParseEncChange,
  hciEvtParseLeLtkReq,
  hciEvtParseVendorSpecCmdStatus,
  hciEvtParseVendorSpecCmdCmpl,
  hciEvtParseVendorSpec,
  hciEvtParseHwError,
  hciEvtParseLeAddDevToResListCmdCmpl,
  hciEvtParseLeRemDevFromResListCmdCmpl,
  hciEvtParseLeClearResListCmdCmpl,
  hciEvtParseLeReadPeerResAddrCmdCmpl,
  hciEvtParseLeReadLocalResAddrCmdCmpl,
  hciEvtParseLeSetAddrResEnableCmdCmpl,
  hciEvtParseLeEncryptCmdCmpl,
  hciEvtParseLeRandCmdCmpl,
  hciEvtParseRemConnParamRepCmdCmpl,
  hciEvtParseRemConnParamNegRepCmdCmpl,
  hciEvtParseReadDefDataLenCmdCmpl,
  hciEvtParseWriteDefDataLenCmdCmpl,
  hciEvtParseSetDataLenCmdCmpl,
  hciEvtParseReadMaxDataLenCmdCmpl,
  hciEvtParseRemConnParamReq,
  hciEvtParseDataLenChange,
  hciEvtParseReadPubKeyCmdCmpl,
  hciEvtParseGenDhKeyCmdCmpl,
  hciEvtParseWriteAuthTimeoutCmdCmpl,
  hciEvtParseAuthTimeoutExpiredEvt,
  hciEvtParseReadPhyCmdCmpl,
  hciEvtParseSetDefPhyCmdCmpl,
  hciEvtParsePhyUpdateCmpl,
  NULL,
  hciEvtParseLeScanTimeout,
  hciEvtParseLeAdvSetTerm,
  hciEvtParseLeScanReqRcvd,
  hciEvtParseLePerAdvSyncEst,
  NULL,
  hciEvtParseLePerAdvSyncLost,
  hciEvtParseLeChSelAlgo,
  hciEvtParseLeCmdCmpl,
  hciEvtParseLeCmdCmpl,
  hciEvtParseLeCmdCmpl,
  hciEvtParseLeCmdCmpl,
  hciEvtParseLeCmdCmpl,
  hciEvtParseReadLocalVerInfo
};

/* HCI event structure length table, indexed by internal callback event value */
static const uint8_t hciEvtCbackLen[] =
{
  sizeof(wsfMsgHdr_t),
  sizeof(hciLeConnCmplEvt_t),
  sizeof(hciLeConnCmplEvt_t),
  sizeof(hciDisconnectCmplEvt_t),
  sizeof(hciLeConnUpdateCmplEvt_t),
  sizeof(hciLeCreateConnCancelCmdCmplEvt_t),
  sizeof(hciLeAdvReportEvt_t),
  sizeof(hciReadRssiCmdCmplEvt_t),
  sizeof(hciReadChanMapCmdCmplEvt_t),
  sizeof(hciReadTxPwrLvlCmdCmplEvt_t),
  sizeof(hciReadRemoteVerInfoCmplEvt_t),
  sizeof(hciLeReadRemoteFeatCmplEvt_t),
  sizeof(hciLeLtkReqReplCmdCmplEvt_t),
  sizeof(hciLeLtkReqNegReplCmdCmplEvt_t),
  sizeof(hciEncKeyRefreshCmpl_t),
  sizeof(hciEncChangeEvt_t),
  sizeof(hciLeLtkReqEvt_t),
  sizeof(hciVendorSpecCmdStatusEvt_t),
  sizeof(hciVendorSpecCmdCmplEvt_t),
  sizeof(hciVendorSpecEvt_t),
  sizeof(hciHwErrorEvt_t),
  sizeof(hciLeAddDevToResListCmdCmplEvt_t),
  sizeof(hciLeRemDevFromResListCmdCmplEvt_t),
  sizeof(hciLeClearResListCmdCmplEvt_t),
  sizeof(hciLeReadPeerResAddrCmdCmplEvt_t),
  sizeof(hciLeReadLocalResAddrCmdCmplEvt_t),
  sizeof(hciLeSetAddrResEnableCmdCmplEvt_t),
  sizeof(hciLeEncryptCmdCmplEvt_t),
  sizeof(hciLeRandCmdCmplEvt_t),
  sizeof(hciLeRemConnParamRepEvt_t),
  sizeof(hciLeRemConnParamNegRepEvt_t),
  sizeof(hciLeReadDefDataLenEvt_t),
  sizeof(hciLeWriteDefDataLenEvt_t),
  sizeof(hciLeSetDataLenEvt_t),
  sizeof(hciLeReadMaxDataLenEvt_t),
  sizeof(hciLeRemConnParamReqEvt_t),
  sizeof(hciLeDataLenChangeEvt_t),
  sizeof(hciLeP256CmplEvt_t),
  sizeof(hciLeGenDhKeyEvt_t),
  sizeof(hciWriteAuthPayloadToCmdCmplEvt_t),
  sizeof(hciAuthPayloadToExpiredEvt_t),
  sizeof(hciLeReadPhyCmdCmplEvt_t),
  sizeof(hciLeSetDefPhyCmdCmplEvt_t),
  sizeof(hciLePhyUpdateEvt_t),
  sizeof(hciLeExtAdvReportEvt_t),
  sizeof(hciLeScanTimeoutEvt_t),
  sizeof(hciLeAdvSetTermEvt_t),
  sizeof(hciLeScanReqRcvdEvt_t),
  sizeof(hciLePerAdvSyncEstEvt_t),
  sizeof(hciLePerAdvReportEvt_t),
  sizeof(hciLePerAdvSyncLostEvt_t),
  sizeof(hciLeChSelAlgoEvt_t),
  sizeof(wsfMsgHdr_t),
  sizeof(wsfMsgHdr_t),
  sizeof(wsfMsgHdr_t),
  sizeof(wsfMsgHdr_t),
  sizeof(wsfMsgHdr_t),
  sizeof(hciReadLocalVerInfo_t)
};

/* Global event statistics. */
static hciEvtStats_t hciEvtStats = {0};


/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeConnCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leConnCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leConnCmpl.handle, p);
  BSTREAM_TO_UINT8(pMsg->leConnCmpl.role, p);
  BSTREAM_TO_UINT8(pMsg->leConnCmpl.addrType, p);
  BSTREAM_TO_BDA(pMsg->leConnCmpl.peerAddr, p);
  BSTREAM_TO_UINT16(pMsg->leConnCmpl.connInterval, p);
  BSTREAM_TO_UINT16(pMsg->leConnCmpl.connLatency, p);
  BSTREAM_TO_UINT16(pMsg->leConnCmpl.supTimeout, p);
  BSTREAM_TO_UINT8(pMsg->leConnCmpl.clockAccuracy, p);

  /* zero out unused fields */
  memset(pMsg->leConnCmpl.localRpa, 0, BDA_ADDR_LEN);
  memset(pMsg->leConnCmpl.peerRpa, 0, BDA_ADDR_LEN);

  pMsg->hdr.param = pMsg->leConnCmpl.handle;
  pMsg->hdr.status = pMsg->leConnCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeEnhancedConnCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leConnCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leConnCmpl.handle, p);
  BSTREAM_TO_UINT8(pMsg->leConnCmpl.role, p);
  BSTREAM_TO_UINT8(pMsg->leConnCmpl.addrType, p);
  BSTREAM_TO_BDA(pMsg->leConnCmpl.peerAddr, p);
  BSTREAM_TO_BDA(pMsg->leConnCmpl.localRpa, p);
  BSTREAM_TO_BDA(pMsg->leConnCmpl.peerRpa, p);
  BSTREAM_TO_UINT16(pMsg->leConnCmpl.connInterval, p);
  BSTREAM_TO_UINT16(pMsg->leConnCmpl.connLatency, p);
  BSTREAM_TO_UINT16(pMsg->leConnCmpl.supTimeout, p);
  BSTREAM_TO_UINT8(pMsg->leConnCmpl.clockAccuracy, p);

  pMsg->hdr.param = pMsg->leConnCmpl.handle;
  pMsg->hdr.status = pMsg->leConnCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseDisconnectCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->disconnectCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->disconnectCmpl.handle, p);
  BSTREAM_TO_UINT8(pMsg->disconnectCmpl.reason, p);

  pMsg->hdr.param = pMsg->disconnectCmpl.handle;
  pMsg->hdr.status = pMsg->disconnectCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeConnUpdateCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leConnUpdateCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leConnUpdateCmpl.handle, p);
  BSTREAM_TO_UINT16(pMsg->leConnUpdateCmpl.connInterval, p);
  BSTREAM_TO_UINT16(pMsg->leConnUpdateCmpl.connLatency, p);
  BSTREAM_TO_UINT16(pMsg->leConnUpdateCmpl.supTimeout, p);

  pMsg->hdr.param = pMsg->leConnUpdateCmpl.handle;
  pMsg->hdr.status = pMsg->leConnUpdateCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeCreateConnCancelCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leCreateConnCancelCmdCmpl.status, p);

  pMsg->hdr.status = pMsg->leCreateConnCancelCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadRssiCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->readRssiCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->readRssiCmdCmpl.handle, p);
  BSTREAM_TO_UINT8(pMsg->readRssiCmdCmpl.rssi, p);

  pMsg->hdr.param = pMsg->readRssiCmdCmpl.handle;
  pMsg->hdr.status = pMsg->readRssiCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadChanMapCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->readChanMapCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->readChanMapCmdCmpl.handle, p);

  memcpy(pMsg->readChanMapCmdCmpl.chanMap, p, HCI_CHAN_MAP_LEN);

  pMsg->hdr.param = pMsg->readChanMapCmdCmpl.handle;
  pMsg->hdr.status = pMsg->readChanMapCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadTxPwrLvlCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->readTxPwrLvlCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->readTxPwrLvlCmdCmpl.handle, p);
  BSTREAM_TO_INT8(pMsg->readTxPwrLvlCmdCmpl.pwrLvl, p);

  pMsg->hdr.param = pMsg->readTxPwrLvlCmdCmpl.handle;
  pMsg->hdr.status = pMsg->readTxPwrLvlCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadRemoteVerInfoCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->readRemoteVerInfoCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->readRemoteVerInfoCmpl.handle, p);
  BSTREAM_TO_UINT8(pMsg->readRemoteVerInfoCmpl.version, p);
  BSTREAM_TO_UINT16(pMsg->readRemoteVerInfoCmpl.mfrName, p);
  BSTREAM_TO_UINT16(pMsg->readRemoteVerInfoCmpl.subversion, p);

  pMsg->hdr.param = pMsg->readRemoteVerInfoCmpl.handle;
  pMsg->hdr.status = pMsg->readRemoteVerInfoCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadLeRemoteFeatCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leReadRemoteFeatCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leReadRemoteFeatCmpl.handle, p);
  memcpy(&pMsg->leReadRemoteFeatCmpl.features, p, HCI_FEAT_LEN);

  pMsg->hdr.param = pMsg->leReadRemoteFeatCmpl.handle;
  pMsg->hdr.status = pMsg->leReadRemoteFeatCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeLtkReqReplCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leLtkReqReplCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leLtkReqReplCmdCmpl.handle, p);

  pMsg->hdr.param = pMsg->leLtkReqReplCmdCmpl.handle;
  pMsg->hdr.status = pMsg->leLtkReqReplCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeLtkReqNegReplCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leLtkReqNegReplCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leLtkReqNegReplCmdCmpl.handle, p);

  pMsg->hdr.param = pMsg->leLtkReqNegReplCmdCmpl.handle;
  pMsg->hdr.status = pMsg->leLtkReqNegReplCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseEncKeyRefreshCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->encKeyRefreshCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->encKeyRefreshCmpl.handle, p);

  pMsg->hdr.param = pMsg->encKeyRefreshCmpl.handle;
  pMsg->hdr.status = pMsg->encKeyRefreshCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseEncChange(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->encChange.status, p);
  BSTREAM_TO_UINT16(pMsg->encChange.handle, p);
  BSTREAM_TO_UINT8(pMsg->encChange.enabled, p);

  pMsg->hdr.param = pMsg->encChange.handle;
  pMsg->hdr.status = pMsg->encChange.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeLtkReq(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT16(pMsg->leLtkReq.handle, p);

  memcpy(pMsg->leLtkReq.randNum, p, HCI_RAND_LEN);
  p += HCI_RAND_LEN;

  BSTREAM_TO_UINT16(pMsg->leLtkReq.encDiversifier, p);

  pMsg->hdr.param = pMsg->leLtkReq.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseVendorSpecCmdStatus(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT16(pMsg->vendorSpecCmdStatus.opcode, p);
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseVendorSpecCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  /* roll pointer back to opcode */
  p -= 2;

  BSTREAM_TO_UINT16(pMsg->vendorSpecCmdCmpl.opcode, p);
  BSTREAM_TO_UINT8(pMsg->hdr.status, p);
  BSTREAM_TO_UINT8(pMsg->vendorSpecCmdCmpl.param[0], p);
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseVendorSpec(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  memcpy(pMsg->vendorSpec.param, p, len);
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseHwError(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->hwError.code, p);
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeEncryptCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leEncryptCmdCmpl.status, p);
  memcpy(pMsg->leEncryptCmdCmpl.data, p, HCI_ENCRYPT_DATA_LEN);

  pMsg->hdr.status = pMsg->leEncryptCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeRandCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leRandCmdCmpl.status, p);
  memcpy(pMsg->leRandCmdCmpl.randNum, p, HCI_RAND_LEN);

  pMsg->hdr.status = pMsg->leRandCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeAddDevToResListCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leAddDevToResListCmdCmpl.status, p);

  pMsg->hdr.status = pMsg->leAddDevToResListCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeRemDevFromResListCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leRemDevFromResListCmdCmpl.status, p);

  pMsg->hdr.status = pMsg->leRemDevFromResListCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeClearResListCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leClearResListCmdCmpl.status, p);

  pMsg->hdr.status = pMsg->leClearResListCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeReadPeerResAddrCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leReadPeerResAddrCmdCmpl.status, p);
  BSTREAM_TO_BDA(pMsg->leReadPeerResAddrCmdCmpl.peerRpa, p);

  pMsg->hdr.status = pMsg->leReadPeerResAddrCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeReadLocalResAddrCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leReadLocalResAddrCmdCmpl.status, p);
  BSTREAM_TO_BDA(pMsg->leReadLocalResAddrCmdCmpl.localRpa, p);

  pMsg->hdr.status = pMsg->leReadLocalResAddrCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeSetAddrResEnableCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leSetAddrResEnableCmdCmpl.status, p);

  pMsg->hdr.status = pMsg->leSetAddrResEnableCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseRemConnParamRepCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leRemConnParamRepCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leRemConnParamRepCmdCmpl.handle, p);

  pMsg->hdr.status = pMsg->leRemConnParamRepCmdCmpl.status;
  pMsg->hdr.param = pMsg->leRemConnParamRepCmdCmpl.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseRemConnParamNegRepCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leRemConnParamNegRepCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leRemConnParamNegRepCmdCmpl.handle, p);

  pMsg->hdr.status = pMsg->leRemConnParamNegRepCmdCmpl.status;
  pMsg->hdr.param = pMsg->leRemConnParamNegRepCmdCmpl.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadDefDataLenCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leReadDefDataLenCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leReadDefDataLenCmdCmpl.suggestedMaxTxOctets, p);
  BSTREAM_TO_UINT16(pMsg->leReadDefDataLenCmdCmpl.suggestedMaxTxTime, p);

  pMsg->hdr.status = pMsg->leReadDefDataLenCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseWriteDefDataLenCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leWriteDefDataLenCmdCmpl.status, p);
  pMsg->hdr.status = pMsg->leWriteDefDataLenCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseSetDataLenCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leSetDataLenCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leSetDataLenCmdCmpl.handle, p);

  pMsg->hdr.status = pMsg->leSetDataLenCmdCmpl.status;
  pMsg->hdr.param = pMsg->leSetDataLenCmdCmpl.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadMaxDataLenCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leReadMaxDataLenCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leReadMaxDataLenCmdCmpl.supportedMaxTxOctets, p);
  BSTREAM_TO_UINT16(pMsg->leReadMaxDataLenCmdCmpl.supportedMaxTxTime, p);
  BSTREAM_TO_UINT16(pMsg->leReadMaxDataLenCmdCmpl.supportedMaxRxOctets, p);
  BSTREAM_TO_UINT16(pMsg->leReadMaxDataLenCmdCmpl.supportedMaxRxTime, p);

  pMsg->hdr.status = pMsg->leReadMaxDataLenCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseRemConnParamReq(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT16(pMsg->leRemConnParamReq.handle, p);
  BSTREAM_TO_UINT16(pMsg->leRemConnParamReq.intervalMin, p);
  BSTREAM_TO_UINT16(pMsg->leRemConnParamReq.intervalMax, p);
  BSTREAM_TO_UINT16(pMsg->leRemConnParamReq.latency, p);
  BSTREAM_TO_UINT16(pMsg->leRemConnParamReq.timeout, p);

  pMsg->hdr.param = pMsg->leRemConnParamReq.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseDataLenChange(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT16(pMsg->leDataLenChange.handle, p);
  BSTREAM_TO_UINT16(pMsg->leDataLenChange.maxTxOctets, p);
  BSTREAM_TO_UINT16(pMsg->leDataLenChange.maxTxTime, p);
  BSTREAM_TO_UINT16(pMsg->leDataLenChange.maxRxOctets, p);
  BSTREAM_TO_UINT16(pMsg->leDataLenChange.maxRxTime, p);

  pMsg->hdr.param = pMsg->leDataLenChange.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadPubKeyCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leP256.status, p);
  memcpy(pMsg->leP256.key, p, HCI_P256_KEY_LEN);

  pMsg->hdr.status = pMsg->leP256.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseGenDhKeyCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leGenDHKey.status, p);
  memcpy(pMsg->leGenDHKey.key, p, HCI_DH_KEY_LEN);

  pMsg->hdr.status = pMsg->leGenDHKey.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Process HCI write authenticated payload timeout command complete event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciEvtParseWriteAuthTimeoutCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->writeAuthPayloadToCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->writeAuthPayloadToCmdCmpl.handle, p);

  pMsg->hdr.status = pMsg->writeAuthPayloadToCmdCmpl.status;
  pMsg->hdr.param = pMsg->writeAuthPayloadToCmdCmpl.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Process HCI authenticated payload timeout expired event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciEvtParseAuthTimeoutExpiredEvt(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT16(pMsg->authPayloadToExpired.handle, p);

  pMsg->hdr.param = pMsg->authPayloadToExpired.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadPhyCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leReadPhyCmdCmpl.status, p);
  BSTREAM_TO_UINT16(pMsg->leReadPhyCmdCmpl.handle, p);
  BSTREAM_TO_UINT8(pMsg->leReadPhyCmdCmpl.txPhy, p);
  BSTREAM_TO_UINT8(pMsg->leReadPhyCmdCmpl.rxPhy, p);

  pMsg->hdr.status = pMsg->leReadPhyCmdCmpl.status;
  pMsg->hdr.param = pMsg->leReadPhyCmdCmpl.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseSetDefPhyCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leSetDefPhyCmdCmpl.status, p);

  pMsg->hdr.status = pMsg->leSetDefPhyCmdCmpl.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParsePhyUpdateCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->lePhyUpdate.status, p);
  BSTREAM_TO_UINT16(pMsg->lePhyUpdate.handle, p);
  BSTREAM_TO_UINT8(pMsg->lePhyUpdate.txPhy, p);
  BSTREAM_TO_UINT8(pMsg->lePhyUpdate.rxPhy, p);

  pMsg->hdr.status = pMsg->lePhyUpdate.status;
  pMsg->hdr.param = pMsg->lePhyUpdate.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Process an HCI LE advertising report.
 *
 *  \param  p    Buffer containing HCI event, points to start of parameters.
 *  \param  len  Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtProcessLeAdvReport(uint8_t *p, uint8_t len)
{
  hciLeAdvReportEvt_t *pMsg;
  uint8_t             i;

  /* get number of reports */
  BSTREAM_TO_UINT8(i, p);

  HCI_TRACE_INFO1("HCI Adv report, num reports: %d", i);

  /* sanity check num reports */
  if (i > HCI_MAX_REPORTS)
  {
    return;
  }

  /* allocate temp buffer that can hold max length adv/scan rsp data */
  if ((pMsg = WsfBufAlloc(sizeof(hciLeAdvReportEvt_t) + HCI_ADV_DATA_LEN)) != NULL)
  {
    /* parse each report and execute callback */
    while (i-- > 0)
    {
      BSTREAM_TO_UINT8(pMsg->eventType, p);
      BSTREAM_TO_UINT8(pMsg->addrType, p);
      BSTREAM_TO_BDA(pMsg->addr, p);
      BSTREAM_TO_UINT8(pMsg->len, p);

      HCI_TRACE_INFO1("HCI Adv report, data len: %d", pMsg->len);

      /* sanity check on report length; quit if invalid */
      if (pMsg->len > HCI_ADV_DATA_LEN)
      {
        HCI_TRACE_WARN0("Invalid adv report data len");
        break;
      }

      /* Copy data to space after end of report struct */
      pMsg->pData = (uint8_t *) (pMsg + 1);
      memcpy(pMsg->pData, p, pMsg->len);
      p += pMsg->len;

      BSTREAM_TO_UINT8(pMsg->rssi, p);

      /* zero out unused fields */
      pMsg->directAddrType = 0;
      memset(pMsg->directAddr, 0, BDA_ADDR_LEN);

      /* initialize message header */
      pMsg->hdr.param = 0;
      pMsg->hdr.event = HCI_LE_ADV_REPORT_CBACK_EVT;
      pMsg->hdr.status = HCI_SUCCESS;

      /* execute callback */
      (*hciCb.evtCback)((hciEvt_t *) pMsg);
    }

    /* free buffer */
    WsfBufFree(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process an HCI LE extended advertising report.
 *
 *  \param  p    Buffer containing HCI event, points to start of parameters.
 *  \param  len  Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtProcessLeExtAdvReport(uint8_t *p, uint8_t len)
{
  hciLeExtAdvReportEvt_t *pMsg;
  uint8_t                i;
  uint8_t                *ptr;
  uint8_t                maxLen;
  uint8_t                dataLen;

  /* get number of reports without advancing pointer */
  i = *p;

  HCI_TRACE_INFO1("HCI Ext Adv report, num reports: %d", i);

  /* sanity check num reports */
  if (i > HCI_MAX_REPORTS)
  {
    return;
  }

  ptr = p + 1;
  maxLen = 0;

  /* find out max length ext adv/scan rsp data */
  while (i-- > 0)
  {
    ptr += HCI_EXT_ADV_RPT_DATA_LEN_OFFSET;
    BSTREAM_TO_UINT8(dataLen, ptr);
    ptr += dataLen;

    /* if len greater than max len seen so far */
    if (dataLen > maxLen)
    {
      /* update max len */
      maxLen = dataLen;
    }
  }

  /* allocate temp buffer that can hold max length ext adv/scan rsp data */
  if ((pMsg = WsfBufAlloc(sizeof(hciLeExtAdvReportEvt_t) + maxLen)) != NULL)
  {
    /* get number of reports */
    BSTREAM_TO_UINT8(i, p);

    /* parse each report and execute callback */
    while (i-- > 0)
    {
      BSTREAM_TO_UINT16(pMsg->eventType, p);
      BSTREAM_TO_UINT8(pMsg->addrType, p);
      BSTREAM_TO_BDA(pMsg->addr, p);
      BSTREAM_TO_UINT8(pMsg->priPhy, p);
      BSTREAM_TO_UINT8(pMsg->secPhy, p);
      BSTREAM_TO_UINT8(pMsg->advSid, p);
      BSTREAM_TO_INT8(pMsg->txPower, p);
      BSTREAM_TO_INT8(pMsg->rssi, p);
      BSTREAM_TO_UINT16(pMsg->perAdvInter, p);
      BSTREAM_TO_UINT8(pMsg->directAddrType, p);
      BSTREAM_TO_BDA(pMsg->directAddr, p);
      BSTREAM_TO_UINT8(pMsg->len, p);

      HCI_TRACE_INFO1("HCI Ext Adv report, data len: %d", pMsg->len);

      /* sanity check on report length; quit if invalid */
      if (pMsg->len > HCI_EXT_ADV_RPT_DATA_LEN)
      {
        HCI_TRACE_WARN0("Invalid ext adv report data len");
        break;
      }

      /* Copy data to space after end of report struct */
      pMsg->pData = (uint8_t *)(pMsg + 1);
      memcpy(pMsg->pData, p, pMsg->len);
      p += pMsg->len;

      /* initialize message header */
      pMsg->hdr.param = 0;
      pMsg->hdr.event = HCI_LE_EXT_ADV_REPORT_CBACK_EVT;
      pMsg->hdr.status = HCI_SUCCESS;

      /* execute callback */
      (*hciCb.evtCback)((hciEvt_t *)pMsg);
    }

    /* free buffer */
    WsfBufFree(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI LE scan timeout event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeScanTimeout(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  /* empty */
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI LE advertising set terminated event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeAdvSetTerm(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leAdvSetTerm.status, p);
  BSTREAM_TO_UINT8(pMsg->leAdvSetTerm.advHandle, p);
  BSTREAM_TO_UINT16(pMsg->leAdvSetTerm.handle, p);
  BSTREAM_TO_UINT8(pMsg->leAdvSetTerm.numComplEvts, p);

  pMsg->hdr.status = pMsg->leAdvSetTerm.status;
  pMsg->hdr.param = pMsg->leAdvSetTerm.advHandle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI LE scan request received event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeScanReqRcvd(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->leScanReqRcvd.advHandle, p);
  BSTREAM_TO_UINT8(pMsg->leScanReqRcvd.scanAddrType, p);
  BSTREAM_TO_BDA(pMsg->leScanReqRcvd.scanAddr, p);

  pMsg->hdr.param = pMsg->leScanReqRcvd.advHandle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI LE periodic advertising sync established event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLePerAdvSyncEst(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->lePerAdvSyncEst.status, p);
  BSTREAM_TO_UINT16(pMsg->lePerAdvSyncEst.syncHandle, p);
  BSTREAM_TO_UINT8(pMsg->lePerAdvSyncEst.advSid, p);
  BSTREAM_TO_UINT8(pMsg->lePerAdvSyncEst.advAddrType, p);
  BSTREAM_TO_BDA(pMsg->lePerAdvSyncEst.advAddr, p);
  BSTREAM_TO_UINT8(pMsg->lePerAdvSyncEst.advPhy, p);
  BSTREAM_TO_UINT16(pMsg->lePerAdvSyncEst.perAdvInterval, p);
  BSTREAM_TO_UINT8(pMsg->lePerAdvSyncEst.clockAccuracy, p);

  pMsg->hdr.status = pMsg->lePerAdvSyncEst.status;
  pMsg->hdr.param = pMsg->lePerAdvSyncEst.syncHandle;
}

/*************************************************************************************************/
/*!
 *  \brief  Process an HCI LE periodic advertising report event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtProcessLePerAdvReport(uint8_t *p, uint8_t len)
{
  hciLePerAdvReportEvt_t *pMsg;
  uint8_t                dataLen;

  HCI_TRACE_INFO0("HCI Per Adv report");

  /* get report data length */
  dataLen = p[HCI_PER_ADV_RPT_DATA_LEN_OFFSET];

  /* sanity check on report length; quit if invalid */
  if (dataLen > HCI_PER_ADV_RPT_DATA_LEN)
  {
    HCI_TRACE_WARN1("Invalid per adv report data len: %d", dataLen);
    return;
  }

  /* allocate temp buffer that can hold max length periodic adv report data */
  if ((pMsg = WsfBufAlloc(sizeof(hciLePerAdvReportEvt_t) + dataLen)) != NULL)
  {
    /* parse report and execute callback */
    BSTREAM_TO_UINT16(pMsg->syncHandle, p);
    BSTREAM_TO_UINT8(pMsg->txPower, p);
    BSTREAM_TO_UINT8(pMsg->rssi, p);
    BSTREAM_TO_UINT8(pMsg->unused, p);
    BSTREAM_TO_UINT8(pMsg->status, p);
    BSTREAM_TO_UINT8(pMsg->len, p);

    HCI_TRACE_INFO1("HCI Per Adv report, data len: %d", pMsg->len);

    /* Copy data to space after end of report struct */
    pMsg->pData = (uint8_t *)(pMsg + 1);
    memcpy(pMsg->pData, p, pMsg->len);

    /* initialize message header */
    pMsg->hdr.param = pMsg->syncHandle;
    pMsg->hdr.event = HCI_LE_PER_ADV_REPORT_CBACK_EVT;
    pMsg->hdr.status = HCI_SUCCESS;

    /* execute callback */
    (*hciCb.evtCback)((hciEvt_t *)pMsg);

    /* free buffer */
    WsfBufFree(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI LE complete event containing a one byte status.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeCmdCmpl(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->hdr.status, p);
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI LE periodic advertising synch lost event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLePerAdvSyncLost(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT16(pMsg->lePerAdvSyncLost.syncHandle, p);

  pMsg->hdr.param = pMsg->lePerAdvSyncLost.syncHandle;
}

/*************************************************************************************************/
/*!
 *  \brief  Parse an HCI LE channel selection algorithm event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Pointer to input HCI event parameter byte stream.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseLeChSelAlgo(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT16(pMsg->leChSelAlgo.handle, p);
  BSTREAM_TO_UINT8(pMsg->leChSelAlgo.chSelAlgo, p);

  pMsg->hdr.param = pMsg->leChSelAlgo.handle;
}

/*************************************************************************************************/
/*!
 *  \brief  Process an HCI LE direct advertising report.
 *
 *  \param  p    Buffer containing HCI event, points to start of parameters.
 *  \param  len  Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtProcessLeDirectAdvReport(uint8_t *p, uint8_t len)
{
  hciLeAdvReportEvt_t *pMsg;
  uint8_t             i;

  /* get number of reports */
  BSTREAM_TO_UINT8(i, p);

  HCI_TRACE_INFO1("HCI Adv report, num reports: %d", i);

  /* sanity check num reports */
  if (i > HCI_MAX_REPORTS)
  {
    return;
  }

  /* allocate temp buffer that can hold max length adv/scan rsp data */
  if ((pMsg = WsfBufAlloc(sizeof(hciLeAdvReportEvt_t))) != NULL)
  {
    /* parse each report and execute callback */
    while (i-- > 0)
    {
      BSTREAM_TO_UINT8(pMsg->eventType, p);
      BSTREAM_TO_UINT8(pMsg->addrType, p);
      BSTREAM_TO_BDA(pMsg->addr, p);
      BSTREAM_TO_UINT8(pMsg->directAddrType, p);
      BSTREAM_TO_BDA(pMsg->directAddr, p);
      BSTREAM_TO_UINT8(pMsg->rssi, p);

      /* zero out unused fields */
      pMsg->len = 0;
      pMsg->pData = NULL;

      /* initialize message header */
      pMsg->hdr.param = 0;
      pMsg->hdr.event = HCI_LE_ADV_REPORT_CBACK_EVT;
      pMsg->hdr.status = HCI_SUCCESS;

      /* execute callback */
      (*hciCb.evtCback)((hciEvt_t *) pMsg);
    }

    /* free buffer */
    WsfBufFree(pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process an HCI Read Local Version Information event.
 *
 *  \param  pMsg    Pointer to output event message structure.
 *  \param  p       Buffer containing HCI event, points to start of parameters.
 *  \param  len     Parameter byte stream length.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void hciEvtParseReadLocalVerInfo(hciEvt_t *pMsg, uint8_t *p, uint8_t len)
{
  BSTREAM_TO_UINT8(pMsg->readLocalVerInfo.status, p);
  BSTREAM_TO_UINT8(pMsg->readLocalVerInfo.hciVersion, p);
  BSTREAM_TO_UINT16(pMsg->readLocalVerInfo.hciRevision, p);
  BSTREAM_TO_UINT8(pMsg->readLocalVerInfo.lmpVersion, p);
  BSTREAM_TO_UINT16(pMsg->readLocalVerInfo.manufacturerName, p);
  BSTREAM_TO_UINT16(pMsg->readLocalVerInfo.lmpSubversion, p);

  /* initialize message header */
  pMsg->hdr.status = pMsg->readLocalVerInfo.status;
}

/*************************************************************************************************/
/*!
 *  \brief  Process HCI command status event with failure status.
 *
 *  \param  status  Event status.
 *  \param  opcode  Command opcode for this event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciEvtCmdStatusFailure(uint8_t status, uint16_t opcode)
{
#if 0
  handle these events:
  translate the command status event into other appropriate event
  HCI_OPCODE_DISCONNECT
  HCI_OPCODE_LE_CREATE_CONN
  HCI_OPCODE_LE_CONN_UPDATE
  HCI_OPCODE_LE_READ_REMOTE_FEAT
  HCI_OPCODE_LE_START_ENCRYPTION
  HCI_OPCODE_READ_REMOTE_VER_INFO
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Process HCI command status event.
 *
 *  \param  p    Buffer containing HCI event, points to start of parameters.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciEvtProcessCmdStatus(uint8_t *p)
{
  uint8_t   status;
  uint8_t   numPkts;
  uint16_t  opcode;

  BSTREAM_TO_UINT8(status, p);
  BSTREAM_TO_UINT8(numPkts, p);
  BSTREAM_TO_UINT16(opcode, p);

  if (status != HCI_SUCCESS)  /* optional: or vendor specific */
  {
    hciEvtCmdStatusFailure(status, opcode);
  }

  /* optional:  handle vendor-specific command status event */

  hciCmdRecvCmpl(numPkts);
}

/*************************************************************************************************/
/*!
 *  \brief  Process HCI command complete event.
 *
 *  \param  p    Buffer containing command complete event, points to start of parameters.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciEvtProcessCmdCmpl(uint8_t *p, uint8_t len)
{
  uint8_t       numPkts;
  uint16_t      opcode;
  hciEvt_t      *pMsg;
  uint8_t       cbackEvt = 0;
  hciEvtCback_t cback = hciCb.evtCback;

  BSTREAM_TO_UINT8(numPkts, p);
  BSTREAM_TO_UINT16(opcode, p);

  /* convert opcode to internal event code and perform special handling */
  switch (opcode)
  {
  case HCI_OPCODE_LE_CREATE_CONN_CANCEL:
    cbackEvt = HCI_LE_CREATE_CONN_CANCEL_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_ENCRYPT:
    cbackEvt = HCI_LE_ENCRYPT_CMD_CMPL_CBACK_EVT;
    cback = hciCb.secCback;
    break;

  case HCI_OPCODE_LE_REM_CONN_PARAM_REP:
    cbackEvt = HCI_LE_REM_CONN_PARAM_REP_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_REM_CONN_PARAM_NEG_REP:
    cbackEvt = HCI_LE_REM_CONN_PARAM_NEG_REP_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_READ_DEF_DATA_LEN:
    cbackEvt = HCI_LE_READ_DEF_DATA_LEN_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_WRITE_DEF_DATA_LEN:
    cbackEvt = HCI_LE_WRITE_DEF_DATA_LEN_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_SET_DATA_LEN:
    cbackEvt = HCI_LE_SET_DATA_LEN_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_READ_MAX_DATA_LEN:
    cbackEvt = HCI_LE_READ_MAX_DATA_LEN_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_LTK_REQ_REPL:
    cbackEvt = HCI_LE_LTK_REQ_REPL_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_LTK_REQ_NEG_REPL:
    cbackEvt = HCI_LE_LTK_REQ_NEG_REPL_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_RAND:
    cbackEvt = HCI_LE_RAND_CMD_CMPL_CBACK_EVT;
    cback = hciCb.secCback;
    break;

  case HCI_OPCODE_LE_READ_CHAN_MAP:
    cbackEvt = HCI_LE_READ_CHAN_MAP_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_READ_RSSI:
    cbackEvt = HCI_READ_RSSI_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_READ_TX_PWR_LVL:
    cbackEvt = HCI_READ_TX_PWR_LVL_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_ADD_DEV_RES_LIST:
    cbackEvt = HCI_LE_ADD_DEV_TO_RES_LIST_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_REMOVE_DEV_RES_LIST:
    cbackEvt = HCI_LE_REM_DEV_FROM_RES_LIST_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_CLEAR_RES_LIST:
    cbackEvt = HCI_LE_CLEAR_RES_LIST_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_READ_PEER_RES_ADDR:
    cbackEvt = HCI_LE_READ_PEER_RES_ADDR_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_READ_LOCAL_RES_ADDR:
    cbackEvt = HCI_LE_READ_LOCAL_RES_ADDR_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_SET_ADDR_RES_ENABLE:
    cbackEvt = HCI_LE_SET_ADDR_RES_ENABLE_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_WRITE_AUTH_PAYLOAD_TO:
    cbackEvt = HCI_WRITE_AUTH_PAYLOAD_TO_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_READ_PHY:
    cbackEvt = HCI_LE_READ_PHY_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_SET_DEF_PHY:
    cbackEvt = HCI_LE_SET_DEF_PHY_CMD_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_SET_SCAN_ENABLE:
    cbackEvt = HCI_LE_SCAN_ENABLE_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_SET_ADV_ENABLE:
    cbackEvt = HCI_LE_ADV_ENABLE_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_SET_EXT_SCAN_ENABLE:
    cbackEvt = HCI_LE_EXT_SCAN_ENABLE_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_SET_EXT_ADV_ENABLE:
    cbackEvt = HCI_LE_EXT_ADV_ENABLE_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_LE_SET_PER_ADV_ENABLE:
    cbackEvt = HCI_LE_PER_ADV_ENABLE_CMPL_CBACK_EVT;
    break;

  case HCI_OPCODE_READ_LOCAL_VER_INFO:
    cbackEvt = HCI_READ_LOCAL_VER_INFO_CMPL_CBACK_EVT;
    break;

  default:
    /* test for vendor specific command completion OGF. */
    if (HCI_OGF(opcode) == HCI_OGF_VENDOR_SPEC)
    {
      cbackEvt = hciCoreVsCmdCmplRcvd(opcode, p, len);
    }
    break;
  }

  /* if callback is executed for this event */
  if (cbackEvt != 0)
  {
    /* allocate temp buffer */
    if ((pMsg = WsfBufAlloc(hciEvtCbackLen[cbackEvt])) != NULL)
    {
      /* initialize message header */
      pMsg->hdr.param = 0;
      pMsg->hdr.event = cbackEvt;
      pMsg->hdr.status = HCI_SUCCESS;

      /* execute parsing function for the event */
      (*hciEvtParseFcnTbl[cbackEvt])(pMsg, p, len);

      /* execute callback */
      (*cback)(pMsg);

      /* free buffer */
      WsfBufFree(pMsg);
    }
  }

  hciCmdRecvCmpl(numPkts);
}

/*************************************************************************************************/
/*!
 *  \brief  Process received HCI events.
 *
 *  \param  pEvt    Buffer containing HCI event.
 *
 *  \return None.
 */
/*************************************************************************************************/
void hciEvtProcessMsg(uint8_t *pEvt)
{
  uint8_t   evt;
  uint8_t   subEvt;
  uint8_t   len;
  uint8_t   cbackEvt = 0;
  hciEvt_t  *pMsg;
  uint16_t  handle;
  hciEvtCback_t cback = hciCb.evtCback;

  /* parse HCI event header */
  BSTREAM_TO_UINT8(evt, pEvt);
  BSTREAM_TO_UINT8(len, pEvt);

  /* convert hci event code to internal event code and perform special handling */
  switch (evt)
  {
    case HCI_CMD_STATUS_EVT:
      /* special handling for command status event */
      hciEvtStats.numCmdStatusEvt++;
      hciEvtProcessCmdStatus(pEvt);
      break;

    case HCI_CMD_CMPL_EVT:
      /* special handling for command complete event */
      hciEvtStats.numCmdCmplEvt++;
      hciEvtProcessCmdCmpl(pEvt, len);
#if defined(HCI_CONN_CANCEL_WORKAROUND) && (HCI_CONN_CANCEL_WORKAROUND == TRUE)
      /* workaround for controllers that don't send an LE connection complete event
       * after a connection cancel command
       */
      {
        uint16_t opcode;
        BYTES_TO_UINT16(opcode, (pEvt + 1));
        if (opcode == HCI_OPCODE_LE_CREATE_CONN_CANCEL)
        {
          cbackEvt = HCI_LE_CONN_CMPL_CBACK_EVT;
        }
      }
#endif
      break;

    case HCI_NUM_CMPL_PKTS_EVT:
      /* handled internally by hci */
      hciCoreNumCmplPkts(pEvt);
      hciEvtStats.numCmplPktsEvt++;
      break;

    case HCI_LE_META_EVT:
      BSTREAM_TO_UINT8(subEvt, pEvt);
      hciEvtStats.numLeMetaEvt++;
      switch (subEvt)
      {
        case HCI_LE_CONN_CMPL_EVT:
          /* if connection created successfully */
          if (*pEvt == HCI_SUCCESS)
          {
            BYTES_TO_UINT16(handle, (pEvt + 1));
            hciCoreConnOpen(handle);
          }
          cbackEvt = HCI_LE_CONN_CMPL_CBACK_EVT;
          break;

        case HCI_LE_ADV_REPORT_EVT:
          /* special case for advertising report */
          hciEvtProcessLeAdvReport(pEvt, len);
          break;

        case HCI_LE_CONN_UPDATE_CMPL_EVT:
          cbackEvt = HCI_LE_CONN_UPDATE_CMPL_CBACK_EVT;
          break;

        case HCI_LE_READ_REMOTE_FEAT_CMPL_EVT:
          cbackEvt = HCI_LE_READ_REMOTE_FEAT_CMPL_CBACK_EVT;
          break;

        case HCI_LE_LTK_REQ_EVT:
          cbackEvt = HCI_LE_LTK_REQ_CBACK_EVT;
          break;

        case HCI_LE_ENHANCED_CONN_CMPL_EVT:
          /* if connection created successfully */
          if (*pEvt == HCI_SUCCESS)
          {
            BYTES_TO_UINT16(handle, (pEvt + 1));
            hciCoreConnOpen(handle);
          }
          cbackEvt = HCI_LE_ENHANCED_CONN_CMPL_CBACK_EVT;
          break;

        case HCI_LE_DIRECT_ADV_REPORT_EVT:
          /* special case for direct advertising report */
          hciEvtProcessLeDirectAdvReport(pEvt, len);
          break;

        case HCI_LE_REM_CONN_PARAM_REQ_EVT:
          cbackEvt = HCI_LE_REM_CONN_PARAM_REQ_CBACK_EVT;
          break;

        case HCI_LE_DATA_LEN_CHANGE_EVT:
          cbackEvt = HCI_LE_DATA_LEN_CHANGE_CBACK_EVT;
          break;

        case HCI_LE_READ_LOCAL_P256_PUB_KEY_CMPL_EVT:
          cback = hciCb.secCback;
          cbackEvt = HCI_LE_READ_LOCAL_P256_PUB_KEY_CMPL_CBACK_EVT;
          break;

        case HCI_LE_GENERATE_DHKEY_CMPL_EVT:
          cback = hciCb.secCback;
          cbackEvt = HCI_LE_GENERATE_DHKEY_CMPL_CBACK_EVT;
          break;

        case HCI_LE_PHY_UPDATE_CMPL_EVT:
          cbackEvt = HCI_LE_PHY_UPDATE_CMPL_CBACK_EVT;
          break;

        case HCI_LE_EXT_ADV_REPORT_EVT:
          /* special case for extended advertising report */
          hciEvtProcessLeExtAdvReport(pEvt, len);
          break;

        case HCI_LE_SCAN_TIMEOUT_EVT:
          cbackEvt = HCI_LE_SCAN_TIMEOUT_CBACK_EVT;
          break;

        case HCI_LE_ADV_SET_TERM_EVT:
          cbackEvt = HCI_LE_ADV_SET_TERM_CBACK_EVT;
          break;

        case HCI_LE_SCAN_REQ_RCVD_EVT:
          cbackEvt = HCI_LE_SCAN_REQ_RCVD_CBACK_EVT;
          break;

        case HCI_LE_PER_ADV_SYNC_EST_EVT:
          cbackEvt = HCI_LE_PER_ADV_SYNC_EST_CBACK_EVT;
          break;

        case HCI_LE_PER_ADV_REPORT_EVT:
          /* special case for periodic advertising report */
          hciEvtProcessLePerAdvReport(pEvt, len);
          break;

        case HCI_LE_PER_ADV_SYNC_LOST_EVT:
          cbackEvt = HCI_LE_PER_ADV_SYNC_LOST_CBACK_EVT;
          break;

        default:
          break;
      }
      break;

    case HCI_DISCONNECT_CMPL_EVT:
      hciEvtStats.numDiscCmplEvt++;
      cbackEvt = HCI_DISCONNECT_CMPL_CBACK_EVT;
      break;

    case HCI_ENC_CHANGE_EVT:
      hciEvtStats.numEncChangeEvt++;
      cbackEvt = HCI_ENC_CHANGE_CBACK_EVT;
      break;

    case HCI_READ_REMOTE_VER_INFO_CMPL_EVT:
      hciEvtStats.numReadRemoteVerInfoCmpEvt++;
      cbackEvt = HCI_READ_REMOTE_VER_INFO_CMPL_CBACK_EVT;
      break;

    case HCI_ENC_KEY_REFRESH_CMPL_EVT:
      hciEvtStats.numEncKeyRefreshCmplEvt++;
      cbackEvt = HCI_ENC_KEY_REFRESH_CMPL_CBACK_EVT;
      break;

    case HCI_DATA_BUF_OVERFLOW_EVT:
      /* handled internally by hci */
      /* optional */
      hciEvtStats.numDataBufOverflowEvt++;
      break;

    case HCI_HW_ERROR_EVT:
      hciEvtStats.numHwErrorEvt++;
      cbackEvt = HCI_HW_ERROR_CBACK_EVT;
      break;

    case HCI_AUTH_PAYLOAD_TIMEOUT_EVT:
      hciEvtStats.numAuthToEvt++;
      cbackEvt = HCI_AUTH_PAYLOAD_TO_EXPIRED_CBACK_EVT;
      break;

    case HCI_VENDOR_SPEC_EVT:
      /* special case for vendor specific event */
      /* optional */
#if defined (HCI_NONSTANDARD_VS_CMPL) && (HCI_NONSTANDARD_VS_CMPL == TRUE)
      /* for nonstandard controllers that send a vendor-specific event instead
       * of a command complete event
       */
      hciCmdRecvCmpl(1);
#endif
      hciEvtStats.numVendorSpecEvt++;
      cbackEvt = HCI_VENDOR_SPEC_CBACK_EVT;

#ifdef WSF_DETOKEN_TRACE
      if (WsfDetokenProcessHciEvent(len, pEvt))
      {
        cbackEvt = 0;
      }
#endif /* LL_DETOKEN_TRACE */
      break;

    default:
      break;
  }

  /* if callback is executed for this event */
  if (cbackEvt != 0)
  {
    /* allocate temp buffer */
    if ((pMsg = WsfBufAlloc(hciEvtCbackLen[cbackEvt])) != NULL)
    {
      /* initialize message header */
      pMsg->hdr.param = 0;
      pMsg->hdr.event = cbackEvt;
      pMsg->hdr.status = HCI_SUCCESS;

      /* execute parsing function for the event */
      (*hciEvtParseFcnTbl[cbackEvt])(pMsg, pEvt, len);

      /* execute callback */
      (*cback)(pMsg);

      /* free buffer */
      WsfBufFree(pMsg);
    }

    /* execute core procedure for connection close after callback */
    if (cbackEvt == HCI_DISCONNECT_CMPL_CBACK_EVT)
    {
      BYTES_TO_UINT16(handle, (pEvt + 1));
      hciCoreConnClose(handle);
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Get event statistics.
 *
 *  \return Event statistics.
 */
/*************************************************************************************************/
hciEvtStats_t *hciEvtGetStats(void)
{
  return &hciEvtStats;
}
