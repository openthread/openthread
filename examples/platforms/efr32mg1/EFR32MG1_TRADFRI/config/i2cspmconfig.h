/***************************************************************************//**
 * @file
 * @brief I2CSPM driver configuration file
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef I2CSPM_CONFIG_H
#define I2CSPM_CONFIG_H

/***************************************************************************//**
 * @addtogroup Drivers
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup I2CSPM
 * @{
 ******************************************************************************/

/* I2C SPM driver config. This default override only works if one I2C interface
   is in use. If multiple interfaces are in use, define the peripheral setup
   inside the application in a I2CSPM_Init_TypeDef and then pass the initialization
   struct to I2CSPM_Init(). */
#define I2CSPM_INIT_DEFAULT                                                   \
  { I2C0,                      /* Use I2C instance 0 */                       \
    gpioPortC,                 /* SCL port */                                 \
    10,                        /* SCL pin */                                  \
    gpioPortC,                 /* SDA port */                                 \
    11,                        /* SDA pin */                                  \
    14,                        /* Location of SCL */                          \
    16,                        /* Location of SDA */                          \
    0,                         /* Use currently configured reference clock */ \
    I2C_FREQ_STANDARD_MAX,     /* Set to standard rate  */                    \
    i2cClockHLRStandard,       /* Set to use 4:4 low/high duty cycle */       \
  }

#define I2CSPM_TRANSFER_TIMEOUT 300000

/** @} (end addtogroup I2CSPM) */
/** @} (end addtogroup Drivers) */

#endif /* I2CSPM_CONFIG_H */
