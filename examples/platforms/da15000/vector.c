/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
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
 */

/**
* @file     cpu/dialog/da15100/boot/vector.c
* CpuIrq_da15x -- IRQ driver for ARM Cortex-M series cores.
*
*/

#include "cpu.h"
#include "tool.h"

#pragma GCC diagnostic ignored "-Wpedantic"
#pragma location=".isr_vector"
/// Declaration of default interrupt service routine
static void halt_isr(void)           {
    __asm ("BKPT 255");
    while(1);
}
static void default_isr(void)        {
    while(1);
}

/* The kernel interrupts - in their CMSIS form.
   These must be implemented locally, or compiler will
   give conflicting definition for __vector_table.
 */
__WEAK void NMI_Handler()            { }
__WEAK void HardFault_Handler()      {
    halt_isr();
}
__WEAK void SVC_Handler()            {
    halt_isr();
}
__WEAK void PendSV_Handler()         {
    halt_isr();
}
__WEAK void SysTick_Handler()        {
    halt_isr();
}

__WEAK void MemManage_Handler()      {
    halt_isr();
}
__WEAK void DebugMon_Handler()       {
    halt_isr();
}
__WEAK void BusFault_Handler()       {
    halt_isr();
}

/* Default handlers for other DA15X peripheral interrupts. */
__WEAK void DEBUG_IRQHandler()       {
    default_isr();
}


__WEAK void BLE_WAKEUP_LP_Handler()  {
    default_isr();    /*  0 */
}
__WEAK void BLE_GEN_Handler()        {
    default_isr();    /*  1 */
}
__WEAK void FTDF_WAKEUP_Handler()    {
    default_isr();    /*  2 */
}
__WEAK void FTDF_GEN_Handler()       {
    default_isr();    /*  3 */
}
__WEAK void RFCAL_Handler()          {
    default_isr();    /*  4 */
}
__WEAK void COEX_Handler()           {
    default_isr();    /*  5 */
}
__WEAK void CRYPTO_Handler()         {
    default_isr();    /*  6 */
}
__WEAK void MRM_Handler()            {
    default_isr();    /*  7 */
}
__WEAK void UART_Handler()           {
    default_isr();    /*  8 */
}
__WEAK void UART2_Handler()          {
    default_isr();    /*  9 */
}
__WEAK void I2C_Handler()            {
    default_isr();    /* 10 */
}
__WEAK void I2C2_Handler()           {
    default_isr();    /* 11 */
}
__WEAK void SPI_Handler()            {
    default_isr();    /* 12 */
}
__WEAK void SPI2_Handler()           {
    default_isr();    /* 13 */
}
__WEAK void ADC_Handler()            {
    default_isr();    /* 14 */
}
__WEAK void KEYBRD_Handler()         {
    default_isr();    /* 15 */
}
__WEAK void IRGEN_Handler()          {
    default_isr();    /* 16 */
}
__WEAK void WKUP_GPIO_Handler()      {
    default_isr();    /* 17 */
}
__WEAK void SWTIM0_Handler()         {
    default_isr();    /* 18 */
}
__WEAK void SWTIM1_Handler()         {
    default_isr();    /* 19 */
}
__WEAK void QUADEC_Handler()         {
    default_isr();    /* 20 */
}
__WEAK void USB_Handler()            {
    default_isr();    /* 21 */
}
__WEAK void PCM_Handler()            {
    default_isr();    /* 22 */
}
__WEAK void SRC_IN_Handler()         {
    default_isr();    /* 23 */
}
__WEAK void SRC_OUT_Handler()        {
    default_isr();    /* 24 */
}
__WEAK void VBUS_Handler()           {
    default_isr();    /* 25 */
}
__WEAK void DMA_Handler()            {
    default_isr();    /* 26 */
}
__WEAK void RF_DIAG_Handler()        {
    default_isr();    /* 27 */
}
__WEAK void TRNG_Handler()           {
    default_isr();    /* 28 */
}
__WEAK void DCDC_Handler()           {
    default_isr();    /* 29 */
}
__WEAK void XTAL16RDY_Handler()      {
    default_isr();    /* 30 */
}
__WEAK void RESERVED31_Handler()     {
    default_isr();    /* 31 */
}




__BOOT_VECTOR __USED
const irq_handler_t __vector_table[] =
{
    // Cortex-M vector table
    (irq_handler_t)__BOOT_STACK,          //INT_Initial_Stack_Pointer    = 0,  // Initial Stack Pointer
    (irq_handler_t)__BOOT_STARTUP,        //INT_Initial_Program_Counter  = 1,  // Initial Program Counter
    NMI_Handler,                          //INT_NMI                      = 2,  // Non-maskable Interrupt (NMI)
    HardFault_Handler,                    //INT_Hard_Fault               = 3,  // Hard Fault
    MemManage_Handler,                    //INT_Mem_Manage_Fault         = 4,  // MemManage Fault
    BusFault_Handler,                     //INT_Bus_Fault                = 5,  // Bus Fault
    default_isr,                          //INT_Usage_Fault              = 6,  // Usage Fault
    default_isr,                          //INT_Reserved7                = 7,  // Reserved interrupt 7
    default_isr,                          //INT_Reserved8                = 8,  // Reserved interrupt 8
    default_isr,                          //INT_Reserved9                = 9,  // Reserved interrupt 9
    default_isr,                          //INT_Reserved10               = 10, // Reserved interrupt 10
    SVC_Handler,                          //INT_SVCall                   = 11, // Supervisor call (SVCall)
    DebugMon_Handler,                     //INT_DebugMonitor             = 12, // Debug Monitor
    default_isr,                          //INT_Reserved13               = 13, // Reserved interrupt 13
    PendSV_Handler,                       //INT_PendableSrvReq           = 14, // Pendable request for system service (PendableSrvReq)
    SysTick_Handler,                      //INT_SysTick                  = 15, // SysTick Interrupt

    // Dialog DA15100 vector table
    BLE_WAKEUP_LP_Handler,                //BLE_WAKEUP_LP_IRQn           =   0, // Wakeup from Sleep by BLE Interrupt request.
    BLE_GEN_Handler,                      //BLE_GEN_IRQn                 =   1, // BLE Interrupt request. Sources:
    FTDF_WAKEUP_Handler,                  //FTDF_WAKEUP_IRQn             =   2, // Wakeup from Sleep by FTDF Interrupt request.
    FTDF_GEN_Handler,                     //FTDF_GEN_IRQn                =   3, // FTDF Interrupt request. Sources:
    RFCAL_Handler,                        //RFCAL_IRQn                   =   4, // RF Calibration Interrupt request. Generated by the DPHY.
    COEX_Handler,                         //COEX_IRQn                    =   5, // Coex Interrupt request.
    CRYPTO_Handler,                       //CRYPTO_IRQn                  =   6, // Crypto Interrupt request. Sources:
    MRM_Handler,                          //MRM_IRQn                     =   7, // Cache Miss Rate Monitor Interrupt request.
    UART_Handler,                         //UART_IRQn                    =   8, // UART Interrupt request.
    UART2_Handler,                        //UART2_IRQn                   =   9, // UART2 Interrupt request.
    I2C_Handler,                          //I2C_IRQn                     =  10, // I2C Interrupt request.
    I2C2_Handler,                         //I2C2_IRQn                    =  11, // I2C2 Interrupt request.
    SPI_Handler,                          //SPI_IRQn                     =  12, // SPI Interrupt request.
    SPI2_Handler,                         //SPI2_IRQn                    =  13, // SPI2 Interrupt request.
    ADC_Handler,                          //ADC_IRQn                     =  14, // Analog-Digital Converter Interrupt request.
    KEYBRD_Handler,                       //KEYBRD_IRQn                  =  15, // Keyboard scanner Interrupt request.
    IRGEN_Handler,                        //IRGEN_IRQn                   =  16, // InfraRed Generator Interrupt request.
    WKUP_GPIO_Handler,                    //WKUP_GPIO_IRQn               =  17, // Wakeup with/without debouncing Interrupt request (when system
    SWTIM0_Handler,                       //SWTIM0_IRQn                  =  18, // SW Timer Interrupt request (Timer0).
    SWTIM1_Handler,                       //SWTIM1_IRQn                  =  19, // SW Timer Interrupt request (Timer1).
    QUADEC_Handler,                       //QUADEC_IRQn                  =  20, // Quadrature Decoder Interrupt request.
    USB_Handler,                          //USB_IRQn                     =  21, // USB device FS Interrupt request.
    PCM_Handler,                          //PCM_IRQn                     =  22, // PCM Interrupt request.
    SRC_IN_Handler,                       //SRC_IN_IRQn                  =  23, // Sample Rate Converter Input Interrupt request.
    SRC_OUT_Handler,                      //SRC_OUT_IRQn                 =  24, // Sample Rate Converter Output Interrupt request.
    VBUS_Handler,                         //VBUS_IRQn                    =  25, // VBUS presence Interrupt request.
    DMA_Handler,                          //DMA_IRQn                     =  26, // DMA Interrupt request.
    RF_DIAG_Handler,                      //RF_DIAG_IRQn                 =  27, // Baseband or Radio Diagnostics Interrupt request.
    TRNG_Handler,                         //TRNG_IRQn                    =  28, // True Random Number Generation Interrupt request.
    DCDC_Handler,                         //DCDC_IRQn                    =  29, // DCDC Interrupt request. Generated upon time out threshold reach.
    XTAL16RDY_Handler,                    //XTAL16RDY_IRQn               =  30, // Indicates that XTAL16 oscillator is trimmed and settled and can
    DEBUG_IRQHandler,                     //RESERVED31_IRQn              =  31, // RESERVED31 Interrupt request.
};

