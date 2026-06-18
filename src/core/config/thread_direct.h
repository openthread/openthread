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
 * @def OPENTHREAD_CONFIG_THREAD_DIRECT_DEFAULT_WAKE_CHANNEL
 *
 * The Thread Direct Wake Channel.  The Thread Direct feature mandates channel 20
 * as the sole Wake Channel.  This value must not be changed.
 */
#ifndef OPENTHREAD_CONFIG_THREAD_DIRECT_DEFAULT_WAKE_CHANNEL
#define OPENTHREAD_CONFIG_THREAD_DIRECT_DEFAULT_WAKE_CHANNEL 20
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
