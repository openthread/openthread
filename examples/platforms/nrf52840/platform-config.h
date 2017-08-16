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
 * @file
 *   This file includes the platform-specific configuration.
 *
 */

#ifndef PLATFORM_CONFIG_H_
#define PLATFORM_CONFIG_H_

#include "device/nrf.h"
#include "hal/nrf_uart.h"
#include "hal/nrf_peripherals.h"
#include "hal/nrf_radio.h"

#include <openthread/config.h>

/*******************************************************************************
 * @section UART Driver Configuration.
 ******************************************************************************/

/**
 * @def UART_INSTANCE
 *
 * UART Instance.
 *
 */
#define UART_INSTANCE  NRF_UART0

/**
 * @def UART_PARITY
 *
 * UART Parity configuration.
 *
 * @brief Possible values:
 *         \ref NRF_UART_PARITY_EXCLUDED - Parity bit is not present.
 *         \ref NRF_UART_PARITY_INCLUDED - Parity bit is present.
 *
 */
#define UART_PARITY  NRF_UART_PARITY_EXCLUDED

/**
 * @def UART_HWFC
 *
 * UART Hardware Flow Control.
 *
 * @brief Possible values:
 *         \ref NRF_UART_HWFC_ENABLED - HW Flow control enabled.
 *         \ref NRF_UART_HWFC_DISABLED - HW Flow control disabled.
 *
 */
#define UART_HWFC  NRF_UART_HWFC_ENABLED

/**
 * @def UART_BAUDRATE
 *
 * UART Baudrate.
 *
 * @brief Possible values:
 *         \ref NRF_UART_BAUDRATE_1200 - 1200 baud.
 *         \ref NRF_UART_BAUDRATE_2400 - 2400 baud.
 *         \ref NRF_UART_BAUDRATE_4800 - 4800 baud.
 *         \ref NRF_UART_BAUDRATE_9600 - 9600 baud.
 *         \ref NRF_UART_BAUDRATE_14400 - 14400 baud.
 *         \ref NRF_UART_BAUDRATE_19200 - 19200 baud.
 *         \ref NRF_UART_BAUDRATE_28800 - 28800 baud.
 *         \ref NRF_UART_BAUDRATE_38400 - 38400 baud.
 *         \ref NRF_UART_BAUDRATE_57600 - 57600 baud.
 *         \ref NRF_UART_BAUDRATE_76800 - 76800 baud.
 *         \ref NRF_UART_BAUDRATE_115200 - 115200 baud.
 *         \ref NRF_UART_BAUDRATE_230400 - 230400 baud.
 *         \ref NRF_UART_BAUDRATE_250000 - 250000 baud.
 *         \ref NRF_UART_BAUDRATE_460800 - 460800 baud.
 *         \ref NRF_UART_BAUDRATE_921600 - 921600 baud.
 *         \ref NRF_UART_BAUDRATE_1000000 - 1000000 baud.
 *
 */
#define UART_BAUDRATE  NRF_UART_BAUDRATE_115200

/**
 *  @def UART_IRQN
 *
 * UART Interrupt number.
 *
 */
#define UART_IRQN  UARTE0_UART0_IRQn

/**
 * @def UART_IRQ_PRIORITY
 *
 * UART Interrupt priority.
 *
 */
#define UART_IRQ_PRIORITY  6

/**
 * @def UART_RX_BUFFER_SIZE
 *
 * UART Receive buffer size.
 *
 */
#define UART_RX_BUFFER_SIZE  256

/**
 * @def UART_PIN_TX
 *
 * UART TX Pin.
 *
 */
#define UART_PIN_TX  6

/**
 * @def UART_PIN_RX
 *
 * UART RX Pin.
 *
 */
#define UART_PIN_RX  8

/**
 * @def UART_PIN_CTS
 *
 * UART CTS Pin.
 *
 */
#define UART_PIN_CTS  7

/**
 * @def UART_PIN_RTS
 *
 * UART RTS Pin.
 *
 */
#define UART_PIN_RTS  5

/*******************************************************************************
 * @section Alarm Driver Configuration.
 ******************************************************************************/

/**
 * @def RTC_INSTANCE
 *
 * RTC Instance.
 *
 */
#define RTC_INSTANCE  NRF_RTC2

/**
 * @def RTC_IRQ_HANDLER
 *
 * RTC interrupt handler name
 *
 */
#define RTC_IRQ_HANDLER  RTC2_IRQHandler

/**
 * @def RTC_IRQN
 *
 * RTC Interrupt number.
 *
 */
#define RTC_IRQN  RTC2_IRQn

/**
 * @def RTC_IRQ_PRIORITY
 *
 * RTC Interrupt priority.
 *
 */
#define RTC_IRQ_PRIORITY  6

/*******************************************************************************
 * @section Random Number Generator Driver Configuration.
 ******************************************************************************/

/**
 * @def RNG_BUFFER_SIZE
 *
 * True Random Number Generator buffer size.
 *
 */
#define RNG_BUFFER_SIZE  64

/**
 * @def RNG_IRQ_PRIORITY
 *
 * RNG Interrupt priority.
 *
 */
#define RNG_IRQ_PRIORITY  6

/*******************************************************************************
 * @section Log module configuration.
 ******************************************************************************/

/**
 * @def LOG_RTT_BUFFER_INDEX
 *
 * RTT's buffer index.
 *
 */
#define LOG_RTT_BUFFER_INDEX  0

/**
 * @def LOG_RTT_BUFFER_NAME
 *
 * RTT's name.
 *
 */
#define LOG_RTT_BUFFER_NAME  "Terminal"

/**
 * @def LOG_RTT_BUFFER_SIZE
 *
 * LOG RTT's buffer size.
 *
 */
#define LOG_RTT_BUFFER_SIZE  256

/**
 * @def LOG_RTT_COLOR_ENABLE
 *
 * Enable colors on RTT Viewer.
 *
 */
#define LOG_RTT_COLOR_ENABLE  1

/**
 * @def LOG_PARSE_BUFFER_SIZE
 *
 * LOG buffer used to parse print format. It will be locally allocated on the
 * stack.
 *
 */
#define LOG_PARSE_BUFFER_SIZE  128

/**
 * @def LOG_TIMESTAMP_ENABLE
 *
 * Enable timestamp in the logs.
 *
 */
#define LOG_TIMESTAMP_ENABLE  1

#endif  // PLATFORM_CONFIG_H_
