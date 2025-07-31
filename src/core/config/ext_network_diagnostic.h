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
 *   This file includes compile-time configurations for the Extended Network Diagnostic.
 */

#ifndef OT_CORE_CONFIG_EXT_NETWORK_DIAGNOSTIC_H_
#define OT_CORE_CONFIG_EXT_NETWORK_DIAGNOSTIC_H_

/**
 * @addtogroup config-ext-network-diagnostic
 *
 * @brief
 *   This module includes configuration variables for the Extended Network Diagnostic.
 *
 * @{
 */

#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_ENABLE
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_ENABLE 0
#endif

#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_ENABLE 0
#endif

#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_SERVER_ENABLE
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_SERVER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CACHE_BUFFER_LIMIT
 *
 * If the number of cache buffers used for the diagnostic cache exceeds this
 * value an update message will be sent irrespective of current timer state.
 *
 * @Note This does not prevent the cache from growing further. The diagnostic
 * cache will be evicted irrespective of its size when more message buffers are
 * needed elsewhere.
 */
#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CACHE_BUFFERS_LIMIT
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CACHE_BUFFERS_LIMIT 4
#endif

/**
 * @def OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_REGISTRATION_INTERVAL
 *
 * The maximum allowed time between registration requests from clients needed
 * to keep servers active.
 *
 * In milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_REGISTRATION_INTERVAL
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_REGISTRATION_INTERVAL (1000 * 60 * 10)
#endif

/**
 * @def OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_BASE_DELAY
 *
 * The standard delay after most state changes before an update is sent to
 * clients.
 *
 * In milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_BASE_DELAY
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_BASE_DELAY (1000 * 10)
#endif

/**
 * @def OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_EXT_DELAY
 *
 * The extended delay after state change to variables which change very frequently
 * before an update is sent to clients. For example this includes most counters
 * and link margins.
 *
 * In milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_EXT_DELAY
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_EXT_DELAY (1000 * 60)
#endif

/**
 * @def OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_JITTER
 *
 * The jitter applied to registration requests on the client.
 *
 * In milliseconds.
 */
#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_JITTER
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_JITTER (1000 * 30)
#endif

/**
 * @def OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_AHEAD
 *
 * The number of jitter periods to subtract from the registration interval to
 * determine the time between registration attempts on the client.
 *
 * @Note If a registration attempt fails retries will randomly happen within a
 * timespan up to the registration jitter period. Thus this value defines
 * how many retries can happen before a registration may be lost.
 */
#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_AHEAD
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_CLIENT_REGISTRATION_AHEAD 4
#endif

/**
 * @def OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_JITTER
 *
 * The random jitter applied to diagnostic update message timing on servers.
 * This helps distribute network traffic when multiple devices have updates
 * ready at similar times, reducing the chance of synchronized transmissions.
 *
 * Applied when scheduling update timers with extended delay (e.g., for counters
 * and link metrics that change frequently).
 *
 * In milliseconds. Default value is 10000 ms (10 seconds).
 */
#ifndef OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_JITTER
#define OPENTHREAD_CONFIG_EXT_NETWORK_DIAGNOSTIC_UPDATE_JITTER (1000 * 10)
#endif

/**
 * @}
 */

#endif // OT_CORE_CONFIG_EXT_NETWORK_DIAGNOSTIC_H_
