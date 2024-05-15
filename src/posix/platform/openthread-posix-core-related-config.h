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

#ifndef OPENTHREAD_PLATFORM_POSIX_CORE_RELATED_CONFIG_H_
#define OPENTHREAD_PLATFORM_POSIX_CORE_RELATED_CONFIG_H_

#include "openthread-core-config.h"

#ifdef OPENTHREAD_POSIX_CONFIG_FILE
#include OPENTHREAD_POSIX_CONFIG_FILE
#endif

/**
 * @file
 * @brief
 *   This file includes the POSIX platform-specific configurations that are derived from OT core
 *   configurations if not not defined.
 */

/**
 * @def OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
 *
 * Defines `1` to enable the posix implementation of platform/infra_if.h APIs.
 * The default value is set to `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` if it's
 * not explicit defined.
 */
#ifndef OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE
#define OPENTHREAD_POSIX_CONFIG_INFRA_IF_ENABLE \
    (OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE || OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE)
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_MAX_MULTICAST_FORWARDING_CACHE_TABLE
 *
 * This setting configures the maximum number of Multicast Forwarding Cache table for POSIX native multicast routing.
 *
 */
#ifndef OPENTHREAD_POSIX_CONFIG_MAX_MULTICAST_FORWARDING_CACHE_TABLE
#define OPENTHREAD_POSIX_CONFIG_MAX_MULTICAST_FORWARDING_CACHE_TABLE (OPENTHREAD_CONFIG_MAX_MULTICAST_LISTENERS * 10)
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
 *
 * Define as 1 to enable multicast routing support.
 *
 */
#ifndef OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
#define OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE \
    OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
#endif

#if OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE && \
    !OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE
#error \
    "OPENTHREAD_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE is required for OPENTHREAD_POSIX_CONFIG_BACKBONE_ROUTER_MULTICAST_ROUTING_ENABLE"
#endif

/**
 * @def OPENTHREAD_POSIX_CONFIG_MAX_OMR_ROUTES_NUM
 *
 * Defines the max number of OMR routes that can be added to kernel.
 *
 */
#ifndef OPENTHREAD_POSIX_CONFIG_MAX_OMR_ROUTES_NUM
#define OPENTHREAD_POSIX_CONFIG_MAX_OMR_ROUTES_NUM OPENTHREAD_CONFIG_IP6_SLAAC_NUM_ADDRESSES
#endif

#endif // OPENTHREAD_PLATFORM_POSIX_CORE_RELATED_CONFIG_H_
