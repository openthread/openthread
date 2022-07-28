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
#include "common/frame_data.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "net/icmp6.hpp"
#include "net/ip6_address.hpp"
#include "net/ip6_headers.hpp"
#include "net/ip6_mpl.hpp"
#include "net/ip6_types.hpp"
#include "net/netif.hpp"
#include "net/socket.hpp"
#include "net/tcp6.hpp"
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
    friend class ot::TimeTicker;
    friend class Mpl;

public:
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
     * @returns A pointer to the message or `nullptr` if insufficient message buffers are available.
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
     * @returns A pointer to the message or `nullptr` if malformed IPv6 header or insufficient message buffers are
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
     * @returns A pointer to the message or `nullptr` if malformed IPv6 header or insufficient message buffers are
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
     * @retval kErrorNone     Successfully enqueued the message into an output interface.
     * @retval kErrorNoBufs   Insufficient available buffer to add the IPv6 headers.
     *
     */
    Error SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto);

    /**
     * This method sends a raw IPv6 datagram with a fully formed IPv6 header.
     *
     * The caller transfers ownership of @p aMessage when making this call. OpenThread will free @p aMessage when
     * processing is complete, including when a value other than `kErrorNone` is returned.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aFromHost         TRUE if the message is originated from the host, FALSE otherwise.
     *
     * @retval kErrorNone     Successfully processed the message.
     * @retval kErrorDrop     Message was well-formed but not fully processed due to packet processing rules.
     * @retval kErrorNoBufs   Could not allocate necessary message buffers when processing the datagram.
     * @retval kErrorNoRoute  No route to host.
     * @retval kErrorParse    Encountered a malformed header when processing the message.
     *
     */
    Error SendRaw(Message &aMessage, bool aFromHost);

    /**
     * This method processes a received IPv6 datagram.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aNetif            A pointer to the network interface that received the message.
     * @param[in]  aLinkMessageInfo  A pointer to link-specific message information.
     * @param[in]  aFromHost         TRUE if the message is originated from the host, FALSE otherwise.
     *
     * @retval kErrorNone     Successfully processed the message.
     * @retval kErrorDrop     Message was well-formed but not fully processed due to packet processing rules.
     * @retval kErrorNoBufs   Could not allocate necessary message buffers when processing the datagram.
     * @retval kErrorNoRoute  No route to host.
     * @retval kErrorParse    Encountered a malformed header when processing the message.
     *
     */
    Error HandleDatagram(Message &aMessage, Netif *aNetif, const void *aLinkMessageInfo, bool aFromHost);

    /**
     * This method registers a callback to provide received raw IPv6 datagrams.
     *
     * By default, this callback does not pass Thread control traffic.  See SetReceiveIp6FilterEnabled() to change
     * the Thread control traffic filter setting.
     *
     * @param[in]  aCallback         A pointer to a function that is called when an IPv6 datagram is received
     *                               or `nullptr` to disable the callback.
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
     * @returns A pointer to the selected IPv6 source address or `nullptr` if no source address was found.
     *
     */
    const Netif::UnicastAddress *SelectSourceAddress(MessageInfo &aMessageInfo);

    /**
     * This method returns a reference to the send queue.
     *
     * @returns A reference to the send queue.
     *
     */
    const PriorityQueue &GetSendQueue(void) const { return mSendQueue; }

    /**
     * This static method converts an IP protocol number to a string.
     *
     * @param[in] aIpProto  An IP protocol number.
     *
     * @returns The string representation of @p aIpProto.
     *
     */
    static const char *IpProtoToString(uint8_t aIpProto);

    /**
     * This static method converts an IP header ECN value to a string.
     *
     * @param[in] aEcn   The 2-bit ECN value.
     *
     * @returns The string representation of @p aEcn.
     *
     */
    static const char *EcnToString(Ecn aEcn);

private:
    static constexpr uint8_t kDefaultHopLimit      = OPENTHREAD_CONFIG_IP6_HOP_LIMIT_DEFAULT;
    static constexpr uint8_t kIp6ReassemblyTimeout = OPENTHREAD_CONFIG_IP6_REASSEMBLY_TIMEOUT;

    static constexpr uint16_t kMinimalMtu = 1280;

    static void HandleSendQueue(Tasklet &aTasklet);
    void        HandleSendQueue(void);

    static uint8_t PriorityToDscp(Message::Priority aPriority);
    static Error   GetDatagramPriority(const uint8_t *aData, uint16_t aDataLen, Message::Priority &aPriority);

    void  EnqueueDatagram(Message &aMessage);
    Error ProcessReceiveCallback(Message &          aMessage,
                                 const MessageInfo &aMessageInfo,
                                 uint8_t            aIpProto,
                                 bool               aFromHost,
                                 bool               aAllowReceiveFilter,
                                 Message::Ownership aMessageOwnership);
    Error HandleExtensionHeaders(Message &    aMessage,
                                 Netif *      aNetif,
                                 MessageInfo &aMessageInfo,
                                 Header &     aHeader,
                                 uint8_t &    aNextHeader,
                                 bool         aIsOutbound,
                                 bool         aFromHost,
                                 bool &       aReceive);
    Error FragmentDatagram(Message &aMessage, uint8_t aIpProto);
    Error HandleFragment(Message &aMessage, Netif *aNetif, MessageInfo &aMessageInfo, bool aFromHost);
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    void CleanupFragmentationBuffer(void);
    void HandleTimeTick(void);
    void UpdateReassemblyList(void);
    void SendIcmpError(Message &aMessage, Icmp::Header::Type aIcmpType, Icmp::Header::Code aIcmpCode);
#endif
    Error AddMplOption(Message &aMessage, Header &aHeader);
    Error AddTunneledMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo);
    Error InsertMplOption(Message &aMessage, Header &aHeader, MessageInfo &aMessageInfo);
    Error RemoveMplOption(Message &aMessage);
    Error HandleOptions(Message &aMessage, Header &aHeader, bool aIsOutbound, bool &aReceive);
    Error HandlePayload(Header &           aIp6Header,
                        Message &          aMessage,
                        MessageInfo &      aMessageInfo,
                        uint8_t            aIpProto,
                        Message::Ownership aMessageOwnership);
    bool  ShouldForwardToThread(const MessageInfo &aMessageInfo, bool aFromHost) const;
    bool  IsOnLink(const Address &aAddress) const;

    bool                 mForwardingEnabled;
    bool                 mIsReceiveIp6FilterEnabled;
    otIp6ReceiveCallback mReceiveIp6DatagramCallback;
    void *               mReceiveIp6DatagramCallbackContext;

    PriorityQueue mSendQueue;
    Tasklet       mSendQueueTask;

    Icmp mIcmp;
    Udp  mUdp;
    Mpl  mMpl;

#if OPENTHREAD_CONFIG_TCP_ENABLE
    Tcp mTcp;
#endif

#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    MessageQueue mReassemblyList;
#endif
};

/**
 * This class represents parsed IPv6 header along with UDP/TCP/ICMP6 headers from a received message/frame.
 *
 */
class Headers : private Clearable<Headers>
{
public:
    /**
     * This method parses the IPv6 and UDP/TCP/ICMP6 headers from a given message.
     *
     * @param[in] aMessage   The message to parse the headers from.
     *
     * @retval kErrorNone    The headers are parsed successfully.
     * @retval kErrorParse   Failed to parse the headers.
     *
     */
    Error ParseFrom(const Message &aMessage);

    /**
     * This method decompresses lowpan frame and parses the IPv6 and UDP/TCP/ICMP6 headers.
     *
     * @param[in]  aMessage         The message from which to read the lowpan frame.
     * @param[in]  aOffset          The offset in @p aMessage to start reading the frame.
     * @param[in]  aMacSource       The MAC source address.
     * @param[in]  aMacDest         The MAC destination address.
     *
     * @retval kErrorNone           Successfully decompressed and parsed IPv6 and UDP/TCP/ICMP6 headers.
     * @retval kErrorNotFound       Lowpan frame is a next fragment and does not contain IPv6 headers.
     * @retval kErrorParse          Failed to parse the headers.
     *
     */
    Error DecompressFrom(const Message &     aMessage,
                         uint16_t            aOffset,
                         const Mac::Address &aMacSource,
                         const Mac::Address &aMacDest);

    /**
     * This method decompresses lowpan frame and parses the IPv6 and UDP/TCP/ICMP6 headers.
     *
     * @param[in]  aFrameData       The lowpan frame data.
     * @param[in]  aMacSource       The MAC source address.
     * @param[in]  aMacDest         The MAC destination address.
     * @param[in]  aInstance        The OpenThread instance.
     *
     * @retval kErrorNone           Successfully decompressed and parsed IPv6 and UDP/TCP/ICMP6 headers.
     * @retval kErrorNotFound       Lowpan frame is a next fragment and does not contain IPv6 headers.
     * @retval kErrorParse          Failed to parse the headers.
     *
     */
    Error DecompressFrom(const FrameData &   aFrameData,
                         const Mac::Address &aMacSource,
                         const Mac::Address &aMacDest,
                         Instance &          aInstance);

    /**
     * This method returns the IPv6 header.
     *
     * @returns The IPv6 header.
     *
     */
    const Header &GetIp6Header(void) const { return mIp6Header; }

    /**
     * This method returns the IP protocol number from IPv6 Next Header field.
     *
     * @returns The IP protocol number.
     *
     */
    uint8_t GetIpProto(void) const { return mIp6Header.GetNextHeader(); }

    /**
     * This method returns the 2-bit Explicit Congestion Notification (ECN) from Traffic Class field from IPv6 header.
     *
     * @returns The ECN value.
     *
     */
    Ecn GetEcn(void) const { return mIp6Header.GetEcn(); }

    /**
     * This method indicates if the protocol number from IPv6 header is UDP.
     *
     * @retval TRUE   If the protocol number in IPv6 header is UDP.
     * @retval FALSE  If the protocol number in IPv6 header is not UDP.
     *
     */
    bool IsUdp(void) const { return GetIpProto() == kProtoUdp; }

    /**
     * This method indicates if the protocol number from IPv6 header is TCP.
     *
     * @retval TRUE   If the protocol number in IPv6 header is TCP.
     * @retval FALSE  If the protocol number in IPv6 header is not TCP.
     *
     */
    bool IsTcp(void) const { return GetIpProto() == kProtoTcp; }

    /**
     * This method indicates if the protocol number from IPv6 header is ICMPv6.
     *
     * @retval TRUE   If the protocol number in IPv6 header is ICMPv6.
     * @retval FALSE  If the protocol number in IPv6 header is not ICMPv6.
     *
     */
    bool IsIcmp6(void) const { return GetIpProto() == kProtoIcmp6; }

    /**
     * This method returns the source IPv6 address from IPv6 header.
     *
     * @returns The source IPv6 address.
     *
     */
    const Address &GetSourceAddress(void) const { return mIp6Header.GetSource(); }

    /**
     * This method returns the destination IPv6 address from IPv6 header.
     *
     * @returns The destination IPv6 address.
     *
     */
    const Address &GetDestinationAddress(void) const { return mIp6Header.GetDestination(); }

    /**
     * This method returns the UDP header.
     *
     * This method MUST be used when `IsUdp() == true`. Otherwise its behavior is undefined
     *
     * @returns The UDP header.
     *
     */
    const Udp::Header &GetUdpHeader(void) const { return mHeader.mUdp; }

    /**
     * This method returns the TCP header.
     *
     * This method MUST be used when `IsTcp() == true`. Otherwise its behavior is undefined
     *
     * @returns The TCP header.
     *
     */
    const Tcp::Header &GetTcpHeader(void) const { return mHeader.mTcp; }

    /**
     * This method returns the ICMPv6 header.
     *
     * This method MUST be used when `IsIcmp6() == true`. Otherwise its behavior is undefined
     *
     * @returns The ICMPv6 header.
     *
     */
    const Icmp::Header &GetIcmpHeader(void) const { return mHeader.mIcmp; }

    /**
     * This method returns the source port number if header is UDP or TCP, or zero otherwise
     *
     * @returns The source port number under UDP / TCP or zero.
     *
     */
    uint16_t GetSourcePort(void) const;

    /**
     * This method returns the destination port number if header is UDP or TCP, or zero otherwise.
     *
     * @returns The destination port number under UDP / TCP or zero.
     *
     */
    uint16_t GetDestinationPort(void) const;

    /**
     * This method returns the checksum values from corresponding UDP, TCP, or ICMPv6 header.
     *
     * @returns The checksum value.
     *
     */
    uint16_t GetChecksum(void) const;

private:
    Header mIp6Header;
    union
    {
        Udp::Header  mUdp;
        Tcp::Header  mTcp;
        Icmp::Header mIcmp;
    } mHeader;
};

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // IP6_HPP_
