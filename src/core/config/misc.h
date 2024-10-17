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
 *   This file includes miscellaneous compile-time configuration constants for OpenThread.
 */

#ifndef CONFIG_MISC_H_
#define CONFIG_MISC_H_

/**
 * @addtogroup config-misc
 *
 * @brief
 *   This module includes configuration variables for Miscellaneous constants.
 *
 * @{
 */

#include "config/coap.h"
#include "config/srp_server.h"

/**
 * @def OPENTHREAD_CONFIG_STACK_VENDOR_OUI
 *
 * The Organizationally Unique Identifier for the Thread stack.
 */
#ifndef OPENTHREAD_CONFIG_STACK_VENDOR_OUI
#define OPENTHREAD_CONFIG_STACK_VENDOR_OUI 0x18b430
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_REV
 *
 * The Stack Version Revision for the Thread stack.
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_REV
#define OPENTHREAD_CONFIG_STACK_VERSION_REV 0
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_MAJOR
 *
 * The Stack Version Major for the Thread stack.
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_MAJOR
#define OPENTHREAD_CONFIG_STACK_VERSION_MAJOR 0
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_MINOR
 *
 * The Stack Version Minor for the Thread stack.
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_MINOR
#define OPENTHREAD_CONFIG_STACK_VERSION_MINOR 1
#endif

/**
 * @def OPENTHREAD_CONFIG_DEVICE_POWER_SUPPLY
 *
 * Specifies the default device power supply config. This config MUST use values from `otPowerSupply` enumeration.
 *
 * Device manufacturer can use this config to set the power supply config used by the device. This is then used as part
 * of default `otDeviceProperties` to determine the Leader Weight used by the device.
 */
#ifndef OPENTHREAD_CONFIG_DEVICE_POWER_SUPPLY
#define OPENTHREAD_CONFIG_DEVICE_POWER_SUPPLY OT_POWER_SUPPLY_EXTERNAL
#endif

/**
 * @def OPENTHREAD_CONFIG_ECDSA_ENABLE
 *
 * Define to 1 to enable ECDSA support.
 */
#ifndef OPENTHREAD_CONFIG_ECDSA_ENABLE
#define OPENTHREAD_CONFIG_ECDSA_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_DETERMINISTIC_ECDSA_ENABLE
 *
 * Define to 1 to generate ECDSA signatures deterministically
 * according to RFC 6979 instead of randomly.
 */
#ifndef OPENTHREAD_CONFIG_DETERMINISTIC_ECDSA_ENABLE
#define OPENTHREAD_CONFIG_DETERMINISTIC_ECDSA_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_UPTIME_ENABLE
 *
 * Define to 1 to enable tracking the uptime of OpenThread instance.
 */
#ifndef OPENTHREAD_CONFIG_UPTIME_ENABLE
#define OPENTHREAD_CONFIG_UPTIME_ENABLE OPENTHREAD_FTD
#endif

/**
 * @def OPENTHREAD_CONFIG_JAM_DETECTION_ENABLE
 *
 * Define to 1 to enable the Jam Detection service.
 */
#ifndef OPENTHREAD_CONFIG_JAM_DETECTION_ENABLE
#define OPENTHREAD_CONFIG_JAM_DETECTION_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE
 *
 * Define to 1 to enable Verhoeff checksum utility module.
 */
#ifndef OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE
#define OPENTHREAD_CONFIG_VERHOEFF_CHECKSUM_ENABLE OPENTHREAD_CONFIG_BORDER_AGENT_EPHEMERAL_KEY_ENABLE
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
 *
 * Define to 1 to enable multiple instance support.
 */
#ifndef OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
#define OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
 *
 * Define to 1 to enable multipan RCP support.
 */
#ifndef OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE
#define OPENTHREAD_CONFIG_MULTIPAN_RCP_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
 *
 * Define to 1 to enable Thread Test Harness reference device support.
 */
#ifndef OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#define OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
 *
 * Define to 1 to enable UDP forward support.
 */
#ifndef OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
#define OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
 *
 * Whether use heap allocator for message buffers.
 *
 * @note If this is set, OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS is ignored.
 */
#ifndef OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
#define OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 */
#ifndef OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 44
#endif

/**
 * @def OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
 *
 * The size of a message buffer in bytes.
 *
 * Message buffers store pointers which have different sizes on different
 * system. Setting message buffer size according to the CPU word length
 * so that message buffer size will be doubled on 64bit system compared
 * to that on 32bit system. As a result, the first message always have some
 * bytes left for small packets.
 *
 * Some configuration options can increase the buffer size requirements, including
 * OPENTHREAD_CONFIG_MLE_MAX_CHILDREN and OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE.
 */
#ifndef OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE (sizeof(void *) * 32)
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER
 *
 * The default IEEE 802.15.4 transmit power (dBm).
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER
#define OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER 0
#endif

/**
 * @def OPENTHREAD_CONFIG_JOINER_UDP_PORT
 *
 * The default Joiner UDP port.
 */
#ifndef OPENTHREAD_CONFIG_JOINER_UDP_PORT
#define OPENTHREAD_CONFIG_JOINER_UDP_PORT 1000
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 */
#ifndef OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS 1
#endif

/**
 * @def OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
 *
 * The value ahead of the current frame counter for persistent storage.
 */
#ifndef OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
#define OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD 1000
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS
 *
 * Define as 1 to enable builtin-mbedtls.
 *
 * Note that the OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS determines whether to use builtin-mbedtls as well as
 * whether to manage mbedTLS internally, such as memory allocation and debug.
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS
#define OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS 1
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT
 *
 * Define as 1 to enable builtin mbedtls management.
 *
 * OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT determines whether to manage mbedTLS memory
 * allocation and debug config internally.  If not configured, the default is to enable builtin
 * management if builtin mbedtls is enabled and disable it otherwise.
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT
#define OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE
 *
 * The size of heap buffer when DTLS is enabled.
 */
#ifndef OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
// Internal heap doesn't support size larger than 64K bytes.
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE (63 * 1024)
#elif OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE (3136 * sizeof(void *))
#else
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE (1616 * sizeof(void *))
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS
 *
 * The size of heap buffer when DTLS is disabled.
 */
#ifndef OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS
#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
// Internal heap doesn't support size larger than 64K bytes.
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS (63 * 1024)
#elif OPENTHREAD_CONFIG_ECDSA_ENABLE
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS 2600
#else
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS 384
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
 *
 * Enable the external heap.
 */
#ifndef OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
#define OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_DTLS_APPLICATION_DATA_MAX_LENGTH
 *
 * The size of dtls application data when the CoAP Secure API is enabled.
 */
#ifndef OPENTHREAD_CONFIG_DTLS_APPLICATION_DATA_MAX_LENGTH
#define OPENTHREAD_CONFIG_DTLS_APPLICATION_DATA_MAX_LENGTH 1400
#endif

/**
 * @def OPENTHREAD_CONFIG_ASSERT_ENABLE
 *
 * Define as 1 to enable assert function `OT_ASSERT()` within OpenThread code and its libraries.
 */
#ifndef OPENTHREAD_CONFIG_ASSERT_ENABLE
#define OPENTHREAD_CONFIG_ASSERT_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL
 *
 * Define as 1 to enable assert check of pointer-type API input parameters against null.
 *
 * Enabling this feature can increase code-size significantly due to many assert checks added for all API pointer
 * parameters. It is recommended to enable and use this feature during debugging only.
 */
#ifndef OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL
#define OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL 0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_DEBUG_UART
 *
 * Enable the "Debug Uart" platform feature.
 *
 * In the embedded world, the CLI application uses a UART as a console
 * and the NCP application can be configured to use either a UART or
 * a SPI type device to transfer data to the host.
 *
 * The Debug UART is or requires a second uart on the platform.
 *
 * The Debug Uart has two uses:
 *
 * Use #1 - for random 'debug printf' type messages a developer may need
 * Use #2 (selected via DEBUG_LOG_OUTPUT) is a log output.
 *
 * See #include <openthread/platform/debug_uart.h> for more details
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_DEBUG_UART
#define OPENTHREAD_CONFIG_ENABLE_DEBUG_UART 0
#endif

/**
 * @def OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH
 *
 * The settings storage path on posix platform.
 */
#ifndef OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH
#define OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH "tmp"
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
 *
 * Define to 1 to enable otPlatFlash* APIs to support non-volatile storage.
 *
 * When defined to 1, the platform MUST implement the otPlatFlash* APIs instead of the otPlatSettings* APIs.
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS
 *
 * This setting configures the number of consecutive MCPS.DATA-Confirms having Status NO_ACK
 * that cause a Child-to-Parent link to be considered broken.
 */
#ifndef OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS
#define OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS 4
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE
 *
 * Specifies the value used in emitted Connectivity TLV "Rx-off Child Buffer Size" field which indicates the
 * guaranteed buffer capacity for all IPv6 datagrams destined to a given rx-off-when-idle child.
 *
 * Changing this config does not automatically adjust message buffers. Vendors should ensure their device can support
 * the specified value based on the message buffer model used:
 *  - OT internal message pool (refer to `OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS` and `MESSAGE_BUFFER_SIZE`), or
 *  - Heap allocated message buffers (refer to `OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE),
 *  - Platform-specific message management (refer to`OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT`).
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE
#define OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE 1280
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT
 *
 * Specifies the value used in emitted Connectivity TLV "Rx-off Child Datagram Count" field which indicates the
 * guaranteed queue capacity in number of IPv6 datagrams destined to a given rx-off-when-idle child.
 *
 * Similar to `OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE`, vendors should ensure their device can support the specified
 * value based on the message buffer model used.
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT
#define OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT
 *
 * Define to 1 to support proprietary radio configurations defined by platform.
 *
 * @note If this setting is set to 1, the channel range is defined by the platform. Choosing this option requires
 * the following configuration options to be defined by Platform:
 * OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_PAGE,
 * OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MIN,
 * OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MAX and,
 * OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_CHANNEL_MASK.
 *
 * @def OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
 *
 * Define to 1 to support OQPSK modulation in 915MHz frequency band. The physical layer parameters are defined in
 * section 6 of IEEE802.15.4-2006.
 *
 * @note If this setting is set to 1, the IEEE 802.15.4 channel range is 1 to 10.
 *
 * @def OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
 *
 * Define to 1 to support OQPSK modulation in 2.4GHz frequency band. The physical layer parameters are defined in
 * section 6 of IEEE802.15.4-2006.
 *
 * @note If this settings is set to 1, the IEEE 802.15.4 channel range is 11 to 26.
 *
 * @note At least one of these settings must be set to 1. The platform must support the modulation and frequency
 *       band configured by the setting.
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT
#ifndef OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#ifndef OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#define OPENTHREAD_CONFIG_PLATFORM_RADIO_PROPRIETARY_SUPPORT 0
#define OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT 0
#define OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT 1
#endif
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CHANNEL
 *
 * The default IEEE 802.15.4 channel.
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_CHANNEL
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#define OPENTHREAD_CONFIG_DEFAULT_CHANNEL 11
#else
#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define OPENTHREAD_CONFIG_DEFAULT_CHANNEL 1
#endif // OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#endif // OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#endif // OPENTHREAD_CONFIG_DEFAULT_CHANNEL

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_WAKEUP_CHANNEL
 *
 * The default IEEE 802.15.4 wake-up channel.
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_WAKEUP_CHANNEL
#define OPENTHREAD_CONFIG_DEFAULT_WAKEUP_CHANNEL 11
#endif

/**
 * @def OPENTHREAD_CONFIG_OTNS_ENABLE
 *
 * Define to 1 to enable OTNS interactions.
 */
#ifndef OPENTHREAD_CONFIG_OTNS_ENABLE
#define OPENTHREAD_CONFIG_OTNS_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_DUA_ENABLE
 *
 * Define as 1 to support Thread 1.2 Domain Unicast Address feature.
 */
#ifndef OPENTHREAD_CONFIG_DUA_ENABLE
#define OPENTHREAD_CONFIG_DUA_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MLR_ENABLE
 *
 * Define as 1 to support Thread 1.2 Multicast Listener Registration feature.
 */
#ifndef OPENTHREAD_CONFIG_MLR_ENABLE
#define OPENTHREAD_CONFIG_MLR_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE
 *
 * Define as 1 to enable support for Neighbor Discover Agent.
 */
#ifndef OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE
#define OPENTHREAD_CONFIG_NEIGHBOR_DISCOVERY_AGENT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE
 *
 * Define to 1 to enable multiple static instance support.
 */
#ifndef OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE
#define OPENTHREAD_CONFIG_MULTIPLE_STATIC_INSTANCE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM
 *
 * Define number of OpenThread instance for static allocation buffer.
 */
#ifndef OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM
#define OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_NUM 3
#endif

/**
 * @def OPENTHREAD_CONFIG_ALLOW_EMPTY_NETWORK_NAME
 *
 * Define as 1 to enable support for an empty network name (zero-length: "")
 */
#ifndef OPENTHREAD_CONFIG_ALLOW_EMPTY_NETWORK_NAME
#define OPENTHREAD_CONFIG_ALLOW_EMPTY_NETWORK_NAME 0
#endif

/**
 * @def OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT
 *
 * Define as 1 to enable support for locally initializing an Active Operational Dataset.
 *
 * @note This functionality is deprecated and not recommended.
 */
#ifndef OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT
#define OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
 *
 * Define to 1 to enable TCAT over BLE support.
 */
#ifndef OPENTHREAD_CONFIG_BLE_TCAT_ENABLE
#define OPENTHREAD_CONFIG_BLE_TCAT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_LOG_CRASH_DUMP_ENABLE
 *
 * Define to 1 to enable crash dump logging.
 *
 * On platforms that support crash dump logging, this feature will log a crash dump using the OT Debug Log service.
 *
 * Logging a crash dump requires the platform to implement the `otPlatLogCrashDump()` function.
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_LOG_CRASH_DUMP_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_LOG_CRASH_DUMP_ENABLE 0
#endif

/**
 * @}
 */

#endif // CONFIG_MISC_H_
