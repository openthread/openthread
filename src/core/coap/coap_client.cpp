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

#include <string.h>

#include <coap/coap_client.hpp>
#include <common/debug.hpp>
#include <common/code_utils.hpp>
#include <net/ip6.hpp>
#include <net/udp6.hpp>
#include <platform/random.h>

/**
 * @file
 *   This file implements the CoAP client.
 */

namespace Thread {
namespace Coap {

Client::Client(Ip6::Netif &aNetif, SenderFunction aSender, ReceiverFunction aReceiver):
    CoapBase(aNetif.GetIp6().mUdp, aSender, aReceiver),
    mRetransmissionTimer(aNetif.GetIp6().mTimerScheduler, &Client::HandleRetransmissionTimer, this)
{
    mMessageId = static_cast<uint16_t>(otPlatRandomGet());
}

ThreadError Client::Start(void)
{
    Ip6::SockAddr addr;
    addr.mPort = static_cast<Ip6::Udp *>(mSocket.mTransport)->GetEphemeralPort();

    return CoapBase::Start(addr);
}

ThreadError Client::Stop(void)
{
    Message *message = mPendingRequests.GetHead();
    Message *messageToRemove;
    RequestMetadata requestMetadata;

    // Remove all pending messages.
    while (message != NULL)
    {
        messageToRemove = message;
        message = message->GetNext();

        requestMetadata.ReadFrom(*messageToRemove);
        FinalizeCoapTransaction(*messageToRemove, requestMetadata, NULL, NULL, kThreadError_Abort);
    }

    return CoapBase::Stop();
}

ThreadError Client::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                                otCoapResponseHandler aHandler, void *aContext)
{
    ThreadError error;
    Header header;
    RequestMetadata requestMetadata;
    Message *storedCopy = NULL;
    uint16_t copyLength = 0;

    SuccessOrExit(error = header.FromMessage(aMessage, false));

    // Set Message Id if it was not already set.
    if (header.GetMessageId() == 0)
    {
        header.SetMessageId(mMessageId++);
        aMessage.Write(0, Header::kMinHeaderLength, header.GetBytes());
    }

    if (header.IsConfirmable())
    {
        // Create a copy of entire message and enqueue it.
        copyLength = aMessage.GetLength();
    }
    else if (header.IsNonConfirmable() && header.IsRequest() && (aHandler != NULL))
    {
        // As we do not retransmit non confirmable messages, create a copy of header only, for token information.
        copyLength = header.GetLength();
    }

    if (copyLength > 0)
    {
        requestMetadata = RequestMetadata(header.IsConfirmable(), aMessageInfo, aHandler, aContext);
        VerifyOrExit((storedCopy = CopyAndEnqueueMessage(aMessage, copyLength, requestMetadata)) != NULL,
                     error = kThreadError_NoBufs);
    }

    SuccessOrExit(error = mSender(this, aMessage, aMessageInfo));

exit:

    if (error != kThreadError_None && storedCopy != NULL)
    {
        DequeueMessage(*storedCopy);
    }

    return error;
}

Message *Client::CopyAndEnqueueMessage(const Message &aMessage, uint16_t aCopyLength,
                                       const RequestMetadata &aRequestMetadata)
{
    ThreadError error = kThreadError_None;
    Message *messageCopy = NULL;
    uint32_t alarmFireTime;

    // Create a message copy of requested size.
    VerifyOrExit((messageCopy = aMessage.Clone(aCopyLength)) != NULL, error = kThreadError_NoBufs);

    // Append the copy with retransmission data.
    SuccessOrExit(error = aRequestMetadata.AppendTo(*messageCopy));

    // Setup the timer.
    if (mRetransmissionTimer.IsRunning())
    {
        // If timer is already running, check if it should be restarted with earlier fire time.
        alarmFireTime = mRetransmissionTimer.Gett0() + mRetransmissionTimer.Getdt();

        if (aRequestMetadata.IsEarlier(alarmFireTime))
        {
            mRetransmissionTimer.Start(aRequestMetadata.mRetransmissionTimeout);
        }
    }
    else
    {
        mRetransmissionTimer.Start(aRequestMetadata.mRetransmissionTimeout);
    }

    // Enqueue the message.
    mPendingRequests.Enqueue(*messageCopy);

exit:

    if (error != kThreadError_None && messageCopy != NULL)
    {
        messageCopy->Free();
        messageCopy = NULL;
    }

    return messageCopy;
}

void Client::DequeueMessage(Message &aMessage)
{
    mPendingRequests.Dequeue(aMessage);

    if (mRetransmissionTimer.IsRunning() && (mPendingRequests.GetHead() == NULL))
    {
        // No more requests pending, stop the timer.
        mRetransmissionTimer.Stop();
    }

    // Free the message memory.
    aMessage.Free();

    // No need to worry that the earliest pending message was removed -
    // the timer would just shoot earlier and then it'd be setup again.
}

ThreadError Client::SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error;
    Message *messageCopy = NULL;

    // Create a message copy for lower layers.
    VerifyOrExit((messageCopy = aMessage.Clone(aMessage.GetLength() - sizeof(RequestMetadata))) != NULL,
                 error = kThreadError_NoBufs);

    // Send the copy.
    SuccessOrExit(error = mSender(this, *messageCopy, aMessageInfo));

exit:

    if (error != kThreadError_None && messageCopy != NULL)
    {
        messageCopy->Free();
    }

    return error;
}

void Client::SendEmptyMessage(const Ip6::Address &aAddress, uint16_t aPort, uint16_t aMessageId, Header::Type aType)
{
    Header header;
    Ip6::MessageInfo messageInfo;
    Message *message;
    ThreadError error = kThreadError_None;

    header.Init(aType, kCoapCodeEmpty);
    header.SetMessageId(aMessageId);

    VerifyOrExit((message = NewMessage(header)) != NULL, ;);

    messageInfo.SetPeerAddr(aAddress);
    messageInfo.SetPeerPort(aPort);

    SuccessOrExit(error = mSender(this, *message, messageInfo));

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }
}

void Client::HandleRetransmissionTimer(void *aContext)
{
    static_cast<Client *>(aContext)->HandleRetransmissionTimer();
}

void Client::HandleRetransmissionTimer(void)
{
    uint32_t now = otPlatAlarmGetNow();
    uint32_t nextDelta = 0xffffffff;
    RequestMetadata requestMetadata;
    Message *message = mPendingRequests.GetHead();
    Message *nextMessage = NULL;
    Ip6::MessageInfo messageInfo;

    while (message != NULL)
    {
        nextMessage = message->GetNext();
        requestMetadata.ReadFrom(*message);

        if (requestMetadata.IsLater(now))
        {
            // Calculate the next delay and choose the lowest.
            if (requestMetadata.mNextTimerShot - now < nextDelta)
            {
                nextDelta = requestMetadata.mNextTimerShot - now;
            }
        }
        else if ((requestMetadata.mConfirmable) &&
                 (requestMetadata.mRetransmissionCount < kMaxRetransmit))
        {
            // Increment retransmission counter and timer.
            requestMetadata.mRetransmissionCount++;
            requestMetadata.mRetransmissionTimeout *= 2;
            requestMetadata.mNextTimerShot = now + requestMetadata.mRetransmissionTimeout;
            requestMetadata.UpdateIn(*message);

            // Check if retransmission time is lower than current lowest.
            if (requestMetadata.mRetransmissionTimeout < nextDelta)
            {
                nextDelta = requestMetadata.mRetransmissionTimeout;
            }

            // Retransmit
            if (!requestMetadata.mAcknowledged)
            {
                messageInfo.SetPeerAddr(requestMetadata.mDestinationAddress);
                messageInfo.SetPeerPort(requestMetadata.mDestinationPort);

                SendCopy(*message, messageInfo);
            }
        }
        else
        {
            // No expected response or acknowledgment.
            FinalizeCoapTransaction(*message, requestMetadata, NULL, NULL, kThreadError_ResponseTimeout);
        }

        message = nextMessage;
    }

    if (nextDelta != 0xffffffff)
    {
        mRetransmissionTimer.Start(nextDelta);
    }
}

Message *Client::FindRelatedRequest(const Header &aResponseHeader, const Ip6::MessageInfo &aMessageInfo,
                                    Header &aRequestHeader, RequestMetadata &aRequestMetadata)
{
    Message *message = mPendingRequests.GetHead();

    while (message != NULL)
    {
        aRequestMetadata.ReadFrom(*message);

        if (((aRequestMetadata.mDestinationAddress == aMessageInfo.GetPeerAddr()) ||
             aRequestMetadata.mDestinationAddress.IsMulticast()) &&
            (aRequestMetadata.mDestinationPort == aMessageInfo.GetPeerPort()))
        {
            assert(aRequestHeader.FromMessage(*message, true) == kThreadError_None);

            switch (aResponseHeader.GetType())
            {
            case kCoapTypeReset:
            case kCoapTypeAcknowledgment:
                if (aResponseHeader.GetMessageId() == aRequestHeader.GetMessageId())
                {
                    ExitNow();
                }

                break;

            case kCoapTypeConfirmable:
            case kCoapTypeNonConfirmable:
                if (aResponseHeader.IsTokenEqual(aRequestHeader))
                {
                    ExitNow();
                }

                break;
            }
        }

        message = message->GetNext();
    }

exit:
    return message;
}

void Client::FinalizeCoapTransaction(Message &aRequest, const RequestMetadata &aRequestMetadata,
                                     Header *aResponseHeader, Message *aResponse, ThreadError aResult)
{
    DequeueMessage(aRequest);

    if (aRequestMetadata.mResponseHandler != NULL)
    {
        aRequestMetadata.mResponseHandler(aRequestMetadata.mResponseContext, aResponseHeader,
                                          aResponse, aResult);
    }
}

void Client::ProcessReceivedMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Header responseHeader;
    Header requestHeader;
    RequestMetadata requestMetadata;
    Message *message = NULL;
    ThreadError error;

    SuccessOrExit(error = responseHeader.FromMessage(aMessage, false));
    aMessage.MoveOffset(responseHeader.GetLength());

    message = FindRelatedRequest(responseHeader, aMessageInfo, requestHeader, requestMetadata);

    if (message == NULL)
    {
        ExitNow();
    }

    switch (responseHeader.GetType())
    {
    case kCoapTypeReset:
        if (responseHeader.IsEmpty())
        {
            FinalizeCoapTransaction(*message, requestMetadata, NULL, NULL, kThreadError_Abort);
        }

        // Silently ignore non-empty reset messages (RFC 7252, p. 4.2).
        break;

    case kCoapTypeAcknowledgment:
        if (responseHeader.IsEmpty())
        {
            // Empty acknowledgment.
            if (requestMetadata.mConfirmable)
            {
                requestMetadata.mAcknowledged = true;
                requestMetadata.UpdateIn(*message);
            }

            // Remove the message if response is not expected, otherwise await response.
            if (requestMetadata.mResponseHandler == NULL)
            {
                DequeueMessage(*message);
            }
        }
        else if (responseHeader.IsResponse() && responseHeader.IsTokenEqual(requestHeader))
        {
            // Piggybacked response.
            FinalizeCoapTransaction(*message, requestMetadata, &responseHeader, &aMessage, kThreadError_None);
        }

        // Silently ignore acknowledgments carrying requests (RFC 7252, p. 4.2)
        // or with no token match (RFC 7252, p. 5.3.2)
        break;

    case kCoapTypeConfirmable:
    case kCoapTypeNonConfirmable:
        if (responseHeader.IsConfirmable())
        {
            // Send empty ACK if it is a CON message.
            SendEmptyAck(aMessageInfo.GetPeerAddr(), aMessageInfo.GetPeerPort(), responseHeader.GetMessageId());
        }

        FinalizeCoapTransaction(*message, requestMetadata, &responseHeader, &aMessage, kThreadError_None);

        break;
    }

exit:

    if (error == kThreadError_None && message == NULL)
    {
        if (responseHeader.IsConfirmable() || responseHeader.IsNonConfirmable())
        {
            // Successfully parsed a header but no matching request was found - reject the message by sending reset.
            SendReset(aMessageInfo.GetPeerAddr(), aMessageInfo.GetPeerPort(), responseHeader.GetMessageId());
        }
    }
}

RequestMetadata::RequestMetadata(bool aConfirmable, const Ip6::MessageInfo &aMessageInfo,
                                 otCoapResponseHandler aHandler, void *aContext)
{
    mDestinationPort = aMessageInfo.GetPeerPort();
    mDestinationAddress = aMessageInfo.GetPeerAddr();
    mResponseHandler = aHandler;
    mResponseContext = aContext;
    mRetransmissionCount = 0;
    mRetransmissionTimeout = Timer::SecToMsec(kAckTimeout);
    mRetransmissionTimeout += otPlatRandomGet() %
                              (Timer::SecToMsec(kAckTimeout) * kAckRandomFactorNumerator / kAckRandomFactorDenominator -
                               Timer::SecToMsec(kAckTimeout) + 1);

    if (aConfirmable)
    {
        // Set next retransmission timeout.
        mNextTimerShot = Timer::GetNow() + mRetransmissionTimeout;
    }
    else
    {
        // Set overall response timeout.
        mNextTimerShot = Timer::GetNow() + kMaxTransmitWait;
    }

    mAcknowledged = false;
    mConfirmable = aConfirmable;
}

}  // namespace Coap
}  // namespace Thread
