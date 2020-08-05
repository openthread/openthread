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
 * This class implements DNS client.
 *
 */
class Client
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit Client(Instance &aInstance);

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

    struct QueryMetadata
    {
        otError AppendTo(Message &aMessage) const { return aMessage.Append(this, sizeof(*this)); }
        void    ReadFrom(const Message &aMessage);
        void    UpdateIn(Message &aMessage) const;

        const char *         mHostname;
        otDnsResponseHandler mResponseHandler;
        void *               mResponseContext;
        TimeMilli            mTransmissionTime;
        Ip6::Address         mSourceAddress;
        Ip6::Address         mDestinationAddress;
        uint16_t             mDestinationPort;
        uint8_t              mRetransmissionCount;
    };

    Message *NewMessage(const Header &aHeader);
    Message *CopyAndEnqueueMessage(const Message &aMessage, const QueryMetadata &aQueryMetadata);
    void     DequeueMessage(Message &aMessage);
    otError  SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    void     SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    otError AppendCompressedHostname(Message &aMessage, const char *aHostname);
    otError CompareQuestions(Message &aMessageResponse, Message &aMessageQuery, uint16_t &aOffset);
    otError SkipHostname(Message &aMessage, uint16_t &aOffset);

    Message *FindRelatedQuery(const Header &aResponseHeader, QueryMetadata &aQueryMetadata);
    void     FinalizeDnsTransaction(Message &            aQuery,
                                    const QueryMetadata &aQueryMetadata,
                                    const otIp6Address * aAddress,
                                    uint32_t             aTtl,
                                    otError              aResult);

    static void HandleRetransmissionTimer(Timer &aTimer);
    void        HandleRetransmissionTimer(void);

    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);

    Ip6::Udp::Socket mSocket;

    uint16_t     mMessageId;
    MessageQueue mPendingQueries;
    TimerMilli   mRetransmissionTimer;
};

} // namespace Dns
} // namespace ot

#endif // DNS_CLIENT_HPP_
