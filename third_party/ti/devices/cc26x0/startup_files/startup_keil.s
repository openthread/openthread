; /******************************************************************************
; *  Filename:       startup_keil.c
; *  Revised:        $Date: 2016-09-21 10:57:23 +0200 (on, 21 sep 2016) $
; *  Revision:       $Revision: 17335 $
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

__Vectors       DCD     __initial_sp              ;  0 Top of Stack
                DCD     ResetISR                  ;  1 Reset Handler
                DCD     NmiSR                     ;  2 NMI Handler
                DCD     FaultISR                  ;  3 Hard Fault Handler
                DCD     MPUFaultIntHandler        ;  4 The MPU fault handler
                DCD     BusFaultIntHandler        ;  5 The bus fault handler
                DCD     UsageFaultIntHandler      ;  6 The usage fault handler
                DCD     0                         ;  7 Reserved
                DCD     0                         ;  8 Reserved
                DCD     0                         ;  9 Reserved
                DCD     0                         ; 10 Reserved
                DCD     SVCallIntHandler          ; 11 SVCall Handler
                DCD     DebugMonIntHandler        ; 12 Debug monitor handler
                DCD     0                         ; 13 Reserved
                DCD     PendSVIntHandler          ; 14 PendSV Handler
                DCD     SysTickIntHandler         ; 15 SysTick Handler

                ; External Interrupts
                DCD     GPIOIntHandler            ; 16 AON edge detect
                DCD     I2CIntHandler             ; 17 I2C
                DCD     RFCCPE1IntHandler         ; 18 RF Core Command & Packet Engine 1
                DCD     IntDefaultHandler         ; 19 Reserved
                DCD     AONRTCIntHandler          ; 20 AON RTC
                DCD     UART0IntHandler           ; 21 UART0 Rx and Tx
                DCD     IntDefaultHandler         ; 22 AUX software event 0
                DCD     SSI0IntHandler            ; 23 SSI0 Rx and Tx
                DCD     SSI1IntHandler            ; 24 SSI1 Rx and Tx
                DCD     RFCCPE0IntHandler         ; 25 RF Core Command & Packet Engine 0
                DCD     RFCHardwareIntHandler     ; 26 RF Core Hardware
                DCD     RFCCmdAckIntHandler       ; 27 RF Core Command Acknowledge
                DCD     I2SIntHandler             ; 28 I2S
                DCD     IntDefaultHandler         ; 29 AUX software event 1
                DCD     WatchdogIntHandler        ; 30 Watchdog timer
                DCD     Timer0AIntHandler         ; 31 Timer 0 subtimer A
                DCD     Timer0BIntHandler         ; 32 Timer 0 subtimer B
                DCD     Timer1AIntHandler         ; 33 Timer 1 subtimer A
                DCD     Timer1BIntHandler         ; 34 Timer 1 subtimer B
                DCD     Timer2AIntHandler         ; 35 Timer 2 subtimer A
                DCD     Timer2BIntHandler         ; 36 Timer 2 subtimer B
                DCD     Timer3AIntHandler         ; 37 Timer 3 subtimer A
                DCD     Timer3BIntHandler         ; 38 Timer 3 subtimer B
                DCD     CryptoIntHandler          ; 39 Crypto Core Result available
                DCD     uDMAIntHandler            ; 40 uDMA Software
                DCD     uDMAErrIntHandler         ; 41 uDMA Error
                DCD     FlashIntHandler           ; 42 Flash controller
                DCD     SWEvent0IntHandler        ; 43 Software Event 0
                DCD     AUXCombEventIntHandler    ; 44 AUX combined event
                DCD     AONProgIntHandler         ; 45 AON programmable 0
                DCD     DynProgIntHandler         ; 46 Dynamic Programmable interrupt
                                                  ;    source (Default: PRCM)
                DCD     AUXCompAIntHandler        ; 47 AUX Comparator A
                DCD     AUXADCIntHandler          ; 48 AUX ADC new sample or ADC DMA
                                                  ;    done, ADC underflow, ADC overflow
                DCD     TRNGIntHandler            ; 49 TRNG event
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
