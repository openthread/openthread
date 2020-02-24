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
 *
 */

#ifndef CONFIG_MAC_H_
#define CONFIG_MAC_H_

#include "config/time_sync.h"

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT
 *
 * The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure.
 *
 * Equivalent to macMaxCSMABackoffs in IEEE 802.15.4-2006, default value is 4.
 *
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
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT
#define OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT 4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT
 *
 * The default maximum number of retries allowed after a transmission failure for direct transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 3.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT 3
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT
 *
 * The default maximum number of retries allowed after a transmission failure for indirect transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 0.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_RETRY_SUCCESS_HISTOGRAM_ENABLE
 *
 * Define to 1 to enable MAC retry packets histogram analysis.
 *
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
 *
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
 *
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
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS
#define OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS 4
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
 *
 * Define to 1 to enable MAC filter support.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_FILTER_ENABLE
#define OPENTHREAD_CONFIG_MAC_FILTER_ENABLE 0
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
 * @def OPENTHREAD_CONFIG_MAC_TX_NUM_BCAST
 *
 * The number of times each IEEE 802.15.4 broadcast frame is transmitted.
 *
 * The minimum value is 1. Values larger than 1 may improve broadcast reliability by increasing redundancy, but may
 * also increase congestion.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_TX_NUM_BCAST
#define OPENTHREAD_CONFIG_MAC_TX_NUM_BCAST 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
 *
 * Define as 1 to disable CSMA-CA on the last transmit attempt.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_DISABLE_CSMA_CA_ON_LAST_ATTEMPT
#define OPENTHREAD_CONFIG_MAC_DISABLE_CSMA_CA_ON_LAST_ATTEMPT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
 *
 * Define as 1 to stay awake between fragments while transmitting a large packet,
 * and to stay awake after receiving a packet with frame pending set to true.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
#define OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION
 *
 * The Beacon version to use when the beacon join flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION
#define OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION OPENTHREAD_THREAD_VERSION
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_BEACON_RSP_WHEN_JOINABLE_ENABLE
 *
 * Define to 1 to enable IEEE 802.15.4 Beacons when joining is enabled.
 *
 * @note When this feature is enabled, the device will transmit IEEE 802.15.4 Beacons in response to IEEE 802.15.4
 * Beacon Requests even while the device is not router capable and detached.
 *
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
 *
 * @note If it's enabled, platform must support interrupt context and concurrent access AES.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
#define OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT 1
#else
#define OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD
 *
 * The Data Poll period during attach in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD
#define OPENTHREAD_CONFIG_MAC_ATTACH_DATA_POLL_PERIOD 100
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD
 *
 * This setting configures the minimum poll period in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD
#define OPENTHREAD_CONFIG_MAC_MINIMUM_POLL_PERIOD 10
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD
 *
 * This setting configures the retx poll period in milliseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD
#define OPENTHREAD_CONFIG_MAC_RETX_POLL_PERIOD 1000
#endif

#endif // CONFIG_MAC_H_
