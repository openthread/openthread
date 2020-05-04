/*
* Copyright 2019-2020 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef _APP_OTA_H
#define _APP_OTA_H

/*!=================================================================================================
\file       app_ota.h
\brief      This is the header file for the ota module
==================================================================================================*/

/*==================================================================================================
Include Files
==================================================================================================*/
#include <openthread/coap.h>
#include <openthread/thread.h>
/*==================================================================================================
Public macros
==================================================================================================*/
#define OTA_CLIENT_URI_PATH                     "otaclient"
#define OTA_SERVER_URI_PATH                     "otaserver"

#ifndef OTA_USE_NWK_DATA
    #define OTA_USE_NWK_DATA                    FALSE
#endif

#if OTA_USE_NWK_DATA
    #define GET_OTA_ADDRESS(otInst)             otThreadGetRloc(otInst)
#else
    #define GET_OTA_ADDRESS(otInst)             otThreadGetMeshLocalEid(otInst)
#endif

/* ota params */
#define gOtaFileIdentifierNo_c                  0x0BEEF11E
#define gOtaManufacturerCode_c                  0x04, 0x10
#define gOtaManufacturerCodeNo_c                0x1004
#define gOtaCurrentImageType_c                  0x00, 0x00
#define gOtaCurrentImageTypeNo_c                0x0000
#define gOtaCurrentFileVersion_c                0x05, 0x40, 0x03, 0x40
#define gOtaCurrentFileVersionNo_c              0x40034005
#define gOtaHardwareVersion_c                   0x21, 0x24
#define gOtaHardwareVersionNo_c                 0x2421

#define gOtaMaxBlockDataSize_c                  64 /* 60 bytes */

#define gOtaServer_MaxSimultaneousClients_c     0x0A

/*==================================================================================================
Public type definitions
==================================================================================================*/

/* ota commands */
typedef enum
{
    gOtaCmd_ImageNotify_c          =       0x00,
    gOtaCmd_QueryImageReq_c,
    gOtaCmd_QueryImageRsp_c,
    gOtaCmd_BlockReq_c,
    gOtaCmd_BlockRsp_c,
    gOtaCmd_UpgradeEndReq_c,
    gOtaCmd_UpgradeEndRsp_c,
    gOtaCmd_ServerDiscovery_c,
    gOtaCmd_Invalid_c              =       0xFF
} gOtaCmd_t;

/*! Ota status */
typedef enum otaStatus_tag
{
    gOtaStatus_Success_c                       = 0x00,
    gOtaStatus_Failed_c                        = 0x01,
    gOtaStatus_InvalidInstance_c               = 0x02,
    gOtaStatus_InvalidParam_c                  = 0x03,
    gOtaStatus_NotPermitted_c                  = 0x04,
    gOtaStatus_NotStarted_c                    = 0x05,
    gOtaStatus_NoMem_c                         = 0x06,
    gOtaStatus_UnsupportedAttr_c               = 0x07,
    gOtaStatus_EmptyEntry_c                    = 0x08,
    gOtaStatus_InvalidValue_c                  = 0x09,
    gOtaStatus_AlreadyStarted_c                = 0x0A,
    gOtaStatus_NoTimers_c                      = 0x0B,
    gOtaStatus_NoUdpSocket_c                   = 0x0C,
	gOtaStatus_FlashError_c                    = 0x0D,
    gOtaStatus_TransferTypeNotSupported_c      = 0x0E,
    gOtaStatus_EntryNotFound_c                 = 0xFF
} otaStatus_t;

/* OTA File Status */
typedef enum otaFileStatus_tag
{
    gOtaFileStatus_Success_c          = 0x00, /* Success Operation */
    gOtaFileStatus_NotAuthorized_c    = 0x7E, /* Server is not authorized to upgrade the client. */
    gOtaFileStatus_Abort_c            = 0x95, /* Failed case when a client or a server decides to abort the upgrade process. */
    gOtaFileStatus_InvalidImage_c     = 0x96, /* Invalid OTA upgrade image. */
    gOtaFileStatus_ServerBusy_c       = 0x97, /* Server is busy, retry later. */
    gOtaFileStatus_NoImageAvailable_c = 0x98, /* No OTA upgrade image available for a particular client. */
    gOtaFileStatus_ImageTooLarge_c    = 0x99, /* Received OTA image is larger than the available storage space. */
    gOtaFileStatus_InvalidOperation_c = 0x9A, /* Client encountered an invalid operation error. */
    gOtaFileStatus_InvalidParameter_c = 0x9B, /* Client encountered an invalid parameter error. */
    gOtaFileStatus_ExtFlashError_c    = 0x9C, /* Client encountered an external flash error. */
    gOtaFileStatus_ClientError_c      = 0x9D /* Generic client error. */
} otaFileStatus_t;

typedef enum otaTransferType_tag
{
    gOtaUnicast_c = 0x00,
    gOtaMulticast_c
} otaTransferType_t;

/* ota command format */
typedef struct otaCommand_tag
{
    uint8_t commandId;
    uint8_t pPayload[1];
} otaCommand_t;

/* ota image notify command format*/
typedef struct otaServerCmd_ImageNotify_tag
{
    uint8_t commandId;
    uint8_t transferType;
    uint8_t manufacturerCode[2];
    uint8_t imageType[2];
    uint8_t imageSize[4];
    uint8_t fileSize[4];
    uint8_t fileVersion[4];
    uint8_t serverDownloadPort[2];
    uint8_t fragmentSize[2];
} otaServerCmd_ImageNotify_t;

/* ota query image req command format*/
typedef struct otaCmd_QueryImageReq_tag
{
    uint8_t commandId;
    uint8_t manufacturerCode[2];
    uint8_t imageType[2];
    uint8_t fileVersion[4];
    uint8_t hardwareVersion[2];
} otaCmd_QueryImageReq_t;

/* ota query image rsp - success */
typedef struct otaCmd_QueryImageRspSuccess_tag
{
    uint8_t manufacturerCode[2];
    uint8_t imageType[2];
    uint8_t fileVersion[4];
    uint8_t fileSize[4];
    uint8_t serverDownloadPort[2];
} otaCmd_QueryImageRspSuccess_t;

/* ota query image rsp - wait */
typedef struct otaCmd_QueryImageRspWait_tag
{
    uint8_t currentTime[4];
    uint8_t requestTime[4];
} otaCmd_QueryImageRspWait_t;

/* ota query image rsp command format*/
typedef struct otaCmd_QueryImageRsp_tag
{
    uint8_t commandId;
    uint8_t status;
    union
    {
        otaCmd_QueryImageRspSuccess_t success;
        otaCmd_QueryImageRspWait_t wait;
    } data;
} otaCmd_QueryImageRsp_t;

/* ota block req command format*/
typedef struct otaCmd_BlockReq_tag
{
    uint8_t commandId;
    uint8_t manufacturerCode[2];
    uint8_t imageType[2];
    uint8_t fileVersion[4];
    uint8_t fileOffset[4];
    uint8_t maxDataSize;
} otaCmd_BlockReq_t;

/* ota block rsp - success */
typedef struct otaCmd_BlockRspSuccess_tag
{
    uint8_t fileVersion[4];
    uint8_t fileOffset[4];
    uint8_t dataSize;
    uint8_t pData[1];
} otaCmd_BlockRspSuccess_t;

/* ota block rsp - wait for data */
typedef struct otaCmd_BlockRspWaitForData_tag
{
    uint8_t currentTime[4];
    uint8_t requestTime[4];
} otaCmd_BlockRspWaitForData_t;

/* ota block rsp command format*/
typedef struct otaCmd_BlockRsp_tag
{
    uint8_t commandId;
    uint8_t status;
    union
    {
        otaCmd_BlockRspSuccess_t success;
        otaCmd_BlockRspWaitForData_t wait;
    } data;
} otaCmd_BlockRsp_t;

/* ota upgrade end req command format*/
typedef struct otaCmd_UpgradeEndReq_tag
{
    uint8_t commandId;
    uint8_t status;
    uint8_t manufacturerCode[2];
    uint8_t imageType[2];
    uint8_t fileVersion[4];
} otaCmd_UpgradeEndReq_t;

/* ota upgrade end rsp - success */
typedef struct otaCmd_UpgradeEndRspSuccess_tag
{
    uint8_t currentTime[4];     /* milliseconds */
    uint8_t upgradeTime[4];     /* milliseconds */
    uint8_t fileVersion[4];
} otaCmd_UpgradeEndRspSuccess_t;

/* ota upgrade end rsp - wait for data */
typedef struct otaCmd_UpgradeEndRspWaitForData_tag
{
    uint8_t currentTime[4];
    uint8_t requestTime[4];
} otaCmd_UpgradeEndRspWaitForData_t;

/* ota upgrade end rsp command format*/
typedef struct otaCmd_UpgradeEndRsp_tag
{
    uint8_t commandId;
    uint8_t status;
    union
    {
        otaCmd_UpgradeEndRspSuccess_t success;
        otaCmd_UpgradeEndRspWaitForData_t wait;
    } data;
} otaCmd_UpgradeEndRsp_t;

/* ota upgrade server discovery command format */
typedef struct otaCmd_ServerDiscovery_tag
{
    uint8_t commandId;
    uint8_t manufacturerCode[2];
    uint8_t imageType[2];
} otaCmd_ServerDiscovery_t;

/* ota file header */
typedef struct otaFileHeader_tag
{
    uint8_t fileIdentifier[4];
    uint8_t headerVersion[2];
    uint8_t headerLength[2];
    uint8_t fieldControl[2];
    uint8_t manufacturerCode[2];
    uint8_t imageType[2];
    uint8_t fileVersion[4];
    uint8_t stackVersion[2];
    uint8_t headerString[32];
    uint8_t totalImageSize[4];
    uint8_t minHwVersion[2];
    uint8_t maxHwVersion[2];
} otaFileHeader_t;

typedef struct otaFileSubElement_tag
{
    uint8_t id[2];
    uint8_t length[4];
} otaFileSubElement_t;

/* OTA Server Serial Protocol */

/* Image notify command format */
typedef struct otaServer_ImageNotify_tag
{
    uint8_t deviceId[2];
    uint8_t manufacturerCode[2];
    uint8_t imageType[2];
    uint8_t imageSize[4];
    uint8_t fileSize[4];
    uint8_t fileVersion[4];
} otaServer_ImageNotify_t;

/* OTA unicast and multicast finished percentage information structures */
typedef struct otaServerUnicastClientEntry_tag
{
    uint16_t clientId;
    uint8_t percentage;
} otaServerUnicastClientEntry_t;

typedef struct otaServerPercentageInfo_tag
{
	uint8_t otaType;
	uint8_t multicastPercentage;
	otaServerUnicastClientEntry_t unicastEntry[gOtaServer_MaxSimultaneousClients_c];
} otaServerPercentageInfo_t;

/* ota server operation mode */
typedef enum
{
    gOtaServerOpMode_Reserved_c = 0,
    gOtaServerOpMode_Standalone_c,           /* requires an external memory or a reserved region of internal MCU flash to keep the client image */
    gOtaServerOpMode_Dongle_c,               /* without internal/external memory capacity */
} otaServerOpMode_t;

/*==================================================================================================
Public global variables declarations
==================================================================================================*/

extern otCoapResource gOTA_CLIENT_URI_PATH;
extern otCoapResource gOTA_SERVER_URI_PATH;

/*==================================================================================================
Public function prototypes
==================================================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

/*!*************************************************************************************************
\public
\fn  otaStatus_t OtaServerInit(taskMsgQueue_t *pMsgQueue)
\brief  Initialize OTA server application

\param  [in]    pMsgQueue   Pointer to task message queue

\return   otaStatus_t
 ***************************************************************************************************/
otaStatus_t OtaServerInit(otInstance *pOtInstance);

/*!*************************************************************************************************
\public
\fn  otaStatus_t OtaServer_StartOta()
\brief  Start OTA process

\param  [in]    otaType        Type of OTA process (unicast or multicast)
\param  [in]    pFilePath      Path to binary

\return         otaStatus_t    Status of the operation
 ***************************************************************************************************/
otaStatus_t OtaServer_StartOta(uint8_t otaType, const char *pFilePath);

/*!*************************************************************************************************
\fn     otaResult_t OtaServer_StopOta(void)
\brief  Process Stop OTA command received from an external application.

\return         otaStatus_t    Status of the operation
***************************************************************************************************/
otaStatus_t OtaServer_StopOta(void);

/*!*************************************************************************************************
\fn     void OtaServer_CheckTime(void)
\brief  This function is used to check if a timer callback for OTA needs to be called.
***************************************************************************************************/
void OtaServer_CheckTime(void);

/*!*************************************************************************************************
\fn     void OtaServer_GetOtaStatus(otaServerPercentageInfo_t *pData)
\brief  This function is used to check the status of the OTA transfer.

\param  [in]    pData        Pointer to output structure
***************************************************************************************************/
void OtaServer_GetOtaStatus(otaServerPercentageInfo_t *pData);

#ifdef __cplusplus
}
#endif

/*================================================================================================*/
#endif  /*  _APP_OTA_H */
