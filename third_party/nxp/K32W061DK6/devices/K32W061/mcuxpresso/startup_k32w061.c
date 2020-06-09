//*****************************************************************************
// K32W061 startup code for use with MCUXpresso IDE
//
// Version : 211119
//*****************************************************************************
//
// Copyright 2016-2019 NXP
// All rights reserved.
//
// SPDX-License-Identifier: BSD-3-Clause
//*****************************************************************************

#if defined(DEBUG)
#pragma GCC push_options
#pragma GCC optimize("Og")
#endif // (DEBUG)

#if defined(__cplusplus)
#ifdef __REDLIB__
#error Redlib does not support C++
#else
//*****************************************************************************
//
// The entry point for the C++ library startup
//
//*****************************************************************************
extern "C" {
extern void __libc_init_array(void);
}
#endif
#endif

#define WEAK __attribute__((weak))
#define WEAK_AV __attribute__((weak, section(".after_vectors")))
#define ALIAS(f) __attribute__((weak, alias(#f)))

//*****************************************************************************
#if defined(__cplusplus)
extern "C" {
#endif

//*****************************************************************************
// Variable to store CRP value in. Will be placed automatically
// by the linker when "Enable Code Read Protect" selected.
// See crp.h header for more information
//*****************************************************************************
#if (defined(__MCUXPRESSO))
#include <NXP/crp.h>
__CRP const unsigned int CRP_WORD = CRP_NO_CRP;
#endif

#include "fsl_device_registers.h"
#include "rom_api.h"

#define PMC_PDSLEEPCFG_PDEN_PD_MEM_ALL_MASK                                                                   \
    (PMC_PDSLEEPCFG_PDEN_PD_MEM0_MASK | PMC_PDSLEEPCFG_PDEN_PD_MEM1_MASK | PMC_PDSLEEPCFG_PDEN_PD_MEM2_MASK | \
     PMC_PDSLEEPCFG_PDEN_PD_MEM3_MASK | PMC_PDSLEEPCFG_PDEN_PD_MEM4_MASK | PMC_PDSLEEPCFG_PDEN_PD_MEM5_MASK | \
     PMC_PDSLEEPCFG_PDEN_PD_MEM6_MASK | PMC_PDSLEEPCFG_PDEN_PD_MEM7_MASK | PMC_PDSLEEPCFG_PDEN_PD_MEM8_MASK | \
     PMC_PDSLEEPCFG_PDEN_PD_MEM9_MASK | PMC_PDSLEEPCFG_PDEN_PD_MEM10_MASK | PMC_PDSLEEPCFG_PDEN_PD_MEM11_MASK)

//*****************************************************************************
// Declaration of external SystemInit function
//*****************************************************************************
extern WEAK void SystemInit(void);

//*****************************************************************************
// Declaration of external WarmMain function
//*****************************************************************************
extern WEAK void WarmMain(void);

//*****************************************************************************
// Forward declaration of the core exception handlers.
// When the application defines a handler (with the same name), this will
// automatically take precedence over these weak definitions.
// If your application is a C++ one, then any interrupt handlers defined
// in C++ files within in your main application will need to have C linkage
// rather than C++ linkage. To do this, make sure that you are using extern "C"
// { .... } around the interrupt handler within your main application code.
//*****************************************************************************
void ResetISR(void);
WEAK void NMI_Handler(void);
WEAK void HardFault_Handler(void);
WEAK void MemManage_Handler(void);
WEAK void BusFault_Handler(void);
WEAK void UsageFault_Handler(void);
WEAK void SVC_Handler(void);
WEAK void PendSV_Handler(void);
WEAK void SysTick_Handler(void);
WEAK void IntDefaultHandler(void);

//*****************************************************************************
// Forward declaration of the application IRQ handlers. When the application
// defines a handler (with the same name), this will automatically take
// precedence over weak definitions below
//*****************************************************************************
WEAK void WDT_BOD_IRQHandler(void);
WEAK void DMA0_IRQHandler(void);
WEAK void GINT0_IRQHandler(void);
WEAK void CIC_IRB_IRQHandler(void);
WEAK void PIN_INT0_IRQHandler(void);
WEAK void PIN_INT1_IRQHandler(void);
WEAK void PIN_INT2_IRQHandler(void);
WEAK void PIN_INT3_IRQHandler(void);
WEAK void SPIFI0_IRQHandler(void);
WEAK void CTIMER0_IRQHandler(void);
WEAK void CTIMER1_IRQHandler(void);
WEAK void FLEXCOMM0_IRQHandler(void);
WEAK void FLEXCOMM1_IRQHandler(void);
WEAK void FLEXCOMM2_IRQHandler(void);
WEAK void FLEXCOMM3_IRQHandler(void);
WEAK void FLEXCOMM4_IRQHandler(void);
WEAK void FLEXCOMM5_IRQHandler(void);
WEAK void PWM0_IRQHandler(void);
WEAK void PWM1_IRQHandler(void);
WEAK void PWM2_IRQHandler(void);
WEAK void PWM3_IRQHandler(void);
WEAK void PWM4_IRQHandler(void);
WEAK void PWM5_IRQHandler(void);
WEAK void PWM6_IRQHandler(void);
WEAK void PWM7_IRQHandler(void);
WEAK void PWM8_IRQHandler(void);
WEAK void PWM9_IRQHandler(void);
WEAK void PWM10_IRQHandler(void);
WEAK void FLEXCOMM6_IRQHandler(void);
WEAK void RTC_IRQHandler(void);
WEAK void NFCTag_IRQHandler(void);
WEAK void MAILBOX_IRQHandler(void);
WEAK void ADC0_SEQA_IRQHandler(void);
WEAK void ADC0_SEQB_IRQHandler(void);
WEAK void ADC0_THCMP_IRQHandler(void);
WEAK void DMIC0_IRQHandler(void);
WEAK void HWVAD0_IRQHandler(void);
WEAK void BLE_DP_IRQHandler(void);
WEAK void BLE_DP0_IRQHandler(void);
WEAK void BLE_DP1_IRQHandler(void);
WEAK void BLE_DP2_IRQHandler(void);
WEAK void BLE_LL_ALL_IRQHandler(void);
WEAK void ZIGBEE_MAC_IRQHandler(void);
WEAK void ZIGBEE_MODEM_IRQHandler(void);
WEAK void RFP_TMU_IRQHandler(void);
WEAK void RFP_AGC_IRQHandler(void);
WEAK void ISO7816_IRQHandler(void);
WEAK void ANA_COMP_IRQHandler(void);
WEAK void WAKE_UP_TIMER0_IRQHandler(void);
WEAK void WAKE_UP_TIMER1_IRQHandler(void);
WEAK void PVTVF0_AMBER_IRQHandler(void);
WEAK void PVTVF0_RED_IRQHandler(void);
WEAK void PVTVF1_AMBER_IRQHandler(void);
WEAK void PVTVF1_RED_IRQHandler(void);
WEAK void BLE_WAKE_UP_TIMER_IRQHandler(void);
WEAK void SHA_IRQHandler(void);
WEAK void vMMAC_IntHandlerBbc(void);
WEAK void vMMAC_IntHandlerPhy(void);

//*****************************************************************************
// Forward declaration of the driver IRQ handlers. These are aliased
// to the IntDefaultHandler, which is a 'forever' loop. When the driver
// defines a handler (with the same name), this will automatically take
// precedence over these weak definitions
//*****************************************************************************
void WDT_BOD_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void DMA0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void GINT0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void CIC_IRB_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PIN_INT0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PIN_INT1_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PIN_INT2_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PIN_INT3_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void SPIFI0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void CTIMER0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void CTIMER1_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void FLEXCOMM0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void FLEXCOMM1_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void FLEXCOMM2_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void FLEXCOMM3_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void FLEXCOMM4_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void FLEXCOMM5_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM1_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM2_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM3_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM4_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM5_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM6_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM7_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM8_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM9_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PWM10_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void FLEXCOMM6_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void RTC_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void NFCTag_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void MAILBOX_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void ADC0_SEQA_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void ADC0_SEQB_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void ADC0_THCMP_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void DMIC0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void HWVAD0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void BLE_DP_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void BLE_DP0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void BLE_DP1_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void BLE_DP2_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void BLE_LL_ALL_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void ZIGBEE_MAC_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void ZIGBEE_MODEM_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void RFP_TMU_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void RFP_AGC_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void ISO7816_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void ANA_COMP_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void WAKE_UP_TIMER0_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void WAKE_UP_TIMER1_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PVTVF0_AMBER_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PVTVF0_RED_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PVTVF1_AMBER_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void PVTVF1_RED_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void BLE_WAKE_UP_TIMER_DriverIRQHandler(void) ALIAS(IntDefaultHandler);
void SHA_DriverIRQHandler(void) ALIAS(IntDefaultHandler);

//*****************************************************************************
// The entry point for the application.
// __main() is the entry point for Redlib based applications
// main() is the entry point for Newlib based applications
//*****************************************************************************
#if defined(__REDLIB__)
extern void __main(void);
#endif
extern int main(void);

//*****************************************************************************
// External declaration for the pointer to the stack top from the Linker Script
//*****************************************************************************
extern void _vStackTop(void);
//*****************************************************************************
// External declaration for LPC MCU vector table checksum from  Linker Script
//*****************************************************************************
WEAK extern void __valid_user_code_checksum();

//*****************************************************************************
//*****************************************************************************
#if defined(__cplusplus)
} // extern "C"
#endif
//*****************************************************************************
// The vector table.
// This relies on the linker script to place at correct location in memory.
//*****************************************************************************
extern void (*const g_pfnVectors[])(void);
extern void *__Vectors __attribute__((alias("g_pfnVectors")));

__attribute__((used, section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
    // Core Level - CM4
    &_vStackTop,                // The initial stack pointer
    ResetISR,                   // The reset handler
    NMI_Handler,                // The NMI handler
    HardFault_Handler,          // The hard fault handler
    MemManage_Handler,          // The MPU fault handler
    BusFault_Handler,           // The bus fault handler
    UsageFault_Handler,         // The usage fault handler
    __valid_user_code_checksum, // LPC MCU checksum
    0,                          // ECRP
    0,                          // Reserved
    0,                          // Reserved
    SVC_Handler,                // SVCall handler
    0,                          // Reserved
    0,                          // Reserved
    PendSV_Handler,             // The PendSV handler
    SysTick_Handler,            // The SysTick handler

    // Chip Level - K32W061
    WDT_BOD_IRQHandler,           // 16: System (BOD, Watchdog Timer, Flash controller) interrupt
    DMA0_IRQHandler,              // 17: DMA interrupt
    GINT0_IRQHandler,             // 18: GPIO global interrupt
    CIC_IRB_IRQHandler,           // 19: Infra Red Blaster interrupt
    PIN_INT0_IRQHandler,          // 20: Pin Interrupt and Pattern matching 0
    PIN_INT1_IRQHandler,          // 21: Pin Interrupt and Pattern matching 1
    PIN_INT2_IRQHandler,          // 22: Pin Interrupt and Pattern matching 2
    PIN_INT3_IRQHandler,          // 23: Pin Interrupt and Pattern matching 3
    SPIFI0_IRQHandler,            // 24: Quad-SPI flash interface interrupt
    CTIMER0_IRQHandler,           // 25: Counter/Timer 0 interrupt
    CTIMER1_IRQHandler,           // 26: Counter/Timer 1 interrupt
    FLEXCOMM0_IRQHandler,         // 27: Flexcomm Interface 0 (USART0, FLEXCOMM0)
    FLEXCOMM1_IRQHandler,         // 28: Flexcomm Interface 1 (USART1, FLEXCOMM1)
    FLEXCOMM2_IRQHandler,         // 29: Flexcomm Interface 2 (I2C0, FLEXCOMM2)
    FLEXCOMM3_IRQHandler,         // 30: Flexcomm Interface 3 (I2C1, FLEXCOMM3)
    FLEXCOMM4_IRQHandler,         // 31: Flexcomm Interface 4 (SPI0, FLEXCOMM4)
    FLEXCOMM5_IRQHandler,         // 32: Flexcomm Interface 5 (SPI5, FLEXCOMM)
    PWM0_IRQHandler,              // 33: PWM channel 0 interrupt
    PWM1_IRQHandler,              // 34: PWM channel 1 interrupt
    PWM2_IRQHandler,              // 35: PWM channel 2 interrupt
    PWM3_IRQHandler,              // 36: PWM channel 3 interrupt
    PWM4_IRQHandler,              // 37: PWM channel 4 interrupt
    PWM5_IRQHandler,              // 38: PWM channel 5 interrupt
    PWM6_IRQHandler,              // 39: PWM channel 6  interrupt
    PWM7_IRQHandler,              // 40: PWM channel 7 interrupt
    PWM8_IRQHandler,              // 41: PWM channel 8 interrupt
    PWM9_IRQHandler,              // 42: PWM channel 9 interrupt
    PWM10_IRQHandler,             // 43: PWM channel 10 interrupt
    FLEXCOMM6_IRQHandler,         // 44: Flexcomm Interface6 (I2C2, FLEXCOMM6)
    RTC_IRQHandler,               // 45: Real Time Clock interrupt
    NFCTag_IRQHandler,            // 46: NFC Tag interrupt
    MAILBOX_IRQHandler,           // 47: Mailbox interrupts, Wake-up from Deep Sleep interrupt
    ADC0_SEQA_IRQHandler,         // 48: ADC Sequence A interrupt
    ADC0_SEQB_IRQHandler,         // 49: ADC Sequence B interrupt
    ADC0_THCMP_IRQHandler,        // 50: ADC Threshold compare and overrun interrupt
    DMIC0_IRQHandler,             // 51: DMIC interrupt
    HWVAD0_IRQHandler,            // 52: Hardware Voice activity detection interrupt
    BLE_DP_IRQHandler,            // 53: BLE Data Path interrupt
    BLE_DP0_IRQHandler,           // 54: BLE Data Path interrupt 0
    BLE_DP1_IRQHandler,           // 55: BLE Data Path interrupt 1
    BLE_DP2_IRQHandler,           // 56: BLE Data Path interrupt 2
    BLE_LL_ALL_IRQHandler,        // 57: All BLE link layer interrupts
    ZIGBEE_MAC_IRQHandler,        // 58: Zigbee MAC interrupt
    ZIGBEE_MODEM_IRQHandler,      // 59: Zigbee MoDem interrupt
    RFP_TMU_IRQHandler,           // 60: RFP Timing Managemnt Unit (TMU) interrupt
    RFP_AGC_IRQHandler,           // 61: RFP AGC interrupt
    ISO7816_IRQHandler,           // 62: ISO7816 controller interrupt
    ANA_COMP_IRQHandler,          // 63: Analog Comparator interrupt
    WAKE_UP_TIMER0_IRQHandler,    // 64: Wake up Timer 0 interrupt
    WAKE_UP_TIMER1_IRQHandler,    // 65: Wake up Timer 1 interrupt
    PVTVF0_AMBER_IRQHandler,      // 66: PVT Monitor interrupt
    PVTVF0_RED_IRQHandler,        // 67: PVT Monitor interrupt
    PVTVF1_AMBER_IRQHandler,      // 68: PVT Monitor interrupt
    PVTVF1_RED_IRQHandler,        // 69: PVT Monitor interrupt
    BLE_WAKE_UP_TIMER_IRQHandler, // 70: BLE Wake up Timer interrupt
    SHA_IRQHandler,               // 71: SHA interrupt

}; /* End of g_pfnVectors */

//*****************************************************************************
// Functions to carry out the initialization of RW and BSS data sections. These
// are written as separate functions rather than being inlined within the
// ResetISR() function in order to cope with MCUs with multiple banks of
// memory.
//*****************************************************************************
__attribute__((section(".after_vectors.init_data"))) void data_init(unsigned int romstart,
                                                                    unsigned int start,
                                                                    unsigned int len)
{
    unsigned int *pulDest = (unsigned int *)start;
    unsigned int *pulSrc  = (unsigned int *)romstart;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = *pulSrc++;
}

__attribute__((section(".after_vectors.init_bss"))) void bss_init(unsigned int start, unsigned int len)
{
    unsigned int *pulDest = (unsigned int *)start;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = 0;
}

//*****************************************************************************
// The following symbols are constructs generated by the linker, indicating
// the location of various points in the "Global Section Table". This table is
// created by the linker via the Code Red managed linker script mechanism. It
// contains the load address, execution address and length of each RW data
// section and the execution and length of each BSS (zero initialized) section.
//*****************************************************************************
extern unsigned int __data_section_table;
extern unsigned int __data_section_table_end;
extern unsigned int __bss_section_table;
extern unsigned int __bss_section_table_end;

//*****************************************************************************
// Reset entry point for your code.
// Sets up a simple runtime environment and initializes the C/C++
// library.
//*****************************************************************************
__attribute__((section(".after_vectors.reset"))) void ResetISR(void)
{
    // Disable interrupts
    __asm volatile("cpsid i");

    // Enable SRAM clock used by Stack
    __asm volatile(
        "LDR R0, =0x40000220\n\t"
        "MOV R1, #56\n\t"
        "STR R1, [R0]");

    __asm volatile(
        ".set   cpu_ctrl,       0x40000800\t\n"
        ".set   coproc_boot,    0x40000804\t\n"
        ".set   coproc_stack,   0x40000808\t\n"
        "LDR    R0,=coproc_boot\t\n"  // load co-processor boot address (from CPBOOT)
        "LDR    R0,[R0]\t\n"          // get address to branch to
        "MOVS   R0,R0\t\n"            // Check if 0
        "BEQ.N  masterboot\t\n"       // if zero in boot reg, we just branch to  real reset
        "LDR    R1,=coproc_stack\t\n" // load co-processor stack pointer (from CPSTACK)
        "LDR    R1,[R1]\t\n"
        "MOV    SP,R1\t\n"
        "BX     R0\t\n" // branch to boot address
        "masterboot:\t\n"
        "LDR    R0, =ResetISR2\t\n" // jump to 'real' reset handler
        "BX     R0\t\n");
}

__attribute__((used, section(".after_vectors"))) void ResetISR2(void)
{
    if (WarmMain)
    {
        unsigned int warm_start;
        uint32_t pmc_lpmode;
        uint32_t pmc_resetcause;
        uint32_t pwr_pdsleepcfg;

        pmc_resetcause = PMC->RESETCAUSE;
        pwr_pdsleepcfg = PMC->PDSLEEPCFG;

        pmc_lpmode = BOOT_GetStartPowerMode();

        warm_start = (pmc_lpmode == 0x02); /* coming from power down mode*/

        // check if the reset cause is only a timer wakeup or io wakeup with all memory banks held
        warm_start &= (!(pmc_resetcause & (PMC_RESETCAUSE_POR_MASK | PMC_RESETCAUSE_PADRESET_MASK |
                                           PMC_RESETCAUSE_BODRESET_MASK | PMC_RESETCAUSE_SYSTEMRESET_MASK |
                                           PMC_RESETCAUSE_WDTRESET_MASK | PMC_RESETCAUSE_WAKEUPIORESET_MASK)) &&
                       (pmc_resetcause & PMC_RESETCAUSE_WAKEUPPWDNRESET_MASK) &&
                       ((pwr_pdsleepcfg & PMC_PDSLEEPCFG_PDEN_PD_MEM7_MASK) == 0x0) /* BANK7 memory bank held */
                       && (pwr_pdsleepcfg & PMC_PDSLEEPCFG_PDEN_LDO_MEM_MASK)       /* LDO MEM enabled */
        );

        if (warm_start)
        {
            if (SYSCON->CPSTACK)
            {
                /* if CPSTACK is not NULL, switch to CPSTACK value so we avoid to corrupt the stack used before power
                 * down Note: it looks like enough to switch to new SP now and not earlier */
                __asm volatile(
                    ".set   coproc_stack,   0x40000808\t\n"
                    "LDR    R1,=coproc_stack\t\n" // load co-processor stack pointer (from CPSTACK)
                    "LDR    R1,[R1]\t\n"
                    "MOV    SP,R1\t\n");
            }

            // Check to see if we are running the code from a non-zero
            // address (eg RAM, external flash), in which case we need
            // to modify the VTOR register to tell the CPU that the
            // vector table is located at a non-0x0 address.
            unsigned int *pSCB_VTOR = (unsigned int *)0xE000ED08;
            if (((unsigned int)g_pfnVectors != 0))
            {
                // CMSIS : SCB->VTOR = <address of vector table>
                *pSCB_VTOR = (unsigned int)g_pfnVectors;
            }

            if (SystemInit != 0)
            {
                SystemInit();
            }

            WarmMain();

            //
            // WarmMain() shouldn't return, but if it does, we'll just enter an infinite loop
            //
            while (1)
            {
                ;
            }
        }
    }

#if defined(__USE_CMSIS)
    // If __USE_CMSIS defined, then call CMSIS SystemInit code
    SystemInit();

#endif // (__USE_CMSIS)

    //
    // Copy the data sections from flash to SRAM.
    //
    unsigned int LoadAddr, ExeAddr, SectionLen;
    unsigned int *SectionTableAddr;

    // Load base address of Global Section Table
    SectionTableAddr = &__data_section_table;

    // Copy the data sections from flash to SRAM.
    while (SectionTableAddr < &__data_section_table_end)
    {
        LoadAddr   = *SectionTableAddr++;
        ExeAddr    = *SectionTableAddr++;
        SectionLen = *SectionTableAddr++;
        data_init(LoadAddr, ExeAddr, SectionLen);
    }

    // At this point, SectionTableAddr = &__bss_section_table;
    // Zero fill the bss segment
    while (SectionTableAddr < &__bss_section_table_end)
    {
        ExeAddr    = *SectionTableAddr++;
        SectionLen = *SectionTableAddr++;
        bss_init(ExeAddr, SectionLen);
    }

#if !defined(__USE_CMSIS)
// Assume that if __USE_CMSIS defined, then CMSIS SystemInit code
// will enable the FPU
#if defined(__VFP_FP__) && !defined(__SOFTFP__)
    //
    // Code to enable the Cortex-M4 FPU only included
    // if appropriate build options have been selected.
    // Code taken from Section 7.1, Cortex-M4 TRM (DDI0439C)
    //
    // Read CPACR (located at address 0xE000ED88)
    // Set bits 20-23 to enable CP10 and CP11 coprocessors
    // Write back the modified value to the CPACR
    __asm volatile(
        "LDR.W R0, =0xE000ED88\n\t"
        "LDR R1, [R0]\n\t"
        "ORR R1, R1, #(0xF << 20)\n\t"
        "STR R1, [R0]");
#endif // (__VFP_FP__) && !(__SOFTFP__)
#endif // (__USE_CMSIS)

#if !defined(__USE_CMSIS)
    // Assume that if __USE_CMSIS defined, then CMSIS SystemInit code
    // will setup the VTOR register

    // Check to see if we are running the code from a non-zero
    // address (eg RAM, external flash), in which case we need
    // to modify the VTOR register to tell the CPU that the
    // vector table is located at a non-0x0 address.
    unsigned int *pSCB_VTOR = (unsigned int *)0xE000ED08;
    if ((unsigned int *)g_pfnVectors != (unsigned int *)0x00000000)
    {
        *pSCB_VTOR = (unsigned int)g_pfnVectors;
    }
#endif // (__USE_CMSIS)

#if defined(__cplusplus)
    //
    // Call C++ library initialisation
    //
    __libc_init_array();
#endif

    // Reenable interrupts
    __asm volatile("cpsie i");

#if defined(__REDLIB__)
    // Call the Redlib library, which in turn calls main()
    __main();
#else
    main();
#endif

    //
    // main() shouldn't return, but if it does, we'll just enter an infinite loop
    //
    while (1)
    {
        ;
    }
}

//*****************************************************************************
// Default core exception handlers. Override the ones here by defining your own
// handler routines in your application code.
//*****************************************************************************
WEAK_AV void NMI_Handler(void)
{
    while (1)
    {
    }
}

WEAK_AV void HardFault_Handler(void)
{
    while (1)
    {
    }
}

WEAK_AV void MemManage_Handler(void)
{
    while (1)
    {
    }
}

WEAK_AV void BusFault_Handler(void)
{
    while (1)
    {
    }
}

WEAK_AV void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

WEAK_AV void SVC_Handler(void)
{
    while (1)
    {
    }
}

WEAK_AV void PendSV_Handler(void)
{
    while (1)
    {
    }
}

WEAK_AV void SysTick_Handler(void)
{
    while (1)
    {
    }
}

//*****************************************************************************
// Processor ends up here if an unexpected interrupt occurs or a specific
// handler is not present in the application code.
//*****************************************************************************
WEAK_AV void IntDefaultHandler(void)
{
    while (1)
    {
    }
}

//*****************************************************************************
// Default application exception handlers. Override the ones here by defining
// your own handler routines in your application code. These routines call
// driver exception handlers or IntDefaultHandler() if no driver exception
// handler is included.
//*****************************************************************************
WEAK void WDT_BOD_IRQHandler(void)
{
    WDT_BOD_DriverIRQHandler();
}

WEAK void DMA0_IRQHandler(void)
{
    DMA0_DriverIRQHandler();
}

WEAK void GINT0_IRQHandler(void)
{
    GINT0_DriverIRQHandler();
}

WEAK void CIC_IRB_IRQHandler(void)
{
    CIC_IRB_DriverIRQHandler();
}

WEAK void PIN_INT0_IRQHandler(void)
{
    PIN_INT0_DriverIRQHandler();
}

WEAK void PIN_INT1_IRQHandler(void)
{
    PIN_INT1_DriverIRQHandler();
}

WEAK void PIN_INT2_IRQHandler(void)
{
    PIN_INT2_DriverIRQHandler();
}

WEAK void PIN_INT3_IRQHandler(void)
{
    PIN_INT3_DriverIRQHandler();
}

WEAK void SPIFI0_IRQHandler(void)
{
    SPIFI0_DriverIRQHandler();
}

WEAK void CTIMER0_IRQHandler(void)
{
    CTIMER0_DriverIRQHandler();
}

WEAK void CTIMER1_IRQHandler(void)
{
    CTIMER1_DriverIRQHandler();
}

WEAK void FLEXCOMM0_IRQHandler(void)
{
    FLEXCOMM0_DriverIRQHandler();
}

WEAK void FLEXCOMM1_IRQHandler(void)
{
    FLEXCOMM1_DriverIRQHandler();
}

WEAK void FLEXCOMM2_IRQHandler(void)
{
    FLEXCOMM2_DriverIRQHandler();
}

WEAK void FLEXCOMM3_IRQHandler(void)
{
    FLEXCOMM3_DriverIRQHandler();
}

WEAK void FLEXCOMM4_IRQHandler(void)
{
    FLEXCOMM4_DriverIRQHandler();
}

WEAK void FLEXCOMM5_IRQHandler(void)
{
    FLEXCOMM5_DriverIRQHandler();
}

WEAK void PWM0_IRQHandler(void)
{
    PWM0_DriverIRQHandler();
}

WEAK void PWM1_IRQHandler(void)
{
    PWM1_DriverIRQHandler();
}

WEAK void PWM2_IRQHandler(void)
{
    PWM2_DriverIRQHandler();
}

WEAK void PWM3_IRQHandler(void)
{
    PWM3_DriverIRQHandler();
}

WEAK void PWM4_IRQHandler(void)
{
    PWM4_DriverIRQHandler();
}

WEAK void PWM5_IRQHandler(void)
{
    PWM5_DriverIRQHandler();
}

WEAK void PWM6_IRQHandler(void)
{
    PWM6_DriverIRQHandler();
}

WEAK void PWM7_IRQHandler(void)
{
    PWM7_DriverIRQHandler();
}

WEAK void PWM8_IRQHandler(void)
{
    PWM8_DriverIRQHandler();
}

WEAK void PWM9_IRQHandler(void)
{
    PWM9_DriverIRQHandler();
}

WEAK void PWM10_IRQHandler(void)
{
    PWM10_DriverIRQHandler();
}

WEAK void FLEXCOMM6_IRQHandler(void)
{
    FLEXCOMM6_DriverIRQHandler();
}

WEAK void RTC_IRQHandler(void)
{
    RTC_DriverIRQHandler();
}

WEAK void NFCTag_IRQHandler(void)
{
    NFCTag_DriverIRQHandler();
}

WEAK void MAILBOX_IRQHandler(void)
{
    MAILBOX_DriverIRQHandler();
}

WEAK void ADC0_SEQA_IRQHandler(void)
{
    ADC0_SEQA_DriverIRQHandler();
}

WEAK void ADC0_SEQB_IRQHandler(void)
{
    ADC0_SEQB_DriverIRQHandler();
}

WEAK void ADC0_THCMP_IRQHandler(void)
{
    ADC0_THCMP_DriverIRQHandler();
}

WEAK void DMIC0_IRQHandler(void)
{
    DMIC0_DriverIRQHandler();
}

WEAK void HWVAD0_IRQHandler(void)
{
    HWVAD0_DriverIRQHandler();
}

WEAK void BLE_DP_IRQHandler(void)
{
    BLE_DP_DriverIRQHandler();
}

WEAK void BLE_DP0_IRQHandler(void)
{
    BLE_DP0_DriverIRQHandler();
}

WEAK void BLE_DP1_IRQHandler(void)
{
    BLE_DP1_DriverIRQHandler();
}

WEAK void BLE_DP2_IRQHandler(void)
{
    BLE_DP2_DriverIRQHandler();
}

WEAK void BLE_LL_ALL_IRQHandler(void)
{
    BLE_LL_ALL_DriverIRQHandler();
}

WEAK void ZIGBEE_MAC_IRQHandler(void)
{
    if (vMMAC_IntHandlerBbc)
    {
        vMMAC_IntHandlerBbc();
    }
    else if (ZIGBEE_MAC_IRQHandler)
    {
        ZIGBEE_MAC_IRQHandler();
    }
    else
    {
        IntDefaultHandler();
    }
}

WEAK void ZIGBEE_MODEM_IRQHandler(void)
{
    if (vMMAC_IntHandlerPhy)
    {
        vMMAC_IntHandlerPhy();
    }
    else if (ZIGBEE_MODEM_IRQHandler)
    {
        ZIGBEE_MODEM_IRQHandler();
    }
    else
    {
        IntDefaultHandler();
    }
}

WEAK void RFP_AGC_IRQHandler(void)
{
    RFP_AGC_DriverIRQHandler();
}

WEAK void ISO7816_IRQHandler(void)
{
    ISO7816_DriverIRQHandler();
}

WEAK void ANA_COMP_IRQHandler(void)
{
    ANA_COMP_DriverIRQHandler();
}

WEAK void WAKE_UP_TIMER0_IRQHandler(void)
{
    WAKE_UP_TIMER0_DriverIRQHandler();
}

WEAK void WAKE_UP_TIMER1_IRQHandler(void)
{
    WAKE_UP_TIMER1_DriverIRQHandler();
}

WEAK void PVTVF0_AMBER_IRQHandler(void)
{
    PVTVF0_AMBER_DriverIRQHandler();
}

WEAK void PVTVF0_RED_IRQHandler(void)
{
    PVTVF0_RED_DriverIRQHandler();
}

WEAK void PVTVF1_AMBER_IRQHandler(void)
{
    PVTVF1_AMBER_DriverIRQHandler();
}

WEAK void PVTVF1_RED_IRQHandler(void)
{
    PVTVF1_RED_DriverIRQHandler();
}

WEAK void BLE_WAKE_UP_TIMER_IRQHandler(void)
{
    BLE_WAKE_UP_TIMER_DriverIRQHandler();
}

WEAK void SHA_IRQHandler(void)
{
    SHA_DriverIRQHandler();
}

//*****************************************************************************

#if defined(DEBUG)
#pragma GCC pop_options
#endif // (DEBUG)
