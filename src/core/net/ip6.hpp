/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file includes definitions for IPv6 packet processing.
 */

#ifndef IP6_HPP_
#define IP6_HPP_

#include <stddef.h>

#include <openthread-types.h>
#include <common/encoding.hpp>
#include <common/message.hpp>
#include <net/icmp6.hpp>
#include <net/ip6_address.hpp>
#include <net/ip6_headers.hpp>
#include <net/ip6_mpl.hpp>
#include <net/netif.hpp>
#include <net/socket.hpp>
#include <net/udp6.hpp>

using Thread::Encoding::BigEndian::HostSwap16;
using Thread::Encoding::BigEndian::HostSwap32;

namespace Thread {

/**
 * @namespace Thread::Ip6
 *
 * @brief
 *   This namespace includes definitions for IPv6 networking.
 *
 */
namespace Ip6 {

/**
 * @addtogroup core-ipv6
 *
 * @brief
 *   This module includes definitions for the IPv6 network layer.
 *
 * @{
 *
 * @defgroup core-ip6-icmp6 ICMPv6
 * @defgroup core-ip6-ip6 IPv6
 * @defgroup core-ip6-mpl MPL
 * @defgroup core-ip6-netif Network Interfaces
 *
 * @}
 *
 */

/**
 * @addtogroup core-ip6-ip6
 *
 * @brief
 *   This module includes definitions for core IPv6 networking.
 *
 * @{
 *
 */

/**
 * This class implements the core IPv6 message processing.
 *
 */
class Ip6
{
public:
    enum
    {
        kDefaultHopLimit = 64,
        kMaxDatagramLength = 1500,
    };

    /**
     * This method allocates a new message buffer from the buffer pool.
     *
     * @param[in]  aReserved  The number of header bytes to reserve following the IPv6 header.
     *
     * @returns A pointer to the message or NULL if insufficient message buffers are available.
     *
     */
    Message *NewMessage(uint16_t aReserved);

    /**
     * This constructor initializes the object.
     */
    Ip6(void);

    /**
     * This method sends an IPv6 datagram.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aIpProto      The Internet Protocol value.
     *
     * @retval kThreadError_None    Successfully enqueued the message into an output interface.
     * @retval kThreadError_NoBufs  Insufficient available buffer to add the IPv6 headers.
     *
     */
    ThreadError SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, IpProto aIpProto);

    /**
     * This method processes a received IPv6 datagram.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aNetif            A pointer to the network interface that received the message.
     * @param[in]  aInterfaceId      The interface identifier of the network interface that received the message.
     * @param[in]  aLinkMessageInfo  A pointer to link-specific message information.
     * @param[in]  aFromNcpHost      TRUE if the message was submitted by the NCP host, FALSE otherwise.
     *
     * @retval kThreadError_None   Successfully processed the message.
     * @retval kThreadError_Drop   Message processing failed and the message should be dropped.
     *
     */
    ThreadError HandleDatagram(Message &aMessage, Netif *aNetif, int8_t aInterfaceId,
                               const void *aLinkMessageInfo, bool aFromNcpHost);

    /**
     * This static method updates a checksum.
     *
     * @param[in]  aChecksum  The checksum value to update.
     * @param[in]  aValue     The 16-bit value to update @p aChecksum with.
     *
     * @returns The updated checksum.
     *
     */
    static uint16_t UpdateChecksum(uint16_t aChecksum, uint16_t aValue);

    /**
     * This static method updates a checksum.
     *
     * @param[in]  aChecksum  The checksum value to update.
     * @param[in]  aBuf       A pointer to a buffer.
     * @param[in]  aLength    The number of bytes in @p aBuf.
     *
     * @returns The updated checksum.
     *
     */
    static uint16_t UpdateChecksum(uint16_t aChecksum, const void *aBuf, uint16_t aLength);

    /**
     * This static method updates a checksum.
     *
     * @param[in]  aChecksum  The checksum value to update.
     * @param[in]  aAddress   A reference to an IPv6 address.
     *
     * @returns The updated checksum.
     *
     */
    static uint16_t UpdateChecksum(uint16_t aChecksum, const Address &aAddress);

    /**
     * This static method computes the pseudoheader checksum.
     *
     * @param[in]  aSource       A reference to the IPv6 source address.
     * @param[in]  aDestination  A reference to the IPv6 destination address.
     * @param[in]  aLength       The IPv6 Payload Length value.
     * @param[in]  aProto        The IPv6 Next Header value.
     *
     * @returns The pseudoheader checksum.
     *
     */
    static uint16_t ComputePseudoheaderChecksum(const Address &aSource, const Address &aDestination,
                                                uint16_t aLength, IpProto aProto);

    /**
     * This method registers a callback to provide received raw IPv6 datagrams.
     *
     * By default, this callback does not pass Thread control traffic.  See SetReceiveIp6FilterEnabled() to change
     * the Thread control traffic filter setting.
     *
     * @param[in]  aCallback         A pointer to a function that is called when an IPv6 datagram is received
     *                               or NULL to disable the callback.
     * @param[in]  aCallbackContext  A pointer to application-specific context.
     *
     * @sa IsReceiveIp6FilterEnabled
     * @sa SetReceiveIp6FilterEnabled
     *
     */
    void SetReceiveDatagramCallback(otReceiveIp6DatagramCallback aCallback, void *aCallbackContext);

    /**
     * This method indicates whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
     * via the callback specified in SetReceiveIp6DatagramCallback().
     *
     * @returns  TRUE if Thread control traffic is filtered out, FALSE otherwise.
     *
     * @sa SetReceiveDatagramCallback
     * @sa SetReceiveIp6FilterEnabled
     *
     */
    bool IsReceiveIp6FilterEnabled(void);

    /**
     * This method sets whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
     * via the callback specified in SetReceiveIp6DatagramCallback().
     *
     * @param[in]  aEnabled  TRUE if Thread control traffic is filtered out, FALSE otherwise.
     *
     * @sa SetReceiveDatagramCallback
     * @sa IsReceiveIp6FilterEnabled
     *
     */
    void SetReceiveIp6FilterEnabled(bool aEnabled);

    /**
     * This method enables/disables IPv6 forwarding.
     *
     * @param[in]  aEnable  TRUE to enable IPv6 forwarding, FALSE otherwise.
     *
     */
    void SetForwardingEnabled(bool aEnable);

    Icmp mIcmp;
    Udp mUdp;

private:
    void ProcessReceiveCallback(const Message &aMessage, const MessageInfo &aMessageInfo, uint8_t aIpProto);
    ThreadError HandleExtensionHeaders(Message &message, uint8_t &nextHeader, bool receive);
    ThreadError HandleFragment(Message &message);
    ThreadError AddMplOption(Message &message, Header &header, IpProto nextHeader, uint16_t payloadLength);
    ThreadError HandleOptions(Message &message);
    ThreadError HandlePayload(Message &message, MessageInfo &messageInfo, uint8_t ipproto);
    ThreadError ForwardMessage(Message &message, MessageInfo &messageInfo);

    Mpl mMpl;
    bool mForwardingEnabled;

    otReceiveIp6DatagramCallback mReceiveIp6DatagramCallback;
    void *mReceiveIp6DatagramCallbackContext;
    bool mIsReceiveIp6FilterEnabled;
};

/**
 * @}
 *
 */

}  // namespace Ip6
}  // namespace Thread

#endif  // NET_IP6_HPP_
