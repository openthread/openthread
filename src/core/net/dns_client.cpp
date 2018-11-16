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

#include "dns_client.hpp"

#include "utils/wrap_string.h"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/owner-locator.hpp"
#include "net/udp6.hpp"
#include "thread/thread_netif.hpp"

#if OPENTHREAD_ENABLE_DNS_CLIENT

/**
 * @file
 *   This file implements the DNS client.
 */

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Dns {

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
        FinalizeDnsTransaction(*messageToRemove, queryMetadata, NULL, 0, OT_ERROR_ABORT);
    }

    return mSocket.Close();
}

otError Client::Query(const otDnsQuery *aQuery, otDnsResponseHandler aHandler, void *aContext)
{
    otError                 error;
    QueryMetadata           queryMetadata(aHandler, aContext);
    Message *               message     = NULL;
    Message *               messageCopy = NULL;
    Header                  header;
    QuestionAaaa            question;
    const Ip6::MessageInfo *messageInfo;

    VerifyOrExit(aQuery->mHostname != NULL && aQuery->mMessageInfo != NULL, error = OT_ERROR_INVALID_ARGS);

    header.SetMessageId(mMessageId++);
    header.SetType(Header::kTypeQuery);
    header.SetQueryType(Header::kQueryTypeStandard);

    if (!aQuery->mNoRecursion)
    {
        header.SetRecursionDesiredFlag();
    }

    header.SetQuestionCount(1);

    VerifyOrExit((message = NewMessage(header)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = AppendCompressedHostname(*message, aQuery->mHostname));
    SuccessOrExit(error = question.AppendTo(*message));

    messageInfo = static_cast<const Ip6::MessageInfo *>(aQuery->mMessageInfo);

    queryMetadata.mHostname            = aQuery->mHostname;
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

otError Client::AppendCompressedHostname(Message &aMessage, const char *aHostname)
{
    otError error         = OT_ERROR_NONE;
    uint8_t index         = 0;
    uint8_t labelPosition = 0;
    uint8_t labelSize     = 0;

    while (true)
    {
        // Look for string separator.
        if (aHostname[index] == kLabelSeparator || aHostname[index] == kLabelTerminator)
        {
            VerifyOrExit(labelSize > 0, error = OT_ERROR_INVALID_ARGS);
            SuccessOrExit(error = aMessage.Append(&labelSize, 1));
            SuccessOrExit(error = aMessage.Append(&aHostname[labelPosition], labelSize));

            labelPosition += labelSize + 1;
            labelSize = 0;

            if (aHostname[index] == kLabelTerminator)
            {
                break;
            }
        }
        else
        {
            labelSize++;
        }

        index++;
    }

    // Add termination character at the end.
    labelSize = kLabelTerminator;
    SuccessOrExit(error = aMessage.Append(&labelSize, 1));

exit:
    return error;
}

otError Client::CompareQuestions(Message &aMessageResponse, Message &aMessageQuery, uint16_t &aOffset)
{
    otError  error = OT_ERROR_NONE;
    uint8_t  bufQuery[kBufSize];
    uint8_t  bufResponse[kBufSize];
    uint16_t read = 0;

    // Compare question section of the query with the response.
    uint16_t length = aMessageQuery.GetLength() - aMessageQuery.GetOffset() - sizeof(Header) - sizeof(QueryMetadata);
    uint16_t offset = aMessageQuery.GetOffset() + sizeof(Header);

    while (length > 0)
    {
        VerifyOrExit(
            (read = aMessageQuery.Read(offset, length < sizeof(bufQuery) ? length : sizeof(bufQuery), bufQuery)) > 0,
            error = OT_ERROR_PARSE);
        VerifyOrExit(aMessageResponse.Read(aOffset, read, bufResponse) == read, error = OT_ERROR_PARSE);

        VerifyOrExit(memcmp(bufResponse, bufQuery, read) == 0, error = OT_ERROR_NOT_FOUND);

        aOffset += read;
        offset += read;
        length -= read;
    }

exit:
    return error;
}

otError Client::SkipHostname(Message &aMessage, uint16_t &aOffset)
{
    otError  error = OT_ERROR_NONE;
    uint8_t  buf[kBufSize];
    uint16_t index;
    uint16_t read   = 0;
    uint16_t offset = aOffset;
    uint16_t length = aMessage.GetLength() - aOffset;

    while (length > 0)
    {
        VerifyOrExit((read = aMessage.Read(offset, sizeof(buf), buf)) > 0, error = OT_ERROR_PARSE);

        index = 0;

        while (index < read)
        {
            if (buf[index] == kLabelTerminator)
            {
                ExitNow(aOffset = offset + 1);
            }

            if ((buf[index] & kCompressionOffsetMask) == kCompressionOffsetMask)
            {
                ExitNow(aOffset = offset + 2);
            }

            index++;
            offset++;
        }

        length -= read;
    }

    ExitNow(error = OT_ERROR_PARSE);

exit:
    return error;
}

Message *Client::FindRelatedQuery(const Header &aResponseHeader, QueryMetadata &aQueryMetadata)
{
    uint16_t messageId;
    Message *message = mPendingQueries.GetHead();

    while (message != NULL)
    {
        // Partially read DNS header to obtain message ID only.
        uint16_t count = message->Read(message->GetOffset(), sizeof(messageId), &messageId);
        assert(count == sizeof(messageId));

        if (HostSwap16(messageId) == aResponseHeader.GetMessageId())
        {
            aQueryMetadata.ReadFrom(*message);
            ExitNow();
        }

        message = message->GetNext();
    }

exit:
    return message;
}

void Client::FinalizeDnsTransaction(Message &            aQuery,
                                    const QueryMetadata &aQueryMetadata,
                                    otIp6Address *       aAddress,
                                    uint32_t             aTtl,
                                    otError              aResult)
{
    DequeueMessage(aQuery);

    if (aQueryMetadata.mResponseHandler != NULL)
    {
        aQueryMetadata.mResponseHandler(aQueryMetadata.mResponseContext, aQueryMetadata.mHostname, aAddress, aTtl,
                                        aResult);
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
            FinalizeDnsTransaction(*message, queryMetadata, NULL, 0, OT_ERROR_RESPONSE_TIMEOUT);
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
    // RFC1035 7.3. Resolver cannot rely that a response will come from the same address
    // which it sent the corresponding query to.
    OT_UNUSED_VARIABLE(aMessageInfo);

    otError            error = OT_ERROR_NONE;
    Header             responseHeader;
    QueryMetadata      queryMetadata;
    ResourceRecordAaaa record;
    Message *          message = NULL;
    uint16_t           offset;

    VerifyOrExit(aMessage.Read(aMessage.GetOffset(), sizeof(responseHeader), &responseHeader) ==
                 sizeof(responseHeader));
    VerifyOrExit(responseHeader.GetType() == Header::kTypeResponse && responseHeader.GetQuestionCount() == 1 &&
                 responseHeader.IsTruncationFlagSet() == false);

    aMessage.MoveOffset(sizeof(responseHeader));
    offset = aMessage.GetOffset();

    VerifyOrExit((message = FindRelatedQuery(responseHeader, queryMetadata)) != NULL);

    VerifyOrExit(responseHeader.GetResponseCode() == Header::kResponseSuccess, error = OT_ERROR_FAILED);

    // Parse and check the question section.
    SuccessOrExit(error = CompareQuestions(aMessage, *message, offset));

    // Parse and check the answer section.
    for (uint32_t index = 0; index < responseHeader.GetAnswerCount(); index++)
    {
        SuccessOrExit(error = SkipHostname(aMessage, offset));

        if (offset + sizeof(ResourceRecord) > aMessage.GetLength())
        {
            ExitNow(error = OT_ERROR_PARSE);
        }

        if (aMessage.Read(offset, sizeof(record), &record) != sizeof(record) ||
            record.GetType() != ResourceRecordAaaa::kType || record.GetClass() != ResourceRecordAaaa::kClass)
        {
            offset += sizeof(ResourceRecord) + record.GetLength();

            continue;
        }

        // Return the first found IPv6 address.
        FinalizeDnsTransaction(*message, queryMetadata, &record.GetAddress(), record.GetTtl(), OT_ERROR_NONE);

        ExitNow();
    }

    ExitNow(error = OT_ERROR_NOT_FOUND);

exit:

    if (message != NULL && error != OT_ERROR_NONE)
    {
        FinalizeDnsTransaction(*message, queryMetadata, NULL, 0, error);
    }

    return;
}

} // namespace Dns
} // namespace ot

#endif // OPENTHREAD_ENABLE_DNS_CLIENT
