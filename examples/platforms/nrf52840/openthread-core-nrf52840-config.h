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
 *   This file includes nrf52840 compile-time configuration constants
 *   for OpenThread.
 */

#ifndef OPENTHREAD_CORE_NRF52840_CONFIG_H_
#define OPENTHREAD_CORE_NRF52840_CONFIG_H_

/*
 * The GNU Autoconf system defines a PACKAGE macro which is the name
 * of the software package. This name collides with PACKAGE field in
 * the nRF52 Factory Information Configuration Registers (FICR)
 * structure.
 */
#undef PACKAGE

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * The nrf52840 platform provides an otPlatLog() function.
 */
#ifndef OPENTHREAD_CONFIG_LOG_OUTPUT /* allow command line override */
#define OPENTHREAD_CONFIG_LOG_OUTPUT  OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO                         "NRF52840"
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MAX_CHILDREN                          32
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS                   160
#endif

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
 *
 * The maximum number of supported IPv6 address registrations per child.
 *
 */
#ifndef OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
#define OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD                    6
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS              3
#endif

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES                 32
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPREND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL                     0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
 *
 * Define to 1 if you want to enable software ACK timeout logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT           0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
 *
 * Define to 1 if you want to enable software retransmission logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT            1
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
 *
 * Define to 1 if you want to enable software CSMA-CA backoff logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF          0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
 *
 * Define to 1 if you want to support microsecond timer in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
#define OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER            1
#endif

/**
 * @def SETTINGS_CONFIG_BASE_ADDRESS
 *
 * The base address of settings.
 *
 */
#ifndef SETTINGS_CONFIG_BASE_ADDRESS
#define SETTINGS_CONFIG_BASE_ADDRESS                            0
#endif

/**
 * @def SETTINGS_CONFIG_PAGE_SIZE
 *
 * The page size of settings.
 *
 */
#ifndef SETTINGS_CONFIG_PAGE_SIZE
#define SETTINGS_CONFIG_PAGE_SIZE                               4096
#endif

/**
 * @def SETTINGS_CONFIG_PAGE_NUM
 *
 * The page number of settings.
 *
 */
#ifndef SETTINGS_CONFIG_PAGE_NUM
#define SETTINGS_CONFIG_PAGE_NUM                                4
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_SIZE
 *
 * The size of heap buffer when DTLS is enabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_HEAP_SIZE
#define OPENTHREAD_CONFIG_HEAP_SIZE                             (4096 * sizeof(void *))
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_SIZE_NO_DTLS
 *
 * The size of heap buffer when DTLS is disabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_HEAP_SIZE_NO_DTLS
#define OPENTHREAD_CONFIG_HEAP_SIZE_NO_DTLS                     2048
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
 *
 * Define as 1 to enable the time synchronization service feature.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
#define OPENTHREAD_CONFIG_ENABLE_TIME_SYNC                      0
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 *  The size of CLI message buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE               2048
#endif

/**
 * @def OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
 *
 * Define as 1 to support IEEE 802.15.4-2015 Header IE (Information Element) generation and parsing, it must be set
 * to support following features:
 *    1. Time synchronization service feature (i.e., OPENTHREAD_CONFIG_ENABLE_TIME_SYNC is set).
 *
 * @note If it's enabled, plaforms must support interrupt context and concurrent access AES.
 *
 */
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
#define OPENTHREAD_CONFIG_HEADER_IE_SUPPORT                     1
#endif

/**
 * @def NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT
 *
 * Define as 1 to enable AES usage in interrupt context and AES-256, by introducing a software AES under platform layer.
 *
 * @note This feature must be enabled to support AES-256 used by Commissioner and Joiner, and AES usage in interrupt context
 *       used by Header IE related features.
 *
 */
#if OPENTHREAD_ENABLE_COMMISSIONER || OPENTHREAD_ENABLE_JOINER || OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
#define NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT                   1
#else
#define NRF_MBEDTLS_AES_ALT_INTERRUPT_CONTEXT                   0
#endif

/*
 * Suppress the ARMCC warning on unreachable statement,
 * e.g. break after assert(false) or ExitNow() macro.
 */
#if defined(__CC_ARM)
    _Pragma("diag_suppress=111")
    _Pragma("diag_suppress=128")
#endif

#endif  // OPENTHREAD_CORE_NRF52840_CONFIG_H_
