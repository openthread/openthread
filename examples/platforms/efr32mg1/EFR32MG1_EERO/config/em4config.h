/***************************************************************************//**
 * @file
 * @brief Provide configuration parameters for EM4 wakeup button.
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

#ifndef EM4CONFIG_H
#define EM4CONFIG_H

#include "bspconfig.h"

#define EM4_WU_PB           PB1
#define EM4_WU_PB_EN        (1 << 17) /* GPIO_EM4WU1 = PF7 = pushbutton 1 */
#define EM4_WU_PB_PIN       BSP_GPIO_PB1_PIN
#define EM4_WU_PB_PORT      BSP_GPIO_PB1_PORT
#define EM4_WU_PB_STR       "PB1"

#define EM4_NON_WU_PB          PB0
#define EM4_NON_WU_PB_PIN      BSP_GPIO_PB0_PIN
#define EM4_NON_WU_PB_PORT     BSP_GPIO_PB0_PORT
#define EM4_NON_WU_PB_STR      "PB0"

#endif /* EM4CONFIG_H */
