/*
 *  Copyright (c) 2017, The OpenThread Authors.
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

#ifndef DNS_CLIENT_HPP_
#define DNS_CLIENT_HPP_

#include "openthread-core-config.h"

#include <openthread/dns.h>

#include "common/message.hpp"
#include "common/timer.hpp"
#include "net/dns_headers.hpp"
#include "net/ip6.hpp"
#include "net/netif.hpp"

/**
 * @file
 *   This file includes definitions for the DNS client.
 */

namespace ot {
namespace Dns {

/**
 * This class implements metadata required for DNS retransmission.
 *
 */
OT_TOOL_PACKED_BEGIN
class QueryMetadata
{
    friend class Client;

public:
    /**
     * Default constructor for the object.
     *
     */
    QueryMetadata(void) { memset(this, 0, sizeof(*this)); };

    /**
     * This constructor initializes the object with specific values.
     *
     * @param[in]  aHandler  Pointer to a handler function for the response.
     * @param[in]  aContext  Context for the handler function.
     *
     */
    QueryMetadata(otDnsResponseHandler aHandler, void *aContext)
    {
        memset(this, 0, sizeof(*this));
        mResponseHandler = aHandler;
        mResponseContext = aContext;
    };

    /**
     * This method appends request data to the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @retval OT_ERROR_NONE     Successfully appended the bytes.
     * @retval OT_ERROR_NO_BUFS  Insufficient available buffers to grow the message.
     *
     */
    otError AppendTo(Message &aMessage) const { return aMessage.Append(this, sizeof(*this)); };

    /**
     * This method reads request data from the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns The number of bytes read.
     *
     */
    uint16_t ReadFrom(const Message &aMessage)
    {
        return aMessage.Read(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    };

    /**
     * This method updates request data in the message.
     *
     * @param[in]  aMessage  A reference to the message.
     *
     * @returns The number of bytes updated.
     *
     */
    int UpdateIn(Message &aMessage) const
    {
        return aMessage.Write(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
    }

    /**
     * This method checks if the message shall be sent before the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent before the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsEarlier(uint32_t aTime) const { return (static_cast<int32_t>(aTime - mTransmissionTime) > 0); };

    /**
     * This method checks if the message shall be sent after the given time.
     *
     * @param[in]  aTime  A time to compare.
     *
     * @retval TRUE   If the message shall be sent after the given time.
     * @retval FALSE  Otherwise.
     */
    bool IsLater(uint32_t aTime) const { return (static_cast<int32_t>(aTime - mTransmissionTime) < 0); };

private:
    const char *         mHostname;            ///< A hostname to be find.
    otDnsResponseHandler mResponseHandler;     ///< A function pointer that is called on response reception.
    void *               mResponseContext;     ///< A pointer to arbitrary context information.
    uint32_t             mTransmissionTime;    ///< Time when the timer should shoot for this message.
    Ip6::Address         mSourceAddress;       ///< IPv6 address of the message source.
    Ip6::Address         mDestinationAddress;  ///< IPv6 address of the message destination.
    uint16_t             mDestinationPort;     ///< UDP port of the message destination.
    uint8_t              mRetransmissionCount; ///< Number of retransmissions.
} OT_TOOL_PACKED_END;

/**
 * This class implements DNS client.
 *
 */
class Client
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aNetif    A reference to the network interface that DNS client should be assigned to.
     *
     */
    Client(Ip6::Netif &aNetif)
        : mSocket(aNetif.GetIp6().GetUdp())
        , mMessageId(0)
        , mRetransmissionTimer(aNetif.GetInstance(), &Client::HandleRetransmissionTimer, this){};

    /**
     * This method starts the DNS client.
     *
     * @retval OT_ERROR_NONE     Successfully started the DNS client.
     * @retval OT_ERROR_ALREADY  The socket is already open.
     */
    otError Start(void);

    /**
     * This method stops the DNS client.
     *
     * @retval OT_ERROR_NONE  Successfully stopped the DNS client.
     *
     */
    otError Stop(void);

    /**
     * This method sends a DNS query.
     *
     * @param[in]  aQuery    A pointer to specify DNS query parameters.
     * @param[in]  aHandler  A function pointer that shall be called on response reception or time-out.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     * @retval OT_ERROR_NONE          Successfully sent DNS query.
     * @retval OT_ERROR_NO_BUFS       Failed to allocate retransmission data.
     * @retval OT_ERROR_INVALID_ARGS  Invalid arguments supplied.
     *
     */
    otError Query(const otDnsQuery *aQuery, otDnsResponseHandler aHandler, void *aContext);

private:
    /**
     * Retransmission parameters.
     *
     */
    enum
    {
        kResponseTimeout = OPENTHREAD_CONFIG_DNS_RESPONSE_TIMEOUT,
        kMaxRetransmit   = OPENTHREAD_CONFIG_DNS_MAX_RETRANSMIT,
    };

    /**
     * Special DNS symbols.
     */
    enum
    {
        kLabelTerminator       = 0,
        kLabelSeparator        = '.',
        kCompressionOffsetMask = 0xc0
    };

    /**
     * Operating on message buffers.
     */
    enum
    {
        kBufSize = 16
    };

    Message *NewMessage(const Header &aHeader);
    Message *CopyAndEnqueueMessage(const Message &aMessage, const QueryMetadata &aQueryMetadata);
    void     DequeueMessage(Message &aMessage);
    otError  SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError  SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    otError AppendCompressedHostname(Message &aMessage, const char *aHostname);
    otError CompareQuestions(Message &aMessageResponse, Message &aMessageQuery, uint16_t &aOffset);
    otError SkipHostname(Message &aMessage, uint16_t &aOffset);

    Message *FindRelatedQuery(const Header &aResponseHeader, QueryMetadata &aQueryMetadata);
    void     FinalizeDnsTransaction(Message &            aQuery,
                                    const QueryMetadata &aQueryMetadata,
                                    otIp6Address *       aAddress,
                                    uint32_t             aTtl,
                                    otError              aResult);

    static void HandleRetransmissionTimer(Timer &aTimer);
    void        HandleRetransmissionTimer(void);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Ip6::UdpSocket mSocket;

    uint16_t     mMessageId;
    MessageQueue mPendingQueries;
    TimerMilli   mRetransmissionTimer;
};

} // namespace Dns
} // namespace ot

#endif // DNS_CLIENT_HPP_
