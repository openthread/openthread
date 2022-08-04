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
 * @def OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
 *
 * Define to 1 to enable Border Router support.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE
#define OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
 *
 * Define to 1 to enable mechanism on a Border Router which provides IP connectivity to request router role upgrade.
 *
 * This config is applicable on an `OPENTHREAD_FTD` build and when `OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE` is also
 * enabled.
 *
 * A Border Router is considered to provide external IP connectivity if at least one of the below conditions hold:
 *
 * - It has added at least one external route entry.
 * - It has added at least one prefix entry with default-route and on-mesh flags set.
 * - It has added at least one domain prefix (domain and on-mesh flags set).
 *
 * A Border Router which provides IP connectivity and is acting as a REED is eligible to request a router role upgrade
 * by sending an "Address Solicit" request to leader with status reason `BorderRouterRequest`. This reason is used when
 * the number of active routers in the Thread mesh is above the threshold, and only if the number of existing eligible
 * BRs (determined from the Thread Network Data) that are acting as router is less than two. This mechanism allows up
 * to two eligible Border Routers to request router role upgrade when the number of routers is already above the
 * threshold.
 *
 */
#ifndef OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE
#define OPENTHREAD_CONFIG_BORDER_ROUTER_REQUEST_ROUTER_ROLE 1
#endif

#endif // CONFIG_BORDER_ROUTER_H_
