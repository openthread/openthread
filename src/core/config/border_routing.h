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
 * Specifies maximum number of on-mesh prefixes (discovered from Thread Network Data) that are included as Route Info
 * Option in emitted Router Advertisement messages.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_ON_MESH_PREFIXES
#define OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_ON_MESH_PREFIXES 16
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_OLD_ON_LINK_PREFIXES
 *
 * Specifies maximum number of old local on-link prefixes (being deprecated) maintained by routing manager.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_OLD_ON_LINK_PREFIXES
#define OPENTHREAD_CONFIG_BORDER_ROUTING_MAX_OLD_ON_LINK_PREFIXES 3
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTING_ROUTER_ACTIVE_CHECK_TIMEOUT
 *
 * Specifies the timeout in msec for a discovered router on infra link side.
 *
 * This parameter is related to mechanism to check that a discovered router is still active.
 *
 * After this timeout elapses since the last received message (a Router or Neighbor Advertisement) from the router,
 * routing manager will start sending Neighbor Solidification (NS) probes to the router to check that it is still
 * active.
 *
 * This parameter can be considered to large value to practically disable this behavior.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTING_ROUTER_ACTIVE_CHECK_TIMEOUT
#define OPENTHREAD_CONFIG_BORDER_ROUTING_ROUTER_ACTIVE_CHECK_TIMEOUT (60 * 1000) // (in msec).
#endif

#endif // CONFIG_BORDER_ROUTING_H_
