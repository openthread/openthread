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
 *  \brief Link layer controller privacy interface file.
 */
/*************************************************************************************************/

#ifndef LCTR_API_PRIV_H
#define LCTR_API_PRIV_H

#include "lctr_api.h"
#include "lmgr_api_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  \addtogroup LL_LCTR_API_PRIV
 *  \{
 */

/**************************************************************************************************
  Constants
**************************************************************************************************/

/*! \brief      Slave advertising task messages for \a LCTR_DISP_ADV dispatcher. */
enum
{
  /* Privacy events */
  LCTR_PRIV_MSG_RES_PRIV_ADDR_TIMEOUT,  /*!< Resolvable private address timeout event. */
  LCTR_PRIV_MSG_ADDR_RES_NEEDED         /*!< Address resolution needed. */
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      Address resolution pending message. */
typedef struct
{
  lctrMsgHdr_t      hdr;                    /*!< Message header. */
  bool_t            peer;                   /*!< TRUE if RPA is a peer's RPA. */
  uint8_t           peerAddrType;           /*!< Peer identity address type. */
  uint64_t          peerIdentityAddr;       /*!< Peer identity address. */
  uint64_t          rpa;                    /*!< Resolvable private address. */
} lctrAddrResNeededMsg_t;

/*! \brief      Address resolution pending message. */
typedef union
{
  lctrMsgHdr_t            hdr;              /*!< Message header. */
  lctrAddrResNeededMsg_t  addrResNeeded;    /*!< Address resolution needed. */
} LctrPrivMsg_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* Initialization */
void LctrPrivInit(void);

/* Control */
void LctrPrivSetResPrivAddrTimeout(uint32_t timeout);

/*! \} */    /* LL_LCTR_API_PRIV */

#ifdef __cplusplus
};
#endif

#endif /* LCTR_API_PRIV_H */
