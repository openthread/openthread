/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#include "openthread-core-config.h"

#include <stddef.h>

#include <openthread/ip6.h>
#include <openthread/udp.h>

#include "common/encoding.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "net/icmp6.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_headers.hpp"
#include "net/ip6_mpl.hpp"
#include "net/ip6_routes.hpp"
#include "net/netif.hpp"
#include "net/socket.hpp"
#include "net/udp6.hpp"

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

namespace ot {

/**
 * @namespace ot::Ip6
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
class Ip6 : public InstanceLocator
{
public:
    enum
    {
        kDefaultHopLimit   = OPENTHREAD_CONFIG_IPV6_DEFAULT_HOP_LIMIT,
        kMaxDatagramLength = OPENTHREAD_CONFIG_IPV6_DEFAULT_MAX_DATAGRAM,
    };

    /**
     * This method allocates a new message buffer from the buffer pool.
     *
     * @note If @p aSettings is 'NULL', the link layer security is enabled and the message priority is set to
     *       OT_MESSAGE_PRIORITY_NORMAL by default.
     *
     * @param[in]  aReserved  The number of header bytes to reserve following the IPv6 header.
     * @param[in]  aSettings  A pointer to the message settings or NULL to set default settings.
     *
     * @returns A pointer to the message or NULL if insufficient message buffers are available.
     *
     */
    Message *NewMessage(uint16_t aReserved, const otMessageSettings *aSettings = NULL);

    /**
     * This method allocates a new message buffer from the buffer pool and writes the IPv6 datagram to the message.
     *
     * @note If @p aSettings is NULL, the link layer security is enabled and the message priority is obtained from
     *       IPv6 message itself.
     *       If @p aSettings is not NULL, the @p aSetting->mPriority is ignored and obtained from IPv6 message itself.
     *
     * @param[in]  aData        A pointer to the IPv6 datagram buffer.
     * @param[in]  aDataLength  The size of the IPV6 datagram buffer pointed by @p aData.
     * @param[in]  aSettings    A pointer to the message settings or NULL to set default settings.
     *
     * @returns A pointer to the message or NULL if malformed IPv6 header or insufficient message buffers are available.
     *
     */
    Message *NewMessage(const uint8_t *aData, uint16_t aDataLength, const otMessageSettings *aSettings);

    /**
     * This method converts the message priority level to IPv6 DSCP value.
     *
     * @param[in]  aPriority  The message priority level.
     *
     * @returns The IPv6 DSCP value.
     *
     */
    static uint8_t PriorityToDscp(uint8_t aPriority);

    /**
     * This method converts the IPv6 DSCP value to message priority level.
     *
     * @param[in]  aDscp  The IPv6 DSCP value.
     *
     * @returns The message priority level.
     *
     */
    static uint8_t DscpToPriority(uint8_t aDscp);

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance   A reference to the otInstance object.
     *
     */
    explicit Ip6(Instance &aInstance);

    /**
     * This method sends an IPv6 datagram.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aIpProto      The Internet Protocol value.
     *
     * @retval OT_ERROR_NONE     Successfully enqueued the message into an output interface.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffer to add the IPv6 headers.
     *
     */
    otError SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, IpProto aIpProto);

    /**
     * This method sends a raw IPv6 datagram with a fully formed IPv6 header.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aInterfaceId      The interface identifier of the network interface that received the message.
     *
     * @retval OT_ERROR_NONE   Successfully processed the message.
     * @retval OT_ERROR_DROP   Message processing failed and the message should be dropped.
     *
     */
    otError SendRaw(Message &aMessage, int8_t aInterfaceId);

    /**
     * This method processes a received IPv6 datagram.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aNetif            A pointer to the network interface that received the message.
     * @param[in]  aInterfaceId      The interface identifier of the network interface that received the message.
     * @param[in]  aLinkMessageInfo  A pointer to link-specific message information.
     * @param[in]  aFromNcpHost      TRUE if the message was submitted by the NCP host, FALSE otherwise.
     *
     * @retval OT_ERROR_NONE   Successfully processed the message.
     * @retval OT_ERROR_DROP   Message processing failed and the message should be dropped.
     *
     */
    otError HandleDatagram(Message &   aMessage,
                           Netif *     aNetif,
                           int8_t      aInterfaceId,
                           const void *aLinkMessageInfo,
                           bool        aFromNcpHost);

    /**
     * This methods adds a full IPv6 packet to the transmit queue.
     *
     * @param aMessage A reference to the message.
     */
    void EnqueueDatagram(Message &aMessage);

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
    static uint16_t ComputePseudoheaderChecksum(const Address &aSource,
                                                const Address &aDestination,
                                                uint16_t       aLength,
                                                IpProto        aProto);

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
    void SetReceiveDatagramCallback(otIp6ReceiveCallback aCallback, void *aCallbackContext);

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
    bool IsReceiveIp6FilterEnabled(void) { return mIsReceiveIp6FilterEnabled; }

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
    void SetReceiveIp6FilterEnabled(bool aEnabled) { mIsReceiveIp6FilterEnabled = aEnabled; }

    /**
     * This method indicates whether or not IPv6 forwarding is enabled.
     *
     * @returns TRUE if IPv6 forwarding is enabled, FALSE otherwise.
     *
     */
    bool IsForwardingEnabled(void) { return mForwardingEnabled; }

    /**
     * This method enables/disables IPv6 forwarding.
     *
     * @param[in]  aEnable  TRUE to enable IPv6 forwarding, FALSE otherwise.
     *
     */
    void SetForwardingEnabled(bool aEnable) { mForwardingEnabled = aEnable; }

    /**
     * This method enables the network interface.
     *
     * @param  aNetif  A reference to the network interface.
     *
     * @retval OT_ERROR_NONE     Successfully enabled the network interface.
     * @retval OT_ERROR_ALREADY  The network interface was already enabled.
     *
     */
    otError AddNetif(Netif &aNetif);

    /**
     * This method disables the network interface.
     *
     * @param  aNetif  A reference to the network interface.
     *
     * @retval OT_ERROR_NONE       Successfully disabled the network interface.
     * @retval OT_ERROR_NOT_FOUND  The network interface was already disabled.
     *
     */
    otError RemoveNetif(Netif &aNetif);

    /**
     * This method returns the network interface list.
     *
     * @returns A pointer to the network interface list.
     *
     */
    Netif *GetNetifList(void) { return mNetifListHead; }

    /**
     * This method returns the network interface identified by @p aInterfaceId.
     *
     * @param[in]  aInterfaceId  The network interface ID.
     *
     * @returns A pointer to the network interface or NULL if none is found.
     *
     */
    Netif *GetNetifById(int8_t aInterfaceId);

    /**
     * This method indicates whether or not @p aAddress is assigned to a network interface.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     *
     * @retval TRUE   If the IPv6 address is assigned to a network interface.
     * @retval FALSE  If the IPv6 address is not assigned to any network interface.
     *
     */
    bool IsUnicastAddress(const Address &aAddress);

    /**
     * This method perform default source address selection.
     *
     * @param[in]  aMessageInfo  A reference to the message information.
     *
     * @returns A pointer to the selected IPv6 source address or NULL if no source address was found.
     *
     */
    const NetifUnicastAddress *SelectSourceAddress(MessageInfo &aMessageInfo);

    /**
     * This method determines which network interface @p aAddress is on-link, if any.
     *
     * @param[in]  aAddress  A reference to the IPv6 address.
     *
     * @returns The network interface identifier for the on-link interface or -1 if none is found.
     *
     */
    int8_t GetOnLinkNetif(const Address &aAddress);

    /**
     * This method returns a reference to the send queue.
     *
     * @returns A reference to the send queue.
     *
     */
    const PriorityQueue &GetSendQueue(void) const { return mSendQueue; }

    /**
     * This method returns a reference to the IPv6 route management instance.
     *
     * @returns A reference to the route management instance.
     *
     */
    Routes &GetRoutes(void) { return mRoutes; }

    /**
     * This method returns a reference to the ICMP6 controller instance.
     *
     * @returns A reference to the ICMP6 instance.
     *
     */
    Icmp &GetIcmp(void) { return mIcmp; }

    /**
     * This method returns a reference to the UDP controller instance.
     *
     * @returns A reference to the UDP instance.
     *
     */
    Udp &GetUdp(void) { return mUdp; }

    /**
     * This method returns a reference to the MPL message processing controller instance.
     *
     * @returns A reference to the Mpl instance.
     *
     */
    Mpl &GetMpl(void) { return mMpl; }

    /**
     * This static method converts an `IpProto` enumeration to a string.
     *
     * @returns The string representation of an IP protocol enumeration.
     *
     */
    static const char *IpProtoToString(IpProto aIpProto);

private:
    enum
    {
        kDefaultIp6MessagePriority = Message::kPriorityNormal,
    };

    static void HandleSendQueue(Tasklet &aTasklet);
    void        HandleSendQueue(void);

    static otError GetDatagramPriority(const uint8_t *aData, uint16_t aDataLen, uint8_t &aPriority);

    otError ProcessReceiveCallback(const Message &    aMessage,
                                   const MessageInfo &aMessageInfo,
                                   uint8_t            aIpProto,
                                   bool               aFromNcpHost);
    otError HandleExtensionHeaders(Message &aMessage,
                                   Header & aHeader,
                                   uint8_t &aNextHeader,
                                   bool     aForward,
                                   bool     aReceive);
    otError HandleFragment(Message &aMessage);
    otError AddMplOption(Message &aMessage, Header &aHeader);
    otError AddTunneledMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo);
    otError InsertMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo);
    otError RemoveMplOption(Message &aMessage);
    otError HandleOptions(Message &aMessage, Header &aHeader, bool &aForward);
    otError HandlePayload(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto);
    int8_t  FindForwardInterfaceId(const MessageInfo &aMessageInfo);

    bool                 mForwardingEnabled;
    bool                 mIsReceiveIp6FilterEnabled;
    otIp6ReceiveCallback mReceiveIp6DatagramCallback;
    void *               mReceiveIp6DatagramCallbackContext;
    Netif *              mNetifListHead;

    PriorityQueue mSendQueue;
    Tasklet       mSendQueueTask;

    Routes mRoutes;
    Icmp   mIcmp;
    Udp    mUdp;
    Mpl    mMpl;
};

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // NET_IP6_HPP_
