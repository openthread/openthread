/*
 *  Copyright (c) 2024, The OpenThread Authors.
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
 *   This file includes compile-time configurations for the Multicast DNS (mDNS).
 */

#ifndef CONFIG_MULTICAST_DNS_H_
#define CONFIG_MULTICAST_DNS_H_

/**
 * @addtogroup config-mdns
 *
 * @brief
 *   This module includes configuration variables for the Multicast DNS (mDNS).
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
 *
 * Define to 1 to enable Multicast DNS (mDNS) support.
 */
#ifndef OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE
#define OPENTHREAD_CONFIG_MULTICAST_DNS_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE
 *
 * Define to 1 to allow public OpenThread APIs to be defined for Multicast DNS (mDNS) module.
 *
 * The OpenThread mDNS module is mainly intended for use by other OT core modules, so the public APIs are by default
 * not provided.
 */
#ifndef OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE
#define OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
 *
 * Define to 1 for mDNS module to provide mechanisms and public APIs to iterate over registered host, service, and
 * key entries, as well as browsers and resolvers.
 */
#ifndef OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE
#define OPENTHREAD_CONFIG_MULTICAST_DNS_ENTRY_ITERATION_API_ENABLE OPENTHREAD_CONFIG_MULTICAST_DNS_PUBLIC_API_ENABLE
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF
 *
 * Define to 1 for mDNS module to be automatically enabled/disabled on the same infra-if used for border routing
 * based on infra-if state.
 */
#ifndef OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF
#define OPENTHREAD_CONFIG_MULTICAST_DNS_AUTO_ENABLE_ON_INFRA_IF OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTICAST_DNS_DEFAULT_QUESTION_UNICAST_ALLOWED
 *
 * Specified the default value for `otMdnsIsQuestionUnicastAllowed()` which indicates whether mDNS core is allowed to
 * send "QU" questions (questions requesting unicast response). When allowed, the first probe will be sent as "QU"
 * question. The `otMdnsSetQuestionUnicastAllowed()` can be used to change the default value at run-time.
 */
#ifndef OPENTHREAD_CONFIG_MULTICAST_DNS_DEFAULT_QUESTION_UNICAST_ALLOWED
#define OPENTHREAD_CONFIG_MULTICAST_DNS_DEFAULT_QUESTION_UNICAST_ALLOWED 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MULTICAST_DNS_MOCK_PLAT_APIS_ENABLE
 *
 * Define to 1 to add mock (empty) implementation of mDNS platform APIs.
 *
 * This is intended for generating code size report only and should not be used otherwise.
 */
#ifndef OPENTHREAD_CONFIG_MULTICAST_DNS_MOCK_PLAT_APIS_ENABLE
#define OPENTHREAD_CONFIG_MULTICAST_DNS_MOCK_PLAT_APIS_ENABLE 0
#endif

/**
 * @}
 */

#endif // CONFIG_MULTICAST_DNS_H_
