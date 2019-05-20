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
 *  \brief LL HCI event module implementation file.
 */
/*************************************************************************************************/

#include "lhci_int.h"
#include "chci_api.h"
#include "hci_defs.h"
#include "wsf_msg.h"
#include "util/bstream.h"
#include <string.h>

/*************************************************************************************************/
/*!
 *  \brief  Pack a number of completed packets event packet.
 *
 *  \param  pBuf        Packed packet buffer.
 *  \param  connHandle  Connection handle.
 *  \param  numPkts     Number of packets.
 *
 *  \return Packet length.
 */
/*************************************************************************************************/
static uint8_t lhciPackNumCompPktsEvt(uint8_t *pBuf, uint16_t connHandle, uint8_t numPkts)
{
  const uint8_t len = HCI_LEN_NUM_CMPL_PKTS;
  uint16_t numPkts_u16 = numPkts;

  UINT8_TO_BSTREAM (pBuf, 1);
  UINT16_TO_BSTREAM(pBuf, connHandle);
  UINT16_TO_BSTREAM(pBuf, numPkts_u16);

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  Pack a connection complete event packet.
 *
 *  \param  pBuf        Packed packet buffer.
 *  \param  pEvt        Connection indication data.
 *
 *  \return Packet length.
 */
/*************************************************************************************************/
static uint8_t lhciPackConnCompleteEvt(uint8_t *pBuf, const LlConnInd_t *pEvt)
{
  const uint8_t len = HCI_LEN_LE_CONN_CMPL;

  UINT8_TO_BSTREAM (pBuf, HCI_LE_CONN_CMPL_EVT);
  UINT8_TO_BSTREAM (pBuf, pEvt->status);
  UINT16_TO_BSTREAM(pBuf, pEvt->handle);
  UINT8_TO_BSTREAM (pBuf, pEvt->role);
  switch (pEvt->addrType)
  {
    default:
    case HCI_ADDR_TYPE_PUBLIC:
    case HCI_ADDR_TYPE_RANDOM:
      UINT8_TO_BSTREAM (pBuf, pEvt->addrType);
      BDA_TO_BSTREAM   (pBuf, pEvt->peerAddr);
      break;
    case HCI_ADDR_TYPE_PUBLIC_IDENTITY:
    case HCI_ADDR_TYPE_RANDOM_IDENTITY:
      UINT8_TO_BSTREAM (pBuf, HCI_ADDR_TYPE_RANDOM);
      BDA_TO_BSTREAM   (pBuf, pEvt->peerRpa);
      break;
  }
  UINT16_TO_BSTREAM(pBuf, pEvt->connInterval);
  UINT16_TO_BSTREAM(pBuf, pEvt->connLatency);
  UINT16_TO_BSTREAM(pBuf, pEvt->supTimeout);
  UINT8_TO_BSTREAM (pBuf, pEvt->clockAccuracy);

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  Pack a disconnect complete event packet.
 *
 *  \param  pBuf        Packed packet buffer.
 *  \param  pEvt        Disconnect indication data.
 *
 *  \return Packet length.
 */
/*************************************************************************************************/
static uint8_t lhciPackDiscCompleteEvt(uint8_t *pBuf, const LlDisconnectInd_t *pEvt)
{
  const uint8_t len = HCI_LEN_DISCONNECT_CMPL;

  UINT8_TO_BSTREAM (pBuf, pEvt->status);
  UINT16_TO_BSTREAM(pBuf, pEvt->handle);
  UINT8_TO_BSTREAM (pBuf, pEvt->reason);

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  Pack a connection update complete event packet.
 *
 *  \param  pBuf        Packed packet buffer.
 *  \param  pEvt        Connection update indication data.
 *
 *  \return Packet length.
 */
/*************************************************************************************************/
static uint8_t lhciPackConnUpdateCompleteEvt(uint8_t *pBuf, const LlConnUpdateInd_t *pEvt)
{
  const uint8_t len = HCI_LEN_LE_CONN_UPDATE_CMPL;

  UINT8_TO_BSTREAM (pBuf, HCI_LE_CONN_UPDATE_CMPL_EVT);
  UINT8_TO_BSTREAM (pBuf, pEvt->status);
  UINT16_TO_BSTREAM(pBuf, pEvt->handle);
  UINT16_TO_BSTREAM(pBuf, pEvt->connInterval);
  UINT16_TO_BSTREAM(pBuf, pEvt->connLatency);
  UINT16_TO_BSTREAM(pBuf, pEvt->supTimeout);

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  Pack a connection update complete event packet.
 *
 *  \param  pBuf        Packed packet buffer.
 *  \param  pEvt        Connection update indication data.
 *
 *  \return Packet length.
 */
/*************************************************************************************************/
static uint8_t lhciPackRemoteConnParamEvt(uint8_t *pBuf, const LlRemConnParamInd_t *pEvt)
{
  const uint8_t len = HCI_LEN_LE_REM_CONN_PARAM_REQ;

  UINT8_TO_BSTREAM (pBuf, HCI_LE_REM_CONN_PARAM_REQ_EVT);
  UINT16_TO_BSTREAM(pBuf, pEvt->handle);
  UINT16_TO_BSTREAM(pBuf, pEvt->connIntervalMin);
  UINT16_TO_BSTREAM(pBuf, pEvt->connIntervalMax);
  UINT16_TO_BSTREAM(pBuf, pEvt->connLatency);
  UINT16_TO_BSTREAM(pBuf, pEvt->supTimeout);

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  Pack a read remote used feature complete event packet.
 *
 *  \param  pBuf        Packed packet buffer.
 *  \param  pEvt        Read remote feature confirm data.
 *
 *  \return Packet length.
 */
/*************************************************************************************************/
static uint8_t lhciPackReadRemoteUsedFeatCompleteEvt(uint8_t *pBuf, const LlReadRemoteFeatCnf_t *pEvt)
{
  const uint8_t len = HCI_LEN_LE_READ_REMOTE_FEAT_CMPL;

  UINT8_TO_BSTREAM (pBuf, HCI_LE_READ_REMOTE_FEAT_CMPL_EVT);
  UINT8_TO_BSTREAM (pBuf, pEvt->status);
  UINT16_TO_BSTREAM(pBuf, pEvt->handle);
  memcpy(pBuf, pEvt->features, sizeof(pEvt->features));

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  Pack a read remote version info event packet.
 *
 *  \param  pBuf        Packed packet buffer.
 *  \param  pEvt        Read remote version info indication data.
 *
 *  \return Packet length.
 */
/*************************************************************************************************/
static uint8_t lhciPackReadRemoteVerInfoCompleteEvt(uint8_t *pBuf, const LlReadRemoteVerInfoCnf_t *pEvt)
{
  const uint8_t len = HCI_LEN_READ_REMOTE_VER_INFO_CMPL;

  UINT8_TO_BSTREAM (pBuf, pEvt->status);
  UINT16_TO_BSTREAM(pBuf, pEvt->handle);
  UINT8_TO_BSTREAM (pBuf, pEvt->version);
  UINT16_TO_BSTREAM(pBuf, pEvt->mfrName);
  UINT16_TO_BSTREAM(pBuf, pEvt->subversion);

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  Data length change event packet.
 *
 *  \param  pBuf        Packed packet buffer.
 *  \param  pEvt        Data length change indication data.
 *
 *  \return Packet length.
 */
/*************************************************************************************************/
static uint8_t lhciPackDataLenChangeEvt(uint8_t *pBuf, const LlDataLenChangeInd_t *pEvt)
{
  const uint8_t len = HCI_LEN_LE_DATA_LEN_CHANGE;

  UINT8_TO_BSTREAM (pBuf, HCI_LE_DATA_LEN_CHANGE_EVT);
  UINT16_TO_BSTREAM(pBuf, pEvt->handle);
  UINT16_TO_BSTREAM(pBuf, pEvt->maxTxLen);
  UINT16_TO_BSTREAM(pBuf, pEvt->maxTxTime);
  UINT16_TO_BSTREAM(pBuf, pEvt->maxRxLen);
  UINT16_TO_BSTREAM(pBuf, pEvt->maxRxTime);

  return len;
}

/*************************************************************************************************/
/*!
 *  \brief  LL ACL send complete event handler.
 *
 *  \param  handle      Connection handle.
 *  \param  numBufs     Number of buffers.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lhciAclSendComplete(uint16_t handle, uint8_t numBufs)
{
  if (lhciCb.genPktLen == 0)
  {
    uint8_t *pEvtBuf;

    if ((pEvtBuf = lhciAllocEvt(HCI_NUM_CMPL_PKTS_EVT, HCI_LEN_NUM_CMPL_PKTS)) != NULL)
    {
      lhciPackNumCompPktsEvt(pEvtBuf, handle, numBufs);
      lhciSendEvt(pEvtBuf);
    }
  }
  else
  {
    lhciCb.genPktCnt++;
    lhciCb.genOctetCnt += lhciCb.genPktLen;

    lhciGenerateAcl(handle, lhciCb.genPktLen, numBufs);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Sink LL ACL.
 *
 *  \param  pBuf      Buffer to sink.
 *
 *  \return TRUE if buffer sink.
 */
/*************************************************************************************************/
static bool_t lhciSinkAcl(uint8_t *pBuf)
{
  if (lhciCb.recvAclSink)
  {
    uint16_t len;

    BYTES_TO_UINT16(len, &pBuf[2]);
    lhciCb.recvAclPktCnt++;
    lhciCb.recvAclOctetCnt += len;

    WsfMsgFree(pBuf);
    LlRecvAclDataComplete(1);
    return TRUE;
  }
  return FALSE;
}

/*************************************************************************************************/
/*!
 *  \brief  Receive LL ACL.
 *
 *  \return Pointer to buffer for transport.
 */
/*************************************************************************************************/
uint8_t *lhciRecvAcl(void)
{
  uint8_t *pAclData;

  if ((pAclData = LlRecvAclData()) != NULL)
  {
    if (!lhciSinkAcl(pAclData))
    {
      return pAclData;
    }
  }

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief  LL ACL receive pending event handler.
 *
 *  \param  handle      Connection handle.
 *  \param  numBufs     Number of buffers.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lhciAclRecvPending(uint16_t handle, uint8_t numBufs)
{
  uint8_t *pAclData;

  if (lhciCb.recvAclSink && ((pAclData = LlRecvAclData()) != NULL))
  {
    lhciSinkAcl(pAclData);
    return;
  }

  ChciTrNeedsService(CHCI_TR_PROT_BLE);
}

/*************************************************************************************************/
/*!
 *  \brief  LL connection event handler.
 *
 *  \param  pEvt    Buffer containing LL event.
 *
 *  \return TRUE if event handled, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t lhciConnEncodeEvtPkt(LlEvt_t *pEvt)
{
  uint8_t *pEvtBuf = NULL;

  switch (pEvt->hdr.event)
  {
    case LL_CONN_IND:
      if ((lhciCb.leEvtMsk & ((uint64_t)(HCI_EVT_MASK_LE_CONN_CMPL_EVT) << LHCI_BYTE_TO_BITS(0))) &&
         (lhciCb.evtMsk & ((uint64_t)(HCI_EVT_MASK_LE_META) << LHCI_BYTE_TO_BITS(7))))
      {
        if ((pEvtBuf = lhciAllocEvt(HCI_LE_META_EVT, HCI_LEN_LE_CONN_CMPL)) != NULL)
        {
          lhciPackConnCompleteEvt(pEvtBuf, &pEvt->connInd);
        }
      }
      break;

    case LL_DISCONNECT_IND:
      if (lhciCb.evtMsk & ((uint64_t)(HCI_EVT_MASK_DISCONNECT_CMPL) << LHCI_BYTE_TO_BITS(0)))
      {
        if ((pEvtBuf = lhciAllocEvt(HCI_DISCONNECT_CMPL_EVT, HCI_LEN_DISCONNECT_CMPL)) != NULL)
        {
          lhciPackDiscCompleteEvt(pEvtBuf, &pEvt->disconnectInd);
        }
      }
      break;

    case LL_CONN_UPDATE_IND:
      if ((lhciCb.leEvtMsk & ((uint64_t)(HCI_EVT_MASK_LE_CONN_UPDATE_CMPL_EVT) << LHCI_BYTE_TO_BITS(0))) &&
         (lhciCb.evtMsk & ((uint64_t)(HCI_EVT_MASK_LE_META) << LHCI_BYTE_TO_BITS(7))))
      {
        if ((pEvtBuf = lhciAllocEvt(HCI_LE_META_EVT, HCI_LEN_LE_CONN_UPDATE_CMPL)) != NULL)
        {
          lhciPackConnUpdateCompleteEvt(pEvtBuf, &pEvt->connUpdateInd);
        }
      }
      break;

    case LL_REM_CONN_PARAM_IND:
      if ((lhciCb.leEvtMsk & ((uint64_t)(HCI_EVT_MASK_LE_REMOTE_CONN_PARAM_REQ_EVT) << LHCI_BYTE_TO_BITS(0))) &&
         (lhciCb.evtMsk & ((uint64_t)(HCI_EVT_MASK_LE_META) << LHCI_BYTE_TO_BITS(7))))
      {
        if ((pEvtBuf = lhciAllocEvt(HCI_LE_META_EVT, HCI_LEN_LE_REM_CONN_PARAM_REQ)) != NULL)
        {
          lhciPackRemoteConnParamEvt(pEvtBuf, &pEvt->remConnParamInd);
        }
      }
      break;

    case LL_READ_REMOTE_FEAT_CNF:
      if ((lhciCb.leEvtMsk & ((uint64_t)(HCI_EVT_MASK_LE_READ_REMOTE_FEAT_CMPL_EVT) << LHCI_BYTE_TO_BITS(0))) &&
         (lhciCb.evtMsk & ((uint64_t)(HCI_EVT_MASK_LE_META) << LHCI_BYTE_TO_BITS(7))))
      {
        if ((pEvtBuf = lhciAllocEvt(HCI_LE_META_EVT, HCI_LEN_LE_READ_REMOTE_FEAT_CMPL)) != NULL)
        {
          lhciPackReadRemoteUsedFeatCompleteEvt(pEvtBuf, &pEvt->readRemoteFeatCnf);
        }
      }
      break;

    case LL_READ_REMOTE_VER_INFO_CNF:
      if (lhciCb.evtMsk & ((uint64_t)(HCI_EVT_MASK_READ_REMOTE_VER_INFO_CMPL) << LHCI_BYTE_TO_BITS(1)))
      {
        if ((pEvtBuf = lhciAllocEvt(HCI_READ_REMOTE_VER_INFO_CMPL_EVT, HCI_LEN_READ_REMOTE_VER_INFO_CMPL)) != NULL)
        {
          lhciPackReadRemoteVerInfoCompleteEvt(pEvtBuf, &pEvt->readRemoteVerInfoCnf);
        }
      }
      break;

    case LL_DATA_LEN_CHANGE_IND:
      if (lhciCb.leEvtMsk & ((uint64_t)(HCI_EVT_MASK_LE_DATA_LEN_CHANGE_EVT) << LHCI_BYTE_TO_BITS(0)))
      {
        if ((pEvtBuf = lhciAllocEvt(HCI_LE_META_EVT, HCI_LEN_LE_DATA_LEN_CHANGE)) != NULL)
        {
          lhciPackDataLenChangeEvt(pEvtBuf, &pEvt->dataLenChangeInd);
        }
      }
      break;

    default:
      break;
  }

  if (pEvtBuf)
  {
    lhciSendEvt(pEvtBuf);
    return TRUE;
  }

  return FALSE;
}
