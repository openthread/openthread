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
 *   This file includes compile-time configurations for platform-specific services.
 *
 */

#ifndef CONFIG_PLATFORM_H_
#define CONFIG_PLATFORM_H_

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO "NONE"
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
 *
 * The message pool is managed by platform defined logic when this flag is set.
 * This feature would typically be used when operating in a multi-threaded system
 * and multiple threads need to access the message pool.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
 *
 * The assert is managed by platform defined logic when this flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
 *
 * Define to 1 to enable platform NETIF support.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_NETIF_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
 *
 * Define to 1 to enable platform UDP support.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to enable microsecond backoff timer implemented in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
 *
 * Define to 1 if you want to enable radio coexistence implemented in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE 0
#endif

#endif // CONFIG_PLATFORM_H_
