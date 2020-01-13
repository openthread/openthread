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

#include "coap.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/random.hpp"
#include "net/ip6.hpp"
#include "net/udp6.hpp"
#include "thread/thread_netif.hpp"

/**
 * @file
 *   This file contains common code base for CoAP client and server.
 */

namespace ot {
namespace Coap {

CoapBase::CoapBase(Instance &aInstance, Sender aSender)
    : InstanceLocator(aInstance)
    , mRetransmissionTimer(aInstance, &Coap::HandleRetransmissionTimer, this)
    , mResources()
    , mContext(NULL)
    , mInterceptor(NULL)
    , mResponsesQueue(aInstance)
    , mDefaultHandler(NULL)
    , mDefaultHandlerContext(NULL)
    , mSender(aSender)
{
    mMessageId = Random::NonCrypto::GetUint16();
}

void CoapBase::ClearRequestsAndResponses(void)
{
    Message *    message = static_cast<Message *>(mPendingRequests.GetHead());
    Message *    messageToRemove;
    CoapMetadata coapMetadata;

    // Remove all pending messages.
    while (message != NULL)
    {
        messageToRemove = message;
        message         = static_cast<Message *>(message->GetNext());

        coapMetadata.ReadFrom(*messageToRemove);
        FinalizeCoapTransaction(*messageToRemove, coapMetadata, NULL, NULL, OT_ERROR_ABORT);
    }

    mResponsesQueue.DequeueAllResponses();
}

otError CoapBase::AddResource(Resource &aResource)
{
    return mResources.Add(aResource);
}

void CoapBase::RemoveResource(Resource &aResource)
{
    mResources.Remove(aResource);
    aResource.SetNext(NULL);
}

void CoapBase::SetDefaultHandler(otCoapRequestHandler aHandler, void *aContext)
{
    mDefaultHandler        = aHandler;
    mDefaultHandlerContext = aContext;
}

Message *CoapBase::NewMessage(const otMessageSettings *aSettings)
{
    Message *message = NULL;

    VerifyOrExit((message = static_cast<Message *>(Get<Ip6::Udp>().NewMessage(0, aSettings))) != NULL);
    message->SetOffset(0);

exit:
    return message;
}

otError CoapBase::SendMessage(Message &               aMessage,
                              const Ip6::MessageInfo &aMessageInfo,
                              otCoapResponseHandler   aHandler,
                              void *                  aContext)
{
    otError      error;
    CoapMetadata coapMetadata;
    Message *    storedCopy = NULL;
    uint16_t     copyLength = 0;

    switch (aMessage.GetType())
    {
    case OT_COAP_TYPE_ACKNOWLEDGMENT:
        mResponsesQueue.EnqueueResponse(aMessage, aMessageInfo);
        break;
    case OT_COAP_TYPE_RESET:
        assert(aMessage.GetCode() == OT_COAP_CODE_EMPTY);
        break;
    default:
        aMessage.SetMessageId(mMessageId++);
        break;
    }

    aMessage.Finish();

    if (aMessage.IsConfirmable())
    {
        // Create a copy of entire message and enqueue it.
        copyLength = aMessage.GetLength();
    }
    else if (aMessage.IsNonConfirmable() && (aHandler != NULL))
    {
        // As we do not retransmit non confirmable messages, create a copy of header only, for token information.
        copyLength = aMessage.GetOptionStart();
    }

    if (copyLength > 0)
    {
        coapMetadata = CoapMetadata(aMessage.IsConfirmable(), aMessageInfo, aHandler, aContext);
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

otError CoapBase::SendEmptyMessage(Message::Type aType, const Message &aRequest, const Ip6::MessageInfo &aMessageInfo)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = NULL;

    VerifyOrExit(aRequest.GetType() == OT_COAP_TYPE_CONFIRMABLE, error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit((message = NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    message->Init(aType, OT_COAP_CODE_EMPTY);
    message->SetMessageId(aRequest.GetMessageId());

    message->Finish();
    SuccessOrExit(error = Send(*message, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

otError CoapBase::SendHeaderResponse(Message::Code aCode, const Message &aRequest, const Ip6::MessageInfo &aMessageInfo)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = NULL;

    VerifyOrExit(aRequest.IsRequest(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((message = NewMessage()) != NULL, error = OT_ERROR_NO_BUFS);

    switch (aRequest.GetType())
    {
    case OT_COAP_TYPE_CONFIRMABLE:
        message->Init(OT_COAP_TYPE_ACKNOWLEDGMENT, aCode);
        message->SetMessageId(aRequest.GetMessageId());
        break;

    case OT_COAP_TYPE_NON_CONFIRMABLE:
        message->Init(OT_COAP_TYPE_NON_CONFIRMABLE, aCode);
        break;

    default:
        ExitNow(error = OT_ERROR_INVALID_ARGS);
        break;
    }

    SuccessOrExit(error = message->SetToken(aRequest.GetToken(), aRequest.GetTokenLength()));

    SuccessOrExit(error = SendMessage(*message, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE && message != NULL)
    {
        message->Free();
    }

    return error;
}

void CoapBase::HandleRetransmissionTimer(Timer &aTimer)
{
    static_cast<Coap *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleRetransmissionTimer();
}

void CoapBase::HandleRetransmissionTimer(void)
{
    TimeMilli        now      = TimerMilli::GetNow();
    TimeMilli        nextTime = now.GetDistantFuture();
    CoapMetadata     coapMetadata;
    Message *        message;
    Message *        nextMessage;
    Ip6::MessageInfo messageInfo;

    for (message = static_cast<Message *>(mPendingRequests.GetHead()); message != NULL; message = nextMessage)
    {
        nextMessage = static_cast<Message *>(message->GetNext());

        coapMetadata.ReadFrom(*message);

        if (now >= coapMetadata.mNextTimerShot)
        {
            if (!coapMetadata.mConfirmable || (coapMetadata.mRetransmissionCount >= kMaxRetransmit))
            {
                // No expected response or acknowledgment.
                FinalizeCoapTransaction(*message, coapMetadata, NULL, NULL, OT_ERROR_RESPONSE_TIMEOUT);
                continue;
            }

            // Increment retransmission counter and timer.
            coapMetadata.mRetransmissionCount++;
            coapMetadata.mRetransmissionTimeout *= 2;
            coapMetadata.mNextTimerShot = now + coapMetadata.mRetransmissionTimeout;
            coapMetadata.UpdateIn(*message);

            // Retransmit
            if (!coapMetadata.mAcknowledged)
            {
                messageInfo.SetPeerAddr(coapMetadata.mDestinationAddress);
                messageInfo.SetPeerPort(coapMetadata.mDestinationPort);
                messageInfo.SetSockAddr(coapMetadata.mSourceAddress);

                SendCopy(*message, messageInfo);
            }
        }

        if (nextTime > coapMetadata.mNextTimerShot)
        {
            nextTime = coapMetadata.mNextTimerShot;
        }
    }

    if (nextTime < now.GetDistantFuture())
    {
        mRetransmissionTimer.FireAt(nextTime);
    }
}

void CoapBase::FinalizeCoapTransaction(Message &               aRequest,
                                       const CoapMetadata &    aCoapMetadata,
                                       Message *               aResponse,
                                       const Ip6::MessageInfo *aMessageInfo,
                                       otError                 aResult)
{
    DequeueMessage(aRequest);

    if (aCoapMetadata.mResponseHandler != NULL)
    {
        aCoapMetadata.mResponseHandler(aCoapMetadata.mResponseContext, aResponse, aMessageInfo, aResult);
    }
}

otError CoapBase::AbortTransaction(otCoapResponseHandler aHandler, void *aContext)
{
    otError      error = OT_ERROR_NOT_FOUND;
    Message *    message;
    Message *    nextMessage;
    CoapMetadata coapMetadata;

    for (message = static_cast<Message *>(mPendingRequests.GetHead()); message != NULL; message = nextMessage)
    {
        nextMessage = static_cast<Message *>(message->GetNext());
        coapMetadata.ReadFrom(*message);

        if (coapMetadata.mResponseHandler == aHandler && coapMetadata.mResponseContext == aContext)
        {
            FinalizeCoapTransaction(*message, coapMetadata, NULL, NULL, OT_ERROR_ABORT);
            error = OT_ERROR_NONE;
        }
    }

    return error;
}

Message *CoapBase::CopyAndEnqueueMessage(const Message &     aMessage,
                                         uint16_t            aCopyLength,
                                         const CoapMetadata &aCoapMetadata)
{
    otError  error       = OT_ERROR_NONE;
    Message *messageCopy = NULL;

    // Create a message copy of requested size.
    VerifyOrExit((messageCopy = aMessage.Clone(aCopyLength)) != NULL, error = OT_ERROR_NO_BUFS);

    // Append the copy with retransmission data.
    SuccessOrExit(error = aCoapMetadata.AppendTo(*messageCopy));

    mRetransmissionTimer.FireAtIfEarlier(aCoapMetadata.mNextTimerShot);

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

void CoapBase::DequeueMessage(Message &aMessage)
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

otError CoapBase::SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError  error;
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

Message *CoapBase::FindRelatedRequest(const Message &         aResponse,
                                      const Ip6::MessageInfo &aMessageInfo,
                                      CoapMetadata &          aCoapMetadata)
{
    Message *message = static_cast<Message *>(mPendingRequests.GetHead());

    while (message != NULL)
    {
        aCoapMetadata.ReadFrom(*message);

        if (((aCoapMetadata.mDestinationAddress == aMessageInfo.GetPeerAddr()) ||
             aCoapMetadata.mDestinationAddress.IsMulticast() ||
             aCoapMetadata.mDestinationAddress.IsAnycastRoutingLocator()) &&
            (aCoapMetadata.mDestinationPort == aMessageInfo.GetPeerPort()))
        {
            switch (aResponse.GetType())
            {
            case OT_COAP_TYPE_RESET:
            case OT_COAP_TYPE_ACKNOWLEDGMENT:
                if (aResponse.GetMessageId() == message->GetMessageId())
                {
                    ExitNow();
                }

                break;

            case OT_COAP_TYPE_CONFIRMABLE:
            case OT_COAP_TYPE_NON_CONFIRMABLE:
                if (aResponse.IsTokenEqual(*message))
                {
                    ExitNow();
                }

                break;
            }
        }

        message = static_cast<Message *>(message->GetNext());
    }

exit:
    return message;
}

void CoapBase::Receive(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Message &message = static_cast<Message &>(aMessage);

    if (message.ParseHeader() != OT_ERROR_NONE)
    {
        otLogDebgCoap("Failed to parse CoAP header");
    }
    else if (message.IsRequest())
    {
        ProcessReceivedRequest(message, aMessageInfo);
    }
    else
    {
        ProcessReceivedResponse(message, aMessageInfo);
    }
}

void CoapBase::ProcessReceivedResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    CoapMetadata coapMetadata;
    Message *    request = NULL;
    otError      error   = OT_ERROR_NONE;

    request = FindRelatedRequest(aMessage, aMessageInfo, coapMetadata);

    if (request == NULL)
    {
        ExitNow();
    }

    switch (aMessage.GetType())
    {
    case OT_COAP_TYPE_RESET:
        if (aMessage.IsEmpty())
        {
            FinalizeCoapTransaction(*request, coapMetadata, NULL, NULL, OT_ERROR_ABORT);
        }

        // Silently ignore non-empty reset messages (RFC 7252, p. 4.2).
        break;

    case OT_COAP_TYPE_ACKNOWLEDGMENT:
        if (aMessage.IsEmpty())
        {
            // Empty acknowledgment.
            if (coapMetadata.mConfirmable)
            {
                coapMetadata.mAcknowledged = true;
                coapMetadata.UpdateIn(*request);
            }

            // Remove the message if response is not expected, otherwise await response.
            if (coapMetadata.mResponseHandler == NULL)
            {
                DequeueMessage(*request);
            }
        }
        else if (aMessage.IsResponse() && aMessage.IsTokenEqual(*request))
        {
            // Piggybacked response.
            FinalizeCoapTransaction(*request, coapMetadata, &aMessage, &aMessageInfo, OT_ERROR_NONE);
        }

        // Silently ignore acknowledgments carrying requests (RFC 7252, p. 4.2)
        // or with no token match (RFC 7252, p. 5.3.2)
        break;

    case OT_COAP_TYPE_CONFIRMABLE:
        // Send empty ACK if it is a CON message.
        SendAck(aMessage, aMessageInfo);
        FinalizeCoapTransaction(*request, coapMetadata, &aMessage, &aMessageInfo, OT_ERROR_NONE);
        break;

    case OT_COAP_TYPE_NON_CONFIRMABLE:
        // Separate response.

        if (coapMetadata.mDestinationAddress.IsMulticast() && coapMetadata.mResponseHandler != NULL)
        {
            // If multicast non-confirmable request, allow multiple responses
            coapMetadata.mResponseHandler(coapMetadata.mResponseContext, &aMessage, &aMessageInfo, OT_ERROR_NONE);
        }
        else
        {
            FinalizeCoapTransaction(*request, coapMetadata, &aMessage, &aMessageInfo, OT_ERROR_NONE);
        }

        break;
    }

exit:

    if (error == OT_ERROR_NONE && request == NULL)
    {
        if (aMessage.IsConfirmable() || aMessage.IsNonConfirmable())
        {
            // Successfully parsed a header but no matching request was found - reject the message by sending reset.
            SendReset(aMessage, aMessageInfo);
        }
    }
}

void CoapBase::ProcessReceivedRequest(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    char           uriPath[Resource::kMaxReceivedUriPath];
    char *         curUriPath     = uriPath;
    Message *      cachedResponse = NULL;
    otError        error          = OT_ERROR_NOT_FOUND;
    OptionIterator iterator;

    if (mInterceptor != NULL)
    {
        SuccessOrExit(error = mInterceptor(aMessage, aMessageInfo, mContext));
    }

    switch (mResponsesQueue.GetMatchedResponseCopy(aMessage, aMessageInfo, &cachedResponse))
    {
    case OT_ERROR_NONE:
        cachedResponse->Finish();
        error = Send(*cachedResponse, aMessageInfo);
        // fall through
        ;

    case OT_ERROR_NO_BUFS:
        ExitNow();

    case OT_ERROR_NOT_FOUND:
    default:
        break;
    }

    SuccessOrExit(error = iterator.Init(&aMessage));
    for (const otCoapOption *option = iterator.GetFirstOption(); option != NULL; option = iterator.GetNextOption())
    {
        switch (option->mNumber)
        {
        case OT_COAP_OPTION_URI_PATH:
            if (curUriPath != uriPath)
            {
                *curUriPath++ = '/';
            }

            VerifyOrExit(option->mLength < sizeof(uriPath) - static_cast<size_t>(curUriPath + 1 - uriPath));

            iterator.GetOptionValue(curUriPath);
            curUriPath += option->mLength;
            break;

        default:
            break;
        }
    }

    curUriPath[0] = '\0';

    for (const Resource *resource = mResources.GetHead(); resource; resource = resource->GetNext())
    {
        if (strcmp(resource->mUriPath, uriPath) == 0)
        {
            resource->HandleRequest(aMessage, aMessageInfo);
            error = OT_ERROR_NONE;
            ExitNow();
        }
    }

    if (mDefaultHandler)
    {
        mDefaultHandler(mDefaultHandlerContext, &aMessage, &aMessageInfo);
        error = OT_ERROR_NONE;
    }

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogInfoCoap("Failed to process request: %s", otThreadErrorToString(error));

        if (error == OT_ERROR_NOT_FOUND && !aMessageInfo.GetSockAddr().IsMulticast())
        {
            SendNotFound(aMessage, aMessageInfo);
        }

        if (cachedResponse != NULL)
        {
            cachedResponse->Free();
        }
    }
}

CoapMetadata::CoapMetadata(bool                    aConfirmable,
                           const Ip6::MessageInfo &aMessageInfo,
                           otCoapResponseHandler   aHandler,
                           void *                  aContext)
{
    mSourceAddress         = aMessageInfo.GetSockAddr();
    mDestinationPort       = aMessageInfo.GetPeerPort();
    mDestinationAddress    = aMessageInfo.GetPeerAddr();
    mResponseHandler       = aHandler;
    mResponseContext       = aContext;
    mRetransmissionCount   = 0;
    mRetransmissionTimeout = Time::SecToMsec(kAckTimeout);
    mRetransmissionTimeout += Random::NonCrypto::GetUint32InRange(
        0, Time::SecToMsec(kAckTimeout) * kAckRandomFactorNumerator / kAckRandomFactorDenominator -
               Time::SecToMsec(kAckTimeout) + 1);

    if (aConfirmable)
    {
        // Set next retransmission timeout.
        mNextTimerShot = TimerMilli::GetNow() + mRetransmissionTimeout;
    }
    else
    {
        // Set overall response timeout.
        mNextTimerShot = TimerMilli::GetNow() + Time::SecToMsec(kMaxTransmitWait);
    }

    mAcknowledged = false;
    mConfirmable  = aConfirmable;
}

ResponsesQueue::ResponsesQueue(Instance &aInstance)
    : mQueue()
    , mTimer(aInstance, &ResponsesQueue::HandleTimer, this)
{
}

otError ResponsesQueue::GetMatchedResponseCopy(const Message &         aRequest,
                                               const Ip6::MessageInfo &aMessageInfo,
                                               Message **              aResponse)
{
    otError        error = OT_ERROR_NONE;
    const Message *cacheResponse;

    cacheResponse = FindMatchedResponse(aRequest, aMessageInfo);
    VerifyOrExit(cacheResponse != NULL, error = OT_ERROR_NOT_FOUND);

    *aResponse = cacheResponse->Clone(cacheResponse->GetLength() - sizeof(EnqueuedResponseHeader));
    VerifyOrExit(*aResponse != NULL, error = OT_ERROR_NO_BUFS);

exit:
    return error;
}

const Message *ResponsesQueue::FindMatchedResponse(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo) const
{
    Message *matchedResponse = NULL;

    for (Message *message = static_cast<Message *>(mQueue.GetHead()); message != NULL;
         message          = static_cast<Message *>(message->GetNext()))
    {
        EnqueuedResponseHeader enqueuedResponseHeader;
        Ip6::MessageInfo       messageInfo;

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
        if (message->GetMessageId() != aRequest.GetMessageId())
        {
            continue;
        }

        ExitNow(matchedResponse = message);
    }

exit:
    return matchedResponse;
}

void ResponsesQueue::EnqueueResponse(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError                error        = OT_ERROR_NONE;
    Message *              responseCopy = NULL;
    EnqueuedResponseHeader enqueuedResponseHeader(aMessageInfo);
    uint16_t               messageCount;
    uint16_t               bufferCount;

    // return success if matched response already exists in the cache
    VerifyOrExit(FindMatchedResponse(aMessage, aMessageInfo) == NULL);

    mQueue.GetInfo(messageCount, bufferCount);

    if (messageCount >= kMaxCachedResponses)
    {
        DequeueOldestResponse();
    }

    VerifyOrExit((responseCopy = aMessage.Clone()) != NULL);

    SuccessOrExit(error = enqueuedResponseHeader.AppendTo(*responseCopy));
    mQueue.Enqueue(*responseCopy);

    if (!mTimer.IsRunning())
    {
        mTimer.Start(Time::SecToMsec(kExchangeLifetime));
    }

exit:

    if (error != OT_ERROR_NONE && responseCopy != NULL)
    {
        responseCopy->Free();
    }

    return;
}

void ResponsesQueue::DequeueOldestResponse(void)
{
    Message *message;

    VerifyOrExit((message = static_cast<Message *>(mQueue.GetHead())) != NULL);
    DequeueResponse(*message);

exit:
    return;
}

void ResponsesQueue::DequeueAllResponses(void)
{
    Message *message;

    while ((message = static_cast<Message *>(mQueue.GetHead())) != NULL)
    {
        DequeueResponse(*message);
    }
}

void ResponsesQueue::HandleTimer(Timer &aTimer)
{
    static_cast<ResponsesQueue *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void ResponsesQueue::HandleTimer(void)
{
    Message *              message;
    EnqueuedResponseHeader enqueuedResponseHeader;

    while ((message = static_cast<Message *>(mQueue.GetHead())) != NULL)
    {
        enqueuedResponseHeader.ReadFrom(*message);

        if (TimerMilli::GetNow() >= enqueuedResponseHeader.mDequeueTime)
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
    TimeMilli now           = TimerMilli::GetNow();
    uint32_t  remainingTime = 0;

    if (mDequeueTime > now)
    {
        remainingTime = mDequeueTime - now;
    }

    return remainingTime;
}

Coap::Coap(Instance &aInstance)
    : CoapBase(aInstance, &Coap::Send)
    , mSocket(aInstance.Get<Ip6::Udp>())
{
}

otError Coap::Start(uint16_t aPort)
{
    otError       error;
    Ip6::SockAddr sockaddr;

    sockaddr.mPort = aPort;
    SuccessOrExit(error = mSocket.Open(&Coap::HandleUdpReceive, this));
    VerifyOrExit((error = mSocket.Bind(sockaddr)) == OT_ERROR_NONE, mSocket.Close());

exit:
    return error;
}

otError Coap::Stop(void)
{
    otError error;

    SuccessOrExit(error = mSocket.Close());
    ClearRequestsAndResponses();

exit:
    return error;
}

void Coap::HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<Coap *>(aContext)->Receive(*static_cast<Message *>(aMessage),
                                           *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
}

otError Coap::Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return mSocket.IsBound() ? mSocket.SendTo(aMessage, aMessageInfo) : OT_ERROR_INVALID_STATE;
}

} // namespace Coap
} // namespace ot
