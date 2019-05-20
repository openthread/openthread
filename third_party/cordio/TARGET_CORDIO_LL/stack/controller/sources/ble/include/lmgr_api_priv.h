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
 *  \brief Link layer manager privacy interface file.
 */
/*************************************************************************************************/

#ifndef LMGR_API_PRIV_H
#define LMGR_API_PRIV_H

#include "lmgr_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  \addtogroup LL_LMGR_API_PRIV
 *  \{
 */

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      Slave role device parameter definition. */
typedef struct
{
  uint32_t          resPrivAddrTimeout; /*!< Resolvable private address timeout. */
  uint8_t           numPendingAddrRes;  /*!< Number of pending address resolutions. */
} lmgrPrivCtrlBlk_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

extern lmgrPrivCtrlBlk_t lmgrPrivCb;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* Initialization */
void LmgrPrivInit(void);

/*! \} */    /* LL_LMGR_API_PRIV */

#ifdef __cplusplus
};
#endif

#endif /* LMGR_API_PRIV_H */
