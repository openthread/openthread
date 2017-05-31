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

#define WPP_NAME "coap.tmh"

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "coap.hpp"

#include <openthread/platform/random.h>

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/logging.hpp"
#include "net/ip6.hpp"
#include "net/udp6.hpp"
#include "thread/thread_netif.hpp"

/**
 * @file
 *   This file contains common code base for CoAP client and server.
 */

namespace ot {
namespace Coap {

Coap::Coap(ThreadNetif &aNetif):
    mNetif(aNetif),
    mSocket(aNetif.GetIp6().mUdp),
    mRetransmissionTimer(aNetif.GetIp6().mTimerScheduler, &Coap::HandleRetransmissionTimer, this),
    mResources(NULL),
    mContext(NULL),
    mInterceptor(NULL),
    mResponsesQueue(aNetif),
    mDefaultHandler(NULL),
    mDefaultHandlerContext(NULL)
{
    mMessageId = static_cast<uint16_t>(otPlatRandomGet());
}

otError Coap::Start(uint16_t aPort)
{
    otError error;
    Ip6::SockAddr sockaddr;
    sockaddr.mPort = aPort;

    SuccessOrExit(error = mSocket.Open(&Coap::HandleUdpReceive, this));
    SuccessOrExit(error = mSocket.Bind(sockaddr));

exit:
    return error;
}

otError Coap::Stop(void)
{
    Message *message = mPendingRequests.GetHead();
    Message *messageToRemove;
    CoapMetadata coapMetadata;

    // Remove all pending messages.
    while (message != NULL)
    {
        messageToRemove = message;
        message = message->GetNext();

        coapMetadata.ReadFrom(*messageToRemove);
        FinalizeCoapTransaction(*messageToRemove, coapMetadata, NULL, NULL, NULL, OT_ERROR_ABORT);
    }

    mResponsesQueue.DequeueAllResponses();

    return mSocket.Close();
}

otError Coap::AddResource(Resource &aResource)
{
    otError error = OT_ERROR_NONE;

    for (Resource *cur = mResources; cur; cur = cur->GetNext())
    {
        VerifyOrExit(cur != &aResource, error = OT_ERROR_ALREADY);
    }

    aResource.mNext = mResources;
    mResources = &aResource;

exit:
    return error;
}

void Coap::RemoveResource(Resource &aResource)
{
    if (mResources == &aResource)
    {
        mResources = aResource.GetNext();
    }
    else
    {
        for (Resource *cur = mResources; cur; cur = cur->GetNext())
        {
            if (cur->mNext == &aResource)
            {
                cur->mNext = aResource.mNext;
                ExitNow();
            }
        }
    }

exit:
    aResource.mNext = NULL;
}

void Coap::SetDefaultHandler(otCoapRequestHandler aHandler, void *aContext)
{
    mDefaultHandler = aHandler;
    mDefaultHandlerContext = aContext;
}

Message *Coap::NewMessage(const Header &aHeader, uint8_t aPriority)
{
    Message *message = NULL;

    // Ensure that header has minimum required length.
    VerifyOrExit(aHeader.GetLength() >= Header::kMinHeaderLength);

    VerifyOrExit((message = mSocket.NewMessage(aHeader.GetLength())) != NULL);
    message->Prepend(aHeader.GetBytes(), aHeader.GetLength());
    message->SetOffset(0);
    message->SetPriority(aPriority);

exit:
    return message;
}

otError Coap::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo,
                          otCoapResponseHandler aHandler, void *aContext)
{
    otError error;
    Header header;
    CoapMetadata coapMetadata;
    Message *storedCopy = NULL;
    uint16_t copyLength = 0;

    SuccessOrExit(error = header.FromMessage(aMessage, 0));

    if ((header.GetType() == OT_COAP_TYPE_ACKNOWLEDGMENT || header.GetType() == OT_COAP_TYPE_RESET) &&
        header.GetCode() != OT_COAP_CODE_EMPTY)
    {
        mResponsesQueue.EnqueueResponse(aMessage, aMessageInfo);
    }

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
    else if (header.IsNonConfirmable() && (aHandler != NULL))
    {
        // As we do not retransmit non confirmable messages, create a copy of header only, for token information.
        copyLength = header.GetLength();
    }

    if (copyLength > 0)
    {
        coapMetadata = CoapMetadata(header.IsConfirmable(), aMessageInfo, aHandler, aContext);
        VerifyOrExit((storedCopy = CopyAndEnqueueMessage(aMessage, copyLength, coapMetadata)) != NULL,
                     error = OT_ERROR_NO_BUFS);
    }

    SuccessOrExit(error = Send(aMessage, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE && storedCopy != NULL)
    {
        DequeueMessage(*storedCopy);
    }

    return error;
}

otError Coap::Send(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return mSocket.SendTo(aMessage, aMessageInfo);
}

otError Coap::SendEmptyMessage(Header::Type aType, const Header &aRequestHeader,
                               const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Header responseHeader;
    Message *message = NULL;

    VerifyOrExit(aRequestHeader.GetType() == OT_COAP_TYPE_CONFIRMABLE, error = OT_ERROR_INVALID_ARGS);

    responseHeader.Init(aType, OT_COAP_CODE_EMPTY);
    responseHeader.SetMessageId(aRequestHeader.GetMessageId());

    VerifyOrExit((message = NewMessage(responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = Send(*message, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError Coap::SendHeaderResponse(Header::Code aCode, const Header &aRequestHeader,
                                 const Ip6::MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;
    Header responseHeader;
    Header::Type requestType;
    Message *message = NULL;

    VerifyOrExit(aRequestHeader.IsRequest(), error = OT_ERROR_INVALID_ARGS);

    requestType = aRequestHeader.GetType();

    switch (requestType)
    {
    case OT_COAP_TYPE_CONFIRMABLE:
        responseHeader.Init(OT_COAP_TYPE_ACKNOWLEDGMENT, aCode);
        responseHeader.SetMessageId(aRequestHeader.GetMessageId());
        break;

    case OT_COAP_TYPE_NON_CONFIRMABLE:
        responseHeader.Init(OT_COAP_TYPE_NON_CONFIRMABLE, aCode);
        responseHeader.SetMessageId(mMessageId++);
        break;

    default:
        ExitNow(error = OT_ERROR_INVALID_ARGS);
        break;
    }

    responseHeader.SetToken(aRequestHeader.GetToken(), aRequestHeader.GetTokenLength());

    VerifyOrExit((message = NewMessage(responseHeader)) != NULL, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = SendMessage(*message, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Coap::HandleRetransmissionTimer(void *aContext)
{
    static_cast<Coap *>(aContext)->HandleRetransmissionTimer();
}

void Coap::HandleRetransmissionTimer(void)
{
    uint32_t now = otPlatAlarmGetNow();
    uint32_t nextDelta = 0xffffffff;
    CoapMetadata coapMetadata;
    Message *message = mPendingRequests.GetHead();
    Message *nextMessage = NULL;
    Ip6::MessageInfo messageInfo;

    while (message != NULL)
    {
        nextMessage = message->GetNext();
        coapMetadata.ReadFrom(*message);

        if (coapMetadata.IsLater(now))
        {
            // Calculate the next delay and choose the lowest.
            if (coapMetadata.mNextTimerShot - now < nextDelta)
            {
                nextDelta = coapMetadata.mNextTimerShot - now;
            }
        }
        else if ((coapMetadata.mConfirmable) &&
                 (coapMetadata.mRetransmissionCount < kMaxRetransmit))
        {
            // Increment retransmission counter and timer.
            coapMetadata.mRetransmissionCount++;
            coapMetadata.mRetransmissionTimeout *= 2;
            coapMetadata.mNextTimerShot = now + coapMetadata.mRetransmissionTimeout;
            coapMetadata.UpdateIn(*message);

            // Check if retransmission time is lower than current lowest.
            if (coapMetadata.mRetransmissionTimeout < nextDelta)
            {
                nextDelta = coapMetadata.mRetransmissionTimeout;
            }

            // Retransmit
            if (!coapMetadata.mAcknowledged)
            {
                messageInfo.SetPeerAddr(coapMetadata.mDestinationAddress);
                messageInfo.SetPeerPort(coapMetadata.mDestinationPort);
                messageInfo.SetSockAddr(coapMetadata.mSourceAddress);

                SendCopy(*message, messageInfo);
            }
        }
        else
        {
            // No expected response or acknowledgment.
            FinalizeCoapTransaction(*message, coapMetadata, NULL, NULL, NULL, OT_ERROR_RESPONSE_TIMEOUT);
        }

        message = nextMessage;
    }

    if (nextDelta != 0xffffffff)
    {
        mRetransmissionTimer.Start(nextDelta);
    }
}

void Coap::FinalizeCoapTransaction(Message &aRequest, const CoapMetadata &aCoapMetadata,
                                   Header *aResponseHeader, Message *aResponse,
                                   const Ip6::MessageInfo *aMessageInfo, otError aResult)
{
    DequeueMessage(aRequest);

    if (aCoapMetadata.mResponseHandler != NULL)
    {
        aCoapMetadata.mResponseHandler(aCoapMetadata.mResponseContext, aResponseHeader,
                                       aResponse, aMessageInfo, aResult);
    }
}

otError Coap::AbortTransaction(otCoapResponseHandler aHandler, void *aContext)
{
    otError error = OT_ERROR_NOT_FOUND;
    Message *message;
    CoapMetadata coapMetadata;

    for (message = mPendingRequests.GetHead(); message != NULL; message = message->GetNext())
    {
        coapMetadata.ReadFrom(*message);

        if (coapMetadata.mResponseHandler == aHandler && coapMetadata.mResponseContext == aContext)
        {
            DequeueMessage(*message);
            error = OT_ERROR_NONE;
        }
    }

    return error;
}


Message *Coap::CopyAndEnqueueMessage(const Message &aMessage, uint16_t aCopyLength,
                                     const CoapMetadata &aCoapMetadata)
{
    otError error = OT_ERROR_NONE;
    Message *messageCopy = NULL;
    uint32_t alarmFireTime;

    // Create a message copy of requested size.
    VerifyOrExit((messageCopy = aMessage.Clone(aCopyLength)) != NULL, error = OT_ERROR_NO_BUFS);

    // Append the copy with retransmission data.
    SuccessOrExit(error = aCoapMetadata.AppendTo(*messageCopy));

    // Setup the timer.
    if (mRetransmissionTimer.IsRunning())
    {
        // If timer is already running, check if it should be restarted with earlier fire time.
        alarmFireTime = mRetransmissionTimer.Gett0() + mRetransmissionTimer.Getdt();

        if (aCoapMetadata.IsEarlier(alarmFireTime))
        {
            mRetransmissionTimer.Start(aCoapMetadata.mRetransmissionTimeout);
        }
    }
    else
    {
        mRetransmissionTimer.Start(aCoapMetadata.mRetransmissionTimeout);
    }

    // Enqueue the message.
    mPendingRequests.Enqueue(*messageCopy);

exit:

    if (error != OT_ERROR_NONE && messageCopy != NULL)
    {
        messageCopy->Free();
        messageCopy = NULL;
    }

    return messageCopy;
}

void Coap::DequeueMessage(Message &aMessage)
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

otError Coap::SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error;
    Message *messageCopy = NULL;

    // Create a message copy for lower layers.
    VerifyOrExit((messageCopy = aMessage.Clone(aMessage.GetLength() - sizeof(CoapMetadata))) != NULL,
                 error = OT_ERROR_NO_BUFS);

    // Send the copy.
    SuccessOrExit(error = Send(*messageCopy, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE && messageCopy != NULL)
    {
        messageCopy->Free();
    }

    return error;
}

Message *Coap::FindRelatedRequest(const Header &aResponseHeader, const Ip6::MessageInfo &aMessageInfo,
                                  Header &aRequestHeader, CoapMetadata &aCoapMetadata)
{
    Message *message = mPendingRequests.GetHead();

    while (message != NULL)
    {
        aCoapMetadata.ReadFrom(*message);

        if (((aCoapMetadata.mDestinationAddress == aMessageInfo.GetPeerAddr()) ||
             aCoapMetadata.mDestinationAddress.IsMulticast() ||
             aCoapMetadata.mDestinationAddress.IsAnycastRoutingLocator()) &&
            (aCoapMetadata.mDestinationPort == aMessageInfo.GetPeerPort()))
        {
            // FromMessage can return OT_ERROR_PARSE if only partial message was stored (header only),
            // but payload marker is present. Assume, that stored messages are always valid.
            aRequestHeader.FromMessage(*message, sizeof(CoapMetadata));

            switch (aResponseHeader.GetType())
            {
            case OT_COAP_TYPE_RESET:
            case OT_COAP_TYPE_ACKNOWLEDGMENT:
                if (aResponseHeader.GetMessageId() == aRequestHeader.GetMessageId())
                {
                    ExitNow();
                }

                break;

            case OT_COAP_TYPE_CONFIRMABLE:
            case OT_COAP_TYPE_NON_CONFIRMABLE:
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

void Coap::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Coap *>(aContext)->Receive(*static_cast<Message *>(aMessage),
                                           *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

void Coap::Receive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError error;
    Header header;

    SuccessOrExit(error = header.FromMessage(aMessage, 0));

    if (header.IsRequest())
    {
        ProcessReceivedRequest(header, aMessage, aMessageInfo);
    }
    else
    {
        ProcessReceivedResponse(header, aMessage, aMessageInfo);
    }

exit:

    if (error)
    {
        otLogInfoCoapErr(mNetif.GetInstance(), error, "Receive failed");
    }
}

void Coap::ProcessReceivedResponse(Header &aResponseHeader, Message &aMessage,
                                   const Ip6::MessageInfo &aMessageInfo)
{
    Header requestHeader;
    CoapMetadata coapMetadata;
    Message *message = NULL;
    otError error = OT_ERROR_NONE;

    aMessage.MoveOffset(aResponseHeader.GetLength());

    message = FindRelatedRequest(aResponseHeader, aMessageInfo, requestHeader, coapMetadata);

    if (message == NULL)
    {
        ExitNow();
    }

    switch (aResponseHeader.GetType())
    {
    case OT_COAP_TYPE_RESET:
        if (aResponseHeader.IsEmpty())
        {
            FinalizeCoapTransaction(*message, coapMetadata, NULL, NULL, NULL, OT_ERROR_ABORT);
        }

        // Silently ignore non-empty reset messages (RFC 7252, p. 4.2).
        break;

    case OT_COAP_TYPE_ACKNOWLEDGMENT:
        if (aResponseHeader.IsEmpty())
        {
            // Empty acknowledgment.
            if (coapMetadata.mConfirmable)
            {
                coapMetadata.mAcknowledged = true;
                coapMetadata.UpdateIn(*message);
            }

            // Remove the message if response is not expected, otherwise await response.
            if (coapMetadata.mResponseHandler == NULL)
            {
                DequeueMessage(*message);
            }
        }
        else if (aResponseHeader.IsResponse() && aResponseHeader.IsTokenEqual(requestHeader))
        {
            // Piggybacked response.
            FinalizeCoapTransaction(*message, coapMetadata, &aResponseHeader, &aMessage, &aMessageInfo, OT_ERROR_NONE);
        }

        // Silently ignore acknowledgments carrying requests (RFC 7252, p. 4.2)
        // or with no token match (RFC 7252, p. 5.3.2)
        break;

    case OT_COAP_TYPE_CONFIRMABLE:
        // Send empty ACK if it is a CON message.
        SendAck(aResponseHeader, aMessageInfo);

        // fall through
        ;

    case OT_COAP_TYPE_NON_CONFIRMABLE:
        // Separate response.
        FinalizeCoapTransaction(*message, coapMetadata, &aResponseHeader, &aMessage, &aMessageInfo, OT_ERROR_NONE);

        break;
    }

exit:

    if (error == OT_ERROR_NONE && message == NULL)
    {
        if (aResponseHeader.IsConfirmable() || aResponseHeader.IsNonConfirmable())
        {
            // Successfully parsed a header but no matching request was found - reject the message by sending reset.
            SendReset(aResponseHeader, aMessageInfo);
        }
    }
}

void Coap::ProcessReceivedRequest(Header &aHeader, Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    char uriPath[Resource::kMaxReceivedUriPath] = "";
    char *curUriPath = uriPath;
    const Header::Option *coapOption;
    Message *cachedResponse = NULL;
    otError error = OT_ERROR_NOT_FOUND;

    if (mInterceptor != NULL)
    {
        SuccessOrExit(error = mInterceptor(aMessage, aMessageInfo, mContext));
    }

    aMessage.MoveOffset(aHeader.GetLength());

    switch (mResponsesQueue.GetMatchedResponseCopy(aHeader, aMessageInfo, &cachedResponse))
    {
    case OT_ERROR_NONE:
        error = Send(*cachedResponse, aMessageInfo);
        // fall through
        ;

    case OT_ERROR_NO_BUFS:
        ExitNow();

    case OT_ERROR_NOT_FOUND:
    default:
        break;
    }

    coapOption = aHeader.GetFirstOption();

    while (coapOption != NULL)
    {
        switch (coapOption->mNumber)
        {
        case OT_COAP_OPTION_URI_PATH:
            if (curUriPath != uriPath)
            {
                *curUriPath++ = '/';
            }

            VerifyOrExit(coapOption->mLength < sizeof(uriPath) - static_cast<size_t>(curUriPath + 1 - uriPath));

            memcpy(curUriPath, coapOption->mValue, coapOption->mLength);
            curUriPath += coapOption->mLength;
            break;

        default:
            break;
        }

        coapOption = aHeader.GetNextOption();
    }

    curUriPath[0] = '\0';

    for (const Resource *resource = mResources; resource; resource = resource->GetNext())
    {
        if (strcmp(resource->mUriPath, uriPath) == 0)
        {
            resource->HandleRequest(aHeader, aMessage, aMessageInfo);
            error = OT_ERROR_NONE;
            ExitNow();
        }
    }

    if (mDefaultHandler)
    {
        mDefaultHandler(mDefaultHandlerContext, &aHeader, &aMessage, &aMessageInfo);
        error = OT_ERROR_NONE;
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogInfoCoapErr(mNetif.GetInstance(), error, "Failed to process request");

        if (error == OT_ERROR_NOT_FOUND)
        {
            SendNotFound(aHeader, aMessageInfo);
        }

        if (cachedResponse != NULL)
        {
            cachedResponse->Free();
        }
    }

    return;
}

CoapMetadata::CoapMetadata(bool aConfirmable, const Ip6::MessageInfo &aMessageInfo,
                           otCoapResponseHandler aHandler, void *aContext)
{
    mSourceAddress = aMessageInfo.GetSockAddr();
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

ResponsesQueue::ResponsesQueue(ThreadNetif &aNetif):
    mTimer(aNetif.GetIp6().mTimerScheduler, &ResponsesQueue::HandleTimer, this)
{
}

otError ResponsesQueue::GetMatchedResponseCopy(const Header &aHeader,
                                               const Ip6::MessageInfo &aMessageInfo,
                                               Message **aResponse)
{
    otError                error = OT_ERROR_NOT_FOUND;
    Message               *message;
    EnqueuedResponseHeader enqueuedResponseHeader;
    Ip6::MessageInfo       messageInfo;
    Header                 header;

    for (message = mQueue.GetHead(); message != NULL; message = message->GetNext())
    {
        enqueuedResponseHeader.ReadFrom(*message);
        messageInfo = enqueuedResponseHeader.GetMessageInfo();

        // Check source endpoint
        if (messageInfo.GetPeerPort() != aMessageInfo.GetPeerPort())
        {
            continue;
        }

        if (messageInfo.GetPeerAddr() != aMessageInfo.GetPeerAddr())
        {
            continue;
        }

        // Check Message Id
        if (header.FromMessage(*message, sizeof(EnqueuedResponseHeader)) != OT_ERROR_NONE)
        {
            continue;
        }

        if (header.GetMessageId() != aHeader.GetMessageId())
        {
            continue;
        }

        *aResponse = message->Clone();
        VerifyOrExit(*aResponse != NULL, error = OT_ERROR_NO_BUFS);

        EnqueuedResponseHeader::RemoveFrom(**aResponse);

        error = OT_ERROR_NONE;
        break;
    }

exit:
    return error;
}

otError ResponsesQueue::GetMatchedResponseCopy(const Message &aRequest,
                                               const Ip6::MessageInfo &aMessageInfo,
                                               Message **aResponse)
{
    otError error = OT_ERROR_NONE;
    Header  header;

    SuccessOrExit(error = header.FromMessage(aRequest, 0));

    error = GetMatchedResponseCopy(header, aMessageInfo, aResponse);

exit:
    return error;
}

void ResponsesQueue::EnqueueResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Header                 header;
    Message               *copy;
    EnqueuedResponseHeader enqueuedResponseHeader(aMessageInfo);
    uint16_t               messageCount;
    uint16_t               bufferCount;

    SuccessOrExit(header.FromMessage(aMessage, 0));

    switch (GetMatchedResponseCopy(aMessage, aMessageInfo, &copy))
    {
    case OT_ERROR_NOT_FOUND:
        break;

    case OT_ERROR_NONE:
        copy->Free();

    // fall through

    case OT_ERROR_NO_BUFS:
    default:
        ExitNow();
    }

    mQueue.GetInfo(messageCount, bufferCount);

    if (messageCount >= kMaxCachedResponses)
    {
        DequeueOldestResponse();
    }

    copy = aMessage.Clone();
    VerifyOrExit(copy != NULL);

    enqueuedResponseHeader.AppendTo(*copy);
    mQueue.Enqueue(*copy);

    if (!mTimer.IsRunning())
    {
        mTimer.Start(Timer::SecToMsec(kExchangeLifetime));
    }

exit:
    return;
}

void ResponsesQueue::DequeueOldestResponse(void)
{
    Message *message;

    VerifyOrExit((message = mQueue.GetHead()) != NULL);
    DequeueResponse(*message);

exit:
    return;
}

void ResponsesQueue::DequeueAllResponses(void)
{
    Message *message;

    while ((message = mQueue.GetHead()) != NULL)
    {
        DequeueResponse(*message);
    }
}

void ResponsesQueue::HandleTimer(void *aContext)
{
    static_cast<ResponsesQueue *>(aContext)->HandleTimer();
}

void ResponsesQueue::HandleTimer(void)
{
    Message               *message;
    EnqueuedResponseHeader enqueuedResponseHeader;

    while ((message = mQueue.GetHead()) != NULL)
    {
        enqueuedResponseHeader.ReadFrom(*message);

        if (enqueuedResponseHeader.IsEarlier(Timer::GetNow()))
        {
            DequeueResponse(*message);
        }
        else
        {
            mTimer.Start(enqueuedResponseHeader.GetRemainingTime());
            break;
        }
    }
}

uint32_t EnqueuedResponseHeader::GetRemainingTime(void) const
{
    int32_t remainingTime = static_cast<int32_t>(mDequeueTime - Timer::GetNow());

    return remainingTime >= 0 ? static_cast<uint32_t>(remainingTime) : 0;
}

}  // namespace Coap
}  // namespace ot
