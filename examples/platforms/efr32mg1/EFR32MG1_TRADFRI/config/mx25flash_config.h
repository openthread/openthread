/***************************************************************************//**
 * @file
 * @brief BRD4150A specific configuration for on-board serial flash.
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

#ifndef MX25CONFIG_H
#define MX25CONFIG_H

#include "em_device.h"
#include "em_gpio.h"

#define MX25_PORT_MOSI         gpioPortD
#define MX25_PIN_MOSI          15
#define MX25_PORT_MISO         gpioPortD
#define MX25_PIN_MISO          14
#define MX25_PORT_SCLK         gpioPortD
#define MX25_PIN_SCLK          13
#define MX25_PORT_CS           gpioPortB
#define MX25_PIN_CS            11

#define MX25_USART             USART1
#define MX25_USART_CLK         cmuClock_USART1

#endif // MX25CONFIG_H
