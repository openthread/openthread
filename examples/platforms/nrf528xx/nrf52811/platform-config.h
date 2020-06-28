/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_peripherals.h"
#include "hal/nrf_radio.h"
#include "hal/nrf_uart.h"

#include "openthread-core-config.h"
#include <openthread/config.h>

/*******************************************************************************
 * @section Alarm Driver Configuration.
 ******************************************************************************/

/**
 * @def RTC_INSTANCE
 *
 * RTC Instance.
 *
 */
#ifndef RTC_INSTANCE
#define RTC_INSTANCE NRF_RTC0
#endif

/**
 * @def RTC_IRQ_HANDLER
 *
 * RTC interrupt handler name
 *
 */
#ifndef RTC_IRQ_HANDLER
#define RTC_IRQ_HANDLER RTC0_IRQHandler
#endif

/**
 * @def RTC_IRQN
 *
 * RTC Interrupt number.
 *
 */
#ifndef RTC_IRQN
#define RTC_IRQN RTC0_IRQn
#endif

/**
 * @def RTC_IRQ_PRIORITY
 *
 * RTC Interrupt priority.
 *
 */
#ifndef RTC_IRQ_PRIORITY
#define RTC_IRQ_PRIORITY 6
#endif

/*******************************************************************************
 * @section Random Number Generator Driver Configuration.
 ******************************************************************************/

/**
 * @def RNG_BUFFER_SIZE
 *
 * True Random Number Generator buffer size.
 *
 */
#ifndef RNG_BUFFER_SIZE
#define RNG_BUFFER_SIZE 128
#endif

/**
 * @def RNG_IRQ_PRIORITY
 *
 * RNG Interrupt priority.
 *
 */
#ifndef RNG_IRQ_PRIORITY
#define RNG_IRQ_PRIORITY 6
#endif

/*******************************************************************************
 * @section Platform Flash Configuration
 ******************************************************************************/

/**
 * @def PLATFORM_FLASH_PAGE_NUM
 *
 * Number of flash pages to use for OpenThread's non-volatile settings.
 *
 * @note This define applies only for MDK-ARM Keil toolchain configuration.
 *
 */
#ifndef PLATFORM_FLASH_PAGE_NUM
#define PLATFORM_FLASH_PAGE_NUM 2
#endif

/*******************************************************************************
 * @section Platform FEM Configuration
 ******************************************************************************/

/**
 * @def PLATFORM_FEM_ENABLE_DEFAULT_CONFIG
 *
 * Enable default front end module configuration.
 *
 */
#ifndef PLATFORM_FEM_ENABLE_DEFAULT_CONFIG
#define PLATFORM_FEM_ENABLE_DEFAULT_CONFIG 0
#endif

/*******************************************************************************
 * @section Radio driver configuration.
 ******************************************************************************/

/**
 * @def NRF_802154_PENDING_SHORT_ADDRESSES
 *
 * Number of slots containing short addresses of nodes for which pending data is stored.
 *
 */
#ifndef NRF_802154_PENDING_SHORT_ADDRESSES
#define NRF_802154_PENDING_SHORT_ADDRESSES OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
#endif

/**
 * @def NRF_802154_PENDING_EXTENDED_ADDRESSES
 *
 * Number of slots containing extended addresses of nodes for which pending data is stored.
 *
 */
#ifndef NRF_802154_PENDING_EXTENDED_ADDRESSES
#define NRF_802154_PENDING_EXTENDED_ADDRESSES OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
#endif

/**
 * @def NRF_802154_CSMA_CA_ENABLED
 *
 * If CSMA-CA procedure should be enabled by the driver. Disabling CSMA-CA procedure improves
 * driver performance.
 *
 */
#ifndef NRF_802154_CSMA_CA_ENABLED
#define NRF_802154_CSMA_CA_ENABLED 1
#endif

/**
 * @def NRF_802154_ACK_TIMEOUT_ENABLED
 *
 * If ACK timeout feature should be enabled in the driver.
 *
 */
#ifndef NRF_802154_ACK_TIMEOUT_ENABLED
#define NRF_802154_ACK_TIMEOUT_ENABLED 1
#endif

/*******************************************************************************
 * @section Temperature sensor driver configuration.
 ******************************************************************************/

/**
 * @def TEMP_MEASUREMENT_INTERVAL
 *
 * Interval of consecutive temperature measurements [s].
 *
 */
#ifndef TEMP_MEASUREMENT_INTERVAL
#define TEMP_MEASUREMENT_INTERVAL 30
#endif

/**
 * @def NRF_802154_TX_STARTED_NOTIFY_ENABLED
 *
 * If notification of started transmission should be enabled in the driver.
 *
 * @note This feature must be enabled to support Header IE related features.
 *
 */
#ifndef NRF_802154_TX_STARTED_NOTIFY_ENABLED
#define NRF_802154_TX_STARTED_NOTIFY_ENABLED 1
#endif

#endif // PLATFORM_CONFIG_H_
