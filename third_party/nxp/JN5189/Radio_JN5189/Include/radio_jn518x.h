/*
* Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2019 NXP
 * All rights reserved.
 *
* SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RADIO_JN518X_H_
#define __RADIO_JN518X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "radio.h"

/****************************************************************************/
/***        Macro/Type Definitions                                        ***/
/****************************************************************************/

#define RADIO_JN518X_VERSION RADIO_VERSION
#define RADIO_JN5189_MAX_RSSI_REPORT RADIO_MAX_RSSI_REPORT
#define RADIO_JN5189_MIN_RSSI_REPORT RADIO_MIN_RSSI_REPORT

#define vRadio_Jn518x_RadioInit vRadio_RadioInit
#define vRadio_Jn518x_RadioDeInit vRadio_RadioDeInit
#define vRadio_Jn518x_Standard_Init vRadio_Standard_Init
#define u8Radio_Jn518x_GetEDfromRSSI u8Radio_GetEDfromRSSI
#define u32Radio_Jn518x_RadioModesAvailable u32Radio_RadioModesAvailable
#define u32Radio_Jn518x_RadioGetVersion u32Radio_RadioGetVersion
#define vRadio_JN518x_Temp_Update vRadio_Temp_Update
#define vRadio_JN518x_ConfigCalFlashUsage vRadio_ConfigCalFlashUsage
#define vRadio_jn518x_Save_ApplicationData_Retention vRadio_Save_ApplicationData_Retention
#define vRadio_jn518x_Restore_Retention_ApplicationData vRadio_Restore_Retention_ApplicationData
#define vRadio_JN518xRecal vRadio_Recal
#define vRadio_Jn518x_RFT1778_bad_crc vRadio_RFT1778_bad_crc
#define vRadio_Jn518x_LockupCheckAndAbortRadio vRadio_LockupCheckAndAbortRadio
#define i16Radio_Jn518x_GetRSSI i16Radio_GetRSSI
#define i16Radio_Jn518x_GetNbRSSISync i16Radio_GetNbRSSISync
#define i16Radio_Jn518x_GetNbRSSISyncCor i16Radio_GetNbRSSISyncCor
#define i8Radio_Jn518x_GetLastPacketRSSI i8Radio_GetLastPacketRSSI
#define i16Radio_Jn518x_BoundRssiValue i16Radio_BoundRssiValue
#define vRadio_Jn518x_BLE_ResetOn vRadio_BLE_ResetOn
#define vRadio_Jn518x_BLE_ResetOff vRadio_BLE_ResetOff
#define vRadio_Jn518x_remove_patch_ISR vRadio_remove_patch_ISR
#define vRadio_Jn518x_SingleRX_AgcReadyPatch vRadio_SingleRX_AgcReadyPatch
#define vRadio_Jn518x_MultiRX_AgcReadyPatch vRadio_MultiRX_AgcReadyPatch
#define vRadio_Jn518x_Enable_AgcReadyPatch vRadio_Enable_AgcReadyPatch;
#define vRadio_Jn518x_Disable_AgcReadyPatch vRadio_Disable_AgcReadyPatch
#define vRadio_Jn518x_SkipXTALInit vRadio_SkipXTALInit
#define vRadio_Jn518x_EnableXTALInit vRadio_EnableXTALInit
#define vRadio_jn518x_ActivateXtal32MRadioBiasing vRadio_ActivateXtal32MRadioBiasing
#define vRadio_jn518x_DisableZBRadio vRadio_DisableZBRadio
#define vRadio_jn518x_DisableBLERadio vRadio_DisableBLERadio
#define vRadio_jn518x_EnableBLEFastTX vRadio_EnableBLEFastTX
#define vRadio_jn518x_DisableBLEFastTX vRadio_DisableBLEFastTX
#define vRadio_jn518x_ZBtoBLE vRadio_ZBtoBLE
#define vRadio_jn518x_BLEtoZB vRadio_BLEtoZB
#define u32Radio_JN518x_Get_Next_Recal_Duration u32Radio_Get_Next_Recal_Duration
#define vRadio_Jn518x_AntennaDiversityTxRxEnable vRadio_AntennaDiversityTxRxEnable
#define vRadio_Jn518x_AntennaDiversityConfigure vRadio_AntennaDiversityConfigure
#define vRadio_Jn518x_AntennaDiversitySwitch vRadio_AntennaDiversitySwitch
#define u8Radio_Jn518x_AntennaDiversityStatus u8Radio_AntennaDiversityStatus



#ifdef __cplusplus
}
#endif

#endif /* __RADIO_JN518X_H_ */
