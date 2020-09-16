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

#include "common/linked_list.hpp"
#include "common/locator.hpp"
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
 *
 */

/**
 * This class implements core UDP message handling.
 *
 */
class Udp : public InstanceLocator
{
public:
    /**
     * This class implements a UDP/IPv6 socket.
     *
     */
    class SocketHandle : public otUdpSocket, public LinkedListEntry<SocketHandle>
    {
        friend class Udp;
        friend class LinkedList<SocketHandle>;

    public:
        /**
         * This method indicates whether or not the socket is bound.
         *
         * @retval TRUE if the socket is bound (i.e. source port is non-zero).
         * @retval FALSE if the socket is not bound (source port is zero).
         *
         */
        bool IsBound(void) const { return mSockName.mPort != 0; }

        /**
         * This method returns the local socket address.
         *
         * @returns A reference to the local socket address.
         *
         */
        SockAddr &GetSockName(void) { return *static_cast<SockAddr *>(&mSockName); }

        /**
         * This method returns the local socket address.
         *
         * @returns A reference to the local socket address.
         *
         */
        const SockAddr &GetSockName(void) const { return *static_cast<const SockAddr *>(&mSockName); }

        /**
         * This method returns the peer's socket address.
         *
         * @returns A reference to the peer's socket address.
         *
         */
        SockAddr &GetPeerName(void) { return *static_cast<SockAddr *>(&mPeerName); }

        /**
         * This method returns the peer's socket address.
         *
         * @returns A reference to the peer's socket address.
         *
         */
        const SockAddr &GetPeerName(void) const { return *static_cast<const SockAddr *>(&mPeerName); }

    private:
        bool Matches(const MessageInfo &aMessageInfo) const;

        void HandleUdpReceive(Message &aMessage, const MessageInfo &aMessageInfo)
        {
            mHandler(mContext, &aMessage, &aMessageInfo);
        }
    };

    /**
     * This class implements a UDP/IPv6 socket.
     *
     */
    class Socket : public InstanceLocator, public SocketHandle
    {
        friend class Udp;

    public:
        /**
         * This constructor initializes the object.
         *
         * @param[in]  aInstance  A reference to OpenThread instance.
         *
         */
        explicit Socket(Instance &aInstance);

        /**
         * This method returns a new UDP message with sufficient header space reserved.
         *
         * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
         * @param[in]  aSettings  The message settings (default is used if not provided).
         *
         * @returns A pointer to the message or nullptr if no buffers are available.
         *
         */
        Message *NewMessage(uint16_t aReserved, const Message::Settings &aSettings = Message::Settings::GetDefault());

        /**
         * This method opens the UDP socket.
         *
         * @param[in]  aHandler  A pointer to a function that is called when receiving UDP messages.
         * @param[in]  aContext  A pointer to arbitrary context information.
         *
         * @retval OT_ERROR_NONE     Successfully opened the socket.
         * @retval OT_ERROR_FAILED   Failed to open the socket.
         *
         */
        otError Open(otUdpReceive aHandler, void *aContext);

        /**
         * This method binds the UDP socket.
         *
         * @param[in]  aSockAddr    A reference to the socket address.
         *
         * @retval OT_ERROR_NONE            Successfully bound the socket.
         * @retval OT_ERROR_INVALID_ARGS    Unable to bind to Thread network interface with the given address.
         * @retval OT_ERROR_FAILED          Failed to bind UDP Socket.
         *
         */
        otError Bind(const SockAddr &aSockAddr);

        /**
         * This method binds the UDP socket to a specified network interface.
         *
         * @param[in]  aNetifIdentifier     The network interface identifier.
         *
         * @retval OT_ERROR_NONE    Successfully bound to the network interface.
         * @retval OT_ERROR_FAILED  Failed to bind to the network interface.
         *
         */
        otError BindToNetif(otNetifIdentifier aNetifIdentifier);

        /**
         * This method binds the UDP socket.
         *
         * @param[in]  aPort        A port number.
         *
         * @retval OT_ERROR_NONE            Successfully bound the socket.
         * @retval OT_ERROR_FAILED          Failed to bind UDP Socket.
         *
         */
        otError Bind(uint16_t aPort);

        /**
         * This method binds the UDP socket.
         *
         * @retval OT_ERROR_NONE    Successfully bound the socket.
         * @retval OT_ERROR_FAILED  Failed to bind UDP Socket.
         *
         */
        otError Bind(void) { return Bind(0); }

        /**
         * This method connects the UDP socket.
         *
         * @param[in]  aSockAddr  A reference to the socket address.
         *
         * @retval OT_ERROR_NONE    Successfully connected the socket.
         * @retval OT_ERROR_FAILED  Failed to connect UDP Socket.
         *
         */
        otError Connect(const SockAddr &aSockAddr);

        /**
         * This method connects the UDP socket.
         *
         * @param[in]  aPort        A port number.
         *
         * @retval OT_ERROR_NONE    Successfully connected the socket.
         * @retval OT_ERROR_FAILED  Failed to connect UDP Socket.
         *
         */
        otError Connect(uint16_t aPort);

        /**
         * This method connects the UDP socket.
         *
         * @retval OT_ERROR_NONE    Successfully connected the socket.
         * @retval OT_ERROR_FAILED  Failed to connect UDP Socket.
         *
         */
        otError Connect(void) { return Connect(0); }

        /**
         * This method closes the UDP socket.
         *
         * @retval OT_ERROR_NONE    Successfully closed the UDP socket.
         * @retval OT_ERROR_FAILED  Failed to close UDP Socket.
         *
         */
        otError Close(void);

        /**
         * This method sends a UDP message.
         *
         * @param[in]  aMessage      The message to send.
         * @param[in]  aMessageInfo  The message info associated with @p aMessage.
         *
         * @retval OT_ERROR_NONE          Successfully sent the UDP message.
         * @retval OT_ERROR_INVALID_ARGS  If no peer is specified in @p aMessageInfo or by Connect().
         * @retval OT_ERROR_NO_BUFS       Insufficient available buffer to add the UDP and IPv6 headers.
         *
         */
        otError SendTo(Message &aMessage, const MessageInfo &aMessageInfo);
    };

    /**
     * This class implements a UDP receiver.
     *
     */
    class Receiver : public otUdpReceiver, public LinkedListEntry<Receiver>
    {
        friend class Udp;

    public:
        /**
         * This constructor initializes the UDP receiver.
         *
         * @param[in]   aUdpHandler     A pointer to the function to handle UDP message.
         * @param[in]   aContext        A pointer to arbitrary context information.
         *
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
     * This class implements UDP header generation and parsing.
     *
     */
    OT_TOOL_PACKED_BEGIN
    class Header
    {
    public:
        enum : uint8_t
        {
            kSourcePortFieldOffset = 0, ///< The byte offset of Source Port field in UDP header.
            kDestPortFieldOffset   = 2, ///< The byte offset of Destination Port field in UDP header.
            kLengthFieldOffset     = 4, ///< The byte offset of Length field in UDP header.
            kChecksumFieldOffset   = 6, ///< The byte offset of Checksum field in UDP header.
        };

        /**
         * This method returns the UDP Source Port.
         *
         * @returns The UDP Source Port.
         *
         */
        uint16_t GetSourcePort(void) const { return HostSwap16(mSourcePort); }

        /**
         * This method sets the UDP Source Port.
         *
         * @param[in]  aPort  The UDP Source Port.
         *
         */
        void SetSourcePort(uint16_t aPort) { mSourcePort = HostSwap16(aPort); }

        /**
         * This method returns the UDP Destination Port.
         *
         * @returns The UDP Destination Port.
         *
         */
        uint16_t GetDestinationPort(void) const { return HostSwap16(mDestinationPort); }

        /**
         * This method sets the UDP Destination Port.
         *
         * @param[in]  aPort  The UDP Destination Port.
         *
         */
        void SetDestinationPort(uint16_t aPort) { mDestinationPort = HostSwap16(aPort); }

        /**
         * This method returns the UDP Length.
         *
         * @returns The UDP Length.
         *
         */
        uint16_t GetLength(void) const { return HostSwap16(mLength); }

        /**
         * This method sets the UDP Length.
         *
         * @param[in]  aLength  The UDP Length.
         *
         */
        void SetLength(uint16_t aLength) { mLength = HostSwap16(aLength); }

        /**
         * This method returns the UDP Checksum.
         *
         * @returns The UDP Checksum.
         *
         */
        uint16_t GetChecksum(void) const { return HostSwap16(mChecksum); }

        /**
         * This method sets the UDP Checksum.
         *
         * @param[in]  aChecksum  The UDP Checksum.
         *
         */
        void SetChecksum(uint16_t aChecksum) { mChecksum = HostSwap16(aChecksum); }

    private:
        uint16_t mSourcePort;
        uint16_t mDestinationPort;
        uint16_t mLength;
        uint16_t mChecksum;

    } OT_TOOL_PACKED_END;

    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A reference to OpenThread instance.
     *
     */
    explicit Udp(Instance &aInstance);

    /**
     * This method adds a UDP receiver.
     *
     * @param[in]  aReceiver  A reference to the UDP receiver.
     *
     * @retval OT_ERROR_NONE    Successfully added the UDP receiver.
     * @retval OT_ERROR_ALREADY The UDP receiver was already added.
     *
     */
    otError AddReceiver(Receiver &aReceiver);

    /**
     * This method removes a UDP receiver.
     *
     * @param[in]  aReceiver  A reference to the UDP receiver.
     *
     * @retval OT_ERROR_NONE        Successfully removed the UDP receiver.
     * @retval OT_ERROR_NOT_FOUND   The UDP receiver was not added.
     *
     */
    otError RemoveReceiver(Receiver &aReceiver);

    /**
     * This method opens a UDP socket.
     *
     * @param[in]  aSocket   A reference to the socket.
     * @param[in]  aHandler  A pointer to a function that is called when receiving UDP messages.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE     Successfully opened the socket.
     * @retval OT_ERROR_FAILED   Failed to open the socket.
     *
     */
    otError Open(SocketHandle &aSocket, otUdpReceive aHandler, void *aContext);

    /**
     * This method binds a UDP socket.
     *
     * @param[in]  aSocket          A reference to the socket.
     * @param[in]  aSockAddr        A reference to the socket address.
     *
     * @retval OT_ERROR_NONE            Successfully bound the socket.
     * @retval OT_ERROR_INVALID_ARGS    Unable to bind to Thread network interface with the given address.
     * @retval OT_ERROR_FAILED          Failed to bind UDP Socket.
     *
     */
    otError Bind(SocketHandle &aSocket, const SockAddr &aSockAddr);

    /**
     * This method binds a UDP socket to the Network interface.
     *
     * @param[in]  aSocket           A reference to the socket.
     * @param[in]  aNetifIdentifier  The network interface identifier.
     *
     */
    void BindToNetif(SocketHandle &aSocket, otNetifIdentifier aNetifIdentifier);

    /**
     * This method connects a UDP socket.
     *
     * @param[in]  aSocket    A reference to the socket.
     * @param[in]  aSockAddr  A reference to the socket address.
     *
     * @retval OT_ERROR_NONE    Successfully connected the socket.
     * @retval OT_ERROR_FAILED  Failed to connect UDP Socket.
     *
     */
    otError Connect(SocketHandle &aSocket, const SockAddr &aSockAddr);

    /**
     * This method closes the UDP socket.
     *
     * @param[in]  aSocket    A reference to the socket.
     *
     * @retval OT_ERROR_NONE    Successfully closed the UDP socket.
     * @retval OT_ERROR_FAILED  Failed to close UDP Socket.
     *
     */
    otError Close(SocketHandle &aSocket);

    /**
     * This method sends a UDP message using a socket.
     *
     * @param[in]  aSocket       A reference to the socket.
     * @param[in]  aMessage      The message to send.
     * @param[in]  aMessageInfo  The message info associated with @p aMessage.
     *
     * @retval OT_ERROR_NONE          Successfully sent the UDP message.
     * @retval OT_ERROR_INVALID_ARGS  If no peer is specified in @p aMessageInfo or by Connect().
     * @retval OT_ERROR_NO_BUFS       Insufficient available buffer to add the UDP and IPv6 headers.
     *
     */
    otError SendTo(SocketHandle &aSocket, Message &aMessage, const MessageInfo &aMessageInfo);

    /**
     * This method returns a new ephemeral port.
     *
     * @returns A new ephemeral port.
     *
     */
    uint16_t GetEphemeralPort(void);

    /**
     * This method returns a new UDP message with sufficient header space reserved.
     *
     * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
     * @param[in]  aSettings  The message settings.
     *
     * @returns A pointer to the message or nullptr if no buffers are available.
     *
     */
    Message *NewMessage(uint16_t aReserved, const Message::Settings &aSettings = Message::Settings::GetDefault());

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
     * This method handles a received UDP message.
     *
     * @param[in]  aMessage      A reference to the UDP message to process.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     * @retval OT_ERROR_NONE  Successfully processed the UDP message.
     * @retval OT_ERROR_DROP  Could not fully process the UDP message.
     *
     */
    otError HandleMessage(Message &aMessage, MessageInfo &aMessageInfo);

    /**
     * This method handles a received UDP message with offset set to the payload.
     *
     * @param[in]  aMessage      A reference to the UDP message to process.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     *
     */
    void HandlePayload(Message &aMessage, MessageInfo &aMessageInfo);

    /**
     * This method returns the head of UDP Sockets list.
     *
     * @returns A pointer to the head of UDP Socket linked list.
     *
     */
    SocketHandle *GetUdpSockets(void) { return mSockets.GetHead(); }

#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    /**
     * This method sets the forward sender.
     *
     * @param[in]   aForwarder  A function pointer to forward UDP packets.
     * @param[in]   aContext    A pointer to arbitrary context information.
     *
     */
    void SetUdpForwarder(otUdpForwarder aForwarder, void *aContext)
    {
        mUdpForwarder        = aForwarder;
        mUdpForwarderContext = aContext;
    }
#endif

private:
    enum
    {
        kDynamicPortMin = 49152, ///< Service Name and Transport Protocol Port Number Registry
        kDynamicPortMax = 65535, ///< Service Name and Transport Protocol Port Number Registry
    };

    void AddSocket(SocketHandle &aSocket);
    void RemoveSocket(SocketHandle &aSocket);
    bool IsMlePort(uint16_t aPort) const;

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    void                SetBackboneSocket(SocketHandle &aSocket);
    const SocketHandle *GetBackboneSockets(void);
#endif

    uint16_t                 mEphemeralPort;
    LinkedList<Receiver>     mReceivers;
    LinkedList<SocketHandle> mSockets;
#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
    SocketHandle *mPrevBackboneSockets;
#endif
#if OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE
    void *         mUdpForwarderContext;
    otUdpForwarder mUdpForwarder;
#endif
};

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // UDP6_HPP_
