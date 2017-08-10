/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file custom_config_qspi.h
 * Custom configuration file for FreeRTOS applications executing from QSPI.
 */

#ifndef CUSTOM_CONFIG_QSPI_H_
#define CUSTOM_CONFIG_QSPI_H_

#include "bsp_definitions.h"

#define dg_configBLACK_ORCA_IC_REV                     BLACK_ORCA_IC_REV_B
#define dg_configBLACK_ORCA_IC_STEP                    BLACK_ORCA_IC_STEP_A

#undef CONFIG_USE_BLE
#define CONFIG_USE_FTDF

#define __HEAP_SIZE                                    0x0600
#define __STACK_SIZE                                   0x0100

#define CONFIG_RETARGET
#define CONFIG_RETARGET_UART                           HW_UART1

#define dg_configUSE_LP_CLK                            LP_CLK_32768
#define dg_configEXEC_MODE                             MODE_IS_CACHED
#define dg_configCODE_LOCATION                         NON_VOLATILE_IS_FLASH
#define dg_configEXT_CRYSTAL_FREQ                      EXT_CRYSTAL_IS_16M

#define dg_configIMAGE_SETUP                           DEVELOPMENT_MODE
#define dg_configEMULATE_OTP_COPY                      (0)

#define dg_configUSER_CAN_USE_TIMER1                   (0)

#define dg_configMEM_RETENTION_MODE                    (0x1F)
#define dg_configSHUFFLING_MODE                        (0x3)

#define dg_configUSE_WDOG                              (0)

#define dg_configFLASH_CONNECTED_TO                    (FLASH_CONNECTED_TO_1V8)
#define dg_configFLASH_POWER_DOWN                      (0)

#define dg_configPOWER_1V8_ACTIVE                      (1)
#define dg_configPOWER_1V8_SLEEP                       (1)

#define dg_configBATTERY_TYPE                          (BATTERY_TYPE_LIMN2O4)
#define dg_configBATTERY_CHARGE_CURRENT                2       // 30mA
#define dg_configBATTERY_PRECHARGE_CURRENT             20      // 2.1mA
#define dg_configBATTERY_CHARGE_NTC                    1       // disabled

#define dg_configUSE_USB                               0
#define dg_configUSE_USB_CHARGER                       0
#define dg_configALLOW_CHARGING_NOT_ENUM               1

#define dg_configUSE_ProDK                             (1)

#define dg_configUSE_SW_CURSOR                         (1)

#define dg_configCACHEABLE_QSPI_AREA_LEN               NVMS_PARAM_PART_start

#define dg_configFEM_DLG_REF_BOARD                     (1)
#define dg_configPOWER_1V8P                            (1)

#define dg_configUSE_HW_TRNG                           (1)

#define dg_configUSE_HW_TIMER0                         (1)

#define dg_configUSE_HW_AES_HASH                       (1)
/*************************************************************************************************\
 * OS specific config
 */
#define OS_BAREMETAL
#define CPU_DA15000                                    (1)

/*************************************************************************************************\
 * Peripheral specific config
 */
#define FTDF_PHY_API
#define dg_configUSE_HW_RF                             (1)
#define dg_configRF_ENABLE_RECALIBRATION               (0)
#define FTDF_NO_CSL
#define FTDF_NO_TSCH
#define FTDF_LITE

#define dg_configFLASH_ADAPTER                         (0)
#define dg_configNVMS_ADAPTER                          (0)
#define dg_configNVMS_VES                              (0)
#define dg_configUSE_WDOG                              (0)
/* Include bsp default values */
#include "bsp_defaults.h"


/*************************************************************************************************\
 * Open Thread specific config
 */

#define SETTINGS_CONFIG_BASE_ADDRESS                   (0x7B000)

#endif /* CUSTOM_CONFIG_QSPI_H_ */
