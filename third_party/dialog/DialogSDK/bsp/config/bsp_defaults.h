/**
****************************************************************************************
*
* @file bsp_defaults.h
*
* @brief Board Support Package. System Configuration file default values.
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
****************************************************************************************
*/

#ifndef BSP_DEFAULTS_H_
#define BSP_DEFAULTS_H_

/**
 * \addtogroup POWER_SETTINGS
 *
 * \{
 */

/* -------------------------------------- Power settings ---------------------------------------- */

/**
 * \brief Controls the power-mode of the application.
 */
#ifndef dg_configPOWER_CONFIG
#define dg_configPOWER_CONFIG           (POWER_CONFIGURATION_1)
#endif

/**
 * \brief Code to disable the debugger and its pins and reset any floating GPIOs to avoid leakage.
 */
#ifndef dg_codeCUT_DEBUGGER_PINS_LEAKAGE
#define dg_codeCUT_DEBUGGER_PINS_LEAKAGE                                \
        do {                                                            \
                if (0) {                                                \
                        GPIO->P15_MODE_REG = 0x200;                     \
                        GPIO->P16_MODE_REG = 0x200;                     \
                        hw_cpm_disable_debugger();                      \
                }                                                       \
        } while (0)
#endif

/**
 * \brief Code to restore the debugger and its pins after waking-up.
 */
#ifndef dg_codeRESTORE_DEBUGGER_PINS
#define dg_codeRESTORE_DEBUGGER_PINS                                    \
        do {                                                            \
                if (0) {                                                \
                        hw_cpm_enable_debugger();                       \
                }                                                       \
        } while (0)
#endif

/**
 * When set to 1, the system will go to sleep and never exit allowing for the sleep current to be
 * measured.
 */
#ifndef dg_configTESTMODE_MEASURE_SLEEP_CURRENT
#define dg_configTESTMODE_MEASURE_SLEEP_CURRENT (0)
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \} POWER_SETTINGS
 */

/**
 * \addtogroup LOW_POWER_CLOCK_SETTINGS
 *
 * \brief Doxygen documentation is not yet available for this module.
 *        Please check the source code file(s)
 * \{
 */

/* --------------------------------- Low-Power clock settings ----------------------------------- */

/*
 * Maximum sleep time the sleep timer supports
 *
 * dg_configTim1Prescaler can be zero. if dg_configTim1Prescaler is not zero then
 * ( dg_configTim1Prescaler + 1 ) MUST be a power of 2! */
#if (dg_configUSE_LP_CLK == LP_CLK_32000)
#define dg_configTim1Prescaler          (3)
#define dg_configMaxSleepTime           (8)     // msec
#elif (dg_configUSE_LP_CLK == LP_CLK_32768)
#define dg_configTim1Prescaler          (3)
#define dg_configMaxSleepTime           (8)     // msec
#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
#define dg_configTim1Prescaler          (0)
#define dg_configMaxSleepTime           (8)     // msec
#else
#error "dg_configUSE_LP_CLK has invalid setting"
#endif

#if (dg_configTim1Prescaler && (((dg_configTim1Prescaler+1) / 2) * 2) != (dg_configTim1Prescaler+1))
#error "dg_configTim1Prescaler+1 is not 0 or a power of 2!"
#endif

#if (dg_configTim1Prescaler == 0)
#define dg_configTim1PrescalerBitRange  (0)
#elif (dg_configTim1Prescaler == 1)
#define dg_configTim1PrescalerBitRange  (1)
#elif (dg_configTim1Prescaler == 3)
#define dg_configTim1PrescalerBitRange  (2)
#else
#error "dg_configTim1Prescaler is larger than 3!"
#endif
/* ---------------------------------------------------------------------------------------------- */

/**
 * \} LOW_POWER_CLOCK_SETTINGS
 */

/**
 * \addtogroup IMAGE_CONFIGURATION_SETTINGS
 *
 * \{
 */

/* ------------------------------- Image configuration settings --------------------------------- */

/**
 * \brief The chip revision that we compile for.
 */
#ifndef dg_configBLACK_ORCA_IC_REV
#define dg_configBLACK_ORCA_IC_REV      BLACK_ORCA_IC_REV_A
#endif

/**
 * \brief The chip stepping that we compile for.
 */
#ifndef dg_configBLACK_ORCA_IC_STEP
#define dg_configBLACK_ORCA_IC_STEP     BLACK_ORCA_IC_STEP_D
#endif

/**
 * \brief The motherboard revision we compile for.
 */
#ifndef dg_configBLACK_ORCA_MB_REV
#define dg_configBLACK_ORCA_MB_REV      BLACK_ORCA_MB_REV_B
#endif

/**
 * \brief When set to 1, the execution mode and the code location are taken from the OTP Header and 
 *        not from the bsp_config.h file.
 */
#ifndef dg_configCONFIG_VIA_OTP_HEADER
#define dg_configCONFIG_VIA_OTP_HEADER  (0)
#endif

/**
 * \brief When set to 1, the values of the trim registers are taken from the Flash.
 */
#ifndef dg_configCONFIG_HEADER_IN_FLASH
#if ((dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
                && (dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_D))
#define dg_configCONFIG_HEADER_IN_FLASH (1)
#else
#define dg_configCONFIG_HEADER_IN_FLASH (0)
#endif
#endif

/**
 * \brief Controls how the image is built.
 *  - DEVELOPMENT_MODE: various debugging options are included.
 *  - PRODUCTION_MODE: all code used for debugging is removed.
 */
#ifndef dg_configIMAGE_SETUP
#define dg_configIMAGE_SETUP            DEVELOPMENT_MODE
#endif

/**
 * \brief When set to 1, the application will write the proper code in the QFIS FIFO.
 */
#ifndef dg_configWRITE_QFIS_UCODE
#if ( (dg_configIMAGE_SETUP == DEVELOPMENT_MODE) \
                || ( (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
                                && (dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_D) ) )
#define dg_configWRITE_QFIS_UCODE       (1)
#else
#define dg_configWRITE_QFIS_UCODE       (0)
#endif
#endif

/**
 * \brief When set to 1, the OTP copy will be emulated when in DEVELOPMENT_MODE.
 */
#ifndef dg_configEMULATE_OTP_COPY
#define dg_configEMULATE_OTP_COPY       (0)
#endif

/**
 * \brief When set to 1, the QSPI copy will be emulated when in DEVELOPMENT_MODE (Not Applicable!).
 */
#ifndef dg_configEMULATE_QSPI_COPY
#define dg_configEMULATE_QSPI_COPY      (0)
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \} IMAGE_CONFIGURATION_SETTINGS
 */

/**
 * \addtogroup SYSTEM_CONFIGURATION_SETTINGS
 *
 * \{
 */

/* ------------------------------- System configuration settings -------------------------------- */

#if (dg_configCONFIG_VIA_OTP_HEADER == 0)

/**
 * \brief Low Power clock used (LP_CLK_32000, LP_CLK_32768, LP_CLK_RCX)
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
 * \warning When #dg_configIMAGE_SETUP is set to DEVELOPMENT_MODE, this setting is overriden!
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
 */
# ifndef dg_configCODE_LOCATION
# define dg_configCODE_LOCATION         NON_VOLATILE_IS_NONE
# endif

/**
 * \brief Frequency of the crystal connected to the XTAL Oscillator: 16MHz or 32MHz.
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
 */
# ifndef dg_configEXT_LP_IS_DIGITAL
# define dg_configEXT_LP_IS_DIGITAL     (0)
# endif

#else

# undef dg_configUSE_LP_CLK
# undef dg_configEXEC_MODE
# undef dg_configCODE_LOCATION
# undef dg_configEXT_CRYSTAL_FREQ

// Configuration is done using variables that get initialized from the OTP
# define dg_configUSE_LP_CLK            lp_clk_sel
# define dg_configEXEC_MODE             code_execution_mode
# define dg_configCODE_LOCATION         code_location
# define dg_configEXT_CRYSTAL_FREQ      xtal16_crystal_freq

#endif // dg_configCONFIG_VIA_OTP_HEADER == 0


/**
 * \brief Deep sleep configuration
 *
 * When set to 1, the system is forced to enter into clockless sleep during deep sleep. This has
 * the following implications:
 * - the 1V8 and 1V8P rails are turned off
 * - the 3V3 rail can provide up to 2mA current and the voltage level is at ~2V
 * - the LP clock is stopped. In case of RCX, it is restarted immediately at wake-up. In case of
 *   XTAL32K, dg_configINITIAL_SLEEP_DELAY_TIME must pass before the system is allowed to go to
 *   sleep again
 * - BOD protection is not available.
 * When set to 0, it is like the extended sleep mode with the difference that the Timer1 is
 * disabled.
 */
#ifndef dg_configFORCE_DEEP_SLEEP
#define dg_configFORCE_DEEP_SLEEP       (0)
#endif

/**
 * \brief Timer 1 usage
 *
 * When set to 0, Timer1 is reserved for the OS tick.
 */
#ifndef dg_configUSER_CAN_USE_TIMER1
#define dg_configUSER_CAN_USE_TIMER1    (0)
#endif

/**
 * \brief Time needed for the settling of the LP clock, in msec.
 */
#ifndef dg_configXTAL32K_SETTLE_TIME
#define dg_configXTAL32K_SETTLE_TIME    (8000)
#endif

/**
 * \brief Sleep delay time
 *
 * At startup, the system will stay active for at least this time period before it is allowed to go
 * to sleep, in msec.
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
 */
#ifndef dg_configXTAL16_SETTLE_TIME
#if (dg_configUSE_LP_CLK != LP_CLK_RCX)
#define dg_configXTAL16_SETTLE_TIME     (85 + 5)
#else
#define dg_configXTAL16_SETTLE_TIME     cm_rcx_us_2_lpcycles((uint32_t)((85 + 5) * 30.5))
#endif
#endif

/**
 * \brief XTAL16 settle time RC32K
 */
#define dg_configXTAL16_SETTLE_TIME_RC32K       (110)

/**
 * \brief RC16 wakeup time
 *
 * This is the maximum time, in LP cycles, needed to wake-up the chip and start executing code
 * using RC16.
 */
#define dg_configWAKEUP_RC16_TIME       (16)

/**
 * \brief XTAL16 wakeup time
 *
 * Wake up N LP cycles before "time 0" to have XTAL16 ready when needed.
 */
#define dg_configWAKEUP_XTAL16_TIME     (dg_configXTAL16_SETTLE_TIME + dg_configWAKEUP_RC16_TIME)

/**
 * \brief OS tick restore time
 *
 * This is the time, in LP cycles, required from the OS to restore the tick timer.
 */
#ifndef dg_configOS_TICK_RESTORE_TIME
#ifdef RELEASE_BUILD
#if (dg_configUSE_LP_CLK != LP_CLK_RCX)
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  (dg_configTim1Prescaler ? 2 + dg_configTim1Prescaler : 3)
        #else
                #if (dg_configIMAGE_SETUP == PRODUCTION_MODE)
                        #define dg_configOS_TICK_RESTORE_TIME  (dg_configTim1Prescaler ? 3 + dg_configTim1Prescaler : 4)
                #else
                        #define dg_configOS_TICK_RESTORE_TIME  (dg_configTim1Prescaler ? 3 + dg_configTim1Prescaler : 4)
                #endif
        #endif
#else
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(120)
        #else
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(120)
        #endif
#endif

#else // RELEASE_BUILD
#if (dg_configUSE_LP_CLK != LP_CLK_RCX)
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  (40)
        #else
                #define dg_configOS_TICK_RESTORE_TIME  (72)
        #endif
#else
        #if (dg_configCODE_LOCATION != NON_VOLATILE_IS_FLASH)
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(1200)
        #else
                #define dg_configOS_TICK_RESTORE_TIME  cm_rcx_us_2_lpcycles(2400)
        #endif
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
 */
#if (dg_configEXEC_MODE != MODE_IS_MIRRORED)
#undef dg_configIMAGE_COPY_TIME
#define dg_configIMAGE_COPY_TIME        (0)
#elif !defined(dg_configIMAGE_COPY_TIME)
        #if (dg_configUSE_LP_CLK != LP_CLK_RCX)
                #define dg_configIMAGE_COPY_TIME        (64)
        #else
                #define dg_configIMAGE_COPY_TIME        cm_rcx_us_2_lpcycles(1950)
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
 */
#ifndef dg_configMEM_RETENTION_MODE
#define dg_configMEM_RETENTION_MODE     (0)
#endif

/**
 * \brief Retention memory configuration when "no-Image copy at wake-up" is selected.
 */
#ifndef dg_configMEM_RETENTION_MODE_PRESERVE_IMAGE
# if (dg_configEXEC_MODE == MODE_IS_CACHED)
        #define dg_configMEM_RETENTION_MODE_PRESERVE_IMAGE dg_configMEM_RETENTION_MODE
# else
        #define dg_configMEM_RETENTION_MODE_PRESERVE_IMAGE (0)
# endif
#endif

/**
 * \brief Memory Shuffling mode. 
 *
 * See SYS_CTRL_REG:REMAP_RAMS field.
 */
#ifndef dg_configSHUFFLING_MODE
#define dg_configSHUFFLING_MODE         (0)
#endif

/**
 * \brief Watchdog Service 
 *
 * - 1: enabled
 * - 0: disabled
 */
#ifndef dg_configUSE_WDOG
#define dg_configUSE_WDOG               (0)
#endif

/**
 * \brief Brown-out Detection
 *
 * - 1: used
 * - 0: not used
 */
#ifndef dg_configUSE_BOD
#define dg_configUSE_BOD                (1)
#endif

/**
 * \brief Reset value for Watchdog. 
 *
 * See WATCHDOG_REG:WDOG_VAL field.
 */
#ifndef dg_configWDOG_RESET_VALUE
#define dg_configWDOG_RESET_VALUE       (0xFF)
#endif

/**
 * \brief Maximum watchdog tasks
 *
 * Maximum number of tasks that the Watchdog Service can monitor. It can be larger (up to 32) than
 * needed, at the expense of increased Retention Memory requirement.
 */
#ifndef dg_configWDOG_MAX_TASKS_CNT
#define dg_configWDOG_MAX_TASKS_CNT     (4)
#endif

/**
 * \brief Watchdog notify interval
 *
 * Interval (in miliseconds) for common timer which can be used to trigger tasks in order to notify
 * watchdog. Can be set to 0 in order to disable timer code entirely.
 */
#ifndef dg_configWDOG_NOTIFY_TRIGGER_TMO
#define dg_configWDOG_NOTIFY_TRIGGER_TMO        (0)
#endif

/**
 * \brief Abort a clock modification if it will cause an error to the SysTick counter
 *
 * - 1: on
 * - 0: off
 */
#ifndef dg_configABORT_IF_SYSTICK_CLK_ERR
#define dg_configABORT_IF_SYSTICK_CLK_ERR       (0)
#endif

/**
 * \brief Maximum adapters count
 *
 * Should be equal to the number of Adapters used by the Application. It can be larger (up to 254)
 * than needed, at the expense of increased Retention Memory requirements. It cannot be 0.
 */
#ifndef dg_configPM_MAX_ADAPTERS_CNT
#define dg_configPM_MAX_ADAPTERS_CNT    (16)
#endif

/**
 * \brief Maximum sleep defer time
 *
 * The maximum time sleep can be deferred via a call to pm_defer_sleep_for(). It is in clock cycles
 * in the case of the XTAL32K and in msec in the case of RCX.
 */
#ifndef dg_configPM_MAX_ADAPTER_DEFER_TIME
#if (dg_configUSE_LP_CLK != LP_CLK_RCX)
        #define dg_configPM_MAX_ADAPTER_DEFER_TIME      (128)
#else
        #define dg_configPM_MAX_ADAPTER_DEFER_TIME      cm_rcx_us_2_lpcycles(4000)
#endif
#endif

/**
 * \brief Minimum sleep time
 *
 *  No power savings if we enter sleep when the sleep time is less than N LP cycles
 */
#ifndef dg_configMIN_SLEEP_TIME
#if (dg_configUSE_LP_CLK != LP_CLK_RCX)
        #define dg_configMIN_SLEEP_TIME (33*3)  // 3 msec
#else
        #define dg_configMIN_SLEEP_TIME cm_rcx_us_2_lpcycles_low_acc(3000)  // 3 msec
#endif
#endif

/**
 * \brief Recharge period
 */
#ifndef dg_configSET_RECHARGE_PERIOD
#if (dg_configUSE_LP_CLK != LP_CLK_RCX)
#define dg_configSET_RECHARGE_PERIOD            3000    /* number of Low Power clock cycles for
                                                           sampling and / or refreshing. */
#else
#define dg_configSET_RECHARGE_PERIOD            90000   /* number of msec for sampling and / or
                                                           refreshing */
#endif
#endif /* dg_configSET_RECHARGE_PERIOD */

/**
 * \brief When set to 1, the DCDC is used.
 */
#ifndef dg_configUSE_DCDC
#define dg_configUSE_DCDC               (0)
#endif

/**
 * \brief When set to 1, the ADC is used. 
 *
 * The ADC is also used when at least one of the following is true:
 *  - the USB Charger is used
 *  - the RF recalibration is enabled
 * Both these cases are covered later on in this file.
 */
#ifndef dg_configUSE_ADC
#define dg_configUSE_ADC                (0)
#endif

#ifndef dg_configBATTERY_TYPE
/**
 * \brief Battery type
 */
#define dg_configBATTERY_TYPE           (BATTERY_TYPE_NO_BATTERY)
/**
 * \brief Battery charge voltage
 */
#define dg_configBATTERY_CHARGE_VOLTAGE (0)
/**
 * \brief Battery charge current
 */
#define dg_configBATTERY_CHARGE_CURRENT (0)
/**
 * \brief Battery charge NTC
 */
#define dg_configBATTERY_CHARGE_NTC     (0)
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
 */
#define dg_configBATTERY_PRECHARGE_CURRENT      (0)
#endif

/**
 * \brief When set to 1, the USB interface is used.
 */
#ifndef dg_configUSE_USB
#define dg_configUSE_USB                (0)
#endif

/**
 * \brief When set to 1, the USB Charger is used to charge the battery.
 */
#ifndef dg_configUSE_USB_CHARGER
#define dg_configUSE_USB_CHARGER        (0)
#else
# if (dg_configUSE_USB_CHARGER == 1)
# undef dg_configUSE_ADC
# define dg_configUSE_ADC               (1)
# undef dg_configUSE_USB
# define dg_configUSE_USB               (1)
# undef dg_configGPADC_ADAPTER
# define dg_configGPADC_ADAPTER         (1)
# undef dg_configBATTERY_ADAPTER
# define dg_configBATTERY_ADAPTER       (1)
# endif
#endif

/**
 * \brief When set to 1, the USB Charger will try to enumerate, if possible.
 */
#ifndef dg_configUSE_USB_ENUMERATION
#define dg_configUSE_USB_ENUMERATION    (0)
#else
# if (dg_configUSE_USB_ENUMERATION == 1)
# undef dg_configUSE_USB
# define dg_configUSE_USB               (1)
# endif
#endif

/**
 * \brief When set to 1, the USB Charger will start charging with less than 100mA until enumerated.
 */
#ifndef dg_configALLOW_CHARGING_NOT_ENUM
#define dg_configALLOW_CHARGING_NOT_ENUM        (0)
#endif

/**
 * \brief When set to 1, the USB charger will stop charging from an SDP port (if
 *        #dg_configALLOW_CHARGING_NOT_ENUM is set to 1) after 45 minutes, if not enumerated. 
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
 * y[ADC units] = 820.32 * Vbat[Volts] - 5.343
 */
#ifndef dg_configPRECHARGING_THRESHOLD
#define dg_configPRECHARGING_THRESHOLD  (0)
#endif

/**
 * \brief Charginf threshold
 *
 * When Vbat is at or above this threshold (in ADC measurement units), pre-charging stops and
 * charging starts.
 *
 * The value must be calculated using this equation:
 *
 * y[ADC units] = 820.32 * Vbat[Volts] - 5.343
 */
#ifndef dg_configCHARGING_THRESHOLD
#define dg_configCHARGING_THRESHOLD     (0)
#endif

/**
 * \brief Pre-charging timeout
 *
 * If after this period, the Vbat is not higher than 3.0V then pre-charging stops (N x 10msec).
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
 */
#ifndef dg_configCHARGING_TIMEOUT
#define dg_configCHARGING_TIMEOUT       (0)
#endif

/**
 * \brief Charging CC timeout
 *
 * If after this period, the charger is still in CC mode then charging stops (N x 10msec).
 */
#ifndef dg_configCHARGING_CC_TIMEOUT
#define dg_configCHARGING_CC_TIMEOUT    (180 * 60 * 100)
#endif

/**
 * \brief Charging CV timeout
 *
 * If after this period, the charger is still in CC mode then charging stops (N x 10msec).
 */
#ifndef dg_configCHARGING_CV_TIMEOUT
#define dg_configCHARGING_CV_TIMEOUT    (360 * 60 * 100)
#endif

/**
 * \brief Charginf polling interval
 *
 * While being attached to a USB cable and the battery has been charged, this is the interval
 * (N x 10msec) that the Vbat is polled to decide whether a new charge cycle will be started.
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
 * y[ADC units] = 820.32 * Vbat[Volts] - 5.343
 */
#ifndef dg_configBATTERY_LOW_LEVEL
#define dg_configBATTERY_LOW_LEVEL              (2291)  // 2.8V
#endif

/**
 * \brief Low VBAT handling
 *
 * Choose whether the Low Power clock will be available when the system enters hibernation mode due
 * to low battery. The options are:
 * -  0 : Low Power clock is disabled - the system reboots only via an interrupt from the WKUP Ctrl
 * -  1 : Low Power clock is enabled - the system wakes up from i.e. the VBUS interrupt.
 * The default setting is: 1. 
 *
 * \warning Do not use 0 for AD chips.
 */
#ifndef dg_configLOW_VBAT_HANDLING
#define dg_configLOW_VBAT_HANDLING      (1)
#elif (dg_configLOW_VBAT_HANDLING == 0)
#if ( (dg_configBLACK_ORCA_IC_REV == BLACK_ORCA_IC_REV_A) \
                                && (dg_configBLACK_ORCA_IC_STEP == BLACK_ORCA_IC_STEP_D) )
#error "dg_configLOW_VBAT_HANDLING cannot be 0 when AD is used"
#endif
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
 * y[ADC units] = 820.32 * Vbat[Volts] - 5.343
 */
#if (dg_configBATTERY_TYPE == BATTERY_TYPE_CUSTOM)
#ifndef dg_configBATTERY_TYPE_CUSTOM_ADC_VOLTAGE
#define dg_configBATTERY_TYPE_CUSTOM_ADC_VOLTAGE        (1553)
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
 * y[ADC units] = 820.32 * Vbat[Volts] - 5.343
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
 * y[ADC units] = 820.32 * Vbat[Volts] - 5.343
 */
#ifndef dg_configBATTERY_REPLENISH_GAP
#define dg_configBATTERY_REPLENISH_GAP  (82*2)
#endif

/**
 * \brief The Rsense of the SOC in multiples of 0.1Ohm. The default value is (1 x 0.1Ohm).
 */
#ifndef dg_configSOC_RSENSE
#define dg_configSOC_RSENSE             (1)     // N x 0.1Ohm
#endif

/**
 * \brief When set to 1, the ProDK is used (controls specific settings for this board).
 */
#ifndef dg_configUSE_ProDK
#define dg_configUSE_ProDK              (0)
#endif

/**
 * \brief When set to 1, the 1V8 for the QSPI FLASH is powered, when active.
 */
#ifndef dg_configPOWER_FLASH
#define dg_configPOWER_FLASH            (0)
#endif

/**
 * \brief When set to 1, the QSPI FLASH is put to power-down state during sleep (applies to W25Q80EW).
 */
#ifndef dg_configFLASH_POWER_DOWN
#define dg_configFLASH_POWER_DOWN       (0)
#endif

/**
 * \brief When set to 1, the QSPI FLASH is turned off during sleep (applies to W25Q80EW).
 */
#ifndef dg_configFLASH_POWER_OFF
#define dg_configFLASH_POWER_OFF        (0)
#endif


/**
 * \brief uCode for handling the QSPI FLASH activation from power off.
 */
#ifndef dg_ucodeFLASH_POWER_OFF
        /*
         * Should work with all Winbond flashes -- verified with W25Q80EW.
         *
         * Delay 10usec
         * 0x01   // CMD_NBYTES = 0, CMD_TX_MD = 0 (Single), CMD_VALID = 1
         * 0xA0   // CMD_WT_CNT_LS = 160 --> 10000 / 62.5 = 160 = 10usec
         * 0x00   // CMD_WT_CNT_MS = 0
         * Exit from Fast Read mode
         * 0x09   // CMD_NBYTES = 1, CMD_TX_MD = 0 (Single), CMD_VALID = 1
         * 0x00   // CMD_WT_CNT_LS = 0
         * 0x00   // CMD_WT_CNT_MS = 0
         * 0xFF   // Enable Reset
         * (up to 16 words)
         */
#define dg_ucodeFLASH_POWER_OFF                                         \
        const uint32_t ucode_wakeup[] = {                               \
                0x0900A001,                                             \
                0x00FF0000,                                             \
        }
#endif

/**
 * \brief uCode for handling the QSPI FLASH release from power-down.
 */
#ifndef dg_ucodeFLASH_POWER_DOWN
        /*
         * Should work with all Winbond flashes -- verified with W25Q80EW.
         *
         * 0x09   // CMD_NBYTES = 1, CMD_TX_MD = 0 (Single), CMD_VALID = 1
         * 0x30   // CMD_WT_CNT_LS = 48 --> 3000 / 62.5 = 48 = 3usec
         * 0x00   // CMD_WT_CNT_MS = 0
         * 0xAB   // Release Power Down
         * (up to 16 words)
         */
#define dg_ucodeFLASH_POWER_DOWN                                        \
        const uint32_t ucode_wakeup[] = {                               \
                0xAB003009,                                             \
        }

/**
 * \brief C code for handling the QSPI FLASH release from power-down.
 */
#define dg_configFLASH_POWER_DOWN_C_CODE                                \
{                                                                       \
        for (volatile int delay = 0; delay < 12; delay++);              \
                                                                        \
        hw_qspi_set_automode(false);                                    \
        hw_qspi_set_bus_mode(HW_QSPI_BUS_MODE_SINGLE);                  \
                                                                        \
        hw_qspi_cs_enable();                                            \
        hw_qspi_write8(0xAB);                                           \
        hw_qspi_cs_disable();                                           \
                                                                        \
        hw_qspi_set_automode(true);                                     \
                                                                        \
        for (volatile int delay = 0; delay < 12; delay++);              \
}
#endif

/**
 * \brief uCode for handling the QSPI FLASH exit from the "Continuous Read Mode".
 */
#ifndef dg_ucodeFLASH_ALWAYS_ACTIVE
        /*
         * Should work with all Winbond flashes -- verified with W25Q80EW.
         *
         * 0x25   // CMD_NBYTES = 4, CMD_TX_MD = 2 (Quad), CMD_VALID = 1
         * 0x00   // CMD_WT_CNT_LS = 0
         * 0x00   // CMD_WT_CNT_MS = 0
         * 0x55   // Clocks 0-1 (A23-16)
         * 0x55   // Clocks 2-3 (A15-8)
         * 0x55   // Clocks 4-5 (A7-0)
         * 0x55   // Clocks 6-7 (M7-0) : M5-4 != '10' ==> Disable "Continuous Read Mode"
         * (up to 16 words)
         */
#define dg_ucodeFLASH_ALWAYS_ACTIVE                                     \
        const uint32_t ucode_wakeup[] = {                               \
                0x55000025,                                             \
                0x00555555,                                             \
        }
#endif

/**
 * \brief When set to 1, the chip provides power to external peripherals.
 */
#ifndef dg_configPOWER_EXT_1V8_PERIPHERALS
#define dg_configPOWER_EXT_1V8_PERIPHERALS      (0)
#endif

/**
 * \brief Enable debugger
 */
#ifndef dg_configENABLE_DEBUGGER
#define dg_configENABLE_DEBUGGER		(1)
#endif

/**
 * \brief Use SW cursor
 */
#ifndef dg_configUSE_SW_CURSOR
#       define dg_configUSE_SW_CURSOR           (0)
#       define SW_CURSOR_PORT                   (0)
#       define SW_CURSOR_PIN                    (0)
#else
#       if dg_configBLACK_ORCA_MB_REV == BLACK_ORCA_MB_REV_B
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
 * \brief Set the size (in bytes) of the QSPI flash cacheable area.
 *
 * All reads from offset 0 up to (not including) offset dg_configCACHEABLE_QSPI_AREA_LEN
 * will be cached. In addition, any writes to this area will trigger cache flushing, to
 * avoid any cache incoherence.
 *
 * The size must be 64KB-aligned, due to the granularity of CACHE_CTRL2_REG[CACHE_LEN].
 *
 * Special values:
 *   0: turn off cache
 *  -1: don't configure cacheable area size (i.e. leave as set by booter)
 */
#ifndef dg_configCACHEABLE_QSPI_AREA_LEN
#define dg_configCACHEABLE_QSPI_AREA_LEN        (-1)
#endif
#if (dg_configCACHEABLE_QSPI_AREA_LEN != -1) && (dg_configCACHEABLE_QSPI_AREA_LEN & 0xFFFF)
#error "dg_configCACHEABLE_QSPI_AREA_LEN must be 64KB-aligned!"
#endif

/* ---------------------------------------------------------------------------------------------- */

/**
 * \} 
 */

/* ------------------------------- Arbiter configuration settings ------------------------------- */

/* Custom arbiter configuration support. */
#ifndef dg_configCOEX_ENABLE_CONFIG
#define dg_configCOEX_ENABLE_CONFIG             (0)
#endif

/* Arbiter statistics */
#ifndef dg_configCOEX_ENABLE_STATS
#define dg_configCOEX_ENABLE_STATS              (0)
#endif

/* -------------------------------- Peripherals (hw_*) selection -------------------------------- */

#ifndef dg_configUSE_HW_AES_HASH
#  define dg_configUSE_HW_AES_HASH              (0)
#endif

#ifndef dg_configUSE_HW_COEX
#  define dg_configUSE_HW_COEX                  (0)
#endif

#ifndef dg_configUSE_HW_CPM
#  define dg_configUSE_HW_CPM                   (1)
#endif

#ifndef dg_configUSE_HW_DMA
#  define dg_configUSE_HW_DMA                   (1)
#endif

#ifndef dg_configUSE_HW_GPADC
#  define dg_configUSE_HW_GPADC                 (1)
#endif

#ifndef dg_configUSE_HW_GPIO
#  define dg_configUSE_HW_GPIO                  (1)
#endif

#ifndef dg_configUSE_HW_I2C
#  define dg_configUSE_HW_I2C                   (0)
#endif

#ifndef dg_configUSE_HW_IRGEN
#  define dg_configUSE_HW_IRGEN                 (0)
#endif

#ifndef dg_configUSE_HW_KEYBOARD_SCANNER
#  define dg_configUSE_HW_KEYBOARD_SCANNER      (0)
#endif

#ifndef dg_configUSE_HW_OTPC
#  define dg_configUSE_HW_OTPC                  (1)
#endif

#ifndef dg_configUSE_HW_QSPI
#  define dg_configUSE_HW_QSPI                  (1)
#endif

#ifndef dg_configUSE_HW_QUAD
#  define dg_configUSE_HW_QUAD                  (0)
#endif

#ifndef dg_configUSE_HW_RF
#  define dg_configUSE_HW_RF                    (1)
#endif

#ifndef dg_configUSE_HW_SOC
#  define dg_configUSE_HW_SOC                   (0)
#endif

#ifndef dg_configUSE_HW_SPI
#define dg_configUSE_HW_SPI                   (0)
#endif

#ifndef dg_configUSE_HW_TEMPSENS
#  define dg_configUSE_HW_TEMPSENS              (1)
#endif

#ifndef dg_configUSE_HW_TIMER0
#  define dg_configUSE_HW_TIMER0                (0)
#endif

#ifndef dg_configUSE_HW_TIMER1
#  define dg_configUSE_HW_TIMER1                (1)
#endif

#ifndef dg_configUSE_HW_TIMER2
#  define dg_configUSE_HW_TIMER2                (0)
#endif

#ifndef dg_configUSE_HW_TRNG
#  define dg_configUSE_HW_TRNG                  (0)
#endif

#ifndef dg_configUSE_HW_UART
#  define dg_configUSE_HW_UART                  (1)
#endif

#ifndef dg_configUART_SOFTWARE_FIFO
#  define dg_configUART_SOFTWARE_FIFO           (0)
#endif

#ifndef dg_configUSE_HW_USB_CHARGER
#  define dg_configUSE_HW_USB_CHARGER           (1)
#endif

#ifndef dg_configUSE_HW_USB_WKUP
#  define dg_configUSE_HW_USB_WKUP              (1)
#endif

#ifndef dg_configUSE_HW_USB
#  define dg_configUSE_HW_USB                   (0)
#endif

/* ---------------------------------------------------------------------------------------------- */


/* -------------------------------- Adapters (ad_*) selection -------------------------------- */

/*
 * Set to 1/0 to, by default, include/exclude the specific adapter when compiling the SDK.
 * It can be overridden in the application configuration file.
 */

#ifndef dg_configFLASH_ADAPTER
#  define dg_configFLASH_ADAPTER                (1)
#endif

#ifndef dg_configIMAGE_FLASH_OFFSET
#  define dg_configIMAGE_FLASH_OFFSET           (0)
#endif

#ifndef dg_configFLASH_MAX_WRITE_SIZE
#  define dg_configFLASH_MAX_WRITE_SIZE         (256)
#endif


#ifndef dg_configI2C_ADAPTER
#  define dg_configI2C_ADAPTER                  (0)
#endif

#ifndef dg_configNVMS_ADAPTER
#  define dg_configNVMS_ADAPTER                 (1)
#endif

#ifndef dg_configNVMS_FLASH_CACHE
#  define dg_configNVMS_FLASH_CACHE             (0)
#endif

#ifndef dg_configNVMS_VES
#  define dg_configNVMS_VES                     (1)
#endif

#ifndef dg_configRF_ADAPTER
#  define dg_configRF_ADAPTER                   (1)
#endif

#ifndef dg_configSPI_ADAPTER
#  define dg_configSPI_ADAPTER                  (0)
#endif

#ifndef dg_configUART_ADAPTER
#  define dg_configUART_ADAPTER                 (0)
#endif

#ifndef dg_configUART_BLE_ADAPTER
#  define dg_configUART_BLE_ADAPTER             (0)
#endif

#ifndef dg_configGPADC_ADAPTER
#  define dg_configGPADC_ADAPTER                (0)
#endif

#ifndef dg_configTEMPSENS_ADAPTER
#  define dg_configTEMPSENS_ADAPTER             (0)
#endif

#ifndef dg_configBATTERY_ADAPTER
#  define dg_configBATTERY_ADAPTER              (0)
#endif

#ifndef dg_configNVPARAM_ADAPTER
#   define dg_configNVPARAM_ADAPTER             (0)
#endif
/* ---------------------------------------------------------------------------------------------- */

/* ----------------------------- Segger System View configuration ------------------------------- */

/*
 * Set to 1/0 to enable/disable Segger System View.  When enabled the application should also call
 * SEGGER_SYSVIEW_Conf() to enable system monitoring.
 * */
#ifndef dg_configSYSTEMVIEW
#  define dg_configSYSTEMVIEW                   (0)
#endif

/*
 * This is the stack size overhead when System View API is used.
 * All thread stack sizes plus the the stack of IRQ handlers will be increased by that amount
 * to avoid stack overflow when System View is monitoring the system.
 * */
#ifndef dg_configSYSTEMVIEW_STACK_OVERHEAD
#  define dg_configSYSTEMVIEW_STACK_OVERHEAD    (512)
#endif


/* ---------------------------------------------------------------------------------------------- */


/* ------------------------------- RF driver configuration settings -------------------------------- */

/* Set to 1 to enable the recalibration procedure */
#ifndef dg_configRF_ENABLE_RECALIBRATION
#define dg_configRF_ENABLE_RECALIBRATION        (1)
#endif

/* If RF recalibration is enabled, we need to enable GPADC as well */
#if dg_configRF_ENABLE_RECALIBRATION
#undef dg_configUSE_ADC
#define dg_configUSE_ADC                        (1)
#undef dg_configGPADC_ADAPTER
#define dg_configGPADC_ADAPTER                  (1)
#undef dg_configTEMPSENS_ADAPTER
#define dg_configTEMPSENS_ADAPTER               (1)
#endif

/* If RF is enabled, we need to enable GPADC adapter as well */
#if  dg_configRF_ADAPTER
#undef dg_configGPADC_ADAPTER
#define dg_configGPADC_ADAPTER                  (1)
#endif

/* Minimum time before performing an RF recalibration, in FreeRTOS scheduler ticks */
#ifndef dg_configRF_MIN_RECALIBRATION_TIMEOUT
# if (dg_configUSE_LP_CLK == LP_CLK_32000)
# define dg_configRF_MIN_RECALIBRATION_TIMEOUT 1000 // ~2 secs
# elif (dg_configUSE_LP_CLK == LP_CLK_32768)
# define dg_configRF_MIN_RECALIBRATION_TIMEOUT 1024 // ~2 secs
# else
# define dg_configRF_MIN_RECALIBRATION_TIMEOUT 1000
# endif
#endif

/* Maximum time before performing an RF recalibration, in FreeRTOS scheduler ticks
 * If this time has elapsed, and RF is about to be powered off, recalibration will
 * be done unconditionally. Set to 0 to disable this functionality */
#ifndef dg_configRF_MAX_RECALIBRATION_TIMEOUT
# define dg_configRF_MAX_RECALIBRATION_TIMEOUT 0 // Disabled
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
# define dg_configRF_RECALIBRATION_TIMER_TIMEOUT 0 // Disabled
#endif


/* Minimum temp difference before performing an RF recalibration, in oC*/
#ifndef dg_configRF_MIN_RECALIBRATION_TEMP_DIFF
# ifdef CONFIG_USE_FTDF
#  define dg_configRF_MIN_RECALIBRATION_TEMP_DIFF (5)
# else
#  define dg_configRF_MIN_RECALIBRATION_TEMP_DIFF (10)
# endif
#endif

/* Duration of recalibration procedure, in lp clk cycles */
#ifndef dg_configRF_RECALIBRATION_DURATION
# if defined(CONFIG_USE_FTDF) && defined(CONFIG_USE_BLE)
#  if (dg_configUSE_LP_CLK != LP_CLK_RCX)
#   define dg_configRF_RECALIBRATION_DURATION 230
#  else
#   define dg_configRF_RECALIBRATION_DURATION cm_rcx_us_2_lpcycles((uint32_t)(230 * 30.5))
#  endif
# else
#  if (dg_configUSE_LP_CLK != LP_CLK_RCX)
#   define dg_configRF_RECALIBRATION_DURATION 131
#  else
#   define dg_configRF_RECALIBRATION_DURATION cm_rcx_us_2_lpcycles((uint32_t)(131 * 30.5))
#  endif
# endif
#endif

#ifndef dg_configRF_IFF_CALIBRATION_TIMEOUT
#  if (dg_configUSE_LP_CLK != LP_CLK_RCX)
#   define dg_configRF_IFF_CALIBRATION_TIMEOUT 40
#  else
#   define dg_configRF_IFF_CALIBRATION_TIMEOUT cm_rcx_us_2_lpcycles((uint32_t)(40 * 30.5))
#  endif
#endif


/* ---------------------------------------------------------------------------------------------- */

/* -----------------------------------OS related configuration ---------------------------------- */
#ifndef dg_configTRACK_OS_HEAP
#   define dg_configTRACK_OS_HEAP               (0)
#endif
/* ---------------------------------------------------------------------------------------------- */

/* ----------------------------------- RF FEM CONFIGURATION ------------------------------------- */

#include "bsp_fem.h"

/* ---------------------------------------------------------------------------------------------- */


/* ----------------------------------- DEBUG CONFIGURATION -------------------------------------- */

#include "bsp_debug.h"

/* ---------------------------------------------------------------------------------------------- */


#endif /* BSP_DEFAULTS_H_ */

/**
\}
\}
\}
*/
