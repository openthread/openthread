/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ROM_LOWPOWER_H_
#define __ROM_LOWPOWER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rom_common.h>
#include <rom_pmc.h>

/*!
 * @addtogroup ROM_API
 * @{
 */

/*! @file */

/*******************
 * EXPORTED MACROS  *
 ********************/

/* Low Power modes  */
#define LOWPOWER_CFG_MODE_INDEX 0
#define LOWPOWER_CFG_MODE_MASK (0x3UL << LOWPOWER_CFG_MODE_INDEX)

#define LOWPOWER_CFG_XTAL32MSTARTENA_INDEX 2
#define LOWPOWER_CFG_XTAL32MSTARTENA_MASK (0x1UL << LOWPOWER_CFG_XTAL32MSTARTENA_INDEX)

#define LOWPOWER_CFG_FLASHPWDNMODE_INDEX 4
#define LOWPOWER_CFG_FLASHPWDNMODE_MASK (0x1UL << LOWPOWER_CFG_FLASHPWDNMODE_INDEX)

#define LOWPOWER_CFG_SRAMPWDNMODE_INDEX 6
#define LOWPOWER_CFG_SRAMPWDNMODE_MASK (0x1UL << LOWPOWER_CFG_SRAMPWDNMODE_INDEX)

#define LOWPOWER_CFG_PDRUNCFG_DISCARD_INDEX 7
#define LOWPOWER_CFG_PDRUNCFG_DISCARD_MASK (0x1UL << LOWPOWER_CFG_PDRUNCFG_DISCARD_INDEX)

#define LOWPOWER_CFG_WFI_NOT_WFE_INDEX 8
#define LOWPOWER_CFG_WFI_NOT_WFE_MASK (0x1UL << LOWPOWER_CFG_WFI_NOT_WFE_INDEX)

#define LOWPOWER_CFG_LDOMEM_FORCE_ENABLE_INDEX 9
#define LOWPOWER_CFG_LDOMEM_FORCE_ENABLE_MASK (0x1UL << LOWPOWER_CFG_LDOMEM_FORCE_ENABLE_INDEX)

#define LOWPOWER_CFG_LDOFLASHCORE_UPDATE_INDEX 10
#define LOWPOWER_CFG_LDOFLASHCORE_UPDATE_MASK (0x1UL << LOWPOWER_CFG_LDOFLASHCORE_UPDATE_INDEX)

/* Delay to wakeup the flash after the LDO Flash Core has been set up to active voltage : 0: 19us, then increment by
 * 4.75us steps */
#define LOWPOWER_CFG_LDOFLASHCORE_DELAY_INDEX 11
#define LOWPOWER_CFG_LDOFLASHCORE_DELAY_MASK (0x7UL << LOWPOWER_CFG_LDOFLASHCORE_DELAY_INDEX)

#define LOWPOWER_CFG_MODE_ACTIVE 0        /*!< ACTIVE mode */
#define LOWPOWER_CFG_MODE_DEEPSLEEP 1     /*!< DEEP SLEEP mode */
#define LOWPOWER_CFG_MODE_POWERDOWN 2     /*!< POWER DOWN mode */
#define LOWPOWER_CFG_MODE_DEEPPOWERDOWN 3 /*!< DEEP POWER DOWN mode */

#define LOWPOWER_CFG_XTAL32MSTART_DISABLE \
    0 /*!< Disable Crystal 32 MHz automatic start when waking up from POWER DOWN and DEEP POWER DOWN modes */
#define LOWPOWER_CFG_XTAL32MSTART_ENABLE \
    1 /*!< Enable Crystal 32 MHz automatic start when waking up from POWER DOWN and DEEP POWER DOWN modes */

#define LOWPOWER_CFG_FLASHPWDNMODE_FLASHPWND \
    0 /*!< Power down the Flash only (send CMD_POWERDOWN to Flash controller). Only valid in DEEP SLEEP mode */
#define LOWPOWER_CFG_FLASHPWDNMODE_LDOSHUTOFF                                                                         \
    1 /*!< Power down the Flash ((send CMD_POWERDOWN to Flash controller) and shutoff both Flash LDOs (Core and NV) \ \
         (only valid in DEEP SLEEP mode) */

/**
 * @brief Analog Power Domains (analog components in Power Management Unit) Low Power Modes control
 */
#define LOWPOWER_PMUPWDN_DCDC (1UL << 0)     /*!< Power Down DCDC Converter  */
#define LOWPOWER_PMUPWDN_BIAS (1UL << 1)     /*!< Power Down all Bias and references */
#define LOWPOWER_PMUPWDN_LDOMEM (1UL << 2)   /*!< Power Down Memories LDO  */
#define LOWPOWER_PMUPWDN_BODVBAT (1UL << 3)  /*!< Power Down VBAT Brown Out Detector */
#define LOWPOWER_PMUPWDN_FRO192M (1UL << 4)  /*!< Power Down FRO 192 MHz  */
#define LOWPOWER_PMUPWDN_FRO1M (1UL << 5)    /*!< Power Down FRO 1 MHz  */
#define LOWPOWER_PMUPWDN_GPADC (1UL << 22)   /*!< Power Down General Purpose ADC */
#define LOWPOWER_PMUPWDN_BODMEM (1UL << 23)  /*!< Power Down Memories Brown Out Detector */
#define LOWPOWER_PMUPWDN_BODCORE (1UL << 24) /*!< Power Down Core Logic Brown Out Detector */
#define LOWPOWER_PMUPWDN_FRO32K (1UL << 25)  /*!< Power Down FRO 32 KHz */
#define LOWPOWER_PMUPWDN_XTAL32K (1UL << 26) /*!< Power Down Crystal 32 KHz */
#define LOWPOWER_PMUPWDN_ANACOMP (1UL << 27) /*!< Power Down Analog Comparator */

#define LOWPOWER_PMUPWDN_XTAL32M (1UL << 28)    /*!< Power Down Crystal 32 MHz */
#define LOWPOWER_PMUPWDN_TEMPSENSOR (1UL << 29) /*!< Power Down Temperature Sensor   */

/**
 * @brief Digital Power Domains Low Power Modes control
 */
#define LOWPOWER_DIGPWDN_FLASH                                                                                     \
    (1UL << 6) /*!< Power Down Flash Power Domain (Flash Macro, Flash Controller and/or FLash LDOs, depending on \ \
                   LOWPOWER_CFG_FLASHPWDNMODE parameter) */
#define LOWPOWER_DIGPWDN_COMM0 (1UL << 7) /*!< Power Down Digital COMM0 power domain (USART0, I2C0 and SPI0) */
#define LOWPOWER_DIGPWDN_MCU_RET                                                                                    \
    (1UL << 8) /*!< Power Down MCU Retention Power Domain (Disable Zigbee IP retention, ES1:Disable CPU retention \ \
                   flip-flops) */
#define LOWPOWER_DIGPWDN_ZIGBLE_RET \
    (1UL << 9) /*!< Power Down ZIGBEE/BLE retention Power Domain (Disable ZIGBEE/BLE retention flip-flops) */

#define LOWPOWER_DIGPWDN_SRAM0_INDEX 10
#define LOWPOWER_DIGPWDN_SRAM0 \
    (1UL << LOWPOWER_DIGPWDN_SRAM0_INDEX)   /*!< Power Down SRAM 0  instance [Bank 0, 16 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM1 (1UL << 11)  /*!< Power Down SRAM 1  instance [Bank 0, 16 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM2 (1UL << 12)  /*!< Power Down SRAM 2  instance [Bank 0, 16 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM3 (1UL << 13)  /*!< Power Down SRAM 3  instance [Bank 0, 16 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM4 (1UL << 14)  /*!< Power Down SRAM 4  instance [Bank 0,  8 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM5 (1UL << 15)  /*!< Power Down SRAM 5  instance [Bank 0,  8 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM6 (1UL << 16)  /*!< Power Down SRAM 6  instance [Bank 0,  4 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM7 (1UL << 17)  /*!< Power Down SRAM 7  instance [Bank 0,  4 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM8 (1UL << 18)  /*!< Power Down SRAM 8  instance [Bank 1, 16 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM9 (1UL << 19)  /*!< Power Down SRAM 9  instance [Bank 1, 16 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM10 (1UL << 20) /*!< Power Down SRAM 10 instance [Bank 1, 16 KB], (no retention) */
#define LOWPOWER_DIGPWDN_SRAM11 (1UL << 21) /*!< Power Down SRAM 11 instance [Bank 1, 16 KB], (no retention) */

#define LOWPOWER_DIGPWDN_SRAM_ALL_MASK                                                                   \
    (LOWPOWER_DIGPWDN_SRAM0 | LOWPOWER_DIGPWDN_SRAM1 | LOWPOWER_DIGPWDN_SRAM2 | LOWPOWER_DIGPWDN_SRAM3 | \
     LOWPOWER_DIGPWDN_SRAM4 | LOWPOWER_DIGPWDN_SRAM5 | LOWPOWER_DIGPWDN_SRAM6 | LOWPOWER_DIGPWDN_SRAM7 | \
     LOWPOWER_DIGPWDN_SRAM8 | LOWPOWER_DIGPWDN_SRAM9 | LOWPOWER_DIGPWDN_SRAM10 | LOWPOWER_DIGPWDN_SRAM11)

#define LOWPOWER_DIGPWDN_IO_INDEX 30
#define LOWPOWER_DIGPWDN_IO (1UL << LOWPOWER_DIGPWDN_IO_INDEX) /*!< Power Down   */

#define LOWPOWER_DIGPWDN_NTAG_FD_INDEX 31
#define LOWPOWER_DIGPWDN_NTAG_FD \
    (1UL << LOWPOWER_DIGPWDN_NTAG_FD_INDEX) /*!< NTAG FD field detect Disable - need the IO source to be set too */

/**
 * @brief LDO Voltage control in Low Power Modes
 */

#define LOWPOWER_SRAM_LPMODE_MASK (0xFUL)
#define LOWPOWER_SRAM_LPMODE_ACTIVE (0x6UL)    /*!<  */
#define LOWPOWER_SRAM_LPMODE_SLEEP (0xFUL)     /*!<  */
#define LOWPOWER_SRAM_LPMODE_DEEPSLEEP (0x8UL) /*!<  */
#define LOWPOWER_SRAM_LPMODE_SHUTDOWN (0x9UL)  /*!<  */
#define LOWPOWER_SRAM_LPMODE_POWERUP (0xAUL)   /*!<  */

/**
 * @brief LDO Voltage control in Low Power Modes
 */
#define LOWPOWER_VOLTAGE_LDO_PMU_INDEX 0
#define LOWPOWER_VOLTAGE_LDO_PMU_MASK (0x1FUL << LOWPOWER_VOLTAGE_LDO_PMU_INDEX)
#define LOWPOWER_VOLTAGE_LDO_MEM_INDEX 5
#define LOWPOWER_VOLTAGE_LDO_MEM_MASK (0x1FUL << LOWPOWER_VOLTAGE_LDO_MEM_INDEX)
#define LOWPOWER_VOLTAGE_LDO_CORE_INDEX 10
#define LOWPOWER_VOLTAGE_LDO_CORE_MASK (0x7UL << LOWPOWER_VOLTAGE_LDO_CORE_INDEX)
#define LOWPOWER_VOLTAGE_LDO_FLASH_CORE_INDEX 13
#define LOWPOWER_VOLTAGE_LDO_FLASH_CORE_MASK (0x7UL << LOWPOWER_VOLTAGE_LDO_FLASH_CORE_INDEX)
#define LOWPOWER_VOLTAGE_LDO_FLASH_NV_INDEX 16
#define LOWPOWER_VOLTAGE_LDO_FLASH_NV_MASK (0x7UL << LOWPOWER_VOLTAGE_LDO_FLASH_NV_INDEX)
#define LOWPOWER_VOLTAGE_LDO_PMU_BOOST_INDEX 19
#define LOWPOWER_VOLTAGE_LDO_PMU_BOOST_MASK (0x1FUL << LOWPOWER_VOLTAGE_LDO_PMU_BOOST_INDEX)
#define LOWPOWER_VOLTAGE_LDO_MEM_BOOST_INDEX 24
#define LOWPOWER_VOLTAGE_LDO_MEM_BOOST_MASK (0x1FUL << LOWPOWER_VOLTAGE_LDO_MEM_BOOST_INDEX)
/* Only for ES2 but defined it for ES1 for easier power driver writing (has no effect on ES1) */
#define LOWPOWER_VOLTAGE_LDO_PMU_BOOST_ENABLE_INDEX 29
#define LOWPOWER_VOLTAGE_LDO_PMU_BOOST_ENABLE_MASK (0x1UL << LOWPOWER_VOLTAGE_LDO_PMU_BOOST_ENABLE_INDEX)

/**
 * @brief Low Power Modes Wake up Interrupt sources
 */
#define LOWPOWER_WAKEUPSRCINT0_SYSTEM_IRQ \
    (1UL << 0) /*!< BOD, Watchdog Timer, Flash controller, Firewall [DEEP SLEEP] BOD [POWER_DOWN]*/
#define LOWPOWER_WAKEUPSRCINT0_DMA_IRQ (1UL << 1)       /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_GINT_IRQ (1UL << 2)      /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_IRBLASTER_IRQ (1UL << 3) /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PINT0_IRQ (1UL << 4)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PINT1_IRQ (1UL << 5)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PINT2_IRQ (1UL << 6)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PINT3_IRQ (1UL << 7)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_SPIFI_IRQ (1UL << 8)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_TIMER0_IRQ (1UL << 9)    /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_TIMER1_IRQ (1UL << 10)   /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_USART0_IRQ (1UL << 11)   /*!< [DEEP SLEEP, POWER DOWN] */
#define LOWPOWER_WAKEUPSRCINT0_USART1_IRQ (1UL << 12)   /*!< [DEEP SLEEP]  */
#define LOWPOWER_WAKEUPSRCINT0_I2C0_IRQ (1UL << 13)     /*!< [DEEP SLEEP, POWER DOWN] */
#define LOWPOWER_WAKEUPSRCINT0_I2C1_IRQ (1UL << 14)     /*!< [DEEP SLEEP]  */
#define LOWPOWER_WAKEUPSRCINT0_SPI0_IRQ (1UL << 15)     /*!< [DEEP SLEEP, POWER DOWN] */
#define LOWPOWER_WAKEUPSRCINT0_SPI1_IRQ (1UL << 16)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM0_IRQ (1UL << 17)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM1_IRQ (1UL << 18)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM2_IRQ (1UL << 19)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM3_IRQ (1UL << 20)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM4_IRQ (1UL << 21)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM5_IRQ (1UL << 22)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM6_IRQ (1UL << 23)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM7_IRQ (1UL << 24)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM8_IRQ (1UL << 25)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM9_IRQ (1UL << 26)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_PWM10_IRQ (1UL << 27)    /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_I2C2_IRQ (1UL << 28)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT0_RTC_IRQ (1UL << 29)      /*!< [DEEP SLEEP, POWER DOWN]  */
#define LOWPOWER_WAKEUPSRCINT0_NFCTAG_IRQ (1UL << 30)   /*!< [DEEP SLEEP]  */
#define LOWPOWER_WAKEUPSRCINT0_MAILBOX_IRQ \
    (1UL << 31) /*!< Mailbox, Wake-up from DEEP SLEEP and POWER DOWN low power mode [DEEP SLEEP, POWER DOWN] */

#define LOWPOWER_WAKEUPSRCINT1_ADC_SEQA_IRQ (1UL << 0)        /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_ADC_SEQB_IRQ (1UL << 1)        /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_ADC_THCMP_OVR_IRQ (1UL << 2)   /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_DMIC_IRQ (1UL << 3)            /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_HWVAD_IRQ (1UL << 4)           /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_BLE_DP_IRQ (1UL << 5)          /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_BLE_DP0_IRQ (1UL << 6)         /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_BLE_DP1_IRQ (1UL << 7)         /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_BLE_DP2_IRQ (1UL << 8)         /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_BLE_LL_ALL_IRQ (1UL << 9)      /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_ZIGBEE_MAC_IRQ (1UL << 10)     /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_ZIGBEE_MODEM_IRQ (1UL << 11)   /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_RFP_TMU_IRQ (1UL << 12)        /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_RFP_AGC_IRQ (1UL << 13)        /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_ISO7816_IRQ (1UL << 14)        /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_ANA_COMP_IRQ (1UL << 15)       /*!< [DEEP SLEEP] */
#define LOWPOWER_WAKEUPSRCINT1_WAKE_UP_TIMER0_IRQ (1UL << 16) /*!< [DEEP SLEEP, POWER DOWN] */
#define LOWPOWER_WAKEUPSRCINT1_WAKE_UP_TIMER1_IRQ (1UL << 17) /*!< [DEEP SLEEP, POWER DOWN] */
#define LOWPOWER_WAKEUPSRCINT1_BLE_WAKE_TIMER_IRQ (1UL << 22) /*!< [DEEP SLEEP, POWER DOWN] */
#define LOWPOWER_WAKEUPSRCINT1_BLE_OSC_EN_IRQ (1UL << 23)     /*!< [DEEP SLEEP, POWER DOWN] */
#define LOWPOWER_WAKEUPSRCINT1_IO_IRQ (1UL << 31)             /*!< [POWER DOWN, DEEP DOWN]  */

/**
 * @brief Sleep Postpone
 */
#define LOWPOWER_SLEEPPOSTPONE_FORCED \
    (1UL << 0) /*!< Forces postpone of power down modes in case the processor request low power mode*/
#define LOWPOWER_SLEEPPOSTPONE_PERIPHERALS                                                                             \
    (1UL << 1) /*!< USART0, USART1, SPI0, SPI1, I2C0, I2C1, I2C2 interrupts can postpone power down modes in case an \ \
                   interrupt is pending when the processor request low power mode */
#define LOWPOWER_SLEEPPOSTPONE_DMIC                                                                                   \
    (1UL << 0) /*!< DMIC interrupt can postpone power down modes in case an interrupt is pending when the processor \ \
                   request low power mode */
#define LOWPOWER_SLEEPPOSTPONE_SDMA                                                                               \
    (1UL << 1) /*!< System DMA interrupt can postpone power down modes in case an interrupt is pending when the \ \
                   processor request low power mode */
#define LOWPOWER_SLEEPPOSTPONE_NFCTAG                                                                          \
    (1UL << 0) /*!< NFC Tag interrupt can postpone power down modes in case an interrupt is pending when the \ \
                   processor request low power mode */
#define LOWPOWER_SLEEPPOSTPONE_BLEOSC                                                                             \
    (1UL << 1) /*!< BLE_OSC_EN interrupt can postpone power down modes in case an interrupt is pending when the \ \
                   processor request low power mode */

/**
 * @brief Wake up I/O sources
 */
#define LOWPOWER_WAKEUPIOSRC_PIO0 (1UL << 0)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO1 (1UL << 1)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO2 (1UL << 2)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO3 (1UL << 3)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO4 (1UL << 4)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO5 (1UL << 5)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO6 (1UL << 6)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO7 (1UL << 7)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO8 (1UL << 8)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO9 (1UL << 9)   /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO10 (1UL << 10) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO11 (1UL << 11) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO12 (1UL << 12) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO13 (1UL << 13) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO14 (1UL << 14) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO15 (1UL << 15) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO16 (1UL << 16) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO17 (1UL << 17) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO18 (1UL << 18) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO19 (1UL << 19) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO20 (1UL << 20) /*!<    */
#define LOWPOWER_WAKEUPIOSRC_PIO21 (1UL << 21) /*!<    */

#ifndef LOWPOWER_API_ES1_ONLY
/* For NTAG FD wakeup source, SW shall enable virtual IO 22 in */
#define LOWPOWER_WAKEUPIOSRC_NTAG_FD (1UL << 22) /*!<    */
#endif

/**
 * @brief I/O whose state must be kept in Power Down mode
 */
#define LOWPOWER_GPIOLATCH_PIO0 (1UL << 0)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO1 (1UL << 1)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO2 (1UL << 2)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO3 (1UL << 3)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO4 (1UL << 4)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO5 (1UL << 5)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO6 (1UL << 6)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO7 (1UL << 7)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO8 (1UL << 8)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO9 (1UL << 9)   /*!<    */
#define LOWPOWER_GPIOLATCH_PIO10 (1UL << 10) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO11 (1UL << 11) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO12 (1UL << 12) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO13 (1UL << 13) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO14 (1UL << 14) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO15 (1UL << 15) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO16 (1UL << 16) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO17 (1UL << 17) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO18 (1UL << 18) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO19 (1UL << 19) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO20 (1UL << 20) /*!<    */
#define LOWPOWER_GPIOLATCH_PIO21 (1UL << 21) /*!<    */

/**
 * @brief Wake up timers configuration in Low Power Modes
 */
#define LOWPOWER_TIMERCFG_ENABLE_INDEX 0
#define LOWPOWER_TIMERCFG_ENABLE_MASK (0x1UL << LOWPOWER_TIMERCFG_ENABLE_INDEX)
#define LOWPOWER_TIMERCFG_TIMER_INDEX 1
#define LOWPOWER_TIMERCFG_TIMER_MASK (0x7UL << LOWPOWER_TIMERCFG_TIMER_INDEX)
#define LOWPOWER_TIMERCFG_OSC32K_INDEX 4
#define LOWPOWER_TIMERCFG_OSC32K_MASK (0x1UL << LOWPOWER_TIMERCFG_OSC32K_INDEX)
#define LOWPOWER_TIMERCFG_2ND_ENABLE_INDEX 5
#define LOWPOWER_TIMERCFG_2ND_ENABLE_MASK (0x1UL << LOWPOWER_TIMERCFG_2ND_ENABLE_INDEX)
#define LOWPOWER_TIMERCFG_2ND_TIMER_INDEX 6
#define LOWPOWER_TIMERCFG_2ND_TIMER_MASK (0x7UL << LOWPOWER_TIMERCFG_2ND_TIMER_INDEX)

#define LOWPOWER_TIMERCFG_TIMER_ENABLE 1 /*!< Wake Timer Enable */

/**
 * @brief Primary Wake up timers configuration in Low Power Modes
 */
#define LOWPOWER_TIMERCFG_TIMER_WAKEUPTIMER0 0   /*!< Zigbee Wake up Counter 0 used as wake up source */
#define LOWPOWER_TIMERCFG_TIMER_WAKEUPTIMER1 1   /*!< Zigbee Wake up Counter 1 used as wake up source */
#define LOWPOWER_TIMERCFG_TIMER_BLEWAKEUPTIMER 2 /*!< BLE Wake up Counter used as wake up source */
#define LOWPOWER_TIMERCFG_TIMER_RTC1KHZ 3        /*!< 1 KHz Real Time Counter (RTC) used as wake up source */
#define LOWPOWER_TIMERCFG_TIMER_RTC1HZ 4         /*!< 1 Hz Real Time Counter (RTC) used as wake up source */

/**
 * @brief Secondary Wake up timers configuration in Low Power Modes
 */
#define LOWPOWER_TIMERCFG_2ND_TIMER_WAKEUPTIMER0 0   /*!< Zigbee Wake up Counter 0 used as secondary wake up source */
#define LOWPOWER_TIMERCFG_2ND_TIMER_WAKEUPTIMER1 1   /*!< Zigbee Wake up Counter 1 used as secondary wake up source */
#define LOWPOWER_TIMERCFG_2ND_TIMER_BLEWAKEUPTIMER 2 /*!< BLE Wake up Counter used as secondary wake up source */
#define LOWPOWER_TIMERCFG_2ND_TIMER_RTC1KHZ 3 /*!< 1 KHz Real Time Counter (RTC) used as secondary wake up source */
#define LOWPOWER_TIMERCFG_2ND_TIMER_RTC1HZ 4  /*!< 1 Hz Real Time Counter (RTC) used as secondary wake up source */

#define LOWPOWER_TIMERCFG_OSC32K_FRO32KHZ 0  /*!< Wake up Timers uses FRO 32 KHz as clock source */
#define LOWPOWER_TIMERCFG_OSC32K_XTAL32KHZ 1 /*!< Wake up Timers uses Chrystal 32 KHz as clock source */

/**
 * @brief BLE Wake up timers configuration in Low Power Modes
 */
#define LOWPOWER_TIMERBLECFG_RADIOEN_INDEX 0
#define LOWPOWER_TIMERBLECFG_RADIOEN_MASK (0x3FFUL << LOWPOWER_TIMERBLECFG_RADIOEN_INDEX)
#define LOWPOWER_TIMERBLECFG_OSCEN_INDEX 10
#define LOWPOWER_TIMERBLECFG_OSCEN_MASK (0x7FFUL << LOWPOWER_TIMERBLECFG_OSCEN_INDEX)

/** @brief	Low Power Main Structure */
typedef struct
{                           /*     */
    uint32_t CFG;           /*!< Low Power Mode Configuration, and miscallenous options  */
    uint32_t PMUPWDN;       /*!< Analog Power Domains (analog components in Power Management Unit) Low Power Modes  */
    uint32_t DIGPWDN;       /*!< Digital Power Domains Low Power Modes */
    uint32_t VOLTAGE;       /*!< LDO Voltage control in Low Power Modes */
    uint32_t WAKEUPSRCINT0; /*!< Wake up sources Interrupt control */
    uint32_t WAKEUPSRCINT1; /*!< Wake up sources Interrupt control */
    uint32_t SLEEPPOSTPONE; /*!< Interrupt that can postpone power down modes in case an interrupt is pending when the
                               processor request deepsleep */
    uint32_t WAKEUPIOSRC;   /*!< Wake up I/O sources */
    uint32_t GPIOLATCH;     /*!< I/Os which outputs level must be kept (in Power Down  mode)*/
    uint32_t TIMERCFG;      /*!< Wake up timers configuration */
    uint32_t TIMERBLECFG;   /*!< BLE wake up timer configuration (OSC_EN and RADIO_EN) */
    uint32_t TIMERCOUNTLSB; /*!< Wake up Timer LSB*/
    uint32_t TIMERCOUNTMSB; /*!< Wake up Timer MSB*/
    uint32_t TIMER2NDCOUNTLSB; /*!< Second Wake up Timer LSB*/
    uint32_t TIMER2NDCOUNTMSB; /*!< Second Wake up Timer MSB*/

} LPC_LOWPOWER_T;

/** @brief	Low Power Main Structure */
typedef struct
{                         /*   */
    uint8_t LDOPMU;       /*!< Always-ON domain LDO voltage configuration */
    uint8_t LDOPMUBOOST;  /*!< Always-ON domain LDO Boost voltage configuration */
    uint8_t LDOMEM;       /*!< Memories LDO voltage configuration */
    uint8_t LDOMEMBOOST;  /*!< Memories LDO Boost voltage configuration */
    uint8_t LDOCORE;      /*!< Core Logic domain LDO voltage configuration */
    uint8_t LDOFLASHNV;   /*!< Flash NV domain LDO voltage configuration */
    uint8_t LDOFLASHCORE; /*!< Flash Core domain LDO voltage configuration */
    uint8_t LDOADC;       /*!< General Purpose ADC LDO voltage configuration */
#ifndef LOWPOWER_API_ES1_ONLY
    uint8_t LDOPMUBOOST_ENABLE; /*!< Force Boost activation on LDOPMU */
#endif
} LPC_LOWPOWER_LDOVOLTAGE_T;

/**
 * @brief	Configure Wake or RTC timers. used for testing only
 * @param	p_lowpower_cfg: pointer to a structure that contains all low power mode parameters
 * @return	Nothing
 */
static inline void Chip_LOWPOWER_SetUpLowPowerModeWakeUpTimer(LPC_LOWPOWER_T *p_lowpower_cfg)
{
    void (*p_Chip_LOWPOWER_SetUpLowPowerModeWakeUpTimer)(LPC_LOWPOWER_T * p_lowpower_cfg);
    p_Chip_LOWPOWER_SetUpLowPowerModeWakeUpTimer = (void (*)(LPC_LOWPOWER_T * p_lowpower_cfg))0x030038d1U;

    p_Chip_LOWPOWER_SetUpLowPowerModeWakeUpTimer(p_lowpower_cfg);
}
/**
 * @brief	Configure CPU and System Bus clock frequency
 * @param	Frequency	:
 * @return	Nothing
 */
static inline int Chip_LOWPOWER_SetSystemFrequency(uint32_t frequency)
{
    int (*p_Chip_LOWPOWER_SetSystemFrequency)(uint32_t frequency);
    p_Chip_LOWPOWER_SetSystemFrequency = (int (*)(uint32_t frequency))0x03003d55U;

    return p_Chip_LOWPOWER_SetSystemFrequency(frequency);
}

/**
 * @brief	Configure Memory instances Low Power Mode
 * @param	p_sram_instance: SRAM instance number, between 0 and 11.
 * @param	p_sram_lp_mode : Low power mode : LOWPOWER_SRAM_LPMODE_ACTIVE, LOWPOWER_SRAM_LPMODE_SLEEP,
 * LOWPOWER_SRAM_LPMODE_DEEPSLEEP, LOWPOWER_SRAM_LPMODE_SHUTDOWN
 * @return	Status code
 */
static inline int Chip_LOWPOWER_SetMemoryLowPowerMode(uint32_t p_sram_instance, uint32_t p_sram_lp_mode)
{
    int (*p_Chip_LOWPOWER_SetMemoryLowPowerMode)(uint32_t p_sram_instance, uint32_t p_sram_lp_mode);
    p_Chip_LOWPOWER_SetMemoryLowPowerMode = (int (*)(uint32_t p_sram_instance, uint32_t p_sram_lp_mode))0x03003d89U;

    return p_Chip_LOWPOWER_SetMemoryLowPowerMode(p_sram_instance, p_sram_lp_mode);
}

/**
 * @brief	Get System Voltages
 * @param	p_ldo_voltage: pointer to a structure to fill with current voltages on the chip
 * @return	Nothing
 */
static inline void Chip_LOWPOWER_GetSystemVoltages(LPC_LOWPOWER_LDOVOLTAGE_T *p_ldo_voltage)
{
    void (*p_Chip_LOWPOWER_GetSystemVoltages)(LPC_LOWPOWER_LDOVOLTAGE_T * p_ldo_voltage);
    p_Chip_LOWPOWER_GetSystemVoltages = (void (*)(LPC_LOWPOWER_LDOVOLTAGE_T * p_ldo_voltage))0x03003de1U;

    p_Chip_LOWPOWER_GetSystemVoltages(p_ldo_voltage);
}

/**
 * @brief	Configure System Voltages
 * @param	p_ldo_voltage: pointer to a structure that contains new voltages to be applied
 * @return	Nothing
 */
static inline void Chip_LOWPOWER_SetSystemVoltages(LPC_LOWPOWER_LDOVOLTAGE_T *p_ldo_voltage)
{
    void (*p_Chip_LOWPOWER_SetSystemVoltages)(LPC_LOWPOWER_LDOVOLTAGE_T * p_ldo_voltage);
    p_Chip_LOWPOWER_SetSystemVoltages = (void (*)(LPC_LOWPOWER_LDOVOLTAGE_T * p_ldo_voltage))0x03003e99U;

    p_Chip_LOWPOWER_SetSystemVoltages(p_ldo_voltage);
}

/**
 * @brief	Configure and enters in low power mode
 * @param	p_lowpower_cfg: pointer to a structure that contains all low power mode parameters
 * @return	Nothing
 */
static inline void Chip_LOWPOWER_SetLowPowerMode(LPC_LOWPOWER_T *p_lowpower_cfg)
{
    void (*p_Chip_LOWPOWER_SetLowPowerMode)(LPC_LOWPOWER_T * p_lowpower_cfg);
    p_Chip_LOWPOWER_SetLowPowerMode = (void (*)(LPC_LOWPOWER_T * p_lowpower_cfg))0x0300404dU;

    p_Chip_LOWPOWER_SetLowPowerMode(p_lowpower_cfg);
}

/**
 * @brief	Perform a Full chip reset using Software reset bit in PMC
 *
 *  Power down the flash then perform the full chip reset as POR or Watchdog do,
 *  The reset includes JTAG debugger, Digital units and Analog modules.
 *  Use the Software reset bit in PMC
 *
 * @return	Nothing
 */
static inline void Chip_LOWPOWER_ChipSoftwareReset(void)
{
    void (*p_Chip_LOWPOWER_ChipSoftwareReset)(void);
    p_Chip_LOWPOWER_ChipSoftwareReset = (void (*)(void))0x03003fa1U;

    p_Chip_LOWPOWER_ChipSoftwareReset();
}

/**
 * @brief	Perform a digital System reset
 *
 *  Power down the flash then perform the Full chip reset as POR or Watchdog,
 *  The reset includes the digital units but excludes the JTAG debugger, and the analog modules.
 *  Use the system reset bit in PMC and ARM reset
 *
 * @return	Nothing
 */
static inline void Chip_LOWPOWER_ArmSoftwareReset(void)
{
    void (*p_Chip_LOWPOWER_ArmSoftwareReset)(void);
    p_Chip_LOWPOWER_ArmSoftwareReset = (void (*)(void))0x0300400dU;

    p_Chip_LOWPOWER_ArmSoftwareReset();
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ROM_LOWPOWER_H_ */
