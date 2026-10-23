/*
 *  Copyright (c) 2024-2026, The OpenThread Authors.
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
 *   This file includes compile-time configurations for Thread Direct (TD).
 */

#ifndef OT_CORE_CONFIG_THREAD_DIRECT_H_
#define OT_CORE_CONFIG_THREAD_DIRECT_H_

#include "config/border_routing.h"

/**
 * @addtogroup config-thread-direct
 *
 * @brief
 *   This module includes configuration variables for Thread Direct (TD).
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
 *
 * Define to 1 to enable Thread Direct Wake Initiator functionality.
 */
#ifndef OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE
#define OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
 *
 * Define to 1 to enable Thread Direct Wake Listener functionality.
 */
#ifndef OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE
#define OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
 *
 * Define to 1 to allow Thread Direct `KeyManager` to allocate registered key entries from the heap.
 *
 * When disabled (set to 0), a fixed-size `Pool` with `OPENTHREAD_CONFIG_TD_MAX_REGISTERED_KEYS` entries is used.
 *
 * By default, this is enabled (set to 1) when both `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` and
 * `OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE` are enabled, and disabled (set to 0) otherwise.
 */
#ifndef OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE && OPENTHREAD_CONFIG_BORDER_ROUTING_USE_HEAP_ENABLE
#define OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE 1
#else
#define OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_TD_MAX_REGISTERED_KEYS
 *
 * Specifies the maximum number of registered Thread Direct keys.
 *
 * When `OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE` is enabled, this limits the maximum number of heap-allocated
 * key entries, and can be set to 0 to allow an unlimited number of keys. When heap allocation is disabled, this
 * specifies the size of the internal `Pool` used for allocating key entries.
 *
 * By default, this is set to 0 (unlimited) when `OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE` is enabled, and 8
 * otherwise.
 */
#ifndef OPENTHREAD_CONFIG_TD_MAX_REGISTERED_KEYS
#if OPENTHREAD_CONFIG_TD_KEY_LIST_USE_HEAP_ENABLE
#define OPENTHREAD_CONFIG_TD_MAX_REGISTERED_KEYS 0
#else
#define OPENTHREAD_CONFIG_TD_MAX_REGISTERED_KEYS 8
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_TD_FRAME_COUNTER_AHEAD
 *
 * The value ahead of the current MAC security frame counter for persistent storage of Thread Direct keys.
 */
#ifndef OPENTHREAD_CONFIG_TD_FRAME_COUNTER_AHEAD
#define OPENTHREAD_CONFIG_TD_FRAME_COUNTER_AHEAD 1000
#endif

//----------------------------------------------------------------------------------------------------------------------
// Derived configs

#ifdef OT_CONFIG_THREAD_DIRECT_ENABLE
#error "OT_CONFIG_THREAD_DIRECT_ENABLE MUST NOT be defined directly. It is derived from other configs"
#endif

#define OT_CONFIG_THREAD_DIRECT_ENABLE \
    (OPENTHREAD_CONFIG_TD_WAKE_INITIATOR_ENABLE || OPENTHREAD_CONFIG_TD_WAKE_LISTENER_ENABLE)

/**
 * @}
 */

#endif // OT_CORE_CONFIG_THREAD_DIRECT_H_
