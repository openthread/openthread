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
#define OPENTHREAD_CONFIG_STACK_VENDOR_OUI 0x18b430
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_REV
 *
 * The Stack Version Revision for the Thread stack.
 *
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_REV
#define OPENTHREAD_CONFIG_STACK_VERSION_REV 0
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_MAJOR
 *
 * The Stack Version Major for the Thread stack.
 *
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_MAJOR
#define OPENTHREAD_CONFIG_STACK_VERSION_MAJOR 0
#endif

/**
 * @def OPENTHREAD_CONFIG_STACK_VERSION_MINOR
 *
 * The Stack Version Minor for the Thread stack.
 *
 */
#ifndef OPENTHREAD_CONFIG_STACK_VERSION_MINOR
#define OPENTHREAD_CONFIG_STACK_VERSION_MINOR 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO "NONE"
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 44
#endif

/**
 * @def OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
 *
 * The size of a message buffer in bytes.
 *
 */
#ifndef OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE 128
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CHANNEL
 *
 * The default IEEE 802.15.4 channel.
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_CHANNEL
#define OPENTHREAD_CONFIG_DEFAULT_CHANNEL 11
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER
 *
 * The default IEEE 802.15.4 transmit power (dBm)
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER
#define OPENTHREAD_CONFIG_DEFAULT_TRANSMIT_POWER 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT
 *
 * The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure.
 *
 * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006, default value is 4.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT
#define OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT 32
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT
 *
 * The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure.
 *
 * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006, default value is 4.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT
#define OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT 4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_DIRECT
 *
 * The maximum number of retries allowed after a transmission failure for direct transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 3.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_DIRECT
#define OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_DIRECT 3
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_INDIRECT
 *
 * The maximum number of retries allowed after a transmission failure for indirect transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 0.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_INDIRECT
#define OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_INDIRECT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_POLLS
 *
 * Maximum number of received IEEE 802.15.4 Data Requests for a queued indirect transaction.
 *
 * The indirect frame remains in the transaction queue until it is successfully transmitted or until the indirect
 * transmission fails after the maximum number of IEEE 802.15.4 Data Request messages have been received.
 *
 * Takes the place of macTransactionPersistenceTime. The time period is specified in units of IEEE 802.15.4 Data
 * Request receptions, rather than being governed by macBeaconOrder.
 *
 * @sa OPENTHREAD_CONFIG_MAC_MAX_FRAME_RETRIES_INDIRECT
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_POLLS
#define OPENTHREAD_CONFIG_MAX_TX_ATTEMPTS_INDIRECT_POLLS 4
#endif

/**
 * @def OPENTHREAD_CONFIG_TX_NUM_BCAST
 *
 * The number of times each IEEE 802.15.4 broadcast frame is transmitted.
 *
 * The minimum value is 1. Values larger than 1 may improve broadcast reliability by increasing redundancy, but may also
 * increase congestion.
 *
 */
#ifndef OPENTHREAD_CONFIG_TX_NUM_BCAST
#define OPENTHREAD_CONFIG_TX_NUM_BCAST 1
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
#define OPENTHREAD_CONFIG_DROP_MESSAGE_ON_FRAGMENT_TX_FAILURE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD
 *
 * The Data Poll period during attach in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD
#define OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD 100
#endif

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES 10
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
#define OPENTHREAD_CONFIG_ADDRESS_QUERY_TIMEOUT 3
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
#define OPENTHREAD_CONFIG_ADDRESS_QUERY_INITIAL_RETRY_DELAY 15
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
#define OPENTHREAD_CONFIG_ADDRESS_QUERY_MAX_RETRY_DELAY 28800
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
 *
 *  The maximum size of the CLI line in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
#define OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH 128
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
 *
 *  The size of CLI UART RX buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE 512
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 *  The size of CLI message buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE 1024
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_ROUTERS
 *
 * The maximum number of routers in a Thread network.
 *
 * @note Thread specifies this value to be 32.  Changing this value may cause interoperability issues with standard
 *       Thread 1.1 devices.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_ROUTERS
#define OPENTHREAD_CONFIG_MAX_ROUTERS 32
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MAX_CHILDREN 10
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT
 *
 * The default child timeout value (in seconds).
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT
#define OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT 240
#endif

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
 *
 * The maximum number of supported IPv6 address registrations per child.
 *
 */
#ifndef OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
#define OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD 4
#endif

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_TO_REGISTER
 *
 * The maximum number of IPv6 address registrations for MTD.
 *
 */
#ifndef OPENTHREAD_CONFIG_IP_ADDRS_TO_REGISTER
#define OPENTHREAD_CONFIG_IP_ADDRS_TO_REGISTER (OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
 *
 * The maximum number of supported IPv6 addresses allows to be externally added.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
#define OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS 4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS
 *
 * The maximum number of supported IPv6 multicast addresses allows to be externally added.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS
#define OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS 2
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_SERVER_ALOCS
 *
 * The maximum number of supported Service ALOCs registrations for this device.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_SERVER_ALOCS
#define OPENTHREAD_CONFIG_MAX_SERVER_ALOCS 1
#endif

/**
 * @def OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
 *
 * The reassembly timeout between 6LoWPAN fragments in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
#define OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT 2
#endif

/**
 * @def OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES
 *
 * The number of MPL Seed Set entries for duplicate detection.
 *
 */
#ifndef OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES
#define OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES 32
#endif

/**
 * @def OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME
 *
 * The MPL Seed Set entry lifetime in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME
#define OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME 5
#endif

/**
 * @def OPENTHREAD_CONFIG_JOINER_UDP_PORT
 *
 * The default Joiner UDP port.
 *
 */
#ifndef OPENTHREAD_CONFIG_JOINER_UDP_PORT
#define OPENTHREAD_CONFIG_JOINER_UDP_PORT 1000
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS
 *
 * The maximum number of Energy List entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS
#define OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS 64
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
 *
 * The maximum number of Joiner entries maintained by the Commissioner.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
#define OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES 2
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
 *
 * The maximum number of Joiner Router entries that can be queued by the Joiner.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_JOINER_ROUTER_ENTRIES
#define OPENTHREAD_CONFIG_MAX_JOINER_ROUTER_ENTRIES 2
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS 1
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT
 *
 * Minimum spacing before first retransmission when ACK is not received (RFC7252 default value is 2).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT
#define OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT 2
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR
 *
 * Numerator of ACK_RANDOM_FACTOR used to calculate maximum spacing before first retransmission when
 * ACK is not received (RFC7252 default value of ACK_RANDOM_FACTOR is 1.5, must not be decreased below 1).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR
#define OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR 3
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR
 *
 * Denominator of ACK_RANDOM_FACTOR used to calculate maximum spacing before first retransmission when
 * ACK is not received (RFC7252 default value of ACK_RANDOM_FACTOR is 1.5, must not be decreased below 1).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR
#define OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR 2
#endif

/**
 * @def OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT
 *
 * Maximum number of retransmissions for CoAP Confirmable messages (RFC7252 default value is 4).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT
#define OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT 4
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
#define OPENTHREAD_CONFIG_COAP_SERVER_MAX_CACHED_RESPONSES 10
#endif

/**
 * @def OPENTHREAD_CONFIG_DNS_RESPONSE_TIMEOUT
 *
 * Maximum time that DNS Client waits for response in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_DNS_RESPONSE_TIMEOUT
#define OPENTHREAD_CONFIG_DNS_RESPONSE_TIMEOUT 3000
#endif

/**
 * @def OPENTHREAD_CONFIG_DNS_MAX_RETRANSMIT
 *
 * Maximum number of retransmissions for DNS client.
 *
 */
#ifndef OPENTHREAD_CONFIG_DNS_MAX_RETRANSMIT
#define OPENTHREAD_CONFIG_DNS_MAX_RETRANSMIT 2
#endif

/**
 * @def OPENTHREAD_CONFIG_SNTP_RESPONSE_TIMEOUT
 *
 * Maximum time that SNTP Client waits for response in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_SNTP_RESPONSE_TIMEOUT
#define OPENTHREAD_CONFIG_SNTP_RESPONSE_TIMEOUT 3000
#endif

/**
 * @def OPENTHREAD_CONFIG_SNTP_MAX_RETRANSMIT
 *
 * Maximum number of retransmissions for SNTP client.
 *
 */
#ifndef OPENTHREAD_CONFIG_SNTP_MAX_RETRANSMIT
#define OPENTHREAD_CONFIG_SNTP_MAX_RETRANSMIT 2
#endif

/**
 * @def OPENTHREAD_CONFIG_JOIN_BEACON_VERSION
 *
 * The Beacon version to use when the beacon join flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_JOIN_BEACON_VERSION
#define OPENTHREAD_CONFIG_JOIN_BEACON_VERSION kProtocolVersion
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
#define OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_FILTER_SIZE
 *
 * The number of MAC Filter entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_FILTER_SIZE
#define OPENTHREAD_CONFIG_MAC_FILTER_SIZE 32
#endif

/**
 * @def OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
 *
 *  The value ahead of the current frame counter for persistent storage
 *
 */
#ifndef OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
#define OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD 1000
#endif

/**
 * @def OPENTHREAD_CONFIG_MESHCOP_PENDING_DATASET_MINIMUM_DELAY
 *
 * Minimum Delay Timer value for a Pending Operational Dataset (in ms).
 *
 * Thread specification defines this value as 30,000 ms. Changing from the specified value should be done for testing
 * only.
 *
 */
#ifndef OPENTHREAD_CONFIG_MESHCOP_PENDING_DATASET_MINIMUM_DELAY
#define OPENTHREAD_CONFIG_MESHCOP_PENDING_DATASET_MINIMUM_DELAY 30000
#endif

/**
 * @def OPENTHREAD_CONFIG_MESHCOP_PENDING_DATASET_DEFAULT_DELAY
 *
 * Default Delay Timer value for a Pending Operational Dataset (in ms).
 *
 * Thread specification defines this value as 300,000 ms. Changing from the specified value should be done for testing
 * only.
 *
 */
#ifndef OPENTHREAD_CONFIG_MESHCOP_PENDING_DATASET_DEFAULT_DELAY
#define OPENTHREAD_CONFIG_MESHCOP_PENDING_DATASET_DEFAULT_DELAY 300000
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * Selects if, and where the LOG output goes to.
 *
 * There are several options available
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_NONE
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_APP
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
 * - @sa OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL
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
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

/** Log output goes to the bit bucket (disabled) */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_NONE 0
/** Log output goes to the debug uart - requires OPENTHREAD_CONFIG_ENABLE_DEBUG_UART to be enabled */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_DEBUG_UART 1
/** Log output goes to the "application" provided otPlatLog() in NCP and CLI code */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_APP 2
/** Log output is handled by a platform defined function */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED 3
/** Log output for NCP goes to Spinel `STREAM_LOG` property (for CLI platform defined function is expected) */
#define OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL 4

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level (used at compile time). If `OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL`
 * is set, this defines the most verbose log level possible. See
 *`OPENTHREAD_CONFIG_INITIAL_LOG_LEVEL` to set the initial log level.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_CRIT
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
#define OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL 0
#endif

/**
 * @def OPENTHREAD_CONFIG_INITIAL_LOG_LEVEL
 *
 * The initial log level used when OpenThread is initialized. See
 * `OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL`.
 */
#ifndef OPENTHREAD_CONFIG_INITIAL_LOG_LEVEL
#define OPENTHREAD_CONFIG_INITIAL_LOG_LEVEL OPENTHREAD_CONFIG_LOG_LEVEL
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_API
 *
 * Define to enable OpenThread API logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_API
#define OPENTHREAD_CONFIG_LOG_API 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_MLE
 *
 * Define to enable MLE logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_MLE
#define OPENTHREAD_CONFIG_LOG_MLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_ARP
 *
 * Define to enable EID-to-RLOC map logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_ARP
#define OPENTHREAD_CONFIG_LOG_ARP 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_NETDATA
 *
 * Define to enable Network Data logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_NETDATA
#define OPENTHREAD_CONFIG_LOG_NETDATA 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_ICMP
 *
 * Define to enable ICMPv6 logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_ICMP
#define OPENTHREAD_CONFIG_LOG_ICMP 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_IP6
 *
 * Define to enable IPv6 logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_IP6
#define OPENTHREAD_CONFIG_LOG_IP6 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_MAC
 *
 * Define to enable IEEE 802.15.4 MAC logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_MAC
#define OPENTHREAD_CONFIG_LOG_MAC 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_MEM
 *
 * Define to enable memory logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_MEM
#define OPENTHREAD_CONFIG_LOG_MEM 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PKT_DUMP
 *
 * Define to enable log content of packets.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PKT_DUMP
#define OPENTHREAD_CONFIG_LOG_PKT_DUMP 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_NETDIAG
 *
 * Define to enable network diagnostic logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_NETDIAG
#define OPENTHREAD_CONFIG_LOG_NETDIAG 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PLATFORM
 *
 * Define to enable platform region logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PLATFORM
#define OPENTHREAD_CONFIG_LOG_PLATFORM 0
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_CLI
 *
 * Define to enable CLI logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_CLI
#define OPENTHREAD_CONFIG_LOG_CLI 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_COAP
 *
 * Define to enable COAP logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_COAP
#define OPENTHREAD_CONFIG_LOG_COAP 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_CORE
 *
 * Define to enable OpenThread Core logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_CORE
#define OPENTHREAD_CONFIG_LOG_CORE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_UTIL
 *
 * Define to enable OpenThread Utility module logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_UTIL
#define OPENTHREAD_CONFIG_LOG_UTIL 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_REGION
 *
 * Define to prepend the log region to all log messages
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_REGION
#define OPENTHREAD_CONFIG_LOG_PREPEND_REGION 1
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_SUFFIX
 *
 * Define suffix to append at the end of logs.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_SUFFIX
#define OPENTHREAD_CONFIG_LOG_SUFFIX ""
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES
 *
 * If defined as 1 when IPv6 message info is logged in mesh-forwarder, the source and destination IPv6 addresses of
 * messages are also included.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES
#define OPENTHREAD_CONFIG_LOG_SRC_DST_IP_ADDRESSES 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION
 *
 * Defines the name of function/macro used for logging inside OpenThread, by default it is set to `otPlatLog()`.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION
#define OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION otPlatLog
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES
 *
 * The number of dhcp prefixes.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES
#define OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES 4
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SLAAC
 *
 * Define as 1 to enable support for adding of auto-configured SLAAC addresses by OpenThread.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_SLAAC
#define OPENTHREAD_CONFIG_ENABLE_SLAAC 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES
 *
 * The number of auto-configured SLAAC addresses. Applicable only if OPENTHREAD_CONFIG_ENABLE_SLAAC is enabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES
#define OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES 4
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
 *
 *  The size of NCP message buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE 512
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE
 *
 *  The size of NCP UART TX chunk in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE
#define OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE 128
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE
 *
 *  The size of NCP UART RX buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE
#if OPENTHREAD_RADIO
#define OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE 512
#else
#define OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE 1300
#endif
#endif // OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE

/**
 * @def OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE
 *
 *  The size of NCP SPI (RX/TX) buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE
#if OPENTHREAD_RADIO
#define OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE 512
#else
#define OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE 1300
#endif
#endif // OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE

/**
 * @def OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE
 *
 *  The size of extra data to be allocated in UART buffer,
 *  needed by NCP Spinel Encrypter.
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE
#define OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE
 *
 * The maximum OpenThread log string size (number of chars) supported by NCP using Spinel `StreamWrite`.
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE
#define OPENTHREAD_CONFIG_NCP_SPINEL_LOG_MAX_SIZE 150
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
 *
 * The assert is managed by platform defined logic when this flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT 0
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
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT 0
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
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
 *
 * Define to 1 if you want to enable software CSMA-CA backoff logic.
 *
 * Applicable only if raw link layer API is enabled (i.e., `OPENTHREAD_ENABLE_RAW_LINK_API` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF 0
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
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ENERGY_SCAN 0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
 *
 * Define to 1 if you want to enable microsecond backoff timer implemented in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
#define OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER 0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE
 *
 * Allows to define custom otPlatRadioGetIeeeEui64 function to retrieve EUI-64.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE
#define OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
 *
 * Define to 1 if you want to enable auto start logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT
#define OPENTHREAD_CONFIG_ENABLE_AUTO_START_SUPPORT 1
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
#define OPENTHREAD_CONFIG_ENABLE_BEACON_RSP_WHEN_JOINABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_SIZE
 *
 * The size of heap buffer when DTLS is enabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_HEAP_SIZE
#if OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#define OPENTHREAD_CONFIG_HEAP_SIZE (3072 * sizeof(void *))
#else
#define OPENTHREAD_CONFIG_HEAP_SIZE (1536 * sizeof(void *))
#endif // OPENTHREAD_ENABLE_APPLICATION_COAP_SECURE
#endif // OPENTHREAD_CONFIG_HEAP_SIZE

/**
 * @def OPENTHREAD_CONFIG_HEAP_SIZE_NO_DTLS
 *
 * The size of heap buffer when DTLS is disabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_HEAP_SIZE_NO_DTLS
#define OPENTHREAD_CONFIG_HEAP_SIZE_NO_DTLS 384
#endif

/**
 * @def OPENTHREAD_CONFIG_DTLS_APPLICATION_DATA_MAX_LENGTH
 *
 * The size of dtls application data when the CoAP Secure API is enabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_DTLS_APPLICATION_DATA_MAX_LENGTH
#define OPENTHREAD_CONFIG_DTLS_APPLICATION_DATA_MAX_LENGTH 1400
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
 *
 * Enable setting steering data out of band.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
#define OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB 0
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
#define OPENTHREAD_CONFIG_CCA_FAILURE_RATE_AVERAGING_WINDOW 512
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING
 *
 * Define as 1 to enable transmission error rate tracking (for both MAC frames and IPv6 messages)
 *
 * When enabled, OpenThread will track average error rate of MAC frame transmissions and IPv6 message error rate for
 * every neighbor.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING
#define OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING 1
#endif

/**
 * @def OPENTHREAD_CONFIG_FRAME_TX_ERR_RATE_AVERAGING_WINDOW
 *
 * Applicable only if error rate tracking is enabled (i.e., `OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING` is set).
 *
 * OpenThread's MAC implementation maintains the average error rate of MAC frame transmissions per neighbor. This
 * parameter specifies the window (in terms of number of frames/sample) over which the average error rate is maintained.
 * Practically, the average value can be considered as the percentage of failed (no ack) MAC frame transmissions  over
 * (approximately) last AVERAGING_WINDOW frame transmission attempts to a specific neighbor.
 *
 */
#ifndef OPENTHREAD_CONFIG_FRAME_TX_ERR_RATE_AVERAGING_WINDOW
#define OPENTHREAD_CONFIG_FRAME_TX_ERR_RATE_AVERAGING_WINDOW 128
#endif

/**
 * @def OPENTHREAD_CONFIG_IPV6_TX_ERR_RATE_AVERAGING_WINDOW
 *
 * Applicable only if error rate tracking is enabled (i.e., `OPENTHREAD_CONFIG_ENABLE_TX_ERROR_RATE_TRACKING` is set).
 *
 * OpenThread maintains the average error rate of IPv6 messages per neighbor. This parameter specifies the
 * window (in terms of number of messages) over which the average error rate is maintained. Practically, the average
 * value can be considered as the percentage of failed (no ack) messages over (approximately) last AVERAGING_WINDOW
 * IPv6 messages sent to a specific neighbor.
 *
 */
#ifndef OPENTHREAD_CONFIG_IPV6_TX_ERR_RATE_AVERAGING_WINDOW
#define OPENTHREAD_CONFIG_IPV6_TX_ERR_RATE_AVERAGING_WINDOW 128
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
#define OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_INTERVAL 41000
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
#define OPENTHREAD_CONFIG_CHANNEL_MONITOR_RSSI_THRESHOLD -75
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
#define OPENTHREAD_CONFIG_CHANNEL_MONITOR_SAMPLE_WINDOW 960
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_DELAY
 *
 * The minimum delay (in seconds) used by Channel Manager module for performing a channel change.
 *
 * The minimum delay should preferably be longer than maximum data poll interval used by all sleepy-end-devices within
 * the Thread network.
 *
 * Applicable only if Channel Manager feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MANAGER` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_DELAY
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_DELAY 120
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_MONITOR_SAMPLE_COUNT
 *
 * The minimum number of RSSI samples per channel by Channel Monitoring feature before the collected data can be used
 * by the Channel Manager module to (auto) select a better channel.
 *
 * Applicable only if Channel Manager and Channel Monitoring features are both enabled (i.e.,
 * `OPENTHREAD_ENABLE_CHANNEL_MANAGER` and `OPENTHREAD_ENABLE_CHANNEL_MONITOR` are set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_MONITOR_SAMPLE_COUNT
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_MONITOR_SAMPLE_COUNT 500
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_SKIP_FAVORED
 *
 * This threshold specifies the minimum occupancy rate difference between two channels for the Channel Manager to
 * prefer an unfavored channel over the best favored one. This is used when (auto) selecting a channel based on the
 * collected channel quality data by "channel monitor" feature.
 *
 * The difference is based on the `ChannelMonitor::GetChannelOccupancy()` definition which provides the average
 * percentage of RSSI samples (within a time window) indicating that channel was busy (i.e., RSSI value higher than
 * a threshold). Value 0 maps to 0% and 0xffff maps to 100%.
 *
 * Applicable only if Channel Manager feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MANAGER` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_SKIP_FAVORED
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_SKIP_FAVORED (0xffff * 7 / 100)
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_CHANGE_CHANNEL
 *
 * This threshold specifies the minimum occupancy rate difference required between the current channel and a newly
 * selected channel for Channel Manager to allow channel change to the new channel.
 *
 * The difference is based on the `ChannelMonitor::GetChannelOccupancy()` definition which provides the average
 * percentage of RSSI samples (within a time window) indicating that channel was busy (i.e., RSSI value higher than
 * a threshold). Value 0 maps to 0% rate and 0xffff maps to 100%.
 *
 * Applicable only if Channel Manager feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MANAGER` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_CHANGE_CHANNEL
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_CHANGE_CHANNEL (0xffff * 10 / 100)
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MANAGER_DEFAULT_AUTO_SELECT_INTERVAL
 *
 * The default time interval (in seconds) used by Channel Manager for auto-channel-selection functionality.
 *
 * Applicable only if Channel Manager feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MANAGER` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MANAGER_DEFAULT_AUTO_SELECT_INTERVAL
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_DEFAULT_AUTO_SELECT_INTERVAL (3 * 60 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MANAGER_CCA_FAILURE_THRESHOLD
 *
 * Minimum CCA failure rate threshold on current channel before Channel Manager starts channel selection attempt.
 *
 * Value 0 maps to 0% and 0xffff maps to 100%.
 *
 * Applicable only if Channel Manager feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MANAGER` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_CHANNEL_MANAGER_CCA_FAILURE_THRESHOLD
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_CCA_FAILURE_THRESHOLD (0xffff * 14 / 100)
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
#define OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL 129
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
#define OPENTHREAD_CONFIG_SUPERVISION_CHECK_TIMEOUT 190
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
#define OPENTHREAD_CONFIG_SUPERVISION_MSG_NO_ACK_REQUEST 0
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
#define OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH 0
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
#define OPENTHREAD_CONFIG_ENABLE_PERIODIC_PARENT_SEARCH 0
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
#define OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL (9 * 60)
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
#define OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL (10 * 60 * 60)
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
#define OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD -65
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF
 *
 * Define as 1 to enable attach backoff feature
 *
 * When this feature is enabled, an exponentially increasing backoff wait time is added between attach attempts.
 * If device is sleepy, the radio will be put to sleep during the wait time. This ensures that a battery-powered sleepy
 * end-device does not drain its battery by continuously searching for a parent to attach to (when there is no
 * router/parent for it to attach).
 *
 * The backoff time starts from a minimum interval specified by `OPENTHREAD_CONFIG_ATTACH_BACKOFF_MINIMUM_INTERVAL`,
 * and every attach attempt the wait time is doubled up to `OPENTHREAD_CONFIG_ATTACH_BACKOFF_MAXIMUM_INTERVAL` which
 * specifies the maximum wait time.
 *
 * Once the wait time reaches the maximum, a random jitter interval is added to it. The maximum value for jitter is
 * specified by `OPENTHREAD_CONFIG_ATTACH_BACKOFF_JITTER_INTERVAL`. The random jitter is selected uniformly within
 * range `[-JITTER, +JITTER]`. It is only added when the backoff wait interval is at maximum value.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF
#define OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF 1
#endif

/**
 * @def OPENTHREAD_CONFIG_ATTACH_BACKOFF_MINIMUM_INTERVAL
 *
 * Specifies the minimum backoff wait interval (in milliseconds) used by attach backoff feature.
 *
 * Applicable only if attach backoff feature is enabled (see `OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF`).
 *
 * Please see `OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF` description for more details.
 *
 */
#ifndef OPENTHREAD_CONFIG_ATTACH_BACKOFF_MINIMUM_INTERVAL
#define OPENTHREAD_CONFIG_ATTACH_BACKOFF_MINIMUM_INTERVAL 251
#endif

/**
 * @def OPENTHREAD_CONFIG_ATTACH_BACKOFF_MAXIMUM_INTERVAL
 *
 * Specifies the maximum backoff wait interval (in milliseconds) used by attach backoff feature.
 *
 * Applicable only if attach backoff feature is enabled (see `OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF`).
 *
 * Please see `OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF` description for more details.
 *
 */
#ifndef OPENTHREAD_CONFIG_ATTACH_BACKOFF_MAXIMUM_INTERVAL
#define OPENTHREAD_CONFIG_ATTACH_BACKOFF_MAXIMUM_INTERVAL 1200000 // 1200 seconds = 20 minutes
#endif

/**
 * @def OPENTHREAD_CONFIG_ATTACH_BACKOFF_JITTER_INTERVAL
 *
 * Specifies the maximum jitter interval (in milliseconds) used by attach backoff feature.
 *
 * Applicable only if attach backoff feature is enabled (see `OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF`).
 *
 * Please see `OPENTHREAD_CONFIG_ENABLE_ATTACH_BACKOFF` description for more details.
 *
 */
#ifndef OPENTHREAD_CONFIG_ATTACH_BACKOFF_JITTER_INTERVAL
#define OPENTHREAD_CONFIG_ATTACH_BACKOFF_JITTER_INTERVAL 2000
#endif

/**
 * @def OPENTHREAD_CONFIG_SEND_UNICAST_ANNOUNCE_RESPONSE
 *
 * Define as 1 to enable sending of a unicast MLE Announce message in response to a received Announce message from
 * a device.
 *
 * @note The unicast MLE announce message is sent in addition to (and after) the multicast MLE Announce.
 *
 */
#ifndef OPENTHREAD_CONFIG_SEND_UNICAST_ANNOUNCE_RESPONSE
#define OPENTHREAD_CONFIG_SEND_UNICAST_ANNOUNCE_RESPONSE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER
 *
 * Define as 1 to enable `AnnounceSender` which will periodically send MLE Announce message on all channels.
 *
 * The list of channels is determined from the Operational Dataset's ChannelMask. The period intervals are determined
 * by `OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_ROUTER` and `OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_REED`
 * configuration options.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER
#define OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER 0
#endif

/**
 * @def OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_ROUTER
 *
 * Specifies the time interval (in milliseconds) between `AnnounceSender` transmit cycles on a device in Router role.
 *
 * In a cycle, the `AnnounceSender` sends MLE Announcement on all channels in Active Operational Dataset's ChannelMask.
 * The transmissions on different channels happen uniformly over the given interval (i.e., if there are 16 channels,
 * there will be 16 MLE Announcement messages each on one channel with `interval / 16`  between two consecutive MLE
 * Announcement transmissions).
 *
 * Applicable only if `AnnounceSender` feature is enabled (see `OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER`).
 *
 */
#ifndef OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_ROUTER
#define OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_ROUTER 688000 // 668 seconds = 11 min and 28 sec.
#endif

/**
 * @def OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_REED
 *
 * Specifies the time interval (in milliseconds) between `AnnounceSender` transmit cycles on a device in REED role.
 *
 * This is similar to `OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_ROUTER` but used when device is in REED role.
 *
 * Applicable only if `AnnounceSender` feature is enabled (see `OPENTHREAD_CONFIG_ENABLE_ANNOUNCE_SENDER`).
 *
 */
#ifndef OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_REED
#define OPENTHREAD_CONFIG_ANNOUNCE_SENDER_INTERVAL_REED (668000 * 3)
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
#define OPENTHREAD_CONFIG_NCP_ENABLE_PEEK_POKE 0
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
#define OPENTHREAD_CONFIG_NCP_SPINEL_RESPONSE_QUEUE_SIZE 15
#endif

/**
 * @def OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL
 *
 * Define to 1 to enable support controlling of desired power state of NCP's micro-controller.
 *
 * The power state specifies the desired power state of NCP's micro-controller (MCU) when the underlying platform's
 * operating system enters idle mode (i.e., all active tasks/events are processed and the MCU can potentially enter a
 * energy-saving power state).
 *
 * The power state primarily determines how the host should interact with the NCP and whether the host needs an
 * external trigger (a "poke") before it can communicate with the NCP or not.
 *
 * When enabled, the platform is expected to provide `otPlatSetMcuPowerState()` and `otPlatGetMcuPowerState()`
 * functions (please see `openthread/platform/misc.h`). Host can then control the power state using
 * `SPINEL_PROP_MCU_POWER_STATE`.
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL
#define OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL 0
#endif

/**
 * @def OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
 *
 * Define as 1 to stay awake between fragments while transmitting a large packet,
 * and to stay awake after receiving a packet with frame pending set to true.
 *
 */
#ifndef OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS
#define OPENTHREAD_CONFIG_STAY_AWAKE_BETWEEN_FRAGMENTS 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT
 *
 * Define to 1 to send an MLE Link Request when MAX_NEIGHBOR_AGE is reached for a neighboring router.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT
#define OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN
 *
 * Specifies the minimum link margin in dBm required before attempting to establish a link with a neighboring router.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN
#define OPENTHREAD_CONFIG_MLE_LINK_REQUEST_MARGIN_MIN 10
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN
 *
 * Specifies the minimum link margin in dBm required before attempting to merge to a different partition.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN
#define OPENTHREAD_CONFIG_MLE_PARTITION_MERGE_MARGIN_MIN 10
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS
 *
 * Specifies the desired number of router links that a REED / FED attempts to maintain.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS
#define OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS 3
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
 * @def OPENTHREAD_CONFIG_ENABLE_DYNAMIC_MPL_INTERVAL
 *
 * Define as 1 to enable dynamic MPL interval feature.
 *
 * If this feature is enabled, the MPL forward interval will be adjusted dynamically according to
 * the network scale, which helps to reduce multicast latency.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_DYNAMIC_MPL_INTERVAL
#define OPENTHREAD_CONFIG_ENABLE_DYNAMIC_MPL_INTERVAL 0
#endif

/**
 * @def OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
 *
 * Define as 1 to disable CSMA-CA on the last transmit attempt
 *
 */
#ifndef OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
#define OPENTHREAD_CONFIG_DISABLE_CSMA_CA_ON_LAST_ATTEMPT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode output buffer size in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
#define OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE 256
#endif

/**
 * @def OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX
 *
 * Define OpenThread diagnostic mode max command line arguments.
 *
 */
#ifndef OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX
#define OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX 32
#endif

/**
 * @def OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode command line buffer size in bytes.
 *
 */
#ifndef OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE 256
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
 *
 * Define as 1 to enable the time synchronization service feature.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
#define OPENTHREAD_CONFIG_ENABLE_TIME_SYNC 0
#endif

/**
 * @def OPENTHREAD_CONFIG_TIME_SYNC_REQUIRED
 *
 * Define as 1 to require time synchronization when attaching to a network. If the device is router capable
 * and cannot find a neighboring router supporting time synchronization, the device will form a new partition.
 * If the device is not router capable, the device will remain an orphan.
 *
 * Applicable only if time synchronization service feature is enabled (i.e., OPENTHREAD_CONFIG_ENABLE_TIME_SYNC is set)
 *
 */
#ifndef OPENTHREAD_CONFIG_TIME_SYNC_REQUIRED
#define OPENTHREAD_CONFIG_TIME_SYNC_REQUIRED 0
#endif

/**
 * @def OPENTHREAD_CONFIG_TIME_SYNC_PERIOD
 *
 * Specifies the default period of time synchronization, in seconds.
 *
 * Applicable only if time synchronization service feature is enabled (i.e., OPENTHREAD_CONFIG_ENABLE_TIME_SYNC is set)
 *
 */
#ifndef OPENTHREAD_CONFIG_TIME_SYNC_PERIOD
#define OPENTHREAD_CONFIG_TIME_SYNC_PERIOD 30
#endif

/**
 * @def OPENTHREAD_CONFIG_TIME_SYNC_XTAL_THRESHOLD
 *
 * Specifies the default XTAL threshold for a device to become Router in time synchronization enabled network, in PPM.
 *
 * Applicable only if time synchronization service feature is enabled (i.e., OPENTHREAD_CONFIG_ENABLE_TIME_SYNC is set)
 *
 */
#ifndef OPENTHREAD_CONFIG_TIME_SYNC_XTAL_THRESHOLD
#define OPENTHREAD_CONFIG_TIME_SYNC_XTAL_THRESHOLD 300
#endif

/**
 * @def OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
 *
 * Define as 1 to support IEEE 802.15.4-2015 Header IE (Information Element) generation and parsing, it must be set
 * to support following features:
 *    1. Time synchronization service feature (i.e., OPENTHREAD_CONFIG_ENABLE_TIME_SYNC is set).
 *
 * @note If it's enabled, platform must support interrupt context and concurrent access AES.
 *
 */
#ifndef OPENTHREAD_CONFIG_HEADER_IE_SUPPORT
#if OPENTHREAD_CONFIG_ENABLE_TIME_SYNC
#define OPENTHREAD_CONFIG_HEADER_IE_SUPPORT 1
#else
#define OPENTHREAD_CONFIG_HEADER_IE_SUPPORT 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_LONG_ROUTES
 *
 * Enable experimental mode for 'deep' networks, allowing packet routes up to 32 nodes.
 * This mode is incompatible with Thread 1.1.1 and older.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_LONG_ROUTES
#define OPENTHREAD_CONFIG_ENABLE_LONG_ROUTES 0
#endif

/**
 * @def OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH
 *
 * The settings storage path on posix platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH
#define OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH "tmp"
#endif

/**
 * @def OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS
 *
 * This setting configures the number of consecutive MCPS.DATA-Confirms having Status NO_ACK
 * that cause a Child-to-Parent link to be considered broken.
 *
 */
#ifndef OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS
#define OPENTHREAD_CONFIG_FAILED_CHILD_TRANSMISSIONS 4
#endif

/**
 * @def OPENTHREAD_CONFIG_MINIMUM_POLL_PERIOD
 *
 * This setting configures the minimum poll period in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_MINIMUM_POLL_PERIOD
#define OPENTHREAD_CONFIG_MINIMUM_POLL_PERIOD 10
#endif

/**
 * @def OPENTHREAD_CONFIG_RETX_POLL_PERIOD
 *
 * This setting configures the retx poll period in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_RETX_POLL_PERIOD
#define OPENTHREAD_CONFIG_RETX_POLL_PERIOD 1000
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE
 *
 * This setting configures the default buffer size for IPv6 datagram destined for an attached SED.
 * A Thread Router MUST be able to buffer at least one 1280-octet IPv6 datagram for an attached SED according to
 * the Thread Conformance Specification.
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE
#define OPENTHREAD_CONFIG_DEFAULT_SED_BUFFER_SIZE 1280
#endif

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT
 *
 * This setting configures the default datagram count of 106-octet IPv6 datagram per attached SED.
 * A Thread Router MUST be able to buffer at least one 106-octet IPv6 datagram per attached SED according to
 * the Thread Conformance Specification.
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT
#define OPENTHREAD_CONFIG_DEFAULT_SED_DATAGRAM_COUNT 1
#endif

/**
 * @def OPENTHREAD_CONFIG_IPV6_DEFAULT_HOP_LIMIT
 *
 * This setting configures the default hop limit of IPv6.
 *
 */
#ifndef OPENTHREAD_CONFIG_IPV6_DEFAULT_HOP_LIMIT
#define OPENTHREAD_CONFIG_IPV6_DEFAULT_HOP_LIMIT 64
#endif

/**
 * @def OPENTHREAD_CONFIG_IPV6_DEFAULT_MAX_DATAGRAM
 *
 * This setting configures the max datagram length of IPv6.
 *
 */
#ifndef OPENTHREAD_CONFIG_IPV6_DEFAULT_MAX_DATAGRAM
#define OPENTHREAD_CONFIG_IPV6_DEFAULT_MAX_DATAGRAM 1280
#endif

/**
 * @def OPENTHREAD_CONFIG_TIME_SYNC_JUMP_NOTIF_MIN_US
 *
 * This setting sets the minimum amount of time (in microseconds) that the network time must jump due to
 * a time sync event for listeners to be notified of the new network time.
 *
 */
#ifndef OPENTHREAD_CONFIG_TIME_SYNC_JUMP_NOTIF_MIN_US
#define OPENTHREAD_CONFIG_TIME_SYNC_JUMP_NOTIF_MIN_US 10000
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES
 *
 * The number of fragment priority entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES
#define OPENTHREAD_CONFIG_NUM_FRAGMENT_PRIORITY_ENTRIES 8
#endif

#endif // OPENTHREAD_CORE_DEFAULT_CONFIG_H_
