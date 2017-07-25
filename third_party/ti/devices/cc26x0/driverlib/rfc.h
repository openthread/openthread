/******************************************************************************
*  Filename:       rfc.h
*  Revised:        2017-05-03 15:46:38 +0200 (Wed, 03 May 2017)
*  Revision:       48887
*
*  Description:    Defines and prototypes for the RF Core.
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

//*****************************************************************************
//
//! \addtogroup rfc_api
//! @{
//
//*****************************************************************************

#ifndef __RFC_H__
#define __RFC_H__

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

#include <stdbool.h>
#include <stdint.h>
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_rfc_pwr.h"
#include "../inc/hw_rfc_dbell.h"
#include "rf_common_cmd.h"
#include "rf_prop_cmd.h"
#include "rf_ble_cmd.h"
#include "../inc/hw_fcfg1.h"
#include "../inc/hw_adi_3_refsys.h"
#include "../inc/hw_adi.h"

typedef struct {
   uint32_t configIfAdc;
   uint32_t configRfFrontend;
   uint32_t configSynth;
   uint32_t configMiscAdc;
} rfTrim_t;

// Define for RFCOverrideSearch
#define RFC_MAX_SEARCH_DEPTH 		5

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
    #define RFCCpeIntGetAndClear            NOROM_RFCCpeIntGetAndClear
    #define RFCDoorbellSendTo               NOROM_RFCDoorbellSendTo
    #define RFCSynthPowerDown               NOROM_RFCSynthPowerDown
    #define RFCRfTrimRead                   NOROM_RFCRfTrimRead
    #define RFCRfTrimSet                    NOROM_RFCRfTrimSet
    #define RFCRTrim                        NOROM_RFCRTrim
    #define RFCCPEPatchReset                NOROM_RFCCPEPatchReset
    #define RFCAdi3VcoLdoVoltageMode        NOROM_RFCAdi3VcoLdoVoltageMode
    #define RFCOverrideUpdate               NOROM_RFCOverrideUpdate
    #define RFCHWIntGetAndClear             NOROM_RFCHWIntGetAndClear
#endif

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Enable the RF core clocks.
//!
//! As soon as the RF core is started it will handle clock control
//! autonomously. No check should be performed to check the clocks. Instead
//! the radio can be ping'ed through the command interface.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
RFCClockEnable(void)
{
    // Enable all clocks
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) =
                                 RFC_PWR_PWMCLKEN_RFCTRC |
                                 RFC_PWR_PWMCLKEN_FSCA |
                                 RFC_PWR_PWMCLKEN_PHA |
                                 RFC_PWR_PWMCLKEN_RAT |
                                 RFC_PWR_PWMCLKEN_RFERAM |
                                 RFC_PWR_PWMCLKEN_RFE |
                                 RFC_PWR_PWMCLKEN_MDMRAM |
                                 RFC_PWR_PWMCLKEN_MDM |
                                 RFC_PWR_PWMCLKEN_CPERAM |
                                 RFC_PWR_PWMCLKEN_CPE |
                                 RFC_PWR_PWMCLKEN_RFC;
}

//*****************************************************************************
//
//! \brief Disable the RF core clocks.
//!
//! As soon as the RF core is started it will handle clock control
//! autonomously. No check should be performed to check the clocks. Instead
//! the radio can be ping'ed through the command interface.
//!
//! When disabling clocks it is the programmers responsibility that the
//! RF core clocks can be safely gated. I.e. the RF core should be safely
//! 'parked'.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
RFCClockDisable(void)
{
    // Disable all clocks
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = 0x0;
}

//*****************************************************************************
//
//! \brief Enable some of the RF core clocks.
//!
//! As soon as the RF core is started it will handle clock control
//! autonomously. No check should be performed to check the clocks. Instead
//! the radio can be ping'ed through the command interface.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
RFCClockSet(uint32_t ui32Mask)
{
    //
    // Enable clocks
    //
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) |= ui32Mask;
}

//*****************************************************************************
//
//! \brief Disable some of the RF core clocks.
//!
//! As soon as the RF core is started it will handle clock control
//! autonomously. No check should be performed to check the clocks. Instead
//! the radio can be ping'ed through the command interface.
//!
//! When disabling clocks it is the programmers responsibility that the
//! RF core clocks can be safely gated. I.e. the RF core should be safely
//! 'parked'.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
RFCClockClear(uint32_t ui32Mask)
{
    //
    // Disable clocks
    //
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) &= ~ui32Mask;
}

//*****************************************************************************
//
//! Enable CPE0 interrupt
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpe0IntEnable(uint32_t ui32Mask)
{
  // Multiplex RF Core interrupts to CPE0 IRQ.
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEISL) &= ~ui32Mask;

  do
  {
    // Clear any pending interrupts.
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
  }while(HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) != 0x0);

  //  Enable the masked interrupts
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) |= ui32Mask;
}


//*****************************************************************************
//
//! Enable CPE1 interrupt
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpe1IntEnable(uint32_t ui32Mask)
{
  // Multiplex RF Core interrupts to CPE1 IRQ.
  HWREG( RFC_DBELL_BASE + RFC_DBELL_O_RFCPEISL) |= ui32Mask;

  do
  {
    // Clear any pending interrupts.
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
  }while(HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) != 0x0);

  //  Enable the masked interrupts
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) |= ui32Mask;
}


//*****************************************************************************
//
//! This function is used to map only HW interrupts, and
//! clears/unmasks them. These interrupts are then enabled.
//
//*****************************************************************************
__STATIC_INLINE void
RFCHwIntEnable(uint32_t ui32Mask)
{
  // Clear any pending interrupts.
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIFG) = 0x0;

  //  Enable the masked interrupts
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIEN) |= ui32Mask;
}


//*****************************************************************************
//
//! Disable CPE interrupt
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpeIntDisable(uint32_t ui32Mask)
{
  //  Disable the masked interrupts
  HWREG( RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN ) &= ~ui32Mask;
}


//*****************************************************************************
//
//! Disable HW interrupt
//
//*****************************************************************************
__STATIC_INLINE void
RFCHwIntDisable(uint32_t ui32Mask)
{
  //  Disable the masked interrupts
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIEN) &= ~ui32Mask;
}


//*****************************************************************************
//
//! Get and clear CPE interrupt flags
//
//*****************************************************************************
extern uint32_t RFCCpeIntGetAndClear(void);


//*****************************************************************************
//
//! Clear interrupt flags
//
//*****************************************************************************
__STATIC_INLINE void
RFCCpeIntClear(uint32_t ui32Mask)
{
  do
  {
    // Clear interrupts that may now be pending
    HWREG(RFC_DBELL_BASE+RFC_DBELL_O_RFCPEIFG) = ~ui32Mask;
  }while (HWREG(RFC_DBELL_BASE+RFC_DBELL_O_RFCPEIFG) & ui32Mask);
}


//*****************************************************************************
//
//! Clear interrupt flags
//
//*****************************************************************************
__STATIC_INLINE void
RFCHwIntClear(uint32_t ui32Mask)
{
  // Clear pending interrupts.
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIFG) = ~ui32Mask;
}


//*****************************************************************************
//
//! Clear interrupt flags
//
//*****************************************************************************
__STATIC_INLINE void
RFCAckIntClear(void)
{
  // Clear any pending interrupts.
  HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0x0;
}


//*****************************************************************************
//
//! Function to search top RFC_MAX_SEARCH_DEPTH (5) overrides
//
//*****************************************************************************
__STATIC_INLINE uint8_t
RFCOverrideSearch(const uint32_t *pOverride, const uint32_t pattern, const uint32_t mask)
{
    uint8_t override_index;

     // Search top overrides for RTRIM
    for(override_index = 0; override_index < RFC_MAX_SEARCH_DEPTH; override_index++)
    {
        if((pOverride[override_index] & mask) == pattern)
        {
            return override_index;
        }
    }
    return 0xFF;
}


//*****************************************************************************
//
//! Send command to doorbell and wait for ack
//
//*****************************************************************************
extern uint32_t RFCDoorbellSendTo(uint32_t pOp);


//*****************************************************************************
//
//! Turn off synth, NOTE: Radio will no longer respond to commands!
//
//*****************************************************************************
extern void RFCSynthPowerDown(void);


//*****************************************************************************
//
//! Read RF trim from flash using CM3
//
//*****************************************************************************
extern void RFCRfTrimRead(rfc_radioOp_t *pOpSetup, rfTrim_t* rfTrim);


//*****************************************************************************
//
//! Write preloaded RF trim values to CM0
//
//*****************************************************************************
extern void RFCRfTrimSet(rfTrim_t* rfTrim);


//*****************************************************************************
//
//! Check Override RTrim vs FCFG RTrim
//
//*****************************************************************************
extern void RFCRTrim(rfc_radioOp_t *pOpSetup);


//*****************************************************************************
//
//! Reset previously patched CPE RAM to a state where it can be patched again
//
//*****************************************************************************
extern void RFCCPEPatchReset(void);


//*****************************************************************************
//
//! Function to set VCOLDO reference to voltage mode
//
//*****************************************************************************
extern void RFCAdi3VcoLdoVoltageMode(bool bEnable);


//*****************************************************************************
//
//! Function to update override list
//
//*****************************************************************************
extern uint8_t RFCOverrideUpdate(rfc_radioOp_t *pOpSetup, uint32_t *pParams);


//*****************************************************************************
//
//! Get and clear HW interrupt flags
//
//*****************************************************************************
extern uint32_t RFCHWIntGetAndClear(uint32_t ui32Mask);



//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_RFCCpeIntGetAndClear
        #undef  RFCCpeIntGetAndClear
        #define RFCCpeIntGetAndClear            ROM_RFCCpeIntGetAndClear
    #endif
    #ifdef ROM_RFCDoorbellSendTo
        #undef  RFCDoorbellSendTo
        #define RFCDoorbellSendTo               ROM_RFCDoorbellSendTo
    #endif
    #ifdef ROM_RFCSynthPowerDown
        #undef  RFCSynthPowerDown
        #define RFCSynthPowerDown               ROM_RFCSynthPowerDown
    #endif
    #ifdef ROM_RFCRfTrimRead
        #undef  RFCRfTrimRead
        #define RFCRfTrimRead                   ROM_RFCRfTrimRead
    #endif
    #ifdef ROM_RFCRfTrimSet
        #undef  RFCRfTrimSet
        #define RFCRfTrimSet                    ROM_RFCRfTrimSet
    #endif
    #ifdef ROM_RFCRTrim
        #undef  RFCRTrim
        #define RFCRTrim                        ROM_RFCRTrim
    #endif
    #ifdef ROM_RFCCPEPatchReset
        #undef  RFCCPEPatchReset
        #define RFCCPEPatchReset                ROM_RFCCPEPatchReset
    #endif
    #ifdef ROM_RFCAdi3VcoLdoVoltageMode
        #undef  RFCAdi3VcoLdoVoltageMode
        #define RFCAdi3VcoLdoVoltageMode        ROM_RFCAdi3VcoLdoVoltageMode
    #endif
    #ifdef ROM_RFCOverrideUpdate
        #undef  RFCOverrideUpdate
        #define RFCOverrideUpdate               ROM_RFCOverrideUpdate
    #endif
    #ifdef ROM_RFCHWIntGetAndClear
        #undef  RFCHWIntGetAndClear
        #define RFCHWIntGetAndClear             ROM_RFCHWIntGetAndClear
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

#endif //  __RFC_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//
//*****************************************************************************
