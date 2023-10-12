/*
 *  Copyright (c) 2021, The OpenThread Authors.
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

#ifndef OPENTHREAD_CORE_ANDROID_CONFIG_H_
#define OPENTHREAD_CORE_ANDROID_CONFIG_H_

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_INFO "POSIX-ANDROID"

/**
 * @def OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH
 *
 * The run time data path.
 *
 */
#define OPENTHREAD_CONFIG_POSIX_SETTINGS_PATH "/data/misc/threadnetwork"

/**
 * @def OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME
 *
 * Define socket basename used by POSIX app daemon.
 *
 */
#define OPENTHREAD_POSIX_CONFIG_DAEMON_SOCKET_BASENAME "/data/misc/threadnetwork/openthread-%s"

/**
 * @def OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE
 *
 * Define as 1 to enable PTY device support in POSIX app.
 *
 */
#define OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 * NOTE: Router devices that support 32 children will have 260 message buffers minimum.
 *       This allows for one 1KB frame per child with a little overhead.
 */
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 280

/**
 * @def OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT
 *
 * The maximum number of retries allowed after a transmission failure for direct transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 3.
 *
 */
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT 15

/**
 * @def OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT
 *
 * The maximum number of retries allowed after a transmission failure for indirect transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 0.
 *
 */
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT 1

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS
 *
 * Maximum number of transmit attempts for an outbound indirect frame (for a sleepy child) each triggered by the
 * reception of a new data request command (a new data poll) from the sleepy child. Each data poll triggered attempt is
 * retried by the MAC layer up to `OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT` times.
 *
 */
#define OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS 16

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES 32

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY
 *
 * Maximum retry delay for address query (in seconds).
 *
 * Replace with 2 min instead of the OT default of 8 hours.
 *
 */
#define OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY 120

/**
 * @def OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#define OPENTHREAD_CONFIG_MLE_MAX_CHILDREN 64

/**
 * @def OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD
 *
 * The minimum number of supported IPv6 address registrations per child.
 *
 */
#define OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD 6

/**
 * @def OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT
 *
 * Define to 1 to send an MLE Link Request when MAX_NEIGHBOR_AGE is reached for a neighboring router.
 *
 * This is enabled to increase reliability of exchanges between routers (default on OpenThread is disabled).
 *
 */
#define OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT 1

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 *
 */
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS 3

/**
 * @def OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
 *
 * Enable setting steering data out of band.
 *
 * This is used for "joinable" mode during weave pairing
 *
 */
#define OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION
 *
 * The Beacon version to use when the beacon join flag is set.
 *
 * This is required to enable the to be able to assist older legacy only device (e.g., T2/T1 not running Fuji) during
 * weave pairing flow.
 *
 */
#define OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION 1

/**
 * @def OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
 *
 * The size of NCP message buffer in bytes
 *
 */
#define OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE 3500

/**
 * @def OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode output buffer size.
 *
 */
#define OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE 1290

/**
 * @def OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX
 *
 * Define OpenThread diagnostic mode max command line arguments.
 *
 */
#define OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX 64

/**
 * @def OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode command line buffer size
 *
 */
#define OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE 1300

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level (used at compile time). If `OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE`
 * is set, this defines the most verbose log level possible.
 *
 */
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_INFO

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
 *
 * Define as 1 to enable dynamic log level control.
 *
 * Note that the OPENTHREAD_CONFIG_LOG_LEVEL determines the log level at
 * compile time. The dynamic log level control (if enabled) only allows
 * decreasing the log level from the compile time value.
 *
 */
#define OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * Selects if, and where the LOG output goes to.
 *
 * OPENTHREAD_CONFIG_LOG_OUTPUT_NONE: Log output goes to the bit bucket (disabled)
 * OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED: Log output is handled by a platform defined function
 * OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL: Log output for NCP goes to Spinel `STREAM_LOG` property.
 *
 * Log output is set to `PLATFORM_DEFINED` which pushes the log through syslog().
 *
 */
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL 1

/**
 * @def OPENTHREAD_CONFIG_LOG_SUFFIX
 *
 * Define suffix to append at the end of logs.
 *
 */
#define OPENTHREAD_CONFIG_LOG_SUFFIX ""

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
 *
 * Define to 1 if you want to enable radio coexistence implemented in platform.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
 *
 * Disable UDP forward support.
 *
 */
#define OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE 0

#define OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE 1
#define OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE 1
#define OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE 1
#define OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE 1
#define OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE 1
#define OPENTHREAD_CONFIG_DNSSD_SERVER_ENABLE 1
#define OPENTHREAD_CONFIG_ECDSA_ENABLE 1
#define OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE 1
#define OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE 1
#define OPENTHREAD_CONFIG_MAC_FILTER_ENABLE 1
#define OPENTHREAD_CONFIG_MLR_ENABLE 1
#define OPENTHREAD_CONFIG_PING_SENDER_ENABLE 1
#define OPENTHREAD_CONFIG_SRP_SERVER_ENABLE 1
#define OPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE 1

// Disables built-in TCP support as TCP can be support on upper layer
#define OPENTHREAD_CONFIG_TCP_ENABLE 0

// Disable posix multicast routing feature in ot-daemon
#define OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE 0

#define OPENTHREAD_POSIX_CONFIG_SPINEL_VENDOR_INTERFACE_URL_PROTOCOL_NAME "threadnetwork_hal"

#endif // OPENTHREAD_CORE_ANDROID_CONFIG_H_
