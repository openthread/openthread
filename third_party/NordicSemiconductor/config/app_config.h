/**
 * Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @brief This file provides OpenThread configuration of nRF5 SDK modules.
 *
 * Below configuration overwrites the default sdk_config.h file for certain platform.
 *
 * Default configuration for the vanilla OpenThread can be overwritten by the custom configuration.
 * To do that, provide the configuration through the APP_CONFIG_CUSTOM_FILE define.
 *
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#if (USB_CDC_AS_SERIAL_TRANSPORT == 1)

#ifndef APP_USBD_ENABLED
#define APP_USBD_ENABLED 1
#endif

#ifndef APP_USBD_VID
#define APP_USBD_VID 0x1915
#endif

#ifndef APP_USBD_PID
#define APP_USBD_PID 0xCAFE
#endif

#ifndef APP_USBD_DEVICE_VER_MAJOR
#define APP_USBD_DEVICE_VER_MAJOR 1
#endif

#ifndef APP_USBD_DEVICE_VER_MINOR
#define APP_USBD_DEVICE_VER_MINOR 0
#endif

#ifndef APP_USBD_DEVICE_VER_SUB
#define APP_USBD_DEVICE_VER_SUB 0
#endif

#ifndef APP_USBD_CONFIG_MAX_POWER
#define APP_USBD_CONFIG_MAX_POWER 500
#endif

#ifndef APP_USBD_STRING_ID_PRODUCT
#define APP_USBD_STRING_ID_PRODUCT 2
#endif

#ifndef APP_USBD_STRINGS_PRODUCT_EXTERN
#define APP_USBD_STRINGS_PRODUCT_EXTERN 0
#endif

#ifndef APP_USBD_STRINGS_PRODUCT
#define APP_USBD_STRINGS_PRODUCT APP_USBD_STRING_DESC("nRF528xx OpenThread Device")
#endif

#ifndef APP_USBD_STRING_ID_SERIAL
#define APP_USBD_STRING_ID_SERIAL 3
#endif

#ifndef APP_USBD_STRING_SERIAL_EXTERN
#define APP_USBD_STRING_SERIAL_EXTERN 1
#endif

#ifndef APP_USBD_STRING_SERIAL
#define APP_USBD_STRING_SERIAL g_extern_serial_number
#endif

#ifndef APP_USBD_CONFIG_LOG_ENABLED
#define APP_USBD_CONFIG_LOG_ENABLED 0
#endif

#ifndef USBD_ENABLED
#define USBD_ENABLED 1
#endif

#ifndef NRFX_USBD_ENABLED
#define NRFX_USBD_ENABLED 1
#endif

#ifndef NRFX_USBD_CONFIG_IRQ_PRIORITY
#define NRFX_USBD_CONFIG_IRQ_PRIORITY 7
#endif

#ifndef NRFX_SYSTICK_ENABLED
#define NRFX_SYSTICK_ENABLED 1
#endif

#ifndef APP_USBD_CDC_ACM_ENABLED
#define APP_USBD_CDC_ACM_ENABLED 1
#endif

#endif // USB_CDC_AS_SERIAL_TRANSPORT == 1

#ifndef APP_USBD_NRF_DFU_TRIGGER_ENABLED
#define APP_USBD_NRF_DFU_TRIGGER_ENABLED 0
#endif

#ifndef BSP_SELF_PINRESET_PIN
#define BSP_SELF_PINRESET_PIN NRF_GPIO_PIN_MAP(0, 19)
#endif

#ifndef NRF_DFU_TRIGGER_USB_USB_SHARED
#define NRF_DFU_TRIGGER_USB_USB_SHARED 1
#endif

#ifndef NRF_DFU_TRIGGER_USB_INTERFACE_NUM
#define NRF_DFU_TRIGGER_USB_INTERFACE_NUM 0
#endif

#ifndef APP_NAME
#define APP_NAME "OpenThread App"
#endif

#ifndef APP_VERSION_MAJOR
#define APP_VERSION_MAJOR 1
#endif

#ifndef APP_VERSION_MINOR
#define APP_VERSION_MINOR 0
#endif

#ifndef APP_VERSION_PATCH
#define APP_VERSION_PATCH 0
#endif

#ifndef APP_ID
#define APP_ID 1
#endif

#ifndef APP_VERSION_PRERELEASE
#define APP_VERSION_PRERELEASE ""
#endif

#ifndef APP_VERSION_METADATA
#define APP_VERSION_METADATA ""
#endif

#ifndef SEGGER_RTT_CONFIG_BUFFER_SIZE_UP
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_UP 512
#endif

#ifndef SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS
#define SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS 2
#endif

#ifndef SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN 16
#endif

#ifndef SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS
#define SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS 2
#endif

#ifndef SEGGER_RTT_CONFIG_DEFAULT_MODE
#define SEGGER_RTT_CONFIG_DEFAULT_MODE 0
#endif

#ifndef NRF_CLOCK_ENABLED
#define NRF_CLOCK_ENABLED 1
#endif

#ifndef CLOCK_CONFIG_LF_SRC
#define CLOCK_CONFIG_LF_SRC 1
#endif

#ifndef CLOCK_CONFIG_IRQ_PRIORITY
#define CLOCK_CONFIG_IRQ_PRIORITY 7
#endif

#if (USB_CDC_AS_SERIAL_TRANSPORT == 1)

#ifndef POWER_ENABLED
#define POWER_ENABLED 1
#endif

#endif // USB_CDC_AS_SERIAL_TRANSPORT == 1

#ifndef POWER_CONFIG_IRQ_PRIORITY
#define POWER_CONFIG_IRQ_PRIORITY 7
#endif

#if (SPIS_AS_SERIAL_TRANSPORT == 1)

#ifndef SPIS_ENABLED
#define SPIS_ENABLED 1
#endif

#ifndef SPIS_DEFAULT_CONFIG_IRQ_PRIORITY
#define SPIS_DEFAULT_CONFIG_IRQ_PRIORITY 7
#endif

#ifndef SPIS0_ENABLED
#define SPIS0_ENABLED 1
#endif

#endif // SPIS_AS_SERIAL_TRANSPORT == 1

#ifndef TIMER_ENABLED
#define TIMER_ENABLED 1
#endif

#ifndef TIMER0_ENABLED
#define TIMER0_ENABLED 1
#endif

#ifndef TIMER1_ENABLED
#define TIMER1_ENABLED 1
#endif

#ifndef TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
#define TIMER_DEFAULT_CONFIG_IRQ_PRIORITY 7
#endif

#endif // APP_CONFIG_H
