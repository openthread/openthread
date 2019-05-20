/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  ATT server write PDU processing functions.
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
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "util/bstream.h"
#include "att_api.h"
#include "att_main.h"
#include "atts_main.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/* Queued prepare write structure */
typedef struct attsPrepWrite_tag
{
  struct attsPrepWrite_tag  *pNext;
  uint16_t                  writeLen;
  uint16_t                  handle;
  uint16_t                  offset;
  uint8_t                   packet[1];
} attsPrepWrite_t;


/*************************************************************************************************/
/*!
 *  \brief  Execute a queued prepared write operation.
 *
 *  \param  pCcb      Connection control block.
 *  \param  pPrep     Pointer to buffer containing prepared write.
 *
 *  \return ATT_SUCCESS or failure status.
 */
/*************************************************************************************************/
static uint8_t attsExecPrepWrite(attCcb_t *pCcb, attsPrepWrite_t *pPrep)
{
  uint8_t     *p;
  attsAttr_t  *pAttr;
  attsGroup_t *pGroup;
  uint8_t     err = ATT_SUCCESS;

  p = pPrep->packet;

  /* find attribute */
  if ((pAttr = attsFindByHandle(pPrep->handle, &pGroup)) == NULL)
  {
    /* handle not found; only possible unless handle was removed */
    err = ATT_ERR_UNLIKELY;
  }
  /* verify write permissions (in theory could have changed) */
  else if (!(pAttr->permissions & ATTS_PERMIT_WRITE))
  {
    err = ATT_ERR_WRITE;
  }
  /* if write callback is desired */
  else if ((pAttr->settings & ATTS_SET_WRITE_CBACK) &&
           (pGroup->writeCback != NULL))
  {

    /* write callback performs the write */
    err = (*pGroup->writeCback)(pCcb->connId, pPrep->handle, ATT_PDU_EXEC_WRITE_REQ,
                                pPrep->offset, pPrep->writeLen, p, pAttr);
  }
  /* else check if CCC */
  else if ((pAttr->settings & ATTS_SET_CCC) && (attsCb.cccCback != NULL))
  {
    err = (*attsCb.cccCback)(pCcb->connId, ATT_METHOD_WRITE, pPrep->handle, p);
  }
  else
  {
    /* perform write; parameters have already been vetted by previous procedures */

    /* write attribute value */
    memcpy((pAttr->pValue + pPrep->offset), p, pPrep->writeLen);

    /* write the length if variable length attribute */
    if ((pAttr->settings & ATTS_SET_VARIABLE_LEN) != 0)
    {
      *(pAttr->pLen) = pPrep->writeLen + pPrep->offset;
    }
  }

  return err;
}

/*************************************************************************************************/
/*!
 *  \brief  Process a write request or write command PDU.
 *
 *  \param  pCcb     Connection control block.
 *  \param  len       The length of the L2CAP payload data in pPacket.
 *  \param  pPacket   A buffer containing the packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
void attsProcWrite(attCcb_t *pCcb, uint16_t len, uint8_t *pPacket)
{
  uint8_t     *pBuf;
  uint8_t     *p;
  attsAttr_t  *pAttr;
  attsGroup_t *pGroup;
  uint8_t     opcode;
  uint16_t    handle;
  uint16_t    writeLen;
  uint8_t     err = ATT_SUCCESS;

  /* parse opcode handle, calculate write length */
  pPacket += L2C_PAYLOAD_START;
  BSTREAM_TO_UINT8(opcode, pPacket);
  BSTREAM_TO_UINT16(handle, pPacket);
  writeLen = len - ATT_WRITE_REQ_LEN;

  /* find attribute */
  if ((pAttr = attsFindByHandle(handle, &pGroup)) != NULL)
  {
    /* verify permissions */
    if ((err = attsPermissions(pCcb->connId, ATTS_PERMIT_WRITE,
                               handle, pAttr->permissions)) != ATT_SUCCESS)
    {
      /* err has been set; fail */
    }
    /* verify write length, fixed length */
    else if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) == 0) &&
             (writeLen != pAttr->maxLen))
    {
      err = ATT_ERR_LENGTH;
    }
    /* verify write length, variable length */
    else if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) != 0) &&
             (writeLen > pAttr->maxLen))
    {
      err = ATT_ERR_LENGTH;
    }
    else
    {
      /* if write callback is desired */
      if ((pAttr->settings & ATTS_SET_WRITE_CBACK) &&
          (pGroup->writeCback != NULL))
      {
        err = (*pGroup->writeCback)(pCcb->connId, handle, opcode, 0, writeLen,
                                    pPacket, pAttr);
      }
      /* else check if CCC */
      else if ((pAttr->settings & ATTS_SET_CCC) && (attsCb.cccCback != NULL))
      {
        err = (*attsCb.cccCback)(pCcb->connId, ATT_METHOD_WRITE, handle, pPacket);
      }
      else
      {
        /* write attribute value */
        memcpy(pAttr->pValue, pPacket, writeLen);

        /* write the length if variable length attribute */
        if ((pAttr->settings & ATTS_SET_VARIABLE_LEN) != 0)
        {
          *(pAttr->pLen) = writeLen;
        }
      }

      /* if success and write req allocate response buffer */
      if (err == ATT_SUCCESS && opcode == ATT_PDU_WRITE_REQ)
      {
        if ((pBuf = attMsgAlloc(L2C_PAYLOAD_START + ATT_WRITE_RSP_LEN)) != NULL)
        {
          /* build and send PDU */
          p = pBuf + L2C_PAYLOAD_START;
          UINT8_TO_BSTREAM(p, ATT_PDU_WRITE_RSP);

          L2cDataReq(L2C_CID_ATT, pCcb->handle, ATT_WRITE_RSP_LEN, pBuf);
        }
      }
    }
  }
  /* else attribute not found */
  else
  {
    err = ATT_ERR_HANDLE;
  }

  /* send error response for write req only */
  if (err && (opcode == ATT_PDU_WRITE_REQ))
  {
    attsErrRsp(pCcb->handle, ATT_PDU_WRITE_REQ, handle, err);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process a prepare write request PDU.
 *
 *  \param  pCcb      Connection control block.
 *  \param  len       The length of the L2CAP payload data in pPacket.
 *  \param  pPacket   A buffer containing the packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
void attsProcPrepWriteReq(attCcb_t *pCcb, uint16_t len, uint8_t *pPacket)
{
  uint8_t         *pBuf;
  uint8_t         *p;
  attsAttr_t      *pAttr;
  attsGroup_t     *pGroup;
  attsPrepWrite_t *pPrep;
  uint16_t        handle;
  uint16_t        offset;
  uint16_t        writeLen;
  uint8_t         err = ATT_SUCCESS;

  /* parse handle and offset, calculate write length */
  pPacket += L2C_PAYLOAD_START + ATT_HDR_LEN;
  BSTREAM_TO_UINT16(handle, pPacket);
  BSTREAM_TO_UINT16(offset, pPacket);
  writeLen = len - ATT_PREP_WRITE_REQ_LEN;    /* length of value being written */

  /* find attribute */
  if ((pAttr = attsFindByHandle(handle, &pGroup)) == NULL)
  {
    /* attribute not found */
    err = ATT_ERR_HANDLE;
  }
  /* verify permissions */
  else if ((err = attsPermissions(pCcb->connId, ATTS_PERMIT_WRITE,
                               handle, pAttr->permissions)) != ATT_SUCCESS)
  {
    /* err has been set; fail */
  }
  /* verify offset is allowed */
  else if ((offset != 0) && ((pAttr->settings & ATTS_SET_ALLOW_OFFSET) == 0))
  {
    err = ATT_ERR_NOT_LONG;
  }
  /* verify write length, fixed length */
  else if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) == 0) &&
           (writeLen != pAttr->maxLen))
  {
    err = ATT_ERR_LENGTH;
  }
  /* verify prepare write queue limit not reached */
  else if (WsfQueueCount(&pCcb->prepWriteQueue) >= pAttCfg->numPrepWrites)
  {
    err = ATT_ERR_QUEUE_FULL;
  }
  /* allocate new buffer to hold prepared write */
  else if ((pPrep = WsfBufAlloc(sizeof(attsPrepWrite_t) - 1 + writeLen)) == NULL)
  {
    err = ATT_ERR_RESOURCES;
  }
  else if ((pAttr->settings & ATTS_SET_WRITE_CBACK) &&
          (pGroup->writeCback != NULL))
  {
    err = (*pGroup->writeCback)(pCcb->connId, handle, ATT_PDU_PREP_WRITE_REQ, 0, writeLen,
                                pPacket, pAttr);
  }

  if (err == ATT_SUCCESS)
  {
    /* copy data to new buffer and queue it */
    pPrep->writeLen = writeLen;
    pPrep->handle = handle;
    pPrep->offset = offset;
    memcpy(pPrep->packet, pPacket, writeLen);
    WsfQueueEnq(&pCcb->prepWriteQueue, pPrep);

    /* allocate response buffer */
    if ((pBuf = attMsgAlloc(L2C_PAYLOAD_START + ATT_PREP_WRITE_RSP_LEN + writeLen)) != NULL)
    {
      /* build and send PDU */
      p = pBuf + L2C_PAYLOAD_START;
      UINT8_TO_BSTREAM(p, ATT_PDU_PREP_WRITE_RSP);
      UINT16_TO_BSTREAM(p, handle);
      UINT16_TO_BSTREAM(p, offset);
      memcpy(p, pPacket, writeLen);

      L2cDataReq(L2C_CID_ATT, pCcb->handle, (ATT_PREP_WRITE_RSP_LEN + writeLen), pBuf);
    }
  }

  if (err)
  {
    attsErrRsp(pCcb->handle, ATT_PDU_PREP_WRITE_REQ, handle, err);
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Process an execute write request PDU.
 *
 *  \param  pCcb      Connection control block.
 *  \param  len       The length of the L2CAP payload data in pPacket.
 *  \param  pPacket   A buffer containing the packet.
 *
 *  \return None.
 */
/*************************************************************************************************/
void attsProcExecWriteReq(attCcb_t *pCcb, uint16_t len, uint8_t *pPacket)
{
  uint8_t         *pBuf;
  uint8_t         *p;
  attsPrepWrite_t *pPrep;
  attsAttr_t      *pAttr;
  attsGroup_t     *pGroup;
  uint8_t         err = ATT_SUCCESS;

  pPacket += L2C_PAYLOAD_START + ATT_HDR_LEN;

  /* if cancelling all prepared writes */
  if (*pPacket == ATT_EXEC_WRITE_CANCEL)
  {
    /* free all queued buffers */
    attsClearPrepWrites(pCcb);
  }
  /* else writing all prepared writes */
  else if (*pPacket == ATT_EXEC_WRITE_ALL)
  {
    /* iterate over prepare write queue and verify offset and length */
    for (pPrep = pCcb->prepWriteQueue.pHead; pPrep != NULL; pPrep = pPrep->pNext)
    {
      /* find attribute */
      if ((pAttr = attsFindByHandle(pPrep->handle, &pGroup)) != NULL)
      {
        /* verify offset */
        if (pPrep->offset > pAttr->maxLen)
        {
          err = ATT_ERR_OFFSET;
        }
        /* verify write length with offset */
        else if ((pPrep->writeLen + pPrep->offset) > pAttr->maxLen)
        {
          err = ATT_ERR_LENGTH;
        }

        if (err)
        {
          /* verification failed; discard all prepared writes */
          attsClearPrepWrites(pCcb);
          break;
        }
      }
    }

    /* if length and offset checks ok then write all buffers in queue */
    if (err == ATT_SUCCESS)
    {
      /* for each buffer */
      while ((pPrep = WsfQueueDeq(&pCcb->prepWriteQueue)) != NULL)
      {
        /* write buffer */
        if ((err = attsExecPrepWrite(pCcb, pPrep)) != ATT_SUCCESS)
        {
          /* write failed; discard remaining prepared writes */
          attsClearPrepWrites(pCcb);
        }

        /* free buffer */
        WsfBufFree(pPrep);
      }
    }
  }
  /* else unknown operation */
  else
  {
    err = ATT_ERR_INVALID_PDU;
  }

  /* send response or error response */
  if (err)
  {
    attsErrRsp(pCcb->handle, ATT_PDU_EXEC_WRITE_REQ, 0, err);
  }
  else
  {
    if ((pBuf = attMsgAlloc(L2C_PAYLOAD_START + ATT_EXEC_WRITE_RSP_LEN)) != NULL)
    {
      /* build and send PDU */
      p = pBuf + L2C_PAYLOAD_START;
      UINT8_TO_BSTREAM(p, ATT_PDU_EXEC_WRITE_RSP);

      L2cDataReq(L2C_CID_ATT, pCcb->handle, ATT_EXEC_WRITE_RSP_LEN, pBuf);
    }
  }
}
