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

namespace Thread {
namespace Coap {

Server::Server(Ip6::Udp &aUdp, uint16_t aPort, SenderFunction aSender, ReceiverFunction aReceiver):
    CoapBase(aUdp, aSender, aReceiver)
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

ThreadError Server::SendMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    return mSender(this, aMessage, aMessageInfo);
}

void Server::ProcessReceivedMessage(Message &aMessage, const Ip6::MessageInfo &aMessageInfo)
{
    Header header;
    char uriPath[Resource::kMaxReceivedUriPath] = "";
    char *curUriPath = uriPath;
    const Header::Option *coapOption;

    SuccessOrExit(header.FromMessage(aMessage, false));
    aMessage.MoveOffset(header.GetLength());

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
    {}
}

ThreadError Server::SetPort(uint16_t aPort)
{
    mPort = aPort;

    Ip6::SockAddr sockaddr;
    sockaddr.mPort = mPort;

    return mSocket.Bind(sockaddr);
}

}  // namespace Coap
}  // namespace Thread
