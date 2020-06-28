/*
** ###################################################################
**     Processors:          JN5189HN
**                          JN5189THN
**
**     Compilers:           Keil ARM C/C++ Compiler
**                          GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          MCUXpresso Compiler
**
**     Reference manual:    JN5189 User manual Rev0.1 27 July 2018
**     Version:             rev. 1.0, 2018-07-31
**     Build:               b180731
**
**     Abstract:
**         Provides a system configuration function and a global variable that
**         contains the system frequency. It configures the device and initializes
**         the oscillator (PLL) that is part of the microcontroller device.
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2018 NXP
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2018-07-31)
**         Initial version.
**
** ###################################################################
*/

/*!
 * @file JN5189
 * @version 1.0
 * @date 2018-07-31
 * @brief Device specific configuration file for JN5189 (implementation file)
 *
 * Provides a system configuration function and a global variable that contains
 * the system frequency. It configures the device and initializes the oscillator
 * (PLL) that is part of the microcontroller device.
 */

#include <stdint.h>
#include "fsl_device_registers.h"
#include "rom_api.h"

/**
 * Clock source selections for the Main Clock
 */
typedef enum _main_clock_src
{
    kCLOCK_MainFro12M 		= 0,
    kCLOCK_MainOsc32k 		= 1,
    kCLOCK_MainXtal32M		= 2,
    kCLOCK_MainFro32M		= 3,
    kCLOCK_MainFro48M		= 4,
    kCLOCK_MainExtClk		= 5,
    kCLOCK_MainFro1M		= 6,
} main_clock_src_t;


/**
 * Clock source selections for CLKOUT
 */
typedef enum _clkout_clock_src
{
    kCLOCK_ClkoutMainClk = 0,
    kCLOCK_ClkoutXtal32k = 1,
    kCLOCK_ClkoutFro32k	 = 2,
    kCLOCK_ClkoutXtal32M = 3,
    kCLOCK_ClkoutDcDcTest= 4,
    kCLOCK_ClkoutFro48M  = 5,
    kCLOCK_ClkoutFro1M   = 6,
    kCLOCK_ClkoutNoClock = 7
} clkout_clock_src_t;

typedef enum
{
    FRO12M_ENA  = (1 << 0),
    FRO32M_ENA  = (1 << 1),
    FRO48M_ENA  = (1 << 2),
    FRO64M_ENA  = (1 << 3),
    FRO96M_ENA  = (1 << 4)
} Fro_ClkSel_t;

#define OSC32K_FREQ        32768UL
#define FRO32K_FREQ        32768UL
#define OSC32M_FREQ        32000000UL
#define XTAL32M_FREQ       32000000UL
#define FRO64M_FREQ        64000000UL
#define FRO1M_FREQ         1000000UL
#define FRO12M_FREQ        12000000UL
#define FRO32M_FREQ        32000000UL
#define FRO48M_FREQ        48000000UL

static const uint32_t g_Ext_Clk_Freq = 0U;

extern unsigned int __Vectors;
extern WEAK void SystemInit(void);
extern WEAK void WarmMain(void);

static uint32_t CLOCK_GetXtal32kFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->PDRUNCFG & PMC_PDRUNCFG_ENA_XTAL32K_MASK)
                >> PMC_PDRUNCFG_ENA_XTAL32K_SHIFT) != 0)
    {
        freq = OSC32K_FREQ;
    }

    return freq;
}

static uint32_t CLOCK_GetXtal32MFreq(void)
{
    return XTAL32M_FREQ;
}

static uint32_t CLOCK_GetFro32kFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->PDRUNCFG & PMC_PDRUNCFG_ENA_FRO32K_MASK)
                >> PMC_PDRUNCFG_ENA_FRO32K_SHIFT) != 0)
    {
        freq = FRO32K_FREQ;
    }

    return freq;
}

static uint32_t CLOCK_GetFro1MFreq(void)
{
    return FRO1M_FREQ;
}

static uint32_t CLOCK_GetFro12MFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->FRO192M & PMC_FRO192M_DIVSEL_MASK) >>
            PMC_FRO192M_DIVSEL_SHIFT) & FRO12M_ENA)
    {
        freq = FRO12M_FREQ;
    }

    return freq;
}

static uint32_t CLOCK_GetFro32MFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->FRO192M & PMC_FRO192M_DIVSEL_MASK) >>
            PMC_FRO192M_DIVSEL_SHIFT) & FRO32M_ENA)
    {
        freq = FRO32M_FREQ;
    }

    return freq;
}

static uint32_t CLOCK_GetFro48MFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->FRO192M & PMC_FRO192M_DIVSEL_MASK) >>
            PMC_FRO192M_DIVSEL_SHIFT) & FRO48M_ENA)
    {
        freq = FRO48M_FREQ;
    }

    return freq;
}

static uint32_t CLOCK_GetOsc32kFreq(void)
{
    uint32_t freq = 0;
    if ((SYSCON->OSC32CLKSEL & SYSCON_OSC32CLKSEL_SEL32KHZ_MASK) != 0)
    {
        freq = CLOCK_GetXtal32kFreq();
    }
    else
    {
        freq = CLOCK_GetFro32kFreq();
    }
    return freq;
}

/* Return main clock rate */
static uint32_t CLOCK_GetMainClockRate(void)
{
    uint32_t freq = 0;

    switch ((main_clock_src_t)((SYSCON->MAINCLKSEL & SYSCON_MAINCLKSEL_SEL_MASK)
                    >> SYSCON_MAINCLKSEL_SEL_SHIFT))
    {
    case kCLOCK_MainFro12M:
        freq = CLOCK_GetFro12MFreq();
        break;

    case kCLOCK_MainOsc32k:
        freq = CLOCK_GetOsc32kFreq();
        break;

    case kCLOCK_MainXtal32M:
        freq = CLOCK_GetXtal32MFreq();
        break;

    case kCLOCK_MainFro32M:
        freq = CLOCK_GetFro32MFreq();
        break;

    case kCLOCK_MainFro48M:
        freq = CLOCK_GetFro48MFreq();
        break;

    case kCLOCK_MainExtClk:
        freq = g_Ext_Clk_Freq;
        break;

    case kCLOCK_MainFro1M:
        freq = CLOCK_GetFro1MFreq();
        break;
    }

    return freq;
}



/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */

uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;

#if defined(__ICCARM__)
//*****************************************************************************
// Reset entry point for your code.
// Sets up a simple runtime environment and initializes the C/C++
// library.
//*****************************************************************************
void ResetISR(void)
{
    __asm volatile(
        "LDR    R0,=0x40000804\t\n"          // load co-processor boot address (from CPBOOT)
        "LDR    R0,[R0]\t\n"                 // get address to branch to
        "MOVS   R0,R0\t\n"                   // Check if 0
        "BEQ.N  masterboot\t\n"              // if zero in boot reg, we just branch to  real reset
        "LDR    R1,=0x40000808\t\n"          // load co-processor stack pointer (from CPSTACK)
        "LDR    R1,[R1]\t\n"
        "MOV    SP,R1\t\n"
        "BX     R0\t\n"                      // branch to boot address
        "masterboot:\t\n"
        "LDR    R0, =ResetISR2\t\n"          // jump to 'real' reset handler
        "BX     R0\t\n");
}

void ResetISR2(void)
{
    if ((void (*)(void))WarmMain != NULL)
    {
        unsigned int warm_start;
        uint32_t     pmc_lpmode;
        uint32_t     pmc_resetcause;
        uint32_t     pwr_pdsleepcfg;

        pmc_resetcause = PMC->RESETCAUSE;
        pwr_pdsleepcfg = PMC->PDSLEEPCFG;

        pmc_lpmode = BOOT_GetStartPowerMode();

        warm_start = (pmc_lpmode == 0x02);  /* coming from power down mode*/

        // check if the reset cause is only a timer wakeup or io wakeup with all memory banks held
        warm_start  &= ( !(pmc_resetcause & (PMC_RESETCAUSE_POR_MASK
                                      | PMC_RESETCAUSE_PADRESET_MASK
                                      | PMC_RESETCAUSE_BODRESET_MASK
                                      | PMC_RESETCAUSE_SYSTEMRESET_MASK
                                      | PMC_RESETCAUSE_WDTRESET_MASK
                                      | PMC_RESETCAUSE_WAKEUPIORESET_MASK ))
                            &&  ( pmc_resetcause & PMC_RESETCAUSE_WAKEUPPWDNRESET_MASK         )
                            &&  ((pwr_pdsleepcfg & PMC_PDSLEEPCFG_PDEN_PD_MEM7_MASK) == 0x0    )     /* BANK7 memory bank held */
                            &&  ( pwr_pdsleepcfg & PMC_PDSLEEPCFG_PDEN_LDO_MEM_MASK            )     /* LDO MEM enabled */
                        );

        if (warm_start)
        {

            if (SYSCON->CPSTACK)
            {
                /* if CPSTACK is not NULL, switch to CPSTACK value so we avoid to corrupt the stack used before power down
                 * Note: it looks like enough to switch to new SP now and not earlier */
                __asm volatile(
                        "LDR    R1,=0x40000808\t\n" // load co-processor stack pointer (from CPSTACK)
                        "LDR    R1,[R1]\t\n"
                        "MOV    SP,R1\t\n"
                        );
            }
            // Check to see if we are running the code from a non-zero
            // address (eg RAM, external flash), in which case we need
            // to modify the VTOR register to tell the CPU that the
            // vector table is located at a non-0x0 address.
            unsigned int *pSCB_VTOR = (unsigned int *)0xE000ED08;
            if (((unsigned int)(&__Vectors) != 0))
            {
                // CMSIS : SCB->VTOR = <address of vector table>
                *pSCB_VTOR = (unsigned int)(&__Vectors);
            }

            if ((void (*)(void))SystemInit != NULL)
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

    // Check to see if we are running the code from a non-zero
    // address (eg RAM, external flash), in which case we need
    // to modify the VTOR register to tell the CPU that the
    // vector table is located at a non-0x0 address.
    unsigned int *pSCB_VTOR = (unsigned int *)0xE000ED08;
    if ((unsigned int)(&__Vectors) != 0)
    {
        // CMSIS : SCB->VTOR = <address of vector table>
        *pSCB_VTOR = (unsigned int)(&__Vectors);
    }

    SystemInit();

#if defined(__cplusplus)
    //
    // Call C++ library initialisation
    //
    __libc_init_array();
#endif
}
#endif

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */

void SystemInit (void) {
    uint32_t trim;
    /* Initialise SystemCoreClock value */
    SystemCoreClockUpdate();

    /* Initialise NVIC priority grouping value */
    NVIC_SetPriorityGrouping(4);

    /* Apply FRO1M trim value */
    trim = *(uint32_t*)(0x9FCD0U);

    if(trim & 0x1U)
    {
        PMC->FRO1M = (PMC->FRO1M & ~PMC_FRO1M_FREQSEL_MASK) | ((trim>>1) & PMC_FRO1M_FREQSEL_MASK);
    }
}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */

void SystemCoreClockUpdate (void) {
    SystemCoreClock = (CLOCK_GetMainClockRate() / ((SYSCON->AHBCLKDIV & SYSCON_AHBCLKDIV_DIV_MASK) + 1U));
}

