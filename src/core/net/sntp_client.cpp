
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
    , mTimer(aInstance)
    , mUnixEra(0)
{
}

Error Client::Start(void)
{
    Error error;

    SuccessOrExit(error = mSocket.Open(Ip6::kNetifUnspecified));
    SuccessOrExit(error = mSocket.Bind(0));

exit:
    return error;
}

Error Client::Stop(void)
{
    for (Message &message : mPendingQueries)
    {
        QueryMetadata metadata;

        metadata.ReadFrom(message);
        Finalize(message, metadata, 0, kErrorAbort);
    }

    return mSocket.Close();
}

Error Client::Query(const QueryInfo &aQuery, ResponseHandler aHandler, void *aContext)
{
    Error         error;
    QueryMetadata metadata;
    Message      *message     = nullptr;
    Message      *messageCopy = nullptr;
    Header        header;

    VerifyOrExit(aQuery.IsValid(), error = kErrorInvalidArgs);

    header.Init();

    // Originate timestamp is used only as a unique token.
    header.GetTxTimestamp().SetSeconds(TimerMilli::GetNow().GetValue() / 1000 + kTimeAt1970);

    message = mSocket.NewMessage();
    VerifyOrExit(message != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = message->Append(header));

    metadata.mResponseCallback.Set(aHandler, aContext);
    metadata.mTxTimestamp = header.GetTxTimestamp().GetSeconds();
    metadata.mRetxTime    = TimerMilli::GetNow() + kResponseTimeout;
    metadata.mSourceAddr  = aQuery.GetMessageInfo().GetSockAddr();
    metadata.mDestPort    = aQuery.GetMessageInfo().GetPeerPort();
    metadata.mDestAddr    = aQuery.GetMessageInfo().GetPeerAddr();
    metadata.mRetxCount   = 0;

    messageCopy = CopyAndEnqueueMessage(*message, metadata);
    VerifyOrExit(messageCopy != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = mSocket.SendTo(*message, aQuery.GetMessageInfo()));

    mTimer.FireAtIfEarlier(metadata.mRetxTime);

    message     = nullptr;
    messageCopy = nullptr;

exit:
    FreeMessage(message);

    if (messageCopy != nullptr)
    {
        mPendingQueries.DequeueAndFree(*messageCopy);
    }

    return error;
}

Message *Client::CopyAndEnqueueMessage(const Message &aMessage, const QueryMetadata &aMetadata)
{
    Error    error;
    Message *messageCopy;

    messageCopy = aMessage.Clone<kNoReservedHeader>();
    VerifyOrExit(messageCopy != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = aMetadata.AppendTo(*messageCopy));

    mPendingQueries.Enqueue(*messageCopy);

exit:
    FreeAndNullMessageOnError(messageCopy, error);
    return messageCopy;
}

void Client::SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error    error;
    Message *messageCopy;

    messageCopy = mSocket.CloneMessageWithout<QueryMetadata>(aMessage);
    VerifyOrExit(messageCopy != nullptr, error = kErrorNoBufs);

    error = mSocket.SendTo(*messageCopy, aMessageInfo);

exit:
    FreeMessageOnError(messageCopy, error);
}

Message *Client::FindRelatedQuery(const Header &aResponseHeader, QueryMetadata &aMetadata)
{
    Message *matched = nullptr;

    for (Message &message : mPendingQueries)
    {
        aMetadata.ReadFrom(message);

        if (aMetadata.mTxTimestamp == aResponseHeader.GetOriginateTimestamp().GetSeconds())
        {
            matched = &message;
            break;
        }
    }

    return matched;
}

void Client::Finalize(Message &aQuery, const QueryMetadata &aMetadata, uint64_t aTime, Error aResult)
{
    mPendingQueries.DequeueAndFree(aQuery);
    aMetadata.mResponseCallback.InvokeIfSet(aTime, aResult);
}

void Client::HandleTimer(void)
{
    NextFireTime     nextTime;
    QueryMetadata    metadata;
    Ip6::MessageInfo messageInfo;

    for (Message &message : mPendingQueries)
    {
        metadata.ReadFrom(message);

        if (nextTime.GetNow() >= metadata.mRetxTime)
        {
            if (metadata.mRetxCount >= kMaxRetransmit)
            {
                // No expected response.
                Finalize(message, metadata, 0, kErrorResponseTimeout);
                continue;
            }

            // Increment retransmission counter and timer.
            metadata.mRetxCount++;
            metadata.mRetxTime = nextTime.GetNow() + kResponseTimeout;
            metadata.UpdateIn(message);

            // Retransmit
            messageInfo.SetPeerAddr(metadata.mDestAddr);
            messageInfo.SetPeerPort(metadata.mDestPort);
            messageInfo.SetSockAddr(metadata.mSourceAddr);

            SendCopy(message, messageInfo);
        }

        nextTime.UpdateIfEarlier(metadata.mRetxTime);
    }

    mTimer.FireAt(nextTime);
}

void Client::HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessageInfo);

    Header        header;
    Message      *query;
    QueryMetadata metadata;

    SuccessOrExit(aMessage.Read(aMessage.GetOffset(), header));

    query = FindRelatedQuery(header, metadata);
    VerifyOrExit(query != nullptr);

    ProcessResponse(*query, metadata, header);

exit:
    return;
}

void Client::ProcessResponse(Message &aQuery, const QueryMetadata &aMetadata, Header &aResponseHeader)
{
    Error    error    = kErrorNone;
    uint64_t unixTime = 0;

    // Check if response came from the server.
    VerifyOrExit(aResponseHeader.GetMode() == Header::kModeServer, error = kErrorFailed);

    // Check the Kiss-o'-death packet.
    if (!aResponseHeader.GetStratum())
    {
        char kissCode[Header::kKissCodeLength + 1];

        memcpy(kissCode, aResponseHeader.GetKissCode(), Header::kKissCodeLength);
        kissCode[Header::kKissCodeLength] = kNullChar;

        LogInfo("SNTP response contains the Kiss-o'-death packet with %s code", kissCode);
        ExitNow(error = kErrorBusy);
    }

    VerifyOrExit(!aResponseHeader.GetTxTimestamp().IsZero(), error = kErrorFailed);

    // The NTP time starts at 1900 while the unix epoch starts at 1970.
    // Due to NTP protocol limitation, this module stops working correctly after around year 2106, if
    // unix era is not updated. This seems to be a reasonable limitation for now. Era number cannot be
    // obtained using NTP protocol, and client of this module is responsible to set it properly.
    unixTime = GetUnixEra() * (1ULL << 32);

    if (aResponseHeader.GetTxTimestamp().GetSeconds() > kTimeAt1970)
    {
        unixTime += static_cast<uint64_t>(aResponseHeader.GetTxTimestamp().GetSeconds()) - kTimeAt1970;
    }
    else
    {
        unixTime += static_cast<uint64_t>(aResponseHeader.GetTxTimestamp().GetSeconds()) + (1ULL << 32) - kTimeAt1970;
    }

exit:
    Finalize(aQuery, aMetadata, unixTime, error);
}

} // namespace Sntp
} // namespace ot

#endif // OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE
