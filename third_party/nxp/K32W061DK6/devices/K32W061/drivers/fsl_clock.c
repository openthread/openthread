/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_clock.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.clock"
#endif
#define OSC32K_FREQ 32768UL
#define FRO32K_FREQ 32768UL
#define OSC32M_FREQ 32000000UL
#define XTAL32M_FREQ 32000000UL
#define FRO64M_FREQ 64000000UL
#define FRO1M_FREQ 1000000UL
#define FRO12M_FREQ 12000000UL
#define FRO32M_FREQ 32000000UL
#define FRO48M_FREQ 48000000UL

/* Return values from Config (N-2) page of flash */
#define GET_CAL_DATE() (*(uint32_t *)0x9FC68)
#define GET_32MXO_TRIM() (*(uint32_t *)0x9FCF0)
#define GET_32KXO_TRIM() (*(uint32_t *)0x9FCF4)
#define GET_ATE_TEMP() (*(uint32_t *)0x9FDC8)

#define XO_SLAVE_EN (1)

/*******************************************************************************
 * Storage
 ******************************************************************************/
/** External clock rate on the CLKIN pin in Hz. If not used,
    set this to 0. Otherwise, set it to the exact rate in Hz this pin is
    being driven at. */
const uint32_t g_Ext_Clk_Freq = 0U;

#define CLOCK_32MfXtalIecLoadpF_x100_default (600)   /* 6.0pF */
#define CLOCK_32MfXtalPPcbParCappF_x100_default (10) /* 0.1pF */
#define CLOCK_32MfXtalNPcbParCappF_x100_default (5)  /* 0.05pF */

const ClockCapacitanceCompensation_t default_Clock32MCapacitanceCharacteristics = {
    .clk_XtalIecLoadpF_x100    = CLOCK_32MfXtalIecLoadpF_x100_default,
    .clk_XtalPPcbParCappF_x100 = CLOCK_32MfXtalPPcbParCappF_x100_default,
    .clk_XtalNPcbParCappF_x100 = CLOCK_32MfXtalNPcbParCappF_x100_default};
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
uint32_t CLOCK_GetOsc32kFreq(void);
uint32_t CLOCK_GetOsc32MFreq(void);
uint32_t CLOCK_GetFRGInputClock(void);
/*! brief      Return Frequency of Spifi Clock
 *  return     Frequency of Spifi.
 */
uint32_t CLOCK_GetSpifiClkFreq(void);
uint32_t CLOCK_GetAdcClock(void);
uint32_t CLOCK_GetSystemClockRate(void);
uint32_t CLOCK_GetCoreSysClkFreq(void);
uint32_t CLOCK_GetMainClockRate(void);
uint32_t CLOCK_GetXtal32kFreq(void);
uint32_t CLOCK_GetXtal32MFreq(void);
uint32_t CLOCK_GetFro32kFreq(void);
uint32_t CLOCK_GetFro1MFreq(void);
uint32_t CLOCK_GetFro12MFreq(void);
uint32_t CLOCK_GetFro32MFreq(void);
uint32_t CLOCK_GetFro48MFreq(void);
uint32_t CLOCK_GetFro64MFreq(void);
uint32_t CLOCK_GetSpifiOscFreq(void);
uint32_t CLOCK_GetWdtOscFreq(void);
uint32_t CLOCK_GetPWMClockFreq(void);

static uint8_t CLOCK_u8OscCapConvert(uint32_t OscCap, uint8_t u8CapBankDiscontinuity);

/*******************************************************************************
 * types
 ******************************************************************************/

/**
 * brief	Selects clock source using <name>SEL register in syscon
 * param   clock_attach_id_t  specify clock mapping
 * return	none
 * note
 */
void CLOCK_AttachClk(clock_attach_id_t connection)
{
    uint8_t mux;
    uint8_t pos;
    volatile uint32_t *pClkSel;
    clock_div_name_t clockDiv = kCLOCK_DivNone;

    pClkSel = (uint32_t *)SYSCON;

    switch (connection)
    {
        case kXTAL32M_to_MAIN_CLK:
        case kXTAL32M_to_OSC32M_CLK:
        case kXTAL32M_to_CLKOUT:
        case kXTAL32M_to_SPIFI:
        case kXTAL32M_to_ADC_CLK:
        case kXTAL32M_to_ASYNC_APB:
            /* Enable the 32MHz clock distribution to digital core (CGU, MCU) */
            ASYNC_SYSCON->XTAL32MCTRL |= ASYNC_SYSCON_XTAL32MCTRL_XO32M_TO_MCU_ENABLE_MASK;
            break;

        default:
            break;
    }

    switch (connection & 0xFFF)
    {
        case CM_CLKOUTCLKSEL:
            clockDiv = kCLOCK_DivClkout;
            break;
        case CM_SPIFICLKSEL:
            clockDiv = kCLOCK_DivSpifiClk;
            break;
        case CM_ADCCLKSEL:
            clockDiv = kCLOCK_DivAdcClk;
            break;
        case CM_IRCLKSEL:
            clockDiv = kCLOCK_DivIrClk;
            break;
        case CM_WDTCLKSEL:
            clockDiv = kCLOCK_DivWdtClk;
            break;
        case CM_DMICLKSEL:
            clockDiv = kCLOCK_DivDmicClk;
            break;
        default:
            break;
    }

    if (clockDiv != 0)
    {
        pClkSel[clockDiv] |= (1U << 30U); /* halts the divider counter to avoid glitch */
    }

    mux = (uint8_t)(connection & 0xfff);
    if (((uint32_t)connection) != 0)
    {
        pos = (uint8_t)((connection & 0xf000U) >> 12U) - 1U;
        if (mux == CM_ASYNCAPB)
        {
            ASYNC_SYSCON->ASYNCAPBCLKSELA = pos;
        }
        else if (mux == CM_OSC32CLKSEL)
        {
            if (pos < 2)
            {
                pClkSel[mux] &= ~SYSCON_OSC32CLKSEL_SEL32MHZ_MASK;
                pClkSel[mux] |= pos;
            }
            else
            {
                pClkSel[mux] &= ~SYSCON_OSC32CLKSEL_SEL32KHZ_MASK;
                pClkSel[mux] |= ((pos - 2) << SYSCON_OSC32CLKSEL_SEL32KHZ_SHIFT);
            }
        }
        else if (mux == CM_MODEMCLKSEL)
        {
            if (pos < 2)
            {
                pClkSel[mux] |= SYSCON_MODEMCLKSEL_SEL_ZIGBEE_MASK;
                pClkSel[mux] &= ((uint32_t)pos | 0x2U);
            }
            else
            {
                pClkSel[mux] |= SYSCON_MODEMCLKSEL_SEL_BLE_MASK;
                pClkSel[mux] &= ((uint32_t)((pos - 2) << SYSCON_MODEMCLKSEL_SEL_BLE_SHIFT) | 0x1U);
            }
        }
        else
        {
            pClkSel[mux] = pos;
        }
    }

    if (clockDiv != 0)
    {
        pClkSel[clockDiv] &= ~(1U << 30U); /* release the divider counter */
    }
}

/**
 * brief	Selects clock divider using <name>DIV register in syscon
 * param   clock_div_name_t  	specifies which DIV register we are accessing
 * param   uint32_t			specifies divisor
 * param	bool				true if a syscon clock reset should also be carried out
 * return	none
 * note
 */
void CLOCK_SetClkDiv(clock_div_name_t div_name, uint32_t divided_by_value, bool reset)
{
    volatile uint32_t *pClkDiv;
    pClkDiv = (uint32_t *)SYSCON;

    pClkDiv[div_name] |= (1U << 30U); /* halts the divider counter to avoid glitch */

    if (divided_by_value != 0U)
    {
        pClkDiv[div_name] = (1U << 30U) | (divided_by_value - 1U);
    }

    if (reset)
    {
        pClkDiv[div_name] |= 1U << 29U;
        pClkDiv[div_name] &= ~(1U << 29U);
    }

    pClkDiv[div_name] &= ~(1U << 30U); /* release the divider counter */
}

uint32_t CLOCK_GetClkDiv(clock_div_name_t div_name)
{
    volatile uint32_t *pClkDiv;
    uint32_t div = 0;

    pClkDiv = (uint32_t *)SYSCON;
    div     = pClkDiv[div_name] + 1;

    return div;
}

uint32_t CLOCK_GetFRGInputClock(void)
{
    uint32_t freq = 0;

    switch ((frg_clock_src_t)((SYSCON->FRGCLKSEL & SYSCON_FRGCLKSEL_SEL_MASK) >> SYSCON_FRGCLKSEL_SEL_SHIFT))
    {
        case kCLOCK_FrgMainClk:
            freq = CLOCK_GetMainClockRate();
            break;

        case kCLOCK_FrgOsc32MClk:
            freq = CLOCK_GetOsc32MFreq();
            break;

        case kCLOCK_FrgFro48M:
            freq = CLOCK_GetFro48MFreq();
            break;

        case kCLOCK_FrgNoClock:
            freq = 0;
            break;

        default:
            freq = 0;
    }

    return freq;
}

uint32_t CLOCK_SetFRGClock(uint32_t freq)
{
    uint32_t input = CLOCK_GetFRGInputClock();
    uint32_t mul;

    if ((freq > 48000000) || (freq > input) || ((freq != 0) && (input / freq >= 2)))
    {
        /* FRG output frequency should be less than equal to 48MHz */
        return 0;
    }
    else
    {
        mul             = ((uint64_t)(input - freq) * 256) / ((uint64_t)freq);
        SYSCON->FRGCTRL = (mul << SYSCON_FRGCTRL_MULT_SHIFT) | SYSCON_FRGCTRL_DIV_MASK;
        return 1;
    }
}

uint32_t CLOCK_GetFRGClock(void)
{
    uint32_t freq = 0;

    if (((SYSCON->FRGCTRL & SYSCON_FRGCTRL_DIV_MASK) >> SYSCON_FRGCTRL_DIV_SHIFT) == 255)
    {
        // We can use a shift here since only divide by 256 is supported
        freq = ((uint64_t)CLOCK_GetFRGInputClock() << 8) /
               (((SYSCON->FRGCTRL & SYSCON_FRGCTRL_MULT_MASK) >> SYSCON_FRGCTRL_MULT_SHIFT) + 256);
    }

    return freq;
}

uint32_t CLOCK_GetClkOutFreq(void)
{
    uint32_t freq = 0;

    switch ((clkout_clock_src_t)((SYSCON->CLKOUTSEL & SYSCON_CLKOUTSEL_SEL_MASK) >> SYSCON_CLKOUTSEL_SEL_SHIFT))
    {
        case kCLOCK_ClkoutMainClk:
            freq = CLOCK_GetMainClockRate();
            break;

        case kCLOCK_ClkoutXtal32k:
            freq = CLOCK_GetXtal32kFreq();
            break;

        case kCLOCK_ClkoutFro32k:
            freq = CLOCK_GetFro32kFreq();
            break;

        case kCLOCK_ClkoutXtal32M:
            freq = CLOCK_GetXtal32MFreq();
            break;

        case kCLOCK_ClkoutDcDcTest:
            freq = CLOCK_GetFro64MFreq();
            break;

        case kCLOCK_ClkoutFro48M:
            freq = CLOCK_GetFro48MFreq();
            break;
        case kCLOCK_ClkoutFro1M:
            freq = CLOCK_GetFro1MFreq();
            break;
        case kCLOCK_ClkoutNoClock:
            freq = 0;
            break;
        default:
            freq = 0;
            break;
    }

    freq /= (((SYSCON->CLKOUTDIV & SYSCON_CLKOUTDIV_DIV_MASK) >> SYSCON_CLKOUTDIV_DIV_SHIFT) + 1);

    return freq;
}

uint32_t CLOCK_GetWdtOscFreq(void)
{
    uint32_t freq = 0;

    switch ((wdt_clock_src_t)((SYSCON->WDTCLKSEL & SYSCON_WDTCLKSEL_SEL_MASK) >> SYSCON_WDTCLKSEL_SEL_SHIFT))
    {
        case kCLOCK_WdtOsc32MClk:
            freq = CLOCK_GetOsc32MFreq();
            break;

        case kCLOCK_WdtOsc32kClk:
            freq = CLOCK_GetOsc32kFreq();
            break;

        case kCLOCK_WdtFro1M:
            freq = CLOCK_GetFro1MFreq();
            break;

        case kCLOCK_WdtNoClock:
            freq = 0;
            break;

        default:
            freq = 0;
            break;
    }

    return freq;
}

uint32_t CLOCK_GetSpifiOscFreq(void)
{
    uint32_t freq = 0;

    switch ((spifi_clock_src_t)((SYSCON->SPIFICLKSEL & SYSCON_SPIFICLKSEL_SEL_MASK) >> SYSCON_SPIFICLKSEL_SEL_SHIFT))
    {
        case kCLOCK_SpifiMainClk:
            freq = CLOCK_GetMainClockRate();
            break;
        case kCLOCK_SpifiXtal32M:
            freq = CLOCK_GetXtal32MFreq();
            break;
        case kCLOCK_SpifiFro64M:
            freq = CLOCK_GetFro64MFreq();
            break;
        case kCLOCK_SpifiFro48M:
            freq = CLOCK_GetFro48MFreq();
            break;
        case kCLOCK_SpifiNoClock:
            freq = 0;
            break;
        default:
            freq = 0;
            break;
    }
    freq /= (((SYSCON->SPIFICLKDIV & SYSCON_SPIFICLKDIV_DIV_MASK) >> SYSCON_SPIFICLKDIV_DIV_SHIFT) + 1);

    return freq;
}

uint32_t CLOCK_GetPWMClockFreq(void)
{
    uint32_t freq = 0;

    switch ((pwm_clock_source_t)((SYSCON->PWMCLKSEL & SYSCON_PWMCLKSEL_SEL_MASK) >> SYSCON_PWMCLKSEL_SEL_SHIFT))
    {
        case kCLOCK_PWMOsc32Mclk:
            freq = CLOCK_GetOsc32MFreq();
            break;
        case kCLOCK_PWMFro48Mclk:
            freq = CLOCK_GetFro48MFreq();
            break;
        case kCLOCK_PWMNoClkSel:
            freq = 0;
            break;
        case kCLOCK_PWMTestClk:
            freq = 0;
            break;
    }

    return freq;
}

/**
 * brief	Obtains frequency of specified clock
 * param   clock_name_t  specify clock to be read
 * return	uint32_t      frequency
 * note
 */
uint32_t CLOCK_GetFreq(clock_name_t clockName)
{
    uint32_t freq = 0;
    switch (clockName)
    {
        case kCLOCK_MainClk:
            freq = CLOCK_GetMainClockRate();
            break;
        case kCLOCK_CoreSysClk:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case kCLOCK_BusClk:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        case kCLOCK_Xtal32k:
            freq = CLOCK_GetXtal32kFreq();
            break;
        case kCLOCK_Xtal32M:
            freq = CLOCK_GetXtal32MFreq();
            break;
        case kCLOCK_Fro32k:
            freq = CLOCK_GetFro32kFreq();
            break;
        case kCLOCK_Fro1M:
            freq = CLOCK_GetFro1MFreq();
            break;
        case kCLOCK_Fro12M:
            freq = CLOCK_GetFro12MFreq();
            break;
        case kCLOCK_Fro32M:
            freq = CLOCK_GetFro32MFreq();
            break;
        case kCLOCK_Fro48M:
            freq = CLOCK_GetFro48MFreq();
            break;
        case kCLOCK_Fro64M:
            freq = CLOCK_GetFro64MFreq();
            break;
        case kCLOCK_ExtClk:
            freq = g_Ext_Clk_Freq;
            break;
        case kCLOCK_WdtOsc:
            freq = CLOCK_GetWdtOscFreq() / ((SYSCON->WDTCLKDIV & SYSCON_WDTCLKDIV_DIV_MASK) + 1);
            ;
            break;
        case kCLOCK_Frg:
            freq = CLOCK_GetFRGClock();
            break;
        case kCLOCK_ClkOut:
            freq = CLOCK_GetClkOutFreq();
            break;
        case kCLOCK_Spifi:
            freq = CLOCK_GetSpifiOscFreq();
            break;
        case kCLOCK_WdtClk:
            freq = CLOCK_GetWdtOscFreq() / ((SYSCON->WDTCLKDIV & SYSCON_WDTCLKDIV_DIV_MASK) + 1);
            break;
        case kCLOCK_Pwm:
            freq = CLOCK_GetPWMClockFreq();
            break;
        case kCLOCK_Timer0:
        case kCLOCK_Timer1:
            freq = CLOCK_GetApbCLkFreq();
            break;
        default:
            freq = 0;
            break;
    }

    return freq;
}

uint32_t CLOCK_GetCoreSysClkFreq(void)
{
    /* No point in checking for divide by 0 */
    return CLOCK_GetMainClockRate() / ((SYSCON->AHBCLKDIV & SYSCON_AHBCLKDIV_DIV_MASK) + 1U);
}

/* Return main clock rate */
uint32_t CLOCK_GetMainClockRate(void)
{
    uint32_t freq = 0;

    switch ((main_clock_src_t)((SYSCON->MAINCLKSEL & SYSCON_MAINCLKSEL_SEL_MASK) >> SYSCON_MAINCLKSEL_SEL_SHIFT))
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

uint32_t CLOCK_GetOsc32kFreq(void)
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

uint32_t CLOCK_GetOsc32MFreq(void)
{
    uint32_t freq = 0;
    if ((SYSCON->OSC32CLKSEL & SYSCON_OSC32CLKSEL_SEL32MHZ_MASK) != 0)
    {
        freq = CLOCK_GetXtal32MFreq();
    }
    else
    {
        freq = CLOCK_GetFro32MFreq();
    }
    return freq;
}

uint32_t CLOCK_GetXtal32kFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->PDRUNCFG & PMC_PDRUNCFG_ENA_XTAL32K_MASK) >> PMC_PDRUNCFG_ENA_XTAL32K_SHIFT) != 0)
    {
        freq = OSC32K_FREQ;
    }

    return freq;
}

uint32_t CLOCK_GetXtal32MFreq(void)
{
    return XTAL32M_FREQ;
}

uint32_t CLOCK_GetFro32kFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->PDRUNCFG & PMC_PDRUNCFG_ENA_FRO32K_MASK) >> PMC_PDRUNCFG_ENA_FRO32K_SHIFT) != 0)
    {
        freq = FRO32K_FREQ;
    }

    return freq;
}

uint32_t CLOCK_GetFro1MFreq(void)
{
    return FRO1M_FREQ;
}

uint32_t CLOCK_GetFro12MFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->FRO192M & PMC_FRO192M_DIVSEL_MASK) >> PMC_FRO192M_DIVSEL_SHIFT) & FRO12M_ENA)
    {
        freq = FRO12M_FREQ;
    }

    return freq;
}

uint32_t CLOCK_GetFro32MFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->FRO192M & PMC_FRO192M_DIVSEL_MASK) >> PMC_FRO192M_DIVSEL_SHIFT) & FRO32M_ENA)
    {
        freq = FRO32M_FREQ;
    }

    return freq;
}

uint32_t CLOCK_GetFro48MFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->FRO192M & PMC_FRO192M_DIVSEL_MASK) >> PMC_FRO192M_DIVSEL_SHIFT) & FRO48M_ENA)
    {
        freq = FRO48M_FREQ;
    }

    return freq;
}

uint32_t CLOCK_GetFro64MFreq(void)
{
    uint32_t freq = 0;

    if (((PMC->FRO192M & PMC_FRO192M_DIVSEL_MASK) >> PMC_FRO192M_DIVSEL_SHIFT) & FRO64M_ENA)
    {
        freq = FRO64M_FREQ;
    }

    return freq;
}

/*! brief      Return Frequency of Spifi Clock
 *  return     Frequency of Spifi.
 */
uint32_t CLOCK_GetSpifiClkFreq(void)
{
    uint32_t freq = 0;

    switch ((spifi_clock_src_t)((SYSCON->SPIFICLKSEL & SYSCON_SPIFICLKSEL_SEL_MASK) >> SYSCON_SPIFICLKSEL_SEL_SHIFT))
    {
        case kCLOCK_SpifiMainClk:
            freq = CLOCK_GetMainClockRate();
            break;

        case kCLOCK_SpifiXtal32M:
            freq = CLOCK_GetXtal32MFreq();
            break;

        case kCLOCK_SpifiFro64M:
            freq = CLOCK_GetFro64MFreq();
            break;

        case kCLOCK_SpifiFro48M:
            freq = CLOCK_GetFro48MFreq();
            break;

        case kCLOCK_SpifiNoClock:
            freq = 0;
            break;

        default:
            freq = 0;
            break;
    }

    if (freq > 0)
    {
    }

    return freq;
}

uint32_t CLOCK_GetAdcClock(void)
{
    uint32_t freq = 0;

    switch ((adc_clock_src_t)((SYSCON->ADCCLKSEL) & SYSCON_ADCCLKSEL_SEL_MASK >> SYSCON_ADCCLKSEL_SEL_SHIFT))
    {
        case kCLOCK_AdcXtal32M:
            freq = CLOCK_GetXtal32MFreq();
            break;

        case kCLOCK_AdcFro12M:
            freq = CLOCK_GetFro12MFreq();
            break;

        case kCLOCK_AdcNoClock:
            freq = 0;
            break;

        default:
            freq = 0;
    }

    if (freq > 0)
    {
        freq /= (((SYSCON->ADCCLKDIV & SYSCON_ADCCLKDIV_DIV_MASK) >> SYSCON_ADCCLKDIV_DIV_SHIFT) + 1);
    }

    return freq;
}

/**
 * brief	Obtains frequency of APB Bus clock
 * param   none
 * return	uint32_t      frequency
 * note
 */
uint32_t CLOCK_GetApbCLkFreq(void)
{
    uint32_t freq = 0;
    /* ASYNCAPBCLKSEL[1:0] says which Clock Mux input is selected for APB */
    switch (ASYNC_SYSCON->ASYNCAPBCLKSELA & ASYNC_SYSCON_ASYNCAPBCLKSELA_SEL_MASK)
    {
        case kCLOCK_ApbMainClk: /* Main Clk */
            freq = CLOCK_GetMainClockRate();
            break;
        case kCLOCK_ApbXtal32M: /* XTAL 32M */
            freq = CLOCK_GetXtal32MFreq();
            break;
        case kCLOCK_ApbFro32M: /* FRO32M */
            freq = CLOCK_GetFro32MFreq();
            break;
        case kCLOCK_ApbFro48M: /* FRO48M */
            freq = CLOCK_GetFro48MFreq();
            break;
        default:
            freq = 0;
            break;
    }

    return freq;
}
/**
 * brief	Enables specific AHB clock channel
 * param   clock_ip_name_t  	specifies which peripheral clock we are controlling
 * return	none
 * note	clock_ip_name_t is a typedef clone of clock_name_t
 */
void CLOCK_EnableClock(clock_ip_name_t clk)
{
    uint32_t index = CLK_GATE_ABSTRACT_REG_OFFSET(clk);
    switch (index)
    {
        case 0:
        case 1:
            SYSCON->AHBCLKCTRLSETS[index] = (1U << CLK_GATE_ABSTRACT_BITS_SHIFT(clk));
            break;

        case 2:
            SYSCON->ASYNCAPBCTRL             = SYSCON_ASYNCAPBCTRL_ENABLE(1);
            ASYNC_SYSCON->ASYNCAPBCLKCTRLSET = (1U << CLK_GATE_ABSTRACT_BITS_SHIFT(clk));
            break;

        default:
            switch (clk)
            {
                case kCLOCK_Xtal32k:
                    PMC->PDRUNCFG |= PMC_PDRUNCFG_ENA_XTAL32K_MASK;
                    SYSCON->OSC32CLKSEL |= SYSCON_OSC32CLKSEL_SEL32KHZ_MASK;
                    break;

                case kCLOCK_Xtal32M:
                    /* Only do something if not started already */
                    if (!(ASYNC_SYSCON->XTAL32MCTRL & ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE_MASK))
                    {
                        /* XTAL only biased from PMC - force this bit on */
                        ASYNC_SYSCON->XTAL32MCTRL |= ASYNC_SYSCON_XTAL32MCTRL_XO_STANDALONE_ENABLE_MASK;
                        /* Enable & set-up XTAL 32 MHz clock core */
                        CLOCK_XtalBasicTrim();

                        /* Wait for clock to stabilize, plus 200us */
                        CLOCK_Xtal32M_WaitUntilStable(200);
                    }
                    break;

                case kCLOCK_Fro32k:
                    PMC->PDRUNCFG |= PMC_PDRUNCFG_ENA_FRO32K_MASK;
                    SYSCON->OSC32CLKSEL &= ~SYSCON_OSC32CLKSEL_SEL32KHZ_MASK;
                    break;

                case kCLOCK_Fro12M:
                    PMC->FRO192M |= (FRO12M_ENA << PMC_FRO192M_DIVSEL_SHIFT);
                    break;

                case kCLOCK_Fro32M:
                    PMC->FRO192M |= (FRO32M_ENA << PMC_FRO192M_DIVSEL_SHIFT);
                    break;

                case kCLOCK_Fro48M:
                    PMC->FRO192M |= (FRO48M_ENA << PMC_FRO192M_DIVSEL_SHIFT);
                    break;

                case kCLOCK_Fro64M:
                    PMC->FRO192M |= (FRO64M_ENA << PMC_FRO192M_DIVSEL_SHIFT);
                    break;

                case kCLOCK_Fmeas:
                    /* FRO1M and XTAL32M clock gating (SYSCON->CLOCK_CTRL)
                     * is handled by FMEAS driver */
                    break;

                default:
                    break;
            }
            break;
    }
}

/**
 * brief	Disables specific AHB clock channel
 * param   clock_ip_name_t  	specifies which peripheral clock we are controlling
 * return	none
 * note	clock_ip_name_t is a typedef clone of clock_name_t
 */
void CLOCK_DisableClock(clock_ip_name_t clk)
{
    uint32_t index = CLK_GATE_ABSTRACT_REG_OFFSET(clk);
    switch (index)
    {
        case 0:
        case 1:
            SYSCON->AHBCLKCTRLCLRS[index] = (1U << CLK_GATE_ABSTRACT_BITS_SHIFT(clk));
            break;

        case 2:
            ASYNC_SYSCON->ASYNCAPBCLKCTRLCLR = (1U << CLK_GATE_ABSTRACT_BITS_SHIFT(clk));
            break;

        default:
            switch (clk)
            {
                case kCLOCK_Fro32k:
                    PMC->PDRUNCFG &= ~PMC_PDRUNCFG_ENA_FRO32K_MASK;
                    break;

                case kCLOCK_Xtal32k:
                    PMC->PDRUNCFG &= ~PMC_PDRUNCFG_ENA_XTAL32K_MASK;
                    break;

                case kCLOCK_Xtal32M:
                    ASYNC_SYSCON->XTAL32MCTRL &= ~(ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE_MASK);
                    break;

                case kCLOCK_Fro12M:
                    PMC->FRO192M &= ~(FRO12M_ENA << PMC_FRO192M_DIVSEL_SHIFT);
                    break;

                case kCLOCK_Fro32M:
                    PMC->FRO192M &= ~(FRO32M_ENA << PMC_FRO192M_DIVSEL_SHIFT);
                    break;

                case kCLOCK_Fro48M:
                    PMC->FRO192M &= ~(FRO48M_ENA << PMC_FRO192M_DIVSEL_SHIFT);
                    break;

                case kCLOCK_Fro64M:
                    PMC->FRO192M &= ~(FRO64M_ENA << PMC_FRO192M_DIVSEL_SHIFT);
                    break;

                default:
                    break;
            }
            break;
    }
}

/**
 * brief	Check if clock is enabled
 * param   clock_ip_name_t  	specifies which peripheral clock we are controlling
 * return  bool
 * note	clock_ip_name_t is a typedef clone of clock_name_t
 */
bool CLOCK_IsClockEnable(clock_ip_name_t clk)
{
    uint32_t index = CLK_GATE_ABSTRACT_REG_OFFSET(clk);
    return (SYSCON->AHBCLKCTRLSETS[index] & (1U << CLK_GATE_ABSTRACT_BITS_SHIFT(clk)));
}

void CLOCK_EnableAPBBridge(void)
{
    SYSCON->ASYNCAPBCTRL |= ((1 << SYSCON_ASYNCAPBCTRL_ENABLE_SHIFT) & SYSCON_ASYNCAPBCTRL_ENABLE_MASK);
}

void CLOCK_DisableAPBBridge(void)
{
    SYSCON->ASYNCAPBCTRL &= ~((1 << SYSCON_ASYNCAPBCTRL_ENABLE_SHIFT) & SYSCON_ASYNCAPBCTRL_ENABLE_MASK);
}

/*! brief   Delay execution by busy waiting
 *  param   delayUs delay duration in micro seconds
 *  return  none
 */
void CLOCK_uDelay(uint32_t delayUs)
{
    uint32_t freqMhz, timeout;
    uint32_t trcena, cyccntena;

    trcena    = CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk;
    cyccntena = DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk;

    /* EMCR.TRCENA: enable DWT unit */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* DWT.CYCCNTENA: enable cycle count register */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    freqMhz = CLOCK_GetFreq(kCLOCK_CoreSysClk) / 1000000;
    timeout = delayUs * freqMhz + DWT->CYCCNT;

    while ((int32_t)(timeout - DWT->CYCCNT) > 0)
        ;

    /* Restore TRCENA and CYCCNTENA original states */
    if (!cyccntena)
    {
        DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    }
    if (!trcena)
    {
        CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
    }
}

void CLOCK_XtalBasicTrim(void)
{
#ifdef OBSOLETED_CODE
    /* Basic trim with default good values */
    uint32_t u32RegVal;

    /* Enable and set LDO, if not already done */
    CLOCK_SetXtal32M_LDO();

    /* Read, modify, write ASYNC_SYSCON->XTAL32MCTRL register */
    u32RegVal = ASYNC_SYSCON->XTAL32MCTRL;

#if (XO_SLAVE_EN == 0)
#error Code below assumes that trimming always sets \
       ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_MASK but configuration \
       of XO_SLAVE_EN means that this may not be the case
#endif

    /* Reset ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_MASK value is 0,
     * but all trimming operations set it to 1. This function is called as a
     * default/catch-all, so if trimming has already been applied since reset
     * we do not want to overwrite it now in case that previous trimming was
     * the more sophisticated CLOCK_Xtal32M_Trim. Hence we only apply the trim
     * values if ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_MASK is 0 */
    if (0U == (u32RegVal & ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_MASK))
    {
        u32RegVal &= ~(ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN_MASK | ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT_MASK |
                       ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE_MASK | ASYNC_SYSCON_XTAL32MCTRL_XO_GM_MASK);

        u32RegVal |= (ASYNC_SYSCON_XTAL32MCTRL_XO_SLAVE(1) | ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE(1) |
                      ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE(1) | ASYNC_SYSCON_XTAL32MCTRL_XO_GM(3));

        /* ES2 default */
        u32RegVal |= 34U << ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN_SHIFT;
        u32RegVal |= 30U << ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT_SHIFT;

        ASYNC_SYSCON->XTAL32MCTRL = u32RegVal;
    }
#else
    CLOCK_Xtal32M_Trim(0, &default_Clock32MCapacitanceCharacteristics);
#endif
}

/*
 * param  XO_32M_OSC_CAP_Delta_x1000 Osc capacitance expressed in fF (femtoFarad).
 * Must be 0 if no temperature compensation algorithm is implemented for a given board
 *
 */
void CLOCK_Xtal32M_Trim(int32_t XO_32M_OSC_CAP_Delta_x1000, const ClockCapacitanceCompensation_t *capa_charac)
{
    uint32_t u32XOTrimValue;
    uint8_t u8IECXinCapCal6pF, u8IECXinCapCal8pF, u8IECXoutCapCal6pF, u8IECXoutCapCal8pF, u8XOSlave;
    int32_t iaXin_x4, ibXin, iaXout_x4, ibXout;
    int32_t iXOCapInpF_x100, iXOCapOutpF_x100;
    uint32_t u32XOCapInCtrl, u32XOCapOutCtrl;
    uint32_t u32RegVal;

    /* Enable and set LDO, if not already done */
    CLOCK_SetXtal32M_LDO();

    /* Get Cal values from Flash */
    u32XOTrimValue = GET_32MXO_TRIM();

    /* Check validity and apply */
    if ((u32XOTrimValue & 1) && ((u32XOTrimValue >> 15) & 1) && (GET_CAL_DATE() >= 20181203))
    {
        /* These fields are 7 bits, unsigned */
        u8IECXinCapCal6pF  = (u32XOTrimValue >> 1) & 0x7f;
        u8IECXinCapCal8pF  = (u32XOTrimValue >> 8) & 0x7f;
        u8IECXoutCapCal6pF = (u32XOTrimValue >> 16) & 0x7f;
        u8IECXoutCapCal8pF = (u32XOTrimValue >> 23) & 0x7f;

        /* This field is 1 bit */
        u8XOSlave = (u32XOTrimValue >> 30) & 0x1;

        /* Linear fit coefficients calculation */
        iaXin_x4  = (int)u8IECXinCapCal8pF - (int)u8IECXinCapCal6pF;
        ibXin     = (int)u8IECXinCapCal6pF - iaXin_x4 * 3;
        iaXout_x4 = (int)u8IECXoutCapCal8pF - (int)u8IECXoutCapCal6pF;
        ibXout    = (int)u8IECXoutCapCal6pF - iaXout_x4 * 3;
    }
    else
    {
        iaXin_x4  = 20;  // gain in LSB/pF 4.882pF
        ibXin     = -14; // offset in LSB -13.586
        iaXout_x4 = 19;  // gain in LSB/pF 4.864
        ibXout    = -15; // offset in LSB -14.5
        u8XOSlave = 0;
    }

    /* In & out load cap calculation with derating */
    iXOCapInpF_x100 = 2 * capa_charac->clk_XtalIecLoadpF_x100 - capa_charac->clk_XtalNPcbParCappF_x100 +
                      39 * (XO_SLAVE_EN - u8XOSlave) - 15;
    iXOCapOutpF_x100 = 2 * capa_charac->clk_XtalIecLoadpF_x100 - capa_charac->clk_XtalPPcbParCappF_x100 - 21;

    iXOCapInpF_x100  = iXOCapInpF_x100 + XO_32M_OSC_CAP_Delta_x1000 / 5;
    iXOCapOutpF_x100 = iXOCapOutpF_x100 + XO_32M_OSC_CAP_Delta_x1000 / 5;

    /* In & out XO_OSC_CAP_Code_CTRL calculation, with rounding */
    u32XOCapInCtrl  = (uint32_t)(((iXOCapInpF_x100 * iaXin_x4 + ibXin * 400) + 200) / 400);
    u32XOCapOutCtrl = (uint32_t)(((iXOCapOutpF_x100 * iaXout_x4 + ibXout * 400) + 200) / 400);

    uint8_t u8XOCapInCtrl  = CLOCK_u8OscCapConvert(u32XOCapInCtrl, 13);
    uint8_t u8XOCapOutCtrl = CLOCK_u8OscCapConvert(u32XOCapOutCtrl, 13);

    /* Read register and clear fields to be written */
    u32RegVal = ASYNC_SYSCON->XTAL32MCTRL;
    u32RegVal &= ~(ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN_MASK | ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT_MASK |
                   ASYNC_SYSCON_XTAL32MCTRL_XO_GM_MASK | ASYNC_SYSCON_XTAL32MCTRL_XO_SLAVE_MASK |
                   ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_MASK);

/* Configuration of 32 MHz XO output buffers */
#if (XO_SLAVE_EN != 0)
    u32RegVal |= (ASYNC_SYSCON_XTAL32MCTRL_XO_SLAVE(1) | ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE(1));
#endif

    /* XO_OSC_CAP_Code_CTRL to XO_OSC_CAP_Code conversion */
    u32RegVal |= ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE(1);
    u32RegVal |= ASYNC_SYSCON_XTAL32MCTRL_XO_GM(3);
    u32RegVal |= ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN(u8XOCapInCtrl);
    u32RegVal |= ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT(u8XOCapOutCtrl);

    /* Write back to register */
    ASYNC_SYSCON->XTAL32MCTRL = u32RegVal;
}

void CLOCK_Xtal32k_Trim(int32_t XO_32k_OSC_CAP_Delta_x1000, const ClockCapacitanceCompensation_t *capa_charac)
{
    uint32_t u32XOTrimValue;
    uint8_t u8IECXinCapCal6pF, u8IECXinCapCal8pF, u8IECXoutCapCal6pF, u8IECXoutCapCal8pF;
    int32_t iaXin_x4, ibXin, iaXout_x4, ibXout;
    int32_t iXOCapInpF_x100, iXOCapOutpF_x100;
    uint32_t u32XOCapInCtrl, u32XOCapOutCtrl;
    uint32_t u32RegVal;

    /* Get Cal values from Flash */
    u32XOTrimValue = GET_32KXO_TRIM();

    /* check validity and apply */
    if ((u32XOTrimValue & 1) && ((u32XOTrimValue >> 15) & 1) && (GET_CAL_DATE() >= 20180301))
    {
        /* These fields are 7 bits, unsigned */
        u8IECXinCapCal6pF  = (u32XOTrimValue >> 1) & 0x7f;
        u8IECXinCapCal8pF  = (u32XOTrimValue >> 8) & 0x7f;
        u8IECXoutCapCal6pF = (u32XOTrimValue >> 16) & 0x7f;
        u8IECXoutCapCal8pF = (u32XOTrimValue >> 23) & 0x7f;
        /* Linear fit coefficients calculation */
        iaXin_x4  = (int)u8IECXinCapCal8pF - (int)u8IECXinCapCal6pF;
        ibXin     = (int)u8IECXinCapCal6pF - iaXin_x4 * 3;
        iaXout_x4 = (int)u8IECXoutCapCal8pF - (int)u8IECXoutCapCal6pF;
        ibXout    = (int)u8IECXoutCapCal6pF - iaXout_x4 * 3;
    }
    else
    {
        iaXin_x4  = 14; // gain in LSB/pF 3.586
        ibXin     = 9;  // offset in LSB 9.286
        iaXout_x4 = 14; // gain in LSB/pF 3.618
        ibXout    = 8;  // offset in LSB 6.786
    }

    /* In & out load cap calculation with derating */
    iXOCapInpF_x100  = 2 * capa_charac->clk_XtalIecLoadpF_x100 - capa_charac->clk_XtalPPcbParCappF_x100 - 130;
    iXOCapOutpF_x100 = 2 * capa_charac->clk_XtalIecLoadpF_x100 - capa_charac->clk_XtalNPcbParCappF_x100 - 41;

    /* Temperature compensation, if not supported XO_32k_OSC_CAP_Delta_x1000 == 0*/
    iXOCapInpF_x100  = iXOCapInpF_x100 + XO_32k_OSC_CAP_Delta_x1000 / 5;
    iXOCapOutpF_x100 = iXOCapOutpF_x100 + XO_32k_OSC_CAP_Delta_x1000 / 5;

    /* In & out XO_OSC_CAP_Code_CTRL calculation, with rounding */
    u32XOCapInCtrl  = (uint32_t)(((iXOCapInpF_x100 * iaXin_x4 + ibXin * 400) + 200) / 400);
    u32XOCapOutCtrl = (uint32_t)(((iXOCapOutpF_x100 * iaXout_x4 + ibXout * 400) + 200) / 400);

    uint8_t u8XOCapInCtrl  = CLOCK_u8OscCapConvert(u32XOCapInCtrl, 23);
    uint8_t u8XOCapOutCtrl = CLOCK_u8OscCapConvert(u32XOCapOutCtrl, 23);

    /* Read register and clear fields to be written */
    u32RegVal = SYSCON->XTAL32KCAP;
    u32RegVal &= ~(SYSCON_XTAL32KCAP_XO_OSC_CAP_IN_MASK | SYSCON_XTAL32KCAP_XO_OSC_CAP_OUT_MASK);

    /* XO_OSC_CAP_Code_CTRL to XO_OSC_CAP_Code conversion */
    u32RegVal |= SYSCON_XTAL32KCAP_XO_OSC_CAP_IN(u8XOCapInCtrl);
    u32RegVal |= SYSCON_XTAL32KCAP_XO_OSC_CAP_OUT(u8XOCapOutCtrl);

    /* Write back to register */
    SYSCON->XTAL32KCAP = u32RegVal;
}

void CLOCK_SetXtal32M_LDO(void)
{
    uint32_t temp;
    const uint32_t u32Mask  = (ASYNC_SYSCON_XTAL32MLDOCTRL_ENABLE_MASK | ASYNC_SYSCON_XTAL32MLDOCTRL_VOUT_MASK |
                              ASYNC_SYSCON_XTAL32MLDOCTRL_IBIAS_MASK | ASYNC_SYSCON_XTAL32MLDOCTRL_STABMODE_MASK);
    const uint32_t u32Value = (ASYNC_SYSCON_XTAL32MLDOCTRL_ENABLE(1) | ASYNC_SYSCON_XTAL32MLDOCTRL_VOUT(0x5) |
                               ASYNC_SYSCON_XTAL32MLDOCTRL_IBIAS(0x2) | ASYNC_SYSCON_XTAL32MLDOCTRL_STABMODE(0x1));

    /* Enable & set-up XTAL 32 MHz clock LDO */
    temp = ASYNC_SYSCON->XTAL32MLDOCTRL;

    if ((temp & u32Mask) != u32Value)
    {
        temp &= ~u32Mask;

        /*
         * Enable the XTAL32M LDO
         * Adjust the output voltage level, 0x5 for 1.1V
         * Adjust the biasing current, 0x2 value
         * Stability configuration, 0x1 default mode
         */
        temp |= u32Value;

        ASYNC_SYSCON->XTAL32MLDOCTRL = temp;

        /* Delay for LDO to be up */
        CLOCK_uDelay(20);
    }
}

void CLOCK_Xtal32M_WaitUntilStable(uint32_t u32AdditionalWait_us)
{
    /* Spin until XO stable flag is set */
    while ((ASYNC_SYSCON->RADIOSTATUS & ASYNC_SYSCON_RADIOSTATUS_PLLXOREADY_MASK) == 0)
    {
    }

    /* Extra wait to ensure XTAL is accurate enough */
    CLOCK_uDelay(u32AdditionalWait_us);
}

static uint8_t CLOCK_u8OscCapConvert(uint32_t OscCap_val, uint8_t u8CapBankDiscontinuity)
{
    /* Compensate for discontinuity in the capacitor banks */
    if (OscCap_val < 64)
    {
        if (OscCap_val >= u8CapBankDiscontinuity)
        {
            OscCap_val -= u8CapBankDiscontinuity;
        }
        else
        {
            OscCap_val = 0;
        }
    }
    else
    {
        if (OscCap_val <= (127 - u8CapBankDiscontinuity))
        {
            OscCap_val += u8CapBankDiscontinuity;
        }
        else
        {
            OscCap_val = 127;
        }
    }

    return (uint8_t)OscCap_val;
}
