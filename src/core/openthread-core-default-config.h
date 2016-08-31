/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS               48
#endif  // OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS

/**
 * @def OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
 *
 * The size of a message buffer in bytes.
 *
 */
#ifndef OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE               128
#endif  // OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CHANNEL
 *
 * The default IEEE 802.15.4 channel.
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_CHANNEL
#define OPENTHREAD_CONFIG_DEFAULT_CHANNEL                   11
#endif  // OPENTHREAD_CONFIG_DEFAULT_CHANNEL

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER
 *
 * The default IEEE 802.15.4 maximum transmit power (dBm)
 *
 */
#ifndef OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER
#define OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER        0
#endif  // OPENTHREAD_CONFIG_DEFAULT_MAX_TRANSMIT_POWER

/**
 * @def OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD
 *
 * The Data Poll period during attach in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD
#define OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD           100
#endif  // OPENTHREAD_CONFIG_ATTACH_DATA_POLL_PERIOD

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES             8
#endif  // OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES

/**
 * @def OPENTHREAD_CONFIG_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MAX_CHILDREN                      5
#endif  // OPENTHREAD_CONFIG_MAX_CHILDREN

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
 *
 * The minimum number of supported IPv6 address registrations per child.
 *
 */
#ifndef OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
#define OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD                4
#endif  // OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
 *
 * The maximum number of supported IPv6 addresses allows to be externally added.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
#define OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS                  4
#endif  // OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS

/**
 * @def OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
 *
 * The 6LoWPAN fragment reassembly timeout in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT
#define OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT        5
#endif  // OPENTHREAD_CONFIG_6LOWPAN_REASSEMBLY_TIMEOUT

/**
 * @def OPENTHREAD_CONFIG_MPL_CACHE_ENTRIES
 *
 * The number of MPL cache entries for duplicate detection.
 *
 */
#ifndef OPENTHREAD_CONFIG_MPL_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_MPL_CACHE_ENTRIES                 32
#endif  // OPENTHREAD_CONFIG_MPL_CACHE_ENTRIES

/**
 * @def OPENTHREAD_CONFIG_MPL_CACHE_ENTRY_LIFETIME
 *
 * The MPL cache entry lifetime in seconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_MPL_CACHE_ENTRY_LIFETIME
#define OPENTHREAD_CONFIG_MPL_CACHE_ENTRY_LIFETIME          5
#endif  // OPENTHREAD_CONFIG_MPL_CACHE_ENTRY_LIFETIME

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL                         OPENTHREAD_LOG_LEVEL_CRIT
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

#endif  // OPENTHREAD_CORE_DEFAULT_CONFIG_H_

