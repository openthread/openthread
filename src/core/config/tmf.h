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
 *   This file includes compile-time configurations for the Thread Management Framework service.
 *
 */

#ifndef CONFIG_TMF_H_
#define CONFIG_TMF_H_

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES 10
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_MAX_SNOOP_ENTRIES
 *
 * The maximum number of EID-to-RLOC cache entries that can be used for "snoop optimization" where an entry is created
 * by inspecting a received message.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_MAX_SNOOP_ENTRIES
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_MAX_SNOOP_ENTRIES 2
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_SNOOP_CACHE_ENTRY_TIMEOUT
 *
 * The timeout value (in seconds) blocking eviction of an address cache entry created through snoop optimization (i.e.,
 * inspection of a received message). After the timeout expires the entry can be reclaimed again. This timeout allows
 * a longer response delay for a received message giving more chance that a snooped entry will be used (avoiding
 * sending Address Query when a response message is sent to same destination from which the message was received
 * earlier).
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_SNOOP_CACHE_ENTRY_TIMEOUT
#define OPENTHREAD_CONFIG_TMF_SNOOP_CACHE_ENTRY_TIMEOUT 3
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_TIMEOUT
 *
 * The timeout value (in seconds) waiting for a address notification response after sending an address query.
 *
 * Default: 3 seconds
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_TIMEOUT
#define OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_TIMEOUT 3
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_INITIAL_RETRY_DELAY
 *
 * Initial retry delay for address query (in seconds).
 *
 * Default: 15 seconds
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_INITIAL_RETRY_DELAY
#define OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_INITIAL_RETRY_DELAY 15
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY
 *
 * Maximum retry delay for address query (in seconds).
 *
 * Default: 28800 seconds (480 minutes or 8 hours)
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY
#define OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY 28800
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_PENDING_DATASET_MINIMUM_DELAY
 *
 * Minimum Delay Timer value for a Pending Operational Dataset (in ms).
 *
 * Thread specification defines this value as 30,000 ms. Changing from the specified value should be done for testing
 * only.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_PENDING_DATASET_MINIMUM_DELAY
#define OPENTHREAD_CONFIG_TMF_PENDING_DATASET_MINIMUM_DELAY 30000
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_PENDING_DATASET_DEFAULT_DELAY
 *
 * Default Delay Timer value for a Pending Operational Dataset (in ms).
 *
 * Thread specification defines this value as 300,000 ms. Changing from the specified value should be done for testing
 * only.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_PENDING_DATASET_DEFAULT_DELAY
#define OPENTHREAD_CONFIG_TMF_PENDING_DATASET_DEFAULT_DELAY 300000
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ENERGY_SCAN_MAX_RESULTS
 *
 * The maximum number of Energy List entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ENERGY_SCAN_MAX_RESULTS
#define OPENTHREAD_CONFIG_TMF_ENERGY_SCAN_MAX_RESULTS 64
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
 *
 * Define to 1 to support injecting Service entries into the Thread Network Data.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE
#define OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS
 *
 * The maximum number of supported Service ALOCs registrations for this device.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS
#define OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_MAX_ALOCS 1
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
 *
 * Define to 1 to enable TMF network diagnostics on MTDs.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE
#define OPENTHREAD_CONFIG_TMF_NETWORK_DIAG_MTD_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
 *
 * Define to 1 for Thread 1.2 FTD device to register DUA of its MTD children registered
 * even if it doesn't enable DUA feature itself.
 *
 */
#define OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE \
    ((OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2) && OPENTHREAD_FTD)

/**
 *
 * This setting configures the Multicast Listener Registration parent proxing in Thread 1.2.
 *
 * This is compulsory for 1.2 FTD.
 *
 */
#ifdef OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE
#error \
    "Don't define `OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE` manually. It's an alias of `((OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2) && OPENTHREAD_FTD)`."
#else
#define OPENTHREAD_CONFIG_TMF_PROXY_MLR_ENABLE \
    ((OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2) && OPENTHREAD_FTD)
#endif

#endif // CONFIG_TMF_H_
