/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_RESET_H_
#define _FSL_RESET_H_

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "fsl_device_registers.h"

/*!
 * @addtogroup ksdk_common
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief RESET driver version 2.0.1. */
#define FSL_RESET_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*!
 * @brief Enumeration for peripheral reset control bits
 *
 * Defines the enumeration for peripheral reset control bits in PRESETCTRL/ASYNCPRESETCTRL registers
 */
typedef enum _SYSCON_RSTn
{
    kSPIFI_RST_SHIFT_RSTn  = (0 | SYSCON_PRESETCTRL0_SPIFI_RST_SHIFT),          /**< SpiFi reset control   */
    kMUX_RST_SHIFT_RSTn    = (0 | SYSCON_PRESETCTRL0_MUX_RST_SHIFT),            /**< Input mux reset control */
    kBLE_TG_RST_SHIFT_RSTn = (0 | SYSCON_PRESETCTRL0_BLE_TIMING_GEN_RST_SHIFT), /**< BLE power module reset control   */
    kIOCON_RST_SHIFT_RSTn  = (0 | SYSCON_PRESETCTRL0_IOCON_RST_SHIFT),          /**< IOCON reset control */
    kGPIO0_RST_SHIFT_RSTn  = (0 | SYSCON_PRESETCTRL0_GPIO_RST_SHIFT),           /**< GPIO0 reset control */
    kPINT_RST_SHIFT_RSTn   = (0 | SYSCON_PRESETCTRL0_PINT_RST_SHIFT), /**< Pin interrupt (PINT) reset control */
    kGINT_RST_SHIFT_RSTn   = (0 | SYSCON_PRESETCTRL0_GINT_RST_SHIFT), /**< Grouped interrupt (PINT) reset control. */
    kDMA_RST_SHIFT_RSTn    = (0 | SYSCON_PRESETCTRL0_DMA_RST_SHIFT),  /**< DMA reset control */
    kWWDT_RST_SHIFT_RSTn   = (0 | SYSCON_PRESETCTRL0_WWDT_RST_SHIFT), /**< Watchdog timer reset control */
    kRTC_RST_SHIFT_RSTn    = (0 | SYSCON_PRESETCTRL0_RTC_RST_SHIFT),  /**< RTC reset control */
    kANA_INT_RST_SHIFT_RSTn = (0 | SYSCON_PRESETCTRL0_ANA_INT_CTRL_RST_SHIFT), /**< Analog interrupt controller reset */
    kWKT_RST_SHIFT_RSTn     = (0 | SYSCON_PRESETCTRL0_WAKE_UP_TIMERS_RST_SHIFT), /**< Wakeup timer reset */
    kADC0_RST_SHIFT_RSTn    = (0 | SYSCON_PRESETCTRL0_ADC_RST_SHIFT),            /**< ADC0 reset control */
    kFC0_RST_SHIFT_RSTn =
        ((1UL << 16) | SYSCON_PRESETCTRL1_USART0_RST_SHIFT), /**< Flexcomm Interface 0 reset control */
    kFC1_RST_SHIFT_RSTn =
        ((1UL << 16) | SYSCON_PRESETCTRL1_USART1_RST_SHIFT),                 /**< Flexcomm Interface 1 reset control */
    kFC2_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_I2C0_RST_SHIFT), /**< Flexcomm Interface 2 reset control */
    kFC3_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_I2C1_RST_SHIFT), /**< Flexcomm Interface 3 reset control */
    kFC4_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_SPI0_RST_SHIFT), /**< Flexcomm Interface 4 reset control */
    kFC5_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_SPI1_RST_SHIFT), /**< Flexcomm Interface 5 reset control */
    kIRB_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_IR_RST_SHIFT),   /**< IR Blaster reset control */
    kPWM_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_PWM_RST_SHIFT),  /**< PWM reset control */
    kRNG_RST_SHIFT_RSTn =
        ((1UL << 16) | SYSCON_PRESETCTRL1_RNG_RST_SHIFT), /**< Random number generator reset control */
    kFC6_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_I2C2_RST_SHIFT), /**< Flexcomm Interface 6 reset control */
    kUSART0_RST_SHIFT_RSTn = kFC0_RST_SHIFT_RSTn,                            /**< USART0 reset control == Flexcomm0 */
    kUSART1_RST_SHIFT_RSTn = kFC1_RST_SHIFT_RSTn,                            /**< USART0 reset control == Flexcomm1 */
    kI2C0_RST_SHIFT_RSTn   = kFC2_RST_SHIFT_RSTn,                            /**< I2C0 reset control == Flexcomm 2  */
    kI2C1_RST_SHIFT_RSTn   = kFC3_RST_SHIFT_RSTn,                            /**< I2C1 reset control == Flexcomm 3  */
    kSPI0_RST_SHIFT_RSTn   = kFC4_RST_SHIFT_RSTn,                            /**< SPI0 reset control == Flexcomm 4  */
    kSPI1_RST_SHIFT_RSTn   = kFC5_RST_SHIFT_RSTn,                            /**< SPI1 reset control == Flexcomm 5  */
    kI2C2_RST_SHIFT_RSTn   = kFC6_RST_SHIFT_RSTn,                            /**< I2C2 reset control == Flexcomm 6  */
    kBLE_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_BLE_RST_SHIFT),  /**< Bluetooth LE modules reset control */
    kMODEM_MASTER_SHIFT_RSTn =
        ((1UL << 16) | SYSCON_PRESETCTRL1_MODEM_MASTER_RST_SHIFT),          /**< AHB Modem master interface reset */
    kAES_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_AES_RST_SHIFT), /**< Encryption module reset control */
    kRFP_RST_SHIFT_RSTn = ((1UL << 16) | SYSCON_PRESETCTRL1_RFP_RST_SHIFT), /**< Radio front end controller reset */
    kDMIC_RST_SHIFT_RSTn =
        ((1UL << 16) | SYSCON_PRESETCTRL1_DMIC_RST_SHIFT), /**< Digital microphone interface reset control */
    kHASH_RST_SHIFT_RSTn    = ((1UL << 16) | SYSCON_PRESETCTRL1_HASH_RST_SHIFT),         /**< Hash SHA reset */
    kCTIMER0_RST_SHIFT_RSTn = ((2UL << 16) | ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B0_SHIFT), /**< CT32B0 reset control */
    kCTIMER1_RST_SHIFT_RSTn = ((2UL << 16) | ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B1_SHIFT), /**< CT32B1 reset control */
} SYSCON_RSTn_t;

/** Array initializers with peripheral reset bits **/
#define ADC_RSTS             \
    {                        \
        kADC0_RST_SHIFT_RSTn \
    } /* Reset bits for ADC peripheral */
#define AES_RSTS             \
    {                        \
        kAES_RST_SHIFT_RSTbn \
    } /* Reset bits for Encryption peripheral */
#define ANA_INT_RSTS            \
    {                           \
        kANA_INT_RST_SHIFT_RSTn \
    } /* Reset bits for Analog interrupts controller */
#define BLE_RSTS            \
    {                       \
        kBLE_RST_SHIFT_RSTn \
    } /* Reset bits for Bluetooth LE peripheral */
#define BLE_TG_RSTS            \
    {                          \
        kBLE_TG_RST_SHIFT_RSTn \
    } /* Bluetooth LE power module reset */
#define CRC_RSTS            \
    {                       \
        kCRC_RST_SHIFT_RSTn \
    } /* Reset bits for CRC peripheral */
#define CTIMER_RSTS                                      \
    {                                                    \
        kCTIMER0_RST_SHIFT_RSTn, kCTIMER1_RST_SHIFT_RSTn \
    } /* Reset bits for TIMER peripheral */
#define DMA_RSTS_N          \
    {                       \
        kDMA_RST_SHIFT_RSTn \
    } /* Reset bits for DMA peripheral */
#define DMIC_RSTS            \
    {                        \
        kDMIC_RST_SHIFT_RSTn \
    } /* Reset bits for ADC peripheral */
#define EFUSE_RSTS            \
    {                         \
        kEFUSE_RST_SHIFT_RSTn \
    } /* Reset bits for EFuse peripheral */
#define FLASH_RSTS            \
    {                         \
        kFLASH_RST_SHIFT_RSTn \
    } /* Reset bits for flash controller */
#define FLEXCOMM_RSTS                                                                                            \
    {                                                                                                            \
        kFC0_RST_SHIFT_RSTn, kFC1_RST_SHIFT_RSTn, kFC2_RST_SHIFT_RSTn, kFC3_RST_SHIFT_RSTn, kFC4_RST_SHIFT_RSTn, \
            kFC5_RST_SHIFT_RSTn, kFC6_RST_SHIFT_RSTn                                                             \
    } /* Reset bits for FLEXCOMM peripheral */
#define GINT_RSTS            \
    {                        \
        kGINT_RST_SHIFT_RSTn \
    } /* Reset bits for GINT peripheral. GINT0 & GINT1 share same slot */
#define GPIO_RSTS_N           \
    {                         \
        kGPIO0_RST_SHIFT_RSTn \
    } /* Reset bits for GPIO peripheral */
#define INPUTMUX_RSTS       \
    {                       \
        kMUX_RST_SHIFT_RSTn \
    } /* Reset bits for INPUTMUX peripheral */
#define IOCON_RSTS            \
    {                         \
        kIOCON_RST_SHIFT_RSTn \
    } /* Reset bits for IOCON peripheral */
#define ZIGBEE_RSTS            \
    {                          \
        kZIGBEE_RST_SHIFT_RSTn \
    } /* Reset bits for RF/Zigbee peripheral */
#define MAILBOX_RSTS            \
    {                           \
        kMAILBOX_RST_SHIFT_RSTn \
    } /* Reset bits for inter-CPU mailbox peripheral */
#define MODEM_RSTS               \
    {                            \
        kMODEM_MASTER_SHIFT_RSTn \
    } /* Reset bits for AHB Modem master interface peripheral */
#define MRT_RSTS            \
    {                       \
        kMRT_RST_SHIFT_RSTn \
    } /* Reset bits for MRT peripheral */
#define PINT_RSTS            \
    {                        \
        kPINT_RST_SHIFT_RSTn \
    } /* Reset bits for PINT peripheral */
#define PVT_RSTS            \
    {                       \
        kPVT_RST_SHIFT_RSTn \
    } /* Reset bits for PVT peripheral */
#define RTC_RSTS            \
    {                       \
        kRTC_RST_SHIFT_RSTn \
    } /* Reset bits for RTC peripheral */
#define SPIFI_RSTS            \
    {                         \
        kSPIFI_RST_SHIFT_RSTn \
    } /* Reset bits for SPIFI peripheral */
#define TPR_RSTS            \
    {                       \
        kTPR_RST_SHIFT_RSTn \
    } /* Reset bits for test pointer register peripheral */
#define WWDT_RSTS            \
    {                        \
        kWWDT_RST_SHIFT_RSTn \
    } /* Reset bits for windowed watchdog timer */

#define WWDT_RSTS            \
    {                        \
        kWWDT_RST_SHIFT_RSTn \
    } /* Reset bits for WWDT peripheral */

typedef SYSCON_RSTn_t reset_ip_name_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Assert reset to peripheral.
 *
 * Asserts reset signal to specified peripheral module.
 *
 * @param peripheral Assert reset to this peripheral. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_SetPeripheralReset(reset_ip_name_t peripheral);

/*!
 * @brief Clear reset to peripheral.
 *
 * Clears reset signal to specified peripheral module, allows it to operate.
 *
 * @param peripheral Clear reset to this peripheral. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_ClearPeripheralReset(reset_ip_name_t peripheral);

/*!
 * @brief Reset peripheral module.
 *
 * Reset peripheral module.
 *
 * @param peripheral Peripheral to reset. The enum argument contains encoding of reset register
 *                   and reset bit position in the reset register.
 */
void RESET_PeripheralReset(reset_ip_name_t peripheral);

/*!
 * @brief Reset the chip.
 *
 * Full software reset of the chip.
 * On reboot, function POWER_GetResetCause() from fsl_power.h will return RESET_SYS_REQ
 */
void RESET_SystemReset(void);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FSL_RESET_H_ */
