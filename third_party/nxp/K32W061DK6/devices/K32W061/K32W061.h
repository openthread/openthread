/*
** ###################################################################
**     Processors:          K32W041HN
**                          K32W061HN
**
**     Compilers:           GNU C Compiler
**                          IAR ANSI C/C++ Compiler for ARM
**                          Keil ARM C/C++ Compiler
**                          MCUXpresso Compiler
**
**     Reference manual:    K32W061UM_Rev.0.3 20 December 2019
**     Version:             rev. 1.0, 2019-11-05
**     Build:               b200228
**
**     Abstract:
**         CMSIS Peripheral Access Layer for K32W061
**
**     Copyright 1997-2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2020 NXP
**     All rights reserved.
**
**     SPDX-License-Identifier: BSD-3-Clause
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
**     Revisions:
**     - rev. 1.0 (2019-11-05)
**         Initial version.
**
** ###################################################################
*/

/*!
 * @file K32W061.h
 * @version 1.0
 * @date 2019-11-05
 * @brief CMSIS Peripheral Access Layer for K32W061
 *
 * CMSIS Peripheral Access Layer for K32W061
 */

#ifndef _K32W061_H_
#define _K32W061_H_                              /**< Symbol preventing repeated inclusion */

/** Memory map major version (memory maps with equal major version number are
 * compatible) */
#define MCU_MEM_MAP_VERSION 0x0100U
/** Memory map minor version */
#define MCU_MEM_MAP_VERSION_MINOR 0x0000U


/* ----------------------------------------------------------------------------
   -- Interrupt vector numbers
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Interrupt_vector_numbers Interrupt vector numbers
 * @{
 */

/** Interrupt Number Definitions */
#define NUMBER_OF_INT_VECTORS 72                 /**< Number of interrupts in the Vector table */

typedef enum IRQn {
  /* Auxiliary constants */
  NotAvail_IRQn                = -128,             /**< Not available device specific interrupt */

  /* Core interrupts */
  NonMaskableInt_IRQn          = -14,              /**< Non Maskable Interrupt */
  HardFault_IRQn               = -13,              /**< Cortex-M4 SV Hard Fault Interrupt */
  MemoryManagement_IRQn        = -12,              /**< Cortex-M4 Memory Management Interrupt */
  BusFault_IRQn                = -11,              /**< Cortex-M4 Bus Fault Interrupt */
  UsageFault_IRQn              = -10,              /**< Cortex-M4 Usage Fault Interrupt */
  SVCall_IRQn                  = -5,               /**< Cortex-M4 SV Call Interrupt */
  DebugMonitor_IRQn            = -4,               /**< Cortex-M4 Debug Monitor Interrupt */
  PendSV_IRQn                  = -2,               /**< Cortex-M4 Pend SV Interrupt */
  SysTick_IRQn                 = -1,               /**< Cortex-M4 System Tick Interrupt */

  /* Device specific interrupts */
  WDT_BOD_IRQn                 = 0,                /**< System (BOD, Watchdog Timer, Flash controller) interrupt */
  DMA0_IRQn                    = 1,                /**< DMA interrupt */
  GINT0_IRQn                   = 2,                /**< GPIO global interrupt */
  CIC_IRB_IRQn                 = 3,                /**< Infra Red Blaster interrupt */
  PIN_INT0_IRQn                = 4,                /**< Pin Interrupt and Pattern matching 0 */
  PIN_INT1_IRQn                = 5,                /**< Pin Interrupt and Pattern matching 1 */
  PIN_INT2_IRQn                = 6,                /**< Pin Interrupt and Pattern matching 2 */
  PIN_INT3_IRQn                = 7,                /**< Pin Interrupt and Pattern matching 3 */
  SPIFI0_IRQn                  = 8,                /**< Quad-SPI flash interface interrupt */
  CTIMER0_IRQn                 = 9,                /**< Counter/Timer 0 interrupt */
  CTIMER1_IRQn                 = 10,               /**< Counter/Timer 1 interrupt */
  FLEXCOMM0_IRQn               = 11,               /**< Flexcomm Interface 0 (USART0, FLEXCOMM0) */
  FLEXCOMM1_IRQn               = 12,               /**< Flexcomm Interface 1 (USART1, FLEXCOMM1) */
  FLEXCOMM2_IRQn               = 13,               /**< Flexcomm Interface 2 (I2C0, FLEXCOMM2) */
  FLEXCOMM3_IRQn               = 14,               /**< Flexcomm Interface 3 (I2C1, FLEXCOMM3) */
  FLEXCOMM4_IRQn               = 15,               /**< Flexcomm Interface 4 (SPI0, FLEXCOMM4) */
  FLEXCOMM5_IRQn               = 16,               /**< Flexcomm Interface 5 (SPI5, FLEXCOMM) */
  PWM0_IRQn                    = 17,               /**< PWM channel 0 interrupt */
  PWM1_IRQn                    = 18,               /**< PWM channel 1 interrupt */
  PWM2_IRQn                    = 19,               /**< PWM channel 2 interrupt */
  PWM3_IRQn                    = 20,               /**< PWM channel 3 interrupt */
  PWM4_IRQn                    = 21,               /**< PWM channel 4 interrupt */
  PWM5_IRQn                    = 22,               /**< PWM channel 5 interrupt */
  PWM6_IRQn                    = 23,               /**< PWM channel 6  interrupt */
  PWM7_IRQn                    = 24,               /**< PWM channel 7 interrupt */
  PWM8_IRQn                    = 25,               /**< PWM channel 8 interrupt */
  PWM9_IRQn                    = 26,               /**< PWM channel 9 interrupt */
  PWM10_IRQn                   = 27,               /**< PWM channel 10 interrupt */
  FLEXCOMM6_IRQn               = 28,               /**< Flexcomm Interface6 (I2C2, FLEXCOMM6) */
  RTC_IRQn                     = 29,               /**< Real Time Clock interrupt */
  NFCTag_IRQn                  = 30,               /**< NFC Tag interrupt */
  MAILBOX_IRQn                 = 31,               /**< Mailbox interrupts, Wake-up from Deep Sleep interrupt */
  ADC0_SEQA_IRQn               = 32,               /**< ADC Sequence A interrupt */
  ADC0_SEQB_IRQn               = 33,               /**< ADC Sequence B interrupt */
  ADC0_THCMP_IRQn              = 34,               /**< ADC Threshold compare and overrun interrupt */
  DMIC0_IRQn                   = 35,               /**< DMIC interrupt */
  HWVAD0_IRQn                  = 36,               /**< Hardware Voice activity detection interrupt */
  BLE_DP_IRQn                  = 37,               /**< BLE Data Path interrupt */
  BLE_DP0_IRQn                 = 38,               /**< BLE Data Path interrupt 0 */
  BLE_DP1_IRQn                 = 39,               /**< BLE Data Path interrupt 1 */
  BLE_DP2_IRQn                 = 40,               /**< BLE Data Path interrupt 2 */
  BLE_LL_ALL_IRQn              = 41,               /**< All BLE link layer interrupts */
  ZIGBEE_MAC_IRQn              = 42,               /**< Zigbee MAC interrupt */
  ZIGBEE_MODEM_IRQn            = 43,               /**< Zigbee MoDem interrupt */
  RFP_TMU_IRQn                 = 44,               /**< RFP Timing Managemnt Unit (TMU) interrupt */
  RFP_AGC_IRQn                 = 45,               /**< RFP AGC interrupt */
  ISO7816_IRQn                 = 46,               /**< ISO7816 controller interrupt */
  ANA_COMP_IRQn                = 47,               /**< Analog Comparator interrupt */
  WAKE_UP_TIMER0_IRQn          = 48,               /**< Wake up Timer 0 interrupt */
  WAKE_UP_TIMER1_IRQn          = 49,               /**< Wake up Timer 1 interrupt */
  PVTVF0_AMBER_IRQn            = 50,               /**< PVT Monitor interrupt */
  PVTVF0_RED_IRQn              = 51,               /**< PVT Monitor interrupt */
  PVTVF1_AMBER_IRQn            = 52,               /**< PVT Monitor interrupt */
  PVTVF1_RED_IRQn              = 53,               /**< PVT Monitor interrupt */
  BLE_WAKE_UP_TIMER_IRQn       = 54,               /**< BLE Wake up Timer interrupt */
  SHA_IRQn                     = 55                /**< SHA interrupt */
} IRQn_Type;

/*!
 * @}
 */ /* end of group Interrupt_vector_numbers */


/* ----------------------------------------------------------------------------
   -- Cortex M4 Core Configuration
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Cortex_Core_Configuration Cortex M4 Core Configuration
 * @{
 */

#define __MPU_PRESENT                  1         /**< Defines if an MPU is present or not */
#define __NVIC_PRIO_BITS               3         /**< Number of priority bits implemented in the NVIC */
#define __Vendor_SysTickConfig         0         /**< Vendor specific implementation of SysTickConfig is defined */
#define __FPU_PRESENT                  0         /**< Defines if an FPU is present or not */

#include "core_cm4.h"                  /* Core Peripheral Access Layer */
#include "system_K32W061.h"            /* Device specific configuration file */

/*!
 * @}
 */ /* end of group Cortex_Core_Configuration */


/* ----------------------------------------------------------------------------
   -- Mapping Information
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Mapping_Information Mapping Information
 * @{
 */

/** Mapping Information */
/*!
 * @addtogroup dma_request
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief Structure for the DMA hardware request
 *
 * Defines the structure for the DMA hardware request collections. The user can configure the
 * hardware request to trigger the DMA transfer accordingly. The index
 * of the hardware request varies according  to the to SoC.
 */
typedef enum _dma_request_source
{
    kDmaRequestUsart0Rx             = 0U,          /**< USART 0 RX */
    kDmaRequestUsart0Tx             = 1U,          /**< USART 0 TX */
    kDmaRequestUsart1Rx             = 2U,          /**< USART 1 RX */
    kDmaRequestUsart1Tx             = 3U,          /**< USART 1 TX */
    kDmaRequestI2c0Slave            = 4U,          /**< I2C 0 Slave */
    kDmaRequestI2c0Master           = 5U,          /**< I2C 0 Master */
    kDmaRequestI2c1Slave            = 6U,          /**< I2C 1 Slave */
    kDmaRequestI2c1Master           = 7U,          /**< I2C 1 Master */
    kDmaRequestSpi0Rx               = 8U,          /**< SPI 0 RX */
    kDmaRequestSpi0Tx               = 9U,          /**< SPI 0 TX */
    kDmaRequestSpi1Rx               = 10U,         /**< SPI 1 RX */
    kDmaRequestSpi1Tx               = 11U,         /**< SPI 1 TX */
    kDmaRequestSPIFI                = 12U,         /**< SPIFI */
    kDmaRequestI2c2Slave            = 13U,         /**< I2C 2 Slave */
    kDmaRequestI2c2Master           = 14U,         /**< I2C 2 Master */
    kDmaRequestDMIC0                = 15U,         /**< DMIC Channel 0 */
    kDmaRequestDMIC1                = 16U,         /**< DMIC Channel 1 */
    kDmaRequestHashRx               = 17U,         /**< Hash RX */
    kDmaRequestHashTx               = 18U,         /**< Hash TX */
} dma_request_source_t;

/* @} */


/*!
 * @}
 */ /* end of group Mapping_Information */


/* ----------------------------------------------------------------------------
   -- Device Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Peripheral_access_layer Device Peripheral Access Layer
 * @{
 */


/*
** Start of section using anonymous unions
*/

#if defined(__ARMCC_VERSION)
  #if (__ARMCC_VERSION >= 6010050)
    #pragma clang diagnostic push
  #else
    #pragma push
    #pragma anon_unions
  #endif
#elif defined(__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined(__IAR_SYSTEMS_ICC__)
  #pragma language=extended
#else
  #error Not supported compiler type
#endif

/* ----------------------------------------------------------------------------
   -- ADC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ADC_Peripheral_Access_Layer ADC Peripheral Access Layer
 * @{
 */

/** ADC - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< ADC Control register. Contains the clock divide value, resolution selection, sampling time selection, and mode controls., offset: 0x0 */
       uint8_t RESERVED_0[4];
  __IO uint32_t SEQ_CTRL[1];                       /**< ADC Conversion Sequence-A control register: Controls triggering and channel selection for conversion sequence-A. Also specifies interrupt mode for sequence-A., array offset: 0x8, array step: 0x4 */
       uint8_t RESERVED_1[4];
  __I  uint32_t SEQ_GDAT[1];                       /**< ADC Sequence-A Global Data register. This register contains the result of the most recent ADC conversion performed under sequence-A., array offset: 0x10, array step: 0x4 */
       uint8_t RESERVED_2[12];
  __I  uint32_t DAT[8];                            /**< ADC Channel X [0:7] Data register. This register contains the result of the most recent conversion completed on channel X [0:7] ., array offset: 0x20, array step: 0x4 */
       uint8_t RESERVED_3[16];
  __IO uint32_t THR0_LOW;                          /**< ADC Low Compare Threshold register 0: Contains the lower threshold level for automatic threshold comparison for any channels linked to threshold pair 0., offset: 0x50 */
  __IO uint32_t THR1_LOW;                          /**< ADC Low Compare Threshold register 1: Contains the lower threshold level for automatic threshold comparison for any channels linked to threshold pair 1., offset: 0x54 */
  __IO uint32_t THR0_HIGH;                         /**< ADC High Compare Threshold register 0: Contains the upper threshold level for automatic threshold comparison for any channels linked to threshold pair 0., offset: 0x58 */
  __IO uint32_t THR1_HIGH;                         /**< ADC High Compare Threshold register 1: Contains the upper threshold level for automatic threshold comparison for any channels linked to threshold pair 1., offset: 0x5C */
  __IO uint32_t CHAN_THRSEL;                       /**< ADC Channel-Threshold Select register. Specifies which set of threshold compare registers are to be used for each channel, offset: 0x60 */
  __IO uint32_t INTEN;                             /**< ADC Interrupt Enable register. This register contains enable bits that enable the sequence-A, sequence-B, threshold compare and data overrun interrupts to be generated., offset: 0x64 */
  __IO uint32_t FLAGS;                             /**< ADC Flags register. Contains the four interrupt/DMA trigger flags and the individual component overrun and threshold-compare flags. (The overrun bits replicate information stored in the result registers)., offset: 0x68 */
  __IO uint32_t STARTUP;                           /**< ADC Startup register (typically only used by the ADC API)., offset: 0x6C */
  __IO uint32_t GPADC_CTRL0;                       /**< Second ADC Control register : ADC internal LDO (within ADC sub-system), offset: 0x70 */
  __IO uint32_t GPADC_CTRL1;                       /**< Third ADC Control register : ADC internal gain and offset, offset: 0x74 */
} ADC_Type;

/* ----------------------------------------------------------------------------
   -- ADC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ADC_Register_Masks ADC Register Masks
 * @{
 */

/*! @name CTRL - ADC Control register. Contains the clock divide value, resolution selection, sampling time selection, and mode controls. */
/*! @{ */
#define ADC_CTRL_CLKDIV_MASK                     (0xFFU)
#define ADC_CTRL_CLKDIV_SHIFT                    (0U)
/*! CLKDIV - Reserved. No changes to this fiedl are necessary.
 */
#define ADC_CTRL_CLKDIV(x)                       (((uint32_t)(((uint32_t)(x)) << ADC_CTRL_CLKDIV_SHIFT)) & ADC_CTRL_CLKDIV_MASK)
#define ADC_CTRL_ASYNMODE_MASK                   (0x100U)
#define ADC_CTRL_ASYNMODE_SHIFT                  (8U)
/*! ASYNMODE - Select clock mode. 0: Synchronous mode. Not Supported. 1: Asynchronous mode. The ADC
 *    clock is based on the output of the ADC clock divider ADCCLKSEL in the SYSCON block.
 */
#define ADC_CTRL_ASYNMODE(x)                     (((uint32_t)(((uint32_t)(x)) << ADC_CTRL_ASYNMODE_SHIFT)) & ADC_CTRL_ASYNMODE_MASK)
#define ADC_CTRL_RESOL_MASK                      (0x600U)
#define ADC_CTRL_RESOL_SHIFT                     (9U)
/*! RESOL - The number of bits of ADC resolution. Note, whatever the resolution setting, the ADC
 *    data will always be shifted to use the MSBs of any ADC data words. Accuracy can be reduced to
 *    achieve higher conversion rates. A single conversion (including one conversion in a burst or
 *    sequence) requires the selected number of bits of resolution plus 3 ADC clocks. Remark: This field
 *    must only be altered when the ADC is fully idle. Changing it during any kind of ADC operation
 *    may have unpredictable results. Remark: ADC clock frequencies for various resolutions must
 *    not exceed: - 5x the system clock rate for 12-bit resolution. - 4.3x the system clock rate for
 *    10-bit resolution. - 3.6x the system clock for 8-bit resolution. - 3x the bus clock rate for
 *    6-bit resolution. Settings: 0x3: 6-bit resolution. An ADC conversion requires 9 ADC clocks, plus
 *    any clocks specified by the TSAMP field; 0x2: 8-bit resolution. An ADC conversion requires 11
 *    ADC clocks, plus any clocks specified by the TSAMP field; 0x1:10-bit resolution. An ADC
 *    conversion requires 13 ADC clocks, plus any clocks specified by the TSAMP field; 0x0: 12-bit
 *    resolution. An ADC conversion requires 15 ADC clocks, plus any clocks specified by the TSAMP field.
 */
#define ADC_CTRL_RESOL(x)                        (((uint32_t)(((uint32_t)(x)) << ADC_CTRL_RESOL_SHIFT)) & ADC_CTRL_RESOL_MASK)
#define ADC_CTRL_RESOL_MASK_DIS_MASK             (0x800U)
#define ADC_CTRL_RESOL_MASK_DIS_SHIFT            (11U)
/*! RESOL_MASK_DIS - According RESOL bit LSB bits are automatickly masked if RESOL_MASK_DIS = 0. If
 *    RESOL_MASK_DIS = 1, the 12bits comming from ADC are directly connect to register RESULT
 */
#define ADC_CTRL_RESOL_MASK_DIS(x)               (((uint32_t)(((uint32_t)(x)) << ADC_CTRL_RESOL_MASK_DIS_SHIFT)) & ADC_CTRL_RESOL_MASK_DIS_MASK)
#define ADC_CTRL_TSAMP_MASK                      (0x7000U)
#define ADC_CTRL_TSAMP_SHIFT                     (12U)
/*! TSAMP - Sample Time. The default sampling period (TSAMP = 000 ) at the start of each conversion
 *    is 2.5 ADC clock periods. Depending on a variety of factors, including operating conditions
 *    and the output impedance of the analog source, longer sampling times may be required. The TSAMP
 *    field should stay to default during application The TSAMP field specifies the number of
 *    additional ADC clock cycles, from zero to seven, by which the sample period will be extended. The
 *    total conversion time will increase by the same number of clocks. 000 - The sample period will
 *    be the default 2.5 ADC clocks. A complete conversion with 12-bits of accuracy will require 15
 *    clocks. 001- The sample period will be extended by one ADC clock to a total of 3.5 clock
 *    periods. A complete 12-bit conversion will require 16 clocks. 010 - The sample period will be
 *    extended by two clocks to 4.5 ADC clock cycles. A complete 12-bit conversion will require 17 ADC
 *    clocks. ... 111 - The sample period will be extended by seven clocks to 9.5 ADC clock cycles. A
 *    complete 12-bit conversion will require 22 ADC clocks.
 */
#define ADC_CTRL_TSAMP(x)                        (((uint32_t)(((uint32_t)(x)) << ADC_CTRL_TSAMP_SHIFT)) & ADC_CTRL_TSAMP_MASK)
/*! @} */

/*! @name SEQ_CTRL - ADC Conversion Sequence-A control register: Controls triggering and channel selection for conversion sequence-A. Also specifies interrupt mode for sequence-A. */
/*! @{ */
#define ADC_SEQ_CTRL_CHANNELS_MASK               (0xFFU)
#define ADC_SEQ_CTRL_CHANNELS_SHIFT              (0U)
/*! CHANNELS - Selects which one or more of the ADC channels will be sampled and converted when this
 *    sequence is launched. A 1 in any bit of this field will cause the corresponding channel to be
 *    included in the conversion sequence, where bit 0 corresponds to channel 0, bit 1 to channel 1
 *    and so forth. Bit 6 is channel 6; the supply monitor. Bit 7 is channel 7; the temperature
 *    sensor. When this conversion sequence is triggered, either by a hardware trigger or via software
 *    command, ADC conversions will be performed on each enabled channel, in sequence, beginning
 *    with the lowest-ordered channel. Remark: This field can ONLY be changed while SEQA_ENA (bit 31)
 *    is LOW. It is allowed to change this field and set bit 31 in the same write.
 */
#define ADC_SEQ_CTRL_CHANNELS(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_CHANNELS_SHIFT)) & ADC_SEQ_CTRL_CHANNELS_MASK)
#define ADC_SEQ_CTRL_TRIGGER_MASK                (0x3F000U)
#define ADC_SEQ_CTRL_TRIGGER_SHIFT               (12U)
/*! TRIGGER - Selects which of the available hardware trigger sources will cause this conversion
 *    sequence to be initiated. Program the trigger input number in this field. Setting: 0 : PINT0; 1 :
 *    PWM8; 2 : PWM9; 3 : ARM TX EV. Remark: In order to avoid generating a spurious trigger, it is
 *    recommended writing to+I32 this field only when SEQA_ENA (bit 31) is low. It is safe to
 *    change this field and set bit 31 in the same write.
 */
#define ADC_SEQ_CTRL_TRIGGER(x)                  (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_TRIGGER_SHIFT)) & ADC_SEQ_CTRL_TRIGGER_MASK)
#define ADC_SEQ_CTRL_TRIGPOL_MASK                (0x40000U)
#define ADC_SEQ_CTRL_TRIGPOL_SHIFT               (18U)
/*! TRIGPOL - Select the polarity of the selected input trigger for this conversion sequence.
 *    Remark: In order to avoid generating a spurious trigger, it is recommended writing to this field
 *    only when SEQA_ENA (bit 31) is low. It is safe to change this field and set bit 31 in the same
 *    write. 0: A negative edge launches the conversion sequence on the selected trigger input. 1: A
 *    positive edge launches the conversion sequence on the selected trigger input.
 */
#define ADC_SEQ_CTRL_TRIGPOL(x)                  (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_TRIGPOL_SHIFT)) & ADC_SEQ_CTRL_TRIGPOL_MASK)
#define ADC_SEQ_CTRL_SYNCBYPASS_MASK             (0x80000U)
#define ADC_SEQ_CTRL_SYNCBYPASS_SHIFT            (19U)
/*! SYNCBYPASS - Setting this bit allows the hardware trigger input to bypass synchronization
 *    flip-flop stages and therefore shorten the time between the trigger input signal and the start of a
 *    conversion. There are slightly different criteria for whether or not this bit can be set
 *    depending on the clock operating mode: Synchronous mode (the ASYNMODE in the CTRL register = 0):
 *    Synchronization may be bypassed (this bit may be set) if the selected trigger source is already
 *    synchronous with the main system clock (eg. coming from an on-chip, system-clock-based timer).
 *    Whether this bit is set or not, a trigger pulse must be maintained for at least one system
 *    clock period. Asynchronous mode (the ASYNMODE in the CTRL register = 1): Synchronization may be
 *    bypassed (this bit may be set) if it is certain that the duration of a trigger input pulse
 *    will be at least one cycle of the ADC clock (regardless of whether the trigger comes from and
 *    on-chip or off-chip source). If this bit is NOT set, the trigger pulse must at least be
 *    maintained for one system clock period. 0: Enable trigger synchronization. The hardware trigger bypass
 *    is not enabled. 1: Bypass trigger synchronization. The hardware trigger bypass is enabled.
 */
#define ADC_SEQ_CTRL_SYNCBYPASS(x)               (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_SYNCBYPASS_SHIFT)) & ADC_SEQ_CTRL_SYNCBYPASS_MASK)
#define ADC_SEQ_CTRL_START_BEHAVIOUR_MASK        (0x2000000U)
#define ADC_SEQ_CTRL_START_BEHAVIOUR_SHIFT       (25U)
/*! START_BEHAVIOUR - the Start behavior used on gpadc: writing 0 for repeat start after each input
 *    selection changed, used for seqA with multiple inputs. writing 1 for continuous start, this
 *    bit is only if seqA is used to have full speed on a single input. Remark: with 0 the word rate
 *    is divided by two.
 */
#define ADC_SEQ_CTRL_START_BEHAVIOUR(x)          (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_START_BEHAVIOUR_SHIFT)) & ADC_SEQ_CTRL_START_BEHAVIOUR_MASK)
#define ADC_SEQ_CTRL_START_MASK                  (0x4000000U)
#define ADC_SEQ_CTRL_START_SHIFT                 (26U)
/*! START - Writing a 1 to this field will launch one pass through this conversion sequence. The
 *    behavior will be identical to a sequence triggered by a hardware trigger. Do not write 1 to this
 *    bit if the BURST bit is set. Remark: This bit is only set to a 1 momentarily when written to
 *    launch a conversion sequence. It will consequently always read back as a zero.
 */
#define ADC_SEQ_CTRL_START(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_START_SHIFT)) & ADC_SEQ_CTRL_START_MASK)
#define ADC_SEQ_CTRL_BURST_MASK                  (0x8000000U)
#define ADC_SEQ_CTRL_BURST_SHIFT                 (27U)
/*! BURST - Writing a 1 to this bit will cause this conversion sequence to be continuously cycled
 *    through. Other sequence A triggers will be ignored while this bit is set. Repeated conversions
 *    can be halted by clearing this bit. The sequence currently in progress will be completed before
 *    conversions are terminated. Note that a new sequence could begin just before BURST is cleared.
 */
#define ADC_SEQ_CTRL_BURST(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_BURST_SHIFT)) & ADC_SEQ_CTRL_BURST_MASK)
#define ADC_SEQ_CTRL_SINGLESTEP_MASK             (0x10000000U)
#define ADC_SEQ_CTRL_SINGLESTEP_SHIFT            (28U)
/*! SINGLESTEP - When this bit is set, a hardware trigger or a write to the START bit will launch a
 *    single conversion on the next channel in the sequence instead of the default response of
 *    launching an entire sequence of conversions. Once all of the channels comprising a sequence have
 *    been converted, a subsequent trigger will repeat the sequence beginning with the first enabled
 *    channel. Interrupt generation will still occur either after each individual conversion or at
 *    the end of the entire sequence, depending on the state of the MODE bit.
 */
#define ADC_SEQ_CTRL_SINGLESTEP(x)               (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_SINGLESTEP_SHIFT)) & ADC_SEQ_CTRL_SINGLESTEP_MASK)
#define ADC_SEQ_CTRL_MODE_MASK                   (0x40000000U)
#define ADC_SEQ_CTRL_MODE_SHIFT                  (30U)
/*! MODE - Indicates whether the primary method for retrieving conversion results for this sequence
 *    will be accomplished via reading the global data register (SEQA_GDAT) at the end of each
 *    conversion, or the individual channel result registers at the end of the entire sequence. Impacts
 *    when conversion-complete interrupt/DMA trigger for sequence-A will be generated and which
 *    overrun conditions contribute to an overrun interrupt as described below. 0: End of conversion. The
 *    sequence A interrupt/DMA trigger will be set at the end of each individual ADC conversion
 *    performed under sequence A. This flag will mirror the DATAVALID bit in the SEQA_GDAT register.
 *    The OVERRUN bit in the SEQA_GDAT register will contribute to generation of an overrun
 *    interrupt/DMA trigger if enabled. 1: End of sequence. The sequence A interrupt/DMA trigger will be set
 *    when the entire set of sequence-A conversions completes. This flag will need to be explicitly
 *    cleared by software or by the DMA-clear signal in this mode. The OVERRUN bit in the SEQA_GDAT
 *    register will NOT contribute to generation of an overrun interrupt/DMA trigger since it is
 *    assumed this register may not be utilized in this mode.
 */
#define ADC_SEQ_CTRL_MODE(x)                     (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_MODE_SHIFT)) & ADC_SEQ_CTRL_MODE_MASK)
#define ADC_SEQ_CTRL_SEQ_ENA_MASK                (0x80000000U)
#define ADC_SEQ_CTRL_SEQ_ENA_SHIFT               (31U)
/*! SEQ_ENA - Sequence Enable. In order to avoid spuriously triggering the sequence, care should be
 *    taken to only set the SEQA_ENA bit when the selected trigger input is in its INACTIVE state
 *    (as defined by the TRIGPOL bit). If this condition is not met, the sequence will be triggered
 *    immediately upon being enabled. Remark: In order to avoid spuriously triggering the sequence,
 *    care should be taken to only set the SEQA_ENA bit when the selected trigger input is in its
 *    INACTIVE state (as defined by the TRIGPOL bit). If this condition is not met, the sequence will be
 *    triggered immediately upon being enabled. 0: Disabled. Sequence A is disabled. Sequence A
 *    triggers are ignored. If this bit is cleared while sequence A is in progress, the sequence will
 *    be halted at the end of the current conversion. After the sequence is re-enabled, a new trigger
 *    will be required to restart the sequence beginning with the next enabled channel. 1: Enabled.
 *    Sequence A is enabled.
 */
#define ADC_SEQ_CTRL_SEQ_ENA(x)                  (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_CTRL_SEQ_ENA_SHIFT)) & ADC_SEQ_CTRL_SEQ_ENA_MASK)
/*! @} */

/* The count of ADC_SEQ_CTRL */
#define ADC_SEQ_CTRL_COUNT                       (1U)

/*! @name SEQ_GDAT - ADC Sequence-A Global Data register. This register contains the result of the most recent ADC conversion performed under sequence-A. */
/*! @{ */
#define ADC_SEQ_GDAT_RESULT_MASK                 (0xFFF0U)
#define ADC_SEQ_GDAT_RESULT_SHIFT                (4U)
/*! RESULT - This field contains the 12-bit ADC conversion result from the most recent conversion
 *    performed under conversion sequence associated with this register. DATAVALID = 1 indicates that
 *    this result has not yet been read. If less than 12-bit resolultion is used the ADC result
 *    occupies the upper MSBs and unused LSBs should be ignored.
 */
#define ADC_SEQ_GDAT_RESULT(x)                   (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_GDAT_RESULT_SHIFT)) & ADC_SEQ_GDAT_RESULT_MASK)
#define ADC_SEQ_GDAT_THCMPRANGE_MASK             (0x30000U)
#define ADC_SEQ_GDAT_THCMPRANGE_SHIFT            (16U)
/*! THCMPRANGE - Threshold Range Comparison result. 0x0 = In Range: The last completed conversion
 *    was greater than or equal to the value programmed into the designated LOW threshold register
 *    (THRn_LOW) but less than or equal to the value programmed into the designated HIGH threshold
 *    register (THRn_HIGH). 0x1 = Below Range: The last completed conversion on was less than the value
 *    programmed into the designated LOW threshold register (THRn_LOW). 0x2 = Above Range: The last
 *    completed conversion was greater than the value programmed into the designated HIGH threshold
 *    register (THRn_HIGH). 0x3 = Reserved.
 */
#define ADC_SEQ_GDAT_THCMPRANGE(x)               (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_GDAT_THCMPRANGE_SHIFT)) & ADC_SEQ_GDAT_THCMPRANGE_MASK)
#define ADC_SEQ_GDAT_THCMPCROSS_MASK             (0xC0000U)
#define ADC_SEQ_GDAT_THCMPCROSS_SHIFT            (18U)
/*! THCMPCROSS - Threshold Crossing Comparison result. 0x0 = No threshold Crossing detected: The
 *    most recent completed conversion on this channel had the same relationship (above or below) to
 *    the threshold value established by the designated LOW threshold register (THRn_LOW) as did the
 *    previous conversion on this channel. 0x1 = Reserved. 0x2 = Downward Threshold Crossing
 *    Detected. Indicates that a threshold crossing in the downward direction has occurred - i.e. the
 *    previous sample on this channel was above the threshold value established by the designated LOW
 *    threshold register (THRn_LOW) and the current sample is below that threshold. 0x3 = Upward
 *    Threshold Crossing Detected. Indicates that a threshold crossing in the upward direction has occurred
 *    - i.e. the previous sample on this channel was below the threshold value established by the
 *    designated LOW threshold register (THRn_LOW) and the current sample is above that threshold.
 */
#define ADC_SEQ_GDAT_THCMPCROSS(x)               (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_GDAT_THCMPCROSS_SHIFT)) & ADC_SEQ_GDAT_THCMPCROSS_MASK)
#define ADC_SEQ_GDAT_CHN_MASK                    (0x3C000000U)
#define ADC_SEQ_GDAT_CHN_SHIFT                   (26U)
/*! CHN - These bits contain the channel from which the RESULT bits were converted (e.g. 0000
 *    identifies channel 0, 0001 channel 1, etc.).
 */
#define ADC_SEQ_GDAT_CHN(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_GDAT_CHN_SHIFT)) & ADC_SEQ_GDAT_CHN_MASK)
#define ADC_SEQ_GDAT_OVERRUN_MASK                (0x40000000U)
#define ADC_SEQ_GDAT_OVERRUN_SHIFT               (30U)
/*! OVERRUN - This bit is set if a new conversion result is loaded into the RESULT field before a
 *    previous result has been read - i.e. while the DATAVALID bit is set. This bit is cleared, along
 *    with the DATAVALID bit, whenever this register is read. This bit will contribute to an overrun
 *    interrupt/DMA trigger if the MODE bit (in SEQA_CTRL) for the corresponding sequence is set to
 *    0 (and if the overrun interrupt is enabled).
 */
#define ADC_SEQ_GDAT_OVERRUN(x)                  (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_GDAT_OVERRUN_SHIFT)) & ADC_SEQ_GDAT_OVERRUN_MASK)
#define ADC_SEQ_GDAT_DATAVALID_MASK              (0x80000000U)
#define ADC_SEQ_GDAT_DATAVALID_SHIFT             (31U)
/*! DATAVALID - This bit is set to 1 at the end of each conversion when a new result is loaded into
 *    the RESULT field. It is cleared whenever this register is read. This bit will cause a
 *    conversion-complete interrupt for the corresponding sequence if the MODE bit (in SEQA_CTRL) for that
 *    sequence is set to 0 (and if the interrupt is enabled).
 */
#define ADC_SEQ_GDAT_DATAVALID(x)                (((uint32_t)(((uint32_t)(x)) << ADC_SEQ_GDAT_DATAVALID_SHIFT)) & ADC_SEQ_GDAT_DATAVALID_MASK)
/*! @} */

/* The count of ADC_SEQ_GDAT */
#define ADC_SEQ_GDAT_COUNT                       (1U)

/*! @name DAT - ADC Channel X [0:7] Data register. This register contains the result of the most recent conversion completed on channel X [0:7] . */
/*! @{ */
#define ADC_DAT_RESULT_MASK                      (0xFFF0U)
#define ADC_DAT_RESULT_SHIFT                     (4U)
/*! RESULT - This field contains the 12-bit ADC conversion result from the most recent conversion
 *    performed for this channel under conversion sequence associated with this register. DATAVALID =
 *    1 indicates that this result has not yet been read. If less than 12-bit resolultion is used
 *    the ADC result occupies the upper MSBs and unused LSBs should be ignored.
 */
#define ADC_DAT_RESULT(x)                        (((uint32_t)(((uint32_t)(x)) << ADC_DAT_RESULT_SHIFT)) & ADC_DAT_RESULT_MASK)
#define ADC_DAT_THCMPRANGE_MASK                  (0x30000U)
#define ADC_DAT_THCMPRANGE_SHIFT                 (16U)
/*! THCMPRANGE - Threshold Range Comparison result for this channel. 0x0 = In Range: The last
 *    completed conversion was greater than or equal to the value programmed into the designated LOW
 *    threshold register (THRn_LOW) but less than or equal to the value programmed into the designated
 *    HIGH threshold register (THRn_HIGH). 0x1 = Below Range: The last completed conversion on was
 *    less than the value programmed into the designated LOW threshold register (THRn_LOW). 0x2 = Above
 *    Range: The last completed conversion was greater than the value programmed into the
 *    designated HIGH threshold register (THRn_HIGH). 0x3 = Reserved.
 */
#define ADC_DAT_THCMPRANGE(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_DAT_THCMPRANGE_SHIFT)) & ADC_DAT_THCMPRANGE_MASK)
#define ADC_DAT_THCMPCROSS_MASK                  (0xC0000U)
#define ADC_DAT_THCMPCROSS_SHIFT                 (18U)
/*! THCMPCROSS - Threshold Crossing Comparison result for this channel. 0x0 = No threshold Crossing
 *    detected: The most recent completed conversion on this channel had the same relationship
 *    (above or below) to the threshold value established by the designated LOW threshold register
 *    (THRn_LOW) as did the previous conversion on this channel. 0x1 = Reserved. 0x2 = Downward Threshold
 *    Crossing Detected. Indicates that a threshold crossing in the downward direction has occurred
 *    - i.e. the previous sample on this channel was above the threshold value established by the
 *    designated LOW threshold register (THRn_LOW) and the current sample is below that threshold. 0x3
 *    = Upward Threshold Crossing Detected. Indicates that a threshold crossing in the upward
 *    direction has occurred - i.e. the previous sample on this channel was below the threshold value
 *    established by the designated LOW threshold register (THRn_LOW) and the current sample is above
 *    that threshold.
 */
#define ADC_DAT_THCMPCROSS(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_DAT_THCMPCROSS_SHIFT)) & ADC_DAT_THCMPCROSS_MASK)
#define ADC_DAT_CHANNEL_MASK                     (0x3C000000U)
#define ADC_DAT_CHANNEL_SHIFT                    (26U)
/*! CHANNEL - This field is hard-coded to contain the channel number that this particular register
 *    relates to (i.e. this field will contain 0b0000 for the DAT0 register, 0b0001 for the DAT1
 *    register, etc)
 */
#define ADC_DAT_CHANNEL(x)                       (((uint32_t)(((uint32_t)(x)) << ADC_DAT_CHANNEL_SHIFT)) & ADC_DAT_CHANNEL_MASK)
#define ADC_DAT_OVERRUN_MASK                     (0x40000000U)
#define ADC_DAT_OVERRUN_SHIFT                    (30U)
/*! OVERRUN - This bit is set if a new conversion result is loaded into the RESULT field before a
 *    previous result has been read - i.e. while the DATAVALID bit is set. This bit is cleared, along
 *    with the DATAVALID bit, whenever this register is read or when the data related to this
 *    channel is read from the global SEQA_GDAT register. This bit will contribute to an overrun
 *    interrupt/DMA trigger if the MODE bit (in SEQA_CTRL) for the corresponding sequence is set to 0 (and if
 *    the overrun interrupt is enabled).
 */
#define ADC_DAT_OVERRUN(x)                       (((uint32_t)(((uint32_t)(x)) << ADC_DAT_OVERRUN_SHIFT)) & ADC_DAT_OVERRUN_MASK)
#define ADC_DAT_DATAVALID_MASK                   (0x80000000U)
#define ADC_DAT_DATAVALID_SHIFT                  (31U)
/*! DATAVALID - This bit is set to 1 at the end of each conversion for this channel when a new
 *    result is loaded into the RESULT field. It is cleared whenever this register is read or when the
 *    data related to this channel is read from the global SEQA_GDAT regsiter.
 */
#define ADC_DAT_DATAVALID(x)                     (((uint32_t)(((uint32_t)(x)) << ADC_DAT_DATAVALID_SHIFT)) & ADC_DAT_DATAVALID_MASK)
/*! @} */

/* The count of ADC_DAT */
#define ADC_DAT_COUNT                            (8U)

/*! @name THR0_LOW - ADC Low Compare Threshold register 0: Contains the lower threshold level for automatic threshold comparison for any channels linked to threshold pair 0. */
/*! @{ */
#define ADC_THR0_LOW_THRLOW_MASK                 (0xFFF0U)
#define ADC_THR0_LOW_THRLOW_SHIFT                (4U)
/*! THRLOW - Low threshold value against which ADC results will be compared
 */
#define ADC_THR0_LOW_THRLOW(x)                   (((uint32_t)(((uint32_t)(x)) << ADC_THR0_LOW_THRLOW_SHIFT)) & ADC_THR0_LOW_THRLOW_MASK)
/*! @} */

/*! @name THR1_LOW - ADC Low Compare Threshold register 1: Contains the lower threshold level for automatic threshold comparison for any channels linked to threshold pair 1. */
/*! @{ */
#define ADC_THR1_LOW_THRLOW_MASK                 (0xFFF0U)
#define ADC_THR1_LOW_THRLOW_SHIFT                (4U)
/*! THRLOW - Low threshold value against which ADC results will be compared
 */
#define ADC_THR1_LOW_THRLOW(x)                   (((uint32_t)(((uint32_t)(x)) << ADC_THR1_LOW_THRLOW_SHIFT)) & ADC_THR1_LOW_THRLOW_MASK)
/*! @} */

/*! @name THR0_HIGH - ADC High Compare Threshold register 0: Contains the upper threshold level for automatic threshold comparison for any channels linked to threshold pair 0. */
/*! @{ */
#define ADC_THR0_HIGH_THRHIGH_MASK               (0xFFF0U)
#define ADC_THR0_HIGH_THRHIGH_SHIFT              (4U)
/*! THRHIGH - High threshold value against which ADC results will be compared
 */
#define ADC_THR0_HIGH_THRHIGH(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_THR0_HIGH_THRHIGH_SHIFT)) & ADC_THR0_HIGH_THRHIGH_MASK)
/*! @} */

/*! @name THR1_HIGH - ADC High Compare Threshold register 1: Contains the upper threshold level for automatic threshold comparison for any channels linked to threshold pair 1. */
/*! @{ */
#define ADC_THR1_HIGH_THRHIGH_MASK               (0xFFF0U)
#define ADC_THR1_HIGH_THRHIGH_SHIFT              (4U)
/*! THRHIGH - High threshold value against which ADC results will be compared
 */
#define ADC_THR1_HIGH_THRHIGH(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_THR1_HIGH_THRHIGH_SHIFT)) & ADC_THR1_HIGH_THRHIGH_MASK)
/*! @} */

/*! @name CHAN_THRSEL - ADC Channel-Threshold Select register. Specifies which set of threshold compare registers are to be used for each channel */
/*! @{ */
#define ADC_CHAN_THRSEL_CH0_THRSEL_MASK          (0x1U)
#define ADC_CHAN_THRSEL_CH0_THRSEL_SHIFT         (0U)
/*! CH0_THRSEL - Threshold select for channel 0. 0: Threshold 0. Results for this channel will be
 *    compared against the threshold levels indicated in the THR0_LOW and THR0_HIGH registers. 1:
 *    Threshold 1. Results for this channel will be compared against the threshold levels indicated in
 *    the THR1_LOW and THR1_HIGH registers.
 */
#define ADC_CHAN_THRSEL_CH0_THRSEL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_CHAN_THRSEL_CH0_THRSEL_SHIFT)) & ADC_CHAN_THRSEL_CH0_THRSEL_MASK)
#define ADC_CHAN_THRSEL_CH1_THRSEL_MASK          (0x2U)
#define ADC_CHAN_THRSEL_CH1_THRSEL_SHIFT         (1U)
/*! CH1_THRSEL - Threshold select for channel 1. See description for channel 0.
 */
#define ADC_CHAN_THRSEL_CH1_THRSEL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_CHAN_THRSEL_CH1_THRSEL_SHIFT)) & ADC_CHAN_THRSEL_CH1_THRSEL_MASK)
#define ADC_CHAN_THRSEL_CH2_THRSEL_MASK          (0x4U)
#define ADC_CHAN_THRSEL_CH2_THRSEL_SHIFT         (2U)
/*! CH2_THRSEL - Threshold select for channel 2. See description for channel 0.
 */
#define ADC_CHAN_THRSEL_CH2_THRSEL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_CHAN_THRSEL_CH2_THRSEL_SHIFT)) & ADC_CHAN_THRSEL_CH2_THRSEL_MASK)
#define ADC_CHAN_THRSEL_CH3_THRSEL_MASK          (0x8U)
#define ADC_CHAN_THRSEL_CH3_THRSEL_SHIFT         (3U)
/*! CH3_THRSEL - Threshold select for channel 3. See description for channel 0.
 */
#define ADC_CHAN_THRSEL_CH3_THRSEL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_CHAN_THRSEL_CH3_THRSEL_SHIFT)) & ADC_CHAN_THRSEL_CH3_THRSEL_MASK)
#define ADC_CHAN_THRSEL_CH4_THRSEL_MASK          (0x10U)
#define ADC_CHAN_THRSEL_CH4_THRSEL_SHIFT         (4U)
/*! CH4_THRSEL - Threshold select for channel 4. See description for channel 0.
 */
#define ADC_CHAN_THRSEL_CH4_THRSEL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_CHAN_THRSEL_CH4_THRSEL_SHIFT)) & ADC_CHAN_THRSEL_CH4_THRSEL_MASK)
#define ADC_CHAN_THRSEL_CH5_THRSEL_MASK          (0x20U)
#define ADC_CHAN_THRSEL_CH5_THRSEL_SHIFT         (5U)
/*! CH5_THRSEL - Threshold select for channel 5. See description for channel 0.
 */
#define ADC_CHAN_THRSEL_CH5_THRSEL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_CHAN_THRSEL_CH5_THRSEL_SHIFT)) & ADC_CHAN_THRSEL_CH5_THRSEL_MASK)
#define ADC_CHAN_THRSEL_CH6_THRSEL_MASK          (0x40U)
#define ADC_CHAN_THRSEL_CH6_THRSEL_SHIFT         (6U)
/*! CH6_THRSEL - Threshold select for channel 6. See description for channel 0.
 */
#define ADC_CHAN_THRSEL_CH6_THRSEL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_CHAN_THRSEL_CH6_THRSEL_SHIFT)) & ADC_CHAN_THRSEL_CH6_THRSEL_MASK)
#define ADC_CHAN_THRSEL_CH7_THRSEL_MASK          (0x80U)
#define ADC_CHAN_THRSEL_CH7_THRSEL_SHIFT         (7U)
/*! CH7_THRSEL - Threshold select for channel 7. See description for channel 0.
 */
#define ADC_CHAN_THRSEL_CH7_THRSEL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_CHAN_THRSEL_CH7_THRSEL_SHIFT)) & ADC_CHAN_THRSEL_CH7_THRSEL_MASK)
/*! @} */

/*! @name INTEN - ADC Interrupt Enable register. This register contains enable bits that enable the sequence-A, sequence-B, threshold compare and data overrun interrupts to be generated. */
/*! @{ */
#define ADC_INTEN_SEQA_INTEN_MASK                (0x1U)
#define ADC_INTEN_SEQA_INTEN_SHIFT               (0U)
/*! SEQA_INTEN - Sequence A interrupt enable. 0: Disabled. The sequence A interrupt/DMA trigger is
 *    disabled. 1: Enabled. The sequence A interrupt/DMA trigger is enabled and will be asserted
 *    either upon completion of each individual conversion performed as part of sequence A, or upon
 *    completion of the entire A sequence of conversions, depending on the MODE bit in the SEQA_CTRL
 *    register.
 */
#define ADC_INTEN_SEQA_INTEN(x)                  (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_SEQA_INTEN_SHIFT)) & ADC_INTEN_SEQA_INTEN_MASK)
#define ADC_INTEN_OVR_INTEN_MASK                 (0x4U)
#define ADC_INTEN_OVR_INTEN_SHIFT                (2U)
/*! OVR_INTEN - Overrun interrupt enable. 0: Disabled. The overrun interrupt is disabled. 1:
 *    Enabled. The overrun interrupt is enabled. Detection of an overrun condition on any of the 12 channel
 *    data registers will cause an overrun interrupt/DMA trigger. In addition, if the MODE bit for
 *    a particular sequence is 0, then an overrun in the global data register for that sequence will
 *    also cause this interrupt/DMA trigger to be asserted.
 */
#define ADC_INTEN_OVR_INTEN(x)                   (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_OVR_INTEN_SHIFT)) & ADC_INTEN_OVR_INTEN_MASK)
#define ADC_INTEN_ADCMPINTEN0_MASK               (0x18U)
#define ADC_INTEN_ADCMPINTEN0_SHIFT              (3U)
/*! ADCMPINTEN0 - Threshold comparison interrupt enable for channel 0. 0x0: Disabled. 0x1: Outside
 *    threshold. 0x2: Crossing threshold. 0x3: Reserved
 */
#define ADC_INTEN_ADCMPINTEN0(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_ADCMPINTEN0_SHIFT)) & ADC_INTEN_ADCMPINTEN0_MASK)
#define ADC_INTEN_ADCMPINTEN1_MASK               (0x60U)
#define ADC_INTEN_ADCMPINTEN1_SHIFT              (5U)
/*! ADCMPINTEN1 - Channel 1 threshold comparison interrupt enable. See description for channel 0.
 */
#define ADC_INTEN_ADCMPINTEN1(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_ADCMPINTEN1_SHIFT)) & ADC_INTEN_ADCMPINTEN1_MASK)
#define ADC_INTEN_ADCMPINTEN2_MASK               (0x180U)
#define ADC_INTEN_ADCMPINTEN2_SHIFT              (7U)
/*! ADCMPINTEN2 - Channel 2 threshold comparison interrupt enable. See description for channel 0.
 */
#define ADC_INTEN_ADCMPINTEN2(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_ADCMPINTEN2_SHIFT)) & ADC_INTEN_ADCMPINTEN2_MASK)
#define ADC_INTEN_ADCMPINTEN3_MASK               (0x600U)
#define ADC_INTEN_ADCMPINTEN3_SHIFT              (9U)
/*! ADCMPINTEN3 - Channel 3 threshold comparison interrupt enable. See description for channel 0.
 */
#define ADC_INTEN_ADCMPINTEN3(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_ADCMPINTEN3_SHIFT)) & ADC_INTEN_ADCMPINTEN3_MASK)
#define ADC_INTEN_ADCMPINTEN4_MASK               (0x1800U)
#define ADC_INTEN_ADCMPINTEN4_SHIFT              (11U)
/*! ADCMPINTEN4 - Channel 4 threshold comparison interrupt enable. See description for channel 0.
 */
#define ADC_INTEN_ADCMPINTEN4(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_ADCMPINTEN4_SHIFT)) & ADC_INTEN_ADCMPINTEN4_MASK)
#define ADC_INTEN_ADCMPINTEN5_MASK               (0x6000U)
#define ADC_INTEN_ADCMPINTEN5_SHIFT              (13U)
/*! ADCMPINTEN5 - Channel 5 threshold comparison interrupt enable. See description for channel 0.
 */
#define ADC_INTEN_ADCMPINTEN5(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_ADCMPINTEN5_SHIFT)) & ADC_INTEN_ADCMPINTEN5_MASK)
#define ADC_INTEN_ADCMPINTEN6_MASK               (0x18000U)
#define ADC_INTEN_ADCMPINTEN6_SHIFT              (15U)
/*! ADCMPINTEN6 - Channel 6 threshold comparison interrupt enable. See description for channel 0.
 */
#define ADC_INTEN_ADCMPINTEN6(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_ADCMPINTEN6_SHIFT)) & ADC_INTEN_ADCMPINTEN6_MASK)
#define ADC_INTEN_ADCMPINTEN7_MASK               (0x60000U)
#define ADC_INTEN_ADCMPINTEN7_SHIFT              (17U)
/*! ADCMPINTEN7 - Channel 7 threshold comparison interrupt enable. See description for channel 0.
 */
#define ADC_INTEN_ADCMPINTEN7(x)                 (((uint32_t)(((uint32_t)(x)) << ADC_INTEN_ADCMPINTEN7_SHIFT)) & ADC_INTEN_ADCMPINTEN7_MASK)
/*! @} */

/*! @name FLAGS - ADC Flags register. Contains the four interrupt/DMA trigger flags and the individual component overrun and threshold-compare flags. (The overrun bits replicate information stored in the result registers). */
/*! @{ */
#define ADC_FLAGS_THCMP0_MASK                    (0x1U)
#define ADC_FLAGS_THCMP0_SHIFT                   (0U)
/*! THCMP0 - Threshold comparison event on Channel 0. Set to 1 upon either an out-of-range result or
 *    a threshold-crossing result if enabled to do so in the INTEN register. This bit is cleared by
 *    writing a 1.
 */
#define ADC_FLAGS_THCMP0(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP0_SHIFT)) & ADC_FLAGS_THCMP0_MASK)
#define ADC_FLAGS_THCMP1_MASK                    (0x2U)
#define ADC_FLAGS_THCMP1_SHIFT                   (1U)
/*! THCMP1 - Threshold comparison event on Channel 1. See description for channel 0.
 */
#define ADC_FLAGS_THCMP1(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP1_SHIFT)) & ADC_FLAGS_THCMP1_MASK)
#define ADC_FLAGS_THCMP2_MASK                    (0x4U)
#define ADC_FLAGS_THCMP2_SHIFT                   (2U)
/*! THCMP2 - Threshold comparison event on Channel 2. See description for channel 0.
 */
#define ADC_FLAGS_THCMP2(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP2_SHIFT)) & ADC_FLAGS_THCMP2_MASK)
#define ADC_FLAGS_THCMP3_MASK                    (0x8U)
#define ADC_FLAGS_THCMP3_SHIFT                   (3U)
/*! THCMP3 - Threshold comparison event on Channel 3. See description for channel 0.
 */
#define ADC_FLAGS_THCMP3(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP3_SHIFT)) & ADC_FLAGS_THCMP3_MASK)
#define ADC_FLAGS_THCMP4_MASK                    (0x10U)
#define ADC_FLAGS_THCMP4_SHIFT                   (4U)
/*! THCMP4 - Threshold comparison event on Channel 4. See description for channel 0.
 */
#define ADC_FLAGS_THCMP4(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP4_SHIFT)) & ADC_FLAGS_THCMP4_MASK)
#define ADC_FLAGS_THCMP5_MASK                    (0x20U)
#define ADC_FLAGS_THCMP5_SHIFT                   (5U)
/*! THCMP5 - Threshold comparison event on Channel 5. See description for channel 0.
 */
#define ADC_FLAGS_THCMP5(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP5_SHIFT)) & ADC_FLAGS_THCMP5_MASK)
#define ADC_FLAGS_THCMP6_MASK                    (0x40U)
#define ADC_FLAGS_THCMP6_SHIFT                   (6U)
/*! THCMP6 - Threshold comparison event on Channel 6. See description for channel 0.
 */
#define ADC_FLAGS_THCMP6(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP6_SHIFT)) & ADC_FLAGS_THCMP6_MASK)
#define ADC_FLAGS_THCMP7_MASK                    (0x80U)
#define ADC_FLAGS_THCMP7_SHIFT                   (7U)
/*! THCMP7 - Threshold comparison event on Channel 7. See description for channel 0.
 */
#define ADC_FLAGS_THCMP7(x)                      (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP7_SHIFT)) & ADC_FLAGS_THCMP7_MASK)
#define ADC_FLAGS_OVERRUN0_MASK                  (0x1000U)
#define ADC_FLAGS_OVERRUN0_SHIFT                 (12U)
/*! OVERRUN0 - Mirrors the OVERRRUN status flag from the result register for ADC channel 0
 */
#define ADC_FLAGS_OVERRUN0(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVERRUN0_SHIFT)) & ADC_FLAGS_OVERRUN0_MASK)
#define ADC_FLAGS_OVERRUN1_MASK                  (0x2000U)
#define ADC_FLAGS_OVERRUN1_SHIFT                 (13U)
/*! OVERRUN1 - Mirrors the OVERRRUN status flag from the result register for ADC channel 1
 */
#define ADC_FLAGS_OVERRUN1(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVERRUN1_SHIFT)) & ADC_FLAGS_OVERRUN1_MASK)
#define ADC_FLAGS_OVERRUN2_MASK                  (0x4000U)
#define ADC_FLAGS_OVERRUN2_SHIFT                 (14U)
/*! OVERRUN2 - Mirrors the OVERRRUN status flag from the result register for ADC channel 2
 */
#define ADC_FLAGS_OVERRUN2(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVERRUN2_SHIFT)) & ADC_FLAGS_OVERRUN2_MASK)
#define ADC_FLAGS_OVERRUN3_MASK                  (0x8000U)
#define ADC_FLAGS_OVERRUN3_SHIFT                 (15U)
/*! OVERRUN3 - Mirrors the OVERRRUN status flag from the result register for ADC channel 3
 */
#define ADC_FLAGS_OVERRUN3(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVERRUN3_SHIFT)) & ADC_FLAGS_OVERRUN3_MASK)
#define ADC_FLAGS_OVERRUN4_MASK                  (0x10000U)
#define ADC_FLAGS_OVERRUN4_SHIFT                 (16U)
/*! OVERRUN4 - Mirrors the OVERRRUN status flag from the result register for ADC channel 4
 */
#define ADC_FLAGS_OVERRUN4(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVERRUN4_SHIFT)) & ADC_FLAGS_OVERRUN4_MASK)
#define ADC_FLAGS_OVERRUN5_MASK                  (0x20000U)
#define ADC_FLAGS_OVERRUN5_SHIFT                 (17U)
/*! OVERRUN5 - Mirrors the OVERRRUN status flag from the result register for ADC channel 5
 */
#define ADC_FLAGS_OVERRUN5(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVERRUN5_SHIFT)) & ADC_FLAGS_OVERRUN5_MASK)
#define ADC_FLAGS_OVERRUN6_MASK                  (0x40000U)
#define ADC_FLAGS_OVERRUN6_SHIFT                 (18U)
/*! OVERRUN6 - Mirrors the OVERRRUN status flag from the result register for ADC channel 6
 */
#define ADC_FLAGS_OVERRUN6(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVERRUN6_SHIFT)) & ADC_FLAGS_OVERRUN6_MASK)
#define ADC_FLAGS_OVERRUN7_MASK                  (0x80000U)
#define ADC_FLAGS_OVERRUN7_SHIFT                 (19U)
/*! OVERRUN7 - Mirrors the OVERRRUN status flag from the result register for ADC channel 7
 */
#define ADC_FLAGS_OVERRUN7(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVERRUN7_SHIFT)) & ADC_FLAGS_OVERRUN7_MASK)
#define ADC_FLAGS_SEQA_OVR_MASK                  (0x1000000U)
#define ADC_FLAGS_SEQA_OVR_SHIFT                 (24U)
/*! SEQA_OVR - Mirrors the global OVERRUN status flag in the SEQA_GDAT register
 */
#define ADC_FLAGS_SEQA_OVR(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_SEQA_OVR_SHIFT)) & ADC_FLAGS_SEQA_OVR_MASK)
#define ADC_FLAGS_SEQA_INT_MASK                  (0x10000000U)
#define ADC_FLAGS_SEQA_INT_SHIFT                 (28U)
/*! SEQA_INT - Sequence A interrupt/DMA trigger. If the MODE bit in the SEQA_CTRL register is 0,
 *    this flag will mirror the DATAVALID bit in the sequence A global data register (SEQA_GDAT), which
 *    is set at the end of every ADC conversion performed as part of sequence A. It will be cleared
 *    automatically when the SEQA_GDAT register is read. If the MODE bit in the SEQA_CTRL register
 *    is 1, this flag will be set upon completion of an entire A sequence. In this case it must be
 *    cleared by writing a 1 to this SEQA_INT bit. This interrupt must be enabled in the INTEN
 *    register.
 */
#define ADC_FLAGS_SEQA_INT(x)                    (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_SEQA_INT_SHIFT)) & ADC_FLAGS_SEQA_INT_MASK)
#define ADC_FLAGS_THCMP_INT_MASK                 (0x40000000U)
#define ADC_FLAGS_THCMP_INT_SHIFT                (30U)
/*! THCMP_INT - Threshold Comparison Interrupt. This bit will be set if any of the THCMP flags in
 *    the lower bits of this register are set to 1 (due to an enabled out-of-range or
 *    threshold-crossing event on any channel). Each type of threshold comparison interrupt on each channel must be
 *    individually enabled in the INTEN register to cause this interrupt. This bit will be cleared
 *    when all of the individual threshold flags are cleared via writing 1s to those bits.
 */
#define ADC_FLAGS_THCMP_INT(x)                   (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_THCMP_INT_SHIFT)) & ADC_FLAGS_THCMP_INT_MASK)
#define ADC_FLAGS_OVR_INT_MASK                   (0x80000000U)
#define ADC_FLAGS_OVR_INT_SHIFT                  (31U)
/*! OVR_INT - Overrun Interrupt flag. Any overrun bit in any of the individual channel data
 *    registers will cause this interrupt. In addition, if the MODE bit in either of the SEQn_CTRL registers
 *    is 0 then the OVERRUN bit in the corresponding SEQn_GDAT register will also cause this
 *    interrupt. This interrupt must be enabled in the INTEN register. This bit will be cleared when all
 *    of the individual overrun bits have been cleared via reading the corresponding data registers.
 */
#define ADC_FLAGS_OVR_INT(x)                     (((uint32_t)(((uint32_t)(x)) << ADC_FLAGS_OVR_INT_SHIFT)) & ADC_FLAGS_OVR_INT_MASK)
/*! @} */

/*! @name STARTUP - ADC Startup register (typically only used by the ADC API). */
/*! @{ */
#define ADC_STARTUP_ADC_ENA_MASK                 (0x1U)
#define ADC_STARTUP_ADC_ENA_SHIFT                (0U)
/*! ADC_ENA - ADC Enable bit. This bit can only be set to a 1 by software. It is cleared
 *    automatically whenever the ADC is powered down. This bit must not be set until at least 10 microseconds
 *    after the ADC is powered up (typically by altering a system-level ADC power control bit).
 */
#define ADC_STARTUP_ADC_ENA(x)                   (((uint32_t)(((uint32_t)(x)) << ADC_STARTUP_ADC_ENA_SHIFT)) & ADC_STARTUP_ADC_ENA_MASK)
/*! @} */

/*! @name GPADC_CTRL0 - Second ADC Control register : ADC internal LDO (within ADC sub-system) */
/*! @{ */
#define ADC_GPADC_CTRL0_LDO_POWER_EN_MASK        (0x1U)
#define ADC_GPADC_CTRL0_LDO_POWER_EN_SHIFT       (0U)
/*! LDO_POWER_EN - LDO Power enable signal (active high). This is for the LDO within the ADC itself.
 *    There is also LDOADC controlled from PMC that is outside the ADC block. The LDOADC should
 *    have been enabled for 10usec before enabling this LDO. After enabling this LDO it is necessary to
 *    wait for 230usec, before ADC sampling is commenced, so that full accuraccy of the ADC will be
 *    obtained.
 */
#define ADC_GPADC_CTRL0_LDO_POWER_EN(x)          (((uint32_t)(((uint32_t)(x)) << ADC_GPADC_CTRL0_LDO_POWER_EN_SHIFT)) & ADC_GPADC_CTRL0_LDO_POWER_EN_MASK)
#define ADC_GPADC_CTRL0_LDO_SEL_OUT_MASK         (0xF8U)
#define ADC_GPADC_CTRL0_LDO_SEL_OUT_SHIFT        (3U)
/*! LDO_SEL_OUT - Select LDO output voltage (10mV step) [between 0.64V and 0.95V]. This is for the
 *    LDO within the ADC itself. There is also LDOADC controlled from PMC that is outside the ADC
 *    block.
 */
#define ADC_GPADC_CTRL0_LDO_SEL_OUT(x)           (((uint32_t)(((uint32_t)(x)) << ADC_GPADC_CTRL0_LDO_SEL_OUT_SHIFT)) & ADC_GPADC_CTRL0_LDO_SEL_OUT_MASK)
#define ADC_GPADC_CTRL0_PASS_ENABLE_MASK         (0x100U)
#define ADC_GPADC_CTRL0_PASS_ENABLE_SHIFT        (8U)
/*! PASS_ENABLE - Enable pass mode when set to high. This is for the LDO within the ADC itself.
 *    There is also LDOADC controlled from PMC that is outside the ADC block.
 */
#define ADC_GPADC_CTRL0_PASS_ENABLE(x)           (((uint32_t)(((uint32_t)(x)) << ADC_GPADC_CTRL0_PASS_ENABLE_SHIFT)) & ADC_GPADC_CTRL0_PASS_ENABLE_MASK)
#define ADC_GPADC_CTRL0_GPADC_TSAMP_MASK         (0x3E00U)
#define ADC_GPADC_CTRL0_GPADC_TSAMP_SHIFT        (9U)
/*! GPADC_TSAMP - Extand ADC sampling time according to source impedance 1: 0.621 kOhm 20 (default): 55 kOhm 31: 87 kOhm
 */
#define ADC_GPADC_CTRL0_GPADC_TSAMP(x)           (((uint32_t)(((uint32_t)(x)) << ADC_GPADC_CTRL0_GPADC_TSAMP_SHIFT)) & ADC_GPADC_CTRL0_GPADC_TSAMP_MASK)
#define ADC_GPADC_CTRL0_TEST_MASK                (0xC000U)
#define ADC_GPADC_CTRL0_TEST_SHIFT               (14U)
/*! TEST - Mode selection: '00': Normal functional mode (DIV4 mode). Input range is 0 to 3.6V,
 *    although max input voltage is affected by supply voltage of the device. '01': Multiplexer test mode
 *    '10': ADC in unity gain mode. (DIV1 mode). Input range is 0 to 0.9V. Voltages above this may
 *    damage the device. '11': Not used
 */
#define ADC_GPADC_CTRL0_TEST(x)                  (((uint32_t)(((uint32_t)(x)) << ADC_GPADC_CTRL0_TEST_SHIFT)) & ADC_GPADC_CTRL0_TEST_MASK)
/*! @} */

/*! @name GPADC_CTRL1 - Third ADC Control register : ADC internal gain and offset */
/*! @{ */
#define ADC_GPADC_CTRL1_OFFSET_CAL_MASK          (0x3FFU)
#define ADC_GPADC_CTRL1_OFFSET_CAL_SHIFT         (0U)
/*! OFFSET_CAL - offset_cal the setting is used within the ADC to compensate for a DC shift in values for this particular device.
 */
#define ADC_GPADC_CTRL1_OFFSET_CAL(x)            (((uint32_t)(((uint32_t)(x)) << ADC_GPADC_CTRL1_OFFSET_CAL_SHIFT)) & ADC_GPADC_CTRL1_OFFSET_CAL_MASK)
#define ADC_GPADC_CTRL1_GAIN_CAL_MASK            (0xFFC00U)
#define ADC_GPADC_CTRL1_GAIN_CAL_SHIFT           (10U)
/*! GAIN_CAL - gain_cal the setting is used within the ADC to compensate for any gain variation for this particular device.
 */
#define ADC_GPADC_CTRL1_GAIN_CAL(x)              (((uint32_t)(((uint32_t)(x)) << ADC_GPADC_CTRL1_GAIN_CAL_SHIFT)) & ADC_GPADC_CTRL1_GAIN_CAL_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group ADC_Register_Masks */


/* ADC - Peripheral instance base addresses */
/** Peripheral ADC0 base address */
#define ADC0_BASE                                (0x40089000u)
/** Peripheral ADC0 base pointer */
#define ADC0                                     ((ADC_Type *)ADC0_BASE)
/** Array initializer of ADC peripheral base addresses */
#define ADC_BASE_ADDRS                           { ADC0_BASE }
/** Array initializer of ADC peripheral base pointers */
#define ADC_BASE_PTRS                            { ADC0 }
/** Interrupt vectors for the ADC peripheral type */
#define ADC_SEQ_IRQS                             { ADC0_SEQA_IRQn, ADC0_SEQB_IRQn }
#define ADC_THCMP_IRQS                           { ADC0_THCMP_IRQn }

/*!
 * @}
 */ /* end of group ADC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- AES Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup AES_Peripheral_Access_Layer AES Peripheral Access Layer
 * @{
 */

/** AES - Register Layout Typedef */
typedef struct {
  __IO uint32_t CFG;                               /**< Configuration, offset: 0x0 */
  __IO uint32_t CMD;                               /**< Command, offset: 0x4 */
  __IO uint32_t STAT;                              /**< Status, offset: 0x8 */
  __IO uint32_t CTR_INCR;                          /**< Counter Increment. Increment value for HOLDING when in Counter modes, offset: 0xC */
       uint8_t RESERVED_0[16];
  __O  uint32_t KEY[8];                            /**< Bits of the AES key, array offset: 0x20, array step: 0x4 */
  __O  uint32_t INTEXT[4];                         /**< Input text bits, array offset: 0x40, array step: 0x4 */
  __O  uint32_t HOLDING[4];                        /**< Holding register bits, array offset: 0x50, array step: 0x4 */
  __I  uint32_t OUTTEXT[4];                        /**< Output text bits, array offset: 0x60, array step: 0x4 */
  __O  uint32_t GF128_Y[4];                        /**< Y bits input of GF128 hash, array offset: 0x70, array step: 0x4 */
  __I  uint32_t GF128_Z[4];                        /**< Result bits of GF128 hash, array offset: 0x80, array step: 0x4 */
  __I  uint32_t GCM_TAG[4];                        /**< GCM Tag bits, array offset: 0x90, array step: 0x4 */
} AES_Type;

/* ----------------------------------------------------------------------------
   -- AES Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup AES_Register_Masks AES Register Masks
 * @{
 */

/*! @name CFG - Configuration */
/*! @{ */
#define AES_CFG_PROC_EN_MASK                     (0x3U)
#define AES_CFG_PROC_EN_SHIFT                    (0U)
/*! PROC_EN - Processing Mode Enable. 00: Reserved. 01: Encrypt/Decrypt Only. 10: GF128 Hash Only. 11: Encrypt/Decrypt and Hash.
 */
#define AES_CFG_PROC_EN(x)                       (((uint32_t)(((uint32_t)(x)) << AES_CFG_PROC_EN_SHIFT)) & AES_CFG_PROC_EN_MASK)
#define AES_CFG_GF128_SEL_MASK                   (0x4U)
#define AES_CFG_GF128_SEL_SHIFT                  (2U)
/*! GF128_SEL - GF128 Select Mode. 0: GF128 Hash Input Text. 1: GF128 Hash Output Text.
 */
#define AES_CFG_GF128_SEL(x)                     (((uint32_t)(((uint32_t)(x)) << AES_CFG_GF128_SEL_SHIFT)) & AES_CFG_GF128_SEL_MASK)
#define AES_CFG_INTEXT_BSWAP_MASK                (0x10U)
#define AES_CFG_INTEXT_BSWAP_SHIFT               (4U)
/*! INTEXT_BSWAP - Input Text Byte Swap
 */
#define AES_CFG_INTEXT_BSWAP(x)                  (((uint32_t)(((uint32_t)(x)) << AES_CFG_INTEXT_BSWAP_SHIFT)) & AES_CFG_INTEXT_BSWAP_MASK)
#define AES_CFG_INTEXT_WSWAP_MASK                (0x20U)
#define AES_CFG_INTEXT_WSWAP_SHIFT               (5U)
/*! INTEXT_WSWAP - Input Text Word Swap
 */
#define AES_CFG_INTEXT_WSWAP(x)                  (((uint32_t)(((uint32_t)(x)) << AES_CFG_INTEXT_WSWAP_SHIFT)) & AES_CFG_INTEXT_WSWAP_MASK)
#define AES_CFG_OUTTEXT_BSWAP_MASK               (0x40U)
#define AES_CFG_OUTTEXT_BSWAP_SHIFT              (6U)
/*! OUTTEXT_BSWAP - Output Text Byte Swap
 */
#define AES_CFG_OUTTEXT_BSWAP(x)                 (((uint32_t)(((uint32_t)(x)) << AES_CFG_OUTTEXT_BSWAP_SHIFT)) & AES_CFG_OUTTEXT_BSWAP_MASK)
#define AES_CFG_OUTTEXT_WSWAP_MASK               (0x80U)
#define AES_CFG_OUTTEXT_WSWAP_SHIFT              (7U)
/*! OUTTEXT_WSWAP - Output Text Word Swap
 */
#define AES_CFG_OUTTEXT_WSWAP(x)                 (((uint32_t)(((uint32_t)(x)) << AES_CFG_OUTTEXT_WSWAP_SHIFT)) & AES_CFG_OUTTEXT_WSWAP_MASK)
#define AES_CFG_KEY_CFG_MASK                     (0x300U)
#define AES_CFG_KEY_CFG_SHIFT                    (8U)
/*! KEY_CFG - Key Configuration. 00: 128 Bit Key. 01: 192 Bit Key. 10: 256 Bit Key. 11: Reserved.
 */
#define AES_CFG_KEY_CFG(x)                       (((uint32_t)(((uint32_t)(x)) << AES_CFG_KEY_CFG_SHIFT)) & AES_CFG_KEY_CFG_MASK)
#define AES_CFG_INBLK_SEL_MASK                   (0x30000U)
#define AES_CFG_INBLK_SEL_SHIFT                  (16U)
/*! INBLK_SEL - Input Block Selection From: 00: Reserved. 01: Input Text. 10: Holding. 11: Input Text XOR Holding.
 */
#define AES_CFG_INBLK_SEL(x)                     (((uint32_t)(((uint32_t)(x)) << AES_CFG_INBLK_SEL_SHIFT)) & AES_CFG_INBLK_SEL_MASK)
#define AES_CFG_HOLD_SEL_MASK                    (0x300000U)
#define AES_CFG_HOLD_SEL_SHIFT                   (20U)
/*! HOLD_SEL - Holding Select From: 00: Counter. 01: Input Text. 10: Output Block. 11: Input Text XOR Output Block.
 */
#define AES_CFG_HOLD_SEL(x)                      (((uint32_t)(((uint32_t)(x)) << AES_CFG_HOLD_SEL_SHIFT)) & AES_CFG_HOLD_SEL_MASK)
#define AES_CFG_OUTTEXT_SEL_MASK                 (0x3000000U)
#define AES_CFG_OUTTEXT_SEL_SHIFT                (24U)
/*! OUTTEXT_SEL - Output Text Selection From: 00: Output Block. 01: Output Block XOR Input Text. 10:
 *    Output Block XOR Holding. 11: Reserved.
 */
#define AES_CFG_OUTTEXT_SEL(x)                   (((uint32_t)(((uint32_t)(x)) << AES_CFG_OUTTEXT_SEL_SHIFT)) & AES_CFG_OUTTEXT_SEL_MASK)
/*! @} */

/*! @name CMD - Command */
/*! @{ */
#define AES_CMD_COPY_SKEY_MASK                   (0x1U)
#define AES_CMD_COPY_SKEY_SHIFT                  (0U)
/*! COPY_SKEY - Copies Secret Key and enables cipher. Secret key is typically held in OTP or other secure memory.
 */
#define AES_CMD_COPY_SKEY(x)                     (((uint32_t)(((uint32_t)(x)) << AES_CMD_COPY_SKEY_SHIFT)) & AES_CMD_COPY_SKEY_MASK)
#define AES_CMD_COPY_TO_Y_MASK                   (0x2U)
#define AES_CMD_COPY_TO_Y_SHIFT                  (1U)
/*! COPY_TO_Y - Copies Output Text to GF128 Y. Typically used for GCM where the Hash requires a Y
 *    input which is the result of an ECB encryption of 0s. Should be performed after encryption of 0s.
 */
#define AES_CMD_COPY_TO_Y(x)                     (((uint32_t)(((uint32_t)(x)) << AES_CMD_COPY_TO_Y_SHIFT)) & AES_CMD_COPY_TO_Y_MASK)
#define AES_CMD_SWITCH_MODE_MASK                 (0x10U)
#define AES_CMD_SWITCH_MODE_SHIFT                (4U)
/*! SWITCH_MODE - Switches mode from Forward to Reverse or from Reverse to Forward. Must wait for
 *    Idle after command. Typically used for non-counter modes (ECB, CBC, CFB, OFB) to switch from
 *    forward to reverse mode for decryption.
 */
#define AES_CMD_SWITCH_MODE(x)                   (((uint32_t)(((uint32_t)(x)) << AES_CMD_SWITCH_MODE_SHIFT)) & AES_CMD_SWITCH_MODE_MASK)
#define AES_CMD_ABORT_MASK                       (0x100U)
#define AES_CMD_ABORT_SHIFT                      (8U)
/*! ABORT - Aborts Encrypt/Decrypt and GF128 Hash, clears INTEXT, clears OUTTEXT, and clears HOLDING
 */
#define AES_CMD_ABORT(x)                         (((uint32_t)(((uint32_t)(x)) << AES_CMD_ABORT_SHIFT)) & AES_CMD_ABORT_MASK)
#define AES_CMD_WIPE_MASK                        (0x200U)
#define AES_CMD_WIPE_SHIFT                       (9U)
/*! WIPE - Performs Abort, clears KEY, disables cipher, and clears GF128_Y
 */
#define AES_CMD_WIPE(x)                          (((uint32_t)(((uint32_t)(x)) << AES_CMD_WIPE_SHIFT)) & AES_CMD_WIPE_MASK)
/*! @} */

/*! @name STAT - Status */
/*! @{ */
#define AES_STAT_IDLE_MASK                       (0x1U)
#define AES_STAT_IDLE_SHIFT                      (0U)
/*! IDLE - When set, all state machines are idle
 */
#define AES_STAT_IDLE(x)                         (((uint32_t)(((uint32_t)(x)) << AES_STAT_IDLE_SHIFT)) & AES_STAT_IDLE_MASK)
#define AES_STAT_IN_READY_MASK                   (0x2U)
#define AES_STAT_IN_READY_SHIFT                  (1U)
/*! IN_READY - When set, input Text can be written
 */
#define AES_STAT_IN_READY(x)                     (((uint32_t)(((uint32_t)(x)) << AES_STAT_IN_READY_SHIFT)) & AES_STAT_IN_READY_MASK)
#define AES_STAT_OUT_READY_MASK                  (0x4U)
#define AES_STAT_OUT_READY_SHIFT                 (2U)
/*! OUT_READY - When set, output Text can be read
 */
#define AES_STAT_OUT_READY(x)                    (((uint32_t)(((uint32_t)(x)) << AES_STAT_OUT_READY_SHIFT)) & AES_STAT_OUT_READY_MASK)
#define AES_STAT_REVERSE_MASK                    (0x10U)
#define AES_STAT_REVERSE_SHIFT                   (4U)
/*! REVERSE - When set, Cipher in reverse mode
 */
#define AES_STAT_REVERSE(x)                      (((uint32_t)(((uint32_t)(x)) << AES_STAT_REVERSE_SHIFT)) & AES_STAT_REVERSE_MASK)
#define AES_STAT_KEY_VALID_MASK                  (0x20U)
#define AES_STAT_KEY_VALID_SHIFT                 (5U)
/*! KEY_VALID - When set, Key is valid
 */
#define AES_STAT_KEY_VALID(x)                    (((uint32_t)(((uint32_t)(x)) << AES_STAT_KEY_VALID_SHIFT)) & AES_STAT_KEY_VALID_MASK)
/*! @} */

/*! @name CTR_INCR - Counter Increment. Increment value for HOLDING when in Counter modes */
/*! @{ */
#define AES_CTR_INCR_CTR_INCR_MASK               (0xFFFFFFFFU)
#define AES_CTR_INCR_CTR_INCR_SHIFT              (0U)
/*! CTR_INCR - Counter Increment. Increment value for HOLDING when in Counter modes
 */
#define AES_CTR_INCR_CTR_INCR(x)                 (((uint32_t)(((uint32_t)(x)) << AES_CTR_INCR_CTR_INCR_SHIFT)) & AES_CTR_INCR_CTR_INCR_MASK)
/*! @} */

/*! @name KEY - Bits of the AES key */
/*! @{ */
#define AES_KEY_KEY_MASK                         (0xFFFFFFFFU)
#define AES_KEY_KEY_SHIFT                        (0U)
/*! KEY - Contains the bits of the AES key.
 */
#define AES_KEY_KEY(x)                           (((uint32_t)(((uint32_t)(x)) << AES_KEY_KEY_SHIFT)) & AES_KEY_KEY_MASK)
/*! @} */

/* The count of AES_KEY */
#define AES_KEY_COUNT                            (8U)

/*! @name INTEXT - Input text bits */
/*! @{ */
#define AES_INTEXT_INTEXT_MASK                   (0xFFFFFFFFU)
#define AES_INTEXT_INTEXT_SHIFT                  (0U)
/*! INTEXT - Contains bits of the AES key.
 */
#define AES_INTEXT_INTEXT(x)                     (((uint32_t)(((uint32_t)(x)) << AES_INTEXT_INTEXT_SHIFT)) & AES_INTEXT_INTEXT_MASK)
/*! @} */

/* The count of AES_INTEXT */
#define AES_INTEXT_COUNT                         (4U)

/*! @name HOLDING - Holding register bits */
/*! @{ */
#define AES_HOLDING_HOLDING_MASK                 (0xFFFFFFFFU)
#define AES_HOLDING_HOLDING_SHIFT                (0U)
/*! HOLDING - Contains the first word (bits 31:0) of the 128 bit Holding value.
 */
#define AES_HOLDING_HOLDING(x)                   (((uint32_t)(((uint32_t)(x)) << AES_HOLDING_HOLDING_SHIFT)) & AES_HOLDING_HOLDING_MASK)
/*! @} */

/* The count of AES_HOLDING */
#define AES_HOLDING_COUNT                        (4U)

/*! @name OUTTEXT - Output text bits */
/*! @{ */
#define AES_OUTTEXT_OUTTEXT_MASK                 (0xFFFFFFFFU)
#define AES_OUTTEXT_OUTTEXT_SHIFT                (0U)
/*! OUTTEXT - Contains the bits of the 128 bit Output text data.
 */
#define AES_OUTTEXT_OUTTEXT(x)                   (((uint32_t)(((uint32_t)(x)) << AES_OUTTEXT_OUTTEXT_SHIFT)) & AES_OUTTEXT_OUTTEXT_MASK)
/*! @} */

/* The count of AES_OUTTEXT */
#define AES_OUTTEXT_COUNT                        (4U)

/*! @name GF128_Y - Y bits input of GF128 hash */
/*! @{ */
#define AES_GF128_Y_GF128_Y_MASK                 (0xFFFFFFFFU)
#define AES_GF128_Y_GF128_Y_SHIFT                (0U)
/*! GF128_Y - Contains the bits of the Y input of GF128 hash.
 */
#define AES_GF128_Y_GF128_Y(x)                   (((uint32_t)(((uint32_t)(x)) << AES_GF128_Y_GF128_Y_SHIFT)) & AES_GF128_Y_GF128_Y_MASK)
/*! @} */

/* The count of AES_GF128_Y */
#define AES_GF128_Y_COUNT                        (4U)

/*! @name GF128_Z - Result bits of GF128 hash */
/*! @{ */
#define AES_GF128_Z_GF128_Z_MASK                 (0xFFFFFFFFU)
#define AES_GF128_Z_GF128_Z_SHIFT                (0U)
/*! GF128_Z - Contains bits of the result of GF128 hash.
 */
#define AES_GF128_Z_GF128_Z(x)                   (((uint32_t)(((uint32_t)(x)) << AES_GF128_Z_GF128_Z_SHIFT)) & AES_GF128_Z_GF128_Z_MASK)
/*! @} */

/* The count of AES_GF128_Z */
#define AES_GF128_Z_COUNT                        (4U)

/*! @name GCM_TAG - GCM Tag bits */
/*! @{ */
#define AES_GCM_TAG_GCM_TAG_MASK                 (0xFFFFFFFFU)
#define AES_GCM_TAG_GCM_TAG_SHIFT                (0U)
/*! GCM_TAG - Contains bits of the 128 bit GCM tag.
 */
#define AES_GCM_TAG_GCM_TAG(x)                   (((uint32_t)(((uint32_t)(x)) << AES_GCM_TAG_GCM_TAG_SHIFT)) & AES_GCM_TAG_GCM_TAG_MASK)
/*! @} */

/* The count of AES_GCM_TAG */
#define AES_GCM_TAG_COUNT                        (4U)


/*!
 * @}
 */ /* end of group AES_Register_Masks */


/* AES - Peripheral instance base addresses */
/** Peripheral AES0 base address */
#define AES0_BASE                                (0x40086000u)
/** Peripheral AES0 base pointer */
#define AES0                                     ((AES_Type *)AES0_BASE)
/** Array initializer of AES peripheral base addresses */
#define AES_BASE_ADDRS                           { AES0_BASE }
/** Array initializer of AES peripheral base pointers */
#define AES_BASE_PTRS                            { AES0 }

/*!
 * @}
 */ /* end of group AES_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- ASYNC_SYSCON Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ASYNC_SYSCON_Peripheral_Access_Layer ASYNC_SYSCON Peripheral Access Layer
 * @{
 */

/** ASYNC_SYSCON - Register Layout Typedef */
typedef struct {
  __IO uint32_t ASYNCPRESETCTRL;                   /**< Asynchronous peripherals reset control. The ASYNCPRESETCTRL register allows software to reset specific peripherals attached to the async APB bridge. Writing a zero to any assigned bit in this register clears the reset and allows the specified peripheral to operate. Writing a one asserts the reset., offset: 0x0 */
  __O  uint32_t ASYNCPRESETCTRLSET;                /**< Set bits in ASYNCPRESETCTRL. Writing ones to this register sets the corresponding bit or bits in the ASYNCPRESETCTRL register, if they are implemented, offset: 0x4 */
  __O  uint32_t ASYNCPRESETCTRLCLR;                /**< Clear bits in ASYNCPRESETCTRL. Writing ones to this register clears the corresponding bit or bits in the ASYNCPRESETCTRL register, if they are implemented, offset: 0x8 */
       uint8_t RESERVED_0[4];
  __IO uint32_t ASYNCAPBCLKCTRL;                   /**< Asynchronous peripherals clock control. This register controls how the clock selected for the asynchronous APB peripherals is divided to provide the clock to the asynchronous peripherals, offset: 0x10 */
  __O  uint32_t ASYNCAPBCLKCTRLSET;                /**< Set bits in ASYNCAPBCLKCTRL. Writing ones to this register sets the corresponding bit or bits in the ASYNCAPBCLKCTRLSET register, if they are implemented, offset: 0x14 */
  __O  uint32_t ASYNCAPBCLKCTRLCLR;                /**< Clear bits in ASYNCAPBCLKCTRL. Writing ones to this register sets the corresponding bit or bits in the ASYNCAPBCLKCTRLSET register, if they are implemented, offset: 0x18 */
       uint8_t RESERVED_1[4];
  __IO uint32_t ASYNCAPBCLKSELA;                   /**< Asynchronous APB clock source select, offset: 0x20 */
       uint8_t RESERVED_2[124];
  __IO uint32_t TEMPSENSORCTRL;                    /**< Temperature Sensor controls, offset: 0xA0 */
  __IO uint32_t NFCTAGPADSCTRL;                    /**< NFC Tag pads control for I2C interface to internal NFC Tag (T parts only): I2C interface + 1 interrupt/ field detect input pad + NTAG VDD output pad, offset: 0xA4 */
  __IO uint32_t XTAL32MLDOCTRL;                    /**< XTAL 32 MHz LDO control register. If XTAL has been auto started due to EFUSE XTAL32MSTART_ENA or BLE low power timers then the effect of these need disabling via SYSCON.XTAL32MCTRL before the full control by this register is possible., offset: 0xA8 */
  __IO uint32_t XTAL32MCTRL;                       /**< XTAL 32 MHz control register. If XTAL has been auto started due to EFUSE XTAL32MSTART_ENA or BLE low power timers then the effect of these need disabling via SYSCON.XTAL32MCTRL before the full control by this register is possible., offset: 0xAC */
  __I  uint32_t ANALOGID;                          /**< Analog Interfaces (PMU and Radio) identity registers, offset: 0xB0 */
  __I  uint32_t RADIOSTATUS;                       /**< All Radio Analog modules status register., offset: 0xB4 */
       uint8_t RESERVED_3[4];
  __IO uint32_t DCBUSCTRL;                         /**< DC Bus can be used during device test and evaluation to give observation of internal signals., offset: 0xBC */
  __IO uint32_t FREQMECTRL;                        /**< Frequency measure register, offset: 0xC0 */
       uint8_t RESERVED_4[4];
  __IO uint32_t NFCTAG_VDD;                        /**< NFCTAG VDD output control, offset: 0xC8 */
  __O  uint32_t SWRESETCTRL;                       /**< Full IC reset request (from Software application)., offset: 0xCC */
} ASYNC_SYSCON_Type;

/* ----------------------------------------------------------------------------
   -- ASYNC_SYSCON Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ASYNC_SYSCON_Register_Masks ASYNC_SYSCON Register Masks
 * @{
 */

/*! @name ASYNCPRESETCTRL - Asynchronous peripherals reset control. The ASYNCPRESETCTRL register allows software to reset specific peripherals attached to the async APB bridge. Writing a zero to any assigned bit in this register clears the reset and allows the specified peripheral to operate. Writing a one asserts the reset. */
/*! @{ */
#define ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B0_MASK (0x2U)
#define ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B0_SHIFT (1U)
/*! CT32B0 - Controls the reset for Counter/Timer CT32B0
 *  0b0..Clear reset to Counter/Timer CT32B0.
 *  0b1..Assert reset to Counter/Timer CT32B0.
 */
#define ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B0(x)   (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B0_SHIFT)) & ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B0_MASK)
#define ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B1_MASK (0x4U)
#define ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B1_SHIFT (2U)
/*! CT32B1 - Controls the reset for Counter/Timer CT32B1
 *  0b0..Clear reset to Counter/Timer CT32B1.
 *  0b1..Assert reset to Counter/Timer CT32B1.
 */
#define ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B1(x)   (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B1_SHIFT)) & ASYNC_SYSCON_ASYNCPRESETCTRL_CT32B1_MASK)
/*! @} */

/*! @name ASYNCPRESETCTRLSET - Set bits in ASYNCPRESETCTRL. Writing ones to this register sets the corresponding bit or bits in the ASYNCPRESETCTRL register, if they are implemented */
/*! @{ */
#define ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B0_MASK (0x2U)
#define ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B0_SHIFT (1U)
/*! CT32B0 - Writing 1 to this register sets the bit ASYNCPRESETCTRL.CT32B0
 *  0b0..No effect.
 *  0b1..Set the bit ASYNCPRESETCTRL.CT32B0.
 */
#define ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B0(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B0_SHIFT)) & ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B0_MASK)
#define ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B1_MASK (0x4U)
#define ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B1_SHIFT (2U)
/*! CT32B1 - Writing 1 to this register sets the bit ASYNCPRESETCTRL.CT32B1
 *  0b0..No effect.
 *  0b1..Set the bit ASYNCPRESETCTRL.CT32B1.
 */
#define ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B1(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B1_SHIFT)) & ASYNC_SYSCON_ASYNCPRESETCTRLSET_CT32B1_MASK)
/*! @} */

/*! @name ASYNCPRESETCTRLCLR - Clear bits in ASYNCPRESETCTRL. Writing ones to this register clears the corresponding bit or bits in the ASYNCPRESETCTRL register, if they are implemented */
/*! @{ */
#define ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B0_MASK (0x2U)
#define ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B0_SHIFT (1U)
/*! CT32B0 - Writing 1 to this register clears the bit ASYNCPRESETCTRL.CT32B0
 *  0b0..No effect.
 *  0b1..Clear the bit ASYNCPRESETCTRL.CT32B0.
 */
#define ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B0(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B0_SHIFT)) & ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B0_MASK)
#define ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B1_MASK (0x4U)
#define ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B1_SHIFT (2U)
/*! CT32B1 - Writing 1 to this register clears the bit ASYNCPRESETCTRL.CT32B1
 *  0b0..No effect.
 *  0b1..Clear the bit ASYNCPRESETCTRL.CT32B1.
 */
#define ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B1(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B1_SHIFT)) & ASYNC_SYSCON_ASYNCPRESETCTRLCLR_CT32B1_MASK)
/*! @} */

/*! @name ASYNCAPBCLKCTRL - Asynchronous peripherals clock control. This register controls how the clock selected for the asynchronous APB peripherals is divided to provide the clock to the asynchronous peripherals */
/*! @{ */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B0_MASK (0x2U)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B0_SHIFT (1U)
/*! CT32B0 - Controls the clock for Counter/Timer CT32B0
 *  0b0..Disable clock to Counter/Timer CT32B0.
 *  0b1..Enable clock to Counter/Timer CT32B0.
 */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B0(x)   (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B0_SHIFT)) & ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B0_MASK)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B1_MASK (0x4U)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B1_SHIFT (2U)
/*! CT32B1 - Controls the clock for Counter/Timer CT32B1
 *  0b0..Disable clock to Counter/Timer CT32B1.
 *  0b1..Enable clock to Counter/Timer CT32B1.
 */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B1(x)   (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B1_SHIFT)) & ASYNC_SYSCON_ASYNCAPBCLKCTRL_CT32B1_MASK)
/*! @} */

/*! @name ASYNCAPBCLKCTRLSET - Set bits in ASYNCAPBCLKCTRL. Writing ones to this register sets the corresponding bit or bits in the ASYNCAPBCLKCTRLSET register, if they are implemented */
/*! @{ */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B0_MASK (0x2U)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B0_SHIFT (1U)
/*! CT32B0 - Writing 1 to this register sets the bit ASYNCAPBCLKCTRL.CT32B0
 *  0b0..No effect.
 *  0b1..Set the bit ASYNCAPBCLKCTRL.CT32B0.
 */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B0(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B0_SHIFT)) & ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B0_MASK)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B1_MASK (0x4U)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B1_SHIFT (2U)
/*! CT32B1 - Writing 1 to this register sets the bit ASYNCAPBCLKCTRL.CT32B1
 *  0b0..No effect.
 *  0b1..Set the bit ASYNCAPBCLKCTRL.CT32B1.
 */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B1(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B1_SHIFT)) & ASYNC_SYSCON_ASYNCAPBCLKCTRLSET_CT32B1_MASK)
/*! @} */

/*! @name ASYNCAPBCLKCTRLCLR - Clear bits in ASYNCAPBCLKCTRL. Writing ones to this register sets the corresponding bit or bits in the ASYNCAPBCLKCTRLSET register, if they are implemented */
/*! @{ */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B0_MASK (0x2U)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B0_SHIFT (1U)
/*! CT32B0 - Writing 1 to this register clears the bit ASYNCAPBCLKCTRL.CT32B0
 *  0b0..No effect.
 *  0b1..Clear the ASYNCAPBCLKCTRL.CT32B0.
 */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B0(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B0_SHIFT)) & ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B0_MASK)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B1_MASK (0x4U)
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B1_SHIFT (2U)
/*! CT32B1 - Writing 1 to this register clears the bit ASYNCAPBCLKCTRL.CT32B1
 *  0b0..No effect.
 *  0b1..Clear the bit ASYNCAPBCLKCTRL.CT32B1.
 */
#define ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B1(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B1_SHIFT)) & ASYNC_SYSCON_ASYNCAPBCLKCTRLCLR_CT32B1_MASK)
/*! @} */

/*! @name ASYNCAPBCLKSELA - Asynchronous APB clock source select */
/*! @{ */
#define ASYNC_SYSCON_ASYNCAPBCLKSELA_SEL_MASK    (0x3U)
#define ASYNC_SYSCON_ASYNCAPBCLKSELA_SEL_SHIFT   (0U)
/*! SEL - Clock source for modules beyond asynchronous Bus bridge: ASYNC_SYSCON itself, timers 0/1.
 *  0b00..System Bus clock.
 *  0b01..32 MHz crystal oscillator (XTAL32M).
 *  0b10..32 MHz free running oscillator (FRO32M).
 *  0b11..48 MHz free running oscillator (FRO48M).
 */
#define ASYNC_SYSCON_ASYNCAPBCLKSELA_SEL(x)      (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ASYNCAPBCLKSELA_SEL_SHIFT)) & ASYNC_SYSCON_ASYNCAPBCLKSELA_SEL_MASK)
/*! @} */

/*! @name TEMPSENSORCTRL - Temperature Sensor controls */
/*! @{ */
#define ASYNC_SYSCON_TEMPSENSORCTRL_ENABLE_MASK  (0x1U)
#define ASYNC_SYSCON_TEMPSENSORCTRL_ENABLE_SHIFT (0U)
/*! ENABLE - Temperature sensor enable
 */
#define ASYNC_SYSCON_TEMPSENSORCTRL_ENABLE(x)    (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_TEMPSENSORCTRL_ENABLE_SHIFT)) & ASYNC_SYSCON_TEMPSENSORCTRL_ENABLE_MASK)
#define ASYNC_SYSCON_TEMPSENSORCTRL_CM_MASK      (0xCU)
#define ASYNC_SYSCON_TEMPSENSORCTRL_CM_SHIFT     (2U)
/*! CM - Temerature sensor common mode output voltage selection: 0x0: high negative offset added;
 *    0x1: intermediate negative offset added; 0x2: no offset added; 0x3: low positive offset added.
 *    Only setting 0x2 should be used.
 */
#define ASYNC_SYSCON_TEMPSENSORCTRL_CM(x)        (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_TEMPSENSORCTRL_CM_SHIFT)) & ASYNC_SYSCON_TEMPSENSORCTRL_CM_MASK)
/*! @} */

/*! @name NFCTAGPADSCTRL - NFC Tag pads control for I2C interface to internal NFC Tag (T parts only): I2C interface + 1 interrupt/ field detect input pad + NTAG VDD output pad */
/*! @{ */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPD_MASK (0x1U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPD_SHIFT (0U)
/*! I2C_SDA_EPD - I2C_SDA, Enable weak pull down on IO pad. 0= disabled. 1=pull down enabled.
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPD(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPD_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPD_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPUN_MASK (0x2U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPUN_SHIFT (1U)
/*! I2C_SDA_EPUN - I2C_SDA, Enable weak pull up on IO pad, active low. 0=pullup enabled. 1=pullup disabled
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPUN(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPUN_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EPUN_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS0_MASK (0x4U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS0_SHIFT (2U)
/*! I2C_SDA_EHS0 - I2C_SDA IO Driver slew rate LSB. (I2C_SDA_EHS1, I2C_SDA_EHS0). RESERVED: use default value (0)
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS0(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS0_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS0_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_INVERT_MASK (0x8U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_INVERT_SHIFT (3U)
/*! I2C_SDA_INVERT - I2C_SDA Input polarity. 0 : Input function is not inverted. 1 : Input function is inverted.
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_INVERT(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_INVERT_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_INVERT_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_ENZI_MASK (0x10U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_ENZI_SHIFT (4U)
/*! I2C_SDA_ENZI - I2C_SDA Receiver enable, active high
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_ENZI(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_ENZI_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_ENZI_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_FILTEROFF_MASK (0x20U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_FILTEROFF_SHIFT (5U)
/*! I2C_SDA_FILTEROFF - I2C_SDA input glitch filter control. 0 Filter enabled. Short noise pulses
 *    are filtered out. 1 Filter disabled. No input filtering is done.
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_FILTEROFF(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_FILTEROFF_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_FILTEROFF_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS1_MASK (0x40U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS1_SHIFT (6U)
/*! I2C_SDA_EHS1 - I2C_SDA IO Driver slew rate MSB. (I2C_SDA_EHS1, I2C_SDA_EHS0). RESERVED: use default value (0)
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS1(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS1_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_EHS1_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_OD_MASK (0x80U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_OD_SHIFT (7U)
/*! I2C_SDA_OD - I2C_SDA, Controls open-drain mode. 0 Normal. Normal push-pull output 1 Open-drain.
 *    Simulated open-drain output (high drive disabled).
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_OD(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_OD_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SDA_OD_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPD_MASK (0x100U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPD_SHIFT (8U)
/*! I2C_SCL_EPD - I2C_SCL, Enable weak pull down on IO pad. 0: disabled; 1: pull down enabled.
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPD(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPD_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPD_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPUN_MASK (0x200U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPUN_SHIFT (9U)
/*! I2C_SCL_EPUN - I2C_SCL, Enable weak pull up on IO pad, active low. 0: pullup enabled; 1: pullup disabled.
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPUN(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPUN_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EPUN_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS0_MASK (0x400U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS0_SHIFT (10U)
/*! I2C_SCL_EHS0 - I2C_SCL IO Driver slew rate LSB. (I2C_SCL_EHS1, I2C_SCL_EHS0). RESERVED: use default value (0)
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS0(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS0_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS0_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_ENZI_MASK (0x1000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_ENZI_SHIFT (12U)
/*! I2C_SCL_ENZI - I2C_SCL Receiver enable, active high
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_ENZI(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_ENZI_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_ENZI_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_FILTEROFF_MASK (0x2000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_FILTEROFF_SHIFT (13U)
/*! I2C_SCL_FILTEROFF - I2C_SCL, input glitch filter control: 0: Filter enabled. Short noise pulses
 *    are filtered out; 1: Filter disabled. No input filtering is done.
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_FILTEROFF(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_FILTEROFF_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_FILTEROFF_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS1_MASK (0x4000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS1_SHIFT (14U)
/*! I2C_SCL_EHS1 - I2C_SCL IO Driver slew rate MSB. (I2C_SCL_EHS1, I2C_SCL_EHS0). RESERVED: use default value (0)
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS1(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS1_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_EHS1_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_OD_MASK (0x8000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_OD_SHIFT (15U)
/*! I2C_SCL_OD - I2C_SCL open-drain mode control: 0: Normal. Normal push-pull output; 1: Open-drain.
 *    Simulated open-drain output (high drive disabled).
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_OD(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_OD_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_I2C_SCL_OD_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_INVERT_MASK (0x40000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_INVERT_SHIFT (18U)
/*! INT_INVERT - NTAG INT/FD Input polarity: 0: Input function is not inverted; 1: Input function is inverted.
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_INVERT(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_INT_INVERT_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_INT_INVERT_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_ENZI_MASK (0x80000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_ENZI_SHIFT (19U)
/*! INT_ENZI - Reserved. NTAG INT/FD IO cell always enabled, not configurable
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_ENZI(x)  (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_INT_ENZI_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_INT_ENZI_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_FILTEROFF_MASK (0x100000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_FILTEROFF_SHIFT (20U)
/*! INT_FILTEROFF - Reserved. NTAG INT/FD IO cell always filters signal, not configurable
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_INT_FILTEROFF(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_INT_FILTEROFF_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_INT_FILTEROFF_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPD_MASK (0x200000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPD_SHIFT (21U)
/*! VDD_EPD - NTAG VDD, Enable weak pull down on IO pad
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPD(x)   (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPD_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPD_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPUN_MASK (0x400000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPUN_SHIFT (22U)
/*! VDD_EPUN - NTAG_VDD, Enable weak pull up on IO pad, active low
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPUN(x)  (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPUN_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EPUN_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS0_MASK (0x800000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS0_SHIFT (23U)
/*! VDD_EHS0 - NTAG VDD IO Driver slew rate LSB. (VDD_EHS1, VDD_EHS0) sets IO cell speed when
 *    enabled as an output. 00=Low speed, 01=nominal speed, 10=fast speed, 11=high speed. Recommendation
 *    is to use default value (0)
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS0(x)  (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS0_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS0_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS1_MASK (0x8000000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS1_SHIFT (27U)
/*! VDD_EHS1 - NTAG VDD IO Driver slew rate MSB. (VDD_EHS1, VDD_EHS0) sets IO cell speed when
 *    enabled as an output. 00: Low speed; 01: nominal speed; 10: fast speed; 11: high speed.
 *    Recommendation is to use default value (0)
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS1(x)  (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS1_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_EHS1_MASK)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_OD_MASK  (0x10000000U)
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_OD_SHIFT (28U)
/*! VDD_OD - NTAG VDD open-drain mode control: 0: Normal. Normal push-pull output; 1: Open-drain.
 *    Simulated open-drain output (high drive disabled).
 */
#define ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_OD(x)    (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_OD_SHIFT)) & ASYNC_SYSCON_NFCTAGPADSCTRL_VDD_OD_MASK)
/*! @} */

/*! @name XTAL32MLDOCTRL - XTAL 32 MHz LDO control register. If XTAL has been auto started due to EFUSE XTAL32MSTART_ENA or BLE low power timers then the effect of these need disabling via SYSCON.XTAL32MCTRL before the full control by this register is possible. */
/*! @{ */
#define ASYNC_SYSCON_XTAL32MLDOCTRL_ENABLE_MASK  (0x2U)
#define ASYNC_SYSCON_XTAL32MLDOCTRL_ENABLE_SHIFT (1U)
/*! ENABLE - Enable the LDO when set. Setting managed by software API.
 */
#define ASYNC_SYSCON_XTAL32MLDOCTRL_ENABLE(x)    (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MLDOCTRL_ENABLE_SHIFT)) & ASYNC_SYSCON_XTAL32MLDOCTRL_ENABLE_MASK)
#define ASYNC_SYSCON_XTAL32MLDOCTRL_VOUT_MASK    (0x38U)
#define ASYNC_SYSCON_XTAL32MLDOCTRL_VOUT_SHIFT   (3U)
/*! VOUT - Adjust the output voltage level, setting managed by software API.
 */
#define ASYNC_SYSCON_XTAL32MLDOCTRL_VOUT(x)      (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MLDOCTRL_VOUT_SHIFT)) & ASYNC_SYSCON_XTAL32MLDOCTRL_VOUT_MASK)
#define ASYNC_SYSCON_XTAL32MLDOCTRL_IBIAS_MASK   (0xC0U)
#define ASYNC_SYSCON_XTAL32MLDOCTRL_IBIAS_SHIFT  (6U)
/*! IBIAS - Adjust the biasing current, setting managed by software API.
 */
#define ASYNC_SYSCON_XTAL32MLDOCTRL_IBIAS(x)     (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MLDOCTRL_IBIAS_SHIFT)) & ASYNC_SYSCON_XTAL32MLDOCTRL_IBIAS_MASK)
#define ASYNC_SYSCON_XTAL32MLDOCTRL_STABMODE_MASK (0x300U)
#define ASYNC_SYSCON_XTAL32MLDOCTRL_STABMODE_SHIFT (8U)
/*! STABMODE - Stability configuration, only required for test purposes.
 */
#define ASYNC_SYSCON_XTAL32MLDOCTRL_STABMODE(x)  (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MLDOCTRL_STABMODE_SHIFT)) & ASYNC_SYSCON_XTAL32MLDOCTRL_STABMODE_MASK)
/*! @} */

/*! @name XTAL32MCTRL - XTAL 32 MHz control register. If XTAL has been auto started due to EFUSE XTAL32MSTART_ENA or BLE low power timers then the effect of these need disabling via SYSCON.XTAL32MCTRL before the full control by this register is possible. */
/*! @{ */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_MASK (0x1U)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_SHIFT (0U)
/*! XO_ACBUF_PASS_ENABLE - Bypass enable of XTAL AC buffer enable in pll and top level. Setting managed by software API.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO_ACBUF_PASS_ENABLE_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_AMP_MASK     (0xEU)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_AMP_SHIFT    (1U)
/*! XO_AMP - Amplitude selection , Min amp: 001, Max amp: 110. Setting managed by software API.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_AMP(x)       (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO_AMP_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO_AMP_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN_MASK (0x7F0U)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN_SHIFT (4U)
/*! XO_OSC_CAP_IN - Internal Capacitor Selection for XTAL_P. Each XTAL pin has a capacitance value
 *    up to approximately 25pF. Device test calibration data is stored on chip so that a software
 *    function can configure a capacitiance with high accuracy.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_IN_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT_MASK (0x3F800U)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT_SHIFT (11U)
/*! XO_OSC_CAP_OUT - Internal Capacitor Selection for XTAL_N. Each XTAL pin has a capacitance value
 *    up to approximately 25pF. Device test calibration data is stored on chip so that a software
 *    function can configure a capacitiance with high accuracy.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO_OSC_CAP_OUT_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE_MASK  (0x400000U)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE_SHIFT (22U)
/*! XO_ENABLE - Enable signal for 32MHz XTAL.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE(x)    (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO_ENABLE_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_GM_MASK      (0x3800000U)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_GM_SHIFT     (23U)
/*! XO_GM - Gm value for XTAL.. Setting managed by software API.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_GM(x)        (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO_GM_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO_GM_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_SLAVE_MASK   (0x4000000U)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_SLAVE_SHIFT  (26U)
/*! XO_SLAVE - XTAL in slave mode. Setting managed by software API.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_SLAVE(x)     (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO_SLAVE_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO_SLAVE_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_STANDALONE_ENABLE_MASK (0x8000000U)
#define ASYNC_SYSCON_XTAL32MCTRL_XO_STANDALONE_ENABLE_SHIFT (27U)
/*! XO_STANDALONE_ENABLE - Selection of the LDO and core XO reference biasing sources (1uA bandgap
 *    current and 0.6V bandgap voltage): 0: biasing provided by radio reference biasing sources.
 *    Don't switch to this value without prior radio biasing, LDO XO, core XO, LDO 1.4V and PLL current
 *    distribution enabling. 1: biasing is provided by Power Management Unit (PMU). Control of this
 *    bit is generally managed by the radio or clock software functions.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO_STANDALONE_ENABLE(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO_STANDALONE_ENABLE_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO_STANDALONE_ENABLE_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_XO32M_TO_MCU_ENABLE_MASK (0x10000000U)
#define ASYNC_SYSCON_XTAL32MCTRL_XO32M_TO_MCU_ENABLE_SHIFT (28U)
/*! XO32M_TO_MCU_ENABLE - Enable the 32MHz clock to MCU and the clock generators.
 */
#define ASYNC_SYSCON_XTAL32MCTRL_XO32M_TO_MCU_ENABLE(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_XO32M_TO_MCU_ENABLE_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_XO32M_TO_MCU_ENABLE_MASK)
#define ASYNC_SYSCON_XTAL32MCTRL_CLK_TO_GPADC_ENABLE_MASK (0x20000000U)
#define ASYNC_SYSCON_XTAL32MCTRL_CLK_TO_GPADC_ENABLE_SHIFT (29U)
/*! CLK_TO_GPADC_ENABLE - Enable the 16MHz clock to General Purpose ADC
 */
#define ASYNC_SYSCON_XTAL32MCTRL_CLK_TO_GPADC_ENABLE(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_XTAL32MCTRL_CLK_TO_GPADC_ENABLE_SHIFT)) & ASYNC_SYSCON_XTAL32MCTRL_CLK_TO_GPADC_ENABLE_MASK)
/*! @} */

/*! @name ANALOGID - Analog Interfaces (PMU and Radio) identity registers */
/*! @{ */
#define ASYNC_SYSCON_ANALOGID_PMUID_MASK         (0x3FU)
#define ASYNC_SYSCON_ANALOGID_PMUID_SHIFT        (0U)
/*! PMUID - PMU Identitty register used ti indicate a version of the PMU.
 */
#define ASYNC_SYSCON_ANALOGID_PMUID(x)           (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_ANALOGID_PMUID_SHIFT)) & ASYNC_SYSCON_ANALOGID_PMUID_MASK)
/*! @} */

/*! @name RADIOSTATUS - All Radio Analog modules status register. */
/*! @{ */
#define ASYNC_SYSCON_RADIOSTATUS_PLLXOREADY_MASK (0x1U)
#define ASYNC_SYSCON_RADIOSTATUS_PLLXOREADY_SHIFT (0U)
/*! PLLXOREADY - Value of status output by 32M XTAL oscillator. Aserted to indicate that the clock
 *    is active. Note that the quality of the 32M clock may improve even after this is asserted.
 *    Additionally, if settings are changed such as ibias control then this status flag will probably
 *    remain asserted even though changes to the clock signal will be occuring,
 */
#define ASYNC_SYSCON_RADIOSTATUS_PLLXOREADY(x)   (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_RADIOSTATUS_PLLXOREADY_SHIFT)) & ASYNC_SYSCON_RADIOSTATUS_PLLXOREADY_MASK)
/*! @} */

/*! @name DCBUSCTRL - DC Bus can be used during device test and evaluation to give observation of internal signals. */
/*! @{ */
#define ASYNC_SYSCON_DCBUSCTRL_ADDR_MASK         (0x1FFU)
#define ASYNC_SYSCON_DCBUSCTRL_ADDR_SHIFT        (0U)
/*! ADDR - ADDR[8] should be set to 1 before entering power down to prevent the risk of a small
 *    amount of leakage current during power down.
 */
#define ASYNC_SYSCON_DCBUSCTRL_ADDR(x)           (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_DCBUSCTRL_ADDR_SHIFT)) & ASYNC_SYSCON_DCBUSCTRL_ADDR_MASK)
/*! @} */

/*! @name FREQMECTRL - Frequency measure register */
/*! @{ */
#define ASYNC_SYSCON_FREQMECTRL_CAPVAL_MASK      (0x7FFFFFFFU)
#define ASYNC_SYSCON_FREQMECTRL_CAPVAL_SHIFT     (0U)
/*! CAPVAL - Frequency Measure control and status; the function differs for a read and write
 *    operation. CAPVAL: FREQMECTRL[30:0] (Read-only) : Stores the target counter result from the last
 *    frequency measure activiation, this is used in the calculation of the unknown clock frequency of
 *    the reference or target clock. SCALE: FREQMECTRL[4:0] (Write-only) : define the count duration,
 *    (2^SCALE)-1, that reference counter counts to during measurement. Note that the value is 2
 *    giving a minimum count 2^2-1 = 3. The result of freq_me_plus can be calculated as follows :
 *    freq_targetclk =freq_refclk* (CAPVAL+1) / ((2^SCALE)-1);
 */
#define ASYNC_SYSCON_FREQMECTRL_CAPVAL(x)        (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_FREQMECTRL_CAPVAL_SHIFT)) & ASYNC_SYSCON_FREQMECTRL_CAPVAL_MASK)
#define ASYNC_SYSCON_FREQMECTRL_PROG_MASK        (0x80000000U)
#define ASYNC_SYSCON_FREQMECTRL_PROG_SHIFT       (31U)
/*! PROG - Set this bit to one to initiate a frequency measurement cycle. Hardware clears this bit
 *    when the measurement cycle has completed and there is valid capture data in the CAPVAL field
 *    (bits 13:0).
 */
#define ASYNC_SYSCON_FREQMECTRL_PROG(x)          (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_FREQMECTRL_PROG_SHIFT)) & ASYNC_SYSCON_FREQMECTRL_PROG_MASK)
/*! @} */

/*! @name NFCTAG_VDD - NFCTAG VDD output control */
/*! @{ */
#define ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OUT_MASK (0x1U)
#define ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OUT_SHIFT (0U)
/*! NFCTAG_VDD_OUT - Output value for the NFC Tag Vdd IO, if enabled with NFCTAG_VDD_OE.
 */
#define ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OUT(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OUT_SHIFT)) & ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OUT_MASK)
#define ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OE_MASK (0x2U)
#define ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OE_SHIFT (1U)
/*! NFCTAG_VDD_OE - Output enable for the NFC Tag Vdd IO cell.
 */
#define ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OE(x) (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OE_SHIFT)) & ASYNC_SYSCON_NFCTAG_VDD_NFCTAG_VDD_OE_MASK)
/*! @} */

/*! @name SWRESETCTRL - Full IC reset request (from Software application). */
/*! @{ */
#define ASYNC_SYSCON_SWRESETCTRL_ICRESETREQ_MASK (0x1U)
#define ASYNC_SYSCON_SWRESETCTRL_ICRESETREQ_SHIFT (0U)
/*! ICRESETREQ - IC reset request. This bit is only valid if VECTKEY is set correctly. 0: No effect;
 *    1: Request a fulll IC reset level reset
 */
#define ASYNC_SYSCON_SWRESETCTRL_ICRESETREQ(x)   (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_SWRESETCTRL_ICRESETREQ_SHIFT)) & ASYNC_SYSCON_SWRESETCTRL_ICRESETREQ_MASK)
#define ASYNC_SYSCON_SWRESETCTRL_VECTKEY_MASK    (0xFFFF0000U)
#define ASYNC_SYSCON_SWRESETCTRL_VECTKEY_SHIFT   (16U)
/*! VECTKEY - Register Key: On write, write 0x05FA to VECTKEY, otherwise the write is ignored.
 */
#define ASYNC_SYSCON_SWRESETCTRL_VECTKEY(x)      (((uint32_t)(((uint32_t)(x)) << ASYNC_SYSCON_SWRESETCTRL_VECTKEY_SHIFT)) & ASYNC_SYSCON_SWRESETCTRL_VECTKEY_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group ASYNC_SYSCON_Register_Masks */


/* ASYNC_SYSCON - Peripheral instance base addresses */
/** Peripheral ASYNC_SYSCON base address */
#define ASYNC_SYSCON_BASE                        (0x40020000u)
/** Peripheral ASYNC_SYSCON base pointer */
#define ASYNC_SYSCON                             ((ASYNC_SYSCON_Type *)ASYNC_SYSCON_BASE)
/** Array initializer of ASYNC_SYSCON peripheral base addresses */
#define ASYNC_SYSCON_BASE_ADDRS                  { ASYNC_SYSCON_BASE }
/** Array initializer of ASYNC_SYSCON peripheral base pointers */
#define ASYNC_SYSCON_BASE_PTRS                   { ASYNC_SYSCON }

/*!
 * @}
 */ /* end of group ASYNC_SYSCON_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- BLE_DP_TOP Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup BLE_DP_TOP_Peripheral_Access_Layer BLE_DP_TOP Peripheral Access Layer
 * @{
 */

/** BLE_DP_TOP - Register Layout Typedef */
typedef struct {
       uint8_t RESERVED_0[176];
  __IO uint32_t ANT_DIVERSITY;                     /**< Antenna diversity, offset: 0xB0 */
} BLE_DP_TOP_Type;

/* ----------------------------------------------------------------------------
   -- BLE_DP_TOP Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup BLE_DP_TOP_Register_Masks BLE_DP_TOP Register Masks
 * @{
 */

/*! @name ANT_DIVERSITY - Antenna diversity */
/*! @{ */
#define BLE_DP_TOP_ANT_DIVERSITY_ble_ant_selected_MASK (0x1U)
#define BLE_DP_TOP_ANT_DIVERSITY_ble_ant_selected_SHIFT (0U)
/*! ble_ant_selected - Selection of antenna when selection mode is direct from register, see
 *    ble_ant_mode. 0: ADE is asserted. 1: ADE de-asserted. ADO is always the inverse of ADE and so is also
 *    controlled by this setting as well.
 */
#define BLE_DP_TOP_ANT_DIVERSITY_ble_ant_selected(x) (((uint32_t)(((uint32_t)(x)) << BLE_DP_TOP_ANT_DIVERSITY_ble_ant_selected_SHIFT)) & BLE_DP_TOP_ANT_DIVERSITY_ble_ant_selected_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group BLE_DP_TOP_Register_Masks */


/* BLE_DP_TOP - Peripheral instance base addresses */
/** Peripheral BLE_DP_TOP base address */
#define BLE_DP_TOP_BASE                          (0x40014000u)
/** Peripheral BLE_DP_TOP base pointer */
#define BLE_DP_TOP                               ((BLE_DP_TOP_Type *)BLE_DP_TOP_BASE)
/** Array initializer of BLE_DP_TOP peripheral base addresses */
#define BLE_DP_TOP_BASE_ADDRS                    { BLE_DP_TOP_BASE }
/** Array initializer of BLE_DP_TOP peripheral base pointers */
#define BLE_DP_TOP_BASE_PTRS                     { BLE_DP_TOP }

/*!
 * @}
 */ /* end of group BLE_DP_TOP_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- CIC_IRB Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CIC_IRB_Peripheral_Access_Layer CIC_IRB Peripheral Access Layer
 * @{
 */

/** CIC_IRB - Register Layout Typedef */
typedef struct {
  __IO uint32_t CONF;                              /**< IR Blaster configuration, offset: 0x0 */
  __IO uint32_t CARRIER;                           /**< IR Blaster carrier configuration, offset: 0x4 */
  __IO uint32_t FIFO_IN;                           /**< IR Blaster Envelope FIFO input, offset: 0x8 */
  __I  uint32_t STATUS;                            /**< IR Blaster Status, offset: 0xC */
  __O  uint32_t CMD;                               /**< IR Blaster Commands, offset: 0x10 */
       uint8_t RESERVED_0[4044];
  __I  uint32_t INT_STATUS;                        /**< Interrupt Status, offset: 0xFE0 */
  __IO uint32_t INT_ENA;                           /**< Interrupt Enable, offset: 0xFE4 */
  __O  uint32_t INT_CLR;                           /**< Interrupt Clear, offset: 0xFE8 */
  __O  uint32_t INT_SET;                           /**< Interrupt Set, offset: 0xFEC */
       uint8_t RESERVED_1[12];
  __I  uint32_t MODULE_ID;                         /**< IR Blaster Module Identifier, offset: 0xFFC */
} CIC_IRB_Type;

/* ----------------------------------------------------------------------------
   -- CIC_IRB Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CIC_IRB_Register_Masks CIC_IRB Register Masks
 * @{
 */

/*! @name CONF - IR Blaster configuration */
/*! @{ */
#define CIC_IRB_CONF_ENV_INI_MASK                (0x1U)
#define CIC_IRB_CONF_ENV_INI_SHIFT               (0U)
/*! ENV_INI - Initial envelope value. This is the level of the first envelope after IR Blaster start or restart.
 *  0b0..First envelope will be a low level
 *  0b1..First envelope will be a high level
 */
#define CIC_IRB_CONF_ENV_INI(x)                  (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CONF_ENV_INI_SHIFT)) & CIC_IRB_CONF_ENV_INI_MASK)
#define CIC_IRB_CONF_MODE_MASK                   (0x2U)
#define CIC_IRB_CONF_MODE_SHIFT                  (1U)
/*! MODE - Blaster mode
 *  0b0..Normal mode. IR Blaster will stop when it encouter an envelope with ENV_LAST bit = '1'
 *  0b1..Automatic restart. IR Blaster will transmit all envelopes and stop only when FIFO becames empty. ENV_LAST
 *       bit only generates an interrupt but doesn't stop transmission.
 */
#define CIC_IRB_CONF_MODE(x)                     (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CONF_MODE_SHIFT)) & CIC_IRB_CONF_MODE_MASK)
#define CIC_IRB_CONF_OUT_MASK                    (0xCU)
#define CIC_IRB_CONF_OUT_SHIFT                   (2U)
/*! OUT - Output logic function
 *  0b00..envelope AND carrier
 *  0b01..envelope OR carrier
 *  0b10..envelope NAND carrier
 *  0b11..envelope NOR carrier
 */
#define CIC_IRB_CONF_OUT(x)                      (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CONF_OUT_SHIFT)) & CIC_IRB_CONF_OUT_MASK)
#define CIC_IRB_CONF_NO_CAR_MASK                 (0x10U)
#define CIC_IRB_CONF_NO_CAR_SHIFT                (4U)
/*! NO_CAR - No carrier
 *  0b0..Normal. IR_OUT = envelope + carrier
 *  0b1..Carrier is inhibited. IR_OUT = envelope only
 */
#define CIC_IRB_CONF_NO_CAR(x)                   (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CONF_NO_CAR_SHIFT)) & CIC_IRB_CONF_NO_CAR_MASK)
#define CIC_IRB_CONF_CAR_INI_MASK                (0x20U)
#define CIC_IRB_CONF_CAR_INI_SHIFT               (5U)
/*! CAR_INI - Initial carrier value.
 *  0b0..Carrier starts with '0' (the carrier is low during CHIGH[1:0] and high during CLOW[2:0])
 *  0b1..Carrier starts with '1' (the carrier is high during CHIGH[1:0] and low during CLOW[2:0])
 */
#define CIC_IRB_CONF_CAR_INI(x)                  (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CONF_CAR_INI_SHIFT)) & CIC_IRB_CONF_CAR_INI_MASK)
/*! @} */

/*! @name CARRIER - IR Blaster carrier configuration */
/*! @{ */
#define CIC_IRB_CARRIER_CTU_MASK                 (0xFFFFU)
#define CIC_IRB_CARRIER_CTU_SHIFT                (0U)
/*! CTU - Carrier Time Unit (CTU) CTU = CTU * TIRCP, TIRCP = IR module clock period = 1/48MHz. Value
 *    0x0 is equivalent to 0x1. It is recommended to modify this field when the blaster unit is
 *    disable (i.e when ENA_ST = '0' in STATUS register)
 */
#define CIC_IRB_CARRIER_CTU(x)                   (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CARRIER_CTU_SHIFT)) & CIC_IRB_CARRIER_CTU_MASK)
#define CIC_IRB_CARRIER_CLOW_MASK                (0x70000U)
#define CIC_IRB_CARRIER_CLOW_SHIFT               (16U)
/*! CLOW - Carrier low period. Carrier low level duration = (CLOW + 1) * CTU. It is recommended to
 *    modify this field when the blaster unit is disable (i.e when ENA_ST = '0' in STATUS register)
 */
#define CIC_IRB_CARRIER_CLOW(x)                  (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CARRIER_CLOW_SHIFT)) & CIC_IRB_CARRIER_CLOW_MASK)
#define CIC_IRB_CARRIER_CHIGH_MASK               (0x180000U)
#define CIC_IRB_CARRIER_CHIGH_SHIFT              (19U)
/*! CHIGH - Carrier high period Carrier high level duration = (CHIGH + 1) * CTU. It is recommended
 *    to modify this field when the blaster unit is disable (i.e when ENA_ST = '0' in STATUS register)
 */
#define CIC_IRB_CARRIER_CHIGH(x)                 (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CARRIER_CHIGH_SHIFT)) & CIC_IRB_CARRIER_CHIGH_MASK)
/*! @} */

/*! @name FIFO_IN - IR Blaster Envelope FIFO input */
/*! @{ */
#define CIC_IRB_FIFO_IN_ENV_MASK                 (0xFFFU)
#define CIC_IRB_FIFO_IN_ENV_SHIFT                (0U)
/*! ENV - Envelope duration expressed in carrier period number. Tenvelope = ENV * (CHIGH + CLOW + 2
 *    ) * CTU. Value 0x000 has the same behaviour has value 0x001.
 */
#define CIC_IRB_FIFO_IN_ENV(x)                   (((uint32_t)(((uint32_t)(x)) << CIC_IRB_FIFO_IN_ENV_SHIFT)) & CIC_IRB_FIFO_IN_ENV_MASK)
#define CIC_IRB_FIFO_IN_ENV_INT_MASK             (0x1000U)
#define CIC_IRB_FIFO_IN_ENV_INT_SHIFT            (12U)
/*! ENV_INT - Generate an interrupt when starting emission of the envelope
 *  0b0..Don't generate interrupt
 *  0b1..Generate interrupt
 */
#define CIC_IRB_FIFO_IN_ENV_INT(x)               (((uint32_t)(((uint32_t)(x)) << CIC_IRB_FIFO_IN_ENV_INT_SHIFT)) & CIC_IRB_FIFO_IN_ENV_INT_MASK)
#define CIC_IRB_FIFO_IN_ENV_LAST_MASK            (0x2000U)
#define CIC_IRB_FIFO_IN_ENV_LAST_SHIFT           (13U)
/*! ENV_LAST - Last envelope.
 *  0b0..IR Blaster loads the next envelope when this envelope finishes.
 *  0b1..IR Blaster stops and generates an interrupt when this envelope is completly transmitted. If MODE = '1',
 *       then IR Blaster only generates an interrupt
 */
#define CIC_IRB_FIFO_IN_ENV_LAST(x)              (((uint32_t)(((uint32_t)(x)) << CIC_IRB_FIFO_IN_ENV_LAST_SHIFT)) & CIC_IRB_FIFO_IN_ENV_LAST_MASK)
/*! @} */

/*! @name STATUS - IR Blaster Status */
/*! @{ */
#define CIC_IRB_STATUS_FIFO_LVL_MASK             (0x1FU)
#define CIC_IRB_STATUS_FIFO_LVL_SHIFT            (0U)
/*! FIFO_LVL - Current IR Blaster FIFO level
 *  0b00000..FIFO is empty
 *  0b10000..FIFO is full
 */
#define CIC_IRB_STATUS_FIFO_LVL(x)               (((uint32_t)(((uint32_t)(x)) << CIC_IRB_STATUS_FIFO_LVL_SHIFT)) & CIC_IRB_STATUS_FIFO_LVL_MASK)
#define CIC_IRB_STATUS_FIFO_FULL_MASK            (0x20U)
#define CIC_IRB_STATUS_FIFO_FULL_SHIFT           (5U)
/*! FIFO_FULL - IR Blaster FIFO full flag
 *  0b0..FIFO is not full
 *  0b1..FIFO is full (FIFO level = 100000)
 */
#define CIC_IRB_STATUS_FIFO_FULL(x)              (((uint32_t)(((uint32_t)(x)) << CIC_IRB_STATUS_FIFO_FULL_SHIFT)) & CIC_IRB_STATUS_FIFO_FULL_MASK)
#define CIC_IRB_STATUS_FIFO_EMPTY_MASK           (0x40U)
#define CIC_IRB_STATUS_FIFO_EMPTY_SHIFT          (6U)
/*! FIFO_EMPTY - IR Blaster FIFO empty flag
 *  0b0..FIFO is not empty
 *  0b1..FIFO is empty (FIFO level = 000000)
 */
#define CIC_IRB_STATUS_FIFO_EMPTY(x)             (((uint32_t)(((uint32_t)(x)) << CIC_IRB_STATUS_FIFO_EMPTY_SHIFT)) & CIC_IRB_STATUS_FIFO_EMPTY_MASK)
#define CIC_IRB_STATUS_ENA_ST_MASK               (0x80U)
#define CIC_IRB_STATUS_ENA_ST_SHIFT              (7U)
/*! ENA_ST - IR Blaster status
 *  0b0..IR Blaster is disabled
 *  0b1..IR Blaster is enabled
 */
#define CIC_IRB_STATUS_ENA_ST(x)                 (((uint32_t)(((uint32_t)(x)) << CIC_IRB_STATUS_ENA_ST_SHIFT)) & CIC_IRB_STATUS_ENA_ST_MASK)
#define CIC_IRB_STATUS_RUN_ST_MASK               (0x100U)
#define CIC_IRB_STATUS_RUN_ST_SHIFT              (8U)
/*! RUN_ST - IR Blaster run status
 *  0b0..IR Blaster is not running. Either transmission has not yet been started or it is finished.
 *  0b1..IR Blaster is running.
 */
#define CIC_IRB_STATUS_RUN_ST(x)                 (((uint32_t)(((uint32_t)(x)) << CIC_IRB_STATUS_RUN_ST_SHIFT)) & CIC_IRB_STATUS_RUN_ST_MASK)
/*! @} */

/*! @name CMD - IR Blaster Commands */
/*! @{ */
#define CIC_IRB_CMD_ENA_MASK                     (0x1U)
#define CIC_IRB_CMD_ENA_SHIFT                    (0U)
/*! ENA - Enable IR Blaster. This bit is self clearing.
 *  0b0..No effect
 *  0b1..Enable IR Blaster
 */
#define CIC_IRB_CMD_ENA(x)                       (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CMD_ENA_SHIFT)) & CIC_IRB_CMD_ENA_MASK)
#define CIC_IRB_CMD_DIS_MASK                     (0x2U)
#define CIC_IRB_CMD_DIS_SHIFT                    (1U)
/*! DIS - Disable IR Blaster. This bit is self clearing.
 *  0b0..No effect
 *  0b1..Disable IR Blaster. The transmission of envelopes is immediatly stopped. The FIFO is not reinitialized
 *       (the content of the FIFO is conserved).
 */
#define CIC_IRB_CMD_DIS(x)                       (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CMD_DIS_SHIFT)) & CIC_IRB_CMD_DIS_MASK)
#define CIC_IRB_CMD_START_MASK                   (0x4U)
#define CIC_IRB_CMD_START_SHIFT                  (2U)
/*! START - Start IR Blaster. This bit is self clearing.
 *  0b0..No effect
 *  0b1..Start transmission Before setting this field, the blaster must be enable (the bit field ENA must be set first).
 */
#define CIC_IRB_CMD_START(x)                     (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CMD_START_SHIFT)) & CIC_IRB_CMD_START_MASK)
#define CIC_IRB_CMD_FIFO_RST_MASK                (0x8U)
#define CIC_IRB_CMD_FIFO_RST_SHIFT               (3U)
/*! FIFO_RST - Reset IR Blaster FIFO. This bit is self clearing.
 *  0b0..No effect
 *  0b1..Reset FIFO. IR Blaster FIFO is completly re-initialized all data present in FIFO are erased.
 */
#define CIC_IRB_CMD_FIFO_RST(x)                  (((uint32_t)(((uint32_t)(x)) << CIC_IRB_CMD_FIFO_RST_SHIFT)) & CIC_IRB_CMD_FIFO_RST_MASK)
/*! @} */

/*! @name INT_STATUS - Interrupt Status */
/*! @{ */
#define CIC_IRB_INT_STATUS_ENV_START_INT_MASK    (0x1U)
#define CIC_IRB_INT_STATUS_ENV_START_INT_SHIFT   (0U)
/*! ENV_START_INT - IR Blaster has started to transmit an envelope with ENV_INT bit = '1
 *  0b0..Interrupt is not pending
 *  0b1..Interrupt is pending
 */
#define CIC_IRB_INT_STATUS_ENV_START_INT(x)      (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_STATUS_ENV_START_INT_SHIFT)) & CIC_IRB_INT_STATUS_ENV_START_INT_MASK)
#define CIC_IRB_INT_STATUS_ENV_LAST_INT_MASK     (0x2U)
#define CIC_IRB_INT_STATUS_ENV_LAST_INT_SHIFT    (1U)
/*! ENV_LAST_INT - IR Blaster has finished to transmit an envelope with ENV_LAST bit = '1'.
 *  0b0..Interrupt is not pending
 *  0b1..Interrupt is pending
 */
#define CIC_IRB_INT_STATUS_ENV_LAST_INT(x)       (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_STATUS_ENV_LAST_INT_SHIFT)) & CIC_IRB_INT_STATUS_ENV_LAST_INT_MASK)
#define CIC_IRB_INT_STATUS_FIFO_UFL_INT_MASK     (0x4U)
#define CIC_IRB_INT_STATUS_FIFO_UFL_INT_SHIFT    (2U)
/*! FIFO_UFL_INT - IR Blaster FIFO underflow. IR Blaster has tried to transmit a data but the FIFO was empty.
 *  0b0..Interrupt is not pending
 *  0b1..Interrupt is pending
 */
#define CIC_IRB_INT_STATUS_FIFO_UFL_INT(x)       (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_STATUS_FIFO_UFL_INT_SHIFT)) & CIC_IRB_INT_STATUS_FIFO_UFL_INT_MASK)
/*! @} */

/*! @name INT_ENA - Interrupt Enable */
/*! @{ */
#define CIC_IRB_INT_ENA_ENV_START_ENA_MASK       (0x1U)
#define CIC_IRB_INT_ENA_ENV_START_ENA_SHIFT      (0U)
/*! ENV_START_ENA - Enable/Disable ENV_START interrupt
 *  0b0..Disable ENV_START interrupt
 *  0b1..Enable ENV_START interrupt
 */
#define CIC_IRB_INT_ENA_ENV_START_ENA(x)         (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_ENA_ENV_START_ENA_SHIFT)) & CIC_IRB_INT_ENA_ENV_START_ENA_MASK)
#define CIC_IRB_INT_ENA_ENV_LAST_ENA_MASK        (0x2U)
#define CIC_IRB_INT_ENA_ENV_LAST_ENA_SHIFT       (1U)
/*! ENV_LAST_ENA - Enable/Disable ENV_LAST interrupt
 *  0b0..Disable ENV_LAST interrupt
 *  0b1..Enable ENV_LAST interrupt
 */
#define CIC_IRB_INT_ENA_ENV_LAST_ENA(x)          (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_ENA_ENV_LAST_ENA_SHIFT)) & CIC_IRB_INT_ENA_ENV_LAST_ENA_MASK)
#define CIC_IRB_INT_ENA_FIFO_UFL_ENA_MASK        (0x4U)
#define CIC_IRB_INT_ENA_FIFO_UFL_ENA_SHIFT       (2U)
/*! FIFO_UFL_ENA - Enable/Disable FIFO_UFL interrupt
 *  0b0..Disable FIFO_UFL interrupt
 *  0b1..Enable FIFO_UFL nterrupt
 */
#define CIC_IRB_INT_ENA_FIFO_UFL_ENA(x)          (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_ENA_FIFO_UFL_ENA_SHIFT)) & CIC_IRB_INT_ENA_FIFO_UFL_ENA_MASK)
/*! @} */

/*! @name INT_CLR - Interrupt Clear */
/*! @{ */
#define CIC_IRB_INT_CLR_ENV_START_CLR_MASK       (0x1U)
#define CIC_IRB_INT_CLR_ENV_START_CLR_SHIFT      (0U)
/*! ENV_START_CLR - Clear ENV_START interrupt
 *  0b0..no effect
 *  0b1..clear ENV_START interrupt This bit is self clearing
 */
#define CIC_IRB_INT_CLR_ENV_START_CLR(x)         (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_CLR_ENV_START_CLR_SHIFT)) & CIC_IRB_INT_CLR_ENV_START_CLR_MASK)
#define CIC_IRB_INT_CLR_ENV_LAST_CLR_MASK        (0x2U)
#define CIC_IRB_INT_CLR_ENV_LAST_CLR_SHIFT       (1U)
/*! ENV_LAST_CLR - Clear ENV_LAST interrupt
 *  0b0..no effect
 *  0b1..clear ENV_LAST interrupt This bit is self clearing
 */
#define CIC_IRB_INT_CLR_ENV_LAST_CLR(x)          (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_CLR_ENV_LAST_CLR_SHIFT)) & CIC_IRB_INT_CLR_ENV_LAST_CLR_MASK)
#define CIC_IRB_INT_CLR_FIFO_UFL_CLR_MASK        (0x4U)
#define CIC_IRB_INT_CLR_FIFO_UFL_CLR_SHIFT       (2U)
/*! FIFO_UFL_CLR - Clear FIFO_UFL interrupt
 *  0b0..no effect
 *  0b1..clear FIFO_UFL interrupt This bit is self clearing
 */
#define CIC_IRB_INT_CLR_FIFO_UFL_CLR(x)          (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_CLR_FIFO_UFL_CLR_SHIFT)) & CIC_IRB_INT_CLR_FIFO_UFL_CLR_MASK)
/*! @} */

/*! @name INT_SET - Interrupt Set */
/*! @{ */
#define CIC_IRB_INT_SET_ENV_START_SET_MASK       (0x1U)
#define CIC_IRB_INT_SET_ENV_START_SET_SHIFT      (0U)
/*! ENV_START_SET - Set ENV_START interrupt
 *  0b0..no effect
 *  0b1..set ENV_START interrupt This bit is self clearing
 */
#define CIC_IRB_INT_SET_ENV_START_SET(x)         (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_SET_ENV_START_SET_SHIFT)) & CIC_IRB_INT_SET_ENV_START_SET_MASK)
#define CIC_IRB_INT_SET_ENV_LAST_SET_MASK        (0x2U)
#define CIC_IRB_INT_SET_ENV_LAST_SET_SHIFT       (1U)
/*! ENV_LAST_SET - Set ENV_LAST interrupt
 *  0b0..no effect
 *  0b1..set ENV_LAST interrupt This bit is self clearing
 */
#define CIC_IRB_INT_SET_ENV_LAST_SET(x)          (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_SET_ENV_LAST_SET_SHIFT)) & CIC_IRB_INT_SET_ENV_LAST_SET_MASK)
#define CIC_IRB_INT_SET_FIFO_UFL_SET_MASK        (0x4U)
#define CIC_IRB_INT_SET_FIFO_UFL_SET_SHIFT       (2U)
/*! FIFO_UFL_SET - Set FIFO_UFL interrupt
 *  0b0..no effect
 *  0b1..set FIFO_UFL interrupt This bit is self clearing
 */
#define CIC_IRB_INT_SET_FIFO_UFL_SET(x)          (((uint32_t)(((uint32_t)(x)) << CIC_IRB_INT_SET_FIFO_UFL_SET_SHIFT)) & CIC_IRB_INT_SET_FIFO_UFL_SET_MASK)
/*! @} */

/*! @name MODULE_ID - IR Blaster Module Identifier */
/*! @{ */
#define CIC_IRB_MODULE_ID_APERTURE_MASK          (0xFFU)
#define CIC_IRB_MODULE_ID_APERTURE_SHIFT         (0U)
/*! APERTURE - Aperture i.e. number minus 1 of consecutive packets 4 Kbytes reserved for this IP
 */
#define CIC_IRB_MODULE_ID_APERTURE(x)            (((uint32_t)(((uint32_t)(x)) << CIC_IRB_MODULE_ID_APERTURE_SHIFT)) & CIC_IRB_MODULE_ID_APERTURE_MASK)
#define CIC_IRB_MODULE_ID_MIN_REV_MASK           (0xF00U)
#define CIC_IRB_MODULE_ID_MIN_REV_SHIFT          (8U)
/*! MIN_REV - Minor revision i.e. with no software consequences
 */
#define CIC_IRB_MODULE_ID_MIN_REV(x)             (((uint32_t)(((uint32_t)(x)) << CIC_IRB_MODULE_ID_MIN_REV_SHIFT)) & CIC_IRB_MODULE_ID_MIN_REV_MASK)
#define CIC_IRB_MODULE_ID_MAJ_REV_MASK           (0xF000U)
#define CIC_IRB_MODULE_ID_MAJ_REV_SHIFT          (12U)
/*! MAJ_REV - Major revision i.e. implies software modifications
 */
#define CIC_IRB_MODULE_ID_MAJ_REV(x)             (((uint32_t)(((uint32_t)(x)) << CIC_IRB_MODULE_ID_MAJ_REV_SHIFT)) & CIC_IRB_MODULE_ID_MAJ_REV_MASK)
#define CIC_IRB_MODULE_ID_ID_MASK                (0xFFFF0000U)
#define CIC_IRB_MODULE_ID_ID_SHIFT               (16U)
/*! ID - Identifier. This is the unique identifier of the module
 */
#define CIC_IRB_MODULE_ID_ID(x)                  (((uint32_t)(((uint32_t)(x)) << CIC_IRB_MODULE_ID_ID_SHIFT)) & CIC_IRB_MODULE_ID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group CIC_IRB_Register_Masks */


/* CIC_IRB - Peripheral instance base addresses */
/** Peripheral CIC_IRB base address */
#define CIC_IRB_BASE                             (0x40007000u)
/** Peripheral CIC_IRB base pointer */
#define CIC_IRB                                  ((CIC_IRB_Type *)CIC_IRB_BASE)
/** Array initializer of CIC_IRB peripheral base addresses */
#define CIC_IRB_BASE_ADDRS                       { CIC_IRB_BASE }
/** Array initializer of CIC_IRB peripheral base pointers */
#define CIC_IRB_BASE_PTRS                        { CIC_IRB }
/** Interrupt vectors for the CIC_IRB peripheral type */
#define CIC_IRB_IRQS                             { CIC_IRB_IRQn }

/*!
 * @}
 */ /* end of group CIC_IRB_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- CTIMER Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CTIMER_Peripheral_Access_Layer CTIMER Peripheral Access Layer
 * @{
 */

/** CTIMER - Register Layout Typedef */
typedef struct {
  __IO uint32_t IR;                                /**< Interrupt Register. The IR can be written to clear interrupts. The IR can be read to identify which of eight possible interrupt sources are pending., offset: 0x0 */
  __IO uint32_t TCR;                               /**< Timer Control Register. The TCR is used to control the Timer Counter functions. The Timer Counter can be disabled or reset through the TCR., offset: 0x4 */
  __IO uint32_t TC;                                /**< Timer Counter. The 32 bit TC is incremented every PR+1 cycles of the APB bus clock. The TC is controlled through the TCR., offset: 0x8 */
  __IO uint32_t PR;                                /**< Prescale Register. When the Prescale Counter (PC) is equal to this value, the next clock increments the TC and clears the PC., offset: 0xC */
  __IO uint32_t PC;                                /**< Prescale Counter. The 32 bit PC is a counter which is incremented to the value stored in PR. When the value in PR is reached, the TC is incremented and the PC is cleared. The PC is observable and controllable through the bus interface., offset: 0x10 */
  __IO uint32_t MCR;                               /**< Match Control Register. The MCR is used to control if an interrupt is generated and if the TC is reset when a Match occurs., offset: 0x14 */
  __IO uint32_t MR[4];                             /**< Match Register . MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC., array offset: 0x18, array step: 0x4 */
  __IO uint32_t CCR;                               /**< Capture Control Register. The CCR controls which edges of the capture inputs are used to load the Capture Registers and whether or not an interrupt is generated when a capture takes place., offset: 0x28 */
  __I  uint32_t CR[2];                             /**< Capture Register . CR is loaded with the value of TC when there is an event on the CAPn.0 input., array offset: 0x2C, array step: 0x4 */
       uint8_t RESERVED_0[8];
  __IO uint32_t EMR;                               /**< External Match Register. The EMR controls the match function and the external match pins., offset: 0x3C */
       uint8_t RESERVED_1[48];
  __IO uint32_t CTCR;                              /**< Count Control Register. The CTCR selects between Timer and Counter mode, and in Counter mode selects the signal and edge(s) for counting., offset: 0x70 */
  __IO uint32_t PWMC;                              /**< PWM Control Register. The PWMCON enables PWM mode for the external match pins., offset: 0x74 */
} CTIMER_Type;

/* ----------------------------------------------------------------------------
   -- CTIMER Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CTIMER_Register_Masks CTIMER Register Masks
 * @{
 */

/*! @name IR - Interrupt Register. The IR can be written to clear interrupts. The IR can be read to identify which of eight possible interrupt sources are pending. */
/*! @{ */
#define CTIMER_IR_MR0INT_MASK                    (0x1U)
#define CTIMER_IR_MR0INT_SHIFT                   (0U)
/*! MR0INT - Interrupt flag for match channel 0.
 */
#define CTIMER_IR_MR0INT(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_IR_MR0INT_SHIFT)) & CTIMER_IR_MR0INT_MASK)
#define CTIMER_IR_MR1INT_MASK                    (0x2U)
#define CTIMER_IR_MR1INT_SHIFT                   (1U)
/*! MR1INT - Interrupt flag for match channel 1.
 */
#define CTIMER_IR_MR1INT(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_IR_MR1INT_SHIFT)) & CTIMER_IR_MR1INT_MASK)
#define CTIMER_IR_MR2INT_MASK                    (0x4U)
#define CTIMER_IR_MR2INT_SHIFT                   (2U)
/*! MR2INT - Interrupt flag for match channel 2.
 */
#define CTIMER_IR_MR2INT(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_IR_MR2INT_SHIFT)) & CTIMER_IR_MR2INT_MASK)
#define CTIMER_IR_MR3INT_MASK                    (0x8U)
#define CTIMER_IR_MR3INT_SHIFT                   (3U)
/*! MR3INT - Interrupt flag for match channel 3.
 */
#define CTIMER_IR_MR3INT(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_IR_MR3INT_SHIFT)) & CTIMER_IR_MR3INT_MASK)
#define CTIMER_IR_CR0INT_MASK                    (0x10U)
#define CTIMER_IR_CR0INT_SHIFT                   (4U)
/*! CR0INT - Interrupt flag for capture channel 0 event.
 */
#define CTIMER_IR_CR0INT(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_IR_CR0INT_SHIFT)) & CTIMER_IR_CR0INT_MASK)
#define CTIMER_IR_CR1INT_MASK                    (0x20U)
#define CTIMER_IR_CR1INT_SHIFT                   (5U)
/*! CR1INT - Interrupt flag for capture channel 1 event.
 */
#define CTIMER_IR_CR1INT(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_IR_CR1INT_SHIFT)) & CTIMER_IR_CR1INT_MASK)
#define CTIMER_IR_CR2INT_MASK                    (0x40U)
#define CTIMER_IR_CR2INT_SHIFT                   (6U)
/*! CR2INT - Interrupt flag for capture channel 2 event.
 */
#define CTIMER_IR_CR2INT(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_IR_CR2INT_SHIFT)) & CTIMER_IR_CR2INT_MASK)
#define CTIMER_IR_CR3INT_MASK                    (0x80U)
#define CTIMER_IR_CR3INT_SHIFT                   (7U)
/*! CR3INT - Interrupt flag for capture channel 3 event.
 */
#define CTIMER_IR_CR3INT(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_IR_CR3INT_SHIFT)) & CTIMER_IR_CR3INT_MASK)
/*! @} */

/*! @name TCR - Timer Control Register. The TCR is used to control the Timer Counter functions. The Timer Counter can be disabled or reset through the TCR. */
/*! @{ */
#define CTIMER_TCR_CEN_MASK                      (0x1U)
#define CTIMER_TCR_CEN_SHIFT                     (0U)
/*! CEN - Counter enable. 0 Disabled.The counters are disabled. 1 Enabled. The Timer Counter and Prescale Counter are enabled.
 */
#define CTIMER_TCR_CEN(x)                        (((uint32_t)(((uint32_t)(x)) << CTIMER_TCR_CEN_SHIFT)) & CTIMER_TCR_CEN_MASK)
#define CTIMER_TCR_CRST_MASK                     (0x2U)
#define CTIMER_TCR_CRST_SHIFT                    (1U)
/*! CRST - Counter reset. 0 Disabled. Do nothing. 1 Enabled. The Timer Counter and the Prescale
 *    Counter are synchronously reset on the next positive edge of the APB bus clock. The counters
 *    remain reset until TCR[1] is returned to zero.
 */
#define CTIMER_TCR_CRST(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_TCR_CRST_SHIFT)) & CTIMER_TCR_CRST_MASK)
/*! @} */

/*! @name TC - Timer Counter. The 32 bit TC is incremented every PR+1 cycles of the APB bus clock. The TC is controlled through the TCR. */
/*! @{ */
#define CTIMER_TC_TCVAL_MASK                     (0xFFFFFFFFU)
#define CTIMER_TC_TCVAL_SHIFT                    (0U)
/*! TCVAL - Timer counter value.
 */
#define CTIMER_TC_TCVAL(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_TC_TCVAL_SHIFT)) & CTIMER_TC_TCVAL_MASK)
/*! @} */

/*! @name PR - Prescale Register. When the Prescale Counter (PC) is equal to this value, the next clock increments the TC and clears the PC. */
/*! @{ */
#define CTIMER_PR_PRVAL_MASK                     (0xFFFFFFFFU)
#define CTIMER_PR_PRVAL_SHIFT                    (0U)
/*! PRVAL - Prescale counter value.
 */
#define CTIMER_PR_PRVAL(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_PR_PRVAL_SHIFT)) & CTIMER_PR_PRVAL_MASK)
/*! @} */

/*! @name PC - Prescale Counter. The 32 bit PC is a counter which is incremented to the value stored in PR. When the value in PR is reached, the TC is incremented and the PC is cleared. The PC is observable and controllable through the bus interface. */
/*! @{ */
#define CTIMER_PC_PCVAL_MASK                     (0xFFFFFFFFU)
#define CTIMER_PC_PCVAL_SHIFT                    (0U)
/*! PCVAL - Prescale counter value.
 */
#define CTIMER_PC_PCVAL(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_PC_PCVAL_SHIFT)) & CTIMER_PC_PCVAL_MASK)
/*! @} */

/*! @name MCR - Match Control Register. The MCR is used to control if an interrupt is generated and if the TC is reset when a Match occurs. */
/*! @{ */
#define CTIMER_MCR_MR0I_MASK                     (0x1U)
#define CTIMER_MCR_MR0I_SHIFT                    (0U)
/*! MR0I - Interrupt on MR0: an interrupt is generated when MR0 matches the value in the TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR0I(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR0I_SHIFT)) & CTIMER_MCR_MR0I_MASK)
#define CTIMER_MCR_MR0R_MASK                     (0x2U)
#define CTIMER_MCR_MR0R_SHIFT                    (1U)
/*! MR0R - Reset on MR0: the TC will be reset if MR0 matches it. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR0R(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR0R_SHIFT)) & CTIMER_MCR_MR0R_MASK)
#define CTIMER_MCR_MR0S_MASK                     (0x4U)
#define CTIMER_MCR_MR0S_SHIFT                    (2U)
/*! MR0S - Stop on MR0: the TC and PC will be stopped and TCR[0] will be set to 0 if MR0 matches the TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR0S(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR0S_SHIFT)) & CTIMER_MCR_MR0S_MASK)
#define CTIMER_MCR_MR1I_MASK                     (0x8U)
#define CTIMER_MCR_MR1I_SHIFT                    (3U)
/*! MR1I - Interrupt on MR1: an interrupt is generated when MR1 matches the value in the TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR1I(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR1I_SHIFT)) & CTIMER_MCR_MR1I_MASK)
#define CTIMER_MCR_MR1R_MASK                     (0x10U)
#define CTIMER_MCR_MR1R_SHIFT                    (4U)
/*! MR1R - Reset on MR1: the TC will be reset if MR1 matches it. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR1R(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR1R_SHIFT)) & CTIMER_MCR_MR1R_MASK)
#define CTIMER_MCR_MR1S_MASK                     (0x20U)
#define CTIMER_MCR_MR1S_SHIFT                    (5U)
/*! MR1S - Stop on MR1: the TC and PC will be stopped and TCR[0] will be set to 0 if MR1 matches the TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR1S(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR1S_SHIFT)) & CTIMER_MCR_MR1S_MASK)
#define CTIMER_MCR_MR2I_MASK                     (0x40U)
#define CTIMER_MCR_MR2I_SHIFT                    (6U)
/*! MR2I - Interrupt on MR2: an interrupt is generated when MR2 matches the value in the TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR2I(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR2I_SHIFT)) & CTIMER_MCR_MR2I_MASK)
#define CTIMER_MCR_MR2R_MASK                     (0x80U)
#define CTIMER_MCR_MR2R_SHIFT                    (7U)
/*! MR2R - Reset on MR2: the TC will be reset if MR2 matches it. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR2R(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR2R_SHIFT)) & CTIMER_MCR_MR2R_MASK)
#define CTIMER_MCR_MR2S_MASK                     (0x100U)
#define CTIMER_MCR_MR2S_SHIFT                    (8U)
/*! MR2S - Stop on MR2: the TC and PC will be stopped and TCR[0] will be set to 0 if MR2 matches the TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR2S(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR2S_SHIFT)) & CTIMER_MCR_MR2S_MASK)
#define CTIMER_MCR_MR3I_MASK                     (0x200U)
#define CTIMER_MCR_MR3I_SHIFT                    (9U)
/*! MR3I - Interrupt on MR3: an interrupt is generated when MR3 matches the value in the TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR3I(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR3I_SHIFT)) & CTIMER_MCR_MR3I_MASK)
#define CTIMER_MCR_MR3R_MASK                     (0x400U)
#define CTIMER_MCR_MR3R_SHIFT                    (10U)
/*! MR3R - Reset on MR3: the TC will be reset if MR3 matches it. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR3R(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR3R_SHIFT)) & CTIMER_MCR_MR3R_MASK)
#define CTIMER_MCR_MR3S_MASK                     (0x800U)
#define CTIMER_MCR_MR3S_SHIFT                    (11U)
/*! MR3S - Stop on MR3: the TC and PC will be stopped and TCR[0] will be set to 0 if MR3 matches the TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_MCR_MR3S(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MCR_MR3S_SHIFT)) & CTIMER_MCR_MR3S_MASK)
/*! @} */

/*! @name MR - Match Register . MR can be enabled through the MCR to reset the TC, stop both the TC and PC, and/or generate an interrupt every time MR matches the TC. */
/*! @{ */
#define CTIMER_MR_MATCH_MASK                     (0xFFFFFFFFU)
#define CTIMER_MR_MATCH_SHIFT                    (0U)
/*! MATCH - Timer counter match value.
 */
#define CTIMER_MR_MATCH(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_MR_MATCH_SHIFT)) & CTIMER_MR_MATCH_MASK)
/*! @} */

/* The count of CTIMER_MR */
#define CTIMER_MR_COUNT                          (4U)

/*! @name CCR - Capture Control Register. The CCR controls which edges of the capture inputs are used to load the Capture Registers and whether or not an interrupt is generated when a capture takes place. */
/*! @{ */
#define CTIMER_CCR_CAP0RE_MASK                   (0x1U)
#define CTIMER_CCR_CAP0RE_SHIFT                  (0U)
/*! CAP0RE - Rising edge of capture channel 0: a sequence of 0 then 1 causes CR0 to be loaded with
 *    the contents of TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_CCR_CAP0RE(x)                     (((uint32_t)(((uint32_t)(x)) << CTIMER_CCR_CAP0RE_SHIFT)) & CTIMER_CCR_CAP0RE_MASK)
#define CTIMER_CCR_CAP0FE_MASK                   (0x2U)
#define CTIMER_CCR_CAP0FE_SHIFT                  (1U)
/*! CAP0FE - Falling edge of capture channel 0: a sequence of 1 then 0 causes CR0 to be loaded with
 *    the contents of TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_CCR_CAP0FE(x)                     (((uint32_t)(((uint32_t)(x)) << CTIMER_CCR_CAP0FE_SHIFT)) & CTIMER_CCR_CAP0FE_MASK)
#define CTIMER_CCR_CAP0I_MASK                    (0x4U)
#define CTIMER_CCR_CAP0I_SHIFT                   (2U)
/*! CAP0I - Generate interrupt on channel 0 capture event: a CR0 load generates an interrupt.
 */
#define CTIMER_CCR_CAP0I(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_CCR_CAP0I_SHIFT)) & CTIMER_CCR_CAP0I_MASK)
#define CTIMER_CCR_CAP1RE_MASK                   (0x8U)
#define CTIMER_CCR_CAP1RE_SHIFT                  (3U)
/*! CAP1RE - Rising edge of capture channel 1: a sequence of 0 then 1 causes CR1 to be loaded with
 *    the contents of TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_CCR_CAP1RE(x)                     (((uint32_t)(((uint32_t)(x)) << CTIMER_CCR_CAP1RE_SHIFT)) & CTIMER_CCR_CAP1RE_MASK)
#define CTIMER_CCR_CAP1FE_MASK                   (0x10U)
#define CTIMER_CCR_CAP1FE_SHIFT                  (4U)
/*! CAP1FE - Falling edge of capture channel 1: a sequence of 1 then 0 causes CR1 to be loaded with
 *    the contents of TC. 0 = disabled. 1 = enabled.
 */
#define CTIMER_CCR_CAP1FE(x)                     (((uint32_t)(((uint32_t)(x)) << CTIMER_CCR_CAP1FE_SHIFT)) & CTIMER_CCR_CAP1FE_MASK)
#define CTIMER_CCR_CAP1I_MASK                    (0x20U)
#define CTIMER_CCR_CAP1I_SHIFT                   (5U)
/*! CAP1I - Generate interrupt on channel 1 capture event: a CR1 load generates an interrupt.
 */
#define CTIMER_CCR_CAP1I(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_CCR_CAP1I_SHIFT)) & CTIMER_CCR_CAP1I_MASK)
/*! @} */

/*! @name CR - Capture Register . CR is loaded with the value of TC when there is an event on the CAPn.0 input. */
/*! @{ */
#define CTIMER_CR_CAP_MASK                       (0xFFFFFFFFU)
#define CTIMER_CR_CAP_SHIFT                      (0U)
/*! CAP - Timer counter capture value.
 */
#define CTIMER_CR_CAP(x)                         (((uint32_t)(((uint32_t)(x)) << CTIMER_CR_CAP_SHIFT)) & CTIMER_CR_CAP_MASK)
/*! @} */

/* The count of CTIMER_CR */
#define CTIMER_CR_COUNT                          (2U)

/*! @name EMR - External Match Register. The EMR controls the match function and the external match pins. */
/*! @{ */
#define CTIMER_EMR_EM0_MASK                      (0x1U)
#define CTIMER_EMR_EM0_SHIFT                     (0U)
/*! EM0 - External Match 0. This bit reflects the state of output MAT0, whether or not this output
 *    is connected to a pin. When a match occurs between the TC and MR0, this bit can either toggle,
 *    go LOW, go HIGH, or do nothing, as selected by EMR[5:4]. This bit is driven to the MAT pins if
 *    the match function is selected via IOCON. 0 = LOW. 1 = HIGH.
 */
#define CTIMER_EMR_EM0(x)                        (((uint32_t)(((uint32_t)(x)) << CTIMER_EMR_EM0_SHIFT)) & CTIMER_EMR_EM0_MASK)
#define CTIMER_EMR_EM1_MASK                      (0x2U)
#define CTIMER_EMR_EM1_SHIFT                     (1U)
/*! EM1 - External Match 1. This bit reflects the state of output MAT1, whether or not this output
 *    is connected to a pin. When a match occurs between the TC and MR1, this bit can either toggle,
 *    go LOW, go HIGH, or do nothing, as selected by EMR[7:6]. This bit is driven to the MAT pins if
 *    the match function is selected via IOCON. 0 = LOW. 1 = HIGH.
 */
#define CTIMER_EMR_EM1(x)                        (((uint32_t)(((uint32_t)(x)) << CTIMER_EMR_EM1_SHIFT)) & CTIMER_EMR_EM1_MASK)
#define CTIMER_EMR_EM2_MASK                      (0x4U)
#define CTIMER_EMR_EM2_SHIFT                     (2U)
/*! EM2 - External Match 2. This bit reflects the state of output MAT2, whether or not this output
 *    is connected to a pin. When a match occurs between the TC and MR2, this bit can either toggle,
 *    go LOW, go HIGH, or do nothing, as selected by EMR[9:8]. This bit is driven to the MAT pins if
 *    the match function is selected via IOCON. 0 = LOW. 1 = HIGH
 */
#define CTIMER_EMR_EM2(x)                        (((uint32_t)(((uint32_t)(x)) << CTIMER_EMR_EM2_SHIFT)) & CTIMER_EMR_EM2_MASK)
#define CTIMER_EMR_EM3_MASK                      (0x8U)
#define CTIMER_EMR_EM3_SHIFT                     (3U)
/*! EM3 - External Match 3. This bit reflects the state of output MAT3, whether or not this output
 *    is connected to a pin. When a match occurs between the TC and MR3, this bit can either toggle,
 *    go LOW, go HIGH, or do nothing, as selected by MR[11:10]. This bit is driven to the MAT pins
 *    if the match function is selected via IOCON. 0 = LOW. 1 = HIGH.
 */
#define CTIMER_EMR_EM3(x)                        (((uint32_t)(((uint32_t)(x)) << CTIMER_EMR_EM3_SHIFT)) & CTIMER_EMR_EM3_MASK)
#define CTIMER_EMR_EMC0_MASK                     (0x30U)
#define CTIMER_EMR_EMC0_SHIFT                    (4U)
/*! EMC0 - External Match Control 0. Determines the functionality of External Match 0. 0x0 Do
 *    Nothing. 0x1 Clear. Clear the corresponding External Match bit/output to 0 (MAT0 pin is LOW if
 *    pinned out). 0x2 Set. Set the corresponding External Match bit/output to 1 (MAT0 pin is HIGH if
 *    pinned out). 0x3 Toggle. Toggle the corresponding External Match bit/output.
 */
#define CTIMER_EMR_EMC0(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_EMR_EMC0_SHIFT)) & CTIMER_EMR_EMC0_MASK)
#define CTIMER_EMR_EMC1_MASK                     (0xC0U)
#define CTIMER_EMR_EMC1_SHIFT                    (6U)
/*! EMC1 - External Match Control 1. Determines the functionality of External Match 1. 0x0 Do
 *    Nothing. 0x1 Clear. Clear the corresponding External Match bit/output to 0 (MAT1 pin is LOW if
 *    pinned out). 0x2 Set. Set the corresponding External Match bit/output to 1 (MAT1 pin is HIGH if
 *    pinned out). 0x3 Toggle. Toggle the corresponding External Match bit/output.
 */
#define CTIMER_EMR_EMC1(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_EMR_EMC1_SHIFT)) & CTIMER_EMR_EMC1_MASK)
#define CTIMER_EMR_EMC2_MASK                     (0x300U)
#define CTIMER_EMR_EMC2_SHIFT                    (8U)
/*! EMC2 - External Match Control 2. Determines the functionality of External Match 2. 0x0 Do
 *    Nothing. 0x1 Clear. Clear the corresponding External Match bit/output to 0 (MAT2 pin is LOW if
 *    pinned out). 0x2 Set. Set the corresponding External Match bit/output to 1 (MAT2 pin is HIGH if
 *    pinned out). 0x3 Toggle. Toggle the corresponding External Match bit/output.
 */
#define CTIMER_EMR_EMC2(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_EMR_EMC2_SHIFT)) & CTIMER_EMR_EMC2_MASK)
#define CTIMER_EMR_EMC3_MASK                     (0xC00U)
#define CTIMER_EMR_EMC3_SHIFT                    (10U)
/*! EMC3 - External Match Control 3. Determines the functionality of External Match 3. 0x0 Do
 *    Nothing. 0x1 Clear. Clear the corresponding External Match bit/output to 0 (MAT3 pin is LOW if
 *    pinned out). 0x2 Set. Set the corresponding External Match bit/output to 1 (MAT3 pin is HIGH if
 *    pinned out). 0x3 Toggle. Toggle the corresponding External Match bit/output.
 */
#define CTIMER_EMR_EMC3(x)                       (((uint32_t)(((uint32_t)(x)) << CTIMER_EMR_EMC3_SHIFT)) & CTIMER_EMR_EMC3_MASK)
/*! @} */

/*! @name CTCR - Count Control Register. The CTCR selects between Timer and Counter mode, and in Counter mode selects the signal and edge(s) for counting. */
/*! @{ */
#define CTIMER_CTCR_CTMODE_MASK                  (0x3U)
#define CTIMER_CTCR_CTMODE_SHIFT                 (0U)
/*! CTMODE - Counter/Timer Mode This field selects which rising APB bus clock edges can increment
 *    Timer s Prescale Counter (PC), or clear PC and increment Timer Counter (TC). Timer Mode: the TC
 *    is incremented when the Prescale Counter matches the Prescale Register. 0x0 Timer Mode.
 *    Incremented every rising APB bus clock edge. 0x1 Counter Mode rising edge. TC is incremented on
 *    rising edges on the CAP input selected by bits 3:2. 0x2 Counter Mode falling edge. TC is
 *    incremented on falling edges on the CAP input selected by bits 3:2. 0x3 Counter Mode dual edge. TC is
 *    incremented on both edges on the CAP input selected by bits 3:2.
 */
#define CTIMER_CTCR_CTMODE(x)                    (((uint32_t)(((uint32_t)(x)) << CTIMER_CTCR_CTMODE_SHIFT)) & CTIMER_CTCR_CTMODE_MASK)
#define CTIMER_CTCR_CINSEL_MASK                  (0xCU)
#define CTIMER_CTCR_CINSEL_SHIFT                 (2U)
/*! CINSEL - Count Input Select When bits 1:0 in this register are not 00, these bits select which
 *    CAP pin is sampled for clocking. Note: If Counter mode is selected for a particular CAPn input
 *    in the CTCR, the 3 bits for that input in the Capture Control Register (CCR) must be
 *    programmed as 000. However, capture and/or interrupt can be selected for the other 3 CAPn inputs in the
 *    same timer. 0x0 Channel 0. CAPn.0 for CT32Bn 0x1 Channel 1. CAPn.1 for CT32Bn 0x2 Channel 2.
 *    CAPn.2 for CT32Bn 0x3 Channel 3. CAPn.3 for CT32Bn
 */
#define CTIMER_CTCR_CINSEL(x)                    (((uint32_t)(((uint32_t)(x)) << CTIMER_CTCR_CINSEL_SHIFT)) & CTIMER_CTCR_CINSEL_MASK)
#define CTIMER_CTCR_ENCC_MASK                    (0x10U)
#define CTIMER_CTCR_ENCC_SHIFT                   (4U)
/*! ENCC - Setting this bit to 1 enables clearing of the timer and the prescaler when the
 *    capture-edge event specified in bits 7:5 occurs.
 */
#define CTIMER_CTCR_ENCC(x)                      (((uint32_t)(((uint32_t)(x)) << CTIMER_CTCR_ENCC_SHIFT)) & CTIMER_CTCR_ENCC_MASK)
#define CTIMER_CTCR_SELCC_MASK                   (0xE0U)
#define CTIMER_CTCR_SELCC_SHIFT                  (5U)
/*! SELCC - Edge select. When bit 4 is 1, these bits select which capture input edge will cause the
 *    timer and prescaler to be cleared. These bits have no effect when bit 4 is low. Values 0x2 to
 *    0x3 and 0x6 to 0x7 are reserved. 0 0x0 Channel 0 Rising Edge. Rising edge of the signal on
 *    capture channel 0 clears the timer (if bit 4 is set). 0x1 Channel 0 Falling Edge. Falling edge of
 *    the signal on capture channel 0 clears the timer (if bit 4 is set). 0x2 Channel 1 Rising
 *    Edge. Rising edge of the signal on capture channel 1 clears the timer (if bit 4 is set). 0x3
 *    Channel 1 Falling Edge. Falling edge of the signal on capture channel 1 clears the timer (if bit 4
 *    is set). 0x4 Channel 2 Rising Edge. Rising edge of the signal on capture channel 2 clears the
 *    timer (if bit 4 is set). 0x5 Channel 2 Falling Edge. Falling edge of the signal on capture
 *    channel 2 clears the timer (if bit 4 is set).
 */
#define CTIMER_CTCR_SELCC(x)                     (((uint32_t)(((uint32_t)(x)) << CTIMER_CTCR_SELCC_SHIFT)) & CTIMER_CTCR_SELCC_MASK)
/*! @} */

/*! @name PWMC - PWM Control Register. The PWMCON enables PWM mode for the external match pins. */
/*! @{ */
#define CTIMER_PWMC_PWMEN0_MASK                  (0x1U)
#define CTIMER_PWMC_PWMEN0_SHIFT                 (0U)
/*! PWMEN0 - PWM mode enable for channel0. 0 Match. CT32Bn_MAT0 is controlled by EM0. 1 PWM. PWM mode is enabled for CT32Bn_MAT0.
 */
#define CTIMER_PWMC_PWMEN0(x)                    (((uint32_t)(((uint32_t)(x)) << CTIMER_PWMC_PWMEN0_SHIFT)) & CTIMER_PWMC_PWMEN0_MASK)
#define CTIMER_PWMC_PWMEN1_MASK                  (0x2U)
#define CTIMER_PWMC_PWMEN1_SHIFT                 (1U)
/*! PWMEN1 - PWM mode enable for channel1. 0 Match. CT32Bn_MAT01 is controlled by EM1. 1 PWM. PWM mode is enabled for CT32Bn_MAT1.
 */
#define CTIMER_PWMC_PWMEN1(x)                    (((uint32_t)(((uint32_t)(x)) << CTIMER_PWMC_PWMEN1_SHIFT)) & CTIMER_PWMC_PWMEN1_MASK)
#define CTIMER_PWMC_PWMEN2_MASK                  (0x4U)
#define CTIMER_PWMC_PWMEN2_SHIFT                 (2U)
/*! PWMEN2 - PWM mode enable for channel2. 0 Match. CT32Bn_MAT2 is controlled by EM2. 1 PWM. PWM mode is enabled for CT32Bn_MAT2.
 */
#define CTIMER_PWMC_PWMEN2(x)                    (((uint32_t)(((uint32_t)(x)) << CTIMER_PWMC_PWMEN2_SHIFT)) & CTIMER_PWMC_PWMEN2_MASK)
#define CTIMER_PWMC_PWMEN3_MASK                  (0x8U)
#define CTIMER_PWMC_PWMEN3_SHIFT                 (3U)
/*! PWMEN3 - PWM mode enable for channel3. Note: It is recommended to use match channel 3 to set the
 *    PWM cycle. 0 Match. CT32Bn_MAT3 is controlled by EM3. 1 PWM. PWM mode is enabled for
 *    CT132Bn_MAT3
 */
#define CTIMER_PWMC_PWMEN3(x)                    (((uint32_t)(((uint32_t)(x)) << CTIMER_PWMC_PWMEN3_SHIFT)) & CTIMER_PWMC_PWMEN3_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group CTIMER_Register_Masks */


/* CTIMER - Peripheral instance base addresses */
/** Peripheral CTIMER0 base address */
#define CTIMER0_BASE                             (0x40021000u)
/** Peripheral CTIMER0 base pointer */
#define CTIMER0                                  ((CTIMER_Type *)CTIMER0_BASE)
/** Peripheral CTIMER1 base address */
#define CTIMER1_BASE                             (0x40022000u)
/** Peripheral CTIMER1 base pointer */
#define CTIMER1                                  ((CTIMER_Type *)CTIMER1_BASE)
/** Array initializer of CTIMER peripheral base addresses */
#define CTIMER_BASE_ADDRS                        { CTIMER0_BASE, CTIMER1_BASE }
/** Array initializer of CTIMER peripheral base pointers */
#define CTIMER_BASE_PTRS                         { CTIMER0, CTIMER1 }
/** Interrupt vectors for the CTIMER peripheral type */
#define CTIMER_IRQS                              { CTIMER0_IRQn, CTIMER1_IRQn }

/*!
 * @}
 */ /* end of group CTIMER_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- DMA Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DMA_Peripheral_Access_Layer DMA Peripheral Access Layer
 * @{
 */

/** DMA - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< DMA control., offset: 0x0 */
  __I  uint32_t INTSTAT;                           /**< Interrupt status., offset: 0x4 */
  __IO uint32_t SRAMBASE;                          /**< SRAM address of the channel configuration table., offset: 0x8 */
       uint8_t RESERVED_0[20];
  struct {                                         /* offset: 0x20, array step: 0x5C */
    __IO uint32_t ENABLESET;                         /**< Channel Enable read and Set for all DMA channels, array offset: 0x20, array step: 0x5C */
         uint8_t RESERVED_0[4];
    __O  uint32_t ENABLECLR;                         /**< Channel Enable Clear for all DMA channels., array offset: 0x28, array step: 0x5C */
         uint8_t RESERVED_1[4];
    __I  uint32_t ACTIVE;                            /**< Channel Active status for all DMA channels., array offset: 0x30, array step: 0x5C */
         uint8_t RESERVED_2[4];
    __I  uint32_t BUSY;                              /**< Channel Busy status for all DMA channels., array offset: 0x38, array step: 0x5C */
         uint8_t RESERVED_3[4];
    __IO uint32_t ERRINT;                            /**< Error Interrupt status for all DMA channels., array offset: 0x40, array step: 0x5C */
         uint8_t RESERVED_4[4];
    __IO uint32_t INTENSET;                          /**< Interrupt Enable read and Set for all DMA channels., array offset: 0x48, array step: 0x5C */
         uint8_t RESERVED_5[4];
    __O  uint32_t INTENCLR;                          /**< Interrupt Enable Clear for all DMA channels., array offset: 0x50, array step: 0x5C */
         uint8_t RESERVED_6[4];
    __IO uint32_t INTA;                              /**< Interrupt A status for all DMA channels., array offset: 0x58, array step: 0x5C */
         uint8_t RESERVED_7[4];
    __IO uint32_t INTB;                              /**< Interrupt B status for all DMA channels., array offset: 0x60, array step: 0x5C */
         uint8_t RESERVED_8[4];
    __O  uint32_t SETVALID;                          /**< Set ValidPending control bits for all DMA channels., array offset: 0x68, array step: 0x5C */
         uint8_t RESERVED_9[4];
    __O  uint32_t SETTRIG;                           /**< Set Trigger control bits for all DMA channels., array offset: 0x70, array step: 0x5C */
         uint8_t RESERVED_10[4];
    __O  uint32_t ABORT;                             /**< Channel Abort control for all DMA channels., array offset: 0x78, array step: 0x5C */
  } COMMON[1];
       uint8_t RESERVED_1[900];
  struct {                                         /* offset: 0x400, array step: 0x10 */
    __IO uint32_t CFG;                               /**< Configuration register for DMA channel x, array offset: 0x400, array step: 0x10 */
    __I  uint32_t CTLSTAT;                           /**< Control and status register for DMA channel x, array offset: 0x404, array step: 0x10 */
    __IO uint32_t XFERCFG;                           /**< Transfer configuration register for DMA channel x, array offset: 0x408, array step: 0x10 */
    __IO uint32_t RESERVED0;                         /**< Reserved, array offset: 0x40C, array step: 0x10 */
  } CHANNEL[19];
} DMA_Type;

/* ----------------------------------------------------------------------------
   -- DMA Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DMA_Register_Masks DMA Register Masks
 * @{
 */

/*! @name CTRL - DMA control. */
/*! @{ */
#define DMA_CTRL_ENABLE_MASK                     (0x1U)
#define DMA_CTRL_ENABLE_SHIFT                    (0U)
/*! ENABLE - DMA controller master enable. 0: Disabled. The DMA controller is disabled. This clears
 *    any triggers that were asserted at the point when disabled, but does not prevent re-triggering
 *    when the DMA controller is re-enabled. 1: Enabled. The DMA controller is enabled.
 */
#define DMA_CTRL_ENABLE(x)                       (((uint32_t)(((uint32_t)(x)) << DMA_CTRL_ENABLE_SHIFT)) & DMA_CTRL_ENABLE_MASK)
/*! @} */

/*! @name INTSTAT - Interrupt status. */
/*! @{ */
#define DMA_INTSTAT_ACTIVEINT_MASK               (0x2U)
#define DMA_INTSTAT_ACTIVEINT_SHIFT              (1U)
/*! ACTIVEINT - Summarizes whether any enabled interrupts (other than error interrupts) are pending.
 *    0: Not pending. No enabled interrupts are pending. 1: Pending. At least one enabled interrupt
 *    is pending.
 */
#define DMA_INTSTAT_ACTIVEINT(x)                 (((uint32_t)(((uint32_t)(x)) << DMA_INTSTAT_ACTIVEINT_SHIFT)) & DMA_INTSTAT_ACTIVEINT_MASK)
#define DMA_INTSTAT_ACTIVEERRINT_MASK            (0x4U)
#define DMA_INTSTAT_ACTIVEERRINT_SHIFT           (2U)
/*! ACTIVEERRINT - Summarizes whether any error interrupts are pending. 0: Not pending. No error
 *    interrupts are pending. 1: Pending. At least one error interrupt is pending.
 */
#define DMA_INTSTAT_ACTIVEERRINT(x)              (((uint32_t)(((uint32_t)(x)) << DMA_INTSTAT_ACTIVEERRINT_SHIFT)) & DMA_INTSTAT_ACTIVEERRINT_MASK)
/*! @} */

/*! @name SRAMBASE - SRAM address of the channel configuration table. */
/*! @{ */
#define DMA_SRAMBASE_OFFSET_MASK                 (0xFFFFFE00U)
#define DMA_SRAMBASE_OFFSET_SHIFT                (9U)
/*! OFFSET - Address bits 31:9 of the beginning of the DMA descriptor table. For 19 channels, the
 *    table must begin on a 512 byte boundary. The SRAMBASE register must be configured with an
 *    address (preferably in on-chip SRAM) where DMA descriptors will be stored.
 */
#define DMA_SRAMBASE_OFFSET(x)                   (((uint32_t)(((uint32_t)(x)) << DMA_SRAMBASE_OFFSET_SHIFT)) & DMA_SRAMBASE_OFFSET_MASK)
/*! @} */

/*! @name COMMON_ENABLESET - Channel Enable read and Set for all DMA channels */
/*! @{ */
#define DMA_COMMON_ENABLESET_ENA_MASK            (0xFFFFFFFFU)
#define DMA_COMMON_ENABLESET_ENA_SHIFT           (0U)
/*! ENA - Enable for DMA channels. Bit n enables or disables DMA channel n. 0: disabled. 1: enabled.
 */
#define DMA_COMMON_ENABLESET_ENA(x)              (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_ENABLESET_ENA_SHIFT)) & DMA_COMMON_ENABLESET_ENA_MASK)
/*! @} */

/* The count of DMA_COMMON_ENABLESET */
#define DMA_COMMON_ENABLESET_COUNT               (1U)

/*! @name COMMON_ENABLECLR - Channel Enable Clear for all DMA channels. */
/*! @{ */
#define DMA_COMMON_ENABLECLR_CLR_MASK            (0xFFFFFFFFU)
#define DMA_COMMON_ENABLECLR_CLR_SHIFT           (0U)
/*! CLR - Writing ones to this register clears the corresponding bits in ENABLESET0. Bit n clears the channel enable bit n.
 */
#define DMA_COMMON_ENABLECLR_CLR(x)              (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_ENABLECLR_CLR_SHIFT)) & DMA_COMMON_ENABLECLR_CLR_MASK)
/*! @} */

/* The count of DMA_COMMON_ENABLECLR */
#define DMA_COMMON_ENABLECLR_COUNT               (1U)

/*! @name COMMON_ACTIVE - Channel Active status for all DMA channels. */
/*! @{ */
#define DMA_COMMON_ACTIVE_ACT_MASK               (0xFFFFFFFFU)
#define DMA_COMMON_ACTIVE_ACT_SHIFT              (0U)
/*! ACT - Active flag for DMA channel n. Bit n corresponds to DMA channel n. 0: not active. 1: active.
 */
#define DMA_COMMON_ACTIVE_ACT(x)                 (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_ACTIVE_ACT_SHIFT)) & DMA_COMMON_ACTIVE_ACT_MASK)
/*! @} */

/* The count of DMA_COMMON_ACTIVE */
#define DMA_COMMON_ACTIVE_COUNT                  (1U)

/*! @name COMMON_BUSY - Channel Busy status for all DMA channels. */
/*! @{ */
#define DMA_COMMON_BUSY_BSY_MASK                 (0xFFFFFFFFU)
#define DMA_COMMON_BUSY_BSY_SHIFT                (0U)
/*! BSY - Busy flag for DMA channel n. Bit n corresponds to DMA channel n. 0 : not busy. 1: busy.
 */
#define DMA_COMMON_BUSY_BSY(x)                   (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_BUSY_BSY_SHIFT)) & DMA_COMMON_BUSY_BSY_MASK)
/*! @} */

/* The count of DMA_COMMON_BUSY */
#define DMA_COMMON_BUSY_COUNT                    (1U)

/*! @name COMMON_ERRINT - Error Interrupt status for all DMA channels. */
/*! @{ */
#define DMA_COMMON_ERRINT_ERR_MASK               (0xFFFFFFFFU)
#define DMA_COMMON_ERRINT_ERR_SHIFT              (0U)
/*! ERR - Error Interrupt flag for DMA channel n. Bit n corresponds to DMA channel n. 0: error
 *    interrupt is not active. 1: error interrupt is active.
 */
#define DMA_COMMON_ERRINT_ERR(x)                 (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_ERRINT_ERR_SHIFT)) & DMA_COMMON_ERRINT_ERR_MASK)
/*! @} */

/* The count of DMA_COMMON_ERRINT */
#define DMA_COMMON_ERRINT_COUNT                  (1U)

/*! @name COMMON_INTENSET - Interrupt Enable read and Set for all DMA channels. */
/*! @{ */
#define DMA_COMMON_INTENSET_INTEN_MASK           (0xFFFFFFFFU)
#define DMA_COMMON_INTENSET_INTEN_SHIFT          (0U)
/*! INTEN - Interrupt Enable read and set for DMA channel n. Bit n corresponds to DMA channel n. 0:
 *    interrupt for DMA channel is disabled. 1: interrupt for DMA channel is enabled.
 */
#define DMA_COMMON_INTENSET_INTEN(x)             (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_INTENSET_INTEN_SHIFT)) & DMA_COMMON_INTENSET_INTEN_MASK)
/*! @} */

/* The count of DMA_COMMON_INTENSET */
#define DMA_COMMON_INTENSET_COUNT                (1U)

/*! @name COMMON_INTENCLR - Interrupt Enable Clear for all DMA channels. */
/*! @{ */
#define DMA_COMMON_INTENCLR_CLR_MASK             (0xFFFFFFFFU)
#define DMA_COMMON_INTENCLR_CLR_SHIFT            (0U)
/*! CLR - Writing ones to this register clears corresponding bits in the INTENSET0. Bit n corresponds to DMA channel n.
 */
#define DMA_COMMON_INTENCLR_CLR(x)               (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_INTENCLR_CLR_SHIFT)) & DMA_COMMON_INTENCLR_CLR_MASK)
/*! @} */

/* The count of DMA_COMMON_INTENCLR */
#define DMA_COMMON_INTENCLR_COUNT                (1U)

/*! @name COMMON_INTA - Interrupt A status for all DMA channels. */
/*! @{ */
#define DMA_COMMON_INTA_IA_MASK                  (0xFFFFFFFFU)
#define DMA_COMMON_INTA_IA_SHIFT                 (0U)
/*! IA - Interrupt A status for DMA channel n. Bit n corresponds to DMA channel n. 0: the DMA
 *    channel interrupt A is not active. 1: the DMA channel interrupt A is active.
 */
#define DMA_COMMON_INTA_IA(x)                    (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_INTA_IA_SHIFT)) & DMA_COMMON_INTA_IA_MASK)
/*! @} */

/* The count of DMA_COMMON_INTA */
#define DMA_COMMON_INTA_COUNT                    (1U)

/*! @name COMMON_INTB - Interrupt B status for all DMA channels. */
/*! @{ */
#define DMA_COMMON_INTB_IB_MASK                  (0xFFFFFFFFU)
#define DMA_COMMON_INTB_IB_SHIFT                 (0U)
/*! IB - Interrupt B status for DMA channel n. Bit n corresponds to DMA channel n. 0: the DMA
 *    channel interrupt B is not active. 1: the DMA channel interrupt B is active.
 */
#define DMA_COMMON_INTB_IB(x)                    (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_INTB_IB_SHIFT)) & DMA_COMMON_INTB_IB_MASK)
/*! @} */

/* The count of DMA_COMMON_INTB */
#define DMA_COMMON_INTB_COUNT                    (1U)

/*! @name COMMON_SETVALID - Set ValidPending control bits for all DMA channels. */
/*! @{ */
#define DMA_COMMON_SETVALID_SV_MASK              (0xFFFFFFFFU)
#define DMA_COMMON_SETVALID_SV_SHIFT             (0U)
/*! SV - SETVALID control for DMA channel n. Bit n corresponds to DMA channel n. 0: no effect. 1:
 *    sets the VALIDPENDING control bit for DMA channel n
 */
#define DMA_COMMON_SETVALID_SV(x)                (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_SETVALID_SV_SHIFT)) & DMA_COMMON_SETVALID_SV_MASK)
/*! @} */

/* The count of DMA_COMMON_SETVALID */
#define DMA_COMMON_SETVALID_COUNT                (1U)

/*! @name COMMON_SETTRIG - Set Trigger control bits for all DMA channels. */
/*! @{ */
#define DMA_COMMON_SETTRIG_TRIG_MASK             (0xFFFFFFFFU)
#define DMA_COMMON_SETTRIG_TRIG_SHIFT            (0U)
/*! TRIG - Set Trigger control bit for DMA channel n. Bit n corresponds to DMA channel n. 0: no
 *    effect. 1: sets the TRIG bit for DMA channel n.
 */
#define DMA_COMMON_SETTRIG_TRIG(x)               (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_SETTRIG_TRIG_SHIFT)) & DMA_COMMON_SETTRIG_TRIG_MASK)
/*! @} */

/* The count of DMA_COMMON_SETTRIG */
#define DMA_COMMON_SETTRIG_COUNT                 (1U)

/*! @name COMMON_ABORT - Channel Abort control for all DMA channels. */
/*! @{ */
#define DMA_COMMON_ABORT_ABORTCTRL_MASK          (0xFFFFFFFFU)
#define DMA_COMMON_ABORT_ABORTCTRL_SHIFT         (0U)
/*! ABORTCTRL - Abort control for DMA channel 0. Bit n corresponds to DMA channel n. 0: no effect.
 *    1: aborts DMA operations on channel n.
 */
#define DMA_COMMON_ABORT_ABORTCTRL(x)            (((uint32_t)(((uint32_t)(x)) << DMA_COMMON_ABORT_ABORTCTRL_SHIFT)) & DMA_COMMON_ABORT_ABORTCTRL_MASK)
/*! @} */

/* The count of DMA_COMMON_ABORT */
#define DMA_COMMON_ABORT_COUNT                   (1U)

/*! @name CHANNEL_CFG - Configuration register for DMA channel x */
/*! @{ */
#define DMA_CHANNEL_CFG_PERIPHREQEN_MASK         (0x1U)
#define DMA_CHANNEL_CFG_PERIPHREQEN_SHIFT        (0U)
/*! PERIPHREQEN - Peripheral request Enable. If a DMA channel is used to perform a memory-to-memory
 *    move, any peripheral DMA request associated with that channel can be disabled to prevent any
 *    interaction between the peripheral and the DMA controller. 0 Disabled. Peripheral DMA requests
 *    are disabled. 1 Enabled. Peripheral DMA requests are enabled.
 */
#define DMA_CHANNEL_CFG_PERIPHREQEN(x)           (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_PERIPHREQEN_SHIFT)) & DMA_CHANNEL_CFG_PERIPHREQEN_MASK)
#define DMA_CHANNEL_CFG_HWTRIGEN_MASK            (0x2U)
#define DMA_CHANNEL_CFG_HWTRIGEN_SHIFT           (1U)
/*! HWTRIGEN - Hardware Triggering Enable for this channel. 0 Disabled. Hardware triggering is not
 *    used. 1 Enabled. Use hardware triggering.
 */
#define DMA_CHANNEL_CFG_HWTRIGEN(x)              (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_HWTRIGEN_SHIFT)) & DMA_CHANNEL_CFG_HWTRIGEN_MASK)
#define DMA_CHANNEL_CFG_TRIGPOL_MASK             (0x10U)
#define DMA_CHANNEL_CFG_TRIGPOL_SHIFT            (4U)
/*! TRIGPOL - Trigger Polarity. Selects the polarity of a hardware trigger for this channel. 0
 *    Active low - falling edge. Hardware trigger is active low or falling edge triggered, based on
 *    TRIGTYPE. 1 Active high - rising edge. Hardware trigger is active high or rising edge triggered,
 *    based on TRIGTYPE.
 */
#define DMA_CHANNEL_CFG_TRIGPOL(x)               (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_TRIGPOL_SHIFT)) & DMA_CHANNEL_CFG_TRIGPOL_MASK)
#define DMA_CHANNEL_CFG_TRIGTYPE_MASK            (0x20U)
#define DMA_CHANNEL_CFG_TRIGTYPE_SHIFT           (5U)
/*! TRIGTYPE - Trigger Type. Selects hardware trigger as edge triggered or level triggered. 0 Edge.
 *    Hardware trigger is edge triggered. Transfers will be initiated and completed, as specified
 *    for a single trigger. 1 Level. Hardware trigger is level triggered. Note that when level
 *    triggering without burst (BURSTPOWER = 0) is selected, only hardware triggers should be used on that
 *    channel. Transfers continue as long as the trigger level is asserted. Once the trigger is
 *    de-asserted, the transfer will be paused until the trigger is, again, asserted. However, the
 *    transfer will not be paused until any remaining transfers within the current BURSTPOWER length are
 *    completed.
 */
#define DMA_CHANNEL_CFG_TRIGTYPE(x)              (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_TRIGTYPE_SHIFT)) & DMA_CHANNEL_CFG_TRIGTYPE_MASK)
#define DMA_CHANNEL_CFG_TRIGBURST_MASK           (0x40U)
#define DMA_CHANNEL_CFG_TRIGBURST_SHIFT          (6U)
/*! TRIGBURST - Trigger Burst. Selects whether hardware triggers cause a single or burst transfer. 0
 *    Single transfer. Hardware trigger causes a single transfer. 1 Burst transfer. When the
 *    trigger for this channel is set to edge triggered, a hardware trigger causes a burst transfer, as
 *    defined by BURSTPOWER. When the trigger for this channel is set to level triggered, a hardware
 *    trigger causes transfers to continue as long as the trigger is asserted, unless the transfer is
 *    complete.
 */
#define DMA_CHANNEL_CFG_TRIGBURST(x)             (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_TRIGBURST_SHIFT)) & DMA_CHANNEL_CFG_TRIGBURST_MASK)
#define DMA_CHANNEL_CFG_BURSTPOWER_MASK          (0xF00U)
#define DMA_CHANNEL_CFG_BURSTPOWER_SHIFT         (8U)
/*! BURSTPOWER - Burst Power is used in two ways. It always selects the address wrap size when
 *    SRCBURSTWRAP and/or DSTBURSTWRAP modes are selected (see descriptions elsewhere in this register).
 *    When the TRIGBURST field elsewhere in this register = 1, Burst Power selects how many
 *    transfers are performed for each DMA trigger. This can be used, for example, with peripherals that
 *    contain a FIFO that can initiate a DMA operation when the FIFO reaches a certain level. 0000:
 *    Burst size = 1 (2^0). 0001: Burst size = 2 (2^1). 0010: Burst size = 4 (2^2). ... 1010: Burst
 *    size = 1024 (2^10). This corresponds to the maximum supported transfer count. others: not
 *    supported. The total transfer length as defined in the XFERCOUNT bits in the XFERCFG register must be
 *    an even multiple of the burst size.
 */
#define DMA_CHANNEL_CFG_BURSTPOWER(x)            (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_BURSTPOWER_SHIFT)) & DMA_CHANNEL_CFG_BURSTPOWER_MASK)
#define DMA_CHANNEL_CFG_SRCBURSTWRAP_MASK        (0x4000U)
#define DMA_CHANNEL_CFG_SRCBURSTWRAP_SHIFT       (14U)
/*! SRCBURSTWRAP - Source Burst Wrap. When enabled, the source data address for the DMA is wrapped ,
 *    meaning that the source address range for each burst will be the same. As an example, this
 *    could be used to read several sequential registers from a peripheral for each DMA burst, reading
 *    the same registers again for each burst. 0 Disabled. Source burst wrapping is not enabled for
 *    this DMA channel. 1 Enabled. Source burst wrapping is enabled for this DMA channel.
 */
#define DMA_CHANNEL_CFG_SRCBURSTWRAP(x)          (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_SRCBURSTWRAP_SHIFT)) & DMA_CHANNEL_CFG_SRCBURSTWRAP_MASK)
#define DMA_CHANNEL_CFG_DSTBURSTWRAP_MASK        (0x8000U)
#define DMA_CHANNEL_CFG_DSTBURSTWRAP_SHIFT       (15U)
/*! DSTBURSTWRAP - Destination Burst Wrap. When enabled, the destination data address for the DMA is
 *    wrapped , meaning that the destination address range for each burst will be the same. As an
 *    example, this could be used to write several sequential registers to a peripheral for each DMA
 *    burst, writing the same registers again for each burst. 0 Disabled. Destination burst wrapping
 *    is not enabled for this DMA channel. 1 Enabled. Destination burst wrapping is enabled for
 *    this DMA channel.
 */
#define DMA_CHANNEL_CFG_DSTBURSTWRAP(x)          (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_DSTBURSTWRAP_SHIFT)) & DMA_CHANNEL_CFG_DSTBURSTWRAP_MASK)
#define DMA_CHANNEL_CFG_CHPRIORITY_MASK          (0x70000U)
#define DMA_CHANNEL_CFG_CHPRIORITY_SHIFT         (16U)
/*! CHPRIORITY - Priority of this channel when multiple DMA requests are pending. Eight priority
 *    levels are supported: 0x0 = highest priority. 0x7 = lowest priority.
 */
#define DMA_CHANNEL_CFG_CHPRIORITY(x)            (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CFG_CHPRIORITY_SHIFT)) & DMA_CHANNEL_CFG_CHPRIORITY_MASK)
/*! @} */

/* The count of DMA_CHANNEL_CFG */
#define DMA_CHANNEL_CFG_COUNT                    (19U)

/*! @name CHANNEL_CTLSTAT - Control and status register for DMA channel x */
/*! @{ */
#define DMA_CHANNEL_CTLSTAT_VALIDPENDING_MASK    (0x1U)
#define DMA_CHANNEL_CTLSTAT_VALIDPENDING_SHIFT   (0U)
/*! VALIDPENDING - Valid pending flag for this channel. This bit is set when a 1 is written to the
 *    corresponding bit in the related SETVALID register when CFGVALID = 1 for the same channel. 0 No
 *    effect. No effect on DMA operation. 1 Valid pending.
 */
#define DMA_CHANNEL_CTLSTAT_VALIDPENDING(x)      (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CTLSTAT_VALIDPENDING_SHIFT)) & DMA_CHANNEL_CTLSTAT_VALIDPENDING_MASK)
#define DMA_CHANNEL_CTLSTAT_TRIG_MASK            (0x4U)
#define DMA_CHANNEL_CTLSTAT_TRIG_SHIFT           (2U)
/*! TRIG - Trigger flag. Indicates that the trigger for this channel is currently set. This bit is
 *    cleared at the end of an entire transfer or upon reload when CLRTRIG = 1. 0 Not triggered. The
 *    trigger for this DMA channel is not set. DMA operations will not be carried out. 1 Triggered.
 *    The trigger for this DMA channel is set. DMA operations will be carried out.
 */
#define DMA_CHANNEL_CTLSTAT_TRIG(x)              (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_CTLSTAT_TRIG_SHIFT)) & DMA_CHANNEL_CTLSTAT_TRIG_MASK)
/*! @} */

/* The count of DMA_CHANNEL_CTLSTAT */
#define DMA_CHANNEL_CTLSTAT_COUNT                (19U)

/*! @name CHANNEL_XFERCFG - Transfer configuration register for DMA channel x */
/*! @{ */
#define DMA_CHANNEL_XFERCFG_CFGVALID_MASK        (0x1U)
#define DMA_CHANNEL_XFERCFG_CFGVALID_SHIFT       (0U)
/*! CFGVALID - Configuration Valid flag. This bit indicates whether the current channel descriptor
 *    is valid and can potentially be acted upon, if all other activation criteria are fulfilled. 0
 *    Not valid. The channel descriptor is not considered valid until validated by an associated
 *    SETVALID0 setting. 1 Valid. The current channel descriptor is considered valid.
 */
#define DMA_CHANNEL_XFERCFG_CFGVALID(x)          (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_CFGVALID_SHIFT)) & DMA_CHANNEL_XFERCFG_CFGVALID_MASK)
#define DMA_CHANNEL_XFERCFG_RELOAD_MASK          (0x2U)
#define DMA_CHANNEL_XFERCFG_RELOAD_SHIFT         (1U)
/*! RELOAD - Indicates whether the channel s control structure will be reloaded when the current
 *    descriptor is exhausted. Reloading allows ping-pong and linked transfers. 0 Disabled. Do not
 *    reload the channels control structure when the current descriptor is exhausted. 1 Enabled. Reload
 *    the channels control structure when the current descriptor is exhausted.
 */
#define DMA_CHANNEL_XFERCFG_RELOAD(x)            (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_RELOAD_SHIFT)) & DMA_CHANNEL_XFERCFG_RELOAD_MASK)
#define DMA_CHANNEL_XFERCFG_SWTRIG_MASK          (0x4U)
#define DMA_CHANNEL_XFERCFG_SWTRIG_SHIFT         (2U)
/*! SWTRIG - Software Trigger. 0 Not set. When written by software, the trigger for this channel is
 *    not set. A new trigger, as defined by the HWTRIGEN, TRIGPOL, and TRIGTYPE will be needed to
 *    start the channel. 1 Set. When written by software, the trigger for this channel is set
 *    immediately. This feature should not be used with level triggering when TRIGBURST = 0.
 */
#define DMA_CHANNEL_XFERCFG_SWTRIG(x)            (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_SWTRIG_SHIFT)) & DMA_CHANNEL_XFERCFG_SWTRIG_MASK)
#define DMA_CHANNEL_XFERCFG_CLRTRIG_MASK         (0x8U)
#define DMA_CHANNEL_XFERCFG_CLRTRIG_SHIFT        (3U)
/*! CLRTRIG - Clear Trigger. 0 Not cleared. The trigger is not cleared when this descriptor is
 *    exhausted. If there is a reload, the next descriptor will be started. 1 Cleared. The trigger is
 *    cleared when this descriptor is exhausted.
 */
#define DMA_CHANNEL_XFERCFG_CLRTRIG(x)           (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_CLRTRIG_SHIFT)) & DMA_CHANNEL_XFERCFG_CLRTRIG_MASK)
#define DMA_CHANNEL_XFERCFG_SETINTA_MASK         (0x10U)
#define DMA_CHANNEL_XFERCFG_SETINTA_SHIFT        (4U)
/*! SETINTA - Set Interrupt flag A for this channel. There is no hardware distinction between
 *    interrupt A and B. They can be used by software to assist with more complex descriptor usage. By
 *    convention, interrupt A may be used when only one interrupt flag is needed. 0 No effect. 1 Set.
 *    The INTA flag for this channel will be set when the current descriptor is exhausted.
 */
#define DMA_CHANNEL_XFERCFG_SETINTA(x)           (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_SETINTA_SHIFT)) & DMA_CHANNEL_XFERCFG_SETINTA_MASK)
#define DMA_CHANNEL_XFERCFG_SETINTB_MASK         (0x20U)
#define DMA_CHANNEL_XFERCFG_SETINTB_SHIFT        (5U)
/*! SETINTB - Set Interrupt flag B for this channel. There is no hardware distinction between
 *    interrupt A and B. They can be used by software to assist with more complex descriptor usage. By
 *    convention, interrupt A may be used when only one interrupt flag is needed. 0 No effect. 1 Set.
 *    The INTB flag for this channel will be set when the current descriptor is exhausted.
 */
#define DMA_CHANNEL_XFERCFG_SETINTB(x)           (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_SETINTB_SHIFT)) & DMA_CHANNEL_XFERCFG_SETINTB_MASK)
#define DMA_CHANNEL_XFERCFG_WIDTH_MASK           (0x300U)
#define DMA_CHANNEL_XFERCFG_WIDTH_SHIFT          (8U)
/*! WIDTH - Transfer width used for this DMA channel. 0x0 8-bit. 8-bit transfers are performed
 *    (8-bit source reads and destination writes). 0x1 16-bit. 6-bit transfers are performed (16-bit
 *    source reads and destination writes). 0x2 32-bit. 32-bit transfers are performed (32-bit source
 *    reads and destination writes). 0x3 Reserved. Reserved setting, do not use.
 */
#define DMA_CHANNEL_XFERCFG_WIDTH(x)             (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_WIDTH_SHIFT)) & DMA_CHANNEL_XFERCFG_WIDTH_MASK)
#define DMA_CHANNEL_XFERCFG_SRCINC_MASK          (0x3000U)
#define DMA_CHANNEL_XFERCFG_SRCINC_SHIFT         (12U)
/*! SRCINC - Determines whether the source address is incremented for each DMA transfer. 0x0 No
 *    increment. The source address is not incremented for each transfer. This is the usual case when
 *    the source is a peripheral device. 0x1 1 x width. The source address is incremented by the
 *    amount specified by Width for each transfer. This is the usual case when the source is memory. 0x2
 *    2 x width. The source address is incremented by 2 times the amount specified by Width for each
 *    transfer. 0x3 4 x width. The source address is incremented by 4 times the amount specified by
 *    Width for each transfer.
 */
#define DMA_CHANNEL_XFERCFG_SRCINC(x)            (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_SRCINC_SHIFT)) & DMA_CHANNEL_XFERCFG_SRCINC_MASK)
#define DMA_CHANNEL_XFERCFG_DSTINC_MASK          (0xC000U)
#define DMA_CHANNEL_XFERCFG_DSTINC_SHIFT         (14U)
/*! DSTINC - Determines whether the destination address is incremented for each DMA transfer. 0x0 No
 *    increment. The destination address is not incremented for each transfer. This is the usual
 *    case when the destination is a peripheral device. 0x1 1 x width. The destination address is
 *    incremented by the amount specified by Width for each transfer. This is the usual case when the
 *    destination is memory. 0x2 2 x width. The destination address is incremented by 2 times the
 *    amount specified by Width for each transfer. 0x3 4 x width. The destination address is incremented
 *    by 4 times the amount specified by Width for each transfer.
 */
#define DMA_CHANNEL_XFERCFG_DSTINC(x)            (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_DSTINC_SHIFT)) & DMA_CHANNEL_XFERCFG_DSTINC_MASK)
#define DMA_CHANNEL_XFERCFG_XFERCOUNT_MASK       (0x3FF0000U)
#define DMA_CHANNEL_XFERCFG_XFERCOUNT_SHIFT      (16U)
/*! XFERCOUNT - Total number of transfers to be performed, minus 1 encoded. The number of bytes
 *    transferred is: (XFERCOUNT + 1) x data width (as defined by the WIDTH field). Remark: The DMA
 *    controller uses this bit field during transfer to count down. Hence, it cannot be used by software
 *    to read back the size of the transfer, for instance, in an interrupt handler. 0x0 = a total
 *    of 1 transfer will be performed. 0x1 = a total of 2 transfers will be performed. ... 0x3FF = a
 *    total of 1,024 transfers will be performed.
 */
#define DMA_CHANNEL_XFERCFG_XFERCOUNT(x)         (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_XFERCFG_XFERCOUNT_SHIFT)) & DMA_CHANNEL_XFERCFG_XFERCOUNT_MASK)
/*! @} */

/* The count of DMA_CHANNEL_XFERCFG */
#define DMA_CHANNEL_XFERCFG_COUNT                (19U)

/*! @name CHANNEL_RESERVED0 - Reserved */
/*! @{ */
#define DMA_CHANNEL_RESERVED0_DUMMYWORD_MASK     (0xFFFFFFFFU)
#define DMA_CHANNEL_RESERVED0_DUMMYWORD_SHIFT    (0U)
/*! DUMMYWORD - Reserved. The value read from a reserved bit is not defined.
 */
#define DMA_CHANNEL_RESERVED0_DUMMYWORD(x)       (((uint32_t)(((uint32_t)(x)) << DMA_CHANNEL_RESERVED0_DUMMYWORD_SHIFT)) & DMA_CHANNEL_RESERVED0_DUMMYWORD_MASK)
/*! @} */

/* The count of DMA_CHANNEL_RESERVED0 */
#define DMA_CHANNEL_RESERVED0_COUNT              (19U)


/*!
 * @}
 */ /* end of group DMA_Register_Masks */


/* DMA - Peripheral instance base addresses */
/** Peripheral DMA0 base address */
#define DMA0_BASE                                (0x40085000u)
/** Peripheral DMA0 base pointer */
#define DMA0                                     ((DMA_Type *)DMA0_BASE)
/** Array initializer of DMA peripheral base addresses */
#define DMA_BASE_ADDRS                           { DMA0_BASE }
/** Array initializer of DMA peripheral base pointers */
#define DMA_BASE_PTRS                            { DMA0 }
/** Interrupt vectors for the DMA peripheral type */
#define DMA_IRQS                                 { DMA0_IRQn }

/*!
 * @}
 */ /* end of group DMA_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- DMIC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DMIC_Peripheral_Access_Layer DMIC Peripheral Access Layer
 * @{
 */

/** DMIC - Register Layout Typedef */
typedef struct {
  struct {                                         /* offset: 0x0, array step: 0x100 */
    __IO uint32_t OSR;                               /**< Oversample Rate register 0. This register selects the oversample rate (CIC decimation rate) for the input channel., array offset: 0x0, array step: 0x100 */
    __IO uint32_t DIVHFCLK;                          /**< DMIC Clock Register 0. This register controls the clock pre-divider for the input channel., array offset: 0x4, array step: 0x100 */
    __IO uint32_t PREAC2FSCOEF;                      /**< Pre-Emphasis Filter Coefficient for 2 FS register 0. This register seclects the pre-emphasis filter coeffcient for the input channel when 2 FS mode is used., array offset: 0x8, array step: 0x100 */
    __IO uint32_t PREAC4FSCOEF;                      /**< Pre-Emphasis Filter Coefficient for 4 FS register 0. This register seclects the pre-emphasis filter coeffcient for the input channel when 4FS mode is used, array offset: 0xC, array step: 0x100 */
    __IO uint32_t GAINSHIFT;                         /**< Decimator Gain Shift register 0. This register adjusts the gain of the 4FS PCM data from the input filter., array offset: 0x10, array step: 0x100 */
         uint8_t RESERVED_0[108];
    __IO uint32_t FIFO_CTRL;                         /**< FIFO Control register 0. This register configures FIFO usage., array offset: 0x80, array step: 0x100 */
    __IO uint32_t FIFO_STATUS;                       /**< FIFO Status register 0 . This register provides status information for the FIFO and also indicates an interrupt from the peripheral funcion., array offset: 0x84, array step: 0x100 */
    __IO uint32_t FIFO_DATA;                         /**< FIFO Data Register 0. This register is used to read values that have been received via the PDM stream., array offset: 0x88, array step: 0x100 */
    __IO uint32_t PHY_CTRL;                          /**< PHY Control / PDM Source Configuration register 0. This register configures how the PDM source signals are interpreted., array offset: 0x8C, array step: 0x100 */
    __IO uint32_t DC_CTRL;                           /**< DC Control register 0. This register controls the DC filter., array offset: 0x90, array step: 0x100 */
         uint8_t RESERVED_1[108];
  } CHANNEL[2];
       uint8_t RESERVED_0[3328];
  __IO uint32_t CHANEN;                            /**< Channel Enable register. This register allows enabling either or both PDM channels., offset: 0xF00 */
       uint8_t RESERVED_1[8];
  __IO uint32_t IOCFG;                             /**< I/O Configuration register. This register configures the use of the PDM pins., offset: 0xF0C */
  __IO uint32_t USE2FS;                            /**< Use 2FS register. This register allow selecting 2FS output rather than 1FS output., offset: 0xF10 */
       uint8_t RESERVED_2[108];
  __IO uint32_t HWVADGAIN;                         /**< HWVAD input gain register. This register controls the input gain of the HWVAD., offset: 0xF80 */
  __IO uint32_t HWVADHPFS;                         /**< HWVAD filter control register. This register controls the HWVAD filter setting., offset: 0xF84 */
  __IO uint32_t HWVADST10;                         /**< HWVAD control register. This register controls the operation of the filter block and resets the internal interrut flag., offset: 0xF88 */
  __IO uint32_t HWVADRSTT;                         /**< HWVAD filter reset register, offset: 0xF8C */
  __IO uint32_t HWVADTHGN;                         /**< HWVAD noise estimator gain register, offset: 0xF90 */
  __IO uint32_t HWVADTHGS;                         /**< HWVAD signal estimator gain register, offset: 0xF94 */
  __I  uint32_t HWVADLOWZ;                         /**< HWVAD noise envelope estimator register, offset: 0xF98 */
       uint8_t RESERVED_3[96];
  __I  uint32_t ID;                                /**< Module Identification register, offset: 0xFFC */
} DMIC_Type;

/* ----------------------------------------------------------------------------
   -- DMIC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup DMIC_Register_Masks DMIC Register Masks
 * @{
 */

/*! @name CHANNEL_OSR - Oversample Rate register 0. This register selects the oversample rate (CIC decimation rate) for the input channel. */
/*! @{ */
#define DMIC_CHANNEL_OSR_OSR_MASK                (0xFFU)
#define DMIC_CHANNEL_OSR_OSR_SHIFT               (0U)
/*! OSR - Selects the oversample rate for the related input channel.
 */
#define DMIC_CHANNEL_OSR_OSR(x)                  (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_OSR_OSR_SHIFT)) & DMIC_CHANNEL_OSR_OSR_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_OSR */
#define DMIC_CHANNEL_OSR_COUNT                   (2U)

/*! @name CHANNEL_DIVHFCLK - DMIC Clock Register 0. This register controls the clock pre-divider for the input channel. */
/*! @{ */
#define DMIC_CHANNEL_DIVHFCLK_PDMDIV_MASK        (0xFU)
#define DMIC_CHANNEL_DIVHFCLK_PDMDIV_SHIFT       (0U)
/*! PDMDIV - PDM clock divider value. 0: divide by 1; 1: divide by 2; 2: divide by 3; 3: divide by
 *    4; 4: divide by 6; 5: divide by 8; 6: divide by 12; 7: divide by 16; 8: divide by 24; 9: divide
 *    by 32; 10: divide by 48; 11: divide by 64; 12: divide by 96; 13: divide by 128; others =
 *    reserved.
 */
#define DMIC_CHANNEL_DIVHFCLK_PDMDIV(x)          (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_DIVHFCLK_PDMDIV_SHIFT)) & DMIC_CHANNEL_DIVHFCLK_PDMDIV_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_DIVHFCLK */
#define DMIC_CHANNEL_DIVHFCLK_COUNT              (2U)

/*! @name CHANNEL_PREAC2FSCOEF - Pre-Emphasis Filter Coefficient for 2 FS register 0. This register seclects the pre-emphasis filter coeffcient for the input channel when 2 FS mode is used. */
/*! @{ */
#define DMIC_CHANNEL_PREAC2FSCOEF_COMP_MASK      (0x3U)
#define DMIC_CHANNEL_PREAC2FSCOEF_COMP_SHIFT     (0U)
/*! COMP - Pre-emphasis filer coefficient for 2 FS mode. 0: Compensation = 0 1: Compensation = -0.16
 *    2: Compensation = -0.15 3: Compensation = -0.13
 */
#define DMIC_CHANNEL_PREAC2FSCOEF_COMP(x)        (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_PREAC2FSCOEF_COMP_SHIFT)) & DMIC_CHANNEL_PREAC2FSCOEF_COMP_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_PREAC2FSCOEF */
#define DMIC_CHANNEL_PREAC2FSCOEF_COUNT          (2U)

/*! @name CHANNEL_PREAC4FSCOEF - Pre-Emphasis Filter Coefficient for 4 FS register 0. This register seclects the pre-emphasis filter coeffcient for the input channel when 4FS mode is used */
/*! @{ */
#define DMIC_CHANNEL_PREAC4FSCOEF_COMP_MASK      (0x3U)
#define DMIC_CHANNEL_PREAC4FSCOEF_COMP_SHIFT     (0U)
/*! COMP - Pre-emphasis filer coefficient for 4 FS mode. 0: Compensation = 0; 1: Compensation =
 *    -0.16; 2: Compensation = -0.15; 3: Compensation = -0.13.
 */
#define DMIC_CHANNEL_PREAC4FSCOEF_COMP(x)        (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_PREAC4FSCOEF_COMP_SHIFT)) & DMIC_CHANNEL_PREAC4FSCOEF_COMP_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_PREAC4FSCOEF */
#define DMIC_CHANNEL_PREAC4FSCOEF_COUNT          (2U)

/*! @name CHANNEL_GAINSHIFT - Decimator Gain Shift register 0. This register adjusts the gain of the 4FS PCM data from the input filter. */
/*! @{ */
#define DMIC_CHANNEL_GAINSHIFT_GAIN_MASK         (0x3FU)
#define DMIC_CHANNEL_GAINSHIFT_GAIN_SHIFT        (0U)
/*! GAIN - Gain control, as a positive or negative (two s complement) number of bits to shift.
 */
#define DMIC_CHANNEL_GAINSHIFT_GAIN(x)           (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_GAINSHIFT_GAIN_SHIFT)) & DMIC_CHANNEL_GAINSHIFT_GAIN_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_GAINSHIFT */
#define DMIC_CHANNEL_GAINSHIFT_COUNT             (2U)

/*! @name CHANNEL_FIFO_CTRL - FIFO Control register 0. This register configures FIFO usage. */
/*! @{ */
#define DMIC_CHANNEL_FIFO_CTRL_ENABLE_MASK       (0x1U)
#define DMIC_CHANNEL_FIFO_CTRL_ENABLE_SHIFT      (0U)
/*! ENABLE - FIFO enable. 0: FIFO is not enabled. Enabling a DMIC channel with the FIFO disabled
 *    could be useful in order to avoid a filter settling. delay when a channel is re-enabled after a
 *    period when the data was not needed. 1: FIFO is enabled. The FIFO must be enabled in order for
 *    the CPU or DMA to read data from the DMIC via the FIFODATA register.
 */
#define DMIC_CHANNEL_FIFO_CTRL_ENABLE(x)         (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_CTRL_ENABLE_SHIFT)) & DMIC_CHANNEL_FIFO_CTRL_ENABLE_MASK)
#define DMIC_CHANNEL_FIFO_CTRL_RESETN_MASK       (0x2U)
#define DMIC_CHANNEL_FIFO_CTRL_RESETN_SHIFT      (1U)
/*! RESETN - FIFO reset. 0: Reset the FIFO. This bit must be cleared before resuming operation. 1: Normal operation.
 */
#define DMIC_CHANNEL_FIFO_CTRL_RESETN(x)         (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_CTRL_RESETN_SHIFT)) & DMIC_CHANNEL_FIFO_CTRL_RESETN_MASK)
#define DMIC_CHANNEL_FIFO_CTRL_INTEN_MASK        (0x4U)
#define DMIC_CHANNEL_FIFO_CTRL_INTEN_SHIFT       (2U)
/*! INTEN - Interrupt enable. 0: FIFO level interrupts are not enabled. 1: FIFO level interrupts are enabled.
 */
#define DMIC_CHANNEL_FIFO_CTRL_INTEN(x)          (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_CTRL_INTEN_SHIFT)) & DMIC_CHANNEL_FIFO_CTRL_INTEN_MASK)
#define DMIC_CHANNEL_FIFO_CTRL_DMAEN_MASK        (0x8U)
#define DMIC_CHANNEL_FIFO_CTRL_DMAEN_SHIFT       (3U)
/*! DMAEN - DMA enable. 0: DMA requests are not enabled. 1: DMA requests based on FIFO level are enabled.
 */
#define DMIC_CHANNEL_FIFO_CTRL_DMAEN(x)          (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_CTRL_DMAEN_SHIFT)) & DMIC_CHANNEL_FIFO_CTRL_DMAEN_MASK)
#define DMIC_CHANNEL_FIFO_CTRL_TRIGLVL_MASK      (0x1F0000U)
#define DMIC_CHANNEL_FIFO_CTRL_TRIGLVL_SHIFT     (16U)
/*! TRIGLVL - FIFO trigger level. Selects the data trigger level for interrupt or DMA operation. If
 *    enabled to do so, the FIFO level can wake up the device. 0: trigger when the FIFO has received
 *    one entry (is no longer empty). 1: trigger when the FIFO has received two entries. 15:
 *    trigger when the FIFO has received 16 entries (has become full).
 */
#define DMIC_CHANNEL_FIFO_CTRL_TRIGLVL(x)        (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_CTRL_TRIGLVL_SHIFT)) & DMIC_CHANNEL_FIFO_CTRL_TRIGLVL_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_FIFO_CTRL */
#define DMIC_CHANNEL_FIFO_CTRL_COUNT             (2U)

/*! @name CHANNEL_FIFO_STATUS - FIFO Status register 0 . This register provides status information for the FIFO and also indicates an interrupt from the peripheral funcion. */
/*! @{ */
#define DMIC_CHANNEL_FIFO_STATUS_INT_MASK        (0x1U)
#define DMIC_CHANNEL_FIFO_STATUS_INT_SHIFT       (0U)
/*! INT - Interrupt flag. Asserted when FIFO data reaches the level specified in the FIFOCTRL
 *    register. Writing a one to this bit clears the flag. Remark: note that the bus clock to the DMIC
 *    subsystem must be running in order for an interrupt to occur.
 */
#define DMIC_CHANNEL_FIFO_STATUS_INT(x)          (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_STATUS_INT_SHIFT)) & DMIC_CHANNEL_FIFO_STATUS_INT_MASK)
#define DMIC_CHANNEL_FIFO_STATUS_OVERRUN_MASK    (0x2U)
#define DMIC_CHANNEL_FIFO_STATUS_OVERRUN_SHIFT   (1U)
/*! OVERRUN - Overrun flag. Indicates that a FIFO overflow has occurred at some point. Writing a one
 *    to this bit clears the flag. This flag does not cause an interrupt.
 */
#define DMIC_CHANNEL_FIFO_STATUS_OVERRUN(x)      (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_STATUS_OVERRUN_SHIFT)) & DMIC_CHANNEL_FIFO_STATUS_OVERRUN_MASK)
#define DMIC_CHANNEL_FIFO_STATUS_UNDERRUN_MASK   (0x4U)
#define DMIC_CHANNEL_FIFO_STATUS_UNDERRUN_SHIFT  (2U)
/*! UNDERRUN - Underrun flag. Indicates that a FIFO underflow has occurred at some point. Writing a one to this bit clears the flag.
 */
#define DMIC_CHANNEL_FIFO_STATUS_UNDERRUN(x)     (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_STATUS_UNDERRUN_SHIFT)) & DMIC_CHANNEL_FIFO_STATUS_UNDERRUN_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_FIFO_STATUS */
#define DMIC_CHANNEL_FIFO_STATUS_COUNT           (2U)

/*! @name CHANNEL_FIFO_DATA - FIFO Data Register 0. This register is used to read values that have been received via the PDM stream. */
/*! @{ */
#define DMIC_CHANNEL_FIFO_DATA_DATA_MASK         (0xFFFFFFU)
#define DMIC_CHANNEL_FIFO_DATA_DATA_SHIFT        (0U)
/*! DATA - Data from the top of the input filter FIFO.
 */
#define DMIC_CHANNEL_FIFO_DATA_DATA(x)           (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_FIFO_DATA_DATA_SHIFT)) & DMIC_CHANNEL_FIFO_DATA_DATA_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_FIFO_DATA */
#define DMIC_CHANNEL_FIFO_DATA_COUNT             (2U)

/*! @name CHANNEL_PHY_CTRL - PHY Control / PDM Source Configuration register 0. This register configures how the PDM source signals are interpreted. */
/*! @{ */
#define DMIC_CHANNEL_PHY_CTRL_PHY_FALL_MASK      (0x1U)
#define DMIC_CHANNEL_PHY_CTRL_PHY_FALL_SHIFT     (0U)
/*! PHY_FALL - Capture PDM_DATA. 0: Capture PDM_DATA on the rising edge of PDM_CLK. 1: Capture PDM_DATA on the falling edge of PDM_CLK.
 */
#define DMIC_CHANNEL_PHY_CTRL_PHY_FALL(x)        (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_PHY_CTRL_PHY_FALL_SHIFT)) & DMIC_CHANNEL_PHY_CTRL_PHY_FALL_MASK)
#define DMIC_CHANNEL_PHY_CTRL_PHY_HALF_MASK      (0x2U)
#define DMIC_CHANNEL_PHY_CTRL_PHY_HALF_SHIFT     (1U)
/*! PHY_HALF - Half rate sampling. 0: Standard half rate sampling. The clock to the DMIC is sent at
 *    the same rate as the decimator is providing. 1: Use half rate sampling. The PDM clock to DMIC
 *    is divided by 2. Each PDM data is sampled twice into the decimator. The purpose of this mode
 *    is to allow slower sampling rate in quiet periods of listening for a trigger. Allowing the
 *    decimator to maintain the same decimation rate between the higher quality, higher PDM clock rate
 *    and the lower quality lower PDM clock rate means that the user can quickly switch to higher
 *    quality without re-configuring the decimator, and thus avoiding long filter settling times, when
 *    switching to higher quality (higher freq PDM clock) for recognition.
 */
#define DMIC_CHANNEL_PHY_CTRL_PHY_HALF(x)        (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_PHY_CTRL_PHY_HALF_SHIFT)) & DMIC_CHANNEL_PHY_CTRL_PHY_HALF_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_PHY_CTRL */
#define DMIC_CHANNEL_PHY_CTRL_COUNT              (2U)

/*! @name CHANNEL_DC_CTRL - DC Control register 0. This register controls the DC filter. */
/*! @{ */
#define DMIC_CHANNEL_DC_CTRL_DCPOLE_MASK         (0x3U)
#define DMIC_CHANNEL_DC_CTRL_DCPOLE_SHIFT        (0U)
/*! DCPOLE - DC block filter. 0: Flat response, no filter. 1: 155 Hz. 2: 78 Hz. 3: 39 Hz. These
 *    frequencies assume a PCM output frequency of 16 MHz. If the actual PCM output frequency is 8 MHz,
 *    for example, the noted frequencies would be divided by 2.
 */
#define DMIC_CHANNEL_DC_CTRL_DCPOLE(x)           (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_DC_CTRL_DCPOLE_SHIFT)) & DMIC_CHANNEL_DC_CTRL_DCPOLE_MASK)
#define DMIC_CHANNEL_DC_CTRL_DCGAIN_MASK         (0xF0U)
#define DMIC_CHANNEL_DC_CTRL_DCGAIN_SHIFT        (4U)
/*! DCGAIN - Fine gain adjustment in the form of a number of bits to downshift.
 */
#define DMIC_CHANNEL_DC_CTRL_DCGAIN(x)           (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_DC_CTRL_DCGAIN_SHIFT)) & DMIC_CHANNEL_DC_CTRL_DCGAIN_MASK)
#define DMIC_CHANNEL_DC_CTRL_SATURATEAT16BIT_MASK (0x100U)
#define DMIC_CHANNEL_DC_CTRL_SATURATEAT16BIT_SHIFT (8U)
/*! SATURATEAT16BIT - Selects 16-bit saturation. 0:Results roll over if out range and do not
 *    saturate. 1:If the result overflows, it saturates at 0xFFFF for positive overflow and 0x8000 for
 *    negative overflow.
 */
#define DMIC_CHANNEL_DC_CTRL_SATURATEAT16BIT(x)  (((uint32_t)(((uint32_t)(x)) << DMIC_CHANNEL_DC_CTRL_SATURATEAT16BIT_SHIFT)) & DMIC_CHANNEL_DC_CTRL_SATURATEAT16BIT_MASK)
/*! @} */

/* The count of DMIC_CHANNEL_DC_CTRL */
#define DMIC_CHANNEL_DC_CTRL_COUNT               (2U)

/*! @name CHANEN - Channel Enable register. This register allows enabling either or both PDM channels. */
/*! @{ */
#define DMIC_CHANEN_EN_CH0_MASK                  (0x1U)
#define DMIC_CHANEN_EN_CH0_SHIFT                 (0U)
/*! EN_CH0 - Enable channel 0. When 1, PDM channel 0 is enabled.
 */
#define DMIC_CHANEN_EN_CH0(x)                    (((uint32_t)(((uint32_t)(x)) << DMIC_CHANEN_EN_CH0_SHIFT)) & DMIC_CHANEN_EN_CH0_MASK)
#define DMIC_CHANEN_EN_CH1_MASK                  (0x2U)
#define DMIC_CHANEN_EN_CH1_SHIFT                 (1U)
/*! EN_CH1 - Enable channel 1. When 1, PDM channel 1 is enabled.
 */
#define DMIC_CHANEN_EN_CH1(x)                    (((uint32_t)(((uint32_t)(x)) << DMIC_CHANEN_EN_CH1_SHIFT)) & DMIC_CHANEN_EN_CH1_MASK)
/*! @} */

/*! @name IOCFG - I/O Configuration register. This register configures the use of the PDM pins. */
/*! @{ */
#define DMIC_IOCFG_CLK_BYPASS0_MASK              (0x1U)
#define DMIC_IOCFG_CLK_BYPASS0_SHIFT             (0U)
/*! CLK_BYPASS0 - Bypass CLK0. When 1, PDM_DATA1 becomes the clock for PDM channel 0. This provides
 *    for the possibility of an external codec taking over the PDM bus.
 */
#define DMIC_IOCFG_CLK_BYPASS0(x)                (((uint32_t)(((uint32_t)(x)) << DMIC_IOCFG_CLK_BYPASS0_SHIFT)) & DMIC_IOCFG_CLK_BYPASS0_MASK)
#define DMIC_IOCFG_CLK_BYPASS1_MASK              (0x2U)
#define DMIC_IOCFG_CLK_BYPASS1_SHIFT             (1U)
/*! CLK_BYPASS1 - Bypass CLK1. When 1, PDM_DATA1 becomes the clock for PDM channel 1. This provides
 *    for the possibility of an external codec taking over the PDM bus.
 */
#define DMIC_IOCFG_CLK_BYPASS1(x)                (((uint32_t)(((uint32_t)(x)) << DMIC_IOCFG_CLK_BYPASS1_SHIFT)) & DMIC_IOCFG_CLK_BYPASS1_MASK)
#define DMIC_IOCFG_STEREO_DATA0_MASK             (0x4U)
#define DMIC_IOCFG_STEREO_DATA0_SHIFT            (2U)
/*! STEREO_DATA0 - Stereo PDM select. When 1, PDM_DATA0 is routed to both PDM channels in a
 *    configuration that supports a single stereo digital microphone.
 */
#define DMIC_IOCFG_STEREO_DATA0(x)               (((uint32_t)(((uint32_t)(x)) << DMIC_IOCFG_STEREO_DATA0_SHIFT)) & DMIC_IOCFG_STEREO_DATA0_MASK)
/*! @} */

/*! @name USE2FS - Use 2FS register. This register allow selecting 2FS output rather than 1FS output. */
/*! @{ */
#define DMIC_USE2FS_USE2FS_MASK                  (0x1U)
#define DMIC_USE2FS_USE2FS_SHIFT                 (0U)
/*! USE2FS - Use 2FS register. 0: Use 1FS output for PCM data. 1: Use 2FS output for PCM data.
 */
#define DMIC_USE2FS_USE2FS(x)                    (((uint32_t)(((uint32_t)(x)) << DMIC_USE2FS_USE2FS_SHIFT)) & DMIC_USE2FS_USE2FS_MASK)
/*! @} */

/*! @name HWVADGAIN - HWVAD input gain register. This register controls the input gain of the HWVAD. */
/*! @{ */
#define DMIC_HWVADGAIN_INPUTGAIN_MASK            (0xFU)
#define DMIC_HWVADGAIN_INPUTGAIN_SHIFT           (0U)
/*! INPUTGAIN - Shift value for input bits. 0x0: -10 bits; 0x1: -8 bits; 0x2: -6 bits; 0x3: -4 bits;
 *    0x4: -2 bits; 0x5: 0 bits (default); 0x6: +2 bits; 0x7: +4 bits; 0x8: +6 bits; 0x9: +8 bits;
 *    0xA: +10 bits; 0xB: +12 bits; 0xC: +14 bits; 0xD to 0xF: Reserved.
 */
#define DMIC_HWVADGAIN_INPUTGAIN(x)              (((uint32_t)(((uint32_t)(x)) << DMIC_HWVADGAIN_INPUTGAIN_SHIFT)) & DMIC_HWVADGAIN_INPUTGAIN_MASK)
/*! @} */

/*! @name HWVADHPFS - HWVAD filter control register. This register controls the HWVAD filter setting. */
/*! @{ */
#define DMIC_HWVADHPFS_HPFS_MASK                 (0x3U)
#define DMIC_HWVADHPFS_HPFS_SHIFT                (0U)
/*! HPFS - High pass filter. 0: First filter by-pass; 1: High pass filter with -3dB cut-off at
 *    1750Hz; 2: High pass filter with -3dB cut-off at 215Hz.; 3: Reserved. This filter setting parameter
 *    can be used to optimize performance for different background noise situations. In order to
 *    find the best setting, software needs to perform a rough spectral analysis of the audio signal.
 *    Rule of thumb: If the amount of low-frequency content in the background noise is small, then
 *    set HPFS=0x2, otherwise use 0x1.
 */
#define DMIC_HWVADHPFS_HPFS(x)                   (((uint32_t)(((uint32_t)(x)) << DMIC_HWVADHPFS_HPFS_SHIFT)) & DMIC_HWVADHPFS_HPFS_MASK)
/*! @} */

/*! @name HWVADST10 - HWVAD control register. This register controls the operation of the filter block and resets the internal interrut flag. */
/*! @{ */
#define DMIC_HWVADST10_ST10_MASK                 (0x1U)
#define DMIC_HWVADST10_ST10_SHIFT                (0U)
/*! ST10 - ST10. Once the HWVAD has triggered an interrupt, a short '1' pulse on bit ST10 clears the
 *    interrupt. Alternatively, keeping the bit on '1' level for some time has a special function
 *    for filter convergence. 0: Normal operation, waiting for HWVAD trigger event (stage 0). 1:
 *    Reset internal interrupt flag by writing a 1 pulse.
 */
#define DMIC_HWVADST10_ST10(x)                   (((uint32_t)(((uint32_t)(x)) << DMIC_HWVADST10_ST10_SHIFT)) & DMIC_HWVADST10_ST10_MASK)
/*! @} */

/*! @name HWVADRSTT - HWVAD filter reset register */
/*! @{ */
#define DMIC_HWVADRSTT_RSTT_MASK                 (0x1U)
#define DMIC_HWVADRSTT_RSTT_SHIFT                (0U)
/*! RSTT - HWVAD filter reset. Writing a 1, then writing a '0' resets all filter values. 0: Filters
 *    anr not held in reset. 1: Holds the filters in reset.
 */
#define DMIC_HWVADRSTT_RSTT(x)                   (((uint32_t)(((uint32_t)(x)) << DMIC_HWVADRSTT_RSTT_SHIFT)) & DMIC_HWVADRSTT_RSTT_MASK)
/*! @} */

/*! @name HWVADTHGN - HWVAD noise estimator gain register */
/*! @{ */
#define DMIC_HWVADTHGN_THGN_MASK                 (0xFU)
#define DMIC_HWVADTHGN_THGN_SHIFT                (0U)
/*! THGN - Gain value for the noise estimator. Values 0 to 14. 0 corresponds to a gain of 1. THGN
 *    and THGS are used within the hardware to determine when to assert the HWVAD result.
 */
#define DMIC_HWVADTHGN_THGN(x)                   (((uint32_t)(((uint32_t)(x)) << DMIC_HWVADTHGN_THGN_SHIFT)) & DMIC_HWVADTHGN_THGN_MASK)
/*! @} */

/*! @name HWVADTHGS - HWVAD signal estimator gain register */
/*! @{ */
#define DMIC_HWVADTHGS_THGS_MASK                 (0xFU)
#define DMIC_HWVADTHGS_THGS_SHIFT                (0U)
/*! THGS - Gain value for the signal estimator. Values 0 to 14. 0 corresponds to a gain of 1. THGN
 *    and THGS are used within the hardware to determine when to assert the HWVAD result.
 */
#define DMIC_HWVADTHGS_THGS(x)                   (((uint32_t)(((uint32_t)(x)) << DMIC_HWVADTHGS_THGS_SHIFT)) & DMIC_HWVADTHGS_THGS_MASK)
/*! @} */

/*! @name HWVADLOWZ - HWVAD noise envelope estimator register */
/*! @{ */
#define DMIC_HWVADLOWZ_LOWZ_MASK                 (0xFFFFU)
#define DMIC_HWVADLOWZ_LOWZ_SHIFT                (0U)
/*! LOWZ - Noise envelope estimator value. This register contains 2 bytes of the output of filter
 *    stage z7. It can be used as an indication for the noise floor and must be evaluated by software.
 *    Note: For power saving reasons this register is not synchronized to the AHB bus clock domain.
 *    To ensure correct data is read, the register should be read twice. If the data is the same,
 *    then the data is correct, if not, the register should be read one more time. The noise floor is
 *    a slowly moving calculation, so several reads in a row can guarantee that register value
 *    being read can be assured to not be in the middle of a transition.
 */
#define DMIC_HWVADLOWZ_LOWZ(x)                   (((uint32_t)(((uint32_t)(x)) << DMIC_HWVADLOWZ_LOWZ_SHIFT)) & DMIC_HWVADLOWZ_LOWZ_MASK)
/*! @} */

/*! @name ID - Module Identification register */
/*! @{ */
#define DMIC_ID_ID_MASK                          (0xFFFFFFFFU)
#define DMIC_ID_ID_SHIFT                         (0U)
/*! ID - Indicates module ID and the number of channels in this DMIC interface.
 */
#define DMIC_ID_ID(x)                            (((uint32_t)(((uint32_t)(x)) << DMIC_ID_ID_SHIFT)) & DMIC_ID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group DMIC_Register_Masks */


/* DMIC - Peripheral instance base addresses */
/** Peripheral DMIC0 base address */
#define DMIC0_BASE                               (0x4008A000u)
/** Peripheral DMIC0 base pointer */
#define DMIC0                                    ((DMIC_Type *)DMIC0_BASE)
/** Array initializer of DMIC peripheral base addresses */
#define DMIC_BASE_ADDRS                          { DMIC0_BASE }
/** Array initializer of DMIC peripheral base pointers */
#define DMIC_BASE_PTRS                           { DMIC0 }
/** Interrupt vectors for the DMIC peripheral type */
#define DMIC_IRQS                                { DMIC0_IRQn }
#define DMIC_HWVAD_IRQS                          { HWVAD0_IRQn }

/*!
 * @}
 */ /* end of group DMIC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- FLASH Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FLASH_Peripheral_Access_Layer FLASH Peripheral Access Layer
 * @{
 */

/** FLASH - Register Layout Typedef */
typedef struct {
  __O  uint32_t CMD;                               /**< command register, offset: 0x0 */
  __O  uint32_t EVENT;                             /**< event register, offset: 0x4 */
       uint8_t RESERVED_0[4];
  __IO uint32_t AUTOPROG;                          /**< specifies what commands are performed on AHB write, offset: 0xC */
  __IO uint32_t STARTA;                            /**< start address for next flash command, offset: 0x10 */
  __IO uint32_t STOPA;                             /**< end address for next flash command, if command operates on address ranges, offset: 0x14 */
       uint8_t RESERVED_1[104];
  __IO uint32_t DATAW[4];                          /**< data register, word 0-3; Memory data, or command parameter, or command result., array offset: 0x80, array step: 0x4 */
       uint8_t RESERVED_2[3912];
  __O  uint32_t INT_CLR_ENABLE;                    /**< Clear interrupt enable bits, offset: 0xFD8 */
  __O  uint32_t INT_SET_ENABLE;                    /**< Set interrupt enable bits, offset: 0xFDC */
  __I  uint32_t INT_STATUS;                        /**< Interrupt status bits, offset: 0xFE0 */
  __I  uint32_t INT_ENABLE;                        /**< Interrupt enable bits, offset: 0xFE4 */
  __O  uint32_t INT_CLR_STATUS;                    /**< Clear interrupt status bits, offset: 0xFE8 */
  __O  uint32_t INT_SET_STATUS;                    /**< Set interrupt status bits, offset: 0xFEC */
       uint8_t RESERVED_3[12];
  __I  uint32_t MODULE_ID;                         /**< Controller and Memory module identification, offset: 0xFFC */
} FLASH_Type;

/* ----------------------------------------------------------------------------
   -- FLASH Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FLASH_Register_Masks FLASH Register Masks
 * @{
 */

/*! @name CMD - command register */
/*! @{ */
#define FLASH_CMD_CMD_MASK                       (0xFFFFFFFFU)
#define FLASH_CMD_CMD_SHIFT                      (0U)
/*! CMD - command register
 */
#define FLASH_CMD_CMD(x)                         (((uint32_t)(((uint32_t)(x)) << FLASH_CMD_CMD_SHIFT)) & FLASH_CMD_CMD_MASK)
/*! @} */

/*! @name EVENT - event register */
/*! @{ */
#define FLASH_EVENT_RST_MASK                     (0x1U)
#define FLASH_EVENT_RST_SHIFT                    (0U)
/*! RST - When bit is set, the controller and flash are reset.
 */
#define FLASH_EVENT_RST(x)                       (((uint32_t)(((uint32_t)(x)) << FLASH_EVENT_RST_SHIFT)) & FLASH_EVENT_RST_MASK)
#define FLASH_EVENT_WAKEUP_MASK                  (0x2U)
#define FLASH_EVENT_WAKEUP_SHIFT                 (1U)
/*! WAKEUP - When bit is set, the controller wakes up from whatever low power or powerdown mode was
 *    active. If not in a powerdown mode, this bit has no effect.
 */
#define FLASH_EVENT_WAKEUP(x)                    (((uint32_t)(((uint32_t)(x)) << FLASH_EVENT_WAKEUP_SHIFT)) & FLASH_EVENT_WAKEUP_MASK)
#define FLASH_EVENT_ABORT_MASK                   (0x4U)
#define FLASH_EVENT_ABORT_SHIFT                  (2U)
/*! ABORT - When bit is set, a running program/erase command is aborted.
 */
#define FLASH_EVENT_ABORT(x)                     (((uint32_t)(((uint32_t)(x)) << FLASH_EVENT_ABORT_SHIFT)) & FLASH_EVENT_ABORT_MASK)
/*! @} */

/*! @name AUTOPROG - specifies what commands are performed on AHB write */
/*! @{ */
#define FLASH_AUTOPROG_AUTOPROG_MASK             (0x3U)
#define FLASH_AUTOPROG_AUTOPROG_SHIFT            (0U)
/*! AUTOPROG - Auto programmings configuration. 00: auto programming switched off. 01: execute write
 *    word . 10: execute write word then, if the last word in a page was written, program page .
 *    11: reserved for future use / no action.
 */
#define FLASH_AUTOPROG_AUTOPROG(x)               (((uint32_t)(((uint32_t)(x)) << FLASH_AUTOPROG_AUTOPROG_SHIFT)) & FLASH_AUTOPROG_AUTOPROG_MASK)
/*! @} */

/*! @name STARTA - start address for next flash command */
/*! @{ */
#define FLASH_STARTA_STARTA_MASK                 (0x3FFFFU)
#define FLASH_STARTA_STARTA_SHIFT                (0U)
/*! STARTA - Address / Start address for commands that take an address (range) as a parameter. The
 *    address is in units of memory words, not bytes.
 */
#define FLASH_STARTA_STARTA(x)                   (((uint32_t)(((uint32_t)(x)) << FLASH_STARTA_STARTA_SHIFT)) & FLASH_STARTA_STARTA_MASK)
/*! @} */

/*! @name STOPA - end address for next flash command, if command operates on address ranges */
/*! @{ */
#define FLASH_STOPA_STOPA_MASK                   (0x3FFFFU)
#define FLASH_STOPA_STOPA_SHIFT                  (0U)
/*! STOPA - Stop address for commands that take an address range as a parameter (the word specified
 *    by STOPA is included in the address range). The address is in units of memory words, not bytes.
 */
#define FLASH_STOPA_STOPA(x)                     (((uint32_t)(((uint32_t)(x)) << FLASH_STOPA_STOPA_SHIFT)) & FLASH_STOPA_STOPA_MASK)
/*! @} */

/*! @name DATAW - data register, word 0-3; Memory data, or command parameter, or command result. */
/*! @{ */
#define FLASH_DATAW_DATAW_MASK                   (0xFFFFFFFFU)
#define FLASH_DATAW_DATAW_SHIFT                  (0U)
/*! DATAW - data register, word 0-3; Memory data, or command parameter, or command result.
 */
#define FLASH_DATAW_DATAW(x)                     (((uint32_t)(((uint32_t)(x)) << FLASH_DATAW_DATAW_SHIFT)) & FLASH_DATAW_DATAW_MASK)
/*! @} */

/* The count of FLASH_DATAW */
#define FLASH_DATAW_COUNT                        (4U)

/*! @name INT_CLR_ENABLE - Clear interrupt enable bits */
/*! @{ */
#define FLASH_INT_CLR_ENABLE_FAIL_MASK           (0x1U)
#define FLASH_INT_CLR_ENABLE_FAIL_SHIFT          (0U)
/*! FAIL - When a CLR_ENABLE bit is written to 1, the corresponding INT_ENABLE bit is cleared
 */
#define FLASH_INT_CLR_ENABLE_FAIL(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_INT_CLR_ENABLE_FAIL_SHIFT)) & FLASH_INT_CLR_ENABLE_FAIL_MASK)
#define FLASH_INT_CLR_ENABLE_ERR_MASK            (0x2U)
#define FLASH_INT_CLR_ENABLE_ERR_SHIFT           (1U)
/*! ERR - When a CLR_ENABLE bit is written to 1, the corresponding INT_ENABLE bit is cleared
 */
#define FLASH_INT_CLR_ENABLE_ERR(x)              (((uint32_t)(((uint32_t)(x)) << FLASH_INT_CLR_ENABLE_ERR_SHIFT)) & FLASH_INT_CLR_ENABLE_ERR_MASK)
#define FLASH_INT_CLR_ENABLE_DONE_MASK           (0x4U)
#define FLASH_INT_CLR_ENABLE_DONE_SHIFT          (2U)
/*! DONE - When a CLR_ENABLE bit is written to 1, the corresponding INT_ENABLE bit is cleared
 */
#define FLASH_INT_CLR_ENABLE_DONE(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_INT_CLR_ENABLE_DONE_SHIFT)) & FLASH_INT_CLR_ENABLE_DONE_MASK)
#define FLASH_INT_CLR_ENABLE_ECC_ERR_MASK        (0x8U)
#define FLASH_INT_CLR_ENABLE_ECC_ERR_SHIFT       (3U)
/*! ECC_ERR - When a CLR_ENABLE bit is written to 1, the corresponding INT_ENABLE bit is cleared
 */
#define FLASH_INT_CLR_ENABLE_ECC_ERR(x)          (((uint32_t)(((uint32_t)(x)) << FLASH_INT_CLR_ENABLE_ECC_ERR_SHIFT)) & FLASH_INT_CLR_ENABLE_ECC_ERR_MASK)
/*! @} */

/*! @name INT_SET_ENABLE - Set interrupt enable bits */
/*! @{ */
#define FLASH_INT_SET_ENABLE_FAIL_MASK           (0x1U)
#define FLASH_INT_SET_ENABLE_FAIL_SHIFT          (0U)
/*! FAIL - When a SET_ENABLE bit is written to 1, the corresponding INT_ENABLE bit is set
 */
#define FLASH_INT_SET_ENABLE_FAIL(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_INT_SET_ENABLE_FAIL_SHIFT)) & FLASH_INT_SET_ENABLE_FAIL_MASK)
#define FLASH_INT_SET_ENABLE_ERR_MASK            (0x2U)
#define FLASH_INT_SET_ENABLE_ERR_SHIFT           (1U)
/*! ERR - When a SET_ENABLE bit is written to 1, the corresponding INT_ENABLE bit is set
 */
#define FLASH_INT_SET_ENABLE_ERR(x)              (((uint32_t)(((uint32_t)(x)) << FLASH_INT_SET_ENABLE_ERR_SHIFT)) & FLASH_INT_SET_ENABLE_ERR_MASK)
#define FLASH_INT_SET_ENABLE_DONE_MASK           (0x4U)
#define FLASH_INT_SET_ENABLE_DONE_SHIFT          (2U)
/*! DONE - When a SET_ENABLE bit is written to 1, the corresponding INT_ENABLE bit is set
 */
#define FLASH_INT_SET_ENABLE_DONE(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_INT_SET_ENABLE_DONE_SHIFT)) & FLASH_INT_SET_ENABLE_DONE_MASK)
#define FLASH_INT_SET_ENABLE_ECC_ERR_MASK        (0x8U)
#define FLASH_INT_SET_ENABLE_ECC_ERR_SHIFT       (3U)
/*! ECC_ERR - When a SET_ENABLE bit is written to 1, the corresponding INT_ENABLE bit is set
 */
#define FLASH_INT_SET_ENABLE_ECC_ERR(x)          (((uint32_t)(((uint32_t)(x)) << FLASH_INT_SET_ENABLE_ECC_ERR_SHIFT)) & FLASH_INT_SET_ENABLE_ECC_ERR_MASK)
/*! @} */

/*! @name INT_STATUS - Interrupt status bits */
/*! @{ */
#define FLASH_INT_STATUS_FAIL_MASK               (0x1U)
#define FLASH_INT_STATUS_FAIL_SHIFT              (0U)
/*! FAIL - This status bit is set if execution of a (legal) command failed. The flag can be set at
 *    any time during command execution, not just at the end.
 */
#define FLASH_INT_STATUS_FAIL(x)                 (((uint32_t)(((uint32_t)(x)) << FLASH_INT_STATUS_FAIL_SHIFT)) & FLASH_INT_STATUS_FAIL_MASK)
#define FLASH_INT_STATUS_ERR_MASK                (0x2U)
#define FLASH_INT_STATUS_ERR_SHIFT               (1U)
/*! ERR - This status bit is set if execution of an illegal command is detected. A command is
 *    illegal if it is unknown, or it is not allowed in the current mode, or it is violating access
 *    restrictions, or it has invalid parameters.
 */
#define FLASH_INT_STATUS_ERR(x)                  (((uint32_t)(((uint32_t)(x)) << FLASH_INT_STATUS_ERR_SHIFT)) & FLASH_INT_STATUS_ERR_MASK)
#define FLASH_INT_STATUS_DONE_MASK               (0x4U)
#define FLASH_INT_STATUS_DONE_SHIFT              (2U)
/*! DONE - This status bit is set at the end of command execution
 */
#define FLASH_INT_STATUS_DONE(x)                 (((uint32_t)(((uint32_t)(x)) << FLASH_INT_STATUS_DONE_SHIFT)) & FLASH_INT_STATUS_DONE_MASK)
#define FLASH_INT_STATUS_ECC_ERR_MASK            (0x8U)
#define FLASH_INT_STATUS_ECC_ERR_SHIFT           (3U)
/*! ECC_ERR - This status bit is set if, during a memory read operation (either a user-requested
 *    read, or a speculative read, or reads performed by a controller command), a correctable or
 *    uncorrectable error is detected by ECC decoding logic.
 */
#define FLASH_INT_STATUS_ECC_ERR(x)              (((uint32_t)(((uint32_t)(x)) << FLASH_INT_STATUS_ECC_ERR_SHIFT)) & FLASH_INT_STATUS_ECC_ERR_MASK)
/*! @} */

/*! @name INT_ENABLE - Interrupt enable bits */
/*! @{ */
#define FLASH_INT_ENABLE_FAIL_MASK               (0x1U)
#define FLASH_INT_ENABLE_FAIL_SHIFT              (0U)
/*! FAIL - If an INT_ENABLE bit is set, an interrupt request will be generated if the corresponding INT_STATUS bit is high.
 */
#define FLASH_INT_ENABLE_FAIL(x)                 (((uint32_t)(((uint32_t)(x)) << FLASH_INT_ENABLE_FAIL_SHIFT)) & FLASH_INT_ENABLE_FAIL_MASK)
#define FLASH_INT_ENABLE_ERR_MASK                (0x2U)
#define FLASH_INT_ENABLE_ERR_SHIFT               (1U)
/*! ERR - If an INT_ENABLE bit is set, an interrupt request will be generated if the corresponding INT_STATUS bit is high.
 */
#define FLASH_INT_ENABLE_ERR(x)                  (((uint32_t)(((uint32_t)(x)) << FLASH_INT_ENABLE_ERR_SHIFT)) & FLASH_INT_ENABLE_ERR_MASK)
#define FLASH_INT_ENABLE_DONE_MASK               (0x4U)
#define FLASH_INT_ENABLE_DONE_SHIFT              (2U)
/*! DONE - If an INT_ENABLE bit is set, an interrupt request will be generated if the corresponding INT_STATUS bit is high.
 */
#define FLASH_INT_ENABLE_DONE(x)                 (((uint32_t)(((uint32_t)(x)) << FLASH_INT_ENABLE_DONE_SHIFT)) & FLASH_INT_ENABLE_DONE_MASK)
#define FLASH_INT_ENABLE_ECC_ERR_MASK            (0x8U)
#define FLASH_INT_ENABLE_ECC_ERR_SHIFT           (3U)
/*! ECC_ERR - If an INT_ENABLE bit is set, an interrupt request will be generated if the corresponding INT_STATUS bit is high.
 */
#define FLASH_INT_ENABLE_ECC_ERR(x)              (((uint32_t)(((uint32_t)(x)) << FLASH_INT_ENABLE_ECC_ERR_SHIFT)) & FLASH_INT_ENABLE_ECC_ERR_MASK)
/*! @} */

/*! @name INT_CLR_STATUS - Clear interrupt status bits */
/*! @{ */
#define FLASH_INT_CLR_STATUS_FAIL_MASK           (0x1U)
#define FLASH_INT_CLR_STATUS_FAIL_SHIFT          (0U)
/*! FAIL - When a CLR_STATUS bit is written to 1, the corresponding INT_STATUS bit is cleared
 */
#define FLASH_INT_CLR_STATUS_FAIL(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_INT_CLR_STATUS_FAIL_SHIFT)) & FLASH_INT_CLR_STATUS_FAIL_MASK)
#define FLASH_INT_CLR_STATUS_ERR_MASK            (0x2U)
#define FLASH_INT_CLR_STATUS_ERR_SHIFT           (1U)
/*! ERR - When a CLR_STATUS bit is written to 1, the corresponding INT_STATUS bit is cleared
 */
#define FLASH_INT_CLR_STATUS_ERR(x)              (((uint32_t)(((uint32_t)(x)) << FLASH_INT_CLR_STATUS_ERR_SHIFT)) & FLASH_INT_CLR_STATUS_ERR_MASK)
#define FLASH_INT_CLR_STATUS_DONE_MASK           (0x4U)
#define FLASH_INT_CLR_STATUS_DONE_SHIFT          (2U)
/*! DONE - When a CLR_STATUS bit is written to 1, the corresponding INT_STATUS bit is cleared
 */
#define FLASH_INT_CLR_STATUS_DONE(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_INT_CLR_STATUS_DONE_SHIFT)) & FLASH_INT_CLR_STATUS_DONE_MASK)
#define FLASH_INT_CLR_STATUS_ECC_ERR_MASK        (0x8U)
#define FLASH_INT_CLR_STATUS_ECC_ERR_SHIFT       (3U)
/*! ECC_ERR - When a CLR_STATUS bit is written to 1, the corresponding INT_STATUS bit is cleared
 */
#define FLASH_INT_CLR_STATUS_ECC_ERR(x)          (((uint32_t)(((uint32_t)(x)) << FLASH_INT_CLR_STATUS_ECC_ERR_SHIFT)) & FLASH_INT_CLR_STATUS_ECC_ERR_MASK)
/*! @} */

/*! @name INT_SET_STATUS - Set interrupt status bits */
/*! @{ */
#define FLASH_INT_SET_STATUS_FAIL_MASK           (0x1U)
#define FLASH_INT_SET_STATUS_FAIL_SHIFT          (0U)
/*! FAIL - When a SET_STATUS bit is written to 1, the corresponding INT_STATUS bit is set
 */
#define FLASH_INT_SET_STATUS_FAIL(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_INT_SET_STATUS_FAIL_SHIFT)) & FLASH_INT_SET_STATUS_FAIL_MASK)
#define FLASH_INT_SET_STATUS_ERR_MASK            (0x2U)
#define FLASH_INT_SET_STATUS_ERR_SHIFT           (1U)
/*! ERR - When a SET_STATUS bit is written to 1, the corresponding INT_STATUS bit is set
 */
#define FLASH_INT_SET_STATUS_ERR(x)              (((uint32_t)(((uint32_t)(x)) << FLASH_INT_SET_STATUS_ERR_SHIFT)) & FLASH_INT_SET_STATUS_ERR_MASK)
#define FLASH_INT_SET_STATUS_DONE_MASK           (0x4U)
#define FLASH_INT_SET_STATUS_DONE_SHIFT          (2U)
/*! DONE - When a SET_STATUS bit is written to 1, the corresponding INT_STATUS bit is set
 */
#define FLASH_INT_SET_STATUS_DONE(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_INT_SET_STATUS_DONE_SHIFT)) & FLASH_INT_SET_STATUS_DONE_MASK)
#define FLASH_INT_SET_STATUS_ECC_ERR_MASK        (0x8U)
#define FLASH_INT_SET_STATUS_ECC_ERR_SHIFT       (3U)
/*! ECC_ERR - When a SET_STATUS bit is written to 1, the corresponding INT_STATUS bit is set
 */
#define FLASH_INT_SET_STATUS_ECC_ERR(x)          (((uint32_t)(((uint32_t)(x)) << FLASH_INT_SET_STATUS_ECC_ERR_SHIFT)) & FLASH_INT_SET_STATUS_ECC_ERR_MASK)
/*! @} */

/*! @name MODULE_ID - Controller and Memory module identification */
/*! @{ */
#define FLASH_MODULE_ID_APERTURE_MASK            (0xFFU)
#define FLASH_MODULE_ID_APERTURE_SHIFT           (0U)
/*! APERTURE - Aperture i.e. number minus 1 of consecutive packets 4 Kbytes reserved for this IP
 */
#define FLASH_MODULE_ID_APERTURE(x)              (((uint32_t)(((uint32_t)(x)) << FLASH_MODULE_ID_APERTURE_SHIFT)) & FLASH_MODULE_ID_APERTURE_MASK)
#define FLASH_MODULE_ID_MINOR_REV_MASK           (0xF00U)
#define FLASH_MODULE_ID_MINOR_REV_SHIFT          (8U)
/*! MINOR_REV - Minor revision i.e. with no software consequences
 */
#define FLASH_MODULE_ID_MINOR_REV(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_MODULE_ID_MINOR_REV_SHIFT)) & FLASH_MODULE_ID_MINOR_REV_MASK)
#define FLASH_MODULE_ID_MAJOR_REV_MASK           (0xF000U)
#define FLASH_MODULE_ID_MAJOR_REV_SHIFT          (12U)
/*! MAJOR_REV - Major revision i.e. implies software modifications
 */
#define FLASH_MODULE_ID_MAJOR_REV(x)             (((uint32_t)(((uint32_t)(x)) << FLASH_MODULE_ID_MAJOR_REV_SHIFT)) & FLASH_MODULE_ID_MAJOR_REV_MASK)
#define FLASH_MODULE_ID_ID_MASK                  (0xFFFF0000U)
#define FLASH_MODULE_ID_ID_SHIFT                 (16U)
/*! ID - Identifier. This is the unique identifier of the module
 */
#define FLASH_MODULE_ID_ID(x)                    (((uint32_t)(((uint32_t)(x)) << FLASH_MODULE_ID_ID_SHIFT)) & FLASH_MODULE_ID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group FLASH_Register_Masks */


/* FLASH - Peripheral instance base addresses */
/** Peripheral FLASH base address */
#define FLASH_BASE                               (0x40009000u)
/** Peripheral FLASH base pointer */
#define FLASH                                    ((FLASH_Type *)FLASH_BASE)
/** Array initializer of FLASH peripheral base addresses */
#define FLASH_BASE_ADDRS                         { FLASH_BASE }
/** Array initializer of FLASH peripheral base pointers */
#define FLASH_BASE_PTRS                          { FLASH }

/*!
 * @}
 */ /* end of group FLASH_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- FLEXCOMM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FLEXCOMM_Peripheral_Access_Layer FLEXCOMM Peripheral Access Layer
 * @{
 */

/** FLEXCOMM - Register Layout Typedef */
typedef struct {
       uint8_t RESERVED_0[4088];
  __IO uint32_t PSELID;                            /**< Peripheral Select and Flexcomm ID register., offset: 0xFF8 */
  __IO uint32_t PID;                               /**< Peripheral identification register., offset: 0xFFC */
} FLEXCOMM_Type;

/* ----------------------------------------------------------------------------
   -- FLEXCOMM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup FLEXCOMM_Register_Masks FLEXCOMM Register Masks
 * @{
 */

/*! @name PSELID - Peripheral Select and Flexcomm ID register. */
/*! @{ */
#define FLEXCOMM_PSELID_PERSEL_MASK              (0x7U)
#define FLEXCOMM_PSELID_PERSEL_SHIFT             (0U)
/*! PERSEL - Peripheral Select. This field is writable by software.
 *  0b000..No peripheral selected.
 *  0b001..USART function selected.
 *  0b010..SPI function selected.
 *  0b011..I2C function selected.
 *  0b100..I2S transmit function selected.
 *  0b101..I2S receive function selected.
 *  0b110..Reserved
 *  0b111..Reserved
 */
#define FLEXCOMM_PSELID_PERSEL(x)                (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PSELID_PERSEL_SHIFT)) & FLEXCOMM_PSELID_PERSEL_MASK)
#define FLEXCOMM_PSELID_LOCK_MASK                (0x8U)
#define FLEXCOMM_PSELID_LOCK_SHIFT               (3U)
/*! LOCK - Lock the peripheral select. This field is writable by software.
 *  0b0..Peripheral select can be changed by software.
 *  0b1..Peripheral select is locked and cannot be changed until this Flexcomm or the entire device is reset.
 */
#define FLEXCOMM_PSELID_LOCK(x)                  (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PSELID_LOCK_SHIFT)) & FLEXCOMM_PSELID_LOCK_MASK)
#define FLEXCOMM_PSELID_USARTPRESENT_MASK        (0x10U)
#define FLEXCOMM_PSELID_USARTPRESENT_SHIFT       (4U)
/*! USARTPRESENT - USART present indicator. This field is Read-only.
 *  0b0..This Flexcomm does not include the USART function.
 *  0b1..This Flexcomm includes the USART function.
 */
#define FLEXCOMM_PSELID_USARTPRESENT(x)          (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PSELID_USARTPRESENT_SHIFT)) & FLEXCOMM_PSELID_USARTPRESENT_MASK)
#define FLEXCOMM_PSELID_SPIPRESENT_MASK          (0x20U)
#define FLEXCOMM_PSELID_SPIPRESENT_SHIFT         (5U)
/*! SPIPRESENT - SPI present indicator. This field is Read-only.
 *  0b0..This Flexcomm does not include the SPI function.
 *  0b1..This Flexcomm includes the SPI function.
 */
#define FLEXCOMM_PSELID_SPIPRESENT(x)            (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PSELID_SPIPRESENT_SHIFT)) & FLEXCOMM_PSELID_SPIPRESENT_MASK)
#define FLEXCOMM_PSELID_I2CPRESENT_MASK          (0x40U)
#define FLEXCOMM_PSELID_I2CPRESENT_SHIFT         (6U)
/*! I2CPRESENT - I2C present indicator. This field is Read-only.
 *  0b0..This Flexcomm does not include the I2C function.
 *  0b1..This Flexcomm includes the I2C function.
 */
#define FLEXCOMM_PSELID_I2CPRESENT(x)            (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PSELID_I2CPRESENT_SHIFT)) & FLEXCOMM_PSELID_I2CPRESENT_MASK)
#define FLEXCOMM_PSELID_I2SPRESENT_MASK          (0x80U)
#define FLEXCOMM_PSELID_I2SPRESENT_SHIFT         (7U)
/*! I2SPRESENT - I 2S present indicator. This field is Read-only.
 *  0b0..This Flexcomm does not include the I2S function.
 *  0b1..This Flexcomm includes the I2S function.
 */
#define FLEXCOMM_PSELID_I2SPRESENT(x)            (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PSELID_I2SPRESENT_SHIFT)) & FLEXCOMM_PSELID_I2SPRESENT_MASK)
#define FLEXCOMM_PSELID_ID_MASK                  (0xFFFFF000U)
#define FLEXCOMM_PSELID_ID_SHIFT                 (12U)
/*! ID - Flexcomm ID.
 */
#define FLEXCOMM_PSELID_ID(x)                    (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PSELID_ID_SHIFT)) & FLEXCOMM_PSELID_ID_MASK)
/*! @} */

/*! @name PID - Peripheral identification register. */
/*! @{ */
#define FLEXCOMM_PID_Minor_Rev_MASK              (0xF00U)
#define FLEXCOMM_PID_Minor_Rev_SHIFT             (8U)
/*! Minor_Rev - Minor revision of module implementation.
 */
#define FLEXCOMM_PID_Minor_Rev(x)                (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PID_Minor_Rev_SHIFT)) & FLEXCOMM_PID_Minor_Rev_MASK)
#define FLEXCOMM_PID_Major_Rev_MASK              (0xF000U)
#define FLEXCOMM_PID_Major_Rev_SHIFT             (12U)
/*! Major_Rev - Major revision of module implementation.
 */
#define FLEXCOMM_PID_Major_Rev(x)                (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PID_Major_Rev_SHIFT)) & FLEXCOMM_PID_Major_Rev_MASK)
#define FLEXCOMM_PID_ID_MASK                     (0xFFFF0000U)
#define FLEXCOMM_PID_ID_SHIFT                    (16U)
/*! ID - Module identifier for the selected function.
 */
#define FLEXCOMM_PID_ID(x)                       (((uint32_t)(((uint32_t)(x)) << FLEXCOMM_PID_ID_SHIFT)) & FLEXCOMM_PID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group FLEXCOMM_Register_Masks */


/* FLEXCOMM - Peripheral instance base addresses */
/** Peripheral FLEXCOMM0 base address */
#define FLEXCOMM0_BASE                           (0x4008B000u)
/** Peripheral FLEXCOMM0 base pointer */
#define FLEXCOMM0                                ((FLEXCOMM_Type *)FLEXCOMM0_BASE)
/** Peripheral FLEXCOMM1 base address */
#define FLEXCOMM1_BASE                           (0x4008C000u)
/** Peripheral FLEXCOMM1 base pointer */
#define FLEXCOMM1                                ((FLEXCOMM_Type *)FLEXCOMM1_BASE)
/** Peripheral FLEXCOMM2 base address */
#define FLEXCOMM2_BASE                           (0x40003000u)
/** Peripheral FLEXCOMM2 base pointer */
#define FLEXCOMM2                                ((FLEXCOMM_Type *)FLEXCOMM2_BASE)
/** Peripheral FLEXCOMM3 base address */
#define FLEXCOMM3_BASE                           (0x40004000u)
/** Peripheral FLEXCOMM3 base pointer */
#define FLEXCOMM3                                ((FLEXCOMM_Type *)FLEXCOMM3_BASE)
/** Peripheral FLEXCOMM4 base address */
#define FLEXCOMM4_BASE                           (0x4008D000u)
/** Peripheral FLEXCOMM4 base pointer */
#define FLEXCOMM4                                ((FLEXCOMM_Type *)FLEXCOMM4_BASE)
/** Peripheral FLEXCOMM5 base address */
#define FLEXCOMM5_BASE                           (0x4008E000u)
/** Peripheral FLEXCOMM5 base pointer */
#define FLEXCOMM5                                ((FLEXCOMM_Type *)FLEXCOMM5_BASE)
/** Peripheral FLEXCOMM6 base address */
#define FLEXCOMM6_BASE                           (0x40005000u)
/** Peripheral FLEXCOMM6 base pointer */
#define FLEXCOMM6                                ((FLEXCOMM_Type *)FLEXCOMM6_BASE)
/** Array initializer of FLEXCOMM peripheral base addresses */
#define FLEXCOMM_BASE_ADDRS                      { FLEXCOMM0_BASE, FLEXCOMM1_BASE, FLEXCOMM2_BASE, FLEXCOMM3_BASE, FLEXCOMM4_BASE, FLEXCOMM5_BASE, FLEXCOMM6_BASE }
/** Array initializer of FLEXCOMM peripheral base pointers */
#define FLEXCOMM_BASE_PTRS                       { FLEXCOMM0, FLEXCOMM1, FLEXCOMM2, FLEXCOMM3, FLEXCOMM4, FLEXCOMM5, FLEXCOMM6 }
/** Interrupt vectors for the FLEXCOMM peripheral type */
#define FLEXCOMM_IRQS                            { FLEXCOMM0_IRQn, FLEXCOMM1_IRQn, FLEXCOMM2_IRQn, FLEXCOMM3_IRQn, FLEXCOMM4_IRQn, FLEXCOMM5_IRQn, FLEXCOMM6_IRQn }

/*!
 * @}
 */ /* end of group FLEXCOMM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- GINT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup GINT_Peripheral_Access_Layer GINT Peripheral Access Layer
 * @{
 */

/** GINT - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< GPIO Grouped interrupt control register, offset: 0x0 */
       uint8_t RESERVED_0[28];
  __IO uint32_t PORT_POL[1];                       /**< GPIO Grouped Interrupt polarity register. Configure the pin polarity of each PIO signal into the group interrupt function. If a bit is low then the corresponding PIO has an active low contribution into the group interrupt. If a bit is high then the corresponding PIO has an active high contribution., array offset: 0x20, array step: 0x4 */
       uint8_t RESERVED_1[28];
  __IO uint32_t PORT_ENA[1];                       /**< GPIO Grouped Interrupt port enable register. When a bit is set then the corresponding PIO is enabled for the group interrupt function., array offset: 0x40, array step: 0x4 */
} GINT_Type;

/* ----------------------------------------------------------------------------
   -- GINT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup GINT_Register_Masks GINT Register Masks
 * @{
 */

/*! @name CTRL - GPIO Grouped interrupt control register */
/*! @{ */
#define GINT_CTRL_INT_MASK                       (0x1U)
#define GINT_CTRL_INT_SHIFT                      (0U)
/*! INT - Group interrupt status. This bit is cleared by writing a one to it. Writing zero has no effect.
 */
#define GINT_CTRL_INT(x)                         (((uint32_t)(((uint32_t)(x)) << GINT_CTRL_INT_SHIFT)) & GINT_CTRL_INT_MASK)
#define GINT_CTRL_COMB_MASK                      (0x2U)
#define GINT_CTRL_COMB_SHIFT                     (1U)
/*! COMB - Combine enabled inputs for group interrupt. 0: Or, OR functionality: A grouped interrupt
 *    is generated when any one of the enabled inputs is active (based on its programmed polarity)
 *    1: And, AND functionality: An interrupt is generated when all enabled bits are active (based on
 *    their programmed polarity)
 */
#define GINT_CTRL_COMB(x)                        (((uint32_t)(((uint32_t)(x)) << GINT_CTRL_COMB_SHIFT)) & GINT_CTRL_COMB_MASK)
#define GINT_CTRL_TRIG_MASK                      (0x4U)
#define GINT_CTRL_TRIG_SHIFT                     (2U)
/*! TRIG - Group interrupt trigger. 0: Edge Triggered. 1: Level Triggered.
 */
#define GINT_CTRL_TRIG(x)                        (((uint32_t)(((uint32_t)(x)) << GINT_CTRL_TRIG_SHIFT)) & GINT_CTRL_TRIG_MASK)
/*! @} */

/*! @name PORT_POL - GPIO Grouped Interrupt polarity register. Configure the pin polarity of each PIO signal into the group interrupt function. If a bit is low then the corresponding PIO has an active low contribution into the group interrupt. If a bit is high then the corresponding PIO has an active high contribution. */
/*! @{ */
#define GINT_PORT_POL_POL_MASK                   (0x3FFFFFU)
#define GINT_PORT_POL_POL_SHIFT                  (0U)
/*! POL - Configure pin polarity of pin PIOn.
 */
#define GINT_PORT_POL_POL(x)                     (((uint32_t)(((uint32_t)(x)) << GINT_PORT_POL_POL_SHIFT)) & GINT_PORT_POL_POL_MASK)
/*! @} */

/* The count of GINT_PORT_POL */
#define GINT_PORT_POL_COUNT                      (1U)

/*! @name PORT_ENA - GPIO Grouped Interrupt port enable register. When a bit is set then the corresponding PIO is enabled for the group interrupt function. */
/*! @{ */
#define GINT_PORT_ENA_ENA_MASK                   (0x3FFFFFU)
#define GINT_PORT_ENA_ENA_SHIFT                  (0U)
/*! ENA - Enable pin PIOn for group interrupt.
 */
#define GINT_PORT_ENA_ENA(x)                     (((uint32_t)(((uint32_t)(x)) << GINT_PORT_ENA_ENA_SHIFT)) & GINT_PORT_ENA_ENA_MASK)
/*! @} */

/* The count of GINT_PORT_ENA */
#define GINT_PORT_ENA_COUNT                      (1U)


/*!
 * @}
 */ /* end of group GINT_Register_Masks */


/* GINT - Peripheral instance base addresses */
/** Peripheral GINT0 base address */
#define GINT0_BASE                               (0x40011000u)
/** Peripheral GINT0 base pointer */
#define GINT0                                    ((GINT_Type *)GINT0_BASE)
/** Array initializer of GINT peripheral base addresses */
#define GINT_BASE_ADDRS                          { GINT0_BASE }
/** Array initializer of GINT peripheral base pointers */
#define GINT_BASE_PTRS                           { GINT0 }
/** Interrupt vectors for the GINT peripheral type */
#define GINT_IRQS                                { GINT0_IRQn }

/*!
 * @}
 */ /* end of group GINT_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- GPIO Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup GPIO_Peripheral_Access_Layer GPIO Peripheral Access Layer
 * @{
 */

/** GPIO - Register Layout Typedef */
typedef struct {
  __IO uint8_t B[1][22];                           /**< Byte pin registers. Read 0: pin PIOn is LOW. Read 0xFF: pin PIOn is HIGH. Only 0 or 0xFF can be read. Write 0: clear output bit. Write any value 0x01 to 0xFF: set output bit. Reset values reflects the state of pin given by the relevant bit of PIN reset value, array offset: 0x0, array step: index*0x16, index2*0x1 */
       uint8_t RESERVED_0[4074];
  __IO uint32_t W[1][22];                          /**< Word pin registers Read 0: pin PIOn is LOW. Read 0xFFFFFFFF: pin PIOn is HIGH. Only 0 or 0xFFFF FFFF can be read. Write 0: clear output bit. Write any value 0x00000001 to 0xFFFFFFFF: set output bit. Reset values reflects the state of pin given by the relevant bit of PIN reset value, array offset: 0x1000, array step: index*0x58, index2*0x4 */
       uint8_t RESERVED_1[4008];
  __IO uint32_t DIR[1];                            /**< Direction register, array offset: 0x2000, array step: 0x4 */
       uint8_t RESERVED_2[124];
  __IO uint32_t MASK[1];                           /**< Mask register, array offset: 0x2080, array step: 0x4 */
       uint8_t RESERVED_3[124];
  __IO uint32_t PIN[1];                            /**< Pin register, array offset: 0x2100, array step: 0x4 */
       uint8_t RESERVED_4[124];
  __IO uint32_t MPIN[1];                           /**< Masked Pin register, array offset: 0x2180, array step: 0x4 */
       uint8_t RESERVED_5[124];
  __IO uint32_t SET[1];                            /**< Write: Set Pin register bits Read: output bits, array offset: 0x2200, array step: 0x4 */
       uint8_t RESERVED_6[124];
  __O  uint32_t CLR[1];                            /**< Clear Pin register bits, array offset: 0x2280, array step: 0x4 */
       uint8_t RESERVED_7[124];
  __O  uint32_t NOT[1];                            /**< Toggle Pin register bits, array offset: 0x2300, array step: 0x4 */
       uint8_t RESERVED_8[124];
  __O  uint32_t DIRSET[1];                         /**< Set pin direction bits, array offset: 0x2380, array step: 0x4 */
       uint8_t RESERVED_9[124];
  __O  uint32_t DIRCLR[1];                         /**< Clear pin direction bits, array offset: 0x2400, array step: 0x4 */
       uint8_t RESERVED_10[124];
  __O  uint32_t DIRNOT[1];                         /**< Toggle pin direction bits, array offset: 0x2480, array step: 0x4 */
} GPIO_Type;

/* ----------------------------------------------------------------------------
   -- GPIO Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup GPIO_Register_Masks GPIO Register Masks
 * @{
 */

/*! @name B - Byte pin registers. Read 0: pin PIOn is LOW. Read 0xFF: pin PIOn is HIGH. Only 0 or 0xFF can be read. Write 0: clear output bit. Write any value 0x01 to 0xFF: set output bit. Reset values reflects the state of pin given by the relevant bit of PIN reset value */
/*! @{ */
#define GPIO_B_B_MASK                            (0xFFU)
#define GPIO_B_B_SHIFT                           (0U)
/*! B - Byte pin registers. Read 0: pin PIOn is LOW. Read 0xFF: pin PIOn is HIGH. Only 0 or 0xFF can
 *    be read. Write 0: clear output bit. Write any value 0x01 to 0xFF: set output bit. Reset
 *    values reflects the state of pin given by the relevant bit of PIN reset value
 */
#define GPIO_B_B(x)                              (((uint8_t)(((uint8_t)(x)) << GPIO_B_B_SHIFT)) & GPIO_B_B_MASK)
/*! @} */

/* The count of GPIO_B */
#define GPIO_B_COUNT                             (1U)

/* The count of GPIO_B */
#define GPIO_B_COUNT2                            (22U)

/*! @name W - Word pin registers Read 0: pin PIOn is LOW. Read 0xFFFFFFFF: pin PIOn is HIGH. Only 0 or 0xFFFF FFFF can be read. Write 0: clear output bit. Write any value 0x00000001 to 0xFFFFFFFF: set output bit. Reset values reflects the state of pin given by the relevant bit of PIN reset value */
/*! @{ */
#define GPIO_W_W_MASK                            (0xFFFFFFFFU)
#define GPIO_W_W_SHIFT                           (0U)
/*! W - Word pin registers Read 0: pin PIOn is LOW. Read 0xFFFFFFFF: pin PIOn is HIGH. Only 0 or
 *    0xFFFF FFFF can be read. Write 0: clear output bit. Write any value 0x00000001 to 0xFFFFFFFF: set
 *    output bit. Reset values reflects the state of pin given by the relevant bit of PIN reset
 *    value
 */
#define GPIO_W_W(x)                              (((uint32_t)(((uint32_t)(x)) << GPIO_W_W_SHIFT)) & GPIO_W_W_MASK)
/*! @} */

/* The count of GPIO_W */
#define GPIO_W_COUNT                             (1U)

/* The count of GPIO_W */
#define GPIO_W_COUNT2                            (22U)

/*! @name DIR - Direction register */
/*! @{ */
#define GPIO_DIR_DIRP_PIO0_MASK                  (0x1U)
#define GPIO_DIR_DIRP_PIO0_SHIFT                 (0U)
/*! DIRP_PIO0 - Selects pin direction for pin PIO0 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO0(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO0_SHIFT)) & GPIO_DIR_DIRP_PIO0_MASK)
#define GPIO_DIR_DIRP_PIO1_MASK                  (0x2U)
#define GPIO_DIR_DIRP_PIO1_SHIFT                 (1U)
/*! DIRP_PIO1 - Selects pin direction for pin PIO1 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO1(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO1_SHIFT)) & GPIO_DIR_DIRP_PIO1_MASK)
#define GPIO_DIR_DIRP_PIO2_MASK                  (0x4U)
#define GPIO_DIR_DIRP_PIO2_SHIFT                 (2U)
/*! DIRP_PIO2 - Selects pin direction for pin PIO2 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO2(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO2_SHIFT)) & GPIO_DIR_DIRP_PIO2_MASK)
#define GPIO_DIR_DIRP_PIO3_MASK                  (0x8U)
#define GPIO_DIR_DIRP_PIO3_SHIFT                 (3U)
/*! DIRP_PIO3 - Selects pin direction for pin PIO3 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO3(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO3_SHIFT)) & GPIO_DIR_DIRP_PIO3_MASK)
#define GPIO_DIR_DIRP_PIO4_MASK                  (0x10U)
#define GPIO_DIR_DIRP_PIO4_SHIFT                 (4U)
/*! DIRP_PIO4 - Selects pin direction for pin PIO4 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO4(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO4_SHIFT)) & GPIO_DIR_DIRP_PIO4_MASK)
#define GPIO_DIR_DIRP_PIO5_MASK                  (0x20U)
#define GPIO_DIR_DIRP_PIO5_SHIFT                 (5U)
/*! DIRP_PIO5 - Selects pin direction for pin PIO5 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO5(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO5_SHIFT)) & GPIO_DIR_DIRP_PIO5_MASK)
#define GPIO_DIR_DIRP_PIO6_MASK                  (0x40U)
#define GPIO_DIR_DIRP_PIO6_SHIFT                 (6U)
/*! DIRP_PIO6 - Selects pin direction for pin PIO6 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO6(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO6_SHIFT)) & GPIO_DIR_DIRP_PIO6_MASK)
#define GPIO_DIR_DIRP_PIO7_MASK                  (0x80U)
#define GPIO_DIR_DIRP_PIO7_SHIFT                 (7U)
/*! DIRP_PIO7 - Selects pin direction for pin PIO7 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO7(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO7_SHIFT)) & GPIO_DIR_DIRP_PIO7_MASK)
#define GPIO_DIR_DIRP_PIO8_MASK                  (0x100U)
#define GPIO_DIR_DIRP_PIO8_SHIFT                 (8U)
/*! DIRP_PIO8 - Selects pin direction for pin PIO8 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO8(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO8_SHIFT)) & GPIO_DIR_DIRP_PIO8_MASK)
#define GPIO_DIR_DIRP_PIO9_MASK                  (0x200U)
#define GPIO_DIR_DIRP_PIO9_SHIFT                 (9U)
/*! DIRP_PIO9 - Selects pin direction for pin PIO9 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO9(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO9_SHIFT)) & GPIO_DIR_DIRP_PIO9_MASK)
#define GPIO_DIR_DIRP_PIO10_MASK                 (0x400U)
#define GPIO_DIR_DIRP_PIO10_SHIFT                (10U)
/*! DIRP_PIO10 - Selects pin direction for pin PIO10 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO10(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO10_SHIFT)) & GPIO_DIR_DIRP_PIO10_MASK)
#define GPIO_DIR_DIRP_PIO11_MASK                 (0x800U)
#define GPIO_DIR_DIRP_PIO11_SHIFT                (11U)
/*! DIRP_PIO11 - Selects pin direction for pin PIO11 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO11(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO11_SHIFT)) & GPIO_DIR_DIRP_PIO11_MASK)
#define GPIO_DIR_DIRP_PIO12_MASK                 (0x1000U)
#define GPIO_DIR_DIRP_PIO12_SHIFT                (12U)
/*! DIRP_PIO12 - Selects pin direction for pin PIO12 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO12(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO12_SHIFT)) & GPIO_DIR_DIRP_PIO12_MASK)
#define GPIO_DIR_DIRP_PIO13_MASK                 (0x2000U)
#define GPIO_DIR_DIRP_PIO13_SHIFT                (13U)
/*! DIRP_PIO13 - Selects pin direction for pin PIO13 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO13(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO13_SHIFT)) & GPIO_DIR_DIRP_PIO13_MASK)
#define GPIO_DIR_DIRP_PIO14_MASK                 (0x4000U)
#define GPIO_DIR_DIRP_PIO14_SHIFT                (14U)
/*! DIRP_PIO14 - Selects pin direction for pin PIO14 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO14(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO14_SHIFT)) & GPIO_DIR_DIRP_PIO14_MASK)
#define GPIO_DIR_DIRP_PIO15_MASK                 (0x8000U)
#define GPIO_DIR_DIRP_PIO15_SHIFT                (15U)
/*! DIRP_PIO15 - Selects pin direction for pin PIO15 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO15(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO15_SHIFT)) & GPIO_DIR_DIRP_PIO15_MASK)
#define GPIO_DIR_DIRP_PIO16_MASK                 (0x10000U)
#define GPIO_DIR_DIRP_PIO16_SHIFT                (16U)
/*! DIRP_PIO16 - Selects pin direction for pin PIO16 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO16(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO16_SHIFT)) & GPIO_DIR_DIRP_PIO16_MASK)
#define GPIO_DIR_DIRP_PIO17_MASK                 (0x20000U)
#define GPIO_DIR_DIRP_PIO17_SHIFT                (17U)
/*! DIRP_PIO17 - Selects pin direction for pin PIO17 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO17(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO17_SHIFT)) & GPIO_DIR_DIRP_PIO17_MASK)
#define GPIO_DIR_DIRP_PIO18_MASK                 (0x40000U)
#define GPIO_DIR_DIRP_PIO18_SHIFT                (18U)
/*! DIRP_PIO18 - Selects pin direction for pin PIO18 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO18(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO18_SHIFT)) & GPIO_DIR_DIRP_PIO18_MASK)
#define GPIO_DIR_DIRP_PIO19_MASK                 (0x80000U)
#define GPIO_DIR_DIRP_PIO19_SHIFT                (19U)
/*! DIRP_PIO19 - Selects pin direction for pin PIO19 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO19(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO19_SHIFT)) & GPIO_DIR_DIRP_PIO19_MASK)
#define GPIO_DIR_DIRP_PIO20_MASK                 (0x100000U)
#define GPIO_DIR_DIRP_PIO20_SHIFT                (20U)
/*! DIRP_PIO20 - Selects pin direction for pin PIO20 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO20(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO20_SHIFT)) & GPIO_DIR_DIRP_PIO20_MASK)
#define GPIO_DIR_DIRP_PIO21_MASK                 (0x200000U)
#define GPIO_DIR_DIRP_PIO21_SHIFT                (21U)
/*! DIRP_PIO21 - Selects pin direction for pin PIO21 . 0 = input. 1 = output.
 */
#define GPIO_DIR_DIRP_PIO21(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_DIR_DIRP_PIO21_SHIFT)) & GPIO_DIR_DIRP_PIO21_MASK)
/*! @} */

/* The count of GPIO_DIR */
#define GPIO_DIR_COUNT                           (1U)

/*! @name MASK - Mask register */
/*! @{ */
#define GPIO_MASK_MASKP_PIO0_MASK                (0x1U)
#define GPIO_MASK_MASKP_PIO0_SHIFT               (0U)
/*! MASKP_PIO0 - Controls if PIO0 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO0(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO0_SHIFT)) & GPIO_MASK_MASKP_PIO0_MASK)
#define GPIO_MASK_MASKP_PIO1_MASK                (0x2U)
#define GPIO_MASK_MASKP_PIO1_SHIFT               (1U)
/*! MASKP_PIO1 - Controls if PIO1 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO1(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO1_SHIFT)) & GPIO_MASK_MASKP_PIO1_MASK)
#define GPIO_MASK_MASKP_PIO2_MASK                (0x4U)
#define GPIO_MASK_MASKP_PIO2_SHIFT               (2U)
/*! MASKP_PIO2 - Controls if PIO2 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO2(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO2_SHIFT)) & GPIO_MASK_MASKP_PIO2_MASK)
#define GPIO_MASK_MASKP_PIO3_MASK                (0x8U)
#define GPIO_MASK_MASKP_PIO3_SHIFT               (3U)
/*! MASKP_PIO3 - Controls if PIO3 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO3(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO3_SHIFT)) & GPIO_MASK_MASKP_PIO3_MASK)
#define GPIO_MASK_MASKP_PIO4_MASK                (0x10U)
#define GPIO_MASK_MASKP_PIO4_SHIFT               (4U)
/*! MASKP_PIO4 - Controls if PIO4 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO4(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO4_SHIFT)) & GPIO_MASK_MASKP_PIO4_MASK)
#define GPIO_MASK_MASKP_PIO5_MASK                (0x20U)
#define GPIO_MASK_MASKP_PIO5_SHIFT               (5U)
/*! MASKP_PIO5 - Controls if PIO5 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO5(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO5_SHIFT)) & GPIO_MASK_MASKP_PIO5_MASK)
#define GPIO_MASK_MASKP_PIO6_MASK                (0x40U)
#define GPIO_MASK_MASKP_PIO6_SHIFT               (6U)
/*! MASKP_PIO6 - Controls if PIO6 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO6(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO6_SHIFT)) & GPIO_MASK_MASKP_PIO6_MASK)
#define GPIO_MASK_MASKP_PIO7_MASK                (0x80U)
#define GPIO_MASK_MASKP_PIO7_SHIFT               (7U)
/*! MASKP_PIO7 - Controls if PIO7 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO7(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO7_SHIFT)) & GPIO_MASK_MASKP_PIO7_MASK)
#define GPIO_MASK_MASKP_PIO8_MASK                (0x100U)
#define GPIO_MASK_MASKP_PIO8_SHIFT               (8U)
/*! MASKP_PIO8 - Controls if PIO8 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO8(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO8_SHIFT)) & GPIO_MASK_MASKP_PIO8_MASK)
#define GPIO_MASK_MASKP_PIO9_MASK                (0x200U)
#define GPIO_MASK_MASKP_PIO9_SHIFT               (9U)
/*! MASKP_PIO9 - Controls if PIO9 is active in MPIN register. 0 = Mask bit is clear, the PIO will be
 *    active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO9(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO9_SHIFT)) & GPIO_MASK_MASKP_PIO9_MASK)
#define GPIO_MASK_MASKP_PIO10_MASK               (0x400U)
#define GPIO_MASK_MASKP_PIO10_SHIFT              (10U)
/*! MASKP_PIO10 - Controls if PIO10 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO10(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO10_SHIFT)) & GPIO_MASK_MASKP_PIO10_MASK)
#define GPIO_MASK_MASKP_PIO11_MASK               (0x800U)
#define GPIO_MASK_MASKP_PIO11_SHIFT              (11U)
/*! MASKP_PIO11 - Controls if PIO11 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO11(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO11_SHIFT)) & GPIO_MASK_MASKP_PIO11_MASK)
#define GPIO_MASK_MASKP_PIO12_MASK               (0x1000U)
#define GPIO_MASK_MASKP_PIO12_SHIFT              (12U)
/*! MASKP_PIO12 - Controls if PIO12 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO12(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO12_SHIFT)) & GPIO_MASK_MASKP_PIO12_MASK)
#define GPIO_MASK_MASKP_PIO13_MASK               (0x2000U)
#define GPIO_MASK_MASKP_PIO13_SHIFT              (13U)
/*! MASKP_PIO13 - Controls if PIO13 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO13(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO13_SHIFT)) & GPIO_MASK_MASKP_PIO13_MASK)
#define GPIO_MASK_MASKP_PIO14_MASK               (0x4000U)
#define GPIO_MASK_MASKP_PIO14_SHIFT              (14U)
/*! MASKP_PIO14 - Controls if PIO14 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO14(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO14_SHIFT)) & GPIO_MASK_MASKP_PIO14_MASK)
#define GPIO_MASK_MASKP_PIO15_MASK               (0x8000U)
#define GPIO_MASK_MASKP_PIO15_SHIFT              (15U)
/*! MASKP_PIO15 - Controls if PIO150 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO15(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO15_SHIFT)) & GPIO_MASK_MASKP_PIO15_MASK)
#define GPIO_MASK_MASKP_PIO16_MASK               (0x10000U)
#define GPIO_MASK_MASKP_PIO16_SHIFT              (16U)
/*! MASKP_PIO16 - Controls if PIO16 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO16(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO16_SHIFT)) & GPIO_MASK_MASKP_PIO16_MASK)
#define GPIO_MASK_MASKP_PIO17_MASK               (0x20000U)
#define GPIO_MASK_MASKP_PIO17_SHIFT              (17U)
/*! MASKP_PIO17 - Controls if PIO17 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO17(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO17_SHIFT)) & GPIO_MASK_MASKP_PIO17_MASK)
#define GPIO_MASK_MASKP_PIO18_MASK               (0x40000U)
#define GPIO_MASK_MASKP_PIO18_SHIFT              (18U)
/*! MASKP_PIO18 - Controls if PIO18 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO18(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO18_SHIFT)) & GPIO_MASK_MASKP_PIO18_MASK)
#define GPIO_MASK_MASKP_PIO19_MASK               (0x80000U)
#define GPIO_MASK_MASKP_PIO19_SHIFT              (19U)
/*! MASKP_PIO19 - Controls if PIO19 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO19(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO19_SHIFT)) & GPIO_MASK_MASKP_PIO19_MASK)
#define GPIO_MASK_MASKP_PIO20_MASK               (0x100000U)
#define GPIO_MASK_MASKP_PIO20_SHIFT              (20U)
/*! MASKP_PIO20 - Controls if PIO20 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO20(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO20_SHIFT)) & GPIO_MASK_MASKP_PIO20_MASK)
#define GPIO_MASK_MASKP_PIO21_MASK               (0x200000U)
#define GPIO_MASK_MASKP_PIO21_SHIFT              (21U)
/*! MASKP_PIO21 - Controls if PIO21 is active in MPIN register. 0 = Mask bit is clear, the PIO will
 *    be active. 1 = Mask bit is set, the PIO will not be active.
 */
#define GPIO_MASK_MASKP_PIO21(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MASK_MASKP_PIO21_SHIFT)) & GPIO_MASK_MASKP_PIO21_MASK)
/*! @} */

/* The count of GPIO_MASK */
#define GPIO_MASK_COUNT                          (1U)

/*! @name PIN - Pin register */
/*! @{ */
#define GPIO_PIN_PORT_PIO0_MASK                  (0x1U)
#define GPIO_PIN_PORT_PIO0_SHIFT                 (0U)
/*! PORT_PIO0 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO0(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO0_SHIFT)) & GPIO_PIN_PORT_PIO0_MASK)
#define GPIO_PIN_PORT_PIO1_MASK                  (0x2U)
#define GPIO_PIN_PORT_PIO1_SHIFT                 (1U)
/*! PORT_PIO1 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO1(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO1_SHIFT)) & GPIO_PIN_PORT_PIO1_MASK)
#define GPIO_PIN_PORT_PIO2_MASK                  (0x4U)
#define GPIO_PIN_PORT_PIO2_SHIFT                 (2U)
/*! PORT_PIO2 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO2(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO2_SHIFT)) & GPIO_PIN_PORT_PIO2_MASK)
#define GPIO_PIN_PORT_PIO3_MASK                  (0x8U)
#define GPIO_PIN_PORT_PIO3_SHIFT                 (3U)
/*! PORT_PIO3 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO3(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO3_SHIFT)) & GPIO_PIN_PORT_PIO3_MASK)
#define GPIO_PIN_PORT_PIO4_MASK                  (0x10U)
#define GPIO_PIN_PORT_PIO4_SHIFT                 (4U)
/*! PORT_PIO4 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO4(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO4_SHIFT)) & GPIO_PIN_PORT_PIO4_MASK)
#define GPIO_PIN_PORT_PIO5_MASK                  (0x20U)
#define GPIO_PIN_PORT_PIO5_SHIFT                 (5U)
/*! PORT_PIO5 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO5(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO5_SHIFT)) & GPIO_PIN_PORT_PIO5_MASK)
#define GPIO_PIN_PORT_PIO6_MASK                  (0x40U)
#define GPIO_PIN_PORT_PIO6_SHIFT                 (6U)
/*! PORT_PIO6 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO6(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO6_SHIFT)) & GPIO_PIN_PORT_PIO6_MASK)
#define GPIO_PIN_PORT_PIO7_MASK                  (0x80U)
#define GPIO_PIN_PORT_PIO7_SHIFT                 (7U)
/*! PORT_PIO7 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO7(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO7_SHIFT)) & GPIO_PIN_PORT_PIO7_MASK)
#define GPIO_PIN_PORT_PIO8_MASK                  (0x100U)
#define GPIO_PIN_PORT_PIO8_SHIFT                 (8U)
/*! PORT_PIO8 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO8(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO8_SHIFT)) & GPIO_PIN_PORT_PIO8_MASK)
#define GPIO_PIN_PORT_PIO9_MASK                  (0x200U)
#define GPIO_PIN_PORT_PIO9_SHIFT                 (9U)
/*! PORT_PIO9 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO9(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO9_SHIFT)) & GPIO_PIN_PORT_PIO9_MASK)
#define GPIO_PIN_PORT_PIO10_MASK                 (0x400U)
#define GPIO_PIN_PORT_PIO10_SHIFT                (10U)
/*! PORT_PIO10 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO10(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO10_SHIFT)) & GPIO_PIN_PORT_PIO10_MASK)
#define GPIO_PIN_PORT_PIO11_MASK                 (0x800U)
#define GPIO_PIN_PORT_PIO11_SHIFT                (11U)
/*! PORT_PIO11 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO11(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO11_SHIFT)) & GPIO_PIN_PORT_PIO11_MASK)
#define GPIO_PIN_PORT_PIO12_MASK                 (0x1000U)
#define GPIO_PIN_PORT_PIO12_SHIFT                (12U)
/*! PORT_PIO12 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO12(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO12_SHIFT)) & GPIO_PIN_PORT_PIO12_MASK)
#define GPIO_PIN_PORT_PIO13_MASK                 (0x2000U)
#define GPIO_PIN_PORT_PIO13_SHIFT                (13U)
/*! PORT_PIO13 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO13(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO13_SHIFT)) & GPIO_PIN_PORT_PIO13_MASK)
#define GPIO_PIN_PORT_PIO14_MASK                 (0x4000U)
#define GPIO_PIN_PORT_PIO14_SHIFT                (14U)
/*! PORT_PIO14 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO14(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO14_SHIFT)) & GPIO_PIN_PORT_PIO14_MASK)
#define GPIO_PIN_PORT_PIO15_MASK                 (0x8000U)
#define GPIO_PIN_PORT_PIO15_SHIFT                (15U)
/*! PORT_PIO15 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO15(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO15_SHIFT)) & GPIO_PIN_PORT_PIO15_MASK)
#define GPIO_PIN_PORT_PIO16_MASK                 (0x10000U)
#define GPIO_PIN_PORT_PIO16_SHIFT                (16U)
/*! PORT_PIO16 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO16(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO16_SHIFT)) & GPIO_PIN_PORT_PIO16_MASK)
#define GPIO_PIN_PORT_PIO17_MASK                 (0x20000U)
#define GPIO_PIN_PORT_PIO17_SHIFT                (17U)
/*! PORT_PIO17 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO17(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO17_SHIFT)) & GPIO_PIN_PORT_PIO17_MASK)
#define GPIO_PIN_PORT_PIO18_MASK                 (0x40000U)
#define GPIO_PIN_PORT_PIO18_SHIFT                (18U)
/*! PORT_PIO18 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO18(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO18_SHIFT)) & GPIO_PIN_PORT_PIO18_MASK)
#define GPIO_PIN_PORT_PIO19_MASK                 (0x80000U)
#define GPIO_PIN_PORT_PIO19_SHIFT                (19U)
/*! PORT_PIO19 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO19(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO19_SHIFT)) & GPIO_PIN_PORT_PIO19_MASK)
#define GPIO_PIN_PORT_PIO20_MASK                 (0x100000U)
#define GPIO_PIN_PORT_PIO20_SHIFT                (20U)
/*! PORT_PIO20 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO20(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO20_SHIFT)) & GPIO_PIN_PORT_PIO20_MASK)
#define GPIO_PIN_PORT_PIO21_MASK                 (0x200000U)
#define GPIO_PIN_PORT_PIO21_SHIFT                (21U)
/*! PORT_PIO21 - Reads pin states or loads output bits. 0 = Read: pin is low; write: clear output
 *    bit. 1 = Read: pin is high; write: set output bit.
 */
#define GPIO_PIN_PORT_PIO21(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_PIN_PORT_PIO21_SHIFT)) & GPIO_PIN_PORT_PIO21_MASK)
/*! @} */

/* The count of GPIO_PIN */
#define GPIO_PIN_COUNT                           (1U)

/*! @name MPIN - Masked Pin register */
/*! @{ */
#define GPIO_MPIN_MPORT_PIO0_MASK                (0x1U)
#define GPIO_MPIN_MPORT_PIO0_SHIFT               (0U)
/*! MPORT_PIO0 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO0(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO0_SHIFT)) & GPIO_MPIN_MPORT_PIO0_MASK)
#define GPIO_MPIN_MPORT_PIO1_MASK                (0x2U)
#define GPIO_MPIN_MPORT_PIO1_SHIFT               (1U)
/*! MPORT_PIO1 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO1(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO1_SHIFT)) & GPIO_MPIN_MPORT_PIO1_MASK)
#define GPIO_MPIN_MPORT_PIO2_MASK                (0x4U)
#define GPIO_MPIN_MPORT_PIO2_SHIFT               (2U)
/*! MPORT_PIO2 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO2(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO2_SHIFT)) & GPIO_MPIN_MPORT_PIO2_MASK)
#define GPIO_MPIN_MPORT_PIO3_MASK                (0x8U)
#define GPIO_MPIN_MPORT_PIO3_SHIFT               (3U)
/*! MPORT_PIO3 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO3(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO3_SHIFT)) & GPIO_MPIN_MPORT_PIO3_MASK)
#define GPIO_MPIN_MPORT_PIO4_MASK                (0x10U)
#define GPIO_MPIN_MPORT_PIO4_SHIFT               (4U)
/*! MPORT_PIO4 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO4(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO4_SHIFT)) & GPIO_MPIN_MPORT_PIO4_MASK)
#define GPIO_MPIN_MPORT_PIO5_MASK                (0x20U)
#define GPIO_MPIN_MPORT_PIO5_SHIFT               (5U)
/*! MPORT_PIO5 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO5(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO5_SHIFT)) & GPIO_MPIN_MPORT_PIO5_MASK)
#define GPIO_MPIN_MPORT_PIO6_MASK                (0x40U)
#define GPIO_MPIN_MPORT_PIO6_SHIFT               (6U)
/*! MPORT_PIO6 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO6(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO6_SHIFT)) & GPIO_MPIN_MPORT_PIO6_MASK)
#define GPIO_MPIN_MPORT_PIO7_MASK                (0x80U)
#define GPIO_MPIN_MPORT_PIO7_SHIFT               (7U)
/*! MPORT_PIO7 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO7(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO7_SHIFT)) & GPIO_MPIN_MPORT_PIO7_MASK)
#define GPIO_MPIN_MPORT_PIO8_MASK                (0x100U)
#define GPIO_MPIN_MPORT_PIO8_SHIFT               (8U)
/*! MPORT_PIO8 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO8(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO8_SHIFT)) & GPIO_MPIN_MPORT_PIO8_MASK)
#define GPIO_MPIN_MPORT_PIO9_MASK                (0x200U)
#define GPIO_MPIN_MPORT_PIO9_SHIFT               (9U)
/*! MPORT_PIO9 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1 =
 *    Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO9(x)                  (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO9_SHIFT)) & GPIO_MPIN_MPORT_PIO9_MASK)
#define GPIO_MPIN_MPORT_PIO10_MASK               (0x400U)
#define GPIO_MPIN_MPORT_PIO10_SHIFT              (10U)
/*! MPORT_PIO10 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO10(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO10_SHIFT)) & GPIO_MPIN_MPORT_PIO10_MASK)
#define GPIO_MPIN_MPORT_PIO11_MASK               (0x800U)
#define GPIO_MPIN_MPORT_PIO11_SHIFT              (11U)
/*! MPORT_PIO11 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO11(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO11_SHIFT)) & GPIO_MPIN_MPORT_PIO11_MASK)
#define GPIO_MPIN_MPORT_PIO12_MASK               (0x1000U)
#define GPIO_MPIN_MPORT_PIO12_SHIFT              (12U)
/*! MPORT_PIO12 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO12(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO12_SHIFT)) & GPIO_MPIN_MPORT_PIO12_MASK)
#define GPIO_MPIN_MPORT_PIO13_MASK               (0x2000U)
#define GPIO_MPIN_MPORT_PIO13_SHIFT              (13U)
/*! MPORT_PIO13 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO13(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO13_SHIFT)) & GPIO_MPIN_MPORT_PIO13_MASK)
#define GPIO_MPIN_MPORT_PIO14_MASK               (0x4000U)
#define GPIO_MPIN_MPORT_PIO14_SHIFT              (14U)
/*! MPORT_PIO14 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO14(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO14_SHIFT)) & GPIO_MPIN_MPORT_PIO14_MASK)
#define GPIO_MPIN_MPORT_PIO15_MASK               (0x8000U)
#define GPIO_MPIN_MPORT_PIO15_SHIFT              (15U)
/*! MPORT_PIO15 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO15(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO15_SHIFT)) & GPIO_MPIN_MPORT_PIO15_MASK)
#define GPIO_MPIN_MPORT_PIO16_MASK               (0x10000U)
#define GPIO_MPIN_MPORT_PIO16_SHIFT              (16U)
/*! MPORT_PIO16 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO16(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO16_SHIFT)) & GPIO_MPIN_MPORT_PIO16_MASK)
#define GPIO_MPIN_MPORT_PIO17_MASK               (0x20000U)
#define GPIO_MPIN_MPORT_PIO17_SHIFT              (17U)
/*! MPORT_PIO17 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO17(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO17_SHIFT)) & GPIO_MPIN_MPORT_PIO17_MASK)
#define GPIO_MPIN_MPORT_PIO18_MASK               (0x40000U)
#define GPIO_MPIN_MPORT_PIO18_SHIFT              (18U)
/*! MPORT_PIO18 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO18(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO18_SHIFT)) & GPIO_MPIN_MPORT_PIO18_MASK)
#define GPIO_MPIN_MPORT_PIO19_MASK               (0x80000U)
#define GPIO_MPIN_MPORT_PIO19_SHIFT              (19U)
/*! MPORT_PIO19 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO19(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO19_SHIFT)) & GPIO_MPIN_MPORT_PIO19_MASK)
#define GPIO_MPIN_MPORT_PIO20_MASK               (0x100000U)
#define GPIO_MPIN_MPORT_PIO20_SHIFT              (20U)
/*! MPORT_PIO20 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO20(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO20_SHIFT)) & GPIO_MPIN_MPORT_PIO20_MASK)
#define GPIO_MPIN_MPORT_PIO21_MASK               (0x200000U)
#define GPIO_MPIN_MPORT_PIO21_SHIFT              (21U)
/*! MPORT_PIO21 - Masked pin register. 0 = Read: pin is LOW and/or the corresponding bit in the MASK
 *    register is 1; write: clear output bit if the corresponding bit in the MASK register is 0. 1
 *    = Read: pin is HIGH and the corresponding bit in the MASK register is 0; write: set output bit
 *    if the corresponding bit in the MASK register is 0.
 */
#define GPIO_MPIN_MPORT_PIO21(x)                 (((uint32_t)(((uint32_t)(x)) << GPIO_MPIN_MPORT_PIO21_SHIFT)) & GPIO_MPIN_MPORT_PIO21_MASK)
/*! @} */

/* The count of GPIO_MPIN */
#define GPIO_MPIN_COUNT                          (1U)

/*! @name SET - Write: Set Pin register bits Read: output bits */
/*! @{ */
#define GPIO_SET_SETP_PIO0_MASK                  (0x1U)
#define GPIO_SET_SETP_PIO0_SHIFT                 (0U)
/*! SETP_PIO0 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO0(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO0_SHIFT)) & GPIO_SET_SETP_PIO0_MASK)
#define GPIO_SET_SETP_PIO1_MASK                  (0x2U)
#define GPIO_SET_SETP_PIO1_SHIFT                 (1U)
/*! SETP_PIO1 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO1(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO1_SHIFT)) & GPIO_SET_SETP_PIO1_MASK)
#define GPIO_SET_SETP_PIO2_MASK                  (0x4U)
#define GPIO_SET_SETP_PIO2_SHIFT                 (2U)
/*! SETP_PIO2 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO2(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO2_SHIFT)) & GPIO_SET_SETP_PIO2_MASK)
#define GPIO_SET_SETP_PIO3_MASK                  (0x8U)
#define GPIO_SET_SETP_PIO3_SHIFT                 (3U)
/*! SETP_PIO3 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO3(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO3_SHIFT)) & GPIO_SET_SETP_PIO3_MASK)
#define GPIO_SET_SETP_PIO4_MASK                  (0x10U)
#define GPIO_SET_SETP_PIO4_SHIFT                 (4U)
/*! SETP_PIO4 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO4(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO4_SHIFT)) & GPIO_SET_SETP_PIO4_MASK)
#define GPIO_SET_SETP_PIO5_MASK                  (0x20U)
#define GPIO_SET_SETP_PIO5_SHIFT                 (5U)
/*! SETP_PIO5 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO5(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO5_SHIFT)) & GPIO_SET_SETP_PIO5_MASK)
#define GPIO_SET_SETP_PIO6_MASK                  (0x40U)
#define GPIO_SET_SETP_PIO6_SHIFT                 (6U)
/*! SETP_PIO6 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO6(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO6_SHIFT)) & GPIO_SET_SETP_PIO6_MASK)
#define GPIO_SET_SETP_PIO7_MASK                  (0x80U)
#define GPIO_SET_SETP_PIO7_SHIFT                 (7U)
/*! SETP_PIO7 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO7(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO7_SHIFT)) & GPIO_SET_SETP_PIO7_MASK)
#define GPIO_SET_SETP_PIO8_MASK                  (0x100U)
#define GPIO_SET_SETP_PIO8_SHIFT                 (8U)
/*! SETP_PIO8 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO8(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO8_SHIFT)) & GPIO_SET_SETP_PIO8_MASK)
#define GPIO_SET_SETP_PIO9_MASK                  (0x200U)
#define GPIO_SET_SETP_PIO9_SHIFT                 (9U)
/*! SETP_PIO9 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO9(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO9_SHIFT)) & GPIO_SET_SETP_PIO9_MASK)
#define GPIO_SET_SETP_PIO10_MASK                 (0x400U)
#define GPIO_SET_SETP_PIO10_SHIFT                (10U)
/*! SETP_PIO10 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO10(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO10_SHIFT)) & GPIO_SET_SETP_PIO10_MASK)
#define GPIO_SET_SETP_PIO11_MASK                 (0x800U)
#define GPIO_SET_SETP_PIO11_SHIFT                (11U)
/*! SETP_PIO11 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO11(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO11_SHIFT)) & GPIO_SET_SETP_PIO11_MASK)
#define GPIO_SET_SETP_PIO12_MASK                 (0x1000U)
#define GPIO_SET_SETP_PIO12_SHIFT                (12U)
/*! SETP_PIO12 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO12(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO12_SHIFT)) & GPIO_SET_SETP_PIO12_MASK)
#define GPIO_SET_SETP_PIO13_MASK                 (0x2000U)
#define GPIO_SET_SETP_PIO13_SHIFT                (13U)
/*! SETP_PIO13 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO13(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO13_SHIFT)) & GPIO_SET_SETP_PIO13_MASK)
#define GPIO_SET_SETP_PIO14_MASK                 (0x4000U)
#define GPIO_SET_SETP_PIO14_SHIFT                (14U)
/*! SETP_PIO14 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO14(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO14_SHIFT)) & GPIO_SET_SETP_PIO14_MASK)
#define GPIO_SET_SETP_PIO15_MASK                 (0x8000U)
#define GPIO_SET_SETP_PIO15_SHIFT                (15U)
/*! SETP_PIO15 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO15(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO15_SHIFT)) & GPIO_SET_SETP_PIO15_MASK)
#define GPIO_SET_SETP_PIO16_MASK                 (0x10000U)
#define GPIO_SET_SETP_PIO16_SHIFT                (16U)
/*! SETP_PIO16 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO16(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO16_SHIFT)) & GPIO_SET_SETP_PIO16_MASK)
#define GPIO_SET_SETP_PIO17_MASK                 (0x20000U)
#define GPIO_SET_SETP_PIO17_SHIFT                (17U)
/*! SETP_PIO17 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO17(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO17_SHIFT)) & GPIO_SET_SETP_PIO17_MASK)
#define GPIO_SET_SETP_PIO18_MASK                 (0x40000U)
#define GPIO_SET_SETP_PIO18_SHIFT                (18U)
/*! SETP_PIO18 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO18(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO18_SHIFT)) & GPIO_SET_SETP_PIO18_MASK)
#define GPIO_SET_SETP_PIO19_MASK                 (0x80000U)
#define GPIO_SET_SETP_PIO19_SHIFT                (19U)
/*! SETP_PIO19 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO19(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO19_SHIFT)) & GPIO_SET_SETP_PIO19_MASK)
#define GPIO_SET_SETP_PIO20_MASK                 (0x100000U)
#define GPIO_SET_SETP_PIO20_SHIFT                (20U)
/*! SETP_PIO20 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO20(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO20_SHIFT)) & GPIO_SET_SETP_PIO20_MASK)
#define GPIO_SET_SETP_PIO21_MASK                 (0x200000U)
#define GPIO_SET_SETP_PIO21_SHIFT                (21U)
/*! SETP_PIO21 - Read or set output bits. 0 = Read: output bit: write: no operation. 1 = Read: output bit; write: set output bit.
 */
#define GPIO_SET_SETP_PIO21(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_SET_SETP_PIO21_SHIFT)) & GPIO_SET_SETP_PIO21_MASK)
/*! @} */

/* The count of GPIO_SET */
#define GPIO_SET_COUNT                           (1U)

/*! @name CLR - Clear Pin register bits */
/*! @{ */
#define GPIO_CLR_CLRP_PIO0_MASK                  (0x1U)
#define GPIO_CLR_CLRP_PIO0_SHIFT                 (0U)
/*! CLRP_PIO0 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO0(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO0_SHIFT)) & GPIO_CLR_CLRP_PIO0_MASK)
#define GPIO_CLR_CLRP_PIO1_MASK                  (0x2U)
#define GPIO_CLR_CLRP_PIO1_SHIFT                 (1U)
/*! CLRP_PIO1 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO1(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO1_SHIFT)) & GPIO_CLR_CLRP_PIO1_MASK)
#define GPIO_CLR_CLRP_PIO2_MASK                  (0x4U)
#define GPIO_CLR_CLRP_PIO2_SHIFT                 (2U)
/*! CLRP_PIO2 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO2(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO2_SHIFT)) & GPIO_CLR_CLRP_PIO2_MASK)
#define GPIO_CLR_CLRP_PIO3_MASK                  (0x8U)
#define GPIO_CLR_CLRP_PIO3_SHIFT                 (3U)
/*! CLRP_PIO3 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO3(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO3_SHIFT)) & GPIO_CLR_CLRP_PIO3_MASK)
#define GPIO_CLR_CLRP_PIO4_MASK                  (0x10U)
#define GPIO_CLR_CLRP_PIO4_SHIFT                 (4U)
/*! CLRP_PIO4 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO4(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO4_SHIFT)) & GPIO_CLR_CLRP_PIO4_MASK)
#define GPIO_CLR_CLRP_PIO5_MASK                  (0x20U)
#define GPIO_CLR_CLRP_PIO5_SHIFT                 (5U)
/*! CLRP_PIO5 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO5(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO5_SHIFT)) & GPIO_CLR_CLRP_PIO5_MASK)
#define GPIO_CLR_CLRP_PIO6_MASK                  (0x40U)
#define GPIO_CLR_CLRP_PIO6_SHIFT                 (6U)
/*! CLRP_PIO6 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO6(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO6_SHIFT)) & GPIO_CLR_CLRP_PIO6_MASK)
#define GPIO_CLR_CLRP_PIO7_MASK                  (0x80U)
#define GPIO_CLR_CLRP_PIO7_SHIFT                 (7U)
/*! CLRP_PIO7 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO7(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO7_SHIFT)) & GPIO_CLR_CLRP_PIO7_MASK)
#define GPIO_CLR_CLRP_PIO8_MASK                  (0x100U)
#define GPIO_CLR_CLRP_PIO8_SHIFT                 (8U)
/*! CLRP_PIO8 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO8(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO8_SHIFT)) & GPIO_CLR_CLRP_PIO8_MASK)
#define GPIO_CLR_CLRP_PIO9_MASK                  (0x200U)
#define GPIO_CLR_CLRP_PIO9_SHIFT                 (9U)
/*! CLRP_PIO9 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO9(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO9_SHIFT)) & GPIO_CLR_CLRP_PIO9_MASK)
#define GPIO_CLR_CLRP_PIO10_MASK                 (0x400U)
#define GPIO_CLR_CLRP_PIO10_SHIFT                (10U)
/*! CLRP_PIO10 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO10(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO10_SHIFT)) & GPIO_CLR_CLRP_PIO10_MASK)
#define GPIO_CLR_CLRP_PIO11_MASK                 (0x800U)
#define GPIO_CLR_CLRP_PIO11_SHIFT                (11U)
/*! CLRP_PIO11 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO11(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO11_SHIFT)) & GPIO_CLR_CLRP_PIO11_MASK)
#define GPIO_CLR_CLRP_PIO12_MASK                 (0x1000U)
#define GPIO_CLR_CLRP_PIO12_SHIFT                (12U)
/*! CLRP_PIO12 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO12(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO12_SHIFT)) & GPIO_CLR_CLRP_PIO12_MASK)
#define GPIO_CLR_CLRP_PIO13_MASK                 (0x2000U)
#define GPIO_CLR_CLRP_PIO13_SHIFT                (13U)
/*! CLRP_PIO13 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO13(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO13_SHIFT)) & GPIO_CLR_CLRP_PIO13_MASK)
#define GPIO_CLR_CLRP_PIO14_MASK                 (0x4000U)
#define GPIO_CLR_CLRP_PIO14_SHIFT                (14U)
/*! CLRP_PIO14 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO14(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO14_SHIFT)) & GPIO_CLR_CLRP_PIO14_MASK)
#define GPIO_CLR_CLRP_PIO15_MASK                 (0x8000U)
#define GPIO_CLR_CLRP_PIO15_SHIFT                (15U)
/*! CLRP_PIO15 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO15(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO15_SHIFT)) & GPIO_CLR_CLRP_PIO15_MASK)
#define GPIO_CLR_CLRP_PIO16_MASK                 (0x10000U)
#define GPIO_CLR_CLRP_PIO16_SHIFT                (16U)
/*! CLRP_PIO16 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO16(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO16_SHIFT)) & GPIO_CLR_CLRP_PIO16_MASK)
#define GPIO_CLR_CLRP_PIO17_MASK                 (0x20000U)
#define GPIO_CLR_CLRP_PIO17_SHIFT                (17U)
/*! CLRP_PIO17 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO17(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO17_SHIFT)) & GPIO_CLR_CLRP_PIO17_MASK)
#define GPIO_CLR_CLRP_PIO18_MASK                 (0x40000U)
#define GPIO_CLR_CLRP_PIO18_SHIFT                (18U)
/*! CLRP_PIO18 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO18(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO18_SHIFT)) & GPIO_CLR_CLRP_PIO18_MASK)
#define GPIO_CLR_CLRP_PIO19_MASK                 (0x80000U)
#define GPIO_CLR_CLRP_PIO19_SHIFT                (19U)
/*! CLRP_PIO19 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO19(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO19_SHIFT)) & GPIO_CLR_CLRP_PIO19_MASK)
#define GPIO_CLR_CLRP_PIO20_MASK                 (0x100000U)
#define GPIO_CLR_CLRP_PIO20_SHIFT                (20U)
/*! CLRP_PIO20 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO20(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO20_SHIFT)) & GPIO_CLR_CLRP_PIO20_MASK)
#define GPIO_CLR_CLRP_PIO21_MASK                 (0x200000U)
#define GPIO_CLR_CLRP_PIO21_SHIFT                (21U)
/*! CLRP_PIO21 - Clear output bits. 0 = No operation. 1 = Clear output bit.
 */
#define GPIO_CLR_CLRP_PIO21(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_CLR_CLRP_PIO21_SHIFT)) & GPIO_CLR_CLRP_PIO21_MASK)
/*! @} */

/* The count of GPIO_CLR */
#define GPIO_CLR_COUNT                           (1U)

/*! @name NOT - Toggle Pin register bits */
/*! @{ */
#define GPIO_NOT_NOTP_PIO0_MASK                  (0x1U)
#define GPIO_NOT_NOTP_PIO0_SHIFT                 (0U)
/*! NOTP_PIO0 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO0(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO0_SHIFT)) & GPIO_NOT_NOTP_PIO0_MASK)
#define GPIO_NOT_NOTP_PIO1_MASK                  (0x2U)
#define GPIO_NOT_NOTP_PIO1_SHIFT                 (1U)
/*! NOTP_PIO1 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO1(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO1_SHIFT)) & GPIO_NOT_NOTP_PIO1_MASK)
#define GPIO_NOT_NOTP_PIO2_MASK                  (0x4U)
#define GPIO_NOT_NOTP_PIO2_SHIFT                 (2U)
/*! NOTP_PIO2 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO2(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO2_SHIFT)) & GPIO_NOT_NOTP_PIO2_MASK)
#define GPIO_NOT_NOTP_PIO3_MASK                  (0x8U)
#define GPIO_NOT_NOTP_PIO3_SHIFT                 (3U)
/*! NOTP_PIO3 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO3(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO3_SHIFT)) & GPIO_NOT_NOTP_PIO3_MASK)
#define GPIO_NOT_NOTP_PIO4_MASK                  (0x10U)
#define GPIO_NOT_NOTP_PIO4_SHIFT                 (4U)
/*! NOTP_PIO4 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO4(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO4_SHIFT)) & GPIO_NOT_NOTP_PIO4_MASK)
#define GPIO_NOT_NOTP_PIO5_MASK                  (0x20U)
#define GPIO_NOT_NOTP_PIO5_SHIFT                 (5U)
/*! NOTP_PIO5 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO5(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO5_SHIFT)) & GPIO_NOT_NOTP_PIO5_MASK)
#define GPIO_NOT_NOTP_PIO6_MASK                  (0x40U)
#define GPIO_NOT_NOTP_PIO6_SHIFT                 (6U)
/*! NOTP_PIO6 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO6(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO6_SHIFT)) & GPIO_NOT_NOTP_PIO6_MASK)
#define GPIO_NOT_NOTP_PIO7_MASK                  (0x80U)
#define GPIO_NOT_NOTP_PIO7_SHIFT                 (7U)
/*! NOTP_PIO7 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO7(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO7_SHIFT)) & GPIO_NOT_NOTP_PIO7_MASK)
#define GPIO_NOT_NOTP_PIO8_MASK                  (0x100U)
#define GPIO_NOT_NOTP_PIO8_SHIFT                 (8U)
/*! NOTP_PIO8 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO8(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO8_SHIFT)) & GPIO_NOT_NOTP_PIO8_MASK)
#define GPIO_NOT_NOTP_PIO9_MASK                  (0x200U)
#define GPIO_NOT_NOTP_PIO9_SHIFT                 (9U)
/*! NOTP_PIO9 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO9(x)                    (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO9_SHIFT)) & GPIO_NOT_NOTP_PIO9_MASK)
#define GPIO_NOT_NOTP_PIO10_MASK                 (0x400U)
#define GPIO_NOT_NOTP_PIO10_SHIFT                (10U)
/*! NOTP_PIO10 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO10(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO10_SHIFT)) & GPIO_NOT_NOTP_PIO10_MASK)
#define GPIO_NOT_NOTP_PIO11_MASK                 (0x800U)
#define GPIO_NOT_NOTP_PIO11_SHIFT                (11U)
/*! NOTP_PIO11 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO11(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO11_SHIFT)) & GPIO_NOT_NOTP_PIO11_MASK)
#define GPIO_NOT_NOTP_PIO12_MASK                 (0x1000U)
#define GPIO_NOT_NOTP_PIO12_SHIFT                (12U)
/*! NOTP_PIO12 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO12(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO12_SHIFT)) & GPIO_NOT_NOTP_PIO12_MASK)
#define GPIO_NOT_NOTP_PIO13_MASK                 (0x2000U)
#define GPIO_NOT_NOTP_PIO13_SHIFT                (13U)
/*! NOTP_PIO13 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO13(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO13_SHIFT)) & GPIO_NOT_NOTP_PIO13_MASK)
#define GPIO_NOT_NOTP_PIO14_MASK                 (0x4000U)
#define GPIO_NOT_NOTP_PIO14_SHIFT                (14U)
/*! NOTP_PIO14 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO14(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO14_SHIFT)) & GPIO_NOT_NOTP_PIO14_MASK)
#define GPIO_NOT_NOTP_PIO15_MASK                 (0x8000U)
#define GPIO_NOT_NOTP_PIO15_SHIFT                (15U)
/*! NOTP_PIO15 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO15(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO15_SHIFT)) & GPIO_NOT_NOTP_PIO15_MASK)
#define GPIO_NOT_NOTP_PIO16_MASK                 (0x10000U)
#define GPIO_NOT_NOTP_PIO16_SHIFT                (16U)
/*! NOTP_PIO16 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO16(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO16_SHIFT)) & GPIO_NOT_NOTP_PIO16_MASK)
#define GPIO_NOT_NOTP_PIO17_MASK                 (0x20000U)
#define GPIO_NOT_NOTP_PIO17_SHIFT                (17U)
/*! NOTP_PIO17 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO17(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO17_SHIFT)) & GPIO_NOT_NOTP_PIO17_MASK)
#define GPIO_NOT_NOTP_PIO18_MASK                 (0x40000U)
#define GPIO_NOT_NOTP_PIO18_SHIFT                (18U)
/*! NOTP_PIO18 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO18(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO18_SHIFT)) & GPIO_NOT_NOTP_PIO18_MASK)
#define GPIO_NOT_NOTP_PIO19_MASK                 (0x80000U)
#define GPIO_NOT_NOTP_PIO19_SHIFT                (19U)
/*! NOTP_PIO19 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO19(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO19_SHIFT)) & GPIO_NOT_NOTP_PIO19_MASK)
#define GPIO_NOT_NOTP_PIO20_MASK                 (0x100000U)
#define GPIO_NOT_NOTP_PIO20_SHIFT                (20U)
/*! NOTP_PIO20 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO20(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO20_SHIFT)) & GPIO_NOT_NOTP_PIO20_MASK)
#define GPIO_NOT_NOTP_PIO21_MASK                 (0x200000U)
#define GPIO_NOT_NOTP_PIO21_SHIFT                (21U)
/*! NOTP_PIO21 - Toggle output bits. 0 = no operation. 1 = Toggle output bit.
 */
#define GPIO_NOT_NOTP_PIO21(x)                   (((uint32_t)(((uint32_t)(x)) << GPIO_NOT_NOTP_PIO21_SHIFT)) & GPIO_NOT_NOTP_PIO21_MASK)
/*! @} */

/* The count of GPIO_NOT */
#define GPIO_NOT_COUNT                           (1U)

/*! @name DIRSET - Set pin direction bits */
/*! @{ */
#define GPIO_DIRSET_DIRSETP_PIO0_MASK            (0x1U)
#define GPIO_DIRSET_DIRSETP_PIO0_SHIFT           (0U)
/*! DIRSETP_PIO0 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO0(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO0_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO0_MASK)
#define GPIO_DIRSET_DIRSETP_PIO1_MASK            (0x2U)
#define GPIO_DIRSET_DIRSETP_PIO1_SHIFT           (1U)
/*! DIRSETP_PIO1 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO1(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO1_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO1_MASK)
#define GPIO_DIRSET_DIRSETP_PIO2_MASK            (0x4U)
#define GPIO_DIRSET_DIRSETP_PIO2_SHIFT           (2U)
/*! DIRSETP_PIO2 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO2(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO2_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO2_MASK)
#define GPIO_DIRSET_DIRSETP_PIO3_MASK            (0x8U)
#define GPIO_DIRSET_DIRSETP_PIO3_SHIFT           (3U)
/*! DIRSETP_PIO3 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO3(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO3_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO3_MASK)
#define GPIO_DIRSET_DIRSETP_PIO4_MASK            (0x10U)
#define GPIO_DIRSET_DIRSETP_PIO4_SHIFT           (4U)
/*! DIRSETP_PIO4 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO4(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO4_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO4_MASK)
#define GPIO_DIRSET_DIRSETP_PIO5_MASK            (0x20U)
#define GPIO_DIRSET_DIRSETP_PIO5_SHIFT           (5U)
/*! DIRSETP_PIO5 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO5(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO5_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO5_MASK)
#define GPIO_DIRSET_DIRSETP_PIO6_MASK            (0x40U)
#define GPIO_DIRSET_DIRSETP_PIO6_SHIFT           (6U)
/*! DIRSETP_PIO6 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO6(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO6_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO6_MASK)
#define GPIO_DIRSET_DIRSETP_PIO7_MASK            (0x80U)
#define GPIO_DIRSET_DIRSETP_PIO7_SHIFT           (7U)
/*! DIRSETP_PIO7 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO7(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO7_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO7_MASK)
#define GPIO_DIRSET_DIRSETP_PIO8_MASK            (0x100U)
#define GPIO_DIRSET_DIRSETP_PIO8_SHIFT           (8U)
/*! DIRSETP_PIO8 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO8(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO8_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO8_MASK)
#define GPIO_DIRSET_DIRSETP_PIO9_MASK            (0x200U)
#define GPIO_DIRSET_DIRSETP_PIO9_SHIFT           (9U)
/*! DIRSETP_PIO9 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO9(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO9_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO9_MASK)
#define GPIO_DIRSET_DIRSETP_PIO10_MASK           (0x400U)
#define GPIO_DIRSET_DIRSETP_PIO10_SHIFT          (10U)
/*! DIRSETP_PIO10 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO10(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO10_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO10_MASK)
#define GPIO_DIRSET_DIRSETP_PIO11_MASK           (0x800U)
#define GPIO_DIRSET_DIRSETP_PIO11_SHIFT          (11U)
/*! DIRSETP_PIO11 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO11(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO11_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO11_MASK)
#define GPIO_DIRSET_DIRSETP_PIO12_MASK           (0x1000U)
#define GPIO_DIRSET_DIRSETP_PIO12_SHIFT          (12U)
/*! DIRSETP_PIO12 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO12(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO12_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO12_MASK)
#define GPIO_DIRSET_DIRSETP_PIO13_MASK           (0x2000U)
#define GPIO_DIRSET_DIRSETP_PIO13_SHIFT          (13U)
/*! DIRSETP_PIO13 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO13(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO13_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO13_MASK)
#define GPIO_DIRSET_DIRSETP_PIO14_MASK           (0x4000U)
#define GPIO_DIRSET_DIRSETP_PIO14_SHIFT          (14U)
/*! DIRSETP_PIO14 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO14(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO14_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO14_MASK)
#define GPIO_DIRSET_DIRSETP_PIO15_MASK           (0x8000U)
#define GPIO_DIRSET_DIRSETP_PIO15_SHIFT          (15U)
/*! DIRSETP_PIO15 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO15(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO15_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO15_MASK)
#define GPIO_DIRSET_DIRSETP_PIO16_MASK           (0x10000U)
#define GPIO_DIRSET_DIRSETP_PIO16_SHIFT          (16U)
/*! DIRSETP_PIO16 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO16(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO16_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO16_MASK)
#define GPIO_DIRSET_DIRSETP_PIO17_MASK           (0x20000U)
#define GPIO_DIRSET_DIRSETP_PIO17_SHIFT          (17U)
/*! DIRSETP_PIO17 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO17(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO17_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO17_MASK)
#define GPIO_DIRSET_DIRSETP_PIO18_MASK           (0x40000U)
#define GPIO_DIRSET_DIRSETP_PIO18_SHIFT          (18U)
/*! DIRSETP_PIO18 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO18(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO18_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO18_MASK)
#define GPIO_DIRSET_DIRSETP_PIO19_MASK           (0x80000U)
#define GPIO_DIRSET_DIRSETP_PIO19_SHIFT          (19U)
/*! DIRSETP_PIO19 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO19(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO19_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO19_MASK)
#define GPIO_DIRSET_DIRSETP_PIO20_MASK           (0x100000U)
#define GPIO_DIRSET_DIRSETP_PIO20_SHIFT          (20U)
/*! DIRSETP_PIO20 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO20(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO20_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO20_MASK)
#define GPIO_DIRSET_DIRSETP_PIO21_MASK           (0x200000U)
#define GPIO_DIRSET_DIRSETP_PIO21_SHIFT          (21U)
/*! DIRSETP_PIO21 - Set direction bits. 0 = no operation. 1 = Set direction bit.
 */
#define GPIO_DIRSET_DIRSETP_PIO21(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRSET_DIRSETP_PIO21_SHIFT)) & GPIO_DIRSET_DIRSETP_PIO21_MASK)
/*! @} */

/* The count of GPIO_DIRSET */
#define GPIO_DIRSET_COUNT                        (1U)

/*! @name DIRCLR - Clear pin direction bits */
/*! @{ */
#define GPIO_DIRCLR_DIRCLRP_PIO0_MASK            (0x1U)
#define GPIO_DIRCLR_DIRCLRP_PIO0_SHIFT           (0U)
/*! DIRCLRP_PIO0 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO0(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO0_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO0_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO1_MASK            (0x2U)
#define GPIO_DIRCLR_DIRCLRP_PIO1_SHIFT           (1U)
/*! DIRCLRP_PIO1 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO1(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO1_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO1_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO2_MASK            (0x4U)
#define GPIO_DIRCLR_DIRCLRP_PIO2_SHIFT           (2U)
/*! DIRCLRP_PIO2 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO2(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO2_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO2_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO3_MASK            (0x8U)
#define GPIO_DIRCLR_DIRCLRP_PIO3_SHIFT           (3U)
/*! DIRCLRP_PIO3 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO3(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO3_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO3_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO4_MASK            (0x10U)
#define GPIO_DIRCLR_DIRCLRP_PIO4_SHIFT           (4U)
/*! DIRCLRP_PIO4 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO4(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO4_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO4_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO5_MASK            (0x20U)
#define GPIO_DIRCLR_DIRCLRP_PIO5_SHIFT           (5U)
/*! DIRCLRP_PIO5 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO5(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO5_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO5_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO6_MASK            (0x40U)
#define GPIO_DIRCLR_DIRCLRP_PIO6_SHIFT           (6U)
/*! DIRCLRP_PIO6 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO6(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO6_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO6_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO7_MASK            (0x80U)
#define GPIO_DIRCLR_DIRCLRP_PIO7_SHIFT           (7U)
/*! DIRCLRP_PIO7 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO7(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO7_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO7_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO8_MASK            (0x100U)
#define GPIO_DIRCLR_DIRCLRP_PIO8_SHIFT           (8U)
/*! DIRCLRP_PIO8 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO8(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO8_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO8_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO9_MASK            (0x200U)
#define GPIO_DIRCLR_DIRCLRP_PIO9_SHIFT           (9U)
/*! DIRCLRP_PIO9 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO9(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO9_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO9_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO10_MASK           (0x400U)
#define GPIO_DIRCLR_DIRCLRP_PIO10_SHIFT          (10U)
/*! DIRCLRP_PIO10 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO10(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO10_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO10_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO11_MASK           (0x800U)
#define GPIO_DIRCLR_DIRCLRP_PIO11_SHIFT          (11U)
/*! DIRCLRP_PIO11 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO11(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO11_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO11_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO12_MASK           (0x1000U)
#define GPIO_DIRCLR_DIRCLRP_PIO12_SHIFT          (12U)
/*! DIRCLRP_PIO12 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO12(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO12_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO12_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO13_MASK           (0x2000U)
#define GPIO_DIRCLR_DIRCLRP_PIO13_SHIFT          (13U)
/*! DIRCLRP_PIO13 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO13(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO13_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO13_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO14_MASK           (0x4000U)
#define GPIO_DIRCLR_DIRCLRP_PIO14_SHIFT          (14U)
/*! DIRCLRP_PIO14 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO14(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO14_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO14_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO15_MASK           (0x8000U)
#define GPIO_DIRCLR_DIRCLRP_PIO15_SHIFT          (15U)
/*! DIRCLRP_PIO15 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO15(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO15_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO15_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO16_MASK           (0x10000U)
#define GPIO_DIRCLR_DIRCLRP_PIO16_SHIFT          (16U)
/*! DIRCLRP_PIO16 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO16(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO16_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO16_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO17_MASK           (0x20000U)
#define GPIO_DIRCLR_DIRCLRP_PIO17_SHIFT          (17U)
/*! DIRCLRP_PIO17 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO17(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO17_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO17_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO18_MASK           (0x40000U)
#define GPIO_DIRCLR_DIRCLRP_PIO18_SHIFT          (18U)
/*! DIRCLRP_PIO18 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO18(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO18_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO18_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO19_MASK           (0x80000U)
#define GPIO_DIRCLR_DIRCLRP_PIO19_SHIFT          (19U)
/*! DIRCLRP_PIO19 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO19(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO19_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO19_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO20_MASK           (0x100000U)
#define GPIO_DIRCLR_DIRCLRP_PIO20_SHIFT          (20U)
/*! DIRCLRP_PIO20 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO20(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO20_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO20_MASK)
#define GPIO_DIRCLR_DIRCLRP_PIO21_MASK           (0x200000U)
#define GPIO_DIRCLR_DIRCLRP_PIO21_SHIFT          (21U)
/*! DIRCLRP_PIO21 - Clear direction bits. 0 = no operation. 1 = Clear direction bit.
 */
#define GPIO_DIRCLR_DIRCLRP_PIO21(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRCLR_DIRCLRP_PIO21_SHIFT)) & GPIO_DIRCLR_DIRCLRP_PIO21_MASK)
/*! @} */

/* The count of GPIO_DIRCLR */
#define GPIO_DIRCLR_COUNT                        (1U)

/*! @name DIRNOT - Toggle pin direction bits */
/*! @{ */
#define GPIO_DIRNOT_DIRNOTP_PIO0_MASK            (0x1U)
#define GPIO_DIRNOT_DIRNOTP_PIO0_SHIFT           (0U)
/*! DIRNOTP_PIO0 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO0(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO0_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO0_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO1_MASK            (0x2U)
#define GPIO_DIRNOT_DIRNOTP_PIO1_SHIFT           (1U)
/*! DIRNOTP_PIO1 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO1(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO1_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO1_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO2_MASK            (0x4U)
#define GPIO_DIRNOT_DIRNOTP_PIO2_SHIFT           (2U)
/*! DIRNOTP_PIO2 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO2(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO2_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO2_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO3_MASK            (0x8U)
#define GPIO_DIRNOT_DIRNOTP_PIO3_SHIFT           (3U)
/*! DIRNOTP_PIO3 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO3(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO3_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO3_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO4_MASK            (0x10U)
#define GPIO_DIRNOT_DIRNOTP_PIO4_SHIFT           (4U)
/*! DIRNOTP_PIO4 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO4(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO4_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO4_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO5_MASK            (0x20U)
#define GPIO_DIRNOT_DIRNOTP_PIO5_SHIFT           (5U)
/*! DIRNOTP_PIO5 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO5(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO5_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO5_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO6_MASK            (0x40U)
#define GPIO_DIRNOT_DIRNOTP_PIO6_SHIFT           (6U)
/*! DIRNOTP_PIO6 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO6(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO6_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO6_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO7_MASK            (0x80U)
#define GPIO_DIRNOT_DIRNOTP_PIO7_SHIFT           (7U)
/*! DIRNOTP_PIO7 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO7(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO7_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO7_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO8_MASK            (0x100U)
#define GPIO_DIRNOT_DIRNOTP_PIO8_SHIFT           (8U)
/*! DIRNOTP_PIO8 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO8(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO8_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO8_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO9_MASK            (0x200U)
#define GPIO_DIRNOT_DIRNOTP_PIO9_SHIFT           (9U)
/*! DIRNOTP_PIO9 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO9(x)              (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO9_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO9_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO10_MASK           (0x400U)
#define GPIO_DIRNOT_DIRNOTP_PIO10_SHIFT          (10U)
/*! DIRNOTP_PIO10 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO10(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO10_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO10_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO11_MASK           (0x800U)
#define GPIO_DIRNOT_DIRNOTP_PIO11_SHIFT          (11U)
/*! DIRNOTP_PIO11 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO11(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO11_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO11_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO12_MASK           (0x1000U)
#define GPIO_DIRNOT_DIRNOTP_PIO12_SHIFT          (12U)
/*! DIRNOTP_PIO12 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO12(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO12_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO12_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO13_MASK           (0x2000U)
#define GPIO_DIRNOT_DIRNOTP_PIO13_SHIFT          (13U)
/*! DIRNOTP_PIO13 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO13(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO13_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO13_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO14_MASK           (0x4000U)
#define GPIO_DIRNOT_DIRNOTP_PIO14_SHIFT          (14U)
/*! DIRNOTP_PIO14 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO14(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO14_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO14_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO15_MASK           (0x8000U)
#define GPIO_DIRNOT_DIRNOTP_PIO15_SHIFT          (15U)
/*! DIRNOTP_PIO15 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO15(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO15_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO15_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO16_MASK           (0x10000U)
#define GPIO_DIRNOT_DIRNOTP_PIO16_SHIFT          (16U)
/*! DIRNOTP_PIO16 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO16(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO16_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO16_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO17_MASK           (0x20000U)
#define GPIO_DIRNOT_DIRNOTP_PIO17_SHIFT          (17U)
/*! DIRNOTP_PIO17 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO17(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO17_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO17_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO18_MASK           (0x40000U)
#define GPIO_DIRNOT_DIRNOTP_PIO18_SHIFT          (18U)
/*! DIRNOTP_PIO18 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO18(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO18_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO18_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO19_MASK           (0x80000U)
#define GPIO_DIRNOT_DIRNOTP_PIO19_SHIFT          (19U)
/*! DIRNOTP_PIO19 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO19(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO19_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO19_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO20_MASK           (0x100000U)
#define GPIO_DIRNOT_DIRNOTP_PIO20_SHIFT          (20U)
/*! DIRNOTP_PIO20 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO20(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO20_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO20_MASK)
#define GPIO_DIRNOT_DIRNOTP_PIO21_MASK           (0x200000U)
#define GPIO_DIRNOT_DIRNOTP_PIO21_SHIFT          (21U)
/*! DIRNOTP_PIO21 - Toggle direction bits. 0 = no operation. 1 = Toggle direction bit.
 */
#define GPIO_DIRNOT_DIRNOTP_PIO21(x)             (((uint32_t)(((uint32_t)(x)) << GPIO_DIRNOT_DIRNOTP_PIO21_SHIFT)) & GPIO_DIRNOT_DIRNOTP_PIO21_MASK)
/*! @} */

/* The count of GPIO_DIRNOT */
#define GPIO_DIRNOT_COUNT                        (1U)


/*!
 * @}
 */ /* end of group GPIO_Register_Masks */


/* GPIO - Peripheral instance base addresses */
/** Peripheral GPIO base address */
#define GPIO_BASE                                (0x40080000u)
/** Peripheral GPIO base pointer */
#define GPIO                                     ((GPIO_Type *)GPIO_BASE)
/** Array initializer of GPIO peripheral base addresses */
#define GPIO_BASE_ADDRS                          { GPIO_BASE }
/** Array initializer of GPIO peripheral base pointers */
#define GPIO_BASE_PTRS                           { GPIO }

/*!
 * @}
 */ /* end of group GPIO_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- I2C Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup I2C_Peripheral_Access_Layer I2C Peripheral Access Layer
 * @{
 */

/** I2C - Register Layout Typedef */
typedef struct {
  __IO uint32_t CFG;                               /**< Configuration for shared functions., offset: 0x0 */
  __IO uint32_t STAT;                              /**< Status register for Master, Slave and Monitor functions., offset: 0x4 */
  __IO uint32_t INTENSET;                          /**< Interrupt Enable Set and read register., offset: 0x8 */
  __O  uint32_t INTENCLR;                          /**< Interrupt Enable Clear register. Writing a 1 to this bit clears the corresponding bit in the INTENSET register, disabling that interrupt. This is a Write-only register., offset: 0xC */
  __IO uint32_t TIMEOUT;                           /**< Time-out value register., offset: 0x10 */
  __IO uint32_t CLKDIV;                            /**< Clock pre-divider for the entire I2C interface. This determines what time increments are used for the MSTTIME register and controls some timing of the Slave function., offset: 0x14 */
  __I  uint32_t INTSTAT;                           /**< Interrupt Status register for Master, Slave and Monitor functions., offset: 0x18 */
       uint8_t RESERVED_0[4];
  __IO uint32_t MSTCTL;                            /**< Master control register., offset: 0x20 */
  __IO uint32_t MSTTIME;                           /**< Master timing configuration., offset: 0x24 */
  __IO uint32_t MSTDAT;                            /**< Combined Master receiver and transmitter data register., offset: 0x28 */
       uint8_t RESERVED_1[20];
  __IO uint32_t SLVCTL;                            /**< Slave control register., offset: 0x40 */
  __IO uint32_t SLVDAT;                            /**< Combined Slave receiver and transmitter data register., offset: 0x44 */
  __IO uint32_t SLVADR[4];                         /**< Slave address 0., array offset: 0x48, array step: 0x4 */
  __IO uint32_t SLVQUAL0;                          /**< Slave Qualification for address 0., offset: 0x58 */
       uint8_t RESERVED_2[36];
  __I  uint32_t MONRXDAT;                          /**< Monitor receiver data register., offset: 0x80 */
       uint8_t RESERVED_3[3960];
  __I  uint32_t ID;                                /**< I2C Module Identifier, offset: 0xFFC */
} I2C_Type;

/* ----------------------------------------------------------------------------
   -- I2C Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup I2C_Register_Masks I2C Register Masks
 * @{
 */

/*! @name CFG - Configuration for shared functions. */
/*! @{ */
#define I2C_CFG_MSTEN_MASK                       (0x1U)
#define I2C_CFG_MSTEN_SHIFT                      (0U)
/*! MSTEN - Master Enable. When disabled, configurations settings for the Master function are not
 *    changed, but the Master function is internally reset.
 */
#define I2C_CFG_MSTEN(x)                         (((uint32_t)(((uint32_t)(x)) << I2C_CFG_MSTEN_SHIFT)) & I2C_CFG_MSTEN_MASK)
#define I2C_CFG_SLVEN_MASK                       (0x2U)
#define I2C_CFG_SLVEN_SHIFT                      (1U)
/*! SLVEN - Slave Enable. When disabled, configurations settings for the Slave function are not
 *    changed, but the Slave function is internally reset.
 */
#define I2C_CFG_SLVEN(x)                         (((uint32_t)(((uint32_t)(x)) << I2C_CFG_SLVEN_SHIFT)) & I2C_CFG_SLVEN_MASK)
#define I2C_CFG_MONEN_MASK                       (0x4U)
#define I2C_CFG_MONEN_SHIFT                      (2U)
/*! MONEN - Monitor Enable. When disabled, configurations settings for the Monitor function are not
 *    changed, but the Monitor function is internally reset.
 */
#define I2C_CFG_MONEN(x)                         (((uint32_t)(((uint32_t)(x)) << I2C_CFG_MONEN_SHIFT)) & I2C_CFG_MONEN_MASK)
#define I2C_CFG_TIMEOUT_MASK                     (0x8U)
#define I2C_CFG_TIMEOUT_SHIFT                    (3U)
/*! TIMEOUT - I2C bus Time-out Enable. When disabled, the time-out function is internally reset.
 */
#define I2C_CFG_TIMEOUT(x)                       (((uint32_t)(((uint32_t)(x)) << I2C_CFG_TIMEOUT_SHIFT)) & I2C_CFG_TIMEOUT_MASK)
#define I2C_CFG_MONCLKSTR_MASK                   (0x10U)
#define I2C_CFG_MONCLKSTR_SHIFT                  (4U)
/*! MONCLKSTR - Monitor function Clock Stretching.
 */
#define I2C_CFG_MONCLKSTR(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_CFG_MONCLKSTR_SHIFT)) & I2C_CFG_MONCLKSTR_MASK)
#define I2C_CFG_HSCAPABLE_MASK                   (0x20U)
#define I2C_CFG_HSCAPABLE_SHIFT                  (5U)
/*! HSCAPABLE - High-speed mode Capable enable. Since High Speed mode alters the way I2C pins drive
 *    and filter, as well as the timing for certain I2C signalling, enabling High-speed mode applies
 *    to all functions: master, slave, and monitor.
 */
#define I2C_CFG_HSCAPABLE(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_CFG_HSCAPABLE_SHIFT)) & I2C_CFG_HSCAPABLE_MASK)
/*! @} */

/*! @name STAT - Status register for Master, Slave and Monitor functions. */
/*! @{ */
#define I2C_STAT_MSTPENDING_MASK                 (0x1U)
#define I2C_STAT_MSTPENDING_SHIFT                (0U)
/*! MSTPENDING - Master Pending. Indicates that the Master is waiting to continue communication on
 *    the I2C-bus (pending) or is idle. When the master is pending, the MSTSTATE bits indicate what
 *    type of software service if any the master expects. This flag will cause an interrupt when set
 *    if, enabled via the INTENSET register. The MSTPENDING flag is not set when the DMA is handling
 *    an event (if the MSTDMA bit in the MSTCTL register is set). If the master is in the idle
 *    state, and no communication is needed, mask this interrupt. 0: In progress. Communication is in
 *    progress and the Master function is busy and cannot currently accept a command. 1: Pending. The
 *    Master function needs software service or is in the idle state. If the master is not in the
 *    idle state, it is waiting to receive or transmit data or the NACK bit.
 */
#define I2C_STAT_MSTPENDING(x)                   (((uint32_t)(((uint32_t)(x)) << I2C_STAT_MSTPENDING_SHIFT)) & I2C_STAT_MSTPENDING_MASK)
#define I2C_STAT_MSTSTATE_MASK                   (0xEU)
#define I2C_STAT_MSTSTATE_SHIFT                  (1U)
/*! MSTSTATE - Master State code. The master state code reflects the master state when the
 *    MSTPENDING bit is set, that is the master is pending or in the idle state. Each value of this field
 *    indicates a specific required service for the Master function. All other values are reserved. 0:
 *    Idle. The Master function is available to be used for a new transaction. 1: Receive ready.
 *    Received data available (Master Receiver mode). Address plus Read was previously sent and
 *    Acknowledged by slave. 2: Transmit ready. Data can be transmitted (Master Transmitter mode). Address
 *    plus Write was previously sent and Acknowledged by slave. 3: NACK address. Slave NACKed
 *    address. 4: NACK data. Slave NACKed transmitted data.
 */
#define I2C_STAT_MSTSTATE(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_STAT_MSTSTATE_SHIFT)) & I2C_STAT_MSTSTATE_MASK)
#define I2C_STAT_MSTARBLOSS_MASK                 (0x10U)
#define I2C_STAT_MSTARBLOSS_SHIFT                (4U)
/*! MSTARBLOSS - Master Arbitration Loss flag. This flag can be cleared by software writing a 1 to
 *    this bit. It is also cleared automatically a 1 is written to MSTCONTINUE. 0: No Arbitration
 *    Loss has occurred. 1: Arbitration Loss. The mater function has experienced an Arbitration Loss.
 *    At this point, the Master function has already stopped driving the bus and gone to an idle
 *    state. Software can respond by doing nothing, or by sending a Start in order to attempt to gain
 *    control of the bus when it next becomes idle.
 */
#define I2C_STAT_MSTARBLOSS(x)                   (((uint32_t)(((uint32_t)(x)) << I2C_STAT_MSTARBLOSS_SHIFT)) & I2C_STAT_MSTARBLOSS_MASK)
#define I2C_STAT_MSTSTSTPERR_MASK                (0x40U)
#define I2C_STAT_MSTSTSTPERR_SHIFT               (6U)
/*! MSTSTSTPERR - Master Start/Stop Error flag. This flag can be cleared by software writing a 1 to
 *    this bit. It is also cleared automatically a 1 is written to MSTCONTINUE. 0: No start/stop
 *    Error has occurred. 1: The master function has experienced a Start/Stop Error. A Start or Stop
 *    was detected at a time when it is not allowed by the I2C specification. The Master interface has
 *    stopped driving the bus and gone to an idle state, no action is required. A request for a
 *    Start could be made, or software could attempt to insure that thet bus has not stalled.
 */
#define I2C_STAT_MSTSTSTPERR(x)                  (((uint32_t)(((uint32_t)(x)) << I2C_STAT_MSTSTSTPERR_SHIFT)) & I2C_STAT_MSTSTSTPERR_MASK)
#define I2C_STAT_SLVPENDING_MASK                 (0x100U)
#define I2C_STAT_SLVPENDING_SHIFT                (8U)
/*! SLVPENDING - Slave Pending. Indicates that the Slave function is waiting to continue
 *    communication on the I2C-bus and needs software service. This flag will cause an interrupt when set if
 *    enabled via INTENSET. The SLVPENDING flag is not set when the DMA is handling an event (if the
 *    SLVDMA bit in the SLVCTL register is set). The SLVPENDING flag is read-only and is
 *    automatically cleared when a 1 is written to the SLVCONTINUE bit in the SLVCTL register. The point in time
 *    when SlvPending is set depends on whether the I2C interface is in HSCAPABLE mode. When the
 *    I2C interface is configured to be HSCAPABLE, HS master codes are detected automatically. Due to
 *    the requirements of the HS I2C specification, slave addresses must also be detected
 *    automatically, since the address must be acknowledged before the clock can be stretched. 0: In progress.
 *    The slave function does not currently need service. 1: Pending. The Slave function needs
 *    service. Information on what is needed can be found in the adjacent SLVSTATE field.
 */
#define I2C_STAT_SLVPENDING(x)                   (((uint32_t)(((uint32_t)(x)) << I2C_STAT_SLVPENDING_SHIFT)) & I2C_STAT_SLVPENDING_MASK)
#define I2C_STAT_SLVSTATE_MASK                   (0x600U)
#define I2C_STAT_SLVSTATE_SHIFT                  (9U)
/*! SLVSTATE - Slave State code. Each value of this field indicates a specific required service for
 *    the Slave function. All other values are reserved. See Table 393 for state values and actions.
 *    Remark: note that the occurrence of some states and how they are handled are affected by DMA
 *    mode and Automatic Operation modes. Slave state codes are: 0: Slave address. Address plus R/W
 *    received. At least one of the four slave addresses has been matched by hardware. 1: Slave
 *    receive. Received data is available (Slave Receiver mode). 2: Slave transmit. Data can be
 *    transmitted (Slave transmitter mode).
 */
#define I2C_STAT_SLVSTATE(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_STAT_SLVSTATE_SHIFT)) & I2C_STAT_SLVSTATE_MASK)
#define I2C_STAT_SLVNOTSTR_MASK                  (0x800U)
#define I2C_STAT_SLVNOTSTR_SHIFT                 (11U)
/*! SLVNOTSTR - Slave Not Stretching. Indicates when the slave function is stretching the I2C clock.
 *    This is needed in order to gracefully invoke Deep Sleep or Power-down modes during slave
 *    operation. This read-only flag reflects the slave function status in real time. 0: stretching. The
 *    slave function is currently stretching the I2C bus clock. Deep-sleep mode can not be entered
 *    at this time. 1. Not stretching. The slave function is not currently stretching the I2C bus
 *    clock. Deep-sleep mode could be entered at this time.
 */
#define I2C_STAT_SLVNOTSTR(x)                    (((uint32_t)(((uint32_t)(x)) << I2C_STAT_SLVNOTSTR_SHIFT)) & I2C_STAT_SLVNOTSTR_MASK)
#define I2C_STAT_SLVIDX_MASK                     (0x3000U)
#define I2C_STAT_SLVIDX_SHIFT                    (12U)
/*! SLVIDX - Slave address match Index. This field is valid when the I2C slave function has been
 *    selected by receiving an address that matches one of the slave addresses defined by any enabled
 *    slave address registers, and provides an identification of the address that was matched. It is
 *    possible that more than one address could be matched, but only one match can be reported here.
 *    0: Address 0. Slave address 0 was matched. 1: Address 1. Slave address 1 was matched. 2:
 *    Address 2. Slave address 2 was matched. 3: Address 3. Slave address 3 was matched.
 */
#define I2C_STAT_SLVIDX(x)                       (((uint32_t)(((uint32_t)(x)) << I2C_STAT_SLVIDX_SHIFT)) & I2C_STAT_SLVIDX_MASK)
#define I2C_STAT_SLVSEL_MASK                     (0x4000U)
#define I2C_STAT_SLVSEL_SHIFT                    (14U)
/*! SLVSEL - Slave selected flag. SLVSEL is set after an address match when software tells the Slave
 *    function to acknowledge the address, or when the address has been automatically acknowledged.
 *    It is cleared when another address cycle presents an address that does not match an enabled
 *    address on the Slave function, when slave software decides to NACK a matched address, when
 *    there is a Stop detected on the bus, when the master NACKs slave data, and in some combinations of
 *    Automatic Operation. SLVSEL is not cleared if software NACKs data. 0: Not selected. The slave
 *    function is not currently selected. 1: Selected. The slave function is currently selected.
 */
#define I2C_STAT_SLVSEL(x)                       (((uint32_t)(((uint32_t)(x)) << I2C_STAT_SLVSEL_SHIFT)) & I2C_STAT_SLVSEL_MASK)
#define I2C_STAT_SLVDESEL_MASK                   (0x8000U)
#define I2C_STAT_SLVDESEL_SHIFT                  (15U)
/*! SLVDESEL - Slave Deselected flag. This flag will cause an interrupt when set if enabled via
 *    INTENSET. This flag can be cleared by writing a 1 to this bit. 0: Not deselected. The slave
 *    function has not become deslected. This does not mean that it is currently selected. That
 *    information can be found in the SLVSEL flag. 1: Deselected. The slave function has become deselected.
 *    This is specifically caused by the SLVSEL flag changing from 1 to 0. See the description of
 *    SLVSEL for details on when that event occurs.
 */
#define I2C_STAT_SLVDESEL(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_STAT_SLVDESEL_SHIFT)) & I2C_STAT_SLVDESEL_MASK)
#define I2C_STAT_MONRDY_MASK                     (0x10000U)
#define I2C_STAT_MONRDY_SHIFT                    (16U)
/*! MONRDY - Monitor Ready. This flag is cleared when the MONRXDAT register is read. 0: No data. The
 *    Monitor function does not currently have data available. 1: Data waiting. The Monitor
 *    function has data waiting to be read.
 */
#define I2C_STAT_MONRDY(x)                       (((uint32_t)(((uint32_t)(x)) << I2C_STAT_MONRDY_SHIFT)) & I2C_STAT_MONRDY_MASK)
#define I2C_STAT_MONOV_MASK                      (0x20000U)
#define I2C_STAT_MONOV_SHIFT                     (17U)
/*! MONOV - Monitor Overflow flag. 0: No overrun. Monitor data has not overrun 1: Overrun. A monitor
 *    data overrun has occurred. This can only happen when Monitor clock stretching is not enabled
 *    via the MOCCLKSTR bit in the CFG register. Writing 1 to this bit clears the flag.
 */
#define I2C_STAT_MONOV(x)                        (((uint32_t)(((uint32_t)(x)) << I2C_STAT_MONOV_SHIFT)) & I2C_STAT_MONOV_MASK)
#define I2C_STAT_MONACTIVE_MASK                  (0x40000U)
#define I2C_STAT_MONACTIVE_SHIFT                 (18U)
/*! MONACTIVE - Monitor Active flag. Indicates when the Monitor function considers the I2C bus to be
 *    active. Active is defined here as when some Master is on the bus: a bus Start has occurred
 *    more recently than a bus Stop. 0: Inactive. The Monitor function considers the I2C bus to be
 *    inactive. 1: Active. The Monitor function considers the I2C bus to be active.
 */
#define I2C_STAT_MONACTIVE(x)                    (((uint32_t)(((uint32_t)(x)) << I2C_STAT_MONACTIVE_SHIFT)) & I2C_STAT_MONACTIVE_MASK)
#define I2C_STAT_MONIDLE_MASK                    (0x80000U)
#define I2C_STAT_MONIDLE_SHIFT                   (19U)
/*! MONIDLE - Monitor Idle flag. This flag is set when the Monitor function sees the I2C bus change
 *    from active to inactive. This can be used by software to decide when to process data
 *    accumulated by the Monitor function. This flag will cause an interrupt when set if enabled via the
 *    INTENSET register. The flag can be cleared by writing a 1 to this bit. 0: Not idle. The I2C bus is
 *    not idle, or this flag has been cleared by software. 1: Idle. The I2C bus has gone idle at
 *    least once since the last time this flag was cleared by software.
 */
#define I2C_STAT_MONIDLE(x)                      (((uint32_t)(((uint32_t)(x)) << I2C_STAT_MONIDLE_SHIFT)) & I2C_STAT_MONIDLE_MASK)
#define I2C_STAT_EVENTTIMEOUT_MASK               (0x1000000U)
#define I2C_STAT_EVENTTIMEOUT_SHIFT              (24U)
/*! EVENTTIMEOUT - Event Time-out Interrupt flag. Indicates when the time between events has been
 *    longer than the time specified by the TIMEOUT register. Events include Start, Stop, and clock
 *    edges. The flag is cleared by writing a 1 to this bit. No time-out is created when the I2C-bus
 *    is idle. 0: No time-out. I2C bus events have not casued a time-out. 1: Event time-out. The time
 *    between I2C bus events has been longer than the time specified by the TIMEOUT register.
 */
#define I2C_STAT_EVENTTIMEOUT(x)                 (((uint32_t)(((uint32_t)(x)) << I2C_STAT_EVENTTIMEOUT_SHIFT)) & I2C_STAT_EVENTTIMEOUT_MASK)
#define I2C_STAT_SCLTIMEOUT_MASK                 (0x2000000U)
#define I2C_STAT_SCLTIMEOUT_SHIFT                (25U)
/*! SCLTIMEOUT - SCL Time-out Interrupt flag. Indicates when SCL has remained low longer than the
 *    time specific by the TIMEOUT register. The flag is cleared by writing a 1 to this bit. 0: No
 *    time-out. SCL low time has not caused a time-out. 1: Time-out. SCL low time has not caused a
 *    time-out.
 */
#define I2C_STAT_SCLTIMEOUT(x)                   (((uint32_t)(((uint32_t)(x)) << I2C_STAT_SCLTIMEOUT_SHIFT)) & I2C_STAT_SCLTIMEOUT_MASK)
/*! @} */

/*! @name INTENSET - Interrupt Enable Set and read register. */
/*! @{ */
#define I2C_INTENSET_MSTPENDINGEN_MASK           (0x1U)
#define I2C_INTENSET_MSTPENDINGEN_SHIFT          (0U)
/*! MSTPENDINGEN - Master Pending interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_MSTPENDINGEN(x)             (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_MSTPENDINGEN_SHIFT)) & I2C_INTENSET_MSTPENDINGEN_MASK)
#define I2C_INTENSET_MSTARBLOSSEN_MASK           (0x10U)
#define I2C_INTENSET_MSTARBLOSSEN_SHIFT          (4U)
/*! MSTARBLOSSEN - Master Arbitration Loss interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_MSTARBLOSSEN(x)             (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_MSTARBLOSSEN_SHIFT)) & I2C_INTENSET_MSTARBLOSSEN_MASK)
#define I2C_INTENSET_MSTSTSTPERREN_MASK          (0x40U)
#define I2C_INTENSET_MSTSTSTPERREN_SHIFT         (6U)
/*! MSTSTSTPERREN - Master Start/Stop Error interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_MSTSTSTPERREN(x)            (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_MSTSTSTPERREN_SHIFT)) & I2C_INTENSET_MSTSTSTPERREN_MASK)
#define I2C_INTENSET_SLVPENDINGEN_MASK           (0x100U)
#define I2C_INTENSET_SLVPENDINGEN_SHIFT          (8U)
/*! SLVPENDINGEN - Slave Pending interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_SLVPENDINGEN(x)             (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_SLVPENDINGEN_SHIFT)) & I2C_INTENSET_SLVPENDINGEN_MASK)
#define I2C_INTENSET_SLVNOTSTREN_MASK            (0x800U)
#define I2C_INTENSET_SLVNOTSTREN_SHIFT           (11U)
/*! SLVNOTSTREN - Slave Not Stretching interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_SLVNOTSTREN(x)              (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_SLVNOTSTREN_SHIFT)) & I2C_INTENSET_SLVNOTSTREN_MASK)
#define I2C_INTENSET_SLVDESELEN_MASK             (0x8000U)
#define I2C_INTENSET_SLVDESELEN_SHIFT            (15U)
/*! SLVDESELEN - Slave Deselect interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_SLVDESELEN(x)               (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_SLVDESELEN_SHIFT)) & I2C_INTENSET_SLVDESELEN_MASK)
#define I2C_INTENSET_MONRDYEN_MASK               (0x10000U)
#define I2C_INTENSET_MONRDYEN_SHIFT              (16U)
/*! MONRDYEN - Monitor data Ready interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_MONRDYEN(x)                 (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_MONRDYEN_SHIFT)) & I2C_INTENSET_MONRDYEN_MASK)
#define I2C_INTENSET_MONOVEN_MASK                (0x20000U)
#define I2C_INTENSET_MONOVEN_SHIFT               (17U)
/*! MONOVEN - Monitor Overrun interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_MONOVEN(x)                  (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_MONOVEN_SHIFT)) & I2C_INTENSET_MONOVEN_MASK)
#define I2C_INTENSET_MONIDLEEN_MASK              (0x80000U)
#define I2C_INTENSET_MONIDLEEN_SHIFT             (19U)
/*! MONIDLEEN - Monitor Idle interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_MONIDLEEN(x)                (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_MONIDLEEN_SHIFT)) & I2C_INTENSET_MONIDLEEN_MASK)
#define I2C_INTENSET_EVENTTIMEOUTEN_MASK         (0x1000000U)
#define I2C_INTENSET_EVENTTIMEOUTEN_SHIFT        (24U)
/*! EVENTTIMEOUTEN - Event time-out interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_EVENTTIMEOUTEN(x)           (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_EVENTTIMEOUTEN_SHIFT)) & I2C_INTENSET_EVENTTIMEOUTEN_MASK)
#define I2C_INTENSET_SCLTIMEOUTEN_MASK           (0x2000000U)
#define I2C_INTENSET_SCLTIMEOUTEN_SHIFT          (25U)
/*! SCLTIMEOUTEN - SCL time-out interrupt Enable. 0: interrupt is disabled. 1: interrupt is enabled.
 */
#define I2C_INTENSET_SCLTIMEOUTEN(x)             (((uint32_t)(((uint32_t)(x)) << I2C_INTENSET_SCLTIMEOUTEN_SHIFT)) & I2C_INTENSET_SCLTIMEOUTEN_MASK)
/*! @} */

/*! @name INTENCLR - Interrupt Enable Clear register. Writing a 1 to this bit clears the corresponding bit in the INTENSET register, disabling that interrupt. This is a Write-only register. */
/*! @{ */
#define I2C_INTENCLR_MSTPCLRDINGCLR_MASK         (0x1U)
#define I2C_INTENCLR_MSTPCLRDINGCLR_SHIFT        (0U)
/*! MSTPCLRDINGCLR - Master Pending interrupt clear.
 */
#define I2C_INTENCLR_MSTPCLRDINGCLR(x)           (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_MSTPCLRDINGCLR_SHIFT)) & I2C_INTENCLR_MSTPCLRDINGCLR_MASK)
#define I2C_INTENCLR_MSTARBLOSSCLR_MASK          (0x10U)
#define I2C_INTENCLR_MSTARBLOSSCLR_SHIFT         (4U)
/*! MSTARBLOSSCLR - Master Arbitration Loss interrupt clear.
 */
#define I2C_INTENCLR_MSTARBLOSSCLR(x)            (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_MSTARBLOSSCLR_SHIFT)) & I2C_INTENCLR_MSTARBLOSSCLR_MASK)
#define I2C_INTENCLR_MSTSTSTPERRCLR_MASK         (0x40U)
#define I2C_INTENCLR_MSTSTSTPERRCLR_SHIFT        (6U)
/*! MSTSTSTPERRCLR - Master Start/Stop Error interrupt clear.
 */
#define I2C_INTENCLR_MSTSTSTPERRCLR(x)           (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_MSTSTSTPERRCLR_SHIFT)) & I2C_INTENCLR_MSTSTSTPERRCLR_MASK)
#define I2C_INTENCLR_SLVPENDINGCLR_MASK          (0x100U)
#define I2C_INTENCLR_SLVPENDINGCLR_SHIFT         (8U)
/*! SLVPENDINGCLR - Slave Pending interrupt clear.
 */
#define I2C_INTENCLR_SLVPENDINGCLR(x)            (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_SLVPENDINGCLR_SHIFT)) & I2C_INTENCLR_SLVPENDINGCLR_MASK)
#define I2C_INTENCLR_SLVNOTSTRCLR_MASK           (0x800U)
#define I2C_INTENCLR_SLVNOTSTRCLR_SHIFT          (11U)
/*! SLVNOTSTRCLR - Slave Not Stretching interrupt clear.
 */
#define I2C_INTENCLR_SLVNOTSTRCLR(x)             (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_SLVNOTSTRCLR_SHIFT)) & I2C_INTENCLR_SLVNOTSTRCLR_MASK)
#define I2C_INTENCLR_SLVDESELCLR_MASK            (0x8000U)
#define I2C_INTENCLR_SLVDESELCLR_SHIFT           (15U)
/*! SLVDESELCLR - Slave Deselect interrupt clear.
 */
#define I2C_INTENCLR_SLVDESELCLR(x)              (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_SLVDESELCLR_SHIFT)) & I2C_INTENCLR_SLVDESELCLR_MASK)
#define I2C_INTENCLR_MONRDYCLR_MASK              (0x10000U)
#define I2C_INTENCLR_MONRDYCLR_SHIFT             (16U)
/*! MONRDYCLR - Monitor data Ready interrupt clear.
 */
#define I2C_INTENCLR_MONRDYCLR(x)                (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_MONRDYCLR_SHIFT)) & I2C_INTENCLR_MONRDYCLR_MASK)
#define I2C_INTENCLR_MONOVCLR_MASK               (0x20000U)
#define I2C_INTENCLR_MONOVCLR_SHIFT              (17U)
/*! MONOVCLR - Monitor Overrun interrupt clear.
 */
#define I2C_INTENCLR_MONOVCLR(x)                 (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_MONOVCLR_SHIFT)) & I2C_INTENCLR_MONOVCLR_MASK)
#define I2C_INTENCLR_MONIDLECLR_MASK             (0x80000U)
#define I2C_INTENCLR_MONIDLECLR_SHIFT            (19U)
/*! MONIDLECLR - Monitor Idle interrupt clear.
 */
#define I2C_INTENCLR_MONIDLECLR(x)               (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_MONIDLECLR_SHIFT)) & I2C_INTENCLR_MONIDLECLR_MASK)
#define I2C_INTENCLR_EVCLRTTIMEOUTCLR_MASK       (0x1000000U)
#define I2C_INTENCLR_EVCLRTTIMEOUTCLR_SHIFT      (24U)
/*! EVCLRTTIMEOUTCLR - Event time-out interrupt clear.
 */
#define I2C_INTENCLR_EVCLRTTIMEOUTCLR(x)         (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_EVCLRTTIMEOUTCLR_SHIFT)) & I2C_INTENCLR_EVCLRTTIMEOUTCLR_MASK)
#define I2C_INTENCLR_SCLTIMEOUTCLR_MASK          (0x2000000U)
#define I2C_INTENCLR_SCLTIMEOUTCLR_SHIFT         (25U)
/*! SCLTIMEOUTCLR - SCL time-out interrupt clear.
 */
#define I2C_INTENCLR_SCLTIMEOUTCLR(x)            (((uint32_t)(((uint32_t)(x)) << I2C_INTENCLR_SCLTIMEOUTCLR_SHIFT)) & I2C_INTENCLR_SCLTIMEOUTCLR_MASK)
/*! @} */

/*! @name TIMEOUT - Time-out value register. */
/*! @{ */
#define I2C_TIMEOUT_TOMIN_MASK                   (0xFU)
#define I2C_TIMEOUT_TOMIN_SHIFT                  (0U)
/*! TOMIN - Time-out time value, bottom four bits. These are hard-wired to 0xF. This gives a minimum
 *    time-out of 16 I2C function clocks and also a time-out resolution of 16 I2C function clocks.
 */
#define I2C_TIMEOUT_TOMIN(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_TIMEOUT_TOMIN_SHIFT)) & I2C_TIMEOUT_TOMIN_MASK)
#define I2C_TIMEOUT_TO_MASK                      (0xFFF0U)
#define I2C_TIMEOUT_TO_SHIFT                     (4U)
/*! TO - Time-out time value. Specifies the time-out interval value in increments of 16 I2C function
 *    clocks, as defined by the CLKDIV register. To change this value while I2C is in operation,
 *    disable all time-outs, write a new value to TIMEOUT, then re-enable time-outs. 0x000: A time-out
 *    will occur after 16 counts of the I2C function clock. 0x001: A time-out will occur after 32
 *    counts of the I2C function clock. ... 0xFFF: A time-out will occur after 65,536 counts of the
 *    I2C function clock.
 */
#define I2C_TIMEOUT_TO(x)                        (((uint32_t)(((uint32_t)(x)) << I2C_TIMEOUT_TO_SHIFT)) & I2C_TIMEOUT_TO_MASK)
/*! @} */

/*! @name CLKDIV - Clock pre-divider for the entire I2C interface. This determines what time increments are used for the MSTTIME register and controls some timing of the Slave function. */
/*! @{ */
#define I2C_CLKDIV_DIVVAL_MASK                   (0xFFFFU)
#define I2C_CLKDIV_DIVVAL_SHIFT                  (0U)
/*! DIVVAL - This field controls how the I2C clock (FCLK) is used by the I2C functions that need an
 *    internal clock in order to operate. I2C block should be configured for 8MHz clock, this will
 *    limit SCL master clock range from 444kHz to 2MHz. 0x0000 = FCLK is used directly by the I2C.
 *    0x0001 = FCLK is divided by 2 before use. 0x0002 = FCLK is divided by 3 before use. ... 0xFFFF =
 *    FCLK is divided by 65,536 before use.
 */
#define I2C_CLKDIV_DIVVAL(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_CLKDIV_DIVVAL_SHIFT)) & I2C_CLKDIV_DIVVAL_MASK)
/*! @} */

/*! @name INTSTAT - Interrupt Status register for Master, Slave and Monitor functions. */
/*! @{ */
#define I2C_INTSTAT_MSTPENDING_MASK              (0x1U)
#define I2C_INTSTAT_MSTPENDING_SHIFT             (0U)
/*! MSTPENDING - Master Pending interrupt.
 */
#define I2C_INTSTAT_MSTPENDING(x)                (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_MSTPENDING_SHIFT)) & I2C_INTSTAT_MSTPENDING_MASK)
#define I2C_INTSTAT_MSTARBLOSS_MASK              (0x10U)
#define I2C_INTSTAT_MSTARBLOSS_SHIFT             (4U)
/*! MSTARBLOSS - Master Arbitration Loss interrupt.
 */
#define I2C_INTSTAT_MSTARBLOSS(x)                (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_MSTARBLOSS_SHIFT)) & I2C_INTSTAT_MSTARBLOSS_MASK)
#define I2C_INTSTAT_MSTSTSTPERR_MASK             (0x40U)
#define I2C_INTSTAT_MSTSTSTPERR_SHIFT            (6U)
/*! MSTSTSTPERR - Master Start/Stop Error interrupt.
 */
#define I2C_INTSTAT_MSTSTSTPERR(x)               (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_MSTSTSTPERR_SHIFT)) & I2C_INTSTAT_MSTSTSTPERR_MASK)
#define I2C_INTSTAT_SLVPENDING_MASK              (0x100U)
#define I2C_INTSTAT_SLVPENDING_SHIFT             (8U)
/*! SLVPENDING - Slave Pending interrupt.
 */
#define I2C_INTSTAT_SLVPENDING(x)                (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_SLVPENDING_SHIFT)) & I2C_INTSTAT_SLVPENDING_MASK)
#define I2C_INTSTAT_SLVNOTSTR_MASK               (0x800U)
#define I2C_INTSTAT_SLVNOTSTR_SHIFT              (11U)
/*! SLVNOTSTR - Slave Not Stretching interrupt.
 */
#define I2C_INTSTAT_SLVNOTSTR(x)                 (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_SLVNOTSTR_SHIFT)) & I2C_INTSTAT_SLVNOTSTR_MASK)
#define I2C_INTSTAT_SLVDESEL_MASK                (0x8000U)
#define I2C_INTSTAT_SLVDESEL_SHIFT               (15U)
/*! SLVDESEL - Slave Deselect interrupt.
 */
#define I2C_INTSTAT_SLVDESEL(x)                  (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_SLVDESEL_SHIFT)) & I2C_INTSTAT_SLVDESEL_MASK)
#define I2C_INTSTAT_MONRDY_MASK                  (0x10000U)
#define I2C_INTSTAT_MONRDY_SHIFT                 (16U)
/*! MONRDY - Monitor data Ready interrupt.
 */
#define I2C_INTSTAT_MONRDY(x)                    (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_MONRDY_SHIFT)) & I2C_INTSTAT_MONRDY_MASK)
#define I2C_INTSTAT_MONOV_MASK                   (0x20000U)
#define I2C_INTSTAT_MONOV_SHIFT                  (17U)
/*! MONOV - Monitor Overrun interrupt.
 */
#define I2C_INTSTAT_MONOV(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_MONOV_SHIFT)) & I2C_INTSTAT_MONOV_MASK)
#define I2C_INTSTAT_MONIDLE_MASK                 (0x80000U)
#define I2C_INTSTAT_MONIDLE_SHIFT                (19U)
/*! MONIDLE - Monitor Idle interrupt.
 */
#define I2C_INTSTAT_MONIDLE(x)                   (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_MONIDLE_SHIFT)) & I2C_INTSTAT_MONIDLE_MASK)
#define I2C_INTSTAT_EVTTIMEOUT_MASK              (0x1000000U)
#define I2C_INTSTAT_EVTTIMEOUT_SHIFT             (24U)
/*! EVTTIMEOUT - Event time-out interrupt.
 */
#define I2C_INTSTAT_EVTTIMEOUT(x)                (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_EVTTIMEOUT_SHIFT)) & I2C_INTSTAT_EVTTIMEOUT_MASK)
#define I2C_INTSTAT_SCLTIMEOUT_MASK              (0x2000000U)
#define I2C_INTSTAT_SCLTIMEOUT_SHIFT             (25U)
/*! SCLTIMEOUT - SCL time-out interrupt.
 */
#define I2C_INTSTAT_SCLTIMEOUT(x)                (((uint32_t)(((uint32_t)(x)) << I2C_INTSTAT_SCLTIMEOUT_SHIFT)) & I2C_INTSTAT_SCLTIMEOUT_MASK)
/*! @} */

/*! @name MSTCTL - Master control register. */
/*! @{ */
#define I2C_MSTCTL_MSTCONTINUE_MASK              (0x1U)
#define I2C_MSTCTL_MSTCONTINUE_SHIFT             (0U)
/*! MSTCONTINUE - Master Continue. This bit is write-only. 0: No effect. 1: Continue. Informs the
 *    Master function to continue to the next operation. This must be done after writing transmit
 *    data, reading received data, or other housekeeping related to the next bus operation.
 */
#define I2C_MSTCTL_MSTCONTINUE(x)                (((uint32_t)(((uint32_t)(x)) << I2C_MSTCTL_MSTCONTINUE_SHIFT)) & I2C_MSTCTL_MSTCONTINUE_MASK)
#define I2C_MSTCTL_MSTSTART_MASK                 (0x2U)
#define I2C_MSTCTL_MSTSTART_SHIFT                (1U)
/*! MSTSTART - Master Start control. This bit is write-only. 0: No effect. 1. Start. A start will be
 *    generated on the I2C bus at the next allowed time.
 */
#define I2C_MSTCTL_MSTSTART(x)                   (((uint32_t)(((uint32_t)(x)) << I2C_MSTCTL_MSTSTART_SHIFT)) & I2C_MSTCTL_MSTSTART_MASK)
#define I2C_MSTCTL_MSTSTOP_MASK                  (0x4U)
#define I2C_MSTCTL_MSTSTOP_SHIFT                 (2U)
/*! MSTSTOP - Master Stop control. This bit is write-only. 0: No effect. 1. Stop. A stop will be
 *    generated on the I2C bus at the next allowed time, preceded by a NACK to the slave if the master
 *    is receiving data from the salve (Master Receiver mode).
 */
#define I2C_MSTCTL_MSTSTOP(x)                    (((uint32_t)(((uint32_t)(x)) << I2C_MSTCTL_MSTSTOP_SHIFT)) & I2C_MSTCTL_MSTSTOP_MASK)
#define I2C_MSTCTL_MSTDMA_MASK                   (0x8U)
#define I2C_MSTCTL_MSTDMA_SHIFT                  (3U)
/*! MSTDMA - Master DMA enable. Data operations of the I2C can be performed with DMA. Protocol type
 *    operations such as Start, address, Stop, and address match must always be done with software,
 *    typically via an interrupt. Address acknowledgement must also be done by software except when
 *    the I2C is configured to be HSCAPABLE (and address acknowledgement is handled entirely by
 *    hardware) or when Automatic Operation is enabled. When a DMA data transfer is complete, MSTDMA
 *    must be cleared prior to beginning the next operation, typically a Start or Stop.This bit is
 *    read/write. 0: Disable. No DMA requests are generated for master operation. 1: Enable. A DMA
 *    request is generated for I2C master data operations. When this I2C master is generating Acknowledge
 *    bits in Master Receiver mode, the acknowledge is generated automatically.
 */
#define I2C_MSTCTL_MSTDMA(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_MSTCTL_MSTDMA_SHIFT)) & I2C_MSTCTL_MSTDMA_MASK)
/*! @} */

/*! @name MSTTIME - Master timing configuration. */
/*! @{ */
#define I2C_MSTTIME_MSTSCLLOW_MASK               (0x7U)
#define I2C_MSTTIME_MSTSCLLOW_SHIFT              (0U)
/*! MSTSCLLOW - Master SCL Low time. Specifies the minimum low time that will be asserted by this
 *    master on SCL. Other devices on the bus (masters or slaves) could lengthen this time. This
 *    corresponds to the parameter tLOW in the I2C bus specification. I2C bus specification parameters
 *    tBUF and tSU;STA have the same values and are also controlled by MSTSCLLOW. The minimum SCL low
 *    time is (MSTSCLLOW + 2) clocks of the I2C clock pre-divider.
 */
#define I2C_MSTTIME_MSTSCLLOW(x)                 (((uint32_t)(((uint32_t)(x)) << I2C_MSTTIME_MSTSCLLOW_SHIFT)) & I2C_MSTTIME_MSTSCLLOW_MASK)
#define I2C_MSTTIME_MSTSCLHIGH_MASK              (0x70U)
#define I2C_MSTTIME_MSTSCLHIGH_SHIFT             (4U)
/*! MSTSCLHIGH - Master SCL High time. Specifies the minimum high time that will be asserted by this
 *    master on SCL. Other masters in a multi-master system could shorten this time. This
 *    corresponds to the parameter tHIGH in the I2C bus specification. I2C bus specification parameters
 *    tSU;STO and tHD;STA have the same values and are also controlled by MSTSCLHIGH.
 */
#define I2C_MSTTIME_MSTSCLHIGH(x)                (((uint32_t)(((uint32_t)(x)) << I2C_MSTTIME_MSTSCLHIGH_SHIFT)) & I2C_MSTTIME_MSTSCLHIGH_MASK)
/*! @} */

/*! @name MSTDAT - Combined Master receiver and transmitter data register. */
/*! @{ */
#define I2C_MSTDAT_DATA_MASK                     (0xFFU)
#define I2C_MSTDAT_DATA_SHIFT                    (0U)
/*! DATA - Master function data register. Read: read the most recently received data for the Master
 *    function. Write: transmit data using the Master function.
 */
#define I2C_MSTDAT_DATA(x)                       (((uint32_t)(((uint32_t)(x)) << I2C_MSTDAT_DATA_SHIFT)) & I2C_MSTDAT_DATA_MASK)
/*! @} */

/*! @name SLVCTL - Slave control register. */
/*! @{ */
#define I2C_SLVCTL_SLVCONTINUE_MASK              (0x1U)
#define I2C_SLVCTL_SLVCONTINUE_SHIFT             (0U)
/*! SLVCONTINUE - Slave Continue. 0: no effect 1: Continue. Informs the Slave function to continue
 *    to the next operation, by clearing the SLVPENDING flag in the STAT register. This must be done
 *    after writing transmit data, reading recevied data, or any other housekeeping related to the
 *    next bus operation. Automatic Operation has different requirements. SLVCONTINUE should not be
 *    set unless SLVPENDING=1.
 */
#define I2C_SLVCTL_SLVCONTINUE(x)                (((uint32_t)(((uint32_t)(x)) << I2C_SLVCTL_SLVCONTINUE_SHIFT)) & I2C_SLVCTL_SLVCONTINUE_MASK)
#define I2C_SLVCTL_SLVNACK_MASK                  (0x2U)
#define I2C_SLVCTL_SLVNACK_SHIFT                 (1U)
/*! SLVNACK - Slave NACK. 0: No effect. 1: NACK. Causes the Slave function to NACK the master when
 *    the slave is receiving data from the master (Slave Receiver mode).
 */
#define I2C_SLVCTL_SLVNACK(x)                    (((uint32_t)(((uint32_t)(x)) << I2C_SLVCTL_SLVNACK_SHIFT)) & I2C_SLVCTL_SLVNACK_MASK)
#define I2C_SLVCTL_SLVDMA_MASK                   (0x8U)
#define I2C_SLVCTL_SLVDMA_SHIFT                  (3U)
/*! SLVDMA - Slave DMA enable. 0: Slave DMA enable. 1: Enabled. DMA requests are issued for I2C
 *    slave data transmission and reception.
 */
#define I2C_SLVCTL_SLVDMA(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_SLVCTL_SLVDMA_SHIFT)) & I2C_SLVCTL_SLVDMA_MASK)
#define I2C_SLVCTL_AUTOACK_MASK                  (0x100U)
#define I2C_SLVCTL_AUTOACK_SHIFT                 (8U)
/*! AUTOACK - Automatic Acknowledge.When this bit is set, it will cause an I2C header which matches
 *    SLVADR0 and the direction set by AUTOMATCHREAD to be ACKed immediately; this is used with DMA
 *    to allow processing of the data without intervention. If this bit is clear and a header
 *    matches SLVADR0, the behavior is controlled by AUTONACK in the SLVADR0 register: allowing NACK or
 *    interrupt. 0: Normal, non-automatic operation. If AUTONACK = 0, an SlvPending interrupt is
 *    generated when a matching address is received. If AUTONACK-1, receiver addresses are NACKed
 *    (ignored). 1: A header with matching SLVADR0 and matching direction as set by AUTOMATCHREAD will be
 *    ACKed immediately, allowing the master to move on to the data bytes. If the address matches
 *    SLVADR0, but the direction does not match AUTOMATCHREAD, the behaviour will depend on the
 *    AUTONACK bit in the SLVADR0 register: if AUTONACK is set, then it will be Nacked; else if AUTONACK is
 *    cl;ear, then a SlvPending interrupt is geernated.
 */
#define I2C_SLVCTL_AUTOACK(x)                    (((uint32_t)(((uint32_t)(x)) << I2C_SLVCTL_AUTOACK_SHIFT)) & I2C_SLVCTL_AUTOACK_MASK)
#define I2C_SLVCTL_AUTOMATCHREAD_MASK            (0x200U)
#define I2C_SLVCTL_AUTOMATCHREAD_SHIFT           (9U)
/*! AUTOMATCHREAD - When AUTOACK is set, this bit controls whether it matches a read or write
 *    request on the next header with an address matching SLVADR0. Since DMA needs to be configured to
 *    match the transfer direction, the direction needs to be specified. This bit allows a direction to
 *    be chosen for the next operation. 0: The expected next operation in Automatic Mode is an I2C
 *    write. 1: The expected next operation in Automatic Mode is an I2C read.
 */
#define I2C_SLVCTL_AUTOMATCHREAD(x)              (((uint32_t)(((uint32_t)(x)) << I2C_SLVCTL_AUTOMATCHREAD_SHIFT)) & I2C_SLVCTL_AUTOMATCHREAD_MASK)
/*! @} */

/*! @name SLVDAT - Combined Slave receiver and transmitter data register. */
/*! @{ */
#define I2C_SLVDAT_DATA_MASK                     (0xFFU)
#define I2C_SLVDAT_DATA_SHIFT                    (0U)
/*! DATA - Slave function data register. Read: read the most recently received data for the Slave
 *    function. Write: transmit data using the Slave function.
 */
#define I2C_SLVDAT_DATA(x)                       (((uint32_t)(((uint32_t)(x)) << I2C_SLVDAT_DATA_SHIFT)) & I2C_SLVDAT_DATA_MASK)
/*! @} */

/*! @name SLVADR - Slave address 0. */
/*! @{ */
#define I2C_SLVADR_SADISABLE_MASK                (0x1U)
#define I2C_SLVADR_SADISABLE_SHIFT               (0U)
/*! SADISABLE - Slave Address 0 Disable. 0: Slave Address 0 is enabled. 1: Slave Address 0 is ignored.
 */
#define I2C_SLVADR_SADISABLE(x)                  (((uint32_t)(((uint32_t)(x)) << I2C_SLVADR_SADISABLE_SHIFT)) & I2C_SLVADR_SADISABLE_MASK)
#define I2C_SLVADR_SLVADR_MASK                   (0xFEU)
#define I2C_SLVADR_SLVADR_SHIFT                  (1U)
/*! SLVADR - Slave Address. Seven bit slave address that is compared to received addresses if
 *    enabled. The compare can be affected by the setting of the SLVQUAL0 register.
 */
#define I2C_SLVADR_SLVADR(x)                     (((uint32_t)(((uint32_t)(x)) << I2C_SLVADR_SLVADR_SHIFT)) & I2C_SLVADR_SLVADR_MASK)
#define I2C_SLVADR_AUTONACK_MASK                 (0x8000U)
#define I2C_SLVADR_AUTONACK_SHIFT                (15U)
/*! AUTONACK - Automatic NACK operation. Used in conjunction with AUTOACK and AUTOMATCHREAD, allows
 *    software to ignore I2C traffic while handling previous I2C data or other operations. 0: Normal
 *    operation, matching I2C addresses are not ignored. 1: Automatic-only mode. If AUTOACK is not
 *    set, all incoming I2C addresses are ignored.
 */
#define I2C_SLVADR_AUTONACK(x)                   (((uint32_t)(((uint32_t)(x)) << I2C_SLVADR_AUTONACK_SHIFT)) & I2C_SLVADR_AUTONACK_MASK)
/*! @} */

/* The count of I2C_SLVADR */
#define I2C_SLVADR_COUNT                         (4U)

/*! @name SLVQUAL0 - Slave Qualification for address 0. */
/*! @{ */
#define I2C_SLVQUAL0_QUALMODE0_MASK              (0x1U)
#define I2C_SLVQUAL0_QUALMODE0_SHIFT             (0U)
/*! QUALMODE0 - Qualify mode for slave address 0. 0: Mask. The SLVQUAL0 field is used as a logical
 *    mask for matching address 0. 1: Extend. The SLVQAL0 field is used to extend address 0 matching
 *    in a range of addresses.
 */
#define I2C_SLVQUAL0_QUALMODE0(x)                (((uint32_t)(((uint32_t)(x)) << I2C_SLVQUAL0_QUALMODE0_SHIFT)) & I2C_SLVQUAL0_QUALMODE0_MASK)
#define I2C_SLVQUAL0_SLVQUAL0_MASK               (0xFEU)
#define I2C_SLVQUAL0_SLVQUAL0_SHIFT              (1U)
/*! SLVQUAL0 - Slave address Qualifier for address 0. A value of 0 causes the address in SLVADR0 to
 *    be used as-is, assuming that it is enabled. If QUALMODE0 = 0, any bit in this field which is
 *    set to 1 will cause an automatic match of the corresponding bit of the received address when it
 *    is compared to the SLVADR0 register. If QUALMODE0 = 1, an address range is matched for
 *    address 0. This range extends from the value defined by SLVADR0 to the address defined by SLVQUAL0
 *    (address matches when SLVADR0[7:1] received address SLVQUAL0[7:1]).
 */
#define I2C_SLVQUAL0_SLVQUAL0(x)                 (((uint32_t)(((uint32_t)(x)) << I2C_SLVQUAL0_SLVQUAL0_SHIFT)) & I2C_SLVQUAL0_SLVQUAL0_MASK)
/*! @} */

/*! @name MONRXDAT - Monitor receiver data register. */
/*! @{ */
#define I2C_MONRXDAT_MONRXDAT_MASK               (0xFFU)
#define I2C_MONRXDAT_MONRXDAT_SHIFT              (0U)
/*! MONRXDAT - Monitor function Receiver Data. This reflects every data byte that passes on the I2C pins.
 */
#define I2C_MONRXDAT_MONRXDAT(x)                 (((uint32_t)(((uint32_t)(x)) << I2C_MONRXDAT_MONRXDAT_SHIFT)) & I2C_MONRXDAT_MONRXDAT_MASK)
#define I2C_MONRXDAT_MONSTART_MASK               (0x100U)
#define I2C_MONRXDAT_MONSTART_SHIFT              (8U)
/*! MONSTART - Monitor Received Start. 0: No start detected. The monitor function has not detected a
 *    Start event on the I2C bus. 1: Start detected. The Monitor function has detected a Start
 *    event on the I2C bus.
 */
#define I2C_MONRXDAT_MONSTART(x)                 (((uint32_t)(((uint32_t)(x)) << I2C_MONRXDAT_MONSTART_SHIFT)) & I2C_MONRXDAT_MONSTART_MASK)
#define I2C_MONRXDAT_MONRESTART_MASK             (0x200U)
#define I2C_MONRXDAT_MONRESTART_SHIFT            (9U)
/*! MONRESTART - Monitor Received Repeated Start. 0: No repeated start detected. The Monitor
 *    function has not detected a Repeated Start event on the I2C bus. 1: Repeate start detected. The
 *    Monitor function has detected a Repeated Start event on the I2C bus.
 */
#define I2C_MONRXDAT_MONRESTART(x)               (((uint32_t)(((uint32_t)(x)) << I2C_MONRXDAT_MONRESTART_SHIFT)) & I2C_MONRXDAT_MONRESTART_MASK)
#define I2C_MONRXDAT_MONNACK_MASK                (0x400U)
#define I2C_MONRXDAT_MONNACK_SHIFT               (10U)
/*! MONNACK - Monitor Received NACK. 0: Acknowledged. The data currently being provided by the
 *    Monitor function was acknowledged by at least one master or slave recevier. 1: Not Acknowledged.
 *    The data currently being provided by the Monitor function was not acknowledged by any receiver.
 */
#define I2C_MONRXDAT_MONNACK(x)                  (((uint32_t)(((uint32_t)(x)) << I2C_MONRXDAT_MONNACK_SHIFT)) & I2C_MONRXDAT_MONNACK_MASK)
/*! @} */

/*! @name ID - I2C Module Identifier */
/*! @{ */
#define I2C_ID_APERTURE_MASK                     (0xFFU)
#define I2C_ID_APERTURE_SHIFT                    (0U)
/*! APERTURE - Aperture i.e. number minus 1 of consecutive packets 4 Kbytes reserved for this IP
 */
#define I2C_ID_APERTURE(x)                       (((uint32_t)(((uint32_t)(x)) << I2C_ID_APERTURE_SHIFT)) & I2C_ID_APERTURE_MASK)
#define I2C_ID_MIN_REV_MASK                      (0xF00U)
#define I2C_ID_MIN_REV_SHIFT                     (8U)
/*! MIN_REV - Minor revision i.e. with no software consequences
 */
#define I2C_ID_MIN_REV(x)                        (((uint32_t)(((uint32_t)(x)) << I2C_ID_MIN_REV_SHIFT)) & I2C_ID_MIN_REV_MASK)
#define I2C_ID_MAJ_REV_MASK                      (0xF000U)
#define I2C_ID_MAJ_REV_SHIFT                     (12U)
/*! MAJ_REV - Major revision i.e. there may be software incompatability between major revisions.
 */
#define I2C_ID_MAJ_REV(x)                        (((uint32_t)(((uint32_t)(x)) << I2C_ID_MAJ_REV_SHIFT)) & I2C_ID_MAJ_REV_MASK)
#define I2C_ID_ID_MASK                           (0xFFFF0000U)
#define I2C_ID_ID_SHIFT                          (16U)
/*! ID - Identifier. This is the unique identifier of the module
 */
#define I2C_ID_ID(x)                             (((uint32_t)(((uint32_t)(x)) << I2C_ID_ID_SHIFT)) & I2C_ID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group I2C_Register_Masks */


/* I2C - Peripheral instance base addresses */
/** Peripheral I2C0 base address */
#define I2C0_BASE                                (0x40003000u)
/** Peripheral I2C0 base pointer */
#define I2C0                                     ((I2C_Type *)I2C0_BASE)
/** Peripheral I2C1 base address */
#define I2C1_BASE                                (0x40004000u)
/** Peripheral I2C1 base pointer */
#define I2C1                                     ((I2C_Type *)I2C1_BASE)
/** Peripheral I2C2 base address */
#define I2C2_BASE                                (0x40005000u)
/** Peripheral I2C2 base pointer */
#define I2C2                                     ((I2C_Type *)I2C2_BASE)
/** Array initializer of I2C peripheral base addresses */
#define I2C_BASE_ADDRS                           { I2C0_BASE, I2C1_BASE, I2C2_BASE }
/** Array initializer of I2C peripheral base pointers */
#define I2C_BASE_PTRS                            { I2C0, I2C1, I2C2 }
/** Interrupt vectors for the I2C peripheral type */
#define I2C_IRQS                                 { FLEXCOMM2_IRQn, FLEXCOMM3_IRQn, FLEXCOMM6_IRQn }

/*!
 * @}
 */ /* end of group I2C_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- INPUTMUX Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup INPUTMUX_Peripheral_Access_Layer INPUTMUX Peripheral Access Layer
 * @{
 */

/** INPUTMUX - Register Layout Typedef */
typedef struct {
       uint8_t RESERVED_0[192];
  __IO uint32_t PINTSEL[8];                        /**< Pin interrupt select register, array offset: 0xC0, array step: 0x4 */
  __IO uint32_t DMA_ITRIG_INMUX[19];               /**< Trigger select register for DMA channel. Configurable for each of the DMA channels., array offset: 0xE0, array step: 0x4 */
       uint8_t RESERVED_1[52];
  __IO uint32_t DMA_OTRIG_INMUX[4];                /**< DMA output trigger selection to become an input to the DMA trigger mux. Four selections can be made., array offset: 0x160, array step: 0x4 */
       uint8_t RESERVED_2[16];
  __IO uint32_t FREQMEAS_REF;                      /**< Selection for frequency measurement reference clock, offset: 0x180 */
  __IO uint32_t FREQMEAS_TARGET;                   /**< Selection for frequency measurement target clock, offset: 0x184 */
} INPUTMUX_Type;

/* ----------------------------------------------------------------------------
   -- INPUTMUX Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup INPUTMUX_Register_Masks INPUTMUX Register Masks
 * @{
 */

/*! @name PINTSEL - Pin interrupt select register */
/*! @{ */
#define INPUTMUX_PINTSEL_INTPIN_MASK             (0x1FU)
#define INPUTMUX_PINTSEL_INTPIN_SHIFT            (0U)
/*! INTPIN - Pin number select for pin interrupt or pattern match engine input.
 */
#define INPUTMUX_PINTSEL_INTPIN(x)               (((uint32_t)(((uint32_t)(x)) << INPUTMUX_PINTSEL_INTPIN_SHIFT)) & INPUTMUX_PINTSEL_INTPIN_MASK)
/*! @} */

/* The count of INPUTMUX_PINTSEL */
#define INPUTMUX_PINTSEL_COUNT                   (8U)

/*! @name DMA_ITRIG_INMUX - Trigger select register for DMA channel. Configurable for each of the DMA channels. */
/*! @{ */
#define INPUTMUX_DMA_ITRIG_INMUX_INP_MASK        (0x1FU)
#define INPUTMUX_DMA_ITRIG_INMUX_INP_SHIFT       (0U)
/*! INP - Trigger input number (decimal value) for DMA channel n (n = 0 to 17). 0: ADC0 Sequence A
 *    interrupt; 1: Reserved; 2: Timer CT32B0 Match 0; 3: Timer CT32B0 Match 1; 4: Timer CT32B1 Match
 *    0; 5: Timer CT32B1 Match 1; 6: Pin interrupt 0; 7: Pin interrupt 1; 8: Pin interrupt 2; 9:
 *    Pin interrupt 3; 10: AES RX; 11: AES TX; 12: Hash RX; 13: Hash TX; 14: DMA output trigger mux 0;
 *    15: DMA output trigger mux 1; 16: DMA output trigger mux 2; 17: DMA output trigger mux 3; 18-
 *    31: reserved.
 */
#define INPUTMUX_DMA_ITRIG_INMUX_INP(x)          (((uint32_t)(((uint32_t)(x)) << INPUTMUX_DMA_ITRIG_INMUX_INP_SHIFT)) & INPUTMUX_DMA_ITRIG_INMUX_INP_MASK)
/*! @} */

/* The count of INPUTMUX_DMA_ITRIG_INMUX */
#define INPUTMUX_DMA_ITRIG_INMUX_COUNT           (19U)

/*! @name DMA_OTRIG_INMUX - DMA output trigger selection to become an input to the DMA trigger mux. Four selections can be made. */
/*! @{ */
#define INPUTMUX_DMA_OTRIG_INMUX_INP_MASK        (0x1FU)
#define INPUTMUX_DMA_OTRIG_INMUX_INP_SHIFT       (0U)
/*! INP - DMA trigger output number (decimal value) for DMA channel n (n = 0 to 19).
 */
#define INPUTMUX_DMA_OTRIG_INMUX_INP(x)          (((uint32_t)(((uint32_t)(x)) << INPUTMUX_DMA_OTRIG_INMUX_INP_SHIFT)) & INPUTMUX_DMA_OTRIG_INMUX_INP_MASK)
/*! @} */

/* The count of INPUTMUX_DMA_OTRIG_INMUX */
#define INPUTMUX_DMA_OTRIG_INMUX_COUNT           (4U)

/*! @name FREQMEAS_REF - Selection for frequency measurement reference clock */
/*! @{ */
#define INPUTMUX_FREQMEAS_REF_CLKIN_MASK         (0xFU)
#define INPUTMUX_FREQMEAS_REF_CLKIN_SHIFT        (0U)
/*! CLKIN - Clock source number (decimal value) for frequency measure function ref clock: 0: CLK_IN
 *    (must be enabled in functional mux); 1: XTAL 32 MHz (must be enabled in clock_ctrl); 2: FRO 1
 *    MHz (must be enabled in clock_ctrl); 3: 32 kHz oscillator (either FRO 32 KHz or XTAL 32 KHZ);
 *    4: Main clock (divided); 5: PIO[4] (must be configured as GPIO); 6: PIO[20] (must be
 *    configured as GPIO); 7: PIO[16] (must be configured as GPIO); 8: PIO[15] (must be configured as GPIO);
 *    9 - 15: reserved.
 */
#define INPUTMUX_FREQMEAS_REF_CLKIN(x)           (((uint32_t)(((uint32_t)(x)) << INPUTMUX_FREQMEAS_REF_CLKIN_SHIFT)) & INPUTMUX_FREQMEAS_REF_CLKIN_MASK)
/*! @} */

/*! @name FREQMEAS_TARGET - Selection for frequency measurement target clock */
/*! @{ */
#define INPUTMUX_FREQMEAS_TARGET_CLKIN_MASK      (0xFU)
#define INPUTMUX_FREQMEAS_TARGET_CLKIN_SHIFT     (0U)
/*! CLKIN - Clock source number (decimal value) for frequency measure function target clock: 0:
 *    CLK_IN (must be enabled in functional mux); 1: XTAL 32 MHz (must be enabled in clock_ctrl); 2: FRO
 *    1 MHz (must be enabled in clock_ctrl); 3: 32 kHz oscillator (either FRO 32 KHz or XTAL 32
 *    KHZ); 4: Main clock (divided); 5: PIO[4] (must be configured as GPIO); 6: PIO[20] (must be
 *    configured as GPIO); 7: PIO[16] (must be configured as GPIO); 8: PIO[15] (must be configured as
 *    GPIO); 9 - 15: reserved.
 */
#define INPUTMUX_FREQMEAS_TARGET_CLKIN(x)        (((uint32_t)(((uint32_t)(x)) << INPUTMUX_FREQMEAS_TARGET_CLKIN_SHIFT)) & INPUTMUX_FREQMEAS_TARGET_CLKIN_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group INPUTMUX_Register_Masks */


/* INPUTMUX - Peripheral instance base addresses */
/** Peripheral INPUTMUX base address */
#define INPUTMUX_BASE                            (0x4000E000u)
/** Peripheral INPUTMUX base pointer */
#define INPUTMUX                                 ((INPUTMUX_Type *)INPUTMUX_BASE)
/** Array initializer of INPUTMUX peripheral base addresses */
#define INPUTMUX_BASE_ADDRS                      { INPUTMUX_BASE }
/** Array initializer of INPUTMUX peripheral base pointers */
#define INPUTMUX_BASE_PTRS                       { INPUTMUX }

/*!
 * @}
 */ /* end of group INPUTMUX_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- IOCON Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup IOCON_Peripheral_Access_Layer IOCON Peripheral Access Layer
 * @{
 */

/** IOCON - Register Layout Typedef */
typedef struct {
  __IO uint32_t PIO[1][22];                        /**< Configuration array for PIO0 to PIO21. PIO[10] and PIO[11] use a different IO cell type to the other PIO pins and so there are some differences in the bit field descriptions of the PIO register for these Ios. Reset values vary depending on whether the IO is configured with a pull-up or pull-down resistor as default. The value is also affected by the IO type. Reset value 0x180 for PIO 0,3,4,5,8,9,12,13,14,15,16,21. Reset value 0x198 for PIO 1,2,6,7,17,18,19,20. Reset value 0x188 for PIO 10,11., array offset: 0x0, array step: index*0x58, index2*0x4 */
} IOCON_Type;

/* ----------------------------------------------------------------------------
   -- IOCON Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup IOCON_Register_Masks IOCON Register Masks
 * @{
 */

/*! @name PIO - Configuration array for PIO0 to PIO21. PIO[10] and PIO[11] use a different IO cell type to the other PIO pins and so there are some differences in the bit field descriptions of the PIO register for these Ios. Reset values vary depending on whether the IO is configured with a pull-up or pull-down resistor as default. The value is also affected by the IO type. Reset value 0x180 for PIO 0,3,4,5,8,9,12,13,14,15,16,21. Reset value 0x198 for PIO 1,2,6,7,17,18,19,20. Reset value 0x188 for PIO 10,11. */
/*! @{ */
#define IOCON_PIO_FUNC_MASK                      (0x7U)
#define IOCON_PIO_FUNC_SHIFT                     (0U)
/*! FUNC - Select digital function assigned to this pin.
 *  0b000..Alternative connection 0.
 *  0b001..Alternative connection 1.
 *  0b010..Alternative connection 2.
 *  0b011..Alternative connection 3.
 *  0b100..Alternative connection 4.
 *  0b101..Alternative connection 5.
 *  0b110..Alternative connection 6.
 *  0b111..Alternative connection 7.
 */
#define IOCON_PIO_FUNC(x)                        (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_FUNC_SHIFT)) & IOCON_PIO_FUNC_MASK)
#define IOCON_PIO_EGP_MASK                       (0x8U)
#define IOCON_PIO_EGP_SHIFT                      (3U)
/*! EGP - GPIO Mode of IO Cell.
 *  0b0..IIC mode.
 *  0b1..GPIO mode.
 */
#define IOCON_PIO_EGP(x)                         (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_EGP_SHIFT)) & IOCON_PIO_EGP_MASK)
#define IOCON_PIO_MODE_MASK                      (0x18U)
#define IOCON_PIO_MODE_SHIFT                     (3U)
/*! MODE - Select function mode (on-chip pull-up/pull-down resistor control). For MFIO type ONLY
 *    (all PIOs except PIO10 & 11): 0x0 : Pull-up. Pull-up resistor enabled. 0x1 : Repeater mode (bus
 *    keeper) 0x2 : Plain Input 0x3 : Pull-down. Pull-down resistor enabled. Note: When the register
 *    is related to a general purpose MFIO type pad (that is all PIOs except PIO10 & 11) - Bit [3]
 *    (of the register) is connected to EPD (enable pull-down) input of the MFIO pad. - Bit [4] (of
 *    the register) is connected to EPUN (enable pull-up NOT) input of MFIO pad.
 *  0b00..Pull-up. Pull-up resistor enabled.
 *  0b01..Repeater. Repeater mode (bus keeper).
 *  0b10..Inactive. Plain Input.
 *  0b11..Pull-down. Pull-down resistor enabled.
 */
#define IOCON_PIO_MODE(x)                        (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_MODE_SHIFT)) & IOCON_PIO_MODE_MASK)
#define IOCON_PIO_ECS_MASK                       (0x10U)
#define IOCON_PIO_ECS_SHIFT                      (4U)
/*! ECS - Pull-up current source enable when set. When IO is is IIC mode (EGP=0) and ECS is low, the
 *    IO cell is an open drain cell.
 *  0b0..Pull-up current source disabled.
 *  0b1..Pull-up current source enabled.
 */
#define IOCON_PIO_ECS(x)                         (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_ECS_SHIFT)) & IOCON_PIO_ECS_MASK)
#define IOCON_PIO_EHS_MASK                       (0x20U)
#define IOCON_PIO_EHS_SHIFT                      (5U)
/*! EHS - Speed selection. When IO is in GPIO mode set 1 for high speed GPIO, 0 for low speed GPIO.
 *    For IIC mode, this bit has no effect and the IO is always in low speed.
 *  0b0..low speed for GPIO mode or i2c mode.
 *  0b1..High speed for GPIO mode.
 */
#define IOCON_PIO_EHS(x)                         (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_EHS_SHIFT)) & IOCON_PIO_EHS_MASK)
#define IOCON_PIO_SLEW0_MASK                     (0x20U)
#define IOCON_PIO_SLEW0_SHIFT                    (5U)
/*! SLEW0 - This bit field is used in combination with SLEW1. The higher [SLEW1,SLEW0] the quicker the IO cell slew rate.
 *  0b0..Driver slew0 rate is disabled.
 *  0b1..Driver slew0 rate is enabled.
 */
#define IOCON_PIO_SLEW0(x)                       (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_SLEW0_SHIFT)) & IOCON_PIO_SLEW0_MASK)
#define IOCON_PIO_INVERT_MASK                    (0x40U)
#define IOCON_PIO_INVERT_SHIFT                   (6U)
/*! INVERT - Input Polarity.
 *  0b0..Disabled. Input function is not inverted.
 *  0b1..Enabled. Input function is inverted.
 */
#define IOCON_PIO_INVERT(x)                      (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_INVERT_SHIFT)) & IOCON_PIO_INVERT_MASK)
#define IOCON_PIO_DIGIMODE_MASK                  (0x80U)
#define IOCON_PIO_DIGIMODE_SHIFT                 (7U)
/*! DIGIMODE - Select Analog/Digital Mode. 0 Analog mode. 1 Digital mode. When in analog mode, the
 *    receiver path in the IO cell is disabled. In this mode, it is essential that the digital
 *    function (e.g. GPIO) is not configured as an output. Otherwise it may conflict with analog stuff
 *    (loopback of digital on analog input). In other words, the digital output is not automatically
 *    disabled when the IO is in analog mode. As a consequence, it is not possible to disable the
 *    receiver path when the IO is used for digital output purpose.
 *  0b0..Analog mode, digital input is disabled.
 *  0b1..Digital mode, digital input is enabled.
 */
#define IOCON_PIO_DIGIMODE(x)                    (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_DIGIMODE_SHIFT)) & IOCON_PIO_DIGIMODE_MASK)
#define IOCON_PIO_FILTEROFF_MASK                 (0x100U)
#define IOCON_PIO_FILTEROFF_SHIFT                (8U)
/*! FILTEROFF - Controls Input Glitch Filter.
 *  0b0..Filter enabled. Noise pulses below approximately 1 ns are filtered out.
 *  0b1..Filter disabled. No input filtering is done.
 */
#define IOCON_PIO_FILTEROFF(x)                   (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_FILTEROFF_SHIFT)) & IOCON_PIO_FILTEROFF_MASK)
#define IOCON_PIO_FSEL_MASK                      (0x200U)
#define IOCON_PIO_FSEL_SHIFT                     (9U)
/*! FSEL - Control Input Glitch Filter.
 *  0b0..Noise pulses below approximately 50ns are filtered out.
 *  0b1..Noise pulses below approximately 10 ns are filtered out. If IO is in GPIO mode this control bit is irrelevant, a 3 ns filter is used.
 */
#define IOCON_PIO_FSEL(x)                        (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_FSEL_SHIFT)) & IOCON_PIO_FSEL_MASK)
#define IOCON_PIO_SLEW1_MASK                     (0x200U)
#define IOCON_PIO_SLEW1_SHIFT                    (9U)
/*! SLEW1 - Driver Slew Rate. This bit is used in combination with SLEW0. The higher [SLEW1,SLEW0], the quicker the slew rate.
 *  0b0..Driver slew1 rate is disabled.
 *  0b1..Driver slew1 rate is enabled.
 */
#define IOCON_PIO_SLEW1(x)                       (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_SLEW1_SHIFT)) & IOCON_PIO_SLEW1_MASK)
#define IOCON_PIO_OD_MASK                        (0x400U)
#define IOCON_PIO_OD_SHIFT                       (10U)
/*! OD - Controls open-drain mode.
 *  0b0..Normal. Normal push-pull output
 *  0b1..Open-drain. Simulated open-drain output (high drive disabled).
 */
#define IOCON_PIO_OD(x)                          (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_OD_SHIFT)) & IOCON_PIO_OD_MASK)
#define IOCON_PIO_SSEL_MASK                      (0x800U)
#define IOCON_PIO_SSEL_SHIFT                     (11U)
/*! SSEL - IO Clamping Function
 *  0b0..This bit controls the IO clamping function is disabled.
 *  0b1..This bit controls the IO clamping function is enabled.
 */
#define IOCON_PIO_SSEL(x)                        (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_SSEL_SHIFT)) & IOCON_PIO_SSEL_MASK)
#define IOCON_PIO_IO_CLAMP_MASK                  (0x1000U)
#define IOCON_PIO_IO_CLAMP_SHIFT                 (12U)
/*! IO_CLAMP - Assert to freeze the IO. Also needs SYSCON:RETENTIONCTRL set as well. Useful in power
 *    down mode. This mode is held through power down cycle. Before releasing this mode on a
 *    wake-up, ensure the IO is set to the required direction and value using GPIO DIR and PIN registers.
 *  0b0..IO_CLAMP is disabled.
 *  0b1..IO_CLAMP is enabled.
 */
#define IOCON_PIO_IO_CLAMP(x)                    (((uint32_t)(((uint32_t)(x)) << IOCON_PIO_IO_CLAMP_SHIFT)) & IOCON_PIO_IO_CLAMP_MASK)
/*! @} */

/* The count of IOCON_PIO */
#define IOCON_PIO_COUNT                          (1U)

/* The count of IOCON_PIO */
#define IOCON_PIO_COUNT2                         (22U)


/*!
 * @}
 */ /* end of group IOCON_Register_Masks */


/* IOCON - Peripheral instance base addresses */
/** Peripheral IOCON base address */
#define IOCON_BASE                               (0x4000F000u)
/** Peripheral IOCON base pointer */
#define IOCON                                    ((IOCON_Type *)IOCON_BASE)
/** Array initializer of IOCON peripheral base addresses */
#define IOCON_BASE_ADDRS                         { IOCON_BASE }
/** Array initializer of IOCON peripheral base pointers */
#define IOCON_BASE_PTRS                          { IOCON }

/*!
 * @}
 */ /* end of group IOCON_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- ISO7816 Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ISO7816_Peripheral_Access_Layer ISO7816 Peripheral Access Layer
 * @{
 */

/** ISO7816 - Register Layout Typedef */
typedef struct {
  __IO uint32_t SSR;                               /**< Slot Select Register, offset: 0x0 */
  __IO uint32_t PDR1_LSB;                          /**< Programmable Divider Register (LSB) slot 1. Least significant byte of a 16-bit counter defining the ETU. The ETU counter counts a number of cycles of the Contact Interface clock, this defines the ETU. The minimum acceptable value is 0001 0000b., offset: 0x4 */
  __IO uint32_t PDR1_MSB;                          /**< Programmable Divider Register (MSB) slot 1. Most significant byte of a 16-bit counter defining the ETU. The ETU counter counts a number of cycles of the Contact Interface clock, this defines the ETU, offset: 0x8 */
  __IO uint32_t FCR;                               /**< FIFO Control Register, offset: 0xC */
  __IO uint32_t GTR1;                              /**< Guard Time Register slot 1. Value used by the Contact UART notably in transmission mode. The Contact UART will wait this number of ETUs before transmitting the character. In protocol T=1, gtr = FFh means operation at 11 ETUs. In protocol T=0, gtr = FFh means operation at 12 ETUs., offset: 0x10 */
  __IO uint32_t UCR11;                             /**< UART Configuration Register 1 slot 1, offset: 0x14 */
  __IO uint32_t UCR21;                             /**< UART Configuration Register 2 slot 1, offset: 0x18 */
  __IO uint32_t CCR1;                              /**< Clock Configuration Register slot 1, offset: 0x1C */
  __IO uint32_t PCR;                               /**< Power Control Register, offset: 0x20 */
  __IO uint32_t ECR;                               /**< Early answer Counter register, offset: 0x24 */
  __IO uint32_t MCRL_LSB;                          /**< Mute card Counter RST Low register (LSB), offset: 0x28 */
  __IO uint32_t MCRL_MSB;                          /**< Mute card Counter RST Low register (MSB), offset: 0x2C */
  __IO uint32_t MCRH_LSB;                          /**< Mute card Counter RST High register (LSB), offset: 0x30 */
  __IO uint32_t MCRH_MSB;                          /**< Mute card Counter RST High register (MSB), offset: 0x34 */
       uint8_t RESERVED_0[4];
  __IO uint32_t URR_UTR;                           /**< UART Receive Register / UART Transmit Register, offset: 0x3C */
       uint8_t RESERVED_1[12];
  __O  uint32_t TOR1;                              /**< Time-Out Register 1, offset: 0x4C */
  __O  uint32_t TOR2;                              /**< Time-Out Register 2, offset: 0x50 */
  __O  uint32_t TOR3;                              /**< Time-Out Register 3, offset: 0x54 */
  __IO uint32_t TOC;                               /**< Time-Out Configuration register, offset: 0x58 */
  __I  uint32_t FSR;                               /**< FIFO Status Register, offset: 0x5C */
  __I  uint32_t MSR;                               /**< Mixed Status Register, offset: 0x60 */
  __I  uint32_t USR1;                              /**< UART Status Register 1, offset: 0x64 */
  __I  uint32_t USR2;                              /**< UART Status Register 2, offset: 0x68 */
} ISO7816_Type;

/* ----------------------------------------------------------------------------
   -- ISO7816 Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup ISO7816_Register_Masks ISO7816 Register Masks
 * @{
 */

/*! @name SSR - Slot Select Register */
/*! @{ */
#define ISO7816_SSR_SOFTRESETN_MASK              (0x1U)
#define ISO7816_SSR_SOFTRESETN_SHIFT             (0U)
/*! SOFTRESETN - When set to logic 0 this bit resets the whole Contact UART (software reset), sets
 *    to logic 1 automatically by hardware after after one clock cycle if slot 1 is not activated
 *    else after one clock cycle after slot 1 has been automatically deactivated. Software should check
 *    soft reset is finished by reading SSR register before any further action.
 */
#define ISO7816_SSR_SOFTRESETN(x)                (((uint32_t)(((uint32_t)(x)) << ISO7816_SSR_SOFTRESETN_SHIFT)) & ISO7816_SSR_SOFTRESETN_MASK)
#define ISO7816_SSR_SEQ_EN_MASK                  (0x2U)
#define ISO7816_SSR_SEQ_EN_SHIFT                 (1U)
/*! SEQ_EN - Set this bit to enable the sequencer. If this field is 0b, the sequencer will not respond to the Start control bit.
 */
#define ISO7816_SSR_SEQ_EN(x)                    (((uint32_t)(((uint32_t)(x)) << ISO7816_SSR_SEQ_EN_SHIFT)) & ISO7816_SSR_SEQ_EN_MASK)
/*! @} */

/*! @name PDR1_LSB - Programmable Divider Register (LSB) slot 1. Least significant byte of a 16-bit counter defining the ETU. The ETU counter counts a number of cycles of the Contact Interface clock, this defines the ETU. The minimum acceptable value is 0001 0000b. */
/*! @{ */
#define ISO7816_PDR1_LSB_PDR1_LSB_MASK           (0xFFFFFFFFU)
#define ISO7816_PDR1_LSB_PDR1_LSB_SHIFT          (0U)
/*! PDR1_LSB - Programmable Divider Register (LSB) slot 1. Least significant byte of a 16-bit
 *    counter defining the ETU. The ETU counter counts a number of cycles of the Contact Interface clock,
 *    this defines the ETU. The minimum acceptable value is 0001 0000b.
 */
#define ISO7816_PDR1_LSB_PDR1_LSB(x)             (((uint32_t)(((uint32_t)(x)) << ISO7816_PDR1_LSB_PDR1_LSB_SHIFT)) & ISO7816_PDR1_LSB_PDR1_LSB_MASK)
/*! @} */

/*! @name PDR1_MSB - Programmable Divider Register (MSB) slot 1. Most significant byte of a 16-bit counter defining the ETU. The ETU counter counts a number of cycles of the Contact Interface clock, this defines the ETU */
/*! @{ */
#define ISO7816_PDR1_MSB_PDR1_MSB_MASK           (0xFFFFFFFFU)
#define ISO7816_PDR1_MSB_PDR1_MSB_SHIFT          (0U)
/*! PDR1_MSB - Programmable Divider Register (MSB) slot 1. Most significant byte of a 16-bit counter
 *    defining the ETU. The ETU counter counts a number of cycles of the Contact Interface clock,
 *    this defines the ETU
 */
#define ISO7816_PDR1_MSB_PDR1_MSB(x)             (((uint32_t)(((uint32_t)(x)) << ISO7816_PDR1_MSB_PDR1_MSB_SHIFT)) & ISO7816_PDR1_MSB_PDR1_MSB_MASK)
/*! @} */

/*! @name FCR - FIFO Control Register */
/*! @{ */
#define ISO7816_FCR_FTC_MASK                     (0x1FU)
#define ISO7816_FCR_FTC_SHIFT                    (0U)
/*! FTC - FIFO Threshold Configuration: Define the number of received or transmitted characters in
 *    the FIFO triggering the ft bit in USR1. The FIFO depth is 32 bytes. In reception mode, it
 *    enables to know that a number equals to ftc(4:0) + 1 bytes have been received. In transmission
 *    mode, ftc(4:0) equals to the number of remaining bytes into the FIFO. Be careful: in reception
 *    mode 00000 = length 1, and in transmission mode 00000 = length 0.
 */
#define ISO7816_FCR_FTC(x)                       (((uint32_t)(((uint32_t)(x)) << ISO7816_FCR_FTC_SHIFT)) & ISO7816_FCR_FTC_MASK)
#define ISO7816_FCR_PEC_MASK                     (0xE0U)
#define ISO7816_FCR_PEC_SHIFT                    (5U)
/*! PEC - Parity Error Count [For protocol T = 0] Set the number of allowed repetitions in reception
 *    or transmission mode before setting pe in ct_usr1_reg. The value 000 indicates that, if only
 *    one parity error has occurred, bit pe is set at logic 1; the value 111 indicates that bit pe
 *    will be set at logic 1 after 8 parity errors. If a correct character is received before the
 *    programmed error number is reached, the error counter will be reset. If the programmed number of
 *    allowed parity errors is reached, bit pe in register ct_usr1_reg will be set at logic 1. If a
 *    transmitted character has been naked by the card, then the Contact UART will automatically
 *    retransmit it up to a number of times equal to the value programmed in bits PEC(2:0); the
 *    character will be resent at 15 ETU. If a transmitted character is considered as correct by the card
 *    after having been naked a number of times less than the value programmed in bits PEC(2:0) +1,
 *    the error counter will be reset. If a transmitted has been naked by the card a number of times
 *    equal to the value programmed in bits PEC(2:0) +1, the transmission stops and bit pe in
 *    register ct_usr1_reg is set at logic 1. The firmware is supposed to deactivate the card. If not, the
 *    firmware has the possibility to pursue the transmission. By reading the number of bytes
 *    present into the FIFO (ffl bits), it can determine which character has been naked PEC +1 times by
 *    the card. It will then flush the FIFO (FIFO flush bit). The next step consits in unlocking the
 *    transmission using dispe bit. By writing this bit at logic level one (and then at logic level
 *    zero if the firmware still wants to check parity errors), the transmission is unlocked. The
 *    firmware can now write bytes into the FIFO. In transmission mode, if bits PEC(2:0) are at logic
 *    0, then the automatic retransmission is invalidated. There is no retransmission; the
 *    transmission continues with the next character sent at 13 ETU. [For protocol T = 1]: The error counter
 *    has no action: bit pe is set at logic 1 at the first wrong received character.
 */
#define ISO7816_FCR_PEC(x)                       (((uint32_t)(((uint32_t)(x)) << ISO7816_FCR_PEC_SHIFT)) & ISO7816_FCR_PEC_MASK)
/*! @} */

/*! @name GTR1 - Guard Time Register slot 1. Value used by the Contact UART notably in transmission mode. The Contact UART will wait this number of ETUs before transmitting the character. In protocol T=1, gtr = FFh means operation at 11 ETUs. In protocol T=0, gtr = FFh means operation at 12 ETUs. */
/*! @{ */
#define ISO7816_GTR1_GTR1_MASK                   (0xFFFFFFFFU)
#define ISO7816_GTR1_GTR1_SHIFT                  (0U)
/*! GTR1 - Guard Time Register slot 1. Value used by the Contact UART notably in transmission mode.
 *    The Contact UART will wait this number of ETUs before transmitting the character. In protocol
 *    T=1, gtr = FFh means operation at 11 ETUs. In protocol T=0, gtr = FFh means operation at 12
 *    ETUs.
 */
#define ISO7816_GTR1_GTR1(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_GTR1_GTR1_SHIFT)) & ISO7816_GTR1_GTR1_MASK)
/*! @} */

/*! @name UCR11 - UART Configuration Register 1 slot 1 */
/*! @{ */
#define ISO7816_UCR11_CONV_MASK                  (0x1U)
#define ISO7816_UCR11_CONV_SHIFT                 (0U)
/*! CONV - CONVention: Bit CONV is set to logic 1 if the convention is direct. Bit CONV is either
 *    automatically written by hardware according to the convention detected during ATR, or by
 *    software if the bit AUTOCONV in register ct_ucr1_reg is set to logic 1.
 */
#define ISO7816_UCR11_CONV(x)                    (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR11_CONV_SHIFT)) & ISO7816_UCR11_CONV_MASK)
#define ISO7816_UCR11_LCT_MASK                   (0x2U)
#define ISO7816_UCR11_LCT_SHIFT                  (1U)
/*! LCT - Last Character to Transmit: Bit LCT is set to logic 1 by software before writing the last
 *    character to be transmitted in register ct_utr_reg. It allows automatic change to reception
 *    mode. It is reset to logic 0 by hardware at the end of a successful transmission after 11.75
 *    ETUs in protocol T = 0 and after 10.75 ETUs in protocol T = 1. When bit LCT is being reset to
 *    logic 0, bit T/R is also reset to logic 0 and the UART is ready to receive a character. LCT bit
 *    can be set to logic 1 by software not only when writing the last character to be transmitted
 *    but also during the transmission or even at the beginning of the transmission. It will be taken
 *    into account when the FIFO becomes empty, which implies for the software to be able to
 *    regularly re-load the FIFO when transmitting more than 32 bytes to ensure there is at least one byte
 *    into the FIFO as long as the transmission is not finished. Else, a switch to reception mode
 *    will prematurely occur before having transmitted all the bytes.
 */
#define ISO7816_UCR11_LCT(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR11_LCT_SHIFT)) & ISO7816_UCR11_LCT_MASK)
#define ISO7816_UCR11_T_R_MASK                   (0x4U)
#define ISO7816_UCR11_T_R_SHIFT                  (2U)
/*! T_R - Transmit/Receive: Defines the mode: logic 1 means transmission and logic 0 reception. Bit
 *    T/R is set by software for transmission mode. Bit T/R is automatically reset to logic 0 by
 *    hardware, if bit LCT has been used before transmitting the last character. Note that when
 *    switching from/to reception to/from transmission mode, the FIFO is flushed. Any remaining bytes are
 *    lost.
 */
#define ISO7816_UCR11_T_R(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR11_T_R_SHIFT)) & ISO7816_UCR11_T_R_MASK)
#define ISO7816_UCR11_PROT_MASK                  (0x8U)
#define ISO7816_UCR11_PROT_SHIFT                 (3U)
/*! PROT - PROTocol: Selects the protocol: logic 1 means T=1 and logic 0 T=0.
 */
#define ISO7816_UCR11_PROT(x)                    (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR11_PROT_SHIFT)) & ISO7816_UCR11_PROT_MASK)
#define ISO7816_UCR11_FC_MASK                    (0x10U)
#define ISO7816_UCR11_FC_SHIFT                   (4U)
/*! FC - Described in a separated document.
 */
#define ISO7816_UCR11_FC(x)                      (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR11_FC_SHIFT)) & ISO7816_UCR11_FC_MASK)
#define ISO7816_UCR11_FIP_MASK                   (0x20U)
#define ISO7816_UCR11_FIP_SHIFT                  (5U)
/*! FIP - Force Inverse Parity: If bit FIP is set to logic 1, the Contact UART will NAK a correctly
 *    received character, and will transmit characters with wrong parity bits.
 */
#define ISO7816_UCR11_FIP(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR11_FIP_SHIFT)) & ISO7816_UCR11_FIP_MASK)
/*! @} */

/*! @name UCR21 - UART Configuration Register 2 slot 1 */
/*! @{ */
#define ISO7816_UCR21_AUTOCONVN_MASK             (0x1U)
#define ISO7816_UCR21_AUTOCONVN_SHIFT            (0U)
/*! AUTOCONVN - AUTOmatically detected CONVention: If bit AUTOCONV = 1, then the convention is set
 *    by software using bit CONV in register ct_ucr1_reg. If the bit is reset to logic 0, then the
 *    configuration is automatically detected on the first received character and the bit
 *    automatically set after convention detection.
 */
#define ISO7816_UCR21_AUTOCONVN(x)               (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR21_AUTOCONVN_SHIFT)) & ISO7816_UCR21_AUTOCONVN_MASK)
#define ISO7816_UCR21_MANBGT_MASK                (0x2U)
#define ISO7816_UCR21_MANBGT_SHIFT               (1U)
/*! MANBGT - MANual BGT: When set to logic 1, BGT is managed by software, else by hardware.
 */
#define ISO7816_UCR21_MANBGT(x)                  (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR21_MANBGT_SHIFT)) & ISO7816_UCR21_MANBGT_MASK)
#define ISO7816_UCR21_DISFT_MASK                 (0x4U)
#define ISO7816_UCR21_DISFT_SHIFT                (2U)
/*! DISFT - DISable Fifo Threshold interrupt bit: When set to logic 1 the bit ft in register
 *    ct_usr1_reg will not generate interrupt.
 */
#define ISO7816_UCR21_DISFT(x)                   (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR21_DISFT_SHIFT)) & ISO7816_UCR21_DISFT_MASK)
#define ISO7816_UCR21_DISPE_MASK                 (0x8U)
#define ISO7816_UCR21_DISPE_SHIFT                (3U)
/*! DISPE - DISable Parity Error interrupt bit: When set to logic 1, the parity is not checked in
 *    both reception and transmission modes, the bit pe in register ct_usr1_reg will not generate
 *    interrupt.
 */
#define ISO7816_UCR21_DISPE(x)                   (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR21_DISPE_SHIFT)) & ISO7816_UCR21_DISPE_MASK)
#define ISO7816_UCR21_DISATRCOUNTER_MASK         (0x10U)
#define ISO7816_UCR21_DISATRCOUNTER_SHIFT        (4U)
/*! DISATRCOUNTER - DISable ATR counter: [For Slot 1 only] When set to logic 1 the bits EARLY and
 *    MUTE in register ct_usr1_reg will not generate interrupt. This bit should be set before
 *    activating.
 */
#define ISO7816_UCR21_DISATRCOUNTER(x)           (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR21_DISATRCOUNTER_SHIFT)) & ISO7816_UCR21_DISATRCOUNTER_MASK)
#define ISO7816_UCR21_FIFOFLUSH_MASK             (0x40U)
#define ISO7816_UCR21_FIFOFLUSH_SHIFT            (6U)
/*! FIFOFLUSH - FIFO flush: When set to logic 1, the FIFO is flushed whatever the mode (reception or
 *    transmission) is. It can be used before any reception or transmission of characters but not
 *    while receiving or transmitting a character. It is reset to logic 0 by hardware after one
 *    clk_ip cycle.
 */
#define ISO7816_UCR21_FIFOFLUSH(x)               (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR21_FIFOFLUSH_SHIFT)) & ISO7816_UCR21_FIFOFLUSH_MASK)
#define ISO7816_UCR21_WRDACC_MASK                (0x80U)
#define ISO7816_UCR21_WRDACC_SHIFT               (7U)
/*! WRDACC - FIFO WoRD ACCess: When set to logic 1, the FIFO supports word (4 bytes) access (read
 *    and write), access failure is indicated by bit wrdaccerr in register USR2. When set to logic 0,
 *    the FIFO supports byte access (read and write).
 */
#define ISO7816_UCR21_WRDACC(x)                  (((uint32_t)(((uint32_t)(x)) << ISO7816_UCR21_WRDACC_SHIFT)) & ISO7816_UCR21_WRDACC_MASK)
/*! @} */

/*! @name CCR1 - Clock Configuration Register slot 1 */
/*! @{ */
#define ISO7816_CCR1_ACC_MASK                    (0x7U)
#define ISO7816_CCR1_ACC_SHIFT                   (0U)
/*! ACC - Asynchronous Card Clock: Defines the card clock frequency: 000: card clock frequency =
 *    fclk_ip; 001: card clock frequency = fclk_ip /2; 010: card clock frequency = fclk_ip /3; 011:
 *    card clock frequency = fclk_ip /4; 100: card clock frequency = fclk_ip /5; 101: card clock
 *    frequency = fclk_ip /6; 110: card clock frequency = fclk_ip /8; 111: card clock frequency = fclk_ip
 *    /16. All frequency changes are synchronous, thus ensuring that no spikes or unwanted pulse
 *    widths occur during changes. In conjunction with registers ct_etucr_lsb_reg and ct_etucr_msb_reg,
 *    the bits ACC2, ACC1 and ACC0 defines the baudrate used by the Contact UART.
 */
#define ISO7816_CCR1_ACC(x)                      (((uint32_t)(((uint32_t)(x)) << ISO7816_CCR1_ACC_SHIFT)) & ISO7816_CCR1_ACC_MASK)
#define ISO7816_CCR1_SAN_MASK                    (0x8U)
#define ISO7816_CCR1_SAN_SHIFT                   (3U)
/*! SAN - Synchronous/Asynchronous Card: [For Slot 1]: When set to logic 1, the Contact UART
 *    supports synchronous card. The Contact UART is then bypassed, only bit 0 of registers ct_urr_reg and
 *    ct_utr_reg is connected to pin I/O. In this case, the card clock is controlled by bit SHL and
 *    RST card is controlled by bit RSTIN in register ct_pcr_reg. When set to logic 0, the Contact
 *    UART supports asynchronous card. Dynamic change (while activated) is not supported. The choice
 *    should be done before activating the card. [For Slot AUX]: When set to logic 1, the Contact
 *    UART supports synchronous card. The Contact UART is then bypassed, only bit 0 of registers
 *    ct_urr_reg and ct_utr_reg is connected to pin I/O. In this case, the card clock is controlled by
 *    bit SHL. When set to logic 0, the Contact UART supports asynchronous card. Dynamic change (while
 *    CLKAUXen = 1) is not supported. The choice should be done before enabling CLKAUX clock.
 */
#define ISO7816_CCR1_SAN(x)                      (((uint32_t)(((uint32_t)(x)) << ISO7816_CCR1_SAN_SHIFT)) & ISO7816_CCR1_SAN_MASK)
#define ISO7816_CCR1_CST_MASK                    (0x10U)
#define ISO7816_CCR1_CST_SHIFT                   (4U)
/*! CST - Clock STop: [For Slot 1]: In the case of an asynchronous card, bit CST defines whether the
 *    clock to the card is stopped or not; if bit CST is reset to logic 0, then the clock is
 *    determined by bits ACC0, ACC1 and ACC2. [For Slot AUX]+I40: This bit is not available for the
 *    auxiliary slot (ct_ccr2_reg) since clock stop feature is supported using CLKAUXen bit in ct_ssr_reg
 *    register.
 */
#define ISO7816_CCR1_CST(x)                      (((uint32_t)(((uint32_t)(x)) << ISO7816_CCR1_CST_SHIFT)) & ISO7816_CCR1_CST_MASK)
#define ISO7816_CCR1_SHL_MASK                    (0x20U)
#define ISO7816_CCR1_SHL_SHIFT                   (5U)
/*! SHL - Stop HIGH or LOW: - Slot 1: If bits SAN = 0 and CST = 1, then the clock is stopped at LOW
 *    level. If bit SHL = 0, and at HIGH level if bit SHL = 1. I+I10f bit SAN = 1, then contact CLK
 *    is the copy of the value of bit SHL.
 */
#define ISO7816_CCR1_SHL(x)                      (((uint32_t)(((uint32_t)(x)) << ISO7816_CCR1_SHL_SHIFT)) & ISO7816_CCR1_SHL_MASK)
/*! @} */

/*! @name PCR - Power Control Register */
/*! @{ */
#define ISO7816_PCR_PCR_MASK                     (0xFFFFFFFFU)
#define ISO7816_PCR_PCR_SHIFT                    (0U)
/*! PCR - Power Control Register
 */
#define ISO7816_PCR_PCR(x)                       (((uint32_t)(((uint32_t)(x)) << ISO7816_PCR_PCR_SHIFT)) & ISO7816_PCR_PCR_MASK)
/*! @} */

/*! @name ECR - Early answer Counter register */
/*! @{ */
#define ISO7816_ECR_ECR_MASK                     (0xFFFFFFFFU)
#define ISO7816_ECR_ECR_SHIFT                    (0U)
/*! ECR - Early answer Counter register
 */
#define ISO7816_ECR_ECR(x)                       (((uint32_t)(((uint32_t)(x)) << ISO7816_ECR_ECR_SHIFT)) & ISO7816_ECR_ECR_MASK)
/*! @} */

/*! @name MCRL_LSB - Mute card Counter RST Low register (LSB) */
/*! @{ */
#define ISO7816_MCRL_LSB_MCRL_LSB_MASK           (0xFFFFFFFFU)
#define ISO7816_MCRL_LSB_MCRL_LSB_SHIFT          (0U)
/*! MCRL_LSB - Mute card Counter RST Low register (LSB)
 */
#define ISO7816_MCRL_LSB_MCRL_LSB(x)             (((uint32_t)(((uint32_t)(x)) << ISO7816_MCRL_LSB_MCRL_LSB_SHIFT)) & ISO7816_MCRL_LSB_MCRL_LSB_MASK)
/*! @} */

/*! @name MCRL_MSB - Mute card Counter RST Low register (MSB) */
/*! @{ */
#define ISO7816_MCRL_MSB_MCRL_MSB_MASK           (0xFFFFFFFFU)
#define ISO7816_MCRL_MSB_MCRL_MSB_SHIFT          (0U)
/*! MCRL_MSB - Mute card Counter RST Low register (MSB)
 */
#define ISO7816_MCRL_MSB_MCRL_MSB(x)             (((uint32_t)(((uint32_t)(x)) << ISO7816_MCRL_MSB_MCRL_MSB_SHIFT)) & ISO7816_MCRL_MSB_MCRL_MSB_MASK)
/*! @} */

/*! @name MCRH_LSB - Mute card Counter RST High register (LSB) */
/*! @{ */
#define ISO7816_MCRH_LSB_MCRH_LSB_MASK           (0xFFFFFFFFU)
#define ISO7816_MCRH_LSB_MCRH_LSB_SHIFT          (0U)
/*! MCRH_LSB - Mute card Counter RST High register (LSB)
 */
#define ISO7816_MCRH_LSB_MCRH_LSB(x)             (((uint32_t)(((uint32_t)(x)) << ISO7816_MCRH_LSB_MCRH_LSB_SHIFT)) & ISO7816_MCRH_LSB_MCRH_LSB_MASK)
/*! @} */

/*! @name MCRH_MSB - Mute card Counter RST High register (MSB) */
/*! @{ */
#define ISO7816_MCRH_MSB_MCRH_MSB_MASK           (0xFFFFFFFFU)
#define ISO7816_MCRH_MSB_MCRH_MSB_SHIFT          (0U)
/*! MCRH_MSB - Mute card Counter RST High register (MSB)
 */
#define ISO7816_MCRH_MSB_MCRH_MSB(x)             (((uint32_t)(((uint32_t)(x)) << ISO7816_MCRH_MSB_MCRH_MSB_SHIFT)) & ISO7816_MCRH_MSB_MCRH_MSB_MASK)
/*! @} */

/*! @name URR_UTR - UART Receive Register / UART Transmit Register */
/*! @{ */
#define ISO7816_URR_UTR_URR_UTR_MASK             (0xFFFFFFFFU)
#define ISO7816_URR_UTR_URR_UTR_SHIFT            (0U)
/*! URR_UTR - UART Receive Register / UART Transmit Register
 */
#define ISO7816_URR_UTR_URR_UTR(x)               (((uint32_t)(((uint32_t)(x)) << ISO7816_URR_UTR_URR_UTR_SHIFT)) & ISO7816_URR_UTR_URR_UTR_MASK)
/*! @} */

/*! @name TOR1 - Time-Out Register 1 */
/*! @{ */
#define ISO7816_TOR1_TOR1_MASK                   (0xFFFFFFFFU)
#define ISO7816_TOR1_TOR1_SHIFT                  (0U)
/*! TOR1 - Time-Out Register 1
 */
#define ISO7816_TOR1_TOR1(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_TOR1_TOR1_SHIFT)) & ISO7816_TOR1_TOR1_MASK)
/*! @} */

/*! @name TOR2 - Time-Out Register 2 */
/*! @{ */
#define ISO7816_TOR2_TOR2_MASK                   (0xFFFFFFFFU)
#define ISO7816_TOR2_TOR2_SHIFT                  (0U)
/*! TOR2 - Time-Out Register 2
 */
#define ISO7816_TOR2_TOR2(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_TOR2_TOR2_SHIFT)) & ISO7816_TOR2_TOR2_MASK)
/*! @} */

/*! @name TOR3 - Time-Out Register 3 */
/*! @{ */
#define ISO7816_TOR3_TOR3_MASK                   (0xFFFFFFFFU)
#define ISO7816_TOR3_TOR3_SHIFT                  (0U)
/*! TOR3 - Time-Out Register 3
 */
#define ISO7816_TOR3_TOR3(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_TOR3_TOR3_SHIFT)) & ISO7816_TOR3_TOR3_MASK)
/*! @} */

/*! @name TOC - Time-Out Configuration register */
/*! @{ */
#define ISO7816_TOC_TOC_MASK                     (0xFFFFFFFFU)
#define ISO7816_TOC_TOC_SHIFT                    (0U)
/*! TOC - Time-Out Configuration register
 */
#define ISO7816_TOC_TOC(x)                       (((uint32_t)(((uint32_t)(x)) << ISO7816_TOC_TOC_SHIFT)) & ISO7816_TOC_TOC_MASK)
/*! @} */

/*! @name FSR - FIFO Status Register */
/*! @{ */
#define ISO7816_FSR_FSR_MASK                     (0xFFFFFFFFU)
#define ISO7816_FSR_FSR_SHIFT                    (0U)
/*! FSR - FIFO Status Register
 */
#define ISO7816_FSR_FSR(x)                       (((uint32_t)(((uint32_t)(x)) << ISO7816_FSR_FSR_SHIFT)) & ISO7816_FSR_FSR_MASK)
/*! @} */

/*! @name MSR - Mixed Status Register */
/*! @{ */
#define ISO7816_MSR_MSR_MASK                     (0xFFFFFFFFU)
#define ISO7816_MSR_MSR_SHIFT                    (0U)
/*! MSR - Mixed Status Register
 */
#define ISO7816_MSR_MSR(x)                       (((uint32_t)(((uint32_t)(x)) << ISO7816_MSR_MSR_SHIFT)) & ISO7816_MSR_MSR_MASK)
/*! @} */

/*! @name USR1 - UART Status Register 1 */
/*! @{ */
#define ISO7816_USR1_USR1_MASK                   (0xFFFFFFFFU)
#define ISO7816_USR1_USR1_SHIFT                  (0U)
/*! USR1 - UART Status Register 1
 */
#define ISO7816_USR1_USR1(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_USR1_USR1_SHIFT)) & ISO7816_USR1_USR1_MASK)
/*! @} */

/*! @name USR2 - UART Status Register 2 */
/*! @{ */
#define ISO7816_USR2_USR2_MASK                   (0xFFFFFFFFU)
#define ISO7816_USR2_USR2_SHIFT                  (0U)
/*! USR2 - UART Status Register 2
 */
#define ISO7816_USR2_USR2(x)                     (((uint32_t)(((uint32_t)(x)) << ISO7816_USR2_USR2_SHIFT)) & ISO7816_USR2_USR2_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group ISO7816_Register_Masks */


/* ISO7816 - Peripheral instance base addresses */
/** Peripheral ISO7816 base address */
#define ISO7816_BASE                             (0x40006000u)
/** Peripheral ISO7816 base pointer */
#define ISO7816                                  ((ISO7816_Type *)ISO7816_BASE)
/** Array initializer of ISO7816 peripheral base addresses */
#define ISO7816_BASE_ADDRS                       { ISO7816_BASE }
/** Array initializer of ISO7816 peripheral base pointers */
#define ISO7816_BASE_PTRS                        { ISO7816 }

/*!
 * @}
 */ /* end of group ISO7816_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- OTPC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup OTPC_Peripheral_Access_Layer OTPC Peripheral Access Layer
 * @{
 */

/** OTPC - Register Layout Typedef */
typedef struct {
  __IO uint32_t ADDR;                              /**< Address register for reading the E-Fuse OTP, offset: 0x0 */
       uint8_t RESERVED_0[4];
  __O  uint32_t READ;                              /**< Register for reading the E-Fuse OTP., offset: 0x8 */
       uint8_t RESERVED_1[8];
  __I  uint32_t RDATA;                             /**< Register for the OTP read back data., offset: 0x14 */
} OTPC_Type;

/* ----------------------------------------------------------------------------
   -- OTPC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup OTPC_Register_Masks OTPC Register Masks
 * @{
 */

/*! @name ADDR - Address register for reading the E-Fuse OTP */
/*! @{ */
#define OTPC_ADDR_ADDR_MASK                      (0xFFFU)
#define OTPC_ADDR_ADDR_SHIFT                     (0U)
/*! ADDR - Address of OTP value to be read
 */
#define OTPC_ADDR_ADDR(x)                        (((uint32_t)(((uint32_t)(x)) << OTPC_ADDR_ADDR_SHIFT)) & OTPC_ADDR_ADDR_MASK)
/*! @} */

/*! @name READ - Register for reading the E-Fuse OTP. */
/*! @{ */
#define OTPC_READ_READ_MASK                      (0x1U)
#define OTPC_READ_READ_SHIFT                     (0U)
/*! READ - When 1 is written, the OTP is read. Note, this operation only occurs if correct SEQ value is also written.
 */
#define OTPC_READ_READ(x)                        (((uint32_t)(((uint32_t)(x)) << OTPC_READ_READ_SHIFT)) & OTPC_READ_READ_MASK)
#define OTPC_READ_SEQ_MASK                       (0xFFFF0000U)
#define OTPC_READ_SEQ_SHIFT                      (16U)
/*! SEQ - Read unlock sequence: only when 0x7F12 is written is the Read command accepted.
 */
#define OTPC_READ_SEQ(x)                         (((uint32_t)(((uint32_t)(x)) << OTPC_READ_SEQ_SHIFT)) & OTPC_READ_SEQ_MASK)
/*! @} */

/*! @name RDATA - Register for the OTP read back data. */
/*! @{ */
#define OTPC_RDATA_DATA_MASK                     (0xFFFFU)
#define OTPC_RDATA_DATA_SHIFT                    (0U)
/*! DATA - Read back data from the E-Fuse OTP.
 */
#define OTPC_RDATA_DATA(x)                       (((uint32_t)(((uint32_t)(x)) << OTPC_RDATA_DATA_SHIFT)) & OTPC_RDATA_DATA_MASK)
#define OTPC_RDATA_VALID_MASK                    (0x80000000U)
#define OTPC_RDATA_VALID_SHIFT                   (31U)
/*! VALID - Valid bit. This bit will be cleared when a Read command has been given and will be set
 *    when the sequencer has successfully captured the E-Fuse OTP data.
 */
#define OTPC_RDATA_VALID(x)                      (((uint32_t)(((uint32_t)(x)) << OTPC_RDATA_VALID_SHIFT)) & OTPC_RDATA_VALID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group OTPC_Register_Masks */


/* OTPC - Peripheral instance base addresses */
/** Peripheral OTPC base address */
#define OTPC_BASE                                (0x40002000u)
/** Peripheral OTPC base pointer */
#define OTPC                                     ((OTPC_Type *)OTPC_BASE)
/** Array initializer of OTPC peripheral base addresses */
#define OTPC_BASE_ADDRS                          { OTPC_BASE }
/** Array initializer of OTPC peripheral base pointers */
#define OTPC_BASE_PTRS                           { OTPC }

/*!
 * @}
 */ /* end of group OTPC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- PINT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PINT_Peripheral_Access_Layer PINT Peripheral Access Layer
 * @{
 */

/** PINT - Register Layout Typedef */
typedef struct {
  __IO uint32_t ISEL;                              /**< Pin Interrupt Mode register (only interrupts 0 to 3 supported to processor), offset: 0x0 */
  __IO uint32_t IENR;                              /**< Pin interrupt level or rising edge interrupt enable register (only interrupts 0 to 3 supported to processor), offset: 0x4 */
  __O  uint32_t SIENR;                             /**< Pin interrupt level or rising edge interrupt set register (only interrupts 0 to 3 supported to processor), offset: 0x8 */
  __O  uint32_t CIENR;                             /**< Pin interrupt level (rising edge interrupt) clear register (only interrupts 0 to 3 supported to processor), offset: 0xC */
  __IO uint32_t IENF;                              /**< Pin interrupt active level or falling edge interrupt enable register, offset: 0x10 */
  __O  uint32_t SIENF;                             /**< Pin interrupt active level or falling edge interrupt set register, offset: 0x14 */
  __O  uint32_t CIENF;                             /**< Pin interrupt active level or falling edge interrupt clear register, offset: 0x18 */
  __IO uint32_t RISE;                              /**< Pin interrupt rising edge register, offset: 0x1C */
  __IO uint32_t FALL;                              /**< Pin interrupt falling edge register, offset: 0x20 */
  __IO uint32_t IST;                               /**< Pin interrupt status register. For bits in this regsiter the following functionality occurs for the corresponding PIN bit: Read 0: interrupt is not being requested for this interrupt pin. Write 0: no operation. Read 1: interrupt is being requested for this interrupt pin. Write 1 (edge-sensitive): clear rising- and falling-edge detection for this pin. Write 1 (level-sensitive): switch the active level for this pin (in the IENF register)., offset: 0x24 */
  __IO uint32_t PMCTRL;                            /**< Pattern match interrupt control register, offset: 0x28 */
  __IO uint32_t PMSRC;                             /**< Pattern match interrupt bit-slice source register, offset: 0x2C */
  __IO uint32_t PMCFG;                             /**< Pattern match interrupt bit slice configuration register, offset: 0x30 */
} PINT_Type;

/* ----------------------------------------------------------------------------
   -- PINT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PINT_Register_Masks PINT Register Masks
 * @{
 */

/*! @name ISEL - Pin Interrupt Mode register (only interrupts 0 to 3 supported to processor) */
/*! @{ */
#define PINT_ISEL_PMODE_PIN0_MASK                (0x1U)
#define PINT_ISEL_PMODE_PIN0_SHIFT               (0U)
/*! PMODE_PIN0 - Selects the interrupt mode for pin interrupt 0 (selected in PINTSEL0). 0: Edge sensitive. 1: Level sensitive.
 */
#define PINT_ISEL_PMODE_PIN0(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_ISEL_PMODE_PIN0_SHIFT)) & PINT_ISEL_PMODE_PIN0_MASK)
#define PINT_ISEL_PMODE_PIN1_MASK                (0x2U)
#define PINT_ISEL_PMODE_PIN1_SHIFT               (1U)
/*! PMODE_PIN1 - Selects the interrupt mode for pin interrupt 1 (selected in PINTSEL1). 0: Edge sensitive. 1: Level sensitive.
 */
#define PINT_ISEL_PMODE_PIN1(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_ISEL_PMODE_PIN1_SHIFT)) & PINT_ISEL_PMODE_PIN1_MASK)
#define PINT_ISEL_PMODE_PIN2_MASK                (0x4U)
#define PINT_ISEL_PMODE_PIN2_SHIFT               (2U)
/*! PMODE_PIN2 - Selects the interrupt mode for pin interrupt 2 (selected in PINTSEL2). 0: Edge sensitive. 1: Level sensitive.
 */
#define PINT_ISEL_PMODE_PIN2(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_ISEL_PMODE_PIN2_SHIFT)) & PINT_ISEL_PMODE_PIN2_MASK)
#define PINT_ISEL_PMODE_PIN3_MASK                (0x8U)
#define PINT_ISEL_PMODE_PIN3_SHIFT               (3U)
/*! PMODE_PIN3 - Selects the interrupt mode for pin interrupt 3 (selected in PINTSEL3). 0: Edge sensitive. 1: Level sensitive.
 */
#define PINT_ISEL_PMODE_PIN3(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_ISEL_PMODE_PIN3_SHIFT)) & PINT_ISEL_PMODE_PIN3_MASK)
#define PINT_ISEL_PMODE_PIN4_MASK                (0x10U)
#define PINT_ISEL_PMODE_PIN4_SHIFT               (4U)
/*! PMODE_PIN4 - Selects the interrupt mode for pin interrupt 4 (selected in PINTSEL4). [Note
 *    interrupt not supported to processor] 0: Edge sensitive. 1: Level sensitive.
 */
#define PINT_ISEL_PMODE_PIN4(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_ISEL_PMODE_PIN4_SHIFT)) & PINT_ISEL_PMODE_PIN4_MASK)
#define PINT_ISEL_PMODE_PIN5_MASK                (0x20U)
#define PINT_ISEL_PMODE_PIN5_SHIFT               (5U)
/*! PMODE_PIN5 - Selects the interrupt mode for pin interrupt 5 (selected in PINTSEL5). [Note
 *    interrupt not supported to processor] 0: Edge sensitive 1: Level sensitive
 */
#define PINT_ISEL_PMODE_PIN5(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_ISEL_PMODE_PIN5_SHIFT)) & PINT_ISEL_PMODE_PIN5_MASK)
#define PINT_ISEL_PMODE_PIN6_MASK                (0x40U)
#define PINT_ISEL_PMODE_PIN6_SHIFT               (6U)
/*! PMODE_PIN6 - Selects the interrupt mode for pin interrupt 6 (selected in PINTSEL6). [Note
 *    interrupt not supported to processor] 0: Edge sensitive. 1: Level sensitive.
 */
#define PINT_ISEL_PMODE_PIN6(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_ISEL_PMODE_PIN6_SHIFT)) & PINT_ISEL_PMODE_PIN6_MASK)
#define PINT_ISEL_PMODE_PIN7_MASK                (0x80U)
#define PINT_ISEL_PMODE_PIN7_SHIFT               (7U)
/*! PMODE_PIN7 - Selects the interrupt mode for pin interrupt 7 (selected in PINTSEL7). [Note
 *    interrupt not supported to processor] 0: Edge sensitive. 1: Level sensitive.
 */
#define PINT_ISEL_PMODE_PIN7(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_ISEL_PMODE_PIN7_SHIFT)) & PINT_ISEL_PMODE_PIN7_MASK)
/*! @} */

/*! @name IENR - Pin interrupt level or rising edge interrupt enable register (only interrupts 0 to 3 supported to processor) */
/*! @{ */
#define PINT_IENR_ENRL_PIN0_MASK                 (0x1U)
#define PINT_IENR_ENRL_PIN0_SHIFT                (0U)
/*! ENRL_PIN0 - Enables the rising edge or level interrupt for pin interrupt 0 (selected in
 *    PINTSEL0). 0: Disable rising edge or level interrupt. 1: Enable rising edge or level interrupt.
 */
#define PINT_IENR_ENRL_PIN0(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENR_ENRL_PIN0_SHIFT)) & PINT_IENR_ENRL_PIN0_MASK)
#define PINT_IENR_ENRL_PIN1_MASK                 (0x2U)
#define PINT_IENR_ENRL_PIN1_SHIFT                (1U)
/*! ENRL_PIN1 - Enables the rising edge or level interrupt for pin interrupt 1 (selected in
 *    PINTSEL1). 0: Disable rising edge or level interrupt. 1: Enable rising edge or level interrupt.
 */
#define PINT_IENR_ENRL_PIN1(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENR_ENRL_PIN1_SHIFT)) & PINT_IENR_ENRL_PIN1_MASK)
#define PINT_IENR_ENRL_PIN2_MASK                 (0x4U)
#define PINT_IENR_ENRL_PIN2_SHIFT                (2U)
/*! ENRL_PIN2 - Enables the rising edge or level interrupt for pin interrupt 2 (selected in
 *    PINTSEL2). 0: Disable rising edge or level interrupt. 1: Enable rising edge or level interrupt.
 */
#define PINT_IENR_ENRL_PIN2(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENR_ENRL_PIN2_SHIFT)) & PINT_IENR_ENRL_PIN2_MASK)
#define PINT_IENR_ENRL_PIN3_MASK                 (0x8U)
#define PINT_IENR_ENRL_PIN3_SHIFT                (3U)
/*! ENRL_PIN3 - Enables the rising edge or level interrupt for pin interrupt 3 (selected in
 *    PINTSEL3). 0: Disable rising edge or level interrupt. 1: Enable rising edge or level interrupt.
 */
#define PINT_IENR_ENRL_PIN3(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENR_ENRL_PIN3_SHIFT)) & PINT_IENR_ENRL_PIN3_MASK)
#define PINT_IENR_ENRL_PIN4_MASK                 (0x10U)
#define PINT_IENR_ENRL_PIN4_SHIFT                (4U)
/*! ENRL_PIN4 - Enables the rising edge or level interrupt for pin interrupt 4 (selected in
 *    PINTSEL4). [Note interrupt not supported to processor] 0: Disable rising edge or level interrupt. 1:
 *    Enable rising edge or level interrupt.
 */
#define PINT_IENR_ENRL_PIN4(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENR_ENRL_PIN4_SHIFT)) & PINT_IENR_ENRL_PIN4_MASK)
#define PINT_IENR_ENRL_PIN5_MASK                 (0x20U)
#define PINT_IENR_ENRL_PIN5_SHIFT                (5U)
/*! ENRL_PIN5 - Enables the rising edge or level interrupt for pin interrupt 5 (selected in
 *    PINTSEL5). [Note interrupt not supported to processor] 0: Disable rising edge or level interrupt. 1:
 *    Enable rising edge or level interrupt.
 */
#define PINT_IENR_ENRL_PIN5(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENR_ENRL_PIN5_SHIFT)) & PINT_IENR_ENRL_PIN5_MASK)
#define PINT_IENR_ENRL_PIN6_MASK                 (0x40U)
#define PINT_IENR_ENRL_PIN6_SHIFT                (6U)
/*! ENRL_PIN6 - Enables the rising edge or level interrupt for pin interrupt 6 (selected in
 *    PINTSEL6). [Note interrupt not supported to processor] 0: Disable rising edge or level interrupt. 1:
 *    Enable rising edge or level interrupt.
 */
#define PINT_IENR_ENRL_PIN6(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENR_ENRL_PIN6_SHIFT)) & PINT_IENR_ENRL_PIN6_MASK)
#define PINT_IENR_ENRL_PIN7_MASK                 (0x80U)
#define PINT_IENR_ENRL_PIN7_SHIFT                (7U)
/*! ENRL_PIN7 - Enables the rising edge or level interrupt for pin interrupt 7 (selected in
 *    PINTSEL7). [Note interrupt not supported to processor] 0: Disable rising edge or level interrupt. 1:
 *    Enable rising edge or level interrupt.
 */
#define PINT_IENR_ENRL_PIN7(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENR_ENRL_PIN7_SHIFT)) & PINT_IENR_ENRL_PIN7_MASK)
/*! @} */

/*! @name SIENR - Pin interrupt level or rising edge interrupt set register (only interrupts 0 to 3 supported to processor) */
/*! @{ */
#define PINT_SIENR_SETENRL_PIN0_MASK             (0x1U)
#define PINT_SIENR_SETENRL_PIN0_SHIFT            (0U)
/*! SETENRL_PIN0 - Ones written to this address set bits in the IENR, thus enabling interrupts. Bit
 *    0 sets bit 0 in the IENR register. 0: No operation. 1: Enable rising edge or level interrupt.
 */
#define PINT_SIENR_SETENRL_PIN0(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENR_SETENRL_PIN0_SHIFT)) & PINT_SIENR_SETENRL_PIN0_MASK)
#define PINT_SIENR_SETENRL_PIN1_MASK             (0x2U)
#define PINT_SIENR_SETENRL_PIN1_SHIFT            (1U)
/*! SETENRL_PIN1 - Ones written to this address set bits in the IENR, thus enabling interrupts. Bit
 *    1 sets bit 1 in the IENR register. 0: No operation. 1: Enable rising edge or level interrupt.
 */
#define PINT_SIENR_SETENRL_PIN1(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENR_SETENRL_PIN1_SHIFT)) & PINT_SIENR_SETENRL_PIN1_MASK)
#define PINT_SIENR_SETENRL_PIN2_MASK             (0x4U)
#define PINT_SIENR_SETENRL_PIN2_SHIFT            (2U)
/*! SETENRL_PIN2 - Ones written to this address set bits in the IENR, thus enabling interrupts. Bit
 *    2 sets bit 2 in the IENR register. 0: No operation. 1: Enable rising edge or level interrupt.
 */
#define PINT_SIENR_SETENRL_PIN2(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENR_SETENRL_PIN2_SHIFT)) & PINT_SIENR_SETENRL_PIN2_MASK)
#define PINT_SIENR_SETENRL_PIN3_MASK             (0x8U)
#define PINT_SIENR_SETENRL_PIN3_SHIFT            (3U)
/*! SETENRL_PIN3 - Ones written to this address set bits in the IENR, thus enabling interrupts. Bit
 *    3 sets bit 3 in the IENR register. 0: No operation. 1: Enable rising edge or level interrupt.
 */
#define PINT_SIENR_SETENRL_PIN3(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENR_SETENRL_PIN3_SHIFT)) & PINT_SIENR_SETENRL_PIN3_MASK)
#define PINT_SIENR_SETENRL_PIN4_MASK             (0x10U)
#define PINT_SIENR_SETENRL_PIN4_SHIFT            (4U)
/*! SETENRL_PIN4 - Ones written to this address set bits in the IENR, thus enabling interrupts. Bit
 *    4 sets bit 4 in the IENR register. [Note interrupt not supported to processor] 0: No
 *    operation. 1: Enable rising edge or level interrupt.
 */
#define PINT_SIENR_SETENRL_PIN4(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENR_SETENRL_PIN4_SHIFT)) & PINT_SIENR_SETENRL_PIN4_MASK)
#define PINT_SIENR_SETENRL_PIN5_MASK             (0x20U)
#define PINT_SIENR_SETENRL_PIN5_SHIFT            (5U)
/*! SETENRL_PIN5 - Ones written to this address set bits in the IENR, thus enabling interrupts. Bit
 *    5 sets bit 5 in the IENR register. [Note interrupt not supported to processor] 0: No
 *    operation. 1: Enable rising edge or level interrupt.
 */
#define PINT_SIENR_SETENRL_PIN5(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENR_SETENRL_PIN5_SHIFT)) & PINT_SIENR_SETENRL_PIN5_MASK)
#define PINT_SIENR_SETENRL_PIN6_MASK             (0x40U)
#define PINT_SIENR_SETENRL_PIN6_SHIFT            (6U)
/*! SETENRL_PIN6 - Ones written to this address set bits in the IENR, thus enabling interrupts. Bit
 *    6 sets bit 6 in the IENR register. [Note interrupt not supported to processor] 0: No
 *    operation. 1: Enable rising edge or level interrupt.
 */
#define PINT_SIENR_SETENRL_PIN6(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENR_SETENRL_PIN6_SHIFT)) & PINT_SIENR_SETENRL_PIN6_MASK)
#define PINT_SIENR_SETENRL_PIN7_MASK             (0x80U)
#define PINT_SIENR_SETENRL_PIN7_SHIFT            (7U)
/*! SETENRL_PIN7 - Ones written to this address set bits in the IENR, thus enabling interrupts. Bit
 *    7 sets bit 7 in the IENR register. [Note interrupt not supported to processor] 0: No
 *    operation. 1: Enable rising edge or level interrupt.
 */
#define PINT_SIENR_SETENRL_PIN7(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENR_SETENRL_PIN7_SHIFT)) & PINT_SIENR_SETENRL_PIN7_MASK)
/*! @} */

/*! @name CIENR - Pin interrupt level (rising edge interrupt) clear register (only interrupts 0 to 3 supported to processor) */
/*! @{ */
#define PINT_CIENR_CLRENRL_PIN0_MASK             (0x1U)
#define PINT_CIENR_CLRENRL_PIN0_SHIFT            (0U)
/*! CLRENRL_PIN0 - Ones written to this address clear bits in the IENR, thus disabling the
 *    interrupts. Bit 0 clears bit 0 in the IENR register. 0: No operation. 1: Disable rising edge or level
 *    interrupt.
 */
#define PINT_CIENR_CLRENRL_PIN0(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENR_CLRENRL_PIN0_SHIFT)) & PINT_CIENR_CLRENRL_PIN0_MASK)
#define PINT_CIENR_CLRENRL_PIN1_MASK             (0x2U)
#define PINT_CIENR_CLRENRL_PIN1_SHIFT            (1U)
/*! CLRENRL_PIN1 - Ones written to this address clear bits in the IENR, thus disabling the
 *    interrupts. Bit 1 clears bit 1 in the IENR register. 0: No operation. 1: Disable rising edge or level
 *    interrupt.
 */
#define PINT_CIENR_CLRENRL_PIN1(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENR_CLRENRL_PIN1_SHIFT)) & PINT_CIENR_CLRENRL_PIN1_MASK)
#define PINT_CIENR_CLRENRL_PIN2_MASK             (0x4U)
#define PINT_CIENR_CLRENRL_PIN2_SHIFT            (2U)
/*! CLRENRL_PIN2 - Ones written to this address clear bits in the IENR, thus disabling the
 *    interrupts. Bit 2 clears bit 2 in the IENR register. 0: No operation. 1: Disable rising edge or level
 *    interrupt.
 */
#define PINT_CIENR_CLRENRL_PIN2(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENR_CLRENRL_PIN2_SHIFT)) & PINT_CIENR_CLRENRL_PIN2_MASK)
#define PINT_CIENR_CLRENRL_PIN3_MASK             (0x8U)
#define PINT_CIENR_CLRENRL_PIN3_SHIFT            (3U)
/*! CLRENRL_PIN3 - Ones written to this address clear bits in the IENR, thus disabling the
 *    interrupts. Bit 3 clears bit 3 in the IENR register. 0: No operation. 1: Disable rising edge or level
 *    interrupt.
 */
#define PINT_CIENR_CLRENRL_PIN3(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENR_CLRENRL_PIN3_SHIFT)) & PINT_CIENR_CLRENRL_PIN3_MASK)
#define PINT_CIENR_CLRENRL_PIN4_MASK             (0x10U)
#define PINT_CIENR_CLRENRL_PIN4_SHIFT            (4U)
/*! CLRENRL_PIN4 - Ones written to this address clear bits in the IENR, thus disabling the
 *    interrupts. Bit 4 clears bit 4 in the IENR register. [Note interrupt not supported to processor] 0: No
 *    operation. 1: Disable rising edge or level interrupt.
 */
#define PINT_CIENR_CLRENRL_PIN4(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENR_CLRENRL_PIN4_SHIFT)) & PINT_CIENR_CLRENRL_PIN4_MASK)
#define PINT_CIENR_CLRENRL_PIN5_MASK             (0x20U)
#define PINT_CIENR_CLRENRL_PIN5_SHIFT            (5U)
/*! CLRENRL_PIN5 - Ones written to this address clear bits in the IENR, thus disabling the
 *    interrupts. Bit 5 clears bit 5 in the IENR register. [Note interrupt not supported to processor] 0: No
 *    operation. 1: Disable rising edge or level interrupt.
 */
#define PINT_CIENR_CLRENRL_PIN5(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENR_CLRENRL_PIN5_SHIFT)) & PINT_CIENR_CLRENRL_PIN5_MASK)
#define PINT_CIENR_CLRENRL_PIN6_MASK             (0x40U)
#define PINT_CIENR_CLRENRL_PIN6_SHIFT            (6U)
/*! CLRENRL_PIN6 - Ones written to this address clear bits in the IENR, thus disabling the
 *    interrupts. Bit 6 clears bit 6 in the IENR register. [Note interrupt not supported to processor] 0: No
 *    operation. 1: Disable rising edge or level interrupt.
 */
#define PINT_CIENR_CLRENRL_PIN6(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENR_CLRENRL_PIN6_SHIFT)) & PINT_CIENR_CLRENRL_PIN6_MASK)
#define PINT_CIENR_CLRENRL_PIN7_MASK             (0x80U)
#define PINT_CIENR_CLRENRL_PIN7_SHIFT            (7U)
/*! CLRENRL_PIN7 - Ones written to this address clear bits in the IENR, thus disabling the
 *    interrupts. Bit 7 clears bit 7 in the IENR register. [Note interrupt not supported to processor] 0: No
 *    operation. 1: Disable rising edge or level interrupt.
 */
#define PINT_CIENR_CLRENRL_PIN7(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENR_CLRENRL_PIN7_SHIFT)) & PINT_CIENR_CLRENRL_PIN7_MASK)
/*! @} */

/*! @name IENF - Pin interrupt active level or falling edge interrupt enable register */
/*! @{ */
#define PINT_IENF_ENAF_PIN0_MASK                 (0x1U)
#define PINT_IENF_ENAF_PIN0_SHIFT                (0U)
/*! ENAF_PIN0 - Enables the falling edge or configures the active level interrupt for pin interrupt
 *    0 (selected in PINTSEL0). 0: Disable falling edge interrupt or set active interrupt level LOW.
 *    1: Enable falling edge interrupt enabled or set active interrupt level HIGH.
 */
#define PINT_IENF_ENAF_PIN0(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENF_ENAF_PIN0_SHIFT)) & PINT_IENF_ENAF_PIN0_MASK)
#define PINT_IENF_ENAF_PIN1_MASK                 (0x2U)
#define PINT_IENF_ENAF_PIN1_SHIFT                (1U)
/*! ENAF_PIN1 - Enables the falling edge or configures the active level interrupt for pin interrupt
 *    1 (selected in PINTSEL1). 0: Disable falling edge interrupt or set active interrupt level LOW.
 *    1: Enable falling edge interrupt enabled or set active interrupt level HIGH.
 */
#define PINT_IENF_ENAF_PIN1(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENF_ENAF_PIN1_SHIFT)) & PINT_IENF_ENAF_PIN1_MASK)
#define PINT_IENF_ENAF_PIN2_MASK                 (0x4U)
#define PINT_IENF_ENAF_PIN2_SHIFT                (2U)
/*! ENAF_PIN2 - Enables the falling edge or configures the active level interrupt for pin interrupt
 *    2 (selected in PINTSEL2). 0: Disable falling edge interrupt or set active interrupt level LOW.
 *    1: Enable falling edge interrupt enabled or set active interrupt level HIGH.
 */
#define PINT_IENF_ENAF_PIN2(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENF_ENAF_PIN2_SHIFT)) & PINT_IENF_ENAF_PIN2_MASK)
#define PINT_IENF_ENAF_PIN3_MASK                 (0x8U)
#define PINT_IENF_ENAF_PIN3_SHIFT                (3U)
/*! ENAF_PIN3 - Enables the falling edge or configures the active level interrupt for pin interrupt
 *    3 (selected in PINTSEL3). 0: Disable falling edge interrupt or set active interrupt level LOW.
 *    1: Enable falling edge interrupt enabled or set active interrupt level HIGH.
 */
#define PINT_IENF_ENAF_PIN3(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENF_ENAF_PIN3_SHIFT)) & PINT_IENF_ENAF_PIN3_MASK)
#define PINT_IENF_ENAF_PIN4_MASK                 (0x10U)
#define PINT_IENF_ENAF_PIN4_SHIFT                (4U)
/*! ENAF_PIN4 - Enables the falling edge or configures the active level interrupt for pin interrupt
 *    4 (selected in PINTSEL4). [Note interrupt not supported to processor] 0: Disable falling edge
 *    interrupt or set active interrupt level LOW. 1: Enable falling edge interrupt enabled or set
 *    active interrupt level HIGH.
 */
#define PINT_IENF_ENAF_PIN4(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENF_ENAF_PIN4_SHIFT)) & PINT_IENF_ENAF_PIN4_MASK)
#define PINT_IENF_ENAF_PIN5_MASK                 (0x20U)
#define PINT_IENF_ENAF_PIN5_SHIFT                (5U)
/*! ENAF_PIN5 - Enables the falling edge or configures the active level interrupt for pin interrupt
 *    5 (selected in PINTSEL5). [Note interrupt not supported to processor] 0: Disable falling edge
 *    interrupt or set active interrupt level LOW. 1: Enable falling edge interrupt enabled or set
 *    active interrupt level HIGH.
 */
#define PINT_IENF_ENAF_PIN5(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENF_ENAF_PIN5_SHIFT)) & PINT_IENF_ENAF_PIN5_MASK)
#define PINT_IENF_ENAF_PIN6_MASK                 (0x40U)
#define PINT_IENF_ENAF_PIN6_SHIFT                (6U)
/*! ENAF_PIN6 - Enables the falling edge or configures the active level interrupt for pin interrupt
 *    6 (selected in PINTSEL6). [Note interrupt not supported to processor] 0: Disable falling edge
 *    interrupt or set active interrupt level LOW. 1: Enable falling edge interrupt enabled or set
 *    active interrupt level HIGH.
 */
#define PINT_IENF_ENAF_PIN6(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENF_ENAF_PIN6_SHIFT)) & PINT_IENF_ENAF_PIN6_MASK)
#define PINT_IENF_ENAF_PIN7_MASK                 (0x80U)
#define PINT_IENF_ENAF_PIN7_SHIFT                (7U)
/*! ENAF_PIN7 - Enables the falling edge or configures the active level interrupt for pin interrupt
 *    7 (selected in PINTSEL7). [Note interrupt not supported to processor] 0: Disable falling edge
 *    interrupt or set active interrupt level LOW. 1: Enable falling edge interrupt enabled or set
 *    active interrupt level HIGH.
 */
#define PINT_IENF_ENAF_PIN7(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IENF_ENAF_PIN7_SHIFT)) & PINT_IENF_ENAF_PIN7_MASK)
/*! @} */

/*! @name SIENF - Pin interrupt active level or falling edge interrupt set register */
/*! @{ */
#define PINT_SIENF_SETENAF_PIN0_MASK             (0x1U)
#define PINT_SIENF_SETENAF_PIN0_SHIFT            (0U)
/*! SETENAF_PIN0 - Ones written to this address set bits in the IENF, thus enabling interrupts. Bit
 *    0 sets bit 0 in the IENF register. 0: No operation. 1: Select HIGH-active interrupt or enable
 *    falling edge interrupt.
 */
#define PINT_SIENF_SETENAF_PIN0(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENF_SETENAF_PIN0_SHIFT)) & PINT_SIENF_SETENAF_PIN0_MASK)
#define PINT_SIENF_SETENAF_PIN1_MASK             (0x2U)
#define PINT_SIENF_SETENAF_PIN1_SHIFT            (1U)
/*! SETENAF_PIN1 - Ones written to this address set bits in the IENF, thus enabling interrupts. Bit
 *    1 sets bit 1 in the IENF register. 0: No operation. 1: Select HIGH-active interrupt or enable
 *    falling edge interrupt.
 */
#define PINT_SIENF_SETENAF_PIN1(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENF_SETENAF_PIN1_SHIFT)) & PINT_SIENF_SETENAF_PIN1_MASK)
#define PINT_SIENF_SETENAF_PIN2_MASK             (0x4U)
#define PINT_SIENF_SETENAF_PIN2_SHIFT            (2U)
/*! SETENAF_PIN2 - Ones written to this address set bits in the IENF, thus enabling interrupts. Bit
 *    2 sets bit 2 in the IENF register. 0: No operation. 1: Select HIGH-active interrupt or enable
 *    falling edge interrupt.
 */
#define PINT_SIENF_SETENAF_PIN2(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENF_SETENAF_PIN2_SHIFT)) & PINT_SIENF_SETENAF_PIN2_MASK)
#define PINT_SIENF_SETENAF_PIN3_MASK             (0x8U)
#define PINT_SIENF_SETENAF_PIN3_SHIFT            (3U)
/*! SETENAF_PIN3 - Ones written to this address set bits in the IENF, thus enabling interrupts. Bit
 *    3 sets bit 3 in the IENF register. 0: No operation. 1: Select HIGH-active interrupt or enable
 *    falling edge interrupt.
 */
#define PINT_SIENF_SETENAF_PIN3(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENF_SETENAF_PIN3_SHIFT)) & PINT_SIENF_SETENAF_PIN3_MASK)
#define PINT_SIENF_SETENAF_PIN4_MASK             (0x10U)
#define PINT_SIENF_SETENAF_PIN4_SHIFT            (4U)
/*! SETENAF_PIN4 - Ones written to this address set bits in the IENF, thus enabling interrupts. Bit
 *    4 sets bit 4 in the IENF register. 0: No operation. 1: Select HIGH-active interrupt or enable
 *    falling edge interrupt.
 */
#define PINT_SIENF_SETENAF_PIN4(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENF_SETENAF_PIN4_SHIFT)) & PINT_SIENF_SETENAF_PIN4_MASK)
#define PINT_SIENF_SETENAF_PIN5_MASK             (0x20U)
#define PINT_SIENF_SETENAF_PIN5_SHIFT            (5U)
/*! SETENAF_PIN5 - Ones written to this address set bits in the IENF, thus enabling interrupts. Bit
 *    5 sets bit 5 in the IENF register. 0: No operation. 1: Select HIGH-active interrupt or enable
 *    falling edge interrupt.
 */
#define PINT_SIENF_SETENAF_PIN5(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENF_SETENAF_PIN5_SHIFT)) & PINT_SIENF_SETENAF_PIN5_MASK)
#define PINT_SIENF_SETENAF_PIN6_MASK             (0x40U)
#define PINT_SIENF_SETENAF_PIN6_SHIFT            (6U)
/*! SETENAF_PIN6 - Ones written to this address set bits in the IENF, thus enabling interrupts. Bit
 *    6 sets bit 6 in the IENF register. 0: No operation. 1: Select HIGH-active interrupt or enable
 *    falling edge interrupt.
 */
#define PINT_SIENF_SETENAF_PIN6(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENF_SETENAF_PIN6_SHIFT)) & PINT_SIENF_SETENAF_PIN6_MASK)
#define PINT_SIENF_SETENAF_PIN7_MASK             (0x80U)
#define PINT_SIENF_SETENAF_PIN7_SHIFT            (7U)
/*! SETENAF_PIN7 - Ones written to this address set bits in the IENF, thus enabling interrupts. Bit
 *    7 sets bit 7 in the IENF register. 0: No operation. 1: Select HIGH-active interrupt or enable
 *    falling edge interrupt.
 */
#define PINT_SIENF_SETENAF_PIN7(x)               (((uint32_t)(((uint32_t)(x)) << PINT_SIENF_SETENAF_PIN7_SHIFT)) & PINT_SIENF_SETENAF_PIN7_MASK)
/*! @} */

/*! @name CIENF - Pin interrupt active level or falling edge interrupt clear register */
/*! @{ */
#define PINT_CIENF_CLRENAF_PIN0_MASK             (0x1U)
#define PINT_CIENF_CLRENAF_PIN0_SHIFT            (0U)
/*! CLRENAF_PIN0 - Ones written to this address clears bits in the IENF, thus disabling interrupts.
 *    Bit 0 clears bit 0 in the IENF register. 0: No operation. 1: LOW-active interrupt selected or
 *    falling edge interrupt disabled.
 */
#define PINT_CIENF_CLRENAF_PIN0(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENF_CLRENAF_PIN0_SHIFT)) & PINT_CIENF_CLRENAF_PIN0_MASK)
#define PINT_CIENF_CLRENAF_PIN1_MASK             (0x2U)
#define PINT_CIENF_CLRENAF_PIN1_SHIFT            (1U)
/*! CLRENAF_PIN1 - Ones written to this address clears bits in the IENF, thus disabling interrupts.
 *    Bit 1 clears bit 1 in the IENF register. 0: No operation. 1: LOW-active interrupt selected or
 *    falling edge interrupt disabled.
 */
#define PINT_CIENF_CLRENAF_PIN1(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENF_CLRENAF_PIN1_SHIFT)) & PINT_CIENF_CLRENAF_PIN1_MASK)
#define PINT_CIENF_CLRENAF_PIN2_MASK             (0x4U)
#define PINT_CIENF_CLRENAF_PIN2_SHIFT            (2U)
/*! CLRENAF_PIN2 - Ones written to this address clears bits in the IENF, thus disabling interrupts.
 *    Bit 2 clears bit 2 in the IENF register. 0: No operation. 1: LOW-active interrupt selected or
 *    falling edge interrupt disabled.
 */
#define PINT_CIENF_CLRENAF_PIN2(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENF_CLRENAF_PIN2_SHIFT)) & PINT_CIENF_CLRENAF_PIN2_MASK)
#define PINT_CIENF_CLRENAF_PIN3_MASK             (0x8U)
#define PINT_CIENF_CLRENAF_PIN3_SHIFT            (3U)
/*! CLRENAF_PIN3 - Ones written to this address clears bits in the IENF, thus disabling interrupts.
 *    Bit 3 clears bit 3 in the IENF register. 0: No operation. 1: LOW-active interrupt selected or
 *    falling edge interrupt disabled.
 */
#define PINT_CIENF_CLRENAF_PIN3(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENF_CLRENAF_PIN3_SHIFT)) & PINT_CIENF_CLRENAF_PIN3_MASK)
#define PINT_CIENF_CLRENAF_PIN4_MASK             (0x10U)
#define PINT_CIENF_CLRENAF_PIN4_SHIFT            (4U)
/*! CLRENAF_PIN4 - Ones written to this address clears bits in the IENF, thus disabling interrupts.
 *    Bit 4 clears bit 4 in the IENF register. 0: No operation. 1: LOW-active interrupt selected or
 *    falling edge interrupt disabled.
 */
#define PINT_CIENF_CLRENAF_PIN4(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENF_CLRENAF_PIN4_SHIFT)) & PINT_CIENF_CLRENAF_PIN4_MASK)
#define PINT_CIENF_CLRENAF_PIN5_MASK             (0x20U)
#define PINT_CIENF_CLRENAF_PIN5_SHIFT            (5U)
/*! CLRENAF_PIN5 - Ones written to this address clears bits in the IENF, thus disabling interrupts.
 *    Bit 5 clears bit 5 in the IENF register. 0: No operation. 1: LOW-active interrupt selected or
 *    falling edge interrupt disabled.
 */
#define PINT_CIENF_CLRENAF_PIN5(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENF_CLRENAF_PIN5_SHIFT)) & PINT_CIENF_CLRENAF_PIN5_MASK)
#define PINT_CIENF_CLRENAF_PIN6_MASK             (0x40U)
#define PINT_CIENF_CLRENAF_PIN6_SHIFT            (6U)
/*! CLRENAF_PIN6 - Ones written to this address clears bits in the IENF, thus disabling interrupts.
 *    Bit 6 clears bit 6 in the IENF register. 0: No operation. 1: LOW-active interrupt selected or
 *    falling edge interrupt disabled.
 */
#define PINT_CIENF_CLRENAF_PIN6(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENF_CLRENAF_PIN6_SHIFT)) & PINT_CIENF_CLRENAF_PIN6_MASK)
#define PINT_CIENF_CLRENAF_PIN7_MASK             (0x80U)
#define PINT_CIENF_CLRENAF_PIN7_SHIFT            (7U)
/*! CLRENAF_PIN7 - Ones written to this address clears bits in the IENF, thus disabling interrupts.
 *    Bit 7 clears bit 7 in the IENF register. 0: No operation. 1: LOW-active interrupt selected or
 *    falling edge interrupt disabled.
 */
#define PINT_CIENF_CLRENAF_PIN7(x)               (((uint32_t)(((uint32_t)(x)) << PINT_CIENF_CLRENAF_PIN7_SHIFT)) & PINT_CIENF_CLRENAF_PIN7_MASK)
/*! @} */

/*! @name RISE - Pin interrupt rising edge register */
/*! @{ */
#define PINT_RISE_RDET_MASK                      (0xFFU)
#define PINT_RISE_RDET_SHIFT                     (0U)
/*! RDET - Rising edge detect. Bit n detects the rising edge of the pin selected in PINTSELn. Read
 *    0: No rising edge has been detected on this pin since Reset or the last time a one was written
 *    to this bit. Write 0: no operation. Read 1: a rising edge has been detected since Reset or the
 *    last time a one was written to this bit. Write 1: clear rising edge detection for this pin.
 */
#define PINT_RISE_RDET(x)                        (((uint32_t)(((uint32_t)(x)) << PINT_RISE_RDET_SHIFT)) & PINT_RISE_RDET_MASK)
/*! @} */

/*! @name FALL - Pin interrupt falling edge register */
/*! @{ */
#define PINT_FALL_FDET_MASK                      (0xFFU)
#define PINT_FALL_FDET_SHIFT                     (0U)
/*! FDET - Falling edge detect. Bit n detects the falling edge of the pin selected in PINTSELn. Read
 *    0: No falling edge has been detected on this pin since Reset or the last time a one was
 *    written to this bit. Write 0: no operation. Read 1: a falling edge has been detected since Reset or
 *    the last time a one was written to this bit. Write 1: clear falling edge detection for this
 *    pin.
 */
#define PINT_FALL_FDET(x)                        (((uint32_t)(((uint32_t)(x)) << PINT_FALL_FDET_SHIFT)) & PINT_FALL_FDET_MASK)
/*! @} */

/*! @name IST - Pin interrupt status register. For bits in this regsiter the following functionality occurs for the corresponding PIN bit: Read 0: interrupt is not being requested for this interrupt pin. Write 0: no operation. Read 1: interrupt is being requested for this interrupt pin. Write 1 (edge-sensitive): clear rising- and falling-edge detection for this pin. Write 1 (level-sensitive): switch the active level for this pin (in the IENF register). */
/*! @{ */
#define PINT_IST_PSTAT_PIN0_MASK                 (0x1U)
#define PINT_IST_PSTAT_PIN0_SHIFT                (0U)
/*! PSTAT_PIN0 - Pin interrupt status. Bit 0 returns the status, clears the edge interrupt, or
 *    inverts the active level of the pin 0 (selected in PINTSEL0).
 */
#define PINT_IST_PSTAT_PIN0(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IST_PSTAT_PIN0_SHIFT)) & PINT_IST_PSTAT_PIN0_MASK)
#define PINT_IST_PSTAT_PIN1_MASK                 (0x2U)
#define PINT_IST_PSTAT_PIN1_SHIFT                (1U)
/*! PSTAT_PIN1 - Pin interrupt status. Bit 1 returns the status, clears the edge interrupt, or
 *    inverts the active level of the pin 1 (selected in PINTSEL0).
 */
#define PINT_IST_PSTAT_PIN1(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IST_PSTAT_PIN1_SHIFT)) & PINT_IST_PSTAT_PIN1_MASK)
#define PINT_IST_PSTAT_PIN2_MASK                 (0x4U)
#define PINT_IST_PSTAT_PIN2_SHIFT                (2U)
/*! PSTAT_PIN2 - Pin interrupt status. Bit 2 returns the status, clears the edge interrupt, or
 *    inverts the active level of the pin 2 (selected in PINTSEL2).
 */
#define PINT_IST_PSTAT_PIN2(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IST_PSTAT_PIN2_SHIFT)) & PINT_IST_PSTAT_PIN2_MASK)
#define PINT_IST_PSTAT_PIN3_MASK                 (0x8U)
#define PINT_IST_PSTAT_PIN3_SHIFT                (3U)
/*! PSTAT_PIN3 - Pin interrupt status. Bit 3 returns the status, clears the edge interrupt, or
 *    inverts the active level of the pin 3 (selected in PINTSEL3).
 */
#define PINT_IST_PSTAT_PIN3(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IST_PSTAT_PIN3_SHIFT)) & PINT_IST_PSTAT_PIN3_MASK)
#define PINT_IST_PSTAT_PIN4_MASK                 (0x10U)
#define PINT_IST_PSTAT_PIN4_SHIFT                (4U)
/*! PSTAT_PIN4 - Pin interrupt status. Bit 4 returns the status, clears the edge interrupt, or
 *    inverts the active level of the pin 4 (selected in PINTSEL4).
 */
#define PINT_IST_PSTAT_PIN4(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IST_PSTAT_PIN4_SHIFT)) & PINT_IST_PSTAT_PIN4_MASK)
#define PINT_IST_PSTAT_PIN5_MASK                 (0x20U)
#define PINT_IST_PSTAT_PIN5_SHIFT                (5U)
/*! PSTAT_PIN5 - Pin interrupt status. Bit 5 returns the status, clears the edge interrupt, or
 *    inverts the active level of the pin 5 (selected in PINTSEL5).
 */
#define PINT_IST_PSTAT_PIN5(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IST_PSTAT_PIN5_SHIFT)) & PINT_IST_PSTAT_PIN5_MASK)
#define PINT_IST_PSTAT_PIN6_MASK                 (0x40U)
#define PINT_IST_PSTAT_PIN6_SHIFT                (6U)
/*! PSTAT_PIN6 - Pin interrupt status. Bit 6 returns the status, clears the edge interrupt, or
 *    inverts the active level of the pin 6 (selected in PINTSEL6).
 */
#define PINT_IST_PSTAT_PIN6(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IST_PSTAT_PIN6_SHIFT)) & PINT_IST_PSTAT_PIN6_MASK)
#define PINT_IST_PSTAT_PIN7_MASK                 (0x80U)
#define PINT_IST_PSTAT_PIN7_SHIFT                (7U)
/*! PSTAT_PIN7 - Pin interrupt status. Bit 7 returns the status, clears the edge interrupt, or
 *    inverts the active level of the pin 7 (selected in PINTSEL7).
 */
#define PINT_IST_PSTAT_PIN7(x)                   (((uint32_t)(((uint32_t)(x)) << PINT_IST_PSTAT_PIN7_SHIFT)) & PINT_IST_PSTAT_PIN7_MASK)
/*! @} */

/*! @name PMCTRL - Pattern match interrupt control register */
/*! @{ */
#define PINT_PMCTRL_SEL_PMATCH_MASK              (0x1U)
#define PINT_PMCTRL_SEL_PMATCH_SHIFT             (0U)
/*! SEL_PMATCH - Specifies whether the 8 pin interrupts are controlled by the pin interrupt function
 *    or by the pattern match function. 0: Pin interrupt. Interrupts are driven in response to the
 *    standard pin interrupt function. 1: Pattern match. Interrupts are driven in response to
 *    pattern matches.
 */
#define PINT_PMCTRL_SEL_PMATCH(x)                (((uint32_t)(((uint32_t)(x)) << PINT_PMCTRL_SEL_PMATCH_SHIFT)) & PINT_PMCTRL_SEL_PMATCH_MASK)
#define PINT_PMCTRL_ENA_RXEV_MASK                (0x2U)
#define PINT_PMCTRL_ENA_RXEV_SHIFT               (1U)
/*! ENA_RXEV - Enables the RXEV output to the CPU when the specified boolean expression evaluates to
 *    true. 0: Disabled. RXEV output to the CPU is disabled. 1: Enabled. RXEV output to the CPU is
 *    enabled.
 */
#define PINT_PMCTRL_ENA_RXEV(x)                  (((uint32_t)(((uint32_t)(x)) << PINT_PMCTRL_ENA_RXEV_SHIFT)) & PINT_PMCTRL_ENA_RXEV_MASK)
#define PINT_PMCTRL_PMAT_MASK                    (0xFF000000U)
#define PINT_PMCTRL_PMAT_SHIFT                   (24U)
/*! PMAT - This field displays the current state of pattern matches. A 1 in any bit of this field
 *    indicates that the corresponding product term is matched by the current state of the appropriate
 *    inputs.
 */
#define PINT_PMCTRL_PMAT(x)                      (((uint32_t)(((uint32_t)(x)) << PINT_PMCTRL_PMAT_SHIFT)) & PINT_PMCTRL_PMAT_MASK)
/*! @} */

/*! @name PMSRC - Pattern match interrupt bit-slice source register */
/*! @{ */
#define PINT_PMSRC_SRC0_MASK                     (0x700U)
#define PINT_PMSRC_SRC0_SHIFT                    (8U)
/*! SRC0 - Selects the input source for bit slice 0. Value X selects the pin selected in the
 *    PINTSELX register as the source to this bit slice. For example 3 selects the pin selected in PINTSEL3
 *    regsiter.
 */
#define PINT_PMSRC_SRC0(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMSRC_SRC0_SHIFT)) & PINT_PMSRC_SRC0_MASK)
#define PINT_PMSRC_SRC1_MASK                     (0x3800U)
#define PINT_PMSRC_SRC1_SHIFT                    (11U)
/*! SRC1 - Selects the input source for bit slice 1. Value X selects the pin selected in the
 *    PINTSELX register as the source to this bit slice. For example 3 selects the pin selected in PINTSEL3
 *    regsiter.
 */
#define PINT_PMSRC_SRC1(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMSRC_SRC1_SHIFT)) & PINT_PMSRC_SRC1_MASK)
#define PINT_PMSRC_SRC2_MASK                     (0x1C000U)
#define PINT_PMSRC_SRC2_SHIFT                    (14U)
/*! SRC2 - Selects the input source for bit slice 2. Value X selects the pin selected in the
 *    PINTSELX register as the source to this bit slice. For example 3 selects the pin selected in PINTSEL3
 *    regsiter.
 */
#define PINT_PMSRC_SRC2(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMSRC_SRC2_SHIFT)) & PINT_PMSRC_SRC2_MASK)
#define PINT_PMSRC_SRC3_MASK                     (0xE0000U)
#define PINT_PMSRC_SRC3_SHIFT                    (17U)
/*! SRC3 - Selects the input source for bit slice 3. Value X selects the pin selected in the
 *    PINTSELX register as the source to this bit slice. For example 3 selects the pin selected in PINTSEL3
 *    regsiter.
 */
#define PINT_PMSRC_SRC3(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMSRC_SRC3_SHIFT)) & PINT_PMSRC_SRC3_MASK)
#define PINT_PMSRC_SRC4_MASK                     (0x700000U)
#define PINT_PMSRC_SRC4_SHIFT                    (20U)
/*! SRC4 - Selects the input source for bit slice 4. Value X selects the pin selected in the
 *    PINTSELX register as the source to this bit slice. For example 3 selects the pin selected in PINTSEL3
 *    regsiter.
 */
#define PINT_PMSRC_SRC4(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMSRC_SRC4_SHIFT)) & PINT_PMSRC_SRC4_MASK)
#define PINT_PMSRC_SRC5_MASK                     (0x3800000U)
#define PINT_PMSRC_SRC5_SHIFT                    (23U)
/*! SRC5 - Selects the input source for bit slice 5. Value X selects the pin selected in the
 *    PINTSELX register as the source to this bit slice. For example 3 selects the pin selected in PINTSEL3
 *    regsiter.
 */
#define PINT_PMSRC_SRC5(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMSRC_SRC5_SHIFT)) & PINT_PMSRC_SRC5_MASK)
#define PINT_PMSRC_SRC6_MASK                     (0x1C000000U)
#define PINT_PMSRC_SRC6_SHIFT                    (26U)
/*! SRC6 - Selects the input source for bit slice 6. Value X selects the pin selected in the
 *    PINTSELX register as the source to this bit slice. For example 3 selects the pin selected in PINTSEL3
 *    regsiter.
 */
#define PINT_PMSRC_SRC6(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMSRC_SRC6_SHIFT)) & PINT_PMSRC_SRC6_MASK)
#define PINT_PMSRC_SRC7_MASK                     (0xE0000000U)
#define PINT_PMSRC_SRC7_SHIFT                    (29U)
/*! SRC7 - Selects the input source for bit slice 7. Value X selects the pin selected in the
 *    PINTSELX register as the source to this bit slice. For example 3 selects the pin selected in PINTSEL3
 *    regsiter.
 */
#define PINT_PMSRC_SRC7(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMSRC_SRC7_SHIFT)) & PINT_PMSRC_SRC7_MASK)
/*! @} */

/*! @name PMCFG - Pattern match interrupt bit slice configuration register */
/*! @{ */
#define PINT_PMCFG_PROD_ENDPTS0_MASK             (0x1U)
#define PINT_PMCFG_PROD_ENDPTS0_SHIFT            (0U)
/*! PROD_ENDPTS0 - Determines whether slice 0 is an endpoint. 0: No effect. Slice 0 is not an
 *    endpoint. 1: endpoint. Slice 0 is the endpoint of a product term (minterm). Interrupt PINT0 in the
 *    NVIC is raised if the minterm evaluates as true.
 */
#define PINT_PMCFG_PROD_ENDPTS0(x)               (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_PROD_ENDPTS0_SHIFT)) & PINT_PMCFG_PROD_ENDPTS0_MASK)
#define PINT_PMCFG_PROD_ENDPTS1_MASK             (0x2U)
#define PINT_PMCFG_PROD_ENDPTS1_SHIFT            (1U)
/*! PROD_ENDPTS1 - Determines whether slice 1 is an endpoint. 0: No effect. Slice 1 is not an
 *    endpoint. 1: endpoint. Slice 1 is the endpoint of a product term (minterm). Interrupt PINT1 in the
 *    NVIC is raised if the minterm evaluates as true.
 */
#define PINT_PMCFG_PROD_ENDPTS1(x)               (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_PROD_ENDPTS1_SHIFT)) & PINT_PMCFG_PROD_ENDPTS1_MASK)
#define PINT_PMCFG_PROD_ENDPTS2_MASK             (0x4U)
#define PINT_PMCFG_PROD_ENDPTS2_SHIFT            (2U)
/*! PROD_ENDPTS2 - Determines whether slice 2 is an endpoint. 0: No effect. Slice 2 is not an
 *    endpoint. 1: endpoint. Slice 2 is the endpoint of a product term (minterm). Interrupt PINT2 in the
 *    NVIC is raised if the minterm evaluates as true.
 */
#define PINT_PMCFG_PROD_ENDPTS2(x)               (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_PROD_ENDPTS2_SHIFT)) & PINT_PMCFG_PROD_ENDPTS2_MASK)
#define PINT_PMCFG_PROD_ENDPTS3_MASK             (0x8U)
#define PINT_PMCFG_PROD_ENDPTS3_SHIFT            (3U)
/*! PROD_ENDPTS3 - Determines whether slice 3 is an endpoint. 0: No effect. Slice 3 is not an
 *    endpoint. 1: endpoint. Slice 3 is the endpoint of a product term (minterm). Interrupt PINT3 in the
 *    NVIC is raised if the minterm evaluates as true.
 */
#define PINT_PMCFG_PROD_ENDPTS3(x)               (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_PROD_ENDPTS3_SHIFT)) & PINT_PMCFG_PROD_ENDPTS3_MASK)
#define PINT_PMCFG_PROD_ENDPTS4_MASK             (0x10U)
#define PINT_PMCFG_PROD_ENDPTS4_SHIFT            (4U)
/*! PROD_ENDPTS4 - Determines whether slice 4 is an endpoint. 0: No effect. Slice 4 is not an
 *    endpoint. 1: endpoint. Slice 4 is the endpoint of a product term (minterm). No NVIC interrupt is
 *    assocaited with this.
 */
#define PINT_PMCFG_PROD_ENDPTS4(x)               (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_PROD_ENDPTS4_SHIFT)) & PINT_PMCFG_PROD_ENDPTS4_MASK)
#define PINT_PMCFG_PROD_ENDPTS5_MASK             (0x20U)
#define PINT_PMCFG_PROD_ENDPTS5_SHIFT            (5U)
/*! PROD_ENDPTS5 - Determines whether slice 5 is an endpoint. 0 No effect. Slice 5 is not an
 *    endpoint. 1: endpoint. Slice 5 is the endpoint of a product term (minterm). No NVIC interrupt is
 *    assocaited with this.
 */
#define PINT_PMCFG_PROD_ENDPTS5(x)               (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_PROD_ENDPTS5_SHIFT)) & PINT_PMCFG_PROD_ENDPTS5_MASK)
#define PINT_PMCFG_PROD_ENDPTS6_MASK             (0x40U)
#define PINT_PMCFG_PROD_ENDPTS6_SHIFT            (6U)
/*! PROD_ENDPTS6 - Determines whether slice 6 is an endpoint. 0: No effect. Slice 6 is not an
 *    endpoint. 1: endpoint. Slice 6 is the endpoint of a product term (minterm). No NVIC interrupt is
 *    assocaited with this.
 */
#define PINT_PMCFG_PROD_ENDPTS6(x)               (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_PROD_ENDPTS6_SHIFT)) & PINT_PMCFG_PROD_ENDPTS6_MASK)
#define PINT_PMCFG_CFG0_MASK                     (0x700U)
#define PINT_PMCFG_CFG0_SHIFT                    (8U)
/*! CFG0 - Specifies the match contribution condition for bit slice 0. 0x0: Constant HIGH. This bit
 *    slice always contributes to a product term match. 0x1: Sticky rising edge. Match occurs if a
 *    rising edge on the specified input has occurred since the last time the edge detection for this
 *    bit slice was cleared. This bit is only cleared when the PMCFG or the PMSRC registers are
 *    written to. 0x2: Sticky falling edge. Match occurs if a falling edge on the specified input has
 *    occurred since the last time the edge detection for this bit slice was cleared. This bit is
 *    only cleared when the PMCFG or the PMSRC registers are written to. 0x3: Sticky rising or falling
 *    edge. Match occurs if either a rising or falling edge on the specified input has occurred
 *    since the last time the edge detection for this bit slice was cleared. This bit is only cleared
 *    when the PMCFG or the PMSRC registers are written to. 0x4: High level. Match (for this bit
 *    slice) occurs when there is a high level on the input specified for this bit slice in the PMSRC
 *    register. 0x5: Low level. Match occurs when there is a low level on the specified input. 0x6:
 *    Constant 0. This bit slice never contributes to a match (should be used to disable any unused bit
 *    slices). 0x7: Event. Non-sticky rising or falling edge. Match occurs on an event - i.e. when
 *    either a rising or falling edge is first detected on the specified input (this is a non-sticky
 *    version of value 0x3). This bit is cleared after one clock cycle.
 */
#define PINT_PMCFG_CFG0(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_CFG0_SHIFT)) & PINT_PMCFG_CFG0_MASK)
#define PINT_PMCFG_CFG1_MASK                     (0x3800U)
#define PINT_PMCFG_CFG1_SHIFT                    (11U)
/*! CFG1 - Specifies the match contribution condition for bit slice 1. See CFG0 for the modes available.
 */
#define PINT_PMCFG_CFG1(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_CFG1_SHIFT)) & PINT_PMCFG_CFG1_MASK)
#define PINT_PMCFG_CFG2_MASK                     (0x1C000U)
#define PINT_PMCFG_CFG2_SHIFT                    (14U)
/*! CFG2 - Specifies the match contribution condition for bit slice 2. See CFG0 for the modes available.
 */
#define PINT_PMCFG_CFG2(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_CFG2_SHIFT)) & PINT_PMCFG_CFG2_MASK)
#define PINT_PMCFG_CFG3_MASK                     (0xE0000U)
#define PINT_PMCFG_CFG3_SHIFT                    (17U)
/*! CFG3 - Specifies the match contribution condition for bit slice 3. See CFG0 for the modes available.
 */
#define PINT_PMCFG_CFG3(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_CFG3_SHIFT)) & PINT_PMCFG_CFG3_MASK)
#define PINT_PMCFG_CFG4_MASK                     (0x700000U)
#define PINT_PMCFG_CFG4_SHIFT                    (20U)
/*! CFG4 - Specifies the match contribution condition for bit slice 4. See CFG0 for the modes available.
 */
#define PINT_PMCFG_CFG4(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_CFG4_SHIFT)) & PINT_PMCFG_CFG4_MASK)
#define PINT_PMCFG_CFG5_MASK                     (0x3800000U)
#define PINT_PMCFG_CFG5_SHIFT                    (23U)
/*! CFG5 - Specifies the match contribution condition for bit slice 5. See CFG0 for the modes available.
 */
#define PINT_PMCFG_CFG5(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_CFG5_SHIFT)) & PINT_PMCFG_CFG5_MASK)
#define PINT_PMCFG_CFG6_MASK                     (0x1C000000U)
#define PINT_PMCFG_CFG6_SHIFT                    (26U)
/*! CFG6 - Specifies the match contribution condition for bit slice 6. See CFG0 for the modes available.
 */
#define PINT_PMCFG_CFG6(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_CFG6_SHIFT)) & PINT_PMCFG_CFG6_MASK)
#define PINT_PMCFG_CFG7_MASK                     (0xE0000000U)
#define PINT_PMCFG_CFG7_SHIFT                    (29U)
/*! CFG7 - Specifies the match contribution condition for bit slice 7. See CFG0 for the modes available.
 */
#define PINT_PMCFG_CFG7(x)                       (((uint32_t)(((uint32_t)(x)) << PINT_PMCFG_CFG7_SHIFT)) & PINT_PMCFG_CFG7_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group PINT_Register_Masks */


/* PINT - Peripheral instance base addresses */
/** Peripheral PINT base address */
#define PINT_BASE                                (0x40010000u)
/** Peripheral PINT base pointer */
#define PINT                                     ((PINT_Type *)PINT_BASE)
/** Array initializer of PINT peripheral base addresses */
#define PINT_BASE_ADDRS                          { PINT_BASE }
/** Array initializer of PINT peripheral base pointers */
#define PINT_BASE_PTRS                           { PINT }
/** Interrupt vectors for the PINT peripheral type */
#define PINT_IRQS                                { PIN_INT0_IRQn, PIN_INT1_IRQn, PIN_INT2_IRQn, PIN_INT3_IRQn }

/*!
 * @}
 */ /* end of group PINT_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- PMC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PMC_Peripheral_Access_Layer PMC Peripheral Access Layer
 * @{
 */

/** PMC - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< Power Management Control [Reset by POR, RSTN, WDT ], offset: 0x0 */
  __IO uint32_t DCDC0;                             /**< DCDC control register (1st). This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset], offset: 0x4 */
  __IO uint32_t DCDC1;                             /**< DCDC control register (2nd). This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset], offset: 0x8 */
  __IO uint32_t BIAS;                              /**< Bias current source control register. This reigster is controlled by the boot code and the Low power API software. [Reset by POR, RSTN, WDT ], offset: 0xC */
  __IO uint32_t LDOPMU;                            /**< PMU & Always On domains LDO control. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset], offset: 0x10 */
  __IO uint32_t LDOMEM;                            /**< Memories LDO control register. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset], offset: 0x14 */
  __IO uint32_t LDOCORE;                           /**< Digital Core LDO control register. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset], offset: 0x18 */
  __IO uint32_t LDOFLASHNV;                        /**< Flash LDO control register. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset], offset: 0x1C */
  __IO uint32_t LDOFLASHCORE;                      /**< Flash Core LDO control register. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset], offset: 0x20 */
  __IO uint32_t LDOADC;                            /**< General Purpose ADC LDO control register This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset], offset: 0x24 */
       uint8_t RESERVED_0[8];
  __IO uint32_t BODVBAT;                           /**< VBAT Brown Out Dectector control register This reigster is controlled by the boot code and the Low power API software. [Reset by POR, RSTN, WDT ], offset: 0x30 */
       uint8_t RESERVED_1[12];
  __IO uint32_t FRO192M;                           /**< High Speed FRO control register This reigster is controlled by the boot code and the Low power API software. [Reset by POR, RSTN, WDT ], offset: 0x40 */
  __IO uint32_t FRO1M;                             /**< 1 MHz Free Running Oscillator control register. [Reset by all reset sources, except ARM SystemReset], offset: 0x44 */
       uint8_t RESERVED_2[8];
  __IO uint32_t ANAMUXCOMP;                        /**< Analog Comparator and Analog Mux control register. [Reset by all reset sources, except ARM SystemReset], offset: 0x50 */
       uint8_t RESERVED_3[12];
  __I  uint32_t PWRSWACK;                          /**< Power Switch acknowledge. [Reset by all reset sources, except ARM SystemReset], offset: 0x60 */
  __IO uint32_t DPDWKSRC;                          /**< Power Down and Deep Power Down wake-up source. [Reset by POR, RSTN, WDT ], offset: 0x64 */
  __I  uint32_t STATUSPWR;                         /**< Power OK and Ready signals from various analog modules (DCDC, LDO, ). [Reset by all reset sources, except ARM SystemReset], offset: 0x68 */
  __I  uint32_t STATUSCLK;                         /**< FRO and XTAL status register. [Reset by all reset sources, except ARM SystemReset], offset: 0x6C */
  __IO uint32_t RESETCAUSE;                        /**< Reset Cause register. [Reset by POR], offset: 0x70 */
       uint8_t RESERVED_4[12];
  __IO uint32_t AOREG0;                            /**< General purpose always on domain data storage. [Reset by all reset sources, except ARM SystemReset], offset: 0x80 */
  __IO uint32_t AOREG1;                            /**< General purpose always on domain data storage. [Reset by POR, RSTN], offset: 0x84 */
  __IO uint32_t AOREG2;                            /**< General purpose always on domain data storage. [Reset by POR, RSTN], offset: 0x88 */
       uint8_t RESERVED_5[12];
  __IO uint32_t DPDCTRL;                           /**< Configuration parameters for Power Down and Deep Power Down mode. [Reset by POR, RSTN, WDT ], offset: 0x98 */
  __I  uint32_t PIOPORCAP;                         /**< The PIOPORCAP register captures the state of GPIO at power-on-reset or pin reset. Each bit represents the power-on reset state of one GPIO pin. [Reset by POR, RSTN], offset: 0x9C */
  __I  uint32_t PIORESCAP;                         /**< The PIORESCAP0 register captures the state of GPIO port 0 when a reset other than a power-on reset or pin reset occurs. Each bit represents the reset state of one GPIO pin. [Reset by WDT, BOD, WAKEUP IO, ARM System reset ], offset: 0xA0 */
       uint8_t RESERVED_6[12];
  __IO uint32_t PDSLEEPCFG;                        /**< Controls the power to various modules in Low Power modes. [Reset by all reset sources, except ARM SystemReset], offset: 0xB0 */
       uint8_t RESERVED_7[4];
  __IO uint32_t PDRUNCFG;                          /**< Controls the power to various analog blocks. [Reset by all reset sources, except ARM SystemReset], offset: 0xB8 */
  __I  uint32_t WAKEIOCAUSE;                       /**< Wake-up source from Power Down and Deep Power Down modes. Allow to identify the Wake-up source from Power-Down mode or Deep Power Down mode.[Reset by POR, RSTN, WDT ], offset: 0xBC */
       uint8_t RESERVED_8[12];
  __IO uint32_t CTRLNORST;                         /**< Extension of CTRL register, but never reset except by POR, offset: 0xCC */
} PMC_Type;

/* ----------------------------------------------------------------------------
   -- PMC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PMC_Register_Masks PMC Register Masks
 * @{
 */

/*! @name CTRL - Power Management Control [Reset by POR, RSTN, WDT ] */
/*! @{ */
#define PMC_CTRL_LPMODE_MASK                     (0x3U)
#define PMC_CTRL_LPMODE_SHIFT                    (0U)
/*! LPMODE - Power Mode Control. 00: Active; 01: Deep Sleep; 10: Power Down; 11: Deep Power Down.
 */
#define PMC_CTRL_LPMODE(x)                       (((uint32_t)(((uint32_t)(x)) << PMC_CTRL_LPMODE_SHIFT)) & PMC_CTRL_LPMODE_MASK)
#define PMC_CTRL_SYSTEMRESETENABLE_MASK          (0x4U)
#define PMC_CTRL_SYSTEMRESETENABLE_SHIFT         (2U)
/*! SYSTEMRESETENABLE - ARM system reset request enable. If set enables the ARM system reset to affect the system.
 */
#define PMC_CTRL_SYSTEMRESETENABLE(x)            (((uint32_t)(((uint32_t)(x)) << PMC_CTRL_SYSTEMRESETENABLE_SHIFT)) & PMC_CTRL_SYSTEMRESETENABLE_MASK)
#define PMC_CTRL_WDTRESETENABLE_MASK             (0x8U)
#define PMC_CTRL_WDTRESETENABLE_SHIFT            (3U)
/*! WDTRESETENABLE - Watchdog Timer reset enable. If set allow a watchdog timer reset event to affect the system.
 */
#define PMC_CTRL_WDTRESETENABLE(x)               (((uint32_t)(((uint32_t)(x)) << PMC_CTRL_WDTRESETENABLE_SHIFT)) & PMC_CTRL_WDTRESETENABLE_MASK)
#define PMC_CTRL_WAKUPRESETENABLE_MASK           (0x10U)
#define PMC_CTRL_WAKUPRESETENABLE_SHIFT          (4U)
/*! WAKUPRESETENABLE - Wake-up I/Os reset enable. When set, the I/O power domain is not shutoff in deep powerdown mode.
 */
#define PMC_CTRL_WAKUPRESETENABLE(x)             (((uint32_t)(((uint32_t)(x)) << PMC_CTRL_WAKUPRESETENABLE_SHIFT)) & PMC_CTRL_WAKUPRESETENABLE_MASK)
#define PMC_CTRL_NTAGWAKUPRESETENABLE_MASK       (0x20U)
#define PMC_CTRL_NTAGWAKUPRESETENABLE_SHIFT      (5U)
/*! NTAGWAKUPRESETENABLE - Wake-up NTAG reset enable. When set, the device can wake from deep power
 *    down by edge on NTAG FD signal, even if I/O power domain is off (see WAKUPRESETENABLE). Note
 *    that if I/O power domain is ON, wake-up by NTAG FD is enabled by default thus content of this
 *    bit does not care. Do not set unless entering Deep Power Down.
 */
#define PMC_CTRL_NTAGWAKUPRESETENABLE(x)         (((uint32_t)(((uint32_t)(x)) << PMC_CTRL_NTAGWAKUPRESETENABLE_SHIFT)) & PMC_CTRL_NTAGWAKUPRESETENABLE_MASK)
#define PMC_CTRL_SELLDOVOLTAGE_MASK              (0x100U)
#define PMC_CTRL_SELLDOVOLTAGE_SHIFT             (8U)
/*! SELLDOVOLTAGE - 0 = all LDOs current output levels are determined by their associated VADJ
 *    bitfield. 1 = all LDOs current output levels are determined by their associated VADJ_2 bitfield.
 */
#define PMC_CTRL_SELLDOVOLTAGE(x)                (((uint32_t)(((uint32_t)(x)) << PMC_CTRL_SELLDOVOLTAGE_SHIFT)) & PMC_CTRL_SELLDOVOLTAGE_MASK)
#define PMC_CTRL_SWRRESETENABLE_MASK             (0x200U)
#define PMC_CTRL_SWRRESETENABLE_SHIFT            (9U)
/*! SWRRESETENABLE - Software reset enable. If set enables the software reset to affect the system.
 */
#define PMC_CTRL_SWRRESETENABLE(x)               (((uint32_t)(((uint32_t)(x)) << PMC_CTRL_SWRRESETENABLE_SHIFT)) & PMC_CTRL_SWRRESETENABLE_MASK)
/*! @} */

/*! @name DCDC0 - DCDC control register (1st). This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_DCDC0_DCDC0_MASK                     (0xFFFFFFFFU)
#define PMC_DCDC0_DCDC0_SHIFT                    (0U)
/*! DCDC0 - DCDC control register (1st). This reigster is controlled by the boot code and the Low
 *    power API software. [Reset by all reset sources, except ARM SystemReset]
 */
#define PMC_DCDC0_DCDC0(x)                       (((uint32_t)(((uint32_t)(x)) << PMC_DCDC0_DCDC0_SHIFT)) & PMC_DCDC0_DCDC0_MASK)
/*! @} */

/*! @name DCDC1 - DCDC control register (2nd). This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_DCDC1_DCDC1_MASK                     (0xFFFFFFFFU)
#define PMC_DCDC1_DCDC1_SHIFT                    (0U)
/*! DCDC1 - DCDC control register (2nd). This reigster is controlled by the boot code and the Low
 *    power API software. [Reset by all reset sources, except ARM SystemReset]
 */
#define PMC_DCDC1_DCDC1(x)                       (((uint32_t)(((uint32_t)(x)) << PMC_DCDC1_DCDC1_SHIFT)) & PMC_DCDC1_DCDC1_MASK)
/*! @} */

/*! @name BIAS - Bias current source control register. This reigster is controlled by the boot code and the Low power API software. [Reset by POR, RSTN, WDT ] */
/*! @{ */
#define PMC_BIAS_BIAS_MASK                       (0xFFFFFFFFU)
#define PMC_BIAS_BIAS_SHIFT                      (0U)
/*! BIAS - Bias current source control register. This reigster is controlled by the boot code and
 *    the Low power API software. [Reset by POR, RSTN, WDT ]
 */
#define PMC_BIAS_BIAS(x)                         (((uint32_t)(((uint32_t)(x)) << PMC_BIAS_BIAS_SHIFT)) & PMC_BIAS_BIAS_MASK)
/*! @} */

/*! @name LDOPMU - PMU & Always On domains LDO control. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_LDOPMU_LDOPMU_MASK                   (0xFFFFFFFFU)
#define PMC_LDOPMU_LDOPMU_SHIFT                  (0U)
/*! LDOPMU - PMU & Always On domains LDO control. This reigster is controlled by the boot code and
 *    the Low power API software. [Reset by all reset sources, except ARM SystemReset]
 */
#define PMC_LDOPMU_LDOPMU(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_LDOPMU_LDOPMU_SHIFT)) & PMC_LDOPMU_LDOPMU_MASK)
/*! @} */

/*! @name LDOMEM - Memories LDO control register. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_LDOMEM_LDOMEM_MASK                   (0xFFFFFFFFU)
#define PMC_LDOMEM_LDOMEM_SHIFT                  (0U)
/*! LDOMEM - Memories LDO control register. This reigster is controlled by the boot code and the Low
 *    power API software. [Reset by all reset sources, except ARM SystemReset]
 */
#define PMC_LDOMEM_LDOMEM(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_LDOMEM_LDOMEM_SHIFT)) & PMC_LDOMEM_LDOMEM_MASK)
/*! @} */

/*! @name LDOCORE - Digital Core LDO control register. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_LDOCORE_LDOCORE_MASK                 (0xFFFFFFFFU)
#define PMC_LDOCORE_LDOCORE_SHIFT                (0U)
/*! LDOCORE - Digital Core LDO control register. This reigster is controlled by the boot code and
 *    the Low power API software. [Reset by all reset sources, except ARM SystemReset]
 */
#define PMC_LDOCORE_LDOCORE(x)                   (((uint32_t)(((uint32_t)(x)) << PMC_LDOCORE_LDOCORE_SHIFT)) & PMC_LDOCORE_LDOCORE_MASK)
/*! @} */

/*! @name LDOFLASHNV - Flash LDO control register. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_LDOFLASHNV_LDOFLASHNV_MASK           (0xFFFFFFFFU)
#define PMC_LDOFLASHNV_LDOFLASHNV_SHIFT          (0U)
/*! LDOFLASHNV - Flash LDO control register. This reigster is controlled by the boot code and the
 *    Low power API software. [Reset by all reset sources, except ARM SystemReset]
 */
#define PMC_LDOFLASHNV_LDOFLASHNV(x)             (((uint32_t)(((uint32_t)(x)) << PMC_LDOFLASHNV_LDOFLASHNV_SHIFT)) & PMC_LDOFLASHNV_LDOFLASHNV_MASK)
/*! @} */

/*! @name LDOFLASHCORE - Flash Core LDO control register. This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_LDOFLASHCORE_LDOFLASHCORE_MASK       (0xFFFFFFFFU)
#define PMC_LDOFLASHCORE_LDOFLASHCORE_SHIFT      (0U)
/*! LDOFLASHCORE - Flash Core LDO control register. This reigster is controlled by the boot code and
 *    the Low power API software. [Reset by all reset sources, except ARM SystemReset]
 */
#define PMC_LDOFLASHCORE_LDOFLASHCORE(x)         (((uint32_t)(((uint32_t)(x)) << PMC_LDOFLASHCORE_LDOFLASHCORE_SHIFT)) & PMC_LDOFLASHCORE_LDOFLASHCORE_MASK)
/*! @} */

/*! @name LDOADC - General Purpose ADC LDO control register This reigster is controlled by the boot code and the Low power API software. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_LDOADC_LDOADC_MASK                   (0xFFFFFFFFU)
#define PMC_LDOADC_LDOADC_SHIFT                  (0U)
/*! LDOADC - General Purpose ADC LDO control register This reigster is controlled by the boot code
 *    and the Low power API software. [Reset by all reset sources, except ARM SystemReset]
 */
#define PMC_LDOADC_LDOADC(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_LDOADC_LDOADC_SHIFT)) & PMC_LDOADC_LDOADC_MASK)
/*! @} */

/*! @name BODVBAT - VBAT Brown Out Dectector control register This reigster is controlled by the boot code and the Low power API software. [Reset by POR, RSTN, WDT ] */
/*! @{ */
#define PMC_BODVBAT_TRIGLVL_MASK                 (0x1FU)
#define PMC_BODVBAT_TRIGLVL_SHIFT                (0U)
/*! TRIGLVL - BOD trigger level
 */
#define PMC_BODVBAT_TRIGLVL(x)                   (((uint32_t)(((uint32_t)(x)) << PMC_BODVBAT_TRIGLVL_SHIFT)) & PMC_BODVBAT_TRIGLVL_MASK)
#define PMC_BODVBAT_HYST_MASK                    (0x60U)
#define PMC_BODVBAT_HYST_SHIFT                   (5U)
/*! HYST - BOD Hysteresis control
 */
#define PMC_BODVBAT_HYST(x)                      (((uint32_t)(((uint32_t)(x)) << PMC_BODVBAT_HYST_SHIFT)) & PMC_BODVBAT_HYST_MASK)
/*! @} */

/*! @name FRO192M - High Speed FRO control register This reigster is controlled by the boot code and the Low power API software. [Reset by POR, RSTN, WDT ] */
/*! @{ */
#define PMC_FRO192M_DIVSEL_MASK                  (0x1F00000U)
#define PMC_FRO192M_DIVSEL_SHIFT                 (20U)
/*! DIVSEL - Mode of operation (which clock to output). Each bit enables a clocks as shown. Enables
 *    are additive meaning that two or more clocks can be enabled together. xxxx1: 12MHz enabled;
 *    xxx1x: 32MHz enabled; xx1xx: 48MHz enabled; x1xxx: Not applicable; 1xxxx: Not applicable.
 */
#define PMC_FRO192M_DIVSEL(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_FRO192M_DIVSEL_SHIFT)) & PMC_FRO192M_DIVSEL_MASK)
/*! @} */

/*! @name FRO1M - 1 MHz Free Running Oscillator control register. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_FRO1M_FREQSEL_MASK                   (0x7FU)
#define PMC_FRO1M_FREQSEL_SHIFT                  (0U)
/*! FREQSEL - Frequency trimming bits. This field is used to give accurate frequency for each
 *    device. The required setting is based upon calibration data sotred in flash during device test. This
 *    setting is applied by the clock driver function.
 */
#define PMC_FRO1M_FREQSEL(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_FRO1M_FREQSEL_SHIFT)) & PMC_FRO1M_FREQSEL_MASK)
#define PMC_FRO1M_ATBCTRL_MASK                   (0x180U)
#define PMC_FRO1M_ATBCTRL_SHIFT                  (7U)
/*! ATBCTRL - Debug control bits to set the analog/digital test modes; only required for test purposes.
 */
#define PMC_FRO1M_ATBCTRL(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_FRO1M_ATBCTRL_SHIFT)) & PMC_FRO1M_ATBCTRL_MASK)
#define PMC_FRO1M_DIVSEL_MASK                    (0x3E00U)
#define PMC_FRO1M_DIVSEL_SHIFT                   (9U)
/*! DIVSEL - Divider selection bits.
 */
#define PMC_FRO1M_DIVSEL(x)                      (((uint32_t)(((uint32_t)(x)) << PMC_FRO1M_DIVSEL_SHIFT)) & PMC_FRO1M_DIVSEL_MASK)
/*! @} */

/*! @name ANAMUXCOMP - Analog Comparator and Analog Mux control register. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_ANAMUXCOMP_COMP_HYST_MASK            (0x2U)
#define PMC_ANAMUXCOMP_COMP_HYST_SHIFT           (1U)
/*! COMP_HYST - Hysteris enabled in comparator when hyst = '1', no hysteresis when = '0'.
 */
#define PMC_ANAMUXCOMP_COMP_HYST(x)              (((uint32_t)(((uint32_t)(x)) << PMC_ANAMUXCOMP_COMP_HYST_SHIFT)) & PMC_ANAMUXCOMP_COMP_HYST_MASK)
#define PMC_ANAMUXCOMP_COMP_INNINT_MASK          (0x4U)
#define PMC_ANAMUXCOMP_COMP_INNINT_SHIFT         (2U)
/*! COMP_INNINT - Voltage reference inn_int input is selected for _n comparator input when
 *    sel_inn_int = 1 . This setting also requires PMU_BIASING to be active. Also flash biasing and DCDC
 *    converter needs to be enabled. If this setting = '0' then _n input comes from device pins, based
 *    on COMP_INPUTSWAP setting.
 */
#define PMC_ANAMUXCOMP_COMP_INNINT(x)            (((uint32_t)(((uint32_t)(x)) << PMC_ANAMUXCOMP_COMP_INNINT_SHIFT)) & PMC_ANAMUXCOMP_COMP_INNINT_MASK)
#define PMC_ANAMUXCOMP_COMP_LOWPOWER_MASK        (0x8U)
#define PMC_ANAMUXCOMP_COMP_LOWPOWER_SHIFT       (3U)
/*! COMP_LOWPOWER - Comparator Low power mode enabled when set.
 */
#define PMC_ANAMUXCOMP_COMP_LOWPOWER(x)          (((uint32_t)(((uint32_t)(x)) << PMC_ANAMUXCOMP_COMP_LOWPOWER_SHIFT)) & PMC_ANAMUXCOMP_COMP_LOWPOWER_MASK)
#define PMC_ANAMUXCOMP_COMP_INPUTSWAP_MASK       (0x10U)
#define PMC_ANAMUXCOMP_COMP_INPUTSWAP_SHIFT      (4U)
/*! COMP_INPUTSWAP - Input swap is enabled when set. Comparator{ _p, _n} ports are connected to
 *    {ACM, ACP}. Otherwsie normal configuration occurs, {_p, _n} is connected to {ACP, ACM} .
 */
#define PMC_ANAMUXCOMP_COMP_INPUTSWAP(x)         (((uint32_t)(((uint32_t)(x)) << PMC_ANAMUXCOMP_COMP_INPUTSWAP_SHIFT)) & PMC_ANAMUXCOMP_COMP_INPUTSWAP_MASK)
/*! @} */

/*! @name PWRSWACK - Power Switch acknowledge. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_PWRSWACK_PDCOMM0_MASK                (0x2U)
#define PMC_PWRSWACK_PDCOMM0_SHIFT               (1U)
/*! PDCOMM0 - Comm0 (USART0, I2C0, SPI0) Power Domain power switch status.
 */
#define PMC_PWRSWACK_PDCOMM0(x)                  (((uint32_t)(((uint32_t)(x)) << PMC_PWRSWACK_PDCOMM0_SHIFT)) & PMC_PWRSWACK_PDCOMM0_MASK)
#define PMC_PWRSWACK_PDSYSTEM_MASK               (0x4U)
#define PMC_PWRSWACK_PDSYSTEM_SHIFT              (2U)
/*! PDSYSTEM - System Power Domain power switch status.
 */
#define PMC_PWRSWACK_PDSYSTEM(x)                 (((uint32_t)(((uint32_t)(x)) << PMC_PWRSWACK_PDSYSTEM_SHIFT)) & PMC_PWRSWACK_PDSYSTEM_MASK)
#define PMC_PWRSWACK_PDMCURETENTION_MASK         (0x8U)
#define PMC_PWRSWACK_PDMCURETENTION_SHIFT        (3U)
/*! PDMCURETENTION - MCU Retention Power Domain power switch status.
 */
#define PMC_PWRSWACK_PDMCURETENTION(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PWRSWACK_PDMCURETENTION_SHIFT)) & PMC_PWRSWACK_PDMCURETENTION_MASK)
/*! @} */

/*! @name DPDWKSRC - Power Down and Deep Power Down wake-up source. [Reset by POR, RSTN, WDT ] */
/*! @{ */
#define PMC_DPDWKSRC_PIO0_MASK                   (0x1U)
#define PMC_DPDWKSRC_PIO0_SHIFT                  (0U)
/*! PIO0 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO0: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO0(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO0_SHIFT)) & PMC_DPDWKSRC_PIO0_MASK)
#define PMC_DPDWKSRC_PIO1_MASK                   (0x2U)
#define PMC_DPDWKSRC_PIO1_SHIFT                  (1U)
/*! PIO1 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO1: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO1(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO1_SHIFT)) & PMC_DPDWKSRC_PIO1_MASK)
#define PMC_DPDWKSRC_PIO2_MASK                   (0x4U)
#define PMC_DPDWKSRC_PIO2_SHIFT                  (2U)
/*! PIO2 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO2: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO2(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO2_SHIFT)) & PMC_DPDWKSRC_PIO2_MASK)
#define PMC_DPDWKSRC_PIO3_MASK                   (0x8U)
#define PMC_DPDWKSRC_PIO3_SHIFT                  (3U)
/*! PIO3 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO3: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO3(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO3_SHIFT)) & PMC_DPDWKSRC_PIO3_MASK)
#define PMC_DPDWKSRC_PIO4_MASK                   (0x10U)
#define PMC_DPDWKSRC_PIO4_SHIFT                  (4U)
/*! PIO4 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO4: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO4(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO4_SHIFT)) & PMC_DPDWKSRC_PIO4_MASK)
#define PMC_DPDWKSRC_PIO5_MASK                   (0x20U)
#define PMC_DPDWKSRC_PIO5_SHIFT                  (5U)
/*! PIO5 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO5: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO5(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO5_SHIFT)) & PMC_DPDWKSRC_PIO5_MASK)
#define PMC_DPDWKSRC_PIO6_MASK                   (0x40U)
#define PMC_DPDWKSRC_PIO6_SHIFT                  (6U)
/*! PIO6 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO6: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO6(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO6_SHIFT)) & PMC_DPDWKSRC_PIO6_MASK)
#define PMC_DPDWKSRC_PIO7_MASK                   (0x80U)
#define PMC_DPDWKSRC_PIO7_SHIFT                  (7U)
/*! PIO7 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO7: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO7(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO7_SHIFT)) & PMC_DPDWKSRC_PIO7_MASK)
#define PMC_DPDWKSRC_PIO8_MASK                   (0x100U)
#define PMC_DPDWKSRC_PIO8_SHIFT                  (8U)
/*! PIO8 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO8: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO8(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO8_SHIFT)) & PMC_DPDWKSRC_PIO8_MASK)
#define PMC_DPDWKSRC_PIO9_MASK                   (0x200U)
#define PMC_DPDWKSRC_PIO9_SHIFT                  (9U)
/*! PIO9 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO9: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO9(x)                     (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO9_SHIFT)) & PMC_DPDWKSRC_PIO9_MASK)
#define PMC_DPDWKSRC_PIO10_MASK                  (0x400U)
#define PMC_DPDWKSRC_PIO10_SHIFT                 (10U)
/*! PIO10 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO10: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO10(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO10_SHIFT)) & PMC_DPDWKSRC_PIO10_MASK)
#define PMC_DPDWKSRC_PIO11_MASK                  (0x800U)
#define PMC_DPDWKSRC_PIO11_SHIFT                 (11U)
/*! PIO11 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO11: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO11(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO11_SHIFT)) & PMC_DPDWKSRC_PIO11_MASK)
#define PMC_DPDWKSRC_PIO12_MASK                  (0x1000U)
#define PMC_DPDWKSRC_PIO12_SHIFT                 (12U)
/*! PIO12 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO12: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO12(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO12_SHIFT)) & PMC_DPDWKSRC_PIO12_MASK)
#define PMC_DPDWKSRC_PIO13_MASK                  (0x2000U)
#define PMC_DPDWKSRC_PIO13_SHIFT                 (13U)
/*! PIO13 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO13: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO13(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO13_SHIFT)) & PMC_DPDWKSRC_PIO13_MASK)
#define PMC_DPDWKSRC_PIO14_MASK                  (0x4000U)
#define PMC_DPDWKSRC_PIO14_SHIFT                 (14U)
/*! PIO14 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO14: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO14(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO14_SHIFT)) & PMC_DPDWKSRC_PIO14_MASK)
#define PMC_DPDWKSRC_PIO15_MASK                  (0x8000U)
#define PMC_DPDWKSRC_PIO15_SHIFT                 (15U)
/*! PIO15 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO15: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO15(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO15_SHIFT)) & PMC_DPDWKSRC_PIO15_MASK)
#define PMC_DPDWKSRC_PIO16_MASK                  (0x10000U)
#define PMC_DPDWKSRC_PIO16_SHIFT                 (16U)
/*! PIO16 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO16: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO16(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO16_SHIFT)) & PMC_DPDWKSRC_PIO16_MASK)
#define PMC_DPDWKSRC_PIO17_MASK                  (0x20000U)
#define PMC_DPDWKSRC_PIO17_SHIFT                 (17U)
/*! PIO17 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO17: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO17(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO17_SHIFT)) & PMC_DPDWKSRC_PIO17_MASK)
#define PMC_DPDWKSRC_PIO18_MASK                  (0x40000U)
#define PMC_DPDWKSRC_PIO18_SHIFT                 (18U)
/*! PIO18 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO18: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO18(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO18_SHIFT)) & PMC_DPDWKSRC_PIO18_MASK)
#define PMC_DPDWKSRC_PIO19_MASK                  (0x80000U)
#define PMC_DPDWKSRC_PIO19_SHIFT                 (19U)
/*! PIO19 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO19: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO19(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO19_SHIFT)) & PMC_DPDWKSRC_PIO19_MASK)
#define PMC_DPDWKSRC_PIO20_MASK                  (0x100000U)
#define PMC_DPDWKSRC_PIO20_SHIFT                 (20U)
/*! PIO20 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO20: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO20(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO20_SHIFT)) & PMC_DPDWKSRC_PIO20_MASK)
#define PMC_DPDWKSRC_PIO21_MASK                  (0x200000U)
#define PMC_DPDWKSRC_PIO21_SHIFT                 (21U)
/*! PIO21 - Enable / disable wakeup from Power down and Deep Power Down modes by GPIO21: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_PIO21(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_PIO21_SHIFT)) & PMC_DPDWKSRC_PIO21_MASK)
#define PMC_DPDWKSRC_NTAG_FD_MASK                (0x400000U)
#define PMC_DPDWKSRC_NTAG_FD_SHIFT               (22U)
/*! NTAG_FD - Enable / disable wakeup from Power down and Deep Power Down modes by NTAG_FD: 0: Disable; 1: Enable.
 */
#define PMC_DPDWKSRC_NTAG_FD(x)                  (((uint32_t)(((uint32_t)(x)) << PMC_DPDWKSRC_NTAG_FD_SHIFT)) & PMC_DPDWKSRC_NTAG_FD_MASK)
/*! @} */

/*! @name STATUSPWR - Power OK and Ready signals from various analog modules (DCDC, LDO, ). [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_STATUSPWR_DCDCPWROK_MASK             (0x1U)
#define PMC_STATUSPWR_DCDCPWROK_SHIFT            (0U)
/*! DCDCPWROK - DCDC converter power OK
 */
#define PMC_STATUSPWR_DCDCPWROK(x)               (((uint32_t)(((uint32_t)(x)) << PMC_STATUSPWR_DCDCPWROK_SHIFT)) & PMC_STATUSPWR_DCDCPWROK_MASK)
#define PMC_STATUSPWR_DCDCVXCTRLMON_MASK         (0x2U)
#define PMC_STATUSPWR_DCDCVXCTRLMON_SHIFT        (1U)
/*! DCDCVXCTRLMON - Picture of the DCDC output state.
 */
#define PMC_STATUSPWR_DCDCVXCTRLMON(x)           (((uint32_t)(((uint32_t)(x)) << PMC_STATUSPWR_DCDCVXCTRLMON_SHIFT)) & PMC_STATUSPWR_DCDCVXCTRLMON_MASK)
#define PMC_STATUSPWR_LDOCOREPWROK_MASK          (0x4U)
#define PMC_STATUSPWR_LDOCOREPWROK_SHIFT         (2U)
/*! LDOCOREPWROK - CORE LDO power OK. Max switch on time 2us.
 */
#define PMC_STATUSPWR_LDOCOREPWROK(x)            (((uint32_t)(((uint32_t)(x)) << PMC_STATUSPWR_LDOCOREPWROK_SHIFT)) & PMC_STATUSPWR_LDOCOREPWROK_MASK)
#define PMC_STATUSPWR_LDOFLASHNVPWROK_MASK       (0x8U)
#define PMC_STATUSPWR_LDOFLASHNVPWROK_SHIFT      (3U)
/*! LDOFLASHNVPWROK - Flash NV LDO power OK Max switch on time 20us.
 */
#define PMC_STATUSPWR_LDOFLASHNVPWROK(x)         (((uint32_t)(((uint32_t)(x)) << PMC_STATUSPWR_LDOFLASHNVPWROK_SHIFT)) & PMC_STATUSPWR_LDOFLASHNVPWROK_MASK)
#define PMC_STATUSPWR_LDOFLASHCOREPWROK_MASK     (0x10U)
#define PMC_STATUSPWR_LDOFLASHCOREPWROK_SHIFT    (4U)
/*! LDOFLASHCOREPWROK - Flash Core LDO power OK Max switch on time should be considered as 10us.
 */
#define PMC_STATUSPWR_LDOFLASHCOREPWROK(x)       (((uint32_t)(((uint32_t)(x)) << PMC_STATUSPWR_LDOFLASHCOREPWROK_SHIFT)) & PMC_STATUSPWR_LDOFLASHCOREPWROK_MASK)
#define PMC_STATUSPWR_LDOADC1V1PWROK_MASK        (0x20U)
#define PMC_STATUSPWR_LDOADC1V1PWROK_SHIFT       (5U)
/*! LDOADC1V1PWROK - General Purpose ADC LDO power OK. Max switch on time is 8us.
 */
#define PMC_STATUSPWR_LDOADC1V1PWROK(x)          (((uint32_t)(((uint32_t)(x)) << PMC_STATUSPWR_LDOADC1V1PWROK_SHIFT)) & PMC_STATUSPWR_LDOADC1V1PWROK_MASK)
/*! @} */

/*! @name STATUSCLK - FRO and XTAL status register. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_STATUSCLK_FRO192MCLKVALID_MASK       (0x1U)
#define PMC_STATUSCLK_FRO192MCLKVALID_SHIFT      (0U)
/*! FRO192MCLKVALID - High Speed FRO (FRO 192 MHz) clock valid signal. The FRO192M clock generator
 *    also generates the FRO12M, FRO32M and FRO48M clock signals. These will also be valid when this
 *    flag is assertetd.
 */
#define PMC_STATUSCLK_FRO192MCLKVALID(x)         (((uint32_t)(((uint32_t)(x)) << PMC_STATUSCLK_FRO192MCLKVALID_SHIFT)) & PMC_STATUSCLK_FRO192MCLKVALID_MASK)
#define PMC_STATUSCLK_XTAL32KOK_MASK             (0x2U)
#define PMC_STATUSCLK_XTAL32KOK_SHIFT            (1U)
/*! XTAL32KOK - XTAL oscillator 32KHz OK signal. When the XTAL is stable, then a transition from 1
 *    to 0 will indicate a clock issue. Can not be used to identify a stable clock during XTAL start.
 */
#define PMC_STATUSCLK_XTAL32KOK(x)               (((uint32_t)(((uint32_t)(x)) << PMC_STATUSCLK_XTAL32KOK_SHIFT)) & PMC_STATUSCLK_XTAL32KOK_MASK)
#define PMC_STATUSCLK_FRO1MCLKVALID_MASK         (0x4U)
#define PMC_STATUSCLK_FRO1MCLKVALID_SHIFT        (2U)
/*! FRO1MCLKVALID - FRO 1 MHz CCO voltage detector output.
 */
#define PMC_STATUSCLK_FRO1MCLKVALID(x)           (((uint32_t)(((uint32_t)(x)) << PMC_STATUSCLK_FRO1MCLKVALID_SHIFT)) & PMC_STATUSCLK_FRO1MCLKVALID_MASK)
/*! @} */

/*! @name RESETCAUSE - Reset Cause register. [Reset by POR] */
/*! @{ */
#define PMC_RESETCAUSE_POR_MASK                  (0x1U)
#define PMC_RESETCAUSE_POR_SHIFT                 (0U)
/*! POR - 1 : The last chip reset was caused by a Power On Reset. Write '1' to clear this bit.
 */
#define PMC_RESETCAUSE_POR(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_RESETCAUSE_POR_SHIFT)) & PMC_RESETCAUSE_POR_MASK)
#define PMC_RESETCAUSE_PADRESET_MASK             (0x2U)
#define PMC_RESETCAUSE_PADRESET_SHIFT            (1U)
/*! PADRESET - 1 : The last chip reset was caused by a Pad Reset. Write '1' to clear this bit.
 */
#define PMC_RESETCAUSE_PADRESET(x)               (((uint32_t)(((uint32_t)(x)) << PMC_RESETCAUSE_PADRESET_SHIFT)) & PMC_RESETCAUSE_PADRESET_MASK)
#define PMC_RESETCAUSE_BODRESET_MASK             (0x4U)
#define PMC_RESETCAUSE_BODRESET_SHIFT            (2U)
/*! BODRESET - 1 : The last chip reset was caused by a Brown Out Detector. Write '1' to clear this bit.
 */
#define PMC_RESETCAUSE_BODRESET(x)               (((uint32_t)(((uint32_t)(x)) << PMC_RESETCAUSE_BODRESET_SHIFT)) & PMC_RESETCAUSE_BODRESET_MASK)
#define PMC_RESETCAUSE_SYSTEMRESET_MASK          (0x8U)
#define PMC_RESETCAUSE_SYSTEMRESET_SHIFT         (3U)
/*! SYSTEMRESET - 1 : The last chip reset was caused by a System Reset requested by the ARM CPU. Write '1' to clear this bit.
 */
#define PMC_RESETCAUSE_SYSTEMRESET(x)            (((uint32_t)(((uint32_t)(x)) << PMC_RESETCAUSE_SYSTEMRESET_SHIFT)) & PMC_RESETCAUSE_SYSTEMRESET_MASK)
#define PMC_RESETCAUSE_WDTRESET_MASK             (0x10U)
#define PMC_RESETCAUSE_WDTRESET_SHIFT            (4U)
/*! WDTRESET - 1 : The last chip reset was caused by the Watchdog Timer. Write '1' to clear this bit.
 */
#define PMC_RESETCAUSE_WDTRESET(x)               (((uint32_t)(((uint32_t)(x)) << PMC_RESETCAUSE_WDTRESET_SHIFT)) & PMC_RESETCAUSE_WDTRESET_MASK)
#define PMC_RESETCAUSE_WAKEUPIORESET_MASK        (0x20U)
#define PMC_RESETCAUSE_WAKEUPIORESET_SHIFT       (5U)
/*! WAKEUPIORESET - 1 : The last chip reset was caused by a Wake-up I/O (GPIO or internal NTAG FD INT). Write '1' to clear this bit.
 */
#define PMC_RESETCAUSE_WAKEUPIORESET(x)          (((uint32_t)(((uint32_t)(x)) << PMC_RESETCAUSE_WAKEUPIORESET_SHIFT)) & PMC_RESETCAUSE_WAKEUPIORESET_MASK)
#define PMC_RESETCAUSE_WAKEUPPWDNRESET_MASK      (0x40U)
#define PMC_RESETCAUSE_WAKEUPPWDNRESET_SHIFT     (6U)
/*! WAKEUPPWDNRESET - 1 : The last CPU reset was caused by a Wake-up from Power down (many sources
 *    possible: timer, IO, ...). Write '1' to clear this bit. Check NVIC register if not waken-up by
 *    IO (NVIC_GetPendingIRQ).
 */
#define PMC_RESETCAUSE_WAKEUPPWDNRESET(x)        (((uint32_t)(((uint32_t)(x)) << PMC_RESETCAUSE_WAKEUPPWDNRESET_SHIFT)) & PMC_RESETCAUSE_WAKEUPPWDNRESET_MASK)
#define PMC_RESETCAUSE_SWRRESET_MASK             (0x80U)
#define PMC_RESETCAUSE_SWRRESET_SHIFT            (7U)
/*! SWRRESET - 1 : The last chip reset was caused by a Software. Write '1' to clear this bit.
 */
#define PMC_RESETCAUSE_SWRRESET(x)               (((uint32_t)(((uint32_t)(x)) << PMC_RESETCAUSE_SWRRESET_SHIFT)) & PMC_RESETCAUSE_SWRRESET_MASK)
/*! @} */

/*! @name AOREG0 - General purpose always on domain data storage. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_AOREG0_DATA31_0_MASK                 (0xFFFFFFFFU)
#define PMC_AOREG0_DATA31_0_SHIFT                (0U)
/*! DATA31_0 - General purpose always on domain data storage. Only writable 1 time after any chip
 *    reset. After the 1st write, any further writes are blocked. After any chip reset the write block
 *    is disabled until after next write. The chip reset includes POR, RSTIN, WDT reset, SW reset
 *    and WAKEUP IO reset.
 */
#define PMC_AOREG0_DATA31_0(x)                   (((uint32_t)(((uint32_t)(x)) << PMC_AOREG0_DATA31_0_SHIFT)) & PMC_AOREG0_DATA31_0_MASK)
/*! @} */

/*! @name AOREG1 - General purpose always on domain data storage. [Reset by POR, RSTN] */
/*! @{ */
#define PMC_AOREG1_DATA31_0_MASK                 (0xFFFFFFFFU)
#define PMC_AOREG1_DATA31_0_SHIFT                (0U)
/*! DATA31_0 - Reserved for use by NXP system software. General purpose always on domain data
 *    storage. Only reinitialized on Power On Reset and RSTIN Pin reset.
 */
#define PMC_AOREG1_DATA31_0(x)                   (((uint32_t)(((uint32_t)(x)) << PMC_AOREG1_DATA31_0_SHIFT)) & PMC_AOREG1_DATA31_0_MASK)
/*! @} */

/*! @name AOREG2 - General purpose always on domain data storage. [Reset by POR, RSTN] */
/*! @{ */
#define PMC_AOREG2_DATA31_0_MASK                 (0xFFFFFFFFU)
#define PMC_AOREG2_DATA31_0_SHIFT                (0U)
/*! DATA31_0 - General purpose always on domain data storage. Only reinitialized on Power On Reset and RSTIN Pin reset.
 */
#define PMC_AOREG2_DATA31_0(x)                   (((uint32_t)(((uint32_t)(x)) << PMC_AOREG2_DATA31_0_SHIFT)) & PMC_AOREG2_DATA31_0_MASK)
/*! @} */

/*! @name DPDCTRL - Configuration parameters for Power Down and Deep Power Down mode. [Reset by POR, RSTN, WDT ] */
/*! @{ */
#define PMC_DPDCTRL_XTAL32MSTARTENA_MASK         (0x1U)
#define PMC_DPDCTRL_XTAL32MSTARTENA_SHIFT        (0U)
/*! XTAL32MSTARTENA - Enable XTAL32MHz automatic start-up at power up & wake up from power down or
 *    deep power down. Reset value is set by eFuse content. This register field will overwrite option
 *    selected by eFuse content only if power-up or wake-up is NOT triggered by any kind of reset.
 *    Thus if power-up or wake-up is triggered by I/O or a timer. On the contrary, if power-up or
 *    wake-up is triggered by POR (typically during initial power up) or watchdog reset or SW reset or
 *    PAD RSTN then eFuse content will reset this register and will be applied. Take care that
 *    option selected here can by masked in power down by a register of SYSCON - XTAL32MCTRL - which is
 *    itself reset after each deep power down. Thus SYSCON/XTAL32MCTRL will not have any impact
 *    after a deep power down, only after a power down.
 */
#define PMC_DPDCTRL_XTAL32MSTARTENA(x)           (((uint32_t)(((uint32_t)(x)) << PMC_DPDCTRL_XTAL32MSTARTENA_SHIFT)) & PMC_DPDCTRL_XTAL32MSTARTENA_MASK)
#define PMC_DPDCTRL_XTAL32MSTARTDLY_MASK         (0x6U)
#define PMC_DPDCTRL_XTAL32MSTARTDLY_SHIFT        (1U)
/*! XTAL32MSTARTDLY - Delay between xtal ldo enable and release of reset to xtal 0:16us 1:32us
 *    2:48us 3:64us. LSB reset value set by efuse (wake-up by I/O only). This delay is applied within PMC
 *    for Efuse controlled XTAL start and also BLE link layer for BLE controlled auto-start.
 */
#define PMC_DPDCTRL_XTAL32MSTARTDLY(x)           (((uint32_t)(((uint32_t)(x)) << PMC_DPDCTRL_XTAL32MSTARTDLY_SHIFT)) & PMC_DPDCTRL_XTAL32MSTARTDLY_MASK)
/*! @} */

/*! @name PIOPORCAP - The PIOPORCAP register captures the state of GPIO at power-on-reset or pin reset. Each bit represents the power-on reset state of one GPIO pin. [Reset by POR, RSTN] */
/*! @{ */
#define PMC_PIOPORCAP_GPIO_MASK                  (0x3FFFFFU)
#define PMC_PIOPORCAP_GPIO_SHIFT                 (0U)
/*! GPIO - Capture of GPIO values at power-on-reset and pin reset.
 */
#define PMC_PIOPORCAP_GPIO(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_PIOPORCAP_GPIO_SHIFT)) & PMC_PIOPORCAP_GPIO_MASK)
#define PMC_PIOPORCAP_NTAG_FD_MASK               (0x400000U)
#define PMC_PIOPORCAP_NTAG_FD_SHIFT              (22U)
/*! NTAG_FD - Capture of NTAG_FD value at power-on-reset and pin reset.
 */
#define PMC_PIOPORCAP_NTAG_FD(x)                 (((uint32_t)(((uint32_t)(x)) << PMC_PIOPORCAP_NTAG_FD_SHIFT)) & PMC_PIOPORCAP_NTAG_FD_MASK)
/*! @} */

/*! @name PIORESCAP - The PIORESCAP0 register captures the state of GPIO port 0 when a reset other than a power-on reset or pin reset occurs. Each bit represents the reset state of one GPIO pin. [Reset by WDT, BOD, WAKEUP IO, ARM System reset ] */
/*! @{ */
#define PMC_PIORESCAP_GPIO_MASK                  (0x3FFFFFU)
#define PMC_PIORESCAP_GPIO_SHIFT                 (0U)
/*! GPIO - Capture of GPIO values.
 */
#define PMC_PIORESCAP_GPIO(x)                    (((uint32_t)(((uint32_t)(x)) << PMC_PIORESCAP_GPIO_SHIFT)) & PMC_PIORESCAP_GPIO_MASK)
#define PMC_PIORESCAP_NTAG_FD_MASK               (0x400000U)
#define PMC_PIORESCAP_NTAG_FD_SHIFT              (22U)
/*! NTAG_FD - Capture of NTAG_FD value.
 */
#define PMC_PIORESCAP_NTAG_FD(x)                 (((uint32_t)(((uint32_t)(x)) << PMC_PIORESCAP_NTAG_FD_SHIFT)) & PMC_PIORESCAP_NTAG_FD_MASK)
/*! @} */

/*! @name PDSLEEPCFG - Controls the power to various modules in Low Power modes. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_PDSLEEPCFG_PDEN_DCDC_MASK            (0x1U)
#define PMC_PDSLEEPCFG_PDEN_DCDC_SHIFT           (0U)
/*! PDEN_DCDC - Controls DCDC power in Power down and Deep Power down modes. Automatically switched
 *    off in deep power down. 0: DCDC is disabled in Power down and Deep Power down modes; 1: DCDC
 *    is enabled in Power down and Deep Power down modes.
 */
#define PMC_PDSLEEPCFG_PDEN_DCDC(x)              (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_DCDC_SHIFT)) & PMC_PDSLEEPCFG_PDEN_DCDC_MASK)
#define PMC_PDSLEEPCFG_PDEN_BIAS_MASK            (0x2U)
#define PMC_PDSLEEPCFG_PDEN_BIAS_SHIFT           (1U)
/*! PDEN_BIAS - Controls Bias power in Power down and Deep Power down modes. 0: Bias is disabled in
 *    Power down and Deep Power down modes; 1: Bias is enabled in Power down and Deep Power down
 *    modes.
 */
#define PMC_PDSLEEPCFG_PDEN_BIAS(x)              (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_BIAS_SHIFT)) & PMC_PDSLEEPCFG_PDEN_BIAS_MASK)
#define PMC_PDSLEEPCFG_PDEN_LDO_MEM_MASK         (0x4U)
#define PMC_PDSLEEPCFG_PDEN_LDO_MEM_SHIFT        (2U)
/*! PDEN_LDO_MEM - Controls LDO memories power in Power down mode. Automatically switched off in
 *    deep power down 0: LDO is disabled in Power down mode; 1: LDO is enabled in Power down mode.
 */
#define PMC_PDSLEEPCFG_PDEN_LDO_MEM(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_LDO_MEM_SHIFT)) & PMC_PDSLEEPCFG_PDEN_LDO_MEM_MASK)
#define PMC_PDSLEEPCFG_PDEN_VBAT_BOD_MASK        (0x8U)
#define PMC_PDSLEEPCFG_PDEN_VBAT_BOD_SHIFT       (3U)
/*! PDEN_VBAT_BOD - Controls VBAT BOD power in Power down and Deep Power down modes. 0: VBAT BOD is
 *    disabled in Power down and Deep Power down modes; 1: VBAT BOD is enabled in Power down and
 *    Deep Power down modes.
 */
#define PMC_PDSLEEPCFG_PDEN_VBAT_BOD(x)          (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_VBAT_BOD_SHIFT)) & PMC_PDSLEEPCFG_PDEN_VBAT_BOD_MASK)
#define PMC_PDSLEEPCFG_PDEN_FRO192M_MASK         (0x10U)
#define PMC_PDSLEEPCFG_PDEN_FRO192M_SHIFT        (4U)
/*! PDEN_FRO192M - Controls FRO192M power in Deep Sleep, Power down and Deep Power down modes. This
 *    should be disabled before entering power down or deep power down mode. 0: FRO192M is disabled;
 *    1: FRO192M is enabled.
 */
#define PMC_PDSLEEPCFG_PDEN_FRO192M(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_FRO192M_SHIFT)) & PMC_PDSLEEPCFG_PDEN_FRO192M_MASK)
#define PMC_PDSLEEPCFG_PDEN_FRO1M_MASK           (0x20U)
#define PMC_PDSLEEPCFG_PDEN_FRO1M_SHIFT          (5U)
/*! PDEN_FRO1M - Controls FRO1M power in Deep Sleep, Power down and Deep Power down modes. This
 *    should be disabled before entering power down (unless needed for low power timers) or deep power
 *    down mode. 0: FRO1M is disabled; 1: FRO1M is enabled.
 */
#define PMC_PDSLEEPCFG_PDEN_FRO1M(x)             (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_FRO1M_SHIFT)) & PMC_PDSLEEPCFG_PDEN_FRO1M_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_FLASH_MASK        (0x40U)
#define PMC_PDSLEEPCFG_PDEN_PD_FLASH_SHIFT       (6U)
/*! PDEN_PD_FLASH - Enable Flash power domain Power Down mode (power shutoff) when entering in
 *    DeepSleep. In PowerDown modes this domain is automatically powered off.
 */
#define PMC_PDSLEEPCFG_PDEN_PD_FLASH(x)          (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_FLASH_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_FLASH_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_COMM0_MASK        (0x80U)
#define PMC_PDSLEEPCFG_PDEN_PD_COMM0_SHIFT       (7U)
/*! PDEN_PD_COMM0 - Enable Comm0 power domain (USART0, I2C0, SPI0) Power Down mode when entering in
 *    Powerdown mode. In Deep power down it is disabled by hardware. In deep sleep it is always
 *    enabled.
 */
#define PMC_PDSLEEPCFG_PDEN_PD_COMM0(x)          (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_COMM0_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_COMM0_MASK)
#define PMC_PDSLEEPCFG_EN_PDMCU_RETENTION_MASK   (0x100U)
#define PMC_PDSLEEPCFG_EN_PDMCU_RETENTION_SHIFT  (8U)
/*! EN_PDMCU_RETENTION - Enable MCU Power Domain state retention when entering in 'Powerdown' mode for modem and radio cal values
 */
#define PMC_PDSLEEPCFG_EN_PDMCU_RETENTION(x)     (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_EN_PDMCU_RETENTION_SHIFT)) & PMC_PDSLEEPCFG_EN_PDMCU_RETENTION_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM0_MASK         (0x400U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM0_SHIFT        (10U)
/*! PDEN_PD_MEM0 - Enable Power Down mode of SRAM 0 when entering in Powerdown mode. Automatically switched off in deep power down.
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM0(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM0_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM0_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM1_MASK         (0x800U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM1_SHIFT        (11U)
/*! PDEN_PD_MEM1 - Enable Power Down mode of SRAM 1 when entering in Powerdown mode. Automatically switched off in deep power down.
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM1(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM1_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM1_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM2_MASK         (0x1000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM2_SHIFT        (12U)
/*! PDEN_PD_MEM2 - Enable Power Down mode of SRAM 2 when entering in Powerdown mode. Automatically switched off in deep power down.
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM2(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM2_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM2_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM3_MASK         (0x2000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM3_SHIFT        (13U)
/*! PDEN_PD_MEM3 - Enable Power Down mode of SRAM 3 when entering in Powerdown mode. Automatically switched off in deep power down.
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM3(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM3_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM3_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM4_MASK         (0x4000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM4_SHIFT        (14U)
/*! PDEN_PD_MEM4 - Enable Power Down mode of SRAM 4 when entering in Powerdown mode. Automatically switched off in deep power down.
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM4(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM4_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM4_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM5_MASK         (0x8000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM5_SHIFT        (15U)
/*! PDEN_PD_MEM5 - Enable Power Down mode of SRAM 5 when entering in Powerdown mode. Automatically switched off in deep power down
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM5(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM5_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM5_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM6_MASK         (0x10000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM6_SHIFT        (16U)
/*! PDEN_PD_MEM6 - Enable Power Down mode of SRAM 6 when entering in Powerdown mode. Automatically switched off in deep power down
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM6(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM6_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM6_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM7_MASK         (0x20000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM7_SHIFT        (17U)
/*! PDEN_PD_MEM7 - Enable Power Down mode of SRAM 7 when entering in Powerdown mode. Automatically switched off in deep power down
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM7(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM7_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM7_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM8_MASK         (0x40000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM8_SHIFT        (18U)
/*! PDEN_PD_MEM8 - Enable Power Down mode of SRAM 8 when entering in Powerdown mode. Automatically switched off in deep power down
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM8(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM8_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM8_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM9_MASK         (0x80000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM9_SHIFT        (19U)
/*! PDEN_PD_MEM9 - Enable Power Down mode of SRAM 9 when entering in Powerdown mode. Automatically switched off in deep power down
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM9(x)           (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM9_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM9_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM10_MASK        (0x100000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM10_SHIFT       (20U)
/*! PDEN_PD_MEM10 - Enable Power Down mode of SRAM 10 when entering in Powerdown mode. Automatically switched off in deep power down
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM10(x)          (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM10_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM10_MASK)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM11_MASK        (0x200000U)
#define PMC_PDSLEEPCFG_PDEN_PD_MEM11_SHIFT       (21U)
/*! PDEN_PD_MEM11 - Enable Power Down mode of SRAM 11 when entering in Powerdown mode. Automatically switched off in deep power down
 */
#define PMC_PDSLEEPCFG_PDEN_PD_MEM11(x)          (((uint32_t)(((uint32_t)(x)) << PMC_PDSLEEPCFG_PDEN_PD_MEM11_SHIFT)) & PMC_PDSLEEPCFG_PDEN_PD_MEM11_MASK)
/*! @} */

/*! @name PDRUNCFG - Controls the power to various analog blocks. [Reset by all reset sources, except ARM SystemReset] */
/*! @{ */
#define PMC_PDRUNCFG_ENA_LDO_ADC_MASK            (0x400000U)
#define PMC_PDRUNCFG_ENA_LDO_ADC_SHIFT           (22U)
/*! ENA_LDO_ADC - LDO ADC enabled. See STATUSPWR.LDOADC1V1PWROK for when the power domain is ready.
 */
#define PMC_PDRUNCFG_ENA_LDO_ADC(x)              (((uint32_t)(((uint32_t)(x)) << PMC_PDRUNCFG_ENA_LDO_ADC_SHIFT)) & PMC_PDRUNCFG_ENA_LDO_ADC_MASK)
#define PMC_PDRUNCFG_ENA_BOD_MEM_MASK            (0x800000U)
#define PMC_PDRUNCFG_ENA_BOD_MEM_SHIFT           (23U)
/*! ENA_BOD_MEM - BOD MEM enabled.
 */
#define PMC_PDRUNCFG_ENA_BOD_MEM(x)              (((uint32_t)(((uint32_t)(x)) << PMC_PDRUNCFG_ENA_BOD_MEM_SHIFT)) & PMC_PDRUNCFG_ENA_BOD_MEM_MASK)
#define PMC_PDRUNCFG_ENA_BOD_CORE_MASK           (0x1000000U)
#define PMC_PDRUNCFG_ENA_BOD_CORE_SHIFT          (24U)
/*! ENA_BOD_CORE - BOD CORE enabled.
 */
#define PMC_PDRUNCFG_ENA_BOD_CORE(x)             (((uint32_t)(((uint32_t)(x)) << PMC_PDRUNCFG_ENA_BOD_CORE_SHIFT)) & PMC_PDRUNCFG_ENA_BOD_CORE_MASK)
#define PMC_PDRUNCFG_ENA_FRO32K_MASK             (0x2000000U)
#define PMC_PDRUNCFG_ENA_FRO32K_SHIFT            (25U)
/*! ENA_FRO32K - FRO32K enabled.
 */
#define PMC_PDRUNCFG_ENA_FRO32K(x)               (((uint32_t)(((uint32_t)(x)) << PMC_PDRUNCFG_ENA_FRO32K_SHIFT)) & PMC_PDRUNCFG_ENA_FRO32K_MASK)
#define PMC_PDRUNCFG_ENA_XTAL32K_MASK            (0x4000000U)
#define PMC_PDRUNCFG_ENA_XTAL32K_SHIFT           (26U)
/*! ENA_XTAL32K - XTAL32K enabled.
 */
#define PMC_PDRUNCFG_ENA_XTAL32K(x)              (((uint32_t)(((uint32_t)(x)) << PMC_PDRUNCFG_ENA_XTAL32K_SHIFT)) & PMC_PDRUNCFG_ENA_XTAL32K_MASK)
#define PMC_PDRUNCFG_ENA_ANA_COMP_MASK           (0x8000000U)
#define PMC_PDRUNCFG_ENA_ANA_COMP_SHIFT          (27U)
/*! ENA_ANA_COMP - Analog Comparator enabled.
 */
#define PMC_PDRUNCFG_ENA_ANA_COMP(x)             (((uint32_t)(((uint32_t)(x)) << PMC_PDRUNCFG_ENA_ANA_COMP_SHIFT)) & PMC_PDRUNCFG_ENA_ANA_COMP_MASK)
/*! @} */

/*! @name WAKEIOCAUSE - Wake-up source from Power Down and Deep Power Down modes. Allow to identify the Wake-up source from Power-Down mode or Deep Power Down mode.[Reset by POR, RSTN, WDT ] */
/*! @{ */
#define PMC_WAKEIOCAUSE_GPIO00_MASK              (0x1U)
#define PMC_WAKEIOCAUSE_GPIO00_SHIFT             (0U)
/*! GPIO00 - Wake up was triggered by GPIO 00
 */
#define PMC_WAKEIOCAUSE_GPIO00(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO00_SHIFT)) & PMC_WAKEIOCAUSE_GPIO00_MASK)
#define PMC_WAKEIOCAUSE_GPIO01_MASK              (0x2U)
#define PMC_WAKEIOCAUSE_GPIO01_SHIFT             (1U)
/*! GPIO01 - Wake up was triggered by GPIO 01
 */
#define PMC_WAKEIOCAUSE_GPIO01(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO01_SHIFT)) & PMC_WAKEIOCAUSE_GPIO01_MASK)
#define PMC_WAKEIOCAUSE_GPIO02_MASK              (0x4U)
#define PMC_WAKEIOCAUSE_GPIO02_SHIFT             (2U)
/*! GPIO02 - Wake up was triggered by GPIO 02
 */
#define PMC_WAKEIOCAUSE_GPIO02(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO02_SHIFT)) & PMC_WAKEIOCAUSE_GPIO02_MASK)
#define PMC_WAKEIOCAUSE_GPIO03_MASK              (0x8U)
#define PMC_WAKEIOCAUSE_GPIO03_SHIFT             (3U)
/*! GPIO03 - Wake up was triggered by GPIO 03
 */
#define PMC_WAKEIOCAUSE_GPIO03(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO03_SHIFT)) & PMC_WAKEIOCAUSE_GPIO03_MASK)
#define PMC_WAKEIOCAUSE_GPIO04_MASK              (0x10U)
#define PMC_WAKEIOCAUSE_GPIO04_SHIFT             (4U)
/*! GPIO04 - Wake up was triggered by GPIO 04
 */
#define PMC_WAKEIOCAUSE_GPIO04(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO04_SHIFT)) & PMC_WAKEIOCAUSE_GPIO04_MASK)
#define PMC_WAKEIOCAUSE_GPIO05_MASK              (0x20U)
#define PMC_WAKEIOCAUSE_GPIO05_SHIFT             (5U)
/*! GPIO05 - Wake up was triggered by GPIO 05
 */
#define PMC_WAKEIOCAUSE_GPIO05(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO05_SHIFT)) & PMC_WAKEIOCAUSE_GPIO05_MASK)
#define PMC_WAKEIOCAUSE_GPIO06_MASK              (0x40U)
#define PMC_WAKEIOCAUSE_GPIO06_SHIFT             (6U)
/*! GPIO06 - Wake up was triggered by GPIO 06
 */
#define PMC_WAKEIOCAUSE_GPIO06(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO06_SHIFT)) & PMC_WAKEIOCAUSE_GPIO06_MASK)
#define PMC_WAKEIOCAUSE_GPIO07_MASK              (0x80U)
#define PMC_WAKEIOCAUSE_GPIO07_SHIFT             (7U)
/*! GPIO07 - Wake up was triggered by GPIO 07
 */
#define PMC_WAKEIOCAUSE_GPIO07(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO07_SHIFT)) & PMC_WAKEIOCAUSE_GPIO07_MASK)
#define PMC_WAKEIOCAUSE_GPIO08_MASK              (0x100U)
#define PMC_WAKEIOCAUSE_GPIO08_SHIFT             (8U)
/*! GPIO08 - Wake up was triggered by GPIO 08
 */
#define PMC_WAKEIOCAUSE_GPIO08(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO08_SHIFT)) & PMC_WAKEIOCAUSE_GPIO08_MASK)
#define PMC_WAKEIOCAUSE_GPIO09_MASK              (0x200U)
#define PMC_WAKEIOCAUSE_GPIO09_SHIFT             (9U)
/*! GPIO09 - Wake up was triggered by GPIO 09
 */
#define PMC_WAKEIOCAUSE_GPIO09(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO09_SHIFT)) & PMC_WAKEIOCAUSE_GPIO09_MASK)
#define PMC_WAKEIOCAUSE_GPIO10_MASK              (0x400U)
#define PMC_WAKEIOCAUSE_GPIO10_SHIFT             (10U)
/*! GPIO10 - Wake up was triggered by GPIO 10
 */
#define PMC_WAKEIOCAUSE_GPIO10(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO10_SHIFT)) & PMC_WAKEIOCAUSE_GPIO10_MASK)
#define PMC_WAKEIOCAUSE_GPIO11_MASK              (0x800U)
#define PMC_WAKEIOCAUSE_GPIO11_SHIFT             (11U)
/*! GPIO11 - Wake up was triggered by GPIO 11
 */
#define PMC_WAKEIOCAUSE_GPIO11(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO11_SHIFT)) & PMC_WAKEIOCAUSE_GPIO11_MASK)
#define PMC_WAKEIOCAUSE_GPIO12_MASK              (0x1000U)
#define PMC_WAKEIOCAUSE_GPIO12_SHIFT             (12U)
/*! GPIO12 - Wake up was triggered by GPIO 12
 */
#define PMC_WAKEIOCAUSE_GPIO12(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO12_SHIFT)) & PMC_WAKEIOCAUSE_GPIO12_MASK)
#define PMC_WAKEIOCAUSE_GPIO13_MASK              (0x2000U)
#define PMC_WAKEIOCAUSE_GPIO13_SHIFT             (13U)
/*! GPIO13 - Wake up was triggered by GPIO 13
 */
#define PMC_WAKEIOCAUSE_GPIO13(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO13_SHIFT)) & PMC_WAKEIOCAUSE_GPIO13_MASK)
#define PMC_WAKEIOCAUSE_GPIO14_MASK              (0x4000U)
#define PMC_WAKEIOCAUSE_GPIO14_SHIFT             (14U)
/*! GPIO14 - Wake up was triggered by GPIO 14
 */
#define PMC_WAKEIOCAUSE_GPIO14(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO14_SHIFT)) & PMC_WAKEIOCAUSE_GPIO14_MASK)
#define PMC_WAKEIOCAUSE_GPIO15_MASK              (0x8000U)
#define PMC_WAKEIOCAUSE_GPIO15_SHIFT             (15U)
/*! GPIO15 - Wake up was triggered by GPIO 15
 */
#define PMC_WAKEIOCAUSE_GPIO15(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO15_SHIFT)) & PMC_WAKEIOCAUSE_GPIO15_MASK)
#define PMC_WAKEIOCAUSE_GPIO16_MASK              (0x10000U)
#define PMC_WAKEIOCAUSE_GPIO16_SHIFT             (16U)
/*! GPIO16 - Wake up was triggered by GPIO 16
 */
#define PMC_WAKEIOCAUSE_GPIO16(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO16_SHIFT)) & PMC_WAKEIOCAUSE_GPIO16_MASK)
#define PMC_WAKEIOCAUSE_GPIO17_MASK              (0x20000U)
#define PMC_WAKEIOCAUSE_GPIO17_SHIFT             (17U)
/*! GPIO17 - Wake up was triggered by GPIO 17
 */
#define PMC_WAKEIOCAUSE_GPIO17(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO17_SHIFT)) & PMC_WAKEIOCAUSE_GPIO17_MASK)
#define PMC_WAKEIOCAUSE_GPIO18_MASK              (0x40000U)
#define PMC_WAKEIOCAUSE_GPIO18_SHIFT             (18U)
/*! GPIO18 - Wake up was triggered by GPIO 18
 */
#define PMC_WAKEIOCAUSE_GPIO18(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO18_SHIFT)) & PMC_WAKEIOCAUSE_GPIO18_MASK)
#define PMC_WAKEIOCAUSE_GPIO19_MASK              (0x80000U)
#define PMC_WAKEIOCAUSE_GPIO19_SHIFT             (19U)
/*! GPIO19 - Wake up was triggered by GPIO 19
 */
#define PMC_WAKEIOCAUSE_GPIO19(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO19_SHIFT)) & PMC_WAKEIOCAUSE_GPIO19_MASK)
#define PMC_WAKEIOCAUSE_GPIO20_MASK              (0x100000U)
#define PMC_WAKEIOCAUSE_GPIO20_SHIFT             (20U)
/*! GPIO20 - Wake up was triggered by GPIO 20
 */
#define PMC_WAKEIOCAUSE_GPIO20(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO20_SHIFT)) & PMC_WAKEIOCAUSE_GPIO20_MASK)
#define PMC_WAKEIOCAUSE_GPIO21_MASK              (0x200000U)
#define PMC_WAKEIOCAUSE_GPIO21_SHIFT             (21U)
/*! GPIO21 - Wake up was triggered by GPIO 21
 */
#define PMC_WAKEIOCAUSE_GPIO21(x)                (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_GPIO21_SHIFT)) & PMC_WAKEIOCAUSE_GPIO21_MASK)
#define PMC_WAKEIOCAUSE_NTAG_FD_MASK             (0x400000U)
#define PMC_WAKEIOCAUSE_NTAG_FD_SHIFT            (22U)
/*! NTAG_FD - Wake up was triggered by NTAG FD
 */
#define PMC_WAKEIOCAUSE_NTAG_FD(x)               (((uint32_t)(((uint32_t)(x)) << PMC_WAKEIOCAUSE_NTAG_FD_SHIFT)) & PMC_WAKEIOCAUSE_NTAG_FD_MASK)
/*! @} */

/*! @name CTRLNORST - Extension of CTRL register, but never reset except by POR */
/*! @{ */
#define PMC_CTRLNORST_FASTLDOENABLE_MASK         (0x7U)
#define PMC_CTRLNORST_FASTLDOENABLE_SHIFT        (0U)
/*! FASTLDOENABLE - Fast LDO wake-up enable. 3 bits for the different wake-up sources: {generic
 *    async wake up event as selected by SLEEPCON/STARTER0&1, IO wake-up event, RSTN pad event}. If
 *    required, this field should only be managed by the Low power driver software.
 */
#define PMC_CTRLNORST_FASTLDOENABLE(x)           (((uint32_t)(((uint32_t)(x)) << PMC_CTRLNORST_FASTLDOENABLE_SHIFT)) & PMC_CTRLNORST_FASTLDOENABLE_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group PMC_Register_Masks */


/* PMC - Peripheral instance base addresses */
/** Peripheral PMC base address */
#define PMC_BASE                                 (0x40012000u)
/** Peripheral PMC base pointer */
#define PMC                                      ((PMC_Type *)PMC_BASE)
/** Array initializer of PMC peripheral base addresses */
#define PMC_BASE_ADDRS                           { PMC_BASE }
/** Array initializer of PMC peripheral base pointers */
#define PMC_BASE_PTRS                            { PMC }

/*!
 * @}
 */ /* end of group PMC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- PWM Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PWM_Peripheral_Access_Layer PWM Peripheral Access Layer
 * @{
 */

/** PWM - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL0;                             /**< PWM 1st Control Register (Channel 0 to Channel 10) for channel enables and interrupt enables. Note if all interrupts are enabled with short period timings it will not be possible to manage all the interrupts., offset: 0x0 */
  __IO uint32_t CTRL1;                             /**< PWM 2nd Control Register (Channel 0 to Channel 10) for channel polarity and output state for a disabled channel., offset: 0x4 */
  __IO uint32_t PSCL01;                            /**< PWM Channels 0 & 1 prescalers, offset: 0x8 */
  __IO uint32_t PSCL23;                            /**< PWM Channels 2 & 3 prescalers, offset: 0xC */
  __IO uint32_t PSCL45;                            /**< PWM Channels 4 & 5 prescalers, offset: 0x10 */
  __IO uint32_t PSCL67;                            /**< PWM Channels 6 & 7 prescalers, offset: 0x14 */
  __IO uint32_t PSCL89;                            /**< PWM Channels 8 & 9 prescalers, offset: 0x18 */
  __IO uint32_t PSCL1011;                          /**< PWM Channel 10 prescaler, offset: 0x1C */
  __IO uint32_t PCP0;                              /**< PWM Channel 0 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x20 */
  __IO uint32_t PCP1;                              /**< PWM Channel 1 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x24 */
  __IO uint32_t PCP2;                              /**< PWM Channel 2 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x28 */
  __IO uint32_t PCP3;                              /**< PWM Channel 3 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x2C */
  __IO uint32_t PCP4;                              /**< PWM Channel 4 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x30 */
  __IO uint32_t PCP5;                              /**< PWM Channel 5 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x34 */
  __IO uint32_t PCP6;                              /**< PWM Channel 6 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x38 */
  __IO uint32_t PCP7;                              /**< PWM Channel 7 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x3C */
  __IO uint32_t PCP8;                              /**< PWM Channel 8 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x40 */
  __IO uint32_t PCP9;                              /**< PWM Channel 9 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x44 */
  __IO uint32_t PCP10;                             /**< PWM Channel 10 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached all PWM outputs will change on next counter decrement and be stable from 'Compare-1' to 0., offset: 0x48 */
  __IO uint32_t PST0;                              /**< PWM 1st Status Register (Channel 0 to Channel 3), offset: 0x4C */
  __IO uint32_t PST1;                              /**< PWM 2nd Status Register (Channel 4 to Channel 7), offset: 0x50 */
  __IO uint32_t PST2;                              /**< PWM 3rd Status Register (Channel 8 to Channel 10), offset: 0x54 */
       uint8_t RESERVED_0[4004];
  __I  uint32_t MODULE_ID;                         /**< PWM Module Identifier ('PW' in ASCII), offset: 0xFFC */
} PWM_Type;

/* ----------------------------------------------------------------------------
   -- PWM Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup PWM_Register_Masks PWM Register Masks
 * @{
 */

/*! @name CTRL0 - PWM 1st Control Register (Channel 0 to Channel 10) for channel enables and interrupt enables. Note if all interrupts are enabled with short period timings it will not be possible to manage all the interrupts. */
/*! @{ */
#define PWM_CTRL0_PWM_EN_0_MASK                  (0x1U)
#define PWM_CTRL0_PWM_EN_0_SHIFT                 (0U)
/*! PWM_EN_0 - PWM channel 0 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_0(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_0_SHIFT)) & PWM_CTRL0_PWM_EN_0_MASK)
#define PWM_CTRL0_PWM_EN_1_MASK                  (0x2U)
#define PWM_CTRL0_PWM_EN_1_SHIFT                 (1U)
/*! PWM_EN_1 - PWM channel 1 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_1(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_1_SHIFT)) & PWM_CTRL0_PWM_EN_1_MASK)
#define PWM_CTRL0_PWM_EN_2_MASK                  (0x4U)
#define PWM_CTRL0_PWM_EN_2_SHIFT                 (2U)
/*! PWM_EN_2 - PWM channel 2 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_2(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_2_SHIFT)) & PWM_CTRL0_PWM_EN_2_MASK)
#define PWM_CTRL0_PWM_EN_3_MASK                  (0x8U)
#define PWM_CTRL0_PWM_EN_3_SHIFT                 (3U)
/*! PWM_EN_3 - PWM channel 3 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_3(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_3_SHIFT)) & PWM_CTRL0_PWM_EN_3_MASK)
#define PWM_CTRL0_PWM_EN_4_MASK                  (0x10U)
#define PWM_CTRL0_PWM_EN_4_SHIFT                 (4U)
/*! PWM_EN_4 - PWM channel 4 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_4(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_4_SHIFT)) & PWM_CTRL0_PWM_EN_4_MASK)
#define PWM_CTRL0_PWM_EN_5_MASK                  (0x20U)
#define PWM_CTRL0_PWM_EN_5_SHIFT                 (5U)
/*! PWM_EN_5 - PWM channel 5 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_5(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_5_SHIFT)) & PWM_CTRL0_PWM_EN_5_MASK)
#define PWM_CTRL0_PWM_EN_6_MASK                  (0x40U)
#define PWM_CTRL0_PWM_EN_6_SHIFT                 (6U)
/*! PWM_EN_6 - PWM channel 6 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_6(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_6_SHIFT)) & PWM_CTRL0_PWM_EN_6_MASK)
#define PWM_CTRL0_PWM_EN_7_MASK                  (0x80U)
#define PWM_CTRL0_PWM_EN_7_SHIFT                 (7U)
/*! PWM_EN_7 - PWM channel 7 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_7(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_7_SHIFT)) & PWM_CTRL0_PWM_EN_7_MASK)
#define PWM_CTRL0_PWM_EN_8_MASK                  (0x100U)
#define PWM_CTRL0_PWM_EN_8_SHIFT                 (8U)
/*! PWM_EN_8 - PWM channel 8 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_8(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_8_SHIFT)) & PWM_CTRL0_PWM_EN_8_MASK)
#define PWM_CTRL0_PWM_EN_9_MASK                  (0x200U)
#define PWM_CTRL0_PWM_EN_9_SHIFT                 (9U)
/*! PWM_EN_9 - PWM channel 9 enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_PWM_EN_9(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_9_SHIFT)) & PWM_CTRL0_PWM_EN_9_MASK)
#define PWM_CTRL0_PWM_EN_10_MASK                 (0x400U)
#define PWM_CTRL0_PWM_EN_10_SHIFT                (10U)
/*! PWM_EN_10 - PWM channel 10 enable. 0 = Disable / 1 = Enable. Note, this enables the common PWM
 *    mode where PWM10 will be routed to all PWM channels.
 */
#define PWM_CTRL0_PWM_EN_10(x)                   (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_PWM_EN_10_SHIFT)) & PWM_CTRL0_PWM_EN_10_MASK)
#define PWM_CTRL0_INT_EN_0_MASK                  (0x10000U)
#define PWM_CTRL0_INT_EN_0_SHIFT                 (16U)
/*! INT_EN_0 - PWM channel 0 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_0(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_0_SHIFT)) & PWM_CTRL0_INT_EN_0_MASK)
#define PWM_CTRL0_INT_EN_1_MASK                  (0x20000U)
#define PWM_CTRL0_INT_EN_1_SHIFT                 (17U)
/*! INT_EN_1 - PWM channel 1 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_1(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_1_SHIFT)) & PWM_CTRL0_INT_EN_1_MASK)
#define PWM_CTRL0_INT_EN_2_MASK                  (0x40000U)
#define PWM_CTRL0_INT_EN_2_SHIFT                 (18U)
/*! INT_EN_2 - PWM channel 2 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_2(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_2_SHIFT)) & PWM_CTRL0_INT_EN_2_MASK)
#define PWM_CTRL0_INT_EN_3_MASK                  (0x80000U)
#define PWM_CTRL0_INT_EN_3_SHIFT                 (19U)
/*! INT_EN_3 - PWM channel 3 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_3(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_3_SHIFT)) & PWM_CTRL0_INT_EN_3_MASK)
#define PWM_CTRL0_INT_EN_4_MASK                  (0x100000U)
#define PWM_CTRL0_INT_EN_4_SHIFT                 (20U)
/*! INT_EN_4 - PWM channel 4 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_4(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_4_SHIFT)) & PWM_CTRL0_INT_EN_4_MASK)
#define PWM_CTRL0_INT_EN_5_MASK                  (0x200000U)
#define PWM_CTRL0_INT_EN_5_SHIFT                 (21U)
/*! INT_EN_5 - PWM channel 5 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_5(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_5_SHIFT)) & PWM_CTRL0_INT_EN_5_MASK)
#define PWM_CTRL0_INT_EN_6_MASK                  (0x400000U)
#define PWM_CTRL0_INT_EN_6_SHIFT                 (22U)
/*! INT_EN_6 - PWM channel 6 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_6(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_6_SHIFT)) & PWM_CTRL0_INT_EN_6_MASK)
#define PWM_CTRL0_INT_EN_7_MASK                  (0x800000U)
#define PWM_CTRL0_INT_EN_7_SHIFT                 (23U)
/*! INT_EN_7 - PWM channel 7 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_7(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_7_SHIFT)) & PWM_CTRL0_INT_EN_7_MASK)
#define PWM_CTRL0_INT_EN_8_MASK                  (0x1000000U)
#define PWM_CTRL0_INT_EN_8_SHIFT                 (24U)
/*! INT_EN_8 - PWM channel 8 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_8(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_8_SHIFT)) & PWM_CTRL0_INT_EN_8_MASK)
#define PWM_CTRL0_INT_EN_9_MASK                  (0x2000000U)
#define PWM_CTRL0_INT_EN_9_SHIFT                 (25U)
/*! INT_EN_9 - PWM channel 9 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_9(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_9_SHIFT)) & PWM_CTRL0_INT_EN_9_MASK)
#define PWM_CTRL0_INT_EN_10_MASK                 (0x4000000U)
#define PWM_CTRL0_INT_EN_10_SHIFT                (26U)
/*! INT_EN_10 - PWM channel 10 interrupt enable. 0 = Disable / 1 = Enable.
 */
#define PWM_CTRL0_INT_EN_10(x)                   (((uint32_t)(((uint32_t)(x)) << PWM_CTRL0_INT_EN_10_SHIFT)) & PWM_CTRL0_INT_EN_10_MASK)
/*! @} */

/*! @name CTRL1 - PWM 2nd Control Register (Channel 0 to Channel 10) for channel polarity and output state for a disabled channel. */
/*! @{ */
#define PWM_CTRL1_POL_0_MASK                     (0x1U)
#define PWM_CTRL1_POL_0_SHIFT                    (0U)
/*! POL_0 - PWM channel 0 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_0(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_0_SHIFT)) & PWM_CTRL1_POL_0_MASK)
#define PWM_CTRL1_POL_1_MASK                     (0x2U)
#define PWM_CTRL1_POL_1_SHIFT                    (1U)
/*! POL_1 - PWM channel 1 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_1(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_1_SHIFT)) & PWM_CTRL1_POL_1_MASK)
#define PWM_CTRL1_POL_2_MASK                     (0x4U)
#define PWM_CTRL1_POL_2_SHIFT                    (2U)
/*! POL_2 - PWM channel 2 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_2(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_2_SHIFT)) & PWM_CTRL1_POL_2_MASK)
#define PWM_CTRL1_POL_3_MASK                     (0x8U)
#define PWM_CTRL1_POL_3_SHIFT                    (3U)
/*! POL_3 - PWM channel 3 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_3(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_3_SHIFT)) & PWM_CTRL1_POL_3_MASK)
#define PWM_CTRL1_POL_4_MASK                     (0x10U)
#define PWM_CTRL1_POL_4_SHIFT                    (4U)
/*! POL_4 - PWM channel 4 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_4(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_4_SHIFT)) & PWM_CTRL1_POL_4_MASK)
#define PWM_CTRL1_POL_5_MASK                     (0x20U)
#define PWM_CTRL1_POL_5_SHIFT                    (5U)
/*! POL_5 - PWM channel 5 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_5(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_5_SHIFT)) & PWM_CTRL1_POL_5_MASK)
#define PWM_CTRL1_POL_6_MASK                     (0x40U)
#define PWM_CTRL1_POL_6_SHIFT                    (6U)
/*! POL_6 - PWM channel 6 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_6(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_6_SHIFT)) & PWM_CTRL1_POL_6_MASK)
#define PWM_CTRL1_POL_7_MASK                     (0x80U)
#define PWM_CTRL1_POL_7_SHIFT                    (7U)
/*! POL_7 - PWM channel 7 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_7(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_7_SHIFT)) & PWM_CTRL1_POL_7_MASK)
#define PWM_CTRL1_POL_8_MASK                     (0x100U)
#define PWM_CTRL1_POL_8_SHIFT                    (8U)
/*! POL_8 - PWM channel 8 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_8(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_8_SHIFT)) & PWM_CTRL1_POL_8_MASK)
#define PWM_CTRL1_POL_9_MASK                     (0x200U)
#define PWM_CTRL1_POL_9_SHIFT                    (9U)
/*! POL_9 - PWM channel 9 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_9(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_9_SHIFT)) & PWM_CTRL1_POL_9_MASK)
#define PWM_CTRL1_POL_10_MASK                    (0x400U)
#define PWM_CTRL1_POL_10_SHIFT                   (10U)
/*! POL_10 - PWM channel 10 waveform Polarity control. 0: Set high on compare match, set low at the
 *    end of PWM period. 1: Set low on compare match, set high at the end of PWM period
 */
#define PWM_CTRL1_POL_10(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_POL_10_SHIFT)) & PWM_CTRL1_POL_10_MASK)
#define PWM_CTRL1_DIS_LEVEL_0_MASK               (0x10000U)
#define PWM_CTRL1_DIS_LEVEL_0_SHIFT              (16U)
/*! DIS_LEVEL_0 - PWM channel 0 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_0(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_0_SHIFT)) & PWM_CTRL1_DIS_LEVEL_0_MASK)
#define PWM_CTRL1_DIS_LEVEL_1_MASK               (0x20000U)
#define PWM_CTRL1_DIS_LEVEL_1_SHIFT              (17U)
/*! DIS_LEVEL_1 - PWM channel 1 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_1(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_1_SHIFT)) & PWM_CTRL1_DIS_LEVEL_1_MASK)
#define PWM_CTRL1_DIS_LEVEL_2_MASK               (0x40000U)
#define PWM_CTRL1_DIS_LEVEL_2_SHIFT              (18U)
/*! DIS_LEVEL_2 - PWM channel 2 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_2(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_2_SHIFT)) & PWM_CTRL1_DIS_LEVEL_2_MASK)
#define PWM_CTRL1_DIS_LEVEL_3_MASK               (0x80000U)
#define PWM_CTRL1_DIS_LEVEL_3_SHIFT              (19U)
/*! DIS_LEVEL_3 - PWM channel 3 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_3(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_3_SHIFT)) & PWM_CTRL1_DIS_LEVEL_3_MASK)
#define PWM_CTRL1_DIS_LEVEL_4_MASK               (0x100000U)
#define PWM_CTRL1_DIS_LEVEL_4_SHIFT              (20U)
/*! DIS_LEVEL_4 - PWM channel 4 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_4(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_4_SHIFT)) & PWM_CTRL1_DIS_LEVEL_4_MASK)
#define PWM_CTRL1_DIS_LEVEL_5_MASK               (0x200000U)
#define PWM_CTRL1_DIS_LEVEL_5_SHIFT              (21U)
/*! DIS_LEVEL_5 - PWM channel 5 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_5(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_5_SHIFT)) & PWM_CTRL1_DIS_LEVEL_5_MASK)
#define PWM_CTRL1_DIS_LEVEL_6_MASK               (0x400000U)
#define PWM_CTRL1_DIS_LEVEL_6_SHIFT              (22U)
/*! DIS_LEVEL_6 - PWM channel 6 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_6(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_6_SHIFT)) & PWM_CTRL1_DIS_LEVEL_6_MASK)
#define PWM_CTRL1_DIS_LEVEL_7_MASK               (0x800000U)
#define PWM_CTRL1_DIS_LEVEL_7_SHIFT              (23U)
/*! DIS_LEVEL_7 - PWM channel 7 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_7(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_7_SHIFT)) & PWM_CTRL1_DIS_LEVEL_7_MASK)
#define PWM_CTRL1_DIS_LEVEL_8_MASK               (0x1000000U)
#define PWM_CTRL1_DIS_LEVEL_8_SHIFT              (24U)
/*! DIS_LEVEL_8 - PWM channel 8 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_8(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_8_SHIFT)) & PWM_CTRL1_DIS_LEVEL_8_MASK)
#define PWM_CTRL1_DIS_LEVEL_9_MASK               (0x2000000U)
#define PWM_CTRL1_DIS_LEVEL_9_SHIFT              (25U)
/*! DIS_LEVEL_9 - PWM channel 9 output level when PWM channel 0 is disable. 0 = Low Level / 1 = High Level.
 */
#define PWM_CTRL1_DIS_LEVEL_9(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_CTRL1_DIS_LEVEL_9_SHIFT)) & PWM_CTRL1_DIS_LEVEL_9_MASK)
/*! @} */

/*! @name PSCL01 - PWM Channels 0 & 1 prescalers */
/*! @{ */
#define PWM_PSCL01_PSCL_0_MASK                   (0x3FFU)
#define PWM_PSCL01_PSCL_0_SHIFT                  (0U)
/*! PSCL_0 - PWM channel 0 prescaler. The output frequency equals to clk/(PSCL_0 + 1)
 */
#define PWM_PSCL01_PSCL_0(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL01_PSCL_0_SHIFT)) & PWM_PSCL01_PSCL_0_MASK)
#define PWM_PSCL01_PSCL_1_MASK                   (0x3FF0000U)
#define PWM_PSCL01_PSCL_1_SHIFT                  (16U)
/*! PSCL_1 - PWM channel 1 prescaler. The output frequency equals to clk/(PSCL_1 + 1)
 */
#define PWM_PSCL01_PSCL_1(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL01_PSCL_1_SHIFT)) & PWM_PSCL01_PSCL_1_MASK)
/*! @} */

/*! @name PSCL23 - PWM Channels 2 & 3 prescalers */
/*! @{ */
#define PWM_PSCL23_PSCL_2_MASK                   (0x3FFU)
#define PWM_PSCL23_PSCL_2_SHIFT                  (0U)
/*! PSCL_2 - PWM channel 2 prescaler. The output frequency equals to clk/(PSCL_2 + 1)
 */
#define PWM_PSCL23_PSCL_2(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL23_PSCL_2_SHIFT)) & PWM_PSCL23_PSCL_2_MASK)
#define PWM_PSCL23_PSCL_3_MASK                   (0x3FF0000U)
#define PWM_PSCL23_PSCL_3_SHIFT                  (16U)
/*! PSCL_3 - PWM channel 3 prescaler. The output frequency equals to clk/(PSCL_3 + 1)
 */
#define PWM_PSCL23_PSCL_3(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL23_PSCL_3_SHIFT)) & PWM_PSCL23_PSCL_3_MASK)
/*! @} */

/*! @name PSCL45 - PWM Channels 4 & 5 prescalers */
/*! @{ */
#define PWM_PSCL45_PSCL_4_MASK                   (0x3FFU)
#define PWM_PSCL45_PSCL_4_SHIFT                  (0U)
/*! PSCL_4 - PWM channel 4 prescaler. The output frequency equals to clk/(PSCL_4 + 1)
 */
#define PWM_PSCL45_PSCL_4(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL45_PSCL_4_SHIFT)) & PWM_PSCL45_PSCL_4_MASK)
#define PWM_PSCL45_PSCL_5_MASK                   (0x3FF0000U)
#define PWM_PSCL45_PSCL_5_SHIFT                  (16U)
/*! PSCL_5 - PWM channel 5 prescaler. The output frequency equals to clk/(PSCL_5 + 1)
 */
#define PWM_PSCL45_PSCL_5(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL45_PSCL_5_SHIFT)) & PWM_PSCL45_PSCL_5_MASK)
/*! @} */

/*! @name PSCL67 - PWM Channels 6 & 7 prescalers */
/*! @{ */
#define PWM_PSCL67_PSCL_6_MASK                   (0x3FFU)
#define PWM_PSCL67_PSCL_6_SHIFT                  (0U)
/*! PSCL_6 - PWM channel 6 prescaler. The output frequency equals to clk/(PSCL_6 + 1)
 */
#define PWM_PSCL67_PSCL_6(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL67_PSCL_6_SHIFT)) & PWM_PSCL67_PSCL_6_MASK)
#define PWM_PSCL67_PSCL_7_MASK                   (0x3FF0000U)
#define PWM_PSCL67_PSCL_7_SHIFT                  (16U)
/*! PSCL_7 - PWM channel 7 prescaler. The output frequency equals to clk/(PSCL_7 + 1)
 */
#define PWM_PSCL67_PSCL_7(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL67_PSCL_7_SHIFT)) & PWM_PSCL67_PSCL_7_MASK)
/*! @} */

/*! @name PSCL89 - PWM Channels 8 & 9 prescalers */
/*! @{ */
#define PWM_PSCL89_PSCL_8_MASK                   (0x3FFU)
#define PWM_PSCL89_PSCL_8_SHIFT                  (0U)
/*! PSCL_8 - PWM channel 8 prescaler. The output frequency equals to clk/(PSCL_8 + 1)
 */
#define PWM_PSCL89_PSCL_8(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL89_PSCL_8_SHIFT)) & PWM_PSCL89_PSCL_8_MASK)
#define PWM_PSCL89_PSCL_9_MASK                   (0x3FF0000U)
#define PWM_PSCL89_PSCL_9_SHIFT                  (16U)
/*! PSCL_9 - PWM channel 9 prescaler. The output frequency equals to clk/(PSCL_9 + 1)
 */
#define PWM_PSCL89_PSCL_9(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PSCL89_PSCL_9_SHIFT)) & PWM_PSCL89_PSCL_9_MASK)
/*! @} */

/*! @name PSCL1011 - PWM Channel 10 prescaler */
/*! @{ */
#define PWM_PSCL1011_PSCL_10_MASK                (0x3FFU)
#define PWM_PSCL1011_PSCL_10_SHIFT               (0U)
/*! PSCL_10 - PWM channel 10 prescaler. The output frequency equals to clk/(PSCL_10 + 1)
 */
#define PWM_PSCL1011_PSCL_10(x)                  (((uint32_t)(((uint32_t)(x)) << PWM_PSCL1011_PSCL_10_SHIFT)) & PWM_PSCL1011_PSCL_10_MASK)
/*! @} */

/*! @name PCP0 - PWM Channel 0 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP0_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP0_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 0 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP0_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP0_PERIOD_SHIFT)) & PWM_PCP0_PERIOD_MASK)
#define PWM_PCP0_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP0_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 0 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP0_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP0_COMPARE_SHIFT)) & PWM_PCP0_COMPARE_MASK)
/*! @} */

/*! @name PCP1 - PWM Channel 1 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP1_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP1_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 1 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP1_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP1_PERIOD_SHIFT)) & PWM_PCP1_PERIOD_MASK)
#define PWM_PCP1_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP1_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 1 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP1_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP1_COMPARE_SHIFT)) & PWM_PCP1_COMPARE_MASK)
/*! @} */

/*! @name PCP2 - PWM Channel 2 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP2_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP2_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 2 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP2_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP2_PERIOD_SHIFT)) & PWM_PCP2_PERIOD_MASK)
#define PWM_PCP2_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP2_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 2 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP2_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP2_COMPARE_SHIFT)) & PWM_PCP2_COMPARE_MASK)
/*! @} */

/*! @name PCP3 - PWM Channel 3 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP3_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP3_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 3 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP3_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP3_PERIOD_SHIFT)) & PWM_PCP3_PERIOD_MASK)
#define PWM_PCP3_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP3_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 3 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP3_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP3_COMPARE_SHIFT)) & PWM_PCP3_COMPARE_MASK)
/*! @} */

/*! @name PCP4 - PWM Channel 4 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP4_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP4_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 4 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP4_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP4_PERIOD_SHIFT)) & PWM_PCP4_PERIOD_MASK)
#define PWM_PCP4_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP4_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 4 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP4_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP4_COMPARE_SHIFT)) & PWM_PCP4_COMPARE_MASK)
/*! @} */

/*! @name PCP5 - PWM Channel 5 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP5_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP5_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 5 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP5_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP5_PERIOD_SHIFT)) & PWM_PCP5_PERIOD_MASK)
#define PWM_PCP5_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP5_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 5 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP5_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP5_COMPARE_SHIFT)) & PWM_PCP5_COMPARE_MASK)
/*! @} */

/*! @name PCP6 - PWM Channel 6 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP6_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP6_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 6 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP6_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP6_PERIOD_SHIFT)) & PWM_PCP6_PERIOD_MASK)
#define PWM_PCP6_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP6_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 6 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP6_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP6_COMPARE_SHIFT)) & PWM_PCP6_COMPARE_MASK)
/*! @} */

/*! @name PCP7 - PWM Channel 7 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP7_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP7_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 7 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP7_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP7_PERIOD_SHIFT)) & PWM_PCP7_PERIOD_MASK)
#define PWM_PCP7_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP7_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 7 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP7_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP7_COMPARE_SHIFT)) & PWM_PCP7_COMPARE_MASK)
/*! @} */

/*! @name PCP8 - PWM Channel 8 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP8_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP8_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 8 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP8_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP8_PERIOD_SHIFT)) & PWM_PCP8_PERIOD_MASK)
#define PWM_PCP8_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP8_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 8 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP8_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP8_COMPARE_SHIFT)) & PWM_PCP8_COMPARE_MASK)
/*! @} */

/*! @name PCP9 - PWM Channel 9 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached PWM output will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP9_PERIOD_MASK                     (0xFFFFU)
#define PWM_PCP9_PERIOD_SHIFT                    (0U)
/*! PERIOD - PWM channel 9 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP9_PERIOD(x)                       (((uint32_t)(((uint32_t)(x)) << PWM_PCP9_PERIOD_SHIFT)) & PWM_PCP9_PERIOD_MASK)
#define PWM_PCP9_COMPARE_MASK                    (0xFFFF0000U)
#define PWM_PCP9_COMPARE_SHIFT                   (16U)
/*! COMPARE - PWM channel 9 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP9_COMPARE(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP9_COMPARE_SHIFT)) & PWM_PCP9_COMPARE_MASK)
/*! @} */

/*! @name PCP10 - PWM Channel 10 Period and Compare register. Counter will count down from period to zero. When Comapre value is reached all PWM outputs will change on next counter decrement and be stable from 'Compare-1' to 0. */
/*! @{ */
#define PWM_PCP10_PERIOD_MASK                    (0xFFFFU)
#define PWM_PCP10_PERIOD_SHIFT                   (0U)
/*! PERIOD - PWM channel 10 period register. The actual period equals to [PERIOD + 1]. 'PERIOD' must not be 0x0.
 */
#define PWM_PCP10_PERIOD(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_PCP10_PERIOD_SHIFT)) & PWM_PCP10_PERIOD_MASK)
#define PWM_PCP10_COMPARE_MASK                   (0xFFFF0000U)
#define PWM_PCP10_COMPARE_SHIFT                  (16U)
/*! COMPARE - PWM channel 10 compare register. 'COMPARE' must not be 0x0.
 */
#define PWM_PCP10_COMPARE(x)                     (((uint32_t)(((uint32_t)(x)) << PWM_PCP10_COMPARE_SHIFT)) & PWM_PCP10_COMPARE_MASK)
/*! @} */

/*! @name PST0 - PWM 1st Status Register (Channel 0 to Channel 3) */
/*! @{ */
#define PWM_PST0_INT_FLG_0_MASK                  (0x1U)
#define PWM_PST0_INT_FLG_0_SHIFT                 (0U)
/*! INT_FLG_0 - PWM channel 0 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST0_INT_FLG_0(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST0_INT_FLG_0_SHIFT)) & PWM_PST0_INT_FLG_0_MASK)
#define PWM_PST0_INT_FLG_1_MASK                  (0x100U)
#define PWM_PST0_INT_FLG_1_SHIFT                 (8U)
/*! INT_FLG_1 - PWM channel 1 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST0_INT_FLG_1(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST0_INT_FLG_1_SHIFT)) & PWM_PST0_INT_FLG_1_MASK)
#define PWM_PST0_INT_FLG_2_MASK                  (0x10000U)
#define PWM_PST0_INT_FLG_2_SHIFT                 (16U)
/*! INT_FLG_2 - PWM channel 2 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST0_INT_FLG_2(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST0_INT_FLG_2_SHIFT)) & PWM_PST0_INT_FLG_2_MASK)
#define PWM_PST0_INT_FLG_3_MASK                  (0x1000000U)
#define PWM_PST0_INT_FLG_3_SHIFT                 (24U)
/*! INT_FLG_3 - PWM channel 3 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST0_INT_FLG_3(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST0_INT_FLG_3_SHIFT)) & PWM_PST0_INT_FLG_3_MASK)
/*! @} */

/*! @name PST1 - PWM 2nd Status Register (Channel 4 to Channel 7) */
/*! @{ */
#define PWM_PST1_INT_FLG_4_MASK                  (0x1U)
#define PWM_PST1_INT_FLG_4_SHIFT                 (0U)
/*! INT_FLG_4 - PWM channel 4 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST1_INT_FLG_4(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST1_INT_FLG_4_SHIFT)) & PWM_PST1_INT_FLG_4_MASK)
#define PWM_PST1_INT_FLG_5_MASK                  (0x100U)
#define PWM_PST1_INT_FLG_5_SHIFT                 (8U)
/*! INT_FLG_5 - PWM channel 5 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST1_INT_FLG_5(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST1_INT_FLG_5_SHIFT)) & PWM_PST1_INT_FLG_5_MASK)
#define PWM_PST1_INT_FLG_6_MASK                  (0x10000U)
#define PWM_PST1_INT_FLG_6_SHIFT                 (16U)
/*! INT_FLG_6 - PWM channel 6 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST1_INT_FLG_6(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST1_INT_FLG_6_SHIFT)) & PWM_PST1_INT_FLG_6_MASK)
#define PWM_PST1_INT_FLG_7_MASK                  (0x1000000U)
#define PWM_PST1_INT_FLG_7_SHIFT                 (24U)
/*! INT_FLG_7 - PWM channel 7 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST1_INT_FLG_7(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST1_INT_FLG_7_SHIFT)) & PWM_PST1_INT_FLG_7_MASK)
/*! @} */

/*! @name PST2 - PWM 3rd Status Register (Channel 8 to Channel 10) */
/*! @{ */
#define PWM_PST2_INT_FLG_8_MASK                  (0x1U)
#define PWM_PST2_INT_FLG_8_SHIFT                 (0U)
/*! INT_FLG_8 - PWM channel 8 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST2_INT_FLG_8(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST2_INT_FLG_8_SHIFT)) & PWM_PST2_INT_FLG_8_MASK)
#define PWM_PST2_INT_FLG_9_MASK                  (0x100U)
#define PWM_PST2_INT_FLG_9_SHIFT                 (8U)
/*! INT_FLG_9 - PWM channel 9 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST2_INT_FLG_9(x)                    (((uint32_t)(((uint32_t)(x)) << PWM_PST2_INT_FLG_9_SHIFT)) & PWM_PST2_INT_FLG_9_MASK)
#define PWM_PST2_INT_FLG_10_MASK                 (0x10000U)
#define PWM_PST2_INT_FLG_10_SHIFT                (16U)
/*! INT_FLG_10 - PWM channel 10 interrupt flag. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear the interrupt.
 */
#define PWM_PST2_INT_FLG_10(x)                   (((uint32_t)(((uint32_t)(x)) << PWM_PST2_INT_FLG_10_SHIFT)) & PWM_PST2_INT_FLG_10_MASK)
/*! @} */

/*! @name MODULE_ID - PWM Module Identifier ('PW' in ASCII) */
/*! @{ */
#define PWM_MODULE_ID_APERTURE_MASK              (0xFFU)
#define PWM_MODULE_ID_APERTURE_SHIFT             (0U)
/*! APERTURE - Aperture i.e. number minus 1 of consecutive packets 4 Kbytes reserved for this IP
 */
#define PWM_MODULE_ID_APERTURE(x)                (((uint32_t)(((uint32_t)(x)) << PWM_MODULE_ID_APERTURE_SHIFT)) & PWM_MODULE_ID_APERTURE_MASK)
#define PWM_MODULE_ID_MIN_REV_MASK               (0xF00U)
#define PWM_MODULE_ID_MIN_REV_SHIFT              (8U)
/*! MIN_REV - Minor revision i.e. with no software consequences
 */
#define PWM_MODULE_ID_MIN_REV(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_MODULE_ID_MIN_REV_SHIFT)) & PWM_MODULE_ID_MIN_REV_MASK)
#define PWM_MODULE_ID_MAJ_REV_MASK               (0xF000U)
#define PWM_MODULE_ID_MAJ_REV_SHIFT              (12U)
/*! MAJ_REV - Major revision i.e. implies software modifications
 */
#define PWM_MODULE_ID_MAJ_REV(x)                 (((uint32_t)(((uint32_t)(x)) << PWM_MODULE_ID_MAJ_REV_SHIFT)) & PWM_MODULE_ID_MAJ_REV_MASK)
#define PWM_MODULE_ID_ID_MASK                    (0xFFFF0000U)
#define PWM_MODULE_ID_ID_SHIFT                   (16U)
/*! ID - Identifier. This is the unique identifier of the module
 */
#define PWM_MODULE_ID_ID(x)                      (((uint32_t)(((uint32_t)(x)) << PWM_MODULE_ID_ID_SHIFT)) & PWM_MODULE_ID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group PWM_Register_Masks */


/* PWM - Peripheral instance base addresses */
/** Peripheral PWM base address */
#define PWM_BASE                                 (0x4000C000u)
/** Peripheral PWM base pointer */
#define PWM                                      ((PWM_Type *)PWM_BASE)
/** Array initializer of PWM peripheral base addresses */
#define PWM_BASE_ADDRS                           { PWM_BASE }
/** Array initializer of PWM peripheral base pointers */
#define PWM_BASE_PTRS                            { PWM }
/** Interrupt vectors for the PWM peripheral type */
#define PWM_IRQS                                 { PWM0_IRQn, PWM1_IRQn, PWM2_IRQn, PWM3_IRQn, PWM4_IRQn, PWM5_IRQn, PWM6_IRQn, PWM7_IRQn, PWM8_IRQn, PWM9_IRQn, PWM10_IRQn }

/*!
 * @}
 */ /* end of group PWM_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- RNG Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RNG_Peripheral_Access_Layer RNG Peripheral Access Layer
 * @{
 */

/** RNG - Register Layout Typedef */
typedef struct {
  __I  uint32_t RANDOM_NUMBER;                     /**< Random number, offset: 0x0 */
       uint8_t RESERVED_0[4];
  __I  uint32_t COUNTER_VAL;                       /**< Counter values to show information about the random process, offset: 0x8 */
  __IO uint32_t COUNTER_CFG;                       /**< Register linked to the comupting of statistics, not required for normal operation., offset: 0xC */
  __IO uint32_t ONLINE_TEST_CFG;                   /**< Configuration for the online test features, offset: 0x10 */
  __I  uint32_t ONLINE_TEST_VAL;                   /**< Online test results, offset: 0x14 */
       uint8_t RESERVED_1[4060];
  __IO uint32_t POWERDOWN;                         /**< Powerdown mode and reset control, generally use of this register is not necessary, offset: 0xFF4 */
       uint8_t RESERVED_2[4];
  __I  uint32_t MODULEID;                          /**< IP identifier, offset: 0xFFC */
} RNG_Type;

/* ----------------------------------------------------------------------------
   -- RNG Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RNG_Register_Masks RNG Register Masks
 * @{
 */

/*! @name RANDOM_NUMBER - Random number */
/*! @{ */
#define RNG_RANDOM_NUMBER_RAND_NUM_MASK          (0xFFFFFFFFU)
#define RNG_RANDOM_NUMBER_RAND_NUM_SHIFT         (0U)
/*! RAND_NUM - This register contains a random 32 bit number which is computed on demand, at each
 *    time it is read. Weak cryptographic post-processing is used to maximize throughput. The block
 *    will start computing before the first register access and so the reset value is not relevant.
 */
#define RNG_RANDOM_NUMBER_RAND_NUM(x)            (((uint32_t)(((uint32_t)(x)) << RNG_RANDOM_NUMBER_RAND_NUM_SHIFT)) & RNG_RANDOM_NUMBER_RAND_NUM_MASK)
/*! @} */

/*! @name COUNTER_VAL - Counter values to show information about the random process */
/*! @{ */
#define RNG_COUNTER_VAL_CLK_RATIO_MASK           (0xFFU)
#define RNG_COUNTER_VAL_CLK_RATIO_SHIFT          (0U)
/*! CLK_RATIO - Gives the ratio between the internal clocks frequencies and the register clock
 *    frequency for evaluation and certification purposes. Internal clock frequencies are half the
 *    incoming ones: COUNTER_VAL = round[ (intFreq/2)/regFreq*256*(1<<(4*shift4x)) ] MODULO 256 If
 *    shitf4x==0, intFreq ~= regFreq*COUNTER_VAL/256*2 Use clock_sel to select which clock you want to
 *    measure, in this range: 1..5
 */
#define RNG_COUNTER_VAL_CLK_RATIO(x)             (((uint32_t)(((uint32_t)(x)) << RNG_COUNTER_VAL_CLK_RATIO_SHIFT)) & RNG_COUNTER_VAL_CLK_RATIO_MASK)
#define RNG_COUNTER_VAL_REFRESH_CNT_MASK         (0x1F00U)
#define RNG_COUNTER_VAL_REFRESH_CNT_SHIFT        (8U)
/*! REFRESH_CNT - Incremented (till max possible value) each time COUNTER was updated since last
 *    reading to any *_NUMBER. This gives an indication on 'entropy refill'. Note that there is no
 *    linear accumulation of entropy: as implemented today, entropy refill will be about 4 bits each
 *    time 'refresh_cnt' reaches its maximum value. See user manual for further details on how to
 *    benefit from linear entropy accumulation using a specific procedure.
 */
#define RNG_COUNTER_VAL_REFRESH_CNT(x)           (((uint32_t)(((uint32_t)(x)) << RNG_COUNTER_VAL_REFRESH_CNT_SHIFT)) & RNG_COUNTER_VAL_REFRESH_CNT_MASK)
/*! @} */

/*! @name COUNTER_CFG - Register linked to the comupting of statistics, not required for normal operation. */
/*! @{ */
#define RNG_COUNTER_CFG_MODE_MASK                (0x3U)
#define RNG_COUNTER_CFG_MODE_SHIFT               (0U)
/*! MODE - 00: disabled 01: update once. Will return to 00 once done 10: free running: updates countinuously 11: reserved
 */
#define RNG_COUNTER_CFG_MODE(x)                  (((uint32_t)(((uint32_t)(x)) << RNG_COUNTER_CFG_MODE_SHIFT)) & RNG_COUNTER_CFG_MODE_MASK)
#define RNG_COUNTER_CFG_CLOCK_SEL_MASK           (0x1CU)
#define RNG_COUNTER_CFG_CLOCK_SEL_SHIFT          (2U)
/*! CLOCK_SEL - Selects the internal clock on which to compute statistics. 1 is for first one, 2 for
 *    second one, . And 0 is for a XOR of results from all clocks
 */
#define RNG_COUNTER_CFG_CLOCK_SEL(x)             (((uint32_t)(((uint32_t)(x)) << RNG_COUNTER_CFG_CLOCK_SEL_SHIFT)) & RNG_COUNTER_CFG_CLOCK_SEL_MASK)
#define RNG_COUNTER_CFG_SHIFT4X_MASK             (0xE0U)
#define RNG_COUNTER_CFG_SHIFT4X_SHIFT            (5U)
/*! SHIFT4X - To be used to add precision to clock_ratio and determine 'entropy refill'. Supported
 *    range is 0..4 Used as well for ONLINE_TEST
 */
#define RNG_COUNTER_CFG_SHIFT4X(x)               (((uint32_t)(((uint32_t)(x)) << RNG_COUNTER_CFG_SHIFT4X_SHIFT)) & RNG_COUNTER_CFG_SHIFT4X_MASK)
/*! @} */

/*! @name ONLINE_TEST_CFG - Configuration for the online test features */
/*! @{ */
#define RNG_ONLINE_TEST_CFG_ACTIVATE_MASK        (0x1U)
#define RNG_ONLINE_TEST_CFG_ACTIVATE_SHIFT       (0U)
/*! ACTIVATE - 0: disabled 1: activated Update rhythm for VAL depends on COUNTER_CFG if data_sel is
 *    set to COUNTER. Otherwise VAL is updated each time RANDOM_NUMBER is read
 */
#define RNG_ONLINE_TEST_CFG_ACTIVATE(x)          (((uint32_t)(((uint32_t)(x)) << RNG_ONLINE_TEST_CFG_ACTIVATE_SHIFT)) & RNG_ONLINE_TEST_CFG_ACTIVATE_MASK)
#define RNG_ONLINE_TEST_CFG_DATA_SEL_MASK        (0x6U)
#define RNG_ONLINE_TEST_CFG_DATA_SEL_SHIFT       (1U)
/*! DATA_SEL - Selects source on which to apply online test: 00: LSB of COUNTER: raw data from one
 *    or all sources of entropy 01: MSB of COUNTER: raw data from one or all sources of entropy (do
 *    not use) 10: RANDOM_NUMBER 11: not valid 'activate' should be set to 'disabled' before changing
 *    this field
 */
#define RNG_ONLINE_TEST_CFG_DATA_SEL(x)          (((uint32_t)(((uint32_t)(x)) << RNG_ONLINE_TEST_CFG_DATA_SEL_SHIFT)) & RNG_ONLINE_TEST_CFG_DATA_SEL_MASK)
/*! @} */

/*! @name ONLINE_TEST_VAL - Online test results */
/*! @{ */
#define RNG_ONLINE_TEST_VAL_LIVE_CHI_SQUARED_MASK (0xFU)
#define RNG_ONLINE_TEST_VAL_LIVE_CHI_SQUARED_SHIFT (0U)
/*! LIVE_CHI_SQUARED - This value is updated as described in field 'activate'. This value is a
 *    statistic value that indicates the quality of entropy generation. Low value means good, high value
 *    means no good. If 'data_sel'<10, increase 'shift4x' till 'chi' is correct and poll
 *    'refresh_cnt' before reading RANDOM_NUMBER.
 */
#define RNG_ONLINE_TEST_VAL_LIVE_CHI_SQUARED(x)  (((uint32_t)(((uint32_t)(x)) << RNG_ONLINE_TEST_VAL_LIVE_CHI_SQUARED_SHIFT)) & RNG_ONLINE_TEST_VAL_LIVE_CHI_SQUARED_MASK)
#define RNG_ONLINE_TEST_VAL_MIN_CHI_SQUARED_MASK (0xF0U)
#define RNG_ONLINE_TEST_VAL_MIN_CHI_SQUARED_SHIFT (4U)
/*! MIN_CHI_SQUARED - Minimum value of LIVE_CHI_SQUARED since the last reset of this field. This field is reset when 'activate'=0
 */
#define RNG_ONLINE_TEST_VAL_MIN_CHI_SQUARED(x)   (((uint32_t)(((uint32_t)(x)) << RNG_ONLINE_TEST_VAL_MIN_CHI_SQUARED_SHIFT)) & RNG_ONLINE_TEST_VAL_MIN_CHI_SQUARED_MASK)
#define RNG_ONLINE_TEST_VAL_MAX_CHI_SQUARED_MASK (0xF00U)
#define RNG_ONLINE_TEST_VAL_MAX_CHI_SQUARED_SHIFT (8U)
/*! MAX_CHI_SQUARED - Maximum value of LIVE_CHI_SQUARED since the last reset of this field. This field is reset when 'activate'=0
 */
#define RNG_ONLINE_TEST_VAL_MAX_CHI_SQUARED(x)   (((uint32_t)(((uint32_t)(x)) << RNG_ONLINE_TEST_VAL_MAX_CHI_SQUARED_SHIFT)) & RNG_ONLINE_TEST_VAL_MAX_CHI_SQUARED_MASK)
/*! @} */

/*! @name POWERDOWN - Powerdown mode and reset control, generally use of this register is not necessary */
/*! @{ */
#define RNG_POWERDOWN_SOFT_RESET_MASK            (0x1U)
#define RNG_POWERDOWN_SOFT_RESET_SHIFT           (0U)
/*! SOFT_RESET - Request softreset that will go low automaticaly after acknowledge from CORE
 */
#define RNG_POWERDOWN_SOFT_RESET(x)              (((uint32_t)(((uint32_t)(x)) << RNG_POWERDOWN_SOFT_RESET_SHIFT)) & RNG_POWERDOWN_SOFT_RESET_MASK)
#define RNG_POWERDOWN_FORCE_SOFT_RESET_MASK      (0x2U)
#define RNG_POWERDOWN_FORCE_SOFT_RESET_SHIFT     (1U)
/*! FORCE_SOFT_RESET - When used with softreset it forces CORE_RESETN to low on acknowledge from CORE
 */
#define RNG_POWERDOWN_FORCE_SOFT_RESET(x)        (((uint32_t)(((uint32_t)(x)) << RNG_POWERDOWN_FORCE_SOFT_RESET_SHIFT)) & RNG_POWERDOWN_FORCE_SOFT_RESET_MASK)
#define RNG_POWERDOWN_POWERDOWN_MASK             (0x80000000U)
#define RNG_POWERDOWN_POWERDOWN_SHIFT            (31U)
/*! POWERDOWN - When set all accesses to standard registers are blocked
 */
#define RNG_POWERDOWN_POWERDOWN(x)               (((uint32_t)(((uint32_t)(x)) << RNG_POWERDOWN_POWERDOWN_SHIFT)) & RNG_POWERDOWN_POWERDOWN_MASK)
/*! @} */

/*! @name MODULEID - IP identifier */
/*! @{ */
#define RNG_MODULEID_APERTURE_MASK               (0xFFU)
#define RNG_MODULEID_APERTURE_SHIFT              (0U)
/*! APERTURE - Aperture i.e. number minus 1 of consecutive packets 4 Kbytes reserved for this IP
 */
#define RNG_MODULEID_APERTURE(x)                 (((uint32_t)(((uint32_t)(x)) << RNG_MODULEID_APERTURE_SHIFT)) & RNG_MODULEID_APERTURE_MASK)
#define RNG_MODULEID_MIN_REV_MASK                (0xF00U)
#define RNG_MODULEID_MIN_REV_SHIFT               (8U)
/*! MIN_REV - Minor revision i.e. with no software consequences
 */
#define RNG_MODULEID_MIN_REV(x)                  (((uint32_t)(((uint32_t)(x)) << RNG_MODULEID_MIN_REV_SHIFT)) & RNG_MODULEID_MIN_REV_MASK)
#define RNG_MODULEID_MAJ_REV_MASK                (0xF000U)
#define RNG_MODULEID_MAJ_REV_SHIFT               (12U)
/*! MAJ_REV - Major revision i.e. implies software modifications
 */
#define RNG_MODULEID_MAJ_REV(x)                  (((uint32_t)(((uint32_t)(x)) << RNG_MODULEID_MAJ_REV_SHIFT)) & RNG_MODULEID_MAJ_REV_MASK)
#define RNG_MODULEID_ID_MASK                     (0xFFFF0000U)
#define RNG_MODULEID_ID_SHIFT                    (16U)
/*! ID - Identifier. This is the unique identifier of the module
 */
#define RNG_MODULEID_ID(x)                       (((uint32_t)(((uint32_t)(x)) << RNG_MODULEID_ID_SHIFT)) & RNG_MODULEID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group RNG_Register_Masks */


/* RNG - Peripheral instance base addresses */
/** Peripheral RNG base address */
#define RNG_BASE                                 (0x4000D000u)
/** Peripheral RNG base pointer */
#define RNG                                      ((RNG_Type *)RNG_BASE)
/** Array initializer of RNG peripheral base addresses */
#define RNG_BASE_ADDRS                           { RNG_BASE }
/** Array initializer of RNG peripheral base pointers */
#define RNG_BASE_PTRS                            { RNG }

/*!
 * @}
 */ /* end of group RNG_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- RTC Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RTC_Peripheral_Access_Layer RTC Peripheral Access Layer
 * @{
 */

/** RTC - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< RTC control register, offset: 0x0 */
  __IO uint32_t MATCH;                             /**< RTC 32-bit counter match register, offset: 0x4 */
  __IO uint32_t COUNT;                             /**< RTC 32-bit counter register, offset: 0x8 */
  __IO uint32_t WAKE;                              /**< 16-bit RTC timer register, offset: 0xC */
} RTC_Type;

/* ----------------------------------------------------------------------------
   -- RTC Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup RTC_Register_Masks RTC Register Masks
 * @{
 */

/*! @name CTRL - RTC control register */
/*! @{ */
#define RTC_CTRL_SWRESET_MASK                    (0x1U)
#define RTC_CTRL_SWRESET_SHIFT                   (0U)
/*! SWRESET - Software reset control. 0: Not in reset. The RTC is not held in reset. This bit must
 *    be cleared prior to configuring or initiating any operation of the RTC. 1: In reset. The RTC is
 *    held in reset. All register bits within the RTC will be forced to their reset value except
 *    the OFD bit. This bit must be cleared before writing to any register in the RTC - including
 *    writes to set any of the other bits within this register. Do not attempt to write to any bits of
 *    this register at the same time that the reset bit is being cleared.
 */
#define RTC_CTRL_SWRESET(x)                      (((uint32_t)(((uint32_t)(x)) << RTC_CTRL_SWRESET_SHIFT)) & RTC_CTRL_SWRESET_MASK)
#define RTC_CTRL_ALARM1HZ_MASK                   (0x4U)
#define RTC_CTRL_ALARM1HZ_SHIFT                  (2U)
/*! ALARM1HZ - RTC 32-bit timer alarm flag status. 0: No match has occurred on the 32-bit RTC timer.
 *    Writing a 0 has no effect. 1: A match condition has occurred on the 32-bit RTC timer. This
 *    flag generates an RTC alarm interrupt request. RTC_ALARM which can also wake up the part from
 *    low power modes (excluding deep power down mode). Writing a 1 clears this bit.
 */
#define RTC_CTRL_ALARM1HZ(x)                     (((uint32_t)(((uint32_t)(x)) << RTC_CTRL_ALARM1HZ_SHIFT)) & RTC_CTRL_ALARM1HZ_MASK)
#define RTC_CTRL_WAKE1KHZ_MASK                   (0x8U)
#define RTC_CTRL_WAKE1KHZ_SHIFT                  (3U)
/*! WAKE1KHZ - RTC 16-bit timer wake-up flag status. 0: The RTC 16-bit timer is running. Writing a 0
 *    has no effect. 1: The 16-bit timer has timed out. This flag generates an RTC wake-up
 *    interrupt request RTC-WAKE which can also wake up the part from low power modes (excluding deep power
 *    down mode). Writing a 1 clears this bit.
 */
#define RTC_CTRL_WAKE1KHZ(x)                     (((uint32_t)(((uint32_t)(x)) << RTC_CTRL_WAKE1KHZ_SHIFT)) & RTC_CTRL_WAKE1KHZ_MASK)
#define RTC_CTRL_ALARMDPD_EN_MASK                (0x10U)
#define RTC_CTRL_ALARMDPD_EN_SHIFT               (4U)
/*! ALARMDPD_EN - RTC 32-bit timer alarm enable for Low power mode. 0: Disable. A match on the
 *    32-bit RTC timer will not bring the part out of power-down modes. 1: Enable. A match on the 32-bit
 *    RTC timer will bring the part out of power-down modes.
 */
#define RTC_CTRL_ALARMDPD_EN(x)                  (((uint32_t)(((uint32_t)(x)) << RTC_CTRL_ALARMDPD_EN_SHIFT)) & RTC_CTRL_ALARMDPD_EN_MASK)
#define RTC_CTRL_WAKEDPD_EN_MASK                 (0x20U)
#define RTC_CTRL_WAKEDPD_EN_SHIFT                (5U)
/*! WAKEDPD_EN - RTC 16-bit timer wake-up enable for power-down modes. 0: Disable. A match on the
 *    16-bit RTC timer will not bring the part out of power-down modes. 1: Enable. A match on the
 *    16-bit RTC timer will bring the part out of power-down modes.
 */
#define RTC_CTRL_WAKEDPD_EN(x)                   (((uint32_t)(((uint32_t)(x)) << RTC_CTRL_WAKEDPD_EN_SHIFT)) & RTC_CTRL_WAKEDPD_EN_MASK)
#define RTC_CTRL_RTC1KHZ_EN_MASK                 (0x40U)
#define RTC_CTRL_RTC1KHZ_EN_SHIFT                (6U)
/*! RTC1KHZ_EN - RTC 16-bit timer clock enable. This bit can be set to 0 to conserve power if the
 *    16-bit timer is not used. This bit has no effect when the RTC is disabled (bit 7 of this
 *    register is 0). 0: Disable. A match on the 16-bit RTC timer will not bring the part out of Deep
 *    power-down mode. 1: Enable. The 16-bit RTC timer is enabled.
 */
#define RTC_CTRL_RTC1KHZ_EN(x)                   (((uint32_t)(((uint32_t)(x)) << RTC_CTRL_RTC1KHZ_EN_SHIFT)) & RTC_CTRL_RTC1KHZ_EN_MASK)
#define RTC_CTRL_RTC_EN_MASK                     (0x80U)
#define RTC_CTRL_RTC_EN_SHIFT                    (7U)
/*! RTC_EN - RTC enable. 0: Disable. The RTC 32-bit timer and 16-bit timer clocks are shut down and
 *    the RTC operation is disabled. This bit should be 0 when writing to load a value in the RTC
 *    counter register. 1: Enable. The 32-bit RTC clock is running and RTC operation is enabled. This
 *    bit must be set to initiate operation of the RTC. To also enable the 16-bit timer clock, set
 *    bit 6 in this register.
 */
#define RTC_CTRL_RTC_EN(x)                       (((uint32_t)(((uint32_t)(x)) << RTC_CTRL_RTC_EN_SHIFT)) & RTC_CTRL_RTC_EN_MASK)
/*! @} */

/*! @name MATCH - RTC 32-bit counter match register */
/*! @{ */
#define RTC_MATCH_MATVAL_MASK                    (0xFFFFFFFFU)
#define RTC_MATCH_MATVAL_SHIFT                   (0U)
/*! MATVAL - Contains the match value against which the 1 Hz RTC timer will be compared to generate
 *    the alarm flag RTC_ALARM and generate an alarm interrupt/wake-up if enabled.
 */
#define RTC_MATCH_MATVAL(x)                      (((uint32_t)(((uint32_t)(x)) << RTC_MATCH_MATVAL_SHIFT)) & RTC_MATCH_MATVAL_MASK)
/*! @} */

/*! @name COUNT - RTC 32-bit counter register */
/*! @{ */
#define RTC_COUNT_VAL_MASK                       (0xFFFFFFFFU)
#define RTC_COUNT_VAL_SHIFT                      (0U)
/*! VAL - A read reflects the current value of the main, 32-bit, RTC timer. A write loads a new
 *    initial value into the timer. The RTC 32-bit counter will count up continuously at the 32-bit
 *    timer clock rate once the RTC Software Reset is removed (by clearing bit 0 of the CTRL register).
 *    Remark: Only write to this register when the RTC_EN bit in the RTC CTRL Register is 0.
 */
#define RTC_COUNT_VAL(x)                         (((uint32_t)(((uint32_t)(x)) << RTC_COUNT_VAL_SHIFT)) & RTC_COUNT_VAL_MASK)
/*! @} */

/*! @name WAKE - 16-bit RTC timer register */
/*! @{ */
#define RTC_WAKE_VAL_MASK                        (0xFFFFU)
#define RTC_WAKE_VAL_SHIFT                       (0U)
/*! VAL - A read reflects the current value of 16-bit timer. A write pre-loads a start count value
 *    into the 16-bit timer and initializes a count-down sequence. Do not write to this register
 *    while counting is in progress.
 */
#define RTC_WAKE_VAL(x)                          (((uint32_t)(((uint32_t)(x)) << RTC_WAKE_VAL_SHIFT)) & RTC_WAKE_VAL_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group RTC_Register_Masks */


/* RTC - Peripheral instance base addresses */
/** Peripheral RTC base address */
#define RTC_BASE                                 (0x4000B000u)
/** Peripheral RTC base pointer */
#define RTC                                      ((RTC_Type *)RTC_BASE)
/** Array initializer of RTC peripheral base addresses */
#define RTC_BASE_ADDRS                           { RTC_BASE }
/** Array initializer of RTC peripheral base pointers */
#define RTC_BASE_PTRS                            { RTC }
/** Interrupt vectors for the RTC peripheral type */
#define RTC_IRQS                                 { RTC_IRQn }

/*!
 * @}
 */ /* end of group RTC_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- SHA Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SHA_Peripheral_Access_Layer SHA Peripheral Access Layer
 * @{
 */

/** SHA - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< Control register, offset: 0x0 */
  __I  uint32_t STATUS;                            /**< Status Regsiter, offset: 0x4 */
  __IO uint32_t INTENSET;                          /**< Interrupt Enable and Interrupt enable set function, offset: 0x8 */
  __O  uint32_t INTENCLR;                          /**< Interrupt Clear Register, offset: 0xC */
  __IO uint32_t MEMCTRL;                           /**< Setup Master to access memory, offset: 0x10 */
  __IO uint32_t MEMADDR;                           /**< Address to start memory access from, offset: 0x14 */
       uint8_t RESERVED_0[8];
  __IO uint32_t INDATA[8];                         /**< Input Data register, array offset: 0x20, array step: 0x4 */
  __I  uint32_t DIGEST[8];                         /**< DIGEST or OUTD0, 5 or 8 bytes of output data, depending upon mode, array offset: 0x40, array step: 0x4 */
       uint8_t RESERVED_1[48];
  __IO uint32_t MASK;                              /**< Mask register, offset: 0x90 */
       uint8_t RESERVED_2[3944];
  __I  uint32_t ID;                                /**< IP identifier, offset: 0xFFC */
} SHA_Type;

/* ----------------------------------------------------------------------------
   -- SHA Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SHA_Register_Masks SHA Register Masks
 * @{
 */

/*! @name CTRL - Control register */
/*! @{ */
#define SHA_CTRL_MODE_MASK                       (0x7U)
#define SHA_CTRL_MODE_SHIFT                      (0U)
/*! MODE - Operational mode: 0: Disabled; 1: SHA1; 2: SHA2-256; 3: SHA2-512; 4-7: Not valid
 */
#define SHA_CTRL_MODE(x)                         (((uint32_t)(((uint32_t)(x)) << SHA_CTRL_MODE_SHIFT)) & SHA_CTRL_MODE_MASK)
#define SHA_CTRL_NEW_MASK                        (0x10U)
#define SHA_CTRL_NEW_SHIFT                       (4U)
/*! NEW - Written with 1 when starting a new Hash. It self clears. Note that the WAITING Status bit
 *    will clear for a cycle during the initialization from New=1. Digest/Result is initialized when
 *    New is set to 1.
 */
#define SHA_CTRL_NEW(x)                          (((uint32_t)(((uint32_t)(x)) << SHA_CTRL_NEW_SHIFT)) & SHA_CTRL_NEW_MASK)
#define SHA_CTRL_DMA_I_MASK                      (0x100U)
#define SHA_CTRL_DMA_I_SHIFT                     (8U)
/*! DMA_I - Written with 1 to use DMA to fill INDATA. For Hash, will request from DMA for 16 words
 *    and then will process the Hash. Normal model is that the DMA interrupts the processor when its
 *    length expires.
 */
#define SHA_CTRL_DMA_I(x)                        (((uint32_t)(((uint32_t)(x)) << SHA_CTRL_DMA_I_SHIFT)) & SHA_CTRL_DMA_I_MASK)
#define SHA_CTRL_DMA_O_MASK                      (0x200U)
#define SHA_CTRL_DMA_O_SHIFT                     (9U)
/*! DMA_O - Written to 1 to use DMA to drain the digest/output. If both DMA_I and DMA_O are set, the
 *    DMA has to know to switch direction and the locations. If written to 0 the DMA is not used
 *    and the processor has to read the digest/output in response to the DIGEST interrupt.
 */
#define SHA_CTRL_DMA_O(x)                        (((uint32_t)(((uint32_t)(x)) << SHA_CTRL_DMA_O_SHIFT)) & SHA_CTRL_DMA_O_MASK)
#define SHA_CTRL_HASHSWPB_MASK                   (0x1000U)
#define SHA_CTRL_HASHSWPB_SHIFT                  (12U)
/*! HASHSWPB - If 1, will swap bytes in the word for SHA hashing. The default is byte order (so LSB
 *    is first byte) but this allows swapping to MSB is first.
 */
#define SHA_CTRL_HASHSWPB(x)                     (((uint32_t)(((uint32_t)(x)) << SHA_CTRL_HASHSWPB_SHIFT)) & SHA_CTRL_HASHSWPB_MASK)
/*! @} */

/*! @name STATUS - Status Regsiter */
/*! @{ */
#define SHA_STATUS_WAITING_MASK                  (0x1U)
#define SHA_STATUS_WAITING_SHIFT                 (0U)
/*! WAITING - Waiting Status 0: Not waiting for data may be disabled or may be busy. Note that for
 *    cryptographic uses, this is not set if IsLast is set nor will it set until at least 1 word is
 *    read of the output. 1: Waiting for data to be written in (16 words)
 */
#define SHA_STATUS_WAITING(x)                    (((uint32_t)(((uint32_t)(x)) << SHA_STATUS_WAITING_SHIFT)) & SHA_STATUS_WAITING_MASK)
#define SHA_STATUS_DIGEST_MASK                   (0x2U)
#define SHA_STATUS_DIGEST_SHIFT                  (1U)
/*! DIGEST - If 1 then a DIGEST is ready and waiting and there is no active next block already
 *    started. This is cleared when any data is written, when New is written, or when the block is
 *    disabled.
 */
#define SHA_STATUS_DIGEST(x)                     (((uint32_t)(((uint32_t)(x)) << SHA_STATUS_DIGEST_SHIFT)) & SHA_STATUS_DIGEST_MASK)
#define SHA_STATUS_ERROR_MASK                    (0x4U)
#define SHA_STATUS_ERROR_SHIFT                   (2U)
/*! ERROR - If 1, an error occurred. For normal uses, this is due to an attempted overrun: INDATA
 *    was written when it was not appropriate. Write 1 to clear.
 */
#define SHA_STATUS_ERROR(x)                      (((uint32_t)(((uint32_t)(x)) << SHA_STATUS_ERROR_SHIFT)) & SHA_STATUS_ERROR_MASK)
#define SHA_STATUS_NEEDKEY_MASK                  (0x10U)
#define SHA_STATUS_NEEDKEY_SHIFT                 (4U)
/*! NEEDKEY - Indicates the block wants the key to be written in (set along with WAITING). 0: no key
 *    is needed and writes will not be treated as Key. 1: key is needed and INDATA will be accepted
 *    as key. Will also set WAITING.
 */
#define SHA_STATUS_NEEDKEY(x)                    (((uint32_t)(((uint32_t)(x)) << SHA_STATUS_NEEDKEY_SHIFT)) & SHA_STATUS_NEEDKEY_MASK)
#define SHA_STATUS_NEEDIV_MASK                   (0x20U)
#define SHA_STATUS_NEEDIV_SHIFT                  (5U)
/*! NEEDIV - Indicates the block wants an IV/NONE to be written in (set along with WAITING). 0: no
 *    IV is needed, either because written already or because not needed. 1: IV needed and INDATA
 *    will be accepted as IV.
 */
#define SHA_STATUS_NEEDIV(x)                     (((uint32_t)(((uint32_t)(x)) << SHA_STATUS_NEEDIV_SHIFT)) & SHA_STATUS_NEEDIV_MASK)
/*! @} */

/*! @name INTENSET - Interrupt Enable and Interrupt enable set function */
/*! @{ */
#define SHA_INTENSET_WAITING_MASK                (0x1U)
#define SHA_INTENSET_WAITING_SHIFT               (0U)
/*! WAITING - 0: Will not interrupt when waiting. 1: Will interrupt when waiting. Write 1 to set this bit.
 */
#define SHA_INTENSET_WAITING(x)                  (((uint32_t)(((uint32_t)(x)) << SHA_INTENSET_WAITING_SHIFT)) & SHA_INTENSET_WAITING_MASK)
#define SHA_INTENSET_DIGEST_MASK                 (0x2U)
#define SHA_INTENSET_DIGEST_SHIFT                (1U)
/*! DIGEST - 0: Will not interrupt when Digest is ready. 1: Will interrupt when Digest is ready. Write 1 to set this bit.
 */
#define SHA_INTENSET_DIGEST(x)                   (((uint32_t)(((uint32_t)(x)) << SHA_INTENSET_DIGEST_SHIFT)) & SHA_INTENSET_DIGEST_MASK)
#define SHA_INTENSET_ERROR_MASK                  (0x4U)
#define SHA_INTENSET_ERROR_SHIFT                 (2U)
/*! ERROR - 0: Will not interrupt on Error. 1: Will interrupt on Error. Write 1 to set this bit.
 */
#define SHA_INTENSET_ERROR(x)                    (((uint32_t)(((uint32_t)(x)) << SHA_INTENSET_ERROR_SHIFT)) & SHA_INTENSET_ERROR_MASK)
/*! @} */

/*! @name INTENCLR - Interrupt Clear Register */
/*! @{ */
#define SHA_INTENCLR_WAITING_MASK                (0x1U)
#define SHA_INTENCLR_WAITING_SHIFT               (0U)
/*! WAITING - Write 1 to clear correspnding bit in INTENSET.
 */
#define SHA_INTENCLR_WAITING(x)                  (((uint32_t)(((uint32_t)(x)) << SHA_INTENCLR_WAITING_SHIFT)) & SHA_INTENCLR_WAITING_MASK)
#define SHA_INTENCLR_DIGEST_MASK                 (0x2U)
#define SHA_INTENCLR_DIGEST_SHIFT                (1U)
/*! DIGEST - Write 1 to clear correspnding bit in INTENSET.
 */
#define SHA_INTENCLR_DIGEST(x)                   (((uint32_t)(((uint32_t)(x)) << SHA_INTENCLR_DIGEST_SHIFT)) & SHA_INTENCLR_DIGEST_MASK)
#define SHA_INTENCLR_ERROR_MASK                  (0x4U)
#define SHA_INTENCLR_ERROR_SHIFT                 (2U)
/*! ERROR - Write 1 to clear correspnding bit in INTENSET.
 */
#define SHA_INTENCLR_ERROR(x)                    (((uint32_t)(((uint32_t)(x)) << SHA_INTENCLR_ERROR_SHIFT)) & SHA_INTENCLR_ERROR_MASK)
/*! @} */

/*! @name MEMCTRL - Setup Master to access memory */
/*! @{ */
#define SHA_MEMCTRL_MASTER_MASK                  (0x1U)
#define SHA_MEMCTRL_MASTER_SHIFT                 (0U)
/*! MASTER - Enables Mastering. 0: Mastering is not used and the normal DMA or Interrupt based model
 *    is used with INDATA. 1: Mastering is enabled and DMA and INDATA should not be used.
 */
#define SHA_MEMCTRL_MASTER(x)                    (((uint32_t)(((uint32_t)(x)) << SHA_MEMCTRL_MASTER_SHIFT)) & SHA_MEMCTRL_MASTER_MASK)
#define SHA_MEMCTRL_COUNT_MASK                   (0x7FF0000U)
#define SHA_MEMCTRL_COUNT_SHIFT                  (16U)
/*! COUNT - Number of 512-bit blocks to copy starting at MEMADDR. This register will decrement after
 *    each block is copied, ending in 0. For Hash, the DIGEST interrupt will occur when it reaches
 *    0. If a bus error occurs, it will stop with this field set to the block that failed
 */
#define SHA_MEMCTRL_COUNT(x)                     (((uint32_t)(((uint32_t)(x)) << SHA_MEMCTRL_COUNT_SHIFT)) & SHA_MEMCTRL_COUNT_MASK)
/*! @} */

/*! @name MEMADDR - Address to start memory access from */
/*! @{ */
#define SHA_MEMADDR_BASEADDR_MASK                (0xFFFFFFFFU)
#define SHA_MEMADDR_BASEADDR_SHIFT               (0U)
/*! BASEADDR - This field indicates the base address in Internal Flash, SRAM0, SRAMX, or SPIFI to start copying from.
 */
#define SHA_MEMADDR_BASEADDR(x)                  (((uint32_t)(((uint32_t)(x)) << SHA_MEMADDR_BASEADDR_SHIFT)) & SHA_MEMADDR_BASEADDR_MASK)
/*! @} */

/*! @name INDATA - Input Data register */
/*! @{ */
#define SHA_INDATA_DATA_MASK                     (0xFFFFFFFFU)
#define SHA_INDATA_DATA_SHIFT                    (0U)
/*! DATA - In this field the next word is written in little-endian format.
 */
#define SHA_INDATA_DATA(x)                       (((uint32_t)(((uint32_t)(x)) << SHA_INDATA_DATA_SHIFT)) & SHA_INDATA_DATA_MASK)
/*! @} */

/* The count of SHA_INDATA */
#define SHA_INDATA_COUNT                         (8U)

/*! @name DIGEST - DIGEST or OUTD0, 5 or 8 bytes of output data, depending upon mode */
/*! @{ */
#define SHA_DIGEST_DIGEST_MASK                   (0xFFFFFFFFU)
#define SHA_DIGEST_DIGEST_SHIFT                  (0U)
/*! DIGEST - This field contains one word of the Digest.
 */
#define SHA_DIGEST_DIGEST(x)                     (((uint32_t)(((uint32_t)(x)) << SHA_DIGEST_DIGEST_SHIFT)) & SHA_DIGEST_DIGEST_MASK)
/*! @} */

/* The count of SHA_DIGEST */
#define SHA_DIGEST_COUNT                         (8U)

/*! @name MASK - Mask register */
/*! @{ */
#define SHA_MASK_MASK_MASK                       (0xFFFFFFFFU)
#define SHA_MASK_MASK_SHIFT                      (0U)
/*! MASK - Mask register
 */
#define SHA_MASK_MASK(x)                         (((uint32_t)(((uint32_t)(x)) << SHA_MASK_MASK_SHIFT)) & SHA_MASK_MASK_MASK)
/*! @} */

/*! @name ID - IP identifier */
/*! @{ */
#define SHA_ID_APERTURE_MASK                     (0xFFU)
#define SHA_ID_APERTURE_SHIFT                    (0U)
/*! APERTURE - Aperture i.e. number minus 1 of consecutive packets 4 Kbytes reserved for this IP
 */
#define SHA_ID_APERTURE(x)                       (((uint32_t)(((uint32_t)(x)) << SHA_ID_APERTURE_SHIFT)) & SHA_ID_APERTURE_MASK)
#define SHA_ID_MIN_REV_MASK                      (0xF00U)
#define SHA_ID_MIN_REV_SHIFT                     (8U)
/*! MIN_REV - Minor revision i.e. with no software consequences
 */
#define SHA_ID_MIN_REV(x)                        (((uint32_t)(((uint32_t)(x)) << SHA_ID_MIN_REV_SHIFT)) & SHA_ID_MIN_REV_MASK)
#define SHA_ID_MAJ_REV_MASK                      (0xF000U)
#define SHA_ID_MAJ_REV_SHIFT                     (12U)
/*! MAJ_REV - Major revision i.e. implies software modifications
 */
#define SHA_ID_MAJ_REV(x)                        (((uint32_t)(((uint32_t)(x)) << SHA_ID_MAJ_REV_SHIFT)) & SHA_ID_MAJ_REV_MASK)
#define SHA_ID_ID_MASK                           (0xFFFF0000U)
#define SHA_ID_ID_SHIFT                          (16U)
/*! ID - Identifier. This is the unique identifier of the module
 */
#define SHA_ID_ID(x)                             (((uint32_t)(((uint32_t)(x)) << SHA_ID_ID_SHIFT)) & SHA_ID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group SHA_Register_Masks */


/* SHA - Peripheral instance base addresses */
/** Peripheral SHA0 base address */
#define SHA0_BASE                                (0x4008F000u)
/** Peripheral SHA0 base pointer */
#define SHA0                                     ((SHA_Type *)SHA0_BASE)
/** Array initializer of SHA peripheral base addresses */
#define SHA_BASE_ADDRS                           { SHA0_BASE }
/** Array initializer of SHA peripheral base pointers */
#define SHA_BASE_PTRS                            { SHA0 }
/** Interrupt vectors for the SHA peripheral type */
#define SHA_IRQS                                 { SHA_IRQn }

/*!
 * @}
 */ /* end of group SHA_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- SPI Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SPI_Peripheral_Access_Layer SPI Peripheral Access Layer
 * @{
 */

/** SPI - Register Layout Typedef */
typedef struct {
       uint8_t RESERVED_0[1024];
  __IO uint32_t CFG;                               /**< SPI Configuration register, offset: 0x400 */
  __IO uint32_t DLY;                               /**< SPI Delay register, offset: 0x404 */
  __IO uint32_t STAT;                              /**< SPI Status. Some status flags can be cleared by writing a 1 to that bit position., offset: 0x408 */
  __IO uint32_t INTENSET;                          /**< SPI Interrupt Enable read and Set. A complete value may be read from this register. Writing a 1 to any implemented bit position causes that bit to be set., offset: 0x40C */
  __IO uint32_t INTENCLR;                          /**< SPI Interrupt Enable Clear. Writing a 1 to any implemented bit position causes the corresponding bit in INTENSET to be cleared., offset: 0x410 */
       uint8_t RESERVED_1[12];
  __IO uint32_t TXCTL;                             /**< SPI Transmit Control. If Transmit FIFO is enabled, in FIFOCFG, then values read in this register are affected values in FIFO., offset: 0x420 */
  __IO uint32_t DIV;                               /**< SPI clock Divider, offset: 0x424 */
  __I  uint32_t INTSTAT;                           /**< SPI Interrupt Status, offset: 0x428 */
       uint8_t RESERVED_2[2516];
  __IO uint32_t FIFOCFG;                           /**< FIFO configuration and enable register., offset: 0xE00 */
  __IO uint32_t FIFOSTAT;                          /**< FIFO status register., offset: 0xE04 */
  __IO uint32_t FIFOTRIG;                          /**< FIFO trigger settings for interrupt and DMA request., offset: 0xE08 */
       uint8_t RESERVED_3[4];
  __IO uint32_t FIFOINTENSET;                      /**< FIFO interrupt enable set (enable) and read register., offset: 0xE10 */
  __IO uint32_t FIFOINTENCLR;                      /**< FIFO interrupt enable clear (disable) and read register., offset: 0xE14 */
  __I  uint32_t FIFOINTSTAT;                       /**< FIFO interrupt status register., offset: 0xE18 */
       uint8_t RESERVED_4[4];
  __O  uint32_t FIFOWR;                            /**< FIFO write data. FIFO data not reset by block reset, offset: 0xE20 */
       uint8_t RESERVED_5[12];
  __I  uint32_t FIFORD;                            /**< FIFO read data., offset: 0xE30 */
       uint8_t RESERVED_6[12];
  __I  uint32_t FIFORDNOPOP;                       /**< FIFO data read with no FIFO pop., offset: 0xE40 */
       uint8_t RESERVED_7[436];
  __IO uint32_t PSELID;                            /**< Peripheral function select and ID register, offset: 0xFF8 */
  __I  uint32_t ID;                                /**< SPI Module Identifier, offset: 0xFFC */
} SPI_Type;

/* ----------------------------------------------------------------------------
   -- SPI Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SPI_Register_Masks SPI Register Masks
 * @{
 */

/*! @name CFG - SPI Configuration register */
/*! @{ */
#define SPI_CFG_ENABLE_MASK                      (0x1U)
#define SPI_CFG_ENABLE_SHIFT                     (0U)
/*! ENABLE - SPI enable.
 */
#define SPI_CFG_ENABLE(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_CFG_ENABLE_SHIFT)) & SPI_CFG_ENABLE_MASK)
#define SPI_CFG_MASTER_MASK                      (0x4U)
#define SPI_CFG_MASTER_SHIFT                     (2U)
/*! MASTER - Master mode select.
 */
#define SPI_CFG_MASTER(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_CFG_MASTER_SHIFT)) & SPI_CFG_MASTER_MASK)
#define SPI_CFG_LSBF_MASK                        (0x8U)
#define SPI_CFG_LSBF_SHIFT                       (3U)
/*! LSBF - LSB First mode enable.
 */
#define SPI_CFG_LSBF(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_CFG_LSBF_SHIFT)) & SPI_CFG_LSBF_MASK)
#define SPI_CFG_CPHA_MASK                        (0x10U)
#define SPI_CFG_CPHA_SHIFT                       (4U)
/*! CPHA - Clock Phase select.
 */
#define SPI_CFG_CPHA(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_CFG_CPHA_SHIFT)) & SPI_CFG_CPHA_MASK)
#define SPI_CFG_CPOL_MASK                        (0x20U)
#define SPI_CFG_CPOL_SHIFT                       (5U)
/*! CPOL - Clock Polarity select.
 */
#define SPI_CFG_CPOL(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_CFG_CPOL_SHIFT)) & SPI_CFG_CPOL_MASK)
#define SPI_CFG_LOOP_MASK                        (0x80U)
#define SPI_CFG_LOOP_SHIFT                       (7U)
/*! LOOP - Loopback mode enable. Loopback mode applies only to Master mode, and connects transmit
 *    and receive data connected together to allow simple software testing.
 */
#define SPI_CFG_LOOP(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_CFG_LOOP_SHIFT)) & SPI_CFG_LOOP_MASK)
#define SPI_CFG_SPOL0_MASK                       (0x100U)
#define SPI_CFG_SPOL0_SHIFT                      (8U)
/*! SPOL0 - SSEL0 Polarity select.
 */
#define SPI_CFG_SPOL0(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_CFG_SPOL0_SHIFT)) & SPI_CFG_SPOL0_MASK)
#define SPI_CFG_SPOL1_MASK                       (0x200U)
#define SPI_CFG_SPOL1_SHIFT                      (9U)
/*! SPOL1 - SSEL1 Polarity select. Valid only for SPI-1
 */
#define SPI_CFG_SPOL1(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_CFG_SPOL1_SHIFT)) & SPI_CFG_SPOL1_MASK)
#define SPI_CFG_SPOL2_MASK                       (0x400U)
#define SPI_CFG_SPOL2_SHIFT                      (10U)
/*! SPOL2 - SSEL2 Polarity select. Valid only for SPI-1
 */
#define SPI_CFG_SPOL2(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_CFG_SPOL2_SHIFT)) & SPI_CFG_SPOL2_MASK)
/*! @} */

/*! @name DLY - SPI Delay register */
/*! @{ */
#define SPI_DLY_PRE_DELAY_MASK                   (0xFU)
#define SPI_DLY_PRE_DELAY_SHIFT                  (0U)
/*! PRE_DELAY - Controls the amount of time between SSEL assertion and the beginning of a data
 *    transfer. There is always one SPI clock time between SSEL assertion and the first clock edge. This
 *    is not considered part of the pre-delay. 0x0 = No additional time is inserted. 0x1 = 1 SPI
 *    clock time is inserted. 0x2 = 2 SPI clock times are inserted. ... 0xF = 15 SPI clock times are
 *    inserted.
 */
#define SPI_DLY_PRE_DELAY(x)                     (((uint32_t)(((uint32_t)(x)) << SPI_DLY_PRE_DELAY_SHIFT)) & SPI_DLY_PRE_DELAY_MASK)
#define SPI_DLY_POST_DELAY_MASK                  (0xF0U)
#define SPI_DLY_POST_DELAY_SHIFT                 (4U)
/*! POST_DELAY - Controls the amount of time between the end of a data transfer and SSEL
 *    deassertion. 0x0 = No additional time is inserted. 0x1 = 1 SPI clock time is inserted. 0x2 = 2 SPI clock
 *    times are inserted. ... 0xF = 15 SPI clock times are inserted.
 */
#define SPI_DLY_POST_DELAY(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_DLY_POST_DELAY_SHIFT)) & SPI_DLY_POST_DELAY_MASK)
#define SPI_DLY_FRAME_DELAY_MASK                 (0xF00U)
#define SPI_DLY_FRAME_DELAY_SHIFT                (8U)
/*! FRAME_DELAY - If the EOFR flag is set, controls the minimum amount of time between the current
 *    frame and the next frame (or SSEL deassertion if EOTR). 0x0 = No additional time is inserted.
 *    0x1 = 1 SPI clock time is inserted. 0x2 = 2 SPI clock times are inserted. ... 0xF = 15 SPI
 *    clock times are inserted.
 */
#define SPI_DLY_FRAME_DELAY(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_DLY_FRAME_DELAY_SHIFT)) & SPI_DLY_FRAME_DELAY_MASK)
#define SPI_DLY_TRANSFER_DELAY_MASK              (0xF000U)
#define SPI_DLY_TRANSFER_DELAY_SHIFT             (12U)
/*! TRANSFER_DELAY - Controls the minimum amount of time that the SSEL is deasserted between
 *    transfers. 0x0 = The minimum time that SSEL is deasserted is 1 SPI clock time. (Zero added time.) 0x1
 *    = The minimum time that SSEL is deasserted is 2 SPI clock times. 0x2 = The minimum time that
 *    SSEL is deasserted is 3 SPI clock times. ... 0xF = The minimum time that SSEL is deasserted is
 *    16 SPI clock times.
 */
#define SPI_DLY_TRANSFER_DELAY(x)                (((uint32_t)(((uint32_t)(x)) << SPI_DLY_TRANSFER_DELAY_SHIFT)) & SPI_DLY_TRANSFER_DELAY_MASK)
/*! @} */

/*! @name STAT - SPI Status. Some status flags can be cleared by writing a 1 to that bit position. */
/*! @{ */
#define SPI_STAT_RXOV_MASK                       (0x4U)
#define SPI_STAT_RXOV_SHIFT                      (2U)
/*! RXOV - Receiver Overrun interrupt flag. This flag applies only to slave mode (Master = 0). This
 *    flag is set when the beginning of a received character is detected while the receiver buffer
 *    is still in use. If this occurs, the receiver buffer contents are preserved, and the incoming
 *    data is lost. Data received by the SPI should be considered undefined if RxOv is set.
 */
#define SPI_STAT_RXOV(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_STAT_RXOV_SHIFT)) & SPI_STAT_RXOV_MASK)
#define SPI_STAT_TXUR_MASK                       (0x8U)
#define SPI_STAT_TXUR_SHIFT                      (3U)
/*! TXUR - Transmitter Underrun interrupt flag. This flag applies only to slave mode (Master = 0).
 *    In this case, the transmitter must begin sending new data on the next input clock if the
 *    transmitter is idle. If that data is not available in the transmitter holding register at that
 *    point, there is no data to transmit and the TXUR flag is set. Data transmitted by the SPI should be
 *    considered undefined if TXUR is set.
 */
#define SPI_STAT_TXUR(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_STAT_TXUR_SHIFT)) & SPI_STAT_TXUR_MASK)
#define SPI_STAT_SSA_MASK                        (0x10U)
#define SPI_STAT_SSA_SHIFT                       (4U)
/*! SSA - Slave Select Assert. This flag is set whenever any slave select transitions from
 *    deasserted to asserted, in both master and slave modes. This allows determining when the SPI
 *    transmit/receive functions become busy, and allows waking up the device from reduced power modes when a
 *    slave mode access begins. This flag is cleared by software.
 */
#define SPI_STAT_SSA(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_STAT_SSA_SHIFT)) & SPI_STAT_SSA_MASK)
#define SPI_STAT_SSD_MASK                        (0x20U)
#define SPI_STAT_SSD_SHIFT                       (5U)
/*! SSD - Slave Select Deassert. This flag is set whenever any asserted slave selects transition to
 *    deasserted, in both master and slave modes. This allows determining when the SPI
 *    transmit/receive functions become idle. This flag is cleared by software.
 */
#define SPI_STAT_SSD(x)                          (((uint32_t)(((uint32_t)(x)) << SPI_STAT_SSD_SHIFT)) & SPI_STAT_SSD_MASK)
#define SPI_STAT_STALLED_MASK                    (0x40U)
#define SPI_STAT_STALLED_SHIFT                   (6U)
/*! STALLED - Stalled status flag. This indicates whether the SPI is currently in a stall condition.
 */
#define SPI_STAT_STALLED(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_STAT_STALLED_SHIFT)) & SPI_STAT_STALLED_MASK)
#define SPI_STAT_ENDTRANSFER_MASK                (0x80U)
#define SPI_STAT_ENDTRANSFER_SHIFT               (7U)
/*! ENDTRANSFER - End Transfer control bit. Software can set this bit to force an end to the current
 *    transfer when the transmitter finishes any activity already in progress, as if the EOTR flag
 *    had been set prior to the last transmission. This capability is included to support cases
 *    where it is not known when transmit data is written that it will be the end of a transfer. The bit
 *    is cleared when the transmitter becomes idle as the transfer comes to an end. Forcing an end
 *    of transfer in this manner causes any specified FRAME_DELAY and TRANSFER_DELAY to be inserted.
 */
#define SPI_STAT_ENDTRANSFER(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_STAT_ENDTRANSFER_SHIFT)) & SPI_STAT_ENDTRANSFER_MASK)
#define SPI_STAT_MSTIDLE_MASK                    (0x100U)
#define SPI_STAT_MSTIDLE_SHIFT                   (8U)
/*! MSTIDLE - Master idle status flag. This bit is 1 whenever the SPI master function is fully idle.
 *    This means that the transmit holding register is empty and the transmitter is not in the
 *    process of sending data.
 */
#define SPI_STAT_MSTIDLE(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_STAT_MSTIDLE_SHIFT)) & SPI_STAT_MSTIDLE_MASK)
/*! @} */

/*! @name INTENSET - SPI Interrupt Enable read and Set. A complete value may be read from this register. Writing a 1 to any implemented bit position causes that bit to be set. */
/*! @{ */
#define SPI_INTENSET_RXOVEN_MASK                 (0x4U)
#define SPI_INTENSET_RXOVEN_SHIFT                (2U)
/*! RXOVEN - RX overrun interrupt enable. Determines whether an interrupt occurs when a receiver
 *    overrun occurs. This happens in slave mode when there is a need for the receiver to move newly
 *    received data to the RXDAT register when it is already in use. The interface prevents receiver
 *    overrun in Master mode by not allowing a new transmission to begin when a receiver overrun
 *    would otherwise occur.
 */
#define SPI_INTENSET_RXOVEN(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_INTENSET_RXOVEN_SHIFT)) & SPI_INTENSET_RXOVEN_MASK)
#define SPI_INTENSET_TXUREN_MASK                 (0x8U)
#define SPI_INTENSET_TXUREN_SHIFT                (3U)
/*! TXUREN - TX underrun interrupt enable. Determines whether an interrupt occurs when a transmitter
 *    underrun occurs. This happens in slave mode when there is a need to transmit data when none
 *    is available.
 */
#define SPI_INTENSET_TXUREN(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_INTENSET_TXUREN_SHIFT)) & SPI_INTENSET_TXUREN_MASK)
#define SPI_INTENSET_SSAEN_MASK                  (0x10U)
#define SPI_INTENSET_SSAEN_SHIFT                 (4U)
/*! SSAEN - Slave select assert interrupt enable. Determines whether an interrupt occurs when the Slave Select is asserted.
 */
#define SPI_INTENSET_SSAEN(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_INTENSET_SSAEN_SHIFT)) & SPI_INTENSET_SSAEN_MASK)
#define SPI_INTENSET_SSDEN_MASK                  (0x20U)
#define SPI_INTENSET_SSDEN_SHIFT                 (5U)
/*! SSDEN - Slave select deassert interrupt enable. Determines whether an interrupt occurs when the Slave Select is deasserted.
 */
#define SPI_INTENSET_SSDEN(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_INTENSET_SSDEN_SHIFT)) & SPI_INTENSET_SSDEN_MASK)
#define SPI_INTENSET_MSTIDLEEN_MASK              (0x100U)
#define SPI_INTENSET_MSTIDLEEN_SHIFT             (8U)
/*! MSTIDLEEN - Master idle interrupt enable.
 */
#define SPI_INTENSET_MSTIDLEEN(x)                (((uint32_t)(((uint32_t)(x)) << SPI_INTENSET_MSTIDLEEN_SHIFT)) & SPI_INTENSET_MSTIDLEEN_MASK)
/*! @} */

/*! @name INTENCLR - SPI Interrupt Enable Clear. Writing a 1 to any implemented bit position causes the corresponding bit in INTENSET to be cleared. */
/*! @{ */
#define SPI_INTENCLR_RXOVCLR_MASK                (0x4U)
#define SPI_INTENCLR_RXOVCLR_SHIFT               (2U)
/*! RXOVCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define SPI_INTENCLR_RXOVCLR(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_INTENCLR_RXOVCLR_SHIFT)) & SPI_INTENCLR_RXOVCLR_MASK)
#define SPI_INTENCLR_TXURCLR_MASK                (0x8U)
#define SPI_INTENCLR_TXURCLR_SHIFT               (3U)
/*! TXURCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define SPI_INTENCLR_TXURCLR(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_INTENCLR_TXURCLR_SHIFT)) & SPI_INTENCLR_TXURCLR_MASK)
#define SPI_INTENCLR_SSACLR_MASK                 (0x10U)
#define SPI_INTENCLR_SSACLR_SHIFT                (4U)
/*! SSACLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define SPI_INTENCLR_SSACLR(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_INTENCLR_SSACLR_SHIFT)) & SPI_INTENCLR_SSACLR_MASK)
#define SPI_INTENCLR_SSDCLR_MASK                 (0x20U)
#define SPI_INTENCLR_SSDCLR_SHIFT                (5U)
/*! SSDCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define SPI_INTENCLR_SSDCLR(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_INTENCLR_SSDCLR_SHIFT)) & SPI_INTENCLR_SSDCLR_MASK)
#define SPI_INTENCLR_MSTIDLECLR_MASK             (0x100U)
#define SPI_INTENCLR_MSTIDLECLR_SHIFT            (8U)
/*! MSTIDLECLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define SPI_INTENCLR_MSTIDLECLR(x)               (((uint32_t)(((uint32_t)(x)) << SPI_INTENCLR_MSTIDLECLR_SHIFT)) & SPI_INTENCLR_MSTIDLECLR_MASK)
/*! @} */

/*! @name TXCTL - SPI Transmit Control. If Transmit FIFO is enabled, in FIFOCFG, then values read in this register are affected values in FIFO. */
/*! @{ */
#define SPI_TXCTL_TXSSEL0_N_MASK                 (0x10000U)
#define SPI_TXCTL_TXSSEL0_N_SHIFT                (16U)
/*! TXSSEL0_N - Transmit Slave Select 0.
 */
#define SPI_TXCTL_TXSSEL0_N(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_TXCTL_TXSSEL0_N_SHIFT)) & SPI_TXCTL_TXSSEL0_N_MASK)
#define SPI_TXCTL_TXSSEL1_N_MASK                 (0x20000U)
#define SPI_TXCTL_TXSSEL1_N_SHIFT                (17U)
/*! TXSSEL1_N - Transmit Slave Select 1. Valid only for SPI-1
 */
#define SPI_TXCTL_TXSSEL1_N(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_TXCTL_TXSSEL1_N_SHIFT)) & SPI_TXCTL_TXSSEL1_N_MASK)
#define SPI_TXCTL_TXSSEL2_N_MASK                 (0x40000U)
#define SPI_TXCTL_TXSSEL2_N_SHIFT                (18U)
/*! TXSSEL2_N - Transmit Slave Select 2. Valid only for SPI-1
 */
#define SPI_TXCTL_TXSSEL2_N(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_TXCTL_TXSSEL2_N_SHIFT)) & SPI_TXCTL_TXSSEL2_N_MASK)
#define SPI_TXCTL_TXSSEL3_N_MASK                 (0x80000U)
#define SPI_TXCTL_TXSSEL3_N_SHIFT                (19U)
/*! TXSSEL3_N - [Reserved] Transmit Slave Select 3.
 */
#define SPI_TXCTL_TXSSEL3_N(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_TXCTL_TXSSEL3_N_SHIFT)) & SPI_TXCTL_TXSSEL3_N_MASK)
#define SPI_TXCTL_EOTR_MASK                      (0x100000U)
#define SPI_TXCTL_EOTR_SHIFT                     (20U)
/*! EOTR - End of Transfer.
 */
#define SPI_TXCTL_EOTR(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_TXCTL_EOTR_SHIFT)) & SPI_TXCTL_EOTR_MASK)
#define SPI_TXCTL_EOFR_MASK                      (0x200000U)
#define SPI_TXCTL_EOFR_SHIFT                     (21U)
/*! EOFR - End of Frame.
 */
#define SPI_TXCTL_EOFR(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_TXCTL_EOFR_SHIFT)) & SPI_TXCTL_EOFR_MASK)
#define SPI_TXCTL_RXIGNORE_MASK                  (0x400000U)
#define SPI_TXCTL_RXIGNORE_SHIFT                 (22U)
/*! RXIGNORE - Receive Ignore.
 */
#define SPI_TXCTL_RXIGNORE(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_TXCTL_RXIGNORE_SHIFT)) & SPI_TXCTL_RXIGNORE_MASK)
#define SPI_TXCTL_LEN_MASK                       (0xF000000U)
#define SPI_TXCTL_LEN_SHIFT                      (24U)
/*! LEN - Data transfer Length.
 */
#define SPI_TXCTL_LEN(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_TXCTL_LEN_SHIFT)) & SPI_TXCTL_LEN_MASK)
/*! @} */

/*! @name DIV - SPI clock Divider */
/*! @{ */
#define SPI_DIV_DIVVAL_MASK                      (0xFFFFU)
#define SPI_DIV_DIVVAL_SHIFT                     (0U)
/*! DIVVAL - Rate divider value. Specifies how the SPI Module clock is divided to produce the SPI
 *    clock rate in master mode. DIVVAL is -1 encoded such that the value 0 results in SPICLK/1, the
 *    value 1 results in SPICLK/2, up to the maximum possible divide value of 0xFFFF, which results
 *    in SPICLK/65536.
 */
#define SPI_DIV_DIVVAL(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_DIV_DIVVAL_SHIFT)) & SPI_DIV_DIVVAL_MASK)
/*! @} */

/*! @name INTSTAT - SPI Interrupt Status */
/*! @{ */
#define SPI_INTSTAT_RXOV_MASK                    (0x4U)
#define SPI_INTSTAT_RXOV_SHIFT                   (2U)
/*! RXOV - Receiver Overrun interrupt flag.
 */
#define SPI_INTSTAT_RXOV(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_INTSTAT_RXOV_SHIFT)) & SPI_INTSTAT_RXOV_MASK)
#define SPI_INTSTAT_TXUR_MASK                    (0x8U)
#define SPI_INTSTAT_TXUR_SHIFT                   (3U)
/*! TXUR - Transmitter Underrun interrupt flag.
 */
#define SPI_INTSTAT_TXUR(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_INTSTAT_TXUR_SHIFT)) & SPI_INTSTAT_TXUR_MASK)
#define SPI_INTSTAT_SSA_MASK                     (0x10U)
#define SPI_INTSTAT_SSA_SHIFT                    (4U)
/*! SSA - Slave Select Assert.
 */
#define SPI_INTSTAT_SSA(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_INTSTAT_SSA_SHIFT)) & SPI_INTSTAT_SSA_MASK)
#define SPI_INTSTAT_SSD_MASK                     (0x20U)
#define SPI_INTSTAT_SSD_SHIFT                    (5U)
/*! SSD - Slave Select Deassert.
 */
#define SPI_INTSTAT_SSD(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_INTSTAT_SSD_SHIFT)) & SPI_INTSTAT_SSD_MASK)
#define SPI_INTSTAT_MSTIDLE_MASK                 (0x100U)
#define SPI_INTSTAT_MSTIDLE_SHIFT                (8U)
/*! MSTIDLE - Master Idle status flag.
 */
#define SPI_INTSTAT_MSTIDLE(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_INTSTAT_MSTIDLE_SHIFT)) & SPI_INTSTAT_MSTIDLE_MASK)
/*! @} */

/*! @name FIFOCFG - FIFO configuration and enable register. */
/*! @{ */
#define SPI_FIFOCFG_ENABLETX_MASK                (0x1U)
#define SPI_FIFOCFG_ENABLETX_SHIFT               (0U)
/*! ENABLETX - Enable the transmit FIFO. This is automatically enabled when PSELID.PERSEL is set to
 *    2 to configure for SPI functionality
 */
#define SPI_FIFOCFG_ENABLETX(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFOCFG_ENABLETX_SHIFT)) & SPI_FIFOCFG_ENABLETX_MASK)
#define SPI_FIFOCFG_ENABLERX_MASK                (0x2U)
#define SPI_FIFOCFG_ENABLERX_SHIFT               (1U)
/*! ENABLERX - Enable the receive FIFO. This is automatically enabled when PSELID.PERSEL is set to 2 to configure for SPI functionality
 */
#define SPI_FIFOCFG_ENABLERX(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFOCFG_ENABLERX_SHIFT)) & SPI_FIFOCFG_ENABLERX_MASK)
#define SPI_FIFOCFG_SIZE_MASK                    (0x30U)
#define SPI_FIFOCFG_SIZE_SHIFT                   (4U)
/*! SIZE - FIFO size configuration. This is a read-only field. 0x0 = Reset value. 0x1 = FIFO is
 *    configured as 4 entries of 16bits. This value is read after PSELID.PERSEL=2 for SPI functionlaity.
 *    0x2, 0x3 = not applicable.
 */
#define SPI_FIFOCFG_SIZE(x)                      (((uint32_t)(((uint32_t)(x)) << SPI_FIFOCFG_SIZE_SHIFT)) & SPI_FIFOCFG_SIZE_MASK)
#define SPI_FIFOCFG_DMATX_MASK                   (0x1000U)
#define SPI_FIFOCFG_DMATX_SHIFT                  (12U)
/*! DMATX - DMA configuration for transmit.
 */
#define SPI_FIFOCFG_DMATX(x)                     (((uint32_t)(((uint32_t)(x)) << SPI_FIFOCFG_DMATX_SHIFT)) & SPI_FIFOCFG_DMATX_MASK)
#define SPI_FIFOCFG_DMARX_MASK                   (0x2000U)
#define SPI_FIFOCFG_DMARX_SHIFT                  (13U)
/*! DMARX - DMA configuration for receive.
 */
#define SPI_FIFOCFG_DMARX(x)                     (((uint32_t)(((uint32_t)(x)) << SPI_FIFOCFG_DMARX_SHIFT)) & SPI_FIFOCFG_DMARX_MASK)
#define SPI_FIFOCFG_EMPTYTX_MASK                 (0x10000U)
#define SPI_FIFOCFG_EMPTYTX_SHIFT                (16U)
/*! EMPTYTX - Empty command for the transmit FIFO. When a 1 is written to this bit, the TX FIFO is emptied.
 */
#define SPI_FIFOCFG_EMPTYTX(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_FIFOCFG_EMPTYTX_SHIFT)) & SPI_FIFOCFG_EMPTYTX_MASK)
#define SPI_FIFOCFG_EMPTYRX_MASK                 (0x20000U)
#define SPI_FIFOCFG_EMPTYRX_SHIFT                (17U)
/*! EMPTYRX - Empty command for the receive FIFO. When a 1 is written to this bit, the RX FIFO is emptied.
 */
#define SPI_FIFOCFG_EMPTYRX(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_FIFOCFG_EMPTYRX_SHIFT)) & SPI_FIFOCFG_EMPTYRX_MASK)
#define SPI_FIFOCFG_POPDBG_MASK                  (0x40000U)
#define SPI_FIFOCFG_POPDBG_SHIFT                 (18U)
/*! POPDBG - Pop FIFO for debug reads.
 */
#define SPI_FIFOCFG_POPDBG(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_FIFOCFG_POPDBG_SHIFT)) & SPI_FIFOCFG_POPDBG_MASK)
/*! @} */

/*! @name FIFOSTAT - FIFO status register. */
/*! @{ */
#define SPI_FIFOSTAT_TXERR_MASK                  (0x1U)
#define SPI_FIFOSTAT_TXERR_SHIFT                 (0U)
/*! TXERR - TX FIFO error. Will be set if a transmit FIFO error occurs. This could be an overflow
 *    caused by pushing data into a full FIFO, or by an underflow if the FIFO is empty when data is
 *    needed. Cleared by writing a 1 to this bit.
 */
#define SPI_FIFOSTAT_TXERR(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_TXERR_SHIFT)) & SPI_FIFOSTAT_TXERR_MASK)
#define SPI_FIFOSTAT_RXERR_MASK                  (0x2U)
#define SPI_FIFOSTAT_RXERR_SHIFT                 (1U)
/*! RXERR - RX FIFO error. Will be set if a receive FIFO overflow occurs, caused by software or DMA
 *    not emptying the FIFO fast enough. Cleared by writing a 1 to this bit.
 */
#define SPI_FIFOSTAT_RXERR(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_RXERR_SHIFT)) & SPI_FIFOSTAT_RXERR_MASK)
#define SPI_FIFOSTAT_PERINT_MASK                 (0x8U)
#define SPI_FIFOSTAT_PERINT_SHIFT                (3U)
/*! PERINT - Peripheral interrupt. When 1, this indicates that the peripheral function has asserted
 *    an interrupt. The details can be found by reading the peripheral s STAT register.
 */
#define SPI_FIFOSTAT_PERINT(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_PERINT_SHIFT)) & SPI_FIFOSTAT_PERINT_MASK)
#define SPI_FIFOSTAT_TXEMPTY_MASK                (0x10U)
#define SPI_FIFOSTAT_TXEMPTY_SHIFT               (4U)
/*! TXEMPTY - Transmit FIFO empty. When 1, the transmit FIFO is empty. The peripheral may still be processing the last piece of data.
 */
#define SPI_FIFOSTAT_TXEMPTY(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_TXEMPTY_SHIFT)) & SPI_FIFOSTAT_TXEMPTY_MASK)
#define SPI_FIFOSTAT_TXNOTFULL_MASK              (0x20U)
#define SPI_FIFOSTAT_TXNOTFULL_SHIFT             (5U)
/*! TXNOTFULL - Transmit FIFO not full. When 1, the transmit FIFO is not full, so more data can be
 *    written. When 0, the transmit FIFO is full and another write would cause it to overflow.
 */
#define SPI_FIFOSTAT_TXNOTFULL(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_TXNOTFULL_SHIFT)) & SPI_FIFOSTAT_TXNOTFULL_MASK)
#define SPI_FIFOSTAT_RXNOTEMPTY_MASK             (0x40U)
#define SPI_FIFOSTAT_RXNOTEMPTY_SHIFT            (6U)
/*! RXNOTEMPTY - Receive FIFO not empty. When 1, the receive FIFO is not empty, so data can be read. When 0, the receive FIFO is empty.
 */
#define SPI_FIFOSTAT_RXNOTEMPTY(x)               (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_RXNOTEMPTY_SHIFT)) & SPI_FIFOSTAT_RXNOTEMPTY_MASK)
#define SPI_FIFOSTAT_RXFULL_MASK                 (0x80U)
#define SPI_FIFOSTAT_RXFULL_SHIFT                (7U)
/*! RXFULL - Receive FIFO full. When 1, the receive FIFO is full. Data needs to be read out to
 *    prevent the peripheral from causing an overflow.
 */
#define SPI_FIFOSTAT_RXFULL(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_RXFULL_SHIFT)) & SPI_FIFOSTAT_RXFULL_MASK)
#define SPI_FIFOSTAT_TXLVL_MASK                  (0x1F00U)
#define SPI_FIFOSTAT_TXLVL_SHIFT                 (8U)
/*! TXLVL - Transmit FIFO current level. A 0 means the TX FIFO is currently empty, and the TXEMPTY
 *    and TXNOTFULL flags will be 1. Other values tell how much data is actually in the TX FIFO at
 *    the point where the read occurs. If the TX FIFO is full, the TXEMPTY and TXNOTFULL flags will be
 *    0.
 */
#define SPI_FIFOSTAT_TXLVL(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_TXLVL_SHIFT)) & SPI_FIFOSTAT_TXLVL_MASK)
#define SPI_FIFOSTAT_RXLVL_MASK                  (0x1F0000U)
#define SPI_FIFOSTAT_RXLVL_SHIFT                 (16U)
/*! RXLVL - Receive FIFO current level. A 0 means the RX FIFO is currently empty, and the RXFULL and
 *    RXNOTEMPTY flags will be 0. Other values tell how much data is actually in the RX FIFO at the
 *    point where the read occurs. If the RX FIFO is full, the RXFULL and RXNOTEMPTY flags will be
 *    1.
 */
#define SPI_FIFOSTAT_RXLVL(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_FIFOSTAT_RXLVL_SHIFT)) & SPI_FIFOSTAT_RXLVL_MASK)
/*! @} */

/*! @name FIFOTRIG - FIFO trigger settings for interrupt and DMA request. */
/*! @{ */
#define SPI_FIFOTRIG_TXLVLENA_MASK               (0x1U)
#define SPI_FIFOTRIG_TXLVLENA_SHIFT              (0U)
/*! TXLVLENA - Transmit FIFO level trigger enable. This trigger will become an interrupt if enabled
 *    in FIFOINTENSET, or a DMA trigger if DMATX in FIFOCFG is set.
 */
#define SPI_FIFOTRIG_TXLVLENA(x)                 (((uint32_t)(((uint32_t)(x)) << SPI_FIFOTRIG_TXLVLENA_SHIFT)) & SPI_FIFOTRIG_TXLVLENA_MASK)
#define SPI_FIFOTRIG_RXLVLENA_MASK               (0x2U)
#define SPI_FIFOTRIG_RXLVLENA_SHIFT              (1U)
/*! RXLVLENA - Receive FIFO level trigger enable. This trigger will become an interrupt if enabled
 *    in FIFOINTENSET, or a DMA trigger if DMARX in FIFOCFG is set.
 */
#define SPI_FIFOTRIG_RXLVLENA(x)                 (((uint32_t)(((uint32_t)(x)) << SPI_FIFOTRIG_RXLVLENA_SHIFT)) & SPI_FIFOTRIG_RXLVLENA_MASK)
#define SPI_FIFOTRIG_TXLVL_MASK                  (0xF00U)
#define SPI_FIFOTRIG_TXLVL_SHIFT                 (8U)
/*! TXLVL - Transmit FIFO level trigger point. This field is used only when TXLVLENA = 1. 0 =
 *    trigger when the TX FIFO becomes empty. 1 = trigger when the TX FIFO level decreases to one entry.
 *    ... 7 = 1 = trigger when the TX FIFO level decreases to 7 entries (is no longer full).
 */
#define SPI_FIFOTRIG_TXLVL(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_FIFOTRIG_TXLVL_SHIFT)) & SPI_FIFOTRIG_TXLVL_MASK)
#define SPI_FIFOTRIG_RXLVL_MASK                  (0xF0000U)
#define SPI_FIFOTRIG_RXLVL_SHIFT                 (16U)
/*! RXLVL - Receive FIFO level trigger point. The RX FIFO level is checked when a new piece of data
 *    is received. This field is used only when RXLVLENA = 1. 0 = trigger when the RX FIFO has
 *    received one entry (is no longer empty). 1 = trigger when the RX FIFO has received two entries. ...
 *    7 = trigger when the RX FIFO has received 8 entries (has become full).
 */
#define SPI_FIFOTRIG_RXLVL(x)                    (((uint32_t)(((uint32_t)(x)) << SPI_FIFOTRIG_RXLVL_SHIFT)) & SPI_FIFOTRIG_RXLVL_MASK)
/*! @} */

/*! @name FIFOINTENSET - FIFO interrupt enable set (enable) and read register. */
/*! @{ */
#define SPI_FIFOINTENSET_TXERR_MASK              (0x1U)
#define SPI_FIFOINTENSET_TXERR_SHIFT             (0U)
/*! TXERR - Determines whether an interrupt occurs when a transmit error occurs, based on the TXERR flag in the FIFOSTAT register.
 */
#define SPI_FIFOINTENSET_TXERR(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTENSET_TXERR_SHIFT)) & SPI_FIFOINTENSET_TXERR_MASK)
#define SPI_FIFOINTENSET_RXERR_MASK              (0x2U)
#define SPI_FIFOINTENSET_RXERR_SHIFT             (1U)
/*! RXERR - Determines whether an interrupt occurs when a receive error occurs, based on the RXERR flag in the FIFOSTAT register.
 */
#define SPI_FIFOINTENSET_RXERR(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTENSET_RXERR_SHIFT)) & SPI_FIFOINTENSET_RXERR_MASK)
#define SPI_FIFOINTENSET_TXLVL_MASK              (0x4U)
#define SPI_FIFOINTENSET_TXLVL_SHIFT             (2U)
/*! TXLVL - Determines whether an interrupt occurs when a the transmit FIFO reaches the level
 *    specified by the TXLVL field in the FIFOTRIG register.
 */
#define SPI_FIFOINTENSET_TXLVL(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTENSET_TXLVL_SHIFT)) & SPI_FIFOINTENSET_TXLVL_MASK)
#define SPI_FIFOINTENSET_RXLVL_MASK              (0x8U)
#define SPI_FIFOINTENSET_RXLVL_SHIFT             (3U)
/*! RXLVL - Determines whether an interrupt occurs when a the receive FIFO reaches the level
 *    specified by the TXLVL field in the FIFOTRIG register.
 */
#define SPI_FIFOINTENSET_RXLVL(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTENSET_RXLVL_SHIFT)) & SPI_FIFOINTENSET_RXLVL_MASK)
/*! @} */

/*! @name FIFOINTENCLR - FIFO interrupt enable clear (disable) and read register. */
/*! @{ */
#define SPI_FIFOINTENCLR_TXERR_MASK              (0x1U)
#define SPI_FIFOINTENCLR_TXERR_SHIFT             (0U)
/*! TXERR - Writing 1 clears the corresponding bit in the FIFOINTENSET register
 */
#define SPI_FIFOINTENCLR_TXERR(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTENCLR_TXERR_SHIFT)) & SPI_FIFOINTENCLR_TXERR_MASK)
#define SPI_FIFOINTENCLR_RXERR_MASK              (0x2U)
#define SPI_FIFOINTENCLR_RXERR_SHIFT             (1U)
/*! RXERR - Writing 1 clears the corresponding bit in the FIFOINTENSET register
 */
#define SPI_FIFOINTENCLR_RXERR(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTENCLR_RXERR_SHIFT)) & SPI_FIFOINTENCLR_RXERR_MASK)
#define SPI_FIFOINTENCLR_TXLVL_MASK              (0x4U)
#define SPI_FIFOINTENCLR_TXLVL_SHIFT             (2U)
/*! TXLVL - Writing 1 clears the corresponding bit in the FIFOINTENSET register
 */
#define SPI_FIFOINTENCLR_TXLVL(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTENCLR_TXLVL_SHIFT)) & SPI_FIFOINTENCLR_TXLVL_MASK)
#define SPI_FIFOINTENCLR_RXLVL_MASK              (0x8U)
#define SPI_FIFOINTENCLR_RXLVL_SHIFT             (3U)
/*! RXLVL - Writing 1 clears the corresponding bit in the FIFOINTENSET register
 */
#define SPI_FIFOINTENCLR_RXLVL(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTENCLR_RXLVL_SHIFT)) & SPI_FIFOINTENCLR_RXLVL_MASK)
/*! @} */

/*! @name FIFOINTSTAT - FIFO interrupt status register. */
/*! @{ */
#define SPI_FIFOINTSTAT_TXERR_MASK               (0x1U)
#define SPI_FIFOINTSTAT_TXERR_SHIFT              (0U)
/*! TXERR - TX FIFO error.
 */
#define SPI_FIFOINTSTAT_TXERR(x)                 (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTSTAT_TXERR_SHIFT)) & SPI_FIFOINTSTAT_TXERR_MASK)
#define SPI_FIFOINTSTAT_RXERR_MASK               (0x2U)
#define SPI_FIFOINTSTAT_RXERR_SHIFT              (1U)
/*! RXERR - RX FIFO error.
 */
#define SPI_FIFOINTSTAT_RXERR(x)                 (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTSTAT_RXERR_SHIFT)) & SPI_FIFOINTSTAT_RXERR_MASK)
#define SPI_FIFOINTSTAT_TXLVL_MASK               (0x4U)
#define SPI_FIFOINTSTAT_TXLVL_SHIFT              (2U)
/*! TXLVL - Transmit FIFO level interrupt.
 */
#define SPI_FIFOINTSTAT_TXLVL(x)                 (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTSTAT_TXLVL_SHIFT)) & SPI_FIFOINTSTAT_TXLVL_MASK)
#define SPI_FIFOINTSTAT_RXLVL_MASK               (0x8U)
#define SPI_FIFOINTSTAT_RXLVL_SHIFT              (3U)
/*! RXLVL - Receive FIFO level interrupt.
 */
#define SPI_FIFOINTSTAT_RXLVL(x)                 (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTSTAT_RXLVL_SHIFT)) & SPI_FIFOINTSTAT_RXLVL_MASK)
#define SPI_FIFOINTSTAT_PERINT_MASK              (0x10U)
#define SPI_FIFOINTSTAT_PERINT_SHIFT             (4U)
/*! PERINT - Peripheral interrupt.
 */
#define SPI_FIFOINTSTAT_PERINT(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFOINTSTAT_PERINT_SHIFT)) & SPI_FIFOINTSTAT_PERINT_MASK)
/*! @} */

/*! @name FIFOWR - FIFO write data. FIFO data not reset by block reset */
/*! @{ */
#define SPI_FIFOWR_TXDATA_MASK                   (0xFFFFU)
#define SPI_FIFOWR_TXDATA_SHIFT                  (0U)
/*! TXDATA - Transmit data to the FIFO.
 */
#define SPI_FIFOWR_TXDATA(x)                     (((uint32_t)(((uint32_t)(x)) << SPI_FIFOWR_TXDATA_SHIFT)) & SPI_FIFOWR_TXDATA_MASK)
#define SPI_FIFOWR_TXSSEL0_N_MASK                (0x10000U)
#define SPI_FIFOWR_TXSSEL0_N_SHIFT               (16U)
/*! TXSSEL0_N - Transmit Slave Select. This field asserts SSEL0 in master mode. The output on the
 *    pin is active LOW by default. Remark: The active state of the SSEL0 pin is configured by bits in
 *    the CFG register.
 */
#define SPI_FIFOWR_TXSSEL0_N(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFOWR_TXSSEL0_N_SHIFT)) & SPI_FIFOWR_TXSSEL0_N_MASK)
#define SPI_FIFOWR_TXSSEL1_N_MASK                (0x20000U)
#define SPI_FIFOWR_TXSSEL1_N_SHIFT               (17U)
/*! TXSSEL1_N - Transmit Slave Select. This field asserts SSEL1 in master mode. The output on the
 *    pin is active LOW by default. Remark: The active state of the SSEL1 pin is configured by bits in
 *    the CFG register.
 */
#define SPI_FIFOWR_TXSSEL1_N(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFOWR_TXSSEL1_N_SHIFT)) & SPI_FIFOWR_TXSSEL1_N_MASK)
#define SPI_FIFOWR_TXSSEL2_N_MASK                (0x40000U)
#define SPI_FIFOWR_TXSSEL2_N_SHIFT               (18U)
/*! TXSSEL2_N - Transmit Slave Select. This field asserts SSEL2 in master mode. The output on the
 *    pin is active LOW by default. Remark: The active state of the SSEL2 pin is configured by bits in
 *    the CFG register.
 */
#define SPI_FIFOWR_TXSSEL2_N(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFOWR_TXSSEL2_N_SHIFT)) & SPI_FIFOWR_TXSSEL2_N_MASK)
#define SPI_FIFOWR_EOT_MASK                      (0x100000U)
#define SPI_FIFOWR_EOT_SHIFT                     (20U)
/*! EOT - End of Transfer. The asserted SSEL will be deasserted at the end of a transfer, and remain
 *    so for at least the time specified by the Transfer_delay value in the DLY register.
 */
#define SPI_FIFOWR_EOT(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_FIFOWR_EOT_SHIFT)) & SPI_FIFOWR_EOT_MASK)
#define SPI_FIFOWR_EOF_MASK                      (0x200000U)
#define SPI_FIFOWR_EOF_SHIFT                     (21U)
/*! EOF - End of Frame. Between frames, a delay may be inserted, as defined by the FRAME_DELAY value
 *    in the DLY register. The end of a frame may not be particularly meaningful if the FRAME_DELAY
 *    value = 0. This control can be used as part of the support for frame lengths greater than 16
 *    bits.
 */
#define SPI_FIFOWR_EOF(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_FIFOWR_EOF_SHIFT)) & SPI_FIFOWR_EOF_MASK)
#define SPI_FIFOWR_RXIGNORE_MASK                 (0x400000U)
#define SPI_FIFOWR_RXIGNORE_SHIFT                (22U)
/*! RXIGNORE - Receive Ignore. This allows data to be transmitted using the SPI without the need to
 *    read unneeded data from the receiver.Setting this bit simplifies the transmit process and can
 *    be used with the DMA.
 */
#define SPI_FIFOWR_RXIGNORE(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_FIFOWR_RXIGNORE_SHIFT)) & SPI_FIFOWR_RXIGNORE_MASK)
#define SPI_FIFOWR_LEN_MASK                      (0xF000000U)
#define SPI_FIFOWR_LEN_SHIFT                     (24U)
/*! LEN - Data Length. Specifies the data length from 1 to 16 bits. Note that transfer lengths
 *    greater than 16 bits are supported by implementing multiple sequential transmits. 0x0 = Data
 *    transfer is 1 bit in length. Note: when LEN = 0, the underrun status is not meaningful. 0x1 = Data
 *    transfer is 2 bits in length. 0x2 = Data transfer is 3 bits in length. ... 0xF = Data transfer
 *    is 16 bits in length.
 */
#define SPI_FIFOWR_LEN(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_FIFOWR_LEN_SHIFT)) & SPI_FIFOWR_LEN_MASK)
/*! @} */

/*! @name FIFORD - FIFO read data. */
/*! @{ */
#define SPI_FIFORD_RXDATA_MASK                   (0xFFFFU)
#define SPI_FIFORD_RXDATA_SHIFT                  (0U)
/*! RXDATA - Received data from the FIFO.
 */
#define SPI_FIFORD_RXDATA(x)                     (((uint32_t)(((uint32_t)(x)) << SPI_FIFORD_RXDATA_SHIFT)) & SPI_FIFORD_RXDATA_MASK)
#define SPI_FIFORD_RXSSEL0_N_MASK                (0x10000U)
#define SPI_FIFORD_RXSSEL0_N_SHIFT               (16U)
/*! RXSSEL0_N - Slave Select for receive. This field allows the state of the SSEL0 pin to be saved
 *    along with received data. The value will reflect the SSEL0 pin for both master and slave
 *    operation. A zero indicates that a slave select is active. The actual polarity of each slave select
 *    pin is configured by the related SPOL bit in CFG.
 */
#define SPI_FIFORD_RXSSEL0_N(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFORD_RXSSEL0_N_SHIFT)) & SPI_FIFORD_RXSSEL0_N_MASK)
#define SPI_FIFORD_RXSSEL1_N_MASK                (0x20000U)
#define SPI_FIFORD_RXSSEL1_N_SHIFT               (17U)
/*! RXSSEL1_N - Slave Select for receive. This field allows the state of the SSEL1 pin to be saved
 *    along with received data. The value will reflect the SSEL1 pin for both master and slave
 *    operation. A zero indicates that a slave select is active. The actual polarity of each slave select
 *    pin is configured by the related SPOL bit in CFG.
 */
#define SPI_FIFORD_RXSSEL1_N(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFORD_RXSSEL1_N_SHIFT)) & SPI_FIFORD_RXSSEL1_N_MASK)
#define SPI_FIFORD_RXSSEL2_N_MASK                (0x40000U)
#define SPI_FIFORD_RXSSEL2_N_SHIFT               (18U)
/*! RXSSEL2_N - Slave Select for receive. This field allows the state of the SSEL2 pin to be saved
 *    along with received data. The value will reflect the SSEL2 pin for both master and slave
 *    operation. A zero indicates that a slave select is active. The actual polarity of each slave select
 *    pin is configured by the related SPOL bit in CFG.
 */
#define SPI_FIFORD_RXSSEL2_N(x)                  (((uint32_t)(((uint32_t)(x)) << SPI_FIFORD_RXSSEL2_N_SHIFT)) & SPI_FIFORD_RXSSEL2_N_MASK)
#define SPI_FIFORD_SOT_MASK                      (0x100000U)
#define SPI_FIFORD_SOT_SHIFT                     (20U)
/*! SOT - Start of Transfer flag. This flag will be 1 if this is the first data after the SSELs went
 *    from deasserted to asserted (i.e., any previous transfer has ended). This information can be
 *    used to identify the first piece of data in cases where the transfer length is greater than 16
 *    bit.
 */
#define SPI_FIFORD_SOT(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_FIFORD_SOT_SHIFT)) & SPI_FIFORD_SOT_MASK)
/*! @} */

/*! @name FIFORDNOPOP - FIFO data read with no FIFO pop. */
/*! @{ */
#define SPI_FIFORDNOPOP_RXDATA_MASK              (0xFFFFU)
#define SPI_FIFORDNOPOP_RXDATA_SHIFT             (0U)
/*! RXDATA - Received data from the FIFO.
 */
#define SPI_FIFORDNOPOP_RXDATA(x)                (((uint32_t)(((uint32_t)(x)) << SPI_FIFORDNOPOP_RXDATA_SHIFT)) & SPI_FIFORDNOPOP_RXDATA_MASK)
#define SPI_FIFORDNOPOP_RXSSEL0_N_MASK           (0x10000U)
#define SPI_FIFORDNOPOP_RXSSEL0_N_SHIFT          (16U)
/*! RXSSEL0_N - Slave Select for receive. This field allows the state of the SSEL0 pin to be saved
 *    along with received data. The value will reflect the SSEL0 pin for both master and slave
 *    operation. A zero indicates that a slave select is active. The actual polarity of each slave select
 *    pin is configured by the related SPOL bit in CFG.
 */
#define SPI_FIFORDNOPOP_RXSSEL0_N(x)             (((uint32_t)(((uint32_t)(x)) << SPI_FIFORDNOPOP_RXSSEL0_N_SHIFT)) & SPI_FIFORDNOPOP_RXSSEL0_N_MASK)
#define SPI_FIFORDNOPOP_RXSSEL1_N_MASK           (0x20000U)
#define SPI_FIFORDNOPOP_RXSSEL1_N_SHIFT          (17U)
/*! RXSSEL1_N - Slave Select for receive. This field allows the state of the SSEL1 pin to be saved
 *    along with received data. The value will reflect the SSEL1 pin for both master and slave
 *    operation. A zero indicates that a slave select is active. The actual polarity of each slave select
 *    pin is configured by the related SPOL bit in CFG.
 */
#define SPI_FIFORDNOPOP_RXSSEL1_N(x)             (((uint32_t)(((uint32_t)(x)) << SPI_FIFORDNOPOP_RXSSEL1_N_SHIFT)) & SPI_FIFORDNOPOP_RXSSEL1_N_MASK)
#define SPI_FIFORDNOPOP_RXSSEL2_N_MASK           (0x40000U)
#define SPI_FIFORDNOPOP_RXSSEL2_N_SHIFT          (18U)
/*! RXSSEL2_N - Slave Select for receive. This field allows the state of the SSEL2 pin to be saved
 *    along with received data. The value will reflect the SSEL2 pin for both master and slave
 *    operation. A zero indicates that a slave select is active. The actual polarity of each slave select
 *    pin is configured by the related SPOL bit in CFG.
 */
#define SPI_FIFORDNOPOP_RXSSEL2_N(x)             (((uint32_t)(((uint32_t)(x)) << SPI_FIFORDNOPOP_RXSSEL2_N_SHIFT)) & SPI_FIFORDNOPOP_RXSSEL2_N_MASK)
#define SPI_FIFORDNOPOP_SOT_MASK                 (0x100000U)
#define SPI_FIFORDNOPOP_SOT_SHIFT                (20U)
/*! SOT - Start of Transfer flag. This flag will be 1 if this is the first data after the SSELs went
 *    from deasserted to asserted (i.e., any previous transfer has ended). This information can be
 *    used to identify the first piece of data in cases where the transfer length is greater than 16
 *    bit.
 */
#define SPI_FIFORDNOPOP_SOT(x)                   (((uint32_t)(((uint32_t)(x)) << SPI_FIFORDNOPOP_SOT_SHIFT)) & SPI_FIFORDNOPOP_SOT_MASK)
/*! @} */

/*! @name PSELID - Peripheral function select and ID register */
/*! @{ */
#define SPI_PSELID_PERSEL_MASK                   (0x7U)
#define SPI_PSELID_PERSEL_SHIFT                  (0U)
/*! PERSEL - Peripheral Select. This field is writable by software. Reset value is 0x0 showing that
 *    no peripheral is selected. Write 0x2 to select the SPI function. All other values are not
 *    valid.
 */
#define SPI_PSELID_PERSEL(x)                     (((uint32_t)(((uint32_t)(x)) << SPI_PSELID_PERSEL_SHIFT)) & SPI_PSELID_PERSEL_MASK)
#define SPI_PSELID_LOCK_MASK                     (0x8U)
#define SPI_PSELID_LOCK_SHIFT                    (3U)
/*! LOCK - Lock the peripheral select. This field is writable by software. 0 Peripheral select can
 *    be changed by software. 1 Peripheral select is locked and cannot be changed until this
 *    peripheral or the entire device is reset.
 */
#define SPI_PSELID_LOCK(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_PSELID_LOCK_SHIFT)) & SPI_PSELID_LOCK_MASK)
#define SPI_PSELID_SPIPRESENT_MASK               (0x20U)
#define SPI_PSELID_SPIPRESENT_SHIFT              (5U)
/*! SPIPRESENT - SPI present indicator. This field is Read-only and has value 0x1 to indicate SPI function is present.
 */
#define SPI_PSELID_SPIPRESENT(x)                 (((uint32_t)(((uint32_t)(x)) << SPI_PSELID_SPIPRESENT_SHIFT)) & SPI_PSELID_SPIPRESENT_MASK)
#define SPI_PSELID_ID_MASK                       (0xFFFFF000U)
#define SPI_PSELID_ID_SHIFT                      (12U)
/*! ID - Peripheral Select ID.
 */
#define SPI_PSELID_ID(x)                         (((uint32_t)(((uint32_t)(x)) << SPI_PSELID_ID_SHIFT)) & SPI_PSELID_ID_MASK)
/*! @} */

/*! @name ID - SPI Module Identifier */
/*! @{ */
#define SPI_ID_APERTURE_MASK                     (0xFFU)
#define SPI_ID_APERTURE_SHIFT                    (0U)
/*! APERTURE - Aperture i.e. number minus 1 of consecutive packets 4 Kbytes reserved for this IP
 */
#define SPI_ID_APERTURE(x)                       (((uint32_t)(((uint32_t)(x)) << SPI_ID_APERTURE_SHIFT)) & SPI_ID_APERTURE_MASK)
#define SPI_ID_MIN_REV_MASK                      (0xF00U)
#define SPI_ID_MIN_REV_SHIFT                     (8U)
/*! MIN_REV - Minor revision i.e. with no software consequences
 */
#define SPI_ID_MIN_REV(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_ID_MIN_REV_SHIFT)) & SPI_ID_MIN_REV_MASK)
#define SPI_ID_MAJ_REV_MASK                      (0xF000U)
#define SPI_ID_MAJ_REV_SHIFT                     (12U)
/*! MAJ_REV - Major revision i.e. implies software modifications
 */
#define SPI_ID_MAJ_REV(x)                        (((uint32_t)(((uint32_t)(x)) << SPI_ID_MAJ_REV_SHIFT)) & SPI_ID_MAJ_REV_MASK)
#define SPI_ID_ID_MASK                           (0xFFFF0000U)
#define SPI_ID_ID_SHIFT                          (16U)
/*! ID - Identifier. This is the unique identifier of the module
 */
#define SPI_ID_ID(x)                             (((uint32_t)(((uint32_t)(x)) << SPI_ID_ID_SHIFT)) & SPI_ID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group SPI_Register_Masks */


/* SPI - Peripheral instance base addresses */
/** Peripheral SPI0 base address */
#define SPI0_BASE                                (0x4008D000u)
/** Peripheral SPI0 base pointer */
#define SPI0                                     ((SPI_Type *)SPI0_BASE)
/** Peripheral SPI1 base address */
#define SPI1_BASE                                (0x4008E000u)
/** Peripheral SPI1 base pointer */
#define SPI1                                     ((SPI_Type *)SPI1_BASE)
/** Array initializer of SPI peripheral base addresses */
#define SPI_BASE_ADDRS                           { SPI0_BASE, SPI1_BASE }
/** Array initializer of SPI peripheral base pointers */
#define SPI_BASE_PTRS                            { SPI0, SPI1 }
/** Interrupt vectors for the SPI peripheral type */
#define SPI_IRQS                                 { FLEXCOMM4_IRQn, FLEXCOMM5_IRQn }

/*!
 * @}
 */ /* end of group SPI_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- SPIFI Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SPIFI_Peripheral_Access_Layer SPIFI Peripheral Access Layer
 * @{
 */

/** SPIFI - Register Layout Typedef */
typedef struct {
  __IO uint32_t CTRL;                              /**< SPIFI control register, offset: 0x0 */
  __IO uint32_t CMD;                               /**< SPIFI command register, offset: 0x4 */
  __IO uint32_t ADDR;                              /**< SPIFI address register, offset: 0x8 */
  __IO uint32_t IDATA;                             /**< SPIFI intermediate data register, offset: 0xC */
  __IO uint32_t CLIMIT;                            /**< SPIFI limit register, offset: 0x10 */
  __IO uint32_t DATA;                              /**< SPIFI data register. Input or output data, offset: 0x14 */
  __IO uint32_t MCMD;                              /**< SPIFI memory command register, offset: 0x18 */
  __IO uint32_t STAT;                              /**< SPIFI status register, offset: 0x1C */
} SPIFI_Type;

/* ----------------------------------------------------------------------------
   -- SPIFI Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SPIFI_Register_Masks SPIFI Register Masks
 * @{
 */

/*! @name CTRL - SPIFI control register */
/*! @{ */
#define SPIFI_CTRL_TIMEOUT_MASK                  (0xFFFFU)
#define SPIFI_CTRL_TIMEOUT_SHIFT                 (0U)
/*! TIMEOUT - This field contains the number of serial clock periods without the processor reading
 *    data in memory mode, which will cause the SPIFI hardware to terminate the command by driving
 *    the CS pin high and negating the CMD bit in the Status register. (This allows the flash memory
 *    to enter a lower-power state.) If the processor reads data from the flash region after a
 *    time-out, the command in the Memory Command Register is issued again.
 */
#define SPIFI_CTRL_TIMEOUT(x)                    (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_TIMEOUT_SHIFT)) & SPIFI_CTRL_TIMEOUT_MASK)
#define SPIFI_CTRL_CSHIGH_MASK                   (0xF0000U)
#define SPIFI_CTRL_CSHIGH_SHIFT                  (16U)
/*! CSHIGH - This field controls the minimum CS high time, expressed as a number of serial clock periods minus one.
 */
#define SPIFI_CTRL_CSHIGH(x)                     (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_CSHIGH_SHIFT)) & SPIFI_CTRL_CSHIGH_MASK)
#define SPIFI_CTRL_D_PRFTCH_DIS_MASK             (0x200000U)
#define SPIFI_CTRL_D_PRFTCH_DIS_SHIFT            (21U)
/*! D_PRFTCH_DIS - This bit allows conditioning of memory mode prefetches based on the AHB HPROT
 *    (instruction/data) access information. A 1 in this register means that the SPIFI will not attempt
 *    a speculative prefetch when it encounters data accesses.
 */
#define SPIFI_CTRL_D_PRFTCH_DIS(x)               (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_D_PRFTCH_DIS_SHIFT)) & SPIFI_CTRL_D_PRFTCH_DIS_MASK)
#define SPIFI_CTRL_INTEN_MASK                    (0x400000U)
#define SPIFI_CTRL_INTEN_SHIFT                   (22U)
/*! INTEN - If this bit is 1 when a command ends, the SPIFI will assert its interrupt request
 *    output. See INTRQ in the status register for further details.
 */
#define SPIFI_CTRL_INTEN(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_INTEN_SHIFT)) & SPIFI_CTRL_INTEN_MASK)
#define SPIFI_CTRL_MODE3_MASK                    (0x800000U)
#define SPIFI_CTRL_MODE3_SHIFT                   (23U)
/*! MODE3 - SPI Mode 3 select. 0: SCK LOW. The SPIFI drives SCK low after the rising edge at which
 *    the last bit of each command is captured, and keeps it low while CS is HIGH. 1: SCK HIGH. the
 *    SPIFI keeps SCK high after the rising edge for the last bit of each command and while CS is
 *    HIGH, and drives it low after it drives CS LOW. (Known serial flash devices can handle either
 *    mode, but some devices may require a particular mode for proper operation.) Remark: MODE3, RFCLK,
 *    and FBCLK should not all be 1, because in this case there is no final falling edge on SCK on
 *    which to sample the last data bit of the frame.
 */
#define SPIFI_CTRL_MODE3(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_MODE3_SHIFT)) & SPIFI_CTRL_MODE3_MASK)
#define SPIFI_CTRL_PRFTCH_DIS_MASK               (0x8000000U)
#define SPIFI_CTRL_PRFTCH_DIS_SHIFT              (27U)
/*! PRFTCH_DIS - Cache prefetching enable. The SPIFI includes an internal cache. A 1 in this bit
 *    disables prefetching of cache lines. 0: Enable. Cache prefetching enabled. 1: Disable. Disables
 *    prefetching of cache lines.
 */
#define SPIFI_CTRL_PRFTCH_DIS(x)                 (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_PRFTCH_DIS_SHIFT)) & SPIFI_CTRL_PRFTCH_DIS_MASK)
#define SPIFI_CTRL_DUAL_MASK                     (0x10000000U)
#define SPIFI_CTRL_DUAL_SHIFT                    (28U)
/*! DUAL - Select dual protocol. 0: Quad protocol. This protocol uses IO3:0. 1: Dual protocol. This protocol uses IO1:0.
 */
#define SPIFI_CTRL_DUAL(x)                       (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_DUAL_SHIFT)) & SPIFI_CTRL_DUAL_MASK)
#define SPIFI_CTRL_RFCLK_MASK                    (0x20000000U)
#define SPIFI_CTRL_RFCLK_SHIFT                   (29U)
/*! RFCLK - Select active clock edge for input data. 0: Rising edge. Read data is sampled on rising
 *    edges on the clock, as in classic SPI operation. 1: Falling edge. Read data is sampled on
 *    falling edges of the clock, allowing a full serial clock of of time in order to maximize the
 *    serial clock frequency. Remark: MODE3, RFCLK, and FBCLK should not all be 1, because in this case
 *    there is no final falling edge on SCK on which to sample the last data bit of the frame.
 */
#define SPIFI_CTRL_RFCLK(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_RFCLK_SHIFT)) & SPIFI_CTRL_RFCLK_MASK)
#define SPIFI_CTRL_FBCLK_MASK                    (0x40000000U)
#define SPIFI_CTRL_FBCLK_SHIFT                   (30U)
/*! FBCLK - Feedback clock select. 0: Internal clock. The SPIFI samples read data using an internal
 *    clock. 1: Feedback clock. Read data is sampled using a feedback clock from the SCK pin. This
 *    allows slightly more time for each received bit. Remark: MODE3, RFCLK, and FBCLK should not all
 *    be 1, because in this case there is no final falling edge on SCK on which to sample the last
 *    data bit of the frame.
 */
#define SPIFI_CTRL_FBCLK(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_FBCLK_SHIFT)) & SPIFI_CTRL_FBCLK_MASK)
#define SPIFI_CTRL_DMAEN_MASK                    (0x80000000U)
#define SPIFI_CTRL_DMAEN_SHIFT                   (31U)
/*! DMAEN - A 1 in this bit enables the DMA Request output from the SPIFI. Set this bit only when a
 *    DMA channel is used to transfer data in peripheral mode. Do not set this bit when a DMA
 *    channel is used for memory-to-memory transfers from the SPIFI memory area. DRQEN should only be used
 *    in Command mode.
 */
#define SPIFI_CTRL_DMAEN(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_CTRL_DMAEN_SHIFT)) & SPIFI_CTRL_DMAEN_MASK)
/*! @} */

/*! @name CMD - SPIFI command register */
/*! @{ */
#define SPIFI_CMD_DATALEN_MASK                   (0x3FFFU)
#define SPIFI_CMD_DATALEN_SHIFT                  (0U)
/*! DATALEN - Except when the POLL bit in this register is 1, this field controls how many data
 *    bytes are in the command. 0 indicates that the command does not contain a data field.
 */
#define SPIFI_CMD_DATALEN(x)                     (((uint32_t)(((uint32_t)(x)) << SPIFI_CMD_DATALEN_SHIFT)) & SPIFI_CMD_DATALEN_MASK)
#define SPIFI_CMD_POLL_MASK                      (0x4000U)
#define SPIFI_CMD_POLL_SHIFT                     (14U)
/*! POLL - This bit should be written as 1 only with an opcode that a) contains an input data field,
 *    and b) causes the serial flash device to return byte status repetitively (e.g., a Read Status
 *    command). When this bit is 1, the SPIFI hardware continues to read bytes until the test
 *    specified by the DATALEN field is met. The hardware tests the bit in each status byte selected by
 *    DATALEN bits 2:0, until a bit is found that is equal to DATALEN bit 3. When the test succeeds,
 *    the SPIFI captures the byte that meets this test so that it can be read from the Data
 *    Register, and terminates the command by raising CS. The end-of-command interrupt can be enabled to
 *    inform software when this occurs
 */
#define SPIFI_CMD_POLL(x)                        (((uint32_t)(((uint32_t)(x)) << SPIFI_CMD_POLL_SHIFT)) & SPIFI_CMD_POLL_MASK)
#define SPIFI_CMD_DOUT_MASK                      (0x8000U)
#define SPIFI_CMD_DOUT_SHIFT                     (15U)
/*! DOUT - If the DATALEN field is not zero, this bit controls the direction of the data: 0: Input
 *    from serial flash. 1: Output to serial flash.
 */
#define SPIFI_CMD_DOUT(x)                        (((uint32_t)(((uint32_t)(x)) << SPIFI_CMD_DOUT_SHIFT)) & SPIFI_CMD_DOUT_MASK)
#define SPIFI_CMD_INTLEN_MASK                    (0x70000U)
#define SPIFI_CMD_INTLEN_SHIFT                   (16U)
/*! INTLEN - This field controls how many intermediate bytes precede the data. (Each such byte may
 *    require 8 or 2 SCK cycles, depending on whether the intermediate field is in serial, 2-bit, or
 *    4-bit format.) Intermediate bytes are output by the SPIFI, and include post-address control
 *    information, dummy and delay bytes. See the description of the Intermediate Data register for
 *    the contents of such bytes.
 */
#define SPIFI_CMD_INTLEN(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_CMD_INTLEN_SHIFT)) & SPIFI_CMD_INTLEN_MASK)
#define SPIFI_CMD_FIELDFORM_MASK                 (0x180000U)
#define SPIFI_CMD_FIELDFORM_SHIFT                (19U)
/*! FIELDFORM - This field controls how the fields of the command are sent. 0x0: All serial. All
 *    fields of the command are serial. 0x1: Quad/dual data. Data field is quad/dual, other fields are
 *    serial. 0x2: Serial opcode. Opcode field is serial. Other fields are quad/dual. 0x3: All
 *    quad/dual. All fields of the command are in quad/dual format.
 */
#define SPIFI_CMD_FIELDFORM(x)                   (((uint32_t)(((uint32_t)(x)) << SPIFI_CMD_FIELDFORM_SHIFT)) & SPIFI_CMD_FIELDFORM_MASK)
#define SPIFI_CMD_FRAMEFORM_MASK                 (0xE00000U)
#define SPIFI_CMD_FRAMEFORM_SHIFT                (21U)
/*! FRAMEFORM - This field controls the opcode and address fields. 0x0: Reserved. 0x1: Opcode.
 *    Opcode only, no address. 0x2: Opcode one byte. Opcode, least significant byte of address. 0x3:
 *    Opcode two bytes. Opcode, two least significant bytes of address. 0x4: Opcode three bytes. Opcode,
 *    three least significant bytes of address. 0x5: Opcode four bytes. Opcode, 4 bytes of address.
 *    0x6: No opcode three bytes. No opcode, 3 least significant bytes of address. 0x7: No opcode
 *    four bytes. No opcode, 4 bytes of address.
 */
#define SPIFI_CMD_FRAMEFORM(x)                   (((uint32_t)(((uint32_t)(x)) << SPIFI_CMD_FRAMEFORM_SHIFT)) & SPIFI_CMD_FRAMEFORM_MASK)
#define SPIFI_CMD_OPCODE_MASK                    (0xFF000000U)
#define SPIFI_CMD_OPCODE_SHIFT                   (24U)
/*! OPCODE - The opcode of the command (not used for some FRAMEFORM values).
 */
#define SPIFI_CMD_OPCODE(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_CMD_OPCODE_SHIFT)) & SPIFI_CMD_OPCODE_MASK)
/*! @} */

/*! @name ADDR - SPIFI address register */
/*! @{ */
#define SPIFI_ADDR_ADDR_MASK                     (0xFFFFFFFFU)
#define SPIFI_ADDR_ADDR_SHIFT                    (0U)
/*! ADDR - SPIFI address register
 */
#define SPIFI_ADDR_ADDR(x)                       (((uint32_t)(((uint32_t)(x)) << SPIFI_ADDR_ADDR_SHIFT)) & SPIFI_ADDR_ADDR_MASK)
/*! @} */

/*! @name IDATA - SPIFI intermediate data register */
/*! @{ */
#define SPIFI_IDATA_IDATA_MASK                   (0xFFFFFFFFU)
#define SPIFI_IDATA_IDATA_SHIFT                  (0U)
/*! IDATA - SPIFI intermediate data register
 */
#define SPIFI_IDATA_IDATA(x)                     (((uint32_t)(((uint32_t)(x)) << SPIFI_IDATA_IDATA_SHIFT)) & SPIFI_IDATA_IDATA_MASK)
/*! @} */

/*! @name CLIMIT - SPIFI limit register */
/*! @{ */
#define SPIFI_CLIMIT_CLIMIT_MASK                 (0xFFFFFFFFU)
#define SPIFI_CLIMIT_CLIMIT_SHIFT                (0U)
/*! CLIMIT - SPIFI limit register
 */
#define SPIFI_CLIMIT_CLIMIT(x)                   (((uint32_t)(((uint32_t)(x)) << SPIFI_CLIMIT_CLIMIT_SHIFT)) & SPIFI_CLIMIT_CLIMIT_MASK)
/*! @} */

/*! @name DATA - SPIFI data register. Input or output data */
/*! @{ */
#define SPIFI_DATA_DATA_MASK                     (0xFFFFFFFFU)
#define SPIFI_DATA_DATA_SHIFT                    (0U)
/*! DATA - SPIFI data register. Input or output data
 */
#define SPIFI_DATA_DATA(x)                       (((uint32_t)(((uint32_t)(x)) << SPIFI_DATA_DATA_SHIFT)) & SPIFI_DATA_DATA_MASK)
/*! @} */

/*! @name MCMD - SPIFI memory command register */
/*! @{ */
#define SPIFI_MCMD_POLL_MASK                     (0x4000U)
#define SPIFI_MCMD_POLL_SHIFT                    (14U)
/*! POLL - This bit should be written as 0.
 */
#define SPIFI_MCMD_POLL(x)                       (((uint32_t)(((uint32_t)(x)) << SPIFI_MCMD_POLL_SHIFT)) & SPIFI_MCMD_POLL_MASK)
#define SPIFI_MCMD_DOUT_MASK                     (0x8000U)
#define SPIFI_MCMD_DOUT_SHIFT                    (15U)
/*! DOUT - This bit should be written as 0.
 */
#define SPIFI_MCMD_DOUT(x)                       (((uint32_t)(((uint32_t)(x)) << SPIFI_MCMD_DOUT_SHIFT)) & SPIFI_MCMD_DOUT_MASK)
#define SPIFI_MCMD_INTLEN_MASK                   (0x70000U)
#define SPIFI_MCMD_INTLEN_SHIFT                  (16U)
/*! INTLEN - This field controls how many intermediate bytes precede the data. (Each such byte may
 *    require 8 or 2 SCK cycles, depending on whether the intermediate field is in serial, 2-bit, or
 *    4-bit format.) Intermediate bytes are output by the SPIFI, and include post-address control
 *    information, dummy and delay bytes. See the description of the Intermediate Data register for
 *    the contents of such bytes.
 */
#define SPIFI_MCMD_INTLEN(x)                     (((uint32_t)(((uint32_t)(x)) << SPIFI_MCMD_INTLEN_SHIFT)) & SPIFI_MCMD_INTLEN_MASK)
#define SPIFI_MCMD_FIELDFORM_MASK                (0x180000U)
#define SPIFI_MCMD_FIELDFORM_SHIFT               (19U)
/*! FIELDFORM - This field controls how the fields of the command are sent. 0x0 All serial. All
 *    fields of the command are serial. 0x1 Quad/dual data. Data field is quad/dual, other fields are
 *    serial. 0x2 Serial opcode. Opcode field is serial. Other fields are quad/dual. 0x3 All
 *    quad/dual. All fields of the command are in quad/dual format.
 */
#define SPIFI_MCMD_FIELDFORM(x)                  (((uint32_t)(((uint32_t)(x)) << SPIFI_MCMD_FIELDFORM_SHIFT)) & SPIFI_MCMD_FIELDFORM_MASK)
#define SPIFI_MCMD_FRAMEFORM_MASK                (0xE00000U)
#define SPIFI_MCMD_FRAMEFORM_SHIFT               (21U)
/*! FRAMEFORM - This field controls the opcode and address fields. 0x0: Reserved. 0x1: Opcode.
 *    Opcode only, no address. 0x2: Opcode one byte. Opcode, least-significant byte of address. 0x3:
 *    Opcode two bytes. Opcode, 2 least-significant bytes of address. 0x4: Opcode three bytes. Opcode, 3
 *    least-significant bytes of address. 0x5: Opcode four bytes. Opcode, 4 bytes of address. 0x6:
 *    No opcode three bytes. No opcode, 3 least-significant bytes of address. 0x7: No opcode, 4
 *    bytes of address.
 */
#define SPIFI_MCMD_FRAMEFORM(x)                  (((uint32_t)(((uint32_t)(x)) << SPIFI_MCMD_FRAMEFORM_SHIFT)) & SPIFI_MCMD_FRAMEFORM_MASK)
#define SPIFI_MCMD_OPCODE_MASK                   (0xFF000000U)
#define SPIFI_MCMD_OPCODE_SHIFT                  (24U)
/*! OPCODE - The opcode of the command (not used for some FRAMEFORM values).
 */
#define SPIFI_MCMD_OPCODE(x)                     (((uint32_t)(((uint32_t)(x)) << SPIFI_MCMD_OPCODE_SHIFT)) & SPIFI_MCMD_OPCODE_MASK)
/*! @} */

/*! @name STAT - SPIFI status register */
/*! @{ */
#define SPIFI_STAT_MCINIT_MASK                   (0x1U)
#define SPIFI_STAT_MCINIT_SHIFT                  (0U)
/*! MCINIT - This bit is set when software successfully writes the Memory Command register, and is
 *    cleared by Reset or by writing a 1 to the RESET bit in this register.
 */
#define SPIFI_STAT_MCINIT(x)                     (((uint32_t)(((uint32_t)(x)) << SPIFI_STAT_MCINIT_SHIFT)) & SPIFI_STAT_MCINIT_MASK)
#define SPIFI_STAT_CMD_MASK                      (0x2U)
#define SPIFI_STAT_CMD_SHIFT                     (1U)
/*! CMD - This bit is 1 when the Command register is written. It is cleared by a hardware reset, a
 *    write to the RESET bit in this register, or the deassertion of CS which indicates that the
 *    command has completed communication with the SPI Flash.
 */
#define SPIFI_STAT_CMD(x)                        (((uint32_t)(((uint32_t)(x)) << SPIFI_STAT_CMD_SHIFT)) & SPIFI_STAT_CMD_MASK)
#define SPIFI_STAT_RESET_MASK                    (0x10U)
#define SPIFI_STAT_RESET_SHIFT                   (4U)
/*! RESET - Write a 1 to this bit to abort a current command or memory mode. This bit is cleared
 *    when the hardware is ready for a new command to be written to the Command register.
 */
#define SPIFI_STAT_RESET(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_STAT_RESET_SHIFT)) & SPIFI_STAT_RESET_MASK)
#define SPIFI_STAT_INTRQ_MASK                    (0x20U)
#define SPIFI_STAT_INTRQ_SHIFT                   (5U)
/*! INTRQ - This bit reflects the SPIFI interrupt request. Write a 1 to this bit to clear it. This
 *    bit is set when a CMD was previously 1 and has been cleared due to the deassertion of CS.
 */
#define SPIFI_STAT_INTRQ(x)                      (((uint32_t)(((uint32_t)(x)) << SPIFI_STAT_INTRQ_SHIFT)) & SPIFI_STAT_INTRQ_MASK)
#define SPIFI_STAT_VERSION_MASK                  (0xFF000000U)
#define SPIFI_STAT_VERSION_SHIFT                 (24U)
#define SPIFI_STAT_VERSION(x)                    (((uint32_t)(((uint32_t)(x)) << SPIFI_STAT_VERSION_SHIFT)) & SPIFI_STAT_VERSION_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group SPIFI_Register_Masks */


/* SPIFI - Peripheral instance base addresses */
/** Peripheral SPIFI base address */
#define SPIFI_BASE                               (0x40084000u)
/** Peripheral SPIFI base pointer */
#define SPIFI                                    ((SPIFI_Type *)SPIFI_BASE)
/** Array initializer of SPIFI peripheral base addresses */
#define SPIFI_BASE_ADDRS                         { SPIFI_BASE }
/** Array initializer of SPIFI peripheral base pointers */
#define SPIFI_BASE_PTRS                          { SPIFI }

/*!
 * @}
 */ /* end of group SPIFI_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- SYSCON Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SYSCON_Peripheral_Access_Layer SYSCON Peripheral Access Layer
 * @{
 */

/** SYSCON - Register Layout Typedef */
typedef struct {
  __IO uint32_t MEMORYREMAP;                       /**< Memory Remap control register, offset: 0x0 */
       uint8_t RESERVED_0[12];
  __IO uint32_t AHBMATPRIO;                        /**< AHB Matrix priority control register, offset: 0x10 */
       uint8_t RESERVED_1[44];
  __IO uint32_t SYSTCKCAL;                         /**< System tick counter calibration, offset: 0x40 */
       uint8_t RESERVED_2[4];
  __IO uint32_t NMISRC;                            /**< NMI Source Select, offset: 0x48 */
  __IO uint32_t ASYNCAPBCTRL;                      /**< Asynchronous APB Control, offset: 0x4C */
       uint8_t RESERVED_3[176];
  union {                                          /* offset: 0x100 */
    struct {                                         /* offset: 0x100 */
      __IO uint32_t PRESETCTRL0;                       /**< Peripheral reset control 0, offset: 0x100 */
      __IO uint32_t PRESETCTRL1;                       /**< Peripheral reset control 1, offset: 0x104 */
    } PRESETCTRL;
    __IO uint32_t PRESETCTRLS[2];                    /**< Pin assign register, array offset: 0x100, array step: 0x4 */
  };
       uint8_t RESERVED_4[24];
  union {                                          /* offset: 0x120 */
    struct {                                         /* offset: 0x120 */
      __O  uint32_t PRESETCTRLSET0;                    /**< Set bits in PRESETCTRL0. It is recommended that changes to PRESETCTRL registers be accomplished by using the related PRESETCTRLSET and PRESETCTRLCLR registers., offset: 0x120 */
      __O  uint32_t PRESETCTRLSET1;                    /**< Set bits in PRESETCTRL1. It is recommended that changes to PRESETCTRL registers be accomplished by using the related PRESETCTRLSET and PRESETCTRLCLR registers., offset: 0x124 */
    } PRESETCTRLSET;
    __IO uint32_t PRESETCTRLSETS[2];                 /**< Pin assign register, array offset: 0x120, array step: 0x4 */
  };
       uint8_t RESERVED_5[24];
  union {                                          /* offset: 0x140 */
    struct {                                         /* offset: 0x140 */
      __O  uint32_t PRESETCTRLCLR0;                    /**< Clear bits in PRESETCTRL0. It is recommended that changes to PRESETCTRL registers be accomplished by using the related PRESETCTRLSET and PRESETCTRLCLR registers., offset: 0x140 */
      __O  uint32_t PRESETCTRLCLR1;                    /**< Clear bits in PRESETCTRL1. It is recommended that changes to PRESETCTRL registers be accomplished by using the related PRESETCTRLSET and PRESETCTRLCLR registers., offset: 0x144 */
    } PRESETCTRLCLR;
    __IO uint32_t PRESETCTRLCLRS[2];                 /**< Pin assign register, array offset: 0x140, array step: 0x4 */
  };
       uint8_t RESERVED_6[184];
  union {                                          /* offset: 0x200 */
    struct {                                         /* offset: 0x200 */
      __IO uint32_t AHBCLKCTRL0;                       /**< AHB Clock control 0, offset: 0x200 */
      __IO uint32_t AHBCLKCTRL1;                       /**< AHB Clock control 1, offset: 0x204 */
    } AHBCLKCTRL;
    __IO uint32_t AHBCLKCTRLS[2];                    /**< Pin assign register, array offset: 0x200, array step: 0x4 */
  };
       uint8_t RESERVED_7[24];
  union {                                          /* offset: 0x220 */
    struct {                                         /* offset: 0x220 */
      __O  uint32_t AHBCLKCTRLSET0;                    /**< Set bits in AHBCLKCTRL0, offset: 0x220 */
      __O  uint32_t AHBCLKCTRLSET1;                    /**< Set bits in AHBCLKCTRL1, offset: 0x224 */
    } AHBCLKCTRLSET;
    __IO uint32_t AHBCLKCTRLSETS[2];                 /**< Pin assign register, array offset: 0x220, array step: 0x4 */
  };
       uint8_t RESERVED_8[24];
  union {                                          /* offset: 0x240 */
    struct {                                         /* offset: 0x240 */
      __O  uint32_t AHBCLKCTRLCLR0;                    /**< Clear bits in AHBCLKCTRL0, offset: 0x240 */
      __O  uint32_t AHBCLKCTRLCLR1;                    /**< Clear bits in AHBCLKCTRL1, offset: 0x244 */
    } AHBCLKCTRLCLR;
    __IO uint32_t AHBCLKCTRLCLRS[2];                 /**< Pin assign register, array offset: 0x240, array step: 0x4 */
  };
       uint8_t RESERVED_9[56];
  __IO uint32_t MAINCLKSEL;                        /**< Main clock source select, offset: 0x280 */
  __IO uint32_t OSC32CLKSEL;                       /**< OSC32KCLK and OSC32MCLK clock sources select. Note: this register is not locked by CLOCKGENUPDATELOCKOUT, offset: 0x284 */
  __IO uint32_t CLKOUTSEL;                         /**< CLKOUT clock source select, offset: 0x288 */
       uint8_t RESERVED_10[20];
  __IO uint32_t SPIFICLKSEL;                       /**< SPIFI clock source select, offset: 0x2A0 */
  __IO uint32_t ADCCLKSEL;                         /**< ADC clock source select, offset: 0x2A4 */
       uint8_t RESERVED_11[8];
  __IO uint32_t USARTCLKSEL;                       /**< USART0 & 1 clock source select, offset: 0x2B0 */
  __IO uint32_t I2CCLKSEL;                         /**< I2C0, 1 and 2 clock source select, offset: 0x2B4 */
  __IO uint32_t SPICLKSEL;                         /**< SPI0 & 1 clock source select, offset: 0x2B8 */
  __IO uint32_t IRCLKSEL;                          /**< Infra Red clock source select, offset: 0x2BC */
  __IO uint32_t PWMCLKSEL;                         /**< PWM clock source select, offset: 0x2C0 */
  __IO uint32_t WDTCLKSEL;                         /**< Watchdog Timer clock source select, offset: 0x2C4 */
       uint8_t RESERVED_12[4];
  __IO uint32_t MODEMCLKSEL;                       /**< Modem clock source select, offset: 0x2CC */
       uint8_t RESERVED_13[24];
  __IO uint32_t FRGCLKSEL;                         /**< Fractional Rate Generator (FRG) clock source select. The FRG is one of the USART clocking options., offset: 0x2E8 */
  __IO uint32_t DMICCLKSEL;                        /**< Digital microphone (DMIC) subsystem clock select, offset: 0x2EC */
  __IO uint32_t WKTCLKSEL;                         /**< Wake-up Timer clock select, offset: 0x2F0 */
       uint8_t RESERVED_14[12];
  __IO uint32_t SYSTICKCLKDIV;                     /**< SYSTICK clock divider. The SYSTICK clock can drive the SYSTICK function within the processor., offset: 0x300 */
  __IO uint32_t TRACECLKDIV;                       /**< TRACE clock divider, used for part of the Serial debugger (SWD) feature., offset: 0x304 */
       uint8_t RESERVED_15[100];
  __IO uint32_t WDTCLKDIV;                         /**< Watchdog Timer clock divider, offset: 0x36C */
       uint8_t RESERVED_16[8];
  __IO uint32_t IRCLKDIV;                          /**< Infra Red clock divider, offset: 0x378 */
       uint8_t RESERVED_17[4];
  __IO uint32_t AHBCLKDIV;                         /**< System clock divider, offset: 0x380 */
  __IO uint32_t CLKOUTDIV;                         /**< CLKOUT clock divider, offset: 0x384 */
       uint8_t RESERVED_18[8];
  __IO uint32_t SPIFICLKDIV;                       /**< SPIFI clock divider, offset: 0x390 */
  __IO uint32_t ADCCLKDIV;                         /**< ADC clock divider, offset: 0x394 */
  __IO uint32_t RTCCLKDIV;                         /**< Real Time Clock divider (1 KHz clock generation), offset: 0x398 */
       uint8_t RESERVED_19[4];
  __IO uint32_t FRGCTRL;                           /**< Fractional rate generator divider. The FRG is one of the USART clocking options., offset: 0x3A0 */
       uint8_t RESERVED_20[4];
  __IO uint32_t DMICCLKDIV;                        /**< DMIC clock divider, offset: 0x3A8 */
  __IO uint32_t RTC1HZCLKDIV;                      /**< Real Time Clock divider (1 Hz clock generation. The divider is fixed to 32768), offset: 0x3AC */
       uint8_t RESERVED_21[76];
  __IO uint32_t CLOCKGENUPDATELOCKOUT;             /**< Control clock configuration registers access (like xxxDIV, xxxSEL), offset: 0x3FC */
       uint8_t RESERVED_22[412];
  __IO uint32_t RNGCLKCTRL;                        /**< Random Number Generator Clocks control, offset: 0x59C */
  __IO uint32_t SRAMCTRL;                          /**< All SRAMs common control signals, offset: 0x5A0 */
       uint8_t RESERVED_23[40];
  __IO uint32_t MODEMCTRL;                         /**< Modem (Bluetooth) control and 32K clock enable, offset: 0x5CC */
  __I  uint32_t MODEMSTATUS;                       /**< Modem (Bluetooth) status, offset: 0x5D0 */
  __IO uint32_t XTAL32KCAP;                        /**< XTAL 32 KHz oscillator Capacitor control, offset: 0x5D4 */
  __IO uint32_t XTAL32MCTRL;                       /**< XTAL 32 MHz oscillator control register, offset: 0x5D8 */
       uint8_t RESERVED_24[164];
  union {                                          /* offset: 0x680 */
    struct {                                         /* offset: 0x680 */
      __IO uint32_t STARTER0;                          /**< Start logic 0 wake-up enable register. Enable an interrupt for wake-up from deep-sleep mode. Some bits can also control wake-up from powerdown mode, offset: 0x680 */
      __IO uint32_t STARTER1;                          /**< Start logic 1 wake-up enable register. Enable an interrupt for wake-up from deep-sleep mode. Some bits can also control wake-up from powerdown mode, offset: 0x684 */
    } STARTER;
    __IO uint32_t STARTERS[2];                       /**< Pin assign register, array offset: 0x680, array step: 0x4 */
  };
       uint8_t RESERVED_25[24];
  union {                                          /* offset: 0x6A0 */
    struct {                                         /* offset: 0x6A0 */
      __O  uint32_t STARTERSET0;                       /**< Set bits in STARTER0, offset: 0x6A0 */
      __O  uint32_t STARTERSET1;                       /**< Set bits in STARTER1, offset: 0x6A4 */
    } STARTERSET;
    __IO uint32_t STARTERSETS[2];                    /**< Pin assign register, array offset: 0x6A0, array step: 0x4 */
  };
       uint8_t RESERVED_26[24];
  union {                                          /* offset: 0x6C0 */
    struct {                                         /* offset: 0x6C0 */
      __O  uint32_t STARTERCLR0;                       /**< Clear bits in STARTER0, offset: 0x6C0 */
      __O  uint32_t STARTERCLR1;                       /**< Clear bits in STARTER1, offset: 0x6C4 */
    } STARTERCLR;
    __IO uint32_t STARTERCLRS[2];                    /**< Pin assign register, array offset: 0x6C0, array step: 0x4 */
  };
       uint8_t RESERVED_27[64];
  __IO uint32_t RETENTIONCTRL;                     /**< I/O retention control register, offset: 0x708 */
       uint8_t RESERVED_28[252];
  __IO uint32_t CPSTACK;                           /**< CPSTACK, offset: 0x808 */
       uint8_t RESERVED_29[500];
  __IO uint32_t ANACTRL_CTRL;                      /**< Analog Interrupt control register. Requires AHBCLKCTRL0.ANA_INT_CTRL to be set., offset: 0xA00 */
  __I  uint32_t ANACTRL_VAL;                       /**< Analog modules (BOD and Analog Comparator) outputs current values (BOD 'Power OK' and Analog comparator out). Requires AHBCLKCTRL0.ANA_INT_CTRL to be set., offset: 0xA04 */
  __IO uint32_t ANACTRL_STAT;                      /**< Analog modules (BOD and Analog Comparator) interrupt status. Requires AHBCLKCTRL0.ANA_INT_CTRL to be set., offset: 0xA08 */
  __IO uint32_t ANACTRL_INTENSET;                  /**< Analog modules (BOD and Analog Comparator) Interrupt Enable Read and Set register. Read value indicates which interrupts are enabled. Writing ones sets the corresponding interrupt enable bits. Note, interrupt enable bits are cleared using ANACTRL_INTENCLR. Requires AHBCLKCTRL0.ANA_INT_CTRL to be set to use this register., offset: 0xA0C */
  __O  uint32_t ANACTRL_INTENCLR;                  /**< Analog modules (BOD and Analog Comparator) Interrupt Enable Clear register. Writing ones clears the corresponding interrupt enable bits. Note, interrupt enable bits are set in ANACTRL_INTENSET. Requires AHBCLKCTRL0.ANA_INT_CTRL to be set to use this register., offset: 0xA10 */
  __I  uint32_t ANACTRL_INTSTAT;                   /**< Analog modules (BOD and Analog Comparator) Interrupt Status register (masked with interrupt enable). Requires AHBCLKCTRL0.ANA_INT_CTRL to be set to use this register. Interrupt status bit are cleared using ANACTRL_STAT., offset: 0xA14 */
  __IO uint32_t CLOCK_CTRL;                        /**< Various system clock controls : Flash clock (48 MHz) control, clocks to Frequency Measure function, offset: 0xA18 */
       uint8_t RESERVED_30[4];
  __IO uint32_t WKT_CTRL;                          /**< Wake-up timers control, offset: 0xA20 */
  __IO uint32_t WKT_LOAD_WKT0_LSB;                 /**< Wake-up timer 0 reload value least significant bits ([31:0])., offset: 0xA24 */
  __IO uint32_t WKT_LOAD_WKT0_MSB;                 /**< Wake-up timer 0 reload value most significant bits ([8:0])., offset: 0xA28 */
  __IO uint32_t WKT_LOAD_WKT1;                     /**< Wake-up timer 1 reload value., offset: 0xA2C */
  __I  uint32_t WKT_VAL_WKT0_LSB;                  /**< Wake-up timer 0 current value least significant bits ([31:0]). WARNING : reading not reliable: read this register several times until you get a stable value., offset: 0xA30 */
  __I  uint32_t WKT_VAL_WKT0_MSB;                  /**< Wake-up timer 0 current value most significant bits ([8:0]). WARNING : reading not reliable: read this register several times until you get a stable value., offset: 0xA34 */
  __I  uint32_t WKT_VAL_WKT1;                      /**< Wake-up timer 1 current value. WARNING : reading not reliable: read this register several times until you get a stable value., offset: 0xA38 */
  __IO uint32_t WKT_STAT;                          /**< Wake-up timers status, offset: 0xA3C */
  __IO uint32_t WKT_INTENSET;                      /**< Interrupt Enable Read and Set register, offset: 0xA40 */
  __O  uint32_t WKT_INTENCLR;                      /**< Interrupt Enable Clear register, offset: 0xA44 */
  __I  uint32_t WKT_INTSTAT;                       /**< Interrupt Status register, offset: 0xA48 */
       uint8_t RESERVED_31[956];
  __IO uint32_t GPIOPSYNC;                         /**< Enable bypass of the first stage of synchonization inside GPIO_INT module., offset: 0xE08 */
       uint8_t RESERVED_32[420];
  __I  uint32_t DIEID;                             /**< Chip revision ID & Number, offset: 0xFB0 */
       uint8_t RESERVED_33[60];
  __O  uint32_t CODESECURITYPROT;                  /**< Security code to allow test access via SWD/JTAG. Reset with POR, SW reset or BOD, offset: 0xFF0 */
} SYSCON_Type;

/* ----------------------------------------------------------------------------
   -- SYSCON Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SYSCON_Register_Masks SYSCON Register Masks
 * @{
 */

/*! @name MEMORYREMAP - Memory Remap control register */
/*! @{ */
#define SYSCON_MEMORYREMAP_MAP_MASK              (0x3U)
#define SYSCON_MEMORYREMAP_MAP_SHIFT             (0U)
/*! MAP - Select the location of the vector table : 0: Vector Table in ROM. 1: Vector Table in RAM.
 *    2: Vector Table in Flash. 3: Vector Table in Flash.
 */
#define SYSCON_MEMORYREMAP_MAP(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_MEMORYREMAP_MAP_SHIFT)) & SYSCON_MEMORYREMAP_MAP_MASK)
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_0_MASK (0x300000U)
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_0_SHIFT (20U)
/*! QSPI_REMAP_APP_0 - Address bits to use when QSPI Flash address [19:18] = 0 (256-KB unit page). Setting 00 gives no remapping.
 */
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_0(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_MEMORYREMAP_QSPI_REMAP_APP_0_SHIFT)) & SYSCON_MEMORYREMAP_QSPI_REMAP_APP_0_MASK)
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_1_MASK (0xC00000U)
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_1_SHIFT (22U)
/*! QSPI_REMAP_APP_1 - Address bits to use when QSPI Flash address [19:18] = 1 (256-KB unit page). Setting 01 gives no remapping.
 */
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_1(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_MEMORYREMAP_QSPI_REMAP_APP_1_SHIFT)) & SYSCON_MEMORYREMAP_QSPI_REMAP_APP_1_MASK)
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_2_MASK (0x3000000U)
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_2_SHIFT (24U)
/*! QSPI_REMAP_APP_2 - Address bits to use when QSPI Flash address [19:18] = 2 (256-KB unit page). Setting 10 gives no remapping.
 */
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_2(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_MEMORYREMAP_QSPI_REMAP_APP_2_SHIFT)) & SYSCON_MEMORYREMAP_QSPI_REMAP_APP_2_MASK)
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_3_MASK (0xC000000U)
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_3_SHIFT (26U)
/*! QSPI_REMAP_APP_3 - Address bits to use when QSPI Flash address [19:18] = 3 (256-KB unit page). Setting 11 gives no remapping.
 */
#define SYSCON_MEMORYREMAP_QSPI_REMAP_APP_3(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_MEMORYREMAP_QSPI_REMAP_APP_3_SHIFT)) & SYSCON_MEMORYREMAP_QSPI_REMAP_APP_3_MASK)
/*! @} */

/*! @name AHBMATPRIO - AHB Matrix priority control register */
/*! @{ */
#define SYSCON_AHBMATPRIO_AHBMATPRIO_MASK        (0xFFFFFFFFU)
#define SYSCON_AHBMATPRIO_AHBMATPRIO_SHIFT       (0U)
/*! AHBMATPRIO - AHB Matrix priority control register
 */
#define SYSCON_AHBMATPRIO_AHBMATPRIO(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBMATPRIO_AHBMATPRIO_SHIFT)) & SYSCON_AHBMATPRIO_AHBMATPRIO_MASK)
/*! @} */

/*! @name SYSTCKCAL - System tick counter calibration */
/*! @{ */
#define SYSCON_SYSTCKCAL_CAL_MASK                (0xFFFFFFU)
#define SYSCON_SYSTCKCAL_CAL_SHIFT               (0U)
/*! CAL - Cortex System tick timer calibration value, readable from Cortex SYST_CALIB.TENMS register
 *    field. Set this value to be the number of clock periods to give 10ms period. SYSTICK freq is
 *    a function of the mainclk and SYSTICKCLKDIV register. If the tick timer is configured to use
 *    the System clock directly then this value must reflect the 10ms tick count for that clock.
 */
#define SYSCON_SYSTCKCAL_CAL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_SYSTCKCAL_CAL_SHIFT)) & SYSCON_SYSTCKCAL_CAL_MASK)
#define SYSCON_SYSTCKCAL_SKEW_MASK               (0x1000000U)
#define SYSCON_SYSTCKCAL_SKEW_SHIFT              (24U)
/*! SKEW - Cortex System tick timer SYST_CALIB.SKEW setting. When 0, the value of SYST_CALIB.TENMS
 *    field is considered to be precise. When 1, the value of TENMS is not considered to be precise.
 */
#define SYSCON_SYSTCKCAL_SKEW(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_SYSTCKCAL_SKEW_SHIFT)) & SYSCON_SYSTCKCAL_SKEW_MASK)
#define SYSCON_SYSTCKCAL_NOREF_MASK              (0x2000000U)
#define SYSCON_SYSTCKCAL_NOREF_SHIFT             (25U)
/*! NOREF - Cortex System tick timer SYST_CALIB.NOREF setting. When 0, a separate reference clock is
 *    available. When 1, a separate reference clock is not available.
 */
#define SYSCON_SYSTCKCAL_NOREF(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_SYSTCKCAL_NOREF_SHIFT)) & SYSCON_SYSTCKCAL_NOREF_MASK)
/*! @} */

/*! @name NMISRC - NMI Source Select */
/*! @{ */
#define SYSCON_NMISRC_IRQM40_MASK                (0x3FU)
#define SYSCON_NMISRC_IRQM40_SHIFT               (0U)
/*! IRQM40 - The number of the interrupt source within the interrupt array that acts as the
 *    Non-Maskable Interrupt (NMI) for the Cortex-M4, if enabled by NMIENM40. This can also cause the device
 *    to wakeup from sleep.
 */
#define SYSCON_NMISRC_IRQM40(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_NMISRC_IRQM40_SHIFT)) & SYSCON_NMISRC_IRQM40_MASK)
#define SYSCON_NMISRC_NMIENM40_MASK              (0x80000000U)
#define SYSCON_NMISRC_NMIENM40_SHIFT             (31U)
/*! NMIENM40 - Write a 1 to this bit to enable the Non-Maskable Interrupt (NMI) source selected by
 *    IRQM40. The NMI Interrupt should be disabled before changing the source selection (IRQM40)
 */
#define SYSCON_NMISRC_NMIENM40(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_NMISRC_NMIENM40_SHIFT)) & SYSCON_NMISRC_NMIENM40_MASK)
/*! @} */

/*! @name ASYNCAPBCTRL - Asynchronous APB Control */
/*! @{ */
#define SYSCON_ASYNCAPBCTRL_ENABLE_MASK          (0x1U)
#define SYSCON_ASYNCAPBCTRL_ENABLE_SHIFT         (0U)
/*! ENABLE - Enables the asynchronous APB bridge and subsystem
 */
#define SYSCON_ASYNCAPBCTRL_ENABLE(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_ASYNCAPBCTRL_ENABLE_SHIFT)) & SYSCON_ASYNCAPBCTRL_ENABLE_MASK)
/*! @} */

/*! @name PRESETCTRL0 - Peripheral reset control 0 */
/*! @{ */
#define SYSCON_PRESETCTRL0_SPIFI_RST_MASK        (0x400U)
#define SYSCON_PRESETCTRL0_SPIFI_RST_SHIFT       (10U)
/*! SPIFI_RST - Quad SPI Flash controller reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_SPIFI_RST(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_SPIFI_RST_SHIFT)) & SYSCON_PRESETCTRL0_SPIFI_RST_MASK)
#define SYSCON_PRESETCTRL0_MUX_RST_MASK          (0x800U)
#define SYSCON_PRESETCTRL0_MUX_RST_SHIFT         (11U)
/*! MUX_RST - Input Mux reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_MUX_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_MUX_RST_SHIFT)) & SYSCON_PRESETCTRL0_MUX_RST_MASK)
#define SYSCON_PRESETCTRL0_BLE_TIMING_GEN_RST_MASK (0x1000U)
#define SYSCON_PRESETCTRL0_BLE_TIMING_GEN_RST_SHIFT (12U)
/*! BLE_TIMING_GEN_RST - BLE Low Power Control module reset. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_BLE_TIMING_GEN_RST(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_BLE_TIMING_GEN_RST_SHIFT)) & SYSCON_PRESETCTRL0_BLE_TIMING_GEN_RST_MASK)
#define SYSCON_PRESETCTRL0_IOCON_RST_MASK        (0x2000U)
#define SYSCON_PRESETCTRL0_IOCON_RST_SHIFT       (13U)
/*! IOCON_RST - I/O controller reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_IOCON_RST(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_IOCON_RST_SHIFT)) & SYSCON_PRESETCTRL0_IOCON_RST_MASK)
#define SYSCON_PRESETCTRL0_GPIO_RST_MASK         (0x4000U)
#define SYSCON_PRESETCTRL0_GPIO_RST_SHIFT        (14U)
/*! GPIO_RST - GPIO reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_GPIO_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_GPIO_RST_SHIFT)) & SYSCON_PRESETCTRL0_GPIO_RST_MASK)
#define SYSCON_PRESETCTRL0_PINT_RST_MASK         (0x40000U)
#define SYSCON_PRESETCTRL0_PINT_RST_SHIFT        (18U)
/*! PINT_RST - Pin interrupt (PINT) reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_PINT_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_PINT_RST_SHIFT)) & SYSCON_PRESETCTRL0_PINT_RST_MASK)
#define SYSCON_PRESETCTRL0_GINT_RST_MASK         (0x80000U)
#define SYSCON_PRESETCTRL0_GINT_RST_SHIFT        (19U)
/*! GINT_RST - Group interrupt (GINT) reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_GINT_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_GINT_RST_SHIFT)) & SYSCON_PRESETCTRL0_GINT_RST_MASK)
#define SYSCON_PRESETCTRL0_DMA_RST_MASK          (0x100000U)
#define SYSCON_PRESETCTRL0_DMA_RST_SHIFT         (20U)
/*! DMA_RST - DMA reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_DMA_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_DMA_RST_SHIFT)) & SYSCON_PRESETCTRL0_DMA_RST_MASK)
#define SYSCON_PRESETCTRL0_ISO7816_RST_MASK      (0x200000U)
#define SYSCON_PRESETCTRL0_ISO7816_RST_SHIFT     (21U)
/*! ISO7816_RST - ISO7816 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_ISO7816_RST(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_ISO7816_RST_SHIFT)) & SYSCON_PRESETCTRL0_ISO7816_RST_MASK)
#define SYSCON_PRESETCTRL0_WWDT_RST_MASK         (0x400000U)
#define SYSCON_PRESETCTRL0_WWDT_RST_SHIFT        (22U)
/*! WWDT_RST - Watchdog Timer reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_WWDT_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_WWDT_RST_SHIFT)) & SYSCON_PRESETCTRL0_WWDT_RST_MASK)
#define SYSCON_PRESETCTRL0_RTC_RST_MASK          (0x800000U)
#define SYSCON_PRESETCTRL0_RTC_RST_SHIFT         (23U)
/*! RTC_RST - Real Time Clock (RTC) reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_RTC_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_RTC_RST_SHIFT)) & SYSCON_PRESETCTRL0_RTC_RST_MASK)
#define SYSCON_PRESETCTRL0_ANA_INT_CTRL_RST_MASK (0x1000000U)
#define SYSCON_PRESETCTRL0_ANA_INT_CTRL_RST_SHIFT (24U)
/*! ANA_INT_CTRL_RST - Analog Modules Interrupt Controller reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_ANA_INT_CTRL_RST(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_ANA_INT_CTRL_RST_SHIFT)) & SYSCON_PRESETCTRL0_ANA_INT_CTRL_RST_MASK)
#define SYSCON_PRESETCTRL0_WAKE_UP_TIMERS_RST_MASK (0x2000000U)
#define SYSCON_PRESETCTRL0_WAKE_UP_TIMERS_RST_SHIFT (25U)
/*! WAKE_UP_TIMERS_RST - Wake up Timers reset control. 0: Clear reset to this function. 1: Assert
 *    reset to this function. This will clear interrupt status flag. However, configuration for wake
 *    timers that is in SYSCON will not be reset, these should be managed through the SYSCON
 *    regsiters.
 */
#define SYSCON_PRESETCTRL0_WAKE_UP_TIMERS_RST(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_WAKE_UP_TIMERS_RST_SHIFT)) & SYSCON_PRESETCTRL0_WAKE_UP_TIMERS_RST_MASK)
#define SYSCON_PRESETCTRL0_ADC_RST_MASK          (0x8000000U)
#define SYSCON_PRESETCTRL0_ADC_RST_SHIFT         (27U)
/*! ADC_RST - ADC reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL0_ADC_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL0_ADC_RST_SHIFT)) & SYSCON_PRESETCTRL0_ADC_RST_MASK)
/*! @} */

/*! @name PRESETCTRL1 - Peripheral reset control 1 */
/*! @{ */
#define SYSCON_PRESETCTRL1_USART0_RST_MASK       (0x800U)
#define SYSCON_PRESETCTRL1_USART0_RST_SHIFT      (11U)
/*! USART0_RST - UART0 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_USART0_RST(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_USART0_RST_SHIFT)) & SYSCON_PRESETCTRL1_USART0_RST_MASK)
#define SYSCON_PRESETCTRL1_USART1_RST_MASK       (0x1000U)
#define SYSCON_PRESETCTRL1_USART1_RST_SHIFT      (12U)
/*! USART1_RST - UART1 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_USART1_RST(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_USART1_RST_SHIFT)) & SYSCON_PRESETCTRL1_USART1_RST_MASK)
#define SYSCON_PRESETCTRL1_I2C0_RST_MASK         (0x2000U)
#define SYSCON_PRESETCTRL1_I2C0_RST_SHIFT        (13U)
/*! I2C0_RST - I2C0 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_I2C0_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_I2C0_RST_SHIFT)) & SYSCON_PRESETCTRL1_I2C0_RST_MASK)
#define SYSCON_PRESETCTRL1_I2C1_RST_MASK         (0x4000U)
#define SYSCON_PRESETCTRL1_I2C1_RST_SHIFT        (14U)
/*! I2C1_RST - I2C1 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_I2C1_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_I2C1_RST_SHIFT)) & SYSCON_PRESETCTRL1_I2C1_RST_MASK)
#define SYSCON_PRESETCTRL1_SPI0_RST_MASK         (0x8000U)
#define SYSCON_PRESETCTRL1_SPI0_RST_SHIFT        (15U)
/*! SPI0_RST - SPI0 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_SPI0_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_SPI0_RST_SHIFT)) & SYSCON_PRESETCTRL1_SPI0_RST_MASK)
#define SYSCON_PRESETCTRL1_SPI1_RST_MASK         (0x10000U)
#define SYSCON_PRESETCTRL1_SPI1_RST_SHIFT        (16U)
/*! SPI1_RST - SPI1 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_SPI1_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_SPI1_RST_SHIFT)) & SYSCON_PRESETCTRL1_SPI1_RST_MASK)
#define SYSCON_PRESETCTRL1_IR_RST_MASK           (0x20000U)
#define SYSCON_PRESETCTRL1_IR_RST_SHIFT          (17U)
/*! IR_RST - Infra Red reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_IR_RST(x)             (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_IR_RST_SHIFT)) & SYSCON_PRESETCTRL1_IR_RST_MASK)
#define SYSCON_PRESETCTRL1_PWM_RST_MASK          (0x40000U)
#define SYSCON_PRESETCTRL1_PWM_RST_SHIFT         (18U)
/*! PWM_RST - PWM reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_PWM_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_PWM_RST_SHIFT)) & SYSCON_PRESETCTRL1_PWM_RST_MASK)
#define SYSCON_PRESETCTRL1_RNG_RST_MASK          (0x80000U)
#define SYSCON_PRESETCTRL1_RNG_RST_SHIFT         (19U)
/*! RNG_RST - Random Number Generator reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_RNG_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_RNG_RST_SHIFT)) & SYSCON_PRESETCTRL1_RNG_RST_MASK)
#define SYSCON_PRESETCTRL1_I2C2_RST_MASK         (0x100000U)
#define SYSCON_PRESETCTRL1_I2C2_RST_SHIFT        (20U)
/*! I2C2_RST - I2C2 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_I2C2_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_I2C2_RST_SHIFT)) & SYSCON_PRESETCTRL1_I2C2_RST_MASK)
#define SYSCON_PRESETCTRL1_ZIGBEE_RST_MASK       (0x200000U)
#define SYSCON_PRESETCTRL1_ZIGBEE_RST_SHIFT      (21U)
/*! ZIGBEE_RST - Zigbee reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_ZIGBEE_RST(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_ZIGBEE_RST_SHIFT)) & SYSCON_PRESETCTRL1_ZIGBEE_RST_MASK)
#define SYSCON_PRESETCTRL1_BLE_RST_MASK          (0x400000U)
#define SYSCON_PRESETCTRL1_BLE_RST_SHIFT         (22U)
/*! BLE_RST - BLE reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_BLE_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_BLE_RST_SHIFT)) & SYSCON_PRESETCTRL1_BLE_RST_MASK)
#define SYSCON_PRESETCTRL1_MODEM_MASTER_RST_MASK (0x800000U)
#define SYSCON_PRESETCTRL1_MODEM_MASTER_RST_SHIFT (23U)
/*! MODEM_MASTER_RST - MODEM AHB Master Interface reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_MODEM_MASTER_RST(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_MODEM_MASTER_RST_SHIFT)) & SYSCON_PRESETCTRL1_MODEM_MASTER_RST_MASK)
#define SYSCON_PRESETCTRL1_AES_RST_MASK          (0x1000000U)
#define SYSCON_PRESETCTRL1_AES_RST_SHIFT         (24U)
/*! AES_RST - AES256 reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_AES_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_AES_RST_SHIFT)) & SYSCON_PRESETCTRL1_AES_RST_MASK)
#define SYSCON_PRESETCTRL1_RFP_RST_MASK          (0x2000000U)
#define SYSCON_PRESETCTRL1_RFP_RST_SHIFT         (25U)
/*! RFP_RST - RFP (Radio controller) reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_RFP_RST(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_RFP_RST_SHIFT)) & SYSCON_PRESETCTRL1_RFP_RST_MASK)
#define SYSCON_PRESETCTRL1_DMIC_RST_MASK         (0x4000000U)
#define SYSCON_PRESETCTRL1_DMIC_RST_SHIFT        (26U)
/*! DMIC_RST - DMIC Reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_DMIC_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_DMIC_RST_SHIFT)) & SYSCON_PRESETCTRL1_DMIC_RST_MASK)
#define SYSCON_PRESETCTRL1_HASH_RST_MASK         (0x8000000U)
#define SYSCON_PRESETCTRL1_HASH_RST_SHIFT        (27U)
/*! HASH_RST - HASH reset control. 0: Clear reset to this function. 1: Assert reset to this function.
 */
#define SYSCON_PRESETCTRL1_HASH_RST(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRL1_HASH_RST_SHIFT)) & SYSCON_PRESETCTRL1_HASH_RST_MASK)
/*! @} */

/*! @name PRESETCTRLX_PRESETCTRLS - Pin assign register */
/*! @{ */
#define SYSCON_PRESETCTRLX_PRESETCTRLS_DATA_MASK (0xFFFFFFFFU)
#define SYSCON_PRESETCTRLX_PRESETCTRLS_DATA_SHIFT (0U)
#define SYSCON_PRESETCTRLX_PRESETCTRLS_DATA(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLX_PRESETCTRLS_DATA_SHIFT)) & SYSCON_PRESETCTRLX_PRESETCTRLS_DATA_MASK)
/*! @} */

/* The count of SYSCON_PRESETCTRLX_PRESETCTRLS */
#define SYSCON_PRESETCTRLX_PRESETCTRLS_COUNT     (2U)

/*! @name PRESETCTRLSET0 - Set bits in PRESETCTRL0. It is recommended that changes to PRESETCTRL registers be accomplished by using the related PRESETCTRLSET and PRESETCTRLCLR registers. */
/*! @{ */
#define SYSCON_PRESETCTRLSET0_SPIFI_RST_SET_MASK (0x400U)
#define SYSCON_PRESETCTRLSET0_SPIFI_RST_SET_SHIFT (10U)
/*! SPIFI_RST_SET - Writing one to this register sets the SPIFI_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_SPIFI_RST_SET(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_SPIFI_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_SPIFI_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_MUX_RST_SET_MASK   (0x800U)
#define SYSCON_PRESETCTRLSET0_MUX_RST_SET_SHIFT  (11U)
/*! MUX_RST_SET - Writing one to this register sets the MUX_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_MUX_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_MUX_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_MUX_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_BLE_TIMING_GEN_RST_SET_MASK (0x1000U)
#define SYSCON_PRESETCTRLSET0_BLE_TIMING_GEN_RST_SET_SHIFT (12U)
/*! BLE_TIMING_GEN_RST_SET - Writing one to this register sets the BLE_TIMING_GEN_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_BLE_TIMING_GEN_RST_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_BLE_TIMING_GEN_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_BLE_TIMING_GEN_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_IOCON_RST_SET_MASK (0x2000U)
#define SYSCON_PRESETCTRLSET0_IOCON_RST_SET_SHIFT (13U)
/*! IOCON_RST_SET - Writing one to this register sets the IOCON_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_IOCON_RST_SET(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_IOCON_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_IOCON_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_GPIO_RST_SET_MASK  (0x4000U)
#define SYSCON_PRESETCTRLSET0_GPIO_RST_SET_SHIFT (14U)
/*! GPIO_RST_SET - Writing one to this register sets the GPIO_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_GPIO_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_GPIO_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_GPIO_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_PINT_RST_SET_MASK  (0x40000U)
#define SYSCON_PRESETCTRLSET0_PINT_RST_SET_SHIFT (18U)
/*! PINT_RST_SET - Writing one to this register sets the PINT_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_PINT_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_PINT_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_PINT_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_GINT_RST_SET_MASK  (0x80000U)
#define SYSCON_PRESETCTRLSET0_GINT_RST_SET_SHIFT (19U)
/*! GINT_RST_SET - Writing one to this register sets the GINT_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_GINT_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_GINT_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_GINT_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_DMA_RST_SET_MASK   (0x100000U)
#define SYSCON_PRESETCTRLSET0_DMA_RST_SET_SHIFT  (20U)
/*! DMA_RST_SET - Writing one to this register sets the DMA_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_DMA_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_DMA_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_DMA_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_ISO7816_RST_SET_MASK (0x200000U)
#define SYSCON_PRESETCTRLSET0_ISO7816_RST_SET_SHIFT (21U)
/*! ISO7816_RST_SET - Writing one to this register sets the ISO7816_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_ISO7816_RST_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_ISO7816_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_ISO7816_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_WWDT_RST_SET_MASK  (0x400000U)
#define SYSCON_PRESETCTRLSET0_WWDT_RST_SET_SHIFT (22U)
/*! WWDT_RST_SET - Writing one to this register sets the WWDT_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_WWDT_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_WWDT_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_WWDT_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_RTC_RST_SET_MASK   (0x800000U)
#define SYSCON_PRESETCTRLSET0_RTC_RST_SET_SHIFT  (23U)
/*! RTC_RST_SET - Writing one to this register sets the RTC_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_RTC_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_RTC_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_RTC_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_ANA_INT_CTRL_RST_SET_MASK (0x1000000U)
#define SYSCON_PRESETCTRLSET0_ANA_INT_CTRL_RST_SET_SHIFT (24U)
/*! ANA_INT_CTRL_RST_SET - Writing one to this register sets the ANA_INT_CTRL_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_ANA_INT_CTRL_RST_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_ANA_INT_CTRL_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_ANA_INT_CTRL_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_WAKE_UP_TIMERS_RST_SET_MASK (0x2000000U)
#define SYSCON_PRESETCTRLSET0_WAKE_UP_TIMERS_RST_SET_SHIFT (25U)
/*! WAKE_UP_TIMERS_RST_SET - Writing one to this register sets the WAKE_UP_TIMERS bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_WAKE_UP_TIMERS_RST_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_WAKE_UP_TIMERS_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_WAKE_UP_TIMERS_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET0_ADC_RST_SET_MASK   (0x8000000U)
#define SYSCON_PRESETCTRLSET0_ADC_RST_SET_SHIFT  (27U)
/*! ADC_RST_SET - Writing one to this register sets the ADC_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLSET0_ADC_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET0_ADC_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET0_ADC_RST_SET_MASK)
/*! @} */

/*! @name PRESETCTRLSET1 - Set bits in PRESETCTRL1. It is recommended that changes to PRESETCTRL registers be accomplished by using the related PRESETCTRLSET and PRESETCTRLCLR registers. */
/*! @{ */
#define SYSCON_PRESETCTRLSET1_USART0_RST_SET_MASK (0x800U)
#define SYSCON_PRESETCTRLSET1_USART0_RST_SET_SHIFT (11U)
/*! USART0_RST_SET - Writing one to this register sets the UART0_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_USART0_RST_SET(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_USART0_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_USART0_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_USART1_RST_SET_MASK (0x1000U)
#define SYSCON_PRESETCTRLSET1_USART1_RST_SET_SHIFT (12U)
/*! USART1_RST_SET - Writing one to this register sets the UART1_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_USART1_RST_SET(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_USART1_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_USART1_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_I2C0_RST_SET_MASK  (0x2000U)
#define SYSCON_PRESETCTRLSET1_I2C0_RST_SET_SHIFT (13U)
/*! I2C0_RST_SET - Writing one to this register sets the I2C0_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_I2C0_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_I2C0_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_I2C0_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_I2C1_RST_SET_MASK  (0x4000U)
#define SYSCON_PRESETCTRLSET1_I2C1_RST_SET_SHIFT (14U)
/*! I2C1_RST_SET - Writing one to this register sets the I2C1_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_I2C1_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_I2C1_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_I2C1_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_SPI0_RST_SET_MASK  (0x8000U)
#define SYSCON_PRESETCTRLSET1_SPI0_RST_SET_SHIFT (15U)
/*! SPI0_RST_SET - Writing one to this register sets the SPI0_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_SPI0_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_SPI0_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_SPI0_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_SPI1_RST_SET_MASK  (0x10000U)
#define SYSCON_PRESETCTRLSET1_SPI1_RST_SET_SHIFT (16U)
/*! SPI1_RST_SET - Writing one to this register sets the SPI1_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_SPI1_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_SPI1_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_SPI1_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_IR_RST_SET_MASK    (0x20000U)
#define SYSCON_PRESETCTRLSET1_IR_RST_SET_SHIFT   (17U)
/*! IR_RST_SET - Writing one to this register sets the IR_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_IR_RST_SET(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_IR_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_IR_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_PWM_RST_SET_MASK   (0x40000U)
#define SYSCON_PRESETCTRLSET1_PWM_RST_SET_SHIFT  (18U)
/*! PWM_RST_SET - Writing one to this register sets the PWM_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_PWM_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_PWM_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_PWM_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_RNG_RST_SET_MASK   (0x80000U)
#define SYSCON_PRESETCTRLSET1_RNG_RST_SET_SHIFT  (19U)
/*! RNG_RST_SET - Writing one to this register sets the RNG_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_RNG_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_RNG_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_RNG_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_I2C2_RST_SET_MASK  (0x100000U)
#define SYSCON_PRESETCTRLSET1_I2C2_RST_SET_SHIFT (20U)
/*! I2C2_RST_SET - Writing one to this register sets the I2C2_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_I2C2_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_I2C2_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_I2C2_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_ZIGBEE_RST_SET_MASK (0x200000U)
#define SYSCON_PRESETCTRLSET1_ZIGBEE_RST_SET_SHIFT (21U)
/*! ZIGBEE_RST_SET - Writing one to this register sets the ZIGBEE_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_ZIGBEE_RST_SET(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_ZIGBEE_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_ZIGBEE_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_BLE_RST_SET_MASK   (0x400000U)
#define SYSCON_PRESETCTRLSET1_BLE_RST_SET_SHIFT  (22U)
/*! BLE_RST_SET - Writing one to this register sets the BLE_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_BLE_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_BLE_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_BLE_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_MODEM_MASTER_RST_SET_MASK (0x800000U)
#define SYSCON_PRESETCTRLSET1_MODEM_MASTER_RST_SET_SHIFT (23U)
/*! MODEM_MASTER_RST_SET - Writing one to this register sets the MODEM_MASTER_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_MODEM_MASTER_RST_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_MODEM_MASTER_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_MODEM_MASTER_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_AES_RST_SET_MASK   (0x1000000U)
#define SYSCON_PRESETCTRLSET1_AES_RST_SET_SHIFT  (24U)
/*! AES_RST_SET - Writing one to this register sets the AES_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_AES_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_AES_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_AES_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_RFP_RST_SET_MASK   (0x2000000U)
#define SYSCON_PRESETCTRLSET1_RFP_RST_SET_SHIFT  (25U)
/*! RFP_RST_SET - Writing one to this register sets the RFP_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_RFP_RST_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_RFP_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_RFP_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_DMIC_RST_SET_MASK  (0x4000000U)
#define SYSCON_PRESETCTRLSET1_DMIC_RST_SET_SHIFT (26U)
/*! DMIC_RST_SET - Writing one to this register sets the DMIC_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_DMIC_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_DMIC_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_DMIC_RST_SET_MASK)
#define SYSCON_PRESETCTRLSET1_HASH_RST_SET_MASK  (0x8000000U)
#define SYSCON_PRESETCTRLSET1_HASH_RST_SET_SHIFT (27U)
/*! HASH_RST_SET - Writing one to this register sets the HASH_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLSET1_HASH_RST_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSET1_HASH_RST_SET_SHIFT)) & SYSCON_PRESETCTRLSET1_HASH_RST_SET_MASK)
/*! @} */

/*! @name PRESETCTRLSETX_PRESETCTRLSETS - Pin assign register */
/*! @{ */
#define SYSCON_PRESETCTRLSETX_PRESETCTRLSETS_DATA_MASK (0xFFFFFFFFU)
#define SYSCON_PRESETCTRLSETX_PRESETCTRLSETS_DATA_SHIFT (0U)
#define SYSCON_PRESETCTRLSETX_PRESETCTRLSETS_DATA(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLSETX_PRESETCTRLSETS_DATA_SHIFT)) & SYSCON_PRESETCTRLSETX_PRESETCTRLSETS_DATA_MASK)
/*! @} */

/* The count of SYSCON_PRESETCTRLSETX_PRESETCTRLSETS */
#define SYSCON_PRESETCTRLSETX_PRESETCTRLSETS_COUNT (2U)

/*! @name PRESETCTRLCLR0 - Clear bits in PRESETCTRL0. It is recommended that changes to PRESETCTRL registers be accomplished by using the related PRESETCTRLSET and PRESETCTRLCLR registers. */
/*! @{ */
#define SYSCON_PRESETCTRLCLR0_SPIFI_RST_CLR_MASK (0x400U)
#define SYSCON_PRESETCTRLCLR0_SPIFI_RST_CLR_SHIFT (10U)
/*! SPIFI_RST_CLR - Writing one to this register clears the SPIFI_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_SPIFI_RST_CLR(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_SPIFI_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_SPIFI_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_MUX_RST_CLR_MASK   (0x800U)
#define SYSCON_PRESETCTRLCLR0_MUX_RST_CLR_SHIFT  (11U)
/*! MUX_RST_CLR - Writing one to this register clears the MUX_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_MUX_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_MUX_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_MUX_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_BLE_TIMING_GEN_RST_CLR_MASK (0x1000U)
#define SYSCON_PRESETCTRLCLR0_BLE_TIMING_GEN_RST_CLR_SHIFT (12U)
/*! BLE_TIMING_GEN_RST_CLR - Writing one to this register clears the BLE_TIMING_GEN_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_BLE_TIMING_GEN_RST_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_BLE_TIMING_GEN_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_BLE_TIMING_GEN_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_IOCON_RST_CLR_MASK (0x2000U)
#define SYSCON_PRESETCTRLCLR0_IOCON_RST_CLR_SHIFT (13U)
/*! IOCON_RST_CLR - Writing one to this register clears the IOCON_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_IOCON_RST_CLR(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_IOCON_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_IOCON_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_GPIO_RST_CLR_MASK  (0x4000U)
#define SYSCON_PRESETCTRLCLR0_GPIO_RST_CLR_SHIFT (14U)
/*! GPIO_RST_CLR - Writing one to this register clears the GPIO_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_GPIO_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_GPIO_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_GPIO_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_PINT_RST_CLR_MASK  (0x40000U)
#define SYSCON_PRESETCTRLCLR0_PINT_RST_CLR_SHIFT (18U)
/*! PINT_RST_CLR - Writing one to this register clears the PINT_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_PINT_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_PINT_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_PINT_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_GINT_RST_CLR_MASK  (0x80000U)
#define SYSCON_PRESETCTRLCLR0_GINT_RST_CLR_SHIFT (19U)
/*! GINT_RST_CLR - Writing one to this register clears the GINT_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_GINT_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_GINT_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_GINT_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_DMA_RST_CLR_MASK   (0x100000U)
#define SYSCON_PRESETCTRLCLR0_DMA_RST_CLR_SHIFT  (20U)
/*! DMA_RST_CLR - Writing one to this register clears the DMA_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_DMA_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_DMA_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_DMA_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_ISO7816_RST_CLR_MASK (0x200000U)
#define SYSCON_PRESETCTRLCLR0_ISO7816_RST_CLR_SHIFT (21U)
/*! ISO7816_RST_CLR - Writing one to this register clears the ISO7816_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_ISO7816_RST_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_ISO7816_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_ISO7816_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_WWDT_RST_CLR_MASK  (0x400000U)
#define SYSCON_PRESETCTRLCLR0_WWDT_RST_CLR_SHIFT (22U)
/*! WWDT_RST_CLR - Writing one to this register clears the WWDT_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_WWDT_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_WWDT_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_WWDT_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_RTC_RST_CLR_MASK   (0x800000U)
#define SYSCON_PRESETCTRLCLR0_RTC_RST_CLR_SHIFT  (23U)
/*! RTC_RST_CLR - Writing one to this register clears the RTC_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_RTC_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_RTC_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_RTC_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_ANA_INT_CTRL_RST_CLR_MASK (0x1000000U)
#define SYSCON_PRESETCTRLCLR0_ANA_INT_CTRL_RST_CLR_SHIFT (24U)
/*! ANA_INT_CTRL_RST_CLR - Writing one to this register clears the ANA_INT_CTRL_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_ANA_INT_CTRL_RST_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_ANA_INT_CTRL_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_ANA_INT_CTRL_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_WAKE_UP_TIMERS_RST_CLR_MASK (0x2000000U)
#define SYSCON_PRESETCTRLCLR0_WAKE_UP_TIMERS_RST_CLR_SHIFT (25U)
/*! WAKE_UP_TIMERS_RST_CLR - Writing one to this register clears the WAKE_UP_TIMERS_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_WAKE_UP_TIMERS_RST_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_WAKE_UP_TIMERS_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_WAKE_UP_TIMERS_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR0_ADC_RST_CLR_MASK   (0x8000000U)
#define SYSCON_PRESETCTRLCLR0_ADC_RST_CLR_SHIFT  (27U)
/*! ADC_RST_CLR - Writing one to this register clears the ADC_RST bit in the PRESETCTRL0 register
 */
#define SYSCON_PRESETCTRLCLR0_ADC_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR0_ADC_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR0_ADC_RST_CLR_MASK)
/*! @} */

/*! @name PRESETCTRLCLR1 - Clear bits in PRESETCTRL1. It is recommended that changes to PRESETCTRL registers be accomplished by using the related PRESETCTRLSET and PRESETCTRLCLR registers. */
/*! @{ */
#define SYSCON_PRESETCTRLCLR1_USART0_RST_CLR_MASK (0x800U)
#define SYSCON_PRESETCTRLCLR1_USART0_RST_CLR_SHIFT (11U)
/*! USART0_RST_CLR - Writing one to this register clears the UART0_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_USART0_RST_CLR(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_USART0_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_USART0_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_USART1_RST_CLR_MASK (0x1000U)
#define SYSCON_PRESETCTRLCLR1_USART1_RST_CLR_SHIFT (12U)
/*! USART1_RST_CLR - Writing one to this register clears the UART1_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_USART1_RST_CLR(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_USART1_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_USART1_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_I2C0_RST_CLR_MASK  (0x2000U)
#define SYSCON_PRESETCTRLCLR1_I2C0_RST_CLR_SHIFT (13U)
/*! I2C0_RST_CLR - Writing one to this register clears the I2C0_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_I2C0_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_I2C0_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_I2C0_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_I2C1_RST_CLR_MASK  (0x4000U)
#define SYSCON_PRESETCTRLCLR1_I2C1_RST_CLR_SHIFT (14U)
/*! I2C1_RST_CLR - Writing one to this register clears the I2C1_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_I2C1_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_I2C1_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_I2C1_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_SPI0_RST_CLR_MASK  (0x8000U)
#define SYSCON_PRESETCTRLCLR1_SPI0_RST_CLR_SHIFT (15U)
/*! SPI0_RST_CLR - Writing one to this register clears the SPI0_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_SPI0_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_SPI0_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_SPI0_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_SPI1_RST_CLR_MASK  (0x10000U)
#define SYSCON_PRESETCTRLCLR1_SPI1_RST_CLR_SHIFT (16U)
/*! SPI1_RST_CLR - Writing one to this register clears the SPI1_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_SPI1_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_SPI1_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_SPI1_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_IR_RST_CLR_MASK    (0x20000U)
#define SYSCON_PRESETCTRLCLR1_IR_RST_CLR_SHIFT   (17U)
/*! IR_RST_CLR - Writing one to this register clears the IR_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_IR_RST_CLR(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_IR_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_IR_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_PWM_RST_CLR_MASK   (0x40000U)
#define SYSCON_PRESETCTRLCLR1_PWM_RST_CLR_SHIFT  (18U)
/*! PWM_RST_CLR - Writing one to this register clears the PWM_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_PWM_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_PWM_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_PWM_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_RNG_RST_CLR_MASK   (0x80000U)
#define SYSCON_PRESETCTRLCLR1_RNG_RST_CLR_SHIFT  (19U)
/*! RNG_RST_CLR - Writing one to this register clears the RNG_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_RNG_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_RNG_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_RNG_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_I2C2_RST_CLR_MASK  (0x100000U)
#define SYSCON_PRESETCTRLCLR1_I2C2_RST_CLR_SHIFT (20U)
/*! I2C2_RST_CLR - Writing one to this register clears the I2C2_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_I2C2_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_I2C2_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_I2C2_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_ZIGBEE_RST_CLR_MASK (0x200000U)
#define SYSCON_PRESETCTRLCLR1_ZIGBEE_RST_CLR_SHIFT (21U)
/*! ZIGBEE_RST_CLR - Writing one to this register clears the ZIGBEE_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_ZIGBEE_RST_CLR(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_ZIGBEE_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_ZIGBEE_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_BLE_RST_CLR_MASK   (0x400000U)
#define SYSCON_PRESETCTRLCLR1_BLE_RST_CLR_SHIFT  (22U)
/*! BLE_RST_CLR - Writing one to this register clears the BLE_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_BLE_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_BLE_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_BLE_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_MODEM_MASTER_RST_CLR_MASK (0x800000U)
#define SYSCON_PRESETCTRLCLR1_MODEM_MASTER_RST_CLR_SHIFT (23U)
/*! MODEM_MASTER_RST_CLR - Writing one to this register clears the MODEM_MASTER_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_MODEM_MASTER_RST_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_MODEM_MASTER_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_MODEM_MASTER_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_AES_RST_CLR_MASK   (0x1000000U)
#define SYSCON_PRESETCTRLCLR1_AES_RST_CLR_SHIFT  (24U)
/*! AES_RST_CLR - Writing one to this register clears the AES_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_AES_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_AES_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_AES_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_RFP_RST_CLR_MASK   (0x2000000U)
#define SYSCON_PRESETCTRLCLR1_RFP_RST_CLR_SHIFT  (25U)
/*! RFP_RST_CLR - Writing one to this register clears the RFP_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_RFP_RST_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_RFP_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_RFP_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_DMIC_RST_CLR_MASK  (0x4000000U)
#define SYSCON_PRESETCTRLCLR1_DMIC_RST_CLR_SHIFT (26U)
/*! DMIC_RST_CLR - Writing one to this register clears the DMIC_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_DMIC_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_DMIC_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_DMIC_RST_CLR_MASK)
#define SYSCON_PRESETCTRLCLR1_HASH_RST_CLR_MASK  (0x8000000U)
#define SYSCON_PRESETCTRLCLR1_HASH_RST_CLR_SHIFT (27U)
/*! HASH_RST_CLR - Writing one to this register clears the HASH_RST bit in the PRESETCTRL1 register
 */
#define SYSCON_PRESETCTRLCLR1_HASH_RST_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLR1_HASH_RST_CLR_SHIFT)) & SYSCON_PRESETCTRLCLR1_HASH_RST_CLR_MASK)
/*! @} */

/*! @name PRESETCTRLCLRX_PRESETCTRLCLRS - Pin assign register */
/*! @{ */
#define SYSCON_PRESETCTRLCLRX_PRESETCTRLCLRS_DATA_MASK (0xFFFFFFFFU)
#define SYSCON_PRESETCTRLCLRX_PRESETCTRLCLRS_DATA_SHIFT (0U)
#define SYSCON_PRESETCTRLCLRX_PRESETCTRLCLRS_DATA(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_PRESETCTRLCLRX_PRESETCTRLCLRS_DATA_SHIFT)) & SYSCON_PRESETCTRLCLRX_PRESETCTRLCLRS_DATA_MASK)
/*! @} */

/* The count of SYSCON_PRESETCTRLCLRX_PRESETCTRLCLRS */
#define SYSCON_PRESETCTRLCLRX_PRESETCTRLCLRS_COUNT (2U)

/*! @name AHBCLKCTRL0 - AHB Clock control 0 */
/*! @{ */
#define SYSCON_AHBCLKCTRL0_SRAM_CTRL0_MASK       (0x8U)
#define SYSCON_AHBCLKCTRL0_SRAM_CTRL0_SHIFT      (3U)
/*! SRAM_CTRL0 - Enables the clock for the SRAM Controller 0 (SRAM 0 to SRAM 7). 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_SRAM_CTRL0(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_SRAM_CTRL0_SHIFT)) & SYSCON_AHBCLKCTRL0_SRAM_CTRL0_MASK)
#define SYSCON_AHBCLKCTRL0_SRAM_CTRL1_MASK       (0x10U)
#define SYSCON_AHBCLKCTRL0_SRAM_CTRL1_SHIFT      (4U)
/*! SRAM_CTRL1 - Enables the clock for the SRAM Controller 1 (SRAM 8 to SRAM 11). 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_SRAM_CTRL1(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_SRAM_CTRL1_SHIFT)) & SYSCON_AHBCLKCTRL0_SRAM_CTRL1_MASK)
#define SYSCON_AHBCLKCTRL0_SPIFI_MASK            (0x400U)
#define SYSCON_AHBCLKCTRL0_SPIFI_SHIFT           (10U)
/*! SPIFI - Enables the clock for the Quad SPI Flash controller [Note: SPIFI IOs need configuring
 *    for high drive]. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_SPIFI(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_SPIFI_SHIFT)) & SYSCON_AHBCLKCTRL0_SPIFI_MASK)
#define SYSCON_AHBCLKCTRL0_MUX_MASK              (0x800U)
#define SYSCON_AHBCLKCTRL0_MUX_SHIFT             (11U)
/*! MUX - Enables the clock for the Input Mux. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_MUX(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_MUX_SHIFT)) & SYSCON_AHBCLKCTRL0_MUX_MASK)
#define SYSCON_AHBCLKCTRL0_IOCON_MASK            (0x2000U)
#define SYSCON_AHBCLKCTRL0_IOCON_SHIFT           (13U)
/*! IOCON - Enables the clock for the I/O controller block. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_IOCON(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_IOCON_SHIFT)) & SYSCON_AHBCLKCTRL0_IOCON_MASK)
#define SYSCON_AHBCLKCTRL0_GPIO_MASK             (0x4000U)
#define SYSCON_AHBCLKCTRL0_GPIO_SHIFT            (14U)
/*! GPIO - Enables the clock for the GPIO. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_GPIO(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_GPIO_SHIFT)) & SYSCON_AHBCLKCTRL0_GPIO_MASK)
#define SYSCON_AHBCLKCTRL0_PINT_MASK             (0x40000U)
#define SYSCON_AHBCLKCTRL0_PINT_SHIFT            (18U)
/*! PINT - Enables the clock for the Pin interrupt block (PINT). 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_PINT(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_PINT_SHIFT)) & SYSCON_AHBCLKCTRL0_PINT_MASK)
#define SYSCON_AHBCLKCTRL0_GINT_MASK             (0x80000U)
#define SYSCON_AHBCLKCTRL0_GINT_SHIFT            (19U)
/*! GINT - Enables the clock for the Group interrupt block (GINT). 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_GINT(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_GINT_SHIFT)) & SYSCON_AHBCLKCTRL0_GINT_MASK)
#define SYSCON_AHBCLKCTRL0_DMA_MASK              (0x100000U)
#define SYSCON_AHBCLKCTRL0_DMA_SHIFT             (20U)
/*! DMA - Enables the clock for the DMA controller. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_DMA(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_DMA_SHIFT)) & SYSCON_AHBCLKCTRL0_DMA_MASK)
#define SYSCON_AHBCLKCTRL0_ISO7816_MASK          (0x200000U)
#define SYSCON_AHBCLKCTRL0_ISO7816_SHIFT         (21U)
/*! ISO7816 - Enables the clock for the ISO7816 smart card interface. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_ISO7816(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_ISO7816_SHIFT)) & SYSCON_AHBCLKCTRL0_ISO7816_MASK)
#define SYSCON_AHBCLKCTRL0_WWDT_MASK             (0x400000U)
#define SYSCON_AHBCLKCTRL0_WWDT_SHIFT            (22U)
/*! WWDT - Enables the clock for the Watchdog Timer. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_WWDT(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_WWDT_SHIFT)) & SYSCON_AHBCLKCTRL0_WWDT_MASK)
#define SYSCON_AHBCLKCTRL0_RTC_MASK              (0x800000U)
#define SYSCON_AHBCLKCTRL0_RTC_SHIFT             (23U)
/*! RTC - Enables the clock for the RTC. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_RTC(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_RTC_SHIFT)) & SYSCON_AHBCLKCTRL0_RTC_MASK)
#define SYSCON_AHBCLKCTRL0_ANA_INT_CTRL_MASK     (0x1000000U)
#define SYSCON_AHBCLKCTRL0_ANA_INT_CTRL_SHIFT    (24U)
/*! ANA_INT_CTRL - Enables the clock for the Analog Interrupt Control module (for BOD and comparator
 *    status and interrupt control). 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_ANA_INT_CTRL(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_ANA_INT_CTRL_SHIFT)) & SYSCON_AHBCLKCTRL0_ANA_INT_CTRL_MASK)
#define SYSCON_AHBCLKCTRL0_WAKE_UP_TIMERS_MASK   (0x2000000U)
#define SYSCON_AHBCLKCTRL0_WAKE_UP_TIMERS_SHIFT  (25U)
/*! WAKE_UP_TIMERS - Enables the clock for the Wake up Timers. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_WAKE_UP_TIMERS(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_WAKE_UP_TIMERS_SHIFT)) & SYSCON_AHBCLKCTRL0_WAKE_UP_TIMERS_MASK)
#define SYSCON_AHBCLKCTRL0_ADC_MASK              (0x8000000U)
#define SYSCON_AHBCLKCTRL0_ADC_SHIFT             (27U)
/*! ADC - Enables the clock for the ADC Controller. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL0_ADC(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL0_ADC_SHIFT)) & SYSCON_AHBCLKCTRL0_ADC_MASK)
/*! @} */

/*! @name AHBCLKCTRL1 - AHB Clock control 1 */
/*! @{ */
#define SYSCON_AHBCLKCTRL1_USART0_MASK           (0x800U)
#define SYSCON_AHBCLKCTRL1_USART0_SHIFT          (11U)
/*! USART0 - Enable the clock for the UART0. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_USART0(x)             (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_USART0_SHIFT)) & SYSCON_AHBCLKCTRL1_USART0_MASK)
#define SYSCON_AHBCLKCTRL1_USART1_MASK           (0x1000U)
#define SYSCON_AHBCLKCTRL1_USART1_SHIFT          (12U)
/*! USART1 - Enable the clock for the UART1. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_USART1(x)             (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_USART1_SHIFT)) & SYSCON_AHBCLKCTRL1_USART1_MASK)
#define SYSCON_AHBCLKCTRL1_I2C0_MASK             (0x2000U)
#define SYSCON_AHBCLKCTRL1_I2C0_SHIFT            (13U)
/*! I2C0 - Enable the clock for the I2C0. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_I2C0(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_I2C0_SHIFT)) & SYSCON_AHBCLKCTRL1_I2C0_MASK)
#define SYSCON_AHBCLKCTRL1_I2C1_MASK             (0x4000U)
#define SYSCON_AHBCLKCTRL1_I2C1_SHIFT            (14U)
/*! I2C1 - Enable the clock for the I2C1. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_I2C1(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_I2C1_SHIFT)) & SYSCON_AHBCLKCTRL1_I2C1_MASK)
#define SYSCON_AHBCLKCTRL1_SPI0_MASK             (0x8000U)
#define SYSCON_AHBCLKCTRL1_SPI0_SHIFT            (15U)
/*! SPI0 - Enable the clock for the SPI0. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_SPI0(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_SPI0_SHIFT)) & SYSCON_AHBCLKCTRL1_SPI0_MASK)
#define SYSCON_AHBCLKCTRL1_SPI1_MASK             (0x10000U)
#define SYSCON_AHBCLKCTRL1_SPI1_SHIFT            (16U)
/*! SPI1 - Enable the clock for the SPI1. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_SPI1(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_SPI1_SHIFT)) & SYSCON_AHBCLKCTRL1_SPI1_MASK)
#define SYSCON_AHBCLKCTRL1_IR_MASK               (0x20000U)
#define SYSCON_AHBCLKCTRL1_IR_SHIFT              (17U)
/*! IR - Enable the clock for the Infra Red. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_IR(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_IR_SHIFT)) & SYSCON_AHBCLKCTRL1_IR_MASK)
#define SYSCON_AHBCLKCTRL1_PWM_MASK              (0x40000U)
#define SYSCON_AHBCLKCTRL1_PWM_SHIFT             (18U)
/*! PWM - Enable the clock for the PWM. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_PWM(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_PWM_SHIFT)) & SYSCON_AHBCLKCTRL1_PWM_MASK)
#define SYSCON_AHBCLKCTRL1_RNG_MASK              (0x80000U)
#define SYSCON_AHBCLKCTRL1_RNG_SHIFT             (19U)
/*! RNG - Enable the clock for the Random Number Generator. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_RNG(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_RNG_SHIFT)) & SYSCON_AHBCLKCTRL1_RNG_MASK)
#define SYSCON_AHBCLKCTRL1_I2C2_MASK             (0x100000U)
#define SYSCON_AHBCLKCTRL1_I2C2_SHIFT            (20U)
/*! I2C2 - Enable the clock for the I2C2. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_I2C2(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_I2C2_SHIFT)) & SYSCON_AHBCLKCTRL1_I2C2_MASK)
#define SYSCON_AHBCLKCTRL1_ZIGBEE_MASK           (0x200000U)
#define SYSCON_AHBCLKCTRL1_ZIGBEE_SHIFT          (21U)
/*! ZIGBEE - Enable the clock for the Zigbee Modem . 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_ZIGBEE(x)             (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_ZIGBEE_SHIFT)) & SYSCON_AHBCLKCTRL1_ZIGBEE_MASK)
#define SYSCON_AHBCLKCTRL1_BLE_MASK              (0x400000U)
#define SYSCON_AHBCLKCTRL1_BLE_SHIFT             (22U)
/*! BLE - Enable the clock for the BLE Modem. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_BLE(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_BLE_SHIFT)) & SYSCON_AHBCLKCTRL1_BLE_MASK)
#define SYSCON_AHBCLKCTRL1_MODEM_MASTER_MASK     (0x800000U)
#define SYSCON_AHBCLKCTRL1_MODEM_MASTER_SHIFT    (23U)
/*! MODEM_MASTER - Enable the clock for the Modem AHB Master Interface. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_MODEM_MASTER(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_MODEM_MASTER_SHIFT)) & SYSCON_AHBCLKCTRL1_MODEM_MASTER_MASK)
#define SYSCON_AHBCLKCTRL1_AES_MASK              (0x1000000U)
#define SYSCON_AHBCLKCTRL1_AES_SHIFT             (24U)
/*! AES - Enable the clock for the AES. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_AES(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_AES_SHIFT)) & SYSCON_AHBCLKCTRL1_AES_MASK)
#define SYSCON_AHBCLKCTRL1_RFP_MASK              (0x2000000U)
#define SYSCON_AHBCLKCTRL1_RFP_SHIFT             (25U)
/*! RFP - Enable the clock for the RFP (Radio Front End controller). 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_RFP(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_RFP_SHIFT)) & SYSCON_AHBCLKCTRL1_RFP_MASK)
#define SYSCON_AHBCLKCTRL1_DMIC_MASK             (0x4000000U)
#define SYSCON_AHBCLKCTRL1_DMIC_SHIFT            (26U)
/*! DMIC - Enable the clock for the DMIC. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_DMIC(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_DMIC_SHIFT)) & SYSCON_AHBCLKCTRL1_DMIC_MASK)
#define SYSCON_AHBCLKCTRL1_HASH_MASK             (0x8000000U)
#define SYSCON_AHBCLKCTRL1_HASH_SHIFT            (27U)
/*! HASH - Enable the clock for the Hash. 0: Disable. 1: Enable.
 */
#define SYSCON_AHBCLKCTRL1_HASH(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRL1_HASH_SHIFT)) & SYSCON_AHBCLKCTRL1_HASH_MASK)
/*! @} */

/*! @name AHBCLKCTRLX_AHBCLKCTRLS - Pin assign register */
/*! @{ */
#define SYSCON_AHBCLKCTRLX_AHBCLKCTRLS_DATA_MASK (0xFFFFFFFFU)
#define SYSCON_AHBCLKCTRLX_AHBCLKCTRLS_DATA_SHIFT (0U)
#define SYSCON_AHBCLKCTRLX_AHBCLKCTRLS_DATA(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLX_AHBCLKCTRLS_DATA_SHIFT)) & SYSCON_AHBCLKCTRLX_AHBCLKCTRLS_DATA_MASK)
/*! @} */

/* The count of SYSCON_AHBCLKCTRLX_AHBCLKCTRLS */
#define SYSCON_AHBCLKCTRLX_AHBCLKCTRLS_COUNT     (2U)

/*! @name AHBCLKCTRLSET0 - Set bits in AHBCLKCTRL0 */
/*! @{ */
#define SYSCON_AHBCLKCTRLSET0_SRAM_CTRL0_CLK_SET_MASK (0x8U)
#define SYSCON_AHBCLKCTRLSET0_SRAM_CTRL0_CLK_SET_SHIFT (3U)
/*! SRAM_CTRL0_CLK_SET - Writing one to this register sets the SRAM_CTRL0 bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_SRAM_CTRL0_CLK_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_SRAM_CTRL0_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_SRAM_CTRL0_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_SRAM_CTRL1_CLK_SET_MASK (0x10U)
#define SYSCON_AHBCLKCTRLSET0_SRAM_CTRL1_CLK_SET_SHIFT (4U)
/*! SRAM_CTRL1_CLK_SET - Writing one to this register sets the SRAM_CTRL1 bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_SRAM_CTRL1_CLK_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_SRAM_CTRL1_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_SRAM_CTRL1_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_SPIFI_CLK_SET_MASK (0x400U)
#define SYSCON_AHBCLKCTRLSET0_SPIFI_CLK_SET_SHIFT (10U)
/*! SPIFI_CLK_SET - Writing one to this register sets the SPIFI bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_SPIFI_CLK_SET(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_SPIFI_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_SPIFI_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_MUX_CLK_SET_MASK   (0x800U)
#define SYSCON_AHBCLKCTRLSET0_MUX_CLK_SET_SHIFT  (11U)
/*! MUX_CLK_SET - Writing one to this register sets the MUX bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_MUX_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_MUX_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_MUX_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_IOCON_CLK_SET_MASK (0x2000U)
#define SYSCON_AHBCLKCTRLSET0_IOCON_CLK_SET_SHIFT (13U)
/*! IOCON_CLK_SET - Writing one to this register sets the IOCON bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_IOCON_CLK_SET(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_IOCON_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_IOCON_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_GPIO_CLK_SET_MASK  (0x4000U)
#define SYSCON_AHBCLKCTRLSET0_GPIO_CLK_SET_SHIFT (14U)
/*! GPIO_CLK_SET - Writing one to this register sets the GPIO bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_GPIO_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_GPIO_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_GPIO_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_PINT_CLK_SET_MASK  (0x40000U)
#define SYSCON_AHBCLKCTRLSET0_PINT_CLK_SET_SHIFT (18U)
/*! PINT_CLK_SET - Writing one to this register sets the PINT bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_PINT_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_PINT_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_PINT_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_GINT_CLK_SET_MASK  (0x80000U)
#define SYSCON_AHBCLKCTRLSET0_GINT_CLK_SET_SHIFT (19U)
/*! GINT_CLK_SET - Writing one to this register sets the GINT bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_GINT_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_GINT_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_GINT_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_DMA_CLK_SET_MASK   (0x100000U)
#define SYSCON_AHBCLKCTRLSET0_DMA_CLK_SET_SHIFT  (20U)
/*! DMA_CLK_SET - Writing one to this register sets the DMA bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_DMA_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_DMA_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_DMA_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_ISO7816_CLK_SET_MASK (0x200000U)
#define SYSCON_AHBCLKCTRLSET0_ISO7816_CLK_SET_SHIFT (21U)
/*! ISO7816_CLK_SET - Writing one to this register sets the ISO7816 bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_ISO7816_CLK_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_ISO7816_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_ISO7816_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_WWDT_CLK_SET_MASK  (0x400000U)
#define SYSCON_AHBCLKCTRLSET0_WWDT_CLK_SET_SHIFT (22U)
/*! WWDT_CLK_SET - Writing one to this register sets the WWDT bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_WWDT_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_WWDT_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_WWDT_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_RTC_CLK_SET_MASK   (0x800000U)
#define SYSCON_AHBCLKCTRLSET0_RTC_CLK_SET_SHIFT  (23U)
/*! RTC_CLK_SET - Writing one to this register sets the RTC bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_RTC_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_RTC_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_RTC_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_ANA_INT_CTRL_CLK_SET_MASK (0x1000000U)
#define SYSCON_AHBCLKCTRLSET0_ANA_INT_CTRL_CLK_SET_SHIFT (24U)
/*! ANA_INT_CTRL_CLK_SET - Writing one to this register sets the ANA_INT_CTRL bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_ANA_INT_CTRL_CLK_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_ANA_INT_CTRL_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_ANA_INT_CTRL_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_WAKE_UP_TIMERS_CLK_SET_MASK (0x2000000U)
#define SYSCON_AHBCLKCTRLSET0_WAKE_UP_TIMERS_CLK_SET_SHIFT (25U)
/*! WAKE_UP_TIMERS_CLK_SET - Writing one to this register sets the WAKE_UP_TIMERS bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_WAKE_UP_TIMERS_CLK_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_WAKE_UP_TIMERS_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_WAKE_UP_TIMERS_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET0_ADC_CLK_SET_MASK   (0x8000000U)
#define SYSCON_AHBCLKCTRLSET0_ADC_CLK_SET_SHIFT  (27U)
/*! ADC_CLK_SET - Writing one to this register sets the ADC bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLSET0_ADC_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET0_ADC_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET0_ADC_CLK_SET_MASK)
/*! @} */

/*! @name AHBCLKCTRLSET1 - Set bits in AHBCLKCTRL1 */
/*! @{ */
#define SYSCON_AHBCLKCTRLSET1_USART0_CLK_SET_MASK (0x800U)
#define SYSCON_AHBCLKCTRLSET1_USART0_CLK_SET_SHIFT (11U)
/*! USART0_CLK_SET - Writing one to this register sets the UART0 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_USART0_CLK_SET(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_USART0_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_USART0_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_USART1_CLK_SET_MASK (0x1000U)
#define SYSCON_AHBCLKCTRLSET1_USART1_CLK_SET_SHIFT (12U)
/*! USART1_CLK_SET - Writing one to this register sets the UART1 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_USART1_CLK_SET(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_USART1_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_USART1_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_I2C0_CLK_SET_MASK  (0x2000U)
#define SYSCON_AHBCLKCTRLSET1_I2C0_CLK_SET_SHIFT (13U)
/*! I2C0_CLK_SET - Writing one to this register sets the I2C0 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_I2C0_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_I2C0_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_I2C0_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_I2C1_CLK_SET_MASK  (0x4000U)
#define SYSCON_AHBCLKCTRLSET1_I2C1_CLK_SET_SHIFT (14U)
/*! I2C1_CLK_SET - Writing one to this register sets the I2C1 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_I2C1_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_I2C1_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_I2C1_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_SPI0_CLK_SET_MASK  (0x8000U)
#define SYSCON_AHBCLKCTRLSET1_SPI0_CLK_SET_SHIFT (15U)
/*! SPI0_CLK_SET - Writing one to this register sets the SPI0 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_SPI0_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_SPI0_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_SPI0_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_SPI1_CLK_SET_MASK  (0x10000U)
#define SYSCON_AHBCLKCTRLSET1_SPI1_CLK_SET_SHIFT (16U)
/*! SPI1_CLK_SET - Writing one to this register sets the SPI1 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_SPI1_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_SPI1_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_SPI1_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_IR_CLK_SET_MASK    (0x20000U)
#define SYSCON_AHBCLKCTRLSET1_IR_CLK_SET_SHIFT   (17U)
/*! IR_CLK_SET - Writing one to this register sets the IR bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_IR_CLK_SET(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_IR_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_IR_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_PWM_CLK_SET_MASK   (0x40000U)
#define SYSCON_AHBCLKCTRLSET1_PWM_CLK_SET_SHIFT  (18U)
/*! PWM_CLK_SET - Writing one to this register sets the PWM bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_PWM_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_PWM_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_PWM_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_RNG_CLK_SET_MASK   (0x80000U)
#define SYSCON_AHBCLKCTRLSET1_RNG_CLK_SET_SHIFT  (19U)
/*! RNG_CLK_SET - Writing one to this register sets the RNG bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_RNG_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_RNG_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_RNG_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_I2C2_CLK_SET_MASK  (0x100000U)
#define SYSCON_AHBCLKCTRLSET1_I2C2_CLK_SET_SHIFT (20U)
/*! I2C2_CLK_SET - Writing one to this register sets the I2C2 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_I2C2_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_I2C2_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_I2C2_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_ZIGBEE_CLK_SET_MASK (0x200000U)
#define SYSCON_AHBCLKCTRLSET1_ZIGBEE_CLK_SET_SHIFT (21U)
/*! ZIGBEE_CLK_SET - Writing one to this register sets the ZIGBEE bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_ZIGBEE_CLK_SET(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_ZIGBEE_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_ZIGBEE_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_BLE_CLK_SET_MASK   (0x400000U)
#define SYSCON_AHBCLKCTRLSET1_BLE_CLK_SET_SHIFT  (22U)
/*! BLE_CLK_SET - Writing one to this register sets the BLE bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_BLE_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_BLE_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_BLE_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_MODEM_MASTER_CLK_SET_MASK (0x800000U)
#define SYSCON_AHBCLKCTRLSET1_MODEM_MASTER_CLK_SET_SHIFT (23U)
/*! MODEM_MASTER_CLK_SET - Writing one to this register sets the MODEM_MASTER bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_MODEM_MASTER_CLK_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_MODEM_MASTER_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_MODEM_MASTER_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_AES_CLK_SET_MASK   (0x1000000U)
#define SYSCON_AHBCLKCTRLSET1_AES_CLK_SET_SHIFT  (24U)
/*! AES_CLK_SET - Writing one to this register sets the AES bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_AES_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_AES_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_AES_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_RFP_CLK_SET_MASK   (0x2000000U)
#define SYSCON_AHBCLKCTRLSET1_RFP_CLK_SET_SHIFT  (25U)
/*! RFP_CLK_SET - Writing one to this register sets the RFP bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_RFP_CLK_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_RFP_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_RFP_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_DMIC_CLK_SET_MASK  (0x4000000U)
#define SYSCON_AHBCLKCTRLSET1_DMIC_CLK_SET_SHIFT (26U)
/*! DMIC_CLK_SET - Writing one to this register sets the DMIC bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_DMIC_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_DMIC_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_DMIC_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLSET1_HASH_CLK_SET_MASK  (0x8000000U)
#define SYSCON_AHBCLKCTRLSET1_HASH_CLK_SET_SHIFT (27U)
/*! HASH_CLK_SET - Writing one to this register sets the HASH bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLSET1_HASH_CLK_SET(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSET1_HASH_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLSET1_HASH_CLK_SET_MASK)
/*! @} */

/*! @name AHBCLKCTRLSETX_AHBCLKCTRLSETS - Pin assign register */
/*! @{ */
#define SYSCON_AHBCLKCTRLSETX_AHBCLKCTRLSETS_DATA_MASK (0xFFFFFFFFU)
#define SYSCON_AHBCLKCTRLSETX_AHBCLKCTRLSETS_DATA_SHIFT (0U)
#define SYSCON_AHBCLKCTRLSETX_AHBCLKCTRLSETS_DATA(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLSETX_AHBCLKCTRLSETS_DATA_SHIFT)) & SYSCON_AHBCLKCTRLSETX_AHBCLKCTRLSETS_DATA_MASK)
/*! @} */

/* The count of SYSCON_AHBCLKCTRLSETX_AHBCLKCTRLSETS */
#define SYSCON_AHBCLKCTRLSETX_AHBCLKCTRLSETS_COUNT (2U)

/*! @name AHBCLKCTRLCLR0 - Clear bits in AHBCLKCTRL0 */
/*! @{ */
#define SYSCON_AHBCLKCTRLCLR0_ROM_CLK_CLR_MASK   (0x2U)
#define SYSCON_AHBCLKCTRLCLR0_ROM_CLK_CLR_SHIFT  (1U)
/*! ROM_CLK_CLR - Writing one to this register clears the ROM bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_ROM_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_ROM_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_ROM_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL0_CLK_CLR_MASK (0x8U)
#define SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL0_CLK_CLR_SHIFT (3U)
/*! SRAM_CTRL0_CLK_CLR - Writing one to this register clears the SRAM_CTRL0 bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL0_CLK_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL0_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL0_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL1_CLK_CLR_MASK (0x10U)
#define SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL1_CLK_CLR_SHIFT (4U)
/*! SRAM_CTRL1_CLK_CLR - Writing one to this register clears the SRAM_CTRL1 bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL1_CLK_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL1_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_SRAM_CTRL1_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_FLASH_CLK_CLR_MASK (0x100U)
#define SYSCON_AHBCLKCTRLCLR0_FLASH_CLK_CLR_SHIFT (8U)
/*! FLASH_CLK_CLR - Writing one to this register clears the FLASH bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_FLASH_CLK_CLR(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_FLASH_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_FLASH_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_SPIFI_CLK_CLR_MASK (0x400U)
#define SYSCON_AHBCLKCTRLCLR0_SPIFI_CLK_CLR_SHIFT (10U)
/*! SPIFI_CLK_CLR - Writing one to this register clears the SPIFI bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_SPIFI_CLK_CLR(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_SPIFI_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_SPIFI_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_MUX_CLK_CLR_MASK   (0x800U)
#define SYSCON_AHBCLKCTRLCLR0_MUX_CLK_CLR_SHIFT  (11U)
/*! MUX_CLK_CLR - Writing one to this register clears the MUX bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_MUX_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_MUX_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_MUX_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_IOCON_CLK_CLR_MASK (0x2000U)
#define SYSCON_AHBCLKCTRLCLR0_IOCON_CLK_CLR_SHIFT (13U)
/*! IOCON_CLK_CLR - Writing one to this register clears the IOCON bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_IOCON_CLK_CLR(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_IOCON_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_IOCON_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_GPIO_CLK_CLR_MASK  (0x4000U)
#define SYSCON_AHBCLKCTRLCLR0_GPIO_CLK_CLR_SHIFT (14U)
/*! GPIO_CLK_CLR - Writing one to this register clears the GPIO bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_GPIO_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_GPIO_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_GPIO_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_PINT_CLK_CLR_MASK  (0x40000U)
#define SYSCON_AHBCLKCTRLCLR0_PINT_CLK_CLR_SHIFT (18U)
/*! PINT_CLK_CLR - Writing one to this register clears the PINT bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_PINT_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_PINT_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_PINT_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_GINT_CLK_CLR_MASK  (0x80000U)
#define SYSCON_AHBCLKCTRLCLR0_GINT_CLK_CLR_SHIFT (19U)
/*! GINT_CLK_CLR - Writing one to this register clears the GINT bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_GINT_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_GINT_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_GINT_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_DMA_CLK_CLR_MASK   (0x100000U)
#define SYSCON_AHBCLKCTRLCLR0_DMA_CLK_CLR_SHIFT  (20U)
/*! DMA_CLK_CLR - Writing one to this register clears the DMA bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_DMA_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_DMA_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_DMA_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_ISO7816_CLK_CLR_MASK (0x200000U)
#define SYSCON_AHBCLKCTRLCLR0_ISO7816_CLK_CLR_SHIFT (21U)
/*! ISO7816_CLK_CLR - Writing one to this register clears the ISO7816 bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_ISO7816_CLK_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_ISO7816_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_ISO7816_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_WWDT_CLK_CLR_MASK  (0x400000U)
#define SYSCON_AHBCLKCTRLCLR0_WWDT_CLK_CLR_SHIFT (22U)
/*! WWDT_CLK_CLR - Writing one to this register clears the WWDT bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_WWDT_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_WWDT_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_WWDT_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_RTC_CLK_CLR_MASK   (0x800000U)
#define SYSCON_AHBCLKCTRLCLR0_RTC_CLK_CLR_SHIFT  (23U)
/*! RTC_CLK_CLR - Writing one to this register clears the RTC bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_RTC_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_RTC_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_RTC_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_ANA_INT_CTRL_CLK_SET_MASK (0x1000000U)
#define SYSCON_AHBCLKCTRLCLR0_ANA_INT_CTRL_CLK_SET_SHIFT (24U)
/*! ANA_INT_CTRL_CLK_SET - Writing one to this register clears the ANA_INT_CTRL bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_ANA_INT_CTRL_CLK_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_ANA_INT_CTRL_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_ANA_INT_CTRL_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLCLR0_WAKE_UP_TIMERS_CLK_SET_MASK (0x2000000U)
#define SYSCON_AHBCLKCTRLCLR0_WAKE_UP_TIMERS_CLK_SET_SHIFT (25U)
/*! WAKE_UP_TIMERS_CLK_SET - Writing one to this register clears the WAKE_UP_TIMERS bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_WAKE_UP_TIMERS_CLK_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_WAKE_UP_TIMERS_CLK_SET_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_WAKE_UP_TIMERS_CLK_SET_MASK)
#define SYSCON_AHBCLKCTRLCLR0_ADC_CLK_CLR_MASK   (0x8000000U)
#define SYSCON_AHBCLKCTRLCLR0_ADC_CLK_CLR_SHIFT  (27U)
/*! ADC_CLK_CLR - Writing one to this register clears the ADC bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_ADC_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_ADC_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_ADC_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR0_EFUSE_CLK_CLR_MASK (0x10000000U)
#define SYSCON_AHBCLKCTRLCLR0_EFUSE_CLK_CLR_SHIFT (28U)
/*! EFUSE_CLK_CLR - Writing one to this register clears the EFUSE bit in the AHBCLKCTRL0 register.
 */
#define SYSCON_AHBCLKCTRLCLR0_EFUSE_CLK_CLR(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR0_EFUSE_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR0_EFUSE_CLK_CLR_MASK)
/*! @} */

/*! @name AHBCLKCTRLCLR1 - Clear bits in AHBCLKCTRL1 */
/*! @{ */
#define SYSCON_AHBCLKCTRLCLR1_USART0_CLK_CLR_MASK (0x800U)
#define SYSCON_AHBCLKCTRLCLR1_USART0_CLK_CLR_SHIFT (11U)
/*! USART0_CLK_CLR - Writing one to this register clears the UART0 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_USART0_CLK_CLR(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_USART0_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_USART0_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_USART1_CLK_CLR_MASK (0x1000U)
#define SYSCON_AHBCLKCTRLCLR1_USART1_CLK_CLR_SHIFT (12U)
/*! USART1_CLK_CLR - Writing one to this register clears the UART1 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_USART1_CLK_CLR(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_USART1_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_USART1_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_I2C0_CLK_CLR_MASK  (0x2000U)
#define SYSCON_AHBCLKCTRLCLR1_I2C0_CLK_CLR_SHIFT (13U)
/*! I2C0_CLK_CLR - Writing one to this register clears the I2C0 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_I2C0_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_I2C0_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_I2C0_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_I2C1_CLK_CLR_MASK  (0x4000U)
#define SYSCON_AHBCLKCTRLCLR1_I2C1_CLK_CLR_SHIFT (14U)
/*! I2C1_CLK_CLR - Writing one to this register clears the I2C1 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_I2C1_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_I2C1_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_I2C1_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_SPI0_CLK_CLR_MASK  (0x8000U)
#define SYSCON_AHBCLKCTRLCLR1_SPI0_CLK_CLR_SHIFT (15U)
/*! SPI0_CLK_CLR - Writing one to this register clears the SPI0 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_SPI0_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_SPI0_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_SPI0_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_SPI1_CLK_CLR_MASK  (0x10000U)
#define SYSCON_AHBCLKCTRLCLR1_SPI1_CLK_CLR_SHIFT (16U)
/*! SPI1_CLK_CLR - Writing one to this register clears the SPI1 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_SPI1_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_SPI1_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_SPI1_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_IR_CLK_CLR_MASK    (0x20000U)
#define SYSCON_AHBCLKCTRLCLR1_IR_CLK_CLR_SHIFT   (17U)
/*! IR_CLK_CLR - Writing one to this register clears the IR bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_IR_CLK_CLR(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_IR_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_IR_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_PWM_CLK_CLR_MASK   (0x40000U)
#define SYSCON_AHBCLKCTRLCLR1_PWM_CLK_CLR_SHIFT  (18U)
/*! PWM_CLK_CLR - Writing one to this register clears the PWM bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_PWM_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_PWM_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_PWM_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_RNG_CLK_CLR_MASK   (0x80000U)
#define SYSCON_AHBCLKCTRLCLR1_RNG_CLK_CLR_SHIFT  (19U)
/*! RNG_CLK_CLR - Writing one to this register clears the RNG bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_RNG_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_RNG_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_RNG_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_I2C2_CLK_CLR_MASK  (0x100000U)
#define SYSCON_AHBCLKCTRLCLR1_I2C2_CLK_CLR_SHIFT (20U)
/*! I2C2_CLK_CLR - Writing one to this register clears the I2C2 bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_I2C2_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_I2C2_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_I2C2_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_ZIGBEE_CLK_CLR_MASK (0x200000U)
#define SYSCON_AHBCLKCTRLCLR1_ZIGBEE_CLK_CLR_SHIFT (21U)
/*! ZIGBEE_CLK_CLR - Writing one to this register clears the ZIGBEE bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_ZIGBEE_CLK_CLR(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_ZIGBEE_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_ZIGBEE_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_BLE_CLK_CLR_MASK   (0x400000U)
#define SYSCON_AHBCLKCTRLCLR1_BLE_CLK_CLR_SHIFT  (22U)
/*! BLE_CLK_CLR - Writing one to this register clears the BLE bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_BLE_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_BLE_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_BLE_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_MODEM_MASTER_CLK_CLR_MASK (0x800000U)
#define SYSCON_AHBCLKCTRLCLR1_MODEM_MASTER_CLK_CLR_SHIFT (23U)
/*! MODEM_MASTER_CLK_CLR - Writing one to this register clears the MODEM_MASTER bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_MODEM_MASTER_CLK_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_MODEM_MASTER_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_MODEM_MASTER_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_AES_CLK_CLR_MASK   (0x1000000U)
#define SYSCON_AHBCLKCTRLCLR1_AES_CLK_CLR_SHIFT  (24U)
/*! AES_CLK_CLR - Writing one to this register clears the AES bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_AES_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_AES_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_AES_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_RFP_CLK_CLR_MASK   (0x2000000U)
#define SYSCON_AHBCLKCTRLCLR1_RFP_CLK_CLR_SHIFT  (25U)
/*! RFP_CLK_CLR - Writing one to this register clears the RFP bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_RFP_CLK_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_RFP_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_RFP_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_DMIC_CLK_CLR_MASK  (0x4000000U)
#define SYSCON_AHBCLKCTRLCLR1_DMIC_CLK_CLR_SHIFT (26U)
/*! DMIC_CLK_CLR - Writing one to this register clears the DMIC bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_DMIC_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_DMIC_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_DMIC_CLK_CLR_MASK)
#define SYSCON_AHBCLKCTRLCLR1_HASH_CLK_CLR_MASK  (0x8000000U)
#define SYSCON_AHBCLKCTRLCLR1_HASH_CLK_CLR_SHIFT (27U)
/*! HASH_CLK_CLR - Writing one to this register clears the HASH bit in the AHBCLKCTRL1 register.
 */
#define SYSCON_AHBCLKCTRLCLR1_HASH_CLK_CLR(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLR1_HASH_CLK_CLR_SHIFT)) & SYSCON_AHBCLKCTRLCLR1_HASH_CLK_CLR_MASK)
/*! @} */

/*! @name AHBCLKCTRLCLRX_AHBCLKCTRLCLRS - Pin assign register */
/*! @{ */
#define SYSCON_AHBCLKCTRLCLRX_AHBCLKCTRLCLRS_DATA_MASK (0xFFFFFFFFU)
#define SYSCON_AHBCLKCTRLCLRX_AHBCLKCTRLCLRS_DATA_SHIFT (0U)
#define SYSCON_AHBCLKCTRLCLRX_AHBCLKCTRLCLRS_DATA(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKCTRLCLRX_AHBCLKCTRLCLRS_DATA_SHIFT)) & SYSCON_AHBCLKCTRLCLRX_AHBCLKCTRLCLRS_DATA_MASK)
/*! @} */

/* The count of SYSCON_AHBCLKCTRLCLRX_AHBCLKCTRLCLRS */
#define SYSCON_AHBCLKCTRLCLRX_AHBCLKCTRLCLRS_COUNT (2U)

/*! @name MAINCLKSEL - Main clock source select */
/*! @{ */
#define SYSCON_MAINCLKSEL_SEL_MASK               (0x7U)
#define SYSCON_MAINCLKSEL_SEL_SHIFT              (0U)
/*! SEL - Main clock source selection:
 *  0b000..12 MHz free running oscillator (FRO)
 *  0b010..32 MHz crystal oscillator (XTAL)
 *  0b011..32 MHz free running oscillator (FRO)
 *  0b100..48 MHz free running oscillator (FRO)
 */
#define SYSCON_MAINCLKSEL_SEL(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_MAINCLKSEL_SEL_SHIFT)) & SYSCON_MAINCLKSEL_SEL_MASK)
/*! @} */

/*! @name OSC32CLKSEL - OSC32KCLK and OSC32MCLK clock sources select. Note: this register is not locked by CLOCKGENUPDATELOCKOUT */
/*! @{ */
#define SYSCON_OSC32CLKSEL_SEL32MHZ_MASK         (0x1U)
#define SYSCON_OSC32CLKSEL_SEL32MHZ_SHIFT        (0U)
/*! SEL32MHZ - OSC32MCLK clock source selection
 *  0b0..32 MHz free running oscillator (FRO)
 *  0b1..32 MHz crystal oscillator
 */
#define SYSCON_OSC32CLKSEL_SEL32MHZ(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_OSC32CLKSEL_SEL32MHZ_SHIFT)) & SYSCON_OSC32CLKSEL_SEL32MHZ_MASK)
#define SYSCON_OSC32CLKSEL_SEL32KHZ_MASK         (0x2U)
#define SYSCON_OSC32CLKSEL_SEL32KHZ_SHIFT        (1U)
/*! SEL32KHZ - OSC32KCLK clock source selection
 *  0b0..32 KHz free running oscillator (FRO)
 *  0b1..32 KHz crystal oscillator
 */
#define SYSCON_OSC32CLKSEL_SEL32KHZ(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_OSC32CLKSEL_SEL32KHZ_SHIFT)) & SYSCON_OSC32CLKSEL_SEL32KHZ_MASK)
/*! @} */

/*! @name CLKOUTSEL - CLKOUT clock source select */
/*! @{ */
#define SYSCON_CLKOUTSEL_SEL_MASK                (0x7U)
#define SYSCON_CLKOUTSEL_SEL_SHIFT               (0U)
/*! SEL - CLKOUT clock source selection
 *  0b000..CPU & System Bus clock
 *  0b001..32 KHz crystal oscillator (XTAL)
 *  0b010..32 KHz free running oscillator (FRO)
 *  0b011..32 MHz crystal oscillator (XTAL)
 *  0b101..48 MHz free running oscillator (FRO)
 *  0b110..1 MHz free running oscillator (FRO)
 *  0b111..No clock
 */
#define SYSCON_CLKOUTSEL_SEL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_CLKOUTSEL_SEL_SHIFT)) & SYSCON_CLKOUTSEL_SEL_MASK)
/*! @} */

/*! @name SPIFICLKSEL - SPIFI clock source select */
/*! @{ */
#define SYSCON_SPIFICLKSEL_SEL_MASK              (0x7U)
#define SYSCON_SPIFICLKSEL_SEL_SHIFT             (0U)
/*! SEL - SPIFICLK clock source selection
 *  0b000..CPU & System Bus clock
 *  0b001..32 MHz crystal oscillator (XTAL)
 *  0b010..No clock
 *  0b011..No clock
 *  0b100..No clock
 *  0b101..No clock
 *  0b110..No clock
 *  0b111..No clock
 */
#define SYSCON_SPIFICLKSEL_SEL(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_SPIFICLKSEL_SEL_SHIFT)) & SYSCON_SPIFICLKSEL_SEL_MASK)
/*! @} */

/*! @name ADCCLKSEL - ADC clock source select */
/*! @{ */
#define SYSCON_ADCCLKSEL_SEL_MASK                (0x3U)
#define SYSCON_ADCCLKSEL_SEL_SHIFT               (0U)
/*! SEL - ADCCLK clock source selection
 *  0b00..32 MHz crystal oscillator (XTAL)
 *  0b01..FRO 12 MHz
 *  0b11..No clock
 */
#define SYSCON_ADCCLKSEL_SEL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_ADCCLKSEL_SEL_SHIFT)) & SYSCON_ADCCLKSEL_SEL_MASK)
/*! @} */

/*! @name USARTCLKSEL - USART0 & 1 clock source select */
/*! @{ */
#define SYSCON_USARTCLKSEL_SEL_MASK              (0x3U)
#define SYSCON_USARTCLKSEL_SEL_SHIFT             (0U)
/*! SEL - USARTCLK (USART0 & 1) clock source selection
 *  0b00..Either 32 MHz FRO or 32 MHz XTAL (see OSC32CLKSEL)
 *  0b01..48 MHz free running oscillator (FRO)
 *  0b10..Fractional Rate Generator clock (see FRGCLKSEL)
 *  0b11..No clock
 */
#define SYSCON_USARTCLKSEL_SEL(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_USARTCLKSEL_SEL_SHIFT)) & SYSCON_USARTCLKSEL_SEL_MASK)
/*! @} */

/*! @name I2CCLKSEL - I2C0, 1 and 2 clock source select */
/*! @{ */
#define SYSCON_I2CCLKSEL_SEL_MASK                (0x3U)
#define SYSCON_I2CCLKSEL_SEL_SHIFT               (0U)
/*! SEL - I2CCLK (I2C0 & 1) clock source selection
 *  0b00..Either 32 MHz FRO or 32 MHz XTAL (see OSC32CLKSEL)
 *  0b01..48 MHz free running oscillator (FRO)
 *  0b11..No clock
 */
#define SYSCON_I2CCLKSEL_SEL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_I2CCLKSEL_SEL_SHIFT)) & SYSCON_I2CCLKSEL_SEL_MASK)
/*! @} */

/*! @name SPICLKSEL - SPI0 & 1 clock source select */
/*! @{ */
#define SYSCON_SPICLKSEL_SEL_MASK                (0x3U)
#define SYSCON_SPICLKSEL_SEL_SHIFT               (0U)
/*! SEL - SPICLK (SPI0 & 1) clock source selection
 *  0b00..Either 32 MHz FRO or 32 MHz XTAL (see OSC32CLKSEL)
 *  0b01..48 MHz free running oscillator (FRO)
 *  0b11..No clock
 */
#define SYSCON_SPICLKSEL_SEL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_SPICLKSEL_SEL_SHIFT)) & SYSCON_SPICLKSEL_SEL_MASK)
/*! @} */

/*! @name IRCLKSEL - Infra Red clock source select */
/*! @{ */
#define SYSCON_IRCLKSEL_SEL_MASK                 (0x3U)
#define SYSCON_IRCLKSEL_SEL_SHIFT                (0U)
/*! SEL - IRCLK (IR Blaster) clock source selection
 *  0b00..Either 32 MHz FRO or 32 MHz XTAL (see OSC32CLKSEL)
 *  0b01..48 MHz free running oscillator (FRO)
 *  0b11..No clock
 */
#define SYSCON_IRCLKSEL_SEL(x)                   (((uint32_t)(((uint32_t)(x)) << SYSCON_IRCLKSEL_SEL_SHIFT)) & SYSCON_IRCLKSEL_SEL_MASK)
/*! @} */

/*! @name PWMCLKSEL - PWM clock source select */
/*! @{ */
#define SYSCON_PWMCLKSEL_SEL_MASK                (0x3U)
#define SYSCON_PWMCLKSEL_SEL_SHIFT               (0U)
/*! SEL - PWMCLK (PWM) clock source selection
 *  0b00..Either 32 MHz FRO or 32 MHz XTAL (see OSC32CLKSEL)
 *  0b01..48 MHz free running oscillator (FRO)
 *  0b11..No Clock
 */
#define SYSCON_PWMCLKSEL_SEL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_PWMCLKSEL_SEL_SHIFT)) & SYSCON_PWMCLKSEL_SEL_MASK)
/*! @} */

/*! @name WDTCLKSEL - Watchdog Timer clock source select */
/*! @{ */
#define SYSCON_WDTCLKSEL_SEL_MASK                (0x3U)
#define SYSCON_WDTCLKSEL_SEL_SHIFT               (0U)
/*! SEL - WDTCLK (Watchdog Timer) clock source selection
 *  0b00..Either 32 MHz FRO or 32 MHz XTAL (see OSC32CLKSEL)
 *  0b01..Either 32 KHz FRO or 32 KHz XTAL (see OSC32CLKSEL)
 *  0b10..1 MHz FRO
 *  0b11..No Clock
 */
#define SYSCON_WDTCLKSEL_SEL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_WDTCLKSEL_SEL_SHIFT)) & SYSCON_WDTCLKSEL_SEL_MASK)
/*! @} */

/*! @name MODEMCLKSEL - Modem clock source select */
/*! @{ */
#define SYSCON_MODEMCLKSEL_SEL_ZIGBEE_MASK       (0x1U)
#define SYSCON_MODEMCLKSEL_SEL_ZIGBEE_SHIFT      (0U)
/*! SEL_ZIGBEE - Zigbee Modem clock source selection
 *  0b0..32 MHz XTAL
 *  0b1..No Clock
 */
#define SYSCON_MODEMCLKSEL_SEL_ZIGBEE(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCLKSEL_SEL_ZIGBEE_SHIFT)) & SYSCON_MODEMCLKSEL_SEL_ZIGBEE_MASK)
#define SYSCON_MODEMCLKSEL_SEL_BLE_MASK          (0x2U)
#define SYSCON_MODEMCLKSEL_SEL_BLE_SHIFT         (1U)
/*! SEL_BLE - BLE 32 MHz clock source selection
 *  0b0..32 MHz XTAL
 *  0b1..No Clock
 */
#define SYSCON_MODEMCLKSEL_SEL_BLE(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCLKSEL_SEL_BLE_SHIFT)) & SYSCON_MODEMCLKSEL_SEL_BLE_MASK)
/*! @} */

/*! @name FRGCLKSEL - Fractional Rate Generator (FRG) clock source select. The FRG is one of the USART clocking options. */
/*! @{ */
#define SYSCON_FRGCLKSEL_SEL_MASK                (0x3U)
#define SYSCON_FRGCLKSEL_SEL_SHIFT               (0U)
/*! SEL - Fractional Rate Generator clock source selection
 *  0b00..System Bus clock
 *  0b01..Either 32 MHz FRO or 32 MHz XTAL (see OSC32CLKSEL)
 *  0b10..48 MHz free running oscillator (FRO)
 *  0b11..No Clock
 */
#define SYSCON_FRGCLKSEL_SEL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_FRGCLKSEL_SEL_SHIFT)) & SYSCON_FRGCLKSEL_SEL_MASK)
/*! @} */

/*! @name DMICCLKSEL - Digital microphone (DMIC) subsystem clock select */
/*! @{ */
#define SYSCON_DMICCLKSEL_SEL_MASK               (0x7U)
#define SYSCON_DMICCLKSEL_SEL_SHIFT              (0U)
/*! SEL - DMIC clock source selection
 *  0b000..System Bus clock
 *  0b001..Either 32 KHz FRO or 32 KHz XTAL (see OSC32CLKSEL)
 *  0b010..48 MHz free running oscillator (FRO)
 *  0b011..External clock
 *  0b100..1 MHz free running oscillator (FRO)
 *  0b101..12 MHz free running oscillator (FRO)
 *  0b111..No Clock
 */
#define SYSCON_DMICCLKSEL_SEL(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_DMICCLKSEL_SEL_SHIFT)) & SYSCON_DMICCLKSEL_SEL_MASK)
/*! @} */

/*! @name WKTCLKSEL - Wake-up Timer clock select */
/*! @{ */
#define SYSCON_WKTCLKSEL_SEL_MASK                (0x3U)
#define SYSCON_WKTCLKSEL_SEL_SHIFT               (0U)
/*! SEL - Wake-up Timers clock source selection
 *  0b00..Either 32 KHz FRO or 32 KHz XTAL (see OSC32CLKSEL)
 *  0b11..No Clock
 */
#define SYSCON_WKTCLKSEL_SEL(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_WKTCLKSEL_SEL_SHIFT)) & SYSCON_WKTCLKSEL_SEL_MASK)
/*! @} */

/*! @name SYSTICKCLKDIV - SYSTICK clock divider. The SYSTICK clock can drive the SYSTICK function within the processor. */
/*! @{ */
#define SYSCON_SYSTICKCLKDIV_DIV_MASK            (0xFFU)
#define SYSCON_SYSTICKCLKDIV_DIV_SHIFT           (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 255: Divide by 256.
 */
#define SYSCON_SYSTICKCLKDIV_DIV(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_SYSTICKCLKDIV_DIV_SHIFT)) & SYSCON_SYSTICKCLKDIV_DIV_MASK)
#define SYSCON_SYSTICKCLKDIV_RESET_MASK          (0x20000000U)
#define SYSCON_SYSTICKCLKDIV_RESET_SHIFT         (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_SYSTICKCLKDIV_RESET(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_SYSTICKCLKDIV_RESET_SHIFT)) & SYSCON_SYSTICKCLKDIV_RESET_MASK)
#define SYSCON_SYSTICKCLKDIV_HALT_MASK           (0x40000000U)
#define SYSCON_SYSTICKCLKDIV_HALT_SHIFT          (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_SYSTICKCLKDIV_HALT(x)             (((uint32_t)(((uint32_t)(x)) << SYSCON_SYSTICKCLKDIV_HALT_SHIFT)) & SYSCON_SYSTICKCLKDIV_HALT_MASK)
#define SYSCON_SYSTICKCLKDIV_REQFLAG_MASK        (0x80000000U)
#define SYSCON_SYSTICKCLKDIV_REQFLAG_SHIFT       (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_SYSTICKCLKDIV_REQFLAG(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_SYSTICKCLKDIV_REQFLAG_SHIFT)) & SYSCON_SYSTICKCLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name TRACECLKDIV - TRACE clock divider, used for part of the Serial debugger (SWD) feature. */
/*! @{ */
#define SYSCON_TRACECLKDIV_DIV_MASK              (0xFFU)
#define SYSCON_TRACECLKDIV_DIV_SHIFT             (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 255: Divide by 256.
 */
#define SYSCON_TRACECLKDIV_DIV(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_TRACECLKDIV_DIV_SHIFT)) & SYSCON_TRACECLKDIV_DIV_MASK)
#define SYSCON_TRACECLKDIV_RESET_MASK            (0x20000000U)
#define SYSCON_TRACECLKDIV_RESET_SHIFT           (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_TRACECLKDIV_RESET(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_TRACECLKDIV_RESET_SHIFT)) & SYSCON_TRACECLKDIV_RESET_MASK)
#define SYSCON_TRACECLKDIV_HALT_MASK             (0x40000000U)
#define SYSCON_TRACECLKDIV_HALT_SHIFT            (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_TRACECLKDIV_HALT(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_TRACECLKDIV_HALT_SHIFT)) & SYSCON_TRACECLKDIV_HALT_MASK)
#define SYSCON_TRACECLKDIV_REQFLAG_MASK          (0x80000000U)
#define SYSCON_TRACECLKDIV_REQFLAG_SHIFT         (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_TRACECLKDIV_REQFLAG(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_TRACECLKDIV_REQFLAG_SHIFT)) & SYSCON_TRACECLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name WDTCLKDIV - Watchdog Timer clock divider */
/*! @{ */
#define SYSCON_WDTCLKDIV_DIV_MASK                (0xFFU)
#define SYSCON_WDTCLKDIV_DIV_SHIFT               (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 255: Divide by 256.
 */
#define SYSCON_WDTCLKDIV_DIV(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_WDTCLKDIV_DIV_SHIFT)) & SYSCON_WDTCLKDIV_DIV_MASK)
#define SYSCON_WDTCLKDIV_RESET_MASK              (0x20000000U)
#define SYSCON_WDTCLKDIV_RESET_SHIFT             (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_WDTCLKDIV_RESET(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_WDTCLKDIV_RESET_SHIFT)) & SYSCON_WDTCLKDIV_RESET_MASK)
#define SYSCON_WDTCLKDIV_HALT_MASK               (0x40000000U)
#define SYSCON_WDTCLKDIV_HALT_SHIFT              (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_WDTCLKDIV_HALT(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_WDTCLKDIV_HALT_SHIFT)) & SYSCON_WDTCLKDIV_HALT_MASK)
#define SYSCON_WDTCLKDIV_REQFLAG_MASK            (0x80000000U)
#define SYSCON_WDTCLKDIV_REQFLAG_SHIFT           (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_WDTCLKDIV_REQFLAG(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_WDTCLKDIV_REQFLAG_SHIFT)) & SYSCON_WDTCLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name IRCLKDIV - Infra Red clock divider */
/*! @{ */
#define SYSCON_IRCLKDIV_DIV_MASK                 (0xFU)
#define SYSCON_IRCLKDIV_DIV_SHIFT                (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 15: Divide by 16.
 */
#define SYSCON_IRCLKDIV_DIV(x)                   (((uint32_t)(((uint32_t)(x)) << SYSCON_IRCLKDIV_DIV_SHIFT)) & SYSCON_IRCLKDIV_DIV_MASK)
#define SYSCON_IRCLKDIV_RESET_MASK               (0x20000000U)
#define SYSCON_IRCLKDIV_RESET_SHIFT              (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_IRCLKDIV_RESET(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_IRCLKDIV_RESET_SHIFT)) & SYSCON_IRCLKDIV_RESET_MASK)
#define SYSCON_IRCLKDIV_HALT_MASK                (0x40000000U)
#define SYSCON_IRCLKDIV_HALT_SHIFT               (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_IRCLKDIV_HALT(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_IRCLKDIV_HALT_SHIFT)) & SYSCON_IRCLKDIV_HALT_MASK)
#define SYSCON_IRCLKDIV_REQFLAG_MASK             (0x80000000U)
#define SYSCON_IRCLKDIV_REQFLAG_SHIFT            (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_IRCLKDIV_REQFLAG(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_IRCLKDIV_REQFLAG_SHIFT)) & SYSCON_IRCLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name AHBCLKDIV - System clock divider */
/*! @{ */
#define SYSCON_AHBCLKDIV_DIV_MASK                (0xFFU)
#define SYSCON_AHBCLKDIV_DIV_SHIFT               (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 255: Divide by 256.
 */
#define SYSCON_AHBCLKDIV_DIV(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKDIV_DIV_SHIFT)) & SYSCON_AHBCLKDIV_DIV_MASK)
#define SYSCON_AHBCLKDIV_REQFLAG_MASK            (0x80000000U)
#define SYSCON_AHBCLKDIV_REQFLAG_SHIFT           (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_AHBCLKDIV_REQFLAG(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_AHBCLKDIV_REQFLAG_SHIFT)) & SYSCON_AHBCLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name CLKOUTDIV - CLKOUT clock divider */
/*! @{ */
#define SYSCON_CLKOUTDIV_DIV_MASK                (0xFU)
#define SYSCON_CLKOUTDIV_DIV_SHIFT               (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 15: Divide by 16.
 */
#define SYSCON_CLKOUTDIV_DIV(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_CLKOUTDIV_DIV_SHIFT)) & SYSCON_CLKOUTDIV_DIV_MASK)
#define SYSCON_CLKOUTDIV_RESET_MASK              (0x20000000U)
#define SYSCON_CLKOUTDIV_RESET_SHIFT             (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_CLKOUTDIV_RESET(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_CLKOUTDIV_RESET_SHIFT)) & SYSCON_CLKOUTDIV_RESET_MASK)
#define SYSCON_CLKOUTDIV_HALT_MASK               (0x40000000U)
#define SYSCON_CLKOUTDIV_HALT_SHIFT              (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_CLKOUTDIV_HALT(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_CLKOUTDIV_HALT_SHIFT)) & SYSCON_CLKOUTDIV_HALT_MASK)
#define SYSCON_CLKOUTDIV_REQFLAG_MASK            (0x80000000U)
#define SYSCON_CLKOUTDIV_REQFLAG_SHIFT           (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_CLKOUTDIV_REQFLAG(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_CLKOUTDIV_REQFLAG_SHIFT)) & SYSCON_CLKOUTDIV_REQFLAG_MASK)
/*! @} */

/*! @name SPIFICLKDIV - SPIFI clock divider */
/*! @{ */
#define SYSCON_SPIFICLKDIV_DIV_MASK              (0x3U)
#define SYSCON_SPIFICLKDIV_DIV_SHIFT             (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 3: Divide by 4.
 */
#define SYSCON_SPIFICLKDIV_DIV(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_SPIFICLKDIV_DIV_SHIFT)) & SYSCON_SPIFICLKDIV_DIV_MASK)
#define SYSCON_SPIFICLKDIV_RESET_MASK            (0x20000000U)
#define SYSCON_SPIFICLKDIV_RESET_SHIFT           (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_SPIFICLKDIV_RESET(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_SPIFICLKDIV_RESET_SHIFT)) & SYSCON_SPIFICLKDIV_RESET_MASK)
#define SYSCON_SPIFICLKDIV_HALT_MASK             (0x40000000U)
#define SYSCON_SPIFICLKDIV_HALT_SHIFT            (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_SPIFICLKDIV_HALT(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_SPIFICLKDIV_HALT_SHIFT)) & SYSCON_SPIFICLKDIV_HALT_MASK)
#define SYSCON_SPIFICLKDIV_REQFLAG_MASK          (0x80000000U)
#define SYSCON_SPIFICLKDIV_REQFLAG_SHIFT         (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_SPIFICLKDIV_REQFLAG(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_SPIFICLKDIV_REQFLAG_SHIFT)) & SYSCON_SPIFICLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name ADCCLKDIV - ADC clock divider */
/*! @{ */
#define SYSCON_ADCCLKDIV_DIV_MASK                (0x7U)
#define SYSCON_ADCCLKDIV_DIV_SHIFT               (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 7: Divide by 8.
 */
#define SYSCON_ADCCLKDIV_DIV(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_ADCCLKDIV_DIV_SHIFT)) & SYSCON_ADCCLKDIV_DIV_MASK)
#define SYSCON_ADCCLKDIV_RESET_MASK              (0x20000000U)
#define SYSCON_ADCCLKDIV_RESET_SHIFT             (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_ADCCLKDIV_RESET(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_ADCCLKDIV_RESET_SHIFT)) & SYSCON_ADCCLKDIV_RESET_MASK)
#define SYSCON_ADCCLKDIV_HALT_MASK               (0x40000000U)
#define SYSCON_ADCCLKDIV_HALT_SHIFT              (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_ADCCLKDIV_HALT(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_ADCCLKDIV_HALT_SHIFT)) & SYSCON_ADCCLKDIV_HALT_MASK)
#define SYSCON_ADCCLKDIV_REQFLAG_MASK            (0x80000000U)
#define SYSCON_ADCCLKDIV_REQFLAG_SHIFT           (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_ADCCLKDIV_REQFLAG(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_ADCCLKDIV_REQFLAG_SHIFT)) & SYSCON_ADCCLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name RTCCLKDIV - Real Time Clock divider (1 KHz clock generation) */
/*! @{ */
#define SYSCON_RTCCLKDIV_DIV_MASK                (0x1FU)
#define SYSCON_RTCCLKDIV_DIV_SHIFT               (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 31: Divide by 32.
 */
#define SYSCON_RTCCLKDIV_DIV(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_RTCCLKDIV_DIV_SHIFT)) & SYSCON_RTCCLKDIV_DIV_MASK)
#define SYSCON_RTCCLKDIV_RESET_MASK              (0x20000000U)
#define SYSCON_RTCCLKDIV_RESET_SHIFT             (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_RTCCLKDIV_RESET(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_RTCCLKDIV_RESET_SHIFT)) & SYSCON_RTCCLKDIV_RESET_MASK)
#define SYSCON_RTCCLKDIV_HALT_MASK               (0x40000000U)
#define SYSCON_RTCCLKDIV_HALT_SHIFT              (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_RTCCLKDIV_HALT(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_RTCCLKDIV_HALT_SHIFT)) & SYSCON_RTCCLKDIV_HALT_MASK)
#define SYSCON_RTCCLKDIV_REQFLAG_MASK            (0x80000000U)
#define SYSCON_RTCCLKDIV_REQFLAG_SHIFT           (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_RTCCLKDIV_REQFLAG(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_RTCCLKDIV_REQFLAG_SHIFT)) & SYSCON_RTCCLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name FRGCTRL - Fractional rate generator divider. The FRG is one of the USART clocking options. */
/*! @{ */
#define SYSCON_FRGCTRL_DIV_MASK                  (0xFFU)
#define SYSCON_FRGCTRL_DIV_SHIFT                 (0U)
/*! DIV - Denominator of the fractional divider is equal to the (DIV+1). Always set to 0xFF to use
 *    with the fractional baud rate generator : fout = fin / (1 + MULT/(DIV+1))
 */
#define SYSCON_FRGCTRL_DIV(x)                    (((uint32_t)(((uint32_t)(x)) << SYSCON_FRGCTRL_DIV_SHIFT)) & SYSCON_FRGCTRL_DIV_MASK)
#define SYSCON_FRGCTRL_MULT_MASK                 (0xFF00U)
#define SYSCON_FRGCTRL_MULT_SHIFT                (8U)
/*! MULT - Numerator of the fractional divider. MULT is equal to the programmed value
 */
#define SYSCON_FRGCTRL_MULT(x)                   (((uint32_t)(((uint32_t)(x)) << SYSCON_FRGCTRL_MULT_SHIFT)) & SYSCON_FRGCTRL_MULT_MASK)
/*! @} */

/*! @name DMICCLKDIV - DMIC clock divider */
/*! @{ */
#define SYSCON_DMICCLKDIV_DIV_MASK               (0xFFU)
#define SYSCON_DMICCLKDIV_DIV_SHIFT              (0U)
/*! DIV - Clock divider setting, divider ratio is (DIV+1). E.g. 0: Divide by 1 and 255: Divide by 256.
 */
#define SYSCON_DMICCLKDIV_DIV(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_DMICCLKDIV_DIV_SHIFT)) & SYSCON_DMICCLKDIV_DIV_MASK)
#define SYSCON_DMICCLKDIV_RESET_MASK             (0x20000000U)
#define SYSCON_DMICCLKDIV_RESET_SHIFT            (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_DMICCLKDIV_RESET(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_DMICCLKDIV_RESET_SHIFT)) & SYSCON_DMICCLKDIV_RESET_MASK)
#define SYSCON_DMICCLKDIV_HALT_MASK              (0x40000000U)
#define SYSCON_DMICCLKDIV_HALT_SHIFT             (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_DMICCLKDIV_HALT(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_DMICCLKDIV_HALT_SHIFT)) & SYSCON_DMICCLKDIV_HALT_MASK)
#define SYSCON_DMICCLKDIV_REQFLAG_MASK           (0x80000000U)
#define SYSCON_DMICCLKDIV_REQFLAG_SHIFT          (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_DMICCLKDIV_REQFLAG(x)             (((uint32_t)(((uint32_t)(x)) << SYSCON_DMICCLKDIV_REQFLAG_SHIFT)) & SYSCON_DMICCLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name RTC1HZCLKDIV - Real Time Clock divider (1 Hz clock generation. The divider is fixed to 32768) */
/*! @{ */
#define SYSCON_RTC1HZCLKDIV_RESET_MASK           (0x20000000U)
#define SYSCON_RTC1HZCLKDIV_RESET_SHIFT          (29U)
/*! RESET - Resets the divider counter. Can be used to make sure a new divider value is used right
 *    away rather than completing the previous count.
 */
#define SYSCON_RTC1HZCLKDIV_RESET(x)             (((uint32_t)(((uint32_t)(x)) << SYSCON_RTC1HZCLKDIV_RESET_SHIFT)) & SYSCON_RTC1HZCLKDIV_RESET_MASK)
#define SYSCON_RTC1HZCLKDIV_HALT_MASK            (0x40000000U)
#define SYSCON_RTC1HZCLKDIV_HALT_SHIFT           (30U)
/*! HALT - Halts the divider counter. The intent is to allow the divider s clock source to be
 *    changed without the risk of a glitch at the output.
 */
#define SYSCON_RTC1HZCLKDIV_HALT(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_RTC1HZCLKDIV_HALT_SHIFT)) & SYSCON_RTC1HZCLKDIV_HALT_MASK)
#define SYSCON_RTC1HZCLKDIV_REQFLAG_MASK         (0x80000000U)
#define SYSCON_RTC1HZCLKDIV_REQFLAG_SHIFT        (31U)
/*! REQFLAG - Divider status flag. Set when a change is made to the divider value, cleared when the change is complete.
 */
#define SYSCON_RTC1HZCLKDIV_REQFLAG(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_RTC1HZCLKDIV_REQFLAG_SHIFT)) & SYSCON_RTC1HZCLKDIV_REQFLAG_MASK)
/*! @} */

/*! @name CLOCKGENUPDATELOCKOUT - Control clock configuration registers access (like xxxDIV, xxxSEL) */
/*! @{ */
#define SYSCON_CLOCKGENUPDATELOCKOUT_LOCK_MASK   (0x1U)
#define SYSCON_CLOCKGENUPDATELOCKOUT_LOCK_SHIFT  (0U)
/*! LOCK - When set, disables access to clock control registers (like xxxDIV, xxxSEL). Affects all
 *    clock control registers except OSC32CLKSEL.
 */
#define SYSCON_CLOCKGENUPDATELOCKOUT_LOCK(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_CLOCKGENUPDATELOCKOUT_LOCK_SHIFT)) & SYSCON_CLOCKGENUPDATELOCKOUT_LOCK_MASK)
/*! @} */

/*! @name RNGCLKCTRL - Random Number Generator Clocks control */
/*! @{ */
#define SYSCON_RNGCLKCTRL_ENABLE_MASK            (0x1U)
#define SYSCON_RNGCLKCTRL_ENABLE_SHIFT           (0U)
/*! ENABLE - Enable the clocks used by the Random Number Generator (RNG)
 */
#define SYSCON_RNGCLKCTRL_ENABLE(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_RNGCLKCTRL_ENABLE_SHIFT)) & SYSCON_RNGCLKCTRL_ENABLE_MASK)
/*! @} */

/*! @name SRAMCTRL - All SRAMs common control signals */
/*! @{ */
#define SYSCON_SRAMCTRL_SMB_MASK                 (0x3U)
#define SYSCON_SRAMCTRL_SMB_SHIFT                (0U)
/*! SMB - SMB
 */
#define SYSCON_SRAMCTRL_SMB(x)                   (((uint32_t)(((uint32_t)(x)) << SYSCON_SRAMCTRL_SMB_SHIFT)) & SYSCON_SRAMCTRL_SMB_MASK)
/*! @} */

/*! @name MODEMCTRL - Modem (Bluetooth) control and 32K clock enable */
/*! @{ */
#define SYSCON_MODEMCTRL_BLE_LP_SLEEP_TRIG_MASK  (0x1U)
#define SYSCON_MODEMCTRL_BLE_LP_SLEEP_TRIG_SHIFT (0U)
/*! BLE_LP_SLEEP_TRIG - For normal operation leave at 0. If set to 1 will cause the load of register
 *    in the BLE low power timing block, as if going into deep sleep. This is not needed for the
 *    standard method to enter power down modes.
 */
#define SYSCON_MODEMCTRL_BLE_LP_SLEEP_TRIG(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_LP_SLEEP_TRIG_SHIFT)) & SYSCON_MODEMCTRL_BLE_LP_SLEEP_TRIG_MASK)
#define SYSCON_MODEMCTRL_BLE_FREQ_SEL_MASK       (0x2U)
#define SYSCON_MODEMCTRL_BLE_FREQ_SEL_SHIFT      (1U)
/*! BLE_FREQ_SEL - For normal BLE operation set to 1. BLE operation with this bit set to 0 is not supported. 1 = 16 MHz ; 0 = 8 MHz
 */
#define SYSCON_MODEMCTRL_BLE_FREQ_SEL(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_FREQ_SEL_SHIFT)) & SYSCON_MODEMCTRL_BLE_FREQ_SEL_MASK)
#define SYSCON_MODEMCTRL_BLE_DP_DIV_EN_MASK      (0x4U)
#define SYSCON_MODEMCTRL_BLE_DP_DIV_EN_SHIFT     (2U)
/*! BLE_DP_DIV_EN - For normal BLE operation set to 1. BLE operation with this bit set to 0 is not supported.
 */
#define SYSCON_MODEMCTRL_BLE_DP_DIV_EN(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_DP_DIV_EN_SHIFT)) & SYSCON_MODEMCTRL_BLE_DP_DIV_EN_MASK)
#define SYSCON_MODEMCTRL_BLE_CLK32M_SEL_MASK     (0x8U)
#define SYSCON_MODEMCTRL_BLE_CLK32M_SEL_SHIFT    (3U)
/*! BLE_CLK32M_SEL - For normal BLE operation set to 0. BLE operation with this bit set to 1 is not supported.
 */
#define SYSCON_MODEMCTRL_BLE_CLK32M_SEL(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_CLK32M_SEL_SHIFT)) & SYSCON_MODEMCTRL_BLE_CLK32M_SEL_MASK)
#define SYSCON_MODEMCTRL_BLE_AHB_DIV0_MASK       (0x10U)
#define SYSCON_MODEMCTRL_BLE_AHB_DIV0_SHIFT      (4U)
/*! BLE_AHB_DIV0 - For normal BLE operation set to 0. BLE operation with this bit set to 1 is not supported.
 */
#define SYSCON_MODEMCTRL_BLE_AHB_DIV0(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_AHB_DIV0_SHIFT)) & SYSCON_MODEMCTRL_BLE_AHB_DIV0_MASK)
#define SYSCON_MODEMCTRL_BLE_AHB_DIV1_MASK       (0x20U)
#define SYSCON_MODEMCTRL_BLE_AHB_DIV1_SHIFT      (5U)
/*! BLE_AHB_DIV1 - For normal BLE operation set to 0. BLE operation with this bit set to 1 is not supported.
 */
#define SYSCON_MODEMCTRL_BLE_AHB_DIV1(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_AHB_DIV1_SHIFT)) & SYSCON_MODEMCTRL_BLE_AHB_DIV1_MASK)
#define SYSCON_MODEMCTRL_BLE_HCLK_BLE_EN_MASK    (0x40U)
#define SYSCON_MODEMCTRL_BLE_HCLK_BLE_EN_SHIFT   (6U)
/*! BLE_HCLK_BLE_EN - For normal BLE operation set to 1 to enable the register interface clock
 */
#define SYSCON_MODEMCTRL_BLE_HCLK_BLE_EN(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_HCLK_BLE_EN_SHIFT)) & SYSCON_MODEMCTRL_BLE_HCLK_BLE_EN_MASK)
#define SYSCON_MODEMCTRL_BLE_PHASE_MATCH_1_MASK  (0x80U)
#define SYSCON_MODEMCTRL_BLE_PHASE_MATCH_1_SHIFT (7U)
/*! BLE_PHASE_MATCH_1 - For normal BLE operation set to 0. BLE operation with this bit set to 1 is not supported.
 */
#define SYSCON_MODEMCTRL_BLE_PHASE_MATCH_1(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_PHASE_MATCH_1_SHIFT)) & SYSCON_MODEMCTRL_BLE_PHASE_MATCH_1_MASK)
#define SYSCON_MODEMCTRL_BLE_ISO_ENABLE_MASK     (0x100U)
#define SYSCON_MODEMCTRL_BLE_ISO_ENABLE_SHIFT    (8U)
/*! BLE_ISO_ENABLE - Control isolation of BLE Low Power Control module. When the BLE low power
 *    controller/timers is being used then the isolation must be enabled before entering power down
 *    state. Must not be set before MODEMSTATUS.BLE_LL_CLK_STATUS is high ('1'). 0: isolation is
 *    disabled. 1: isolation is enabled.
 */
#define SYSCON_MODEMCTRL_BLE_ISO_ENABLE(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_ISO_ENABLE_SHIFT)) & SYSCON_MODEMCTRL_BLE_ISO_ENABLE_MASK)
#define SYSCON_MODEMCTRL_BLE_LP_OSC32K_EN_MASK   (0x200U)
#define SYSCON_MODEMCTRL_BLE_LP_OSC32K_EN_SHIFT  (9U)
/*! BLE_LP_OSC32K_EN - 1 = enable the 32K clock to the BLE Low power wake up counter, USART 0 & 1,
 *    LSPI0 & 1, PMC and the frequency measure block
 */
#define SYSCON_MODEMCTRL_BLE_LP_OSC32K_EN(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMCTRL_BLE_LP_OSC32K_EN_SHIFT)) & SYSCON_MODEMCTRL_BLE_LP_OSC32K_EN_MASK)
/*! @} */

/*! @name MODEMSTATUS - Modem (Bluetooth) status */
/*! @{ */
#define SYSCON_MODEMSTATUS_BLE_LL_CLK_STATUS_MASK (0x1U)
#define SYSCON_MODEMSTATUS_BLE_LL_CLK_STATUS_SHIFT (0U)
/*! BLE_LL_CLK_STATUS - Status bit to indicate the clocking status from the link layer. 1 = BLE IP
 *    in deep sleep using low power clock; 0 BLE IP active using 32M clocks
 */
#define SYSCON_MODEMSTATUS_BLE_LL_CLK_STATUS(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMSTATUS_BLE_LL_CLK_STATUS_SHIFT)) & SYSCON_MODEMSTATUS_BLE_LL_CLK_STATUS_MASK)
#define SYSCON_MODEMSTATUS_BLE_LP_OSC_EN_MASK    (0x2U)
#define SYSCON_MODEMSTATUS_BLE_LP_OSC_EN_SHIFT   (1U)
/*! BLE_LP_OSC_EN - BLE low power timing block can be configured to start the wake-up sequence from
 *    power down. This status bit indicates the value of that control bit.
 */
#define SYSCON_MODEMSTATUS_BLE_LP_OSC_EN(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMSTATUS_BLE_LP_OSC_EN_SHIFT)) & SYSCON_MODEMSTATUS_BLE_LP_OSC_EN_MASK)
#define SYSCON_MODEMSTATUS_BLE_LP_RADIO_EN_MASK  (0x4U)
#define SYSCON_MODEMSTATUS_BLE_LP_RADIO_EN_SHIFT (2U)
/*! BLE_LP_RADIO_EN - BLE low power timing block can be configured to start the radio early; this is
 *    not supoorted in this device. However, that control signal can be observed here.
 */
#define SYSCON_MODEMSTATUS_BLE_LP_RADIO_EN(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_MODEMSTATUS_BLE_LP_RADIO_EN_SHIFT)) & SYSCON_MODEMSTATUS_BLE_LP_RADIO_EN_MASK)
/*! @} */

/*! @name XTAL32KCAP - XTAL 32 KHz oscillator Capacitor control */
/*! @{ */
#define SYSCON_XTAL32KCAP_XO_OSC_CAP_IN_MASK     (0x7FU)
#define SYSCON_XTAL32KCAP_XO_OSC_CAP_IN_SHIFT    (0U)
/*! XO_OSC_CAP_IN - Internal Capacitor setting for XTAL_32K_P. This setting selects the internal
 *    capacitance, to ground, that is connected to this XTAL pin. During device testing the capacitor
 *    banks are calibrated so accurate setting of the capacitance can be achieved. Software function
 *    are provided to support the setting of this register. Capacitance range is to apporximately
 *    24pF.
 */
#define SYSCON_XTAL32KCAP_XO_OSC_CAP_IN(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_XTAL32KCAP_XO_OSC_CAP_IN_SHIFT)) & SYSCON_XTAL32KCAP_XO_OSC_CAP_IN_MASK)
#define SYSCON_XTAL32KCAP_XO_OSC_CAP_OUT_MASK    (0x3F80U)
#define SYSCON_XTAL32KCAP_XO_OSC_CAP_OUT_SHIFT   (7U)
/*! XO_OSC_CAP_OUT - Internal Capacitor setting for XTAL_32K_N. This setting selects the internal
 *    capacitance, to ground, that is connected to this XTAL pin. During device testing the capacitor
 *    banks are calibrated so accurate setting of the capacitance can be achieved. Software function
 *    are provided to support the setting of this register. Capacitance range is to apporximately
 *    24pF.
 */
#define SYSCON_XTAL32KCAP_XO_OSC_CAP_OUT(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_XTAL32KCAP_XO_OSC_CAP_OUT_SHIFT)) & SYSCON_XTAL32KCAP_XO_OSC_CAP_OUT_MASK)
/*! @} */

/*! @name XTAL32MCTRL - XTAL 32 MHz oscillator control register */
/*! @{ */
#define SYSCON_XTAL32MCTRL_DEACTIVATE_PMC_CTRL_MASK (0x1U)
#define SYSCON_XTAL32MCTRL_DEACTIVATE_PMC_CTRL_SHIFT (0U)
/*! DEACTIVATE_PMC_CTRL - The 32MHz XTAL is enabled whenever the device is active due to internal
 *    control signals from the PMC. This control bit can deactivate this. 0: Enable XTAL 32 MHz
 *    controls coming from PMC. 1: Disable XTAL 32 MHz controls coming from PMC.
 */
#define SYSCON_XTAL32MCTRL_DEACTIVATE_PMC_CTRL(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_XTAL32MCTRL_DEACTIVATE_PMC_CTRL_SHIFT)) & SYSCON_XTAL32MCTRL_DEACTIVATE_PMC_CTRL_MASK)
#define SYSCON_XTAL32MCTRL_DEACTIVATE_BLE_CTRL_MASK (0x2U)
#define SYSCON_XTAL32MCTRL_DEACTIVATE_BLE_CTRL_SHIFT (1U)
/*! DEACTIVATE_BLE_CTRL - In order to have XTAL ready for BLE after a low power cycle the XTAL must
 *    be started early by the BLE low power module; this can be deactivated. 0: Enable XTAL 32 MHz
 *    controls coming from BLE Low Power Control module. 1: Disable XTAL 32 MHz controls coming from
 *    BLE Low Power Control module.
 */
#define SYSCON_XTAL32MCTRL_DEACTIVATE_BLE_CTRL(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_XTAL32MCTRL_DEACTIVATE_BLE_CTRL_SHIFT)) & SYSCON_XTAL32MCTRL_DEACTIVATE_BLE_CTRL_MASK)
/*! @} */

/*! @name STARTER0 - Start logic 0 wake-up enable register. Enable an interrupt for wake-up from deep-sleep mode. Some bits can also control wake-up from powerdown mode */
/*! @{ */
#define SYSCON_STARTER0_WDT_BOD_MASK             (0x1U)
#define SYSCON_STARTER0_WDT_BOD_SHIFT            (0U)
/*! WDT_BOD - WWDT and BOD Interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER0_WDT_BOD(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_WDT_BOD_SHIFT)) & SYSCON_STARTER0_WDT_BOD_MASK)
#define SYSCON_STARTER0_DMA_MASK                 (0x2U)
#define SYSCON_STARTER0_DMA_SHIFT                (1U)
/*! DMA - DMA Operation in Deep-Sleep and Powerdown not supported. Leave set to 0.
 */
#define SYSCON_STARTER0_DMA(x)                   (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_DMA_SHIFT)) & SYSCON_STARTER0_DMA_MASK)
#define SYSCON_STARTER0_GINT_MASK                (0x4U)
#define SYSCON_STARTER0_GINT_SHIFT               (2U)
/*! GINT - Group Interrupt 0 (GINT0) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_GINT(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_GINT_SHIFT)) & SYSCON_STARTER0_GINT_MASK)
#define SYSCON_STARTER0_IRBLASTER_MASK           (0x8U)
#define SYSCON_STARTER0_IRBLASTER_SHIFT          (3U)
/*! IRBLASTER - Infra Red Blaster interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_IRBLASTER(x)             (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_IRBLASTER_SHIFT)) & SYSCON_STARTER0_IRBLASTER_MASK)
#define SYSCON_STARTER0_PINT0_MASK               (0x10U)
#define SYSCON_STARTER0_PINT0_SHIFT              (4U)
/*! PINT0 - Pattern Interupt 0 (PINT0) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PINT0(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PINT0_SHIFT)) & SYSCON_STARTER0_PINT0_MASK)
#define SYSCON_STARTER0_PINT1_MASK               (0x20U)
#define SYSCON_STARTER0_PINT1_SHIFT              (5U)
/*! PINT1 - Pattern Interupt 1 (PINT1) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PINT1(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PINT1_SHIFT)) & SYSCON_STARTER0_PINT1_MASK)
#define SYSCON_STARTER0_PINT2_MASK               (0x40U)
#define SYSCON_STARTER0_PINT2_SHIFT              (6U)
/*! PINT2 - Pattern Interupt 2 (PINT2) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PINT2(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PINT2_SHIFT)) & SYSCON_STARTER0_PINT2_MASK)
#define SYSCON_STARTER0_PINT3_MASK               (0x80U)
#define SYSCON_STARTER0_PINT3_SHIFT              (7U)
/*! PINT3 - Pattern Interupt 3 (PINT3) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PINT3(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PINT3_SHIFT)) & SYSCON_STARTER0_PINT3_MASK)
#define SYSCON_STARTER0_SPIFI_MASK               (0x100U)
#define SYSCON_STARTER0_SPIFI_SHIFT              (8U)
/*! SPIFI - SPI Flash Interface (SPIFI) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_SPIFI(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_SPIFI_SHIFT)) & SYSCON_STARTER0_SPIFI_MASK)
#define SYSCON_STARTER0_TIMER0_MASK              (0x200U)
#define SYSCON_STARTER0_TIMER0_SHIFT             (9U)
/*! TIMER0 - Counter/Timer0 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_TIMER0(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_TIMER0_SHIFT)) & SYSCON_STARTER0_TIMER0_MASK)
#define SYSCON_STARTER0_TIMER1_MASK              (0x400U)
#define SYSCON_STARTER0_TIMER1_SHIFT             (10U)
/*! TIMER1 - Counter/Timer1 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_TIMER1(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_TIMER1_SHIFT)) & SYSCON_STARTER0_TIMER1_MASK)
#define SYSCON_STARTER0_USART0_MASK              (0x800U)
#define SYSCON_STARTER0_USART0_SHIFT             (11U)
/*! USART0 - USART0 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER0_USART0(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_USART0_SHIFT)) & SYSCON_STARTER0_USART0_MASK)
#define SYSCON_STARTER0_USART1_MASK              (0x1000U)
#define SYSCON_STARTER0_USART1_SHIFT             (12U)
/*! USART1 - USART1 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_USART1(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_USART1_SHIFT)) & SYSCON_STARTER0_USART1_MASK)
#define SYSCON_STARTER0_I2C0_MASK                (0x2000U)
#define SYSCON_STARTER0_I2C0_SHIFT               (13U)
/*! I2C0 - I2C0 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER0_I2C0(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_I2C0_SHIFT)) & SYSCON_STARTER0_I2C0_MASK)
#define SYSCON_STARTER0_I2C1_MASK                (0x4000U)
#define SYSCON_STARTER0_I2C1_SHIFT               (14U)
/*! I2C1 - I2C1 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_I2C1(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_I2C1_SHIFT)) & SYSCON_STARTER0_I2C1_MASK)
#define SYSCON_STARTER0_SPI0_MASK                (0x8000U)
#define SYSCON_STARTER0_SPI0_SHIFT               (15U)
/*! SPI0 - SPI0 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER0_SPI0(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_SPI0_SHIFT)) & SYSCON_STARTER0_SPI0_MASK)
#define SYSCON_STARTER0_SPI1_MASK                (0x10000U)
#define SYSCON_STARTER0_SPI1_SHIFT               (16U)
/*! SPI1 - SPI1 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_SPI1(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_SPI1_SHIFT)) & SYSCON_STARTER0_SPI1_MASK)
#define SYSCON_STARTER0_PWM0_MASK                (0x20000U)
#define SYSCON_STARTER0_PWM0_SHIFT               (17U)
/*! PWM0 - PWM0 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM0(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM0_SHIFT)) & SYSCON_STARTER0_PWM0_MASK)
#define SYSCON_STARTER0_PWM1_MASK                (0x40000U)
#define SYSCON_STARTER0_PWM1_SHIFT               (18U)
/*! PWM1 - PWM1 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM1(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM1_SHIFT)) & SYSCON_STARTER0_PWM1_MASK)
#define SYSCON_STARTER0_PWM2_MASK                (0x80000U)
#define SYSCON_STARTER0_PWM2_SHIFT               (19U)
/*! PWM2 - PWM2 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM2(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM2_SHIFT)) & SYSCON_STARTER0_PWM2_MASK)
#define SYSCON_STARTER0_PWM3_MASK                (0x100000U)
#define SYSCON_STARTER0_PWM3_SHIFT               (20U)
/*! PWM3 - PWM3 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM3(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM3_SHIFT)) & SYSCON_STARTER0_PWM3_MASK)
#define SYSCON_STARTER0_PWM4_MASK                (0x200000U)
#define SYSCON_STARTER0_PWM4_SHIFT               (21U)
/*! PWM4 - PWM4 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM4(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM4_SHIFT)) & SYSCON_STARTER0_PWM4_MASK)
#define SYSCON_STARTER0_PWM5_MASK                (0x400000U)
#define SYSCON_STARTER0_PWM5_SHIFT               (22U)
/*! PWM5 - PWM5 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM5(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM5_SHIFT)) & SYSCON_STARTER0_PWM5_MASK)
#define SYSCON_STARTER0_PWM6_MASK                (0x800000U)
#define SYSCON_STARTER0_PWM6_SHIFT               (23U)
/*! PWM6 - PWM6 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM6(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM6_SHIFT)) & SYSCON_STARTER0_PWM6_MASK)
#define SYSCON_STARTER0_PWM7_MASK                (0x1000000U)
#define SYSCON_STARTER0_PWM7_SHIFT               (24U)
/*! PWM7 - PWM7 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM7(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM7_SHIFT)) & SYSCON_STARTER0_PWM7_MASK)
#define SYSCON_STARTER0_PWM8_MASK                (0x2000000U)
#define SYSCON_STARTER0_PWM8_SHIFT               (25U)
/*! PWM8 - PWM8 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM8(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM8_SHIFT)) & SYSCON_STARTER0_PWM8_MASK)
#define SYSCON_STARTER0_PWM9_MASK                (0x4000000U)
#define SYSCON_STARTER0_PWM9_SHIFT               (26U)
/*! PWM9 - PWM9 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM9(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM9_SHIFT)) & SYSCON_STARTER0_PWM9_MASK)
#define SYSCON_STARTER0_PWM10_MASK               (0x8000000U)
#define SYSCON_STARTER0_PWM10_SHIFT              (27U)
/*! PWM10 - PWM10 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_PWM10(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_PWM10_SHIFT)) & SYSCON_STARTER0_PWM10_MASK)
#define SYSCON_STARTER0_I2C2_MASK                (0x10000000U)
#define SYSCON_STARTER0_I2C2_SHIFT               (28U)
/*! I2C2 - I2C2 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER0_I2C2(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_I2C2_SHIFT)) & SYSCON_STARTER0_I2C2_MASK)
#define SYSCON_STARTER0_RTC_MASK                 (0x20000000U)
#define SYSCON_STARTER0_RTC_SHIFT                (29U)
/*! RTC - Real Time Clock (RTC) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER0_RTC(x)                   (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_RTC_SHIFT)) & SYSCON_STARTER0_RTC_MASK)
#define SYSCON_STARTER0_NFCTAG_MASK              (0x40000000U)
#define SYSCON_STARTER0_NFCTAG_SHIFT             (30U)
/*! NFCTAG - NFC Tag interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from
 *    Deep-Sleep. Only supported on devices with internal NFC tag.
 */
#define SYSCON_STARTER0_NFCTAG(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER0_NFCTAG_SHIFT)) & SYSCON_STARTER0_NFCTAG_MASK)
/*! @} */

/*! @name STARTER1 - Start logic 1 wake-up enable register. Enable an interrupt for wake-up from deep-sleep mode. Some bits can also control wake-up from powerdown mode */
/*! @{ */
#define SYSCON_STARTER1_ADC_SEQA_MASK            (0x1U)
#define SYSCON_STARTER1_ADC_SEQA_SHIFT           (0U)
/*! ADC_SEQA - ADC Sequence A interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_ADC_SEQA(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_ADC_SEQA_SHIFT)) & SYSCON_STARTER1_ADC_SEQA_MASK)
#define SYSCON_STARTER1_ADC_THCMP_OVR_MASK       (0x4U)
#define SYSCON_STARTER1_ADC_THCMP_OVR_SHIFT      (2U)
/*! ADC_THCMP_OVR - ADC threshold and error interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_ADC_THCMP_OVR(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_ADC_THCMP_OVR_SHIFT)) & SYSCON_STARTER1_ADC_THCMP_OVR_MASK)
#define SYSCON_STARTER1_DMIC_MASK                (0x8U)
#define SYSCON_STARTER1_DMIC_SHIFT               (3U)
/*! DMIC - Digital Microphone (DMIC) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_DMIC(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_DMIC_SHIFT)) & SYSCON_STARTER1_DMIC_MASK)
#define SYSCON_STARTER1_HWVAD_MASK               (0x10U)
#define SYSCON_STARTER1_HWVAD_SHIFT              (4U)
/*! HWVAD - Hardware Voice Activity Detector (HWVAD) interrupt wake-up. 0 = Wake-up disabled. 1 =
 *    Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_HWVAD(x)                 (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_HWVAD_SHIFT)) & SYSCON_STARTER1_HWVAD_MASK)
#define SYSCON_STARTER1_BLE_DP_MASK              (0x20U)
#define SYSCON_STARTER1_BLE_DP_SHIFT             (5U)
/*! BLE_DP - BLE Datapath interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_BLE_DP(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_BLE_DP_SHIFT)) & SYSCON_STARTER1_BLE_DP_MASK)
#define SYSCON_STARTER1_BLE_DP0_MASK             (0x40U)
#define SYSCON_STARTER1_BLE_DP0_SHIFT            (6U)
/*! BLE_DP0 - BLE Datapath 0 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_BLE_DP0(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_BLE_DP0_SHIFT)) & SYSCON_STARTER1_BLE_DP0_MASK)
#define SYSCON_STARTER1_BLE_DP1_MASK             (0x80U)
#define SYSCON_STARTER1_BLE_DP1_SHIFT            (7U)
/*! BLE_DP1 - BLE Datapath 1 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_BLE_DP1(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_BLE_DP1_SHIFT)) & SYSCON_STARTER1_BLE_DP1_MASK)
#define SYSCON_STARTER1_BLE_DP2_MASK             (0x100U)
#define SYSCON_STARTER1_BLE_DP2_SHIFT            (8U)
/*! BLE_DP2 - BLE Datapath 2 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_BLE_DP2(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_BLE_DP2_SHIFT)) & SYSCON_STARTER1_BLE_DP2_MASK)
#define SYSCON_STARTER1_BLE_LL_ALL_MASK          (0x200U)
#define SYSCON_STARTER1_BLE_LL_ALL_SHIFT         (9U)
/*! BLE_LL_ALL - BLE Link Layer interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_BLE_LL_ALL(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_BLE_LL_ALL_SHIFT)) & SYSCON_STARTER1_BLE_LL_ALL_MASK)
#define SYSCON_STARTER1_ZIGBEE_MAC_MASK          (0x400U)
#define SYSCON_STARTER1_ZIGBEE_MAC_SHIFT         (10U)
/*! ZIGBEE_MAC - Zigbee MAC interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_ZIGBEE_MAC(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_ZIGBEE_MAC_SHIFT)) & SYSCON_STARTER1_ZIGBEE_MAC_MASK)
#define SYSCON_STARTER1_ZIGBEE_MODEM_MASK        (0x800U)
#define SYSCON_STARTER1_ZIGBEE_MODEM_SHIFT       (11U)
/*! ZIGBEE_MODEM - Zigbee Modem interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_ZIGBEE_MODEM(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_ZIGBEE_MODEM_SHIFT)) & SYSCON_STARTER1_ZIGBEE_MODEM_MASK)
#define SYSCON_STARTER1_RFP_TMU_MASK             (0x1000U)
#define SYSCON_STARTER1_RFP_TMU_SHIFT            (12U)
/*! RFP_TMU - Radio Controller Timing Controller (RFP TMU) interrupt wake-up. 0 = Wake-up disabled.
 *    1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_RFP_TMU(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_RFP_TMU_SHIFT)) & SYSCON_STARTER1_RFP_TMU_MASK)
#define SYSCON_STARTER1_RFP_AGC_MASK             (0x2000U)
#define SYSCON_STARTER1_RFP_AGC_SHIFT            (13U)
/*! RFP_AGC - Radio Control AGC (RFP AGC) interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_RFP_AGC(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_RFP_AGC_SHIFT)) & SYSCON_STARTER1_RFP_AGC_MASK)
#define SYSCON_STARTER1_ISO7816_MASK             (0x4000U)
#define SYSCON_STARTER1_ISO7816_SHIFT            (14U)
/*! ISO7816 - ISO7816 Smart Card interface interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep.
 */
#define SYSCON_STARTER1_ISO7816(x)               (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_ISO7816_SHIFT)) & SYSCON_STARTER1_ISO7816_MASK)
#define SYSCON_STARTER1_ANA_COMP_MASK            (0x8000U)
#define SYSCON_STARTER1_ANA_COMP_SHIFT           (15U)
/*! ANA_COMP - Analog Comparator interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER1_ANA_COMP(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_ANA_COMP_SHIFT)) & SYSCON_STARTER1_ANA_COMP_MASK)
#define SYSCON_STARTER1_WAKE_UP_TIMER0_MASK      (0x10000U)
#define SYSCON_STARTER1_WAKE_UP_TIMER0_SHIFT     (16U)
/*! WAKE_UP_TIMER0 - Wake-up Timer0 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER1_WAKE_UP_TIMER0(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_WAKE_UP_TIMER0_SHIFT)) & SYSCON_STARTER1_WAKE_UP_TIMER0_MASK)
#define SYSCON_STARTER1_WAKE_UP_TIMER1_MASK      (0x20000U)
#define SYSCON_STARTER1_WAKE_UP_TIMER1_SHIFT     (17U)
/*! WAKE_UP_TIMER1 - Wake-up Timer1 interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER1_WAKE_UP_TIMER1(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_WAKE_UP_TIMER1_SHIFT)) & SYSCON_STARTER1_WAKE_UP_TIMER1_MASK)
#define SYSCON_STARTER1_BLE_WAKE_UP_TIMER_MASK   (0x400000U)
#define SYSCON_STARTER1_BLE_WAKE_UP_TIMER_SHIFT  (22U)
/*! BLE_WAKE_UP_TIMER - BLE Wake-up Timer interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Valid from Deep-Sleep and Powerdown.
 */
#define SYSCON_STARTER1_BLE_WAKE_UP_TIMER(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_BLE_WAKE_UP_TIMER_SHIFT)) & SYSCON_STARTER1_BLE_WAKE_UP_TIMER_MASK)
#define SYSCON_STARTER1_BLE_OSC_EN_MASK          (0x800000U)
#define SYSCON_STARTER1_BLE_OSC_EN_SHIFT         (23U)
/*! BLE_OSC_EN - BLE Oscillator Enable interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled.
 *    Valid from Deep-Sleep and Powerdown. Used as early wake-up trigger to allow 32M XTAL to be
 *    started and ready for BLE timeslot
 */
#define SYSCON_STARTER1_BLE_OSC_EN(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_BLE_OSC_EN_SHIFT)) & SYSCON_STARTER1_BLE_OSC_EN_MASK)
#define SYSCON_STARTER1_GPIO_MASK                (0x80000000U)
#define SYSCON_STARTER1_GPIO_SHIFT               (31U)
/*! GPIO - GPIO interrupt wake-up. 0 = Wake-up disabled. 1 = Wake-up enabled. Set this bit to allow
 *    GPIO or NTAG_INT to cause a wake-up in Deep-Sleep and Power-down mode.
 */
#define SYSCON_STARTER1_GPIO(x)                  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTER1_GPIO_SHIFT)) & SYSCON_STARTER1_GPIO_MASK)
/*! @} */

/*! @name STARTERX_STARTERS - Pin assign register */
/*! @{ */
#define SYSCON_STARTERX_STARTERS_DATA_MASK       (0xFFFFFFFFU)
#define SYSCON_STARTERX_STARTERS_DATA_SHIFT      (0U)
#define SYSCON_STARTERX_STARTERS_DATA(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERX_STARTERS_DATA_SHIFT)) & SYSCON_STARTERX_STARTERS_DATA_MASK)
/*! @} */

/* The count of SYSCON_STARTERX_STARTERS */
#define SYSCON_STARTERX_STARTERS_COUNT           (2U)

/*! @name STARTERSET0 - Set bits in STARTER0 */
/*! @{ */
#define SYSCON_STARTERSET0_WDT_BOD_SET_MASK      (0x1U)
#define SYSCON_STARTERSET0_WDT_BOD_SET_SHIFT     (0U)
/*! WDT_BOD_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_WDT_BOD_SET(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_WDT_BOD_SET_SHIFT)) & SYSCON_STARTERSET0_WDT_BOD_SET_MASK)
#define SYSCON_STARTERSET0_DMA_SET_MASK          (0x2U)
#define SYSCON_STARTERSET0_DMA_SET_SHIFT         (1U)
/*! DMA_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_DMA_SET(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_DMA_SET_SHIFT)) & SYSCON_STARTERSET0_DMA_SET_MASK)
#define SYSCON_STARTERSET0_GINT_SET_MASK         (0x4U)
#define SYSCON_STARTERSET0_GINT_SET_SHIFT        (2U)
/*! GINT_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_GINT_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_GINT_SET_SHIFT)) & SYSCON_STARTERSET0_GINT_SET_MASK)
#define SYSCON_STARTERSET0_IRBLASTER_SET_MASK    (0x8U)
#define SYSCON_STARTERSET0_IRBLASTER_SET_SHIFT   (3U)
/*! IRBLASTER_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_IRBLASTER_SET(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_IRBLASTER_SET_SHIFT)) & SYSCON_STARTERSET0_IRBLASTER_SET_MASK)
#define SYSCON_STARTERSET0_PINT0_SET_MASK        (0x10U)
#define SYSCON_STARTERSET0_PINT0_SET_SHIFT       (4U)
/*! PINT0_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PINT0_SET(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PINT0_SET_SHIFT)) & SYSCON_STARTERSET0_PINT0_SET_MASK)
#define SYSCON_STARTERSET0_PINT1_SET_MASK        (0x20U)
#define SYSCON_STARTERSET0_PINT1_SET_SHIFT       (5U)
/*! PINT1_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PINT1_SET(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PINT1_SET_SHIFT)) & SYSCON_STARTERSET0_PINT1_SET_MASK)
#define SYSCON_STARTERSET0_PINT2_SET_MASK        (0x40U)
#define SYSCON_STARTERSET0_PINT2_SET_SHIFT       (6U)
/*! PINT2_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PINT2_SET(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PINT2_SET_SHIFT)) & SYSCON_STARTERSET0_PINT2_SET_MASK)
#define SYSCON_STARTERSET0_PINT3_SET_MASK        (0x80U)
#define SYSCON_STARTERSET0_PINT3_SET_SHIFT       (7U)
/*! PINT3_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PINT3_SET(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PINT3_SET_SHIFT)) & SYSCON_STARTERSET0_PINT3_SET_MASK)
#define SYSCON_STARTERSET0_SPIFI_SET_MASK        (0x100U)
#define SYSCON_STARTERSET0_SPIFI_SET_SHIFT       (8U)
/*! SPIFI_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_SPIFI_SET(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_SPIFI_SET_SHIFT)) & SYSCON_STARTERSET0_SPIFI_SET_MASK)
#define SYSCON_STARTERSET0_TIMER0_SET_MASK       (0x200U)
#define SYSCON_STARTERSET0_TIMER0_SET_SHIFT      (9U)
/*! TIMER0_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_TIMER0_SET(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_TIMER0_SET_SHIFT)) & SYSCON_STARTERSET0_TIMER0_SET_MASK)
#define SYSCON_STARTERSET0_TIMER1_SET_MASK       (0x400U)
#define SYSCON_STARTERSET0_TIMER1_SET_SHIFT      (10U)
/*! TIMER1_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_TIMER1_SET(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_TIMER1_SET_SHIFT)) & SYSCON_STARTERSET0_TIMER1_SET_MASK)
#define SYSCON_STARTERSET0_USART0_SET_MASK       (0x800U)
#define SYSCON_STARTERSET0_USART0_SET_SHIFT      (11U)
/*! USART0_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_USART0_SET(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_USART0_SET_SHIFT)) & SYSCON_STARTERSET0_USART0_SET_MASK)
#define SYSCON_STARTERSET0_USART1_SET_MASK       (0x1000U)
#define SYSCON_STARTERSET0_USART1_SET_SHIFT      (12U)
/*! USART1_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_USART1_SET(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_USART1_SET_SHIFT)) & SYSCON_STARTERSET0_USART1_SET_MASK)
#define SYSCON_STARTERSET0_I2C0_SET_MASK         (0x2000U)
#define SYSCON_STARTERSET0_I2C0_SET_SHIFT        (13U)
/*! I2C0_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_I2C0_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_I2C0_SET_SHIFT)) & SYSCON_STARTERSET0_I2C0_SET_MASK)
#define SYSCON_STARTERSET0_I2C1_SET_MASK         (0x4000U)
#define SYSCON_STARTERSET0_I2C1_SET_SHIFT        (14U)
/*! I2C1_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_I2C1_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_I2C1_SET_SHIFT)) & SYSCON_STARTERSET0_I2C1_SET_MASK)
#define SYSCON_STARTERSET0_SPI0_SET_MASK         (0x8000U)
#define SYSCON_STARTERSET0_SPI0_SET_SHIFT        (15U)
/*! SPI0_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_SPI0_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_SPI0_SET_SHIFT)) & SYSCON_STARTERSET0_SPI0_SET_MASK)
#define SYSCON_STARTERSET0_SPI1_SET_MASK         (0x10000U)
#define SYSCON_STARTERSET0_SPI1_SET_SHIFT        (16U)
/*! SPI1_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_SPI1_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_SPI1_SET_SHIFT)) & SYSCON_STARTERSET0_SPI1_SET_MASK)
#define SYSCON_STARTERSET0_PWM0_SET_MASK         (0x20000U)
#define SYSCON_STARTERSET0_PWM0_SET_SHIFT        (17U)
/*! PWM0_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM0_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM0_SET_SHIFT)) & SYSCON_STARTERSET0_PWM0_SET_MASK)
#define SYSCON_STARTERSET0_PWM1_SET_MASK         (0x40000U)
#define SYSCON_STARTERSET0_PWM1_SET_SHIFT        (18U)
/*! PWM1_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM1_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM1_SET_SHIFT)) & SYSCON_STARTERSET0_PWM1_SET_MASK)
#define SYSCON_STARTERSET0_PWM2_SET_MASK         (0x80000U)
#define SYSCON_STARTERSET0_PWM2_SET_SHIFT        (19U)
/*! PWM2_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM2_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM2_SET_SHIFT)) & SYSCON_STARTERSET0_PWM2_SET_MASK)
#define SYSCON_STARTERSET0_PWM3_SET_MASK         (0x100000U)
#define SYSCON_STARTERSET0_PWM3_SET_SHIFT        (20U)
/*! PWM3_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM3_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM3_SET_SHIFT)) & SYSCON_STARTERSET0_PWM3_SET_MASK)
#define SYSCON_STARTERSET0_PWM4_SET_MASK         (0x200000U)
#define SYSCON_STARTERSET0_PWM4_SET_SHIFT        (21U)
/*! PWM4_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM4_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM4_SET_SHIFT)) & SYSCON_STARTERSET0_PWM4_SET_MASK)
#define SYSCON_STARTERSET0_PWM5_SET_MASK         (0x400000U)
#define SYSCON_STARTERSET0_PWM5_SET_SHIFT        (22U)
/*! PWM5_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM5_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM5_SET_SHIFT)) & SYSCON_STARTERSET0_PWM5_SET_MASK)
#define SYSCON_STARTERSET0_PWM6_SET_MASK         (0x800000U)
#define SYSCON_STARTERSET0_PWM6_SET_SHIFT        (23U)
/*! PWM6_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM6_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM6_SET_SHIFT)) & SYSCON_STARTERSET0_PWM6_SET_MASK)
#define SYSCON_STARTERSET0_PWM7_SET_MASK         (0x1000000U)
#define SYSCON_STARTERSET0_PWM7_SET_SHIFT        (24U)
/*! PWM7_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM7_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM7_SET_SHIFT)) & SYSCON_STARTERSET0_PWM7_SET_MASK)
#define SYSCON_STARTERSET0_PWM8_SET_MASK         (0x2000000U)
#define SYSCON_STARTERSET0_PWM8_SET_SHIFT        (25U)
/*! PWM8_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM8_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM8_SET_SHIFT)) & SYSCON_STARTERSET0_PWM8_SET_MASK)
#define SYSCON_STARTERSET0_PWM9_SET_MASK         (0x4000000U)
#define SYSCON_STARTERSET0_PWM9_SET_SHIFT        (26U)
/*! PWM9_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM9_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM9_SET_SHIFT)) & SYSCON_STARTERSET0_PWM9_SET_MASK)
#define SYSCON_STARTERSET0_PWM10_SET_MASK        (0x8000000U)
#define SYSCON_STARTERSET0_PWM10_SET_SHIFT       (27U)
/*! PWM10_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_PWM10_SET(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_PWM10_SET_SHIFT)) & SYSCON_STARTERSET0_PWM10_SET_MASK)
#define SYSCON_STARTERSET0_I2C2_SET_MASK         (0x10000000U)
#define SYSCON_STARTERSET0_I2C2_SET_SHIFT        (28U)
/*! I2C2_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_I2C2_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_I2C2_SET_SHIFT)) & SYSCON_STARTERSET0_I2C2_SET_MASK)
#define SYSCON_STARTERSET0_RTC_SET_MASK          (0x20000000U)
#define SYSCON_STARTERSET0_RTC_SET_SHIFT         (29U)
/*! RTC_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_RTC_SET(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_RTC_SET_SHIFT)) & SYSCON_STARTERSET0_RTC_SET_MASK)
#define SYSCON_STARTERSET0_NFCTAG_SET_MASK       (0x40000000U)
#define SYSCON_STARTERSET0_NFCTAG_SET_SHIFT      (30U)
/*! NFCTAG_SET - Writing one to this bit sets the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERSET0_NFCTAG_SET(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET0_NFCTAG_SET_SHIFT)) & SYSCON_STARTERSET0_NFCTAG_SET_MASK)
/*! @} */

/*! @name STARTERSET1 - Set bits in STARTER1 */
/*! @{ */
#define SYSCON_STARTERSET1_ADC_SEQA_SET_MASK     (0x1U)
#define SYSCON_STARTERSET1_ADC_SEQA_SET_SHIFT    (0U)
/*! ADC_SEQA_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_ADC_SEQA_SET(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_ADC_SEQA_SET_SHIFT)) & SYSCON_STARTERSET1_ADC_SEQA_SET_MASK)
#define SYSCON_STARTERSET1_ADC_THCMP_OVR_SET_MASK (0x4U)
#define SYSCON_STARTERSET1_ADC_THCMP_OVR_SET_SHIFT (2U)
/*! ADC_THCMP_OVR_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_ADC_THCMP_OVR_SET(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_ADC_THCMP_OVR_SET_SHIFT)) & SYSCON_STARTERSET1_ADC_THCMP_OVR_SET_MASK)
#define SYSCON_STARTERSET1_DMIC_SET_MASK         (0x8U)
#define SYSCON_STARTERSET1_DMIC_SET_SHIFT        (3U)
/*! DMIC_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_DMIC_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_DMIC_SET_SHIFT)) & SYSCON_STARTERSET1_DMIC_SET_MASK)
#define SYSCON_STARTERSET1_HWVAD_SET_MASK        (0x10U)
#define SYSCON_STARTERSET1_HWVAD_SET_SHIFT       (4U)
/*! HWVAD_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_HWVAD_SET(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_HWVAD_SET_SHIFT)) & SYSCON_STARTERSET1_HWVAD_SET_MASK)
#define SYSCON_STARTERSET1_BLE_DP_SET_MASK       (0x20U)
#define SYSCON_STARTERSET1_BLE_DP_SET_SHIFT      (5U)
/*! BLE_DP_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_BLE_DP_SET(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_BLE_DP_SET_SHIFT)) & SYSCON_STARTERSET1_BLE_DP_SET_MASK)
#define SYSCON_STARTERSET1_BLE_DP0_SET_MASK      (0x40U)
#define SYSCON_STARTERSET1_BLE_DP0_SET_SHIFT     (6U)
/*! BLE_DP0_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_BLE_DP0_SET(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_BLE_DP0_SET_SHIFT)) & SYSCON_STARTERSET1_BLE_DP0_SET_MASK)
#define SYSCON_STARTERSET1_BLE_DP1_SET_MASK      (0x80U)
#define SYSCON_STARTERSET1_BLE_DP1_SET_SHIFT     (7U)
/*! BLE_DP1_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_BLE_DP1_SET(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_BLE_DP1_SET_SHIFT)) & SYSCON_STARTERSET1_BLE_DP1_SET_MASK)
#define SYSCON_STARTERSET1_BLE_DP2_SET_MASK      (0x100U)
#define SYSCON_STARTERSET1_BLE_DP2_SET_SHIFT     (8U)
/*! BLE_DP2_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_BLE_DP2_SET(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_BLE_DP2_SET_SHIFT)) & SYSCON_STARTERSET1_BLE_DP2_SET_MASK)
#define SYSCON_STARTERSET1_BLE_LL_ALL_SET_MASK   (0x200U)
#define SYSCON_STARTERSET1_BLE_LL_ALL_SET_SHIFT  (9U)
/*! BLE_LL_ALL_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_BLE_LL_ALL_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_BLE_LL_ALL_SET_SHIFT)) & SYSCON_STARTERSET1_BLE_LL_ALL_SET_MASK)
#define SYSCON_STARTERSET1_ZIGBEE_MAC_SET_MASK   (0x400U)
#define SYSCON_STARTERSET1_ZIGBEE_MAC_SET_SHIFT  (10U)
/*! ZIGBEE_MAC_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_ZIGBEE_MAC_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_ZIGBEE_MAC_SET_SHIFT)) & SYSCON_STARTERSET1_ZIGBEE_MAC_SET_MASK)
#define SYSCON_STARTERSET1_ZIGBEE_MODEM_SET_MASK (0x800U)
#define SYSCON_STARTERSET1_ZIGBEE_MODEM_SET_SHIFT (11U)
/*! ZIGBEE_MODEM_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_ZIGBEE_MODEM_SET(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_ZIGBEE_MODEM_SET_SHIFT)) & SYSCON_STARTERSET1_ZIGBEE_MODEM_SET_MASK)
#define SYSCON_STARTERSET1_RFP_TMU_SET_MASK      (0x1000U)
#define SYSCON_STARTERSET1_RFP_TMU_SET_SHIFT     (12U)
/*! RFP_TMU_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_RFP_TMU_SET(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_RFP_TMU_SET_SHIFT)) & SYSCON_STARTERSET1_RFP_TMU_SET_MASK)
#define SYSCON_STARTERSET1_RFP_AGC_SET_MASK      (0x2000U)
#define SYSCON_STARTERSET1_RFP_AGC_SET_SHIFT     (13U)
/*! RFP_AGC_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_RFP_AGC_SET(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_RFP_AGC_SET_SHIFT)) & SYSCON_STARTERSET1_RFP_AGC_SET_MASK)
#define SYSCON_STARTERSET1_ISO7816_SET_MASK      (0x4000U)
#define SYSCON_STARTERSET1_ISO7816_SET_SHIFT     (14U)
/*! ISO7816_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_ISO7816_SET(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_ISO7816_SET_SHIFT)) & SYSCON_STARTERSET1_ISO7816_SET_MASK)
#define SYSCON_STARTERSET1_ANA_COMP_SET_MASK     (0x8000U)
#define SYSCON_STARTERSET1_ANA_COMP_SET_SHIFT    (15U)
/*! ANA_COMP_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_ANA_COMP_SET(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_ANA_COMP_SET_SHIFT)) & SYSCON_STARTERSET1_ANA_COMP_SET_MASK)
#define SYSCON_STARTERSET1_WAKE_UP_TIMER0_SET_MASK (0x10000U)
#define SYSCON_STARTERSET1_WAKE_UP_TIMER0_SET_SHIFT (16U)
/*! WAKE_UP_TIMER0_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_WAKE_UP_TIMER0_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_WAKE_UP_TIMER0_SET_SHIFT)) & SYSCON_STARTERSET1_WAKE_UP_TIMER0_SET_MASK)
#define SYSCON_STARTERSET1_WAKE_UP_TIMER1_SET_MASK (0x20000U)
#define SYSCON_STARTERSET1_WAKE_UP_TIMER1_SET_SHIFT (17U)
/*! WAKE_UP_TIMER1_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_WAKE_UP_TIMER1_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_WAKE_UP_TIMER1_SET_SHIFT)) & SYSCON_STARTERSET1_WAKE_UP_TIMER1_SET_MASK)
#define SYSCON_STARTERSET1_BLE_WAKE_UP_TIMER_SET_MASK (0x400000U)
#define SYSCON_STARTERSET1_BLE_WAKE_UP_TIMER_SET_SHIFT (22U)
/*! BLE_WAKE_UP_TIMER_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_BLE_WAKE_UP_TIMER_SET(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_BLE_WAKE_UP_TIMER_SET_SHIFT)) & SYSCON_STARTERSET1_BLE_WAKE_UP_TIMER_SET_MASK)
#define SYSCON_STARTERSET1_BLE_OSC_EN_SET_MASK   (0x800000U)
#define SYSCON_STARTERSET1_BLE_OSC_EN_SET_SHIFT  (23U)
/*! BLE_OSC_EN_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_BLE_OSC_EN_SET(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_BLE_OSC_EN_SET_SHIFT)) & SYSCON_STARTERSET1_BLE_OSC_EN_SET_MASK)
#define SYSCON_STARTERSET1_GPIO_SET_MASK         (0x80000000U)
#define SYSCON_STARTERSET1_GPIO_SET_SHIFT        (31U)
/*! GPIO_SET - Writing one to this bit sets the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERSET1_GPIO_SET(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSET1_GPIO_SET_SHIFT)) & SYSCON_STARTERSET1_GPIO_SET_MASK)
/*! @} */

/*! @name STARTERSETX_STARTERSETS - Pin assign register */
/*! @{ */
#define SYSCON_STARTERSETX_STARTERSETS_DATA_MASK (0xFFFFFFFFU)
#define SYSCON_STARTERSETX_STARTERSETS_DATA_SHIFT (0U)
#define SYSCON_STARTERSETX_STARTERSETS_DATA(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERSETX_STARTERSETS_DATA_SHIFT)) & SYSCON_STARTERSETX_STARTERSETS_DATA_MASK)
/*! @} */

/* The count of SYSCON_STARTERSETX_STARTERSETS */
#define SYSCON_STARTERSETX_STARTERSETS_COUNT     (2U)

/*! @name STARTERCLR0 - Clear bits in STARTER0 */
/*! @{ */
#define SYSCON_STARTERCLR0_WDT_BOD_CLR_MASK      (0x1U)
#define SYSCON_STARTERCLR0_WDT_BOD_CLR_SHIFT     (0U)
/*! WDT_BOD_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_WDT_BOD_CLR(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_WDT_BOD_CLR_SHIFT)) & SYSCON_STARTERCLR0_WDT_BOD_CLR_MASK)
#define SYSCON_STARTERCLR0_DMA_CLR_MASK          (0x2U)
#define SYSCON_STARTERCLR0_DMA_CLR_SHIFT         (1U)
/*! DMA_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_DMA_CLR(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_DMA_CLR_SHIFT)) & SYSCON_STARTERCLR0_DMA_CLR_MASK)
#define SYSCON_STARTERCLR0_GINT_CLR_MASK         (0x4U)
#define SYSCON_STARTERCLR0_GINT_CLR_SHIFT        (2U)
/*! GINT_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_GINT_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_GINT_CLR_SHIFT)) & SYSCON_STARTERCLR0_GINT_CLR_MASK)
#define SYSCON_STARTERCLR0_IRBLASTER_CLR_MASK    (0x8U)
#define SYSCON_STARTERCLR0_IRBLASTER_CLR_SHIFT   (3U)
/*! IRBLASTER_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_IRBLASTER_CLR(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_IRBLASTER_CLR_SHIFT)) & SYSCON_STARTERCLR0_IRBLASTER_CLR_MASK)
#define SYSCON_STARTERCLR0_PINT0_CLR_MASK        (0x10U)
#define SYSCON_STARTERCLR0_PINT0_CLR_SHIFT       (4U)
/*! PINT0_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PINT0_CLR(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PINT0_CLR_SHIFT)) & SYSCON_STARTERCLR0_PINT0_CLR_MASK)
#define SYSCON_STARTERCLR0_PINT1_CLR_MASK        (0x20U)
#define SYSCON_STARTERCLR0_PINT1_CLR_SHIFT       (5U)
/*! PINT1_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PINT1_CLR(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PINT1_CLR_SHIFT)) & SYSCON_STARTERCLR0_PINT1_CLR_MASK)
#define SYSCON_STARTERCLR0_PINT2_CLR_MASK        (0x40U)
#define SYSCON_STARTERCLR0_PINT2_CLR_SHIFT       (6U)
/*! PINT2_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PINT2_CLR(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PINT2_CLR_SHIFT)) & SYSCON_STARTERCLR0_PINT2_CLR_MASK)
#define SYSCON_STARTERCLR0_PINT3_CLR_MASK        (0x80U)
#define SYSCON_STARTERCLR0_PINT3_CLR_SHIFT       (7U)
/*! PINT3_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PINT3_CLR(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PINT3_CLR_SHIFT)) & SYSCON_STARTERCLR0_PINT3_CLR_MASK)
#define SYSCON_STARTERCLR0_SPIFI_CLR_MASK        (0x100U)
#define SYSCON_STARTERCLR0_SPIFI_CLR_SHIFT       (8U)
/*! SPIFI_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_SPIFI_CLR(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_SPIFI_CLR_SHIFT)) & SYSCON_STARTERCLR0_SPIFI_CLR_MASK)
#define SYSCON_STARTERCLR0_TIMER0_CLR_MASK       (0x200U)
#define SYSCON_STARTERCLR0_TIMER0_CLR_SHIFT      (9U)
/*! TIMER0_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_TIMER0_CLR(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_TIMER0_CLR_SHIFT)) & SYSCON_STARTERCLR0_TIMER0_CLR_MASK)
#define SYSCON_STARTERCLR0_TIMER1_CLR_MASK       (0x400U)
#define SYSCON_STARTERCLR0_TIMER1_CLR_SHIFT      (10U)
/*! TIMER1_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_TIMER1_CLR(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_TIMER1_CLR_SHIFT)) & SYSCON_STARTERCLR0_TIMER1_CLR_MASK)
#define SYSCON_STARTERCLR0_USART0_CLR_MASK       (0x800U)
#define SYSCON_STARTERCLR0_USART0_CLR_SHIFT      (11U)
/*! USART0_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_USART0_CLR(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_USART0_CLR_SHIFT)) & SYSCON_STARTERCLR0_USART0_CLR_MASK)
#define SYSCON_STARTERCLR0_USART1_CLR_MASK       (0x1000U)
#define SYSCON_STARTERCLR0_USART1_CLR_SHIFT      (12U)
/*! USART1_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_USART1_CLR(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_USART1_CLR_SHIFT)) & SYSCON_STARTERCLR0_USART1_CLR_MASK)
#define SYSCON_STARTERCLR0_I2C0_CLR_MASK         (0x2000U)
#define SYSCON_STARTERCLR0_I2C0_CLR_SHIFT        (13U)
/*! I2C0_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_I2C0_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_I2C0_CLR_SHIFT)) & SYSCON_STARTERCLR0_I2C0_CLR_MASK)
#define SYSCON_STARTERCLR0_I2C1_CLR_MASK         (0x4000U)
#define SYSCON_STARTERCLR0_I2C1_CLR_SHIFT        (14U)
/*! I2C1_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_I2C1_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_I2C1_CLR_SHIFT)) & SYSCON_STARTERCLR0_I2C1_CLR_MASK)
#define SYSCON_STARTERCLR0_SPI0_CLR_MASK         (0x8000U)
#define SYSCON_STARTERCLR0_SPI0_CLR_SHIFT        (15U)
/*! SPI0_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_SPI0_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_SPI0_CLR_SHIFT)) & SYSCON_STARTERCLR0_SPI0_CLR_MASK)
#define SYSCON_STARTERCLR0_SPI1_CLR_MASK         (0x10000U)
#define SYSCON_STARTERCLR0_SPI1_CLR_SHIFT        (16U)
/*! SPI1_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_SPI1_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_SPI1_CLR_SHIFT)) & SYSCON_STARTERCLR0_SPI1_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM0_CLR_MASK         (0x20000U)
#define SYSCON_STARTERCLR0_PWM0_CLR_SHIFT        (17U)
/*! PWM0_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM0_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM0_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM0_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM1_CLR_MASK         (0x40000U)
#define SYSCON_STARTERCLR0_PWM1_CLR_SHIFT        (18U)
/*! PWM1_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM1_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM1_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM1_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM2_CLR_MASK         (0x80000U)
#define SYSCON_STARTERCLR0_PWM2_CLR_SHIFT        (19U)
/*! PWM2_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM2_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM2_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM2_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM3_CLR_MASK         (0x100000U)
#define SYSCON_STARTERCLR0_PWM3_CLR_SHIFT        (20U)
/*! PWM3_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM3_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM3_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM3_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM4_CLR_MASK         (0x200000U)
#define SYSCON_STARTERCLR0_PWM4_CLR_SHIFT        (21U)
/*! PWM4_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM4_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM4_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM4_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM5_CLR_MASK         (0x400000U)
#define SYSCON_STARTERCLR0_PWM5_CLR_SHIFT        (22U)
/*! PWM5_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM5_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM5_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM5_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM6_CLR_MASK         (0x800000U)
#define SYSCON_STARTERCLR0_PWM6_CLR_SHIFT        (23U)
/*! PWM6_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM6_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM6_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM6_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM7_CLR_MASK         (0x1000000U)
#define SYSCON_STARTERCLR0_PWM7_CLR_SHIFT        (24U)
/*! PWM7_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM7_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM7_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM7_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM8_CLR_MASK         (0x2000000U)
#define SYSCON_STARTERCLR0_PWM8_CLR_SHIFT        (25U)
/*! PWM8_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM8_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM8_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM8_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM9_CLR_MASK         (0x4000000U)
#define SYSCON_STARTERCLR0_PWM9_CLR_SHIFT        (26U)
/*! PWM9_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM9_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM9_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM9_CLR_MASK)
#define SYSCON_STARTERCLR0_PWM10_CLR_MASK        (0x8000000U)
#define SYSCON_STARTERCLR0_PWM10_CLR_SHIFT       (27U)
/*! PWM10_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_PWM10_CLR(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_PWM10_CLR_SHIFT)) & SYSCON_STARTERCLR0_PWM10_CLR_MASK)
#define SYSCON_STARTERCLR0_I2C2_CLR_MASK         (0x10000000U)
#define SYSCON_STARTERCLR0_I2C2_CLR_SHIFT        (28U)
/*! I2C2_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_I2C2_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_I2C2_CLR_SHIFT)) & SYSCON_STARTERCLR0_I2C2_CLR_MASK)
#define SYSCON_STARTERCLR0_RTC_CLR_MASK          (0x20000000U)
#define SYSCON_STARTERCLR0_RTC_CLR_SHIFT         (29U)
/*! RTC_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_RTC_CLR(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_RTC_CLR_SHIFT)) & SYSCON_STARTERCLR0_RTC_CLR_MASK)
#define SYSCON_STARTERCLR0_NFCTAG_CLR_MASK       (0x40000000U)
#define SYSCON_STARTERCLR0_NFCTAG_CLR_SHIFT      (30U)
/*! NFCTAG_CLR - Writing one to this bit clears the corresponding bit in the STARTER0 register
 */
#define SYSCON_STARTERCLR0_NFCTAG_CLR(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR0_NFCTAG_CLR_SHIFT)) & SYSCON_STARTERCLR0_NFCTAG_CLR_MASK)
/*! @} */

/*! @name STARTERCLR1 - Clear bits in STARTER1 */
/*! @{ */
#define SYSCON_STARTERCLR1_ADC_SEQA_CLR_MASK     (0x1U)
#define SYSCON_STARTERCLR1_ADC_SEQA_CLR_SHIFT    (0U)
/*! ADC_SEQA_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_ADC_SEQA_CLR(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_ADC_SEQA_CLR_SHIFT)) & SYSCON_STARTERCLR1_ADC_SEQA_CLR_MASK)
#define SYSCON_STARTERCLR1_ADC_THCMP_OVR_CLR_MASK (0x4U)
#define SYSCON_STARTERCLR1_ADC_THCMP_OVR_CLR_SHIFT (2U)
/*! ADC_THCMP_OVR_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_ADC_THCMP_OVR_CLR(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_ADC_THCMP_OVR_CLR_SHIFT)) & SYSCON_STARTERCLR1_ADC_THCMP_OVR_CLR_MASK)
#define SYSCON_STARTERCLR1_DMIC_CLR_MASK         (0x8U)
#define SYSCON_STARTERCLR1_DMIC_CLR_SHIFT        (3U)
/*! DMIC_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_DMIC_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_DMIC_CLR_SHIFT)) & SYSCON_STARTERCLR1_DMIC_CLR_MASK)
#define SYSCON_STARTERCLR1_HWVAD_CLR_MASK        (0x10U)
#define SYSCON_STARTERCLR1_HWVAD_CLR_SHIFT       (4U)
/*! HWVAD_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_HWVAD_CLR(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_HWVAD_CLR_SHIFT)) & SYSCON_STARTERCLR1_HWVAD_CLR_MASK)
#define SYSCON_STARTERCLR1_BLE_DP_CLR_MASK       (0x20U)
#define SYSCON_STARTERCLR1_BLE_DP_CLR_SHIFT      (5U)
/*! BLE_DP_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_BLE_DP_CLR(x)         (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_BLE_DP_CLR_SHIFT)) & SYSCON_STARTERCLR1_BLE_DP_CLR_MASK)
#define SYSCON_STARTERCLR1_BLE_DP0_CLR_MASK      (0x40U)
#define SYSCON_STARTERCLR1_BLE_DP0_CLR_SHIFT     (6U)
/*! BLE_DP0_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_BLE_DP0_CLR(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_BLE_DP0_CLR_SHIFT)) & SYSCON_STARTERCLR1_BLE_DP0_CLR_MASK)
#define SYSCON_STARTERCLR1_BLE_DP1_CLR_MASK      (0x80U)
#define SYSCON_STARTERCLR1_BLE_DP1_CLR_SHIFT     (7U)
/*! BLE_DP1_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_BLE_DP1_CLR(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_BLE_DP1_CLR_SHIFT)) & SYSCON_STARTERCLR1_BLE_DP1_CLR_MASK)
#define SYSCON_STARTERCLR1_BLE_DP2_CLR_MASK      (0x100U)
#define SYSCON_STARTERCLR1_BLE_DP2_CLR_SHIFT     (8U)
/*! BLE_DP2_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_BLE_DP2_CLR(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_BLE_DP2_CLR_SHIFT)) & SYSCON_STARTERCLR1_BLE_DP2_CLR_MASK)
#define SYSCON_STARTERCLR1_BLE_LL_ALL_CLR_MASK   (0x200U)
#define SYSCON_STARTERCLR1_BLE_LL_ALL_CLR_SHIFT  (9U)
/*! BLE_LL_ALL_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_BLE_LL_ALL_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_BLE_LL_ALL_CLR_SHIFT)) & SYSCON_STARTERCLR1_BLE_LL_ALL_CLR_MASK)
#define SYSCON_STARTERCLR1_ZIGBEE_MAC_CLR_MASK   (0x400U)
#define SYSCON_STARTERCLR1_ZIGBEE_MAC_CLR_SHIFT  (10U)
/*! ZIGBEE_MAC_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_ZIGBEE_MAC_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_ZIGBEE_MAC_CLR_SHIFT)) & SYSCON_STARTERCLR1_ZIGBEE_MAC_CLR_MASK)
#define SYSCON_STARTERCLR1_ZIGBEE_MODEM_CLR_MASK (0x800U)
#define SYSCON_STARTERCLR1_ZIGBEE_MODEM_CLR_SHIFT (11U)
/*! ZIGBEE_MODEM_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_ZIGBEE_MODEM_CLR(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_ZIGBEE_MODEM_CLR_SHIFT)) & SYSCON_STARTERCLR1_ZIGBEE_MODEM_CLR_MASK)
#define SYSCON_STARTERCLR1_RFP_TMU_CLR_MASK      (0x1000U)
#define SYSCON_STARTERCLR1_RFP_TMU_CLR_SHIFT     (12U)
/*! RFP_TMU_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_RFP_TMU_CLR(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_RFP_TMU_CLR_SHIFT)) & SYSCON_STARTERCLR1_RFP_TMU_CLR_MASK)
#define SYSCON_STARTERCLR1_RFP_AGC_CLR_MASK      (0x2000U)
#define SYSCON_STARTERCLR1_RFP_AGC_CLR_SHIFT     (13U)
/*! RFP_AGC_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_RFP_AGC_CLR(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_RFP_AGC_CLR_SHIFT)) & SYSCON_STARTERCLR1_RFP_AGC_CLR_MASK)
#define SYSCON_STARTERCLR1_ISO7816_CLR_MASK      (0x4000U)
#define SYSCON_STARTERCLR1_ISO7816_CLR_SHIFT     (14U)
/*! ISO7816_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_ISO7816_CLR(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_ISO7816_CLR_SHIFT)) & SYSCON_STARTERCLR1_ISO7816_CLR_MASK)
#define SYSCON_STARTERCLR1_ANA_COMP_CLR_MASK     (0x8000U)
#define SYSCON_STARTERCLR1_ANA_COMP_CLR_SHIFT    (15U)
/*! ANA_COMP_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_ANA_COMP_CLR(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_ANA_COMP_CLR_SHIFT)) & SYSCON_STARTERCLR1_ANA_COMP_CLR_MASK)
#define SYSCON_STARTERCLR1_WAKE_UP_TIMER0_CLR_MASK (0x10000U)
#define SYSCON_STARTERCLR1_WAKE_UP_TIMER0_CLR_SHIFT (16U)
/*! WAKE_UP_TIMER0_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_WAKE_UP_TIMER0_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_WAKE_UP_TIMER0_CLR_SHIFT)) & SYSCON_STARTERCLR1_WAKE_UP_TIMER0_CLR_MASK)
#define SYSCON_STARTERCLR1_WAKE_UP_TIMER1_CLR_MASK (0x20000U)
#define SYSCON_STARTERCLR1_WAKE_UP_TIMER1_CLR_SHIFT (17U)
/*! WAKE_UP_TIMER1_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_WAKE_UP_TIMER1_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_WAKE_UP_TIMER1_CLR_SHIFT)) & SYSCON_STARTERCLR1_WAKE_UP_TIMER1_CLR_MASK)
#define SYSCON_STARTERCLR1_BLE_WAKE_UP_TIMER_CLR_MASK (0x400000U)
#define SYSCON_STARTERCLR1_BLE_WAKE_UP_TIMER_CLR_SHIFT (22U)
/*! BLE_WAKE_UP_TIMER_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_BLE_WAKE_UP_TIMER_CLR(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_BLE_WAKE_UP_TIMER_CLR_SHIFT)) & SYSCON_STARTERCLR1_BLE_WAKE_UP_TIMER_CLR_MASK)
#define SYSCON_STARTERCLR1_BLE_OSC_EN_CLR_MASK   (0x800000U)
#define SYSCON_STARTERCLR1_BLE_OSC_EN_CLR_SHIFT  (23U)
/*! BLE_OSC_EN_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_BLE_OSC_EN_CLR(x)     (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_BLE_OSC_EN_CLR_SHIFT)) & SYSCON_STARTERCLR1_BLE_OSC_EN_CLR_MASK)
#define SYSCON_STARTERCLR1_GPIO_CLR_MASK         (0x80000000U)
#define SYSCON_STARTERCLR1_GPIO_CLR_SHIFT        (31U)
/*! GPIO_CLR - Writing one to this bit clears the corresponding bit in the STARTER1 register
 */
#define SYSCON_STARTERCLR1_GPIO_CLR(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLR1_GPIO_CLR_SHIFT)) & SYSCON_STARTERCLR1_GPIO_CLR_MASK)
/*! @} */

/*! @name STARTERCLRX_STARTERCLRS - Pin assign register */
/*! @{ */
#define SYSCON_STARTERCLRX_STARTERCLRS_DATA_MASK (0xFFFFFFFFU)
#define SYSCON_STARTERCLRX_STARTERCLRS_DATA_SHIFT (0U)
#define SYSCON_STARTERCLRX_STARTERCLRS_DATA(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_STARTERCLRX_STARTERCLRS_DATA_SHIFT)) & SYSCON_STARTERCLRX_STARTERCLRS_DATA_MASK)
/*! @} */

/* The count of SYSCON_STARTERCLRX_STARTERCLRS */
#define SYSCON_STARTERCLRX_STARTERCLRS_COUNT     (2U)

/*! @name RETENTIONCTRL - I/O retention control register */
/*! @{ */
#define SYSCON_RETENTIONCTRL_IOCLAMP_MASK        (0x1U)
#define SYSCON_RETENTIONCTRL_IOCLAMP_SHIFT       (0U)
/*! IOCLAMP - Global control of activation of I/O clamps to allow IOs to hold a value during a power
 *    mode cycle. To use enable before the power down and then disable after a wake up. Note that
 *    each I/O clamp must also be enabled/disabled individually in IOCON module and also that for an
 *    I2C IO cell it must be in GPIO mode for the clamping to work. 0: I/O clamp is disable 1: I/O
 *    clamp is enable
 */
#define SYSCON_RETENTIONCTRL_IOCLAMP(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_RETENTIONCTRL_IOCLAMP_SHIFT)) & SYSCON_RETENTIONCTRL_IOCLAMP_MASK)
/*! @} */

/*! @name CPSTACK - CPSTACK */
/*! @{ */
#define SYSCON_CPSTACK_CPSTACK_MASK              (0xFFFFFFFFU)
#define SYSCON_CPSTACK_CPSTACK_SHIFT             (0U)
/*! CPSTACK - CPSTACK
 */
#define SYSCON_CPSTACK_CPSTACK(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_CPSTACK_CPSTACK_SHIFT)) & SYSCON_CPSTACK_CPSTACK_MASK)
/*! @} */

/*! @name ANACTRL_CTRL - Analog Interrupt control register. Requires AHBCLKCTRL0.ANA_INT_CTRL to be set. */
/*! @{ */
#define SYSCON_ANACTRL_CTRL_COMPINTRLVL_MASK     (0x1U)
#define SYSCON_ANACTRL_CTRL_COMPINTRLVL_SHIFT    (0U)
/*! COMPINTRLVL - Analog Comparator interrupt type: 0: Analog Comparator interrupt is edge
 *    sensitive. 1: Analog Comparator interrupt is level sensitive.
 */
#define SYSCON_ANACTRL_CTRL_COMPINTRLVL(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_CTRL_COMPINTRLVL_SHIFT)) & SYSCON_ANACTRL_CTRL_COMPINTRLVL_MASK)
#define SYSCON_ANACTRL_CTRL_COMPINTRPOL_MASK     (0x6U)
#define SYSCON_ANACTRL_CTRL_COMPINTRPOL_SHIFT    (1U)
/*! COMPINTRPOL - Analog Comparator interrupt Polarity: When COMPINTRLVL = 0 (edge sensitive): 00:
 *    rising edge. 01: falling edge. 10: both edges (rising and falling). 11: both edges (rising and
 *    falling). When COMPINTRLVL = 1 (level sensitive): 00: Low level ('0'). 01: Low level ('0').
 *    10: High level ('1'). 11: High level ('1').
 */
#define SYSCON_ANACTRL_CTRL_COMPINTRPOL(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_CTRL_COMPINTRPOL_SHIFT)) & SYSCON_ANACTRL_CTRL_COMPINTRPOL_MASK)
/*! @} */

/*! @name ANACTRL_VAL - Analog modules (BOD and Analog Comparator) outputs current values (BOD 'Power OK' and Analog comparator out). Requires AHBCLKCTRL0.ANA_INT_CTRL to be set. */
/*! @{ */
#define SYSCON_ANACTRL_VAL_BODVBAT_MASK          (0x1U)
#define SYSCON_ANACTRL_VAL_BODVBAT_SHIFT         (0U)
/*! BODVBAT - BOD VBAT Status : 0 = Power not OK ; 1 = Power OK
 */
#define SYSCON_ANACTRL_VAL_BODVBAT(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_VAL_BODVBAT_SHIFT)) & SYSCON_ANACTRL_VAL_BODVBAT_MASK)
#define SYSCON_ANACTRL_VAL_ANACOMP_MASK          (0x8U)
#define SYSCON_ANACTRL_VAL_ANACOMP_SHIFT         (3U)
/*! ANACOMP - Analog comparator Status : 0 = Comparator in 0 < in 1 ; 1 = Comparator in 0 > in 1.
 */
#define SYSCON_ANACTRL_VAL_ANACOMP(x)            (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_VAL_ANACOMP_SHIFT)) & SYSCON_ANACTRL_VAL_ANACOMP_MASK)
#define SYSCON_ANACTRL_VAL_BODVBATHIGH_MASK      (0x10U)
#define SYSCON_ANACTRL_VAL_BODVBATHIGH_SHIFT     (4U)
/*! BODVBATHIGH - Not(BOD VBAT). Inverse of BOD VBAT. 0 = Power OK; 1 = Power not OK
 */
#define SYSCON_ANACTRL_VAL_BODVBATHIGH(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_VAL_BODVBATHIGH_SHIFT)) & SYSCON_ANACTRL_VAL_BODVBATHIGH_MASK)
/*! @} */

/*! @name ANACTRL_STAT - Analog modules (BOD and Analog Comparator) interrupt status. Requires AHBCLKCTRL0.ANA_INT_CTRL to be set. */
/*! @{ */
#define SYSCON_ANACTRL_STAT_BODVBAT_MASK         (0x1U)
#define SYSCON_ANACTRL_STAT_BODVBAT_SHIFT        (0U)
/*! BODVBAT - BOD VBAT Interrupt status. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear.
 */
#define SYSCON_ANACTRL_STAT_BODVBAT(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_STAT_BODVBAT_SHIFT)) & SYSCON_ANACTRL_STAT_BODVBAT_MASK)
#define SYSCON_ANACTRL_STAT_ANACOMP_MASK         (0x8U)
#define SYSCON_ANACTRL_STAT_ANACOMP_SHIFT        (3U)
/*! ANACOMP - Analog comparator Interrupt status. 0: No interrupt pending. 1: Interrupt pending. Write 1 to clear.
 */
#define SYSCON_ANACTRL_STAT_ANACOMP(x)           (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_STAT_ANACOMP_SHIFT)) & SYSCON_ANACTRL_STAT_ANACOMP_MASK)
#define SYSCON_ANACTRL_STAT_BODVBATHIGH_MASK     (0x10U)
#define SYSCON_ANACTRL_STAT_BODVBATHIGH_SHIFT    (4U)
/*! BODVBATHIGH - NOT(BOD VBAT) interrupt status. Will be set when BOD VBAT goes high. Write 1 to clear.
 */
#define SYSCON_ANACTRL_STAT_BODVBATHIGH(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_STAT_BODVBATHIGH_SHIFT)) & SYSCON_ANACTRL_STAT_BODVBATHIGH_MASK)
/*! @} */

/*! @name ANACTRL_INTENSET - Analog modules (BOD and Analog Comparator) Interrupt Enable Read and Set register. Read value indicates which interrupts are enabled. Writing ones sets the corresponding interrupt enable bits. Note, interrupt enable bits are cleared using ANACTRL_INTENCLR. Requires AHBCLKCTRL0.ANA_INT_CTRL to be set to use this register. */
/*! @{ */
#define SYSCON_ANACTRL_INTENSET_BODVBAT_MASK     (0x1U)
#define SYSCON_ANACTRL_INTENSET_BODVBAT_SHIFT    (0U)
/*! BODVBAT - BOD VBAT Interrupt Enable Read and Set register
 */
#define SYSCON_ANACTRL_INTENSET_BODVBAT(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTENSET_BODVBAT_SHIFT)) & SYSCON_ANACTRL_INTENSET_BODVBAT_MASK)
#define SYSCON_ANACTRL_INTENSET_ANACOMP_MASK     (0x8U)
#define SYSCON_ANACTRL_INTENSET_ANACOMP_SHIFT    (3U)
/*! ANACOMP - Analog comparator Interrupt Enable Read and Set register
 */
#define SYSCON_ANACTRL_INTENSET_ANACOMP(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTENSET_ANACOMP_SHIFT)) & SYSCON_ANACTRL_INTENSET_ANACOMP_MASK)
#define SYSCON_ANACTRL_INTENSET_BODVBATHIGH_MASK (0x10U)
#define SYSCON_ANACTRL_INTENSET_BODVBATHIGH_SHIFT (4U)
/*! BODVBATHIGH - NOT(BOD VBAT) Interrupt Enable Read and Set register
 */
#define SYSCON_ANACTRL_INTENSET_BODVBATHIGH(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTENSET_BODVBATHIGH_SHIFT)) & SYSCON_ANACTRL_INTENSET_BODVBATHIGH_MASK)
/*! @} */

/*! @name ANACTRL_INTENCLR - Analog modules (BOD and Analog Comparator) Interrupt Enable Clear register. Writing ones clears the corresponding interrupt enable bits. Note, interrupt enable bits are set in ANACTRL_INTENSET. Requires AHBCLKCTRL0.ANA_INT_CTRL to be set to use this register. */
/*! @{ */
#define SYSCON_ANACTRL_INTENCLR_BODVBAT_MASK     (0x1U)
#define SYSCON_ANACTRL_INTENCLR_BODVBAT_SHIFT    (0U)
/*! BODVBAT - BOD VBAT Interrupt Enable Clear register
 */
#define SYSCON_ANACTRL_INTENCLR_BODVBAT(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTENCLR_BODVBAT_SHIFT)) & SYSCON_ANACTRL_INTENCLR_BODVBAT_MASK)
#define SYSCON_ANACTRL_INTENCLR_ANACOMP_MASK     (0x8U)
#define SYSCON_ANACTRL_INTENCLR_ANACOMP_SHIFT    (3U)
/*! ANACOMP - Analog comparator Interrupt Enable Clear register
 */
#define SYSCON_ANACTRL_INTENCLR_ANACOMP(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTENCLR_ANACOMP_SHIFT)) & SYSCON_ANACTRL_INTENCLR_ANACOMP_MASK)
#define SYSCON_ANACTRL_INTENCLR_BODVBATHIGH_MASK (0x10U)
#define SYSCON_ANACTRL_INTENCLR_BODVBATHIGH_SHIFT (4U)
/*! BODVBATHIGH - NOT(BOD VBAT) Interrupt Enable Clear register
 */
#define SYSCON_ANACTRL_INTENCLR_BODVBATHIGH(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTENCLR_BODVBATHIGH_SHIFT)) & SYSCON_ANACTRL_INTENCLR_BODVBATHIGH_MASK)
/*! @} */

/*! @name ANACTRL_INTSTAT - Analog modules (BOD and Analog Comparator) Interrupt Status register (masked with interrupt enable). Requires AHBCLKCTRL0.ANA_INT_CTRL to be set to use this register. Interrupt status bit are cleared using ANACTRL_STAT. */
/*! @{ */
#define SYSCON_ANACTRL_INTSTAT_BODVBAT_MASK      (0x1U)
#define SYSCON_ANACTRL_INTSTAT_BODVBAT_SHIFT     (0U)
/*! BODVBAT - BOD VBAT Interrupt (after interrupt mask). 0 = No interrupt pending. 1 = Interrupt
 *    pending. Only set when BODVBAT is enabled in INTENSET
 */
#define SYSCON_ANACTRL_INTSTAT_BODVBAT(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTSTAT_BODVBAT_SHIFT)) & SYSCON_ANACTRL_INTSTAT_BODVBAT_MASK)
#define SYSCON_ANACTRL_INTSTAT_ANACOMP_MASK      (0x8U)
#define SYSCON_ANACTRL_INTSTAT_ANACOMP_SHIFT     (3U)
/*! ANACOMP - Analog comparator Interrupt (after interrupt mask). 0 = No interrupt pending. 1 =
 *    Interrupt pending. Only set when ANACOMP is enabled in INTENSET
 */
#define SYSCON_ANACTRL_INTSTAT_ANACOMP(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTSTAT_ANACOMP_SHIFT)) & SYSCON_ANACTRL_INTSTAT_ANACOMP_MASK)
#define SYSCON_ANACTRL_INTSTAT_BODVBATHIGH_MASK  (0x10U)
#define SYSCON_ANACTRL_INTSTAT_BODVBATHIGH_SHIFT (4U)
/*! BODVBATHIGH - NOT(BOD VBAT) Interrupt (after interrupt mask). 0 = No interrupt pending. 1 =
 *    Interrupt pending. Only set when BODVBATHIGH is enabled in INTENSET
 */
#define SYSCON_ANACTRL_INTSTAT_BODVBATHIGH(x)    (((uint32_t)(((uint32_t)(x)) << SYSCON_ANACTRL_INTSTAT_BODVBATHIGH_SHIFT)) & SYSCON_ANACTRL_INTSTAT_BODVBATHIGH_MASK)
/*! @} */

/*! @name CLOCK_CTRL - Various system clock controls : Flash clock (48 MHz) control, clocks to Frequency Measure function */
/*! @{ */
#define SYSCON_CLOCK_CTRL_FLASH48MHZ_ENA_MASK    (0x1U)
#define SYSCON_CLOCK_CTRL_FLASH48MHZ_ENA_SHIFT   (0U)
/*! FLASH48MHZ_ENA - Enable Flash 48 MHz clock. 0 = Disabled. 1 = Enabled.
 */
#define SYSCON_CLOCK_CTRL_FLASH48MHZ_ENA(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_CLOCK_CTRL_FLASH48MHZ_ENA_SHIFT)) & SYSCON_CLOCK_CTRL_FLASH48MHZ_ENA_MASK)
#define SYSCON_CLOCK_CTRL_XTAL32MHZ_FREQM_ENA_MASK (0x2U)
#define SYSCON_CLOCK_CTRL_XTAL32MHZ_FREQM_ENA_SHIFT (1U)
/*! XTAL32MHZ_FREQM_ENA - Enable XTAL32MHz clock for Frequency Measure module. 0 = Disabled. 1 = Enabled.
 */
#define SYSCON_CLOCK_CTRL_XTAL32MHZ_FREQM_ENA(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_CLOCK_CTRL_XTAL32MHZ_FREQM_ENA_SHIFT)) & SYSCON_CLOCK_CTRL_XTAL32MHZ_FREQM_ENA_MASK)
#define SYSCON_CLOCK_CTRL_FRO1MHZ_FREQM_ENA_MASK (0x4U)
#define SYSCON_CLOCK_CTRL_FRO1MHZ_FREQM_ENA_SHIFT (2U)
/*! FRO1MHZ_FREQM_ENA - Enable FRO 1MHz clock for Frequency Measure module. 0 = Disabled. 1 = Enabled.
 */
#define SYSCON_CLOCK_CTRL_FRO1MHZ_FREQM_ENA(x)   (((uint32_t)(((uint32_t)(x)) << SYSCON_CLOCK_CTRL_FRO1MHZ_FREQM_ENA_SHIFT)) & SYSCON_CLOCK_CTRL_FRO1MHZ_FREQM_ENA_MASK)
/*! @} */

/*! @name WKT_CTRL - Wake-up timers control */
/*! @{ */
#define SYSCON_WKT_CTRL_WKT0_ENA_MASK            (0x1U)
#define SYSCON_WKT_CTRL_WKT0_ENA_SHIFT           (0U)
/*! WKT0_ENA - Enable wake-up timer 0: 0 = Disabled. 1 = Enabled (counter is running).
 */
#define SYSCON_WKT_CTRL_WKT0_ENA(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_CTRL_WKT0_ENA_SHIFT)) & SYSCON_WKT_CTRL_WKT0_ENA_MASK)
#define SYSCON_WKT_CTRL_WKT1_ENA_MASK            (0x2U)
#define SYSCON_WKT_CTRL_WKT1_ENA_SHIFT           (1U)
/*! WKT1_ENA - Enable wake-up timer 1: 0 = Disabled. 1 = Enabled (counter is running).
 */
#define SYSCON_WKT_CTRL_WKT1_ENA(x)              (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_CTRL_WKT1_ENA_SHIFT)) & SYSCON_WKT_CTRL_WKT1_ENA_MASK)
#define SYSCON_WKT_CTRL_WKT0_CLK_ENA_MASK        (0x4U)
#define SYSCON_WKT_CTRL_WKT0_CLK_ENA_SHIFT       (2U)
/*! WKT0_CLK_ENA - Enable wake-up timer 0 clock: 0 = Disabled. 1 = Enabled.
 */
#define SYSCON_WKT_CTRL_WKT0_CLK_ENA(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_CTRL_WKT0_CLK_ENA_SHIFT)) & SYSCON_WKT_CTRL_WKT0_CLK_ENA_MASK)
#define SYSCON_WKT_CTRL_WKT1_CLK_ENA_MASK        (0x8U)
#define SYSCON_WKT_CTRL_WKT1_CLK_ENA_SHIFT       (3U)
/*! WKT1_CLK_ENA - Enable wake-up timer 1 clock: 0 = Disabled. 1 = Enabled.
 */
#define SYSCON_WKT_CTRL_WKT1_CLK_ENA(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_CTRL_WKT1_CLK_ENA_SHIFT)) & SYSCON_WKT_CTRL_WKT1_CLK_ENA_MASK)
/*! @} */

/*! @name WKT_LOAD_WKT0_LSB - Wake-up timer 0 reload value least significant bits ([31:0]). */
/*! @{ */
#define SYSCON_WKT_LOAD_WKT0_LSB_WKT0_LOAD_LSB_MASK (0xFFFFFFFFU)
#define SYSCON_WKT_LOAD_WKT0_LSB_WKT0_LOAD_LSB_SHIFT (0U)
/*! WKT0_LOAD_LSB - Wake-up timer 0 reload value, least significant bits ([31:0]). Write when timer is not enabled
 */
#define SYSCON_WKT_LOAD_WKT0_LSB_WKT0_LOAD_LSB(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_LOAD_WKT0_LSB_WKT0_LOAD_LSB_SHIFT)) & SYSCON_WKT_LOAD_WKT0_LSB_WKT0_LOAD_LSB_MASK)
/*! @} */

/*! @name WKT_LOAD_WKT0_MSB - Wake-up timer 0 reload value most significant bits ([8:0]). */
/*! @{ */
#define SYSCON_WKT_LOAD_WKT0_MSB_WKT0_LOAD_MSB_MASK (0x1FFU)
#define SYSCON_WKT_LOAD_WKT0_MSB_WKT0_LOAD_MSB_SHIFT (0U)
/*! WKT0_LOAD_MSB - Wake-up timer 0 reload value, most significant bits ([8:0]). Write when timer is not enabled
 */
#define SYSCON_WKT_LOAD_WKT0_MSB_WKT0_LOAD_MSB(x) (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_LOAD_WKT0_MSB_WKT0_LOAD_MSB_SHIFT)) & SYSCON_WKT_LOAD_WKT0_MSB_WKT0_LOAD_MSB_MASK)
/*! @} */

/*! @name WKT_LOAD_WKT1 - Wake-up timer 1 reload value. */
/*! @{ */
#define SYSCON_WKT_LOAD_WKT1_WKT1_LOAD_MASK      (0xFFFFFFFU)
#define SYSCON_WKT_LOAD_WKT1_WKT1_LOAD_SHIFT     (0U)
/*! WKT1_LOAD - Wake-up timer 1 reload value. Write when timer is not enabled
 */
#define SYSCON_WKT_LOAD_WKT1_WKT1_LOAD(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_LOAD_WKT1_WKT1_LOAD_SHIFT)) & SYSCON_WKT_LOAD_WKT1_WKT1_LOAD_MASK)
/*! @} */

/*! @name WKT_VAL_WKT0_LSB - Wake-up timer 0 current value least significant bits ([31:0]). WARNING : reading not reliable: read this register several times until you get a stable value. */
/*! @{ */
#define SYSCON_WKT_VAL_WKT0_LSB_WKT0_VAL_LSB_MASK (0xFFFFFFFFU)
#define SYSCON_WKT_VAL_WKT0_LSB_WKT0_VAL_LSB_SHIFT (0U)
/*! WKT0_VAL_LSB - Wake-up timer 0 value, least significant bits ([31:0]). Reread until stable value seen.
 */
#define SYSCON_WKT_VAL_WKT0_LSB_WKT0_VAL_LSB(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_VAL_WKT0_LSB_WKT0_VAL_LSB_SHIFT)) & SYSCON_WKT_VAL_WKT0_LSB_WKT0_VAL_LSB_MASK)
/*! @} */

/*! @name WKT_VAL_WKT0_MSB - Wake-up timer 0 current value most significant bits ([8:0]). WARNING : reading not reliable: read this register several times until you get a stable value. */
/*! @{ */
#define SYSCON_WKT_VAL_WKT0_MSB_WKT0_VAL_MSB_MASK (0x1FFU)
#define SYSCON_WKT_VAL_WKT0_MSB_WKT0_VAL_MSB_SHIFT (0U)
/*! WKT0_VAL_MSB - Wake-up timer 0 value, most significant bits ([8:0]). Reread until stable value seen.
 */
#define SYSCON_WKT_VAL_WKT0_MSB_WKT0_VAL_MSB(x)  (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_VAL_WKT0_MSB_WKT0_VAL_MSB_SHIFT)) & SYSCON_WKT_VAL_WKT0_MSB_WKT0_VAL_MSB_MASK)
/*! @} */

/*! @name WKT_VAL_WKT1 - Wake-up timer 1 current value. WARNING : reading not reliable: read this register several times until you get a stable value. */
/*! @{ */
#define SYSCON_WKT_VAL_WKT1_WKT1_VAL_MASK        (0xFFFFFFFU)
#define SYSCON_WKT_VAL_WKT1_WKT1_VAL_SHIFT       (0U)
/*! WKT1_VAL - Wake-up timer 1 value. Reread until stable value seen.
 */
#define SYSCON_WKT_VAL_WKT1_WKT1_VAL(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_VAL_WKT1_WKT1_VAL_SHIFT)) & SYSCON_WKT_VAL_WKT1_WKT1_VAL_MASK)
/*! @} */

/*! @name WKT_STAT - Wake-up timers status */
/*! @{ */
#define SYSCON_WKT_STAT_WKT0_TIMEOUT_MASK        (0x1U)
#define SYSCON_WKT_STAT_WKT0_TIMEOUT_SHIFT       (0U)
/*! WKT0_TIMEOUT - Timeout Status of Wake-up timer 0 : 0 = timeout not reached ; 1 = timeout reached. Write 1 to clear.
 */
#define SYSCON_WKT_STAT_WKT0_TIMEOUT(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_STAT_WKT0_TIMEOUT_SHIFT)) & SYSCON_WKT_STAT_WKT0_TIMEOUT_MASK)
#define SYSCON_WKT_STAT_WKT1_TIMEOUT_MASK        (0x2U)
#define SYSCON_WKT_STAT_WKT1_TIMEOUT_SHIFT       (1U)
/*! WKT1_TIMEOUT - Timeout Status of Wake-up timer 1 : 0 = timeout not reached ; 1 = timeout reached. Write 1 to clear.
 */
#define SYSCON_WKT_STAT_WKT1_TIMEOUT(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_STAT_WKT1_TIMEOUT_SHIFT)) & SYSCON_WKT_STAT_WKT1_TIMEOUT_MASK)
#define SYSCON_WKT_STAT_WKT0_RUNNING_MASK        (0x4U)
#define SYSCON_WKT_STAT_WKT0_RUNNING_SHIFT       (2U)
/*! WKT0_RUNNING - Running Status of Wake-up timer 0 : 0 = not running ; 1 = running
 */
#define SYSCON_WKT_STAT_WKT0_RUNNING(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_STAT_WKT0_RUNNING_SHIFT)) & SYSCON_WKT_STAT_WKT0_RUNNING_MASK)
#define SYSCON_WKT_STAT_WKT1_RUNNING_MASK        (0x8U)
#define SYSCON_WKT_STAT_WKT1_RUNNING_SHIFT       (3U)
/*! WKT1_RUNNING - Running Status of Wake-up timer 1 : 0 = not running ; 1 = running
 */
#define SYSCON_WKT_STAT_WKT1_RUNNING(x)          (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_STAT_WKT1_RUNNING_SHIFT)) & SYSCON_WKT_STAT_WKT1_RUNNING_MASK)
/*! @} */

/*! @name WKT_INTENSET - Interrupt Enable Read and Set register */
/*! @{ */
#define SYSCON_WKT_INTENSET_WKT0_TIMEOUT_MASK    (0x1U)
#define SYSCON_WKT_INTENSET_WKT0_TIMEOUT_SHIFT   (0U)
/*! WKT0_TIMEOUT - Wake-up Timer 0 Timeout Interrupt Enable Read and Set register. Read value of '1'
 *    indicates that the interrupt is enabled. Set this bit to enable the interrupt. Use
 *    WKT_INTENCLR to disable the interrupt.
 */
#define SYSCON_WKT_INTENSET_WKT0_TIMEOUT(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_INTENSET_WKT0_TIMEOUT_SHIFT)) & SYSCON_WKT_INTENSET_WKT0_TIMEOUT_MASK)
#define SYSCON_WKT_INTENSET_WKT1_TIMEOUT_MASK    (0x2U)
#define SYSCON_WKT_INTENSET_WKT1_TIMEOUT_SHIFT   (1U)
/*! WKT1_TIMEOUT - Wake-up Timer 1 Timeout Interrupt Enable Read and Set register. Read value of '1'
 *    indicates that the interrupt is enabled. Set this bit to enable the interrupt. Use
 *    WKT_INTENCLR to disable the interrupt.
 */
#define SYSCON_WKT_INTENSET_WKT1_TIMEOUT(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_INTENSET_WKT1_TIMEOUT_SHIFT)) & SYSCON_WKT_INTENSET_WKT1_TIMEOUT_MASK)
/*! @} */

/*! @name WKT_INTENCLR - Interrupt Enable Clear register */
/*! @{ */
#define SYSCON_WKT_INTENCLR_WKT0_TIMEOUT_MASK    (0x1U)
#define SYSCON_WKT_INTENCLR_WKT0_TIMEOUT_SHIFT   (0U)
/*! WKT0_TIMEOUT - Wake-up Timer 0 Timeout Interrupt Enable Clear register. Set this bit to disable the interrupt.
 */
#define SYSCON_WKT_INTENCLR_WKT0_TIMEOUT(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_INTENCLR_WKT0_TIMEOUT_SHIFT)) & SYSCON_WKT_INTENCLR_WKT0_TIMEOUT_MASK)
#define SYSCON_WKT_INTENCLR_WKT1_TIMEOUT_MASK    (0x2U)
#define SYSCON_WKT_INTENCLR_WKT1_TIMEOUT_SHIFT   (1U)
/*! WKT1_TIMEOUT - Wake-up Timer 1 Timeout Interrupt Enable Clear register. Set this bit to disable the interrupt.
 */
#define SYSCON_WKT_INTENCLR_WKT1_TIMEOUT(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_INTENCLR_WKT1_TIMEOUT_SHIFT)) & SYSCON_WKT_INTENCLR_WKT1_TIMEOUT_MASK)
/*! @} */

/*! @name WKT_INTSTAT - Interrupt Status register */
/*! @{ */
#define SYSCON_WKT_INTSTAT_WKT0_TIMEOUT_MASK     (0x1U)
#define SYSCON_WKT_INTSTAT_WKT0_TIMEOUT_SHIFT    (0U)
/*! WKT0_TIMEOUT - Wake-up Timer 0 Timeout Interrupt. 0: No interrupt pending. 1: Interrupt pending.
 *    Only set when WKT0_TIMEOUT is enable in INTENSET
 */
#define SYSCON_WKT_INTSTAT_WKT0_TIMEOUT(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_INTSTAT_WKT0_TIMEOUT_SHIFT)) & SYSCON_WKT_INTSTAT_WKT0_TIMEOUT_MASK)
#define SYSCON_WKT_INTSTAT_WKT1_TIMEOUT_MASK     (0x2U)
#define SYSCON_WKT_INTSTAT_WKT1_TIMEOUT_SHIFT    (1U)
/*! WKT1_TIMEOUT - Wake-up Timer 1 Timeout Interrupt. 0: No interrupt pending. 1: Interrupt pending.
 *    Only set when WKT1_TIMEOUT is enable in INTENSET
 */
#define SYSCON_WKT_INTSTAT_WKT1_TIMEOUT(x)       (((uint32_t)(((uint32_t)(x)) << SYSCON_WKT_INTSTAT_WKT1_TIMEOUT_SHIFT)) & SYSCON_WKT_INTSTAT_WKT1_TIMEOUT_MASK)
/*! @} */

/*! @name GPIOPSYNC - Enable bypass of the first stage of synchonization inside GPIO_INT module. */
/*! @{ */
#define SYSCON_GPIOPSYNC_PSYNC_MASK              (0x1U)
#define SYSCON_GPIOPSYNC_PSYNC_SHIFT             (0U)
/*! PSYNC - Enable bypass of the first stage of synchonization inside GPIO_INT module.
 */
#define SYSCON_GPIOPSYNC_PSYNC(x)                (((uint32_t)(((uint32_t)(x)) << SYSCON_GPIOPSYNC_PSYNC_SHIFT)) & SYSCON_GPIOPSYNC_PSYNC_MASK)
/*! @} */

/*! @name DIEID - Chip revision ID & Number */
/*! @{ */
#define SYSCON_DIEID_REV_ID_MASK                 (0xFU)
#define SYSCON_DIEID_REV_ID_SHIFT                (0U)
/*! REV_ID - Chip Revision ID
 */
#define SYSCON_DIEID_REV_ID(x)                   (((uint32_t)(((uint32_t)(x)) << SYSCON_DIEID_REV_ID_SHIFT)) & SYSCON_DIEID_REV_ID_MASK)
#define SYSCON_DIEID_MCO_NUM_IN_DIE_ID_MASK      (0xFFFFF0U)
#define SYSCON_DIEID_MCO_NUM_IN_DIE_ID_SHIFT     (4U)
/*! MCO_NUM_IN_DIE_ID - Chip Number
 */
#define SYSCON_DIEID_MCO_NUM_IN_DIE_ID(x)        (((uint32_t)(((uint32_t)(x)) << SYSCON_DIEID_MCO_NUM_IN_DIE_ID_SHIFT)) & SYSCON_DIEID_MCO_NUM_IN_DIE_ID_MASK)
/*! @} */

/*! @name CODESECURITYPROT - Security code to allow test access via SWD/JTAG. Reset with POR, SW reset or BOD */
/*! @{ */
#define SYSCON_CODESECURITYPROT_SEC_CODE_MASK    (0xFFFFFFFFU)
#define SYSCON_CODESECURITYPROT_SEC_CODE_SHIFT   (0U)
/*! SEC_CODE - Security code to allow debug via SWD and JTAG access in test mode. Write once
 *    register, value 0x87654321 disables the access and therefore prevents any chance to enable it.
 *    Writing any other value enables the access and locks the mode. In some cases the boot code will
 *    secure the device by writing to this register to disable SWD and JTAG. This would prevent the
 *    application being able to re-enable this access.
 */
#define SYSCON_CODESECURITYPROT_SEC_CODE(x)      (((uint32_t)(((uint32_t)(x)) << SYSCON_CODESECURITYPROT_SEC_CODE_SHIFT)) & SYSCON_CODESECURITYPROT_SEC_CODE_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group SYSCON_Register_Masks */


/* SYSCON - Peripheral instance base addresses */
/** Peripheral SYSCON base address */
#define SYSCON_BASE                              (0x40000000u)
/** Peripheral SYSCON base pointer */
#define SYSCON                                   ((SYSCON_Type *)SYSCON_BASE)
/** Array initializer of SYSCON peripheral base addresses */
#define SYSCON_BASE_ADDRS                        { SYSCON_BASE }
/** Array initializer of SYSCON peripheral base pointers */
#define SYSCON_BASE_PTRS                         { SYSCON }

/*!
 * @}
 */ /* end of group SYSCON_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- USART Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup USART_Peripheral_Access_Layer USART Peripheral Access Layer
 * @{
 */

/** USART - Register Layout Typedef */
typedef struct {
  __IO uint32_t CFG;                               /**< USART Configuration register. Basic USART configuration settings that typically are not changed during operation., offset: 0x0 */
  __IO uint32_t CTL;                               /**< USART Control register. USART control settings that are more likely to change during operation., offset: 0x4 */
  __IO uint32_t STAT;                              /**< USART Status register. The complete status value can be read here. Writing ones clears some bits in the register. Some bits can be cleared by writing a 1 to them., offset: 0x8 */
  __IO uint32_t INTENSET;                          /**< Interrupt Enable read and Set register for USART (not FIFO) status. Contains individual interrupt enable bits for each potential USART interrupt. A complete value may be read from this register. Writing a 1 to any implemented bit position causes that bit to be set., offset: 0xC */
  __O  uint32_t INTENCLR;                          /**< Interrupt Enable Clear register. Allows clearing any combination of bits in the INTENSET register. Writing a 1 to any implemented bit position causes the corresponding bit to be cleared., offset: 0x10 */
       uint8_t RESERVED_0[12];
  __IO uint32_t BRG;                               /**< Baud Rate Generator register. 16-bit integer baud rate divisor value., offset: 0x20 */
  __I  uint32_t INTSTAT;                           /**< Interrupt status register. Reflects the status of interrupts that are currently enabled., offset: 0x24 */
  __IO uint32_t OSR;                               /**< Oversample selection register for asynchronous communication., offset: 0x28 */
  __IO uint32_t ADDR;                              /**< Address register for automatic address matching., offset: 0x2C */
       uint8_t RESERVED_1[3536];
  __IO uint32_t FIFOCFG;                           /**< FIFO configuration and enable register., offset: 0xE00 */
  __IO uint32_t FIFOSTAT;                          /**< FIFO status register., offset: 0xE04 */
  __IO uint32_t FIFOTRIG;                          /**< FIFO trigger settings for interrupt and DMA request., offset: 0xE08 */
       uint8_t RESERVED_2[4];
  __IO uint32_t FIFOINTENSET;                      /**< FIFO interrupt enable set (enable) and read register., offset: 0xE10 */
  __O  uint32_t FIFOINTENCLR;                      /**< FIFO interrupt enable clear (disable) and read register., offset: 0xE14 */
  __I  uint32_t FIFOINTSTAT;                       /**< FIFO interrupt status register. Reflects the status of interrupts that are currently enabled., offset: 0xE18 */
       uint8_t RESERVED_3[4];
  __O  uint32_t FIFOWR;                            /**< FIFO write data. Used to write values to be transmitted to the FIFO., offset: 0xE20 */
       uint8_t RESERVED_4[12];
  __I  uint32_t FIFORD;                            /**< FIFO read data. Used to read values that have been received., offset: 0xE30 */
       uint8_t RESERVED_5[12];
  __I  uint32_t FIFORDNOPOP;                       /**< FIFO data read with no FIFO pop. This register acts in exactly the same way as FIFORD, except that it supplies data from the top of the FIFO without popping the FIFO (i.e. leaving the FIFO state unchanged). This could be used to allow system software to observe incoming data without interfering with the peripheral driver., offset: 0xE40 */
       uint8_t RESERVED_6[436];
  __IO uint32_t PSELID;                            /**< Flexcomm ID and peripheral function select register, offset: 0xFF8 */
  __I  uint32_t ID;                                /**< USART Module Identifier, offset: 0xFFC */
} USART_Type;

/* ----------------------------------------------------------------------------
   -- USART Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup USART_Register_Masks USART Register Masks
 * @{
 */

/*! @name CFG - USART Configuration register. Basic USART configuration settings that typically are not changed during operation. */
/*! @{ */
#define USART_CFG_ENABLE_MASK                    (0x1U)
#define USART_CFG_ENABLE_SHIFT                   (0U)
/*! ENABLE - USART Enable. 0: Disabled. The USART is disabled and the internal state machine and
 *    counters are reset. While Enable = 0, all USART interrupts and DMA transfers are disabled. When
 *    Enable is set again, CFG and most other control bits remain unchanged. When re-enabled, the
 *    USART will immediately be ready to transmit because the transmitter has been reset and is
 *    therefore available. 1: Eanbled. The USART is enabled for operation.
 */
#define USART_CFG_ENABLE(x)                      (((uint32_t)(((uint32_t)(x)) << USART_CFG_ENABLE_SHIFT)) & USART_CFG_ENABLE_MASK)
#define USART_CFG_DATALEN_MASK                   (0xCU)
#define USART_CFG_DATALEN_SHIFT                  (2U)
/*! DATALEN - Selects the data size for the USART. 0: 7 bit Data Length; 1: 8 bit data length; 2: 9
 *    bit data length. The 9th bit is commonly used for addressing in multidrop mode. See the
 *    ADDRDET bit in the CTRL regsiter. 3: Reserved.
 */
#define USART_CFG_DATALEN(x)                     (((uint32_t)(((uint32_t)(x)) << USART_CFG_DATALEN_SHIFT)) & USART_CFG_DATALEN_MASK)
#define USART_CFG_PARITYSEL_MASK                 (0x30U)
#define USART_CFG_PARITYSEL_SHIFT                (4U)
/*! PARITYSEL - Selects what type of parity is used by the USART. 0: No parity; 1: Reserved; 2: Even
 *    Parity. Adds a bit to each character such that the number of 1s in a transmitted character is
 *    even, and the number of 1s in a received character is expected to be even. 3: Odd parity.
 *    Adds a bit to each character such that the number of 1s in a transmitted character is odd, and
 *    the number of 1s in a received character is expected to be odd.
 */
#define USART_CFG_PARITYSEL(x)                   (((uint32_t)(((uint32_t)(x)) << USART_CFG_PARITYSEL_SHIFT)) & USART_CFG_PARITYSEL_MASK)
#define USART_CFG_STOPLEN_MASK                   (0x40U)
#define USART_CFG_STOPLEN_SHIFT                  (6U)
/*! STOPLEN - Number of stop bits appended to transmitted data. Only a single stop bit is required
 *    for received data. 0: 1 stop bit; 1: 2 stop bits. This setting should only be used for
 *    asynchronous communication.
 */
#define USART_CFG_STOPLEN(x)                     (((uint32_t)(((uint32_t)(x)) << USART_CFG_STOPLEN_SHIFT)) & USART_CFG_STOPLEN_MASK)
#define USART_CFG_MODE32K_MASK                   (0x80U)
#define USART_CFG_MODE32K_SHIFT                  (7U)
/*! MODE32K - Selects standard or 32 kHz clocking mode. 0: Disabled. USART uses standard clocking.
 *    1: Enabled. USART uses the 32 kHz clock from the RTC oscillator as the clock source to the BRG,
 *    and uses a special bit clocking scheme.
 */
#define USART_CFG_MODE32K(x)                     (((uint32_t)(((uint32_t)(x)) << USART_CFG_MODE32K_SHIFT)) & USART_CFG_MODE32K_MASK)
#define USART_CFG_LINMODE_MASK                   (0x100U)
#define USART_CFG_LINMODE_SHIFT                  (8U)
/*! LINMODE - LIN bus break mode enable. 0: Disabled. Break detect and generate is configured for
 *    normal operation. 1: Enabled. Break detect and generate is configured for LIN bus operation.
 */
#define USART_CFG_LINMODE(x)                     (((uint32_t)(((uint32_t)(x)) << USART_CFG_LINMODE_SHIFT)) & USART_CFG_LINMODE_MASK)
#define USART_CFG_CTSEN_MASK                     (0x200U)
#define USART_CFG_CTSEN_SHIFT                    (9U)
/*! CTSEN - CTS Enable. Determines whether CTS is used for flow control. CTS can be from the input
 *    pin, or from the USART s own RTS if loopback mode is enabled. 0 : No flow control. The
 *    transmitter does not receive any automatic flow control signal. 1: Flow control enabled. The
 *    transmitter uses the CTS input (or RTS output in loopback mode) for flow control purposes.
 */
#define USART_CFG_CTSEN(x)                       (((uint32_t)(((uint32_t)(x)) << USART_CFG_CTSEN_SHIFT)) & USART_CFG_CTSEN_MASK)
#define USART_CFG_SYNCEN_MASK                    (0x800U)
#define USART_CFG_SYNCEN_SHIFT                   (11U)
/*! SYNCEN - Selects synchronous or asynchronous operation. 0: Asynchronous mode. 1: Synchronous mode.
 */
#define USART_CFG_SYNCEN(x)                      (((uint32_t)(((uint32_t)(x)) << USART_CFG_SYNCEN_SHIFT)) & USART_CFG_SYNCEN_MASK)
#define USART_CFG_CLKPOL_MASK                    (0x1000U)
#define USART_CFG_CLKPOL_SHIFT                   (12U)
/*! CLKPOL - Selects the clock polarity and sampling edge of received data in synchronous mode. 0:
 *    Falling edge. Un_RXD is sampled on the falling edge of SCLK. 1: Rising edge. Un_RXD is sampled
 *    on the rising edge of SCLK.
 */
#define USART_CFG_CLKPOL(x)                      (((uint32_t)(((uint32_t)(x)) << USART_CFG_CLKPOL_SHIFT)) & USART_CFG_CLKPOL_MASK)
#define USART_CFG_SYNCMST_MASK                   (0x4000U)
#define USART_CFG_SYNCMST_SHIFT                  (14U)
/*! SYNCMST - Synchronous mode Master select. 0: Slave. When synchronous mode is enabled, the USART
 *    is a slave. 1: Master. When synchronous mode is enabled, the USART is a master.
 */
#define USART_CFG_SYNCMST(x)                     (((uint32_t)(((uint32_t)(x)) << USART_CFG_SYNCMST_SHIFT)) & USART_CFG_SYNCMST_MASK)
#define USART_CFG_LOOP_MASK                      (0x8000U)
#define USART_CFG_LOOP_SHIFT                     (15U)
/*! LOOP - Selects data loopback mode. 0: Normal operaetion. 1: Loopback mode. This provides a
 *    mechanism to perform diagnostic loopback testing for USART data. Serial data from the transmitter
 *    (Un_TXD) is connected internally to serial input of the receive (Un_RXD). Un_TXD and Un_RTS
 *    activity will also appear on external pins if these functions are configured to appear on device
 *    pins. The receiver RTS signal is also looped back to CTS and performs flow control if enabled
 *    by CTSEN.
 */
#define USART_CFG_LOOP(x)                        (((uint32_t)(((uint32_t)(x)) << USART_CFG_LOOP_SHIFT)) & USART_CFG_LOOP_MASK)
#define USART_CFG_OETA_MASK                      (0x40000U)
#define USART_CFG_OETA_SHIFT                     (18U)
/*! OETA - Output Enable Turnaround time enable for RS-485 operation. 0: Disabled. If selected by
 *    OESEL, the Output Enable signal deasserted at the end of the last stop bit of a transmission. 1:
 *    Enabled. If selected by OESEL, the Output Enable signal remains asserted for one character
 *    time after the end of the last stop bit of a transmission. OE will also remain asserted if
 *    another transmit begins before it is deasserted.
 */
#define USART_CFG_OETA(x)                        (((uint32_t)(((uint32_t)(x)) << USART_CFG_OETA_SHIFT)) & USART_CFG_OETA_MASK)
#define USART_CFG_AUTOADDR_MASK                  (0x80000U)
#define USART_CFG_AUTOADDR_SHIFT                 (19U)
/*! AUTOADDR - Automatic Address matching enable. 0: Disabled. When addressing is enabled by
 *    ADDRDET, address matching is done by software. This provides the possibility of versatile addressing
 *    (e.g. respond to more than one address). 1: Enabled. When addressing is enabled by ADDRDET,
 *    address matching is done by hardware, using the value in the ADDR register as the address to
 *    match.
 */
#define USART_CFG_AUTOADDR(x)                    (((uint32_t)(((uint32_t)(x)) << USART_CFG_AUTOADDR_SHIFT)) & USART_CFG_AUTOADDR_MASK)
#define USART_CFG_OESEL_MASK                     (0x100000U)
#define USART_CFG_OESEL_SHIFT                    (20U)
/*! OESEL - Output Enable Select. 0: Standard. The RTS signal is used as the standard flow control
 *    function. 1: RS-485. The RTS signal configured to provide an output enable signal to control an
 *    RS-485 transceiver.
 */
#define USART_CFG_OESEL(x)                       (((uint32_t)(((uint32_t)(x)) << USART_CFG_OESEL_SHIFT)) & USART_CFG_OESEL_MASK)
#define USART_CFG_OEPOL_MASK                     (0x200000U)
#define USART_CFG_OEPOL_SHIFT                    (21U)
/*! OEPOL - Output Enable Polarity. 0: Low. If selected by OESEL, the output enable is active low.
 *    1: High. If selected by OESEL, the output enable is active high.
 */
#define USART_CFG_OEPOL(x)                       (((uint32_t)(((uint32_t)(x)) << USART_CFG_OEPOL_SHIFT)) & USART_CFG_OEPOL_MASK)
#define USART_CFG_RXPOL_MASK                     (0x400000U)
#define USART_CFG_RXPOL_SHIFT                    (22U)
/*! RXPOL - Receive data polarity. 0: Standard. The RX signal is used as it arrives from the pin.
 *    This means that the RX rest value is 1, start bit is 0, data is not inverted, and the stop bit
 *    is 1. 1: Inverted. The RX signal is inverted before being used by the USART. This means that
 *    the RX rest value is 0, start bit is 1, data is inverted, and the stop bit is 0.
 */
#define USART_CFG_RXPOL(x)                       (((uint32_t)(((uint32_t)(x)) << USART_CFG_RXPOL_SHIFT)) & USART_CFG_RXPOL_MASK)
#define USART_CFG_TXPOL_MASK                     (0x800000U)
#define USART_CFG_TXPOL_SHIFT                    (23U)
/*! TXPOL - Transmit data polarity. 0: Standard. The TX signal is sent out without change. This
 *    means that the TX rest value is 1, start bit is 0, data is not inverted, and the stop bit is 1. 1:
 *    Inverted. The TX signal is inverted by the USART before being sent out. This means that the
 *    TX rest value is 0, start bit is 1, data is inverted, and the stop bit is 0.
 */
#define USART_CFG_TXPOL(x)                       (((uint32_t)(((uint32_t)(x)) << USART_CFG_TXPOL_SHIFT)) & USART_CFG_TXPOL_MASK)
/*! @} */

/*! @name CTL - USART Control register. USART control settings that are more likely to change during operation. */
/*! @{ */
#define USART_CTL_TXBRKEN_MASK                   (0x2U)
#define USART_CTL_TXBRKEN_SHIFT                  (1U)
/*! TXBRKEN - Break Enable. 0: Normal Operation. 1: Continuous break. Continuous break is sent
 *    immediately when this bit is set, and remains until this bit is cleared. A break may be sent
 *    without danger of corrupting any currently transmitting character if the transmitter is first
 *    disabled (TXDIS in CTL is set) and then waiting for the transmitter to be disabled (TXDISINT in STAT
 *    = 1) before writing 1 to TXBRKEN.
 */
#define USART_CTL_TXBRKEN(x)                     (((uint32_t)(((uint32_t)(x)) << USART_CTL_TXBRKEN_SHIFT)) & USART_CTL_TXBRKEN_MASK)
#define USART_CTL_ADDRDET_MASK                   (0x4U)
#define USART_CTL_ADDRDET_SHIFT                  (2U)
/*! ADDRDET - Enable address detect mode. 0: Disabled. The USART presents all incoming data. 1:
 *    Enabled. The USART receiver ignores incoming data that does not have the most significant bit of
 *    the data (typically the 9th bit) = 1. When the data MSB bit = 1, the receiver treats the
 *    incoming data normally, generating a received data interrupt. Software can then check the data to
 *    see if this is an address that should be handled. If it is, the ADDRDET bit is cleared by
 *    software and further incoming data is handled normally.
 */
#define USART_CTL_ADDRDET(x)                     (((uint32_t)(((uint32_t)(x)) << USART_CTL_ADDRDET_SHIFT)) & USART_CTL_ADDRDET_MASK)
#define USART_CTL_TXDIS_MASK                     (0x40U)
#define USART_CTL_TXDIS_SHIFT                    (6U)
/*! TXDIS - Transmit Disable. 0: Not disabled. USART transmitter is not disabled. 1: Disabled. USART
 *    transmitter is disabled after any character currently being transmitted is complete. This
 *    feature can be used to facilitate software flow control.
 */
#define USART_CTL_TXDIS(x)                       (((uint32_t)(((uint32_t)(x)) << USART_CTL_TXDIS_SHIFT)) & USART_CTL_TXDIS_MASK)
#define USART_CTL_CC_MASK                        (0x100U)
#define USART_CTL_CC_SHIFT                       (8U)
/*! CC - Continuous Clock generation. By default, SCLK is only output while data is being
 *    transmitted in synchronous mode. 0: Clock on character. In synchronous mode, SCLK cycles only when
 *    characters are being sent on Un_TXD or to complete a character that is being received. 1:
 *    Continuous clock. SCLK runs continuously in synchronous mode, allowing characters to be received on
 *    Un_RxD independently from transmission on Un_TXD).
 */
#define USART_CTL_CC(x)                          (((uint32_t)(((uint32_t)(x)) << USART_CTL_CC_SHIFT)) & USART_CTL_CC_MASK)
#define USART_CTL_CLRCCONRX_MASK                 (0x200U)
#define USART_CTL_CLRCCONRX_SHIFT                (9U)
/*! CLRCCONRX - Clear Continuous Clock. 0: No effect. No effect on the CC bit. 1: Auto-clear. The CC
 *    bit is automatically cleared when a complete character has been received. This bit is cleared
 *    at the same time.
 */
#define USART_CTL_CLRCCONRX(x)                   (((uint32_t)(((uint32_t)(x)) << USART_CTL_CLRCCONRX_SHIFT)) & USART_CTL_CLRCCONRX_MASK)
#define USART_CTL_AUTOBAUD_MASK                  (0x10000U)
#define USART_CTL_AUTOBAUD_SHIFT                 (16U)
/*! AUTOBAUD - Autobaud enable. 0: Disabled. USART is in normal operating mode. 1: Enabled. USART is
 *    in auto-baud mode. This bit should only be set when the USART receiver is idle. The first
 *    start bit of RX is measured and used the update the BRG register to match the received data rate.
 *    AUTOBAUD is cleared once this process is complete, or if there is an AERR.
 */
#define USART_CTL_AUTOBAUD(x)                    (((uint32_t)(((uint32_t)(x)) << USART_CTL_AUTOBAUD_SHIFT)) & USART_CTL_AUTOBAUD_MASK)
/*! @} */

/*! @name STAT - USART Status register. The complete status value can be read here. Writing ones clears some bits in the register. Some bits can be cleared by writing a 1 to them. */
/*! @{ */
#define USART_STAT_RXIDLE_MASK                   (0x2U)
#define USART_STAT_RXIDLE_SHIFT                  (1U)
/*! RXIDLE - Receiver Idle. When 0, indicates that the receiver is currently in the process of
 *    receiving data. When 1, indicates that the receiver is not currently in the process of receiving
 *    data.
 */
#define USART_STAT_RXIDLE(x)                     (((uint32_t)(((uint32_t)(x)) << USART_STAT_RXIDLE_SHIFT)) & USART_STAT_RXIDLE_MASK)
#define USART_STAT_TXIDLE_MASK                   (0x8U)
#define USART_STAT_TXIDLE_SHIFT                  (3U)
/*! TXIDLE - Transmitter Idle. When 0, indicates that the transmitter is currently in the process of
 *    sending data.When 1, indicate that the transmitter is not currently in the process of sending
 *    data.
 */
#define USART_STAT_TXIDLE(x)                     (((uint32_t)(((uint32_t)(x)) << USART_STAT_TXIDLE_SHIFT)) & USART_STAT_TXIDLE_MASK)
#define USART_STAT_CTS_MASK                      (0x10U)
#define USART_STAT_CTS_SHIFT                     (4U)
/*! CTS - This bit reflects the current state of the CTS signal, regardless of the setting of the
 *    CTSEN bit in the CFG register. This will be the value of the CTS input pin unless loopback mode
 *    is enabled. Hence, reset value not applicable.
 */
#define USART_STAT_CTS(x)                        (((uint32_t)(((uint32_t)(x)) << USART_STAT_CTS_SHIFT)) & USART_STAT_CTS_MASK)
#define USART_STAT_DELTACTS_MASK                 (0x20U)
#define USART_STAT_DELTACTS_SHIFT                (5U)
/*! DELTACTS - This bit is set when a change in the state is detected for the CTS flag above. This bit is cleared by software.
 */
#define USART_STAT_DELTACTS(x)                   (((uint32_t)(((uint32_t)(x)) << USART_STAT_DELTACTS_SHIFT)) & USART_STAT_DELTACTS_MASK)
#define USART_STAT_TXDISSTAT_MASK                (0x40U)
#define USART_STAT_TXDISSTAT_SHIFT               (6U)
/*! TXDISSTAT - Transmitter Disabled Status flag. When 1, this bit indicates that the USART
 *    transmitter is fully idle after being disabled via the TXDIS bit in the CFG register (TXDIS = 1).
 */
#define USART_STAT_TXDISSTAT(x)                  (((uint32_t)(((uint32_t)(x)) << USART_STAT_TXDISSTAT_SHIFT)) & USART_STAT_TXDISSTAT_MASK)
#define USART_STAT_RXBRK_MASK                    (0x400U)
#define USART_STAT_RXBRK_SHIFT                   (10U)
/*! RXBRK - Received Break. This bit reflects the current state of the receiver break detection
 *    logic. It is set when the Un_RXD pin remains low for 16 bit times. Note that FRAMERRINT will also
 *    be set when this condition occurs because the stop bit(s) for the character would be missing.
 *    RXBRK is cleared when the Un_RXD pin goes high.
 */
#define USART_STAT_RXBRK(x)                      (((uint32_t)(((uint32_t)(x)) << USART_STAT_RXBRK_SHIFT)) & USART_STAT_RXBRK_MASK)
#define USART_STAT_DELTARXBRK_MASK               (0x800U)
#define USART_STAT_DELTARXBRK_SHIFT              (11U)
/*! DELTARXBRK - This bit is set when a change in the state of receiver break detection occurs. Cleared by software.
 */
#define USART_STAT_DELTARXBRK(x)                 (((uint32_t)(((uint32_t)(x)) << USART_STAT_DELTARXBRK_SHIFT)) & USART_STAT_DELTARXBRK_MASK)
#define USART_STAT_START_MASK                    (0x1000U)
#define USART_STAT_START_SHIFT                   (12U)
/*! START - This bit is set when a start is detected on the receiver input. Its purpose is primarily
 *    to allow wake-up from Deep sleep or Power-down mode immediately when a start is detected.
 *    Cleared by software.
 */
#define USART_STAT_START(x)                      (((uint32_t)(((uint32_t)(x)) << USART_STAT_START_SHIFT)) & USART_STAT_START_MASK)
#define USART_STAT_FRAMERRINT_MASK               (0x2000U)
#define USART_STAT_FRAMERRINT_SHIFT              (13U)
/*! FRAMERRINT - Framing Error interrupt flag. This flag is set when a character is received with a
 *    missing stop bit at the expected location. This could be an indication of a baud rate or
 *    configuration mismatch with the transmitting source.
 */
#define USART_STAT_FRAMERRINT(x)                 (((uint32_t)(((uint32_t)(x)) << USART_STAT_FRAMERRINT_SHIFT)) & USART_STAT_FRAMERRINT_MASK)
#define USART_STAT_PARITYERRINT_MASK             (0x4000U)
#define USART_STAT_PARITYERRINT_SHIFT            (14U)
/*! PARITYERRINT - Parity Error interrupt flag. This flag is set when a parity error is detected in a received character.
 */
#define USART_STAT_PARITYERRINT(x)               (((uint32_t)(((uint32_t)(x)) << USART_STAT_PARITYERRINT_SHIFT)) & USART_STAT_PARITYERRINT_MASK)
#define USART_STAT_RXNOISEINT_MASK               (0x8000U)
#define USART_STAT_RXNOISEINT_SHIFT              (15U)
/*! RXNOISEINT - Received Noise interrupt flag. Three samples of received data are taken in order to
 *    determine the value of each received data bit, except in synchronous mode. This acts as a
 *    noise filter if one sample disagrees. This flag is set when a received data bit contains one
 *    disagreeing sample. This could indicate line noise, a baud rate or character format mismatch, or
 *    loss of synchronization during data reception.
 */
#define USART_STAT_RXNOISEINT(x)                 (((uint32_t)(((uint32_t)(x)) << USART_STAT_RXNOISEINT_SHIFT)) & USART_STAT_RXNOISEINT_MASK)
#define USART_STAT_ABERR_MASK                    (0x10000U)
#define USART_STAT_ABERR_SHIFT                   (16U)
/*! ABERR - Auto baud Error. An auto baud error can occur if the BRG counts to its limit before the
 *    end of the start bit that is being measured, essentially an auto baud time-out.
 */
#define USART_STAT_ABERR(x)                      (((uint32_t)(((uint32_t)(x)) << USART_STAT_ABERR_SHIFT)) & USART_STAT_ABERR_MASK)
/*! @} */

/*! @name INTENSET - Interrupt Enable read and Set register for USART (not FIFO) status. Contains individual interrupt enable bits for each potential USART interrupt. A complete value may be read from this register. Writing a 1 to any implemented bit position causes that bit to be set. */
/*! @{ */
#define USART_INTENSET_RXRDYEN_MASK              (0x1U)
#define USART_INTENSET_RXRDYEN_SHIFT             (0U)
/*! RXRDYEN - When 1, enables an interrupt when RX becomes ready
 */
#define USART_INTENSET_RXRDYEN(x)                (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_RXRDYEN_SHIFT)) & USART_INTENSET_RXRDYEN_MASK)
#define USART_INTENSET_TXRDTEN_MASK              (0x4U)
#define USART_INTENSET_TXRDTEN_SHIFT             (2U)
/*! TXRDTEN - When 1, enables an interrupt when TX becomes ready
 */
#define USART_INTENSET_TXRDTEN(x)                (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_TXRDTEN_SHIFT)) & USART_INTENSET_TXRDTEN_MASK)
#define USART_INTENSET_TXIDLEEN_MASK             (0x8U)
#define USART_INTENSET_TXIDLEEN_SHIFT            (3U)
/*! TXIDLEEN - When 1, enables an interrupt when the transmitter becomes idle (TXIDLE = 1).
 */
#define USART_INTENSET_TXIDLEEN(x)               (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_TXIDLEEN_SHIFT)) & USART_INTENSET_TXIDLEEN_MASK)
#define USART_INTENSET_DELTACTSEN_MASK           (0x20U)
#define USART_INTENSET_DELTACTSEN_SHIFT          (5U)
/*! DELTACTSEN - Transmitter Idle. When 0, indicates that the transmitter is currently in the
 *    process of sending data.When 1, indicate that the transmitter is not currently in the process of
 *    sending data.
 */
#define USART_INTENSET_DELTACTSEN(x)             (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_DELTACTSEN_SHIFT)) & USART_INTENSET_DELTACTSEN_MASK)
#define USART_INTENSET_TXDISEN_MASK              (0x40U)
#define USART_INTENSET_TXDISEN_SHIFT             (6U)
/*! TXDISEN - When 1, enables an interrupt when the transmitter is fully disabled as indicated by
 *    the TXDISINT flag in STAT. See description of the TXDISINT bit for details.
 */
#define USART_INTENSET_TXDISEN(x)                (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_TXDISEN_SHIFT)) & USART_INTENSET_TXDISEN_MASK)
#define USART_INTENSET_DELTARXBRKEN_MASK         (0x800U)
#define USART_INTENSET_DELTARXBRKEN_SHIFT        (11U)
/*! DELTARXBRKEN - When 1, enables an interrupt when a change of state has occurred in the detection
 *    of a received break condition (break condition asserted or deasserted).
 */
#define USART_INTENSET_DELTARXBRKEN(x)           (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_DELTARXBRKEN_SHIFT)) & USART_INTENSET_DELTARXBRKEN_MASK)
#define USART_INTENSET_STARTEN_MASK              (0x1000U)
#define USART_INTENSET_STARTEN_SHIFT             (12U)
/*! STARTEN - When 1, enables an interrupt when a received start bit has been detected.
 */
#define USART_INTENSET_STARTEN(x)                (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_STARTEN_SHIFT)) & USART_INTENSET_STARTEN_MASK)
#define USART_INTENSET_FRAMERREN_MASK            (0x2000U)
#define USART_INTENSET_FRAMERREN_SHIFT           (13U)
/*! FRAMERREN - When 1, enables an interrupt when a framing error has been detected.
 */
#define USART_INTENSET_FRAMERREN(x)              (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_FRAMERREN_SHIFT)) & USART_INTENSET_FRAMERREN_MASK)
#define USART_INTENSET_PARITYERREN_MASK          (0x4000U)
#define USART_INTENSET_PARITYERREN_SHIFT         (14U)
/*! PARITYERREN - When 1, enables an interrupt when a parity error has been detected.
 */
#define USART_INTENSET_PARITYERREN(x)            (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_PARITYERREN_SHIFT)) & USART_INTENSET_PARITYERREN_MASK)
#define USART_INTENSET_RXNOISEEN_MASK            (0x8000U)
#define USART_INTENSET_RXNOISEEN_SHIFT           (15U)
/*! RXNOISEEN - When 1, enables an interrupt when noise is detected.
 */
#define USART_INTENSET_RXNOISEEN(x)              (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_RXNOISEEN_SHIFT)) & USART_INTENSET_RXNOISEEN_MASK)
#define USART_INTENSET_ABERREN_MASK              (0x10000U)
#define USART_INTENSET_ABERREN_SHIFT             (16U)
/*! ABERREN - When 1, enables an interrupt when an auto baud error occurs.
 */
#define USART_INTENSET_ABERREN(x)                (((uint32_t)(((uint32_t)(x)) << USART_INTENSET_ABERREN_SHIFT)) & USART_INTENSET_ABERREN_MASK)
/*! @} */

/*! @name INTENCLR - Interrupt Enable Clear register. Allows clearing any combination of bits in the INTENSET register. Writing a 1 to any implemented bit position causes the corresponding bit to be cleared. */
/*! @{ */
#define USART_INTENCLR_RXRDYCLR_MASK             (0x1U)
#define USART_INTENCLR_RXRDYCLR_SHIFT            (0U)
/*! RXRDYCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_RXRDYCLR(x)               (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_RXRDYCLR_SHIFT)) & USART_INTENCLR_RXRDYCLR_MASK)
#define USART_INTENCLR_TXRDYCLR_MASK             (0x4U)
#define USART_INTENCLR_TXRDYCLR_SHIFT            (2U)
/*! TXRDYCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_TXRDYCLR(x)               (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_TXRDYCLR_SHIFT)) & USART_INTENCLR_TXRDYCLR_MASK)
#define USART_INTENCLR_TXIDLECLR_MASK            (0x8U)
#define USART_INTENCLR_TXIDLECLR_SHIFT           (3U)
/*! TXIDLECLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_TXIDLECLR(x)              (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_TXIDLECLR_SHIFT)) & USART_INTENCLR_TXIDLECLR_MASK)
#define USART_INTENCLR_DELTACTSCLR_MASK          (0x20U)
#define USART_INTENCLR_DELTACTSCLR_SHIFT         (5U)
/*! DELTACTSCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_DELTACTSCLR(x)            (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_DELTACTSCLR_SHIFT)) & USART_INTENCLR_DELTACTSCLR_MASK)
#define USART_INTENCLR_TXDISCLR_MASK             (0x40U)
#define USART_INTENCLR_TXDISCLR_SHIFT            (6U)
/*! TXDISCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_TXDISCLR(x)               (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_TXDISCLR_SHIFT)) & USART_INTENCLR_TXDISCLR_MASK)
#define USART_INTENCLR_DELTARXBRKCLR_MASK        (0x800U)
#define USART_INTENCLR_DELTARXBRKCLR_SHIFT       (11U)
/*! DELTARXBRKCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_DELTARXBRKCLR(x)          (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_DELTARXBRKCLR_SHIFT)) & USART_INTENCLR_DELTARXBRKCLR_MASK)
#define USART_INTENCLR_STARTCLR_MASK             (0x1000U)
#define USART_INTENCLR_STARTCLR_SHIFT            (12U)
/*! STARTCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_STARTCLR(x)               (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_STARTCLR_SHIFT)) & USART_INTENCLR_STARTCLR_MASK)
#define USART_INTENCLR_FRAMERRCLR_MASK           (0x2000U)
#define USART_INTENCLR_FRAMERRCLR_SHIFT          (13U)
/*! FRAMERRCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_FRAMERRCLR(x)             (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_FRAMERRCLR_SHIFT)) & USART_INTENCLR_FRAMERRCLR_MASK)
#define USART_INTENCLR_PARITYERRCLR_MASK         (0x4000U)
#define USART_INTENCLR_PARITYERRCLR_SHIFT        (14U)
/*! PARITYERRCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_PARITYERRCLR(x)           (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_PARITYERRCLR_SHIFT)) & USART_INTENCLR_PARITYERRCLR_MASK)
#define USART_INTENCLR_RXNOISECLR_MASK           (0x8000U)
#define USART_INTENCLR_RXNOISECLR_SHIFT          (15U)
/*! RXNOISECLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_RXNOISECLR(x)             (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_RXNOISECLR_SHIFT)) & USART_INTENCLR_RXNOISECLR_MASK)
#define USART_INTENCLR_ABERRCLR_MASK             (0x10000U)
#define USART_INTENCLR_ABERRCLR_SHIFT            (16U)
/*! ABERRCLR - Writing 1 clears the corresponding bit in the INTENSET register.
 */
#define USART_INTENCLR_ABERRCLR(x)               (((uint32_t)(((uint32_t)(x)) << USART_INTENCLR_ABERRCLR_SHIFT)) & USART_INTENCLR_ABERRCLR_MASK)
/*! @} */

/*! @name BRG - Baud Rate Generator register. 16-bit integer baud rate divisor value. */
/*! @{ */
#define USART_BRG_BRGVAL_MASK                    (0xFFFFU)
#define USART_BRG_BRGVAL_SHIFT                   (0U)
/*! BRGVAL - This value is used to divide the USART input clock to determine the baud rate, based on
 *    the input clock from the FRG. 0: FCLK is used directly by the USART function. 1: FCLK is
 *    divided by 2 before use by the USART function. 2: FCLK is divided by 3 before use by the USART
 *    function. ... 0xFFFF = FCLK is divided by 65,536 before use by the USART function.
 */
#define USART_BRG_BRGVAL(x)                      (((uint32_t)(((uint32_t)(x)) << USART_BRG_BRGVAL_SHIFT)) & USART_BRG_BRGVAL_MASK)
/*! @} */

/*! @name INTSTAT - Interrupt status register. Reflects the status of interrupts that are currently enabled. */
/*! @{ */
#define USART_INTSTAT_RX_RDY_MASK                (0x1U)
#define USART_INTSTAT_RX_RDY_SHIFT               (0U)
/*! RX_RDY - Receiver Ready Status
 */
#define USART_INTSTAT_RX_RDY(x)                  (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_RX_RDY_SHIFT)) & USART_INTSTAT_RX_RDY_MASK)
#define USART_INTSTAT_RXIDLE_MASK                (0x2U)
#define USART_INTSTAT_RXIDLE_SHIFT               (1U)
/*! RXIDLE - Receiver Idle Status
 */
#define USART_INTSTAT_RXIDLE(x)                  (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_RXIDLE_SHIFT)) & USART_INTSTAT_RXIDLE_MASK)
#define USART_INTSTAT_TX_RDY_MASK                (0x4U)
#define USART_INTSTAT_TX_RDY_SHIFT               (2U)
/*! TX_RDY - Transmitter Ready Status
 */
#define USART_INTSTAT_TX_RDY(x)                  (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_TX_RDY_SHIFT)) & USART_INTSTAT_TX_RDY_MASK)
#define USART_INTSTAT_TXIDLE_MASK                (0x8U)
#define USART_INTSTAT_TXIDLE_SHIFT               (3U)
/*! TXIDLE - Transmitter Idle status.
 */
#define USART_INTSTAT_TXIDLE(x)                  (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_TXIDLE_SHIFT)) & USART_INTSTAT_TXIDLE_MASK)
#define USART_INTSTAT_DELTACTS_MASK              (0x20U)
#define USART_INTSTAT_DELTACTS_SHIFT             (5U)
/*! DELTACTS - This bit is set when a change in the state of the CTS input is detected.
 */
#define USART_INTSTAT_DELTACTS(x)                (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_DELTACTS_SHIFT)) & USART_INTSTAT_DELTACTS_MASK)
#define USART_INTSTAT_TXDIS_MASK                 (0x40U)
#define USART_INTSTAT_TXDIS_SHIFT                (6U)
/*! TXDIS - Transmitter Disabled Interrupt flag.
 */
#define USART_INTSTAT_TXDIS(x)                   (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_TXDIS_SHIFT)) & USART_INTSTAT_TXDIS_MASK)
#define USART_INTSTAT_DELTARXBRK_MASK            (0x800U)
#define USART_INTSTAT_DELTARXBRK_SHIFT           (11U)
/*! DELTARXBRK - This bit is set when a change in the state of receiver break detection occurs.
 */
#define USART_INTSTAT_DELTARXBRK(x)              (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_DELTARXBRK_SHIFT)) & USART_INTSTAT_DELTARXBRK_MASK)
#define USART_INTSTAT_START_MASK                 (0x1000U)
#define USART_INTSTAT_START_SHIFT                (12U)
/*! START - This bit is set when a start is detected on the receiver input.
 */
#define USART_INTSTAT_START(x)                   (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_START_SHIFT)) & USART_INTSTAT_START_MASK)
#define USART_INTSTAT_FRAMERR_MASK               (0x2000U)
#define USART_INTSTAT_FRAMERR_SHIFT              (13U)
/*! FRAMERR - Framing Error interrupt flag.
 */
#define USART_INTSTAT_FRAMERR(x)                 (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_FRAMERR_SHIFT)) & USART_INTSTAT_FRAMERR_MASK)
#define USART_INTSTAT_PARITYERR_MASK             (0x4000U)
#define USART_INTSTAT_PARITYERR_SHIFT            (14U)
/*! PARITYERR - Parity Error interrupt flag.
 */
#define USART_INTSTAT_PARITYERR(x)               (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_PARITYERR_SHIFT)) & USART_INTSTAT_PARITYERR_MASK)
#define USART_INTSTAT_RXNOISE_MASK               (0x8000U)
#define USART_INTSTAT_RXNOISE_SHIFT              (15U)
/*! RXNOISE - Received Noise interrupt flag.
 */
#define USART_INTSTAT_RXNOISE(x)                 (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_RXNOISE_SHIFT)) & USART_INTSTAT_RXNOISE_MASK)
#define USART_INTSTAT_ABERR_MASK                 (0x10000U)
#define USART_INTSTAT_ABERR_SHIFT                (16U)
/*! ABERR - Auto baud Error Interrupt flag.
 */
#define USART_INTSTAT_ABERR(x)                   (((uint32_t)(((uint32_t)(x)) << USART_INTSTAT_ABERR_SHIFT)) & USART_INTSTAT_ABERR_MASK)
/*! @} */

/*! @name OSR - Oversample selection register for asynchronous communication. */
/*! @{ */
#define USART_OSR_OSRVAL_MASK                    (0xFU)
#define USART_OSR_OSRVAL_SHIFT                   (0U)
/*! OSRVAL - Oversample Selection Value. 0 to 3: not supported. 0x4: 5 function clocks are used to
 *    transmit and receive each data bit. 0x5: 6 function clocks are used to transmit and receive
 *    each data bit. ... 0xF: 16 function clocks are used to transmit and receive each data bit.
 */
#define USART_OSR_OSRVAL(x)                      (((uint32_t)(((uint32_t)(x)) << USART_OSR_OSRVAL_SHIFT)) & USART_OSR_OSRVAL_MASK)
/*! @} */

/*! @name ADDR - Address register for automatic address matching. */
/*! @{ */
#define USART_ADDR_ADDRESS_MASK                  (0xFFU)
#define USART_ADDR_ADDRESS_SHIFT                 (0U)
/*! ADDRESS - 8-bit address used with automatic address matching. Used when address detection is
 *    enabled (ADDRDET in CTL = 1) and automatic address matching is enabled (AUTOADDR in CFG = 1).
 */
#define USART_ADDR_ADDRESS(x)                    (((uint32_t)(((uint32_t)(x)) << USART_ADDR_ADDRESS_SHIFT)) & USART_ADDR_ADDRESS_MASK)
/*! @} */

/*! @name FIFOCFG - FIFO configuration and enable register. */
/*! @{ */
#define USART_FIFOCFG_ENABLETX_MASK              (0x1U)
#define USART_FIFOCFG_ENABLETX_SHIFT             (0U)
/*! ENABLETX - Enable the transmit FIFO. 0: The transmit FIFO is not enabled. 1: The transmit FIFO
 *    is enabled. This is automatically enabled when PSELID.PERSEL is set to 1 to configure the USART
 *    functionality.
 */
#define USART_FIFOCFG_ENABLETX(x)                (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_ENABLETX_SHIFT)) & USART_FIFOCFG_ENABLETX_MASK)
#define USART_FIFOCFG_ENABLERX_MASK              (0x2U)
#define USART_FIFOCFG_ENABLERX_SHIFT             (1U)
/*! ENABLERX - Enable the receive FIFO. 0: The receive FIFO is not enabled. 1: The receive FIFO is
 *    enabled. This is automatically enabled when PSELID.PERSEL is set to 1 to configure the USART
 *    functionality.
 */
#define USART_FIFOCFG_ENABLERX(x)                (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_ENABLERX_SHIFT)) & USART_FIFOCFG_ENABLERX_MASK)
#define USART_FIFOCFG_SIZE_MASK                  (0x30U)
#define USART_FIFOCFG_SIZE_SHIFT                 (4U)
/*! SIZE - FIFO size configuration. This is a read-only field. 0x0 = FIFO is configured as 4 entries
 *    of 8 bits. 0x1, 0x2, 0x3 = not applicable.
 */
#define USART_FIFOCFG_SIZE(x)                    (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_SIZE_SHIFT)) & USART_FIFOCFG_SIZE_MASK)
#define USART_FIFOCFG_DMATX_MASK                 (0x1000U)
#define USART_FIFOCFG_DMATX_SHIFT                (12U)
/*! DMATX - DMA configuration for transmit. 0: DMA is not used for the transmit function. 1:
 *    Generate a DMA request for the transmit function if the FIFO is not full. Generally, data interrupts
 *    would be disabled if DMA is enabled
 */
#define USART_FIFOCFG_DMATX(x)                   (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_DMATX_SHIFT)) & USART_FIFOCFG_DMATX_MASK)
#define USART_FIFOCFG_DMARX_MASK                 (0x2000U)
#define USART_FIFOCFG_DMARX_SHIFT                (13U)
/*! DMARX - DMA configuration for receive. 0: DMA is not used for the receive function. 1: Generate
 *    a DMA request for the receive function if the FIFO is not empty. Generally, data interrupts
 *    would be disabled if DMA is enabled.
 */
#define USART_FIFOCFG_DMARX(x)                   (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_DMARX_SHIFT)) & USART_FIFOCFG_DMARX_MASK)
#define USART_FIFOCFG_WAKETX_MASK                (0x4000U)
#define USART_FIFOCFG_WAKETX_SHIFT               (14U)
/*! WAKETX - Wakeup for transmit FIFO level. This allows the device to be woken from reduced power
 *    modes (up to power-down, as long as the peripheral function works in that power mode) without
 *    enabling the TXLVL interrupt. Only DMA wakes up, processes data, and goes back to sleep. The
 *    CPU will remain stopped until woken by another cause, such as DMA completion. 0: Only enabled
 *    interrupts will wake up the device form reduced power modes. 1: A device wake-up for DMA will
 *    occur if the transmit FIFO level reaches the value specified by TXLVL in FIFOTRIG, even when the
 *    TXLVL interrupt is not enabled.
 */
#define USART_FIFOCFG_WAKETX(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_WAKETX_SHIFT)) & USART_FIFOCFG_WAKETX_MASK)
#define USART_FIFOCFG_WAKERX_MASK                (0x8000U)
#define USART_FIFOCFG_WAKERX_SHIFT               (15U)
/*! WAKERX - Wakeup for receive FIFO level. This allows the device to be woken from reduced power
 *    modes (up to power-down, as long as the peripheral function works in that power mode) without
 *    enabling the TXLVL interrupt. Only DMA wakes up, processes data, and goes back to sleep. The CPU
 *    will remain stopped until woken by another cause, such as DMA completion. 0: Only enabled
 *    interrupts will wake up the device form reduced power modes. 1: A device wake-up for DMA will
 *    occur if the receive FIFO level reaches the value specified by RXLVL in FIFOTRIG, even when the
 *    RXLVL interrupt is not enabled.
 */
#define USART_FIFOCFG_WAKERX(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_WAKERX_SHIFT)) & USART_FIFOCFG_WAKERX_MASK)
#define USART_FIFOCFG_EMPTYTX_MASK               (0x10000U)
#define USART_FIFOCFG_EMPTYTX_SHIFT              (16U)
/*! EMPTYTX - Empty command for the transmit FIFO. When a 1 is written to this bit, the TX FIFO is emptied.
 */
#define USART_FIFOCFG_EMPTYTX(x)                 (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_EMPTYTX_SHIFT)) & USART_FIFOCFG_EMPTYTX_MASK)
#define USART_FIFOCFG_EMPTYRX_MASK               (0x20000U)
#define USART_FIFOCFG_EMPTYRX_SHIFT              (17U)
/*! EMPTYRX - Empty command for the receive FIFO. When a 1 is written to this bit, the RX FIFO is emptied.
 */
#define USART_FIFOCFG_EMPTYRX(x)                 (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_EMPTYRX_SHIFT)) & USART_FIFOCFG_EMPTYRX_MASK)
#define USART_FIFOCFG_POPDBG_MASK                (0x40000U)
#define USART_FIFOCFG_POPDBG_SHIFT               (18U)
/*! POPDBG - Pop FIFO for debug reads.
 */
#define USART_FIFOCFG_POPDBG(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOCFG_POPDBG_SHIFT)) & USART_FIFOCFG_POPDBG_MASK)
/*! @} */

/*! @name FIFOSTAT - FIFO status register. */
/*! @{ */
#define USART_FIFOSTAT_TXERR_MASK                (0x1U)
#define USART_FIFOSTAT_TXERR_SHIFT               (0U)
/*! TXERR - TX FIFO error. Will be set if a transmit FIFO error occurs. This could be an overflow
 *    caused by pushing data into a full FIFO, or by an underflow if the FIFO is empty when data is
 *    needed. Cleared by writing a 1 to this bit.
 */
#define USART_FIFOSTAT_TXERR(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_TXERR_SHIFT)) & USART_FIFOSTAT_TXERR_MASK)
#define USART_FIFOSTAT_RXERR_MASK                (0x2U)
#define USART_FIFOSTAT_RXERR_SHIFT               (1U)
/*! RXERR - RX FIFO error. Will be set if a receive FIFO overflow occurs, caused by software or DMA
 *    not emptying the FIFO fast enough. Cleared by writing a 1 to this bit.
 */
#define USART_FIFOSTAT_RXERR(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_RXERR_SHIFT)) & USART_FIFOSTAT_RXERR_MASK)
#define USART_FIFOSTAT_PERINT_MASK               (0x8U)
#define USART_FIFOSTAT_PERINT_SHIFT              (3U)
/*! PERINT - Peripheral interrupt. When 1, this indicates that the peripheral function has asserted
 *    an interrupt. The details can be found by reading the peripheral s STAT register.
 */
#define USART_FIFOSTAT_PERINT(x)                 (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_PERINT_SHIFT)) & USART_FIFOSTAT_PERINT_MASK)
#define USART_FIFOSTAT_TXEMPTY_MASK              (0x10U)
#define USART_FIFOSTAT_TXEMPTY_SHIFT             (4U)
/*! TXEMPTY - Transmit FIFO empty. When 1, the transmit FIFO is empty. The peripheral may still be processing the last piece of data.
 */
#define USART_FIFOSTAT_TXEMPTY(x)                (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_TXEMPTY_SHIFT)) & USART_FIFOSTAT_TXEMPTY_MASK)
#define USART_FIFOSTAT_TXNOTFULL_MASK            (0x20U)
#define USART_FIFOSTAT_TXNOTFULL_SHIFT           (5U)
/*! TXNOTFULL - Transmit FIFO not full. When 1, the transmit FIFO is not full, so more data can be
 *    written. When 0, the transmit FIFO is full and another write would cause it to overflow.
 */
#define USART_FIFOSTAT_TXNOTFULL(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_TXNOTFULL_SHIFT)) & USART_FIFOSTAT_TXNOTFULL_MASK)
#define USART_FIFOSTAT_RXNOTEMPTY_MASK           (0x40U)
#define USART_FIFOSTAT_RXNOTEMPTY_SHIFT          (6U)
/*! RXNOTEMPTY - Receive FIFO not empty. When 1, the receive FIFO is not empty, so data can be read. When 0, the receive FIFO is empty.
 */
#define USART_FIFOSTAT_RXNOTEMPTY(x)             (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_RXNOTEMPTY_SHIFT)) & USART_FIFOSTAT_RXNOTEMPTY_MASK)
#define USART_FIFOSTAT_RXFULL_MASK               (0x80U)
#define USART_FIFOSTAT_RXFULL_SHIFT              (7U)
/*! RXFULL - Receive FIFO full. When 1, the receive FIFO is full. Data needs to be read out to
 *    prevent the peripheral from causing an overflow.
 */
#define USART_FIFOSTAT_RXFULL(x)                 (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_RXFULL_SHIFT)) & USART_FIFOSTAT_RXFULL_MASK)
#define USART_FIFOSTAT_TXLVL_MASK                (0x1F00U)
#define USART_FIFOSTAT_TXLVL_SHIFT               (8U)
/*! TXLVL - Transmit FIFO current level. A 0 means the TX FIFO is currently empty, and the TXEMPTY
 *    and TXNOTFULL flags will be 1. Other values tell how much data is actually in the TX FIFO at
 *    the point where the read occurs. If the TX FIFO is full, the TXEMPTY and TXNOTFULL flags will be
 *    0.
 */
#define USART_FIFOSTAT_TXLVL(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_TXLVL_SHIFT)) & USART_FIFOSTAT_TXLVL_MASK)
#define USART_FIFOSTAT_RXLVL_MASK                (0x1F0000U)
#define USART_FIFOSTAT_RXLVL_SHIFT               (16U)
/*! RXLVL - Receive FIFO current level. A 0 means the RX FIFO is currently empty, and the RXFULL and
 *    RXNOTEMPTY flags will be 0. Other values tell how much data is actually in the RX FIFO at the
 *    point where the read occurs. If the RX FIFO is full, the RXFULL and RXNOTEMPTY flags will be
 *    1.
 */
#define USART_FIFOSTAT_RXLVL(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOSTAT_RXLVL_SHIFT)) & USART_FIFOSTAT_RXLVL_MASK)
/*! @} */

/*! @name FIFOTRIG - FIFO trigger settings for interrupt and DMA request. */
/*! @{ */
#define USART_FIFOTRIG_TXLVLENA_MASK             (0x1U)
#define USART_FIFOTRIG_TXLVLENA_SHIFT            (0U)
/*! TXLVLENA - Transmit FIFO level trigger enable. This trigger will become an interrupt if enabled
 *    in FIFOINTENSET, or a DMA trigger if DMATX in FIFOCFG is set. 0: Transmit FIFO level does not
 *    generate a FIFO level trigger. 1: An interrupt will be generated if the transmit FIFO level
 *    reaches the value specified by the TXLVL field in this register.
 */
#define USART_FIFOTRIG_TXLVLENA(x)               (((uint32_t)(((uint32_t)(x)) << USART_FIFOTRIG_TXLVLENA_SHIFT)) & USART_FIFOTRIG_TXLVLENA_MASK)
#define USART_FIFOTRIG_RXLVLENA_MASK             (0x2U)
#define USART_FIFOTRIG_RXLVLENA_SHIFT            (1U)
/*! RXLVLENA - Receive FIFO level trigger enable. This trigger will become an interrupt if enabled
 *    in FIFOINTENSET, or a DMA trigger if DMARX in FIFOCFG is set. 0: Receive FIFO level does not
 *    generate a FIFO level trigger. 1: An interrupt will be generated if the receive FIFO level
 *    reaches the value specified by the RXLVL field in this register.
 */
#define USART_FIFOTRIG_RXLVLENA(x)               (((uint32_t)(((uint32_t)(x)) << USART_FIFOTRIG_RXLVLENA_SHIFT)) & USART_FIFOTRIG_RXLVLENA_MASK)
#define USART_FIFOTRIG_TXLVL_MASK                (0xF00U)
#define USART_FIFOTRIG_TXLVL_SHIFT               (8U)
/*! TXLVL - Transmit FIFO level trigger point. This field is used only when TXLVLENA = 1. 0: trigger
 *    when the TX FIFO becomes empty. 1: trigger when the TX FIFO level decreases to one entry. ...
 *    3: trigger when the TX FIFO level decreases to 3 entries (is no longer full).
 */
#define USART_FIFOTRIG_TXLVL(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOTRIG_TXLVL_SHIFT)) & USART_FIFOTRIG_TXLVL_MASK)
#define USART_FIFOTRIG_RXLVL_MASK                (0xF0000U)
#define USART_FIFOTRIG_RXLVL_SHIFT               (16U)
/*! RXLVL - Receive FIFO level trigger point. The RX FIFO level is checked when a new piece of data
 *    is received. This field is used only when RXLVLENA = 1. 0: trigger when the RX FIFO has
 *    received one entry (is no longer empty). 1: trigger when the RX FIFO has received two entries. ...
 *    3: trigger when the RX FIFO has received 4 entries (has become full).
 */
#define USART_FIFOTRIG_RXLVL(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFOTRIG_RXLVL_SHIFT)) & USART_FIFOTRIG_RXLVL_MASK)
/*! @} */

/*! @name FIFOINTENSET - FIFO interrupt enable set (enable) and read register. */
/*! @{ */
#define USART_FIFOINTENSET_TXERR_MASK            (0x1U)
#define USART_FIFOINTENSET_TXERR_SHIFT           (0U)
/*! TXERR - Determines whether an interrupt occurs when a transmit error occurs, based on the TXERR
 *    flag in the FIFOSTAT register. 0: No interrupt will be generated for a transmit error. 1: An
 *    interrupt will be generated when a transmit error occurs.
 */
#define USART_FIFOINTENSET_TXERR(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTENSET_TXERR_SHIFT)) & USART_FIFOINTENSET_TXERR_MASK)
#define USART_FIFOINTENSET_RXERR_MASK            (0x2U)
#define USART_FIFOINTENSET_RXERR_SHIFT           (1U)
/*! RXERR - Determines whether an interrupt occurs when a receive error occurs, based on the RXERR
 *    flag in the FIFOSTAT register. 0: No interrupt will be generated for a receive error. 1: An
 *    interrupt will be generated when a receive error occurs.
 */
#define USART_FIFOINTENSET_RXERR(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTENSET_RXERR_SHIFT)) & USART_FIFOINTENSET_RXERR_MASK)
#define USART_FIFOINTENSET_TXLVL_MASK            (0x4U)
#define USART_FIFOINTENSET_TXLVL_SHIFT           (2U)
/*! TXLVL - Determines whether an interrupt occurs when a the transmit FIFO reaches the level
 *    specified by the TXLVL field in the FIFOTRIG register. 0: No interrupt will be generated based on
 *    the TX FIFO level. 1: TXLVLENA in the FIFOTRIG register = 1, an interrupt will be generated when
 *    the TX FIFO level decreases to the level specified by TXLVL in the FIFOTRIG register.
 */
#define USART_FIFOINTENSET_TXLVL(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTENSET_TXLVL_SHIFT)) & USART_FIFOINTENSET_TXLVL_MASK)
#define USART_FIFOINTENSET_RXLVL_MASK            (0x8U)
#define USART_FIFOINTENSET_RXLVL_SHIFT           (3U)
/*! RXLVL - Determines whether an interrupt occurs when a the receive FIFO reaches the level
 *    specified by the TXLVL field in the FIFOTRIG register. 0: No interrupt will be generated based on the
 *    RX FIFO level. 1: If RXLVLENA in the FIFOTRIG register = 1, an interrupt will be generated
 *    when the when the RX FIFO level increases to the level specified by RXLVL in the FIFOTRIG
 *    register.
 */
#define USART_FIFOINTENSET_RXLVL(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTENSET_RXLVL_SHIFT)) & USART_FIFOINTENSET_RXLVL_MASK)
/*! @} */

/*! @name FIFOINTENCLR - FIFO interrupt enable clear (disable) and read register. */
/*! @{ */
#define USART_FIFOINTENCLR_TXERR_MASK            (0x1U)
#define USART_FIFOINTENCLR_TXERR_SHIFT           (0U)
/*! TXERR - Writing 1 clears the corresponding bit in the FIFOINTENSET register
 */
#define USART_FIFOINTENCLR_TXERR(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTENCLR_TXERR_SHIFT)) & USART_FIFOINTENCLR_TXERR_MASK)
#define USART_FIFOINTENCLR_RXERR_MASK            (0x2U)
#define USART_FIFOINTENCLR_RXERR_SHIFT           (1U)
/*! RXERR - Writing 1 clears the corresponding bit in the FIFOINTENSET register
 */
#define USART_FIFOINTENCLR_RXERR(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTENCLR_RXERR_SHIFT)) & USART_FIFOINTENCLR_RXERR_MASK)
#define USART_FIFOINTENCLR_TXLVL_MASK            (0x4U)
#define USART_FIFOINTENCLR_TXLVL_SHIFT           (2U)
/*! TXLVL - Writing 1 clears the corresponding bit in the FIFOINTENSET register
 */
#define USART_FIFOINTENCLR_TXLVL(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTENCLR_TXLVL_SHIFT)) & USART_FIFOINTENCLR_TXLVL_MASK)
#define USART_FIFOINTENCLR_RXLVL_MASK            (0x8U)
#define USART_FIFOINTENCLR_RXLVL_SHIFT           (3U)
/*! RXLVL - Writing 1 clears the corresponding bit in the FIFOINTENSET register
 */
#define USART_FIFOINTENCLR_RXLVL(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTENCLR_RXLVL_SHIFT)) & USART_FIFOINTENCLR_RXLVL_MASK)
/*! @} */

/*! @name FIFOINTSTAT - FIFO interrupt status register. Reflects the status of interrupts that are currently enabled. */
/*! @{ */
#define USART_FIFOINTSTAT_TXERR_MASK             (0x1U)
#define USART_FIFOINTSTAT_TXERR_SHIFT            (0U)
/*! TXERR - TX FIFO error.
 */
#define USART_FIFOINTSTAT_TXERR(x)               (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTSTAT_TXERR_SHIFT)) & USART_FIFOINTSTAT_TXERR_MASK)
#define USART_FIFOINTSTAT_RXERR_MASK             (0x2U)
#define USART_FIFOINTSTAT_RXERR_SHIFT            (1U)
/*! RXERR - RX FIFO error.
 */
#define USART_FIFOINTSTAT_RXERR(x)               (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTSTAT_RXERR_SHIFT)) & USART_FIFOINTSTAT_RXERR_MASK)
#define USART_FIFOINTSTAT_TXLVL_MASK             (0x4U)
#define USART_FIFOINTSTAT_TXLVL_SHIFT            (2U)
/*! TXLVL - Transmit FIFO level interrupt.
 */
#define USART_FIFOINTSTAT_TXLVL(x)               (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTSTAT_TXLVL_SHIFT)) & USART_FIFOINTSTAT_TXLVL_MASK)
#define USART_FIFOINTSTAT_RXLVL_MASK             (0x8U)
#define USART_FIFOINTSTAT_RXLVL_SHIFT            (3U)
/*! RXLVL - Receive FIFO level interrupt.
 */
#define USART_FIFOINTSTAT_RXLVL(x)               (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTSTAT_RXLVL_SHIFT)) & USART_FIFOINTSTAT_RXLVL_MASK)
#define USART_FIFOINTSTAT_PERINT_MASK            (0x10U)
#define USART_FIFOINTSTAT_PERINT_SHIFT           (4U)
/*! PERINT - Peripheral interrupt.
 */
#define USART_FIFOINTSTAT_PERINT(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFOINTSTAT_PERINT_SHIFT)) & USART_FIFOINTSTAT_PERINT_MASK)
/*! @} */

/*! @name FIFOWR - FIFO write data. Used to write values to be transmitted to the FIFO. */
/*! @{ */
#define USART_FIFOWR_TXDATA_MASK                 (0x1FFU)
#define USART_FIFOWR_TXDATA_SHIFT                (0U)
/*! TXDATA - Transmit data to the FIFO. The number of bits used depends on the DATALEN
 */
#define USART_FIFOWR_TXDATA(x)                   (((uint32_t)(((uint32_t)(x)) << USART_FIFOWR_TXDATA_SHIFT)) & USART_FIFOWR_TXDATA_MASK)
/*! @} */

/*! @name FIFORD - FIFO read data. Used to read values that have been received. */
/*! @{ */
#define USART_FIFORD_RXDATA_MASK                 (0x1FFU)
#define USART_FIFORD_RXDATA_SHIFT                (0U)
/*! RXDATA - Received data from the FIFO. The number of bits used depends on the DATALEN and PARITYSEL settings.
 */
#define USART_FIFORD_RXDATA(x)                   (((uint32_t)(((uint32_t)(x)) << USART_FIFORD_RXDATA_SHIFT)) & USART_FIFORD_RXDATA_MASK)
#define USART_FIFORD_FRAMERR_MASK                (0x2000U)
#define USART_FIFORD_FRAMERR_SHIFT               (13U)
/*! FRAMERR - Framing Error status flag. This bit reflects the status for the data it is read along
 *    with from the FIFO, and indicates that the character was received with a missing stop bit at
 *    the expected location. This could be an indication of a baud rate or configuration mismatch
 *    with the transmitting source.
 */
#define USART_FIFORD_FRAMERR(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFORD_FRAMERR_SHIFT)) & USART_FIFORD_FRAMERR_MASK)
#define USART_FIFORD_PARITYERR_MASK              (0x4000U)
#define USART_FIFORD_PARITYERR_SHIFT             (14U)
/*! PARITYERR - Parity Error status flag. This bit reflects the status for the data it is read along
 *    with from the FIFO. This bit will be set when a parity error is detected in a received
 *    character.
 */
#define USART_FIFORD_PARITYERR(x)                (((uint32_t)(((uint32_t)(x)) << USART_FIFORD_PARITYERR_SHIFT)) & USART_FIFORD_PARITYERR_MASK)
#define USART_FIFORD_RXNOISE_MASK                (0x8000U)
#define USART_FIFORD_RXNOISE_SHIFT               (15U)
/*! RXNOISE - Received Noise flag.
 */
#define USART_FIFORD_RXNOISE(x)                  (((uint32_t)(((uint32_t)(x)) << USART_FIFORD_RXNOISE_SHIFT)) & USART_FIFORD_RXNOISE_MASK)
/*! @} */

/*! @name FIFORDNOPOP - FIFO data read with no FIFO pop. This register acts in exactly the same way as FIFORD, except that it supplies data from the top of the FIFO without popping the FIFO (i.e. leaving the FIFO state unchanged). This could be used to allow system software to observe incoming data without interfering with the peripheral driver. */
/*! @{ */
#define USART_FIFORDNOPOP_RXDATA_MASK            (0x1FFU)
#define USART_FIFORDNOPOP_RXDATA_SHIFT           (0U)
/*! RXDATA - Received data from the FIFO. The number of bits used depends on the DATALEN and PARITYSEL settings.
 */
#define USART_FIFORDNOPOP_RXDATA(x)              (((uint32_t)(((uint32_t)(x)) << USART_FIFORDNOPOP_RXDATA_SHIFT)) & USART_FIFORDNOPOP_RXDATA_MASK)
#define USART_FIFORDNOPOP_FRAMERR_MASK           (0x2000U)
#define USART_FIFORDNOPOP_FRAMERR_SHIFT          (13U)
/*! FRAMERR - Framing Error status flag. This bit reflects the status for the data it is read along
 *    with from the FIFO, and indicates that the character was received with a missing stop bit at
 *    the expected location. This could be an indication of a baud rate or configuration mismatch
 *    with the transmitting source.
 */
#define USART_FIFORDNOPOP_FRAMERR(x)             (((uint32_t)(((uint32_t)(x)) << USART_FIFORDNOPOP_FRAMERR_SHIFT)) & USART_FIFORDNOPOP_FRAMERR_MASK)
#define USART_FIFORDNOPOP_PARITYERR_MASK         (0x4000U)
#define USART_FIFORDNOPOP_PARITYERR_SHIFT        (14U)
/*! PARITYERR - Parity Error status flag. This bit reflects the status for the data it is read along
 *    with from the FIFO. This bit will be set when a parity error is detected in a received
 *    character.
 */
#define USART_FIFORDNOPOP_PARITYERR(x)           (((uint32_t)(((uint32_t)(x)) << USART_FIFORDNOPOP_PARITYERR_SHIFT)) & USART_FIFORDNOPOP_PARITYERR_MASK)
#define USART_FIFORDNOPOP_RXNOISE_MASK           (0x8000U)
#define USART_FIFORDNOPOP_RXNOISE_SHIFT          (15U)
/*! RXNOISE - Received Noise flag.
 */
#define USART_FIFORDNOPOP_RXNOISE(x)             (((uint32_t)(((uint32_t)(x)) << USART_FIFORDNOPOP_RXNOISE_SHIFT)) & USART_FIFORDNOPOP_RXNOISE_MASK)
/*! @} */

/*! @name PSELID - Flexcomm ID and peripheral function select register */
/*! @{ */
#define USART_PSELID_PERSEL_MASK                 (0x7U)
#define USART_PSELID_PERSEL_SHIFT                (0U)
/*! PERSEL - Peripheral Select. This field is writable by software. 0x0: No peripheral selected.
 *    0x1: USART function selected. 0x2 - 0x7: Reserved.
 */
#define USART_PSELID_PERSEL(x)                   (((uint32_t)(((uint32_t)(x)) << USART_PSELID_PERSEL_SHIFT)) & USART_PSELID_PERSEL_MASK)
#define USART_PSELID_LOCK_MASK                   (0x8U)
#define USART_PSELID_LOCK_SHIFT                  (3U)
/*! LOCK - Lock the peripheral select. This field is writable by software. 0: Peripheral select can
 *    be changed by software. 1: Peripheral select is locked and cannot be changed until this
 *    Flexcomm or the entire device is reset.
 */
#define USART_PSELID_LOCK(x)                     (((uint32_t)(((uint32_t)(x)) << USART_PSELID_LOCK_SHIFT)) & USART_PSELID_LOCK_MASK)
#define USART_PSELID_USARTPRESENT_MASK           (0x10U)
#define USART_PSELID_USARTPRESENT_SHIFT          (4U)
/*! USARTPRESENT - USART present indicator. This field is Read-only. 0: This Flexcomm does not
 *    include the USART function. 1: This Flexcomm includes the USART function.
 */
#define USART_PSELID_USARTPRESENT(x)             (((uint32_t)(((uint32_t)(x)) << USART_PSELID_USARTPRESENT_SHIFT)) & USART_PSELID_USARTPRESENT_MASK)
#define USART_PSELID_ID_MASK                     (0xFFFFF000U)
#define USART_PSELID_ID_SHIFT                    (12U)
/*! ID - Flexcomm ID.
 */
#define USART_PSELID_ID(x)                       (((uint32_t)(((uint32_t)(x)) << USART_PSELID_ID_SHIFT)) & USART_PSELID_ID_MASK)
/*! @} */

/*! @name ID - USART Module Identifier */
/*! @{ */
#define USART_ID_APERTURE_MASK                   (0xFFU)
#define USART_ID_APERTURE_SHIFT                  (0U)
/*! APERTURE - Aperture i.e. number minus 1 of consecutive packets 4 Kbytes reserved for this IP
 */
#define USART_ID_APERTURE(x)                     (((uint32_t)(((uint32_t)(x)) << USART_ID_APERTURE_SHIFT)) & USART_ID_APERTURE_MASK)
#define USART_ID_MIN_REV_MASK                    (0xF00U)
#define USART_ID_MIN_REV_SHIFT                   (8U)
/*! MIN_REV - Minor revision i.e. with no software consequences
 */
#define USART_ID_MIN_REV(x)                      (((uint32_t)(((uint32_t)(x)) << USART_ID_MIN_REV_SHIFT)) & USART_ID_MIN_REV_MASK)
#define USART_ID_MAJ_REV_MASK                    (0xF000U)
#define USART_ID_MAJ_REV_SHIFT                   (12U)
/*! MAJ_REV - Major revision i.e. implies software modifications
 */
#define USART_ID_MAJ_REV(x)                      (((uint32_t)(((uint32_t)(x)) << USART_ID_MAJ_REV_SHIFT)) & USART_ID_MAJ_REV_MASK)
#define USART_ID_ID_MASK                         (0xFFFF0000U)
#define USART_ID_ID_SHIFT                        (16U)
/*! ID - Identifier. This is the unique identifier of the module
 */
#define USART_ID_ID(x)                           (((uint32_t)(((uint32_t)(x)) << USART_ID_ID_SHIFT)) & USART_ID_ID_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group USART_Register_Masks */


/* USART - Peripheral instance base addresses */
/** Peripheral USART0 base address */
#define USART0_BASE                              (0x4008B000u)
/** Peripheral USART0 base pointer */
#define USART0                                   ((USART_Type *)USART0_BASE)
/** Peripheral USART1 base address */
#define USART1_BASE                              (0x4008C000u)
/** Peripheral USART1 base pointer */
#define USART1                                   ((USART_Type *)USART1_BASE)
/** Array initializer of USART peripheral base addresses */
#define USART_BASE_ADDRS                         { USART0_BASE, USART1_BASE }
/** Array initializer of USART peripheral base pointers */
#define USART_BASE_PTRS                          { USART0, USART1 }
/** Interrupt vectors for the USART peripheral type */
#define USART_IRQS                               { FLEXCOMM0_IRQn, FLEXCOMM1_IRQn }

/*!
 * @}
 */ /* end of group USART_Peripheral_Access_Layer */


/* ----------------------------------------------------------------------------
   -- WWDT Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup WWDT_Peripheral_Access_Layer WWDT Peripheral Access Layer
 * @{
 */

/** WWDT - Register Layout Typedef */
typedef struct {
  __IO uint32_t MOD;                               /**< Watchdog mode register. This register contains the basic mode and status of the Watchdog Timer., offset: 0x0 */
  __IO uint32_t TC;                                /**< Watchdog timer constant register. This 24-bit register determines the time-out value., offset: 0x4 */
  __O  uint32_t FEED;                              /**< Watchdog feed sequence register. Writing 0xAA followed by 0x55 to this register reloads the Watchdog timer with the value contained in TC., offset: 0x8 */
  __I  uint32_t TV;                                /**< Watchdog timer value register. This 24-bit register reads out the current value of the Watchdog timer., offset: 0xC */
       uint8_t RESERVED_0[4];
  __IO uint32_t WARNINT;                           /**< Watchdog Warning Interrupt compare value., offset: 0x14 */
  __IO uint32_t WINDOW;                            /**< Watchdog Window compare value., offset: 0x18 */
} WWDT_Type;

/* ----------------------------------------------------------------------------
   -- WWDT Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup WWDT_Register_Masks WWDT Register Masks
 * @{
 */

/*! @name MOD - Watchdog mode register. This register contains the basic mode and status of the Watchdog Timer. */
/*! @{ */
#define WWDT_MOD_WDEN_MASK                       (0x1U)
#define WWDT_MOD_WDEN_SHIFT                      (0U)
/*! WDEN - Watchdog enable bit. Once this bit is set to one and a watchdog feed is performed, the
 *    watchdog timer will run permanently. 0: The watchdog timer is stopped. 1: The watchdog timer is
 *    running.
 */
#define WWDT_MOD_WDEN(x)                         (((uint32_t)(((uint32_t)(x)) << WWDT_MOD_WDEN_SHIFT)) & WWDT_MOD_WDEN_MASK)
#define WWDT_MOD_WDRESET_MASK                    (0x2U)
#define WWDT_MOD_WDRESET_SHIFT                   (1U)
/*! WDRESET - Watchdog reset enable bit. Once this bit has been written with a 1 it cannot be
 *    re-written with a 0. 0: Interrupt. A watchdog time-out will not cause a chip reset. It will cause an
 *    interrupt of the watchdog. 1: Reset. A watchdog time-out will cause a chip reset.
 */
#define WWDT_MOD_WDRESET(x)                      (((uint32_t)(((uint32_t)(x)) << WWDT_MOD_WDRESET_SHIFT)) & WWDT_MOD_WDRESET_MASK)
#define WWDT_MOD_WDTOF_MASK                      (0x4U)
#define WWDT_MOD_WDTOF_SHIFT                     (2U)
/*! WDTOF - Watchdog time-out flag. Set when the watchdog timer times out, by a feed error, or by
 *    events associated with WDPROTECT. Cleared by software writing a 0 to this bit position. Causes a
 *    chip reset if WDRESET = 1.
 */
#define WWDT_MOD_WDTOF(x)                        (((uint32_t)(((uint32_t)(x)) << WWDT_MOD_WDTOF_SHIFT)) & WWDT_MOD_WDTOF_MASK)
#define WWDT_MOD_WDINT_MASK                      (0x8U)
#define WWDT_MOD_WDINT_SHIFT                     (3U)
/*! WDINT - Warning interrupt flag. Set when the timer reaches the value in WDWARNINT. Cleared by
 *    software writing a 1 to this bit position. Note that this bit cannot be cleared while the
 *    WARNINT value is equal to the value of the TV register. This can occur if the value of WARNINT is 0
 *    and the WDRESET bit is 0 when TV decrements to 0.
 */
#define WWDT_MOD_WDINT(x)                        (((uint32_t)(((uint32_t)(x)) << WWDT_MOD_WDINT_SHIFT)) & WWDT_MOD_WDINT_MASK)
#define WWDT_MOD_WDPROTECT_MASK                  (0x10U)
#define WWDT_MOD_WDPROTECT_SHIFT                 (4U)
/*! WDPROTECT - Watchdog update mode. This bit can be set once by software and is only cleared by a
 *    reset. 0: Flexible. A feed sequence can be performed at any time; ie. the watchdog timer can
 *    be reloaded with time-out value (TC) at any time. 1: Threshold. A feed sequence can be
 *    performed only after the counter is below the value of WDWARNINT and WDWINDOW; ie. the watchdog timer
 *    can be reloaded with time-out value (TC) only when the counter timer value is below the value
 *    of WDWARNINT and WDWINDOW, otherwise a 'feed error' is created.
 */
#define WWDT_MOD_WDPROTECT(x)                    (((uint32_t)(((uint32_t)(x)) << WWDT_MOD_WDPROTECT_SHIFT)) & WWDT_MOD_WDPROTECT_MASK)
/*! @} */

/*! @name TC - Watchdog timer constant register. This 24-bit register determines the time-out value. */
/*! @{ */
#define WWDT_TC_COUNT_MASK                       (0xFFFFFFU)
#define WWDT_TC_COUNT_SHIFT                      (0U)
/*! COUNT - Watchdog time-out value. If MOD.WDPROTECT is set then changing this value may cause an error.
 */
#define WWDT_TC_COUNT(x)                         (((uint32_t)(((uint32_t)(x)) << WWDT_TC_COUNT_SHIFT)) & WWDT_TC_COUNT_MASK)
/*! @} */

/*! @name FEED - Watchdog feed sequence register. Writing 0xAA followed by 0x55 to this register reloads the Watchdog timer with the value contained in TC. */
/*! @{ */
#define WWDT_FEED_FEED_MASK                      (0xFFU)
#define WWDT_FEED_FEED_SHIFT                     (0U)
/*! FEED - Feed value should be 0xAA followed by 0x55. Writing 0xAA followed by 0x55 to this
 *    register will reload the Watchdog timer with the TC value. This operation will also start the
 *    Watchdog if it is enabled via the WDMOD register. Setting the WDEN bit in the WDMOD register is not
 *    sufficient to enable the Watchdog. A valid feed sequence must be completed after setting WDEN
 *    before the Watchdog is capable of generating a reset. Until then, the Watchdog will ignore feed
 *    errors.
 */
#define WWDT_FEED_FEED(x)                        (((uint32_t)(((uint32_t)(x)) << WWDT_FEED_FEED_SHIFT)) & WWDT_FEED_FEED_MASK)
/*! @} */

/*! @name TV - Watchdog timer value register. This 24-bit register reads out the current value of the Watchdog timer. */
/*! @{ */
#define WWDT_TV_COUNT_MASK                       (0xFFFFFFU)
#define WWDT_TV_COUNT_SHIFT                      (0U)
/*! COUNT - Counter timer value. The TV register is used to read the current value of Watchdog timer counter.
 */
#define WWDT_TV_COUNT(x)                         (((uint32_t)(((uint32_t)(x)) << WWDT_TV_COUNT_SHIFT)) & WWDT_TV_COUNT_MASK)
/*! @} */

/*! @name WARNINT - Watchdog Warning Interrupt compare value. */
/*! @{ */
#define WWDT_WARNINT_WARNINT_MASK                (0x3FFU)
#define WWDT_WARNINT_WARNINT_SHIFT               (0U)
/*! WARNINT - Watchdog warning interrupt compare value.A match of the watchdog timer counter to
 *    WARNINT occurs when the bottom 10 bits of the counter have the same value as the 10 bits of
 *    WARNINT, and the remaining upper bits of the counter are all 0. This gives a maximum time of 1,023
 *    watchdog timer counts (4,096 watchdog clocks) for the interrupt to occur prior to a watchdog
 *    event. If WARNINT is 0, the interrupt will occur at the same time as the watchdog event.
 */
#define WWDT_WARNINT_WARNINT(x)                  (((uint32_t)(((uint32_t)(x)) << WWDT_WARNINT_WARNINT_SHIFT)) & WWDT_WARNINT_WARNINT_MASK)
/*! @} */

/*! @name WINDOW - Watchdog Window compare value. */
/*! @{ */
#define WWDT_WINDOW_WINDOW_MASK                  (0xFFFFFFU)
#define WWDT_WINDOW_WINDOW_SHIFT                 (0U)
/*! WINDOW - Watchdog window value. The WINDOW register determines the highest TV value allowed when
 *    a watchdog feed is performed. If a feed sequence occurs when TV is greater than the value in
 *    WINDOW, a watchdog event will occur
 */
#define WWDT_WINDOW_WINDOW(x)                    (((uint32_t)(((uint32_t)(x)) << WWDT_WINDOW_WINDOW_SHIFT)) & WWDT_WINDOW_WINDOW_MASK)
/*! @} */


/*!
 * @}
 */ /* end of group WWDT_Register_Masks */


/* WWDT - Peripheral instance base addresses */
/** Peripheral WWDT base address */
#define WWDT_BASE                                (0x4000A000u)
/** Peripheral WWDT base pointer */
#define WWDT                                     ((WWDT_Type *)WWDT_BASE)
/** Array initializer of WWDT peripheral base addresses */
#define WWDT_BASE_ADDRS                          { WWDT_BASE }
/** Array initializer of WWDT peripheral base pointers */
#define WWDT_BASE_PTRS                           { WWDT }
/** Interrupt vectors for the WWDT peripheral type */
#define WWDT_IRQS                                { WDT_BOD_IRQn }

/*!
 * @}
 */ /* end of group WWDT_Peripheral_Access_Layer */


/*
** End of section using anonymous unions
*/

#if defined(__ARMCC_VERSION)
  #if (__ARMCC_VERSION >= 6010050)
    #pragma clang diagnostic pop
  #else
    #pragma pop
  #endif
#elif defined(__GNUC__)
  /* leave anonymous unions enabled */
#elif defined(__IAR_SYSTEMS_ICC__)
  #pragma language=default
#else
  #error Not supported compiler type
#endif

/*!
 * @}
 */ /* end of group Peripheral_access_layer */


/* ----------------------------------------------------------------------------
   -- Macros for use with bit field definitions (xxx_SHIFT, xxx_MASK).
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup Bit_Field_Generic_Macros Macros for use with bit field definitions (xxx_SHIFT, xxx_MASK).
 * @{
 */

#if defined(__ARMCC_VERSION)
  #if (__ARMCC_VERSION >= 6010050)
    #pragma clang system_header
  #endif
#elif defined(__IAR_SYSTEMS_ICC__)
  #pragma system_include
#endif

/**
 * @brief Mask and left-shift a bit field value for use in a register bit range.
 * @param field Name of the register bit field.
 * @param value Value of the bit field.
 * @return Masked and shifted value.
 */
#define NXP_VAL2FLD(field, value)    (((value) << (field ## _SHIFT)) & (field ## _MASK))
/**
 * @brief Mask and right-shift a register value to extract a bit field value.
 * @param field Name of the register bit field.
 * @param value Value of the register.
 * @return Masked and shifted bit field value.
 */
#define NXP_FLD2VAL(field, value)    (((value) & (field ## _MASK)) >> (field ## _SHIFT))

/*!
 * @}
 */ /* end of group Bit_Field_Generic_Macros */


/* ----------------------------------------------------------------------------
   -- SDK Compatibility
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup SDK_Compatibility_Symbols SDK Compatibility
 * @{
 */

#define USART0_IRQn FLEXCOMM0_IRQn
#define USART1_IRQn FLEXCOMM1_IRQn
#define I2C0_IRQn FLEXCOMM2_IRQn
#define I2C1_IRQn FLEXCOMM3_IRQn
#define SPI0_IRQn FLEXCOMM4_IRQn
#define SPI1_IRQn FLEXCOMM5_IRQn
#define I2C2_IRQn FLEXCOMM6_IRQn
#define DMA_IRQn DMA0_IRQn
#define GINT_IRQn GINT0_IRQn
#define PINT0_IRQn PIN_INT0_IRQn
#define PINT1_IRQn PIN_INT1_IRQn
#define PINT2_IRQn PIN_INT2_IRQn
#define PINT3_IRQn PIN_INT3_IRQn
#define SPIFI_IRQn SPIFI0_IRQn
#define GINT_IRQn GINT0_IRQn
#define Timer0_IRQn CTIMER0_IRQn
#define Timer1_IRQn CTIMER1_IRQn
#define DMIC_IRQn DMIC0_IRQn
#define HWVAD_IRQn HWVAD0_IRQn
#define AES256_Type AES_Type
#define NTAG_IRQ NFCTag_IRQn
#define PRESETCTRL PRESETCTRLS
#define PRESETCTRLSET PRESETCTRLSETS
#define PRESETCTRLCLR PRESETCTRLCLRS
#define AHBCLKCTRL AHBCLKCTRLS
#define AHBCLKCTRLSET AHBCLKCTRLSETS
#define AHBCLKCTRLCLR AHBCLKCTRLCLRS
#define STARTER STARTERS
#define STARTERSET STARTERSETS
#define STARTERCLR STARTERCLRS

/*!
 * @}
 */ /* end of group SDK_Compatibility_Symbols */


#endif  /* _K32W061_H_ */

