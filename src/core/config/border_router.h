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
 *   This file includes compile-time configurations for Border Router services.
 *
 */

#ifndef CONFIG_BORDER_ROUTER_H_
#define CONFIG_BORDER_ROUTER_H_

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
 *
 * Define to 1 to enable Border Agent support.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE
#define OPENTHREAD_CONFIG_BORDER_AGENT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
 *
 * Define to 1 to enable Border Router support.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#define OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
 *
 * Define to 1 to enable Border Routing support.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
#define OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES
 *
 * Specifies maximum number of discovered prefixes (on-link prefixes on the infra link) maintained by routing manager.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES
#define OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_DISCOVERED_PREFIXES 8
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
 *
 * Define to 1 to enable Border Routing NAT64 support.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE
#define OPENTHREAD_CONFIG_BORDER_ROUTING_NAT64_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT
 *
 * Specifies the Border Agent UDP port, and use 0 for ephemeral port.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT
#define OPENTHREAD_CONFIG_BORDER_AGENT_UDP_PORT 0
#endif

#endif // CONFIG_BORDER_ROUTER_H_
