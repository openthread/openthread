/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include "sntp_client.hpp"

#include "utils/wrap_string.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/logging.hpp"
#include "common/owner-locator.hpp"
#include "net/udp6.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_SNTP_CLIENT

/**
 * @file
 *   This file implements the SNTP client.
 */

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Sntp {

otError Client::Start(void)
{
    otError       error;
    Ip6::SockAddr addr;

    SuccessOrExit(error = mSocket.Open(&Client::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(addr));

exit:
    return error;
}

otError Client::Stop(void)
{
    Message *     message = mPendingQueries.GetHead();
    Message *     messageToRemove;
    QueryMetadata queryMetadata;

    // Remove all pending queries.
    while (message != NULL)
    {
        messageToRemove = message;
        message         = message->GetNext();

        queryMetadata.ReadFrom(*messageToRemove);
        FinalizeSntpTransaction(*messageToRemove, queryMetadata, 0, OT_ERROR_ABORT);
    }

    return mSocket.Close();
}

otError Client::Query(const otSntpQuery *aQuery, otSntpResponseHandler aHandler, void *aContext)
{
    otError                 error;
    QueryMetadata           queryMetadata(aHandler, aContext);
    Message *               message     = NULL;
    Message *               messageCopy = NULL;
    Header                  header;
    const Ip6::MessageInfo *messageInfo;

    VerifyOrExit(aQuery->mMessageInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    // Originate timestamp is used only as a unique token.
    header.SetTransmitTimestampSeconds(TimerMilli::GetNow() / 1000 + kTimeAt1970);

    VerifyOrExit((message = NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    messageInfo = static_cast<const Ip6::MessageInfo *>(aQuery->mMessageInfo);

    queryMetadata.mTransmitTimestamp   = header.GetTransmitTimestampSeconds();
    queryMetadata.mTransmissionTime    = TimerMilli::GetNow() + kResponseTimeout;
    queryMetadata.mSourceAddress       = messageInfo->GetSockAddr();
    queryMetadata.mDestinationPort     = messageInfo->GetPeerPort();
    queryMetadata.mDestinationAddress  = messageInfo->GetPeerAddr();
    queryMetadata.mRetransmissionCount = 0;

    VerifyOrExit((messageCopy = CopyAndEnqueueMessage(*message, queryMetadata)) != NULL, error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = SendMessage(*message, *messageInfo));

exit:

    if (error != OT_ERROR_NONE)
    {
        if (message)
        {
            message->Free();
        }

        if (messageCopy)
        {
            DequeueMessage(*messageCopy);
        }
    }

    return error;
}

Message *Client::NewMessage(const Header &aHeader)
{
    Message *message = NULL;

    VerifyOrExit((message = mSocket.NewMessage(sizeof(aHeader))) != NULL);
    message->Prepend(&aHeader, sizeof(aHeader));
    message->SetOffset(0);

exit:
    return message;
}

Message *Client::CopyAndEnqueueMessage(const Message &aMessage, const QueryMetadata &aQueryMetadata)
{
    otError  error       = OT_ERROR_NONE;
    uint32_t now         = TimerMilli::GetNow();
    Message *messageCopy = NULL;
    uint32_t nextTransmissionTime;

    // Create a message copy for further retransmissions.
    VerifyOrExit((messageCopy = aMessage.Clone()) != NULL, error = OT_ERROR_NO_BUFS);

    // Append the copy with retransmission data and add it to the queue.
    SuccessOrExit(error = aQueryMetadata.AppendTo(*messageCopy));
    mPendingQueries.Enqueue(*messageCopy);

    // Setup the timer.
    if (mRetransmissionTimer.IsRunning())
    {
        // If timer is already running, check if it should be restarted with earlier fire time.
        nextTransmissionTime = mRetransmissionTimer.GetFireTime();

        if (aQueryMetadata.IsEarlier(nextTransmissionTime))
        {
            mRetransmissionTimer.Start(aQueryMetadata.mTransmissionTime - now);
        }
    }
    else
    {
        mRetransmissionTimer.Start(aQueryMetadata.mTransmissionTime - now);
    }

exit:

    if (error != OT_ERROR_NONE && messageCopy != NULL)
    {
        messageCopy->Free();
        messageCopy = NULL;
    }

    return messageCopy;
}

void Client::DequeueMessage(Message &aMessage)
{
    mPendingQueries.Dequeue(aMessage);

    if (mRetransmissionTimer.IsRunning() && (mPendingQueries.GetHead() == NULL))
    {
        // No more requests pending, stop the timer.
        mRetransmissionTimer.Stop();
    }

    // Free the message memory.
    aMessage.Free();
}

otError Client::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return mSocket.SendTo(aMessage, aMessageInfo);
}

otError Client::SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError  error;
    Message *messageCopy = NULL;

    // Create a message copy for lower layers.
    VerifyOrExit((messageCopy = aMessage.Clone(aMessage.GetLength() - sizeof(QueryMetadata))) != NULL,
                 error = OT_ERROR_NO_BUFS);

    // Send the copy.
    SuccessOrExit(error = SendMessage(*messageCopy, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE && messageCopy != NULL)
    {
        messageCopy->Free();
    }

    return error;
}

Message *Client::FindRelatedQuery(const Header &aResponseHeader, QueryMetadata &aQueryMetadata)
{
    Header   header;
    Message *message = mPendingQueries.GetHead();

    while (message != NULL)
    {
        // Read originate timestamp.
        uint16_t count = aQueryMetadata.ReadFrom(*message);
        assert(count == sizeof(aQueryMetadata));

        if (aQueryMetadata.mTransmitTimestamp == aResponseHeader.GetOriginateTimestampSeconds())
        {
            aQueryMetadata.ReadFrom(*message);
            ExitNow();
        }

        message = message->GetNext();
    }

exit:
    return message;
}

void Client::FinalizeSntpTransaction(Message &            aQuery,
                                     const QueryMetadata &aQueryMetadata,
                                     uint64_t             aTime,
                                     otError              aResult)
{
    DequeueMessage(aQuery);

    if (aQueryMetadata.mResponseHandler != NULL)
    {
        aQueryMetadata.mResponseHandler(aQueryMetadata.mResponseContext, aTime, aResult);
    }
}

void Client::HandleRetransmissionTimer(Timer &aTimer)
{
    aTimer.GetOwner<Client>().HandleRetransmissionTimer();
}

void Client::HandleRetransmissionTimer(void)
{
    uint32_t         now       = TimerMilli::GetNow();
    uint32_t         nextDelta = 0xffffffff;
    QueryMetadata    queryMetadata;
    Message *        message     = mPendingQueries.GetHead();
    Message *        nextMessage = NULL;
    Ip6::MessageInfo messageInfo;

    while (message != NULL)
    {
        nextMessage = message->GetNext();
        queryMetadata.ReadFrom(*message);

        if (queryMetadata.IsLater(now))
        {
            // Calculate the next delay and choose the lowest.
            if (queryMetadata.mTransmissionTime - now < nextDelta)
            {
                nextDelta = queryMetadata.mTransmissionTime - now;
            }
        }
        else if (queryMetadata.mRetransmissionCount < kMaxRetransmit)
        {
            // Increment retransmission counter and timer.
            queryMetadata.mRetransmissionCount++;
            queryMetadata.mTransmissionTime = now + kResponseTimeout;
            queryMetadata.UpdateIn(*message);

            // Check if retransmission time is lower than current lowest.
            if (queryMetadata.mTransmissionTime - now < nextDelta)
            {
                nextDelta = queryMetadata.mTransmissionTime - now;
            }

            // Retransmit
            messageInfo.SetPeerAddr(queryMetadata.mDestinationAddress);
            messageInfo.SetPeerPort(queryMetadata.mDestinationPort);
            messageInfo.SetSockAddr(queryMetadata.mSourceAddress);

            SendCopy(*message, messageInfo);
        }
        else
        {
            // No expected response.
            FinalizeSntpTransaction(*message, queryMetadata, 0, OT_ERROR_RESPONSE_TIMEOUT);
        }

        message = nextMessage;
    }

    if (nextDelta != 0xffffffff)
    {
        mRetransmissionTimer.Start(nextDelta);
    }
}

void Client::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Client *>(aContext)->HandleUdpReceive(*static_cast<Message *>(aMessage),
                                                      *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Client::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError       error = OT_ERROR_NONE;
    Header        responseHeader;
    QueryMetadata queryMetadata;
    Message *     message  = NULL;
    uint64_t      unixTime = 0;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(responseHeader), &responseHeader) ==
                 sizeof(responseHeader));

    VerifyOrExit((message = FindRelatedQuery(responseHeader, queryMetadata)) != NULL);

    // Check if response came from the server.
    VerifyOrExit(responseHeader.GetMode() == Header::kModeServer, error = OT_ERROR_FAILED);

    // Check the Kiss-o'-death packet.
    if (!responseHeader.GetStratum())
    {
        char kissCode[Header::kKissCodeLength + 1];

        memcpy(kissCode, responseHeader.GetKissCode(), Header::kKissCodeLength);
        kissCode[Header::kKissCodeLength] = 0;

        otLogInfoIp6("SNTP response contains the Kiss-o'-death packet with %s code", kissCode);
        ExitNow(error = OT_ERROR_BUSY);
    }

    // Check if timestamp has been set.
    VerifyOrExit(responseHeader.GetTransmitTimestampSeconds() != 0 &&
                     responseHeader.GetTransmitTimestampFraction() != 0,
                 error = OT_ERROR_FAILED);

    // The NTP time starts at 1900 while the unix epoch starts at 1970.
    // Due to NTP protocol limitation, this module stops working correctly after around year 2106, if
    // unix era is not updated. This seems to be a reasonable limitation for now. Era number cannot be
    // obtained using NTP protocol, and client of this module is responsible to set it properly.
    unixTime = GetUnixEra() * (UINT32_MAX + 1ULL);

    if (responseHeader.GetTransmitTimestampSeconds() > kTimeAt1970)
    {
        unixTime += static_cast<uint64_t>(responseHeader.GetTransmitTimestampSeconds()) - kTimeAt1970;
    }
    else
    {
        unixTime +=
            static_cast<uint64_t>(responseHeader.GetTransmitTimestampSeconds()) + (UINT32_MAX + 1ULL) - kTimeAt1970;
    }

    // Return the time since 1970.
    FinalizeSntpTransaction(*message, queryMetadata, unixTime, OT_ERROR_NONE);

exit:

    if (message != NULL && error != OT_ERROR_NONE)
    {
        FinalizeSntpTransaction(*message, queryMetadata, 0, error);
    }

    return;
}

} // namespace Sntp
} // namespace ot

#endif // OPENTHREAD_ENABLE_SNTP_CLIENT
