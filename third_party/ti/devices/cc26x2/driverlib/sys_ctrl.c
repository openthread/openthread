/******************************************************************************
*  Filename:       sys_ctrl.c
*  Revised:        2017-04-26 18:27:45 +0200 (Wed, 26 Apr 2017)
*  Revision:       48852
*
*  Description:    Driver for the System Control.
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
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

// Hardware headers
#include "../inc/hw_types.h"
#include "../inc/hw_ccfg.h"
// Driverlib headers
#include "aon_batmon.h"
#include "sys_ctrl.h"
#include "setup_rom.h"


//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  SysCtrlSetRechargeBeforePowerDown
    #define SysCtrlSetRechargeBeforePowerDown NOROM_SysCtrlSetRechargeBeforePowerDown
    #undef  SysCtrlAdjustRechargeAfterPowerDown
    #define SysCtrlAdjustRechargeAfterPowerDown NOROM_SysCtrlAdjustRechargeAfterPowerDown
    #undef  SysCtrl_DCDC_VoltageConditionalControl
    #define SysCtrl_DCDC_VoltageConditionalControl NOROM_SysCtrl_DCDC_VoltageConditionalControl
    #undef  SysCtrlResetSourceGet
    #define SysCtrlResetSourceGet           NOROM_SysCtrlResetSourceGet
#endif

//*****************************************************************************
//
// Recharge calculator defines and globals
//
//*****************************************************************************

#define  PD_STATE_CACHE_RET      1
#define  PD_STATE_RFMEM_RET      2
#define  PD_STATE_XOSC_LPM       4
#define  PD_STATE_EXT_REG_MODE   8


//*****************************************************************************
//
// SysCtrlSetRechargeBeforePowerDown( xoscPowerMode )
//
//*****************************************************************************
void
SysCtrlSetRechargeBeforePowerDown( uint32_t xoscPowerMode )
{
   // Nothing to be done but keeping this function for platform compatibility.
}


//*****************************************************************************
//
// SysCtrlAdjustRechargeAfterPowerDown()
//
//*****************************************************************************
void
SysCtrlAdjustRechargeAfterPowerDown( void )
{
   // Nothing to be done but keeping this function for platform compatibility.
}


//*****************************************************************************
//
// SysCtrl_DCDC_VoltageConditionalControl()
//
//*****************************************************************************
void
SysCtrl_DCDC_VoltageConditionalControl( void )
{
   uint32_t batThreshold     ;  // Fractional format with 8 fractional bits.
   uint32_t aonBatmonBat     ;  // Fractional format with 8 fractional bits.
   uint32_t ccfg_ModeConfReg ;  // Holds a copy of the CCFG_O_MODE_CONF register.
   uint32_t aonPmctlPwrctl   ;  // Reflect whats read/written to the AON_PMCTL_O_PWRCTL register.

   // We could potentially call this function before any battery voltage measurement
   // is made/available. In that case we must make sure that we do not turn off the DCDC.
   // This can be done by doing nothing as long as the battery voltage is 0 (Since the
   // reset value of the battery voltage register is 0).
   aonBatmonBat = HWREG( AON_BATMON_BASE + AON_BATMON_O_BAT );
   if ( aonBatmonBat != 0 ) {
      // Check if Voltage Conditional Control is enabled
      // It is enabled if all the following are true:
      // - DCDC in use (either in active or recharge mode), (in use if one of the corresponding CCFG bits are zero).
      // - Alternative DCDC settings are enabled ( DIS_ALT_DCDC_SETTING == 0 )
      // - Not in external regulator mode ( EXT_REG_MODE == 0 )
      ccfg_ModeConfReg = HWREG( CCFG_BASE + CCFG_O_MODE_CONF );

      if (((( ccfg_ModeConfReg & CCFG_MODE_CONF_DCDC_RECHARGE_M ) == 0                                            ) ||
           (( ccfg_ModeConfReg & CCFG_MODE_CONF_DCDC_ACTIVE_M   ) == 0                                            )    ) &&
          (( HWREG( AON_PMCTL_BASE  + AON_PMCTL_O_PWRCTL  ) & AON_PMCTL_PWRCTL_EXT_REG_MODE  )               == 0      ) &&
          (( HWREG( CCFG_BASE + CCFG_O_SIZE_AND_DIS_FLAGS ) & CCFG_SIZE_AND_DIS_FLAGS_DIS_ALT_DCDC_SETTING ) == 0      )    )
      {
         aonPmctlPwrctl = HWREG( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL );
         batThreshold   = (((( HWREG( CCFG_BASE + CCFG_O_MODE_CONF_1 ) &
            CCFG_MODE_CONF_1_ALT_DCDC_VMIN_M ) >>
            CCFG_MODE_CONF_1_ALT_DCDC_VMIN_S ) + 28 ) << 4 );

         if ( aonPmctlPwrctl & ( AON_PMCTL_PWRCTL_DCDC_EN_M | AON_PMCTL_PWRCTL_DCDC_ACTIVE_M )) {
            // DCDC is ON, check if it should be switched off
            if ( aonBatmonBat < batThreshold ) {
               aonPmctlPwrctl &= ~( AON_PMCTL_PWRCTL_DCDC_EN_M | AON_PMCTL_PWRCTL_DCDC_ACTIVE_M );

               HWREG( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL ) = aonPmctlPwrctl;
            }
         } else {
            // DCDC is OFF, check if it should be switched on
            if ( aonBatmonBat > batThreshold ) {
               if (( ccfg_ModeConfReg & CCFG_MODE_CONF_DCDC_RECHARGE_M ) == 0 ) aonPmctlPwrctl |= AON_PMCTL_PWRCTL_DCDC_EN_M     ;
               if (( ccfg_ModeConfReg & CCFG_MODE_CONF_DCDC_ACTIVE_M   ) == 0 ) aonPmctlPwrctl |= AON_PMCTL_PWRCTL_DCDC_ACTIVE_M ;

               HWREG( AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL ) = aonPmctlPwrctl;
            }
         }
      }
   }
}


//*****************************************************************************
//
// SysCtrlResetSourceGet()
//
//*****************************************************************************
uint32_t
SysCtrlResetSourceGet( void )
{
   if ( HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) & AON_PMCTL_RESETCTL_WU_FROM_SD_M ) {
      return ( RSTSRC_WAKEUP_FROM_SHUTDOWN );
   } else {
      return (( HWREG( AON_PMCTL_BASE + AON_PMCTL_O_RESETCTL ) &
                AON_PMCTL_RESETCTL_RESET_SRC_M ) >>
                AON_PMCTL_RESETCTL_RESET_SRC_S ) ;
   }
}
