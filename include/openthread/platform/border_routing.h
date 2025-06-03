/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 * @brief
 *   This file includes the platform abstraction for border routing manager.
 */

#ifndef OPENTHREAD_PLATFORM_BORDER_ROUTER_H_
#define OPENTHREAD_PLATFORM_BORDER_ROUTER_H_

#include <stdint.h>

#include <openthread/border_routing.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Callback from the platform to report DHCPv6 Prefix Delegation (PD) prefixes.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` and `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` to
 * be enabled, while `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE` should be disabled.
 *
 * When `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE` is enabled, the OpenThread stack's native DHCPv6
 * PD client will be used. Otherwise, the platform layer is expected to interact with DHCPv6 servers to acquire
 * and provide the delegated prefix(es) using this callback or `otPlatBorderRoutingProcessDhcp6PdPrefix()`.
 *
 * In this function, an ICMPv6 Router Advertisement (received on the platform's Thread interface) is passed to the
 * OpenThread stack. This RA message is intended as a mechanism to distribute DHCPv6 PD prefixes to a Thread Border
 * Router. Each Prefix Information Option (PIO) in the RA is evaluated as a candidate DHCPv6 PD prefix.
 *
 * This function can be called again to renew/refresh the lifetimes of PD prefixes or to signal their deprecation
 * (by setting a zero "preferred lifetime") or removal (by setting a zero "valid lifetime").
 *
 * Important note: it is not expected that the RA message will contain all currently valid PD prefixes. The OT stack
 * will parse the RA and process all included PIOs as PD prefix candidates. Any previously reported PD prefix (from an
 * earlier call to this function or `otPlatBorderRoutingProcessDhcp6PdPrefix()`) that does not appear in the new RA
 * remains unchanged (i.e., it will be assumed valid until its previously indicated lifetime expires).
 *
 * The `aMessage` should point to a buffer containing the ICMPv6 message payload (excluding the IP headers but
 * including the ICMPv6 header) with "Router Advertisement" (code 134) as the value of the `Type` field in the ICMPv6
 * header.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aMessage  A pointer to an ICMPv6 Router Advertisement message.
 * @param[in] aLength   The length of the ICMPv6 Router Advertisement message.
 */
extern void otPlatBorderRoutingProcessIcmp6Ra(otInstance *aInstance, const uint8_t *aMessage, uint16_t aLength);

/**
 * Callback to report a single DHCPv6 Prefix Delegation (PD) prefix.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE` and `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` to
 * be enabled, while `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE` should be disabled.
 *
 * When `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE` is enabled, the OpenThread stack's native DHCPv6
 * PD client will be used. Otherwise, the platform layer is expected to interact with DHCPv6 servers to acquire
 * and provide the delegated prefix(es) using this callback or `otPlatBorderRoutingProcessIcmp6Ra()`.
 *
 * This function can be called again to renew/refresh the lifetimes of PD prefixes or to signal their deprecation
 * (by setting a zero "preferred lifetime") or removal (by setting a zero "valid lifetime").  This function may be
 * called multiple times to provide different PD prefixes.
 *
 * @param[in] aInstance     A pointer to an OpenThread instance.
 * @param[in] aPrefixInfo   A pointer to the prefix information structure.
 */
extern void otPlatBorderRoutingProcessDhcp6PdPrefix(otInstance                            *aInstance,
                                                    const otBorderRoutingPrefixTableEntry *aPrefixInfo);

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_PLATFORM_BORDER_ROUTER_H_
