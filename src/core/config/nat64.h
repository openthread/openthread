/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file includes compile-time configurations for NAT64.
 */

#ifndef CONFIG_NAT64_H_
#define CONFIG_NAT64_H_

/**
 * @addtogroup config-nat64
 *
 * @brief
 *   This module includes configuration variables for NAT64.
 *
 * @{
 */

/**
 * @def OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
 *
 * Define to 1 to enable the internal NAT64 translator.
 */
#ifndef OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
#define OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_NAT64_MAX_MAPPINGS
 *
 * Specifies maximum number of active mappings for NAT64.
 */
#ifndef OPENTHREAD_CONFIG_NAT64_MAX_MAPPINGS
#define OPENTHREAD_CONFIG_NAT64_MAX_MAPPINGS 254
#endif

/**
 * @def OPENTHREAD_CONFIG_NAT64_IDLE_TIMEOUT_SECONDS
 *
 * Specifies timeout in seconds before removing an inactive address mapping.
 */
#ifndef OPENTHREAD_CONFIG_NAT64_IDLE_TIMEOUT_SECONDS
#define OPENTHREAD_CONFIG_NAT64_IDLE_TIMEOUT_SECONDS 7200
#endif

/**
 * @def OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
 *
 * Define to 1 to enable NAT64 support in Border Routing Manager.
 */
#ifndef OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
#define OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE 0
#endif

/**
 * @}
 */

#endif
