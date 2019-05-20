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
 *  \brief Link layer controller master encryption implementation file.
 */
/*************************************************************************************************/

#include "lctr_int_conn.h"
#include "lctr_int_conn_master.h"
#include "lctr_int_conn_slave.h"
#include "lctr_int_enc_master.h"
#include "lctr_int_enc_slave.h"
#include "lmgr_api.h"
#include "wsf_assert.h"

/*************************************************************************************************/
/*!
 *  \brief      Process a received data channel PDU in master role.
 *
 *  \param      pCtx    Connection context.
 *  \param      pBuf    PDU buffer.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void lctrMstEncProcessDataPdu(lctrConnCtx_t *pCtx, uint8_t *pBuf)
{
  if (pCtx->enabled)
  {
    uint8_t ctrlResult, encResult;

    if (((ctrlResult = lctrDecodeCtrlPdu(&lctrDataPdu, pBuf, pCtx->role)) == LL_SUCCESS) ||
        ((encResult = lctrDecodeEncPdu(&lctrDataPdu, pBuf, pCtx->role)) == LL_SUCCESS))
    {
#if (LL_ENABLE_TESTER)
      if ((llTesterCb.rxLlcpFilter & (1 << lctrDataPdu.opcode)) != 0)
      {
        return;
      }
#endif
      if (pCtx->role == LL_ROLE_MASTER)
      {
        lctrMstConnExecuteSm(pCtx, LCTR_CONN_MSG_RX_LLCP);
      }
      else
      {
        lctrSlvConnExecuteSm(pCtx, LCTR_CONN_MSG_RX_LLCP);
      }
    }
    else if ((ctrlResult == LL_ERROR_CODE_INVALID_LMP_PARAMS) ||
             (encResult == LL_ERROR_CODE_INVALID_LMP_PARAMS))
    {
      if (pCtx->role == LL_ROLE_MASTER)
      {
        lctrMstConnExecuteSm(pCtx, LCTR_CONN_MSG_RX_LLCP_INVALID_PARAM);
      }
      else
      {
        lctrSlvConnExecuteSm(pCtx, LCTR_CONN_MSG_RX_LLCP_INVALID_PARAM);
      }
    }
    else
    {
      if (pCtx->role == LL_ROLE_MASTER)
      {
        lctrMstConnExecuteSm(pCtx, LCTR_CONN_MSG_RX_LLCP_UNKNOWN);
      }
      else
      {
        lctrSlvConnExecuteSm(pCtx, LCTR_CONN_MSG_RX_LLCP_UNKNOWN);
      }
    }
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize link layer controller resources for connectable encrypted master.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LctrMstConnEncInit(void)
{
  LctrMstConnInit();

  /* Add LLCP SM handler. */
  lctrMstLlcpSmTbl[LCTR_LLCP_SM_ENCRYPT] = lctrMstExecuteEncryptSm;
  lctrMstLlcpSmTbl[LCTR_LLCP_SM_PING]    = lctrExecutePingSm;

  /* Add control PDU handler. */
  lctrCtrlPduHdlr = lctrMstEncProcessDataPdu;

  /* Add packet encryption handlers. */
  lctrInitCipherBlkHdlr = BbBleDrvAesInitCipherBlock;
#if (!BB_ENABLE_INLINE_ENC_TX)
  lctrPktEncryptHdlr = BbBleDrvAesCcmEncrypt;
#else
  lctrSetEncryptPktCountHdlr = BbBleDrvSetEncryptPacketCount;
#endif
#if (!BB_ENABLE_INLINE_DEC_RX)
  lctrPktDecryptHdlr = BbBleDrvAesCcmDecrypt;
#else
  lctrSetDecryptPktCountHdlr = BbBleDrvSetDecryptPacketCount;
#endif

  /* Set supported features. */
  lmgrPersistCb.featuresDefault |= LL_FEAT_ENCRYPTION;
  if (pLctrRtCfg->btVer >= LL_VER_BT_CORE_SPEC_4_1)
  {
    lmgrPersistCb.featuresDefault |= LL_FEAT_LE_PING;
  }
}
