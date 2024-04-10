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
 * @brief
 *   This file includes the platform abstraction for mDNS socket.
 *
 */

#ifndef OPENTHREAD_PLATFORM_MULTICAST_DNS_SOCKET_H_
#define OPENTHREAD_PLATFORM_MULTICAST_DNS_SOCKET_H_

#include <stdint.h>

#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-mdns
 *
 * @brief
 *   This module defines platform APIs for Multicast DNS (mDNS) socket.
 *
 * @{
 *
 */

/**
 * Represents a socket address info.
 *
 */
typedef struct otPlatMdnsAddressInfo
{
    otIp6Address mAddress;      ///< IP address. IPv4-mapped IPv6 format should be used to represent IPv4 address.
    uint16_t     mPort;         ///< Port number.
    uint32_t     mInfraIfIndex; ///< Interface index.
} otPlatMdnsAddressInfo;

/**
 * Enables or disables listening for mDNS messages sent to mDNS port 5353.
 *
 * When listening is enabled, the platform MUST listen for multicast messages sent to UDP destination port 5353 at the
 * mDNS link-local multicast address `224.0.0.251` and its IPv6 equivalent `ff02::fb`.
 *
 * The platform SHOULD also listen for any unicast messages sent to UDP destination port 5353. If this is not possible,
 * then OpenThread mDNS module can be configured to not use any "QU" questions requesting unicast response.
 *
 * While enabled, all received messages MUST be reported back using `otPlatMdnsHandleReceive()` callback.
 *
 * @param[in] aInstance        The OpernThread instance.
 * @param[in] aEnable          Indicate whether to enable or disable.
 * @param[in] aInfraInfIndex   The infrastructure network interface index.
 *
 * @retval OT_ERROR_NONE     Successfully enabled/disabled listening for mDNS messages.
 * @retval OT_ERROR_FAILED   Failed to enable/disable listening for mDNS messages.
 *
 */
otError otPlatMdnsSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex);

/**
 * Sends an mDNS message as multicast.
 *
 * The platform MUST multicast the prepared mDNS message in @p aMessage as a UDP message using the mDNS well-known port
 * number 5353 for both source and destination ports. The message MUST be sent to the mDNS link-local multicast
 * address `224.0.0.251` and/or its IPv6 equivalent `ff02::fb`.
 *
 * @p aMessage contains the mDNS message starting with DNS header at offset zero. It does not include IP or UDP headers.
 * This function passes the ownership of @p aMessage to the platform layer and platform implementation MUST free
 * @p aMessage once sent and no longer needed.
 *
 * The platform MUST allow multicast loopback, i.e., the multicast message @p aMessage MUST also be received and
 * passed back to OpenThread stack using `otPlatMdnsHandleReceive()` callback. This behavior is essential for the
 * OpenThread mDNS stack to process and potentially respond to its own queries, while allowing other mDNS receivers
 * to also receive the query and its response.
 *
 * @param[in] aInstance       The OpenThread instance.
 * @param[in] aMessage        The mDNS message to multicast. Ownership is transferred to the platform layer.
 * @param[in] aInfraIfIndex   The infrastructure network interface index.
 *
 */
void otPlatMdnsSendMulticast(otInstance *aInstance, otMessage *aMessage, uint32_t aInfraIfIndex);

/**
 * Sends an mDNS message as unicast.
 *
 * The platform MUST send the prepared mDNS message in @p aMessage as a UDP message using source UDP port 5353 to
 * the destination address and port number specified by @p aAddress.
 *
 * @p aMessage contains the DNS message starting with the DNS header at offset zero. It does not include IP or UDP
 * headers. This function passes the ownership of @p aMessage to the platform layer and platform implementation
 * MUST free @p aMessage once sent and no longer needed.
 *
 * The @p aAddress fields are as follows:
 *
 * - `mAddress` specifies the destination address. IPv4-mapped IPv6 format is used to represent an IPv4 destination.
 * - `mPort` specifies the destination port.
 * - `mInfraIndex` specifies the interface index.
 *
 * If the @aAddress matches this devices address, the platform MUST ensure to receive and pass the message back to
 * the OpenThread stack using `otPlatMdnsHandleReceive()` for processing.
 *
 * @param[in] aInstance   The OpenThread instance.
 * @param[in] aMessage    The mDNS message to multicast. Ownership is transferred to platform layer.
 * @param[in] aAddress    The destination address info.
 *
 */
void otPlatMdnsSendUnicast(otInstance *aInstance, otMessage *aMessage, const otPlatMdnsAddressInfo *aAddress);

/**
 * Callback to notify OpenThread mDNS module of a received message on UDP port 5353.
 *
 * @p aMessage MUST contain DNS message starting with the DNS header at offset zero. This function passes the
 * ownership of @p aMessage from the platform layer to the OpenThread stack. The OpenThread stack will free the
 * message once processed.
 *
 * The @p aAddress fields are as follows:
 *
 * - `mAddress` specifies the sender's address. IPv4-mapped IPv6 format is used to represent an IPv4 destination.
 * - `mPort` specifies the sender's port.
 * - `mInfraIndex` specifies the interface index.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aMessage     The received mDNS message. Ownership is transferred to the OpenThread stack.
 * @param[in] aIsUnicast   Indicates whether the received message is unicast or multicast.
 * @param[in] aAddress     The sender's address info.
 *
 */
extern void otPlatMdnsHandleReceive(otInstance                  *aInstance,
                                    otMessage                   *aMessage,
                                    bool                         aIsUnicast,
                                    const otPlatMdnsAddressInfo *aAddress);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_MULTICAST_DNS_SOCKET_H_
