/******************************************************************************
*  Filename:       startup_gcc.c
*  Revised:        $Date: 2016-05-19 11:08:26 +0200 (to, 19 mai 2016) $
*  Revision:       $Revision: 17109 $
*
*  Description:    Startup code for CC26xx PG2 device family for use with GCC.
*
*  Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
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
// Check if compiler is GNU Compiler
//
//*****************************************************************************
#if !(defined(__GNUC__))
#error "startup_gcc.c: Unsupported compiler!"
#endif

#include <inc/hw_types.h>
#include <driverlib/setup.h>

//*****************************************************************************
//
// Magic C++ functions
//
//*****************************************************************************
__extension__ typedef int __guard __attribute__((mode(__DI__)));
int __cxa_guard_acquire(__guard *g) { return !*(char *)(g); }
void __cxa_guard_release(__guard *g) { *(char *)g = 1; }
void __cxa_guard_abort(__guard *g) {};
/* XXX: alias this to the fault ISR ???? */
void __cxa_pure_virtual(void) { while (1); }


//*****************************************************************************
//
// Macro for weak symbol aliasing
//
//*****************************************************************************
#define WEAK_ALIAS(x) __attribute__ ((weak, alias(#x)))

//*****************************************************************************
//
// Forward declaration of the reset ISR and the default fault handlers.
//
//*****************************************************************************
void        ResetISR( void );
static void NmiSRHandler( void );
static void FaultISRHandler( void );
static void IntDefaultHandler( void );
extern int  main( void );


// Default interrupt handlers
void NmiSR(void) WEAK_ALIAS(NmiSRHandler);
void FaultISR(void) WEAK_ALIAS(FaultISRHandler);
void MPUFaultIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void BusFaultIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void UsageFaultIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void SVCallIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void DebugMonIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void PendSVIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void SysTickIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void GPIOIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void I2CIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void RFCCPE1IntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void AONIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void AONRTCIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void UART0IntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void AUXSWEvent0IntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void SSI0IntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void SSI1IntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void RFCCPE0IntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void RFCHardwareIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void RFCCmdAckIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void I2SIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void AUXSWEvent1IntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void WatchdogIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void Timer0AIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void Timer0BIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void Timer1AIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void Timer1BIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void Timer2AIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void Timer2BIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void Timer3AIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void Timer3BIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void CryptoIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void uDMAIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void uDMAErrIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void FlashIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void SWEvent0IntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void AUXCombEventIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void AONProgIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void DynProgIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void AUXCompAIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void AUXADCIntHandler(void) WEAK_ALIAS(IntDefaultHandler);
void TRNGIntHandler(void) WEAK_ALIAS(IntDefaultHandler);


//*****************************************************************************
//
// The following are constructs created by the linker, indicating where the
// the "data" and "bss" segments reside in memory.
//
//*****************************************************************************
extern uint32_t _etext;
extern uint32_t _data;
extern uint32_t _edata;
extern uint32_t _bss;
extern uint32_t _ebss;
extern uint32_t _estack;

//*****************************************************************************
//
//! The vector table. Note that the proper constructs must be placed on this to
//! ensure that it ends up at physical address 0x0000.0000 or at the start of
//! the program if located at a start address other than 0.
//
//*****************************************************************************
__attribute__ ((section(".vectors"), used))
void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))&_estack,               // The initial stack pointer 
    ResetISR,                               // The reset handler
    NmiSR,                                  // The NMI handler
    FaultISR,                               // The hard fault handler
    MPUFaultIntHandler,                     // The MPU fault handler
    BusFaultIntHandler,                     // The bus fault handler
    UsageFaultIntHandler,                   // The usage fault handler
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    SVCallIntHandler,                       // SVCall handler
    DebugMonIntHandler,                     // Debug monitor handler
    0,                                      // Reserved
    PendSVIntHandler,                       // The PendSV handler
    SysTickIntHandler,                      // The SysTick handler
    GPIOIntHandler,                         // AON edge detect
    I2CIntHandler,                          // I2C
    RFCCPE1IntHandler,                      // RF Core Command & Packet Engine 1
    AONIntHandler,                          // AON SpiSplave Rx, Tx and CS
    AONRTCIntHandler,                       // AON RTC
    UART0IntHandler,                        // UART0 Rx and Tx
    AUXSWEvent0IntHandler,                  // AUX software event 0
    SSI0IntHandler,                         // SSI0 Rx and Tx
    SSI1IntHandler,                         // SSI1 Rx and Tx
    RFCCPE0IntHandler,                      // RF Core Command & Packet Engine 0
    RFCHardwareIntHandler,                  // RF Core Hardware
    RFCCmdAckIntHandler,                    // RF Core Command Acknowledge
    I2SIntHandler,                          // I2S
    AUXSWEvent1IntHandler,                  // AUX software event 1
    WatchdogIntHandler,                     // Watchdog timer
    Timer0AIntHandler,                      // Timer 0 subtimer A
    Timer0BIntHandler,                      // Timer 0 subtimer B
    Timer1AIntHandler,                      // Timer 1 subtimer A
    Timer1BIntHandler,                      // Timer 1 subtimer B
    Timer2AIntHandler,                      // Timer 2 subtimer A
    Timer2BIntHandler,                      // Timer 2 subtimer B
    Timer3AIntHandler,                      // Timer 3 subtimer A
    Timer3BIntHandler,                      // Timer 3 subtimer B
    CryptoIntHandler,                       // Crypto Core Result available
    uDMAIntHandler,                         // uDMA Software
    uDMAErrIntHandler,                      // uDMA Error
    FlashIntHandler,                        // Flash controller
    SWEvent0IntHandler,                     // Software Event 0
    AUXCombEventIntHandler,                 // AUX combined event
    AONProgIntHandler,                      // AON programmable 0
    DynProgIntHandler,                      // Dynamic Programmable interrupt
                                            // source (Default: PRCM)
    AUXCompAIntHandler,                     // AUX Comparator A
    AUXADCIntHandler,                       // AUX ADC new sample or ADC DMA
                                            // done, ADC underflow, ADC overflow
    TRNGIntHandler                          // TRNG event
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
    uint32_t *pui32Src, *pui32Dest;

    //
    // Final trim of device
    //
    SetupTrimDevice();
    
    //
    // Copy the data segment initializers from flash to SRAM.
    //
    pui32Src = &_etext;
    for(pui32Dest = &_data; pui32Dest < &_edata; )
    {
        *pui32Dest++ = *pui32Src++;
    }

    //
    // Zero fill the bss segment.
    //
    __asm("    ldr     r0, =_bss\n"
          "    ldr     r1, =_ebss\n"
          "    mov     r2, #0\n"
          "    .thumb_func\n"
          "zero_loop:\n"
          "        cmp     r0, r1\n"
          "        it      lt\n"
          "        strlt   r2, [r0], #4\n"
          "        blt     zero_loop");

   //
   // Call the application's entry point.
   //
   main();

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
