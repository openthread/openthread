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
    , mPendingRequests()
    , mMessageId(Random::NonCrypto::GetUint16())
    , mRetransmissionTimer(aInstance, Coap::HandleRetransmissionTimer, this)
    , mResources()
    , mContext(nullptr)
    , mInterceptor(nullptr)
    , mResponsesQueue(aInstance)
    , mDefaultHandler(nullptr)
    , mDefaultHandlerContext(nullptr)
    , mSender(aSender)
{
}

void CoapBase::ClearRequestsAndResponses(void)
{
    ClearRequests(nullptr); // Clear requests matching any address.
    mResponsesQueue.DequeueAllResponses();
}

void CoapBase::ClearRequests(const Ip6::Address &aAddress)
{
    ClearRequests(&aAddress);
}

void CoapBase::ClearRequests(const Ip6::Address *aAddress)
{
    Message *nextMessage;

    for (Message *message = mPendingRequests.GetHead(); message != nullptr; message = nextMessage)
    {
        Metadata metadata;

        nextMessage = message->GetNextCoapMessage();
        metadata.ReadFrom(*message);

        if ((aAddress == nullptr) || (metadata.mSourceAddress == *aAddress))
        {
            FinalizeCoapTransaction(*message, metadata, nullptr, nullptr, OT_ERROR_ABORT);
        }
    }
}

void CoapBase::AddResource(Resource &aResource)
{
    IgnoreError(mResources.Add(aResource));
}

void CoapBase::RemoveResource(Resource &aResource)
{
    IgnoreError(mResources.Remove(aResource));
    aResource.SetNext(nullptr);
}

void CoapBase::SetDefaultHandler(RequestHandler aHandler, void *aContext)
{
    mDefaultHandler        = aHandler;
    mDefaultHandlerContext = aContext;
}

void CoapBase::SetInterceptor(Interceptor aInterceptor, void *aContext)
{
    mInterceptor = aInterceptor;
    mContext     = aContext;
}

Message *CoapBase::NewMessage(const Message::Settings &aSettings)
{
    Message *message = nullptr;

    VerifyOrExit((message = static_cast<Message *>(Get<Ip6::Udp>().NewMessage(0, aSettings))) != nullptr, OT_NOOP);
    message->SetOffset(0);

exit:
    return message;
}

otError CoapBase::Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return mSender(*this, aMessage, aMessageInfo);
}

otError CoapBase::SendMessage(Message &               aMessage,
                              const Ip6::MessageInfo &aMessageInfo,
                              const TxParameters &    aTxParameters,
                              ResponseHandler         aHandler,
                              void *                  aContext)
{
    otError  error;
    Message *storedCopy = nullptr;
    uint16_t copyLength = 0;

    switch (aMessage.GetType())
    {
    case kTypeAck:
        mResponsesQueue.EnqueueResponse(aMessage, aMessageInfo, aTxParameters);
        break;
    case kTypeReset:
        OT_ASSERT(aMessage.GetCode() == kCodeEmpty);
        break;
    default:
        aMessage.SetMessageId(mMessageId++);
        break;
    }

    aMessage.Finish();

    if (aMessage.IsConfirmable())
    {
        copyLength = aMessage.GetLength();
    }
    else if (aMessage.IsNonConfirmable() && (aHandler != nullptr))
    {
        // As we do not retransmit non confirmable messages, create a
        // copy of header only, for token information.
        copyLength = aMessage.GetOptionStart();
    }

    if (copyLength > 0)
    {
        Metadata metadata;

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        // Whether or not to turn on special "Observe" handling.
        Option::Iterator iterator;
        bool             observe;

        SuccessOrExit(error = iterator.Init(aMessage, kOptionObserve));
        observe = !iterator.IsDone();

        // Special case, if we're sending a GET with Observe=1, that is a cancellation.
        if (observe && aMessage.IsGetRequest())
        {
            uint64_t observeVal = 0;

            SuccessOrExit(error = iterator.ReadOptionValue(observeVal));

            if (observeVal == 1)
            {
                Metadata handlerMetadata;

                // We're cancelling our subscription, so disable special-case handling on this request.
                observe = false;

                // If we can find the previous handler context, cancel that too.  Peer address
                // and tokens, etc should all match.
                Message *origRequest = FindRelatedRequest(aMessage, aMessageInfo, handlerMetadata);
                if (origRequest != nullptr)
                {
                    FinalizeCoapTransaction(*origRequest, handlerMetadata, nullptr, nullptr, OT_ERROR_NONE);
                }
            }
        }
#endif // OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE

        metadata.mSourceAddress            = aMessageInfo.GetSockAddr();
        metadata.mDestinationPort          = aMessageInfo.GetPeerPort();
        metadata.mDestinationAddress       = aMessageInfo.GetPeerAddr();
        metadata.mResponseHandler          = aHandler;
        metadata.mResponseContext          = aContext;
        metadata.mRetransmissionsRemaining = aTxParameters.mMaxRetransmit;
        metadata.mRetransmissionTimeout    = aTxParameters.CalculateInitialRetransmissionTimeout();
        metadata.mAcknowledged             = false;
        metadata.mConfirmable              = aMessage.IsConfirmable();
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        metadata.mObserve = observe;
#endif
        metadata.mNextTimerShot =
            TimerMilli::GetNow() +
            (metadata.mConfirmable ? metadata.mRetransmissionTimeout : aTxParameters.CalculateMaxTransmitWait());

        storedCopy = CopyAndEnqueueMessage(aMessage, copyLength, metadata);
        VerifyOrExit(storedCopy != nullptr, error = OT_ERROR_NO_BUFS);
    }

    SuccessOrExit(error = Send(aMessage, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE && storedCopy != nullptr)
    {
        DequeueMessage(*storedCopy);
    }

    return error;
}

otError CoapBase::SendMessage(Message &               aMessage,
                              const Ip6::MessageInfo &aMessageInfo,
                              ResponseHandler         aHandler,
                              void *                  aContext)
{
    return SendMessage(aMessage, aMessageInfo, TxParameters::GetDefault(), aHandler, aContext);
}

otError CoapBase::SendReset(Message &aRequest, const Ip6::MessageInfo &aMessageInfo)
{
    return SendEmptyMessage(kTypeReset, aRequest, aMessageInfo);
}

otError CoapBase::SendAck(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo)
{
    return SendEmptyMessage(kTypeAck, aRequest, aMessageInfo);
}

otError CoapBase::SendEmptyAck(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo)
{
    return (aRequest.IsConfirmable() ? SendHeaderResponse(kCodeChanged, aRequest, aMessageInfo)
                                     : OT_ERROR_INVALID_ARGS);
}

otError CoapBase::SendNotFound(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo)
{
    return SendHeaderResponse(kCodeNotFound, aRequest, aMessageInfo);
}

otError CoapBase::SendEmptyMessage(Type aType, const Message &aRequest, const Ip6::MessageInfo &aMessageInfo)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = nullptr;

    VerifyOrExit(aRequest.IsConfirmable(), error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit((message = NewMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    message->Init(aType, kCodeEmpty);
    message->SetMessageId(aRequest.GetMessageId());

    message->Finish();
    SuccessOrExit(error = Send(*message, aMessageInfo));

exit:
    FreeMessageOnError(message, error);
    return error;
}

otError CoapBase::SendHeaderResponse(Message::Code aCode, const Message &aRequest, const Ip6::MessageInfo &aMessageInfo)
{
    otError  error   = OT_ERROR_NONE;
    Message *message = nullptr;

    VerifyOrExit(aRequest.IsRequest(), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit((message = NewMessage()) != nullptr, error = OT_ERROR_NO_BUFS);

    switch (aRequest.GetType())
    {
    case kTypeConfirmable:
        message->Init(kTypeAck, aCode);
        message->SetMessageId(aRequest.GetMessageId());
        break;

    case kTypeNonConfirmable:
        message->Init(kTypeNonConfirmable, aCode);
        break;

    default:
        ExitNow(error = OT_ERROR_INVALID_ARGS);
        OT_UNREACHABLE_CODE(break);
    }

    SuccessOrExit(error = message->SetToken(aRequest.GetToken(), aRequest.GetTokenLength()));

    SuccessOrExit(error = SendMessage(*message, aMessageInfo));

exit:
    FreeMessageOnError(message, error);
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
    Metadata         metadata;
    Message *        nextMessage;
    Ip6::MessageInfo messageInfo;

    for (Message *message = mPendingRequests.GetHead(); message != nullptr; message = nextMessage)
    {
        nextMessage = message->GetNextCoapMessage();

        metadata.ReadFrom(*message);

        if (now >= metadata.mNextTimerShot)
        {
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            if (message->IsRequest() && metadata.mObserve && metadata.mAcknowledged)
            {
                // This is a RFC7641 subscription.  Do not time out.
                continue;
            }
#endif

            if (!metadata.mConfirmable || (metadata.mRetransmissionsRemaining == 0))
            {
                // No expected response or acknowledgment.
                FinalizeCoapTransaction(*message, metadata, nullptr, nullptr, OT_ERROR_RESPONSE_TIMEOUT);
                continue;
            }

            // Increment retransmission counter and timer.
            metadata.mRetransmissionsRemaining--;
            metadata.mRetransmissionTimeout *= 2;
            metadata.mNextTimerShot = now + metadata.mRetransmissionTimeout;
            metadata.UpdateIn(*message);

            // Retransmit
            if (!metadata.mAcknowledged)
            {
                messageInfo.SetPeerAddr(metadata.mDestinationAddress);
                messageInfo.SetPeerPort(metadata.mDestinationPort);
                messageInfo.SetSockAddr(metadata.mSourceAddress);

                SendCopy(*message, messageInfo);
            }
        }

        if (nextTime > metadata.mNextTimerShot)
        {
            nextTime = metadata.mNextTimerShot;
        }
    }

    if (nextTime < now.GetDistantFuture())
    {
        mRetransmissionTimer.FireAt(nextTime);
    }
}

void CoapBase::FinalizeCoapTransaction(Message &               aRequest,
                                       const Metadata &        aMetadata,
                                       Message *               aResponse,
                                       const Ip6::MessageInfo *aMessageInfo,
                                       otError                 aResult)
{
    DequeueMessage(aRequest);

    if (aMetadata.mResponseHandler != nullptr)
    {
        aMetadata.mResponseHandler(aMetadata.mResponseContext, aResponse, aMessageInfo, aResult);
    }
}

otError CoapBase::AbortTransaction(ResponseHandler aHandler, void *aContext)
{
    otError  error = OT_ERROR_NOT_FOUND;
    Message *nextMessage;
    Metadata metadata;

    for (Message *message = mPendingRequests.GetHead(); message != nullptr; message = nextMessage)
    {
        nextMessage = message->GetNextCoapMessage();
        metadata.ReadFrom(*message);

        if (metadata.mResponseHandler == aHandler && metadata.mResponseContext == aContext)
        {
            FinalizeCoapTransaction(*message, metadata, nullptr, nullptr, OT_ERROR_ABORT);
            error = OT_ERROR_NONE;
        }
    }

    return error;
}

Message *CoapBase::CopyAndEnqueueMessage(const Message &aMessage, uint16_t aCopyLength, const Metadata &aMetadata)
{
    otError  error       = OT_ERROR_NONE;
    Message *messageCopy = nullptr;

    VerifyOrExit((messageCopy = aMessage.Clone(aCopyLength)) != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = aMetadata.AppendTo(*messageCopy));

    mRetransmissionTimer.FireAtIfEarlier(aMetadata.mNextTimerShot);

    mPendingRequests.Enqueue(*messageCopy);

exit:
    FreeAndNullMessageOnError(messageCopy, error);
    return messageCopy;
}

void CoapBase::DequeueMessage(Message &aMessage)
{
    mPendingRequests.Dequeue(aMessage);

    if (mRetransmissionTimer.IsRunning() && (mPendingRequests.GetHead() == nullptr))
    {
        mRetransmissionTimer.Stop();
    }

    aMessage.Free();

    // No need to worry that the earliest pending message was removed -
    // the timer would just shoot earlier and then it'd be setup again.
}

void CoapBase::SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    otError  error;
    Message *messageCopy = nullptr;

    // Create a message copy for lower layers.
    messageCopy = aMessage.Clone(aMessage.GetLength() - sizeof(Metadata));
    VerifyOrExit(messageCopy != nullptr, error = OT_ERROR_NO_BUFS);

    SuccessOrExit(error = Send(*messageCopy, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE)
    {
        otLogWarnCoap("Failed to send copy: %s", otThreadErrorToString(error));
        FreeMessage(messageCopy);
    }
}

Message *CoapBase::FindRelatedRequest(const Message &         aResponse,
                                      const Ip6::MessageInfo &aMessageInfo,
                                      Metadata &              aMetadata)
{
    Message *message;

    for (message = mPendingRequests.GetHead(); message != nullptr; message = message->GetNextCoapMessage())
    {
        aMetadata.ReadFrom(*message);

        if (((aMetadata.mDestinationAddress == aMessageInfo.GetPeerAddr()) ||
             aMetadata.mDestinationAddress.IsMulticast() ||
             aMetadata.mDestinationAddress.GetIid().IsAnycastLocator()) &&
            (aMetadata.mDestinationPort == aMessageInfo.GetPeerPort()))
        {
            switch (aResponse.GetType())
            {
            case kTypeReset:
            case kTypeAck:
                if (aResponse.GetMessageId() == message->GetMessageId())
                {
                    ExitNow();
                }

                break;

            case kTypeConfirmable:
            case kTypeNonConfirmable:
                if (aResponse.IsTokenEqual(*message))
                {
                    ExitNow();
                }

                break;
            }
        }
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

        if (!aMessageInfo.GetSockAddr().IsMulticast() && message.IsConfirmable())
        {
            IgnoreError(SendReset(message, aMessageInfo));
        }
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
    Metadata metadata;
    Message *request = nullptr;
    otError  error   = OT_ERROR_NONE;
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    bool responseObserve = false;
#endif

    request = FindRelatedRequest(aMessage, aMessageInfo, metadata);
    VerifyOrExit(request != nullptr, OT_NOOP);

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    if (metadata.mObserve && request->IsRequest())
    {
        // We sent Observe in our request, see if we received Observe in the response too.
        Option::Iterator iterator;

        SuccessOrExit(error = iterator.Init(aMessage, kOptionObserve));
        responseObserve = !iterator.IsDone();
    }
#endif

    switch (aMessage.GetType())
    {
    case kTypeReset:
        if (aMessage.IsEmpty())
        {
            FinalizeCoapTransaction(*request, metadata, nullptr, nullptr, OT_ERROR_ABORT);
        }

        // Silently ignore non-empty reset messages (RFC 7252, p. 4.2).
        break;

    case kTypeAck:
        if (aMessage.IsEmpty())
        {
            // Empty acknowledgment.
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            if (metadata.mObserve && !request->IsRequest())
            {
                // This is the ACK to our RFC7641 notification.  There will be no
                // "separate" response so pass it back as if it were a piggy-backed
                // response so we can stop re-sending and the application can move on.
                FinalizeCoapTransaction(*request, metadata, &aMessage, &aMessageInfo, OT_ERROR_NONE);
            }
            else
#endif
            {
                // This is not related to RFC7641 or the outgoing "request" was not a
                // notification.
                if (metadata.mConfirmable)
                {
                    metadata.mAcknowledged = true;
                    metadata.UpdateIn(*request);
                }

                // Remove the message if response is not expected, otherwise await
                // response.
                if (metadata.mResponseHandler == nullptr)
                {
                    DequeueMessage(*request);
                }
            }
        }
        else if (aMessage.IsResponse() && aMessage.IsTokenEqual(*request))
        {
            // Piggybacked response.  If there's an Observe option present in both
            // request and response, and we have a response handler; then we're
            // dealing with RFC7641 rules here.
            // (If there is no response handler, then we're wasting our time!)
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            if (metadata.mObserve && responseObserve && (metadata.mResponseHandler != nullptr))
            {
                // This is a RFC7641 notification.  The request is *not* done!
                metadata.mResponseHandler(metadata.mResponseContext, &aMessage, &aMessageInfo, OT_ERROR_NONE);

                // Consider the message acknowledged at this point.
                metadata.mAcknowledged = true;
                metadata.UpdateIn(*request);
            }
            else
#endif
            {
                FinalizeCoapTransaction(*request, metadata, &aMessage, &aMessageInfo, OT_ERROR_NONE);
            }
        }

        // Silently ignore acknowledgments carrying requests (RFC 7252, p. 4.2)
        // or with no token match (RFC 7252, p. 5.3.2)
        break;

    case kTypeConfirmable:
        // Send empty ACK if it is a CON message.
        IgnoreError(SendAck(aMessage, aMessageInfo));
        // Fall through
        // Handling of RFC7641 and multicast is below.
    case kTypeNonConfirmable:
        // Separate response or observation notification.  If the request was to a multicast
        // address, OR both the request and response carry Observe options, then this is NOT
        // the final message, we may see multiples.
        if ((metadata.mResponseHandler != nullptr) && (metadata.mDestinationAddress.IsMulticast()
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
                                                       || (metadata.mObserve && responseObserve)
#endif
                                                           ))
        {
            // If multicast non-confirmable request, allow multiple responses
            metadata.mResponseHandler(metadata.mResponseContext, &aMessage, &aMessageInfo, OT_ERROR_NONE);
        }
        else
        {
            FinalizeCoapTransaction(*request, metadata, &aMessage, &aMessageInfo, OT_ERROR_NONE);
        }

        break;
    }

exit:

    if (error == OT_ERROR_NONE && request == nullptr)
    {
        if (aMessage.IsConfirmable() || aMessage.IsNonConfirmable())
        {
            // Successfully parsed a header but no matching request was
            // found - reject the message by sending reset.
            IgnoreError(SendReset(aMessage, aMessageInfo));
        }
    }
}

void CoapBase::ProcessReceivedRequest(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    char             uriPath[Resource::kMaxReceivedUriPath];
    char *           curUriPath     = uriPath;
    Message *        cachedResponse = nullptr;
    otError          error          = OT_ERROR_NOT_FOUND;
    Option::Iterator iterator;

    if (mInterceptor != nullptr)
    {
        SuccessOrExit(error = mInterceptor(aMessage, aMessageInfo, mContext));
    }

    switch (mResponsesQueue.GetMatchedResponseCopy(aMessage, aMessageInfo, &cachedResponse))
    {
    case OT_ERROR_NONE:
        cachedResponse->Finish();
        error = Send(*cachedResponse, aMessageInfo);

        // fall through

    case OT_ERROR_NO_BUFS:
        ExitNow();

    case OT_ERROR_NOT_FOUND:
    default:
        break;
    }

    SuccessOrExit(error = iterator.Init(aMessage, kOptionUriPath));

    while (!iterator.IsDone())
    {
        uint16_t optionLength = iterator.GetOption()->GetLength();

        if (curUriPath != uriPath)
        {
            *curUriPath++ = '/';
        }

        VerifyOrExit(curUriPath + optionLength < OT_ARRAY_END(uriPath), OT_NOOP);

        IgnoreError(iterator.ReadOptionValue(curUriPath));
        curUriPath += optionLength;

        SuccessOrExit(error = iterator.Advance(kOptionUriPath));
    }

    *curUriPath = '\0';

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
            IgnoreError(SendNotFound(aMessage, aMessageInfo));
        }

        FreeMessage(cachedResponse);
    }
}

void CoapBase::Metadata::ReadFrom(const Message &aMessage)
{
    uint16_t length = aMessage.GetLength();

    OT_ASSERT(length >= sizeof(*this));
    aMessage.Read(length - sizeof(*this), sizeof(*this), this);
}

void CoapBase::Metadata::UpdateIn(Message &aMessage) const
{
    aMessage.Write(aMessage.GetLength() - sizeof(*this), sizeof(*this), this);
}

ResponsesQueue::ResponsesQueue(Instance &aInstance)
    : mQueue()
    , mTimer(aInstance, ResponsesQueue::HandleTimer, this)
{
}

otError ResponsesQueue::GetMatchedResponseCopy(const Message &         aRequest,
                                               const Ip6::MessageInfo &aMessageInfo,
                                               Message **              aResponse)
{
    otError        error = OT_ERROR_NONE;
    const Message *cacheResponse;

    cacheResponse = FindMatchedResponse(aRequest, aMessageInfo);
    VerifyOrExit(cacheResponse != nullptr, error = OT_ERROR_NOT_FOUND);

    *aResponse = cacheResponse->Clone(cacheResponse->GetLength() - sizeof(ResponseMetadata));
    VerifyOrExit(*aResponse != nullptr, error = OT_ERROR_NO_BUFS);

exit:
    return error;
}

const Message *ResponsesQueue::FindMatchedResponse(const Message &aRequest, const Ip6::MessageInfo &aMessageInfo) const
{
    Message *message;

    for (message = mQueue.GetHead(); message != nullptr; message = message->GetNextCoapMessage())
    {
        if (message->GetMessageId() == aRequest.GetMessageId())
        {
            ResponseMetadata metadata;

            metadata.ReadFrom(*message);

            if ((metadata.mMessageInfo.GetPeerPort() == aMessageInfo.GetPeerPort()) &&
                (metadata.mMessageInfo.GetPeerAddr() == aMessageInfo.GetPeerAddr()))
            {
                break;
            }
        }
    }

    return message;
}

void ResponsesQueue::EnqueueResponse(Message &               aMessage,
                                     const Ip6::MessageInfo &aMessageInfo,
                                     const TxParameters &    aTxParameters)
{
    Message *        responseCopy;
    ResponseMetadata metadata;

    metadata.mDequeueTime = TimerMilli::GetNow() + aTxParameters.CalculateExchangeLifetime();
    metadata.mMessageInfo = aMessageInfo;

    VerifyOrExit(FindMatchedResponse(aMessage, aMessageInfo) == nullptr, OT_NOOP);

    UpdateQueue();

    VerifyOrExit((responseCopy = aMessage.Clone()) != nullptr, OT_NOOP);

    VerifyOrExit(metadata.AppendTo(*responseCopy) == OT_ERROR_NONE, responseCopy->Free());

    mQueue.Enqueue(*responseCopy);

    mTimer.FireAtIfEarlier(metadata.mDequeueTime);

exit:
    return;
}

void ResponsesQueue::UpdateQueue(void)
{
    uint16_t  msgCount    = 0;
    Message * earliestMsg = nullptr;
    TimeMilli earliestDequeueTime(0);

    // Check the number of messages in the queue and if number is at
    // `kMaxCachedResponses` remove the one with earliest dequeue
    // time.

    for (Message *message = mQueue.GetHead(); message != nullptr; message = message->GetNextCoapMessage())
    {
        ResponseMetadata metadata;

        metadata.ReadFrom(*message);

        if ((earliestMsg == nullptr) || (metadata.mDequeueTime < earliestDequeueTime))
        {
            earliestMsg         = message;
            earliestDequeueTime = metadata.mDequeueTime;
        }

        msgCount++;
    }

    if (msgCount >= kMaxCachedResponses)
    {
        DequeueResponse(*earliestMsg);
    }
}

void ResponsesQueue::DequeueResponse(Message &aMessage)
{
    mQueue.Dequeue(aMessage);
    aMessage.Free();
}

void ResponsesQueue::DequeueAllResponses(void)
{
    Message *message;

    while ((message = mQueue.GetHead()) != nullptr)
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
    TimeMilli now             = TimerMilli::GetNow();
    TimeMilli nextDequeueTime = now.GetDistantFuture();
    Message * nextMessage;

    for (Message *message = mQueue.GetHead(); message != nullptr; message = nextMessage)
    {
        ResponseMetadata metadata;

        nextMessage = message->GetNextCoapMessage();

        metadata.ReadFrom(*message);

        if (now >= metadata.mDequeueTime)
        {
            DequeueResponse(*message);
            continue;
        }

        if (metadata.mDequeueTime < nextDequeueTime)
        {
            nextDequeueTime = metadata.mDequeueTime;
        }
    }

    if (nextDequeueTime < now.GetDistantFuture())
    {
        mTimer.FireAt(nextDequeueTime);
    }
}

void ResponsesQueue::ResponseMetadata::ReadFrom(const Message &aMessage)
{
    uint16_t length = aMessage.GetLength();

    OT_ASSERT(length >= sizeof(*this));
    aMessage.Read(length - sizeof(*this), sizeof(*this), this);
}

/// Return product of @p aValueA and @p aValueB if no overflow otherwise 0.
static uint32_t Multiply(uint32_t aValueA, uint32_t aValueB)
{
    uint32_t result = 0;

    VerifyOrExit(aValueA, OT_NOOP);

    result = aValueA * aValueB;
    result = (result / aValueA == aValueB) ? result : 0;

exit:
    return result;
}

bool TxParameters::IsValid(void) const
{
    bool rval = false;

    if ((mAckRandomFactorDenominator > 0) && (mAckRandomFactorNumerator >= mAckRandomFactorDenominator) &&
        (mAckTimeout >= OT_COAP_MIN_ACK_TIMEOUT) && (mMaxRetransmit <= OT_COAP_MAX_RETRANSMIT))
    {
        // Calulate exchange lifetime step by step and verify no overflow.
        uint32_t tmp = Multiply(mAckTimeout, (1U << (mMaxRetransmit + 1)) - 1);

        tmp /= mAckRandomFactorDenominator;
        tmp = Multiply(tmp, mAckRandomFactorNumerator);

        rval = (tmp != 0 && (tmp + mAckTimeout + 2 * kDefaultMaxLatency) > tmp);
    }

    return rval;
}

uint32_t TxParameters::CalculateInitialRetransmissionTimeout(void) const
{
    return Random::NonCrypto::GetUint32InRange(
        mAckTimeout, mAckTimeout * mAckRandomFactorNumerator / mAckRandomFactorDenominator + 1);
}

uint32_t TxParameters::CalculateExchangeLifetime(void) const
{
    // Final `mAckTimeout` is to account for processing delay.
    return CalculateSpan(mMaxRetransmit) + 2 * kDefaultMaxLatency + mAckTimeout;
}

uint32_t TxParameters::CalculateMaxTransmitWait(void) const
{
    return CalculateSpan(mMaxRetransmit + 1);
}

uint32_t TxParameters::CalculateSpan(uint8_t aMaxRetx) const
{
    return static_cast<uint32_t>(mAckTimeout * ((1U << aMaxRetx) - 1) / mAckRandomFactorDenominator *
                                 mAckRandomFactorNumerator);
}

const otCoapTxParameters TxParameters::kDefaultTxParameters = {
    kDefaultAckTimeout,
    kDefaultAckRandomFactorNumerator,
    kDefaultAckRandomFactorDenominator,
    kDefaultMaxRetransmit,
};

Coap::Coap(Instance &aInstance)
    : CoapBase(aInstance, &Coap::Send)
    , mSocket(aInstance)
{
}

otError Coap::Start(uint16_t aPort, otNetifIdentifier aNetifIdentifier)
{
    otError error        = OT_ERROR_NONE;
    bool    socketOpened = false;

    VerifyOrExit(!mSocket.IsBound(), OT_NOOP);

    SuccessOrExit(error = mSocket.Open(&Coap::HandleUdpReceive, this));
    socketOpened = true;

    SuccessOrExit(error = mSocket.BindToNetif(aNetifIdentifier));
    SuccessOrExit(error = mSocket.Bind(aPort));

exit:
    if (error != OT_ERROR_NONE && socketOpened)
    {
        IgnoreError(mSocket.Close());
    }

    return error;
}

otError Coap::Stop(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mSocket.IsBound(), OT_NOOP);

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

otError Coap::Send(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<Coap &>(aCoapBase).Send(aMessage, aMessageInfo);
}

otError Coap::Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return mSocket.IsBound() ? mSocket.SendTo(aMessage, aMessageInfo) : OT_ERROR_INVALID_STATE;
}

} // namespace Coap
} // namespace ot
