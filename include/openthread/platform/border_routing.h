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

#include <openthread/error.h>
#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handles ICMP6 RA messages received on the Thread interface on the platform.
 *
 * The `aMessage` should point to a buffer of a valid ICMPv6 message (without IP headers) with router advertisement as
 * the value of type field of the message.
 *
 * When DHCPv6 PD is disabled, the message will be dropped silently.
 *
 * Note: RA messages will not be forwarded into Thread networks, while for many platforms, RA messages is the way of
 * distributing a prefix and other infomations to the downstream network. The typical usecase of this function is to
 * handle the router advertisement messages sent by the platform as a result of DHCPv6 Prefix Delegation.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE`.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aMessage  A pointer to an ICMPv6 RouterAdvertisement message.
 * @param[in] aLength   The length of ICMPv6 RouterAdvertisement message.
 */
extern void otPlatBorderRoutingProcessIcmp6Ra(otInstance *aInstance, const uint8_t *aMessage, uint16_t aLength);

/**
 * Process a prefix received from the DHCPv6 PD Server. The prefix is received on
 * the DHCPv6 PD client callback and provided to the Routing Manager via this
 * API.
 *
 * The prefix lifetime can be updated by calling the function again with updated time values.
 * If the preferred lifetime of the prefix is set to 0, the prefix becomes deprecated.
 * When this function is called multiple times, the smallest prefix is preferred as this rule allows
 * choosing a GUA instead of a ULA.
 *
 * Requires `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE`.
 *
 * @param[in] aInstance   A pointer to an OpenThread instance.
 * @param[in] aPrefixInfo A pointer to the prefix information structure
 */
extern void otPlatBorderRoutingProcessDhcp6PdPrefix(otInstance                            *aInstance,
                                                    const otBorderRoutingPrefixTableEntry *aPrefixInfo);

#ifdef __cplusplus
}
#endif

#endif // OPENTHREAD_PLATFORM_BORDER_ROUTER_H_
