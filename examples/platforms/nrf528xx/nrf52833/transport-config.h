/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes the transport-specific configuration.
 *
 */

#ifndef TRANSPORT_CONFIG_H_
#define TRANSPORT_CONFIG_H_

#include "nrf.h"
#include "hal/nrf_spis.h"
#include "hal/nrf_uart.h"

/*******************************************************************************
 * @section UART Driver Configuration.
 ******************************************************************************/

/**
 * @def UART_INSTANCE
 *
 * UART Instance.
 *
 */
#ifndef UART_INSTANCE
#define UART_INSTANCE NRF_UART0
#endif

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
#ifndef UART_PARITY
#define UART_PARITY NRF_UART_PARITY_EXCLUDED
#endif

/**
 * @def UART_HWFC_ENABLED
 *
 * Enable UART Hardware Flow Control.
 *
 */
#ifndef UART_HWFC_ENABLED
#define UART_HWFC_ENABLED 1
#endif

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
#ifndef UART_BAUDRATE
#define UART_BAUDRATE NRF_UART_BAUDRATE_115200
#endif

/**
 *  @def UART_IRQN
 *
 * UART Interrupt number.
 *
 */
#ifndef UART_IRQN
#define UART_IRQN UARTE0_UART0_IRQn
#endif

/**
 * @def UART_IRQ_PRIORITY
 *
 * UART Interrupt priority.
 *
 */
#ifndef UART_IRQ_PRIORITY
#define UART_IRQ_PRIORITY 6
#endif

/**
 * @def UART_RX_BUFFER_SIZE
 *
 * UART Receive buffer size.
 *
 */
#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE 512
#endif

/**
 * @def UART_PIN_TX
 *
 * UART TX Pin.
 *
 */
#ifndef UART_PIN_TX
#define UART_PIN_TX 6
#endif

/**
 * @def UART_PIN_RX
 *
 * UART RX Pin.
 *
 */
#ifndef UART_PIN_RX
#define UART_PIN_RX 8
#endif

/**
 * @def UART_PIN_CTS
 *
 * UART CTS Pin.
 *
 */
#ifndef UART_PIN_CTS
#define UART_PIN_CTS 7
#endif

/**
 * @def UART_PIN_RTS
 *
 * UART RTS Pin.
 *
 */
#ifndef UART_PIN_RTS
#define UART_PIN_RTS 5
#endif

/*******************************************************************************
 * @section SPI Slave configuration.
 ******************************************************************************/

/**
 * @def SPIS Instance.
 */
#ifndef SPIS_INSTANCE
#define SPIS_INSTANCE 0
#endif

/**
 * @def SPIS mode.
 *
 * @brief Possible values:
 *         \ref NRF_SPIS_MODE_0 - SCK active high, sample on leading edge of clock.
 *         \ref NRF_SPIS_MODE_1 - SCK active high, sample on trailing edge of clock.
 *         \ref NRF_SPIS_MODE_2 - SCK active low, sample on leading edge of clock.
 *         \ref NRF_SPIS_MODE_3 - SCK active low, sample on trailing edge of clock.
 */
#ifndef SPIS_MODE
#define SPIS_MODE NRF_SPIS_MODE_0
#endif

/**
 * @def SPIS bit orders.
 *
 * @brief Possible values:
 *         \ref NRF_SPIS_BIT_ORDER_MSB_FIRST - Most significant bit shifted out first.
 *         \ref NRF_SPIS_BIT_ORDER_LSB_FIRST - Least significant bit shifted out first.
 */
#ifndef SPIS_BIT_ORDER
#define SPIS_BIT_ORDER NRF_SPIS_BIT_ORDER_MSB_FIRST
#endif

/**
 * @def SPIS Interrupt priority.
 */
#ifndef SPIS_IRQ_PRIORITY
#define SPIS_IRQ_PRIORITY 6
#endif

/**
 * @def SPIS MOSI Pin.
 */
#ifndef SPIS_PIN_MOSI
#define SPIS_PIN_MOSI 4
#endif

/**
 * @def SPIS MISO Pin.
 */
#ifndef SPIS_PIN_MISO
#define SPIS_PIN_MISO 28
#endif

/**
 * @def SPIS SCK Pin.
 */
#ifndef SPIS_PIN_SCK
#define SPIS_PIN_SCK 3
#endif

/**
 * @def SPIS CSN Pin.
 */
#ifndef SPIS_PIN_CSN
#define SPIS_PIN_CSN 29
#endif

/**
 * @def SPIS Host IRQ Pin.
 */
#ifndef SPIS_PIN_HOST_IRQ
#define SPIS_PIN_HOST_IRQ 30
#endif

/*******************************************************************************
 * @section USB driver configuration.
 ******************************************************************************/

/**
 * @def USB_HOST_UART_CONFIG_DELAY_MS
 *
 * Delay after DTR gets asserted that we start send any queued data. This allows slow
 * Linux-based hosts to have enough time to configure their port for raw mode.
 *
 */
#ifndef USB_HOST_UART_CONFIG_DELAY_MS
#define USB_HOST_UART_CONFIG_DELAY_MS 10
#endif

/**
 * @def USB_CDC_AS_SERIAL_TRANSPORT
 *
 * Use USB CDC driver for serial communication.
 */
#ifndef USB_CDC_AS_SERIAL_TRANSPORT
#define USB_CDC_AS_SERIAL_TRANSPORT 0
#endif

/**
 * @def The USB interface to use for CDC ACM COMM.
 *
 * According to the USB Specification, interface numbers cannot have gaps. Tailor this value to adhere to this
 * limitation. Takes values between 0-255.
 */
#ifndef USB_CDC_ACM_COMM_INTERFACE
#define USB_CDC_ACM_COMM_INTERFACE 1
#endif

/**
 * @def The USB interface to use for CDC ACM DATA.
 *
 * According to the USB Specification, interface numbers cannot have gaps. Tailor this value to adhere to this
 * limitation. Takes values between 0-255.
 */
#ifndef USB_CDC_ACM_DATA_INTERFACE
#define USB_CDC_ACM_DATA_INTERFACE 2
#endif

/**
 * @def OPENTHREAD_PLATFORM_USE_PSEUDO_RESET
 *
 * Reset the application, not the chip, when a software reset is requested.
 * via `otPlatReset()`.
 */
#ifndef OPENTHREAD_PLATFORM_USE_PSEUDO_RESET
#define OPENTHREAD_PLATFORM_USE_PSEUDO_RESET USB_CDC_AS_SERIAL_TRANSPORT
#endif

#endif // TRANSPORT_CONFIG_H_
