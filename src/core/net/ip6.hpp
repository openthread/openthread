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
#include <openthread/nat64.h>
#include <openthread/udp.h>

#include "common/callback.hpp"
#include "common/encoding.hpp"
#include "common/frame_data.hpp"
#include "common/locator.hpp"
#include "common/log.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"
#include "common/owned_ptr.hpp"
#include "common/time_ticker.hpp"
#include "common/timer.hpp"
#include "net/checksum.hpp"
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
 */

/**
 * @addtogroup core-ip6-ip6
 *
 * @brief
 *   This module includes definitions for core IPv6 networking.
 *
 * @{
 */

/**
 * Implements the core IPv6 message processing.
 */
class Ip6 : public InstanceLocator, private NonCopyable
{
    friend class ot::Instance;
    friend class ot::TimeTicker;
    friend class Mpl;

public:
    typedef otIp6ReceiveCallback ReceiveCallback; ///< IPv6 receive callback function pointer.

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance   A reference to the otInstance object.
     */
    explicit Ip6(Instance &aInstance);

    /**
     * Allocates a new message buffer from the buffer pool with default settings (link security
     * enabled and `kPriorityMedium`).
     *
     * @returns A pointer to the message or `nullptr` if insufficient message buffers are available.
     */
    Message *NewMessage(void);

    /**
     * Allocates a new message buffer from the buffer pool with default settings (link security
     * enabled and `kPriorityMedium`).
     *
     * @param[in]  aReserved  The number of header bytes to reserve following the IPv6 header.
     *
     * @returns A pointer to the message or `nullptr` if insufficient message buffers are available.
     */
    Message *NewMessage(uint16_t aReserved);

    /**
     * Allocates a new message buffer from the buffer pool.
     *
     * @param[in]  aReserved  The number of header bytes to reserve following the IPv6 header.
     * @param[in]  aSettings  The message settings.
     *
     * @returns A pointer to the message or `nullptr` if insufficient message buffers are available.
     */
    Message *NewMessage(uint16_t aReserved, const Message::Settings &aSettings);

    /**
     * Allocates a new message buffer from the buffer pool and writes the IPv6 datagram to the message.
     *
     * The message priority is always determined from IPv6 message itself (@p aData) and the priority included in
     * @p aSetting is ignored.
     *
     * @param[in]  aData        A pointer to the IPv6 datagram buffer.
     * @param[in]  aDataLength  The size of the IPV6 datagram buffer pointed by @p aData.
     * @param[in]  aSettings    The message settings.
     *
     * @returns A pointer to the message or `nullptr` if malformed IPv6 header or insufficient message buffers are
     *          available.
     */
    Message *NewMessageFromData(const uint8_t *aData, uint16_t aDataLength, const Message::Settings &aSettings);

    /**
     * Converts the IPv6 DSCP value to message priority level.
     *
     * @param[in]  aDscp  The IPv6 DSCP value.
     *
     * @returns The message priority level.
     */
    static Message::Priority DscpToPriority(uint8_t aDscp);

    /**
     * Sends an IPv6 datagram.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aIpProto      The Internet Protocol value.
     *
     * @retval kErrorNone     Successfully enqueued the message into an output interface.
     * @retval kErrorNoBufs   Insufficient available buffer to add the IPv6 headers.
     */
    Error SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, uint8_t aIpProto);

    /**
     * Sends a raw IPv6 datagram with a fully formed IPv6 header.
     *
     * @param[in]  aMessage   An owned pointer to a message (ownership is transferred to the method).
     *
     * @retval kErrorNone     Successfully processed the message.
     * @retval kErrorDrop     Message was well-formed but not fully processed due to packet processing rules.
     * @retval kErrorNoBufs   Could not allocate necessary message buffers when processing the datagram.
     * @retval kErrorNoRoute  No route to host.
     * @retval kErrorParse    Encountered a malformed header when processing the message.
     */
    Error SendRaw(OwnedPtr<Message> aMessage);

    /**
     * Processes a received IPv6 datagram.
     *
     * @param[in]  aMessage          An owned pointer to a message.
     *
     * @retval kErrorNone     Successfully processed the message.
     * @retval kErrorDrop     Message was well-formed but not fully processed due to packet processing rules.
     * @retval kErrorNoBufs   Could not allocate necessary message buffers when processing the datagram.
     * @retval kErrorNoRoute  No route to host.
     * @retval kErrorParse    Encountered a malformed header when processing the message.
     */
    Error HandleDatagram(OwnedPtr<Message> aMessagePtr, bool aIsReassembled = false);

    /**
     * Sets the callback to provide received raw IPv6 datagrams.
     *
     * By default, this callback does not pass Thread control traffic. See `SetReceiveIp6FilterEnabled()` to change
     * the Thread control traffic filter setting.
     *
     * @param[in]  aCallback    The receive callback function. Can be `nullptr` to disable the callback.
     * @param[in]  aContext     A pointer to application-specific context.
     */
    void SetReceiveCallback(ReceiveCallback aCallback, void *aContext) { mReceiveCallback.Set(aCallback, aContext); }

#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    typedef otNat64ReceiveIp4Callback Ip4ReceiveCallback; ///< NAT64 IPv4 receive callback function pointer.

    /**
     * Sets the callback to provide received translated IPv4 datagrams.
     *
     * @param[in]  aCallback  The NAT64 IPv4 callbac, used when translated IPv4 datagram is received.
     * @param[in]  aContext   A pointer to application-specific context.
     */
    void SetNat64ReceiveIp4Callback(Ip4ReceiveCallback aCallback, void *aContext)
    {
        mIp4ReceiveCallback.Set(aCallback, aContext);
    }
#endif

    /**
     * Indicates whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
     * via the callback specified in `SetReceiveIp6Callback()`.
     *
     * @returns  TRUE if Thread control traffic is filtered out, FALSE otherwise.
     */
    bool IsReceiveIp6FilterEnabled(void) const { return mReceiveFilterEnabled; }

    /**
     * Sets whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
     * via the callback specified in `SetReceiveIp6Callback()`.
     *
     * @param[in]  aEnabled  TRUE if Thread control traffic is filtered out, FALSE otherwise.
     */
    void SetReceiveIp6FilterEnabled(bool aEnabled) { mReceiveFilterEnabled = aEnabled; }

    /**
     * Performs default source address selection.
     *
     * @param[in,out]  aMessageInfo  A reference to the message information.
     *
     * @retval  kErrorNone      Found a source address and updated SockAddr of @p aMessageInfo.
     * @retval  kErrorNotFound  No source address was found and @p aMessageInfo is unchanged.
     */
    Error SelectSourceAddress(MessageInfo &aMessageInfo) const;

    /**
     * Performs default source address selection.
     *
     * @param[in]  aDestination  The destination address.
     *
     * @returns A pointer to the selected IPv6 source address or `nullptr` if no source address was found.
     */
    const Address *SelectSourceAddress(const Address &aDestination) const;

    /**
     * Returns a reference to the send queue.
     *
     * @returns A reference to the send queue.
     */
    const PriorityQueue &GetSendQueue(void) const { return mSendQueue; }

    /**
     * Converts an IP protocol number to a string.
     *
     * @param[in] aIpProto  An IP protocol number.
     *
     * @returns The string representation of @p aIpProto.
     */
    static const char *IpProtoToString(uint8_t aIpProto);

    /**
     * Converts an IP header ECN value to a string.
     *
     * @param[in] aEcn   The 2-bit ECN value.
     *
     * @returns The string representation of @p aEcn.
     */
    static const char *EcnToString(Ecn aEcn);

#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE

    typedef otBorderRoutingCounters BrCounters; ///< Border Routing counters.

    /**
     * Returns a reference to the Border Routing counters.
     *
     * @returns A reference to the Border Routing counters.
     */
    const BrCounters &GetBorderRoutingCounters(void) const { return mBrCounters; }

    /**
     * Returns a reference to the Border Routing counters.
     *
     * @returns A reference to the Border Routing counters.
     */
    BrCounters &GetBorderRoutingCounters(void) { return mBrCounters; }

    /**
     * Resets the Border Routing counters.
     */
    void ResetBorderRoutingCounters(void) { ClearAllBytes(mBrCounters); }

#endif // OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE

    /**
     * Enables or disables the filter that drops TMF UDP messages from untrusted origin.
     *
     * @param[in]  aEnabled  TRUE to enable filter, FALSE otherwise.
     */
    void SetTmfOriginFilterEnabled(bool aEnabled) { mTmfOriginFilterEnabled = aEnabled; }

    /**
     * Indicates whether the filter that drops TMF UDP messages from untrusted origin is enabled or not.
     *
     * @returns TRUE if the filter is enabled, FALSE otherwise.
     */
    bool IsTmfOriginFilterEnabled(void) { return mTmfOriginFilterEnabled; }

#endif

private:
    static constexpr uint8_t kDefaultHopLimit   = OPENTHREAD_CONFIG_IP6_HOP_LIMIT_DEFAULT;
    static constexpr uint8_t kReassemblyTimeout = OPENTHREAD_CONFIG_IP6_REASSEMBLY_TIMEOUT;

    static constexpr uint16_t kMinimalMtu = 1280;

    static uint8_t PriorityToDscp(Message::Priority aPriority);
    static Error   TakeOrCopyMessagePtr(OwnedPtr<Message> &aTargetPtr,
                                        OwnedPtr<Message> &aMessagePtr,
                                        Message::Ownership aMessageOwnership);

    void  EnqueueDatagram(Message &aMessage);
    void  HandleSendQueue(void);
    Error PassToHost(OwnedPtr<Message> &aMessagePtr,
                     const Header      &aHeader,
                     uint8_t            aIpProto,
                     bool               aReceive,
                     Message::Ownership aMessageOwnership);
    Error HandleExtensionHeaders(OwnedPtr<Message> &aMessagePtr,
                                 const Header      &aHeader,
                                 uint8_t           &aNextHeader,
                                 bool              &aReceive);
    Error FragmentDatagram(Message &aMessage, uint8_t aIpProto);
    Error HandleFragment(Message &aMessage);
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    void CleanupFragmentationBuffer(void);
    void HandleTimeTick(void);
    void UpdateReassemblyList(void);
    void SendIcmpError(Message &aMessage, Icmp::Header::Type aIcmpType, Icmp::Header::Code aIcmpCode);
#endif
    Error ReadHopByHopHeader(const Message &aMessage, OffsetRange &aOffsetRange, HopByHopHeader &aHbhHeader) const;
    Error AddMplOption(Message &aMessage, Header &aHeader);
    Error PrepareMulticastToLargerThanRealmLocal(Message &aMessage, const Header &aHeader);
    Error InsertMplOption(Message &aMessage, Header &aHeader);
    Error RemoveMplOption(Message &aMessage);
    Error HandleOptions(Message &aMessage, const Header &aHeader, bool &aReceive);
    Error Receive(Header            &aIp6Header,
                  OwnedPtr<Message> &aMessagePtr,
                  uint8_t            aIpProto,
                  Message::Ownership aMessageOwnership);
    bool  IsOnLink(const Address &aAddress) const;
    Error RouteLookup(const Address &aSource, const Address &aDestination) const;
#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    void UpdateBorderRoutingCounters(const Header &aHeader, uint16_t aMessageLength, bool aIsInbound);
#endif

    using SendQueueTask = TaskletIn<Ip6, &Ip6::HandleSendQueue>;

    bool mReceiveFilterEnabled;
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    bool mTmfOriginFilterEnabled;
#endif
    Callback<ReceiveCallback> mReceiveCallback;
#if OPENTHREAD_CONFIG_NAT64_TRANSLATOR_ENABLE
    Callback<Ip4ReceiveCallback> mIp4ReceiveCallback;
#endif
    PriorityQueue mSendQueue;
    SendQueueTask mSendQueueTask;
    Icmp          mIcmp;
    Udp           mUdp;
    Mpl           mMpl;
#if OPENTHREAD_CONFIG_TCP_ENABLE
    Tcp mTcp;
#endif
#if OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE
    MessageQueue mReassemblyList;
#endif
#if OPENTHREAD_CONFIG_IP6_BR_COUNTERS_ENABLE
    BrCounters mBrCounters;
#endif
};

/**
 * Represents parsed IPv6 header along with UDP/TCP/ICMP6 headers from a received message/frame.
 */
class Headers : private Clearable<Headers>
{
    friend class Clearable<Headers>;

public:
    /**
     * Parses the IPv6 and UDP/TCP/ICMP6 headers from a given message.
     *
     * @param[in] aMessage   The message to parse the headers from.
     *
     * @retval kErrorNone    The headers are parsed successfully.
     * @retval kErrorParse   Failed to parse the headers.
     */
    Error ParseFrom(const Message &aMessage);

    /**
     * Decompresses lowpan frame and parses the IPv6 and UDP/TCP/ICMP6 headers.
     *
     * @param[in]  aMessage         The message from which to read the lowpan frame.
     * @param[in]  aOffset          The offset in @p aMessage to start reading the frame.
     * @param[in]  aMacAddrs        The MAC source and destination addresses.
     *
     * @retval kErrorNone           Successfully decompressed and parsed IPv6 and UDP/TCP/ICMP6 headers.
     * @retval kErrorNotFound       Lowpan frame is a next fragment and does not contain IPv6 headers.
     * @retval kErrorParse          Failed to parse the headers.
     */
    Error DecompressFrom(const Message &aMessage, uint16_t aOffset, const Mac::Addresses &aMacAddrs);

    /**
     * Decompresses lowpan frame and parses the IPv6 and UDP/TCP/ICMP6 headers.
     *
     * @param[in]  aFrameData       The lowpan frame data.
     * @param[in]  aMacAddrs        The MAC source and destination addresses.
     * @param[in]  aInstance        The OpenThread instance.
     *
     * @retval kErrorNone           Successfully decompressed and parsed IPv6 and UDP/TCP/ICMP6 headers.
     * @retval kErrorNotFound       Lowpan frame is a next fragment and does not contain IPv6 headers.
     * @retval kErrorParse          Failed to parse the headers.
     */
    Error DecompressFrom(const FrameData &aFrameData, const Mac::Addresses &aMacAddrs, Instance &aInstance);

    /**
     * Returns the IPv6 header.
     *
     * @returns The IPv6 header.
     */
    const Header &GetIp6Header(void) const { return mIp6Header; }

    /**
     * Returns the IP protocol number from IPv6 Next Header field.
     *
     * @returns The IP protocol number.
     */
    uint8_t GetIpProto(void) const { return mIp6Header.GetNextHeader(); }

    /**
     * Returns the 2-bit Explicit Congestion Notification (ECN) from Traffic Class field from IPv6 header.
     *
     * @returns The ECN value.
     */
    Ecn GetEcn(void) const { return mIp6Header.GetEcn(); }

    /**
     * Indicates if the protocol number from IPv6 header is UDP.
     *
     * @retval TRUE   If the protocol number in IPv6 header is UDP.
     * @retval FALSE  If the protocol number in IPv6 header is not UDP.
     */
    bool IsUdp(void) const { return GetIpProto() == kProtoUdp; }

    /**
     * Indicates if the protocol number from IPv6 header is TCP.
     *
     * @retval TRUE   If the protocol number in IPv6 header is TCP.
     * @retval FALSE  If the protocol number in IPv6 header is not TCP.
     */
    bool IsTcp(void) const { return GetIpProto() == kProtoTcp; }

    /**
     * Indicates if the protocol number from IPv6 header is ICMPv6.
     *
     * @retval TRUE   If the protocol number in IPv6 header is ICMPv6.
     * @retval FALSE  If the protocol number in IPv6 header is not ICMPv6.
     */
    bool IsIcmp6(void) const { return GetIpProto() == kProtoIcmp6; }

    /**
     * Returns the source IPv6 address from IPv6 header.
     *
     * @returns The source IPv6 address.
     */
    const Address &GetSourceAddress(void) const { return mIp6Header.GetSource(); }

    /**
     * Returns the destination IPv6 address from IPv6 header.
     *
     * @returns The destination IPv6 address.
     */
    const Address &GetDestinationAddress(void) const { return mIp6Header.GetDestination(); }

    /**
     * Returns the UDP header.
     *
     * MUST be used when `IsUdp() == true`. Otherwise its behavior is undefined
     *
     * @returns The UDP header.
     */
    const Udp::Header &GetUdpHeader(void) const { return mHeader.mUdp; }

    /**
     * Returns the TCP header.
     *
     * MUST be used when `IsTcp() == true`. Otherwise its behavior is undefined
     *
     * @returns The TCP header.
     */
    const Tcp::Header &GetTcpHeader(void) const { return mHeader.mTcp; }

    /**
     * Returns the ICMPv6 header.
     *
     * MUST be used when `IsIcmp6() == true`. Otherwise its behavior is undefined
     *
     * @returns The ICMPv6 header.
     */
    const Icmp::Header &GetIcmpHeader(void) const { return mHeader.mIcmp; }

    /**
     * Returns the source port number if header is UDP or TCP, or zero otherwise
     *
     * @returns The source port number under UDP / TCP or zero.
     */
    uint16_t GetSourcePort(void) const;

    /**
     * Returns the destination port number if header is UDP or TCP, or zero otherwise.
     *
     * @returns The destination port number under UDP / TCP or zero.
     */
    uint16_t GetDestinationPort(void) const;

    /**
     * Returns the checksum values from corresponding UDP, TCP, or ICMPv6 header.
     *
     * @returns The checksum value.
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
 */

} // namespace Ip6
} // namespace ot

#endif // IP6_HPP_
