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
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS                   40
#endif  // OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS

/**
 * @def OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
 *
 * The size of a message buffer in bytes.
 *
 */
#ifndef OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE                   128
#endif  // OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CHANNEL
 *
 * The default IEEE 802.15.4 channel.
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_CHANNEL
#define OPENTHREAD_CONFIG_DEFAULT_CHANNEL                       11
#endif  // OPENTHREAD_CONFIG_DEFAULT_CHANNEL

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER
 *
 * The default IEEE 802.15.4 maximum transmit power (dBm)
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER
#define OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER            0
#endif  // OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER

/**
 * @def OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD
 *
 * The Data Poll period during attach in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD
#define OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD               100
#endif  // OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES                 10
#endif  // OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES

/**
 * @def OPENTHREAD_CONFIG_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MAX_CHILDREN                          10
#endif  // OPENTHREAD_CONFIG_MAX_CHILDREN

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
 *
 * The minimum number of supported IPv6 address registrations per child.
 *
 */
#ifndef OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
#define OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD                    4
#endif  // OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
 *
 * The maximum number of supported IPv6 addresses allows to be externally added.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
#define OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS                      4
#endif  // OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS
 *
 * The maximum number of supported IPv6 multicast addresses allows to be externally added.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS
#define OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS            2
#endif  // OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS

/**
 * @def OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
 *
 * The 6LoWPAN fragment reassembly timeout in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
#define OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT            5
#endif  // OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT

/**
 * @def OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES
 *
 * The number of MPL Seed Set entries for duplicate detection.
 *
 */
#ifndef OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES
#define OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES                 32
#endif  // OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRIES

/**
 * @def OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME
 *
 * The MPL Seed Set entry lifetime in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME
#define OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME          5
#endif  // OPENTHREAD_CONFIG_MPL_SEED_SET_ENTRY_LIFETIME

/**
 * @def OPENTHREAD_CONFIG_JOINER_UDP_PORT
 *
 * The MPL cache entry lifetime in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_JOINER_UDP_PORT
#define OPENTHREAD_CONFIG_JOINER_UDP_PORT                       1000
#endif  // OPENTHREAD_CONFIG_JOINER_UDP_PORT

/**
 * @def OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS
 *
 * The maximum number of Energy List entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS
#define OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS                    64
#endif  // OPENTHREAD_CONFIG_MAX_ENERGY_RESULTS

/**
 * @def OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
 *
 * The maximum number of Joiner entries maintained by the Commissioner.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES
#define OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES                    2
#endif  // OPENTHREAD_CONFIG_MAX_JOINER_ENTRIES

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS              1
#endif  // OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT
 *
 * Minimum spacing before first retransmission when ACK is not received (RFC7252 default value is 2).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT
#define OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT                      2
#endif  // OPENTHREAD_CONFIG_COAP_ACK_TIMEOUT

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR
 *
 * Numerator of ACK_RANDOM_FACTOR used to calculate maximum spacing before first retransmission when
 * ACK is not received (RFC7252 default value of ACK_RANDOM_FACTOR is 1.5, must not be decreased below 1).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR
#define OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR      3
#endif  // OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_NUMERATOR

/**
 * @def OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR
 *
 * Denominator of ACK_RANDOM_FACTOR used to calculate maximum spacing before first retransmission when
 * ACK is not received (RFC7252 default value of ACK_RANDOM_FACTOR is 1.5, must not be decreased below 1).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR
#define OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR    2
#endif  // OPENTHREAD_CONFIG_COAP_ACK_RANDOM_FACTOR_DENOMINATOR

/**
 * @def OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT
 *
 * Maximum number of retransmissions for CoAP Confirmable messages (RFC7252 default value is 4).
 *
 */
#ifndef OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT
#define OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT                   4
#endif  // OPENTHREAD_CONFIG_COAP_MAX_RETRANSMIT

/**
 * @def OPENTHREAD_CONFIG_JOIN_BEACON_VERSION
 *
 * The Beacon version to use when the beacon join flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_JOIN_BEACON_VERSION
#define OPENTHREAD_CONFIG_JOIN_BEACON_VERSION                   kProtocolVersion
#endif  // OPENTHREAD_CONFIG_JOIN_BEACON_VERSION

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
#endif  // OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT

/**
 * @def OPENTHREAD_CONFIG_MAC_BLACKLIST_SIZE
 *
 * The number if MAC blacklist entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_BLACKLIST_SIZE
#define OPENTHREAD_CONFIG_MAC_BLACKLIST_SIZE                    32
#endif  // OPENTHREAD_CONFIG_MAC_BLACKLIST_SIZE

/**
 * @def OPENTHREAD_CONFIG_MAC_WHITELIST_SIZE
 *
 * The number if MAC whitelist entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_WHITELIST_SIZE
#define OPENTHREAD_CONFIG_MAC_WHITELIST_SIZE                    32
#endif  // OPENTHREAD_CONFIG_MAC_WHITELIST_SIZE

/**
 * @def OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
 *
 *  The value ahead of the current frame counter for persistent storage
 *
 */
#ifndef OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
#define OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD             1000
#endif  // OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL                             OPENTHREAD_LOG_LEVEL_CRIT
#endif  // OPENTHREAD_CONFIG_LOG_LEVEL

/**
 * @def OPENTHREAD_CONFIG_LOG_API
 *
 * Define to enable OpenThread API logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_API

/**
 * @def OPENTHREAD_CONFIG_LOG_MLE
 *
 * Define to enable MLE logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_MLE

/**
 * @def OPENTHREAD_CONFIG_LOG_ARP
 *
 * Define to enable EID-to-RLOC map logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_ARP

/**
 * @def OPENTHREAD_CONFIG_LOG_NETDATA
 *
 * Define to enable Network Data logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_NETDATA

/**
 * @def OPENTHREAD_CONFIG_LOG_ICMP
 *
 * Define to enable ICMPv6 logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_ICMP

/**
 * @def OPENTHREAD_CONFIG_LOG_IP6
 *
 * Define to enable IPv6 logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_IP6

/**
 * @def OPENTHREAD_CONFIG_LOG_MAC
 *
 * Define to enable IEEE 802.15.4 MAC logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_MAC

/**
 * @def OPENTHREAD_CONFIG_LOG_MEM
 *
 * Define to enable memory logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_MEM

/**
 * @def OPENTHREAD_CONFIG_LOG_NETDIAG
 *
 * Define to enable network diagnostic logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_NETDIAG

/**
* @def OPENTHREAD_CONFIG_LOG_PLATFORM
*
* Define to enable platform region logging.
*
*/
//#define OPENTHREAD_CONFIG_LOG_PLATFORM

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPREND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL

/**
* @def OPENTHREAD_CONFIG_LOG_PREPREND_REGION
*
* Define to prepend the log region to all log messages
*
*/
#define OPENTHREAD_CONFIG_LOG_PREPEND_REGION

/**
 * @def OPENTHREAD_CONFIG_LOG_SUFFIX
 *
 * Define suffix to append at the end of logs.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_SUFFIX
#define OPENTHREAD_CONFIG_LOG_SUFFIX                           ""
#endif  // OPENTHREAD_CONFIG_LOG_SUFFIX

/**
 * @def OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES
 *
 * The number of dhcp prefixes.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES
#define OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES                     4
#endif  // OPENTHREAD_CONFIG_NUM_DHCP_PREFIXES

/**
 * @def OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES
 *
 * The number of autoconfigured SLAAC addresses.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES
#define OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES                   4
#endif  // OPENTHREAD_CONFIG_NUM_SLAAC_ADDRESSES

/**
 * @def OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
 *
 *  The size of NCP message buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE                   512
#endif  // OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE

/**
 * @def OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE
 *
 *  The size of NCP UART TX chunk in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE
#define OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE               128
#endif  // OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE

/**
 * @def OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE
 *
 *  The size of NCP UART RX buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE              1500
#endif  // OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE

/**
 * @def OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE
 *
 *  The size of NCP SPI (RX/TX) buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE
#define OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE                  1500
#endif  // OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE

#endif  // OPENTHREAD_CORE_DEFAULT_CONFIG_H_
