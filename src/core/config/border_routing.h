/*
 *  Copyright (c) 2021-22, The OpenThread Authors.
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
 *   This file includes compile-time configurations for Border Routing Manager.
 *
 */

#ifndef CONFIG_BORDER_ROUTING_H_
#define CONFIG_BORDER_ROUTING_H_

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
 *
 * Define to 1 to enable Border Routing Manager feature.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
#define OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_ROUTERS
 *
 * Specifies maximum number of routers (on infra link) to track by routing manager.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_ROUTERS
#define OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_ROUTERS 16
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES
 *
 * Specifies maximum number of discovered prefixes (on-link prefixes on the infra link) maintained by routing manager.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES
#define OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES 64
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_ON_MESH_PREFIXES
 *
 * Specified maximum number of on-mesh prefixes (discovered from Thread Network Data) that are included as Route Info
 * Option in emitted Router Advertisement messages.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_ON_MESH_PREFIXES
#define OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_ON_MESH_PREFIXES 16
#endif

/**
 * @def OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
 *
 * Define to 1 to enable Border Routing NAT64 support.
 *
 */
#ifndef OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE
#define OPENTHREAD_CONFIG_NAT64_BORDER_ROUTING_ENABLE 0
#endif

#endif // CONFIG_BORDER_ROUTING_H_
