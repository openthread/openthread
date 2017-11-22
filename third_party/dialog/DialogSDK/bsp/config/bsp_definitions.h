/**
 * \addtogroup BSP
 * \{
 * \addtogroup BSP_CONFIG
 * \{
 * \addtogroup BSP_CONFIG_DEFINITIONS
 *
 * \brief Doxygen documentation is not yet available for this module.
 *        Please check the source code file(s)
 *
 *\{
 */

/**
 ****************************************************************************************
 *
 * @file bsp_definitions.h
 *
 * @brief Board Support Package. System Configuration file definitions.
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
 ****************************************************************************************
 */

#ifndef BSP_DEFINITIONS_H_
#define BSP_DEFINITIONS_H_


/* ------------------------------------ Generic definitions ------------------------------------- */

#define LP_CLK_32000                    0
#define LP_CLK_32768                    1
#define LP_CLK_RCX                      2
#define LP_CLK_ANY                      3

#define LP_CLK_IS_ANALOG                0
#define LP_CLK_IS_DIGITAL               1

#define MODE_IS_MIRRORED                0
#define MODE_IS_CACHED                  1
#define MODE_IS_RAM                     MODE_IS_MIRRORED

#define NON_VOLATILE_IS_OTP             0       // Code is in OTP
#define NON_VOLATILE_IS_FLASH           1       // Code is in QSPI Flash
#define NON_VOLATILE_IS_NONE            2       // Debug mode! Code is in RAM!

#define EXT_CRYSTAL_IS_16M              0
#define EXT_CRYSTAL_IS_32M              1

#define DEVELOPMENT_MODE                0       // Code is built for debugging
#define PRODUCTION_MODE                 1       // Code is built for production

#define FLASH_IS_NOT_CONNECTED          0
#define FLASH_CONNECTED_TO_1V8          1
#define FLASH_CONNECTED_TO_1V8P         2

#define BATTERY_TYPE_2xNIMH             0
#define BATTERY_TYPE_3xNIMH             1
#define BATTERY_TYPE_LICOO2             2       // 2.5V discharge voltage, 4.20V charge voltage
#define BATTERY_TYPE_LIMN2O4            3       // 2.5V discharge voltage, 4.20V charge voltage
#define BATTERY_TYPE_NMC                4       // 2.5V discharge voltage, 4.20V charge voltage
#define BATTERY_TYPE_LIFEPO4            5       // 2.5V discharge voltage, 3.65V charge voltage
#define BATTERY_TYPE_LINICOAIO2         6       // 3.0V discharge voltage, 4.20V charge voltage
#define BATTERY_TYPE_CUSTOM             7
#define BATTERY_TYPE_NO_RECHARGE        8
#define BATTERY_TYPE_NO_BATTERY         9

#define BATTERY_TYPE_2xNIMH_ADC_VOLTAGE         (2785)
#define BATTERY_TYPE_3xNIMH_ADC_VOLTAGE         (4013)
#define BATTERY_TYPE_LICOO2_ADC_VOLTAGE         (3440)
#define BATTERY_TYPE_LIMN2O4_ADC_VOLTAGE        (3440)
#define BATTERY_TYPE_NMC_ADC_VOLTAGE            (3440)
#define BATTERY_TYPE_LIFEPO4_ADC_VOLTAGE        (2989)
#define BATTERY_TYPE_LINICOAIO2_ADC_VOLTAGE     (3440)

/*
 * The supported chip revisions.
 */
#define BLACK_ORCA_IC_REV_AUTO          (-1)
#define BLACK_ORCA_IC_REV_A             0
#define BLACK_ORCA_IC_REV_B             1

/*
 * The supported chip steppings.
 */
#define BLACK_ORCA_IC_STEP_AUTO         (-1)
#define BLACK_ORCA_IC_STEP_A            0
#define BLACK_ORCA_IC_STEP_B            1
#define BLACK_ORCA_IC_STEP_C            2
#define BLACK_ORCA_IC_STEP_D            3
#define BLACK_ORCA_IC_STEP_E            7

/*
 * Legacy DK motherboards, which are not supported by the SDK.
 * The definitions exist just so that we don't break compilation of old projects.
 */
#define BLACK_ORCA_MB_REV_A             0
#define BLACK_ORCA_MB_REV_B             1
/*
 * The supported DK motherboards.
 */
#define BLACK_ORCA_MB_REV_D             2

/*
 * The cache associativity options.
 */
#define CACHE_ASSOC_AS_IS               (-1)    /// leave as set by the ROM booter
#define CACHE_ASSOC_DIRECT_MAP          0       /// direct-mapped
#define CACHE_ASSOC_2_WAY               1       /// 2-way set associative
#define CACHE_ASSOC_4_WAY               2       /// 4-way set associative

/*
 * The cache line size options.
 */
#define CACHE_LINESZ_AS_IS              (-1)    /// leave as set by the ROM booter
#define CACHE_LINESZ_8_BYTES            0       /// 8 bytes
#define CACHE_LINESZ_16_BYTES           1       /// 16 bytes
#define CACHE_LINESZ_32_BYTES           2       /// 32 bytes

/*
 * The supported RF Front-End Modules
 */
#define FEM_NOFEM                       0
#define FEM_SKY66112_11                 1

/*
 * The BLE event notification user hook types
 */
#define BLE_EVENT_NOTIF_USER_ISR        0       /// User-defined hooks directly from ISR context
#define BLE_EVENT_NOTIF_USER_TASK       1       /// Notification of the user task, using task notifications.

#endif /* BSP_DEFINITIONS_H_ */

/**
\}
\}
\}
*/
