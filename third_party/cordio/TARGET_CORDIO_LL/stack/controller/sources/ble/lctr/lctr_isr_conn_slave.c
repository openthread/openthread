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
 *  \brief Link layer controller slave connection ISR callbacks.
 */
/*************************************************************************************************/

#include "lctr_int_conn.h"
#include "lctr_int_conn_slave.h"
#include "lctr_int_adv_slave.h"
#include "lmgr_api_conn.h"
#include "sch_api.h"
#include "sch_api_ble.h"
#include "bb_ble_api.h"
#include "wsf_assert.h"
#include "wsf_math.h"
#include "wsf_msg.h"
#include "wsf_os.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "bb_drv.h"
#include <string.h>

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief      Slave connection ISR control block (used only by active operation). */
static struct
{
  uint8_t consCrcFailed;        /*!< Number of consecutive CRC failures. */
  bool_t syncWithMaster;        /*!< Flag indicating synchronize packet received from master. */
  uint8_t syncConnHandle;       /*!< Connection handle where synchronized packet was received. */
  bool_t rxFromMaster;          /*!< At least one successful packet received from master. */
  uint32_t firstRxStartTs;      /*!< Timestamp of the first received frame regardless of CRC error. */
} lctrSlvConnIsr;

/*! \brief      Assert BB meets data PDU requirements. */
WSF_CT_ASSERT((BB_FIXED_DATA_PKT_LEN == 0) ||
              (BB_FIXED_DATA_PKT_LEN >= LCTR_DATA_PDU_MAX_LEN));

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

#if (LL_ENABLE_TESTER)
void LctrProcessRxTxAck(lctrConnCtx_t *pCtx, uint8_t *pRxBuf, uint8_t **pNextRxBuf, bool_t *pTxPduIsAcked);
#endif

/*************************************************************************************************/
/*!
 *  \brief  Abort the slave latency and recover connection BOD due time and event counter.
 *
 *  \param  pOp     Connection operation.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void lctrSlvAbortSlvLatency(BbOpDesc_t *pOp)
{
  lctrConnCtx_t * const pCtx = pOp->pCtx;
  const uint32_t curTime = BbDrvGetCurrentTime();
  uint32_t connInterval = BB_US_TO_BB_TICKS(LCTR_CONN_IND_US(pCtx->connInterval));
  uint32_t count = 0;

  if ((pOp->due - curTime) >= LCTR_SCH_MAX_SPAN)
  {
    /* Slave tries to exit latency while the current BoD is executing, do nothing. */
  }
  else if (pOp->due - curTime > connInterval) /* Imply (pOp->due - curTime) < LCTR_SCH_MAX_SPAN */
  {
    /* If the connection BOD is due in the future and after the next immediate anchor point,
     * adjust the event counter. */
    count = LlMathDivideUint32((pOp->due - curTime), connInterval);
    pCtx->eventCounter -= count;

    /* Need to adjust the lastChanIdx for channel selection algorithm 1. */
    if (pCtx->usedChSel == LL_CH_SEL_1)
    {
      if (pCtx->lastChanIdx >= (count * pCtx->hopInc))
      {
        pCtx->lastChanIdx -= (count * pCtx->hopInc);
      }
      else
      {
        if (((count * pCtx->hopInc) - pCtx->lastChanIdx) <= LL_MAX_NUM_CHAN_DATA)
        {
          /* Avoid modulo operation. */
          pCtx->lastChanIdx = LL_MAX_NUM_CHAN_DATA - ((count * pCtx->hopInc) - pCtx->lastChanIdx);
        }
        else
        {
          pCtx->lastChanIdx = LL_MAX_NUM_CHAN_DATA - LL_MATH_MOD_37(((count * pCtx->hopInc) - pCtx->lastChanIdx));
        }
      }
    }

    /* The anchor point, due time, minimum duration and Rx timeout will be adjusted accordingly later. */
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Update a connection operation.
 *
 *  \param  pCtx        Connection context.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void lctrSlvConnUpdateOp(lctrConnCtx_t *pCtx)
{
  /* Pre-resolve common structures for efficient access. */
  BbOpDesc_t * const pOp = &pCtx->connBod;
  BbBleData_t * const pBle = &pCtx->bleData;
  BbBleSlvConnEvent_t * const pConn = &pBle->op.slvConn;
  lctrConnUpdInd_t * const pConnUpdInd = &pCtx->connUpd;

  /*** Connection context setup ***/

  const uint16_t connIntervalOld = pCtx->connInterval;
  const uint16_t latencyOld      = pCtx->maxLatency;
  const uint16_t supTimeoutMsOld = pCtx->supTimeoutMs;

  /* Make sure set the anchor point to the instant in the connection update indication. */
  const uint16_t numIntervals = pCtx->eventCounter - pCtx->data.slv.lastActiveEvent + 1;
  const uint32_t timeUsec     = LCTR_CONN_IND_US(connIntervalOld * numIntervals);
  const uint32_t time         = BB_US_TO_BB_TICKS(timeUsec);
  pCtx->data.slv.anchorPoint  += time;

  if (pCtx->connUpd.instant != pCtx->data.slv.lastActiveEvent)
  {
    /* Save the unsynced time before the connection update. */
    pCtx->data.slv.unsyncedTime = time;
  }

  pCtx->data.slv.lastActiveEvent = pCtx->eventCounter + 1;   /* guarantee execution of first connection event. */
  /* Update connection parameters. */
  pCtx->connInterval             = pConnUpdInd->interval;
  pCtx->maxLatency               = pConnUpdInd->latency;
  pCtx->supTimeoutMs             = LCTR_CONN_IND_TO_MS(pConnUpdInd->timeout);

  /*** General setup ***/

  const uint32_t txWinOffsetUsec    = LCTR_CONN_IND_US(pConnUpdInd->txWinOffset);
  const uint32_t txWinOffset        = BB_US_TO_BB_TICKS(txWinOffsetUsec);
  const uint32_t txWinSizeUsec      = LCTR_CONN_IND_US(pConnUpdInd->txWinSize);
  const uint32_t wwTxWinOffsetUsec  = lctrCalcIntervalWindowWideningUsec(pCtx, txWinOffsetUsec);
  const uint32_t wwTxWinOffset      = BB_US_TO_BB_TICKS(wwTxWinOffsetUsec);

  /* txWinOffset is relative to anchorPoint. */
  pCtx->data.slv.anchorPoint += txWinOffset;

  /* Add additional time due to Tx window offset and subtract WW due to Tx window offset. */
  pOp->due += txWinOffset - wwTxWinOffset;

  pCtx->data.slv.txWinSizeUsec = txWinSizeUsec;

  /* Add additional time due to Tx window size and WW due to Tx window offset. */
  pOp->minDurUsec += txWinSizeUsec + wwTxWinOffsetUsec;

  /*** BLE general setup ***/

  pConn->rxSyncDelayUsec += txWinSizeUsec + (wwTxWinOffsetUsec << 1);

  /* Unconditionally reset supervision timer with transitional value.
   *     connIntervalOld + supervisionTimeoutNew */
  WsfTimerStartMs(&pCtx->tmrSupTimeout, LCTR_CONN_IND_MS(connIntervalOld) + pCtx->supTimeoutMs);

  /*** Notifications ***/

  /* Notify host only if connection parameters changed. */
  if ((connIntervalOld != pCtx->connInterval) ||
      (latencyOld      != pCtx->maxLatency) ||
      (supTimeoutMsOld != pCtx->supTimeoutMs))
  {
    pCtx->llcpNotifyMask |= 1 << LCTR_PROC_CONN_UPD;
  }

  /* Delay notification until CE starts. */
  pCtx->llcpInstantComp = TRUE;

  /* Reset flags. */
  pCtx->data.slv.initAckRcvd = FALSE;

  LL_TRACE_INFO2("    >>> Connection updated, handle=%u, eventCounter=%u <<<", LCTR_GET_CONN_HANDLE(pCtx), pCtx->eventCounter);
  LL_TRACE_INFO1("                            connIntervalUsec=%u", LCTR_CONN_IND_US(pCtx->connInterval));
}

/*************************************************************************************************/
/*!
 *  \brief  Update the channel map.
 *
 *  \param  pCtx        Connection context.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void lctrSlvChanMapUpdateOp(lctrConnCtx_t *pCtx)
{
  pCtx->chanMask = pCtx->chanMapUpd.chanMask;
  lctrBuildRemapTable(pCtx);

  /* Delay notification until CE starts. */
  pCtx->llcpInstantComp = TRUE;

  LL_TRACE_INFO2("    >>> Channel map updated, handle=%u, eventCounter=%u <<<", LCTR_GET_CONN_HANDLE(pCtx), pCtx->eventCounter);
}

/*************************************************************************************************/
/*!
 *  \brief  Update the selected PHY.
 *
 *  \param  pCtx        Connection context.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void lctrSlvPhyUpdateOp(lctrConnCtx_t *pCtx)
{
  uint8_t txPhyOld = lctrPhyToPhysBit(pCtx->bleData.chan.txPhy);
  uint8_t rxPhyOld = lctrPhyToPhysBit(pCtx->bleData.chan.rxPhy);

  /* Notify host only if PHY changed. */
  if ((pCtx->phyUpd.slaveToMasterPhy != LL_PHYS_NONE) &&
      (pCtx->phyUpd.slaveToMasterPhy != txPhyOld))
  {
    pCtx->bleData.chan.txPhy = lctrPhysBitToPhy(pCtx->phyUpd.slaveToMasterPhy);
    pCtx->llcpNotifyMask    |= 1 << LCTR_PROC_PHY_UPD;
  }
  if ((pCtx->phyUpd.masterToSlavePhy != LL_PHYS_NONE) &&
      (pCtx->phyUpd.masterToSlavePhy != rxPhyOld))
  {
    pCtx->bleData.chan.rxPhy = lctrPhysBitToPhy(pCtx->phyUpd.masterToSlavePhy);
    pCtx->llcpNotifyMask    |= 1 << LCTR_PROC_PHY_UPD;
  }

  /* Delay notification until CE starts. */
  pCtx->llcpInstantComp = TRUE;

  /* Update connection event durations. */
  pCtx->effConnDurUsec   = lctrCalcConnDurationUsec(pCtx, &pCtx->effDataPdu);
  pCtx->localConnDurUsec = lctrCalcConnDurationUsec(pCtx, &pCtx->localDataPdu);

  LL_TRACE_INFO2("    >>> PHY updated, handle=%u, eventCounter=%u <<<", LCTR_GET_CONN_HANDLE(pCtx), pCtx->eventCounter);
}

/*************************************************************************************************/
/*!
 *  \brief  Begin a connection operation.
 *
 *  \param  pOp     Begin operation.
 *
 *  \return None.
 *
 *  \note   Scheduler may call this routine multiple times in the following situation:
 *          this BOD is pending and a BOD is inserted before.
 */
/*************************************************************************************************/
void lctrSlvConnBeginOp(BbOpDesc_t *pOp)
{
  lctrConnCtx_t * const pCtx = pOp->pCtx;
  uint8_t *pBuf;

  if (lctrCheckForLinkTerm(pCtx))
  {
    BbSetBodTerminateFlag();
  }

  if (pLctrVsHdlrs && pLctrVsHdlrs->ceSetup)
  {
    pLctrVsHdlrs->ceSetup(LCTR_GET_CONN_HANDLE(pCtx));
  }

  /*** Initialize connection event resources. ***/

  lctrSlvConnIsr.consCrcFailed = 0;
  lctrSlvConnIsr.syncWithMaster = FALSE;
  lctrSlvConnIsr.syncConnHandle = 0xFF;
  lctrSlvConnIsr.rxFromMaster = FALSE;

  /*** Setup receiver ***/

  if ((pBuf = lctrRxPduAlloc(pCtx->localDataPdu.maxRxLen)) != NULL)
  {
    lctrSetBbPacketCounterRx(pCtx);
    BbBleRxData(pBuf, LCTR_DATA_PDU_LEN(pCtx->localDataPdu.maxRxLen));

    /* Rx may fail; no more important statements in this code path */
  }
  else
  {
    LL_TRACE_ERR1("!!! OOM while initializing receive buffer at start of CE, handle=%u", LCTR_GET_CONN_HANDLE(pCtx));
    BbSetBodTerminateFlag();
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Cleanup a connection operation.
 *
 *  \param  pOp     Completed operation.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lctrSlvConnCleanupOp(BbOpDesc_t *pOp)
{

}

/*************************************************************************************************/
/*!
 *  \brief  End a connection operation.
 *
 *  \param  pOp     Completed operation.
 *
 *  \return None.
 */
/*************************************************************************************************/
void lctrSlvConnEndOp(BbOpDesc_t *pOp)
{
  /* Pre-resolve common structures for efficient access. */
  BbBleData_t * const pBle = pOp->prot.pBle;
  BbBleSlvConnEvent_t * const pConn = &pBle->op.slvConn;
  lctrConnCtx_t * const pCtx = pOp->pCtx;

  /* Process event completion */
  if (pCtx->data.slv.abortSlvLatency)
  {
    lctrSlvAbortSlvLatency(pOp);
  }

  pCtx->rssi = pConn->rssi;

  if ((lctrSlvConnIsr.syncWithMaster) && (lctrSlvConnIsr.syncConnHandle == LCTR_GET_CONN_HANDLE(pCtx)))
  {
    /* Re-sync to master's clock. */
    pCtx->data.slv.anchorPoint = lctrSlvConnIsr.firstRxStartTs;
    pCtx->data.slv.lastActiveEvent = pCtx->eventCounter + 1;

    /* Tx window no longer needed. */
    pCtx->data.slv.txWinSizeUsec = 0;

    /* Now sync'd to master, reset the unsynced time before connection update. */
    pCtx->data.slv.unsyncedTime = 0;
  }

  if (!pCtx->connEst && (lctrSlvConnIsr.rxFromMaster || (lctrSlvConnIsr.consCrcFailed > 0)))
  {
    lctrStoreConnTimeoutTerminateReason(pCtx);
    WsfTimerStartMs(&pCtx->tmrSupTimeout, pCtx->supTimeoutMs);

    pCtx->connEst = TRUE;
  }
  else if (lctrSlvConnIsr.rxFromMaster)
  {
    /* Reset supervision timer. */
    WsfTimerStartMs(&pCtx->tmrSupTimeout, pCtx->supTimeoutMs);
  }

  /* Terminate connection */
  if (lctrCheckForLinkTerm(pCtx))
  {
    lctrSendConnMsg(pCtx, LCTR_CONN_TERMINATED);
    WsfTimerStop(&pCtx->tmrSupTimeout);
    return;
  }

  if (pLctrVsHdlrs && pLctrVsHdlrs->ceCleanup)
  {
    pLctrVsHdlrs->ceCleanup(LCTR_GET_CONN_HANDLE(pCtx));
  }

  /* Slave initiates the startup LLCP procedure when receiving a non-control PDU from both master and slave.
   * This message shall be sent only once. */
  if ((pCtx->isFirstNonCtrlPdu == TRUE) &&
      (pCtx->isSlvReadySent == FALSE))
  {
    pCtx->isSlvReadySent = TRUE;
    lctrSendConnMsg(pCtx, LCTR_CONN_SLV_INIT_STARTUP_LLCP);
  }

  /*** Update for next operation ***/

  uint16_t numUnsyncIntervals = pCtx->eventCounter - pCtx->data.slv.lastActiveEvent + 1;

  uint16_t numSkipCe = 0;

  if ((pCtx->data.slv.abortSlvLatency == FALSE) &&
      lctrGetConnOpFlag(pCtx, LL_OP_MODE_FLAG_ENA_SLV_LATENCY) &&
      (pCtx->maxLatency &&
       pCtx->data.slv.initAckRcvd &&
       lctrSlvConnIsr.rxFromMaster &&
       (pCtx->state != LCTR_CONN_STATE_TERMINATING)))
  {
    if ((pCtx->llcpActiveProc == LCTR_PROC_INVALID))
    {
      numSkipCe = pCtx->maxLatency;
    }
    else
    {
      /* Still apply the slave latency if the instant is not reached for the following LLCPs. */
      if ((pCtx->llcpActiveProc == LCTR_PROC_CONN_UPD) && lctrSlvCheckConnUpdInstant(pCtx))
      {
        if ((uint16_t)(pCtx->connUpd.instant - pCtx->eventCounter) >= (pCtx->maxLatency + 1))
        {
          numSkipCe = pCtx->maxLatency;
        }
        else
        {
          numSkipCe = pCtx->connUpd.instant - pCtx->eventCounter - 1;
        }
      }
      else if ((pCtx->llcpActiveProc == LCTR_PROC_CMN_CH_MAP_UPD) &&
               (pCtx->cmnState == LCTR_CMN_STATE_BUSY))
      {
        if ((uint16_t)(pCtx->chanMapUpd.instant - pCtx->eventCounter) >= (pCtx->maxLatency + 1))
        {
          numSkipCe = pCtx->maxLatency;
        }
        else
        {
          numSkipCe = pCtx->chanMapUpd.instant - pCtx->eventCounter - 1;
        }
      }
      else if ((pCtx->llcpActiveProc == LCTR_PROC_PHY_UPD) && (pCtx->isSlvPhyUpdInstant == TRUE))
      {
        if ((uint16_t)(pCtx->phyUpd.instant - pCtx->eventCounter) >= (pCtx->maxLatency + 1))
        {
          numSkipCe = pCtx->maxLatency;
        }
        else
        {
          numSkipCe = pCtx->phyUpd.instant - pCtx->eventCounter - 1;
        }
      }
    }
  }

  if (numSkipCe > 0)
  {
    pCtx->eventCounter += numSkipCe;
    numUnsyncIntervals += numSkipCe;

    lctrChSelHdlr[pCtx->usedChSel](pCtx, numSkipCe - 1);

    LL_TRACE_INFO2("Applying slave latency, waking up at eventCounter=%u, numSkipCE=%u", pCtx->eventCounter + 1, numSkipCe);
  }

  if (pCtx->data.slv.abortSlvLatency)
  {
    pCtx->data.slv.abortSlvLatency = FALSE;
  }

  while (TRUE)
  {
    pCtx->eventCounter += 1;
    numUnsyncIntervals += 1;

    uint32_t unsyncTimeUsec = LCTR_CONN_IND_US(pCtx->connInterval * numUnsyncIntervals);

    /* Need to add the unsynced time before connection update. */
    unsyncTimeUsec += pCtx->data.slv.unsyncedTime;

    uint32_t wwTotalUsec    = lctrCalcIntervalWindowWideningUsec(pCtx, unsyncTimeUsec);
    uint32_t wwTotal        = BB_US_TO_BB_TICKS(wwTotalUsec);
    uint32_t connInterUsec  = LCTR_CONN_IND_US(numUnsyncIntervals * pCtx->connInterval);
    uint32_t connInter      = BB_US_TO_BB_TICKS(connInterUsec);
    int16_t  dueOffsetUsec  = (connInterUsec - wwTotalUsec) - BB_TICKS_TO_US(connInter - wwTotal);

    if (wwTotalUsec >= ((connInterUsec >> 1) - WSF_MAX(LL_BLE_TIFS_US, BbGetSchSetupDelayUs())))
    {
      LL_TRACE_WARN2("!!! Terminating connection due to excessive WW handle=%u, eventCounter=%u", LCTR_GET_CONN_HANDLE(pCtx), pCtx->eventCounter);
      lctrSendConnMsg(pCtx, LCTR_CONN_TERMINATED);
      WsfTimerStop(&pCtx->tmrSupTimeout);
      return;
    }

    /* Advance to next interval. */
    pOp->due = pCtx->data.slv.anchorPoint + connInter - wwTotal;
    pOp->dueOffsetUsec = WSF_MAX(dueOffsetUsec, 0);
    pOp->minDurUsec = pCtx->data.slv.txWinSizeUsec + pCtx->localConnDurUsec + wwTotalUsec;
    pConn->rxSyncDelayUsec = pCtx->data.slv.txWinSizeUsec + (wwTotalUsec << 1);

    if ((pCtx->llcpActiveProc == LCTR_PROC_CONN_UPD) &&
        (pCtx->eventCounter == pCtx->connUpd.instant))
    {
      lctrSlvConnUpdateOp(pCtx);
    }
    else if ((pCtx->llcpActiveProc == LCTR_PROC_CMN_CH_MAP_UPD) &&
             (pCtx->eventCounter == pCtx->chanMapUpd.instant))
    {
      lctrSlvChanMapUpdateOp(pCtx);
    }
    else if ((pCtx->llcpActiveProc == LCTR_PROC_PHY_UPD) &&
             (pCtx->eventCounter == pCtx->phyUpd.instant))
    {
      lctrSlvPhyUpdateOp(pCtx);
    }

    pBle->chan.chanIdx = lctrChSelHdlr[pCtx->usedChSel](pCtx, 0);

    if (SchInsertAtDueTime(pOp, lctrConnResolveConflict))
    {
      break;
    }

    /* TODO: When latency is applied, scheduling conflicts should subtract connection intervals. */

    LL_TRACE_WARN2("!!! CE schedule conflict handle=%u, eventCounter=%u", LCTR_GET_CONN_HANDLE(pCtx), pCtx->eventCounter);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Complete a transmitted data buffer.
 *
 *  \param      pOp     Operation context.
 *  \param      status  Transmit status.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void lctrSlvConnTxCompletion(BbOpDesc_t *pOp, uint8_t status)
{
  if (status == BB_STATUS_SUCCESS)
  {
    lctrConnCtx_t * const pCtx = pOp->pCtx;

    lctrSetControlPduAck(pCtx);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Complete a received data buffer.
 *
 *  \param      pOp     Operation context.
 *  \param      pRxBuf  Receive buffer.
 *  \param      status  Receive status.
 *
 *  \return     None.
 *
 *  \note       1. pRxBuf must be freed or placed into a queue by this function.
 *              2. If the BOD is terminated during this function, no further receive buffer should
 *                 remain allocated for the caller to clean up.
 */
/*************************************************************************************************/
void lctrSlvConnRxCompletion(BbOpDesc_t *pOp, uint8_t *pRxBuf, uint8_t status)
{
  lctrConnCtx_t * const pCtx = pOp->pCtx;

  BbBleData_t * const pBle = &pCtx->bleData;
  BbBleSlvConnEvent_t * const pConn = &pBle->op.slvConn;

  /* Local state */
  bool_t loadRxBuf = FALSE;
  bool_t txPduIsAcked = FALSE;
  uint8_t *pNextRxBuf = NULL;
  uint16_t txLen = 0;

  /*** LLCP instant completion processing ***/

  if (pCtx->llcpInstantComp)
  {
    pCtx->llcpInstantComp = FALSE;
    lctrSendConnMsg(pCtx, LCTR_CONN_LLCP_PROC_CMPL);
  }

  /*** Cancellation processing ***/

  if (status == BB_STATUS_CANCELED)
  {
    lctrRxPduFree(pRxBuf);
    goto Done;
  }

  /*** Connection event pre-processing ***/

  if (lctrCheckForLinkTerm(pCtx) ||
      (status == BB_STATUS_FAILED) ||
      (status == BB_STATUS_RX_TIMEOUT))
  {
    if (status == BB_STATUS_RX_TIMEOUT)
    {
      LL_TRACE_WARN3("lctrSlvConnRxCompletion: BB failed with status=RX_TIMEOUT, eventCounter=%u, bleChan=%u, handle=%u", pCtx->eventCounter, pBle->chan.chanIdx, LCTR_GET_CONN_HANDLE(pCtx));
    }

    if (status == BB_STATUS_FAILED)
    {
      LL_TRACE_ERR3("lctrSlvConnRxCompletion: BB failed with status=FAILED, eventCounter=%u, bleChan=%u, handle=%u", pCtx->eventCounter, pBle->chan.chanIdx, LCTR_GET_CONN_HANDLE(pCtx));
    }

    BbSetBodTerminateFlag();
    lctrRxPduFree(pRxBuf);
    goto Done;
  }

  /* Store anchor point. */
  if ((!lctrSlvConnIsr.syncWithMaster) &&
      ((status == BB_STATUS_SUCCESS) || (status == BB_STATUS_CRC_FAILED)))
  {
    lctrSlvConnIsr.firstRxStartTs = pConn->startTs;
    lctrSlvConnIsr.syncWithMaster = TRUE;
    lctrSlvConnIsr.syncConnHandle = LCTR_GET_CONN_HANDLE(pCtx);
  }

  /*** Receive packet pre-processing ***/

  if (status == BB_STATUS_SUCCESS)
  {
    lctrSlvConnIsr.rxFromMaster = TRUE;

    /* Reset consecutive CRC failure counter. */
    lctrSlvConnIsr.consCrcFailed = 0;
  }
  else if (status == BB_STATUS_CRC_FAILED)
  {
    lctrSlvConnIsr.consCrcFailed++;

    if (lctrSlvConnIsr.consCrcFailed >= LCTR_MAX_CONS_CRC)
    {
      /* Close connection event. */
      BbSetBodTerminateFlag();
      lctrRxPduFree(pRxBuf);
      goto Done;
    }

    goto SetupTx;
  }

  lctrUnpackDataPduHdr(&pCtx->rxHdr, pRxBuf);

  /* Check LLID from master. */
  if ((pCtx->isSlvReadySent == FALSE) &&
      (pCtx->rxHdr.llid != LL_LLID_CTRL_PDU) &&
      (lctrGetConnOpFlag(pCtx, LL_OP_MODE_FLAG_SLV_DELAY_LLCP_STARTUP)))
  {
    pCtx->isFirstNonCtrlPdu = TRUE;
  }

#if (LL_ENABLE_TESTER)
  if (llTesterCb.ackMode != LL_TESTER_ACK_MODE_NORMAL)
  {
    LctrProcessRxTxAck(pCtx, pRxBuf, &pNextRxBuf, &txPduIsAcked);
  }
  else
#endif
  {
    pNextRxBuf = lctrProcessRxAck(pCtx);
    txPduIsAcked = lctrProcessTxAck(pCtx);

    if (!pCtx->data.slv.initAckRcvd &&
        txPduIsAcked)
    {
      pCtx->data.slv.initAckRcvd = TRUE;
    }
  }

  /*** Setup for transmit ***/
SetupTx:

  /* Slave always transmits after receiving. */
  if ((txLen = lctrSetupForTx(pCtx, status, TRUE)) == 0)
  {
    BbSetBodTerminateFlag();
    goto PostProcessing;
  }

  /* Tx llid is obtained from lctrSetupForTx(). */
  if ((pCtx->isSlvReadySent == FALSE) &&
      (pCtx->isFirstNonCtrlPdu == TRUE) &&
      (lctrGetConnOpFlag(pCtx, LL_OP_MODE_FLAG_SLV_DELAY_LLCP_STARTUP)))
  {
    /* Set the flag when both master and slave send non-control PDU. */
    if (pCtx->txHdr.llid == LL_LLID_CTRL_PDU)
    {
      pCtx->isFirstNonCtrlPdu = FALSE;
    }
  }

  /* If Tx failed, return immediately after queueing Rx data. */
  if (BbGetBodTerminateFlag())
  {
    goto PostProcessing;
  }

  /*** Setup for receive ***/

  loadRxBuf = ((status == BB_STATUS_CRC_FAILED) || pCtx->rxHdr.md || pCtx->txHdr.md) &&
              !lctrExceededMaxDur(pCtx, pConn->startTs, SchBleCalcDataPktDurationUsec(pCtx->bleData.chan.txPhy, txLen) + pCtx->effConnDurUsec);

  /*** Packet post-processing ***/
PostProcessing:

  /* Tx post-processing. */
  lctrProcessTxAckCleanup(pCtx);

  /* Rx post-processing. */
  lctrCheckControlPduAck(pCtx);
  lctrRxPostProcessing(pCtx, pRxBuf, pNextRxBuf, loadRxBuf);

  /*** ISR complete ***/
Done:
  return;
}
