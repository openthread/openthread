/***************************************************************************//**
 * @file
 * @brief Provide SWO/ETM TRACE configuration parameters.
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

#ifndef TRACECONFIG_H
#define TRACECONFIG_H

#define BSP_TRACE_SWO_LOCATION GPIO_ROUTELOC0_SWVLOC_LOC0

/* Enable output on pin - GPIO Port F, Pin 2. */
#define TRACE_ENABLE_PINS()                        \
  GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK); \
  GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL

/*  No ETM trace support on this WSTK. */

#endif
