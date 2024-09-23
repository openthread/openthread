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
 *   This file includes compile-time configurations for the MAC.
 */

#ifndef CONFIG_MAC_H_
#define CONFIG_MAC_H_

/**
 * @addtogroup config-mac
 *
 * @brief
 *   This module includes configuration variables for MAC.
 *
 * @{
 */

#include "config/time_sync.h"
#include "config/wakeup.h"

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT
 *
 * The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure.
 *
 * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006, default value is 4.
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT
#define OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT 4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT
 *
 * The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure.
 *
 * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006, default value is 4.
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT
#define OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT 4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT
 *
 * The default maximum number of retries allowed after a transmission failure for direct transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 15.
 */
#ifndef OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT 15
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT
 *
 * The default maximum number of retries allowed after a transmission failure for indirect transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 0.
 */
#ifndef OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
 *
 * Define as 1 to add random backoff delay in between frame transmission retries when the previous attempt resulted in
 * no-ack error.
 */
#ifndef OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY
#define OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_RETX_DELAY_MIN_BACKOFF_EXPONENT
 *
 * Specifies the minimum backoff exponent to start with when adding random delay in between frame transmission
 * retries on no-ack error. It is applicable only when `OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY`
 * is enabled.
 */
#ifndef OPENTHREAD_CONFIG_MAC_RETX_DELAY_MIN_BACKOFF_EXPONENT
#define OPENTHREAD_CONFIG_MAC_RETX_DELAY_MIN_BACKOFF_EXPONENT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_RETX_DELAY_MAX_BACKOFF_EXPONENT
 *
 * Specifies the maximum backoff exponent when adding random delay in between frame transmission retries on no-ack
 * error. It is applicable only when `OPENTHREAD_CONFIG_MAC_ADD_DELAY_ON_NO_ACK_ERROR_BEFORE_RETRY` is enabled.
 */
#ifndef OPENTHREAD_CONFIG_MAC_RETX_DELAY_MAX_BACKOFF_EXPONENT
#define OPENTHREAD_CONFIG_MAC_RETX_DELAY_MAX_BACKOFF_EXPONENT 5
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
 *
 * Define as 1 to enable collision avoidance delay feature, which adds a delay wait time after a successful frame tx
 * to a neighbor which is expected to forward the frame. This delay is applied before the next direct frame tx (towards
 * any neighbor) on an FTD.
 *
 * The delay interval is specified by `OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_INTERVAL` (in milliseconds).
 */
#ifndef OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE
#define OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_INTERVAL
 *
 * Specifies the collision avoidance delay interval in milliseconds. This is added after a successful frame tx to a
 * neighbor that is expected to forward the frame (when `OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_ENABLE` is
 * enabled).
 */
#ifndef OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_INTERVAL
#define OPENTHREAD_CONFIG_MAC_COLLISION_AVOIDANCE_DELAY_INTERVAL 8
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
 *
 * Define to 1 to enable MAC retry packets histogram analysis.
 */
#ifndef OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
#define OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT
 *
 * The default size of MAC histogram array for success message retry direct transmission.
 *
 * Default value is (OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT + 1).
 */
#ifndef OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT
#define OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_DIRECT \
    (OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT + 1)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT
 *
 * The default size of MAC histogram array for success message retry direct transmission.
 *
 * Default value is (OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT + 1).
 */
#ifndef OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT
#define OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_MAX_SIZE_COUNT_INDIRECT \
    (OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT + 1)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS
 *
 * Maximum number of received IEEE 802.15.4 Data Requests for a queued indirect transaction.
 *
 * The indirect frame remains in the transaction queue until it is successfully transmitted or until the indirect
 * transmission fails after the maximum number of IEEE 802.15.4 Data Request messages have been received.
 *
 * Takes the place of macTransactionPersistenceTime. The time period is specified in units of IEEE 802.15.4 Data
 * Request receptions, rather than being governed by macBeaconOrder.
 *
 * @sa OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS
#define OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS 4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US
 *
 * Define how many microseconds ahead should MAC deliver CSL frame to SubMac.
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US
#define OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US 2000
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
 *
 * Define to 1 to enable MAC filter support.
 */
#ifndef OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
#define OPENTHREAD_CONFIG_MAC_FILTER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_FILTER_SIZE
 *
 * The number of MAC Filter entries.
 */
#ifndef OPENTHREAD_CONFIG_MAC_FILTER_SIZE
#define OPENTHREAD_CONFIG_MAC_FILTER_SIZE 32
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_TX_NUM_BCAST
 *
 * The number of times each IEEE 802.15.4 broadcast frame is transmitted.
 *
 * The minimum value is 1. Values larger than 1 may improve broadcast reliability by increasing redundancy, but may
 * also increase congestion.
 */
#ifndef OPENTHREAD_CONFIG_MAC_TX_NUM_BCAST
#define OPENTHREAD_CONFIG_MAC_TX_NUM_BCAST 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
 *
 * Define as 1 to stay awake between fragments while transmitting a large packet,
 * and to stay awake after receiving a packet with frame pending set to true.
 */
#ifndef OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
#define OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION
 *
 * The Beacon version to use when the beacon join flag is set.
 */
#ifndef OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION
#define OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION OPENTHREAD_CONFIG_THREAD_VERSION
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_BEACON_RSP_WHEN_JOINABLE_ENABLE
 *
 * Define to 1 to enable IEEE 802.15.4 Beacons when joining is enabled.
 *
 * @note When this feature is enabled, the device will transmit IEEE 802.15.4 Beacons in response to IEEE 802.15.4
 * Beacon Requests even while the device is not router capable and detached.
 */
#ifndef OPENTHREAD_CONFIG_MAC_BEACON_RSP_WHEN_JOINABLE_ENABLE
#define OPENTHREAD_CONFIG_MAC_BEACON_RSP_WHEN_JOINABLE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
 *
 * Define as 1 to support IEEE 802.15.4-2015 Header IE (Information Element) generation and parsing, it must be set
 * to support following features:
 *    1. Time synchronization service feature (i.e., OPENTHREAD_CONFIG_TIME_SYNC_ENABLE is set).
 *    2. Thread 1.2.
 *
 * @note If it's enabled, platform must support interrupt context and concurrent access AES.
 */
#ifndef OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE || (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#define OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT 1
#else
#define OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD
 *
 * The Data Poll period during attach in milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD
#define OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD 100
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD
 *
 * This setting configures the minimum poll period in milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD
#define OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD 10
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD
 *
 * This setting configures the retx poll period in milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD
#define OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD 1000
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
 *
 * Define to 1 to enable software ACK timeout logic.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
 *
 * Define to 1 to enable software retransmission logic.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
 *
 * Define to 1 to enable software CSMA-CA backoff logic.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
 *
 * Define to 1 to enable software transmission security logic.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
 *
 * Define to 1 to enable software transmission target time logic.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE
 *
 * Define to 1 to enable software reception target time logic.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_ENERGY_SCAN_ENABLE
 *
 * Define to 1 to enable software energy scanning logic.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_ENERGY_SCAN_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_ENERGY_SCAN_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_ON_WHEN_IDLE_ENABLE
 *
 * Define to 1 to enable software rx off when idle switching.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_ON_WHEN_IDLE_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_ON_WHEN_IDLE_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
 *
 * Define to 1 to enable csl transmitter logic.
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE
#define OPENTHREAD_CONFIG_MAC_CSL_TRANSMITTER_ENABLE (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
 *
 * This setting configures the CSL receiver feature in Thread 1.2.
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
#define OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MULTIPURPOSE_FRAME
 *
 * Define to 1 to enable support for IEEE 802.15.4 MAC Multipurpose frame format.
 */
#ifndef OPENTHREAD_CONFIG_MAC_MULTIPURPOSE_FRAME
#define OPENTHREAD_CONFIG_MAC_MULTIPURPOSE_FRAME \
    (OPENTHREAD_CONFIG_WAKEUP_COORDINATOR_ENABLE || OPENTHREAD_CONFIG_WAKEUP_END_DEVICE_ENABLE)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_AUTO_SYNC_ENABLE
 *
 * This setting configures CSL auto synchronization based on data poll mechanism in Thread 1.2.
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_AUTO_SYNC_ENABLE
#define OPENTHREAD_CONFIG_MAC_CSL_AUTO_SYNC_ENABLE OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
 *
 * This setting configures the usage of local time rather than radio time for calculating the
 * elapsed time since last CSL synchronization event in order to schedule the duration of the
 * CSL receive window.
 *
 * This is done at expense of too short or too long receive windows depending on the drift
 * between the two clocks within the CSL timeout period. In order to compensate for a too
 * short receive window, CSL uncertainty can be increased.
 *
 * This setting can be useful for platforms in which is important to reduce the number of
 * radio API calls, for instance when they are costly. One typical situation is a multicore
 * chip architecture in which different instances of current time are being kept in host and
 * radio cores. In this case, accessing the radio core current time API requires serialization
 * and it is more costly than just accessing local host time.
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC
#define OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_LOCAL_TIME_SYNC 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_MIN_PERIOD
 *
 * This setting configures the minimum CSL period that could be used, in units of milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_MIN_PERIOD
#define OPENTHREAD_CONFIG_MAC_CSL_MIN_PERIOD 10
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_MAX_TIMEOUT
 *
 * This setting configures the maximum CSL timeout that could be used, in units of seconds.
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_MAX_TIMEOUT
#define OPENTHREAD_CONFIG_MAC_CSL_MAX_TIMEOUT 10000
#endif

/**
 * @def OPENTHREAD_CONFIG_CSL_TIMEOUT
 *
 * The default CSL timeout in seconds.
 */
#ifndef OPENTHREAD_CONFIG_CSL_TIMEOUT
#define OPENTHREAD_CONFIG_CSL_TIMEOUT 100
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
 *
 * CSL receiver debug option. When this option is enabled, a CSL receiver wouldn't actually sleep in CSL state so it
 * can still receive packets from the CSL transmitter.
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE
#define OPENTHREAD_CONFIG_MAC_CSL_DEBUG_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_CSL_TRANSMIT_TIME_AHEAD
 *
 * Transmission scheduling and ramp up time needed for the CSL transmitter to be ready, in units of microseconds.
 * This time must include at least the radio's turnaround time between end of CCA and start of preamble transmission.
 * To avoid early CSL transmission it also must not be configured higher than the actual scheduling and ramp up time.
 */
#ifndef OPENTHREAD_CONFIG_CSL_TRANSMIT_TIME_AHEAD
#define OPENTHREAD_CONFIG_CSL_TRANSMIT_TIME_AHEAD 40
#endif

/**
 * @def OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD
 *
 * Reception scheduling and ramp up time needed for the CSL receiver to be ready, in units of microseconds.
 */
#ifndef OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD
#define OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD 320
#endif

/**
 * @def OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD
 *
 * The minimum time (in microseconds) before the MHR start that the radio should be in receive state and ready to
 * properly receive in order to properly receive any IEEE 802.15.4 frame. Defaults to the duration of SHR + PHR.
 */
#ifndef OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD
#define OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD (6 * 32)
#endif

/**
 * @def OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER
 *
 * The minimum time (in microseconds) after the MHR start that the radio should be in receive state in order
 * to properly receive any IEEE 802.15.4 frame. Defaults to the duration of a maximum size frame, plus AIFS,
 * plus the duration of maximum enh-ack frame. Platforms are encouraged to improve this value for energy
 * efficiency purposes.
 */
#ifndef OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER
#define OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER ((127 + 6 + 39) * 32)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SCAN_DURATION
 *
 * This setting configures the default scan duration in milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_MAC_SCAN_DURATION
#define OPENTHREAD_CONFIG_MAC_SCAN_DURATION 300
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE
 *
 * This setting configures if the beacon payload parsing needs to be enabled in MAC. This is optional and is disabled by
 * default because Thread 1.2.1 has removed support for beacon payloads.
 */
#ifndef OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE
#define OPENTHREAD_CONFIG_MAC_BEACON_PAYLOAD_PARSING_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE
 *
 * This setting configures if the beacon payload needs to be enabled in outgoing beacon frames. This is optional and is
 * disabled by default because Thread 1.2.1 has removed support for beacon payloads.
 */
#ifndef OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE
#define OPENTHREAD_CONFIG_MAC_OUTGOING_BEACON_PAYLOAD_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_DATA_POLL_TIMEOUT
 *
 * This setting specifies the timeout for receiving the Data Frame (in msec) - after an ACK with FP bit set was
 * received.
 */
#ifndef OPENTHREAD_CONFIG_MAC_DATA_POLL_TIMEOUT
#define OPENTHREAD_CONFIG_MAC_DATA_POLL_TIMEOUT 100
#endif

/**
 * @}
 */

#endif // CONFIG_MAC_H_
