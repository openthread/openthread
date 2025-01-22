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
 * This header file defines the OpenThread core configuration example when building as standalone mDNS library.
 */

#ifndef OT_CORE_CONFIG_MDNS_H_
#define OT_CORE_CONFIG_MDNS_H_

// The following configs are recommended when building OpenThread as a
// standalone mDNS library (`libopenthread-mdns`).

//----------------------------------------------------------------------------------------------------------------------
// mDNS configs
//
// - `MULTICAST_DNS_PUBLIC_API_ENABLE`: MUST be enabled to provide
//   public APIs.
// - `MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE`: Controls whether
//   additional public APIs (for iterating over browsers/resolvers) are
//   provided.  Enabling this is recommended, but it can be disabled if
//   the APIs are not used.
// - `MULTICAST_DNS_DEFAULT_QUESTION_UNICAST_ALLOWED`: Controls whether
//   the OpenThread mDNS can use QU questions. Enabling this is
//   recommended.
//
#define OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE 1
#define OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE 1
#define OPENTHREAD_CONFIG_MULTICAST_DNS_DEFAULT_QUESTION_UNICAST_ALLOWED 1

//----------------------------------------------------------------------------------------------------------------------
// Heap - OT mDNS implementation uses heap - Using external heap is
// recommended.

#define OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE 1

//----------------------------------------------------------------------------------------------------------------------
// Message pool configs
//
// Because the heap is required and used by the OpenThread mDNS
// implementation, `OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE` is also
// recommended. This ensures that `otMessage` instances also use the
// heap and not a pre-allocated message pool.
//

#define OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE 1
#define OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT 0

//----------------------------------------------------------------------------------------------------------------------
// Timer configs
//
// OpenThread mDNS uses millisecond timers, and microsecond timer
// support is not needed and can be disabled.
//
// `OPENTHREAD_CONFIG_UPTIME_ENABLE` is not directly used by the mDNS
// implementation but is recommended when logging is enabled(to provide
// timestamps in the logs).

#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE 0
#define OPENTHREAD_CONFIG_UPTIME_ENABLE 1

//----------------------------------------------------------------------------------------------------------------------
// Assert configs
//
// - `OPENTHREAD_CONFIG_ASSERT_ENABLE`: Enables asserts. Disabling this
//   can reduce code size.
// - `OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT`: Delegates assert
//   implementation to the platform, requiring `otPlatAssertFail()` to
//   be provided by the platform. If asserts are enabled, providing and
//   using a platform-specific `otPlatAssertFail()` is recommended.
// - `OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL`: Adds
//   assert checks for null pointer-type API input parameters. Enabling
//   this can significantly increase code size due to the numerous
//   assert checks added for all API pointer parameters. Therefore,
//   enabling and using this feature during debugging only is
//   recommended.

#ifndef OPENTHREAD_CONFIG_ASSERT_ENABLE
#define OPENTHREAD_CONFIG_ASSERT_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT OPENTHREAD_CONFIG_ASSERT_ENABLE
#endif

#ifndef OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL
#define OPENTHREAD_CONFIG_ASSERT_CHECK_API_POINTER_PARAM_FOR_NULL 0
#endif

//----------------------------------------------------------------------------------------------------------------------
// Log configs
//
// - `OPENTHREAD_CONFIG_LOG_OUTPUT`: Recommended to be set to
//   `PLATFORM_DEFINED`, requiring the platform to provide `otPlatLog()`
//   It can also be set to `OPENTHREAD_CONFIG_LOG_OUTPUT_NONE` to
//   disable all logs (which can reduce code size).
// - `OPENTHREAD_CONFIG_LOG_LEVEL`: Recommended to be set to
//   `OT_LOG_LEVEL_INFO` during development and testing. It can be set
//   to `OT_LOG_LEVEL_NONE` to disable all logs (except platform-specific
//   logs).

#ifndef OPENTHREAD_CONFIG_LOG_OUTPUT
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_INFO
#endif

#define OPENTHREAD_CONFIG_LOG_PREPEND_UPTIME OPENTHREAD_CONFIG_UPTIME_ENABLE

#endif // OT_CORE_CONFIG_MDNS_H_
