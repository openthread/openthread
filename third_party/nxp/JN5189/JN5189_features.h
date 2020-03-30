/*
** ###################################################################
**     Version:             rev. 1.0, 2016-05-09
**     Build:               b160714
**
**     Abstract:
**         Chip specific module features.
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
**     - rev. 1.0 (2016-05-09)
**         Initial version.
**
** ###################################################################
*/

#ifndef _JN518X_FEATURES_H_
#define _JN518X_FEATURES_H_

/* SOC module features */

/* @brief ADC availability on the SoC. */
#define FSL_FEATURE_SOC_ADC_COUNT (1)
/* @brief ASYNC_SYSCON availability on the SoC. */
#define FSL_FEATURE_SOC_ASYNC_SYSCON_COUNT (1)
/* @brief CRC availability on the SoC. */
#define FSL_FEATURE_SOC_CRC_COUNT (1)
/* @brief DMA availability on the SoC. */
#define FSL_FEATURE_SOC_DMA_COUNT (1)
/* @brief Number of DMA channels on the SOC. */
#define FSL_FEATURE_DMA_NUMBER_OF_CHANNELS (19)
/* @brief Align size of DMA descriptor */
#define FSL_FEATURE_DMA_DESCRIPTOR_ALIGN_SIZE (512)
/* @brief DMA head link descriptor table align size */
#define FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE (16U)
/* @brief DMIC availability on the SoC. */
#define FSL_FEATURE_SOC_DMIC_COUNT (1)
/* @brief FLEXCOMM availability on the SoC. */
#define FSL_FEATURE_SOC_FLEXCOMM_COUNT (7)
/* @brief GINT availability on the SoC. */
#define FSL_FEATURE_SOC_GINT_COUNT (1)
/* @brief GPIO availability on the SoC. */
#define FSL_FEATURE_SOC_LPC_GPIO_COUNT (1)
#define FSL_FEATURE_SOC_GPIO_COUNT FSL_FEATURE_SOC_LPC_GPIO_COUNT
/* @brief I2C availability on the SoC. */
#define FSL_FEATURE_SOC_I2C_COUNT (3)
/* @brief I2C are FLEXCOMM on the SoC. */
#define FSL_FEATURE_SOC_FLEXCOMM_I2C_COUNT FSL_FEATURE_SOC_I2C_COUNT
/* @brief INPUTMUX availability on the SoC. */
#define FSL_FEATURE_SOC_INPUTMUX_COUNT (1)
/* @brief IOCON availability on the SoC. */
#define FSL_FEATURE_SOC_IOCON_COUNT (1)
/* @brief MAILBOX availability on the SoC. */
#define FSL_FEATURE_SOC_MAILBOX_COUNT (1)
/* @brief MRT availability on the SoC. */
#define FSL_FEATURE_SOC_MRT_COUNT (1)
/* @brief PINT availability on the SoC. */
#define FSL_FEATURE_SOC_PINT_COUNT (1)
/* @brief RTC availability on the SoC. */
#define FSL_FEATURE_SOC_LPC_RTC_COUNT (1)
/* @brief SCT availability on the SoC. */
#define FSL_FEATURE_SOC_SCT_COUNT (1)
/* @brief SPI availability on the SoC. */
#define FSL_FEATURE_SOC_SPI_COUNT (2)
/* @brief SPI are FLEXCOMM on the SoC. */
#define FSL_FEATURE_SOC_FLEXCOMM_SPI_COUNT FSL_FEATURE_SOC_SPI_COUNT
/* @brief SPIFI availability on the SoC. */
#define FSL_FEATURE_SOC_SPIFI_COUNT (1)
/* @brief SYSCON availability on the SoC. */
#define FSL_FEATURE_SOC_SYSCON_COUNT (1)
/* @brief CTIMER availability on the SoC. */
#define FSL_FEATURE_SOC_CTIMER_COUNT (2)
/* @brief USART availability on the SoC. */
#define FSL_FEATURE_SOC_USART_COUNT (2)
/* @brief USART availability on the SoC. */
#define FSL_FEATURE_SOC_FLEXCOMM_USART_COUNT FSL_FEATURE_SOC_USART_COUNT
/* @brief USB availability on the SoC. */
#define FSL_FEATURE_SOC_UTICK_COUNT (1)
/* @brief WWDT availability on the SoC. */
#define FSL_FEATURE_SOC_WWDT_COUNT (1)
/* @brief IRB availability on the SoC. */
#define FSL_FEATURE_SOC_CIC_IRB_COUNT (1)
/* @brief reduced functionality FLEXCOMM  for low power JN518x family */
#define FSL_REDUCED_FUNCTION_FLEXCOMM
/* @brief PWM availability on the Soc */
#define FSL_FEATURE_SOC_PWM_COUNT (10)

/* @brief HASH/SHA availability on the SoC. */
#define FSL_FEATURE_SOC_SHA_COUNT (1)
/* @brief AES availability on the SoC. */
#define FSL_FEATURE_SOC_AES_COUNT (1)

/* @brief RNG availability on the SoC. */
#define FSL_FEATURE_SOC_TRNG_COUNT (1)

/* SPIFI module features */
/* @brief SPIFI start address */
#define FSL_FEATURE_SPIFI_START_ADDR (0x10000000)
/* @brief SPIFI end address */
#define FSL_FEATURE_SPIFI_END_ADDR (0x17FFFFFF)
/* @brief SPIFI DATALEN bitfile in CMD register*/
#define FSL_FEATURE_SPIFI_DATALEN_CTRL (1)

/* @brief Pointer to ROM IAP entry functions */
#define FSL_FEATURE_SYSCON_IAP_ENTRY_LOCATION (0x03000205)

/* PINT module features */
/* @brief Number of connected outputs */
#define FSL_FEATURE_PINT_NUMBER_OF_CONNECTED_OUTPUTS (4)
/* @brief Number of PIOs  */
#define FSL_FEATURE_GPIO_PIO_COUNT (22)

/* PMC module features */
/* @brief FRO1M trim address */
#define FSL_FEATURE_PMC_FRO1M_ADDRESS (0x9FCD0U)
/* @brief FRO1M trim valid mask */
#define FSL_FEATURE_PMC_FRO1M_VALID_MASK (0x1U)

/* SPI module features */
/* @brief SPI SIZE bitfile in FIFOCFG register */
#define FSL_FEATURE_SPI_FIFOSIZE_CFG (1)
/* @brief SPI has only three SSEL pins */
#define FSL_FEATURE_IS_SPI_SSEL_PIN_COUNT_EQUAL_TO_THREE (1)

/* ADC module features */
/* @brief ADC data alignment mode */
#define FSL_FEATURE_ADC_DAT_OF_HIGH_ALIGNMENT (1)
/* @brief ADC synchronous mode do not modify CTRL.TSAMP [14:12] */
#define FSL_FEATURE_ADC_SYNCHRONOUS_USE_GPADC_CTRL (1)
/* @brief ADC temp cal flash addr */
#define FSL_FEATURE_FLASH_ADDR_OF_TEMP_CAL (0x9FC80)
/* @brief ADC temp cal flash data vaild mask*/
#define FSL_FEATURE_FLASH_ADDR_OF_TEMP_CAL_VALID (0x1)
/* @brief Has ASYNMODE bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_ASYNMODE (1)
/* @brief Has RESOL bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_RESOL (1)
/* @brief Has BYPASSCAL bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_BYPASSCAL (0)
/* @brief Has TSAMP bitfile in CTRL reigster. */
#define FSL_FEATURE_ADC_HAS_CTRL_TSAMP (1)
/* @brief ADC NO calibration*/
#define FSL_FEATURE_ADC_HAS_NO_CALIB_FUNC (1)
/* @brief ADC TEMPSENSORCTRL in ASYNC_SYSCON*/
#define FSL_FEATURE_ADC_ASYNC_SYSCON_TEMP (1)
/* @brief ADC require a delay*/
#define FSL_FEATURE_ADC_REQUIRE_DELAY (1)
/* @brief Has LDO_POWER_EN bitfile in GPADC_CTRL0 register. */
#define FSL_FEATURE_ADC_HAS_GPADC_CTRL0_LDO_POWER_EN (1)
/* @brief Has OFFSET_CAL bitfile in GPADC_CTRL1 register. */
#define FSL_FEATURE_ADC_HAS_GPADC_CTRL1_OFFSET_CAL (1)
/* @brief Has ADC_INIT bitfile in STARTUP register. */
#define FSL_FEATURE_ADC_HAS_STARTUP_ADC_INIT (0)
/* @brief ADC has single SEQ */
#define FSL_FEATURE_ADC_HAS_SINGLE_SEQ (1)
/* SYSCON module features */

/* FLASH module features */
/* @brief P-Flash write unit size. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE (512U)
/* @brief P-Flash sector size. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE (512U)
/* @brief P-Flash block count */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_COUNT (1)
/* @brief P-Flash block size. */
#define FSL_FEATURE_FLASH_PFLASH_BLOCK_SIZE (0xA0000U)

/* CTIMER module features */
/* @brief CTIMER capture channel 3. */
#define FSL_FEATURE_CTIMER_HAS_CCR_CAP3 (1)
/* @brief CTIMER capture 3 interrupt. */
#define FSL_FEATURE_CTIMER_HAS_IR_CR3INT (1)

/* WWDT module features */
/* @brief WWDT WDTOF is not set in case of WD reset - get info from PMC instead. */
#define FSL_FEATURE_WWDT_WDTRESET_FROM_PMC (1)
/* @brief WWDT NO PDCFG*/
#define FSL_FEATURE_WWDT_HAS_NO_PDCFG (1)

/* I2C module features */
/* @brief I2C peripheral clock frequency 8MHz */
#define FSL_FEATURE_I2C_PREPCLKFRG_8MHZ (1)

/* GPIO module feature */
/* @brief GPIO DIRSET and DIRCLR register */
#define FSL_FEATURE_GPIO_DIRSET_AND_DIRCLR (1)

/* FMEAS module feature */
/* @brief FMEAS FMEAS_INDEX is 20 */
#define FSL_FEATURE_FMEAS_INDEX_20 (1)
/* @brief FMEAS FREQMECTRL in ASYNC_SYSCON */
#define FSL_FEATURE_FMEAS_ASYNC_SYSCON_FREQMECTRL (1)
/* @brief FMEAS SYSCON use ASYNC_SYSCON */
#define FSL_FEATURE_FMEAS_USE_ASYNC_SYSCON (1)
/* @brief FMEAS start frequency with ASYNC_SYSCON */
#define FSL_FEATURE_FMEAS_START_FRG_ASYNC_SYSCON (1)
/* @brief FMEAS get frequency with ASYNC_SYSCON */
#define FSL_FEATURE_FMEAS_GET_FRG_ASYNC_SYSCON (1)
/* @brief FMEAS get clock count with scale */
#define FSL_FEATURE_FMEAS_GET_COUNT_SCALE (1)
/* @brief FMEAS start measure with scale */
#define FSL_FEATURE_FMEAS_STARTMEAS_SCALE (1)

/* RTC module features */
/* @brief Has no separate RTC OSC PD in CTRL register. */
#define FSL_FEATURE_RTC_HAS_NO_OSC_PD (1)

/* FLEXCOMM module features */
/* @brief Has no reset in FLEXCOMM register. */
#define FSL_FEATURE_FLEXCOMM_HAS_NO_RESET (1)

#define FSL_FEATURE_CTIMER_HAS_NO_INPUT_CAPTURE (1)
#endif /* _JN518X_FEATURES_H_ */
