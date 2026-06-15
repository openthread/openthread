/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file includes compile-time configurations for Thread Direct,
 *   covering both the Wake Initiator (WI) and Wake Listener (WL) roles.
 */

#ifndef OT_CORE_CONFIG_THREAD_DIRECT_H_
#define OT_CORE_CONFIG_THREAD_DIRECT_H_

/**
 * @addtogroup config-thread-direct
 *
 * @brief
 *   This module includes configuration variables for the Thread Direct Wake Initiator
 *   and Wake Listener roles.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE
 *
 * Define to 1 to enable the Wake Initiator (WI) role.  A WI transmits periodic Wake
 * Frames on Wake Channel 20 to establish a Thread Direct link with a sleeping Wake
 * Listener.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE
#define OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INITIATOR_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
 *
 * Define to 1 to enable the Wake Listener (WL) role.  A WL periodically samples Wake
 * Channel 20 for incoming Wake Frames to establish a Thread Direct link.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE
#define OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_LISTENER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INTERVAL_US
 *
 * Default interval between consecutive Wake Frame transmissions, in microseconds
 *
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INTERVAL_US
#define OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_INTERVAL_US 7500
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_DURATION_MS
 *
 * Default duration for which Wake Frames are periodically transmitted, in milliseconds
 *
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_DURATION_MS
#define OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_DURATION_MS 1090
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_INTERVAL_US
 *
 * Default WL listen interval in microseconds
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_INTERVAL_US
#define OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_INTERVAL_US 1000000
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_DURATION_US
 *
 * Default WL listen window duration in microseconds
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_DURATION_US
#define OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_DURATION_US 8000
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_RECEIVE_TIME_AFTER
 *
 * Margin applied after the end of a WL listen window before scheduling the next listen
 * interval, in microseconds.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_RECEIVE_TIME_AFTER
#define OPENTHREAD_CONFIG_THREAD_DIRECT_LISTEN_RECEIVE_TIME_AFTER 500
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_FRAME_TX_CCA_ENABLE
 *
 * Define to 1 to perform CCA before transmitting a Wake Frame.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_FRAME_TX_CCA_ENABLE
#define OPENTHREAD_CONFIG_THREAD_DIRECT_WAKE_FRAME_TX_CCA_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_CONNECTION_RETRY_INTERVAL
 *
 * The Connection Retry Interval encoded in the Wake Frame Rendezvous Time field.
 * Specifies how frequently a WL should retry sending the TD Link Command after
 * receiving a Wake Frame, in units of Wake Intervals (7.5 ms by default).
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_CONNECTION_RETRY_INTERVAL
#define OPENTHREAD_CONFIG_THREAD_DIRECT_CONNECTION_RETRY_INTERVAL 1
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_CONNECTION_RETRY_COUNT
 *
 * The Connection Retry Count encoded in the Wake Frame.  Specifies the maximum number
 * of times the WL retries sending the TD Link Command after receiving a Wake Frame.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_CONNECTION_RETRY_COUNT
#define OPENTHREAD_CONFIG_THREAD_DIRECT_CONNECTION_RETRY_COUNT 12
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_DEFAULT_WAKE_CHANNEL
 *
 * The Thread Direct Wake Channel.  The Thread Direct feature mandates channel 20
 * as the sole Wake Channel.  This value must not be changed.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_DEFAULT_WAKE_CHANNEL
#define OPENTHREAD_CONFIG_THREAD_DIRECT_DEFAULT_WAKE_CHANNEL 20
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_MIN_DURATION_SLOTS
 *
 * Minimum advertised Scheduled Listen Window (SLW) duration in 160 us slots
 *  8 slots * 160 us = 1.28 ms.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_MIN_DURATION_SLOTS
#define OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_MIN_DURATION_SLOTS 8
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_TIMEOUT
 *
 * Default SLW link inactivity timeout in seconds.
 *
 * After a TD link is established, if no unicast frame is received from the WI
 * peer within this many seconds the stack tears down the link and stops the
 * WL's SLW schedule.  Overridable at runtime via `otThreadDirectSetSlwTimeout()`.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_TIMEOUT
#define OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_TIMEOUT 100
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_MAX_TIMEOUT
 *
 * Maximum value accepted by `otThreadDirectSetSlwTimeout()`, in seconds.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_MAX_TIMEOUT
#define OPENTHREAD_CONFIG_THREAD_DIRECT_SLW_MAX_TIMEOUT 10000
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_COEX_ENABLE
 *
 * Define to 1 to enable CoEx support in Thread Direct.
 *
 * When enabled, the SCA LTV includes a Radio Availability Mask (RAM) bitmap
 * advertising the local device's CoEx constraints.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_COEX_ENABLE
#define OPENTHREAD_CONFIG_THREAD_DIRECT_COEX_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_GUEST_WAKE_KEY_ENABLE
 *
 * Define to 1 to support out-of-band-provisioned guest Wake Keys (key indices 130-192).
 * Enabled by default.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_GUEST_WAKE_KEY_ENABLE
#define OPENTHREAD_CONFIG_THREAD_DIRECT_GUEST_WAKE_KEY_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_MAX_GUEST_WAKE_KEYS
 *
 * Maximum number of guest Wake Keys that can be stored simultaneously.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_MAX_GUEST_WAKE_KEYS
#define OPENTHREAD_CONFIG_THREAD_DIRECT_MAX_GUEST_WAKE_KEYS 4
#endif

/**
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_MAX_DIRECT_PEERS
 *
 * Maximum number of simultaneous Thread Direct peers.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_MAX_DIRECT_PEERS
#define OPENTHREAD_CONFIG_THREAD_DIRECT_MAX_DIRECT_PEERS 1
#endif

/**
 * @}
 */

#endif // OT_CORE_CONFIG_THREAD_DIRECT_H_
