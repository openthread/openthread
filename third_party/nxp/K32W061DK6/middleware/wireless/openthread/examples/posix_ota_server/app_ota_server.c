/*
* Copyright 2019-2020 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/*!=================================================================================================
\file       app_ota_server.c
\brief      This is a public source file for the OTA server module
==================================================================================================*/

/*==================================================================================================
Include Files
==================================================================================================*/

/* Application */
#include <openthread/udp.h>
#include <openthread/coap.h>
#include <openthread/instance.h>
#include <openthread/error.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/ip6.h>
#include <openthread/random_noncrypto.h>

#include "app_ota.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "network_utils.h"

/*==================================================================================================
Private macros
==================================================================================================*/

#ifndef OTA_SERVER_DEFAULT_PORT
    #define OTA_SERVER_DEFAULT_PORT (61630)
#endif

#ifndef gOtaServer_DefaultTransferType_c
    #define gOtaServer_DefaultTransferType_c             gOtaMulticast_c
#endif

#define gOtaServer_MinDelayForEndRequestMs_c             20000
#define gOtaServer_MaxDelayForEndRequestMs_c             40000
#define gOtaServer_InvalidClientId_c                     0xFFFF
#define gOtaServer_MaxOtaImages_c                        0x01

#define gOtaServer_DelayForNextRequestMs_c               60000   /* 60 seconds */
#define gOtaServer_ClientSessionExpirationMs_c           30000   /* 30 seconds */

#define gOtaServer_MulticastInterval_c                   500     /* 500 miliseconds */
#define gOtaServer_MulticastImgNtfInterval_c             1000    /* 1 second */
#define gOtaServer_MulticastImgNtfRetransmissions_c      4
#define gOtaServer_MulticastBlockRspInterval_c           300
#define gOtaServer_MulticastUpgradeEndDelay_c            1000
#define gOtaServer_MulticastNoOfBlockRsps_c              0
#define gOtaServer_MulticastWindowSize_c                 32      /* Must be multiple of 8 */
#define gOtaServer_MulticastWindowRetries_c              0
#define gOtaServer_MulticastAckTimeout_c                 300

#define gOtaFileVersionPolicies_Upgrade_c       (1<<0)
#define gOtaFileVersionPolicies_Reinstall_c     (1<<1)
#define gOtaFileVersionPolicies_Downgrade_c     (1<<2)

#define gOtaFileVersionDefaultPolicies_c         gOtaFileVersionPolicies_Upgrade_c | \
                                                 gOtaFileVersionPolicies_Reinstall_c | \
                                                 gOtaFileVersionPolicies_Downgrade_c

/*==================================================================================================
Private type definitions
==================================================================================================*/
/* ota server multicast state: */
typedef enum
{
    gOtaServerMulticastState_NotInit_c = 0,
    gOtaServerMulticastState_Idle_c,
    gOtaServerMulticastState_SendImgNtf_c,
    gOtaServerMulticastState_GenBlockReq_c,
    gOtaServerMulticastState_WaitForAck_c,
    gOtaServerMulticastState_SendUpgradeEnd_c,
    gOtaServerMulticastState_ResetMulticast_c
} otaServerMulticastState_t;

/* ota server multicast state: */
typedef enum
{
	otaServerClientImageTypeREED = 0x0000,
	otaServerClientImageTypeED   = 0x0001,
	otaServerClientImageTypeLPED = 0x0002
} otaServerClientImageType;

typedef struct otaServerSetup_tag
{
    otInstance *pOtInstance;
    otUdpSocket *pOtaUdpSrvSocket;
    bool_t            isActive;
    uint8_t           fileVersionPolicy;
    otaTransferType_t transferType;
    uint16_t          downloadPort;
    /* Multicast parameters */
    otaServerMulticastState_t multicastState;
    uint8_t ackBitmask[4];
    uint32_t currentWindowOffset;
    uint8_t multicastNoOfImgNtf;
    uint8_t multicastNoOfBlockRsp;
    uint8_t multicastNoOfWindowRetries;
    uint16_t multicastManufacturerCode;
    uint16_t multicastImageType;
    uint32_t multicastImageSize;
    uint32_t multicastFileVersion;
} otaServerSetup_t;

typedef struct otaServerImageList_tag
{
    uint16_t manufCode;
    uint16_t imageType;
    uint32_t fileSize;
    uint32_t imageAddr;
    uint32_t fileVersion;
    bool_t isValidEntry;
} otaServerImageList_t;

typedef struct otaClientInfo_tag
{
    otIp6Address  remoteAddr;
    otIp6Address  sourceAddr;
    uint16_t  port;
    uint32_t  timeStamp;
    uint32_t  dataLen;
    uint8_t   pData[1];
} otaClientInfo_t;

typedef struct otaClientSessionInfo_tag
{
    otIp6Address remoteAddr;
    uint32_t timeStamp;
} otaClientSessionInfo_t;

typedef void ( *otaTmrCallback ) ( void *param );

/*==================================================================================================
Private function prototypes
==================================================================================================*/
/* OTA Server Coap command handlers */
static void OtaServer_ClientProcess(otaClientInfo_t *pOtaClientInfo);
static void OtaServer_CmdProcess(otaClientInfo_t *pOtaClientInfo);
static otaStatus_t OtaServer_CmdCheck(uint8_t otaCommand, uint32_t dataLen);
static otaStatus_t OtaServer_QueryImageReqHandler(otaClientInfo_t *pOtaClientInfo);
static otaStatus_t OtaServer_BlockReqHandler(otaClientInfo_t *pOtaClientInfo);
static otaStatus_t OtaServer_UpgradeEndReqHandler(otaClientInfo_t *pOtaClientInfo);
static otaStatus_t OtaServer_ServerDiscoveryHandler(otaClientInfo_t *pOtaClientInfo);
static void OtaServer_CoapCb(void *pCtx, otMessage *pMsg, const otMessageInfo *pMsgInfo);

/* OTA Server Coap commands */
static otaStatus_t OtaServer_SendImageNotifty(otaServer_ImageNotify_t *pImgNtf, otIp6Address *pAddr);
static otaStatus_t OtaServer_CoapSendImageNotify(otaServer_ImageNotify_t *pImageNotify, otIp6Address *pDestAddr);
static otaStatus_t OtaServer_CoapSendRsp(otaClientInfo_t *pOtaClientInfo, uint8_t *pData, uint32_t pDataLen);
static otaStatus_t OtaServer_CoapSendRspWaitAbortData(otaClientInfo_t *pOtaClientInfo, uint8_t status,
        uint32_t delayInMs);

/* OTA Server utility functions */
static void OtaServer_SetTimeCallback(otaTmrCallback pFunc, void *pData, uint32_t setTime);
static void OtaServer_StopTimeCallback(void);
static bool_t OtaServer_IsClientValid(uint16_t clientId);
static bool_t OtaServer_RemoveClientFromPercentageInfo(uint16_t clientId);
static void OtaServer_ResetPercentageInfo(void);
static otaStatus_t OtaServer_CheckClientSessionTable(otaClientInfo_t *pOtaClientInfo);
static otaStatus_t OtaServer_HandleBlockSocket(bool_t onOff);

/* OTA Server standalone functions: */
static void OtaServer_InitStandaloneOpMode(void);
static uint8_t OtaServerStandalone_ValidateImage(uint16_t manufCode, uint16_t imageType, uint32_t fileVersion, bool_t serialProtocol);
static uint8_t OtaServerStandalone_KeepImageInfo(uint16_t manufCode, uint16_t imageType, uint32_t fileVersion, uint32_t fileSize);
static otaStatus_t OtaServerStandalone_QueryImageReqHandler(otaClientInfo_t *pOtaClientInfo);
static otaStatus_t OtaServerStandalone_BlockReqHandler(otaClientInfo_t *pOtaClientInfo);
static otaStatus_t OtaServerStandalone_ServerDiscoveryHandler(otaClientInfo_t *pOtaClientInfo);

/* OTA Server Multicast */
static otaStatus_t OtaServer_InitMulticast(void *pParam);
static void OtaServer_MulticastMngr(void *pParam);
static void OtaServer_MulticastTimeoutCb(void *pParam);
static otaStatus_t OtaServer_SendImgNtf(void *pParam);
static otaStatus_t OtaServer_ProcessAckTimeout(void *pParam);
static otaStatus_t OtaServer_MulticastUpgradeEnd(void *pParam);
static otaStatus_t OtaServer_GenerateBlockReq(void *pParam);
static void OtaServer_ResetMulticastModule(void *pParam);

/*==================================================================================================
Private global variables declarations
==================================================================================================*/
/* Ota server setup parameters */
static otaServerSetup_t mOtaServerSetup = {.pOtaUdpSrvSocket = NULL,
                                           .downloadPort = OTA_SERVER_DEFAULT_PORT,
                                           .multicastState = gOtaServerMulticastState_NotInit_c,
                                           .transferType = gOtaServer_DefaultTransferType_c
                                          };

/* Ota server standalone informations: */
static otaServerImageList_t mOtaServerImageList[gOtaServer_MaxOtaImages_c];
static uint32_t mOtaServerTempImageIdx = gOtaServer_MaxOtaImages_c;

/* Ota server percentages information */
static otaServerPercentageInfo_t mOtaServerPercentageInformation;

static otaClientSessionInfo_t mOtaClientSessionInfoTable[gOtaServer_MaxSimultaneousClients_c];
static otUdpSocket mOtaUdpSrvSocket;

static char binary_file_path[255];
/*==================================================================================================
Public global variables declarations
==================================================================================================*/
otCoapResource gOTA_CLIENT_URI_PATH = {.mUriPath = OTA_CLIENT_URI_PATH, .mHandler = NULL/*OtaServer_CoapCb*/, .mContext = NULL, .mNext = NULL };
otCoapResource gOTA_SERVER_URI_PATH = {.mUriPath = OTA_SERVER_URI_PATH, .mHandler = NULL/*OtaClient_CoapCb*/, .mContext = NULL, .mNext = NULL };

/* Parameters for simple timer callback */
uint32_t setMilliTime = 0;
otaTmrCallback gpFunction = NULL;
void *gpParameter = NULL;
bool_t callbackIsSet = false;

/*==================================================================================================
Public functions
==================================================================================================*/
/*!*************************************************************************************************
\fn     void OtaServer_CheckTime(void)
\brief  This function is used to check if a timer callback for OTA needs to be called.
***************************************************************************************************/
void OtaServer_CheckTime(void)
{
    if((otPlatAlarmMilliGetNow() > setMilliTime) && (callbackIsSet == true) && (setMilliTime != 0))
    {
        setMilliTime = 0;

        if(gpFunction != NULL)
        {
            callbackIsSet = false;
            gpFunction(gpParameter);
        }
    }
}

/*!*************************************************************************************************
\fn     otaStatus_t OtaServerInit(taskMsgQueue_t *pMsgQueue)
\brief  Initialize OTA server application.

\param  [in]    pMsgQueue      Pointer to task message queue

\return         otaStatus_t    Status of the operation
 ***************************************************************************************************/
otaStatus_t OtaServerInit
(
    otInstance *pOtInstance
)
{
    otaStatus_t otaStatus = gOtaStatus_Success_c;

    if (pOtInstance == NULL)
    {
        otaStatus = gOtaStatus_InvalidInstance_c;
    }

    if (otaStatus == gOtaStatus_Success_c)
    {
        /* Register Services in COAP */
        mOtaServerSetup.pOtInstance = pOtInstance;
        otCoapStart(mOtaServerSetup.pOtInstance, OT_DEFAULT_COAP_PORT);
        gOTA_CLIENT_URI_PATH.mContext = mOtaServerSetup.pOtInstance;
        gOTA_CLIENT_URI_PATH.mHandler = OtaServer_CoapCb;
        otCoapAddResource(mOtaServerSetup.pOtInstance, &gOTA_CLIENT_URI_PATH);

        // Set operation mode to standalone
        mOtaServerSetup.isActive = false;

        OtaServer_ResetPercentageInfo();

        memset(&mOtaClientSessionInfoTable, 0x00,
               sizeof(otaClientSessionInfo_t) * gOtaServer_MaxSimultaneousClients_c);
        mOtaServerSetup.fileVersionPolicy = gOtaFileVersionDefaultPolicies_c;
        memset(&binary_file_path[0], 0x00, sizeof(binary_file_path));

        if (true == mOtaServerSetup.isActive)
        {
            OtaServer_HandleBlockSocket(true);
        }
    }

    return otaStatus;
}

/*!*************************************************************************************************
\public
\fn  otaStatus_t OtaServer_StartOta()
\brief  Start OTA process

\param  [in]    otaType        Type of OTA process (unicast or multicast)
\param  [in]    pFilePath      Path to binary

\return         otaStatus_t    Status of the operation
 ***************************************************************************************************/
otaStatus_t OtaServer_StartOta(uint8_t otaType, const char *pFilePath)
{
    otaServer_ImageNotify_t *pImageNotify = NULL;
    otaStatus_t status = gOtaStatus_Success_c;

    if ((otaType != gOtaUnicast_c) && (otaType != gOtaMulticast_c))
    {
        status = gOtaStatus_Failed_c;
        goto exit;
    }

    // Check if device is connected before starting OTA
    if (otThreadGetDeviceRole(mOtaServerSetup.pOtInstance) < OT_DEVICE_ROLE_CHILD)
    {
        status = gOtaStatus_NotPermitted_c;
        goto exit;
    }

    // Check if OTA process is already active
    if (mOtaServerSetup.isActive == true)
    {
        status = gOtaStatus_AlreadyStarted_c;
        goto exit;
    }

    if (pFilePath != NULL)
    {
        memcpy(binary_file_path, pFilePath, strlen(pFilePath));

        if (!(access(binary_file_path, F_OK) != -1))
        {
            status = gOtaStatus_InvalidValue_c;
            goto exit;
        }
    }
    else
    {
        status = gOtaStatus_EmptyEntry_c;
        goto exit;
    }

    // Set multicast addresses used in OTA process
    NWKU_OtSetMulticastAddresses(mOtaServerSetup.pOtInstance);
    mOtaServerSetup.transferType = otaType;

    OtaServer_ResetPercentageInfo();
    mOtaServerPercentageInformation.otaType = otaType;

    /* clear current image entries */
    for (uint8_t i = 0; i < gOtaServer_MaxOtaImages_c; i++)
    {
        mOtaServerImageList[i].isValidEntry = false;
    }

    mOtaServerTempImageIdx = gOtaServer_MaxOtaImages_c;

    OtaServer_InitStandaloneOpMode();

    if (mOtaServerImageList[mOtaServerTempImageIdx].imageType == otaServerClientImageTypeLPED)
    {
        if (otaType == gOtaMulticast_c)
        {
            mOtaServerSetup.transferType = gOtaUnicast_c;
            mOtaServerPercentageInformation.otaType = gOtaUnicast_c;
            status = gOtaStatus_TransferTypeNotSupported_c;
        }
    }

    // OTA Multicast parameters. Not used for OTA unicast.
    if (mOtaServerSetup.transferType == gOtaMulticast_c)
    {
        // OTA multicast.
        pImageNotify = (otaServer_ImageNotify_t *)calloc(1, sizeof(otaServer_ImageNotify_t));
    }

    OtaServer_SendImageNotifty(pImageNotify, &in6addr_realmlocal_allthreadnodes);

    if (mOtaServerSetup.transferType == gOtaMulticast_c)
    {
        OtaServer_SetTimeCallback(OtaServer_MulticastTimeoutCb, (void *)pImageNotify, 100);
    }

exit:
    return status;
}

/*!*************************************************************************************************
\fn     otaResult_t OtaServer_StopOta(void)
\brief  Process Stop OTA command received from an external application.

\return         otaStatus_t    Result of the operation
***************************************************************************************************/
otaStatus_t OtaServer_StopOta
(
    void
)
{
    otaStatus_t result = gOtaStatus_NoMem_c;
    uint8_t size = sizeof(otaClientInfo_t) - 1 + sizeof(otaCmd_UpgradeEndRsp_t);
    otaClientInfo_t *pOtaClientInfo;

    pOtaClientInfo = calloc(1, size);
    mOtaServerSetup.isActive = false;

    /* clear current image entries */
    for (uint8_t i = 0; i < gOtaServer_MaxOtaImages_c; i++)
    {
        mOtaServerImageList[i].isValidEntry = false;
    }

    mOtaServerTempImageIdx = gOtaServer_MaxOtaImages_c;

    if (pOtaClientInfo)
    {
        otaCmd_QueryImageRsp_t queryRsp = {0};
        uint32_t timeInMs = otPlatAlarmMilliGetNow();
        uint32_t delayInMs = 0;

        queryRsp.commandId = gOtaCmd_UpgradeEndRsp_c;
        queryRsp.status = gOtaFileStatus_Abort_c;

        memcpy(&pOtaClientInfo->remoteAddr, &in6addr_realmlocal_allthreadnodes, sizeof(otIp6Address));
        memcpy(&pOtaClientInfo->sourceAddr, GET_OTA_ADDRESS(mOtaServerSetup.pOtInstance), sizeof(otIp6Address));

        pOtaClientInfo->port = OTA_SERVER_DEFAULT_PORT;
        pOtaClientInfo->dataLen = size - sizeof(otaClientInfo_t) + 1;
        pOtaClientInfo->pData[0] = gOtaCmd_UpgradeEndRsp_c;

        memcpy(&queryRsp.data.wait.currentTime, &timeInMs, sizeof(uint32_t));
        timeInMs = timeInMs + delayInMs;
        memcpy(&queryRsp.data.wait.requestTime, &timeInMs, sizeof(uint32_t));

        // Send the Block Req to the OTA Server.
        result = OtaServer_CoapSendRsp(pOtaClientInfo, (uint8_t *)&queryRsp, sizeof(queryRsp));
    }

    OtaServer_ResetMulticastModule(NULL);

    return result;
}

/*!*************************************************************************************************
\fn     void OtaServer_GetOtaStatus(otaServerPercentageInfo_t *pData)
\brief  This function is used to check the status of the OTA transfer.

\param  [in]    pData        Pointer to output structure
***************************************************************************************************/
void OtaServer_GetOtaStatus(otaServerPercentageInfo_t *pData)
{
    memcpy(pData, (uint8_t *)&mOtaServerPercentageInformation, sizeof(mOtaServerPercentageInformation));
}

/*==================================================================================================
Private functions
==================================================================================================*/
/*!*************************************************************************************************
\private
\fn     static void OtaServer_SetTimeCallback(otaTmrCallback pFunc, void *param, uint32_t setTime)
\brief  This function sets a timer callback for OTA functions.

\param  [in]    pFunc           Callback function
\param  [in]    pData           Callback function parameter
\param  [in]    setTime         Set time for callback in milliseconds
***************************************************************************************************/
static void OtaServer_SetTimeCallback
(
    otaTmrCallback pFunc,
    void *pData,
    uint32_t setTime
)
{
    if(mOtaServerSetup.isActive == true)
    {
        if(pFunc != NULL)
        {
            gpFunction = pFunc;
        }

        gpParameter = pData;
        setMilliTime = otPlatAlarmMilliGetNow() + setTime;
        callbackIsSet = true;
    }
}

/*!*************************************************************************************************
\private
\fn     static void OtaServer_StopTimeCallback(void)
\brief  This function stops and clears the timer callback for OTA functions.
***************************************************************************************************/
static void OtaServer_StopTimeCallback(void)
{
    gpFunction = NULL;
    gpParameter = NULL;
    setMilliTime = 0;
    callbackIsSet = false;
}

/*!*************************************************************************************************
\private
\fn     static void OtaServer_CoapCb(void *pCtx, otMessage *pMsg, const otMessageInfo *pMsgInfo)
\brief  This function is the callback function for CoAP message.

\param  [in]    pCtx            Pointer to OpenThread context
\param  [in]    pMsg            Pointer to CoAP message
\param  [in]    pMsgInfo        Pointer to CoAP message information
***************************************************************************************************/
static void OtaServer_CoapCb
(
    void *pCtx,
    otMessage *pMsg,
    const otMessageInfo *pMsgInfo
)
{
    uint16_t dataLen = otMessageGetLength(pMsg) - otMessageGetOffset(pMsg);
    uint8_t otaCommand = gOtaCmd_Invalid_c;

    otMessageRead(pMsg, otMessageGetOffset(pMsg), (void *)&otaCommand, sizeof(otaCommand));

    if (gOtaStatus_Success_c == OtaServer_CmdCheck(otaCommand, dataLen))
    {
        otaClientInfo_t *pOtaClientInfo = (otaClientInfo_t *)calloc(1, sizeof(otaClientInfo_t) + dataLen);
    
        if (NULL != pOtaClientInfo)
        {
            otIp6Address nullAddr = {0};

            /* Save client info params */
            otMessageRead(pMsg, otMessageGetOffset(pMsg), (void *)&pOtaClientInfo->pData, dataLen);
            pOtaClientInfo->dataLen = dataLen;
            memcpy(&pOtaClientInfo->remoteAddr, &pMsgInfo->mPeerAddr, sizeof(otIp6Address));

            if (memcmp(&pOtaClientInfo->remoteAddr, &nullAddr, sizeof(otIp6Address)))
            {
                memcpy(&pOtaClientInfo->sourceAddr, &in6addr_realmlocal_allthreadnodes, sizeof(otIp6Address));
            }

            pOtaClientInfo->timeStamp = otPlatAlarmMilliGetNow();
            OtaServer_ClientProcess(pOtaClientInfo);
        }
    }
    (void)pCtx;
}

/*!*************************************************************************************************
\private
\fn     static void OtaClient_UdpServerService(uint8_t *pInData)
\brief  This function is the callback function for Ota server socket.

\param  [in]    pCtx            Pointer to OpenThread context
\param  [in]    pMsg            Pointer to CoAP message
\param  [in]    pMsgInfo        Pointer to CoAP message information
***************************************************************************************************/
static void OtaClient_UdpServerService
(
    void *pCtx,
    otMessage *pMsg,
    const otMessageInfo *pMsgInfo
)
{
    uint16_t dataLen = otMessageGetLength(pMsg) - otMessageGetOffset(pMsg);
    uint8_t otaCommand = gOtaCmd_Invalid_c;

    otMessageRead(pMsg, otMessageGetOffset(pMsg), (void *)&otaCommand, sizeof(otaCommand));

    if (gOtaCmd_BlockReq_c == otaCommand)
    {
        otaClientInfo_t *pOtaClientInfo = (otaClientInfo_t *)calloc(1, sizeof(otaClientInfo_t) + dataLen);

        if (NULL != pOtaClientInfo)
        {
            /* Save client info params */
            otMessageRead(pMsg, otMessageGetOffset(pMsg), (void *)pOtaClientInfo->pData, dataLen);
            memcpy(&pOtaClientInfo->remoteAddr, (void *)&pMsgInfo->mPeerAddr, sizeof(otIp6Address));
            memcpy(&pOtaClientInfo->sourceAddr, (void *)&pMsgInfo->mSockAddr, sizeof(otIp6Address));
            pOtaClientInfo->port = pMsgInfo->mPeerPort;
            pOtaClientInfo->dataLen = dataLen;
            pOtaClientInfo->timeStamp = otPlatAlarmMilliGetNow();
            OtaServer_ClientProcess(pOtaClientInfo);
        }
    }
    (void) pCtx;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_SendImageNotifty(otaServer_ImageNotify_t *pImgNtf, otIp6Address *pAddr)
\brief  This function is used for the transmission of Image Notification commands.

\param  [in]    pImgNtf         Pointer to the otaServer_ImageNotify_t structure
\param  [in]    pAddr           Pointer to peer address

\return         otaStatus_t     Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_SendImageNotifty
(
    otaServer_ImageNotify_t *pImgNtf,
    otIp6Address *pAddr
)
{
    bool_t status = false;
    FILE *pFile = fopen(binary_file_path,"rb");
    
    if(pFile != NULL)
    {
        otaServer_ImageNotify_t imageNotify;
        otaFileSubElement_t imageTag;

        /* transfer completed */
        mOtaServerImageList[mOtaServerTempImageIdx].isValidEntry = true;
        mOtaServerSetup.isActive = true;

        /* Set position after file header */
        fseek(pFile, mOtaServerImageList[mOtaServerTempImageIdx].imageAddr + sizeof(otaFileHeader_t), SEEK_SET);

        if(fread((void *)&imageTag, sizeof(otaFileSubElement_t), 1, pFile))
        {
            // Inform clients that a new image is available
            memset(&imageNotify, 0, sizeof(otaServer_ImageNotify_t));
            memcpy(&imageNotify.fileVersion, &mOtaServerImageList[mOtaServerTempImageIdx].fileVersion, sizeof(uint32_t));
            memcpy(&imageNotify.imageType, &mOtaServerImageList[mOtaServerTempImageIdx].imageType, sizeof(uint16_t));
            memcpy(&imageNotify.manufacturerCode, &mOtaServerImageList[mOtaServerTempImageIdx].manufCode, sizeof(uint16_t));
            memcpy(&imageNotify.imageSize, &imageTag.length, sizeof(imageNotify.imageSize));
            memcpy(&imageNotify.fileSize, &mOtaServerImageList[mOtaServerTempImageIdx].fileSize, sizeof(uint32_t));
        }

        /* return image notify data for multicast usage */
        if(pImgNtf != NULL)
        {
            memcpy(pImgNtf, &imageNotify, sizeof(otaServer_ImageNotify_t));
        }

        status = (OtaServer_CoapSendImageNotify(&imageNotify, pAddr) == gOtaStatus_Success_c) ? true : false;
    }

    if (pFile != NULL)
    {
        fclose(pFile);
    }

    return (status == true) ? gOtaStatus_Success_c : gOtaStatus_Failed_c;
}


/*!*************************************************************************************************
\private
\fn     static void OtaServer_ClientProcess(uint8_t *param)
\brief  This function is used to process ota client commands.

\param  [in]    pOtaClientInfo    Pointer to pOtaClientInfo structure
***************************************************************************************************/
static void OtaServer_ClientProcess
(
    otaClientInfo_t *pOtaClientInfo
)
{
    bool_t isServerBusy = false;
    uint16_t clientId;

    if (false == mOtaServerSetup.isActive)
    {
        /* Server is not active -  send back a Rsp command with status no image available */
        OtaServer_CoapSendRspWaitAbortData(pOtaClientInfo, gOtaFileStatus_NoImageAvailable_c, 0);
    }
    else
    {
        if ((gOtaStatus_Success_c == OtaServer_CheckClientSessionTable(pOtaClientInfo)) &&
                (gOtaStatus_Success_c == OtaServer_HandleBlockSocket(true)))
        {
            if (mOtaServerSetup.transferType == gOtaUnicast_c)
            {
                clientId = (pOtaClientInfo->remoteAddr.mFields.m8[14] << 8) + pOtaClientInfo->remoteAddr.mFields.m8[15];

                if((clientId != gOtaServer_InvalidClientId_c) && OtaServer_IsClientValid(clientId))
                {
                    OtaServer_CmdProcess(pOtaClientInfo);
                }
                else
                {
                    isServerBusy = true;
                }
            }
            else if (mOtaServerSetup.transferType == gOtaMulticast_c)
            {
                OtaServer_CmdProcess(pOtaClientInfo);
            }
        }
        else
        {
            isServerBusy = true;
        }

        if (isServerBusy)
        {
            OtaServer_CoapSendRspWaitAbortData(pOtaClientInfo,
                                               gOtaFileStatus_ServerBusy_c,
                                               gOtaServer_DelayForNextRequestMs_c);
        }
    }
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_CheckClientSessionTable(otaClientInfo_t *pOtaClientInfo)
\brief  This function is used to check if the server can process this new client request.

\param  [in]    pOtaClientInfo    Pointer to client info

\return         otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_CheckClientSessionTable
(
    otaClientInfo_t *pOtaClientInfo
)
{
    otaStatus_t status = gOtaStatus_Failed_c;
    uint8_t idxInfoTable;
    uint8_t firstExpiredEntry = gOtaServer_MaxSimultaneousClients_c;

    for (idxInfoTable = 0; idxInfoTable < gOtaServer_MaxSimultaneousClients_c; idxInfoTable++)
    {
        if (memcmp(&pOtaClientInfo->remoteAddr, &mOtaClientSessionInfoTable[idxInfoTable].remoteAddr, sizeof(otIp6Address)))
        {
            mOtaClientSessionInfoTable[idxInfoTable].timeStamp = pOtaClientInfo->timeStamp;
            return gOtaStatus_Success_c;
        }

        if (((mOtaClientSessionInfoTable[idxInfoTable].timeStamp == 0) ||
                (mOtaClientSessionInfoTable[idxInfoTable].timeStamp + (gOtaServer_ClientSessionExpirationMs_c) < pOtaClientInfo->timeStamp)) &&
                (firstExpiredEntry == gOtaServer_MaxSimultaneousClients_c))
        {
            firstExpiredEntry = idxInfoTable;
        }
    }

    if (firstExpiredEntry < gOtaServer_MaxSimultaneousClients_c)
    {
        memcpy(&mOtaClientSessionInfoTable[firstExpiredEntry].remoteAddr, &pOtaClientInfo->remoteAddr, sizeof(otIp6Address));
        mOtaClientSessionInfoTable[firstExpiredEntry].timeStamp = pOtaClientInfo->timeStamp;
        status = gOtaStatus_Success_c;
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_CmdCheck(void *pData, uint32_t dataLen)
\brief  This function is used to check if ota client command is valid.

\param  [in]    pData          OTA cmd data
\param  [in]    dataLen        OTA cmd length

\return         otaStatus_t    Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_CmdCheck
(
    uint8_t otaCommand,
    uint32_t dataLen
)
{
    otaStatus_t status = gOtaStatus_Failed_c;

    switch (otaCommand)
    {
        case gOtaCmd_QueryImageReq_c:
            if (dataLen == sizeof(otaCmd_QueryImageReq_t))
            {
                status = gOtaStatus_Success_c;
            }

            break;

        case gOtaCmd_BlockReq_c:
            if (dataLen == (sizeof(otaCmd_BlockReq_t)))
            {
                status = gOtaStatus_Success_c;
            }

            break;

        case gOtaCmd_UpgradeEndReq_c:
            if (dataLen == sizeof(otaCmd_UpgradeEndReq_t))
            {
                status = gOtaStatus_Success_c;
            }

            break;

        case gOtaCmd_ServerDiscovery_c:
            if (dataLen == sizeof(otaCmd_ServerDiscovery_t))
            {
                status = gOtaStatus_Success_c;
            }

            break;

        default:
            break;
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static void OtaServer_CmdProcess(void *param)
\brief  This function is used to process ota client commands.

\param  [in]    pOtaClientInfo    Pointer to pOtaClientInfo structure
***************************************************************************************************/
static void OtaServer_CmdProcess
(
    otaClientInfo_t *pOtaClientInfo
)
{
    uint8_t otaCommand = *pOtaClientInfo->pData;

    switch (otaCommand)
    {
        case gOtaCmd_QueryImageReq_c:
            (void)OtaServer_QueryImageReqHandler(pOtaClientInfo);
            break;

        case gOtaCmd_BlockReq_c:
            (void)OtaServer_BlockReqHandler(pOtaClientInfo);
            break;

        case gOtaCmd_UpgradeEndReq_c:
            (void)OtaServer_UpgradeEndReqHandler(pOtaClientInfo);
            break;

        case gOtaCmd_ServerDiscovery_c:
            (void)OtaServer_ServerDiscoveryHandler(pOtaClientInfo);
            break;

        default:
            free((void *)pOtaClientInfo);
            break;
    }
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_QueryImageReqHandler(otaClientInfo_t *pOtaClientInfo)
\brief  This function is used to process a QueryImageReq command.

\param  [in]    pOtaClientInfo    Pointer to client info

\return         otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_QueryImageReqHandler
(
    otaClientInfo_t *pOtaClientInfo
)
{
    return OtaServerStandalone_QueryImageReqHandler(pOtaClientInfo);
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_BlockReqHandler(otaClientInfo_t *pOtaClientInfo)
\brief  This function is used to process a BlockRequest command.

\param  [in]    pOtaClientInfo    Pointer to client info

\return         otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_BlockReqHandler
(
    otaClientInfo_t *pOtaClientInfo
)
{
    return OtaServerStandalone_BlockReqHandler(pOtaClientInfo);
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_UpgradeEndReqHandler(otaClientInfo_t *pOtaClientInfo)
\brief  This function is used to process an UpdateEndRequest command.

\param  [in]    pOtaClientInfo    Pointer to client info

\return         otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_UpgradeEndReqHandler
(
    otaClientInfo_t *pOtaClientInfo
)
{
    otaCmd_UpgradeEndRsp_t upgradeRsp = {0};
    uint32_t timeInMs = otPlatAlarmMilliGetNow();
    /* Get the client status from the received packet */
    otaFileStatus_t client_status = (otaFileStatus_t) (*(pOtaClientInfo->pData + 1));
    uint16_t clientId = 0;

    upgradeRsp.commandId = gOtaCmd_UpgradeEndRsp_c;
    upgradeRsp.status = gOtaStatus_Success_c;
    memcpy(&upgradeRsp.data.success.currentTime, &timeInMs, sizeof(uint32_t));
    timeInMs += otRandomNonCryptoGetUint32InRange(gOtaServer_MinDelayForEndRequestMs_c, gOtaServer_MaxDelayForEndRequestMs_c);
    memcpy(&upgradeRsp.data.success.upgradeTime, &timeInMs, sizeof(uint32_t));

    if (client_status == gOtaFileStatus_Success_c)
    {
        (void)OtaServer_CoapSendRsp(pOtaClientInfo, (uint8_t *)&upgradeRsp, sizeof(otaCmd_UpgradeEndRsp_t));
    }

    NWKU_MemCpyReverseOrder(&clientId, &pOtaClientInfo->remoteAddr.mFields.m8[14], sizeof(uint16_t));

    OtaServer_RemoveClientFromPercentageInfo(clientId);

    if (client_status != gOtaFileStatus_Success_c)
    {
        free(pOtaClientInfo);
    }

    return gOtaStatus_Success_c;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_ServerDiscoveryHandler(otaClientInfo_t *pOtaClientInfo)
\brief  This function is used to process a ServerDiscovery command.

\param  [in]    pOtaClientInfo    Pointer to client info

\return         otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_ServerDiscoveryHandler
(
    otaClientInfo_t *pOtaClientInfo
)
{
    return OtaServerStandalone_ServerDiscoveryHandler(pOtaClientInfo);
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_CoapSendImageNotify(otaServer_ImageNotify_t *pThciImageNotify,
                                                         otIp6Address *pDestAddr)
\brief  This function is used to send multicast a coap Image notify command to all thread nodes

\param  [in]   pThciImageNotify    Pointer to image notify message
\param  [in]   pDestAddr           Pointer to IPv6 address

\return        otaStatus_t         Status of the operation
 ***************************************************************************************************/
static otaStatus_t OtaServer_CoapSendImageNotify
(
    otaServer_ImageNotify_t *pImageNotify,
    otIp6Address *pDestAddr
)
{
    otaStatus_t status = gOtaStatus_Failed_c;
    uint16_t fragmentSize = gOtaMaxBlockDataSize_c;
    otCoapType coapType = OT_COAP_TYPE_NON_CONFIRMABLE;
    otCoapCode coapCode = OT_COAP_CODE_POST;
    otError error = OT_ERROR_NONE;

    otMessage *pMsg = otCoapNewMessage(mOtaServerSetup.pOtInstance, NULL);

    if(pMsg)
    {
        otMessageInfo messageInfo = {0};
        otaServerCmd_ImageNotify_t imageNotify;

        otCoapMessageInit(pMsg, coapType, coapCode);
        otCoapMessageGenerateToken(pMsg, 4);

        /* Complete command */
        imageNotify.commandId = gOtaCmd_ImageNotify_c;
        imageNotify.transferType = mOtaServerSetup.transferType;
        memcpy(&imageNotify.manufacturerCode, &pImageNotify->manufacturerCode,
                    sizeof(otaServer_ImageNotify_t) - 2);
        memcpy(&imageNotify.serverDownloadPort[0], (void *)&mOtaServerSetup.downloadPort, sizeof(uint16_t));
        memcpy(&imageNotify.fragmentSize[0], &fragmentSize, sizeof(imageNotify.fragmentSize));

        error = otCoapMessageAppendUriPathOptions(pMsg, OTA_SERVER_URI_PATH);
        error = otCoapMessageSetPayloadMarker(pMsg);
        error = otMessageAppend(pMsg, (const void *)&imageNotify, sizeof(imageNotify));
        
        memset(&messageInfo, 0, sizeof(messageInfo));

        messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;
        memcpy(&messageInfo.mSockAddr, GET_OTA_ADDRESS(mOtaServerSetup.pOtInstance), sizeof(otIp6Address));
        memcpy(&messageInfo.mPeerAddr, pDestAddr, sizeof(otIp6Address));
        
        error = otCoapSendRequest(mOtaServerSetup.pOtInstance, pMsg, &messageInfo, NULL, NULL);

        if ((error != OT_ERROR_NONE) && (pMsg != NULL))
        {
            otMessageFree(pMsg);
        }
        else
        {
            status = gOtaStatus_Success_c;
        }
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_CoapSendRsp(otaClientInfo_t *pOtaClientInfo, uint8_t *pData, uint32_t dataLen)
\brief  This function is used to send a coap response to a OTA client node.

\param  [in]   pOtaClientInfo    Pointer to client info
\param  [in]   pData             Pointer to data
\param  [in]   dataLen           Payload length

\return        otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_CoapSendRsp
(
    otaClientInfo_t *pOtaClientInfo,
    uint8_t *pData,
    uint32_t dataLen
)
{
    otaStatus_t status = gOtaStatus_Failed_c;
    otCoapType coapType = OT_COAP_TYPE_NON_CONFIRMABLE;
    otCoapCode coapCode = OT_COAP_CODE_POST;
    otError error = OT_ERROR_NONE;
    otMessage *pMsg = otCoapNewMessage(mOtaServerSetup.pOtInstance, NULL);

    if(pMsg)
    {
        otMessageInfo messageInfo = {0};

        otCoapMessageInit(pMsg, coapType, coapCode);
        otCoapMessageGenerateToken(pMsg, 4);

        /* Complete command */
        error = otCoapMessageAppendUriPathOptions(pMsg, OTA_SERVER_URI_PATH);
        error = otCoapMessageSetPayloadMarker(pMsg);
        error = otMessageAppend(pMsg, (const void *)pData, dataLen);

        memset(&messageInfo, 0, sizeof(messageInfo));

        messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;
        memcpy(&messageInfo.mSockAddr, GET_OTA_ADDRESS(mOtaServerSetup.pOtInstance), sizeof(otIp6Address));
        memcpy(&messageInfo.mPeerAddr, &pOtaClientInfo->remoteAddr, sizeof(otIp6Address));

        error = otCoapSendRequest(mOtaServerSetup.pOtInstance, pMsg, &messageInfo, NULL, NULL);

        if ((error != OT_ERROR_NONE) && (pMsg != NULL))
        {
            otMessageFree(pMsg);
        }
        else
        {
            status = gOtaStatus_Success_c;
        }
    }

    free((void *)pOtaClientInfo);

    return status;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_SocketSendRsp(otaClientInfo_t *pOtaClientInfo, uint8_t *pData, uint32_t dataLen)
\brief  This function is used to send a socket response to a OTA client node.

\param  [in]   pOtaClientInfo    Pointer to client info
\param  [in]   pData             Pointer to data
\param  [in]   dataLen           Payload length

\return        otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_SocketSendRsp
(
    otaClientInfo_t *pOtaClientInfo,
    uint8_t *pData,
    uint32_t dataLen
)
{
    otMessageInfo messageInfo;
    otMessage *message = NULL;
    otError error = OT_ERROR_NONE;

    /* Set remote address and local port */
    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerPort = pOtaClientInfo->port;
    memcpy(&messageInfo.mPeerAddr, &pOtaClientInfo->remoteAddr, sizeof(otIp6Address));

    message = otUdpNewMessage(mOtaServerSetup.pOtInstance, NULL);
    error = otMessageAppend(message, (const void *)pData, dataLen);
    error = otUdpSend(mOtaServerSetup.pOtaUdpSrvSocket, message, &messageInfo);

    if (error != OT_ERROR_NONE && message != NULL)
    {
        otMessageFree(message);
    }

    free((void *)pOtaClientInfo);

    return gOtaStatus_Success_c;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_CoapSendRspWaitAbortData(otaClientInfo_t *pOtaClientInfo, uint8_t status,
                                                              uint32_t delayInMs)
\brief  This function is used to send a Query Image Rsp command to the OTA client node using
        a status != Success

\param  [in]   pOtaClientInfo    Pointer to client info
\param  [in]   status            Query image response status != Success
\param  [in]   delayInMs         Delay in milliseconds

\return        otaStatus_t       Status of the operation
 ***************************************************************************************************/
static otaStatus_t OtaServer_CoapSendRspWaitAbortData
(
    otaClientInfo_t *pOtaClientInfo,
    uint8_t status,
    uint32_t delayInMs
)
{
    otaStatus_t result = gOtaStatus_Failed_c;

    if (status != gOtaFileStatus_Success_c)
    {
        /* All busy / abort responses have the same structure as queryRsp*/
        otaCmd_QueryImageRsp_t queryRsp = {0};
        uint8_t len = 2 + sizeof(otaCmd_QueryImageRspWait_t);
        uint8_t otaCommand = *pOtaClientInfo->pData;
        uint32_t timeInMs = otPlatAlarmMilliGetNow();

        queryRsp.commandId = gOtaCmd_QueryImageRsp_c;

        switch (otaCommand)
        {
            case gOtaCmd_BlockReq_c:
                queryRsp.commandId = gOtaCmd_BlockRsp_c;
                break;

            case gOtaCmd_QueryImageReq_c:
                queryRsp.commandId = gOtaCmd_QueryImageRsp_c;
                break;

            case gOtaCmd_UpgradeEndReq_c:
                queryRsp.commandId = gOtaCmd_UpgradeEndRsp_c;
                break;

            default:
                free((void *)pOtaClientInfo);
                return gOtaStatus_InvalidParam_c;
        }

        queryRsp.status = status;
        memcpy(&queryRsp.data.wait.currentTime, &timeInMs, sizeof(uint32_t));
        timeInMs = timeInMs + delayInMs;
        memcpy(&queryRsp.data.wait.requestTime, &timeInMs, sizeof(uint32_t));

        if (gOtaStatus_Success_c == OtaServer_CoapSendRsp(pOtaClientInfo, (uint8_t *)&queryRsp, len))
        {
            result = gOtaStatus_Success_c;
        }
    }
    else
    {
        free((void *)pOtaClientInfo);
    }

    return result;
}

/*!*************************************************************************************************
\private
\fn     static bool_t OtaServer_IsClientValid(uint16_t clientId)
\brief  This function is used to validate client ID and add it to percentage list.

\param  [in]   clientId    Client ID

\return        bool_t    TRUE - client valid, FALSE - client is in the abort list
 ***************************************************************************************************/
static bool_t OtaServer_IsClientValid
(
    uint16_t clientId
)
{
    bool_t result = false;
    uint8_t i;

    for (i = 0; i < gOtaServer_MaxSimultaneousClients_c; i++)
    {
        if (mOtaServerPercentageInformation.unicastEntry[i].clientId == clientId)
        {
            result = true;
            break;
        }
    }

    if (result == false)
    {
        for (i = 0; i < gOtaServer_MaxSimultaneousClients_c; i++)
        {
            if (mOtaServerPercentageInformation.unicastEntry[i].clientId == gOtaServer_InvalidClientId_c)
            {
                mOtaServerPercentageInformation.unicastEntry[i].clientId = clientId;
                mOtaServerPercentageInformation.unicastEntry[i].percentage = 0;
                result = true;
                break;
            }
        }
    }

    return result;
}

/*!*************************************************************************************************
\private
\fn     static bool_t OtaServer_RemoveClientFromPercentageInfo(uint16_t clientId)
\brief  This function is used to remove a client ID from percentage information list.

\param [in]   clientId    Client ID

\return       bool_t      TRUE - client removed, FALSE - otherwise
 ***************************************************************************************************/
static bool_t OtaServer_RemoveClientFromPercentageInfo
(
    uint16_t clientId
)
{
    bool_t result = false;

    for (uint8_t i = 0; i < gOtaServer_MaxSimultaneousClients_c; i++)
    {
        if (mOtaServerPercentageInformation.unicastEntry[i].clientId == clientId)
        {
            mOtaServerPercentageInformation.unicastEntry[i].clientId = gOtaServer_InvalidClientId_c;
            mOtaServerPercentageInformation.unicastEntry[i].percentage = 0;
            result = true;
            break;
        }
    }

    return result;
}

/*!*************************************************************************************************
\private
\fn     static void OtaServer_ResetPercentageInfo(void)
\brief  This function is resets the percentage information.
 ***************************************************************************************************/
static void OtaServer_ResetPercentageInfo
(
    void
)
{
    mOtaServerPercentageInformation.multicastPercentage = 0;
    mOtaServerPercentageInformation.otaType = 0xFF;

    for (uint8_t i = 0; i < gOtaServer_MaxSimultaneousClients_c; i++)
    {
        mOtaServerPercentageInformation.unicastEntry[i].clientId = gOtaServer_InvalidClientId_c;
        mOtaServerPercentageInformation.unicastEntry[i].percentage = 0;
    }
}

/*************************************************************************************************
*
*  OTA Server Standalone functions
*
**************************************************************************************************/
/*!*************************************************************************************************
\private
\fn     static void OtaServer_InitStandaloneOpMode(void)
\brief  Initialize ota server standalone operation mode.
***************************************************************************************************/
static void OtaServer_InitStandaloneOpMode
(
    void
)
{
    bool_t imageAvailable = false;
    uint8_t index = 0;
    otaFileHeader_t otaHeader;
    FILE *pFile = NULL;

    /* init external memory */
    pFile = fopen(binary_file_path,"rb");

    /* process OTA header information */
    while ((index < gOtaServer_MaxOtaImages_c) && (pFile != NULL))
    {
        uint32_t fileIdentifier;

        /* Read OTA Header */
        if (fread((void *)&otaHeader, sizeof(otaFileHeader_t), 1, pFile))
        {
            memcpy(&fileIdentifier, otaHeader.fileIdentifier, sizeof(fileIdentifier));

            if (fileIdentifier == gOtaFileIdentifierNo_c)
            {
                uint16_t manufCode, imageType;
                uint32_t fileVersion, fileSize;

                memcpy(&fileVersion, &otaHeader.fileVersion, sizeof(uint32_t));
                memcpy(&imageType, &otaHeader.imageType, sizeof(uint16_t));
                memcpy(&manufCode, &otaHeader.manufacturerCode, sizeof(uint16_t));
                memcpy(&fileSize, &otaHeader.totalImageSize, sizeof(uint32_t));
                
                index = OtaServerStandalone_KeepImageInfo(manufCode, imageType, fileVersion, fileSize);

                if (index < gOtaServer_MaxOtaImages_c)
                {
                    mOtaServerTempImageIdx = index;
                    mOtaServerImageList[index].isValidEntry = true;
                    imageAvailable = true;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            /* ignore other data */
            break;
        }
    }

    if (pFile != NULL)
    {
        fclose(pFile);
    }

    mOtaServerSetup.isActive = imageAvailable;
}

/*!*************************************************************************************************
\private
\fn     static uint8_t OtaServerStandalone_ValidateImage(uint16_t manufCode, uint16_t imageType,
                                                         uint32_t fileVersion, bool_t serialProtocol)
\brief  Validate image by checking internal table.

\param  [in]    manufCode         Manufacturer code
        [in]    imageType         Image type
        [in]    fileVersion       File version
        [in]    serialProtocol    Image validation is required for the ota Server serial protocol

\return         uint8_t           Image index if success, otherwise returns gOtaServer_MaxOtaImages_c
***************************************************************************************************/
static uint8_t OtaServerStandalone_ValidateImage
(
    uint16_t manufCode,
    uint16_t imageType,
    uint32_t fileVersion,
    bool_t serialProtocol
)
{
    uint8_t result = gOtaServer_MaxOtaImages_c;

    /* validate internal list */
    for (uint8_t i = 0; i < gOtaServer_MaxOtaImages_c; i++)
    {
        if ((manufCode == mOtaServerImageList[i].manufCode) &&
                (imageType == mOtaServerImageList[i].imageType) &&
                (mOtaServerImageList[i].isValidEntry))
        {
            if (((fileVersion ==  mOtaServerImageList[i].fileVersion) && (mOtaServerSetup.fileVersionPolicy & gOtaFileVersionPolicies_Reinstall_c)) ||
                    ((fileVersion <  mOtaServerImageList[i].fileVersion) && (mOtaServerSetup.fileVersionPolicy & gOtaFileVersionPolicies_Upgrade_c)) ||
                    ((fileVersion >  mOtaServerImageList[i].fileVersion) && (mOtaServerSetup.fileVersionPolicy & gOtaFileVersionPolicies_Downgrade_c)) ||
                    ((serialProtocol) && (fileVersion ==  mOtaServerImageList[i].fileVersion)) || (fileVersion == 0xFFFFFFFF))
            {
                result = i;
                break;
            }
        }
    }

    return result;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServerStandalone_QueryImageReqHandler(otaClientInfo_t *pOtaClientInfo)
\brief  This function is used to process a QueryImageReq command.

\param  [in]    pOtaClientInfo    Pointer to client info

\return         otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServerStandalone_QueryImageReqHandler
(
    otaClientInfo_t *pOtaClientInfo
)
{
    otaStatus_t status = gOtaStatus_Failed_c;
    uint8_t index = 0;
    otaCmd_QueryImageReq_t *pQueryImgReq = (otaCmd_QueryImageReq_t *)pOtaClientInfo->pData;

    index = OtaServerStandalone_ValidateImage((uint16_t)(pQueryImgReq->manufacturerCode[0] + (pQueryImgReq->manufacturerCode[1] << 8)),
            (uint16_t)(pQueryImgReq->imageType[0] + (pQueryImgReq->imageType[1] << 8)),
            (uint32_t)(pQueryImgReq->fileVersion[0] + (pQueryImgReq->fileVersion[1] << 8) +
                       (pQueryImgReq->fileVersion[2] << 16) + (pQueryImgReq->fileVersion[3] << 24)), false);

    if (index < gOtaServer_MaxOtaImages_c)
    {
        otaCmd_QueryImageRsp_t queryImgRsp = {0};
        /* image size */
        uint8_t len = sizeof(otaCmd_QueryImageRsp_t) - sizeof(queryImgRsp.data);

        queryImgRsp.commandId = gOtaCmd_QueryImageRsp_c;
        queryImgRsp.status = gOtaFileStatus_Success_c;
        memcpy(&queryImgRsp.data.success.manufacturerCode, &mOtaServerImageList[index].manufCode, sizeof(uint16_t));
        memcpy(&queryImgRsp.data.success.fileVersion, &mOtaServerImageList[index].fileVersion, sizeof(uint32_t));
        memcpy(&queryImgRsp.data.success.imageType, &mOtaServerImageList[index].imageType, sizeof(uint16_t));
        memcpy(&queryImgRsp.data.success.fileSize, &mOtaServerImageList[index].fileSize, sizeof(uint32_t));
        memcpy(&queryImgRsp.data.success.serverDownloadPort, &mOtaServerSetup.downloadPort, sizeof(uint16_t));

        len += sizeof(otaCmd_QueryImageRspSuccess_t);
        (void)OtaServer_CoapSendRsp(pOtaClientInfo, (uint8_t *)&queryImgRsp, len);
        status = gOtaStatus_Success_c;
    }
    else
    {
        /* packet in progress -  send back a Query Image Rsp command with status no image available */
        OtaServer_CoapSendRspWaitAbortData(pOtaClientInfo, gOtaFileStatus_NoImageAvailable_c, 0);
        status = gOtaStatus_Success_c;
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static void OtaServer_AddPercentageInfoPerClient(uint16_t clientId, uint8_t percent)
\brief  This function is used to process a BlockRequest command.

\param  [in]    clientId    Client ID
\param  [in]    percent     Unicast OTAP percent done
***************************************************************************************************/
static void OtaServer_AddPercentageInfoPerClient
(
    uint16_t clientId,
    uint8_t percent
)
{
    for (int i = 0; i < gOtaServer_MaxSimultaneousClients_c; i++)
    {
        if (mOtaServerPercentageInformation.unicastEntry[i].clientId == clientId)
        {
            mOtaServerPercentageInformation.unicastEntry[i].percentage = percent;
            break;
        }
    }
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServerStandalone_BlockReqHandler(otaClientInfo_t *pOtaClientInfo)
\brief  This function is used to process a BlockRequest command.

\param  [in]    pOtaClientInfo    Pointer to client info

\return         otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServerStandalone_BlockReqHandler
(
    otaClientInfo_t *pOtaClientInfo
)
{
    otaStatus_t status = gOtaStatus_Failed_c;
    uint8_t index = gOtaServer_MaxOtaImages_c;
    otaCmd_BlockReq_t *pBlockReq = (otaCmd_BlockReq_t *)pOtaClientInfo->pData;
    index = OtaServerStandalone_ValidateImage((uint16_t)(pBlockReq->manufacturerCode[0] + (pBlockReq->manufacturerCode[1] << 8)),
            (uint16_t)(pBlockReq->imageType[0] + (pBlockReq->imageType[1] << 8)),
            (uint32_t)(pBlockReq->fileVersion[0] + (pBlockReq->fileVersion[1] << 8) +
                       (pBlockReq->fileVersion[2] << 16) + (pBlockReq->fileVersion[3] << 24)), false);

    if (index < gOtaServer_MaxOtaImages_c)
    {
        otaCmd_BlockRsp_t *pBlockRsp = NULL;
        uint32_t respLength = 0;
        uint32_t len =  pBlockReq->maxDataSize;
        uint32_t imageOffset = (uint32_t)(pBlockReq->fileOffset[0] + (pBlockReq->fileOffset[1] << 8) +
                                          (pBlockReq->fileOffset[2] << 16) + (pBlockReq->fileOffset[3] << 24));

        if ((mOtaServerImageList[index].fileSize - imageOffset) < len)
        {
            len = mOtaServerImageList[index].fileSize - imageOffset;
        }

        respLength = 2 + sizeof(otaCmd_BlockRspSuccess_t) - 1 + len;
        pBlockRsp = (otaCmd_BlockRsp_t *)calloc(1, respLength);

        if (pBlockRsp)
        {
            uint32_t addr = imageOffset + mOtaServerImageList[index].imageAddr;
            FILE *pFile = fopen(binary_file_path,"rb");
            fseek(pFile, addr, SEEK_SET);

            status = gOtaStatus_Success_c;
            pBlockRsp->commandId = gOtaCmd_BlockRsp_c;
            pBlockRsp->status = gOtaFileStatus_Success_c;
            memcpy(pBlockRsp->data.success.fileVersion, pBlockReq->fileVersion, sizeof(uint32_t));
            memcpy(pBlockRsp->data.success.fileOffset, &imageOffset, sizeof(uint32_t));
            pBlockRsp->data.success.dataSize = len;

            if(fread((void *)&pBlockRsp->data.success.pData[0], len, 1, pFile) == 0)
            {
                (void)OtaServer_CoapSendRspWaitAbortData(pOtaClientInfo, gOtaFileStatus_Abort_c, 0);
            }
            else
            {
                uint16_t clientId = (pOtaClientInfo->remoteAddr.mFields.m8[14] << 8) + pOtaClientInfo->remoteAddr.mFields.m8[15];
                uint8_t percent = 0;

                OtaServer_SocketSendRsp(pOtaClientInfo, (uint8_t *)pBlockRsp, respLength);

                //Calculate percentage of image already sent
                percent = (uint8_t)(((imageOffset + len) * 100) / mOtaServerImageList[index].fileSize);
                mOtaServerPercentageInformation.otaType = mOtaServerSetup.transferType;

                if (mOtaServerSetup.transferType == gOtaMulticast_c)
                {
                    //Percentage of sent packages over multicast OTA
                    mOtaServerPercentageInformation.multicastPercentage = percent;
                }
                else
                {
                    //Percentage of sent packages for a unicast client
                    OtaServer_AddPercentageInfoPerClient(clientId, percent);
                }
            }

            free(pBlockRsp);

            if(pFile != NULL)
            {
                fclose(pFile);    
            }
        }
    }
    else
    {
        /* image is not available, abort current session */
        status = gOtaStatus_Success_c;
        (void)OtaServer_CoapSendRspWaitAbortData(pOtaClientInfo, gOtaFileStatus_Abort_c, 0);
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServerStandalone_ServerDiscoveryHandler(otaClientInfo_t *pOtaClientInfo)
\brief  This function is used to process a ServerDiscovery command.

\param  [in]    pOtaClientInfo    Pointer to OTA client info

\return         otaStatus_t       Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServerStandalone_ServerDiscoveryHandler
(
    otaClientInfo_t *pOtaClientInfo
)
{
    otaStatus_t status = gOtaStatus_Success_c;
    uint8_t index = gOtaServer_MaxOtaImages_c;
    otaCmd_ServerDiscovery_t *pCmdData = (otaCmd_ServerDiscovery_t *)pOtaClientInfo->pData;

    index = OtaServerStandalone_ValidateImage((uint16_t)(pCmdData->manufacturerCode[0] + (pCmdData->manufacturerCode[1] << 8)),
            (uint16_t)(pCmdData->imageType[0] + (pCmdData->imageType[1] << 8)),
            0xFFFFFFFF, false);

    if (index < gOtaServer_MaxOtaImages_c)
    {
        /* send back a unicast image notify command */
        status = OtaServer_SendImageNotifty(NULL, &pOtaClientInfo->remoteAddr);
    }

    free(pOtaClientInfo);

    return status;
}

/*!*************************************************************************************************
\private
\fn     static uint8_t OtaServerStandalone_KeepImageInfo(uint16_t manufCode, uint16_t imageType,
                                                        uint32_t fileVersion, uint32_t fileSize)
\brief  This function is used to store image information in local table.

\param  [in]    manufCode      Manufacturer code
        [in]    imageType      Image type
        [in]    fileVersion    File version
        [in]    imageSize      Image size

\return         uint8_t        Image index if success, otherwise returns gOtaServer_MaxOtaImages_c
***************************************************************************************************/
static uint8_t OtaServerStandalone_KeepImageInfo
(
    uint16_t manufCode,
    uint16_t imageType,
    uint32_t fileVersion,
    uint32_t fileSize
)
{
    uint8_t result = gOtaServer_MaxOtaImages_c;
    uint32_t imageAddrOffset = 0;

    for (uint8_t i = 0; i < gOtaServer_MaxOtaImages_c; i++)
    {
        if (mOtaServerImageList[i].isValidEntry)
        {
            /* entry is valid, update image offset */
            imageAddrOffset += mOtaServerImageList[i].fileSize;
        }
        else
        {
            /* entry is free */
            mOtaServerImageList[i].fileVersion = fileVersion;
            mOtaServerImageList[i].imageAddr = imageAddrOffset;
            mOtaServerImageList[i].imageType = imageType;
            mOtaServerImageList[i].manufCode = manufCode;
            mOtaServerImageList[i].fileSize = fileSize;
            mOtaServerImageList[i].isValidEntry = false; /* True when the download is completed */
            result = i;
            break;
        }
    }

    return result;
}

/*************************************************************************************************
*
*  OTA Server Serial Protocol callbacks
*
**************************************************************************************************/

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_HandleBlockSocket(bool_t onOff)
\brief  This function is used to handle block sockets.

\param  [in]   onOff          If TRUE, create and bind and if FALSE, close socket

\return        otaStatus_t    Status of the operation
 ***************************************************************************************************/
static otaStatus_t OtaServer_HandleBlockSocket
(
    bool_t onOff
)
{
    otaStatus_t status = gOtaStatus_Success_c;

    if (true == onOff)
    {
        if (mOtaServerSetup.pOtaUdpSrvSocket == NULL)
        {
            otError    error;
            otSockAddr portAddr;
            
            /* Set Ota Server local address and local port */
            memset(&portAddr, 0, sizeof(portAddr));
            portAddr.mPort = OTA_SERVER_DEFAULT_PORT;
            mOtaServerSetup.pOtaUdpSrvSocket = &mOtaUdpSrvSocket;

            /* Open Ota Server UDP socket */
            error = otUdpOpen(mOtaServerSetup.pOtInstance, mOtaServerSetup.pOtaUdpSrvSocket, OtaClient_UdpServerService, NULL);

            if (error != OT_ERROR_NONE)
            {
                status = gOtaStatus_NoUdpSocket_c;
            }

            if(status == gOtaStatus_Success_c)
            {
                error = otUdpBind(mOtaServerSetup.pOtaUdpSrvSocket, &portAddr);

                if (error != OT_ERROR_NONE)
                {
                    otUdpClose(mOtaServerSetup.pOtaUdpSrvSocket);
                    mOtaServerSetup.pOtaUdpSrvSocket = NULL;
                    status = gOtaStatus_Failed_c;
                }
            }
        }
        else
        {
            status = gOtaStatus_Success_c;
        }
    }
    else
    {
        if (mOtaServerSetup.pOtaUdpSrvSocket != NULL)
        {
            otUdpClose(mOtaServerSetup.pOtaUdpSrvSocket);
            mOtaServerSetup.pOtaUdpSrvSocket = NULL;
        }
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static void OtaServer_MulticastTimeoutCb(void *pParam)
\brief  This function is used for the ota server multicast timer callback.

\param  [in]    pParam    Generic pointer containing information dependent on the multicast state.
***************************************************************************************************/
static void OtaServer_MulticastTimeoutCb
(
    void *pParam
)
{
    OtaServer_MulticastMngr(pParam);
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_InitMulticast(void *pParam)
\brief  This function is used for the initialization of the OTA Multicast mechanism.

\param  [in]    pParam         Pointer to the otaServer_ImageNotify_t structure containing
                               the Image Notification information required for initialization.

\return         otaStatus_t    Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_InitMulticast
(
    void *pParam
)
{
    otaServer_ImageNotify_t *pImageNotify;
    otaStatus_t status = gOtaStatus_Failed_c;

    if (pParam)
    {
        uint32_t delay;

        pImageNotify = (otaServer_ImageNotify_t *)pParam;
        memcpy(&mOtaServerSetup.multicastManufacturerCode, pImageNotify->manufacturerCode, sizeof(uint16_t));
        memcpy(&mOtaServerSetup.multicastImageType, pImageNotify->imageType, sizeof(uint16_t));
        memcpy(&mOtaServerSetup.multicastFileVersion, pImageNotify->fileVersion, sizeof(uint32_t));
        memset(&mOtaServerSetup.ackBitmask, 0xFF, sizeof(mOtaServerSetup.ackBitmask));
        mOtaServerSetup.currentWindowOffset = 0;
        memcpy(&mOtaServerSetup.multicastImageSize, pImageNotify->fileSize, sizeof(mOtaServerSetup.multicastImageSize));
        mOtaServerSetup.multicastNoOfImgNtf = gOtaServer_MulticastImgNtfRetransmissions_c;
        mOtaServerSetup.multicastNoOfBlockRsp = gOtaServer_MulticastNoOfBlockRsps_c;
        mOtaServerSetup.multicastNoOfWindowRetries = gOtaServer_MulticastWindowRetries_c;

        if (mOtaServerSetup.multicastNoOfImgNtf)
        {
            mOtaServerSetup.multicastState = gOtaServerMulticastState_SendImgNtf_c;
            delay = gOtaServer_MulticastImgNtfInterval_c;
        }
        else
        {
            mOtaServerSetup.multicastState = gOtaServerMulticastState_GenBlockReq_c;
            delay = gOtaServer_MulticastInterval_c;
            free(pParam);
            pParam = NULL;
        }

        OtaServer_SetTimeCallback(OtaServer_MulticastTimeoutCb, pParam, delay);
        status = gOtaStatus_Success_c;
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_SendImgNtf(void *pParam)
\brief  This function is used for the retransmission of Image Notification commands.

\param  [in]    pParam          Pointer to the otaServer_ImageNotify_t structure containing
                                the Image Notification information used for the OTA command.

\return         otaStatus_t     Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_SendImgNtf
(
    void *pParam
)
{
    otaStatus_t status = gOtaStatus_Failed_c;

    if (pParam)
    {
        uint32_t delay;
        otaServer_ImageNotify_t *pImageNotify = pParam;
        status = OtaServer_CoapSendImageNotify(pImageNotify, &in6addr_realmlocal_allthreadnodes);

        if (status == gOtaStatus_Success_c)
        {
            mOtaServerSetup.multicastNoOfImgNtf--;
        }

        if (mOtaServerSetup.multicastNoOfImgNtf)
        {
            delay = gOtaServer_MulticastImgNtfInterval_c;
        }
        else
        {
            mOtaServerSetup.multicastState = gOtaServerMulticastState_GenBlockReq_c;
            delay = gOtaServer_MulticastInterval_c;
            free(pParam);
            pParam = NULL;
        }

        OtaServer_SetTimeCallback(OtaServer_MulticastTimeoutCb, pParam, delay);

        status = gOtaStatus_Success_c;
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_GenerateBlockReq(void *pParam)
\brief  This function is used for generating a Block Request to the local OTA Server
        that will result in a OTA Block Response sent as a multicast.

\param  [in]    pParam         Pointer to additional data. Reserved for future enhancements. Not used.

\return         otaStatus_t    Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_GenerateBlockReq
(
    void *pParam
)
{
    otaStatus_t status = gOtaStatus_Failed_c;
    otaClientInfo_t *pOtaClientInfo;
    otaCmd_BlockReq_t *pBlockReq;

    /* Calculate the size of the Block Req taking into acount the variable length ACK bitfield. */
    uint8_t size = sizeof(otaClientInfo_t) - 1 + sizeof(otaCmd_BlockReq_t);

    (void)pParam;

    pOtaClientInfo = (otaClientInfo_t *)calloc(1, size);

    if (NULL == pOtaClientInfo)
    {
        status = gOtaStatus_NoMem_c;
    }
    else
    {
        uint32_t fragIdx, imageOffset;
        uint32_t delay = gOtaServer_MulticastBlockRspInterval_c;

        memcpy(&pOtaClientInfo->remoteAddr, &in6addr_realmlocal_allthreadnodes, sizeof(otIp6Address));
        memcpy(&pOtaClientInfo->sourceAddr, GET_OTA_ADDRESS(mOtaServerSetup.pOtInstance), sizeof(otIp6Address));
        pOtaClientInfo->port = OTA_SERVER_DEFAULT_PORT;
        pOtaClientInfo->dataLen = size - sizeof(otaClientInfo_t) + 1;
        pOtaClientInfo->timeStamp = otPlatAlarmMilliGetNow();

        pBlockReq = (otaCmd_BlockReq_t *)pOtaClientInfo->pData;
        pBlockReq->commandId = gOtaCmd_BlockReq_c;
        memcpy(pBlockReq->manufacturerCode, &mOtaServerSetup.multicastManufacturerCode, sizeof(pBlockReq->manufacturerCode));
        memcpy(pBlockReq->imageType, &mOtaServerSetup.multicastImageType, sizeof(pBlockReq->imageType));
        memcpy(pBlockReq->fileVersion, &mOtaServerSetup.multicastFileVersion, sizeof(pBlockReq->fileVersion));
        fragIdx = NWKU_GetFirstBitValue(mOtaServerSetup.ackBitmask, gOtaServer_MulticastWindowSize_c / 8, true);

        imageOffset = mOtaServerSetup.currentWindowOffset + (fragIdx * gOtaMaxBlockDataSize_c);
        memcpy(pBlockReq->fileOffset, &imageOffset, sizeof(pBlockReq->fileOffset));
        pBlockReq->maxDataSize = gOtaMaxBlockDataSize_c;

        /* Send the Block Req to the OTA Server. */
        OtaServer_ClientProcess(pOtaClientInfo);

        if (mOtaServerSetup.multicastNoOfBlockRsp == 0)
        {
            if (fragIdx < gOtaServer_MulticastWindowSize_c)
            {
                /* Last retry. */
                NWKU_ClearBit(fragIdx, mOtaServerSetup.ackBitmask);
                mOtaServerSetup.multicastNoOfBlockRsp = gOtaServer_MulticastNoOfBlockRsps_c;
                /* Get next fragment index */
                fragIdx = NWKU_GetFirstBitValue(mOtaServerSetup.ackBitmask, gOtaServer_MulticastWindowSize_c / 8, true);

                if (fragIdx > (gOtaServer_MulticastWindowSize_c - 1))
                {
                    /* Last retry of last fragment from current window. */
                    mOtaServerSetup.multicastState = gOtaServerMulticastState_WaitForAck_c;
                    delay = gOtaServer_MulticastAckTimeout_c;
                }
            }
        }
        else
        {
            mOtaServerSetup.multicastNoOfBlockRsp--;
        }

        OtaServer_SetTimeCallback(OtaServer_MulticastTimeoutCb, NULL, delay);
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_MulticastUpgradeEnd(void *pParam)
\brief  This function is used for sending the Upgrade End Response command at the end of
        the OTA multicast process.

\param  [in]    pParam         Pointer to additional data. Reserved for future enhancements. Not used.

\return         otaStatus_t    Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_MulticastUpgradeEnd
(
    void *pParam
)
{
    (void)pParam;
    OtaServer_ResetMulticastModule(NULL);

    return gOtaStatus_Success_c;
}

/*!*************************************************************************************************
\private
\fn     static otaStatus_t OtaServer_ProcessAckTimeout(void *pParam)
\brief  This function is used for determining the next steps after the ACK window closes.

\param  [in]    pParam         Pointer to additional data. Reserved for future enhancements. Not used.

\return         otaStatus_t    Status of the operation
***************************************************************************************************/
static otaStatus_t OtaServer_ProcessAckTimeout
(
    void *pParam
)
{
    otaStatus_t status = gOtaStatus_Success_c;
    uint32_t fragIdx;
    uint32_t delay = gOtaServer_MulticastBlockRspInterval_c;

    (void)pParam;

    fragIdx = NWKU_GetFirstBitValue(mOtaServerSetup.ackBitmask, gOtaServer_MulticastWindowSize_c / 8, true);

    /* Check if there are fragments that require retransmission. */
    if ((fragIdx < gOtaServer_MulticastWindowSize_c - 1) && (mOtaServerSetup.multicastNoOfWindowRetries))
    {
        /* Retransmit current window. */
        mOtaServerSetup.multicastNoOfWindowRetries--;
    }
    else
    {
        /* Last window? */
        if (mOtaServerSetup.currentWindowOffset + (gOtaServer_MulticastWindowSize_c * gOtaMaxBlockDataSize_c) >= mOtaServerSetup.multicastImageSize)
        {
            /* Move to the next state */
            delay = gOtaServer_MulticastUpgradeEndDelay_c;
            mOtaServerSetup.multicastState = gOtaServerMulticastState_SendUpgradeEnd_c;
        }
        else
        {
            /* Window completed. Move to the next window. */
            mOtaServerSetup.currentWindowOffset += (gOtaServer_MulticastWindowSize_c * gOtaMaxBlockDataSize_c);

            if (mOtaServerSetup.currentWindowOffset + (gOtaServer_MulticastWindowSize_c * gOtaMaxBlockDataSize_c) <= mOtaServerSetup.multicastImageSize)
            {
                /* Current window is full */
                memset(mOtaServerSetup.ackBitmask, 0xFF, sizeof(mOtaServerSetup.ackBitmask));
            }
            else
            {
                /* Calculate the number of fragments remaining */
                uint8_t fragsInWindow = (mOtaServerSetup.multicastImageSize - mOtaServerSetup.currentWindowOffset) / gOtaMaxBlockDataSize_c;

                if ((mOtaServerSetup.multicastImageSize - mOtaServerSetup.currentWindowOffset) % gOtaMaxBlockDataSize_c != 0)
                {
                    fragsInWindow++;
                }

                /* Set corresponding bits in ack bitmask. */
                for (int i = 0; i < fragsInWindow; i++)
                {
                    NWKU_SetBit(i, mOtaServerSetup.ackBitmask);
                }
            }

            mOtaServerSetup.multicastState = gOtaServerMulticastState_GenBlockReq_c;
            /* Debug only */
            /* Get fragment index. Should be 0, should not fail. */
            fragIdx = NWKU_GetFirstBitValue(mOtaServerSetup.ackBitmask, gOtaServer_MulticastWindowSize_c / 8, true);

        }
    }

    OtaServer_SetTimeCallback(OtaServer_MulticastTimeoutCb, NULL, delay);

    return status;
}

/*!*************************************************************************************************
\private
\fn     static void OtaServer_ResetMulticastModule(void *pParam)
\brief  Reset the OTA Multicast mechanism.

\param  [in]    pParam    Pointer to data
***************************************************************************************************/
static void OtaServer_ResetMulticastModule
(
    void *pParam
)
{
    if (callbackIsSet == true)
    {
        mOtaServerSetup.multicastState = gOtaServerMulticastState_ResetMulticast_c;
    }
    else
    {
        mOtaServerSetup.multicastState = gOtaServerMulticastState_NotInit_c;
        mOtaServerSetup.transferType = gOtaUnicast_c;

        OtaServer_ResetPercentageInfo();
        OtaServer_StopTimeCallback();

        if (pParam)
        {
            free(pParam);
        }
    }
}

/*!*************************************************************************************************
\private
\fn     static void OtaServer_MulticastMngr(void *pParam)
\brief  Ota Multicast state machine.

\param  [in]    pParam    Generic pointer containing state depended information
***************************************************************************************************/
static void OtaServer_MulticastMngr
(
    void *pParam
)
{
    switch (mOtaServerSetup.multicastState)
    {
        case gOtaServerMulticastState_NotInit_c:
            (void)OtaServer_InitMulticast(pParam);
            break;

        case gOtaServerMulticastState_SendImgNtf_c:
            (void)OtaServer_SendImgNtf(pParam);
            break;

        case gOtaServerMulticastState_GenBlockReq_c:
            (void)OtaServer_GenerateBlockReq(pParam);
            break;

        case gOtaServerMulticastState_WaitForAck_c:
            (void)OtaServer_ProcessAckTimeout(pParam);
            break;

        case gOtaServerMulticastState_SendUpgradeEnd_c:
            (void)OtaServer_MulticastUpgradeEnd(pParam);
            break;

        case gOtaServerMulticastState_ResetMulticast_c:
            OtaServer_ResetMulticastModule(pParam);
            break;

        case gOtaServerMulticastState_Idle_c:
            break;
    }
}

/*==================================================================================================
Private debug functions
==================================================================================================*/
