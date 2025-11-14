/*
 *  Copyright (c) 2020, The OpenThread Authors.
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
 *   This file includes the platform abstraction for the infrastructure network interface.
 */

#ifndef OPENTHREAD_PLATFORM_INFRA_IF_H_
#define OPENTHREAD_PLATFORM_INFRA_IF_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OT_PLAT_INFRA_IF_MAX_LINK_LAYER_ADDR_LENGTH 16 ///< Maximum InfraIf Link-layer address length.

/**
 * Represents an InfraIf Link-Layer Address.
 */
typedef struct otPlatInfraIfLinkLayerAddress
{
    uint8_t mAddress[OT_PLAT_INFRA_IF_MAX_LINK_LAYER_ADDR_LENGTH]; ///< The link-layer address bytes.
    uint8_t mLength;                                               ///< The address length (number of bytes).
} otPlatInfraIfLinkLayerAddress;

/**
 * @addtogroup plat-infra-if
 *
 * @brief
 *   This module includes the platform abstraction for the adjacent infrastructure network interface.
 *
 * @{
 */

/**
 * Tells whether an infra interface has the given IPv6 address assigned.
 *
 * @param[in]  aInfraIfIndex  The index of the infra interface.
 * @param[in]  aAddress       The IPv6 address.
 *
 * @returns  TRUE if the infra interface has given IPv6 address assigned, FALSE otherwise.
 */
bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress);

/**
 * Sends an ICMPv6 Neighbor Discovery message on given infrastructure interface.
 *
 * See RFC 4861: https://tools.ietf.org/html/rfc4861.
 *
 * @param[in]  aInfraIfIndex  The index of the infrastructure interface this message is sent to.
 * @param[in]  aDestAddress   The destination address this message is sent to.
 * @param[in]  aBuffer        The ICMPv6 message buffer. The ICMPv6 checksum is left zero and the
 *                            platform should do the checksum calculate.
 * @param[in]  aBufferLength  The length of the message buffer.
 *
 * @note  Per RFC 4861, the implementation should send the message with IPv6 link-local source address
 *        of interface @p aInfraIfIndex and IP Hop Limit 255.
 *
 * @retval OT_ERROR_NONE    Successfully sent the ICMPv6 message.
 * @retval OT_ERROR_FAILED  Failed to send the ICMPv6 message.
 */
otError otPlatInfraIfSendIcmp6Nd(uint32_t            aInfraIfIndex,
                                 const otIp6Address *aDestAddress,
                                 const uint8_t      *aBuffer,
                                 uint16_t            aBufferLength);

/**
 * The infra interface driver calls this method to notify OpenThread
 * that an ICMPv6 Neighbor Discovery message is received.
 *
 * See RFC 4861: https://tools.ietf.org/html/rfc4861.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aInfraIfIndex  The index of the infrastructure interface on which the ICMPv6 message is received.
 * @param[in]  aSrcAddress    The source address this message is received from.
 * @param[in]  aBuffer        The ICMPv6 message buffer.
 * @param[in]  aBufferLength  The length of the ICMPv6 message buffer.
 *
 * @note  Per RFC 4861, the caller should enforce that the source address MUST be a IPv6 link-local
 *        address and the IP Hop Limit MUST be 255.
 */
extern void otPlatInfraIfRecvIcmp6Nd(otInstance         *aInstance,
                                     uint32_t            aInfraIfIndex,
                                     const otIp6Address *aSrcAddress,
                                     const uint8_t      *aBuffer,
                                     uint16_t            aBufferLength);

/**
 * The infra interface driver calls this method to notify OpenThread
 * of the interface state changes.
 *
 * It is fine for the platform to call to method even when the running state
 * of the interface hasn't changed. In this case, the Routing Manager state is
 * not affected.
 *
 * @param[in]  aInstance          The OpenThread instance structure.
 * @param[in]  aInfraIfIndex      The index of the infrastructure interface.
 * @param[in]  aIsRunning         A boolean that indicates whether the infrastructure
 *                                interface is running.
 *
 * @retval  OT_ERROR_NONE           Successfully updated the infra interface status.
 * @retval  OT_ERROR_INVALID_STATE  The Routing Manager is not initialized.
 * @retval  OT_ERROR_INVALID_ARGS   The @p aInfraIfIndex doesn't match the infra interface the
 *                                  Routing Manager are initialized with.
 */
extern otError otPlatInfraIfStateChanged(otInstance *aInstance, uint32_t aInfraIfIndex, bool aIsRunning);

/**
 * Send a request to discover the NAT64 prefix on the infrastructure interface with @p aInfraIfIndex.
 *
 * OpenThread will call this method periodically to monitor the presence or change of NAT64 prefix.
 *
 * @param[in]  aInfraIfIndex  The index of the infrastructure interface to discover the NAT64 prefix.
 *
 * @retval  OT_ERROR_NONE    Successfully request NAT64 prefix discovery.
 * @retval  OT_ERROR_FAILED  Failed to request NAT64 prefix discovery.
 */
otError otPlatInfraIfDiscoverNat64Prefix(uint32_t aInfraIfIndex);

/**
 * The infra interface driver calls this method to notify OpenThread that
 * the discovery of NAT64 prefix is done.
 *
 * Is expected to be invoked after calling otPlatInfraIfDiscoverNat64Prefix.
 * If no NAT64 prefix is discovered, @p aIp6Prefix shall point to an empty prefix with zero length.
 *
 * @param[in]  aInstance      The OpenThread instance structure.
 * @param[in]  aInfraIfIndex  The index of the infrastructure interface on which the NAT64 prefix is discovered.
 * @param[in]  aIp6Prefix     A pointer to NAT64 prefix.
 */
extern void otPlatInfraIfDiscoverNat64PrefixDone(otInstance        *aInstance,
                                                 uint32_t           aInfraIfIndex,
                                                 const otIp6Prefix *aIp6Prefix);

/**
 * Get the link-layer address of the infrastructure interface.
 *
 * OpenThread invokes this method when the address is required, for example: when generating an ND6 message
 * which includes a source link-layer address option.
 *
 * @param[in]  aInstance                    The OpenThread instance structure.
 * @param[in]  aInfraIfIndex                The index of the infrastructure interface.
 * @param[out] aInfraIfLinkLayerAddress     A pointer to infrastructure interface link-layer address.
 *
 * @retval  OT_ERROR_NONE    Successfully get the infrastructure interface link-layer address.
 * @retval  OT_ERROR_FAILED  Failed to get the infrastructure interface link-layer address.
 */
otError otPlatGetInfraIfLinkLayerAddress(otInstance                    *aInstance,
                                         uint32_t                       aIfIndex,
                                         otPlatInfraIfLinkLayerAddress *aInfraIfLinkLayerAddress);

//---------------------------------------------------------------------------------------------------------------------
// DHCPv6 Prefix Delegation platform APIs (`OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE`)

/**
 * Enables or disables listening for DHCPv6 Prefix Delegation (PD) messages on client.
 *
 * This function is only used when `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE` is enabled.
 *
 * When enabled, the platform must open a UDP socket on the specified infrastructure interface, binding to the DHCPv6
 * client port 546 to receive messages from DHCPv6 servers.
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aEnable        A boolean to enable (`true`) or disable (`false`) listening.
 * @param[in] aInfraIfIndex  The index of the infrastructure interface to operate on.
 */
void otPlatInfraIfDhcp6PdClientSetListeningEnabled(otInstance *aInstance, bool aEnable, uint32_t aInfraIfIndex);

/**
 * Sends a DHCPv6 message to a unicast or multicast destination address.
 *
 * This function is only used when `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE` is enabled.
 *
 * The platform is responsible for constructing a UDP datagram with the given DHCPv6 message as its payload. The
 * datagram must be sent from the DHCPv6 client port (546) to the server port (547) on the specified infrastructure
 * interface. The destination IPv6 address can be a unicast address or the multicast `All_DHCP_Relay_Agents_and_Servers`
 * address (`ff02::1:2`).
 *
 * This function passes the ownership of @p aMessage to the platform layer. Platform MUST then free the message
 * when no longer needed.
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aMessage       An `otMessage` containing the DHCPv6 payload. Ownership is passed to the platform layer.
 * @param[in] aDestAddress   The IPv6 multicast or unicast destination address.
 * @param[in] aInfraIfIndex  The index of the infrastructure interface from which to send the message.
 */
void otPlatInfraIfDhcp6PdClientSend(otInstance   *aInstance,
                                    otMessage    *aMessage,
                                    otIp6Address *aDestAddress,
                                    uint32_t      aInfraIfIndex);

/**
 * Callback from the platform to notify the OpenThread stack of a received DHCPv6 message.
 *
 * This function is provided when `OPENTHREAD_CONFIG_BORDER_ROUTING_DHCP6_PD_CLIENT_ENABLE` is enabled.
 *
 * The platform calls this function whenever a DHCPv6 message is received on the client port (546) while listening on
 * this port is enabled (refer to `otPlatInfraIfDhcp6PdClientSetListeningEnabled()`).
 *
 * The platform is responsible for allocating the `otMessage` to pass the received UDP payload. Ownership of the
 * @p aMessage is passed to the OpenThread stack (which will free it once no longer needed).

 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aMessage       The `otMessage` containing the received DHCPv6 payload. Ownership is passed to OT stack.
 * @param[in] aInfraIfIndex  The index of the infrastructure interface from which the message was received..
 */
extern void otPlatInfraIfDhcp6PdClientHandleReceived(otInstance *aInstance,
                                                     otMessage  *aMessage,
                                                     uint32_t    aInfraIfIndex);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_INFRA_IF_H_
