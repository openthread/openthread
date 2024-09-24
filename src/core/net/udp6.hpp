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
 *   This file includes definitions for UDP/IPv6 sockets.
 */

#ifndef UDP6_HPP_
#define UDP6_HPP_

#include "openthread-core-config.h"

#include <openthread/udp.h>
#include <openthread/platform/udp.h>

#include "common/as_core_type.hpp"
#include "common/callback.hpp"
#include "common/clearable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "net/ip6_headers.hpp"

namespace ot {
namespace Ip6 {

class Udp;

/**
 * @addtogroup core-udp
 *
 * @brief
 *   This module includes definitions for UDP/IPv6 sockets.
 *
 * @{
 */

#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE && OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
#error "OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE and OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE must not both be set."
#endif

/**
 * Defines the network interface identifiers.
 */
enum NetifIdentifier : uint8_t
{
    kNetifUnspecified = OT_NETIF_UNSPECIFIED, ///< Unspecified network interface.
    kNetifThread      = OT_NETIF_THREAD,      ///< The Thread interface.
    kNetifBackbone    = OT_NETIF_BACKBONE,    ///< The Backbone interface.
};

/**
 * Implements core UDP message handling.
 */
class Udp : public InstanceLocator, private NonCopyable
{
public:
    typedef otUdpReceive ReceiveHandler; ///< Receive handler callback.

    /**
     * Implements a UDP/IPv6 socket.
     */
    class SocketHandle : public otUdpSocket, public LinkedListEntry<SocketHandle>, public Clearable<SocketHandle>
    {
        friend class Udp;
        friend class LinkedList<SocketHandle>;

    public:
        /**
         * Indicates whether or not the socket is bound.
         *
         * @retval TRUE if the socket is bound (i.e. source port is non-zero).
         * @retval FALSE if the socket is not bound (source port is zero).
         */
        bool IsBound(void) const { return mSockName.mPort != 0; }

        /**
         * Returns the local socket address.
         *
         * @returns A reference to the local socket address.
         */
        SockAddr &GetSockName(void) { return AsCoreType(&mSockName); }

        /**
         * Returns the local socket address.
         *
         * @returns A reference to the local socket address.
         */
        const SockAddr &GetSockName(void) const { return AsCoreType(&mSockName); }

        /**
         * Returns the peer's socket address.
         *
         * @returns A reference to the peer's socket address.
         */
        SockAddr &GetPeerName(void) { return AsCoreType(&mPeerName); }

        /**
         * Returns the peer's socket address.
         *
         * @returns A reference to the peer's socket address.
         */
        const SockAddr &GetPeerName(void) const { return AsCoreType(&mPeerName); }

    private:
        bool Matches(const MessageInfo &aMessageInfo) const;

        void HandleUdpReceive(Message &aMessage, const MessageInfo &aMessageInfo)
        {
            mHandler(mContext, &aMessage, &aMessageInfo);
        }
    };

    /**
     * Implements a UDP/IPv6 socket.
     */
    class Socket : public InstanceLocator, public SocketHandle
    {
        friend class Udp;

    public:
        /**
         * Initializes the object.
         *
         * @param[in]  aInstance  A reference to OpenThread instance.
         * @param[in]  aHandler  A pointer to a function that is called when receiving UDP messages.
         * @param[in]  aContext  A pointer to arbitrary context information.
         */
        Socket(Instance &aInstance, ReceiveHandler aHandler, void *aContext);

        /**
         * Returns a new UDP message with default settings (link security enabled and `kPriorityNormal`)
         *
         * @returns A pointer to the message or `nullptr` if no buffers are available.
         */
        Message *NewMessage(void);

        /**
         * Returns a new UDP message with default settings (link security enabled and `kPriorityNormal`)
         *
         * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
         *
         * @returns A pointer to the message or `nullptr` if no buffers are available.
         */
        Message *NewMessage(uint16_t aReserved);

        /**
         * Returns a new UDP message with sufficient header space reserved.
         *
         * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
         * @param[in]  aSettings  The message settings (default is used if not provided).
         *
         * @returns A pointer to the message or `nullptr` if no buffers are available.
         */
        Message *NewMessage(uint16_t aReserved, const Message::Settings &aSettings);

        /**
         * Opens the UDP socket.
         *
         * @retval kErrorNone     Successfully opened the socket.
         * @retval kErrorFailed   Failed to open the socket.
         */
        Error Open(void);

        /**
         * Returns if the UDP socket is open.
         *
         * @returns If the UDP socket is open.
         */
        bool IsOpen(void) const;

        /**
         * Binds the UDP socket.
         *
         * @param[in]  aSockAddr            A reference to the socket address.
         * @param[in]  aNetifIdentifier     The network interface identifier.
         *
         * @retval kErrorNone            Successfully bound the socket.
         * @retval kErrorInvalidArgs     Unable to bind to Thread network interface with the given address.
         * @retval kErrorFailed          Failed to bind UDP Socket.
         */
        Error Bind(const SockAddr &aSockAddr, NetifIdentifier aNetifIdentifier = kNetifThread);

        /**
         * Binds the UDP socket.
         *
         * @param[in]  aPort                A port number.
         * @param[in]  aNetifIdentifier     The network interface identifier.
         *
         * @retval kErrorNone            Successfully bound the socket.
         * @retval kErrorFailed          Failed to bind UDP Socket.
         */
        Error Bind(uint16_t aPort, NetifIdentifier aNetifIdentifier = kNetifThread);

        /**
         * Binds the UDP socket.
         *
         * @retval kErrorNone    Successfully bound the socket.
         * @retval kErrorFailed  Failed to bind UDP Socket.
         */
        Error Bind(void) { return Bind(0); }

        /**
         * Connects the UDP socket.
         *
         * @param[in]  aSockAddr  A reference to the socket address.
         *
         * @retval kErrorNone    Successfully connected the socket.
         * @retval kErrorFailed  Failed to connect UDP Socket.
         */
        Error Connect(const SockAddr &aSockAddr);

        /**
         * Connects the UDP socket.
         *
         * @param[in]  aPort        A port number.
         *
         * @retval kErrorNone    Successfully connected the socket.
         * @retval kErrorFailed  Failed to connect UDP Socket.
         */
        Error Connect(uint16_t aPort);

        /**
         * Connects the UDP socket.
         *
         * @retval kErrorNone    Successfully connected the socket.
         * @retval kErrorFailed  Failed to connect UDP Socket.
         */
        Error Connect(void) { return Connect(0); }

        /**
         * Closes the UDP socket.
         *
         * @retval kErrorNone    Successfully closed the UDP socket.
         * @retval kErrorFailed  Failed to close UDP Socket.
         */
        Error Close(void);

        /**
         * Sends a UDP message.
         *
         * @param[in]  aMessage      The message to send.
         * @param[in]  aMessageInfo  The message info associated with @p aMessage.
         *
         * @retval kErrorNone         Successfully sent the UDP message.
         * @retval kErrorInvalidArgs  If no peer is specified in @p aMessageInfo or by Connect().
         * @retval kErrorNoBufs       Insufficient available buffer to add the UDP and IPv6 headers.
         */
        Error SendTo(Message &aMessage, const MessageInfo &aMessageInfo);

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        /**
         * Configures the UDP socket to join a multicast group on a Host network interface.
         *
         * @param[in]  aNetifIdentifier     The network interface identifier.
         * @param[in]  aAddress             The multicast group address.
         *
         * @retval  kErrorNone    Successfully joined the multicast group.
         * @retval  kErrorFailed  Failed to join the multicast group.
         */
        Error JoinNetifMulticastGroup(NetifIdentifier aNetifIdentifier, const Address &aAddress);

        /**
         * Configures the UDP socket to leave a multicast group on a Host network interface.
         *
         * @param[in]  aNetifIdentifier     The network interface identifier.
         * @param[in]  aAddress             The multicast group address.
         *
         * @retval  kErrorNone   Successfully left the multicast group.
         * @retval  kErrorFailed Failed to leave the multicast group.
         */
        Error LeaveNetifMulticastGroup(NetifIdentifier aNetifIdentifier, const Address &aAddress);
#endif
    };

    /**
     * A socket owned by a specific type with a given  owner type as the callback.
     *
     * @tparam Owner                The type of the owner of this socket.
     * @tparam HandleUdpReceivePtr  A pointer to a non-static member method of `Owner` to handle received messages.
     */
    template <typename Owner, void (Owner::*HandleUdpReceivePtr)(Message &aMessage, const MessageInfo &aMessageInfo)>
    class SocketIn : public Socket
    {
    public:
        /**
         * Initializes the socket.
         *
         * @param[in]  aInstance   The OpenThread instance.
         * @param[in]  aOnwer      The owner of the socket, providing the `HandleUdpReceivePtr` callback.
         */
        explicit SocketIn(Instance &aInstance, Owner &aOwner)
            : Socket(aInstance, HandleUdpReceive, &aOwner)
        {
        }

    private:
        static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
        {
            (reinterpret_cast<Owner *>(aContext)->*HandleUdpReceivePtr)(AsCoreType(aMessage), AsCoreType(aMessageInfo));
        }
    };

    /**
     * Implements a UDP receiver.
     */
    class Receiver : public otUdpReceiver, public LinkedListEntry<Receiver>
    {
        friend class Udp;

    public:
        /**
         * Initializes the UDP receiver.
         *
         * @param[in]   aHandler     A pointer to the function to handle UDP message.
         * @param[in]   aContext     A pointer to arbitrary context information.
         */
        Receiver(otUdpHandler aHandler, void *aContext)
        {
            mNext    = nullptr;
            mHandler = aHandler;
            mContext = aContext;
        }

    private:
        bool HandleMessage(Message &aMessage, const MessageInfo &aMessageInfo)
        {
            return mHandler(mContext, &aMessage, &aMessageInfo);
        }
    };

    /**
     * Implements UDP header generation and parsing.
     */
    OT_TOOL_PACKED_BEGIN
    class Header : public Clearable<Header>
    {
    public:
        static constexpr uint16_t kSourcePortFieldOffset = 0; ///< Byte offset of Source Port field in UDP header.
        static constexpr uint16_t kDestPortFieldOffset   = 2; ///< Byte offset of Destination Port field in UDP header.
        static constexpr uint16_t kLengthFieldOffset     = 4; ///< Byte offset of Length field in UDP header.
        static constexpr uint16_t kChecksumFieldOffset   = 6; ///< Byte offset of Checksum field in UDP header.

        /**
         * Returns the UDP Source Port.
         *
         * @returns The UDP Source Port.
         */
        uint16_t GetSourcePort(void) const { return BigEndian::HostSwap16(mSourcePort); }

        /**
         * Sets the UDP Source Port.
         *
         * @param[in]  aPort  The UDP Source Port.
         */
        void SetSourcePort(uint16_t aPort) { mSourcePort = BigEndian::HostSwap16(aPort); }

        /**
         * Returns the UDP Destination Port.
         *
         * @returns The UDP Destination Port.
         */
        uint16_t GetDestinationPort(void) const { return BigEndian::HostSwap16(mDestinationPort); }

        /**
         * Sets the UDP Destination Port.
         *
         * @param[in]  aPort  The UDP Destination Port.
         */
        void SetDestinationPort(uint16_t aPort) { mDestinationPort = BigEndian::HostSwap16(aPort); }

        /**
         * Returns the UDP Length.
         *
         * @returns The UDP Length.
         */
        uint16_t GetLength(void) const { return BigEndian::HostSwap16(mLength); }

        /**
         * Sets the UDP Length.
         *
         * @param[in]  aLength  The UDP Length.
         */
        void SetLength(uint16_t aLength) { mLength = BigEndian::HostSwap16(aLength); }

        /**
         * Returns the UDP Checksum.
         *
         * @returns The UDP Checksum.
         */
        uint16_t GetChecksum(void) const { return BigEndian::HostSwap16(mChecksum); }

        /**
         * Sets the UDP Checksum.
         *
         * @param[in]  aChecksum  The UDP Checksum.
         */
        void SetChecksum(uint16_t aChecksum) { mChecksum = BigEndian::HostSwap16(aChecksum); }

    private:
        uint16_t mSourcePort;
        uint16_t mDestinationPort;
        uint16_t mLength;
        uint16_t mChecksum;

    } OT_TOOL_PACKED_END;

    /**
     * Initializes the object.
     *
     * @param[in]  aInstance  A reference to OpenThread instance.
     */
    explicit Udp(Instance &aInstance);

    /**
     * Adds a UDP receiver.
     *
     * @param[in]  aReceiver  A reference to the UDP receiver.
     *
     * @retval kErrorNone    Successfully added the UDP receiver.
     * @retval kErrorAlready The UDP receiver was already added.
     */
    Error AddReceiver(Receiver &aReceiver);

    /**
     * Removes a UDP receiver.
     *
     * @param[in]  aReceiver  A reference to the UDP receiver.
     *
     * @retval kErrorNone       Successfully removed the UDP receiver.
     * @retval kErrorNotFound   The UDP receiver was not added.
     */
    Error RemoveReceiver(Receiver &aReceiver);

    /**
     * Opens a UDP socket.
     *
     * @param[in]  aSocket   A reference to the socket.
     * @param[in]  aHandler  A pointer to a function that is called when receiving UDP messages.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval kErrorNone     Successfully opened the socket.
     * @retval kErrorFailed   Failed to open the socket.
     */
    Error Open(SocketHandle &aSocket, ReceiveHandler aHandler, void *aContext);

    /**
     * Returns if a UDP socket is open.
     *
     * @param[in]  aSocket   A reference to the socket.
     *
     * @returns If the UDP socket is open.
     */
    bool IsOpen(const SocketHandle &aSocket) const { return mSockets.Contains(aSocket); }

    /**
     * Binds a UDP socket.
     *
     * @param[in]  aSocket          A reference to the socket.
     * @param[in]  aSockAddr        A reference to the socket address.
     * @param[in]  aNetifIdentifier The network interface identifier.
     *
     * @retval kErrorNone            Successfully bound the socket.
     * @retval kErrorInvalidArgs     Unable to bind to Thread network interface with the given address.
     * @retval kErrorFailed          Failed to bind UDP Socket.
     */
    Error Bind(SocketHandle &aSocket, const SockAddr &aSockAddr, NetifIdentifier aNetifIdentifier);

    /**
     * Connects a UDP socket.
     *
     * @param[in]  aSocket    A reference to the socket.
     * @param[in]  aSockAddr  A reference to the socket address.
     *
     * @retval kErrorNone    Successfully connected the socket.
     * @retval kErrorFailed  Failed to connect UDP Socket.
     */
    Error Connect(SocketHandle &aSocket, const SockAddr &aSockAddr);

    /**
     * Closes the UDP socket.
     *
     * @param[in]  aSocket    A reference to the socket.
     *
     * @retval kErrorNone    Successfully closed the UDP socket.
     * @retval kErrorFailed  Failed to close UDP Socket.
     */
    Error Close(SocketHandle &aSocket);

    /**
     * Sends a UDP message using a socket.
     *
     * @param[in]  aSocket       A reference to the socket.
     * @param[in]  aMessage      The message to send.
     * @param[in]  aMessageInfo  The message info associated with @p aMessage.
     *
     * @retval kErrorNone         Successfully sent the UDP message.
     * @retval kErrorInvalidArgs  If no peer is specified in @p aMessageInfo or by Connect().
     * @retval kErrorNoBufs       Insufficient available buffer to add the UDP and IPv6 headers.
     */
    Error SendTo(SocketHandle &aSocket, Message &aMessage, const MessageInfo &aMessageInfo);

    /**
     * Returns a new ephemeral port.
     *
     * @returns A new ephemeral port.
     */
    uint16_t GetEphemeralPort(void);

    /**
     * Returns a new UDP message with default settings (link security enabled and `kPriorityNormal`)
     *
     * @returns A pointer to the message or `nullptr` if no buffers are available.
     */
    Message *NewMessage(void);

    /**
     * Returns a new UDP message with default settings (link security enabled and `kPriorityNormal`)
     *
     * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
     *
     * @returns A pointer to the message or `nullptr` if no buffers are available.
     */
    Message *NewMessage(uint16_t aReserved);

    /**
     * Returns a new UDP message with sufficient header space reserved.
     *
     * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
     * @param[in]  aSettings  The message settings.
     *
     * @returns A pointer to the message or `nullptr` if no buffers are available.
     */
    Message *NewMessage(uint16_t aReserved, const Message::Settings &aSettings);

    /**
     * Sends an IPv6 datagram.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval kErrorNone    Successfully enqueued the message into an output interface.
     * @retval kErrorNoBufs  Insufficient available buffer to add the IPv6 headers.
     */
    Error SendDatagram(Message &aMessage, MessageInfo &aMessageInfo);

    /**
     * Handles a received UDP message.
     *
     * @param[in]  aMessage      A reference to the UDP message to process.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval kErrorNone  Successfully processed the UDP message.
     * @retval kErrorDrop  Could not fully process the UDP message.
     */
    Error HandleMessage(Message &aMessage, MessageInfo &aMessageInfo);

    /**
     * Handles a received UDP message with offset set to the payload.
     *
     * @param[in]  aMessage      A reference to the UDP message to process.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     */
    void HandlePayload(Message &aMessage, MessageInfo &aMessageInfo);

    /**
     * Returns the head of UDP Sockets list.
     *
     * @returns A pointer to the head of UDP Socket linked list.
     */
    SocketHandle *GetUdpSockets(void) { return mSockets.GetHead(); }

#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    /**
     * Sets the forward sender.
     *
     * @param[in]   aForwarder  A function pointer to forward UDP packets.
     * @param[in]   aContext    A pointer to arbitrary context information.
     */
    void SetUdpForwarder(otUdpForwarder aForwarder, void *aContext) { mUdpForwarder.Set(aForwarder, aContext); }
#endif

    /**
     * Returns whether a udp port is being used by OpenThread or any of it's optional
     * features, e.g. CoAP API.
     *
     * @param[in]   aPort       The udp port
     *
     * @retval True when port is used by the OpenThread.
     * @retval False when the port is not used by OpenThread.
     */
    bool IsPortInUse(uint16_t aPort) const;

    /**
     * Returns whether a udp port belongs to the platform or the stack.
     *
     * @param[in]   aPort       The udp port
     *
     * @retval True when the port belongs to the platform.
     * @retval False when the port belongs to the stack.
     */
    bool ShouldUsePlatformUdp(uint16_t aPort) const;

private:
    static constexpr uint16_t kDynamicPortMin = 49152; // Service Name and Transport Protocol Port Number Registry
    static constexpr uint16_t kDynamicPortMax = 65535; // Service Name and Transport Protocol Port Number Registry

    // Reserved range for use by SRP server
    static constexpr uint16_t kSrpServerPortMin = OPENTHREAD_CONFIG_SRP_SERVER_UDP_PORT_MIN;
    static constexpr uint16_t kSrpServerPortMax = OPENTHREAD_CONFIG_SRP_SERVER_UDP_PORT_MAX;

    static bool IsPortReserved(uint16_t aPort);

    void AddSocket(SocketHandle &aSocket);
    void RemoveSocket(SocketHandle &aSocket);
#if OPENTHREAD_CONFIG_PLATFORM_UDP_ENABLE
    bool ShouldUsePlatformUdp(const SocketHandle &aSocket) const;
#endif

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    void                SetBackboneSocket(SocketHandle &aSocket);
    const SocketHandle *GetBackboneSockets(void) const;
    bool                IsBackboneSocket(const SocketHandle &aSocket) const;
#endif

    uint16_t                 mEphemeralPort;
    LinkedList<Receiver>     mReceivers;
    LinkedList<SocketHandle> mSockets;
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    SocketHandle *mPrevBackboneSockets;
#endif
#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    Callback<otUdpForwarder> mUdpForwarder;
#endif
};

/**
 * @}
 */

} // namespace Ip6

DefineCoreType(otUdpSocket, Ip6::Udp::SocketHandle);
DefineCoreType(otUdpReceiver, Ip6::Udp::Receiver);
DefineMapEnum(otNetifIdentifier, Ip6::NetifIdentifier);

} // namespace ot

#endif // UDP6_HPP_
