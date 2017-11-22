/**
 * \addtogroup BSP
 * \{
 * \addtogroup BSP_CONFIG
 * \{
 * \addtogroup BSP_CONFIG_DEFAULTS
 *
 * \brief Board support package default configuration values
 *
 * The following tags are used to describe the type of each configuration option.
 *
 * - **\bsp_config_option_build**       : To be changed only in the build configuration
 *                                        of the project ("Defined symbols -D" in the
 *                                        preprocessor options).
 *
 * - **\bsp_config_option_app**         : To be changed only in the custom_config*.h
 *                                        project files.
 *
 * - **\bsp_config_option_expert_only** : To be changed only by an expert user.
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file bsp_defaults.h
 *
 * @brief Board Support Package. System Configuration file default values.
 *
 * Copyright (c) 2016, Dialog Semiconductor
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software without 
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ***************************************************************************************
*/

#ifndef BSP_DEFAULTS_H_
#define BSP_DEFAULTS_H_




/**
 * \addtogroup POWER_SETTINGS
 *
 * \brief Power configuration settings
 * \{
 */
/* -------------------------------------- Power settings ---------------------------------------- */

/*
 * This is a deprecated configuration hence not to be defined by an application.
 * There's a single power configuration setup.
 */
#ifdef dg_configPOWER_CONFIG
#error "Configuration option dg_configPOWER_CONFIG is no longer supported."
#endif

/**
 * \brief When set to 1, the system will go to sleep and never exit allowing for the sleep current to be
 *        measured.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configTESTMODE_MEASURE_SLEEP_CURRENT
#define dg_configTESTMODE_MEASURE_SLEEP_CURRENT (0)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup IMAGE_CONFIGURATION_SETTINGS
 *
 * \brief Image configuration settings.
 * \{
 */
/* ------------------------------- Image configuration settings --------------------------------- */

/**
 * \brief The chip revision that we compile for.
 *
 * \note There is no default value defined for the target chip revision. The application must\n
 *       neither explicitly set dg_configBLACK_ORCA_IC_REV, nor set\n
 *       dg_configUSE_AUTO_CHIP_DETECTION to 1.
 *
 * \bsp_default_note{\bsp_config_option_build,}
 */
#ifndef dg_configBLACK_ORCA_IC_REV
#       if dg_configUSE_AUTO_CHIP_DETECTION != 1
#               error "You must either define dg_configBLACK_ORCA_IC_REV in the build configuration or set dg_configUSE_AUTO_CHIP_DETECTION to 1"
#       endif
#endif

/**
 * \brief The chip stepping that we compile for.
 *
 * \note There is no default value defined for the target chip revision. The application must\n
 *       either explicitly set dg_configBLACK_ORCA_IC_STEP nor set\n
 *       dg_configUSE_AUTO_CHIP_DETECTION to 1.
 *
 * \bsp_default_note{\bsp_config_option_build,}
 */
#ifndef dg_configBLACK_ORCA_IC_STEP
#       if dg_configUSE_AUTO_CHIP_DETECTION != 1
#               error "You must either define dg_configBLACK_ORCA_IC_STEP in the build configuration or set dg_configUSE_AUTO_CHIP_DETECTION to 1"
#       endif
#endif

/**
 * \brief The motherboard revision we compile for.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configBLACK_ORCA_MB_REV
#define dg_configBLACK_ORCA_MB_REV      BLACK_ORCA_MB_REV_D
#endif

/*
 * This is a deprecated configuration hence not to be defined by an application.
 * The values of the trim registers are not anymore taken from the Flash.
 *
 */
#ifdef  dg_configCONFIG_HEADER_IN_FLASH
#error "Configuration option dg_configCONFIG_HEADER_IN_FLASH is no longer supported."
#endif

/**
 * \brief Controls how the image is built.
 *  - DEVELOPMENT_MODE: various debugging options are included.
 *  - PRODUCTION_MODE: all code used for debugging is removed.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configIMAGE_SETUP
#define dg_configIMAGE_SETUP            DEVELOPMENT_MODE
#endif

/**
 * \brief When set to 1, the delay at the start of execution of the Reset_Handler is skipped.
 *
 * \details This delay is added in order to facilitate proper programming of the Flash or QSPI\n
 *        launcher invocation. Without it, there is no guarantee that the execution of the image\n
 *        will not proceed, altering the default configuration of the system from the one that the\n
 *        bootloader leaves it.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configSKIP_MAGIC_CHECK_AT_START
#define dg_configSKIP_MAGIC_CHECK_AT_START      (0)
#endif

/**
 * \brief When set to 1, the chip version (DA14680/1-01 or DA14682/3-00, DA15XXX-00 ) will be detected\n
 *        automatically.
 * \note It cannot be used in BLE applications because of the different linker scripts\n
 *       that are used.
 * \warning Not to be used by a generic project, applicable for uartboot only.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configUSE_AUTO_CHIP_DETECTION
#define dg_configUSE_AUTO_CHIP_DETECTION        (0)
#else
# if (dg_configUSE_AUTO_CHIP_DETECTION == 1)
        #undef dg_configBLACK_ORCA_IC_REV
        #define dg_configBLACK_ORCA_IC_REV      BLACK_ORCA_IC_REV_AUTO
        #undef dg_configBLACK_ORCA_IC_STEP
        #define dg_configBLACK_ORCA_IC_STEP     BLACK_ORCA_IC_STEP_AUTO
# endif
#endif

/**
 * \brief When set to 1, the OTP copy will be emulated when in DEVELOPMENT_MODE.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configEMULATE_OTP_COPY
#define dg_configEMULATE_OTP_COPY       (0)
#endif

/**
 * \brief When set to 1, the QSPI copy will be emulated when in DEVELOPMENT_MODE (Not Applicable!).
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configEMULATE_QSPI_COPY
#define dg_configEMULATE_QSPI_COPY      (0)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup LOW_POWER_CLOCK_SETTINGS
 *
 * \brief Doxygen documentation is not yet available for this module.
 *        Please check the source code file(s)
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 * \{
 */
/* --------------------------------- Low-Power clock settings ----------------------------------- */

/*
 * Maximum sleep time the sleep timer supports
 *
 * dg_configTim1Prescaler can be zero. if dg_configTim1Prescaler is not zero then
 * ( dg_configTim1Prescaler + 1 ) MUST be a power of 2! */
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#if (dg_configUSE_LP_CLK == LP_CLK_32000)
#define dg_configTim1Prescaler          (3)
#define dg_configMaxSleepTime           (8)     // sec
#elif (dg_configUSE_LP_CLK == LP_CLK_32768)
#define dg_configTim1Prescaler          (3)
#define dg_configMaxSleepTime           (8)     // sec
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
#define dg_configTim1Prescaler          (0)
#define dg_configMaxSleepTime           (8)     // sec
#elif (dg_configUSE_LP_CLK == LP_CLK_ANY)
/* Assuming that the frequency of the external digital clock is 32000Hz,
 *      period: 1/32000 = 31.25usec
 *      Timer1 wrap-around time: 65536 * 31.25 = 2.048sec
 *      With a prescaler equal to 3, the wrap around time becomes: (1 + 3) * 2.048 = ~8sec.
 * If the clock frequency is too slow, i.e. 16000Hz then
 *      period: 1/16000 = 62.5usec
 *      Timer1 wrap-around time = 65536 * 62.5 = 4.096sec
 *      and the prescaler should be 1 so that: (1 + 1) * 4.096 = ~8sec.
 * If the clock frequency is too high, i.e. 125000Hz then
 *      period: 1/125000 = 8usec
 *      Timer1 wrap-around time = 65536 * 8 = 0.524sec
 *      and the prescaler should be 3 so that: (1 + 3) * 0.524 = ~2sec.
 * The values MUST be defined in the custom_config_<>.h file!
 */
#else
#error "dg_configUSE_LP_CLK has invalid setting"
#endif
#else /* dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A */
#if (dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768) || (dg_configUSE_LP_CLK == LP_CLK_RCX)
#ifdef dg_configTim1Prescaler
#warning "Timer1 prescaler is not supported in DA14682/3 chips."
#undef dg_configTim1Prescaler
#endif /* dg_configTim1Prescaler */
#define dg_configMaxSleepTime           (134217)     // sec
#else
#error "dg_configUSE_LP_CLK has invalid setting"
#endif
#endif /* dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A */

#if (dg_configTim1Prescaler && (((dg_configTim1Prescaler+1) / 2) * 2) != (dg_configTim1Prescaler+1))
#error "dg_configTim1Prescaler+1 is not 0 or a power of 2!"
#endif

#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#if (dg_configTim1Prescaler == 0)
#define dg_configTim1PrescalerBitRange  (0)
#elif (dg_configTim1Prescaler == 1)
#define dg_configTim1PrescalerBitRange  (1)
#elif (dg_configTim1Prescaler == 3)
#define dg_configTim1PrescalerBitRange  (2)
#else
#error "dg_configTim1Prescaler is larger than 3!"
#endif
#else
#ifdef dg_configTim1PrescalerBitRange
#warning "Timer1 prescaler is not supported in DA14682/3 chips."
#undef dg_configTim1PrescalerBitRange
#endif
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup SYSTEM_CONFIGURATION_SETTINGS
 *
 * \brief System configuration settings
 *
 * \{
 */
/* ------------------------------- System configuration settings -------------------------------- */

/**
 * \brief Source of Low Power clock used (LP_CLK_IS_ANALOG, LP_CLK_IS_DIGITAL)
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configLP_CLK_SOURCE
#define dg_configLP_CLK_SOURCE          LP_CLK_IS_ANALOG
#endif

# if ((dg_configLP_CLK_SOURCE == LP_CLK_IS_ANALOG) && (dg_configUSE_LP_CLK == LP_CLK_ANY))
# error "When the LP source is analog (XTAL), the option LP_CLK_ANY is invalid!"
# endif

# if ((dg_configLP_CLK_SOURCE == LP_CLK_IS_DIGITAL) && (dg_configUSE_LP_CLK == LP_CLK_RCX))
# error "When the LP source is digital (External), the option LP_CLK_RCX is invalid!"
# endif

/**
 * \brief Low Power clock used (LP_CLK_32000, LP_CLK_32768, LP_CLK_RCX, LP_CLK_ANY)
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
# ifndef dg_configUSE_LP_CLK
# define dg_configUSE_LP_CLK            LP_CLK_RCX
# endif

/**
 * \brief Code execution mode
 *
 *  - MODE_IS_RAM
 *  - MODE_IS_MIRRORED
 *  - MODE_IS_CACHED
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
# ifndef dg_configEXEC_MODE
# define dg_configEXEC_MODE             MODE_IS_RAM
# endif

/**
 * \brief Code location
 *
 * - NON_VOLATILE_IS_OTP
 * - NON_VOLATILE_IS_FLASH
 * - NON_VOLATILE_IS_NONE (RAM)
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
# ifndef dg_configCODE_LOCATION
# define dg_configCODE_LOCATION         NON_VOLATILE_IS_NONE
# endif

/**
 * \brief Frequency of the crystal connected to the XTAL Oscillator: 16MHz or 32MHz.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
# ifndef dg_configEXT_CRYSTAL_FREQ
# define dg_configEXT_CRYSTAL_FREQ      EXT_CRYSTAL_IS_16M
# endif

/**
 * \brief External LP type
 *
 * - 0: a crystal is connected to XTAL32Kp and XTALK32Km
 * - 1: a digital clock is provided.
 *
 * \note the frequency of the digital clock must be 32KHz or 32.768KHz and be always running.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
# ifndef dg_configEXT_LP_IS_DIGITAL
# define dg_configEXT_LP_IS_DIGITAL     (0)
# endif

/*
 * This is a deprecated configuration hence not to be defined by an application.
 * Forcing the system to enter into clockless sleep during deep sleep is no longer supported.
  */
#ifdef dg_configFORCE_DEEP_SLEEP
#error "Configuration option dg_configFORCE_DEEP_SLEEP is no longer supported."
#endif

/**
 * \brief Timer 1 usage
 *
 * When set to 0, Timer1 is reserved for the OS tick.
 *
 * \note A setting to 1 is meaningful only for non-RTOS projects.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configUSER_CAN_USE_TIMER1
#define dg_configUSER_CAN_USE_TIMER1    (0)
#endif

/**
 * \brief Time needed for the settling of the LP clock, in msec.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configXTAL32K_SETTLE_TIME
#define dg_configXTAL32K_SETTLE_TIME    (8000)
#endif

/**
 * \brief Sleep delay time
 *
 * At startup, the system will stay active for at least this time period before it is allowed to go
 * to sleep, in msec.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#if (dg_configXTAL32K_SETTLE_TIME > 8000)
#define dg_configINITIAL_SLEEP_DELAY_TIME       (dg_configXTAL32K_SETTLE_TIME)
#else
#define dg_configINITIAL_SLEEP_DELAY_TIME       (8000)
#endif

/**
 * \brief XTAL16 settle time
 *
 * Time needed for the settling of the XTAL16, in LP cycles. To this value, 5 LP cycles, that are
 * needed to start the core clock, are added since the SW powers the 1V4 rail after the execution
 * is resumed.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configXTAL16_SETTLE_TIME
#if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
        #define dg_configXTAL16_SETTLE_TIME     (85 + 5)
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
        #define dg_configXTAL16_SETTLE_TIME     cm_rcx_us_2_lpcycles((uint32_t)((85 + 5) * 30.5))
#else /* LP_CLK_ANY */
// Must be defined in the custom_config_<>.h file.
#endif
#endif

/**
 * \brief XTAL16 settle time RC32K
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#define dg_configXTAL16_SETTLE_TIME_RC32K       (110)

/**
 * \brief RC16 wakeup time
 *
 * This is the maximum time, in LP cycles, needed to wake-up the chip and start executing code
 * using RC16.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#define dg_configWAKEUP_RC16_TIME       (16)

/**
 * \brief XTAL16 wakeup time
 *
 * Wake up N LP cycles before "time 0" to have XTAL16 ready when needed.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#define dg_configWAKEUP_XTAL16_TIME     (dg_configXTAL16_SETTLE_TIME + dg_configWAKEUP_RC16_TIME)

/**
 * \brief OS tick restore time
 *
 * This is the time, in LP cycles, required from the OS to restore the tick timer.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configOS_TICK_RESTORE_TIME
#ifdef RELEASE_BUILD
#if dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A
#if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  (dg_configTim1Prescaler ? 2 + dg_configTim1Prescaler : 3)
        #else
                #if (dg_configIMAGE_SETUP == PRODUCTION_MODE)
                        #define dg_configOS_TICK_RESTORE_TIME  (dg_configTim1Prescaler ? 3 + dg_configTim1Prescaler : 4)
                #else
                        #define dg_configOS_TICK_RESTORE_TIME  (dg_configTim1Prescaler ? 3 + dg_configTim1Prescaler : 4)
                #endif
        #endif
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(120)
        #else
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(120)
        #endif
#else /* LP_CLK_ANY */
        /* Must be defined in the custom_config_<>.h file.
         * For QSPI cached, the dg_configOS_TICK_RESTORE_TIME must be ~120usec when no prescaling is
         * used and ~180usec when prescaling is used.
         */
#endif
#else /* dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A */
#if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  (3)
        #else
                #if (dg_configIMAGE_SETUP == PRODUCTION_MODE)
                        #define dg_configOS_TICK_RESTORE_TIME  (4)
                #else
                        #define dg_configOS_TICK_RESTORE_TIME  (4)
                #endif
        #endif
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(120)
        #else
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(120)
        #endif
#else /* LP_CLK_ANY */
        /* Must be defined in the custom_config_<>.h file.
         * For QSPI cached, the dg_configOS_TICK_RESTORE_TIME must be ~120usec when no prescaling is
         * used and ~180usec when prescaling is used.
         */
#endif
#endif /* dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A */
#else // RELEASE_BUILD
#if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  (40)
        #else
                #define dg_configOS_TICK_RESTORE_TIME  (72)
        #endif
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(1200)
        #else
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(2400)
        #endif
#else /* LP_CLK_ANY */
        /* Must be defined in the custom_config_<>.h file.
         * Usually, the dg_configOS_TICK_RESTORE_TIME is set to a large value (i.e. 1.2 - 2.4msec)
         * in order to allow for a more "relaxed" waking up of the system.
         */
#endif
#endif // RELEASE_BUILD
#endif // dg_configOS_TICK_RESTORE_TIME

/**
 * \brief Image copy time
 *
 * The number of LP cycles needed for the application's image data to be copied from the OTP
 * (or QSPI) to the RAM in mirrored mode.
 *
 * \warning MUST BE SMALLER THAN #dg_configMIN_SLEEP_TIME !!!
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#if (dg_configEXEC_MODE != MODE_IS_MIRRORED)
        #undef dg_configIMAGE_COPY_TIME
        #define dg_configIMAGE_COPY_TIME        (0)
#elif !defined(dg_configIMAGE_COPY_TIME)
        #if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
                #define dg_configIMAGE_COPY_TIME        (64)
        #elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
                #define dg_configIMAGE_COPY_TIME        cm_rcx_us_2_lpcycles(1950)
        #else /* LP_CLK_ANY */
                // Must be defined in the custom_config_<>.h file.
        #endif
#endif

/**
 * \brief Retention memory configuration.
 *
 * 5 bits field; each bit controls whether the relevant memory block will be retained (1) or not (0).
 * -  bit 0 : SYSRAM1
 * -  bit 1 : SYSRAM2
 * -  bit 2 : SYSRAM3
 * -  bit 3 : SYSRAM4
 * -  bit 4 : SYSRAM5
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 *
 */
#ifndef dg_configMEM_RETENTION_MODE
#define dg_configMEM_RETENTION_MODE     (0x1F)
#endif

/*
 * This is a deprecated configuration hence not to be defined by an application.
 */
#ifdef dg_configMEM_RETENTION_MODE_PRESERVE_IMAGE
#error "dg_configMEM_RETENTION_MODE_PRESERVE_IMAGE is no longer supported."
#endif

/**
 * \brief Memory Shuffling mode.
 *
 * See SYS_CTRL_REG:REMAP_RAMS field.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configSHUFFLING_MODE
#define dg_configSHUFFLING_MODE         (0)
#endif

/**
 * \brief ECC microcode RAM retainment
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configECC_UCODE_RAM_RETAINED
#define dg_configECC_UCODE_RAM_RETAINED (0)
#endif

/**
 * \brief Watchdog Service
 *
 * - 1: enabled
 * - 0: disabled
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSE_WDOG
#define dg_configUSE_WDOG               (0)
#endif

/**
 * \brief Brown-out Detection
 *
 * - 1: used
 * - 0: not used
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSE_BOD
#define dg_configUSE_BOD                (1)
#endif

/**
 * \brief Reset value for Watchdog.
 *
 * See WATCHDOG_REG:WDOG_VAL field.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configWDOG_RESET_VALUE
#define dg_configWDOG_RESET_VALUE       (0xFF)
#endif

/**
 * \brief Maximum watchdog tasks
 *
 * Maximum number of tasks that the Watchdog Service can monitor. It can be larger (up to 32) than
 * needed, at the expense of increased Retention Memory requirement.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configWDOG_MAX_TASKS_CNT
#define dg_configWDOG_MAX_TASKS_CNT     (4)
#endif

/**
 * \brief Watchdog notify interval
 *
 * Interval (in miliseconds) for common timer which can be used to trigger tasks in order to notify
 * watchdog. Can be set to 0 in order to disable timer code entirely.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configWDOG_NOTIFY_TRIGGER_TMO
#define dg_configWDOG_NOTIFY_TRIGGER_TMO        (0)
#endif

/**
 * \brief Abort a clock modification if it will cause an error to the SysTick counter
 *
 * - 1: on
 * - 0: off
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configABORT_IF_SYSTICK_CLK_ERR
#define dg_configABORT_IF_SYSTICK_CLK_ERR       (0)
#endif

/**
 * \brief Maximum adapters count
 *
 * Should be equal to the number of Adapters used by the Application. It can be larger (up to 254)
 * than needed, at the expense of increased Retention Memory requirements. It cannot be 0.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configPM_MAX_ADAPTERS_CNT
#define dg_configPM_MAX_ADAPTERS_CNT    (16)
#endif

/**
 * \brief Maximum sleep defer time
 *
 * The maximum time sleep can be deferred via a call to pm_defer_sleep_for(). It is in clock cycles
 * in the case of the XTAL32K and in usec in the case of RCX.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configPM_MAX_ADAPTER_DEFER_TIME
#if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
        #define dg_configPM_MAX_ADAPTER_DEFER_TIME      (128)
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
        #define dg_configPM_MAX_ADAPTER_DEFER_TIME      cm_rcx_us_2_lpcycles(4000)
#else /* LP_CLK_ANY */
        // Must be defined in the custom_config_<>.h file. It should be > 3.5msec.
#endif
#endif

/**
 * \brief Minimum sleep time
 *
 *  No power savings if we enter sleep when the sleep time is less than N LP cycles
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configMIN_SLEEP_TIME
#if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
        #define dg_configMIN_SLEEP_TIME (33*3)  // 3 msec
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
        #define dg_configMIN_SLEEP_TIME cm_rcx_us_2_lpcycles_low_acc(3000)  // 3 msec
#else /* LP_CLK_ANY */
        // Must be defined in the custom_config_<>.h file. It should be ~3msec but this may vary.
#endif
#endif

/**
 * \brief Recharge period
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configSET_RECHARGE_PERIOD
#if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
#define dg_configSET_RECHARGE_PERIOD    (3000)  /** number of Low Power clock cycles for
                                                   sampling and / or refreshing. */
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
#define dg_configSET_RECHARGE_PERIOD    (90000) /** number of msec for sampling and / or
                                                   refreshing */
#else /* LP_CLK_ANY */
        // Must be defined in the custom_config_<>.h file.
#endif
#endif /* dg_configSET_RECHARGE_PERIOD */

/**
 * \brief When set to 1, the DCDC is used.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configUSE_DCDC
#define dg_configUSE_DCDC               (1)
#endif

/*
 * This is a deprecated configuration hence not to be defined by an application.
 * The semantics of dg_configUSE_ADC replaced by dg_configUSE_HW_GPADC.
 */
#ifdef dg_configUSE_ADC
#error "Configuration option dg_configUSE_ADC is no longer supported. Use dg_configUSE_HW_GPADC instead."
#endif

/**
 * \brief Apply ADC Gain Error correction.
 * - 1: used
 * - 0: not used
 *
 * The default setting is: 1.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configUSE_ADC_GAIN_ERROR_CORRECTION
#define dg_configUSE_ADC_GAIN_ERROR_CORRECTION  (1)
#endif

/**
 * \brief When set to 1, the USB interface is used for data transfers.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSE_USB
#define dg_configUSE_USB                (0)
#endif

/**
 * \addtogroup CHARGER_SETTINGS
 *
 * \brief Charger configuration settings
 * \{
 */
/* -------------------------------------- Charger settings -------------------------------------- */

/**
 * \brief Battery type
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configBATTERY_TYPE
#define dg_configBATTERY_TYPE           (BATTERY_TYPE_NO_BATTERY)
/**
 * \brief Battery charge voltage
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#define dg_configBATTERY_CHARGE_VOLTAGE (0)
/**
 * \brief Battery charge current
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#define dg_configBATTERY_CHARGE_CURRENT (0)
/**
 * \brief Battery charge NTC
 *
 * 0: NTC is enabled, 1: NTC is disabled
 *
 * Note that when NTC is enabled, the P14 and P16 are controlled by the charger and cannot be used
 * by the application. P14 is set high (at 3.3V) when the charging starts while P16 is an input.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#define dg_configBATTERY_CHARGE_NTC     (0)

#if (dg_configBATTERY_CHARGE_NTC > 1)
#error "dg_configBATTERY_CHARGE_NTC must be either 0 or 1."
#endif
/**
 * \brief Battery pre-charge current
 *
 * Normal charging :
 * -   0 - 15 : as the description of the CHARGER_CTRL1_REG[CHARGE_CUR] field.
 *
 * Ext charging : "end-of-charge" is not functional when this mode is used.
 * -  16 : Reserved
 * -  17 : Reserved
 * -  18 : 1mA
 * -  19 : 1.5mA
 * -  20 : 2.1mA
 * -  21 : 3.2mA
 * -  22 : 4.3mA
 * -  23 : Reserved
 * -  24 : 6.6mA
 * -  25 : 7.8mA
 * -  26 : Reserved
 * -  27 : 11.3mA
 * -  28 : 13.3mA
 * -  29 : 15.3mA
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#define dg_configBATTERY_PRECHARGE_CURRENT      (0)
#endif

/**
 * \brief When set to 1, the USB Charger is used to charge the battery.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSE_USB_CHARGER
#define dg_configUSE_USB_CHARGER        (0)
#else
#if (dg_configUSE_USB_CHARGER == 1)
        #undef dg_configUSE_HW_GPADC
        #define dg_configUSE_HW_GPADC           (1)
        #undef dg_configUSE_USB
        #define dg_configUSE_USB                (1)
        #undef dg_configGPADC_ADAPTER
        #define dg_configGPADC_ADAPTER          (1)
        #undef dg_configBATTERY_ADAPTER
        #define dg_configBATTERY_ADAPTER        (1)
#endif
#endif

/**
 * \brief When set to 1, the USB Charger will try to enumerate, if possible.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configUSE_USB_ENUMERATION
#define dg_configUSE_USB_ENUMERATION    (0)
#else
#if (dg_configUSE_USB_ENUMERATION == 1)
        #undef dg_configUSE_USB
        #define dg_configUSE_USB        (1)
#endif
#endif

/**
 * \brief Controls how the system will behave when the USB i/f is suspended.
 *
 * \details When the USB Node is suspended by the USB Host, the application may have to act in
 *          order to comply with the USB specification (consume less than 2.5mA). The available
 *          options are:
 *          0: do nothing
 *          1: pause system clock => the LP clock is stopped and only VBUS and USB irqs are handled
 *          2: pause application => The system is not paused but the application must stop all
 *             timers and make sure all tasks are blocked.
 *
 *          Both in modes 1 and 2, the application must make sure that all external peripherals are
 *          either powered off or placed in the lowest power consumption mode.
 */
#ifndef dg_configUSB_SUSPEND_MODE
#define dg_configUSB_SUSPEND_MODE       (0)
#endif

/**
 * \brief When set to 1, the USB Charger will start charging with less than 100mA until enumerated.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configALLOW_CHARGING_NOT_ENUM
#define dg_configALLOW_CHARGING_NOT_ENUM        (0)
#endif

/**
 * \brief When set to 1, the USB charger will stop charging from an SDP port (if
 *        #dg_configALLOW_CHARGING_NOT_ENUM is set to 1) after 45 minutes, if not enumerated.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configUSE_NOT_ENUM_CHARGING_TIMEOUT
#define dg_configUSE_NOT_ENUM_CHARGING_TIMEOUT  (1)
#endif

/**
 * \brief Pre-charging initial measure delay
 *
 * This is the time to wait (N x 10 ms) before doing the first voltage measurement after starting
 * pre-charging. This is to ensure that an initial battery voltage overshoot will not trigger the
 * charger to stop pre-charging and move to normal charging.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configPRECHARGING_INITIAL_MEASURE_DELAY
#define dg_configPRECHARGING_INITIAL_MEASURE_DELAY      (3)
#endif

/**
 * \brief Pre-charging threshold
 *
 * When Vbat is below this threshold (in ADC measurement units), pre-charging starts.
 *
 * The value must be calculated using this equation:
 *
 * y[ADC units] = (4095 * Vbat[Volts]) / 5
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configPRECHARGING_THRESHOLD
#define dg_configPRECHARGING_THRESHOLD  (0)
#endif

/**
 * \brief Charging threshold
 *
 * When Vbat is at or above this threshold (in ADC measurement units), pre-charging stops and
 * charging starts.
 *
 * The value must be calculated using this equation:
 *
 * y[ADC units] = (4095 * Vbat[Volts]) / 5
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configCHARGING_THRESHOLD
#define dg_configCHARGING_THRESHOLD     (0)
#endif

/**
 * \brief Pre-charging timeout
 *
 * If after this period, the Vbat is not higher than 3.0V then pre-charging stops (N x 10msec).
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configPRECHARGING_TIMEOUT
#define dg_configPRECHARGING_TIMEOUT    (15 * 60 * 100)
#endif

/**
 * \brief Charging timeout
 *
 * If after this period, the charger is still charging then charging stops (N x 10msec). This
 * timeout has priority over the next two timeouts. If it is not zero then it is the only one taken
 * into account.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configCHARGING_TIMEOUT
#define dg_configCHARGING_TIMEOUT       (0)
#endif

/**
 * \brief Charging CC timeout
 *
 * If after this period, the charger is still in CC mode then charging stops (N x 10msec).
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configCHARGING_CC_TIMEOUT
#define dg_configCHARGING_CC_TIMEOUT    (180 * 60 * 100)
#endif

/**
 * \brief Charging CV timeout
 *
 * If after this period, the charger is still in CC mode then charging stops (N x 10msec).
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configCHARGING_CV_TIMEOUT
#define dg_configCHARGING_CV_TIMEOUT    (360 * 60 * 100)
#endif

/**
 * \brief Charginf polling interval
 *
 * While being attached to a USB cable and the battery has been charged, this is the interval
 * (N x 10msec) that the Vbat is polled to decide whether a new charge cycle will be started.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configUSB_CHARGER_POLLING_INTERVAL
#define dg_configUSB_CHARGER_POLLING_INTERVAL   (100)
#endif

/**
 * \brief Battery low level
 *
 * If not zero, this is the low limit of the battery voltage. If Vbat drops below this limit, the
 * system enters hibernation mode, waiting either for the battery to the changed or recharged.
 *
 * The value must be calculated using this equation:
 *
 * y[ADC units] = (4095 * Vbat[Volts]) / 5
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configBATTERY_LOW_LEVEL
        #if (dg_configBATTERY_TYPE == BATTERY_TYPE_NO_BATTERY) || (dg_configBATTERY_TYPE == BATTERY_TYPE_NO_RECHARGE)
                #define dg_configBATTERY_LOW_LEVEL      (0)
        #else
                #define dg_configBATTERY_LOW_LEVEL      (2293)  // 2.8V
        #endif
#endif

/*
 * This is a deprecated configuration hence not to be defined by an application.
 * When entering hibernation mode then:
 * A DA14680/1-01 system reboots only via an interrupt from the WKUP Ctrl or VBUS.
 * A DA14682/3-00, DA15XXX-00 system reboots only via an interrupt from the WKUP Ctrl.
 */
#ifdef dg_configLOW_VBAT_HANDLING
#error "Configuration option dg_configLOW_VBAT_HANDLING is no longer supported."
#endif

/**
 * \brief Custom battery ADC voltage
 *
 * In case of a custom battery with unknown voltage level, this parameter must be defined to
 * provide the charge level of the battery in ADC measurement units. If not provided for some
 * reason, it is set to the lowest level (1.9V).
 *
 * The value must be calculated using this equation:
 *
 * y[ADC units] = (4095 * Vbat[Volts]) / 5
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#if (dg_configBATTERY_TYPE == BATTERY_TYPE_CUSTOM)
#ifndef dg_configBATTERY_TYPE_CUSTOM_ADC_VOLTAGE
#define dg_configBATTERY_TYPE_CUSTOM_ADC_VOLTAGE        (1556)
#endif
#endif

/**
 * \brief Battery charge gap
 *
 * This is the safety limit used in the "Measurement step" of the specification to decide whether
 * charging should be started. The default value is 0.1V.
 *
 * The value must be calculated using this equation:
 *
 * y[ADC units] = (4095 * Vbat[Volts]) / 5
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configBATTERY_CHARGE_GAP
#define dg_configBATTERY_CHARGE_GAP     (82)
#endif

/**
 * \brief Battery replenish gap
 *
 * This is the limit below the maximum Vbat level (Vbat - dg_configBATTERY_REPLENISH_GAP), where
 * charging will be restarted in order to replenish the battery. The default value is 0.2V.
 *
 * The value must be calculated using this equation:
 *
 * y[ADC units] = (4095 * Vbat[Volts]) / 5
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configBATTERY_REPLENISH_GAP
#define dg_configBATTERY_REPLENISH_GAP  (82*2)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \} CHARGER_SETTINGS
 */


/**
 * \brief The Rsense of the SOC in multiples of 0.1Ohm. The default value is (1 x 0.1Ohm).
 */
#ifndef dg_configSOC_RSENSE
#define dg_configSOC_RSENSE             (1)     // N x 0.1Ohm
#endif

/**
 * \brief When set to 1, the ProDK is used (controls specific settings for this board).
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSE_ProDK
#define dg_configUSE_ProDK              (0)
#endif

/**
 * \brief When set to 1, State of Charge function is enabled.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configUSE_SOC
#define dg_configUSE_SOC                (0)
#else
#   if (dg_configUSE_SOC == 1)
#   undef dg_configUSE_HW_SOC
#   define dg_configUSE_HW_SOC          (1)
#   endif
#endif

/**
 * \addtogroup FLASH_SETTINGS
 *
 * \brief Flash configuration settings
 *
 * \{
 */
/* -------------------------------------- Flash settings ---------------------------------------- */

/**
 * \brief The rail from which the Flash is powered, if a Flash is used.
 *
 * - FLASH_IS_NOT_CONNECTED
 * - FLASH_CONNECTED_TO_1V8
 * - FLASH_CONNECTED_TO_1V8P
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configFLASH_CONNECTED_TO
#error "dg_configFLASH_CONNECTED_TO is not defined!"
#endif

/* Backward compatibility checks */
#if defined(dg_configPOWER_EXT_1V8_PERIPHERALS) || defined(dg_configPOWER_FLASH) \
                || defined(dg_configFLASH_POWER_OFF)
#define PRINT_POWER_RAIL_SETUP
#endif

#if defined(dg_configPOWER_EXT_1V8_PERIPHERALS) && defined(dg_configPOWER_1V8P)
#error "Both dg_configPOWER_EXT_1V8_PERIPHERALS and dg_configPOWER_1V8P are defined! Please use dg_configPOWER_1V8P only!"
#endif

#ifdef dg_configPOWER_EXT_1V8_PERIPHERALS
        #warning "dg_configPOWER_EXT_1V8_PERIPHERALS will be deprecated! Use dg_configPOWER_1V8P instead!"

        #if (dg_configPOWER_EXT_1V8_PERIPHERALS == 0) \
                        && (dg_configFLASH_CONNECTED_TO == FLASH_CONNECTED_TO_1V8P)
                #error "Flash is connected to 1V8P but dg_configPOWER_EXT_1V8_PERIPHERALS is 0..."
        #endif

        #if (dg_configPOWER_EXT_1V8_PERIPHERALS == 1)
                #define dg_configPOWER_1V8P             (1)
        #else
                #define dg_configPOWER_1V8P             (0)
        #endif
        #undef dg_configPOWER_EXT_1V8_PERIPHERALS
#endif

#if defined(dg_configPOWER_FLASH) && defined(dg_configPOWER_1V8_ACTIVE)
#error "Both dg_configPOWER_FLASH and dg_configPOWER_1V8_ACTIVE are defined! Please use dg_configPOWER_1V8_ACTIVE only!"
#endif

#if defined(dg_configFLASH_POWER_OFF) && defined(dg_configPOWER_1V8_SLEEP)
#error "Both dg_configFLASH_POWER_OFF and dg_configPOWER_1V8_SLEEP are defined! Please use dg_configPOWER_1V8_SLEEP only!"
#endif

#ifdef dg_configPOWER_FLASH
#warning "dg_configPOWER_FLASH is deprecated! Use dg_configPOWER_1V8_ACTIVE instead!"
#endif
/* End of backward compatibility checks */

/**
 * \brief When set to 1, the 1V8 rail is powered, when the system is in active state.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configPOWER_1V8_ACTIVE
#ifndef dg_configPOWER_FLASH
        #define dg_configPOWER_1V8_ACTIVE               (0)
#else
        #if (dg_configPOWER_FLASH == 0)
                #if (dg_configFLASH_CONNECTED_TO == FLASH_CONNECTED_TO_1V8)
                        #error "Flash is connected to the 1V8 rail but the rail is turned off (1)..."
                #else
                        #define dg_configPOWER_1V8_ACTIVE       (0)
                #endif
        #else
                #define dg_configPOWER_1V8_ACTIVE       (1)
        #endif
#endif
#endif

/**
 * \brief When set to 1, the 1V8 is powered during sleep.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configPOWER_1V8_SLEEP
#ifndef dg_configFLASH_POWER_OFF
        #define dg_configPOWER_1V8_SLEEP        (0)
#else
        #if (dg_configFLASH_POWER_OFF == 0)
                #define dg_configPOWER_1V8_SLEEP        (1)
        #else
                #define dg_configPOWER_1V8_SLEEP        (0)
        #endif
#endif
#endif

/**
 * \brief When set to 1, the Flash (connected to the 1V8 rail) is powered off during sleep.
 *
 * \note This is an internal define and cannot be overridden!
 */
#undef dg_configFLASH_POWER_OFF
#define dg_configFLASH_POWER_OFF        ((dg_configFLASH_CONNECTED_TO == FLASH_CONNECTED_TO_1V8) \
                                                && (dg_configPOWER_1V8_SLEEP == 0))

/**
 * \brief When set to 1, the 1V8P rail is powered.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configPOWER_1V8P
#define dg_configPOWER_1V8P             (0)
#endif

/**
 * \brief When set to 1, the QSPI FLASH is put to power-down state during sleep.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configFLASH_POWER_DOWN
#define dg_configFLASH_POWER_DOWN       (0)
#endif

/**
 * \brief Enable the Flash Autodetection mode
 *
 * \warning THIS WILL GREATLY INCREASE THE CODE SIZE AND RETRAM USAGE!!! MAKE SURE YOUR PROJECT
 *          CAN SUPPORT THIS.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configFLASH_AUTODETECT
#define dg_configFLASH_AUTODETECT       (0)
#endif

#if dg_configFLASH_AUTODETECT == 0

/**
 * \brief The Flash Driver header file to include
 *
 * The header file must be in the include path of the compiler
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configFLASH_HEADER_FILE
#define dg_configFLASH_HEADER_FILE      "qspi_w25q80ew.h"
#endif

/**
 * \brief The Flash Manufacturer ID
 *
 * This must be defined inside the driver header file
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configFLASH_MANUFACTURER_ID
#define dg_configFLASH_MANUFACTURER_ID  WINBOND_ID
#endif

/**
 * \brief The Flash Device Type ID
 *
 * This must be defined inside the driver header file
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configFLASH_DEVICE_TYPE
#define dg_configFLASH_DEVICE_TYPE      W25Q80EW
#endif

/**
 * \brief The Flash Device Density ID
 *
 * This must be defined inside the driver header file
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configFLASH_DENSITY
#define dg_configFLASH_DENSITY          W25Q_8Mb_SIZE
#endif

#endif /* dg_configFLASH_AUTODETECT == 0 */

/**
 * \brief Offset of the image if not placed at the beginning of QSPI Flash.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configIMAGE_FLASH_OFFSET
#define dg_configIMAGE_FLASH_OFFSET             (0)
#endif

/**
 * \brief Set the flash page size.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configFLASH_MAX_WRITE_SIZE
#define dg_configFLASH_MAX_WRITE_SIZE           (128)
#endif

/**
 * \brief Disable background operations.
 *
 * When enabled, outstanding QSPI operations will take place during sleep time
 * increasing the efficiency.
 *
 * - 1 : Disabled
 * - 0 : Enabled
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configDISABLE_BACKGROUND_FLASH_OPS
#define dg_configDISABLE_BACKGROUND_FLASH_OPS   (0)
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup DEBUG_SETTINGS
 *
 * \brief Debugging settings
 *
 * \{
 */
/* -------------------------------------- Debug settings ---------------------------------------- */

/**
 * \brief Enable debugger
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configENABLE_DEBUGGER
#define dg_configENABLE_DEBUGGER        (1)
#endif

/**
 * \brief Use SW cursor
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSE_SW_CURSOR
#       define dg_configUSE_SW_CURSOR           (0)
#       define SW_CURSOR_PORT                   (0)
#       define SW_CURSOR_PIN                    (0)
#else
#       if dg_configBLACK_ORCA_MB_REV == BLACK_ORCA_MB_REV_D
#               if !defined SW_CURSOR_PORT  &&  !defined SW_CURSOR_PIN
#                       define SW_CURSOR_PORT   (0)
#                       define SW_CURSOR_PIN    (7)
#               endif
#       else
#               if !defined SW_CURSOR_PORT  &&  !defined SW_CURSOR_PIN
#                       define SW_CURSOR_PORT   (2)
#                       define SW_CURSOR_PIN    (3)
#               endif
#       endif
#endif
#define SW_CURSOR_GPIO                  *(SW_CURSOR_PORT == 0 ? \
                                                (SW_CURSOR_PIN == 0 ? &(GPIO->P00_MODE_REG) : \
                                                (SW_CURSOR_PIN == 1 ? &(GPIO->P01_MODE_REG) : \
                                                (SW_CURSOR_PIN == 2 ? &(GPIO->P02_MODE_REG) : \
                                                (SW_CURSOR_PIN == 3 ? &(GPIO->P03_MODE_REG) : \
                                                (SW_CURSOR_PIN == 4 ? &(GPIO->P04_MODE_REG) : \
                                                (SW_CURSOR_PIN == 5 ? &(GPIO->P05_MODE_REG) : \
                                                (SW_CURSOR_PIN == 6 ? &(GPIO->P06_MODE_REG) : \
                                                                      &(GPIO->P07_MODE_REG)))))))) \
                                                : \
                                        (SW_CURSOR_PORT == 1 ? \
                                                (SW_CURSOR_PIN == 0 ? &(GPIO->P10_MODE_REG) : \
                                                (SW_CURSOR_PIN == 1 ? &(GPIO->P11_MODE_REG) : \
                                                (SW_CURSOR_PIN == 2 ? &(GPIO->P12_MODE_REG) : \
                                                (SW_CURSOR_PIN == 3 ? &(GPIO->P13_MODE_REG) : \
                                                (SW_CURSOR_PIN == 4 ? &(GPIO->P14_MODE_REG) : \
                                                (SW_CURSOR_PIN == 5 ? &(GPIO->P15_MODE_REG) : \
                                                (SW_CURSOR_PIN == 6 ? &(GPIO->P16_MODE_REG) : \
                                                                      &(GPIO->P17_MODE_REG)))))))) \
                                                : \
                                        (SW_CURSOR_PORT == 2 ? \
                                                (SW_CURSOR_PIN == 0 ? &(GPIO->P20_MODE_REG) : \
                                                (SW_CURSOR_PIN == 1 ? &(GPIO->P21_MODE_REG) : \
                                                (SW_CURSOR_PIN == 2 ? &(GPIO->P22_MODE_REG) : \
                                                (SW_CURSOR_PIN == 3 ? &(GPIO->P23_MODE_REG) : \
                                                                      &(GPIO->P24_MODE_REG))))) \
                                                : \
                                        (SW_CURSOR_PORT == 3 ? \
                                                (SW_CURSOR_PIN == 0 ? &(GPIO->P30_MODE_REG) : \
                                                (SW_CURSOR_PIN == 1 ? &(GPIO->P31_MODE_REG) : \
                                                (SW_CURSOR_PIN == 2 ? &(GPIO->P32_MODE_REG) : \
                                                (SW_CURSOR_PIN == 3 ? &(GPIO->P33_MODE_REG) : \
                                                (SW_CURSOR_PIN == 4 ? &(GPIO->P34_MODE_REG) : \
                                                (SW_CURSOR_PIN == 5 ? &(GPIO->P35_MODE_REG) : \
                                                (SW_CURSOR_PIN == 6 ? &(GPIO->P36_MODE_REG) : \
                                                                      &(GPIO->P37_MODE_REG)))))))) \
                                                : \
                                                (SW_CURSOR_PIN == 0 ? &(GPIO->P40_MODE_REG) : \
                                                (SW_CURSOR_PIN == 1 ? &(GPIO->P41_MODE_REG) : \
                                                (SW_CURSOR_PIN == 2 ? &(GPIO->P42_MODE_REG) : \
                                                (SW_CURSOR_PIN == 3 ? &(GPIO->P43_MODE_REG) : \
                                                (SW_CURSOR_PIN == 4 ? &(GPIO->P44_MODE_REG) : \
                                                (SW_CURSOR_PIN == 5 ? &(GPIO->P45_MODE_REG) : \
                                                (SW_CURSOR_PIN == 6 ? &(GPIO->P46_MODE_REG) : \
                                                                      &(GPIO->P47_MODE_REG))))))))))))

#define SW_CURSOR_SET                   *(SW_CURSOR_PORT == 0 ? &(GPIO->P0_SET_DATA_REG) :    \
                                        (SW_CURSOR_PORT == 1 ? &(GPIO->P1_SET_DATA_REG) :     \
                                        (SW_CURSOR_PORT == 2 ? &(GPIO->P2_SET_DATA_REG) :     \
                                        (SW_CURSOR_PORT == 3 ? &(GPIO->P3_SET_DATA_REG) :     \
                                                               &(GPIO->P4_SET_DATA_REG)))))

#define SW_CURSOR_RESET                 *(SW_CURSOR_PORT == 0 ? &(GPIO->P0_RESET_DATA_REG) :  \
                                        (SW_CURSOR_PORT == 1 ? &(GPIO->P1_RESET_DATA_REG) :   \
                                        (SW_CURSOR_PORT == 2 ? &(GPIO->P2_RESET_DATA_REG) :   \
                                        (SW_CURSOR_PORT == 3 ? &(GPIO->P3_RESET_DATA_REG) :   \
                                                               &(GPIO->P4_RESET_DATA_REG)))))
/**
 * \brief Enable task monitoring.
 *
 * \note Task monitoring can only be enabled if RTT or RETARGET is enabled
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configENABLE_TASK_MONITORING
#define dg_configENABLE_TASK_MONITORING        (0)
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup CACHE_SETTINGS
 *
 * \brief Cache configuration settings
 * \{
 */
/* -------------------------------------- Cache settings ---------------------------------------- */

/**
 * \brief Set the size (in bytes) of the QSPI flash cacheable area.
 *
 * All reads from offset 0 up to (not including) offset dg_configCACHEABLE_QSPI_AREA_LEN
 * will be cached. In addition, any writes to this area will trigger cache flushing, to
 * avoid any cache incoherence.
 *
 * The size must be 64KB-aligned, due to the granularity of CACHE_CTRL2_REG[CACHE_LEN].
 *
 * Special values:
 *  *  0 : Turn off cache.
 *  * -1 : Don't configure cacheable area size (i.e. leave as set by booter).
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configCACHEABLE_QSPI_AREA_LEN
#define dg_configCACHEABLE_QSPI_AREA_LEN        (-1)
#endif

/**
 * \brief Set the associativity of the cache.
 *
 * Available values:
 *   - CACHE_ASSOC_AS_IS
 *   - CACHE_ASSOC_DIRECT_MAP
 *   - CACHE_ASSOC_2_WAY
 *   - CACHE_ASSOC_4_WAY
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configCACHE_ASSOCIATIVITY
#define dg_configCACHE_ASSOCIATIVITY    CACHE_ASSOC_4_WAY
#endif

/**
 * \brief Set the line size of the cache.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 *
 * Available values:
 *   - CACHE_LINESZ_AS_IS
 *   - CACHE_LINESZ_8_BYTES
 *   - CACHE_LINESZ_16_BYTES
 *   - CACHE_LINESZ_32_BYTES
 */
#ifndef dg_configCACHE_LINESZ
#define dg_configCACHE_LINESZ           CACHE_LINESZ_8_BYTES
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup ARBITER_SETTINGS
 *
 * \brief Arbiter configuration settings
 * \{
 */
/* ------------------------------- Arbiter configuration settings ------------------------------- */

/**
 * \brief Custom arbiter configuration support
 * When defined, coex is configurable and priorities can be set:
 * -either manually, per mac, using coex api
 * -or automatically, by the PTIs provided by each mac
 *
 * When not defined, coex operates with the default/fixed priority scheme:
 * BLE traffic has always higher priority than FTDF.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configCOEX_ENABLE_CONFIG
#define dg_configCOEX_ENABLE_CONFIG     (0)
#endif

/**
 * \brief Arbiter statistics
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configCOEX_ENABLE_STATS
#define dg_configCOEX_ENABLE_STATS      (0)
#endif

/**
 * \brief Arbiter diagnostics enable
 *
 * This automatically enables arbiter diagnostic signals (when RF PD is on)
 *
 * See hw_coex.h for more information.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configCOEX_ENABLE_DIAGS
#define dg_configCOEX_ENABLE_DIAGS      (0)
#endif

/**
 * \brief Arbiter diagnostics mode
 *
 * This is the default mode for arbiter diagnostics.
 *
 * See hw_coex.h for more information.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configCOEX_DIAGS_MODE
#define dg_configCOEX_DIAGS_MODE        (HW_COEX_DIAG_MODE_3)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup PERIPHERAL_SELECTION
 *
 * \brief Peripheral selection
 *
 * When enabled the specific low level driver is included in the compilation of the SDK.
 * - 0 : Disabled
 * - 1 : Enabled
 *
 * The default option can be overridden in the application configuration file.
 *
 * \{
   Driver                         | Setting                                | Default option
   ------------------------------ | -------------------------------------- | :------------------:
   AES HASH                       | dg_configUSE_HW_AES_HASH               | 0
   Radio MAC Arbiter              | dg_configUSE_HW_COEX                   | 0
   Clock and Power Manager        | dg_configUSE_HW_CPM                    | 1
   Direct Memory Access           | dg_configUSE_HW_DMA                    | 1
   Elliptic Curve Controller      | dg_configUSE_HW_ECC                    | 1
   Analog to Digital Converter    | dg_configUSE_HW_GPADC                  | 1
   General Purpose I/O            | dg_configUSE_HW_GPIO                   | 1
   Inter-Integrated Circuit       | dg_configUSE_HW_I2C                    | 0
   Infra Red Generator            | dg_configUSE_HW_IRGEN                  | 0
   Keyboard scanner               | dg_configUSE_HW_KEYBOARD_SCANNER       | 0
   OTP controller                 | dg_configUSE_HW_OTPC                   | 1
   QSPI controller                | dg_configUSE_HW_QSPI                   | 1
   Quadrature decoder             | dg_configUSE_HW_QUAD                   | 0
   Radio module                   | dg_configUSE_HW_RF                     | 1
   State of charge module         | dg_configUSE_HW_SOC                    | 0
   Timer 0                        | dg_configUSE_HW_TIMER0                 | 0
   Timer 1                        | dg_configUSE_HW_TIMER1                 | 1
   Timer 2                        | dg_configUSE_HW_TIMER2                 | 0
   True Random Generator          | dg_configUSE_HW_TRNG                   | 1
   UART                           | dg_configUSE_HW_UART                   | 1
   USB charger                    | dg_configUSE_HW_USB_CHARGER            | 1
   Wakeup timer                   | dg_configUSE_HW_WKUP                   | 1
   PDM interface                  | dg_configUSE_IF_PDM                    | 0
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */

/* -------------------------------- Peripherals (hw_*) selection -------------------------------- */

#ifndef dg_configUSE_HW_AES_HASH
#define dg_configUSE_HW_AES_HASH        (0)
#endif

#ifndef dg_configUSE_HW_COEX
#define dg_configUSE_HW_COEX            (0)
#endif

#ifndef dg_configUSE_HW_CPM
#define dg_configUSE_HW_CPM             (1)
#endif

#ifndef dg_configUSE_HW_DMA
#define dg_configUSE_HW_DMA             (1)
#endif

#ifndef dg_configUSE_HW_ECC
#define dg_configUSE_HW_ECC             (1)
#endif

#ifndef dg_configUSE_HW_GPADC
#define dg_configUSE_HW_GPADC           (1)
#endif

#ifndef dg_configUSE_HW_GPIO
#define dg_configUSE_HW_GPIO            (1)
#endif

#ifndef dg_configUSE_HW_I2C
#define dg_configUSE_HW_I2C             (0)
#endif

#ifndef dg_configUSE_HW_IRGEN
#define dg_configUSE_HW_IRGEN           (0)
#endif

#ifndef dg_configUSE_HW_KEYBOARD_SCANNER
#define dg_configUSE_HW_KEYBOARD_SCANNER        (0)
#endif

#ifndef dg_configUSE_HW_OTPC
#define dg_configUSE_HW_OTPC            (1)
#endif

#ifndef dg_configUSE_HW_QSPI
#define dg_configUSE_HW_QSPI            (1)
#endif

#ifndef dg_configUSE_HW_QUAD
#define dg_configUSE_HW_QUAD            (0)
#endif

#ifndef dg_configUSE_HW_RF
#define dg_configUSE_HW_RF              (1)
#endif

#ifndef dg_configUSE_HW_SOC
#define dg_configUSE_HW_SOC             (0)
#endif

#ifndef dg_configUSE_HW_SPI
#define dg_configUSE_HW_SPI             (0)
#endif

#ifndef dg_configUSE_HW_TEMPSENS
#define dg_configUSE_HW_TEMPSENS        (1)
#endif

#ifndef dg_configUSE_HW_TIMER0
#define dg_configUSE_HW_TIMER0          (0)
#endif

#ifndef dg_configUSE_HW_TIMER1
#define dg_configUSE_HW_TIMER1          (1)
#endif

#ifndef dg_configUSE_HW_TIMER2
#define dg_configUSE_HW_TIMER2          (0)
#endif

#ifndef dg_configUSE_HW_TRNG
#define dg_configUSE_HW_TRNG            (1)
#endif

#ifndef dg_configUSE_HW_UART
#define dg_configUSE_HW_UART            (1)
#endif

#ifndef dg_configUSE_HW_USB_CHARGER
#define dg_configUSE_HW_USB_CHARGER     (1)
#endif

#ifndef dg_configUSE_HW_WKUP
#define dg_configUSE_HW_WKUP            (1)
#endif

#ifndef dg_configUSE_HW_USB
#define dg_configUSE_HW_USB             (0)
#endif

#ifndef dg_configUSE_IF_PDM
#define dg_configUSE_IF_PDM             (0)
#endif

/* ---------------------------------------------------------------------------------------------- */


/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ------------------------------- USB settings ------------------------------------------------ */

/**
 * \addtogroup USB_SETTINGS
 *
 * \brief USB DMA enable configuration settings
 *
 * The macros in this section are used to enable the DMA with USB and to define the two possible end point to use the DMA for data transfers.
 * \{
 */

/**
 * \brief Enable the DMA for reading/writing data to USB EP.\n
 * By default the USB DMA is not enabled.\n
 * To enable the DMA for the USB, set this the macro to value (1) in the custom_config_xxx.h file.\n
 * When the USB DMA is enabled, the default end points with DMA are EP1 and EP2. \n
 * It is possible only one TX and one RX end point to use DMA.\n
 * User can choose a different pair of end points to use the DMA as needed according to app requirements.\n
 * To change the end points using DMA, set in the the custom_config_xxx.h file the desired values for the macros:
 * \par \c dg_configUSB_TX_DMA_EP
 * valid values: 1,3,5\n
 * default value: 1
 * \par \c dg_configUSB_RX_DMA_EP
 * valid values: 2,4,6\n
 * default value: 2
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSB_DMA_SUPPORT
#define dg_configUSB_DMA_SUPPORT                (0)
#endif


/**
 * \brief The USB TX end point (D-->H) to enable the DMA.\n
 * User can choose a different pair of end points to use the DMA as needed according to app requirements.\n
 * To change the TX end point using DMA, set in the the custom_config_xxx.h file the desired value for this macros.
 * \par \c dg_configUSB_TX_DMA_EP
 * valid values: 1,3,5\n
 * default value: 1
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSB_TX_DMA_EP
#define  dg_configUSB_TX_DMA_EP                 (1)
#endif

/**
 * \brief The USB RX end point (D-->H) to enable the DMA.\n
 * User can choose a different pair of end points to use the DMA as needed according to app requirements.\n
 * To change the RX end point using DMA, set in the the custom_config_xxx.h file the desired value for this macros.
 * \par \c dg_configUSB_RX_DMA_EP
 * valid values: 2,4,6\n
 * default value: 2
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUSB_RX_DMA_EP
#define  dg_configUSB_RX_DMA_EP                 (2)
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */



/* ------------------------------- WKUP settings ------------------------------------------------ */

/**
 * \addtogroup WKUP_SETTINGS
 *
 * \brief WKUP configuration settings
 * \{
 */

/* ------------------------------- LATCH settings ------------------------------------------------ */

/**
 * \addtogroup WKUP_LATCH_SETTINGS
 *
 * \brief WKUP LATCH configuration settings
 * \{
 */

/**
 * \brief WKUP latch wakeup (io) source support
 *
 * \note In chip revision DA14680/1-01, this feature is implemented in s/w
 * \note In chip revision DA14682/3-00, DA15XXX-00, this feature is implemented in h/w
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#if (!defined(dg_configLATCH_WKUP_SOURCE)) || (dg_configUSE_HW_WKUP == 0)
        #undef dg_configLATCH_WKUP_SOURCE
        #define dg_configLATCH_WKUP_SOURCE     (0)
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ------------------------------- UART settings ------------------------------------------------ */

/**
 * \addtogroup UART_SETTINGS
 *
 * \brief UART FIFO configuration settings
 * \{
 */

/* ------------------------------- FIFO settings ------------------------------------------------ */

/**
 * \addtogroup UART_FIFO_SETTINGS
 *
 * \brief UART FIFO configuration settings
 * \{
 */

/**
 * \brief Software FIFO support
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUART_SOFTWARE_FIFO
#define dg_configUART_SOFTWARE_FIFO     (0)
#endif

/**
 * \brief UART1's software FIFO size
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUART1_SOFTWARE_FIFO_SIZE
#   define dg_configUART1_SOFTWARE_FIFO_SIZE    (0)
#endif

/**
 * \brief UART2's software FIFO size
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUART2_SOFTWARE_FIFO_SIZE
#   define dg_configUART2_SOFTWARE_FIFO_SIZE    (0)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ------------------------------- Circular DMA for RX settings --------------------------------- */

/**
 * \addtogroup UART_CIRCULAR_DMA_FOR_RX_SETTINGS
 *
 * \brief UART FIFO configuration settings
 * \{
 */

/**
 * \brief Circular DMA support for RX
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUART_RX_CIRCULAR_DMA
#define dg_configUART_RX_CIRCULAR_DMA   (0)
#endif

/**
 * \brief UART1's Circular DMA buffer size for RX
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUART1_RX_CIRCULAR_DMA_BUF_SIZE
#define dg_configUART1_RX_CIRCULAR_DMA_BUF_SIZE (0)
#endif

/**
 * \brief UART2's Circular DMA buffer size for RX
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configUART2_RX_CIRCULAR_DMA_BUF_SIZE
#define dg_configUART2_RX_CIRCULAR_DMA_BUF_SIZE (0)
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup ADAPTER_SELECTION
 *
 * \brief Adapter selection
 *
 * When enabled the specific adapter is included in the compilation of the SDK.
 * - 0 : Disabled
 * - 1 : Enabled
 *
 * The default option can be overridden in the application configuration file.
 *
 * \{
   Adapter                        | Setting                                | Default option
   ------------------------------ | -------------------------------------- | :------------------:
   Flash                          | dg_configFLASH_ADAPTER                 | 1
   Inter-Integrated Circuit       | dg_configI2C_ADAPTER                   | 0
   Non Volatile Memory Storage    | dg_configNVMS_ADAPTER                  | 1
   Virtual EERPOM Storage         | dg_configNVMS_VES                      | 1
   Radio                          | dg_configRF_ADAPTER                    | 1
   Serial Peripheral Interface    | dg_configSPI_ADAPTER                   | 0
   UART                           | dg_configUART_ADAPTER                  | 0
   UART for BLE                   | dg_configUART_BLE_ADAPTER              | 0
   Analog to Digital Converter    | dg_configGPADC_ADAPTER                 | 0
   Temperature Sensor             | dg_configTEMPSENS_ADAPTER              | 0
   Battery                        | dg_configBATTERY_ADAPTER               | 0
   Non Volatile Parameters        | dg_configNVPARAM_ADAPTER               | 0
   Crypto                         | dg_configCRYPTO_ADAPTER                | 1
   Keyboard scanner               | dg_configKEYBOARD_SCANNER_ADAPTER      | 0
 *
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */


/* -------------------------------- Adapters (ad_*) selection -------------------------------- */

#ifndef dg_configFLASH_ADAPTER
#define dg_configFLASH_ADAPTER                  (1)
#endif

#ifndef dg_configI2C_ADAPTER
#define dg_configI2C_ADAPTER                    (0)
#endif

#ifndef dg_configNVMS_ADAPTER
#define dg_configNVMS_ADAPTER                   (1)
#endif

#ifndef dg_configNVMS_FLASH_CACHE
#define dg_configNVMS_FLASH_CACHE               (0)
#endif

#ifndef dg_configNVMS_VES
#define dg_configNVMS_VES                       (1)
#endif

#ifndef dg_configRF_ADAPTER
#define dg_configRF_ADAPTER                     (1)
#endif

#ifndef dg_configSPI_ADAPTER
#define dg_configSPI_ADAPTER                    (0)
#endif

#ifndef dg_configUART_ADAPTER
#define dg_configUART_ADAPTER                   (0)
#endif

#ifndef dg_configUART_BLE_ADAPTER
#define dg_configUART_BLE_ADAPTER               (0)
#endif

#ifndef dg_configGPADC_ADAPTER
#define dg_configGPADC_ADAPTER                  (0)
#endif

#ifndef dg_configTEMPSENS_ADAPTER
#define dg_configTEMPSENS_ADAPTER               (0)
#endif

#ifndef dg_configBATTERY_ADAPTER
#define dg_configBATTERY_ADAPTER                (0)
#endif

#ifndef dg_configNVPARAM_ADAPTER
#define dg_configNVPARAM_ADAPTER                (0)
#endif

#ifndef dg_configCRYPTO_ADAPTER
#define dg_configCRYPTO_ADAPTER                 (1)
#endif

#ifndef dg_configKEYBOARD_SCANNER_ADAPTER
#define dg_configKEYBOARD_SCANNER_ADAPTER       (0)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup CONSOLE_IO_SETTINGS
 *
 * \brief Console IO configuration settings
 *
 * \{
   Description                               | Setting                    | Default option
   ----------------------------------------- | -------------------------- | :---------------:
   Enable serial console module              | dg_configUSE_CONSOLE       | 0
   Enable serial console stubbed API         | dg_configUSE_CONSOLE_STUBS | 0
   Enable Command Line Interface module      | dg_configUSE_CLI           | 0
   Enable Command Line Interface stubbed API | dg_configUSE_CLI_STUBS     | 0

   \see console.h cli.h

   \note CLI module requires dg_configUSE_CONSOLE to be enabled.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */

/* -------------------------------------- Console IO configuration settings --------------------- */

#ifndef dg_configUSE_CONSOLE
#define dg_configUSE_CONSOLE            (0)
#endif

#ifndef dg_configUSE_CONSOLE_STUBS
#define dg_configUSE_CONSOLE_STUBS      (0)
#endif

#ifndef dg_configUSE_CLI
#define dg_configUSE_CLI                (0)
#endif

#ifndef dg_configUSE_CLI_STUBS
#define dg_configUSE_CLI_STUBS          (0)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ----------------------------- DGTL ----------------------------------------------------------- */

/**
 * \brief Enable D.GTL interface
 *
 * When this macro is enabled, the DGTL framework is available for use.
 * The framework must furthermore be initialized in the application using
 * dgtl_init(). Additionally, the UART adapter must be initialized accordingly.
 *
 * Please see sdk/middleware/dgtl/include/ for further DGTL configuration
 * (in dgtl_config.h) and API.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 *
 */
#ifndef dg_configUSE_DGTL
#define dg_configUSE_DGTL               (0)
#endif

/**
 * \addtogroup DEBUG_SETTINGS
 * \{
 * \addtogroup SYSTEM_VIEW
 *
 * \brief System View configuration settings
 * \{
 */

/* ----------------------------- Segger System View configuration ------------------------------- */

/**
 * \brief Segger's System View
 *
 * When enabled the application should also call SEGGER_SYSVIEW_Conf() to enable system monitoring.
 * configTOTAL_HEAP_SIZE should be increased by dg_configSYSTEMVIEW_STACK_OVERHEAD bytes for each system task.
 * For example, if there are 8 system tasks configTOTAL_HEAP_SIZE should be increased by
 * (8 * dg_configSYSTEMVIEW_STACK_OVERHEAD) bytes.
 *
 * - 0 : Disabled
 * - 1 : Enabled
 *
 * \bsp_default_note{\bsp_config_option_app,}
 *
 */
#ifndef dg_configSYSTEMVIEW
#define dg_configSYSTEMVIEW             (0)
#endif

/**
 * \brief Stack size overhead when System View API is used
 *
 * All thread stack sizes plus the the stack of IRQ handlers will be increased by that amount
 * to avoid stack overflow when System View is monitoring the system.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configSYSTEMVIEW_STACK_OVERHEAD
#define dg_configSYSTEMVIEW_STACK_OVERHEAD      (256)
#endif

/*
 * Enable/Disable System View monitoring time critical interrupt handlers (BLE, CPM, USB).
 * Disabling ISR monitoring could help reducing assertions triggered by System View monitoring overhead.
 *
 */

/**
 * \brief Let System View monitor BLE related ISRs (BLE_GEN_Handler / BLE_WAKEUP_LP_Handler).
 *
 * - 0 : Disabled
 * - 1 : Enabled
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configSYSTEMVIEW_MONITOR_BLE_ISR
#define dg_configSYSTEMVIEW_MONITOR_BLE_ISR     (1)
#endif

/**
 * \brief Let System View monitor CPM related ISRs (SWTIM1_Handler / WKUP_GPIO_Handler).
 *
 * - 0 : Disabled
 * - 1 : Enabled
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configSYSTEMVIEW_MONITOR_CPM_ISR
#define dg_configSYSTEMVIEW_MONITOR_CPM_ISR     (1)
#endif

/**
 * \brief Let System View monitor USB related ISRs (USB_Handler / VBUS_Handler).
 *
 * - 0 : Disabled
 * - 1 : Enabled
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configSYSTEMVIEW_MONITOR_USB_ISR
#define dg_configSYSTEMVIEW_MONITOR_USB_ISR     (1)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 * \}
 */

/**
 * \addtogroup RF_DRIVER_SETTINGS
 *
 * \brief Doxygen documentation is not yet available for this module.
 *        Please check the source code file(s)
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 * \{
 */
/* ------------------------------- RF driver configuration settings ----------------------------- */

/* Set to 1 to enable the recalibration procedure */
#ifndef dg_configRF_ENABLE_RECALIBRATION
#define dg_configRF_ENABLE_RECALIBRATION        (1)
#endif

/* If RF recalibration is enabled, we need to enable GPADC as well */
#if dg_configRF_ENABLE_RECALIBRATION
#undef dg_configUSE_HW_GPADC
#define dg_configUSE_HW_GPADC           (1)
#undef dg_configGPADC_ADAPTER
#define dg_configGPADC_ADAPTER          (1)
#undef dg_configTEMPSENS_ADAPTER
#define dg_configTEMPSENS_ADAPTER       (1)
#endif

/* If RF is enabled, we need to enable GPADC adapter as well */
#if  dg_configRF_ADAPTER
#undef dg_configGPADC_ADAPTER
#define dg_configGPADC_ADAPTER          (1)
#endif

/* Minimum time before performing an RF recalibration, in FreeRTOS scheduler ticks */
#ifndef dg_configRF_MIN_RECALIBRATION_TIMEOUT
#if (dg_configUSE_LP_CLK == LP_CLK_32000)
#define dg_configRF_MIN_RECALIBRATION_TIMEOUT   (1000) // ~2 secs
#elif (dg_configUSE_LP_CLK == LP_CLK_32768)
#define dg_configRF_MIN_RECALIBRATION_TIMEOUT   (1024) // ~2 secs
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
#define dg_configRF_MIN_RECALIBRATION_TIMEOUT   (1000)
#else /* LP_CLK_ANY */
// Must be defined in the custom_config_<>.h file. Should result in ~2sec.
#endif
#endif

/* Maximum time before performing an RF recalibration, in FreeRTOS scheduler ticks
 * If this time has elapsed, and RF is about to be powered off, recalibration will
 * be done unconditionally. Set to 0 to disable this functionality */
#ifndef dg_configRF_MAX_RECALIBRATION_TIMEOUT
#define dg_configRF_MAX_RECALIBRATION_TIMEOUT   (0) // Disabled
#endif

/* Timeout value (in FreeRTOS scheduler ticks) for timer to initiate RF recalibration.
 * This will happen at ANY TIME RF is ON and CONFIGURED, EVEN IF A MAC IS RX/TXing DURING
 * THIS TIME, in contrary to dg_configRF_MAX_RECALIBRATION_TIMEOUT, that will be
 * performed ONLY when powering off RF.
 * This is intended for applications where RF is always on, so there is no
 * opportunity to be recalibrated the normal way (i.e. during poweroff)
 *
 * Set to 0 to disable this functionality */
#ifndef dg_configRF_RECALIBRATION_TIMER_TIMEOUT
#define dg_configRF_RECALIBRATION_TIMER_TIMEOUT (0) // Disabled
#endif


/* Minimum temp difference before performing an RF recalibration, in oC*/
#ifndef dg_configRF_MIN_RECALIBRATION_TEMP_DIFF
#ifdef CONFIG_USE_FTDF
        #define dg_configRF_MIN_RECALIBRATION_TEMP_DIFF (5)
#else
        #define dg_configRF_MIN_RECALIBRATION_TEMP_DIFF (10)
#endif
#endif

/* Duration of recalibration procedure, in lp clk cycles */
#ifndef dg_configRF_RECALIBRATION_DURATION
# if defined(CONFIG_USE_FTDF) && defined(CONFIG_USE_BLE)
#  if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
#   define dg_configRF_RECALIBRATION_DURATION (230)
#  elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
#   define dg_configRF_RECALIBRATION_DURATION cm_rcx_us_2_lpcycles((uint32_t)(230 * 30.5))
#  else /* LP_CLK_ANY */
    // Must be defined in the custom_config_<>.h file. It should be ~7msec.
#  endif
# else
#  if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
#   define dg_configRF_RECALIBRATION_DURATION (131)
#  elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
#   define dg_configRF_RECALIBRATION_DURATION cm_rcx_us_2_lpcycles((uint32_t)(131 * 30.5))
#  else /* LP_CLK_ANY */
    // Must be defined in the custom_config_<>.h file. It should be ~4msec.
#  endif
# endif
#endif

#ifndef dg_configRF_IFF_CALIBRATION_TIMEOUT
#if ((dg_configUSE_LP_CLK == LP_CLK_32000) || (dg_configUSE_LP_CLK == LP_CLK_32768))
        #define dg_configRF_IFF_CALIBRATION_TIMEOUT     (40)
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
        #define dg_configRF_IFF_CALIBRATION_TIMEOUT     cm_rcx_us_2_lpcycles((uint32_t)(40 * 30.5))
#else /* LP_CLK_ANY */
// Must be defined in the custom_config_<>.h file. It should be ~1.2msec.
#  endif
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup BLE_EVENT_NOTIFICATIONS
 *
 * \brief Doxygen documentation is not yet available for this module.
 *        Please check the source code file(s)
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 *
 * \{
 */
/* -----------------------------------BLE event notifications configuration --------------------- */

/*
 * BLE ISR event Notifications
 *
 * This facility enables the user app to receive notifications for BLE
 * ISR events. These events can be received either directly from inside
 * the BLE ISR, or as task notifications to the Application Task
 * registered to the BLE manager.
 *
 * To enable, define dg_configBLE_EVENT_NOTIF_TYPE to either
 * BLE_EVENT_NOTIF_USER_ISR or BLE_EVENT_NOTIF_USER_TASK.
 *
 * - When dg_configBLE_EVENT_NOTIF_TYPE == BLE_EVENT_NOTIF_USER_ISR, the user
 *   can define the following macros on his app configuration:
 *
 *   dg_configBLE_EVENT_NOTIF_HOOK_END_EVENT    : The BLE End Event
 *   dg_configBLE_EVENT_NOTIF_HOOK_CSCNT_EVENT  : The BLE CSCNT Event
 *   dg_configBLE_EVENT_NOTIF_HOOK_FINE_EVENT   : The BLE FINE Event
 *
 *   These macros must be actually set to the names of functions defined inside
 *   the user application, having a prototype of
 *
 *     void func(void);
 *
 *   (The user app does not need to explicitly define the prototype).
 *
 *   If one of these macros is not defined, the respective notification
 *   will be suppressed.
 *
 *   Note that these functions will be called in ISR context, directly from the
 *   BLE ISR. They should therefore be very fast and should NEVER block.
 *
 * - When dg_configBLE_EVENT_NOTIF_TYPE == BLE_EVENT_NOTIF_USER_TASK, the user
 *   app will receive task notifications on the task registered to the BLE
 *   manager.
 *
 *   Notifications will be received using the following bit masks:
 *
 *   - dg_configBLE_EVENT_NOTIF_MASK_END_EVENT: End Event Mask (Def: bit 24)
 *   - dg_configBLE_EVENT_NOTIF_MASK_CSCNT_EVENT: CSCNT Event Mask (Def: bit 25)
 *   - dg_configBLE_EVENT_NOTIF_MASK_FINE_EVENT: FINE Event Mask (Def: bit 26)
 *
 *   These macros, defining the task notification bit masks, can be redefined as
 *   needed.
 *
 *   If one of the macros for callback functions presented in the previous
 *   section (for direct ISR notifications) is defined, while in this mode,
 *   the function with the same name will be called directly from the isr
 *   instead of sending a task notification for this particular event to
 *   the app task.
 *
 *   Macro dg_configBLE_EVENT_NOTIF_RUNTIME_CONTROL (Default: 1) enables/disables
 *   runtime control/masking of notifications.
 *
 *   If dg_configBLE_EVENT_NOTIF_RUNTIME_CONTROL == 1, then task notifications
 *   must be enabled/disabled using the
 *   ble_event_notif_[enable|disable]_[end|cscnt|fine]_event() functions.
 *   By default all notifications are disabled.
 *
 *   If dg_configBLE_EVENT_NOTIF_RUNTIME_CONTROL == 0, all notifications will
 *   be sent unconditionally to the application task.
 *
 */
#ifndef dg_configBLE_EVENT_NOTIF_TYPE
# define dg_configBLE_EVENT_NOTIF_TYPE BLE_EVENT_NOTIF_USER_ISR
#endif

#if dg_configBLE_EVENT_NOTIF_TYPE == BLE_EVENT_NOTIF_USER_TASK

/* Default implementation of BLE event task notifications. This
 * implementation allows a user task to register for notifications,
 * and to get task notifications for the available events
 */

/* Default task notification masks for the various events for which
 * the framework provides notifications
 */
# ifndef dg_configBLE_EVENT_NOTIF_MASK_END_EVENT
#  define dg_configBLE_EVENT_NOTIF_MASK_END_EVENT (1 << 24)
# endif

# ifndef dg_configBLE_EVENT_NOTIF_MASK_CSCNT_EVENT
#  define dg_configBLE_EVENT_NOTIF_MASK_CSCNT_EVENT (1 << 25)
# endif

# ifndef dg_configBLE_EVENT_NOTIF_MASK_FINE_EVENT
#  define dg_configBLE_EVENT_NOTIF_MASK_FINE_EVENT (1 << 26)
# endif

/* Implement event hook functions */
# ifndef dg_configBLE_EVENT_NOTIF_HOOK_END_EVENT
#  define dg_configBLE_EVENT_NOTIF_HOOK_END_EVENT ble_event_notif_app_task_end_event
# endif

# ifndef dg_configBLE_EVENT_NOTIF_HOOK_CSCNT_EVENT
#  define dg_configBLE_EVENT_NOTIF_HOOK_CSCNT_EVENT ble_event_notif_app_task_cscnt_event
# endif

# ifndef dg_configBLE_EVENT_NOTIF_HOOK_FINE_EVENT
#  define dg_configBLE_EVENT_NOTIF_HOOK_FINE_EVENT ble_event_notif_app_task_fine_event
# endif

/*
 * Allow runtime control of (un)masking notifications
 */
# ifndef dg_configBLE_EVENT_NOTIF_RUNTIME_CONTROL
#  define dg_configBLE_EVENT_NOTIF_RUNTIME_CONTROL 1
# endif

#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ----------------------------------- BLE hooks configuration ---------------------------------- */

/* Name of the function that is called to block BLE from sleeping under certain conditions
 *
 * The function must be declared as:
 *      unsigned char my_block_sleep(void);
 * The return value of the function controls whether the BLE will be allowed to go to sleep or not,
 *      0: the BLE may go to sleep, if possible
 *      1: the BLE is not allowed to go to sleep. The caller (BLE Adapter) may block or not,
 *         depending on the BLE stack status.
 *
 * dg_configBLE_HOOK_BLOCK_SLEEP should be set to my_block_sleep in this example.
 */
#ifndef dg_configBLE_HOOK_BLOCK_SLEEP
/* Already undefined - Nothing to do. */
#endif

/* Name of the function that is called to modify the PTI value (Payload Type Indication) when
 * arbitration is used
 *
 * Arbitration is needed only when another external radio is present. The function must be declared
 * as:
 *      unsigned char my_pti_modify(void);
 * Details for the implementation of such a function will be provided when the external radio
 * arbitration functionality is integrated.
 *
 * dg_configBLE_HOOK_PTI_MODIFY should be set to my_pti_modify in this example.
 *
 * See also the comment about the <BLE code hooks> in ble_config.h for more info.
 */
#ifndef dg_configBLE_HOOK_PTI_MODIFY
/* Already undefined - Nothing to do. */
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \addtogroup DEBUG_SETTINGS
 *
 * \{
 */
/* ---------------------------------- OS related configuration ---------------------------------- */

/**
 * \brief Monitor OS heap allocations
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configTRACK_OS_HEAP
#define dg_configTRACK_OS_HEAP          (0)
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ---------------------------------- Heap size configuration ----------------------------------- */

/**
 * \brief Heap size for used libc malloc()
 *
 * Specifies the amount of RAM that will be used as heap for libc malloc() function.
 * It can be configured in bare metal projects to match application's requirements.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef __HEAP_SIZE
# if defined (CONFIG_RETARGET) || defined (CONFIG_RTT)
#  define __HEAP_SIZE  0x0600
# else
#  define __HEAP_SIZE  0x0100
# endif
#endif

/* flag used by linker scripts */
#if (dg_configBLACK_ORCA_IC_REV != BLACK_ORCA_IC_REV_A)
#define __HEAP_IS_LESS_THAN_0x200               (__HEAP_SIZE < 0x200)
#endif

/* ---------------------------------------------------------------------------------------------- */

/* --------------------------------- Stack size configuration ----------------------------------- */

/**
 * \brief Stack size for main() function and interrupt handlers.
 *
 * Specifies the amount of RAM that will be used as stack for the main() function and the interrupt
 * handlers.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef __STACK_SIZE
#define __STACK_SIZE  0x0200
#endif

/* ---------------------------------------------------------------------------------------------- */

/* ----------------------------------- RF FEM CONFIGURATION ------------------------------------- */

#include "bsp_fem.h"

/* ---------------------------------------------------------------------------------------------- */


/* ----------------------------------- DEBUG CONFIGURATION -------------------------------------- */

#include "bsp_debug.h"

/* ---------------------------------------------------------------------------------------------- */

/* ---------------------------------- MEMORY LAYOUT CONFIGURATION ------------------------------- */

/**
 * \addtogroup MEMORY_LAYOUT_SETTINGS
 *
 * \brief Memory layout configuration settings.
 * \{
 */

/**
 * \brief Controls the retention RAM optimization.
 *
 * - 0 : All RAM is retained.
 * - 1 : Retention memory size is optimal.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configOPTIMAL_RETRAM
#define dg_configOPTIMAL_RETRAM                 (0)
#endif
/**
 * \addtogroup OTP_PROJECT_MEMORY_LAYOUT_SETTINGS
 *
 * \brief Memory layout configuration settings for a OTP project.
 * \{
 */

/**
 * \brief Code size in OTP projects, no product specific.
 *
 * \note Code size cannot be more than 58K.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CODE_SIZE
#define dg_configOTP_CODE_SIZE                                  ( 58 * 1024)
#endif

/**
 * \brief RAM-block size in cached mode, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RAM_SIZE
#define dg_configOTP_CACHED_RAM_SIZE                            (64 * 1024)
#endif

/**
 * \brief Retained-RAM-0-block size in cached mode, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RETRAM_0_SIZE
#define dg_configOTP_CACHED_RETRAM_0_SIZE                       ( 64 * 1024)
#endif

/**
 * \brief Retained-RAM-1-block size in cached mode, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RETRAM_1_SIZE
#define dg_configOTP_CACHED_RETRAM_1_SIZE                       (  0 * 1024)
#endif

/**
 * \brief Retained-RAM-0-block size in mirror mode, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_MIRROR_RETRAM_0_SIZE
#define dg_configOTP_MIRROR_RETRAM_0_SIZE                       ( 48 * 1024)
#endif

/**
 * \brief Retained-RAM-1-block size in mirror mode, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_MIRROR_RETRAM_1_SIZE
#define dg_configOTP_MIRROR_RETRAM_1_SIZE                       (  0 * 1024)
#endif



/**
 * \brief Retained-RAM-0-block size for optimal retention memory in cached mode, no product specific.
 *
 * \see dg_configOPTIMAL_RETRAM option
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configOTP_CACHED_OPTIMAL_RETRAM_0_SIZE
#define dg_configOTP_CACHED_OPTIMAL_RETRAM_0_SIZE               ( 32 * 1024)
#endif

/**
 * \brief Retained-RAM-1-block size for optimal retention memory in cached mode, no product specific.
 *
 * \see dg_configOPTIMAL_RETRAM option
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configOTP_CACHED_OPTIMAL_RETRAM_1_SIZE
#define dg_configOTP_CACHED_OPTIMAL_RETRAM_1_SIZE               ( 32 * 1024)
#endif

/**
 * \brief RAM-block size in mirror mode, no product specific.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_MIRROR_RAM_SIZE
#define dg_configOTP_MIRROR_RAM_SIZE                            ( 16 * 1024)
#endif

/**
 * \brief Code size in OTP projects for DA14680/1-01.
 *
 * \note Code size cannot be more than 58K.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CODE_SIZE_AE
#define dg_configOTP_CODE_SIZE_AE dg_configOTP_CODE_SIZE
#endif

/**
 * \brief Code size in OTP projects for  DA14682/3-00, DA15XXX-00.
 *
 * \note Code size cannot be more than 58K.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CODE_SIZE_BB
#define dg_configOTP_CODE_SIZE_BB dg_configOTP_CODE_SIZE
#endif

/**
 * \brief RAM-block size in cached mode for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RAM_SIZE_AE
#define dg_configOTP_CACHED_RAM_SIZE_AE dg_configOTP_CACHED_RAM_SIZE
#endif

/**
 * \brief Retained-RAM-0-block size in cached mode for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RETRAM_0_SIZE_AE
#define dg_configOTP_CACHED_RETRAM_0_SIZE_AE dg_configOTP_CACHED_RETRAM_0_SIZE
#endif

/**
 * \brief Retained-RAM-1-block size in cached mode for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RETRAM_1_SIZE_AE
#define dg_configOTP_CACHED_RETRAM_1_SIZE_AE dg_configOTP_CACHED_RETRAM_1_SIZE
#endif

/**
 * \brief Retained-RAM-0-block size for optimal retention memory in cached mode for DA14680/1-01.
 *
 * \see dg_configOPTIMAL_RETRAM option
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configOTP_CACHED_OPTIMAL_RETRAM_0_SIZE_AE
#define dg_configOTP_CACHED_OPTIMAL_RETRAM_0_SIZE_AE dg_configOTP_CACHED_OPTIMAL_RETRAM_0_SIZE
#endif

/**
 * \brief Retained-RAM-1-block size for optimal retention memory in cached mode for DA14680/1-01.
 *
 * \see dg_configOPTIMAL_RETRAM option
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configOTP_CACHED_OPTIMAL_RETRAM_1_SIZE_AE
#define dg_configOTP_CACHED_OPTIMAL_RETRAM_1_SIZE_AE dg_configOTP_CACHED_OPTIMAL_RETRAM_1_SIZE
#endif

/**
 * \brief RAM-block size in cached mode for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RAM_SIZE_BB
#define dg_configOTP_CACHED_RAM_SIZE_BB dg_configOTP_CACHED_RAM_SIZE
#endif

/**
 * \brief Retained-RAM-0-block size in cached mode for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RETRAM_0_SIZE_BB
#define dg_configOTP_CACHED_RETRAM_0_SIZE_BB dg_configOTP_CACHED_RETRAM_0_SIZE
#endif

/**
 * \brief Retained-RAM-1-block size in cached mode for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_CACHED_RETRAM_1_SIZE_BB
#define dg_configOTP_CACHED_RETRAM_1_SIZE_BB dg_configOTP_CACHED_RETRAM_1_SIZE
#endif

/**
 * \brief RAM-block size in mirror mode for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_MIRROR_RAM_SIZE_AE
#define dg_configOTP_MIRROR_RAM_SIZE_AE dg_configOTP_MIRROR_RAM_SIZE
#endif

/**
 * \brief Retained-RAM-1-block size in mirror mode for DA14680/1-01
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_MIRROR_RETRAM_1_SIZE_AE
#define dg_configOTP_MIRROR_RETRAM_1_SIZE_AE  dg_configOTP_MIRROR_RETRAM_1_SIZE
#endif

/**
 * \brief RAM-block size in mirror mode for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_MIRROR_RAM_SIZE_BB
#define dg_configOTP_MIRROR_RAM_SIZE_BB dg_configOTP_MIRROR_RAM_SIZE
#endif

/**
 * \brief Retained-RAM-0-block size in mirror mode for DA14682/3-00, DA15XXX-00
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_MIRROR_RETRAM_0_SIZE_BB
#define dg_configOTP_MIRROR_RETRAM_0_SIZE_BB dg_configOTP_MIRROR_RETRAM_0_SIZE
#endif

/**
 * \brief Retained-RAM-0-block size in mirror mode for DA14682/3-00, DA15XXX-00
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configOTP_MIRROR_RETRAM_1_SIZE_BB
#define dg_configOTP_MIRROR_RETRAM_1_SIZE_BB dg_configOTP_MIRROR_RETRAM_1_SIZE
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup QSPI_PROJECT_MEMORY_LAYOUT_SETTINGS
 *
 * \brief Memory layout configuration settings for a QSPI project
 * \{
 */

/**
 * \brief Code size in QSPI projects, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CODE_SIZE
#define dg_configQSPI_CODE_SIZE                                 (128 * 1024)
#endif

/**
 * \brief RAM-block size in cached mode, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RAM_SIZE
#define dg_configQSPI_CACHED_RAM_SIZE                           ( 64 * 1024)
#endif

/**
 * \brief Retained-RAM-0-block size in cached mode, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RETRAM_0_SIZE
#define dg_configQSPI_CACHED_RETRAM_0_SIZE                      ( 64 * 1024)
#endif

/**
 * \brief Retained-RAM-1-block size in cached mode, no product specific.
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RETRAM_1_SIZE
#define dg_configQSPI_CACHED_RETRAM_1_SIZE                      (  0 * 1024)
#endif

/**
 * \brief Retained-RAM-0-block size for optimal retention memory in cached mode, no product specific.
 *
 * \see dg_configOPTIMAL_RETRAM option
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configQSPI_CACHED_OPTIMAL_RETRAM_0_SIZE
#define dg_configQSPI_CACHED_OPTIMAL_RETRAM_0_SIZE              ( 32 * 1024)
#endif

/**
 * \brief Retained-RAM-1-block size for optimal retention memory in cached mode, no product specific.
 *
 * \see dg_configOPTIMAL_RETRAM option
 *
 * \note Defining the corresponding product-specific configuration macro will override
 *       this one.
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configQSPI_CACHED_OPTIMAL_RETRAM_1_SIZE
#define dg_configQSPI_CACHED_OPTIMAL_RETRAM_1_SIZE              ( 32 * 1024)
#endif

/**
 * \brief Code size in QSPI projects for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CODE_SIZE_AE
#define dg_configQSPI_CODE_SIZE_AE dg_configQSPI_CODE_SIZE
#endif

/**
 * \brief Code size in QSPI projects for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CODE_SIZE_BB
#define dg_configQSPI_CODE_SIZE_BB dg_configQSPI_CODE_SIZE
#endif

/**
 * \brief RAM-block size in cached mode for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RAM_SIZE_AE
#define dg_configQSPI_CACHED_RAM_SIZE_AE dg_configQSPI_CACHED_RAM_SIZE
#endif

/**
 * \brief Retained-RAM-0-block size in cached mode for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RETRAM_0_SIZE_AE
#define dg_configQSPI_CACHED_RETRAM_0_SIZE_AE dg_configQSPI_CACHED_RETRAM_0_SIZE
#endif

/**
 * \brief Retained-RAM-1-block size in cached mode for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RETRAM_1_SIZE_AE
#define dg_configQSPI_CACHED_RETRAM_1_SIZE_AE dg_configQSPI_CACHED_RETRAM_1_SIZE
#endif

/**
 * \brief Retained-RAM-0-block size for optimal retention memory in cached mode for DA14680/1-01.
 *
 * \see dg_configOPTIMAL_RETRAM option
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configQSPI_CACHED_OPTIMAL_RETRAM_0_SIZE_AE
#define dg_configQSPI_CACHED_OPTIMAL_RETRAM_0_SIZE_AE dg_configQSPI_CACHED_OPTIMAL_RETRAM_0_SIZE
#endif

/**
 * \brief Retained-RAM-1-block size for optimal retention memory in cached mode for DA14680/1-01.
 *
 * \see dg_configOPTIMAL_RETRAM option
 *
 * \bsp_default_note{\bsp_config_option_app, \bsp_config_option_expert_only}
 */
#ifndef dg_configQSPI_CACHED_OPTIMAL_RETRAM_1_SIZE_AE
#define dg_configQSPI_CACHED_OPTIMAL_RETRAM_1_SIZE_AE dg_configQSPI_CACHED_OPTIMAL_RETRAM_1_SIZE
#endif

/**
 * \brief RAM-block size in cached mode for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RAM_SIZE_BB
#define dg_configQSPI_CACHED_RAM_SIZE_BB dg_configQSPI_CACHED_RAM_SIZE
#endif

/**
 * \brief Retained-RAM-0-block size in cached mode for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RETRAM_0_SIZE_BB
#define dg_configQSPI_CACHED_RETRAM_0_SIZE_BB dg_configQSPI_CACHED_RETRAM_0_SIZE
#endif

/**
 * \brief Retained-RAM-1-block size in cached mode for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configQSPI_CACHED_RETRAM_1_SIZE_BB
#define dg_configQSPI_CACHED_RETRAM_1_SIZE_BB dg_configQSPI_CACHED_RETRAM_1_SIZE
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/**
 * \addtogroup RAM_PROJECT_MEMORY_LAYOUT_SETTINGS
 *
 * \brief Memory layout configuration settings for a RAM project
 * \{
 */

/**
 * \brief Code size in RAM projects for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configRAM_CODE_SIZE_AE
#define dg_configRAM_CODE_SIZE_AE                      ( 79 * 1024)
#endif

/**
 * \brief RAM-block size for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configRAM_RAM_SIZE_AE
#define dg_configRAM_RAM_SIZE_AE                       ( 16 * 1024)
#endif

/**
 * \brief Retained-RAM-0-block size for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configRAM_RETRAM_0_SIZE_AE
#define dg_configRAM_RETRAM_0_SIZE_AE                  (128 * 1024 - dg_configRAM_CODE_SIZE_AE)
#endif

/**
 * \brief Retained-RAM-1-block size for DA14680/1-01.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configRAM_RETRAM_1_SIZE_AE
#define dg_configRAM_RETRAM_1_SIZE_AE                  (  0 * 1024)
#endif

/**
 * \brief Code size in RAM projects for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configRAM_CODE_SIZE_BB
#define dg_configRAM_CODE_SIZE_BB                      (144 * 1024)
#endif

/**
 * \brief RAM-block size for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configRAM_RAM_SIZE_BB
#define dg_configRAM_RAM_SIZE_BB                       ( 15 * 1024)
#endif

/**
 * \brief Retained-RAM-0-block size for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configRAM_RETRAM_0_SIZE_BB
#define dg_configRAM_RETRAM_0_SIZE_BB                  ( 49 * 1024)
#endif

/**
 * \brief Retained-RAM-0-block size for DA14682/3-00, DA15XXX-00.
 *
 * \bsp_default_note{\bsp_config_option_app,}
 */
#ifndef dg_configRAM_RETRAM_1_SIZE_BB
#define dg_configRAM_RETRAM_1_SIZE_BB                  (  0 * 1024)
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */

/* ---------------------------------------------------------------------------------------------- */

/**
 * \}
 */


/**
 * \}
 */

#endif /* BSP_DEFAULTS_H_ */

/**
\}
\}
\}
*/
