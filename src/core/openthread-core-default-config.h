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
 *   This file includes default compile-time configuration constants
 *   for OpenThread.
 */

#ifndef OPENTHREAD_CORE_DEFAULT_CONFIG_H_
#define OPENTHREAD_CORE_DEFAULT_CONFIG_H_

/**
 * @def OPENTHREAD_CONFIG_STACK_VENDOR_OUI
 *
 * The Organizationally Unique Identifier for the Thread stack.
 *
 */
#ifndef OPENTHREAD_CONFIG_STACK_VENDOR_OUI
#define OPENTHREAD_CONFIG_STACK_VENDOR_OUI                      0x18b430
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_REV
 *
 * The Stack Version Revision for the Thread stack.
 *
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_REV
#define OPENTHREAD_CONFIG_STACK_VERSION_REV                     0
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_MAJOR
 *
 * The Stack Version Major for the Thread stack.
 *
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_MAJOR
#define OPENTHREAD_CONFIG_STACK_VERSION_MAJOR                   0
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_MINOR
 *
 * The Stack Version Minor for the Thread stack.
 *
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_MINOR
#define OPENTHREAD_CONFIG_STACK_VERSION_MINOR                   1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO                         "NONE"
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS                   40
#endif

/**
 * @def OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
 *
 * The size of a message buffer in bytes.
 *
 */
#ifndef OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE                   128
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CHANNEL
 *
 * The default IEEE 802.15.4 channel.
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_CHANNEL
#define OPENTHREAD_CONFIG_DEFAULT_CHANNEL                       11
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER
 *
 * The default IEEE 802.15.4 transmit power (dBm)
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER
#define OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER                0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT
 *
 * Maximum number of MAC layer transmit attempts for an outbound direct frame.
 * Per IEEE 802.15.4-2006, default value is set to (macMaxFrameRetries + 1) with macMaxFrameRetries = 3.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT
#define OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_DIRECT                4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_PER_POLL
 *
 * Maximum number of MAC layer transmit attempts for an outbound indirect frame (to a sleepy child) after receiving
 * a data request command (data poll) from the child.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_PER_POLL
#define OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_PER_POLL     1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_POLLS
 *
 * Maximum number of transmit attempts for an outbound indirect frame (for a sleepy child) each triggered by the
 * reception of a new data request command (a new data poll) from the sleepy child. Each data poll triggered attempt is
 * retried by the MAC layer up to `OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_PER_POLL` times.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_POLLS
#define OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_POLLS        4
#endif

/**
 * @def OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE
 *
 * Define as 1 for OpenThread to drop a message (and not send any remaining fragments of the message) if all transmit
 * attempts fail for a fragment of the message. For a direct transmission, a failure occurs after all MAC transmission
 * attempts for a given fragment are unsuccessful. For an indirect transmission, a failure occurs after all data poll
 * triggered transmission attempts for a given fragment fail.
 *
 * If set to zero (disabled), OpenThread will attempt to send subsequent fragments, whether or not all transmission
 * attempts fail for a given fragment.
 *
 */
#ifndef OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE
#define OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE   1
#endif

/**
 * @def OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD
 *
 * The Data Poll period during attach in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD
#define OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD               100
#endif

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES                 10
#endif

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_QUERY_TIMEOUT
 *
 * The timeout value (in seconds) waiting for a address notification response after sending an address query.
 *
 * Default: 3 seconds
 *
 */
#ifndef OPENTHREAD_CONFIG_ADDRESS_QUERY_TIMEOUT
#define OPENTHREAD_CONFIG_ADDRESS_QUERY_TIMEOUT                 3
#endif

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_QUERY_INITIAL_RETRY_DELAY
 *
 * Initial retry delay for address query (in seconds).
 *
 * Default: 15 seconds
 *
 */
#ifndef OPENTHREAD_CONFIG_ADDRESS_QUERY_INITIAL_RETRY_DELAY
#define OPENTHREAD_CONFIG_ADDRESS_QUERY_INITIAL_RETRY_DELAY     15
#endif

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_QUERY_MAX_RETRY_DELAY
 *
 * Maximum retry delay for address query (in seconds).
 *
 * Default: 28800 seconds (480 minutes or 8 hours)
 *
 */
#ifndef OPENTHREAD_CONFIG_ADDRESS_QUERY_MAX_RETRY_DELAY
#define OPENTHREAD_CONFIG_ADDRESS_QUERY_MAX_RETRY_DELAY         28800
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
 *
 *  The maximum size of the CLI line in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
#define OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH                   128
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
 *
 *  The size of CLI UART RX buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE               512
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 *  The size of CLI message buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE               1024
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MAX_CHILDREN                          10
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT
 *
 * The default child timeout value (in seconds).
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT
#define OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT                 240
#endif

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
 *
 * The maximum number of supported IPv6 address registrations per child.
 *
 */
#ifndef OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
#define OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD                    4
#endif

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_TO_REGISTER
 *
 * The maximum number of IPv6 address registrations for MTD.
 *
 */
#ifndef OPENTHREAD_CONFIG_IP_ADDRS_TO_REGISTER
#define OPENTHREAD_CONFIG_IP_ADDRS_TO_REGISTER                  (OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
 *
 * The maximum number of supported IPv6 addresses allows to be externally added.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
#define OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS                      4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS
 *
 * The maximum number of supported IPv6 multicast addresses allows to be externally added.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS
#define OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS            2
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_SERVER_ALOCS
 *
 * The maximum number of supported Service ALOCs registrations for this device.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_SERVER_ALOCS
#define OPENTHREAD_CONFIG_MAX_SERVER_ALOCS                      1
#endif

/**
 * @def OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
 *
 * The 6LoWPAN fragment reassembly timeout in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
#define OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT            5
#endif

/**
 * @def OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES
 *
 * The number of MPL Seed Set entries for duplicate detection.
 *
 */
#ifndef OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES
#define OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES                  32
#endif

/**
 * @def OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME
 *
 * The MPL Seed Set entry lifetime in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME
#define OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME           5
#endif

/**
 * @def OPENTHREAD_CONFIG_JOINER_UDP_PORT
 *
 * The default Joiner UDP port.
 *
 */
#ifndef OPENTHREAD_CONFIG_JOINER_UDP_PORT
#define OPENTHREAD_CONFIG_JOINER_UDP_PORT                       1000
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS
 *
 * The maximum number of Energy List entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS
#define OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS                    64
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
 *
 * The maximum number of Joiner entries maintained by the Commissioner.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
#define OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES                    2
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
 *
 * The maximum number of Joiner Router entries that can be queued by the Joiner.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_JOINER_ROUTER_ENTRIES
#define OPENTHREAD_CONFIG_MAX_JOINER_ROUTER_ENTRIES             2
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS              1
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT
 *
 * Minimum spacing before first retransmission when ACK is not received (RFC7252 default value is 2).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT
#define OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT                      2
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR
 *
 * Numerator of ACK_RANDOM_FACTOR used to calculate maximum spacing before first retransmission when
 * ACK is not received (RFC7252 default value of ACK_RANDOM_FACTOR is 1.5, must not be decreased below 1).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR
#define OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR      3
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR
 *
 * Denominator of ACK_RANDOM_FACTOR used to calculate maximum spacing before first retransmission when
 * ACK is not received (RFC7252 default value of ACK_RANDOM_FACTOR is 1.5, must not be decreased below 1).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR
#define OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR    2
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT
 *
 * Maximum number of retransmissions for CoAP Confirmable messages (RFC7252 default value is 4).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT
#define OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT                   4
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_SERVER_MAX_CACHED_RESPONSES
 *
 * Maximum number of cached responses for CoAP Confirmable messages.
 *
 * Cached responses are used for message deduplication.
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_SERVER_MAX_CACHED_RESPONSES
#define OPENTHREAD_CONFIG_COAP_SERVER_MAX_CACHED_RESPONSES      10
#endif

/**
 * @def OPENTHREAD_CONFIG_DNS_RESPONSE_TIMEOUT
 *
 * Maximum time that DNS Client waits for response in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_DNS_RESPONSE_TIMEOUT
#define OPENTHREAD_CONFIG_DNS_RESPONSE_TIMEOUT                  3000
#endif

/**
 * @def OPENTHREAD_CONFIG_DNS_MAX_RETRANSMIT
 *
 * Maximum number of retransmissions for DNS client.
 *
 */
#ifndef OPENTHREAD_CONFIG_DNS_MAX_RETRANSMIT
#define OPENTHREAD_CONFIG_DNS_MAX_RETRANSMIT                    2
#endif

/**
 * @def OPENTHREAD_CONFIG_JOIN_BEACON_VERSION
 *
 * The Beacon version to use when the beacon join flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_JOIN_BEACON_VERSION
#define OPENTHREAD_CONFIG_JOIN_BEACON_VERSION                   kProtocolVersion
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
 *
 * The message pool is managed by platform defined logic when this flag is set.
 * This feature would typically be used when operating in a multi-threaded system
 * and multiple threads need to access the message pool.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT           0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_FILTER_SIZE
 *
 * The number of MAC Filter entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_FILTER_SIZE
#define OPENTHREAD_CONFIG_MAC_FILTER_SIZE                       32
#endif

/**
 * @def OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
 *
 *  The value ahead of the current frame counter for persistent storage
 *
 */
#ifndef OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
#define OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD             1000
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * Selects if, and where the LOG output goes to.
 *
 * There are several options available
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_NONE
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART
 * - and others
 *
 * Note:
 *
 * 1) Because the default is: OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
 *    The platform is expected to provide at least a stub for `otPlatLog()`
 *
 * 2) This is effectively an ENUM so it can be if/else/endif at compile time.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_OUTPUT
#define OPENTHREAD_CONFIG_LOG_OUTPUT                            OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

/** Log output goes to the bit bucket (disabled) */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_NONE                       0
/** Log output goes to the debug uart - requires OPENTHREAD_CONFIG_ENABLE_DEBUG_UART to be enabled */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART                 1
/** Log output goes to the "application" provided otPlatLog() in NCP and CLI code */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_APP                        2
/** Log output is handled by a platform defined function */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED           3

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level (used at compile time).
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL                             OT_LOG_LEVEL_CRIT
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
 *
 * Define as 1 to enable dynamic log level control.
 *
 * Note that the OPENTHREAD_CONFIG_LOG_LEVEL determines the log level at
 * compile time. The dynamic log level control (if enabled) only allows
 * decreasing the log level from the compile time value.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
#define OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL              0
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_API
 *
 * Define to enable OpenThread API logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_API
#define OPENTHREAD_CONFIG_LOG_API                               1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_MLE
 *
 * Define to enable MLE logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_MLE
#define OPENTHREAD_CONFIG_LOG_MLE                               1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_ARP
 *
 * Define to enable EID-to-RLOC map logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_ARP
#define OPENTHREAD_CONFIG_LOG_ARP                               1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_NETDATA
 *
 * Define to enable Network Data logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_NETDATA
#define OPENTHREAD_CONFIG_LOG_NETDATA                           1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_ICMP
 *
 * Define to enable ICMPv6 logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_ICMP
#define OPENTHREAD_CONFIG_LOG_ICMP                              1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_IP6
 *
 * Define to enable IPv6 logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_IP6
#define OPENTHREAD_CONFIG_LOG_IP6                               1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_MAC
 *
 * Define to enable IEEE 802.15.4 MAC logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_MAC
#define OPENTHREAD_CONFIG_LOG_MAC                               1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_MEM
 *
 * Define to enable memory logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_MEM
#define OPENTHREAD_CONFIG_LOG_MEM                               1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PKT_DUMP
 *
 * Define to enable log content of packets.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PKT_DUMP
#define OPENTHREAD_CONFIG_LOG_PKT_DUMP                          1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_NETDIAG
 *
 * Define to enable network diagnostic logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_NETDIAG
#define OPENTHREAD_CONFIG_LOG_NETDIAG                           1
#endif

/**
* @def OPENTHREAD_CONFIG_LOG_PLATFORM
*
* Define to enable platform region logging.
*
*/
#ifndef OPENTHREAD_CONFIG_LOG_PLATFORM
#define OPENTHREAD_CONFIG_LOG_PLATFORM                          0
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_CLI
 *
 * Define to enable CLI logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_CLI
#define OPENTHREAD_CONFIG_LOG_CLI                               1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_COAP
 *
 * Define to enable COAP logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_COAP
#define OPENTHREAD_CONFIG_LOG_COAP                              1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL                     1
#endif

/**
* @def OPENTHREAD_CONFIG_LOG_PREPEND_REGION
*
* Define to prepend the log region to all log messages
*
*/
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_REGION
#define OPENTHREAD_CONFIG_LOG_PREPEND_REGION                    1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_SUFFIX
 *
 * Define suffix to append at the end of logs.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_SUFFIX
#define OPENTHREAD_CONFIG_LOG_SUFFIX                            ""
#endif

/**
 * @def OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION
 *
 * Defines the name of function/macro used for logging inside OpenThread, by default it is set to `otPlatLog()`.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION
#define OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION                     otPlatLog
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES
 *
 * The number of dhcp prefixes.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES
#define OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES                     4
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES
 *
 * The number of auto-configured SLAAC addresses.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES
#define OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES                   4
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
 *
 *  The size of NCP message buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE                    512
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE
 *
 *  The size of NCP UART TX chunk in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE
#define OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE                128
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE
 *
 *  The size of NCP UART RX buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE               1300
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE
 *
 *  The size of NCP SPI (RX/TX) buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE
#define OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE                   1300
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE
 *
 *  The size of extra data to be allocated in UART buffer,
 *  needed by NCP Spinel Encrypter.
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE
#define OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE  0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
 *
 * The assert is managed by platform defined logic when this flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT            0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
 *
 * Define to 1 if you want to enable software ACK timeout logic.
 *
 * Applicable only if raw link layer API is enabled (i.e., `OPENTHREAD_ENABLE_RAW_LINK_API` is set).
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
 * Applicable only if raw link layer API is enabled (i.e., `OPENTHREAD_ENABLE_RAW_LINK_API` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT            0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
 *
 * Define to 1 if you want to enable software energy scanning logic.
 *
 * Applicable only if raw link layer API is enabled (i.e., `OPENTHREAD_ENABLE_RAW_LINK_API` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN           0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
 *
 * Define to 1 if you want to enable microsecond backoff timer implemented in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
#define OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER            0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
 *
 * Define to 1 if you want to enable auto start logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
#define OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT             1
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_WHEN_JOINABLE
 *
 * Define to 1 to enable IEEE 802.15.4 Beacons when joining is enabled.
 *
 * @note When this feature is enabled, the device will transmit IEEE 802.15.4 Beacons in response to IEEE 802.15.4
 * Beacon Requests even while the device is not router capable and detached.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_WHEN_JOINABLE
#define OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_WHEN_JOINABLE       0
#endif

/**
 * @def OPENTHREAD_CONFIG_MBEDTLS_HEAP_SIZE
 *
 * The size of mbedTLS heap buffer when DTLS is enabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_MBEDTLS_HEAP_SIZE
#define OPENTHREAD_CONFIG_MBEDTLS_HEAP_SIZE                     (1536 * sizeof(void *))
#endif

/**
 * @def OPENTHREAD_CONFIG_MBEDTLS_HEAP_SIZE_NO_DTLS
 *
 * The size of mbedTLS heap buffer when DTLS is disabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_MBEDTLS_HEAP_SIZE_NO_DTLS
#define OPENTHREAD_CONFIG_MBEDTLS_HEAP_SIZE_NO_DTLS             384
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
 *
 * Enable setting steering data out of band.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
#define OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB          0
#endif

/**
 * @def OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW
 *
 * OpenThread's MAC implementation maintains the average failure rate of CCA (Clear Channel Assessment) operation on
 * frame transmissions. This value specifies the window (in terms of number of transmissions or samples) over which the
 * average rate is maintained. Practically, the average value can be considered as the percentage of CCA failures in
 * (approximately) last AVERAGING_WINDOW frame transmissions.
 *
 */
#ifndef OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW
#define OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW     512
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_INTERVAL
 *
 * The sample interval in milliseconds used by Channel Monitoring feature.

 * When enabled, a zero-duration Energy Scan is performed, collecting a single RSSI sample per channel during each
 * interval.
 *
 * Applicable only if Channel Monitoring feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MONITOR` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_INTERVAL
#define OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_INTERVAL       41000
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MONITOR_RSSI_THRESHOLD
 *
 * The RSSI threshold in dBm used by Channel Monitoring feature.
 *
 * The RSSI samples are compared with the given threshold. Channel monitoring reports the average rate of RSSI samples
 * that are above this threshold within an observation window (per channel).
 *
 * It is recommended that this value is set to same value as the CCA threshold used by radio.
 *
 * Applicable only if Channel Monitoring feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MONITOR` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MONITOR_RSSI_THRESHOLD
#define OPENTHREAD_CONFIG_CHANNEL_MONITOR_RSSI_THRESHOLD        -75
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_WINDOW
 *
 * The averaging sample window length (in units of channel sample interval) used by Channel Monitoring feature.
 *
 * Channel monitoring will sample all channels every sample interval. It maintains the average rate of RSSI samples
 * that are above the RSSI threshold within (approximately) this sample window.
 *
 * Applicable only if Channel Monitoring feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MONITOR` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_WINDOW
#define OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_WINDOW         960
#endif

/**
 * @def OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL
 *
 * The default supervision interval in seconds used by parent. Set to zero to disable the supervision process on the
 * parent.
 *
 * Applicable only if child supervision feature is enabled (i.e., `OPENTHREAD_ENABLE_CHILD_SUPERVISION ` is set).
 *
 * Child supervision feature provides a mechanism for parent to ensure that a message is sent to each sleepy child
 * within the supervision interval. If there is no transmission to the child within the supervision interval, child
 * supervisor will enqueue and send a supervision message (a data message with empty payload) to the child.
 *
 */
#ifndef OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL
#define OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL            129
#endif

/**
 * @def OPENTHREAD_CONFIG_SUPERVISION_CHECK_TIMEOUT
 *
 * The default supervision check timeout interval (in seconds) used by a device in child state. Set to zero to disable
 * the supervision check process on the child.
 *
 * Applicable only if child supervision feature is enabled (i.e., `OPENTHREAD_ENABLE_CHILD_SUPERVISION` is set).
 *
 * If the sleepy child does not hear from its parent within the specified timeout interval, it initiates the re-attach
 * process (MLE Child Update Request/Response exchange with its parent).
 *
 */
#ifndef OPENTHREAD_CONFIG_SUPERVISION_CHECK_TIMEOUT
#define OPENTHREAD_CONFIG_SUPERVISION_CHECK_TIMEOUT             190
#endif

/**
 * @def OPENTHREAD_CONFIG_SUPERVISION_MSG_NO_ACK_REQUEST
 *
 * Define as 1 to clear/disable 15.4 ack request in the MAC header of a supervision message.
 *
 * Applicable only if child supervision feature is enabled (i.e., `OPENTHREAD_ENABLE_CHILD_SUPERVISION` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_SUPERVISION_MSG_NO_ACK_REQUEST
#define OPENTHREAD_CONFIG_SUPERVISION_MSG_NO_ACK_REQUEST        0
#endif

/**
 * @def OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
 *
 * Define as 1 for a child to inform its previous parent when it attaches to a new parent.
 *
 * If this feature is enabled, when a device attaches to a new parent, it will send an IP message (with empty payload
 * and mesh-local IP address as the source address) to its previous parent.
 *
 */
#ifndef OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
#define OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH    0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
 *
 * Define as 1 to enable periodic parent search feature.
 *
 * When this feature is enabled an end-device/child (while staying attached) will periodically search for a possible
 * better parent and will switch parent if a better one is found.
 *
 * The child will periodically check the average RSS value for the current parent, and only if it is below a specific
 * threshold, a parent search is performed. The `OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL` specifies the the
 * check interval (in seconds) and `OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD` gives the RSS threshold.
 *
 * Since the parent search process can be power consuming (child needs to stays in RX mode to collect parent response)
 * and to limit its impact on battery-powered devices, after a parent search is triggered, the child will not trigger
 * another one before a specified backoff interval specified by `OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL`
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH
#define OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH         0
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL
 *
 * Specifies the interval in seconds for a child to check the trigger condition to perform a parent search.
 *
 * Applicable only if periodic parent search feature is enabled (see `OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH`).
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL
#define OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL          (9 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL
 *
 * Specifies the backoff interval in seconds for a child to not perform a parent search after triggering one.
 *
 * Applicable only if periodic parent search feature is enabled (see `OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH`).
 *
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL
#define OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL        (10* 60 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD
 *
 * Specifies the RSS threshold used to trigger a parent search.
 *
 * Applicable only if periodic parent search feature is enabled (see `OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH`).
 *
 */
#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD
#define OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD           -65
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
 *
 * Define as 1 to enable peek/poke functionality on NCP.
 *
 * Peek/Poke allows the host to read/write to memory addresses on NCP. This is intended for debugging.
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE
#define OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE                  0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE
 *
 * Size of NCP Spinel command response queue.
 *
 * NCP guarantees that it can respond up to `OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE` spinel commands at the
 * same time. The spinel protocol defines a Transaction ID (TID) as part of spinel command frame (the TID can be
 * a value 0-15 where TID 0 is used for frames which require no response). Spinel protocol can support at most support
 * 15 simultaneous commands.
 *
 * The host driver implementation may further limit the number of simultaneous Spinel command frames (e.g., wpantund
 * limits this to two). This configuration option can be used to reduce the response queue size.
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE
#define OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE        15
#endif

/**
 * @def OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
 *
 * Define as 1 to stay awake between fragments while transmitting a large packet,
 * and to stay awake after receiving a packet with frame pending set to true.
 *
 */
#ifndef OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
#define OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS          0
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT
 *
 * Define to 1 to send an MLE Link Request when MAX_NEIGHBOR_AGE is reached for a neighboring router.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT
#define OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT  0
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN
 *
 * Specifies the minimum link margin in dBm required before attempting to establish a link with a neighboring router.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN
#define OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN           10
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN
 *
 * Specifies the minimum link margin in dBm required before attempting to merge to a different partition.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN
#define OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN        10
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
#define OPENTHREAD_CONFIG_ENABLE_DEBUG_UART                     0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_DYNAMIC_MPL_INTERVAL
 *
 * Define as 1 to enable dynamic MPL interval feature.
 *
 * If this feature is enabled, the MPL forward interval will be adjusted dynamically according to
 * the network scale, which helps to reduce multicast latency.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_DYNAMIC_MPL_INTERVAL
#define OPENTHREAD_CONFIG_ENABLE_DYNAMIC_MPL_INTERVAL           0
#endif

/**
 * @def OPENTHREAD_CONFIG_DISABLE_CCA_ON_LAST_ATTEMPT
 *
 * Define as 1 to disable CCA on the last transmit attempt
 *
 */
#ifndef OPENTHREAD_CONFIG_DISABLE_CCA_ON_LAST_ATTEMPT
#define OPENTHREAD_CONFIG_DISABLE_CCA_ON_LAST_ATTEMPT           0
#endif

#endif  // OPENTHREAD_CORE_DEFAULT_CONFIG_H_
