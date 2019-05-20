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
 *  \brief Link manager connection implementation file.
 */
/*************************************************************************************************/

#include "lmgr_api_conn.h"
#include "bb_ble_api.h"
#include "lctr_api.h"
#include <string.h>

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief      Slave role device parameters. */
lmgrConnCtrlBlk_t lmgrConnCb;

/*************************************************************************************************/
/*!
 *  \brief      Initialize link layer manager connection resources.
 *
 *  \return     None.
 */
/*************************************************************************************************/
void LmgrConnInit(void)
{
  memset(&lmgrConnCb, 0, sizeof(lmgrConnCb));

  lmgrConnCb.availTxBuf = pLctrRtCfg->numTxBufs;
  lmgrConnCb.availRxBuf = pLctrRtCfg->numRxBufs;

  lmgrConnCb.maxTxLen = LL_MAX_DATA_LEN_MIN;
  lmgrConnCb.maxTxTime = LL_MAX_DATA_TIME_MIN;

  lmgrConnCb.allPhys = LL_ALL_PHY_TX_PREFERENCE_BIT | LL_ALL_PHY_RX_PREFERENCE_BIT;
  lmgrConnCb.rxPhys = LL_PHYS_NONE;
  lmgrConnCb.txPhys = LL_PHYS_NONE;
}
