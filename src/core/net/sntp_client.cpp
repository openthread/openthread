
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

#if OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE

#include "instance/instance.hpp"

/**
 * @file
 *   This file implements the SNTP client.
 */

namespace ot {
namespace Sntp {

RegisterLogModule("SntpClnt");

Client::Client(Instance &aInstance)
    : mSocket(aInstance, *this)
    , mRetransmissionTimer(aInstance)
    , mUnixEra(0)
{
}

Error Client::Start(void)
{
    Error error;

    SuccessOrExit(error = mSocket.Open());
    SuccessOrExit(error = mSocket.Bind(0, Ip6::kNetifUnspecified));

exit:
    return error;
}

Error Client::Stop(void)
{
    for (Message &message : mPendingQueries)
    {
        QueryMetadata queryMetadata;

        queryMetadata.ReadFrom(message);
        FinalizeSntpTransaction(message, queryMetadata, 0, kErrorAbort);
    }

    return mSocket.Close();
}

Error Client::Query(const otSntpQuery *aQuery, otSntpResponseHandler aHandler, void *aContext)
{
    Error                   error;
    QueryMetadata           queryMetadata;
    Message                *message     = nullptr;
    Message                *messageCopy = nullptr;
    Header                  header;
    const Ip6::MessageInfo *messageInfo;

    VerifyOrExit(aQuery->mMessageInfo != nullptr, error = kErrorInvalidArgs);

    header.Init();

    // Originate timestamp is used only as a unique token.
    header.SetTransmitTimestampSeconds(TimerMilli::GetNow().GetValue() / 1000 + kTimeAt1970);

    VerifyOrExit((message = NewMessage(header)) != nullptr, error = kErrorNoBufs);

    messageInfo = AsCoreTypePtr(aQuery->mMessageInfo);

    queryMetadata.mResponseHandler.Set(aHandler, aContext);
    queryMetadata.mTransmitTimestamp   = header.GetTransmitTimestampSeconds();
    queryMetadata.mTransmissionTime    = TimerMilli::GetNow() + kResponseTimeout;
    queryMetadata.mSourceAddress       = messageInfo->GetSockAddr();
    queryMetadata.mDestinationPort     = messageInfo->GetPeerPort();
    queryMetadata.mDestinationAddress  = messageInfo->GetPeerAddr();
    queryMetadata.mRetransmissionCount = 0;

    VerifyOrExit((messageCopy = CopyAndEnqueueMessage(*message, queryMetadata)) != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = SendMessage(*message, *messageInfo));

exit:

    if (error != kErrorNone)
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
    Message *message = nullptr;

    VerifyOrExit((message = mSocket.NewMessage(sizeof(aHeader))) != nullptr);
    IgnoreError(message->Prepend(aHeader));
    message->SetOffset(0);

exit:
    return message;
}

Message *Client::CopyAndEnqueueMessage(const Message &aMessage, const QueryMetadata &aQueryMetadata)
{
    Error    error       = kErrorNone;
    Message *messageCopy = nullptr;

    // Create a message copy for further retransmissions.
    VerifyOrExit((messageCopy = aMessage.Clone()) != nullptr, error = kErrorNoBufs);

    // Append the copy with retransmission data and add it to the queue.
    SuccessOrExit(error = aQueryMetadata.AppendTo(*messageCopy));
    mPendingQueries.Enqueue(*messageCopy);

    mRetransmissionTimer.FireAtIfEarlier(aQueryMetadata.mTransmissionTime);

exit:
    FreeAndNullMessageOnError(messageCopy, error);
    return messageCopy;
}

void Client::DequeueMessage(Message &aMessage)
{
    if (mRetransmissionTimer.IsRunning() && (mPendingQueries.GetHead() == nullptr))
    {
        // No more requests pending, stop the timer.
        mRetransmissionTimer.Stop();
    }

    mPendingQueries.DequeueAndFree(aMessage);
}

Error Client::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return mSocket.SendTo(aMessage, aMessageInfo);
}

void Client::SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error    error;
    Message *messageCopy = nullptr;

    // Create a message copy for lower layers.
    VerifyOrExit((messageCopy = aMessage.Clone(aMessage.GetLength() - sizeof(QueryMetadata))) != nullptr,
                 error = kErrorNoBufs);

    // Send the copy.
    SuccessOrExit(error = SendMessage(*messageCopy, aMessageInfo));

exit:
    if (error != kErrorNone)
    {
        FreeMessage(messageCopy);
        LogWarnOnError(error, "send SNTP request");
    }
}

Message *Client::FindRelatedQuery(const Header &aResponseHeader, QueryMetadata &aQueryMetadata)
{
    Message *matchedMessage = nullptr;

    for (Message &message : mPendingQueries)
    {
        // Read originate timestamp.
        aQueryMetadata.ReadFrom(message);

        if (aQueryMetadata.mTransmitTimestamp == aResponseHeader.GetOriginateTimestampSeconds())
        {
            matchedMessage = &message;
            break;
        }
    }

    return matchedMessage;
}

void Client::FinalizeSntpTransaction(Message             &aQuery,
                                     const QueryMetadata &aQueryMetadata,
                                     uint64_t             aTime,
                                     Error                aResult)
{
    DequeueMessage(aQuery);
    aQueryMetadata.mResponseHandler.InvokeIfSet(aTime, aResult);
}

void Client::HandleRetransmissionTimer(void)
{
    NextFireTime     nextTime;
    QueryMetadata    queryMetadata;
    Ip6::MessageInfo messageInfo;

    for (Message &message : mPendingQueries)
    {
        queryMetadata.ReadFrom(message);

        if (nextTime.GetNow() >= queryMetadata.mTransmissionTime)
        {
            if (queryMetadata.mRetransmissionCount >= kMaxRetransmit)
            {
                // No expected response.
                FinalizeSntpTransaction(message, queryMetadata, 0, kErrorResponseTimeout);
                continue;
            }

            // Increment retransmission counter and timer.
            queryMetadata.mRetransmissionCount++;
            queryMetadata.mTransmissionTime = nextTime.GetNow() + kResponseTimeout;
            queryMetadata.UpdateIn(message);

            // Retransmit
            messageInfo.SetPeerAddr(queryMetadata.mDestinationAddress);
            messageInfo.SetPeerPort(queryMetadata.mDestinationPort);
            messageInfo.SetSockAddr(queryMetadata.mSourceAddress);

            SendCopy(message, messageInfo);
        }

        nextTime.UpdateIfEarlier(queryMetadata.mTransmissionTime);
    }

    mRetransmissionTimer.FireAt(nextTime);
}

void Client::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Error         error = kErrorNone;
    Header        responseHeader;
    QueryMetadata queryMetadata;
    Message      *message  = nullptr;
    uint64_t      unixTime = 0;

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), responseHeader));

    VerifyOrExit((message = FindRelatedQuery(responseHeader, queryMetadata)) != nullptr);

    // Check if response came from the server.
    VerifyOrExit(responseHeader.GetMode() == Header::kModeServer, error = kErrorFailed);

    // Check the Kiss-o'-death packet.
    if (!responseHeader.GetStratum())
    {
        char kissCode[Header::kKissCodeLength + 1];

        memcpy(kissCode, responseHeader.GetKissCode(), Header::kKissCodeLength);
        kissCode[Header::kKissCodeLength] = 0;

        LogInfo("SNTP response contains the Kiss-o'-death packet with %s code", kissCode);
        ExitNow(error = kErrorBusy);
    }

    // Check if timestamp has been set.
    VerifyOrExit(responseHeader.GetTransmitTimestampSeconds() != 0 &&
                     responseHeader.GetTransmitTimestampFraction() != 0,
                 error = kErrorFailed);

    // The NTP time starts at 1900 while the unix epoch starts at 1970.
    // Due to NTP protocol limitation, this module stops working correctly after around year 2106, if
    // unix era is not updated. This seems to be a reasonable limitation for now. Era number cannot be
    // obtained using NTP protocol, and client of this module is responsible to set it properly.
    unixTime = GetUnixEra() * (1ULL << 32);

    if (responseHeader.GetTransmitTimestampSeconds() > kTimeAt1970)
    {
        unixTime += static_cast<uint64_t>(responseHeader.GetTransmitTimestampSeconds()) - kTimeAt1970;
    }
    else
    {
        unixTime += static_cast<uint64_t>(responseHeader.GetTransmitTimestampSeconds()) + (1ULL << 32) - kTimeAt1970;
    }

    // Return the time since 1970.
    FinalizeSntpTransaction(*message, queryMetadata, unixTime, kErrorNone);

exit:

    if (message != nullptr && error != kErrorNone)
    {
        FinalizeSntpTransaction(*message, queryMetadata, 0, error);
    }
}

} // namespace Sntp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
