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
 * This class implements a UDP receiver.
 *
 */
class UdpReceiver : public otUdpReceiver
{
    friend class Udp;

public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]   aUdpHandler     A pointer to the function to handle UDP message.
     * @param[in]   aContext        A pointer to arbitrary context information.
     *
     */
    UdpReceiver(otUdpHandler aHandler, void *aContext)
    {
        mNext    = NULL;
        mHandler = aHandler;
        mContext = aContext;
    }

private:
    UdpReceiver *GetNext(void) { return static_cast<UdpReceiver *>(mNext); }
    void         SetNext(UdpReceiver *aReceiver) { mNext = static_cast<otUdpReceiver *>(aReceiver); }

    bool HandleMessage(Message &aMessage, const MessageInfo &aMessageInfo)
    {
        return mHandler(mContext, &aMessage, &aMessageInfo);
    }
};

/**
 * This class implements a UDP/IPv6 socket.
 *
 */
class UdpSocket : public otUdpSocket, public InstanceLocator
{
    friend class Udp;

public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aUdp  A reference to the UDP transport object.
     *
     */
    explicit UdpSocket(Udp &aUdp);

    /**
     * This method returns a new UDP message with sufficient header space reserved.
     *
     * @note If @p aSettings is 'NULL', the link layer security is enabled and the message priority is set to
     * OT_MESSAGE_PRIORITY_NORMAL by default.
     *
     * @param[in]  aReserved  The number of header bytes to reserve after the UDP header.
     * @param[in]  aSettings  A pointer to the message settings or NULL to set default settings.
     *
     * @returns A pointer to the message or NULL if no buffers are available.
     *
     */
    Message *NewMessage(uint16_t aReserved, const otMessageSettings *aSettings = NULL);

    /**
     * This method opens the UDP socket.
     *
     * @param[in]  aHandler  A pointer to a function that is called when receiving UDP messages.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE     Successfully opened the socket.
     * @retval OT_ERROR_ALREADY  The socket is already open.
     *
     */
    otError Open(otUdpReceive aHandler, void *aContext);

    /**
     * This method binds the UDP socket.
     *
     * @param[in]  aSockAddr  A reference to the socket address.
     *
     * @retval OT_ERROR_NONE    Successfully bound the socket.
     * @retval OT_ERROR_FAILED  Failed to bind UDP Socket.
     *
     */
    otError Bind(const SockAddr &aSockAddr);

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
     * @retval OT_ERROR_INVALID_ARGS  If no peer is specified in @p aMessageInfo or by connect().
     * @retval OT_ERROR_NO_BUFS       Insufficient available buffer to add the UDP and IPv6 headers.
     *
     */
    otError SendTo(Message &aMessage, const MessageInfo &aMessageInfo);

    /**
     * This method returns the local socket address.
     *
     * @returns A reference to the local socket address.
     *
     */
    SockAddr &GetSockName(void) { return *static_cast<SockAddr *>(&mSockName); }

    /**
     * This method returns the peer's socket address.
     *
     * @returns A reference to the peer's socket address.
     *
     */
    SockAddr &GetPeerName(void) { return *static_cast<SockAddr *>(&mPeerName); }

private:
    UdpSocket *GetNext(void) { return static_cast<UdpSocket *>(mNext); }
    void       SetNext(UdpSocket *socket) { mNext = static_cast<otUdpSocket *>(socket); }

    void HandleUdpReceive(Message &aMessage, const MessageInfo &aMessageInfo)
    {
        mHandler(mContext, &aMessage, &aMessageInfo);
    }

    Udp &GetUdp(void);
};

/**
 * This class implements core UDP message handling.
 *
 */
class Udp : public InstanceLocator
{
    friend class UdpSocket;

public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aIp6  A reference to OpenThread instance.
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
    otError AddReceiver(UdpReceiver &aReceiver);

    /**
     * This method removes a UDP receiver.
     *
     * @param[in]  aReceiver  A reference to the UDP receiver.
     *
     * @retval OT_ERROR_NONE        Successfully removed the UDP receiver.
     * @retval OT_ERROR_NOT_FOUND   The UDP receiver was not added.
     *
     */
    otError RemoveReceiver(UdpReceiver &aReceiver);

    /**
     * This method adds a UDP socket.
     *
     * @param[in]  aSocket  A reference to the UDP socket.
     *
     * @retval OT_ERROR_NONE  Successfully added the UDP socket.
     *
     */
    otError AddSocket(UdpSocket &aSocket);

    /**
     * This method removes a UDP socket.
     *
     * @param[in]  aSocket  A reference to the UDP socket.
     *
     * @retval OT_ERROR_NONE  Successfully removed the UDP socket.
     *
     */
    otError RemoveSocket(UdpSocket &aSocket);

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
     * @param[in]  aPriority  The priority of the message.
     *
     * @returns A pointer to the message or NULL if no buffers are available.
     *
     */
    Message *NewMessage(uint16_t aReserved, const otMessageSettings *aSettings = NULL);

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
     * This method updates the UDP checksum.
     *
     * @param[in]  aMessage               A reference to the UDP message.
     * @param[in]  aPseudoHeaderChecksum  The pseudo-header checksum value.
     *
     * @retval OT_ERROR_NONE          Successfully updated the UDP checksum.
     * @retval OT_ERROR_INVALID_ARGS  The message was invalid.
     *
     */
    otError UpdateChecksum(Message &aMessage, uint16_t aPseudoHeaderChecksum);

#if OPENTHREAD_ENABLE_PLATFORM_UDP
    otUdpSocket *GetUdpSockets(void) { return mSockets; }
#endif

#if OPENTHREAD_ENABLE_UDP_FORWARD
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
#endif // OPENTHREAD_ENABLE_UDP_FORWARD

private:
    enum
    {
        kDynamicPortMin = 49152, ///< Service Name and Transport Protocol Port Number Registry
        kDynamicPortMax = 65535, ///< Service Name and Transport Protocol Port Number Registry
    };

    uint16_t     mEphemeralPort;
    UdpReceiver *mReceivers;
    UdpSocket *  mSockets;
#if OPENTHREAD_ENABLE_UDP_FORWARD
    void *         mUdpForwarderContext;
    otUdpForwarder mUdpForwarder;
#endif
};

OT_TOOL_PACKED_BEGIN
struct UdpHeaderPoD
{
    uint16_t mSource;
    uint16_t mDestination;
    uint16_t mLength;
    uint16_t mChecksum;
} OT_TOOL_PACKED_END;

/**
 * This class implements UDP header generation and parsing.
 *
 */
OT_TOOL_PACKED_BEGIN
class UdpHeader : private UdpHeaderPoD
{
public:
    /**
     * This method returns the UDP Source Port.
     *
     * @returns The UDP Source Port.
     *
     */
    uint16_t GetSourcePort(void) const { return HostSwap16(mSource); }

    /**
     * This method sets the UDP Source Port.
     *
     * @param[in]  aPort  The UDP Source Port.
     *
     */
    void SetSourcePort(uint16_t aPort) { mSource = HostSwap16(aPort); }

    /**
     * This method returns the UDP Destination Port.
     *
     * @returns The UDP Destination Port.
     *
     */
    uint16_t GetDestinationPort(void) const { return HostSwap16(mDestination); }

    /**
     * This method sets the UDP Destination Port.
     *
     * @param[in]  aPort  The UDP Destination Port.
     *
     */
    void SetDestinationPort(uint16_t aPort) { mDestination = HostSwap16(aPort); }

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

    /**
     * This static method returns the byte offset for the UDP Length.
     *
     * @returns The byte offset for the UDP Length.
     *
     */
    static uint8_t GetLengthOffset(void) { return offsetof(UdpHeaderPoD, mLength); }

    /**
     * This static method returns the byte offset for the UDP Checksum.
     *
     * @returns The byte offset for the UDP Checksum.
     *
     */
    static uint8_t GetChecksumOffset(void) { return offsetof(UdpHeaderPoD, mChecksum); }

} OT_TOOL_PACKED_END;

/**
 * @}
 *
 */

} // namespace Ip6
} // namespace ot

#endif // NET_UDP6_HPP_
