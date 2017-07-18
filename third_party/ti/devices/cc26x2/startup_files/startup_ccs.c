/******************************************************************************
*  Filename:       startup_ccs.c
*  Revised:        $Date: 2017-02-03 19:16:24 +0100 (fr, 03 feb 2017) $
*  Revision:       $Revision: 17634 $
*
*  Description:    Startup code for CC26x2 device family for use with CCS.
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
// Check if compiler is CCS
//
//*****************************************************************************
#if !(defined(__TI_COMPILER_VERSION__))
#error "startup_ccs.c: Unsupported compiler!"
#endif

#include "../inc/hw_types.h"
#include "../driverlib/setup.h"


//*****************************************************************************
//
//! Forward declaration of the reset ISR and the default fault handlers.
//
//*****************************************************************************
void        ResetISR( void );
static void NmiSR( void );
static void FaultISR( void );
static void IntDefaultHandler( void );
extern int  main(void);


//*****************************************************************************
//
//! The entry point for the application startup code and device trim fxn.
//
//*****************************************************************************
extern void _c_int00(void);


//*****************************************************************************
//
// CCS: Linker variable that marks the top of the stack.
//
//*****************************************************************************
extern unsigned long __STACK_END;


//! The vector table. Note that the proper constructs must be placed on this to
//! ensure that it ends up at physical address 0x0000.0000 or at the start of
//! the program if located at a start address other than 0.
//
//*****************************************************************************
#pragma DATA_SECTION(g_pfnVectors, ".intvecs")
void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))((unsigned long)&__STACK_END),
                                            //  0 The initial stack pointer
    ResetISR,                               //  1 The reset handler
    NmiSR,                                  //  2 The NMI handler
    FaultISR,                               //  3 The hard fault handler
    IntDefaultHandler,                      //  4 Memory Management (MemManage) Fault
    IntDefaultHandler,                      //  5 The bus fault handler
    IntDefaultHandler,                      //  6 The usage fault handler
    0,                                      //  7 Reserved
    0,                                      //  8 Reserved
    0,                                      //  9 Reserved
    0,                                      // 10 Reserved
    IntDefaultHandler,                      // 11 Supervisor Call (SVCall)
    IntDefaultHandler,                      // 12 Debug monitor handler
    0,                                      // 13 Reserved
    IntDefaultHandler,                      // 14 The PendSV handler
    IntDefaultHandler,                      // 15 The SysTick handler
    //--- External interrupts ---
    IntDefaultHandler,                      // 16 AON edge detect
    IntDefaultHandler,                      // 17 I2C
    IntDefaultHandler,                      // 18 RF Core Command & Packet Engine 1
    IntDefaultHandler,                      // 19 PKA Interrupt event
    IntDefaultHandler,                      // 20 AON RTC
    IntDefaultHandler,                      // 21 UART0 Rx and Tx
    IntDefaultHandler,                      // 22 AUX software event 0
    IntDefaultHandler,                      // 23 SSI0 Rx and Tx
    IntDefaultHandler,                      // 24 SSI1 Rx and Tx
    IntDefaultHandler,                      // 25 RF Core Command & Packet Engine 0
    IntDefaultHandler,                      // 26 RF Core Hardware
    IntDefaultHandler,                      // 27 RF Core Command Acknowledge
    IntDefaultHandler,                      // 28 I2S
    IntDefaultHandler,                      // 29 AUX software event 1
    IntDefaultHandler,                      // 30 Watchdog timer
    IntDefaultHandler,                      // 31 Timer 0 subtimer A
    IntDefaultHandler,                      // 32 Timer 0 subtimer B
    IntDefaultHandler,                      // 33 Timer 1 subtimer A
    IntDefaultHandler,                      // 34 Timer 1 subtimer B
    IntDefaultHandler,                      // 35 Timer 2 subtimer A
    IntDefaultHandler,                      // 36 Timer 2 subtimer B
    IntDefaultHandler,                      // 37 Timer 3 subtimer A
    IntDefaultHandler,                      // 38 Timer 3 subtimer B
    IntDefaultHandler,                      // 39 Crypto Core Result available
    IntDefaultHandler,                      // 40 uDMA Software
    IntDefaultHandler,                      // 41 uDMA Error
    IntDefaultHandler,                      // 42 Flash controller
    IntDefaultHandler,                      // 43 Software Event 0
    IntDefaultHandler,                      // 44 AUX combined event
    IntDefaultHandler,                      // 45 AON programmable 0
    IntDefaultHandler,                      // 46 Dynamic Programmable interrupt
                                            //    source (Default: PRCM)
    IntDefaultHandler,                      // 47 AUX Comparator A
    IntDefaultHandler,                      // 48 AUX ADC new sample or ADC DMA
                                            //    done, ADC underflow, ADC overflow
    IntDefaultHandler,                      // 49 TRNG event
    IntDefaultHandler,                      // 50 Combined event from Oscillator control
    IntDefaultHandler,                      // 51 AUX Timer2 event 0
    IntDefaultHandler,                      // 52 UART1 combined interrupt
    IntDefaultHandler                       // 53 Combined event from battery monitor
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
    // Jump to the CCS C Initialization Routine.
    //
    __asm("    .global _c_int00\n"
            "    b.w     _c_int00");

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
NmiSR(void)
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
FaultISR(void)
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
