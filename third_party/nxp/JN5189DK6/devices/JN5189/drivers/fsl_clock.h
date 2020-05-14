/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_CLOCK_H_
#define _FSL_CLOCK_H_

#include "fsl_device_registers.h"
#include "fsl_common.h"
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>

/*! @addtogroup clock */
/*! @{ */

/*! @file */

/*! @name Driver version */
/*@{*/
/*! @brief CLOCK driver version 2.1.0. */
#define FSL_CLOCK_DRIVER_VERSION (MAKE_VERSION(2, 1, 0))
/*@}*/

#ifdef FPGA_50MHZ
#define SYSCON_BASE_CLOCK_DIV (6)
#define SYSCON_BASE_CLOCK_MUL (5)

#define SYS_FREQ(A) ((A * SYSCON_BASE_CLOCK_MUL) / SYSCON_BASE_CLOCK_DIV)
#else
#define SYS_FREQ(A) (A)
#endif

/* Definition for delay API in clock driver, users can redefine it to the real application. */
#ifndef SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY
#define SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY (48000000UL)
#endif

/*! @brief Clock ip name array for FLEXCOMM. */
#define FLEXCOMM_CLOCKS                                                                               \
    {                                                                                                 \
        kCLOCK_Usart0, kCLOCK_Usart1, kCLOCK_I2c0, kCLOCK_I2c1, kCLOCK_Spi0, kCLOCK_Spi1, kCLOCK_I2c2 \
    }
/*! @brief Clock ip name array for CTIMER. */
#define CTIMER_CLOCKS                \
    {                                \
        kCLOCK_Timer0, kCLOCK_Timer1 \
    }
/*! @brief Clock ip name array for GINT. */
#define GINT_CLOCKS \
    {               \
        kCLOCK_Gint \
    }
/*! @brief Clock ip name array for WWDT. */
#define WWDT_CLOCKS   \
    {                 \
        kCLOCK_WdtOsc \
    }
/*! @brief Clock ip name array for DMIC. */
#define DMIC_CLOCKS \
    {               \
        kCLOCK_DMic \
    }
/*! @brief Clock ip name array for ADC. */
#define ADC_CLOCKS  \
    {               \
        kCLOCK_Adc0 \
    }
/*! @brief Clock ip name array for SPIFI. */
#define SPIFI_CLOCKS \
    {                \
        kCLOCK_Spifi \
    }
/*! @brief Clock ip name array for GPIO. */
#define GPIO_CLOCKS  \
    {                \
        kCLOCK_Gpio0 \
    }
/*! @brief Clock ip name array for DMA. */
#define DMA_CLOCKS \
    {              \
        kCLOCK_Dma \
    }

/* Test line to verify RCS */
/* Another test line to verify source control */
/* Test lines added by Robert Gee */

/**
 * @brief Clock sources for main system clock.
 */
typedef enum
{
    SYSCON_MAINCLKSRC_FRO12M,  /*!< FRO 12MHz */
    SYSCON_MAINCLKSRC_OSC32K,  /*!< OSC 32kHz */
    SYSCON_MAINCLKSRC_XTAL32M, /*!< XTAL 32MHz */
    SYSCON_MAINCLKSRC_FRO32M,  /*!< FRO 32MHz */
    SYSCON_MAINCLKSRC_FRO48M,  /*!< FRO 48MHz */
    SYSCON_MAINCLKSRC_EXT,     /*!< External clock */
    SYSCON_MAINCLKSRC_FRO1M,   /*!< FRO 1MHz */
} CHIP_SYSCON_MAINCLKSRC_T;

/**
 * @brief	Fractional Divider clock sources
 */
typedef enum
{
    SYSCON_FRGCLKSRC_MAINCLK,  /*!< Main Clock */
    SYSCON_FRGCLKSRC_OSC32M,   /*!< 32MHz Clock (XTAL or FRO) */
    SYSCON_FRGCLKSRC_FRO48MHZ, /*!< FRO 48-MHz */
    SYSCON_FRGCLKSRC_NONE,     /*!< FRO 48-MHz */
} CHIP_SYSCON_FRGCLKSRC_T;

/*------------------------------------------------------------------------------
 clock_ip_name_t definition:
------------------------------------------------------------------------------*/

#define CLK_GATE_REG_OFFSET_SHIFT 8U
#define CLK_GATE_REG_OFFSET_MASK 0xFFFFFF00U
#define CLK_GATE_BIT_SHIFT_SHIFT 0U
#define CLK_GATE_BIT_SHIFT_MASK 0x000000FFU

#define CLK_GATE_DEFINE(reg_offset, bit_shift)                                  \
    ((((reg_offset) << CLK_GATE_REG_OFFSET_SHIFT) & CLK_GATE_REG_OFFSET_MASK) | \
     (((bit_shift) << CLK_GATE_BIT_SHIFT_SHIFT) & CLK_GATE_BIT_SHIFT_MASK))

#define CLK_GATE_ABSTRACT_REG_OFFSET(x) (((uint32_t)(x)&CLK_GATE_REG_OFFSET_MASK) >> CLK_GATE_REG_OFFSET_SHIFT)
#define CLK_GATE_ABSTRACT_BITS_SHIFT(x) (((uint32_t)(x)&CLK_GATE_BIT_SHIFT_MASK) >> CLK_GATE_BIT_SHIFT_SHIFT)

#define AHB_CLK_CTRL0 0
#define AHB_CLK_CTRL1 1
#define ASYNC_CLK_CTRL0 2

/**
 * @brief   Clock name definition
 */
typedef enum _clock_name
{
    kCLOCK_Sram0    = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_SRAM_CTRL0_SHIFT), /*!< SRAM0 clock */
    kCLOCK_Sram1    = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_SRAM_CTRL1_SHIFT), /*!< SRAM1 clock */
    kCLOCK_Spifi    = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_SPIFI_SHIFT),      /*!< SPIFI clock */
    kCLOCK_InputMux = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_MUX_SHIFT),        /*!< InputMux clock */
    kCLOCK_Iocon    = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_IOCON_SHIFT),      /*!< IOCON clock */
    kCLOCK_Gpio0    = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_GPIO_SHIFT),       /*!< GPIO0 clock */
    kCLOCK_Pint     = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_PINT_SHIFT),       /*!< PINT clock */
    kCLOCK_Gint     = CLK_GATE_DEFINE(
        AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_GINT_SHIFT), /* GPIO_GLOBALINT0 and GPIO_GLOBALINT1 share the same slot  */
    kCLOCK_Dma     = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_DMA_SHIFT),     /*!< DMA clock */
    kCLOCK_Iso7816 = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_ISO7816_SHIFT), /*!< ISO7816 clock */
    kCLOCK_WdtOsc  = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_WWDT_SHIFT),    /*!< WDTOSC clock */
    kCLOCK_Rtc     = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_RTC_SHIFT),     /*!< RTC clock */
    kCLOCK_AnaInt  = CLK_GATE_DEFINE(
        AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_ANA_INT_CTRL_SHIFT), /*!<  Analog Interrupt Control module clock */
    kCLOCK_WakeTmr =
        CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_WAKE_UP_TIMERS_SHIFT),        /*!<  Wake up Timers clock */
    kCLOCK_Adc0      = CLK_GATE_DEFINE(AHB_CLK_CTRL0, SYSCON_AHBCLKCTRL0_ADC_SHIFT),    /*!< ADC0 clock */
    kCLOCK_FlexComm0 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_USART0_SHIFT), /*!< FlexComm0 clock */
    kCLOCK_FlexComm1 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_USART1_SHIFT), /*!< FlexComm1 clock */
    kCLOCK_FlexComm2 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_I2C0_SHIFT),   /*!< FlexComm2 clock */
    kCLOCK_FlexComm3 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_I2C1_SHIFT),   /*!< FlexComm3 clock */
    kCLOCK_FlexComm4 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_SPI0_SHIFT),   /*!< FlexComm4 clock */
    kCLOCK_FlexComm5 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_SPI1_SHIFT),   /*!< FlexComm5 clock */
    kCLOCK_Ir        = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_IR_SHIFT),     /*!< Infra Red clock */
    kCLOCK_Pwm       = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_PWM_SHIFT),    /*!< PWM clock */
    kCLOCK_Rng       = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_RNG_SHIFT),    /*!< RNG clock */
    kCLOCK_FlexComm6 = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_I2C2_SHIFT),   /*!< FlexComm6 clock */
    kCLOCK_Usart0    = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_USART0_SHIFT), /*!< USART0 clock */
    kCLOCK_Usart1    = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_USART1_SHIFT), /*!< USART1 clock */
    kCLOCK_I2c0      = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_I2C0_SHIFT),   /*!< I2C0 clock */
    kCLOCK_I2c1      = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_I2C1_SHIFT),   /*!< I2C1 clock */
    kCLOCK_Spi0      = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_SPI0_SHIFT),   /*!< SPI0 clock */
    kCLOCK_Spi1      = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_SPI1_SHIFT),   /*!< SPI1 clock */
    kCLOCK_I2c2      = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_I2C2_SHIFT),   /*!< I2C2 clock */
    kCLOCK_Modem     = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_MODEM_MASTER_SHIFT), /*!< MODEM clock */
    kCLOCK_Aes       = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_AES_SHIFT),          /*!< AES clock */
    kCLOCK_Rfp       = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_RFP_SHIFT),          /*!< RFP clock */
    kCLOCK_DMic      = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_DMIC_SHIFT),         /*!< DMIC clock */
    kCLOCK_Sha0      = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_HASH_SHIFT),         /*!< SHA0 clock */
    kCLOCK_Timer0    = CLK_GATE_DEFINE(ASYNC_CLK_CTRL0, 1),                                   /*!< Timer0 clock */
    kCLOCK_Timer1    = CLK_GATE_DEFINE(ASYNC_CLK_CTRL0, 2),                                   /*!< Timer1 clock */
    kCLOCK_MainClk   = (1 << 16),                                                             /*!< MAIN_CLK */
    kCLOCK_CoreSysClk,                                                                        /*!< Core/system clock */
    kCLOCK_BusClk,                                                                            /*!< AHB bus clock */
    kCLOCK_Xtal32k,                                                             /*!< 32kHz crystal oscillator */
    kCLOCK_Xtal32M,                                                             /*!< 32MHz crystal oscillator */
    kCLOCK_Fro32k,                                                              /*!< 32kHz free running oscillator */
    kCLOCK_Fro1M,                                                               /*!< 1MHz Free Running Oscillator */
    kCLOCK_Fro12M,                                                              /*!< 12MHz Free Running Oscillator */
    kCLOCK_Fro32M,                                                              /*!< 32MHz Free Running Oscillator */
    kCLOCK_Fro48M,                                                              /*!< 48MHz Free Running Oscillator */
    kCLOCK_Fro64M,                                                              /*!< 64Mhz Free Running Oscillator */
    kCLOCK_ExtClk,                                                              /*!< External clock */
    kCLOCK_WdtClk,                                                              /*!< Watchdog clock */
    kCLOCK_Frg,                                                                 /*!< Fractional divider */
    kCLOCK_ClkOut,                                                              /*!< Clock out */
    kCLOCK_Fmeas,                                                               /*!< FMEAS clock */
    kCLOCK_Sha = CLK_GATE_DEFINE(AHB_CLK_CTRL1, SYSCON_AHBCLKCTRL1_HASH_SHIFT), /*!< Hash clock */
} clock_name_t;

typedef clock_name_t clock_ip_name_t;

#define REG_OFST(block, member) (offsetof(block##_Type, member) / sizeof(uint32_t))

#define MUX_A(m, choice) (((m) << 0) | ((choice + 1) << 12))

/*! @brief Clock source selector definition */
typedef enum
{
    CM_MAINCLKSEL   = REG_OFST(SYSCON, MAINCLKSEL),  /*!< Clock source selector of Main clock source */
    CM_OSC32CLKSEL  = REG_OFST(SYSCON, OSC32CLKSEL), /*!< Clock source selector of OSC32KCLK and OSC32MCLK */
    CM_CLKOUTCLKSEL = REG_OFST(SYSCON, CLKOUTSEL),   /*!< Clock source selector of CLKOUT */
    CM_SPIFICLKSEL  = REG_OFST(SYSCON, SPIFICLKSEL), /*!< Clock source selector of SPIFI */
    CM_ADCCLKSEL    = REG_OFST(SYSCON, ADCCLKSEL),   /*!< Clock source selector of ADC */
    CM_USARTCLKSEL  = REG_OFST(SYSCON, USARTCLKSEL), /*!< Clock source selector of USART0 & 1 */
    CM_I2CCLKSEL    = REG_OFST(SYSCON, I2CCLKSEL),   /*!< Clock source selector of I2C0, 1 and 2 */
    CM_SPICLKSEL    = REG_OFST(SYSCON, SPICLKSEL),   /*!< Clock source selector of SPI0 & 1 */
    CM_IRCLKSEL     = REG_OFST(SYSCON, IRCLKSEL),    /*!< Clock source selector of Infra Red */
    CM_PWMCLKSEL    = REG_OFST(SYSCON, PWMCLKSEL),   /*!< Clock source selector of PWM */
    CM_WDTCLKSEL    = REG_OFST(SYSCON, WDTCLKSEL),   /*!< Clock source selector of Watchdog Timer */
    CM_MODEMCLKSEL  = REG_OFST(SYSCON, MODEMCLKSEL), /*!< Clock source selector of Modem */
    CM_FRGCLKSEL    = REG_OFST(SYSCON, FRGCLKSEL),   /*!< Clock source selector of Fractional Rate Generator (FRG) */
    CM_DMICLKSEL    = REG_OFST(SYSCON, DMICCLKSEL),  /*!< Clock source selector of Digital microphone (DMIC) */
    CM_WKTCLKSEL    = REG_OFST(SYSCON, WKTCLKSEL),   /*!< Clock source selector of Wake-up Timer */
    CM_ASYNCAPB
} clock_sel_ofst_t;

/*! @brief Clock attach definition */
typedef enum _clock_attach_id
{
    kFRO12M_to_MAIN_CLK  = MUX_A(CM_MAINCLKSEL, 0), /*!< Select FRO 12M for main clock */
    kOSC32K_to_MAIN_CLK  = MUX_A(CM_MAINCLKSEL, 1), /*!< Select OSC 32K for main clock */
    kXTAL32M_to_MAIN_CLK = MUX_A(CM_MAINCLKSEL, 2), /*!< Select XTAL 32M for main clock */
    kFRO32M_to_MAIN_CLK  = MUX_A(CM_MAINCLKSEL, 3), /*!< Select FRO 32M for main clock */
    kFRO48M_to_MAIN_CLK  = MUX_A(CM_MAINCLKSEL, 4), /*!< Select FRO 48M for main clock */
    kEXT_CLK_to_MAIN_CLK = MUX_A(CM_MAINCLKSEL, 5), /*!< Select external clock for main clock */
    kFROM1M_to_MAIN_CLK  = MUX_A(CM_MAINCLKSEL, 6), /*!< Select FRO 1M for main clock */

    kFRO32M_to_OSC32M_CLK  = MUX_A(CM_OSC32CLKSEL, 0), /*!< Select FRO 32M for OSC32KCLK and OSC32MCLK */
    kXTAL32M_to_OSC32M_CLK = MUX_A(CM_OSC32CLKSEL, 1), /*!< Select XTAL 32M for OSC32KCLK and OSC32MCLK */
    kFRO32K_to_OSC32K_CLK  = MUX_A(CM_OSC32CLKSEL, 2), /*!< Select FRO 32K for OSC32KCLK and OSC32MCLK */
    kXTAL32K_to_OSC32K_CLK = MUX_A(CM_OSC32CLKSEL, 3), /*!< Select XTAL 32K for OSC32KCLK and OSC32MCLK */

    kMAIN_CLK_to_CLKOUT = MUX_A(CM_CLKOUTCLKSEL, 0), /*!< Select main clock for CLKOUT */
    kXTAL32K_to_CLKOUT  = MUX_A(CM_CLKOUTCLKSEL, 1), /*!< Select XTAL 32K for CLKOUT */
    kFRO32K_to_CLKOUT   = MUX_A(CM_CLKOUTCLKSEL, 2), /*!< Select FRO 32K for CLKOUT */
    kXTAL32M_to_CLKOUT  = MUX_A(CM_CLKOUTCLKSEL, 3), /*!< Select XTAL 32M for CLKOUT */
    kDCDC_to_CLKOUT     = MUX_A(CM_CLKOUTCLKSEL, 4), /*!< Select DCDC for CLKOUT */
    kFRO48M_to_CLKOUT   = MUX_A(CM_CLKOUTCLKSEL, 5), /*!< Select FRO 48M for CLKOUT */
    kFRO1M_to_CLKOUT    = MUX_A(CM_CLKOUTCLKSEL, 6), /*!< Select FRO 1M for CLKOUT */
    kNONE_to_CLKOUT     = MUX_A(CM_CLKOUTCLKSEL, 7), /*!< No clock for CLKOUT */

    kMAIN_CLK_to_SPIFI = MUX_A(CM_SPIFICLKSEL, 0), /*!< Select main clock for SPIFI */
    kXTAL32M_to_SPIFI  = MUX_A(CM_SPIFICLKSEL, 1), /*!< Select XTAL 32M for SPIFI */
    kFRO64M_to_SPIFI   = MUX_A(CM_SPIFICLKSEL, 2), /*!< Select FRO 64M for SPIFI */
    kFRO48M_to_SPIFI   = MUX_A(CM_SPIFICLKSEL, 3), /*!< Select FRO 48M for SPIFI */

    kXTAL32M_to_ADC_CLK = MUX_A(CM_ADCCLKSEL, 0), /*!< Select XTAL 32M for ADC */
    kFRO12M_to_ADC_CLK  = MUX_A(CM_ADCCLKSEL, 1), /*!< Select FRO 12M for ADC */
    kNONE_to_ADC_CLK    = MUX_A(CM_ADCCLKSEL, 2), /*!< No clock for ADC */

    kOSC32M_to_USART_CLK  = MUX_A(CM_USARTCLKSEL, 0), /*!< Select OSC 32M for USART0 & 1 */
    kFRO48M_to_USART_CLK  = MUX_A(CM_USARTCLKSEL, 1), /*!< Select FRO 48M for USART0 & 1 */
    kFRG_CLK_to_USART_CLK = MUX_A(CM_USARTCLKSEL, 2), /*!< Select FRG clock for USART0 & 1 */
    kNONE_to_USART_CLK    = MUX_A(CM_USARTCLKSEL, 3), /*!< No clock for USART0 & 1 */

    kOSC32M_to_I2C_CLK = MUX_A(CM_I2CCLKSEL, 0), /*!< Select OSC 32M for I2C0, 1 and 2 */
    kFRO48M_to_I2C_CLK = MUX_A(CM_I2CCLKSEL, 1), /*!< Select FRO 48M for I2C0, 1 and 2 */
    kNONE_to_I2C_CLK   = MUX_A(CM_I2CCLKSEL, 2), /*!< No clock for I2C0, 1 and 2 */

    kOSC32M_to_SPI_CLK = MUX_A(CM_SPICLKSEL, 0), /*!< Select OSC 32M for SPI0 & 1 */
    kFRO48M_to_SPI_CLK = MUX_A(CM_SPICLKSEL, 1), /*!< Select FRO 48M for SPI0 & 1 */
    kNONE_to_SPI_CLK   = MUX_A(CM_SPICLKSEL, 2), /*!< No clock for SPI0 & 1 */

    kOSC32M_to_IR_CLK = MUX_A(CM_IRCLKSEL, 0), /*!< Select OSC 32M for Infra Red */
    kFRO48M_to_IR_CLK = MUX_A(CM_IRCLKSEL, 1), /*!< Select FRO 48M for Infra Red */
    kNONE_to_IR_CLK   = MUX_A(CM_IRCLKSEL, 2), /*!< No clock for Infra Red */

    kOSC32M_to_PWM_CLK = MUX_A(CM_PWMCLKSEL, 0), /*!< Select OSC 32M for PWM */
    kFRO48M_to_PWM_CLK = MUX_A(CM_PWMCLKSEL, 1), /*!< Select FRO 48M for PWM */
    kNONE_to_PWM_CLK   = MUX_A(CM_PWMCLKSEL, 2), /*!< No clock for PWM */

    kOSC32M_to_WDT_CLK = MUX_A(CM_WDTCLKSEL, 0), /*!< Select OSC 32M for Watchdog Timer */
    kOSC32K_to_WDT_CLK = MUX_A(CM_WDTCLKSEL, 1), /*!< Select FRO 32K for Watchdog Timer */
    kFRO1M_to_WDT_CLK  = MUX_A(CM_WDTCLKSEL, 2), /*!< Select FRO 1M for Watchdog Timer */

    kMAIN_CLK_to_FRG_CLK = MUX_A(CM_FRGCLKSEL, 0), /*!< Select main clock for FRG */
    kOSC32M_to_FRG_CLK   = MUX_A(CM_FRGCLKSEL, 1), /*!< Select OSC 32M for FRG */
    kFRO48M_to_FRG_CLK   = MUX_A(CM_FRGCLKSEL, 2), /*!< Select FRO 48M for FRG */
    kNONE_to_FRG_CLK     = MUX_A(CM_FRGCLKSEL, 3), /*!< No clock for FRG */

    kMAIN_CLK_to_DMI_CLK = MUX_A(CM_DMICLKSEL, 0), /*!< Select main clock for DMIC */
    kOSC32K_to_DMI_CLK   = MUX_A(CM_DMICLKSEL, 1), /*!< Select OSC 32K for DMIC */
    kFRO48M_to_DMI_CLK   = MUX_A(CM_DMICLKSEL, 2), /*!< Select FRO 48M for DMIC */
    kMCLK_to_DMI_CLK     = MUX_A(CM_DMICLKSEL, 3), /*!< Select external clock for DMIC */
    kFRO1M_to_DMI_CLK    = MUX_A(CM_DMICLKSEL, 4), /*!< Select FRO 1M for DMIC */
    kFRO12M_to_DMI_CLK   = MUX_A(CM_DMICLKSEL, 5), /*!< Select FRO 12M for DMIC */
    kNONE_to_DMI_CLK     = MUX_A(CM_DMICLKSEL, 6), /*!< No clock for DMIC */

    kOSC32K_to_WKT_CLK = MUX_A(CM_WKTCLKSEL, 0), /*!< Select OSC 32K for WKT */
    kNONE_to_WKT_CLK   = MUX_A(CM_WKTCLKSEL, 3), /*!< No clock for WKT */

    kXTAL32M_DIV2_to_ZIGBEE_CLK = MUX_A(CM_MODEMCLKSEL, 0), /*!< Select XTAL 32M for ZIGBEE */
    kNONE_to_ZIGBEE_CLK         = MUX_A(CM_MODEMCLKSEL, 1), /*!< No clock for ZIGBEE */

    kMAIN_CLK_to_ASYNC_APB = MUX_A(CM_ASYNCAPB, 0), /*!< Select main clock for Asynchronous APB */
    kXTAL32M_to_ASYNC_APB  = MUX_A(CM_ASYNCAPB, 1), /*!< Select XTAL 32M for Asynchronous APB */
    kFRO32M_to_ASYNC_APB   = MUX_A(CM_ASYNCAPB, 2), /*!< Select FRO 32M for Asynchronous APB */
    kFRO48M_to_ASYNC_APB   = MUX_A(CM_ASYNCAPB, 3), /*!< Select FRO 48M for Asynchronous APB */
    kNONE_to_NONE          = 0x80000000U,
} clock_attach_id_t;

/*  Clock dividers */
#define FIRST_DIV_MEMBER SYSTICKCLKDIV
#define CLOCK_DIV_OFST(member) offsetof(SYSCON_Type, member)

/*! @brief Clock divider definition */
typedef enum _clock_div_name
{
    kCLOCK_DivNone       = 0,
    kCLOCK_DivSystickClk = (offsetof(SYSCON_Type, SYSTICKCLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivWdtClk     = (offsetof(SYSCON_Type, WDTCLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivIrClk      = (offsetof(SYSCON_Type, IRCLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivAhbClk     = (offsetof(SYSCON_Type, AHBCLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivClkout     = (offsetof(SYSCON_Type, CLKOUTDIV) / sizeof(uint32_t)),
    kCLOCK_DivSpifiClk   = (offsetof(SYSCON_Type, SPIFICLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivAdcClk     = (offsetof(SYSCON_Type, ADCCLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivRtcClk     = (offsetof(SYSCON_Type, RTCCLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivDmicClk    = (offsetof(SYSCON_Type, DMICCLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivRtc1HzClk  = (offsetof(SYSCON_Type, RTC1HZCLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivTraceClk   = (offsetof(SYSCON_Type, TRACECLKDIV) / sizeof(uint32_t)),
    kCLOCK_DivFrg        = (offsetof(SYSCON_Type, FRGCTRL) / sizeof(uint32_t))
} clock_div_name_t;

/*! @brief Clock source selections for the Main Clock */
typedef enum _main_clock_src
{
    kCLOCK_MainFro12M  = 0, /*!< FRO 12M for main clock */
    kCLOCK_MainOsc32k  = 1, /*!< OSC 32K for main clock */
    kCLOCK_MainXtal32M = 2, /*!< XTAL 32M for main clock */
    kCLOCK_MainFro32M  = 3, /*!< FRO 32M for main clock */
    kCLOCK_MainFro48M  = 4, /*!< FRO 48M for main clock */
    kCLOCK_MainExtClk  = 5, /*!< External clock for main clock */
    kCLOCK_MainFro1M   = 6, /*!< FRO 1M for main clock */
} main_clock_src_t;

/*! @brief Clock source selections for CLKOUT */
typedef enum _clkout_clock_src
{
    kCLOCK_ClkoutMainClk  = 0, /*!< CPU & System Bus clock for CLKOUT */
    kCLOCK_ClkoutXtal32k  = 1, /*!< XTAL 32K for CLKOUT */
    kCLOCK_ClkoutFro32k   = 2, /*!< FRO 32K for CLKOUT */
    kCLOCK_ClkoutXtal32M  = 3, /*!< XTAL 32M for CLKOUT */
    kCLOCK_ClkoutDcDcTest = 4, /*!< DCDC Test for CLKOUT */
    kCLOCK_ClkoutFro48M   = 5, /*!< FRO 48M for CLKOUT */
    kCLOCK_ClkoutFro1M    = 6, /*!< FRO 1M for CLKOUT */
    kCLOCK_ClkoutNoClock  = 7  /*!< No clock for CLKOUT */
} clkout_clock_src_t;

/*! @brief Clock source definition for Watchdog timer */
typedef enum _wdt_clock_src
{
    kCLOCK_WdtOsc32MClk = 0, /*!< OSC 32M for WDT */
    kCLOCK_WdtOsc32kClk = 1, /*!< OSC 32K for WDT */
    kCLOCK_WdtFro1M     = 2, /*!< FRO 1M for WDT */
    kCLOCK_WdtNoClock   = 3  /*!< No clock for WDT */
} wdt_clock_src_t;

/*! @brief Clock source definition for fractional divider */
typedef enum _frg_clock_src
{
    kCLOCK_FrgMainClk   = 0, /*!< CPU & System Bus clock for FRG */
    kCLOCK_FrgOsc32MClk = 1, /*!< OSC 32M clock for FRG */
    kCLOCK_FrgFro48M    = 2, /*!< FRO 48M for FRG */
    kCLOCK_FrgNoClock   = 3  /*!< No clock for FRG */
} frg_clock_src_t;

/*! @brief Clock source definition for the APB */
typedef enum _apb_clock_src
{
    kCLOCK_ApbMainClk = 0, /*!< CPU & System Bus clock for APB bridge */
    kCLOCK_ApbXtal32M = 1, /*!< XTAL 32M for APB bridge */
    kCLOCK_ApbFro32M  = 2, /*!< FRO 32M for APB bridge */
    kCLOCK_ApbFro48M  = 3  /*!< FRO 48M for APB bridge */
} apb_clock_src_t;

/*! @brief Clock source definition for frequency measure  */
typedef enum _fmeas_clock_src
{
    kCLOCK_fmeasClkIn     = 0, /*!< Clock in for FMEAS */
    kCLOCK_fmeasXtal32Mhz = 1, /*!< XTAL 32M for FMEAS */
    kCLOCK_fmeasFRO1Mhz   = 2, /*!< FRO 1M for FMEAS */
    kCLOCK_fmeasXtal32kHz = 3, /*!< XTAL 32K for FMEAS */
    kCLOCK_fmeasMainClock = 4, /*!< CPU & System Bus clock for FMEAS */
    kCLOCK_fmeasGPIO_0_4  = 5, /*!< GPIO0_4 input for FMEAS */
    kCLOCK_fmeasGPIO_0_20 = 6, /*!< GPIO0_20 input for FMEAS */
    kCLOCK_fmeasGPIO_0_16 = 7, /*!< GPIO0_16 input for FMEAS */
    kCLOCK_fmeasGPIO_0_15 = 8, /*!< GPIO0_15 input for FMEAS */
} fmeas_clock_src_t;

/*! @brief Clock source selection for SPIFI */
typedef enum _spifi_clock_src
{
    kCLOCK_SpifiMainClk = 0, /*!< CPU & System Bus clock for SPIFI */
    kCLOCK_SpifiXtal32M = 1, /*!< XTAL 32M for SPIFI */
    kCLOCK_SpifiFro64M  = 2, /*!< FRO 64M for SPIFI */
    kCLOCK_SpifiFro48M  = 3, /*!< FRO 48M for SPIFI */
    kCLOCK_SpifiNoClock = 4  /*!< No clock for SPIFI */
} spifi_clock_src_t;

/*! @brief Clock definition for ADC */
typedef enum _adc_clock_src
{
    kCLOCK_AdcXtal32M = 0, /*!< XTAL 32MHz for ADC */
    kCLOCK_AdcFro12M  = 1, /*!< FRO 12MHz for ADC */
    kCLOCK_AdcNoClock = 2  /*!< No clock for ADC */
} adc_clock_src_t;

/*! @brief PWM Clock source selection values */
typedef enum _pwm_clock_source
{
    kCLOCK_PWMOsc32Mclk = 0x0, /*!< 32MHz FRO or XTAL clock */
    kCLOCK_PWMFro48Mclk = 0x1, /*!< FRO 48MHz clock */
    kCLOCK_PWMNoClkSel  = 0x2, /*!< No clock selected - Shutdown functional
                            PWM clock for power saving */
    kCLOCK_PWMTestClk = 0x3,   /*!< Test clock input - Shutdown functional
                              PWM clock for power saving */
} pwm_clock_source_t;

/*! @brief FRO clock selection values */
typedef enum
{
    FRO12M_ENA = (1 << 0), /*!< FRO12M */
    FRO32M_ENA = (1 << 1), /*!< FRO32M */
    FRO48M_ENA = (1 << 2), /*!< FRO48M */
    FRO64M_ENA = (1 << 3), /*!< FRO64M */
    FRO96M_ENA = (1 << 4)  /*!< FRO96M */
} Fro_ClkSel_t;

/*! @brief Board specific constant capacitance characteristics
 * Should be supplied by board manufacturer for best performance.
 * Capacitances are expressed in hundreds of pF
 */
typedef struct
{
    uint32_t clk_XtalIecLoadpF_x100;    /*< XTAL Load capacitance */
    uint32_t clk_XtalPPcbParCappF_x100; /*< XTAL PCB +ve parasitic capacitance */
    uint32_t clk_XtalNPcbParCappF_x100; /*< XTAL PCB -ve parasitic capacitance */
} ClockCapacitanceCompensation_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/**
 * @brief	Obtains frequency of specified clock
 * @param   clock_name_t  specify clock to be read
 * @return	uint32_t      frequency
 * @note
 */
uint32_t CLOCK_GetFreq(clock_name_t clock);

/**
 * @brief	Selects clock source using <name>SEL register in syscon
 * @param   clock_attach_id_t  specify clock mapping
 * @return	none
 * @note
 */
void CLOCK_AttachClk(clock_attach_id_t connection);

/**
 * @brief	Selects clock divider using <name>DIV register in syscon
 * @param   clock_div_name_t  	specifies which DIV register we are accessing
 * @param   uint32_t			specifies divisor
 * @param	bool				true if a syscon clock reset should also be carried out
 * @return	none
 * @note
 */
void CLOCK_SetClkDiv(clock_div_name_t div_name, uint32_t divided_by_value, bool reset);

/**
 * @brief	Enables specific AHB clock channel
 * @param   clock_ip_name_t  	specifies which peripheral clock we are controlling
 * @return	none
 * @note	clock_ip_name_t is a typedef clone of clock_name_t
 */
void CLOCK_EnableClock(clock_ip_name_t clk);

/**
 * @brief	Disables specific AHB clock channel
 * @param   clock_ip_name_t  	specifies which peripheral clock we are controlling
 * @return	none
 * @note	clock_ip_name_t is a typedef clone of clock_name_t
 */
void CLOCK_DisableClock(clock_ip_name_t clk);

/**
 * @brief	Check if clock is enabled
 * @param   clock_ip_name_t  	specifies which peripheral clock we are controlling
 * @return  bool
 * @note	clock_ip_name_t is a typedef clone of clock_name_t
 */
bool CLOCK_IsClockEnable(clock_ip_name_t clk);

/**
 * @brief	Obtains frequency of APB Bus clock
 * @param   none
 * @return	uint32_t      frequency
 * @note
 */
uint32_t CLOCK_GetApbCLkFreq(void);

void CLOCK_EnableAPBBridge(void);

void CLOCK_DisableAPBBridge(void);

/*! @brief      Return Frequency of Spifi Clock
 *  @return     Frequency of Spifi.
 */
uint32_t CLOCK_GetSpifiClkFreq(void);

/*! @brief   Delay execution by busy waiting
 *  @param   delayUs delay duration in micro seconds
 *  @return  none
 */
void CLOCK_uDelay(uint32_t delayUs);

/**
 * @brief	Sets default trim values for 32MHz XTAL
 * @param   none
 * @return	none
 * @note    Has no effect if CLOCK_Xtal32M_Trim has been called
 */
void CLOCK_XtalBasicTrim(void);

/**
 * @brief   Sets board-specific trim values for 32MHz XTAL
 * @param   XO_32M_OSC_CAP_Delta_x1000 capacitance correction in fF (femtoFarad)
 * @param   capa_charac board 32M capacitance characteristics pointer
 * @return  none
 * @note    capa_charac must point to a struct set in board.c using
 *          CLOCK_32MfXtalIecLoadpF    Load capacitance, pF
 *          CLOCK_32MfXtalPPcbParCappF PCB +ve parasitic capacitance, pF
 *          CLOCK_32MfXtalNPcbParCappF PCB -ve parasitic capacitance, pF
 */
void CLOCK_Xtal32M_Trim(int32_t XO_32M_OSC_CAP_Delta_x1000, const ClockCapacitanceCompensation_t *capa_charac);

/**
 * @brief   Sets board-specific trim values for 32kHz XTAL
 * @param   XO_32k_OSC_CAP_Delta_x1000 capacitance correction in fF
 * @param   capa_charac board 32k capacitance characteristics pointer
 * @return  none
 * @note    capa_charac must point to a struct set in board.c using
 *          CLOCK_32kfXtalIecLoadpF    Load capacitance, pF
 *          CLOCK_32kfXtalPPcbParCappF PCB +ve parasitic capacitance, pF
 *          CLOCK_32kfXtalNPcbParCappF PCB -ve parasitic capacitance, pF
 */
void CLOCK_Xtal32k_Trim(int32_t XO_32k_OSC_CAP_Delta_x1000, const ClockCapacitanceCompensation_t *capa_charac);

/**
 * @brief	Enables and sets LDO for 32MHz XTAL
 * @param   none
 * @return	none
 */
void CLOCK_SetXtal32M_LDO(void);

/**
 * @brief	Waits for 32MHz XTAL to stabilise
 * @param   u32AdditionalWait_us Additional wait after hardware indicates that
 *                               stability has been reached
 * @return	none
 * @note    Operates as a tight loop. Worst case would be ~600ms
 */
void CLOCK_Xtal32M_WaitUntilStable(uint32_t u32AdditionalWait_us);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @} */

#endif /* _FSL_CLOCK_H_ */
