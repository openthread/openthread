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
#include "common/non_copyable.hpp"
#include "common/timer.hpp"
#include "net/icmp6.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_headers.hpp"
#include "net/ip6_mpl.hpp"
#include "net/netif.hpp"
#include "net/socket.hpp"
#include "net/udp6.hpp"

namespace ot {

/**
 * @namespace ot::Ip6
 *
 * @brief
 *   This namespace includes definitions for IPv6 networking.
 *
 */
namespace Ip6 {

using ot::Encoding::BigEndian::HostSwap16;
using ot::Encoding::BigEndian::HostSwap32;

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
class Ip6 : public InstanceLocator, private NonCopyable
{
    friend class ot::Instance;
    friend class Mpl;

public:
    enum : uint16_t
    {
        /**
         * The max datagram length (in bytes) of an IPv6 message.
         *
         */
        kMaxDatagramLength = OPENTHREAD_CONFIG_IP6_MAX_DATAGRAM_LENGTH,

        /**
         * The max datagram length (in bytes) of an unfragmented IPv6 message.
         *
         */
        kMaxAssembledDatagramLength = OPENTHREAD_CONFIG_IP6_MAX_ASSEMBLED_DATAGRAM,
    };

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance   A reference to the otInstance object.
     *
     */
    explicit Ip6(Instance &aInstance);

    /**
     * This method allocates a new message buffer from the buffer pool.
     *
     * @param[in]  aReserved  The number of header bytes to reserve following the IPv6 header.
     * @param[in]  aSettings  The message settings.
     *
     * @returns A pointer to the message or nullptr if insufficient message buffers are available.
     *
     */
    Message *NewMessage(uint16_t aReserved, const Message::Settings &aSettings = Message::Settings::GetDefault());

    /**
     * This method allocates a new message buffer from the buffer pool and writes the IPv6 datagram to the message.
     *
     * @param[in]  aData        A pointer to the IPv6 datagram buffer.
     * @param[in]  aDataLength  The size of the IPV6 datagram buffer pointed by @p aData.
     * @param[in]  aSettings    The message settings.
     *
     * @returns A pointer to the message or nullptr if malformed IPv6 header or insufficient message buffers are
     * available.
     *
     */
    Message *NewMessage(const uint8_t *aData, uint16_t aDataLength, const Message::Settings &aSettings);

    /**
     * This method allocates a new message buffer from the buffer pool and writes the IPv6 datagram to the message.
     *
     * @note The link layer security is enabled and the message priority is obtained from IPv6 message itself.
     *
     * @param[in]  aData        A pointer to the IPv6 datagram buffer.
     * @param[in]  aDataLength  The size of the IPV6 datagram buffer pointed by @p aData.
     *
     * @returns A pointer to the message or nullptr if malformed IPv6 header or insufficient message buffers are
     * available.
     *
     */
    Message *NewMessage(const uint8_t *aData, uint16_t aDataLength);

    /**
     * This method converts the IPv6 DSCP value to message priority level.
     *
     * @param[in]  aDscp  The IPv6 DSCP value.
     *
     * @returns The message priority level.
     *
     */
    static Message::Priority DscpToPriority(uint8_t aDscp);

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
    otError SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto);

    /**
     * This method sends a raw IPv6 datagram with a fully formed IPv6 header.
     *
     * The caller transfers ownership of @p aMessage when making this call. OpenThread will free @p aMessage when
     * processing is complete, including when a value other than `OT_ERROR_NONE` is returned.
     *
     * @param[in]  aMessage          A reference to the message.
     *
     * @retval OT_ERROR_NONE      Successfully processed the message.
     * @retval OT_ERROR_DROP      Message was well-formed but not fully processed due to packet processing rules.
     * @retval OT_ERROR_NO_BUFS   Could not allocate necessary message buffers when processing the datagram.
     * @retval OT_ERROR_NO_ROUTE  No route to host.
     * @retval OT_ERROR_PARSE     Encountered a malformed header when processing the message.
     *
     */
    otError SendRaw(Message &aMessage);

    /**
     * This method processes a received IPv6 datagram.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aNetif            A pointer to the network interface that received the message.
     * @param[in]  aLinkMessageInfo  A pointer to link-specific message information.
     * @param[in]  aFromNcpHost      TRUE if the message was submitted by the NCP host, FALSE otherwise.
     *
     * @retval OT_ERROR_NONE      Successfully processed the message.
     * @retval OT_ERROR_DROP      Message was well-formed but not fully processed due to packet processing rules.
     * @retval OT_ERROR_NO_BUFS   Could not allocate necessary message buffers when processing the datagram.
     * @retval OT_ERROR_NO_ROUTE  No route to host.
     * @retval OT_ERROR_PARSE     Encountered a malformed header when processing the message.
     *
     */
    otError HandleDatagram(Message &aMessage, Netif *aNetif, const void *aLinkMessageInfo, bool aFromNcpHost);

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
                                                uint8_t        aProto);

    /**
     * This method registers a callback to provide received raw IPv6 datagrams.
     *
     * By default, this callback does not pass Thread control traffic.  See SetReceiveIp6FilterEnabled() to change
     * the Thread control traffic filter setting.
     *
     * @param[in]  aCallback         A pointer to a function that is called when an IPv6 datagram is received
     *                               or nullptr to disable the callback.
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
    bool IsReceiveIp6FilterEnabled(void) const { return mIsReceiveIp6FilterEnabled; }

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
    bool IsForwardingEnabled(void) const { return mForwardingEnabled; }

    /**
     * This method enables/disables IPv6 forwarding.
     *
     * @param[in]  aEnable  TRUE to enable IPv6 forwarding, FALSE otherwise.
     *
     */
    void SetForwardingEnabled(bool aEnable) { mForwardingEnabled = aEnable; }

    /**
     * This method perform default source address selection.
     *
     * @param[in]  aMessageInfo  A reference to the message information.
     *
     * @returns A pointer to the selected IPv6 source address or nullptr if no source address was found.
     *
     */
    const NetifUnicastAddress *SelectSourceAddress(MessageInfo &aMessageInfo);

    /**
     * This method returns a reference to the send queue.
     *
     * @returns A reference to the send queue.
     *
     */
    const PriorityQueue &GetSendQueue(void) const { return mSendQueue; }

    /**
     * This static method converts an `IpProto` enumeration to a string.
     *
     * @returns The string representation of an IP protocol enumeration.
     *
     */
    static const char *IpProtoToString(uint8_t aIpProto);

private:
    enum : uint8_t
    {
        kDefaultHopLimit      = OPENTHREAD_CONFIG_IP6_HOP_LIMIT_DEFAULT,
        kIp6ReassemblyTimeout = OPENTHREAD_CONFIG_IP6_REASSEMBLY_TIMEOUT,
    };

    enum : uint16_t
    {
        kMinimalMtu = 1280,
    };

    enum : uint32_t
    {
        kStateUpdatePeriod = 1000,
    };

    static void HandleSendQueue(Tasklet &aTasklet);
    void        HandleSendQueue(void);

    static uint8_t  PriorityToDscp(Message::Priority aPriority);
    static otError  GetDatagramPriority(const uint8_t *aData, uint16_t aDataLen, Message::Priority &aPriority);
    static uint16_t UpdateChecksum(uint16_t aChecksum, const Address &aAddress);

    void    EnqueueDatagram(Message &aMessage);
    otError ProcessReceiveCallback(const Message &    aMessage,
                                   const MessageInfo &aMessageInfo,
                                   uint8_t            aIpProto,
                                   bool               aFromNcpHost);
    otError HandleExtensionHeaders(Message &    aMessage,
                                   Netif *      aNetif,
                                   MessageInfo &aMessageInfo,
                                   Header &     aHeader,
                                   uint8_t &    aNextHeader,
                                   bool         aForward,
                                   bool         aFromNcpHost,
                                   bool         aReceive);
    otError FragmentDatagram(Message &aMessage, uint8_t aIpProto);
    otError HandleFragment(Message &aMessage, Netif *aNetif, MessageInfo &aMessageInfo, bool aFromNcpHost);
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    void        CleanupFragmentationBuffer(void);
    void        HandleUpdateTimer(void);
    void        UpdateReassemblyList(void);
    void        SendIcmpError(Message &aMessage, Icmp::Header::Type aIcmpType, Icmp::Header::Code aIcmpCode);
    static void HandleTimer(Timer &aTimer);
#endif
    otError AddMplOption(Message &aMessage, Header &aHeader);
    otError AddTunneledMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo);
    otError InsertMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo);
    otError RemoveMplOption(Message &aMessage);
    otError HandleOptions(Message &aMessage, Header &aHeader, bool &aForward);
    otError HandlePayload(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto);
    bool    ShouldForwardToThread(const MessageInfo &aMessageInfo) const;
    bool    IsOnLink(const Address &aAddress) const;

    bool                 mForwardingEnabled;
    bool                 mIsReceiveIp6FilterEnabled;
    otIp6ReceiveCallback mReceiveIp6DatagramCallback;
    void *               mReceiveIp6DatagramCallbackContext;

    PriorityQueue mSendQueue;
    Tasklet       mSendQueueTask;

    Icmp mIcmp;
    Udp  mUdp;
    Mpl  mMpl;

#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    TimerMilli   mTimer;
    MessageQueue mReassemblyList;
#endif
};

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // IP6_HPP_
