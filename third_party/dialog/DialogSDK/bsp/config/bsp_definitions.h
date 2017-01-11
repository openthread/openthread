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
 ****************************************************************************************
 */

#ifndef BSP_DEFINITIONS_H_
#define BSP_DEFINITIONS_H_


/* ------------------------------------ Generic definitions ------------------------------------- */

#define POWER_CONFIGURATION_1           0
#define POWER_CONFIGURATION_2           1
#define POWER_CONFIGURATION_3           2

#define LP_CLK_32000                    0
#define LP_CLK_32768                    1
#define LP_CLK_RCX                      2

#define MODE_IS_MIRRORED                0
#define MODE_IS_CACHED                  1
#define MODE_IS_RAM                     MODE_IS_CACHED

#define NON_VOLATILE_IS_OTP             0       // Code is in OTP
#define NON_VOLATILE_IS_FLASH           1       // Code is in QSPI Flash
#define NON_VOLATILE_IS_NONE            2       // Debug mode! Code is in RAM!

#define EXT_CRYSTAL_IS_16M              0
#define EXT_CRYSTAL_IS_32M              1

#define DEVELOPMENT_MODE                0       // Code is built for debugging
#define PRODUCTION_MODE                 1       // Code is built for production

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

#define BATTERY_TYPE_2xNIMH_ADC_VOLTAGE         (2784)
#define BATTERY_TYPE_3xNIMH_ADC_VOLTAGE         (4013)
#define BATTERY_TYPE_LICOO2_ADC_VOLTAGE         (3439)
#define BATTERY_TYPE_LIMN2O4_ADC_VOLTAGE        (3439)
#define BATTERY_TYPE_NMC_ADC_VOLTAGE            (3439)
#define BATTERY_TYPE_LIFEPO4_ADC_VOLTAGE        (2947)
#define BATTERY_TYPE_LINICOAIO2_ADC_VOLTAGE     (3439)


/*
 * The supported chip revisions.
 */
#define BLACK_ORCA_IC_REV_A             0

/*
 * The supported chip steppings.
 */
#define BLACK_ORCA_IC_STEP_A            0
#define BLACK_ORCA_IC_STEP_B            1
#define BLACK_ORCA_IC_STEP_C            2
#define BLACK_ORCA_IC_STEP_D            3

/*
 * The supported DK motherboards.
 */
#define BLACK_ORCA_MB_REV_A             0
#define BLACK_ORCA_MB_REV_B             1

/*
 * The supported RF Front-End Modules
 */
#define FEM_NOFEM                       0
#define FEM_SKY66112_11                 1

#endif /* BSP_DEFINITIONS_H_ */

/**
\}
\}
\}
*/
