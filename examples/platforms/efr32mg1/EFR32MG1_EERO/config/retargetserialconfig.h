/***************************************************************************//**
 * @file
 * @brief Provide stdio retargeting configuration parameters.
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

#ifndef RETARGETSERIALCONFIG_H
#define RETARGETSERIALCONFIG_H

#include "bsp.h"

/***************************************************************************//**
 *
 * When retargeting serial output the user can choose which peripheral
 * to use as the serial output device. This choice is made by configuring
 * one or more of the following defines: RETARGET_USART0, RETARGET_LEUART0,
 * RETARGET_VCOM.
 *
 * This table shows the supported configurations and the resulting serial
 * output device.
 *
 * +----------------------------------------------------------------------+
 * | Defines                            | Serial Output (Locations)       |
 * |----------------------------------------------------------------------+
 * | None                               | USART0 (Rx #0, Tx #0)           |
 * | RETARGET_USART0                    | USART0 (Rx #0, Tx #0)           |
 * | RETARGET_VCOM                      | VCOM using USART0               |
 * | RETARGET_LEUART0                   | LEUART0 (Rx #0, Tx #0)          |
 * | RETARGET_LEUART0 and RETARGET_VCOM | VCOM using LEUART0              |
 * +----------------------------------------------------------------------+
 *
 * Note that the default configuration is the same as RETARGET_USART0.
 *
 ******************************************************************************/

#if !defined(RETARGET_USART0) \
  && !defined(RETARGET_LEUART0)
#define RETARGET_USART0    /* Use USART0 by default. */
#endif

#if defined(RETARGET_USART0)
  #define RETARGET_IRQ_NAME    USART0_RX_IRQHandler         /* UART IRQ Handler */
  #define RETARGET_CLK         cmuClock_USART0              /* HFPER Clock */
  #define RETARGET_IRQn        USART0_RX_IRQn               /* IRQ number */
  #define RETARGET_UART        USART0                       /* UART instance */
  #define RETARGET_TX          USART_Tx                     /* Set TX to USART_Tx */
  #define RETARGET_RX          USART_Rx                     /* Set RX to USART_Rx */
  #define RETARGET_TX_LOCATION _USART_ROUTELOC0_TXLOC_LOC0  /* Location of of USART TX pin */
  #define RETARGET_RX_LOCATION _USART_ROUTELOC0_RXLOC_LOC0  /* Location of of USART RX pin */
  #define RETARGET_TXPORT      gpioPortA                    /* UART transmission port */
  #define RETARGET_TXPIN       0                            /* UART transmission pin */
  #define RETARGET_RXPORT      gpioPortA                    /* UART reception port */
  #define RETARGET_RXPIN       1                            /* UART reception pin */
  #define RETARGET_USART       1                            /* Includes em_usart.h */
  #define RETARGET_CTS_LOCATION _USART_ROUTELOC1_CTSLOC_LOC30
  #define RETARGET_RTS_LOCATION _USART_ROUTELOC1_RTSLOC_LOC30
  #define RETARGET_CTSPORT      gpioPortA
  #define RETARGET_CTSPIN       2
  #define RETARGET_RTSPORT      gpioPortA
  #define RETARGET_RTSPIN       3

#elif defined(RETARGET_LEUART0)
  #define RETARGET_IRQ_NAME    LEUART0_IRQHandler           /* LEUART IRQ Handler */
  #define RETARGET_CLK         cmuClock_LEUART0             /* HFPER Clock */
  #define RETARGET_IRQn        LEUART0_IRQn                 /* IRQ number */
  #define RETARGET_UART        LEUART0                      /* LEUART instance */
  #define RETARGET_TX          LEUART_Tx                    /* Set TX to LEUART_Tx */
  #define RETARGET_RX          LEUART_Rx                    /* Set RX to LEUART_Rx */
  #define RETARGET_TX_LOCATION _LEUART_ROUTELOC0_TXLOC_LOC0 /* Location of of LEUART TX pin */
  #define RETARGET_RX_LOCATION _LEUART_ROUTELOC0_RXLOC_LOC0 /* Location of of LEUART RX pin */
  #define RETARGET_TXPORT      gpioPortA                    /* LEUART transmission port */
  #define RETARGET_TXPIN       0                            /* LEUART transmission pin */
  #define RETARGET_RXPORT      gpioPortA                    /* LEUART reception port */
  #define RETARGET_RXPIN       1                            /* LEUART reception pin */
  #define RETARGET_LEUART      1                            /* Includes em_leuart.h */

#else
#error "Illegal USART selection."
#endif

#if defined(RETARGET_VCOM)
  #define RETARGET_PERIPHERAL_ENABLE() \
  GPIO_PinModeSet(BSP_BCC_ENABLE_PORT, \
                  BSP_BCC_ENABLE_PIN,  \
                  gpioModePushPull,    \
                  1);
#else
  #define RETARGET_PERIPHERAL_ENABLE()
#endif

#endif
