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
 *  \brief Internal link layer controller slave connection interface file.
 */
/*************************************************************************************************/

#ifndef LCTR_INT_CONN_SLAVE_H
#define LCTR_INT_CONN_SLAVE_H

#include "lctr_int_conn.h"
#include "bb_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

extern LctrLlcpHdlr_t lctrSlvLlcpSmTbl[LCTR_LLCP_SM_TOTAL];

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* State machine */
void lctrSlvConnExecuteSm(lctrConnCtx_t *pConnCtx, uint8_t event);
void lctrSlvLlcpExecuteSm(lctrConnCtx_t *pCtx, uint8_t event);
bool_t lctrSlvLlcpExecuteConnUpdSm(lctrConnCtx_t *pCtx, uint8_t event);
void lctrConnStatelessEventHandler(lctrConnCtx_t *pCtx, uint8_t event);
bool_t lctrLlcpExecuteCommonSm(lctrConnCtx_t *pCtx, uint8_t event);
void lctrLlcpStatelessEventHandler(lctrConnCtx_t *pCtx, uint8_t event);

/* Status */
bool_t lctrSlvCheckEncOverrideConnParam(lctrConnCtx_t *pCtx);
bool_t lctrSlvCheckEncOverrideCommonParam(lctrConnCtx_t *pCtx);
bool_t lctrSlvCheckConnUpdInstant(lctrConnCtx_t *pCtx);

/* Builder */
uint32_t lctrCalcIntervalWindowWideningUsec(lctrConnCtx_t *pCtx, uint32_t unsyncTimeUsec);
void lctrSlvConnBuildOp(lctrConnCtx_t *pCtx);

/* ISR */
void lctrSlvConnBeginOp(BbOpDesc_t *pOp);
void lctrSlvConnCleanupOp(BbOpDesc_t *pOp);
void lctrSlvConnEndOp(BbOpDesc_t *pOp);
void lctrSlvConnTxCompletion(BbOpDesc_t *pOp, uint8_t status);
void lctrSlvConnRxCompletion(BbOpDesc_t *pOp, uint8_t *pRxBuf, uint8_t status);

#ifdef __cplusplus
};
#endif

#endif /* LCTR_INT_CONN_SLAVE_H */
