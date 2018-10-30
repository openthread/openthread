/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#ifndef OPENTHREAD_CORE_TORANJ_CONFIG_H_
#define OPENTHREAD_CORE_TORANJ_CONFIG_H_

/**
 * This header file defines the OpenThread core configuration options used in NCP build for `toranj` test framework.
 *
 */

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#if OPENTHREAD_RADIO
#define OPENTHREAD_CONFIG_PLATFORM_INFO                         "POSIX-RCP-toranj"
#elif OPENTHREAD_ENABLE_POSIX_APP
#define OPENTHREAD_CONFIG_PLATFORM_INFO                         "POSIX-App-toranj"
#else
#define OPENTHREAD_CONFIG_PLATFORM_INFO                         "POSIX-toranj"
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS                   256

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#define OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES                 32

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_QUERY_MAX_RETRY_DELAY
 *
 * Maximum retry delay for address query (in seconds).
 *
 * Default: 28800 seconds (480 minutes or 8 hours)
 *
 */
#define OPENTHREAD_CONFIG_ADDRESS_QUERY_MAX_RETRY_DELAY         120

/**
 * @def OPENTHREAD_CONFIG_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#define OPENTHREAD_CONFIG_MAX_CHILDREN                          32

/**
 * @def OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT
 *
 * The default child timeout value (in seconds).
 *
 */
#define OPENTHREAD_CONFIG_DEFAULT_CHILD_TIMEOUT                 120

/**
 * @def OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD
 *
 * The maximum number of supported IPv6 address registrations per child.
 *
 */
#define OPENTHREAD_CONFIG_IP_ADDRS_PER_CHILD                    10

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS
 *
 * The maximum number of supported IPv6 addresses allows to be externally added.
 *
 */
#define OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS                      8

/**
 * @def OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS
 *
 * The maximum number of supported IPv6 multicast addresses allows to be externally added.
 *
 */
#define OPENTHREAD_CONFIG_MAX_EXT_MULTICAST_IP_ADDRS            4

/**
 * @def OPENTHREAD_CONFIG_MAC_FILTER_SIZE
 *
 * The number of MAC Filter entries.
 *
 */
#define OPENTHREAD_CONFIG_MAC_FILTER_SIZE                       80

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * Selects if, and where the LOG output goes to.
 *
 */
#define OPENTHREAD_CONFIG_LOG_OUTPUT                            OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level (used at compile time).
 *
 */
#define OPENTHREAD_CONFIG_LOG_LEVEL                             OT_LOG_LEVEL_INFO

/**
 * @def OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL
 *
 * Define as 1 to enable dynamic log level control.
 *
 */
#define OPENTHREAD_CONFIG_ENABLE_DYNAMIC_LOG_LEVEL              1

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL                     0

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_REGION
 *
 * Define to prepend the log region to all log messages
 *
 */
#define OPENTHREAD_CONFIG_LOG_PREPEND_REGION                    0

/**
 * @def OPENTHREAD_CONFIG_LOG_SUFFIX
 *
 * Define suffix to append at the end of logs.
 *
 */
#define OPENTHREAD_CONFIG_LOG_SUFFIX                            ""

/**
 * @def OPENTHREAD_CONFIG_LOG_PLATFORM
 *
 * Define to enable platform region logging.
 *
 */
#define OPENTHREAD_CONFIG_LOG_PLATFORM                          1

/**
 * @def OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
 *
 *  The size of NCP message buffer in bytes
 *
 */
#define OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE                    4096

/**
 * @def OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB
 *
 * Enable setting steering data out of band.
 *
 */
#define OPENTHREAD_CONFIG_ENABLE_STEERING_DATA_SET_OOB          1

/**
 * @def OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH
 *
 * Define as 1 for a child to inform its previous parent when it attaches to a new parent.
 *
 * If this feature is enabled, when a device attaches to a new parent, it will send an IP message (with empty payload
 * and mesh-local IP address as the source address) to its previous parent.
 *
 */
#define OPENTHREAD_CONFIG_INFORM_PREVIOUS_PARENT_ON_REATTACH    1

/**
 * @def OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT
 *
 * Define to 1 to send an MLE Link Request when MAX_NEIGHBOR_AGE is reached for a neighboring router.
 *
 */
#define OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT  1

/**
 * @def OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_DELAY
 *
 * The minimum delay in seconds used by Channel Manager module for performing a channel change.
 *
 * The minimum delay should preferably be longer than maximum data poll interval used by all sleepy-end-devices within
 * the Thread network.
 *
 * Applicable only if Channel Manager feature is enabled (i.e., `OPENTHREAD_ENABLE_CHANNEL_MANAGER` is set).
 *
 */
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_MINIMUM_DELAY         2

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
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_SKIP_FAVORED (0xffff * 7 / 100)

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
#define OPENTHREAD_CONFIG_CHANNEL_MANAGER_THRESHOLD_TO_CHANGE_CHANNEL (0xffff * 10 / 100)

/**
 * @def OPENTHREAD_CONFIG_MESHCOP_PENDING_DATASET_MINIMUM_DELAY
 *
 * Minimum Delay Timer value for a Pending Operational Dataset (in ms).
 *
 * Thread specification defines this value as 30,000. Changing from the specified value should be done for testing only.
 *
 * For `toranj` test script the value is decreased so that the tests can be run faster.
 *
 */
#define OPENTHREAD_CONFIG_MESHCOP_PENDING_DATASET_MINIMUM_DELAY 1000

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
#define OPENTHREAD_CONFIG_NCP_ENABLE_MCU_POWER_STATE_CONTROL 1


#if OPENTHREAD_RADIO
/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
 *
 * Define to 1 if you want to enable software ACK timeout logic.
 *
 */
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT 1

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
 *
 * Define to 1 if you want to enable software retransmission logic.
 *
 */
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT 1

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF
 *
 * Define to 1 if you want to enable software CSMA-CA backoff logic.
 *
 */
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_CSMA_BACKOFF          1
#endif // OPENTHREAD_RADIO

#endif /* OPENTHREAD_CORE_TORANJ_CONFIG_H_ */

