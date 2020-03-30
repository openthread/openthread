/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_POWER_H_
#define _FSL_POWER_H_

#include "rom_lowpower.h"
#include "fsl_common.h"

/*! @addtogroup power */
/*! @{ */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief BODVBAT configuration flag */
#define POWER_BOD_ENABLE           ( 1 << 0 )
#define POWER_BOD_DISABLE          ( 0 << 0 )
#define POWER_BOD_INT_ENABLE       ( 1 << 1 )
#define POWER_BOD_RST_ENABLE       ( 1 << 2 )
#define POWER_BOD_HIGH             ( 1 << 3 ) /*!< ES2 BOD VBAT only */
#define POWER_BOD_LOW              ( 0 << 3 )

/*! @brief BOD trigger level setting */
#define POWER_BOD_LVL_1_75V        9        /*!< Default at Reset , 1.7V on ES1 */
#define POWER_BOD_LVL_1_8V         10       /*!< BOD trigger level 1.8V */
#define POWER_BOD_LVL_1_9V         11       /*!< BOD trigger level 1.9V */
#define POWER_BOD_LVL_2_0V         12       /*!< BOD trigger level 2.0V */
#define POWER_BOD_LVL_2_1V         13       /*!< BOD trigger level 2.1V */
#define POWER_BOD_LVL_2_2V         14       /*!< BOD trigger level 2.2V */
#define POWER_BOD_LVL_2_3V         15       /*!< BOD trigger level 2.3V */
#define POWER_BOD_LVL_2_4V         16       /*!< BOD trigger level 2.4V */
#define POWER_BOD_LVL_2_5V         17       /*!< BOD trigger level 2.5V */
#define POWER_BOD_LVL_2_6V         18       /*!< BOD trigger level 2.6V */
#define POWER_BOD_LVL_2_7V         19       /*!< BOD trigger level 2.7V */
#define POWER_BOD_LVL_2_8V         20       /*!< BOD trigger level 2.8V */
#define POWER_BOD_LVL_2_9V         21       /*!< BOD trigger level 2.9V */
#define POWER_BOD_LVL_3_0V         22       /*!< BOD trigger level 3.0V */
#define POWER_BOD_LVL_3_1V         23       /*!< BOD trigger level 3.1V */
#define POWER_BOD_LVL_3_2V         24       /*!< BOD trigger level 3.2V */
#define POWER_BOD_LVL_3_3V         25       /*!< BOD trigger level 3.3V */

/*! @brief BOD Hysteresis control setting */
#define POWER_BOD_HYST_25MV        0        /*!< BOD Hysteresis control 25mV */
#define POWER_BOD_HYST_50MV        1        /*!< BOD Hysteresis control 50mV */
#define POWER_BOD_HYST_75MV        2        /*!< BOD Hysteresis control 75mV */
#define POWER_BOD_HYST_100MV       3        /*!< BOD Hysteresis control 100mV, default at Reset */

/**
 * @brief  SRAM banks definition list for retention in power down modes !
 */
#define        PM_CFG_SRAM_BANK_BIT_BASE   0
#define        PM_CFG_SRAM_BANK0_RET       (1<<0)   /*!< On ES1, this bank shall be kept in retention for Warmstart from power down */
#define        PM_CFG_SRAM_BANK1_RET       (1<<1)   /*!< Bank 1 shall be kept in retention */
#define        PM_CFG_SRAM_BANK2_RET       (1<<2)   /*!< Bank 2 shall be kept in retention */
#define        PM_CFG_SRAM_BANK3_RET       (1<<3)   /*!< Bank 3 shall be kept in retention */
#define        PM_CFG_SRAM_BANK4_RET       (1<<4)   /*!< Bank 4 shall be kept in retention */
#define        PM_CFG_SRAM_BANK5_RET       (1<<5)   /*!< Bank 5 shall be kept in retention */
#define        PM_CFG_SRAM_BANK6_RET       (1<<6)   /*!< Bank 6 shall be kept in retention */
#define        PM_CFG_SRAM_BANK7_RET       (1<<7)   /*!< On ES2, this bank shall be kept in retention for Warmstart */
#define        PM_CFG_SRAM_BANK8_RET       (1<<8)   /*!< Bank 8 shall be kept in retention */
#define        PM_CFG_SRAM_BANK9_RET       (1<<9)   /*!< Bank 9 shall be kept in retention */
#define        PM_CFG_SRAM_BANK10_RET      (1<<10)  /*!< Bank 10 shall be kept in retention */
#define        PM_CFG_SRAM_BANK11_RET      (1<<11)  /*!< Bank 11 shall be kept in retention */

#define        PM_CFG_SRAM_ALL_RETENTION   0xFFF    /*!< All banks shall be kept in retention */

#define        PM_CFG_RADIO_RET            (1<<13)
#define        PM_CFG_XTAL32M_AUTOSTART    (1<<14)
#define        PM_CFG_KEEP_AO_VOLTAGE      (1<<15)  /*!< keep the same voltage on the Always-on power domain - typical used with FRO32K to avoid timebase drift */

#define POWER_WAKEUPSRC_SYSTEM         LOWPOWER_WAKEUPSRCINT0_SYSTEM_IRQ         /*!< BOD, Watchdog Timer, Flash controller, [DEEP SLEEP] BODVBAT [POWER_DOWN]*/
#define POWER_WAKEUPSRC_DMA            LOWPOWER_WAKEUPSRCINT0_DMA_IRQ            /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_GINT           LOWPOWER_WAKEUPSRCINT0_GINT_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_IRBLASTER      LOWPOWER_WAKEUPSRCINT0_IRBLASTER_IRQ      /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PINT0          LOWPOWER_WAKEUPSRCINT0_PINT0_IRQ          /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PINT1          LOWPOWER_WAKEUPSRCINT0_PINT1_IRQ          /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PINT2          LOWPOWER_WAKEUPSRCINT0_PINT2_IRQ          /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PINT3          LOWPOWER_WAKEUPSRCINT0_PINT3_IRQ          /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_SPIFI          LOWPOWER_WAKEUPSRCINT0_SPIFI_IRQ          /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_TIMER0         LOWPOWER_WAKEUPSRCINT0_TIMER0_IRQ         /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_TIMER1         LOWPOWER_WAKEUPSRCINT0_TIMER1_IRQ         /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_USART0         LOWPOWER_WAKEUPSRCINT0_USART0_IRQ         /*!< [DEEP SLEEP, POWER DOWN] */
#define POWER_WAKEUPSRC_USART1         LOWPOWER_WAKEUPSRCINT0_USART1_IRQ         /*!< [DEEP SLEEP]  */
#define POWER_WAKEUPSRC_I2C0           LOWPOWER_WAKEUPSRCINT0_I2C0_IRQ           /*!< [DEEP SLEEP, POWER DOWN] */
#define POWER_WAKEUPSRC_I2C1           LOWPOWER_WAKEUPSRCINT0_I2C1_IRQ           /*!< [DEEP SLEEP]  */
#define POWER_WAKEUPSRC_SPI0           LOWPOWER_WAKEUPSRCINT0_SPI0_IRQ           /*!< [DEEP SLEEP, POWER DOWN] */
#define POWER_WAKEUPSRC_SPI1           LOWPOWER_WAKEUPSRCINT0_SPI1_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM0           LOWPOWER_WAKEUPSRCINT0_PWM0_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM1           LOWPOWER_WAKEUPSRCINT0_PWM1_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM2           LOWPOWER_WAKEUPSRCINT0_PWM2_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM3           LOWPOWER_WAKEUPSRCINT0_PWM3_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM4           LOWPOWER_WAKEUPSRCINT0_PWM4_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM5           LOWPOWER_WAKEUPSRCINT0_PWM5_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM6           LOWPOWER_WAKEUPSRCINT0_PWM6_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM7           LOWPOWER_WAKEUPSRCINT0_PWM7_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM8           LOWPOWER_WAKEUPSRCINT0_PWM8_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM9           LOWPOWER_WAKEUPSRCINT0_PWM9_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_PWM10          LOWPOWER_WAKEUPSRCINT0_PWM10_IR           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_I2C2           LOWPOWER_WAKEUPSRCINT0_I2C2_IRQ           /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_RTC            LOWPOWER_WAKEUPSRCINT0_RTC_IRQ            /*!< [DEEP SLEEP, POWER DOWN]  */
#define POWER_WAKEUPSRC_NFCTAG         LOWPOWER_WAKEUPSRCINT0_NFCTAG_IRQ         /*!< [DEEP SLEEP, POWER DOWN (ES2 Only), DEEP DOWN (ES2 only)]  */
#define POWER_WAKEUPSRC_MAILBOX        LOWPOWER_WAKEUPSRCINT0_MAILBOX_IRQ        /*!< Mailbox, Wake-up from DEEP SLEEP and POWER DOWN low power mode [DEEP SLEEP, POWER DOWN] */

#define POWER_WAKEUPSRC_ADC_SEQA       ((uint64_t)LOWPOWER_WAKEUPSRCINT1_ADC_SEQA_IRQ       << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_ADC_SEQB       ((uint64_t)LOWPOWER_WAKEUPSRCINT1_ADC_SEQB_IRQ       << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_ADC_THCMP_OVR  ((uint64_t)LOWPOWER_WAKEUPSRCINT1_ADC_THCMP_OVR_IRQ  << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_DMIC           ((uint64_t)LOWPOWER_WAKEUPSRCINT1_DMIC_IRQ           << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_HWVAD          ((uint64_t)LOWPOWER_WAKEUPSRCINT1_HWVAD_IRQ          << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_BLE_DP         ((uint64_t)LOWPOWER_WAKEUPSRCINT1_BLE_DP_IRQ         << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_BLE_DP0        ((uint64_t)LOWPOWER_WAKEUPSRCINT1_BLE_DP0_IRQ        << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_BLE_DP1        ((uint64_t)LOWPOWER_WAKEUPSRCINT1_BLE_DP1_IRQ        << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_BLE_DP2        ((uint64_t)LOWPOWER_WAKEUPSRCINT1_BLE_DP2_IRQ        << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_BLE_LL_ALL     ((uint64_t)LOWPOWER_WAKEUPSRCINT1_BLE_LL_ALL_IRQ     << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_ZIGBEE_MAC     ((uint64_t)LOWPOWER_WAKEUPSRCINT1_ZIGBEE_MAC_IRQ     << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_ZIGBEE_MODEM   ((uint64_t)LOWPOWER_WAKEUPSRCINT1_ZIGBEE_MODEM_IRQ   << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_RFP_TMU        ((uint64_t)LOWPOWER_WAKEUPSRCINT1_RFP_TMU_IRQ        << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_RFP_AGC        ((uint64_t)LOWPOWER_WAKEUPSRCINT1_RFP_AGC_IRQ        << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_ISO7816        ((uint64_t)LOWPOWER_WAKEUPSRCINT1_ISO7816_IRQ        << 32)  /*!< [DEEP SLEEP] */
#define POWER_WAKEUPSRC_ANA_COMP       ((uint64_t)LOWPOWER_WAKEUPSRCINT1_ANA_COMP_IRQ       << 32)  /*!< [DEEP SLEEP, POWER DOWN] */
#define POWER_WAKEUPSRC_WAKE_UP_TIMER0 ((uint64_t)LOWPOWER_WAKEUPSRCINT1_WAKE_UP_TIMER0_IRQ << 32)  /*!< [DEEP SLEEP, POWER DOWN] */
#define POWER_WAKEUPSRC_WAKE_UP_TIMER1 ((uint64_t)LOWPOWER_WAKEUPSRCINT1_WAKE_UP_TIMER1_IRQ << 32)  /*!< [DEEP SLEEP, POWER DOWN] */
#define POWER_WAKEUPSRC_BLE_WAKE_TIMER ((uint64_t)LOWPOWER_WAKEUPSRCINT1_BLE_WAKE_TIMER_IRQ << 32)  /*!< [DEEP SLEEP, POWER DOWN] */
#define POWER_WAKEUPSRC_BLE_OSC_EN     ((uint64_t)LOWPOWER_WAKEUPSRCINT1_BLE_OSC_EN_IRQ     << 32)  /*!< [DEEP SLEEP, POWER DOWN] */
#define POWER_WAKEUPSRC_IO             ((uint64_t)LOWPOWER_WAKEUPSRCINT1_IO_IRQ             << 32)  /*!< [POWER DOWN, DEEP DOWN]  */

/**
 * @brief  BOD config
 */
typedef struct {
    uint8_t bod_level;  /*!< BOD trigger level */
    uint8_t bod_hyst;   /*!< BOD Hysteresis control */
    uint8_t bod_cfg;    /*!< BOD config setting */
} pm_bod_cfg_t;

typedef uint64_t       pm_wake_source_t;

/**
 * @brief  PDRUNCFG bits offset
 */
typedef enum pd_bits
{
    kPDRUNCFG_PD_LDO_ADC_EN = 22,           /*!< Offset is 22, LDO ADC enabled */
    kPDRUNCFG_PD_BOD_MEM_EN = 23,           /*!< Offset is 23, BOD MEM enabled */
    kPDRUNCFG_PD_BOD_CORE_EN = 24,          /*!< Offset is 24, BOD CORE enabled */
    kPDRUNCFG_PD_FRO32K_EN       = 25,      /*!< Offset is 25, FRO32K enabled */
    kPDRUNCFG_PD_XTAL32K_EN      = 26,      /*!< Offset is 26, XTAL32K enabled */
    kPDRUNCFG_PD_BOD_ANA_COMP_EN = 27,      /*!< Offset is 27, Analog Comparator enabled */

    kPDRUNCFG_ForceUnsigned = 0x80000000U
} pd_bit_t;

/**
 * @brief  Power modes
 */
typedef enum {
    /* PM_DEEP_SLEEP, */
    PM_POWER_DOWN,  /*!< Power down mode */
    PM_DEEP_DOWN,   /*!< Deep power down mode */
} pm_power_mode_t ;

/**
 * @brief  Power config
 */
typedef struct {
    pm_wake_source_t pm_wakeup_src; /*!< Wakeup source select */
    uint32_t         pm_wakeup_io;  /*!< Wakeup IO */
    uint32_t         pm_config;     /*!< Power mode config */
} pm_power_config_t;

/**
 * @brief  Reset Cause definition
 */
typedef enum reset_cause
{
    RESET_UNDEFINED     = 0,
    RESET_POR           = (1 << 0), /*!< The last chip reset was caused by a Power On Reset. */
    RESET_EXT_PIN       = (1 << 1), /*!< The last chip reset was caused by a Pad Reset. */
    RESET_BOR           = (1 << 2), /*!< The last chip reset was caused by a Brown Out Detector. */
    RESET_SYS_REQ       = (1 << 3), /*!< The last chip reset was caused by a System Reset requested by the ARM CPU. */
    RESET_WDT           = (1 << 4), /*!< The last chip reset was caused by the Watchdog Timer. */
    RESET_WAKE_DEEP_PD  = (1 << 5), /*!< The last chip reset was caused by a Wake-up I/O (GPIO or internal NTAG FD INT). */
    RESET_WAKE_PD       = (1 << 6), /*!< The last CPU reset was caused by a Wake-up from Power down (many sources possible: timer, IO, ...). */
    RESET_SW_REQ        = (1 << 7)  /*!< The last chip reset was caused by a Software. ES2 Only */
} reset_cause_t;

/**
 * @brief  LDO voltage setting
 */
typedef enum {
    PM_LDO_VOLT_1_1V_DEFAULT,   /*!< LDO voltage 1.1V */
    PM_LDO_VOLT_1_0V,  /*!< not safe at system start/wakeup and CPU clock switch to higher frequency */
} pm_ldo_volt_t ;

/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*!
* @name Power Configuration
* @{
*/

/*!
 * @brief Initialize the sdk power drivers
 *
 * Optimize the LDO voltage for power saving
 * Initialize the power domains
 *
 * @return none
 */
void POWER_Init(void);

/*!
 * @brief  Optimize the LDO voltage for power saving
 * Initialize the power domains
 *
 * @return none
 */
void POWER_SetTrimDefaultActiveVoltage(void);

/*!
 * @brief BODMEM and BODCORE setup
 *
 * Enable the BOD core and BOD mem
 * Disable the analog comnparator clock
 *
 * @return none
 */
void POWER_BodSetUp(void);

/*!
 * @brief enable SW reset for the BODCORE
 *
 * @return none
 */
void POWER_BodActivate(void);

/*!
 * @brief API to enable PDRUNCFG bit in the Syscon. Note that enabling the bit powers down the peripheral
 *
 * @param en    peripheral for which to enable the PDRUNCFG bit
 *
 * @return none
 */
static inline void POWER_EnablePD(pd_bit_t en)
{
    /* PDRUNCFGSET */
    PMC->PDRUNCFG |= (1UL << (en & 0xffU));
}

/*!
 * @brief API to disable PDRUNCFG bit in the Syscon. Note that disabling the bit powers up the peripheral
 *
 * @param en    peripheral for which to disable the PDRUNCFG bit
 *
 * @return none
 */
static inline void POWER_DisablePD(pd_bit_t en)
{
    /* PDRUNCFGCLR */
    PMC->PDRUNCFG &= ~(1UL << (en & 0xffU));
}

/*!
 *  @brief Get IO and Ntag Field detect Wake-up sources from Power Down and Deep Power Down modes.
 *  Allow to identify the wake-up source when waking up from Power-Down modes or Deep Power Down modes.
 *  Status is reset by POR, RSTN, WDT.
 *  bit in range from 0 to 21 are for DIO0 to DIO21
 *  bit 22 is NTAG field detect wakeup source
 *
 *  @return IO and Field detect Wake-up source
 */
static inline uint32_t POWER_GetIoWakeStatus(void)
{
    return PMC->WAKEIOCAUSE;
}

/*!
 * @brief Power API to enter sleep mode (Doze mode)
 *
 * @note: The static inline function has not the expecetd effect in -O0. If order to force inline this macro is added
 *
 * @return none
 */
#define POWER_ENTER_SLEEP()  __DSB(); __WFI(); __ISB();

/*!
 * @brief Power API to enter sleep mode (Doze mode)
 *
 * @note: If the user desires to program a wakeup timer before going to sleep, it needs to use
 * either the fsl_wtimer.h API or use the POWER_SetLowPower() API instead
 * see POWER_ENTER_SLEEP
 *
 * @return none
 */
static inline void POWER_EnterSleep(void)
{
    POWER_ENTER_SLEEP();
}

/*!
 * @brief Power Library API to enter different power mode.
 * If requested mode is PM_POWER_DOWN, the API will perform the clamping of the DIOs
 * if the PIO register has the bit IO_CLAMPING set: SYSCON->RETENTIONCTRL.IOCLAMP
 * will be set
 *
 * @param pm_power_mode  Power modes
 * @see pm_power_mode_t
 * @param pm_power_config Power config
 * @see pm_power_config_t
 *
 * @return false if chip could not go to sleep. Configuration structure is incorrect
 */
bool POWER_EnterPowerMode(pm_power_mode_t pm_power_mode, pm_power_config_t* pm_power_config);

/*!
 * @brief determine cause of reset
 *
 * @return reset_cause
 */
reset_cause_t POWER_GetResetCause(void);

/*!
 * @brief Clear cause of reset
 */
void POWER_ClearResetCause(void);

/*!
 * @brief Power Library API to return the library version.
 *
 * @param none
 *
 * @return version number of the power library
 */
uint32_t POWER_GetLibVersion(void);

/*!
 * @brief Get default Vbat BOD config parameters, level @1.75V, Hysteresis @ 100mV
 *
 * @param bod_cfg_p BOD config
 * @see pm_bod_cfg_t
 *
 * @return none
 */
void POWER_BodVbatGetDefaultConfig(pm_bod_cfg_t * bod_cfg_p);

/*!
 * @brief Configure the VBAT BOD
 *
 * @param bod_cfg_p BOD config
 * @see pm_bod_cfg_t
 *
 * @return false if configuration parameters are incorrect
 */
bool POWER_BodVbatConfig(pm_bod_cfg_t * bod_cfg_p);

/*!
 * @brief Configure the LDO voltage
 *
 * @param ldoVolt LDO voltage setting
 * @see pm_ldo_volt_t
 *
 * @return none
 */
void POWER_ApplyLdoActiveVoltage(pm_ldo_volt_t ldoVolt);

/* @} */

#ifdef __cplusplus
}
#endif

/*! @} */

#endif /* _FSL_POWER_H_ */
