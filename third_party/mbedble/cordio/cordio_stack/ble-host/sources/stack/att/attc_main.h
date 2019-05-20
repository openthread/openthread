/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  ATT client main module.
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
#ifndef ATTC_MAIN_H
#define ATTC_MAIN_H

#include "wsf_queue.h"
#include "wsf_timer.h"
#include "wsf_assert.h"
#include "att_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/* Buffer lengths for requests */
#define ATT_MTU_REQ_BUF_LEN               (ATT_MTU_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_FIND_INFO_REQ_BUF_LEN         (ATT_FIND_INFO_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_FIND_TYPE_REQ_BUF_LEN         (ATT_FIND_TYPE_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_READ_TYPE_REQ_BUF_LEN         (ATT_READ_TYPE_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_READ_REQ_BUF_LEN              (ATT_READ_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_READ_BLOB_REQ_BUF_LEN         (ATT_READ_BLOB_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_READ_MULT_REQ_BUF_LEN         (ATT_READ_MULT_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_READ_GROUP_TYPE_REQ_BUF_LEN   (ATT_READ_GROUP_TYPE_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_WRITE_REQ_BUF_LEN             (ATT_WRITE_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_WRITE_CMD_BUF_LEN             (ATT_WRITE_CMD_LEN + L2C_PAYLOAD_START)
#define ATT_SIGNED_WRITE_CMD_BUF_LEN      (ATT_SIGNED_WRITE_CMD_LEN + L2C_PAYLOAD_START)
#define ATT_PREP_WRITE_REQ_BUF_LEN        (ATT_PREP_WRITE_REQ_LEN + L2C_PAYLOAD_START)
#define ATT_EXEC_WRITE_REQ_BUF_LEN        (ATT_EXEC_WRITE_REQ_LEN + L2C_PAYLOAD_START)

/* values for 'continuing' flag */
#define ATTC_CONTINUING                   TRUE
#define ATTC_NOT_CONTINUING               FALSE

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/* ATTC event handler messages */
enum
{
  /* messages from API; note these match method values */
  ATTC_MSG_API_NONE               = ATTC_MSG_START,
  ATTC_MSG_API_MTU                = ATT_METHOD_MTU,
  ATTC_MSG_API_FIND_INFO          = ATT_METHOD_FIND_INFO,
  ATTC_MSG_API_FIND_BY_TYPE_VALUE = ATT_METHOD_FIND_TYPE,
  ATTC_MSG_API_READ_BY_TYPE       = ATT_METHOD_READ_TYPE,
  ATTC_MSG_API_READ               = ATT_METHOD_READ,
  ATTC_MSG_API_READ_LONG          = ATT_METHOD_READ_BLOB,
  ATTC_MSG_API_READ_MULTIPLE      = ATT_METHOD_READ_MULTIPLE,
  ATTC_MSG_API_READ_BY_GROUP_TYPE = ATT_METHOD_READ_GROUP_TYPE,
  ATTC_MSG_API_WRITE              = ATT_METHOD_WRITE,
  ATTC_MSG_API_WRITE_CMD          = ATT_METHOD_WRITE_CMD,
  ATTC_MSG_API_PREP_WRITE         = ATT_METHOD_PREPARE_WRITE,
  ATTC_MSG_API_EXEC_WRITE         = ATT_METHOD_EXECUTE_WRITE,
  ATTC_MSG_API_SIGNED_WRITE_CMD,
  ATTC_MSG_CMAC_CMPL,
  ATTC_MSG_API_CANCEL,
  ATTC_MSG_REQ_TIMEOUT
};

/*!
 * Data buffer format for API request messages:
 *
 * | attcPktParam_t | ATT request data |
 * | bytes 0 to 7   | bytes 8 -        |
 */

/* Structure for API with offset parameter */
typedef struct
{
  uint16_t                len;
  uint16_t                offset;
} attcPktParamOffset_t;

/* Structure for API with start and end handle parameters */
typedef struct
{
  uint16_t                len;
  uint16_t                startHandle;
  uint16_t                endHandle;
} attcPktParamHandles_t;

/* Structure for API with offset and value parameters */
typedef struct
{
  uint16_t                len;
  uint16_t                offset;
  uint8_t                 *pValue;
} attcPktParamPrepWrite_t;

/* union of API parameter types */
typedef union
{
  uint16_t                len;
  attcPktParamOffset_t    o;
  attcPktParamHandles_t   h;
  attcPktParamPrepWrite_t w;
} attcPktParam_t;

/* verify attcPktParam_t will work in data buffer format described above */
//WSF_CT_ASSERT(sizeof(attcPktParam_t) <= L2C_PAYLOAD_START);

/* API message structure */
typedef struct
{
  wsfMsgHdr_t             hdr;
  attcPktParam_t          *pPkt;
  uint16_t                handle;
} attcApiMsg_t;

/* ATTC connection control block */
typedef struct
{
  attCcb_t                *pMainCcb;    /* Pointer to ATT main CCB */
  attcApiMsg_t            onDeck;       /* API message "on deck" waiting to be sent */
  attcApiMsg_t            outReq;       /* Outstanding request waiting for response */
  attcPktParam_t          outReqParams; /* Parameters associated with outstanding request */
  wsfTimer_t              outReqTimer;  /* Outstanding request timer */
  bool_t                  flowDisabled; /* Data flow disabled */
  bool_t                  cnfPending;   /* Handle value confirm packet waiting to be sent */
  uint16_t                pendWriteCmdHandle[ATT_NUM_SIMUL_WRITE_CMD]; /* Callback to app pending for this write cmd handle */
} attcCcb_t;

/* Signed data callbacks */
typedef void (*attcSignMsgCback_t)(attcCcb_t *pCcb, attcApiMsg_t *pMsg);
typedef void (*attcCloseCback_t)(attcCcb_t *pCcb, uint8_t status);

/* Signed data callback interface */
typedef struct
{
  attcSignMsgCback_t      msgCback;     /* Message handling callback */
  attcCloseCback_t        closeCback;   /* Connection close callback */
} attcSignFcnIf_t;

/* Main control block of the ATTC subsystem */
typedef struct
{
  attcCcb_t               ccb[DM_CONN_MAX];
  attcSignFcnIf_t const   *pSign;
} attcCb_t;

/* type for response processing functions */
typedef void (*attcProcRsp_t)(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/* Control block */
extern attcCb_t attcCb;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

attcCcb_t *attcCcbByConnId(dmConnId_t connId);
attcCcb_t *attcCcbByHandle(uint16_t handle);
void attcFreePkt(attcApiMsg_t *pMsg);
void attcExecCallback(dmConnId_t connId, uint8_t event, uint16_t handle, uint8_t status);
void attcReqClear(attcCcb_t *pCcb, attcApiMsg_t *pMsg, uint8_t status);

void attcSetupReq(attcCcb_t *pCcb, attcApiMsg_t *pMsg);
void attcSendReq(attcCcb_t *pCcb);
void attcSendMsg(dmConnId_t connId, uint16_t handle, uint8_t msgId, attcPktParam_t *pPkt, bool_t continuing);

void attcProcRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket);
void attcProcInd(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket);

void attcProcErrRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);
void attcProcMtuRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);
void attcProcFindOrReadRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);
void attcProcFindByTypeRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);
void attcProcReadRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);
void attcProcReadLongRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);
void attcProcWriteRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);
void attcProcPrepWriteRsp(attcCcb_t *pCcb, uint16_t len, uint8_t *pPacket, attEvt_t *pEvt);

#ifdef __cplusplus
};
#endif

#endif /* ATTC_MAIN_H */
