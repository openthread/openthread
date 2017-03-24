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

/**
 * @file
 *   This file implements the CoAP server message dispatch.
 */

#include <coap/coap_server.hpp>
#include <common/code_utils.hpp>
#include <net/ip6.hpp>

namespace Thread {
namespace Coap {

Server::Server(Ip6::Netif &aNetif, uint16_t aPort, SenderFunction aSender, ReceiverFunction aReceiver):
    CoapBase(aNetif.GetIp6().mUdp, aSender, aReceiver),
    mResponsesQueue(aNetif)
{
    mPort = aPort;
    mResources = NULL;
}

ThreadError Server::Start(void)
{
    Ip6::SockAddr sockaddr;
    sockaddr.mPort = mPort;

    return CoapBase::Start(sockaddr);
}

ThreadError Server::Stop(void)
{
    mResponsesQueue.DequeueAllResponses();

    return CoapBase::Stop();
}

ThreadError Server::AddResource(Resource &aResource)
{
    ThreadError error = kThreadError_None;

    for (Resource *cur = mResources; cur; cur = cur->GetNext())
    {
        VerifyOrExit(cur != &aResource, error = kThreadError_Already);
    }

    aResource.mNext = mResources;
    mResources = &aResource;

exit:
    return error;
}

void Server::RemoveResource(Resource &aResource)
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

Message *Server::NewMessage(uint16_t aReserved)
{
    return mSocket.NewMessage(aReserved);
}

Message *Server::NewMeshCoPMessage(uint16_t aReserved)
{
    Message *message = NULL;

    VerifyOrExit((message = NewMessage(aReserved)) != NULL, ;);

    message->SetPriority(kMeshCoPMessagePriority);

exit:
    return message;
}

ThreadError Server::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    mResponsesQueue.EnqueueResponse(aMessage, aMessageInfo);

    return mSender(this, aMessage, aMessageInfo);
}

ThreadError Server::SendEmptyAck(const Header &aRequestHeader, const Ip6::MessageInfo &aMessageInfo)
{
    ThreadError error = kThreadError_None;
    Coap::Header responseHeader;
    Message *message = NULL;

    VerifyOrExit(aRequestHeader.GetType() == kCoapTypeConfirmable, error = kThreadError_InvalidArgs);

    responseHeader.SetDefaultResponseHeader(aRequestHeader);

    VerifyOrExit((message = NewMessage(responseHeader)) != NULL, error = kThreadError_NoBufs);

    SuccessOrExit(error = SendMessage(*message, aMessageInfo));

exit:

    if (error != kThreadError_None && message != NULL)
    {
        message->Free();
    }

    return error;
}

void Server::ProcessReceivedMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Header header;
    char uriPath[Resource::kMaxReceivedUriPath] = "";
    char *curUriPath = uriPath;
    const Header::Option *coapOption;
    Message *response;

    SuccessOrExit(header.FromMessage(aMessage, 0));
    aMessage.MoveOffset(header.GetLength());

    switch (mResponsesQueue.GetMatchedResponseCopy(header, aMessageInfo, &response))
    {
    case kThreadError_None:
        SendMessage(*response, aMessageInfo);

    // fall through

    case kThreadError_NoBufs:
        ExitNow();

    case kThreadError_NotFound:
    default:
        break;
    }

    coapOption = header.GetCurrentOption();

    while (coapOption != NULL)
    {
        switch (coapOption->mNumber)
        {
        case kCoapOptionUriPath:
            if (curUriPath != uriPath)
            {
                *curUriPath++ = '/';
            }

            VerifyOrExit(coapOption->mLength < sizeof(uriPath) - static_cast<size_t>(curUriPath + 1 - uriPath), ;);

            memcpy(curUriPath, coapOption->mValue, coapOption->mLength);
            curUriPath += coapOption->mLength;
            break;

        case kCoapOptionAccept:
        case kCoapOptionContentFormat:
            break;

        default:
            ExitNow();
        }

        coapOption = header.GetNextOption();
    }

    curUriPath[0] = '\0';

    for (Resource *resource = mResources; resource; resource = resource->GetNext())
    {
        if (strcmp(resource->mUriPath, uriPath) == 0)
        {
            resource->HandleRequest(header, aMessage, aMessageInfo);
            ExitNow();
        }
    }

exit:
    return;
}

ThreadError Server::SetPort(uint16_t aPort)
{
    mPort = aPort;

    Ip6::SockAddr sockaddr;
    sockaddr.mPort = mPort;

    return mSocket.Bind(sockaddr);
}

ThreadError ResponsesQueue::GetMatchedResponseCopy(const Header &aHeader,
                                                   const Ip6::MessageInfo &aMessageInfo,
                                                   Message **aResponse)
{
    ThreadError            error = kThreadError_NotFound;
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
        if (header.FromMessage(*message, sizeof(EnqueuedResponseHeader)) != kThreadError_None)
        {
            continue;
        }

        if (header.GetMessageId() != aHeader.GetMessageId())
        {
            continue;
        }

        *aResponse = message->Clone();
        VerifyOrExit(*aResponse != NULL, error = kThreadError_NoBufs);

        EnqueuedResponseHeader::RemoveFrom(**aResponse);

        error = kThreadError_None;
        break;
    }

exit:
    return error;
}

ThreadError ResponsesQueue::GetMatchedResponseCopy(const Message &aRequest,
                                                   const Ip6::MessageInfo &aMessageInfo,
                                                   Message **aResponse)
{
    ThreadError error = kThreadError_None;
    Header      header;

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
    VerifyOrExit(header.GetType() == kCoapTypeAcknowledgment ||
                 header.GetType() == kCoapTypeReset,);

    switch (GetMatchedResponseCopy(aMessage, aMessageInfo, &copy))
    {
    case kThreadError_NotFound:
        break;

    case kThreadError_None:
        copy->Free();

    // fall through

    case kThreadError_NoBufs:
    default:
        ExitNow();
    }

    mQueue.GetInfo(messageCount, bufferCount);

    if (messageCount >= kMaxCachedResponses)
    {
        DequeueOldestResponse();
    }

    copy = aMessage.Clone();
    VerifyOrExit(copy != NULL,);

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

    VerifyOrExit((message = mQueue.GetHead()) != NULL,);
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
}  // namespace Thread
