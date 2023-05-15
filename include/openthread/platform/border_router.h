/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes the platform abstraction for border routers.
 */

#ifndef OPENTHREAD_PLATFORM_BORDER_ROUTER_H_
#define OPENTHREAD_PLATFORM_BORDER_ROUTER_H_

#include <inttypes.h>

#include <openthread/error.h>
#include <openthread/instance.h>

/**
 * Handles ICMP6 RA messages received on platform network interface.
 *
 * Note: ND messages should not be handled by Thread networks, while for many platforms, ND messages is the way of
 * distributing a prefix and other infomations to the downstream network. The typical usecase of this function is to
 * handle the router advertisement messages sent by the platform as a result of DHCPv6 Prefix Delegation.
 *
 * `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_ENABLE` must be enabled.
 *
 * @param[in] aInstance A pointer to an OpenThread instance.
 * @param[in] aMessage  A pointer to an ICMPv6 RouterAdvertisement message.
 * @param[in] aLength   The length of ICMPv6 RouterAdvertisement message.
 *
 * @retval OT_ERROR_NONE          Successfully processed the prefix infomation in the message.
 * @retval OT_ERROR_PARSE         The given message is not a valid ICMPv6 RA message.
 * @retval OT_ERROR_INVALID_STATE Routing manager is configured to not handling RA.
 *
 */
otError otPlatBorderRoutingProcessIcmp6Ra(otInstance *aInstance, const uint8_t *aMessage, uint16_t aLength);

#endif // OPENTHREAD_PLATFORM_BORDER_ROUTER_H_
