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

#include "instance/instance.hpp"

/**
 * @file
 *   This file contains common code base for CoAP client and server.
 */

namespace ot {
namespace Coap {

RegisterLogModule("Coap");

//---------------------------------------------------------------------------------------------------------------------
// CoapBase

CoapBase::CoapBase(Instance &aInstance, Sender aSender)
    : InstanceLocator(aInstance)
    , mMessageId(Random::NonCrypto::GetUint16())
    , mRetransmissionTimer(aInstance, Coap::HandleRetransmissionTimer, this)
    , mResponseCache(aInstance)
    , mResourceHandler(nullptr)
    , mSender(aSender)
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    , mLastResponse(nullptr)
#endif
{
}

void CoapBase::ClearAllRequestsAndResponses(void)
{
    ClearRequests(nullptr); // Clear requests matching any address.
    mResponseCache.RemoveAll();
    mRetransmissionTimer.Stop();
}

void CoapBase::ClearRequests(const Ip6::Address &aAddress) { ClearRequests(&aAddress); }

void CoapBase::ClearRequests(const Ip6::Address *aAddress)
{
    for (Message &message : mPendingRequests)
    {
        Metadata metadata;

        metadata.ReadFrom(message);

        if ((aAddress == nullptr) || (metadata.mSourceAddress == *aAddress))
        {
            FinalizeCoapTransaction(message, metadata, nullptr, kErrorAbort);
        }
    }
}

void CoapBase::AddResource(Resource &aResource) { IgnoreError(mResources.Add(aResource)); }

void CoapBase::RemoveResource(Resource &aResource)
{
    IgnoreError(mResources.Remove(aResource));
    aResource.SetNext(nullptr);
}

Message *CoapBase::NewMessage(const Message::Settings &aSettings)
{
    Message *message = nullptr;

    VerifyOrExit((message = AsCoapMessagePtr(Get<Ip6::Udp>().NewMessage(0, aSettings))) != nullptr);
    message->SetOffset(0);

exit:
    return message;
}

Message *CoapBase::NewMessage(void) { return NewMessage(Message::Settings::GetDefault()); }

Message *CoapBase::NewPriorityMessage(void)
{
    return NewMessage(Message::Settings(kWithLinkSecurity, Message::kPriorityNet));
}

Message *CoapBase::NewPriorityConfirmablePostMessage(Uri aUri)
{
    return InitMessage(NewPriorityMessage(), kTypeConfirmable, aUri);
}

Message *CoapBase::NewConfirmablePostMessage(Uri aUri) { return InitMessage(NewMessage(), kTypeConfirmable, aUri); }

Message *CoapBase::NewPriorityNonConfirmablePostMessage(Uri aUri)
{
    return InitMessage(NewPriorityMessage(), kTypeNonConfirmable, aUri);
}

Message *CoapBase::NewNonConfirmablePostMessage(Uri aUri)
{
    return InitMessage(NewMessage(), kTypeNonConfirmable, aUri);
}

Message *CoapBase::NewPriorityResponseMessage(const Message &aRequest)
{
    return InitResponse(NewPriorityMessage(), aRequest);
}

Message *CoapBase::NewResponseMessage(const Message &aRequest) { return InitResponse(NewMessage(), aRequest); }

Message *CoapBase::InitMessage(Message *aMessage, Type aType, Uri aUri)
{
    Error error = kErrorNone;

    VerifyOrExit(aMessage != nullptr);

    SuccessOrExit(error = aMessage->Init(aType, kCodePost, aUri));
    SuccessOrExit(error = aMessage->SetPayloadMarker());

exit:
    FreeAndNullMessageOnError(aMessage, error);
    return aMessage;
}

Message *CoapBase::InitResponse(Message *aMessage, const Message &aRequest)
{
    Error error = kErrorNone;

    VerifyOrExit(aMessage != nullptr);

    SuccessOrExit(error = aMessage->SetDefaultResponseHeader(aRequest));
    SuccessOrExit(error = aMessage->SetPayloadMarker());

exit:
    FreeAndNullMessageOnError(aMessage, error);
    return aMessage;
}

Error CoapBase::Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error error;

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitCoapSend(AsCoapMessage(&aMessage), aMessageInfo);
#endif

    error = mSender(*this, aMessage, aMessageInfo);

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    if (error != kErrorNone)
    {
        Get<Utils::Otns>().EmitCoapSendFailure(error, AsCoapMessage(&aMessage), aMessageInfo);
    }
#endif
    return error;
}

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
Error CoapBase::SendMessage(Message                &aMessage,
                            const Ip6::MessageInfo &aMessageInfo,
                            const TxParameters     *aTxParameters,
                            ResponseHandler         aHandler,
                            void                   *aContext,
                            BlockwiseTransmitHook   aTransmitHook,
                            BlockwiseReceiveHook    aReceiveHook)
#else
Error CoapBase::SendMessage(Message                &aMessage,
                            const Ip6::MessageInfo &aMessageInfo,
                            const TxParameters     *aTxParameters,
                            ResponseHandler         aHandler,
                            void                   *aContext)
#endif
{
    Error    error;
    Message *storedCopy = nullptr;
    uint16_t copyLength = 0;
    Msg      txMsg(aMessage, aMessageInfo);
    Metadata metadata;

    if (aTxParameters == nullptr)
    {
        aTxParameters = &TxParameters::GetDefault();
    }
    else
    {
        SuccessOrExit(error = aTxParameters->ValidateFor(txMsg.mMessage));
    }

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    metadata.mBlockwiseReceiveHook  = aReceiveHook;
    metadata.mBlockwiseTransmitHook = aTransmitHook;

    SuccessOrExit(error = ProcessBlockwiseSend(txMsg.mMessage, aTransmitHook, aContext));
#endif

    switch (txMsg.mMessage.GetType())
    {
    case kTypeAck:
        mResponseCache.Add(txMsg, aTxParameters->CalculateExchangeLifetime());
        break;
    case kTypeReset:
        OT_ASSERT(txMsg.mMessage.GetCode() == kCodeEmpty);
        break;
    default:
        txMsg.mMessage.SetMessageId(mMessageId++);
        break;
    }

    txMsg.mMessage.Finish();

    if (txMsg.mMessage.IsConfirmable())
    {
        copyLength = txMsg.mMessage.GetLength();
    }
    else if (txMsg.mMessage.IsNonConfirmable() && (aHandler != nullptr))
    {
        // As we do not retransmit non confirmable messages, create a
        // copy of header only, for token information.
        copyLength = txMsg.mMessage.GetOptionStart();
    }

    if (copyLength > 0)
    {
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        {
            bool shouldObserve = false;

            SuccessOrExit(error = ProcessObserveSend(txMsg, shouldObserve));
            metadata.mObserve = shouldObserve;
        }
#endif

        metadata.mSourceAddress            = txMsg.mMessageInfo.GetSockAddr();
        metadata.mDestinationPort          = txMsg.mMessageInfo.GetPeerPort();
        metadata.mDestinationAddress       = txMsg.mMessageInfo.GetPeerAddr();
        metadata.mMulticastLoop            = txMsg.mMessageInfo.GetMulticastLoop();
        metadata.mResponseHandler          = aHandler;
        metadata.mResponseContext          = aContext;
        metadata.mRetransmissionsRemaining = aTxParameters->mMaxRetransmit;
        metadata.mRetransmissionTimeout    = aTxParameters->CalculateInitialRetransmissionTimeout();
        metadata.mAcknowledged             = false;
        metadata.mConfirmable              = txMsg.mMessage.IsConfirmable();
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
        metadata.mHopLimit        = txMsg.mMessageInfo.GetHopLimit();
        metadata.mIsHostInterface = txMsg.mMessageInfo.IsHostInterface();
#endif
        metadata.mNextTimerShot =
            TimerMilli::GetNow() +
            (metadata.mConfirmable ? metadata.mRetransmissionTimeout : aTxParameters->CalculateMaxTransmitWait());

        storedCopy = CopyAndEnqueueMessage(txMsg.mMessage, copyLength, metadata);
        VerifyOrExit(storedCopy != nullptr, error = kErrorNoBufs);
    }

    SuccessOrExit(error = Send(txMsg.mMessage, txMsg.mMessageInfo));

exit:

    if (error != kErrorNone && storedCopy != nullptr)
    {
        DequeueMessage(*storedCopy);
    }

    return error;
}

Error CoapBase::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo, const TxParameters &aTxParameters)
{
    return SendMessage(aMessage, aMessageInfo, &aTxParameters, nullptr, nullptr);
}

Error CoapBase::SendMessage(Message                &aMessage,
                            const Ip6::MessageInfo &aMessageInfo,
                            ResponseHandler         aHandler,
                            void                   *aContext)
{
    return SendMessage(aMessage, aMessageInfo, nullptr, aHandler, aContext);
}

Error CoapBase::SendMessage(OwnedPtr<Message>       aMessage,
                            const Ip6::MessageInfo &aMessageInfo,
                            ResponseHandler         aHandler,
                            void                   *aContext)
{
    Error error;

    OT_ASSERT(aMessage != nullptr);

    SuccessOrExit(error = SendMessage(*aMessage, aMessageInfo, aHandler, aContext));
    aMessage.Release();

exit:
    return error;
}

Error CoapBase::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return SendMessage(aMessage, aMessageInfo, nullptr, nullptr);
}

Error CoapBase::SendMessage(OwnedPtr<Message> aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error error;

    OT_ASSERT(aMessage != nullptr);

    SuccessOrExit(error = SendMessage(*aMessage, aMessageInfo));
    aMessage.Release();

exit:
    return error;
}

Error CoapBase::SendReset(const Msg &aRxMsg) { return SendEmptyMessage(kTypeReset, aRxMsg); }

Error CoapBase::SendAck(const Msg &aRxMsg) { return SendEmptyMessage(kTypeAck, aRxMsg); }

Error CoapBase::SendEmptyAck(const Msg &aRxMsg, Code aCode)
{
    return (aRxMsg.mMessage.IsConfirmable() ? SendHeaderResponse(aCode, aRxMsg) : kErrorInvalidArgs);
}

Error CoapBase::SendEmptyAck(const Msg &aRxMsg) { return SendEmptyAck(aRxMsg, kCodeChanged); }

Error CoapBase::SendNotFound(const Msg &aRxMsg) { return SendHeaderResponse(kCodeNotFound, aRxMsg); }

Error CoapBase::SendEmptyMessage(Type aType, const Msg &aRxMsg)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;

    VerifyOrExit(aRxMsg.mMessage.IsConfirmable(), error = kErrorInvalidArgs);

    VerifyOrExit((message = NewMessage()) != nullptr, error = kErrorNoBufs);

    message->Init(aType, kCodeEmpty);
    message->SetMessageId(aRxMsg.mMessage.GetMessageId());

    message->Finish();
    SuccessOrExit(error = Send(*message, aRxMsg.mMessageInfo));

exit:
    FreeMessageOnError(message, error);
    return error;
}

Error CoapBase::SendHeaderResponse(Message::Code aCode, const Msg &aRxMsg)
{
    Error    error   = kErrorNone;
    Message *message = nullptr;

    VerifyOrExit(aRxMsg.mMessage.IsRequest(), error = kErrorInvalidArgs);
    VerifyOrExit((message = NewMessage()) != nullptr, error = kErrorNoBufs);

    switch (aRxMsg.mMessage.GetType())
    {
    case kTypeConfirmable:
        message->Init(kTypeAck, aCode);
        message->SetMessageId(aRxMsg.mMessage.GetMessageId());
        break;

    case kTypeNonConfirmable:
        message->Init(kTypeNonConfirmable, aCode);
        break;

    default:
        ExitNow(error = kErrorInvalidArgs);
    }

    SuccessOrExit(error = message->WriteTokenFromMessage(aRxMsg.mMessage));

    SuccessOrExit(error = SendMessage(*message, aRxMsg.mMessageInfo));

exit:
    FreeMessageOnError(message, error);
    return error;
}

void CoapBase::ScheduleRetransmissionTimer(void)
{
    NextFireTime nextTime;
    Metadata     metadata;

    for (const Message &message : mPendingRequests)
    {
        metadata.ReadFrom(message);

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        if (IsObserveSubscription(message, metadata))
        {
            // This is an RFC7641 subscription which is already acknowledged.
            // We do not time it out, so skip it when determining the next
            // fire time.
            continue;
        }
#endif

        nextTime.UpdateIfEarlier(metadata.mNextTimerShot);
    }

    mRetransmissionTimer.FireAt(nextTime);
}

void CoapBase::HandleRetransmissionTimer(Timer &aTimer)
{
    static_cast<Coap *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleRetransmissionTimer();
}

void CoapBase::HandleRetransmissionTimer(void)
{
    TimeMilli        now = TimerMilli::GetNow();
    Metadata         metadata;
    Ip6::MessageInfo messageInfo;

    for (Message &message : mPendingRequests)
    {
        metadata.ReadFrom(message);

        if (now >= metadata.mNextTimerShot)
        {
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            if (IsObserveSubscription(message, metadata))
            {
                continue;
            }
#endif

            if (!metadata.mConfirmable || (metadata.mRetransmissionsRemaining == 0))
            {
                // No expected response or acknowledgment.
                FinalizeCoapTransaction(message, metadata, nullptr, kErrorResponseTimeout);
                continue;
            }

            // Increment retransmission counter and timer.
            metadata.mRetransmissionsRemaining--;
            metadata.mRetransmissionTimeout *= 2;
            metadata.mNextTimerShot = now + metadata.mRetransmissionTimeout;
            metadata.UpdateIn(message);

            // Retransmit
            if (!metadata.mAcknowledged)
            {
                messageInfo.SetPeerAddr(metadata.mDestinationAddress);
                messageInfo.SetPeerPort(metadata.mDestinationPort);
                messageInfo.SetSockAddr(metadata.mSourceAddress);
#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE
                messageInfo.SetHopLimit(metadata.mHopLimit);
                messageInfo.SetIsHostInterface(metadata.mIsHostInterface);
#endif
                messageInfo.SetMulticastLoop(metadata.mMulticastLoop);

                SendCopy(message, messageInfo);
            }
        }
    }

    ScheduleRetransmissionTimer();
}

void CoapBase::FinalizeCoapTransaction(Message &aRequest, const Metadata &aMetadata, Msg *aResponse, Error aResult)
{
    DequeueMessage(aRequest);

    aMetadata.InvokeResponseHandler(aResponse, aResult);
}

Error CoapBase::AbortTransaction(ResponseHandler aHandler, void *aContext)
{
    Error    error = kErrorNotFound;
    Metadata metadata;

    for (Message &message : mPendingRequests)
    {
        metadata.ReadFrom(message);

        if (metadata.mResponseHandler == aHandler && metadata.mResponseContext == aContext)
        {
            FinalizeCoapTransaction(message, metadata, nullptr, kErrorAbort);
            error = kErrorNone;
        }
    }

    return error;
}

void CoapBase::GetRequestAndCachedResponsesQueueInfo(MessageQueue::Info &aQueueInfo) const
{
    MessageQueue::Info info;

    mPendingRequests.GetInfo(aQueueInfo);
    mResponseCache.GetInfo(info);
    MessageQueue::AddQueueInfos(aQueueInfo, info);
}

Message *CoapBase::CopyAndEnqueueMessage(const Message &aMessage, uint16_t aCopyLength, const Metadata &aMetadata)
{
    Error    error       = kErrorNone;
    Message *messageCopy = nullptr;

    VerifyOrExit((messageCopy = aMessage.Clone(aCopyLength)) != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = aMetadata.AppendTo(*messageCopy));

    mPendingRequests.Enqueue(*messageCopy);
    ScheduleRetransmissionTimer();

exit:
    FreeAndNullMessageOnError(messageCopy, error);
    return messageCopy;
}

void CoapBase::DequeueMessage(Message &aMessage)
{
    mPendingRequests.DequeueAndFree(aMessage);
    ScheduleRetransmissionTimer();
}

void CoapBase::SendCopy(const Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Error    error;
    Message *messageCopy = nullptr;

    // Create a message copy for lower layers.
    messageCopy = aMessage.Clone(aMessage.GetLength() - sizeof(Metadata));
    VerifyOrExit(messageCopy != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = Send(*messageCopy, aMessageInfo));

exit:

    if (error != kErrorNone)
    {
        LogWarn("Failed to send copy: %s", ErrorToString(error));
        FreeMessage(messageCopy);
    }
}

Message *CoapBase::FindRelatedRequest(const Msg &aMsg, Metadata &aMetadata)
{
    Message *request = nullptr;

    for (Message &message : mPendingRequests)
    {
        aMetadata.ReadFrom(message);

        if (((aMetadata.mDestinationAddress == aMsg.mMessageInfo.GetPeerAddr() &&
              aMetadata.mDestinationPort == aMsg.mMessageInfo.GetPeerPort()) ||
             aMetadata.mDestinationAddress.IsMulticast() || aMetadata.mDestinationAddress.GetIid().IsAnycastLocator()))
        {
            switch (aMsg.mMessage.GetType())
            {
            case kTypeReset:
            case kTypeAck:
                if (aMsg.mMessage.GetMessageId() == message.GetMessageId())
                {
                    request = &message;
                    ExitNow();
                }

                break;

            case kTypeConfirmable:
            case kTypeNonConfirmable:
                if (aMsg.mMessage.HasSameTokenAs(message))
                {
                    request = &message;
                    ExitNow();
                }

                break;
            }
        }
    }

exit:
    return request;
}

void CoapBase::Receive(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Msg rxMsg(AsCoapMessage(&aMessage), aMessageInfo);

    if (rxMsg.mMessage.ParseHeader() != kErrorNone)
    {
        LogDebg("Failed to parse CoAP header");

        if (!aMessageInfo.GetSockAddr().IsMulticast() && rxMsg.mMessage.IsConfirmable())
        {
            IgnoreError(SendReset(rxMsg));
        }
    }
    else if (rxMsg.mMessage.IsRequest())
    {
        ProcessReceivedRequest(rxMsg);
    }
    else
    {
        ProcessReceivedResponse(rxMsg);
    }

#if OPENTHREAD_CONFIG_OTNS_ENABLE
    Get<Utils::Otns>().EmitCoapReceive(rxMsg.mMessage, aMessageInfo);
#endif
}

void CoapBase::ProcessReceivedResponse(Msg &aRxMsg)
{
    Metadata metadata;
    Message *request = nullptr;
    Error    error   = kErrorNone;
#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    bool shouldObserve = false;
#endif

    request = FindRelatedRequest(aRxMsg, metadata);
    VerifyOrExit(request != nullptr);

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
    // If there's an Observe option present in both request and
    // response, and we have a response handler; then we're dealing
    // with RFC7641 rules here. If there is no response handler, then
    // we're wasting our time!
    if (metadata.mObserve && request->IsRequest() && (metadata.mResponseHandler != nullptr))
    {
        Option::Iterator iterator;

        SuccessOrExit(error = iterator.Init(aRxMsg.mMessage, kOptionObserve));
        shouldObserve = !iterator.IsDone();
    }
#endif

    switch (aRxMsg.mMessage.GetType())
    {
    case kTypeReset:
        if (aRxMsg.mMessage.IsEmpty())
        {
            FinalizeCoapTransaction(*request, metadata, nullptr, kErrorAbort);
        }

        // Silently ignore non-empty reset messages (RFC 7252, Section 4.2).
        break;

    case kTypeAck:
        if (aRxMsg.mMessage.IsEmpty())
        {
            // Empty acknowledgment.

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            if (metadata.mObserve && !request->IsRequest())
            {
                // This is the ACK to our RFC7641 CON notification.
                // There will be no "separate" response so pass it back
                // as if it were a piggy-backed response so we can stop
                // re-sending and the application can move on.

                FinalizeCoapTransaction(*request, metadata, &aRxMsg, kErrorNone);
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
        else if (aRxMsg.mMessage.IsResponse() && aRxMsg.mMessage.HasSameTokenAs(*request))
        {
            // Piggybacked response.

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
            if (shouldObserve)
            {
                // This is a RFC7641 notification.  The request is *not* done!
                metadata.InvokeResponseHandler(&aRxMsg, kErrorNone);

                // Consider the message acknowledged at this point.
                metadata.mAcknowledged = true;
                metadata.UpdateIn(*request);
            }
            else
#endif
            {
#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
                SuccessOrExit(error = ProcessBlockwiseResponse(aRxMsg, *request, metadata));
#else
                FinalizeCoapTransaction(*request, metadata, &aRxMsg, kErrorNone);
#endif
            }
        }

        // Silently ignore acknowledgments carrying requests (RFC 7252, p. 4.2)
        // or with no token match (RFC 7252, p. 5.3.2)
        break;

    case kTypeConfirmable:
        // Send empty ACK if it is a CON message.
        IgnoreError(SendAck(aRxMsg));

        // Handling of RFC7641 and multicast is below.

        OT_FALL_THROUGH;

    case kTypeNonConfirmable:

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE
        if (shouldObserve)
        {
            metadata.InvokeResponseHandler(&aRxMsg, kErrorNone);

            // When any Observe response is seen, consider a NON observe
            // request "acknowledged" at this point. This will keep the
            // Observe request active indefinitely until it is
            // canceled.

            if (!metadata.mConfirmable)
            {
                metadata.mAcknowledged = true;
                metadata.UpdateIn(*request);
            }

            break;
        }
#endif

        // If the request was to a multicast address, then this is NOT
        // the final message, we may see more.

        if ((metadata.mResponseHandler != nullptr) && metadata.mDestinationAddress.IsMulticast())
        {
            metadata.InvokeResponseHandler(&aRxMsg, kErrorNone);
        }
        else
        {
            FinalizeCoapTransaction(*request, metadata, &aRxMsg, kErrorNone);
        }

        break;
    }

exit:

    if (error == kErrorNone && request == nullptr)
    {
        bool didHandle = InvokeResponseFallback(aRxMsg);

        if (!didHandle && aRxMsg.mMessage.RequireResetOnError())
        {
            // Successfully parsed a header but no matching request was
            // found - reject the message by sending reset.
            IgnoreError(SendReset(aRxMsg));
        }
    }
}

bool CoapBase::InvokeResponseFallback(Msg &aRxMsg) const
{
    bool didHandle = false;

    VerifyOrExit(mResponseFallback.IsSet());
    didHandle = mResponseFallback.Invoke(&aRxMsg.mMessage, &aRxMsg.mMessageInfo);

exit:
    return didHandle;
}

void CoapBase::ProcessReceivedRequest(Msg &aRxMsg)
{
    Message::UriPathStringBuffer uriPath;
    Error                        error = kErrorNone;

    if (mInterceptor.IsSet())
    {
        SuccessOrExit(error = mInterceptor.Invoke(aRxMsg.mMessage, aRxMsg.mMessageInfo));
    }

    // Check if `mResponseCache` has a matching cached response for this
    // request and send it. Only if not found (`kErrorNotFound`), we
    // continue to process the `aRxMsg.mMessage` further.

    error = mResponseCache.SendCachedResponse(aRxMsg, *this);

    switch (error)
    {
    case kErrorNotFound:
        break;
    case kErrorNone:
    default:
        ExitNow();
    }

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE
    {
        bool didHandle = false;

        SuccessOrExit(error = ProcessBlockwiseRequest(aRxMsg, uriPath, didHandle));
        VerifyOrExit(!didHandle);
    }
#else
    SuccessOrExit(error = aRxMsg.mMessage.ReadUriPathOptions(uriPath));
#endif

    if ((mResourceHandler != nullptr) && mResourceHandler(*this, uriPath, aRxMsg))
    {
        error = kErrorNone;
        ExitNow();
    }

    for (const Resource &resource : mResources)
    {
        if (StringMatch(resource.mUriPath, uriPath))
        {
            resource.HandleRequest(aRxMsg);
            error = kErrorNone;
            ExitNow();
        }
    }

    if (mDefaultHandler.IsSet())
    {
        mDefaultHandler.Invoke(&aRxMsg.mMessage, &aRxMsg.mMessageInfo);
        error = kErrorNone;
        ExitNow();
    }

    error = kErrorNotFound;

exit:

    if (error != kErrorNone)
    {
        LogInfo("Failed to process request: %s", ErrorToString(error));

        if (error == kErrorNotFound && !aRxMsg.mMessageInfo.GetSockAddr().IsMulticast())
        {
            IgnoreError(SendNotFound(aRxMsg));
        }
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// `CoapBase` - BLockwise transfer methods

#if OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

void CoapBase::AddBlockWiseResource(ResourceBlockWise &aResource) { IgnoreError(mBlockWiseResources.Add(aResource)); }

void CoapBase::RemoveBlockWiseResource(ResourceBlockWise &aResource)
{
    IgnoreError(mBlockWiseResources.Remove(aResource));
    aResource.SetNext(nullptr);
}

Error CoapBase::SendMessage(Message                &aMessage,
                            const Ip6::MessageInfo &aMessageInfo,
                            const TxParameters     *aTxParameters,
                            ResponseHandler         aHandler,
                            void                   *aContext)
{
    return SendMessage(aMessage, aMessageInfo, aTxParameters, aHandler, aContext, nullptr, nullptr);
}

Error CoapBase::ProcessBlockwiseSend(Message &aMessage, BlockwiseTransmitHook aTransmitHook, void *aContext)
{
    Error     error      = kErrorNone;
    uint8_t   type       = aMessage.GetType();
    bool      moreBlocks = false;
    uint16_t  blockSize;
    uint8_t   buf[kMaxBlockSize];
    BlockInfo blockInfo;

    VerifyOrExit(type != kTypeReset);

    VerifyOrExit(aTransmitHook != nullptr);

    SuccessOrExit(aMessage.ReadBlockOptionValues(type == kTypeAck ? kOptionBlock2 : kOptionBlock1, blockInfo));

    VerifyOrExit(blockInfo.mBlockNumber == 0);

    blockSize = blockInfo.GetBlockSize();
    VerifyOrExit(blockSize <= kMaxBlockSize, error = kErrorNoBufs);

    SuccessOrExit(error = aTransmitHook(aContext, buf, 0, &blockSize, &moreBlocks));
    SuccessOrExit(error = aMessage.AppendBytes(buf, blockSize));

    switch (type)
    {
    case kTypeAck:
        SuccessOrExit(error = CacheLastBlockResponse(&aMessage));
        break;

    case kTypeNonConfirmable:
        // Block-Wise messages always have to be confirmable
        aMessage.SetType(kTypeConfirmable);
        break;

    default:
        break;
    }

exit:
    return error;
}

Error CoapBase::ProcessBlockwiseResponse(Msg &aRxMsg, Message &aRequest, const Metadata &aMetadata)
{
    Error    error             = kErrorNone;
    uint8_t  blockOptionType   = 0;
    uint32_t totalTransferSize = 0;

    if (aMetadata.mBlockwiseTransmitHook != nullptr || aMetadata.mBlockwiseReceiveHook != nullptr)
    {
        // Search for CoAP Block-Wise Option [RFC7959]
        Option::Iterator iterator;

        SuccessOrExit(error = iterator.Init(aRxMsg.mMessage));

        while (!iterator.IsDone())
        {
            switch (iterator.GetOption()->GetNumber())
            {
            case kOptionBlock1:
                blockOptionType += 1;
                break;

            case kOptionBlock2:
                blockOptionType += 2;
                break;

            case kOptionSize2:
                // ToDo: wait for method to read uint option values
                totalTransferSize = 0;
                break;

            default:
                break;
            }

            SuccessOrExit(error = iterator.Advance());
        }
    }

    switch (blockOptionType)
    {
    case 0:
        // Piggybacked response.
        FinalizeCoapTransaction(aRequest, aMetadata, &aRxMsg, kErrorNone);
        break;
    case 1: // Block1 option
        if (aRxMsg.mMessage.GetCode() == kCodeContinue && aMetadata.mBlockwiseTransmitHook != nullptr)
        {
            error = SendNextBlock1Request(aRequest, aRxMsg, aMetadata);
        }

        if (aRxMsg.mMessage.GetCode() != kCodeContinue || aMetadata.mBlockwiseTransmitHook == nullptr ||
            error != kErrorNone)
        {
            FinalizeCoapTransaction(aRequest, aMetadata, &aRxMsg, error);
        }
        break;
    case 2: // Block2 option
        if (aRxMsg.mMessage.GetCode() < kCodeBadRequest && aMetadata.mBlockwiseReceiveHook != nullptr)
        {
            error = SendNextBlock2Request(aRequest, aRxMsg, aMetadata, totalTransferSize, false);
        }

        if (aRxMsg.mMessage.GetCode() >= kCodeBadRequest || aMetadata.mBlockwiseReceiveHook == nullptr ||
            error != kErrorNone)
        {
            FinalizeCoapTransaction(aRequest, aMetadata, &aRxMsg, error);
        }
        break;
    case 3: // Block1 & Block2 option
        if (aRxMsg.mMessage.GetCode() < kCodeBadRequest && aMetadata.mBlockwiseReceiveHook != nullptr)
        {
            error = SendNextBlock2Request(aRequest, aRxMsg, aMetadata, totalTransferSize, true);
        }

        FinalizeCoapTransaction(aRequest, aMetadata, &aRxMsg, error);
        break;
    default:
        error = kErrorAbort;
        FinalizeCoapTransaction(aRequest, aMetadata, &aRxMsg, error);
        break;
    }

exit:
    return error;
}

Error CoapBase::ProcessBlockwiseRequest(Msg &aRxMsg, Message::UriPathStringBuffer &aUriPath, bool &aDidHandle)
{
    Error            error = kErrorNone;
    Option::Iterator iterator;
    char            *curUriPath        = aUriPath;
    uint8_t          blockOptionType   = 0;
    uint32_t         totalTransferSize = 0;

    SuccessOrExit(error = iterator.Init(aRxMsg.mMessage));

    while (!iterator.IsDone())
    {
        switch (iterator.GetOption()->GetNumber())
        {
        case kOptionUriPath:
            if (curUriPath != aUriPath)
            {
                *curUriPath++ = '/';
            }

            VerifyOrExit(curUriPath + iterator.GetOption()->GetLength() < GetArrayEnd(aUriPath), error = kErrorParse);

            IgnoreError(iterator.ReadOptionValue(curUriPath));
            curUriPath += iterator.GetOption()->GetLength();
            break;

        case kOptionBlock1:
            blockOptionType += 1;
            break;

        case kOptionBlock2:
            blockOptionType += 2;
            break;

        case kOptionSize1:
            // ToDo: wait for method to read uint option values
            totalTransferSize = 0;
            break;

        default:
            break;
        }

        SuccessOrExit(error = iterator.Advance());
    }

    curUriPath[0] = kNullChar;

    for (const ResourceBlockWise &resource : mBlockWiseResources)
    {
        if (!StringMatch(resource.GetUriPath(), aUriPath))
        {
            continue;
        }

        if ((resource.mReceiveHook != nullptr || resource.mTransmitHook != nullptr) && blockOptionType != 0)
        {
            switch (blockOptionType)
            {
            case 1:
                if (resource.mReceiveHook != nullptr)
                {
                    switch (ProcessBlock1Request(aRxMsg, resource, totalTransferSize))
                    {
                    case kErrorNone:
                        resource.HandleRequest(aRxMsg);
                        OT_FALL_THROUGH;
                    case kErrorBusy:
                        error = kErrorNone;
                        break;
                    case kErrorNoBufs:
                        IgnoreError(SendHeaderResponse(kCodeRequestTooLarge, aRxMsg));
                        error = kErrorDrop;
                        break;
                    case kErrorNoFrameReceived:
                        IgnoreError(SendHeaderResponse(kCodeRequestIncomplete, aRxMsg));
                        error = kErrorDrop;
                        break;
                    default:
                        IgnoreError(SendHeaderResponse(kCodeInternalError, aRxMsg));
                        error = kErrorDrop;
                        break;
                    }
                }
                break;
            case 2:
                if (resource.mTransmitHook != nullptr)
                {
                    if ((error = ProcessBlock2Request(aRxMsg, resource)) != kErrorNone)
                    {
                        IgnoreError(SendHeaderResponse(kCodeInternalError, aRxMsg));
                        error = kErrorDrop;
                    }
                }
                break;
            }

            aDidHandle = true;
            ExitNow();
        }
        else
        {
            resource.HandleRequest(aRxMsg);
            error      = kErrorNone;
            aDidHandle = true;
            ExitNow();
        }
    }

exit:
    return error;
}

void CoapBase::FreeLastBlockResponse(void)
{
    if (mLastResponse != nullptr)
    {
        mLastResponse->Free();
        mLastResponse = nullptr;
    }
}

Error CoapBase::CacheLastBlockResponse(Message *aResponse)
{
    Error error = kErrorNone;

    FreeLastBlockResponse();

    if ((mLastResponse = aResponse->Clone()) == nullptr)
    {
        error = kErrorNoBufs;
    }

    return error;
}

Error CoapBase::PrepareNextBlockRequest(uint16_t         aBlockOptionNumber,
                                        Message         &aRequestOld,
                                        Message         &aRequest,
                                        const BlockInfo &aBlockInfo)
{
    Error            error       = kErrorNone;
    bool             isOptionSet = false;
    Option::Iterator iterator;
    Metadata         metadata;

    aRequest.Init(kTypeConfirmable, static_cast<ot::Coap::Code>(aRequestOld.GetCode()));

    metadata.ReadFrom(aRequestOld);
    metadata.RemoveFrom(aRequestOld);

    // Per RFC 7959, all requests in a block-wise transfer MUST use the
    // same token.
    IgnoreError(aRequest.WriteTokenFromMessage(aRequestOld));

    // Copy options from last response to next message

    SuccessOrExit(error = iterator.Init(aRequestOld));

    for (; !iterator.IsDone() && iterator.GetOption()->GetLength() != 0; error = iterator.Advance())
    {
        uint16_t optionNumber = iterator.GetOption()->GetNumber();

        SuccessOrExit(error);

        // Check if option to copy next is higher than or equal to Block1 option
        if (optionNumber >= aBlockOptionNumber && !isOptionSet)
        {
            SuccessOrExit(error = aRequest.AppendBlockOption(aBlockOptionNumber, aBlockInfo));

            isOptionSet = true;

            // If option to copy next is Block1 or Block2 option, option is not copied
            if (optionNumber == kOptionBlock1 || optionNumber == kOptionBlock2)
            {
                continue;
            }
        }

        // Copy option
        SuccessOrExit(error = aRequest.AppendOptionFromMessage(optionNumber, iterator.GetOption()->GetLength(),
                                                               iterator.GetMessage(),
                                                               iterator.GetOptionValueMessageOffset()));
    }

    if (!isOptionSet)
    {
        SuccessOrExit(error = aRequest.AppendBlockOption(aBlockOptionNumber, aBlockInfo));
    }

    error = metadata.AppendTo(aRequestOld);

exit:
    return error;
}

Error CoapBase::SendNextBlock1Request(Message &aRequest, Msg &aRxMsg, const Metadata &aMetadata)
{
    Error     error              = kErrorNone;
    Message  *request            = nullptr;
    uint8_t   buf[kMaxBlockSize] = {0};
    uint16_t  blockSize;
    BlockInfo msgBlockInfo;
    BlockInfo requestBlockInfo;

    SuccessOrExit(error = aRequest.ReadBlockOptionValues(kOptionBlock1, requestBlockInfo));
    SuccessOrExit(error = aRxMsg.mMessage.ReadBlockOptionValues(kOptionBlock1, msgBlockInfo));

    // Conclude block-wise transfer if last block has been received
    if (!requestBlockInfo.mMoreBlocks)
    {
        FinalizeCoapTransaction(aRequest, aMetadata, &aRxMsg, kErrorNone);
        ExitNow();
    }

    blockSize = msgBlockInfo.GetBlockSize();
    VerifyOrExit(blockSize <= kMaxBlockSize, error = kErrorNoBufs);

    requestBlockInfo.mBlockNumber = msgBlockInfo.mBlockNumber + 1;
    requestBlockInfo.mBlockSzx    = msgBlockInfo.mBlockSzx;
    requestBlockInfo.mMoreBlocks  = false;

    SuccessOrExit(error = aMetadata.mBlockwiseTransmitHook(aMetadata.mResponseContext, buf,
                                                           requestBlockInfo.GetBlockOffsetPosition(), &blockSize,
                                                           &requestBlockInfo.mMoreBlocks));

    VerifyOrExit(blockSize <= msgBlockInfo.GetBlockSize(), error = kErrorInvalidArgs);

    VerifyOrExit((request = NewMessage()) != nullptr, error = kErrorNoBufs);

    SuccessOrExit(error = PrepareNextBlockRequest(kOptionBlock1, aRequest, *request, requestBlockInfo));

    SuccessOrExit(error = request->SetPayloadMarker());

    SuccessOrExit(error = request->AppendBytes(buf, blockSize));

    DequeueMessage(aRequest);

    LogInfo("Send Block1 Nr. %d, Size: %d bytes, More Blocks Flag: %d", requestBlockInfo.mBlockNumber,
            requestBlockInfo.GetBlockSize(), requestBlockInfo.mMoreBlocks);

    SuccessOrExit(error = SendMessage(*request, aRxMsg.mMessageInfo, /* aTxParamters */ nullptr,
                                      aMetadata.mResponseHandler, aMetadata.mResponseContext,
                                      aMetadata.mBlockwiseTransmitHook, aMetadata.mBlockwiseReceiveHook));

exit:
    FreeMessageOnError(request, error);

    return error;
}

Error CoapBase::SendNextBlock2Request(Message        &aRequest,
                                      Msg            &aRxMsg,
                                      const Metadata &aMetadata,
                                      uint32_t        aTotalLength,
                                      bool            aBeginBlock1Transfer)
{
    Error       error   = kErrorNone;
    Message    *request = nullptr;
    uint8_t     buf[kMaxBlockSize];
    OffsetRange offsetRange;
    BlockInfo   msgBlockInfo;
    BlockInfo   requestBlockInfo;

    SuccessOrExit(error = aRxMsg.mMessage.ReadBlockOptionValues(kOptionBlock2, msgBlockInfo));

    VerifyOrExit(msgBlockInfo.GetBlockSize() <= kMaxBlockSize, error = kErrorNoBufs);

    offsetRange.InitFromMessageOffsetToEnd(aRxMsg.mMessage);
    VerifyOrExit(offsetRange.GetLength() <= msgBlockInfo.GetBlockSize(), error = kErrorNoBufs);

    aRxMsg.mMessage.ReadBytes(offsetRange, buf);
    SuccessOrExit(
        error = aMetadata.mBlockwiseReceiveHook(aMetadata.mResponseContext, buf, msgBlockInfo.GetBlockOffsetPosition(),
                                                offsetRange.GetLength(), msgBlockInfo.mMoreBlocks, aTotalLength));

    LogInfo("Received Block2 Nr. %d , Size: %d bytes, More Blocks Flag: %d", msgBlockInfo.mBlockNumber,
            msgBlockInfo.GetBlockSize(), msgBlockInfo.mMoreBlocks);

    if (!msgBlockInfo.mMoreBlocks)
    {
        FinalizeCoapTransaction(aRequest, aMetadata, &aRxMsg, kErrorNone);
        ExitNow();
    }

    VerifyOrExit((request = NewMessage()) != nullptr, error = kErrorNoBufs);

    requestBlockInfo = msgBlockInfo;
    requestBlockInfo.mBlockNumber++;

    SuccessOrExit(error = PrepareNextBlockRequest(kOptionBlock2, aRequest, *request, requestBlockInfo));

    if (!aBeginBlock1Transfer)
    {
        DequeueMessage(aRequest);
    }

    LogInfo("Request Block2 Nr. %d, Size: %d bytes", requestBlockInfo.mBlockNumber, requestBlockInfo.GetBlockSize());

    SuccessOrExit(error = SendMessage(*request, aRxMsg.mMessageInfo, /* aTxParameters */ nullptr,
                                      aMetadata.mResponseHandler, aMetadata.mResponseContext, nullptr,
                                      aMetadata.mBlockwiseReceiveHook));

exit:
    FreeMessageOnError(request, error);

    return error;
}

Error CoapBase::ProcessBlock1Request(Msg &aRxMsg, const ResourceBlockWise &aResource, uint32_t aTotalLength)
{
    Error       error    = kErrorNone;
    Message    *response = nullptr;
    uint8_t     buf[kMaxBlockSize];
    OffsetRange offsetRange;
    BlockInfo   msgBlockInfo;

    SuccessOrExit(error = aRxMsg.mMessage.ReadBlockOptionValues(kOptionBlock1, msgBlockInfo));

    offsetRange.InitFromMessageOffsetToEnd(aRxMsg.mMessage);
    VerifyOrExit(offsetRange.GetLength() <= kMaxBlockSize, error = kErrorNoBufs);

    aRxMsg.mMessage.ReadBytes(offsetRange, buf);
    SuccessOrExit(error =
                      aResource.HandleBlockReceive(buf, msgBlockInfo.GetBlockOffsetPosition(), offsetRange.GetLength(),
                                                   msgBlockInfo.mMoreBlocks, aTotalLength));

    if (msgBlockInfo.mMoreBlocks)
    {
        // Set up next response
        VerifyOrExit((response = NewMessage()) != nullptr, error = kErrorFailed);
        response->Init(kTypeAck, kCodeContinue);
        response->SetMessageId(aRxMsg.mMessage.GetMessageId());
        SuccessOrExit(error = response->WriteTokenFromMessage(aRxMsg.mMessage));

        SuccessOrExit(error = response->AppendBlockOption(kOptionBlock1, msgBlockInfo));

        SuccessOrExit(error = CacheLastBlockResponse(response));

        LogInfo("Acknowledge Block1 Nr. %d, Size: %d bytes", msgBlockInfo.mBlockNumber, msgBlockInfo.GetBlockSize());

        SuccessOrExit(error = SendMessage(*response, aRxMsg.mMessageInfo));

        error = kErrorBusy;
    }
    else
    {
        // Conclude block-wise transfer if last block has been received
        FreeLastBlockResponse();
        error = kErrorNone;
    }

exit:
    if (error != kErrorNone && error != kErrorBusy && response != nullptr)
    {
        response->Free();
    }

    return error;
}

Error CoapBase::ProcessBlock2Request(Msg &aRxMsg, const ResourceBlockWise &aResource)
{
    Error            error              = kErrorNone;
    Message         *response           = nullptr;
    uint64_t         optionBuf          = 0;
    uint8_t          buf[kMaxBlockSize] = {0};
    uint16_t         blockSize;
    Option::Iterator iterator;
    BlockInfo        msgBlockInfo;
    BlockInfo        responseBlockInfo;

    SuccessOrExit(error = aRxMsg.mMessage.ReadBlockOptionValues(kOptionBlock2, msgBlockInfo));

    LogInfo("Request for Block2 Nr. %d, Size: %d bytes received", msgBlockInfo.mBlockNumber,
            msgBlockInfo.GetBlockSize());

    if (msgBlockInfo.mBlockNumber == 0)
    {
        aResource.HandleRequest(aRxMsg);
        ExitNow();
    }

    VerifyOrExit((response = NewMessage()) != nullptr, error = kErrorNoBufs);
    response->Init(kTypeAck, kCodeContent);
    response->SetMessageId(aRxMsg.mMessage.GetMessageId());

    SuccessOrExit(error = response->WriteTokenFromMessage(aRxMsg.mMessage));

    responseBlockInfo.mMoreBlocks = false;

    VerifyOrExit((blockSize = msgBlockInfo.GetBlockSize()) <= kMaxBlockSize, error = kErrorNoBufs);
    SuccessOrExit(error = aResource.HandleBlockTransmit(buf, msgBlockInfo.GetBlockOffsetPosition(), &blockSize,
                                                        &responseBlockInfo.mMoreBlocks));

    if (responseBlockInfo.mMoreBlocks)
    {
        SuccessOrExit(error = DetermineBlockSzxFromSize(blockSize, responseBlockInfo.mBlockSzx));
    }
    else
    {
        VerifyOrExit(blockSize <= msgBlockInfo.GetBlockSize(), error = kErrorInvalidArgs);
        responseBlockInfo.mBlockSzx = msgBlockInfo.mBlockSzx;
    }

    responseBlockInfo.mBlockNumber = msgBlockInfo.GetBlockOffsetPosition() / responseBlockInfo.GetBlockSize();

    // Copy options from last response
    SuccessOrExit(error = iterator.Init(*mLastResponse));

    while (!iterator.IsDone())
    {
        uint16_t optionNumber = iterator.GetOption()->GetNumber();

        if (optionNumber == kOptionBlock2)
        {
            SuccessOrExit(error = response->AppendBlockOption(kOptionBlock2, responseBlockInfo));
        }
        else if (optionNumber == kOptionBlock1)
        {
            SuccessOrExit(error = iterator.ReadOptionValue(&optionBuf));
            SuccessOrExit(error = response->AppendOption(optionNumber, iterator.GetOption()->GetLength(), &optionBuf));
        }

        SuccessOrExit(error = iterator.Advance());
    }

    SuccessOrExit(error = response->SetPayloadMarker());
    SuccessOrExit(error = response->AppendBytes(buf, blockSize));

    if (responseBlockInfo.mMoreBlocks)
    {
        SuccessOrExit(error = CacheLastBlockResponse(response));
    }
    else
    {
        // Conclude block-wise transfer if last block has been received
        FreeLastBlockResponse();
    }

    LogInfo("Send Block2 Nr. %d, Size: %d bytes, More Blocks Flag %d", responseBlockInfo.mBlockNumber,
            responseBlockInfo.GetBlockSize(), responseBlockInfo.mMoreBlocks);

    SuccessOrExit(error = SendMessage(*response, aRxMsg.mMessageInfo));

exit:
    FreeMessageOnError(response, error);

    return error;
}

Error CoapBase::DetermineBlockSzxFromSize(uint16_t aSize, BlockSzx &aBlockSzx)
{
    Error error = kErrorNone;

    for (uint8_t szx = kBlockSzx16; szx <= kBlockSzx1024; szx++)
    {
        aBlockSzx = static_cast<BlockSzx>(szx);

        if (BlockSizeFromExponent(aBlockSzx) == aSize)
        {
            ExitNow();
        }
    }

    error = kErrorInvalidArgs;

exit:
    return error;
}

#endif // OPENTHREAD_CONFIG_COAP_BLOCKWISE_TRANSFER_ENABLE

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// `CoapBase` - Observe methods

#if OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE

Error CoapBase::ProcessObserveSend(Msg &aTxMsg, bool &aShouldObserve)
{
    Error            error;
    Option::Iterator iterator;

    aShouldObserve = false;

    SuccessOrExit(error = iterator.Init(aTxMsg.mMessage, kOptionObserve));
    aShouldObserve = !iterator.IsDone();

    // Special case, if we're sending a GET with Observe=1, that is a
    // cancellation.

    if (aShouldObserve && aTxMsg.mMessage.IsGetRequest())
    {
        uint64_t value = 0;

        SuccessOrExit(error = iterator.ReadOptionValue(value));

        if (value == 1)
        {
            Message *request;
            Metadata metadata;

            aShouldObserve = false;

            // If we can find the previous matching request, cancel that too.

            request = FindRelatedRequest(aTxMsg, metadata);

            if (request != nullptr)
            {
                FinalizeCoapTransaction(*request, metadata, nullptr, kErrorNone);
            }
        }
    }

exit:
    return error;
}

bool CoapBase::IsObserveSubscription(const Message &aMessage, const Metadata &aMetadata)
{
    // Indicate whether the message is an RFC7641 subscription which
    // is already acknowledged.

    return aMessage.IsRequest() && aMetadata.mObserve && aMetadata.mAcknowledged;
}

#endif // OPENTHREAD_CONFIG_COAP_OBSERVE_API_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// CoapBase::Metadata

void CoapBase::Metadata::InvokeResponseHandler(Msg *aMsg, Error aResult) const
{
    if (mResponseHandler != nullptr)
    {
        Message                *message     = (aMsg != nullptr) ? &aMsg->mMessage : nullptr;
        const Ip6::MessageInfo *messageInfo = (aMsg != nullptr) ? &aMsg->mMessageInfo : nullptr;

        mResponseHandler(mResponseContext, message, messageInfo, aResult);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// CoapBase::ResponseCache

CoapBase::ResponseCache::ResponseCache(Instance &aInstance)
    : mTimer(aInstance, ResponseCache::HandleTimer, this)
{
}

Error CoapBase::ResponseCache::SendCachedResponse(const Msg &aRxMsg, CoapBase &aCoapBase)
{
    // Search `ResponseCache` for a cached response matching the given
    // request `aRxMsg`. If found, clone the response and send it. Returns
    // `kErrorNotFound` if no match is found, `kErrorNone` on success,
    // or other errors if send fails.

    Error          error    = kErrorNone;
    const Message *match    = FindMatching(aRxMsg);
    Message       *response = nullptr;

    VerifyOrExit(match != nullptr, error = kErrorNotFound);

    response = match->Clone(match->GetLength() - sizeof(ResponseMetadata));
    VerifyOrExit(response != nullptr, error = kErrorNoBufs);

    response->Finish();

    error = aCoapBase.Send(*response, aRxMsg.mMessageInfo);

exit:
    FreeMessageOnError(response, error);
    return error;
}

const Message *CoapBase::ResponseCache::FindMatching(const Msg &aRxMsg) const
{
    const Message *match        = nullptr;
    uint16_t       requestMsgId = aRxMsg.mMessage.GetMessageId();

    for (const Message &response : mResponses)
    {
        if (response.GetMessageId() == requestMsgId)
        {
            ResponseMetadata metadata;

            metadata.ReadFrom(response);

            if (metadata.mMessageInfo.HasSamePeerAddrAndPort(aRxMsg.mMessageInfo))
            {
                match = &response;
                break;
            }
        }
    }

    return match;
}

void CoapBase::ResponseCache::Add(const Msg &aTxMsg, uint32_t aExchangeLifetime)
{
    // Adds a clone of the `aTxMsg` to the cache if a matching
    // entry does not already exist.

    Message         *responseClone = nullptr;
    ResponseMetadata metadata;

    VerifyOrExit(FindMatching(aTxMsg) == nullptr);

    MaintainCacheSize();

    responseClone = aTxMsg.mMessage.Clone();
    VerifyOrExit(responseClone != nullptr);

    metadata.mExpireTime  = TimerMilli::GetNow() + aExchangeLifetime;
    metadata.mMessageInfo = aTxMsg.mMessageInfo;

    SuccessOrExit(metadata.AppendTo(*responseClone));

    mResponses.Enqueue(*responseClone);
    responseClone = nullptr;

    mTimer.FireAtIfEarlier(metadata.mExpireTime);

exit:
    FreeMessage(responseClone);
}

void CoapBase::ResponseCache::MaintainCacheSize(void)
{
    // Checks the cache size. If the limit (`kMaxCacheSize`) is
    // reached, removes the entry with the earliest expire time.

    uint16_t  count       = 0;
    Message  *msgToRemove = nullptr;
    TimeMilli earliestExpireTime;

    for (Message &response : mResponses)
    {
        ResponseMetadata metadata;

        metadata.ReadFrom(response);

        if ((msgToRemove == nullptr) || (metadata.mExpireTime < earliestExpireTime))
        {
            msgToRemove        = &response;
            earliestExpireTime = metadata.mExpireTime;
        }

        count++;
    }

    if (count >= kMaxCacheSize)
    {
        mResponses.DequeueAndFree(*msgToRemove);
    }
}

void CoapBase::ResponseCache::RemoveAll(void)
{
    mResponses.DequeueAndFreeAll();
    mTimer.Stop();
}

void CoapBase::ResponseCache::HandleTimer(Timer &aTimer)
{
    static_cast<ResponseCache *>(static_cast<TimerMilliContext &>(aTimer).GetContext())->HandleTimer();
}

void CoapBase::ResponseCache::HandleTimer(void)
{
    NextFireTime expireTime;

    for (Message &response : mResponses)
    {
        ResponseMetadata metadata;

        metadata.ReadFrom(response);

        if (expireTime.GetNow() >= metadata.mExpireTime)
        {
            mResponses.DequeueAndFree(response);
        }
        else
        {
            expireTime.UpdateIfEarlier(metadata.mExpireTime);
        }
    }

    mTimer.FireAt(expireTime);
}

//---------------------------------------------------------------------------------------------------------------------
// TxParameters

const otCoapTxParameters TxParameters::kDefaultTxParameters = {
    kDefaultAckTimeout,
    kDefaultAckRandomFactorNumerator,
    kDefaultAckRandomFactorDenominator,
    kDefaultMaxRetransmit,
};

const TxParameters &TxParameters::GetDefault(void)
{
    // Validate the default `TxParameters` at compile-time

    static constexpr uint64_t kMaxDuration = static_cast<uint64_t>(kDefaultAckTimeout) *
                                                 kDefaultAckRandomFactorNumerator *
                                                 (1UL << (kDefaultMaxRetransmit + 1)) +
                                             2 * kDefaultMaxLatency;

    static_assert(kDefaultAckRandomFactorDenominator > 0, "kDefaultAckRandomFactorDenominator MUST be non-zero");
    static_assert(kDefaultAckRandomFactorNumerator >= kDefaultAckRandomFactorDenominator, "Numerator is invalid");
    static_assert(kMinAckTimeout > 0, "kMinAckTimeout MUST be non-zero");
    static_assert(kDefaultAckTimeout >= kMinAckTimeout, "kDefaultAckTimeout is invalid");
    static_assert(kMaxRetransmit > 0, "kMaxRetransmit MUST be non-zero");
    static_assert(kMaxRetransmit < 31, "kMaxRetransmit is not valid");
    static_assert(kDefaultMaxRetransmit <= kMaxRetransmit, "kDefaultMaxRetransmit is invalid");
    static_assert(kMaxDuration < NumericLimits<uint32_t>::kMax, "Default `TxParameters` is invalid");

    return AsCoreType(&kDefaultTxParameters);
}

Error TxParameters::ValidateFor(const Message &aMessage) const
{
    Error    error = kErrorInvalidArgs;
    uint32_t duration;
    uint32_t retryFactor;

    if (mAckTimeout == 0)
    {
        // Fire and forget is only allowed for non-confirmable messages.
        VerifyOrExit(aMessage.IsNonConfirmable());
        error = kErrorNone;
        ExitNow();
    }

    VerifyOrExit(mAckRandomFactorDenominator > 0);
    VerifyOrExit(mAckRandomFactorNumerator >= mAckRandomFactorDenominator);
    VerifyOrExit(mAckTimeout >= kMinAckTimeout);
    VerifyOrExit(mMaxRetransmit <= kMaxRetransmit);

    // Calculate exchange lifetime max duration step by step and verify no overflow.

    retryFactor = static_cast<uint32_t>((1U << (mMaxRetransmit + 1)) - 1);
    SuccessOrExit(SafeMultiply<uint32_t>(mAckTimeout, retryFactor, duration));

    SuccessOrExit(SafeMultiply<uint32_t>(duration, mAckRandomFactorNumerator, duration));
    duration /= mAckRandomFactorDenominator;

    VerifyOrExit(duration > 0);
    VerifyOrExit(CanAddSafely<uint32_t>(mAckTimeout, 2 * kDefaultMaxLatency));
    VerifyOrExit(CanAddSafely<uint32_t>(duration, mAckTimeout + 2 * kDefaultMaxLatency));

    error = kErrorNone;

exit:
    return error;
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

uint32_t TxParameters::CalculateMaxTransmitWait(void) const { return CalculateSpan(mMaxRetransmit + 1); }

uint32_t TxParameters::CalculateSpan(uint8_t aMaxRetx) const
{
    return static_cast<uint32_t>(mAckTimeout * ((1U << aMaxRetx) - 1) / mAckRandomFactorDenominator *
                                 mAckRandomFactorNumerator);
}

//---------------------------------------------------------------------------------------------------------------------
// Resource

Resource::Resource(const char *aUriPath, RequestHandler aHandler, void *aContext)
{
    mUriPath = aUriPath;
    mHandler = aHandler;
    mContext = aContext;
    mNext    = nullptr;
}

Resource::Resource(Uri aUri, RequestHandler aHandler, void *aContext)
    : Resource(PathForUri(aUri), aHandler, aContext)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Coap

Coap::Coap(Instance &aInstance)
    : CoapBase(aInstance, &Coap::Send)
    , mSocket(aInstance, *this)
{
}

Error Coap::Start(uint16_t aPort, Ip6::NetifIdentifier aNetifIdentifier)
{
    Error error        = kErrorNone;
    bool  socketOpened = false;

    VerifyOrExit(!mSocket.IsBound());

    SuccessOrExit(error = mSocket.Open(aNetifIdentifier));
    socketOpened = true;

    SuccessOrExit(error = mSocket.Bind(aPort));

exit:
    if (error != kErrorNone && socketOpened)
    {
        IgnoreError(mSocket.Close());
    }

    return error;
}

Error Coap::Stop(void)
{
    Error error = kErrorNone;

    VerifyOrExit(mSocket.IsBound());

    SuccessOrExit(error = mSocket.Close());
    ClearAllRequestsAndResponses();

exit:
    return error;
}

void Coap::HandleUdpReceive(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Receive(AsCoapMessage(&aMessage), aMessageInfo);
}

Error Coap::Send(CoapBase &aCoapBase, ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return static_cast<Coap &>(aCoapBase).Send(aMessage, aMessageInfo);
}

Error Coap::Send(ot::Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return mSocket.IsBound() ? mSocket.SendTo(aMessage, aMessageInfo) : kErrorInvalidState;
}

} // namespace Coap
} // namespace ot
