/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes posix compile-time configuration constants
 *   for OpenThread.
 */

#ifndef OPENTHREAD_CORE_POSIX_CONFIG_H_
#define OPENTHREAD_CORE_POSIX_CONFIG_H_

#ifndef OPENTHREAD_RADIO
#define OPENTHREAD_RADIO 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_INFO "POSIX"

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * Specify where the log output should go.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_OUTPUT /* allow command line override */
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

/**
 * @def OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE
 *
 * Define as 1 to enable support for adding of auto-configured SLAAC addresses by OpenThread.
 *
 */
#ifndef OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE /* allows command line override */
#define OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE 1
#endif

#if OPENTHREAD_RADIO
/**
 * @def OPENTHREAD_CONFIG_SOFTWARE_ACK_TIMEOUT_ENABLE
 *
 * Define to 1 if you want to enable software ACK timeout logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_SOFTWARE_ACK_TIMEOUT_ENABLE
#define OPENTHREAD_CONFIG_SOFTWARE_ACK_TIMEOUT_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_SOFTWARE_ENERGY_SCAN_ENABLE
 *
 * Define to 1 if you want to enable software energy scanning logic.
 *
 * Applicable only if raw link layer API is enabled (i.e., `OPENTHREAD_CONFIG_LINK_RAW_ENABLE` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_SOFTWARE_ENERGY_SCAN_ENABLE
#define OPENTHREAD_CONFIG_SOFTWARE_ENERGY_SCAN_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_SOFTWARE_RETRANSMIT_ENABLE
 *
 * Define to 1 if you want to enable software retransmission logic.
 *
 * Applicable only if raw link layer API is enabled (i.e., `OPENTHREAD_CONFIG_LINK_RAW_ENABLE` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_SOFTWARE_RETRANSMIT_ENABLE
#define OPENTHREAD_CONFIG_SOFTWARE_RETRANSMIT_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_SOFTWARE_CSMA_BACKOFF_ENABLE
 *
 * Define to 1 if you want to enable software CSMA-CA backoff logic.
 *
 * Applicable only if raw link layer API is enabled (i.e., `OPENTHREAD_CONFIG_LINK_RAW_ENABLE` is set).
 *
 */
#ifndef OPENTHREAD_CONFIG_SOFTWARE_CSMA_BACKOFF_ENABLE
#define OPENTHREAD_CONFIG_SOFTWARE_CSMA_BACKOFF_ENABLE 1
#endif
#endif // OPENTHREAD_RADIO

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to support microsecond timer in platform.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE 1

/**
 * @def CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER
 *
 * Define to 1 to use DefaultHandler for unhandled requests
 *
 */
#define CLI_COAP_SECURE_USE_COAP_DEFAULT_HANDLER 1

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
 *
 * Define to 1 if you want to enable radio coexistence implemented in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE 1
#endif

#ifndef OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
#define OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE 1
#endif

#endif // OPENTHREAD_CORE_POSIX_CONFIG_H_
