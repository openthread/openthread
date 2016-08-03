/******************************************************************************
*  Filename:       setup.h
*  Revised:        2016-05-18 12:51:22 +0200 (Wed, 18 May 2016)
*  Revision:       46398
*
*  Description:    Prototypes and defines for the setup API.
*
*  Copyright (c) 2015 - 2016, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

//*****************************************************************************
//
//! \addtogroup system_control_group
//! @{
//! \addtogroup setup_api
//! @{
//
//*****************************************************************************

#ifndef __SETUP_H__
#define __SETUP_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

// Hardware headers
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_adi.h>
#include <inc/hw_adi_0_rf.h>
#include <inc/hw_adi_1_synth.h>
#include <inc/hw_adi_2_refsys.h>
#include <inc/hw_adi_3_refsys.h>
#include <inc/hw_adi_4_aux.h>
#include <inc/hw_aon_ioc.h>
#include <inc/hw_aon_sysctl.h>
#include <inc/hw_aon_wuc.h>
#include <inc/hw_aux_wuc.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_chip_def.h>
#include <inc/hw_ddi.h>
#include <inc/hw_flash.h>
#include <inc/hw_fcfg1.h>
#include <inc/hw_ddi_0_osc.h>
#include <inc/hw_prcm.h>
#include <inc/hw_vims.h>
#include <inc/hw_aon_batmon.h>
#include <inc/hw_aon_rtc.h>
// Driverlib headers
#include <driverlib/adi.h>
#include <driverlib/aon_batmon.h>
#include <driverlib/cpu.h>
#include <driverlib/chipinfo.h>
#include <driverlib/ddi.h>
#include <driverlib/ioc.h>
#include <driverlib/prcm.h>
#include <driverlib/setup.h>
#include <driverlib/sys_ctrl.h>

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define SetupTrimDevice                 NOROM_SetupTrimDevice
    #define SetupAfterColdResetWakeupFromShutDownCfg1 NOROM_SetupAfterColdResetWakeupFromShutDownCfg1
    #define SetupAfterColdResetWakeupFromShutDownCfg2 NOROM_SetupAfterColdResetWakeupFromShutDownCfg2
    #define SetupAfterColdResetWakeupFromShutDownCfg3 NOROM_SetupAfterColdResetWakeupFromShutDownCfg3
    #define SetupGetTrimForAdcShModeEn      NOROM_SetupGetTrimForAdcShModeEn
    #define SetupGetTrimForAdcShVbufEn      NOROM_SetupGetTrimForAdcShVbufEn
    #define SetupGetTrimForAmpcompCtrl      NOROM_SetupGetTrimForAmpcompCtrl
    #define SetupGetTrimForAmpcompTh1       NOROM_SetupGetTrimForAmpcompTh1
    #define SetupGetTrimForAmpcompTh2       NOROM_SetupGetTrimForAmpcompTh2
    #define SetupGetTrimForAnabypassValue1  NOROM_SetupGetTrimForAnabypassValue1
    #define SetupGetTrimForDblrLoopFilterResetVoltage NOROM_SetupGetTrimForDblrLoopFilterResetVoltage
    #define SetupGetTrimForRadcExtCfg       NOROM_SetupGetTrimForRadcExtCfg
    #define SetupGetTrimForRcOscLfIBiasTrim NOROM_SetupGetTrimForRcOscLfIBiasTrim
    #define SetupGetTrimForRcOscLfRtuneCtuneTrim NOROM_SetupGetTrimForRcOscLfRtuneCtuneTrim
    #define SetupGetTrimForXoscHfCtl        NOROM_SetupGetTrimForXoscHfCtl
    #define SetupGetTrimForXoscHfFastStart  NOROM_SetupGetTrimForXoscHfFastStart
    #define SetupGetTrimForXoscHfIbiastherm NOROM_SetupGetTrimForXoscHfIbiastherm
    #define SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio NOROM_SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio
    #define SetupSignExtendVddrTrimValue    NOROM_SetupSignExtendVddrTrimValue
    #define SetupSetCacheModeAccordingToCcfgSetting NOROM_SetupSetCacheModeAccordingToCcfgSetting
    #define SetupSetAonRtcSubSecInc         NOROM_SetupSetAonRtcSubSecInc
#endif

//*****************************************************************************
//
//! \brief Performs the necessary trim of the device which is not done in boot code.
//!
//! This function should only execute coming from ROM boot. The current
//! implementation does not take soft reset into account. However, it does no
//! damage to execute it again. It only consumes time.
//!
//! \return None
//
//*****************************************************************************
extern void SetupTrimDevice( void );

//*****************************************************************************
//
//! \brief First part of configuration required when waking up from shutdown.
//
//*****************************************************************************
extern void SetupAfterColdResetWakeupFromShutDownCfg1( uint32_t ccfg_ModeConfReg );

//*****************************************************************************
//
//! \brief Second part of configuration required when waking up from shutdown.
//
//*****************************************************************************
extern void SetupAfterColdResetWakeupFromShutDownCfg2( uint32_t ui32Fcfg1Revision, uint32_t ccfg_ModeConfReg );

//*****************************************************************************
//
//! \brief Third part of configuration required when waking up from shutdown.
//
//*****************************************************************************
extern void SetupAfterColdResetWakeupFromShutDownCfg3( uint32_t ccfg_ModeConfReg );

//*****************************************************************************
//
//! \brief Returns the trim value from FCFG1 to be used as ADC_SH_MODE_EN setting.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForAdcShModeEn( uint32_t ui32Fcfg1Revision );

//*****************************************************************************
//
//! \brief Returns the trim value from FCFG1 to be used as ADC_SH_VBUF_EN setting.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForAdcShVbufEn( uint32_t ui32Fcfg1Revision );

//*****************************************************************************
//
//! \brief Returns the trim value to be used for the AMPCOMP_CTRL register in OSC_DIG.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForAmpcompCtrl( uint32_t ui32Fcfg1Revision );

//*****************************************************************************
//
//! \brief Returns the trim value to be used for the AMPCOMP_TH1 register in OSC_DIG.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForAmpcompTh1( void );

//*****************************************************************************
//
//! \brief Returns the trim value to be used for the AMPCOMP_TH2 register in OSC_DIG.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForAmpcompTh2( void );

//*****************************************************************************
//
//! \brief Returns the trim value to be used for the ANABYPASS_VALUE1 register in OSC_DIG.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForAnabypassValue1( uint32_t ccfg_ModeConfReg );

//*****************************************************************************
//
//! \brief Returns the trim value from FCFG1 to be used as DBLR_LOOP_FILTER_RESET_VOLTAGE setting.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForDblrLoopFilterResetVoltage( uint32_t ui32Fcfg1Revision );

//*****************************************************************************
//
//! \brief Returns the trim value to be used for the RADCEXTCFG register in OSC_DIG.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForRadcExtCfg( uint32_t ui32Fcfg1Revision );

//*****************************************************************************
//
//! \brief Returns the FCFG1 OSC_CONF_ATESTLF_RCOSCLF_IBIAS_TRIM.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForRcOscLfIBiasTrim( uint32_t ui32Fcfg1Revision );

//*****************************************************************************
//
//! \brief Returns the trim value to be used for the RCOSCLF_RTUNE_TRIM and the
//! RCOSCLF_CTUNE_TRIM bit fields in the XOSCLF_RCOSCLF_CTRL register in OSC_DIG.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForRcOscLfRtuneCtuneTrim( void );

//*****************************************************************************
//
//! \brief Returns the trim value to be used for the XOSCHFCTL register in OSC_DIG.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForXoscHfCtl( uint32_t ui32Fcfg1Revision );

//*****************************************************************************
//
//! \brief Returns the trim value to be used as OSC_DIG:CTL1.XOSC_HF_FAST_START.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForXoscHfFastStart( void );

//*****************************************************************************
//
//! \brief Returns the trim value to be used for the XOSC_HF_IBIASTHERM bit field in
//! the ANABYPASS_VALUE2 register in OSC_DIG.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForXoscHfIbiastherm( void );

//*****************************************************************************
//
//! \brief Returns XOSCLF_REGULATOR_TRIM and XOSCLF_CMIRRWR_RATIO as one packet
//! spanning bits [5:0] in the returned value.
//
//*****************************************************************************
extern uint32_t SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio( uint32_t ui32Fcfg1Revision );

//*****************************************************************************
//
//! \brief Sign extend the VDDR_TRIM setting (special format ranging from -10 to +21)
//!
//! \return
//
//*****************************************************************************
extern int32_t SetupSignExtendVddrTrimValue( uint32_t ui32VddrTrimVal );

//*****************************************************************************
//
//! \brief Set correct VIMS_MODE according to CCFG setting (CACHE or GPRAM)
//!
//! \return None
//
//*****************************************************************************
extern void SetupSetCacheModeAccordingToCcfgSetting( void );

//*****************************************************************************
//
//! \brief Doing the tricky stuff needed to enter new RTCSUBSECINC value
//!
//! \return None
//
//*****************************************************************************
extern void SetupSetAonRtcSubSecInc( uint32_t subSecInc );

//*****************************************************************************
//
//! \brief Set VDDR boost mode
//!
//! \return None
//
//*****************************************************************************

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include <driverlib/rom.h>
    #ifdef ROM_SetupTrimDevice
        #undef  SetupTrimDevice
        #define SetupTrimDevice                 ROM_SetupTrimDevice
    #endif
    #ifdef ROM_SetupAfterColdResetWakeupFromShutDownCfg1
        #undef  SetupAfterColdResetWakeupFromShutDownCfg1
        #define SetupAfterColdResetWakeupFromShutDownCfg1 ROM_SetupAfterColdResetWakeupFromShutDownCfg1
    #endif
    #ifdef ROM_SetupAfterColdResetWakeupFromShutDownCfg2
        #undef  SetupAfterColdResetWakeupFromShutDownCfg2
        #define SetupAfterColdResetWakeupFromShutDownCfg2 ROM_SetupAfterColdResetWakeupFromShutDownCfg2
    #endif
    #ifdef ROM_SetupAfterColdResetWakeupFromShutDownCfg3
        #undef  SetupAfterColdResetWakeupFromShutDownCfg3
        #define SetupAfterColdResetWakeupFromShutDownCfg3 ROM_SetupAfterColdResetWakeupFromShutDownCfg3
    #endif
    #ifdef ROM_SetupGetTrimForAdcShModeEn
        #undef  SetupGetTrimForAdcShModeEn
        #define SetupGetTrimForAdcShModeEn      ROM_SetupGetTrimForAdcShModeEn
    #endif
    #ifdef ROM_SetupGetTrimForAdcShVbufEn
        #undef  SetupGetTrimForAdcShVbufEn
        #define SetupGetTrimForAdcShVbufEn      ROM_SetupGetTrimForAdcShVbufEn
    #endif
    #ifdef ROM_SetupGetTrimForAmpcompCtrl
        #undef  SetupGetTrimForAmpcompCtrl
        #define SetupGetTrimForAmpcompCtrl      ROM_SetupGetTrimForAmpcompCtrl
    #endif
    #ifdef ROM_SetupGetTrimForAmpcompTh1
        #undef  SetupGetTrimForAmpcompTh1
        #define SetupGetTrimForAmpcompTh1       ROM_SetupGetTrimForAmpcompTh1
    #endif
    #ifdef ROM_SetupGetTrimForAmpcompTh2
        #undef  SetupGetTrimForAmpcompTh2
        #define SetupGetTrimForAmpcompTh2       ROM_SetupGetTrimForAmpcompTh2
    #endif
    #ifdef ROM_SetupGetTrimForAnabypassValue1
        #undef  SetupGetTrimForAnabypassValue1
        #define SetupGetTrimForAnabypassValue1  ROM_SetupGetTrimForAnabypassValue1
    #endif
    #ifdef ROM_SetupGetTrimForDblrLoopFilterResetVoltage
        #undef  SetupGetTrimForDblrLoopFilterResetVoltage
        #define SetupGetTrimForDblrLoopFilterResetVoltage ROM_SetupGetTrimForDblrLoopFilterResetVoltage
    #endif
    #ifdef ROM_SetupGetTrimForRadcExtCfg
        #undef  SetupGetTrimForRadcExtCfg
        #define SetupGetTrimForRadcExtCfg       ROM_SetupGetTrimForRadcExtCfg
    #endif
    #ifdef ROM_SetupGetTrimForRcOscLfIBiasTrim
        #undef  SetupGetTrimForRcOscLfIBiasTrim
        #define SetupGetTrimForRcOscLfIBiasTrim ROM_SetupGetTrimForRcOscLfIBiasTrim
    #endif
    #ifdef ROM_SetupGetTrimForRcOscLfRtuneCtuneTrim
        #undef  SetupGetTrimForRcOscLfRtuneCtuneTrim
        #define SetupGetTrimForRcOscLfRtuneCtuneTrim ROM_SetupGetTrimForRcOscLfRtuneCtuneTrim
    #endif
    #ifdef ROM_SetupGetTrimForXoscHfCtl
        #undef  SetupGetTrimForXoscHfCtl
        #define SetupGetTrimForXoscHfCtl        ROM_SetupGetTrimForXoscHfCtl
    #endif
    #ifdef ROM_SetupGetTrimForXoscHfFastStart
        #undef  SetupGetTrimForXoscHfFastStart
        #define SetupGetTrimForXoscHfFastStart  ROM_SetupGetTrimForXoscHfFastStart
    #endif
    #ifdef ROM_SetupGetTrimForXoscHfIbiastherm
        #undef  SetupGetTrimForXoscHfIbiastherm
        #define SetupGetTrimForXoscHfIbiastherm ROM_SetupGetTrimForXoscHfIbiastherm
    #endif
    #ifdef ROM_SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio
        #undef  SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio
        #define SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio ROM_SetupGetTrimForXoscLfRegulatorAndCmirrwrRatio
    #endif
    #ifdef ROM_SetupSignExtendVddrTrimValue
        #undef  SetupSignExtendVddrTrimValue
        #define SetupSignExtendVddrTrimValue    ROM_SetupSignExtendVddrTrimValue
    #endif
    #ifdef ROM_SetupSetCacheModeAccordingToCcfgSetting
        #undef  SetupSetCacheModeAccordingToCcfgSetting
        #define SetupSetCacheModeAccordingToCcfgSetting ROM_SetupSetCacheModeAccordingToCcfgSetting
    #endif
    #ifdef ROM_SetupSetAonRtcSubSecInc
        #undef  SetupSetAonRtcSubSecInc
        #define SetupSetAonRtcSubSecInc         ROM_SetupSetAonRtcSubSecInc
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __SETUP_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
