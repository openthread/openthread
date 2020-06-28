/*
 * @brief Radio driver
 *
 * @note
 * Copyright 2019 NXP
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef __RADIO_H_
#define __RADIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/****************************************************************************/
/***        Macro/Type Definitions                                        ***/
/****************************************************************************/

/****************************************************************************/
/***    Radio driver version (XYYY): X major version, YYY minor version   ***/
/****************************************************************************/
#define RADIO_VERSION (2088)

/****************************************************************************/
/***    Radio calibration data record version                             ***/
/****************************************************************************/
#define RADIO_CAL_RECORD_VERSION (1001)

/****************************************************************************/
/***    Radio driver int time values									  ***/
/***	RADIO_INIT_TIME_WITH_KMOD_INIT_US full init time with Kmod cal    ***/
/***	This init is done only when KMOD cal results are not available    ***/
/***	in flash for the current initialization temperature. As results   ***/
/***	are valid in a [-40...+40C] temperature range around the cal      ***/
/***	temperature, only 4 full calibration are needed in the whole live ***/
/***	of the chip and this init time is quite never reached.            ***/
/***	RADIO_INIT_TIME_WITHOUT_KMOD_INIT_US full init time without Kmod  ***/
/***	calibration. This is the typical coldstart radio init time		  ***/
/***	RADIO_INIT_TIME_WITH_RETENTION_US init time when using retention  ***/
/***	In that case calibrations are not run and parameters kept in      ***/
/***	in retention flops are used instead. This is a the typical warm   ***/
/***	start init time.                                                  ***/
/****************************************************************************/

#define RADIO_FULL_INIT_FIRST_TIME (80300) // no record available and all cal to be done because TCur far from TCAL ATE
#define RADIO_FULL_INIT_TIME (85500) // record available but all cal to be done because TCur far from TCAL ATE
#define RADIO_INIT_ALL_CAL_IN_FLASH (850) // record available, no cal to be done and TCur close to TCAL ATE
#define RADIO_INIT_NO_DCO_CAL_IN_FLASH (13500) // no record available, DCO cal only and TCur close to TCAL ATE
#define RADIO_INIT_TIME_WITH_RETENTION_US (250) // can use retention values
#define RADIO_RECAL_TIME_ALL_CAL_IN_FLASH_US (750) // recal with cal available in flash for TCur
#define RADIO_RECAL_TIME_NO_CAL_IN_FLASH_US (86800) // recal with cal data not available in flash, all call to be done
#define RADIO_RECAL_TIME_NO_DCO_CAL_IN_FLASH_US (16100) // recal with DCO cal to do
#define RADIO_RECAL_TIME_NORECAL_US (8) // recal when not needed
#define RADIO_GET_NEXT_RECAL_DURATION (20) // u32Radio_Get_Next_Recal_Duration max execution time

/****************************************************************************/
/***    Number of bytes of the u8AppliData buffer                         ***/
/***    See vRadio_Save_ApplicationData_Retention                  ***/
/***    and vRadio_Restore_Retention_ApplicationData               ***/
/****************************************************************************/
#define APP_DATA_RET_NB_BYTES (114)
#define ADD_DATA_RET_NB_BITS (906)



/****************************************************************************/
/***    Local defined values        									  ***/
/****************************************************************************/
#define RADIO_MODE_LOPOWER          (0)
#define RADIO_MODE_HITXPOWER        (1)

#define RADIO_INIT_INITCAL			(0)
#define RADIO_INIT_DEF_VAL			(1)
#define RADIO_INIT_RETENTION		(2)
#define RADIO_INIT_FORCE_CAL		(3)

/****************************************************************************/
/***	Values to be used as u32RadioMode parameter                       ***/
/***	for vRadio_RadioInit API                                   ***/
/*** 	RADIO_MODE_STD_USE_INITCAL default init mode to use. in this mode ***/
/***	the radio init will be done in standard lowpower mode and the     ***/
/***	calibrations will be launched automatically based on temperature  ***/
/***	deviation since last calibration. Calibration will also be done   ***/
/***	if no retention values are available or if it is the first init.  ***/
/***	Other values are for test or special needs.                       ***/
/***	RADIO_MODE_STD_USE_DEF_VAL will force using default values for    ***/
/***	all parameters. No calibration is done.                           ***/
/***	RADIO_MODE_STD_USE_RETENTION will force usage of retention values ***/
/***	without any validity test nor temperature control                 ***/
/***    RADIO_MODE_STD_USE_FORCE_CAL will force calibration, ignoring any ***/
/***	retention values and even if temperature has not changed          ***/
/***	RADIO_MODE_HTXP_... have the same meaning but configure radio in  ***/
/***	high tx power mode.                                               ***/
/****************************************************************************/
#define RADIO_MODE_STD_USE_INITCAL		((RADIO_MODE_LOPOWER << 8) | RADIO_INIT_INITCAL)
#define RADIO_MODE_STD_USE_DEF_VAL      ((RADIO_MODE_LOPOWER << 8) | RADIO_INIT_DEF_VAL)
#define RADIO_MODE_STD_USE_RETENTION	((RADIO_MODE_LOPOWER << 8) | RADIO_INIT_RETENTION)
#define RADIO_MODE_STD_USE_FORCE_CAL	((RADIO_MODE_LOPOWER << 8) | RADIO_INIT_FORCE_CAL)
#define RADIO_MODE_HTXP_USE_INITCAL		((RADIO_MODE_HITXPOWER << 8) | RADIO_INIT_INITCAL)
#define RADIO_MODE_HTXP_USE_DEF_VAL		((RADIO_MODE_HITXPOWER << 8) | RADIO_INIT_DEF_VAL)
#define RADIO_MODE_HTXP_USE_RETENTION	((RADIO_MODE_HITXPOWER << 8) | RADIO_INIT_RETENTION)
#define RADIO_MODE_HTXP_USE_FORCE_CAL	((RADIO_MODE_HITXPOWER << 8) | RADIO_INIT_FORCE_CAL)

/****************************************************************************/
/***    Local defined values        									  ***/
/****************************************************************************/
#define TX_REGULAR 0
#define TX_PROP_1  1
#define TX_PROP_2  2
#define TX_BLE_1MB 3
#define TX_BLE_2MB 4
#define TX_UNDEFINED 0xFF

#define RX_DETECTOR_ONLY 0
#define RX_ENABLE_LUT    1
#define RX_BLE_1MB       3
#define RX_BLE_2MB       4
#define RX_UNDEFINED 0xFF

/****************************************************************************/
/***	Values to be used as u32RadioStandard parameter                   ***/
/***	for vRadio_Standard_Init API                               ***/
/***	RADIO_STANDARD_ZIGBEE_REGULAR standard Zigbee Mode                ***/
/***	RADIO_STANDARD_ZIGBEE_PROP_1 Zigbee mode with proprietary mode 1  ***/
/***	Tx configuration (soft spread reduction)                          ***/
/***	RADIO_STANDARD_ZIGBEE_PROP_2 Zigbee mode with proprietary mode 2  ***/
/***	Tx configuration (more agressive spread reduction)                ***/
/***	RADIO_STANDARD_ZIGBEE_REGULAR Zigbee Mode using LUT mode for AGC  ***/
/***	control. Test only.                                               ***/
/***	RADIO_STANDARD_ZIGBEE_PROP_1_LUT and _PROP_2_LUT LUT AGC control  ***/
/***	and Tx mode1/2                                                    ***/
/***	RADIO_STANDARD_BLE_1MB BLE 1 Mbps mode (for TX and RX)			  ***/
/***	RADIO_STANDARD_BLE_2MB BLE 2 Mbps mode (for TX and RX)			  ***/
/***	RADIO_STANDARD_BLE_RX1MB_TX2MB and RADIO_STANDARD_BLE_RX2MB_TX1MB ***/
/***	are BLE configuration with different RX/TX bitrates               ***/
/****************************************************************************/

#define RADIO_STANDARD_ZIGBEE_REGULAR ((RX_DETECTOR_ONLY << 8) | (TX_REGULAR))
#define RADIO_STANDARD_ZIGBEE_PROP_1  ((RX_DETECTOR_ONLY << 8) | (TX_PROP_1))
#define RADIO_STANDARD_ZIGBEE_PROP_2  ((RX_DETECTOR_ONLY << 8) | (TX_PROP_2))
#define RADIO_STANDARD_ZIGBEE_REGULAR_LUT ((RX_ENABLE_LUT << 8) | (TX_REGULAR))
#define RADIO_STANDARD_ZIGBEE_PROP_1_LUT  ((RX_ENABLE_LUT << 8) | (TX_PROP_1))
#define RADIO_STANDARD_ZIGBEE_PROP_2_LUT  ((RX_ENABLE_LUT << 8) | (TX_PROP_2))
#define RADIO_STANDARD_BLE_1MB ((RX_BLE_1MB << 8) | (TX_BLE_1MB))
#define RADIO_STANDARD_BLE_2MB ((RX_BLE_2MB << 8) | (TX_BLE_2MB))
#define RADIO_STANDARD_BLE_RX1MB_TX2MB ((RX_BLE_1MB << 8) | (TX_BLE_2MB))
#define RADIO_STANDARD_BLE_RX2MB_TX1MB ((RX_BLE_2MB << 8) | (TX_BLE_1MB))

/****************************************************************************/

/****************************************************************************/
/*** radio driver types/macros/prototypes                                 ***/
/****************************************************************************/

/****************************************************************************/
/***    Type definition for vRadio_SetChannelStandards function			  ***/
/****************************************************************************/
typedef enum
{
    E_RADIO_TX_MODE_STD    = TX_REGULAR,
    E_RADIO_TX_MODE_PROP_1 = TX_PROP_1,
    E_RADIO_TX_MODE_PROP_2 = TX_PROP_2,
    E_RADIO_TX_MODE_RESET  = TX_UNDEFINED
} teRadioTxMode;

/* RSSI reported by XCV in 1/4 dBm step */
#define RADIO_MAX_RSSI_REPORT       40
#define RADIO_MIN_RSSI_REPORT     -400

/****************************************************************************/
/*** radio driver API prototypes                                          ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME:       vRadio_RadioInit
 *
 * DESCRIPTION:
 * Radio driver initialisation function.
 *
 * PARAMETERS:
 * uint32_t u32RadioMode: use one of the defined values described above
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
void vRadio_RadioInit(uint32_t u32RadioMode);
/****************************************************************************
 *
 * NAME:       vRadio_RadioDeInit
 *
 * DESCRIPTION:
 * Radio driver de-initialisation function.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
void vRadio_RadioDeInit(void);

/****************************************************************************
 *
 * NAME:       vRadio_Standard_Init
 *
 * DESCRIPTION:
 * Set radio parameters for the specified standard in the corresponding
 * parameter bank of the radio HW block. There is one bank of each radio standard.
 *
 * PARAMETERS:
 * uint32_t u32RadioStandard: use one of the defined values described above
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
void vRadio_Standard_Init(uint32_t u32RadioStandard);

/****************************************************************************
 *
 * NAME:       u8Radio_GetEDfromRSSI
 *
 * DESCRIPTION:
 * Returns Energy Detect value calculated from RSSI value
 *
 * PARAMETERS:
 * int16_t i16RSSIval: RSSI value expressed in fourth of dBm (2'complement signed value)
 *
 * RETURNS:
 * uint8_t ED in [0..255] range
 *
 ****************************************************************************/
uint8_t u8Radio_GetEDfromRSSI(int16_t i16RSSIval);

/****************************************************************************
 *
 * NAME:       u32Radio_RadioModesAvailable
 *
 * DESCRIPTION:
 * Returns returns bit combination of available radio modes (1 for LOPOWER and/or 2 for HIGHPOWER)
 * Currently only LOPOWER is implemented so always returns 1
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * uint32_t bit combination of radio modes
 *
 ****************************************************************************/
uint32_t u32Radio_RadioModesAvailable(void);

/****************************************************************************
 *
 * NAME:       u32Radio_RadioGetVersion
 *
 * DESCRIPTION:
 * Returns radio driver version. This function can be used to check the version
 * of the radio driver embedded in the library used for the link against the value
 * defined above as RADIO_VERSION and detected possible mismatch between this
 * header file and the driver version itself.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * uint32_t radio version
 *
 ****************************************************************************/
uint32_t u32Radio_RadioGetVersion(void);

/****************************************************************************
 *
 * NAME:       vRadio_Temp_Update
 *
 * DESCRIPTION:
 * Provides the radio driver with the current temperature value.
 *
 * PARAMETERS:
 * int16_t s16Temp: Temperature expressed in half of degre C (2'complement 16 bit value)
 * 					For example 40 (or 0x28) for 20 degre Celsius
 * 					or -40 (0xFFD8) for -20 degre Celsius
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
void vRadio_Temp_Update(int16_t s16Temp);


/****************************************************************************
 *
 * NAME:       vRadio_ConfigCalFlashUsage
 *
 * DESCRIPTION:
 * Configure usage of flash record for radio calibration parameter.
 * For example, this function can be used to temporarily disable write of
 * calibration results in flash if there is a risk that the power can be removed
 * during next radio init (e.g. energy harvesting application).
 *
 * PARAMETERS:
 * bool bWriteToFlash: Allows radio_init/recal to write calibration results from flash.
 * 					   If set to false, after new calibration, new results will not be
 * 					   saved in flash for future re-use.
 * 					   Default value is true.
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
void vRadio_ConfigCalFlashUsage(bool bWriteToFlash);

/****************************************************************************
 *
 * NAME:       vRadio_Save_ApplicationData_Retention
 *
 * DESCRIPTION:
 * Save application data into radio retention registers.
 *
 * PARAMETERS:
 * uint8_t *u8AppliData: Table of 8bit values to be stored in retention registers
 * 						 It is the responsibility of the application that data
 * 						 are compacted in the table as a bit steam of maximum
 * 						 ADD_DATA_RET_NB_BITS bits over APP_DATA_RET_NB_BYTES bytes
 * 						 See above for ADD_DATA_RET_NB_BITS and APP_DATA_RET_NB_BYTES
 * 						 values.
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
void vRadio_Save_ApplicationData_Retention(uint8_t *u8AppliData);

/****************************************************************************
 *
 * NAME:       vRadio_Restore_Retention_ApplicationData
 *
 * DESCRIPTION:
 * Save application data into radio retention registers.
 *
 * PARAMETERS:
 * uint8_t *u8AppliData: Table of 8bit values to restore the table saved using
 * 						 vRadio_Save_ApplicationData_Retention API
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
void vRadio_Restore_Retention_ApplicationData( uint8_t *u8AppliData);





/****************************************************************************/
/***        Public Functions (to be called by LL or MAC layer)            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME:       vRadio_Recal
 *
 * DESCRIPTION:
 * This function is to be called when it is possible (from the LL or MAC perspective)
 * to re-calibrate the radio. This function will check the latest temperature
 * value provided by the last vRadio_Temp_Update API, and if the difference between
 * this temperature and the temperature used for the latest calibration is higher than
 * 40 degre C, a new calibration is done and this new temperature is saved as the
 * temperature of the latest calibration.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
bool vRadio_Recal(void);

/****************************************************************************
 *
 * NAME:       vRadio_RFT1778_bad_crc
 *
 * DESCRIPTION:
 * This function is to be called by the ZB MAC when a CRC error has been detected.
 * This avoid lockup of the radio in some cases.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * int : 1 if lockup condition has been detected and unlocked, 0 otherwise.
 *
 ****************************************************************************/
int vRadio_RFT1778_bad_crc(void);

/****************************************************************************
 *
 * NAME:       vRadio_AD_control
 *
 * DESCRIPTION:
 * When RX antenna diversity is enabled,  function is to be called by the ZB MAC
 * when PRE_STATE_1 state is reached.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_AD_control(void);

/****************************************************************************
 *
 * NAME:       vRadio_LockupCheckAndAbortRadio
 *
 * DESCRIPTION:
 * This function is to be called by the BLE LL when an error has been detected.
 * This avoid lockup of the radio in some cases.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_LockupCheckAndAbortRadio(void);

/****************************************************************************
 *
 * The next 7 APIs are dedicated to the MAC layer for its internal needs.
 *
 ****************************************************************************/
/* There are 3 sets of operating 'standards', which affects the frequency
   response of the radio:
     normal
     proprietary 1
     proprietary 2

   For compliance it may be necessary to select different standards for
   different channels, and this function allows that to be configured.
   Note that this function has the AppApi prefix rather than the MMAC
   prefix; we have traditionally provided customer-facing functions with
   the AppApi prefix and this avoids a level of indirection via the shim
   layer. */
void vRadio_SetComplianceLimits(int8_t i8TxMaxPower,
                                       int8_t i8TxMaxPowerCh26);
void vRadio_SetChannelStandards(teRadioTxMode eNewTxMode,
                                       teRadioTxMode eNewTxModeCh26);
void vRadio_InitialiseRadioStandard(void);
void vRadio_UpdateRadioStandard(uint8_t u8NewChannel);
void vRadio_SetChannelAndPower(uint8_t u8Channel, int8_t i8TxPower_dBm);
int8_t i8Radio_GetTxPowerLevel_dBm(void);

int16_t i16Radio_GetRSSI(uint32_t u32DurationSymbols,bool bAverage, uint8_t *u8antenna);
int16_t i16Radio_GetNbRSSISync(void);
int16_t i16Radio_GetNbRSSISyncCor(uint8_t u8rate);
int8_t i8Radio_GetLastPacketRSSI();
int16_t i16Radio_BoundRssiValue(int16_t value);

/****************************************************************************
 *
 * These APIs are dedicated to the BLE LL to reset BLE HW block if needed
 * and to execute patch at end of RX process
 *
 ****************************************************************************/

/* BLE reset APIs */
void vRadio_BLE_ResetOn(void);
void vRadio_BLE_ResetOff(void);

void vRadio_remove_patch_ISR(void);
void vRadio_SingleRX_AgcReadyPatch(void);
void vRadio_MultiRX_AgcReadyPatch(void);
void vRadio_Enable_AgcReadyPatch(void);
void vRadio_Disable_AgcReadyPatch(void);



/****************************************************************************
 *
 * The next 2 APIs are for temporary usage and are to be removed when XTAL init
 * will be put out of radio driver
 *
 ****************************************************************************/

/****************************************************************************
 *
 * NAME:       vRadio_SkipXTALInit
 *
 * DESCRIPTION:
 * Set an internal flag in the radio driver to skip any XTAL 32MHz handling
 * by the radio driver. With this flag set, the radio driver does not start
 * nor Trim the 32M XO.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_SkipXTALInit(void);

/****************************************************************************
 *
 * NAME:       vRadio_EnableXTALInit
 *
 * DESCRIPTION:
 * Reset the internal flag in the radio driver used to skip any XTAL 32MHz handling
 * by the radio driver. With this flag cleared, the radio driver checks if 32M XO is
 * running and start it if not already started. It also trim the 32M XO.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_EnableXTALInit(void);

/****************************************************************************
 *
 * NAME:       vRadio_ActivateXtal32MRadioBiasing
 *
 * DESCRIPTION:
 * Reset Radio HW block and switch XTAL32M to radio biasing control.
 * ASSUMES THAT XTAL32M IS ALREADY SETUP, TRIMMED AND RUNNING UNDER PMC BIASING
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_ActivateXtal32MRadioBiasing(void);

/****************************************************************************
 *
 * NAME:       vRadio_DisableZBRadio
 *
 * DESCRIPTION:
 * Disable ZB radio block.
 * This API needs to be called before vRadio_RadioInit and vRadio_ActivateXtal32MRadioBiasing.
 * When ZB radio block is disabled, it is not reset and clocks are not enabled for this
 * HW block.
 * ONLY ONE OF vRadio_DisableZBRadio or vRadio_DisableBLERadio can be called.
 * IF BOTH APIS ARE CALLED, ONLY THE FIRST ONE HAS EFFECT.
 * IT CANNOT BE REVERSED. NEED HW RESET TO USE ZB RADIO AGAIN.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_DisableZBRadio(void);

/****************************************************************************
 *
 * NAME:       vRadio_DisableBLERadio
 *
 * DESCRIPTION:
 * Disable BLE radio block.
 * This API needs to be called before vRadio_RadioInit and vRadio_ActivateXtal32MRadioBiasing.
 * When BLEB radio block is disabled, clocks are not enabled for this HW block.
 * ONLY ONE OF vRadio_DisableZBRadio or vRadio_DisableBLERadio can be called.
 * IF BOTH APIS ARE CALLED, ONLY THE FIRST ONE HAS EFFECT.
 * IT CANNOT BE REVERSED. NEED HW RESET TO USE BLE RADIO AGAIN.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_DisableBLERadio(void);

/****************************************************************************
 *
 * NAME:       vRadio_EnableBLEFastTX
 *
 * DESCRIPTION:
 * Enable keeping G1 and G2 on to give more time between RX and TX.
 * On ES2MF it is also possible to keep PLL group using bKeepPll parameter.
 *
 * PARAMETERS:
 * bool bKeepPll: when true, PLL group is kept active (ES2MF only, no effect
 *                otherwise)
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_EnableBLEFastTX(bool bKeepPll);

/****************************************************************************
 *
 * NAME:       vRadio_DisableBLEFastTX
 *
 * DESCRIPTION:
 * Disable keeping G1 and G2 on to give more time between RX and TX.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_DisableBLEFastTX(void);

/****************************************************************************
 *
 * NAME:       vRadio_Disable_DCO_DAC
 *
 * DESCRIPTION:
 * Disable keeping DCO DAC always on. By default vRadio_ActivateXtal32MRadioBiasing
 * and vRadio_RadioInit force DCO DAC to on state to ensure it is ready to operate
 * at the very beginning of RX process. If no RX is foreseen before next powerdown or sleep
 * DCO DAC can be disable to reduce power consumption.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_Disable_DCO_DAC(void);

/****************************************************************************
 *
 * NAME:       vRadio_ZBtoBLE
 *
 * DESCRIPTION:
 * Change some settings needed when switching from ZB to BLE.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_ZBtoBLE(void);

/****************************************************************************
 *
 * NAME:       vRadio_BLEtoZB
 *
 * DESCRIPTION:
 * Change some settings needed when switching from BLE to ZB.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_BLEtoZB(void);

/****************************************************************************
 *
 * NAME:       u16Radio_Get_Next_Recal_Duration
 *
 * DESCRIPTION:
 * Returns estimate time duration of the next calibration. This estimate is
 * based on the last temperature provided by the vRadio_Temp_Update,
 * the temperature of the last calibration and the operations to do for this
 * calibrations.
 * If no calibration is needed or vRadio_RadioInit has not bee called before,
 * the API returns 0. Otherwise it returns the estimated duration in us.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
uint32_t u32Radio_Get_Next_Recal_Duration(void);


/****************************************************************************
 *
 * NAME:       vRadio_AntennaDiversityTxRxEnable
 *
 * DESCRIPTION:
 * Enables Antenna Diversity for Tx and/or Rx.
 *
 * PARAMETERS:
 * bool bRxEnabled: true to enable Rx AD, false to disable it
 * bool bTxEnabled: true to enable Tx AD, false to disable it
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_AntennaDiversityTxRxEnable(bool bRxEnabled, bool bTxEnabled);
/****************************************************************************
 *
 * NAME:       vRadio_AntennaDiversityConfigure
 *
 * DESCRIPTION:
 * Configure some AD setings.
 *
 * PARAMETERS:
 * uint16_t rssi_thr: RSSI threshold to switch antenna (10bit fourth dBm in two complement)
 *                    Default value is 0x278 (-98dBm)
 * uint8_t rx_timer: Timer before to check received power again (4us steps, 4bits)
 *                   Default value is 0x8 (32us)
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_AntennaDiversityConfigure(uint16_t rssi_thr, uint8_t rx_timer);
/****************************************************************************
 *
 * NAME:       vRadio_AntennaDiversitySwitch
 *
 * DESCRIPTION:
 * Toggle antenna selection.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_AntennaDiversitySwitch(void);
/****************************************************************************
 *
 * NAME:       u8Radio_AntennaDiversityStatus
 *
 * DESCRIPTION:
 * Returns current selected antenna.
 *
 * PARAMETERS:
 * None
 *
 * RETURNS:
 * Selected antenna (0 or 1) on uint8_t .
 *
 ****************************************************************************/
uint8_t u8Radio_AntennaDiversityStatus(void);


/****************************************************************************
 *
 * NAME:       vRadio_SetBLEdpTopEmAddr
 *
 * DESCRIPTION:
 * Configure LL_EM_BASE_ADDRESS of BLEMODEM parameter.
 *
 * PARAMETERS:
 * uint32_t em_addr: EM address.
 *
 * RETURNS:
 * None.
 *
 ****************************************************************************/
void vRadio_SetBLEdpTopEmAddr(uint32_t em_addr);


#ifdef __cplusplus
}
#endif

#endif /* __RADIO_H_ */
