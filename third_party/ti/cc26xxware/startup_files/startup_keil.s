; /******************************************************************************
; *  Filename:       startup_keil.c
; *  Revised:        $Date: 2016-05-20 17:00:40 +0200 (fr, 20 mai 2016) $
; *  Revision:       $Revision: 17133 $
; *
; *  Description:    Startup code for CC26xx device family for use with KEIL.
; *
; *  Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
; *
; *
; *  Redistribution and use in source and binary forms, with or without
; *  modification, are permitted provided that the following conditions
; *  are met:
; *
; *    Redistributions of source code must retain the above copyright
; *    notice, this list of conditions and the following disclaimer.
; *
; *    Redistributions in binary form must reproduce the above copyright
; *    notice, this list of conditions and the following disclaimer in the
; *    documentation and/or other materials provided with the distribution.
; *
; *    Neither the name of Texas Instruments Incorporated nor the names of
; *    its contributors may be used to endorse or promote products derived
; *    from this software without specific prior written permission.
; *
; *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
; *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
; *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
; *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
; *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
; *
; ******************************************************************************/
;// <<< Use Configuration Wizard in Context Menu >>>
;*/


; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>
Stack_Size      EQU     0x00000200

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>
Heap_Size       EQU     0x00000000

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit


                PRESERVE8
                THUMB
;// <<< end of configuration section >>>

; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     ResetISR                  ; Reset Handler
                DCD     NmiSR                     ; NMI Handler
                DCD     FaultISR                  ; Hard Fault Handler
                DCD     MPUFaultIntHandler        ; The MPU fault handler
                DCD     BusFaultIntHandler        ; The bus fault handler
                DCD     UsageFaultIntHandler      ; The usage fault handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVCallIntHandler          ; SVCall Handler
                DCD     DebugMonIntHandler        ; Debug monitor handler
                DCD     0                         ; Reserved
                DCD     PendSVIntHandler          ; PendSV Handler
                DCD     SysTickIntHandler         ; SysTick Handler

                ; External Interrupts
                DCD     GPIOIntHandler            ; 0 AON edge detect
                DCD     I2CIntHandler             ; 1 I2C
                DCD     RFCCPE1IntHandler         ; 2 RF Core Command & Packet Engine 1
                DCD     AONIntHandler             ; 3 AON SpiSplave Rx, Tx and CS
                DCD     AONRTCIntHandler          ; 4 AON RTC
                DCD     UART0IntHandler           ; 5 UART0 Rx and Tx
                DCD     IntDefaultHandler         ; 6 AUX software event 0
                DCD     SSI0IntHandler            ; 7 SSI0 Rx and Tx
                DCD     SSI1IntHandler            ; 8 SSI1 Rx and Tx
                DCD     RFCCPE0IntHandler         ; 9 RF Core Command & Packet Engine 0
                DCD     RFCHardwareIntHandler     ; 10 RF Core Hardware
                DCD     RFCCmdAckIntHandler       ; 11 RF Core Command Acknowledge
                DCD     I2SIntHandler             ; 12 I2S
                DCD     IntDefaultHandler         ; 13 AUX software event 1
                DCD     WatchdogIntHandler        ; 14 Watchdog timer
                DCD     Timer0AIntHandler         ; 15 Timer 0 subtimer A
                DCD     Timer0BIntHandler         ; 16 Timer 0 subtimer B
                DCD     Timer1AIntHandler         ; 17 Timer 1 subtimer A
                DCD     Timer1BIntHandler         ; 18 Timer 1 subtimer B
                DCD     Timer2AIntHandler         ; 19 Timer 2 subtimer A
                DCD     Timer2BIntHandler         ; 20 Timer 2 subtimer B
                DCD     Timer3AIntHandler         ; 21 Timer 3 subtimer A
                DCD     Timer3BIntHandler         ; 22 Timer 3 subtimer B
                DCD     CryptoIntHandler          ; 23 Crypto Core Result available
                DCD     uDMAIntHandler            ; 24 uDMA Software
                DCD     uDMAErrIntHandler         ; 25 uDMA Error
                DCD     FlashIntHandler           ; 26 Flash controller
                DCD     SWEvent0IntHandler        ; 27 Software Event 0
                DCD     AUXCombEventIntHandler    ; 28 AUX combined event
                DCD     AONProgIntHandler         ; 29 AON programmable 0
                DCD     DynProgIntHandler         ; 30 Dynamic Programmable interrupt
                                                  ;    source (Default: PRCM)
                DCD     AUXCompAIntHandler        ; 31 AUX Comparator A
                DCD     AUXADCIntHandler          ; 32 AUX ADC new sample or ADC DMA
                                                  ;    done, ADC underflow, ADC overflow
                DCD     TRNGIntHandler            ; 33 TRNG event
__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY


; Reset Handler

ResetISR        PROC
                EXPORT  ResetISR                  [WEAK]
                IMPORT  NOROM_SetupTrimDevice
                IMPORT  __main
                ; Final trim of device (setup.c in CC26xxWare)
                LDR     R0, =NOROM_SetupTrimDevice
                BLX     R0
                ; Jump to initialization routine
                LDR     R0, =__main
                BX      R0
                ; If we ever return, signal error
                LDR     R0, =FaultISR
                BX      R0
                ENDP


; Dummy Exception Handlers (infinite loops which can be modified)

NmiSR           PROC
                EXPORT  NmiSR                     [WEAK]
                B       .
                ENDP
FaultISR        PROC
                EXPORT  FaultISR                  [WEAK]
                B       .
                ENDP
MPUFaultIntHandler\
                PROC
                EXPORT  MPUFaultIntHandler        [WEAK]
                B       .
                ENDP
BusFaultIntHandler\
                PROC
                EXPORT  BusFaultIntHandler        [WEAK]
                B       .
                ENDP
UsageFaultIntHandler\
                PROC
                EXPORT  UsageFaultIntHandler      [WEAK]
                B       .
                ENDP
SVCallIntHandler\
                PROC
                EXPORT  SVCallIntHandler          [WEAK]
                B       .
                ENDP
DebugMonIntHandler\
                PROC
                EXPORT  DebugMonIntHandler        [WEAK]
                B       .
                ENDP
PendSVIntHandler\
                PROC
                EXPORT  PendSVIntHandler          [WEAK]
                B       .
                ENDP
SysTickIntHandler\
                PROC
                EXPORT  SysTickIntHandler         [WEAK]
                B       .
                ENDP

IntDefaultHandler\
                PROC
                EXPORT  GPIOIntHandler            [WEAK]
                EXPORT  I2CIntHandler             [WEAK]
                EXPORT  RFCCPE1IntHandler         [WEAK]
                EXPORT  AONIntHandler             [WEAK]
                EXPORT  AONRTCIntHandler          [WEAK]
                EXPORT  UART0IntHandler           [WEAK]
                EXPORT  SSI0IntHandler            [WEAK]
                EXPORT  SSI1IntHandler            [WEAK]
                EXPORT  RFCCPE0IntHandler         [WEAK]
                EXPORT  RFCHardwareIntHandler     [WEAK]
                EXPORT  RFCCmdAckIntHandler       [WEAK]
                EXPORT  I2SIntHandler             [WEAK]
                EXPORT  WatchdogIntHandler        [WEAK]
                EXPORT  Timer0AIntHandler         [WEAK]
                EXPORT  Timer0BIntHandler         [WEAK]
                EXPORT  Timer1AIntHandler         [WEAK]
                EXPORT  Timer1BIntHandler         [WEAK]
                EXPORT  Timer2AIntHandler         [WEAK]
                EXPORT  Timer2BIntHandler         [WEAK]
                EXPORT  Timer3AIntHandler         [WEAK]
                EXPORT  Timer3BIntHandler         [WEAK]
                EXPORT  CryptoIntHandler          [WEAK]
                EXPORT  uDMAIntHandler            [WEAK]
                EXPORT  uDMAErrIntHandler         [WEAK]
                EXPORT  FlashIntHandler           [WEAK]
                EXPORT  SWEvent0IntHandler        [WEAK]
                EXPORT  AUXCombEventIntHandler    [WEAK]
                EXPORT  AONProgIntHandler         [WEAK]
                EXPORT  DynProgIntHandler         [WEAK]
                EXPORT  AUXCompAIntHandler        [WEAK]
                EXPORT  AUXADCIntHandler          [WEAK]
                EXPORT  TRNGIntHandler            [WEAK]
GPIOIntHandler
I2CIntHandler
RFCCPE1IntHandler
AONIntHandler
AONRTCIntHandler
UART0IntHandler
SSI0IntHandler
SSI1IntHandler
RFCCPE0IntHandler
RFCHardwareIntHandler
RFCCmdAckIntHandler
I2SIntHandler
WatchdogIntHandler
Timer0AIntHandler
Timer0BIntHandler
Timer1AIntHandler
Timer1BIntHandler
Timer2AIntHandler
Timer2BIntHandler
Timer3AIntHandler
Timer3BIntHandler
CryptoIntHandler
uDMAIntHandler
uDMAErrIntHandler
FlashIntHandler
SWEvent0IntHandler
AUXCombEventIntHandler
AONProgIntHandler
DynProgIntHandler
AUXCompAIntHandler
AUXADCIntHandler
TRNGIntHandler
                B       .
                ENDP


                ALIGN


; User Initial Stack & Heap

                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit

                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap

                LDR     R0, =  Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF


                END
