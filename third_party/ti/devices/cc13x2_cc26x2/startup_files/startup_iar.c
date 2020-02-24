/******************************************************************************
*  Filename:       startup_iar.c
*  Revised:        $Date: 2017-02-03 19:16:24 +0100 (Fri, 03 Feb 2017) $
*  Revision:       $Revision: 17634 $
*
*  Description:    Startup code for CC26x2 device family for use with IAR.
*
*  Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/


//*****************************************************************************
//
// Check if compiler is IAR
//
//*****************************************************************************
#if !(defined(__IAR_SYSTEMS_ICC__))
#error "startup_iar.c: Unsupported compiler!"
#endif


// We need intrinsic functions for IAR (if used in source code)
#include <intrinsics.h>
#include "../inc/hw_types.h"
#include "../driverlib/setup.h"


//*****************************************************************************
//
//! Forward declaration of the reset ISR and the default fault handlers.
//
//*****************************************************************************
void        ResetISR( void );
static void NmiSRHandler( void );
static void FaultISRHandler( void );
static void IntDefaultHandler( void );
extern int  main( void );

extern void NmiSR( void );
extern void FaultISR( void );
extern void MPUFaultIntHandler( void );
extern void BusFaultIntHandler( void );
extern void UsageFaultIntHandler( void );
extern void SVCallIntHandler( void );
extern void DebugMonIntHandler( void );
extern void PendSVIntHandler( void );
extern void SysTickIntHandler( void );
extern void GPIOIntHandler( void );
extern void I2CIntHandler( void );
extern void RFCCPE1IntHandler( void );
extern void PKAIntHandler( void );
extern void AONRTCIntHandler( void );
extern void UART0IntHandler( void );
extern void AUXSWEvent0IntHandler( void );
extern void SSI0IntHandler( void );
extern void SSI1IntHandler( void );
extern void RFCCPE0IntHandler( void );
extern void RFCHardwareIntHandler( void );
extern void RFCCmdAckIntHandler( void );
extern void I2SIntHandler( void );
extern void AUXSWEvent1IntHandler( void );
extern void WatchdogIntHandler( void );
extern void Timer0AIntHandler( void );
extern void Timer0BIntHandler( void );
extern void Timer1AIntHandler( void );
extern void Timer1BIntHandler( void );
extern void Timer2AIntHandler( void );
extern void Timer2BIntHandler( void );
extern void Timer3AIntHandler( void );
extern void Timer3BIntHandler( void );
extern void CryptoIntHandler( void );
extern void uDMAIntHandler( void );
extern void uDMAErrIntHandler( void );
extern void FlashIntHandler( void );
extern void SWEvent0IntHandler( void );
extern void AUXCombEventIntHandler( void );
extern void AONProgIntHandler( void );
extern void DynProgIntHandler( void );
extern void AUXCompAIntHandler( void );
extern void AUXADCIntHandler( void );
extern void TRNGIntHandler( void );
extern void OSCIntHandler( void );
extern void AUXTimer2IntHandler( void );
extern void UART1IntHandler( void );
extern void BatMonIntHandler( void );

// Default interrupt handlers
#pragma weak NmiSR = NmiSRHandler
#pragma weak FaultISR = FaultISRHandler
#pragma weak MPUFaultIntHandler = IntDefaultHandler
#pragma weak BusFaultIntHandler = IntDefaultHandler
#pragma weak UsageFaultIntHandler = IntDefaultHandler
#pragma weak SVCallIntHandler = IntDefaultHandler
#pragma weak DebugMonIntHandler = IntDefaultHandler
#pragma weak PendSVIntHandler = IntDefaultHandler
#pragma weak SysTickIntHandler = IntDefaultHandler
#pragma weak GPIOIntHandler = IntDefaultHandler
#pragma weak I2CIntHandler = IntDefaultHandler
#pragma weak RFCCPE1IntHandler = IntDefaultHandler
#pragma weak PKAIntHandler = IntDefaultHandler
#pragma weak AONRTCIntHandler = IntDefaultHandler
#pragma weak UART0IntHandler = IntDefaultHandler
#pragma weak AUXSWEvent0IntHandler = IntDefaultHandler
#pragma weak SSI0IntHandler = IntDefaultHandler
#pragma weak SSI1IntHandler = IntDefaultHandler
#pragma weak RFCCPE0IntHandler = IntDefaultHandler
#pragma weak RFCHardwareIntHandler = IntDefaultHandler
#pragma weak RFCCmdAckIntHandler = IntDefaultHandler
#pragma weak I2SIntHandler = IntDefaultHandler
#pragma weak AUXSWEvent1IntHandler = IntDefaultHandler
#pragma weak WatchdogIntHandler = IntDefaultHandler
#pragma weak Timer0AIntHandler = IntDefaultHandler
#pragma weak Timer0BIntHandler = IntDefaultHandler
#pragma weak Timer1AIntHandler = IntDefaultHandler
#pragma weak Timer1BIntHandler = IntDefaultHandler
#pragma weak Timer2AIntHandler = IntDefaultHandler
#pragma weak Timer2BIntHandler = IntDefaultHandler
#pragma weak Timer3AIntHandler = IntDefaultHandler
#pragma weak Timer3BIntHandler = IntDefaultHandler
#pragma weak CryptoIntHandler = IntDefaultHandler
#pragma weak uDMAIntHandler = IntDefaultHandler
#pragma weak uDMAErrIntHandler = IntDefaultHandler
#pragma weak FlashIntHandler = IntDefaultHandler
#pragma weak SWEvent0IntHandler = IntDefaultHandler
#pragma weak AUXCombEventIntHandler = IntDefaultHandler
#pragma weak AONProgIntHandler = IntDefaultHandler
#pragma weak DynProgIntHandler = IntDefaultHandler
#pragma weak AUXCompAIntHandler = IntDefaultHandler
#pragma weak AUXADCIntHandler = IntDefaultHandler
#pragma weak TRNGIntHandler = IntDefaultHandler
#pragma weak OSCIntHandler = IntDefaultHandler
#pragma weak AUXTimer2IntHandler = IntDefaultHandler
#pragma weak UART1IntHandler = IntDefaultHandler
#pragma weak BatMonIntHandler = IntDefaultHandler

//*****************************************************************************
//
//! The entry point for the application startup code.
//
//*****************************************************************************
extern void __iar_program_start(void);

//*****************************************************************************
//
//! Get stack start (highest address) symbol from linker file.
//
//*****************************************************************************
extern const void* STACK_TOP;

// It is required to place something in the CSTACK segment to get the stack
// check feature in IAR to work as expected
__root static void* dummy_stack @ ".stack";


//*****************************************************************************
//
//! The vector table. Note that the proper constructs must be placed on this to
//! ensure that it ends up at physical address 0x0000.0000 or at the start of
//! the program if located at a start address other than 0.
//
//*****************************************************************************
__root void (* const __vector_table[])(void) @ ".intvec" =
{
    (void (*)(void))&STACK_TOP,             //  0 The initial stack pointer
    ResetISR,                               //  1 The reset handler
    NmiSR,                                  //  2 The NMI handler
    FaultISR,                               //  3 The hard fault handler
    MPUFaultIntHandler,                     //  4 Memory Management (MemManage) Fault
    BusFaultIntHandler,                     //  5 The bus fault handler
    UsageFaultIntHandler,                   //  6 The usage fault handler
    0,                                      //  7 Reserved
    0,                                      //  8 Reserved
    0,                                      //  9 Reserved
    0,                                      // 10 Reserved
    SVCallIntHandler,                       // 11 Supervisor Call (SVCall)
    DebugMonIntHandler,                     // 12 Debug monitor handler
    0,                                      // 13 Reserved
    PendSVIntHandler,                       // 14 The PendSV handler
    SysTickIntHandler,                      // 15 The SysTick handler
    //--- External interrupts ---
    GPIOIntHandler,                         // 16 AON edge detect
    I2CIntHandler,                          // 17 I2C
    RFCCPE1IntHandler,                      // 18 RF Core Command & Packet Engine 1
    PKAIntHandler,                          // 19 PKA Interrupt event
    AONRTCIntHandler,                       // 20 AON RTC
    UART0IntHandler,                        // 21 UART0 Rx and Tx
    AUXSWEvent0IntHandler,                  // 22 AUX software event 0
    SSI0IntHandler,                         // 23 SSI0 Rx and Tx
    SSI1IntHandler,                         // 24 SSI1 Rx and Tx
    RFCCPE0IntHandler,                      // 25 RF Core Command & Packet Engine 0
    RFCHardwareIntHandler,                  // 26 RF Core Hardware
    RFCCmdAckIntHandler,                    // 27 RF Core Command Acknowledge
    I2SIntHandler,                          // 28 I2S
    AUXSWEvent1IntHandler,                  // 29 AUX software event 1
    WatchdogIntHandler,                     // 30 Watchdog timer
    Timer0AIntHandler,                      // 31 Timer 0 subtimer A
    Timer0BIntHandler,                      // 32 Timer 0 subtimer B
    Timer1AIntHandler,                      // 33 Timer 1 subtimer A
    Timer1BIntHandler,                      // 34 Timer 1 subtimer B
    Timer2AIntHandler,                      // 35 Timer 2 subtimer A
    Timer2BIntHandler,                      // 36 Timer 2 subtimer B
    Timer3AIntHandler,                      // 37 Timer 3 subtimer A
    Timer3BIntHandler,                      // 38 Timer 3 subtimer B
    CryptoIntHandler,                       // 39 Crypto Core Result available
    uDMAIntHandler,                         // 40 uDMA Software
    uDMAErrIntHandler,                      // 41 uDMA Error
    FlashIntHandler,                        // 42 Flash controller
    SWEvent0IntHandler,                     // 43 Software Event 0
    AUXCombEventIntHandler,                 // 44 AUX combined event
    AONProgIntHandler,                      // 45 AON programmable 0
    DynProgIntHandler,                      // 46 Dynamic Programmable interrupt
                                            //    source (Default: PRCM)
    AUXCompAIntHandler,                     // 47 AUX Comparator A
    AUXADCIntHandler,                       // 48 AUX ADC new sample or ADC DMA
                                            //    done, ADC underflow, ADC overflow
    TRNGIntHandler,                         // 49 TRNG event
    OSCIntHandler,                          // 50 Combined event from Oscillator control
    AUXTimer2IntHandler,                    // 51 AUX Timer2 event 0
    UART1IntHandler,                        // 52 UART1 combined interrupt
    BatMonIntHandler                        // 53 Combined event from battery monitor
};


//*****************************************************************************
//
//! This is the code that gets called when the processor first starts execution
//! following a reset event. Only the absolutely necessary set is performed,
//! after which the application supplied entry() routine is called. Any fancy
//! actions (such as making decisions based on the reset cause register, and
//! resetting the bits in that register) are left solely in the hands of the
//! application.
//
//*****************************************************************************
void
ResetISR(void)
{
    //
    // Final trim of device
    //
    SetupTrimDevice();

    //
    // Jump to IAR initialization routine
    //
    __iar_program_start();

    //
    // If we ever return signal Error
    //
    FaultISR();
}

//*****************************************************************************
//
//! This is the code that gets called when the processor receives a NMI. This
//! simply enters an infinite loop, preserving the system state for examination
//! by a debugger.
//
//*****************************************************************************
static void
NmiSRHandler(void)
{
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
//! This is the code that gets called when the processor receives a fault
//! interrupt. This simply enters an infinite loop, preserving the system state
//! for examination by a debugger.
//
//*****************************************************************************
static void
FaultISRHandler(void)
{
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
//! This is the code that gets called when the processor receives an unexpected
//! interrupt. This simply enters an infinite loop, preserving the system state
//! for examination by a debugger.
//
//*****************************************************************************
static void
IntDefaultHandler(void)
{
    //
    // Go into an infinite loop.
    //
    while(1)
    {
    }
}
