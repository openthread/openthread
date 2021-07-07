/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file implements the OpenThread TCP API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_TCP_ENABLE

#include <openthread/tcp.h>

#include "common/instance.hpp"
#include "net/tcp6.hpp"

using namespace ot;

otError otTcpEndpointInitialize(otInstance *aInstance, otTcpEndpoint *aEndpoint, otTcpEndpointInitializeArgs *aArgs)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.Initialize(*static_cast<Instance *>(aInstance), *aArgs);
}

otInstance *otTcpEndpointGetInstance(otTcpEndpoint *aEndpoint)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return &endpoint.GetInstance();
}

void *otTcpEndpointGetContext(otTcpEndpoint *aEndpoint)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.GetContext();
}

const otSockAddr *otTcpGetLocalAddress(const otTcpEndpoint *aEndpoint)
{
    const Ip6::Tcp::Endpoint &endpoint = *static_cast<const Ip6::Tcp::Endpoint *>(aEndpoint);

    return &endpoint.GetLocalAddress();
}

const otSockAddr *otTcpGetPeerAddress(const otTcpEndpoint *aEndpoint)
{
    const Ip6::Tcp::Endpoint &endpoint = *static_cast<const Ip6::Tcp::Endpoint *>(aEndpoint);

    return &endpoint.GetPeerAddress();
}

otError otTcpBind(otTcpEndpoint *aEndpoint, const otSockAddr *aSockName)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.Bind(*static_cast<const Ip6::SockAddr *>(aSockName));
}

otError otTcpConnect(otTcpEndpoint *aEndpoint, const otSockAddr *aSockName, uint32_t aFlags)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.Connect(*static_cast<const Ip6::SockAddr *>(aSockName), aFlags);
}

otError otTcpSendByReference(otTcpEndpoint *aEndpoint, otLinkedBuffer *aBuffer, uint32_t aFlags)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.SendByReference(*aBuffer, aFlags);
}

otError otTcpSendByExtension(otTcpEndpoint *aEndpoint, size_t aNumBytes, uint32_t aFlags)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.SendByExtension(aNumBytes, aFlags);
}

otError otTcpReceiveByReference(const otTcpEndpoint *aEndpoint, const otLinkedBuffer **aBuffer)
{
    const Ip6::Tcp::Endpoint &endpoint = *static_cast<const Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.ReceiveByReference(*aBuffer);
}

otError otTcpReceiveContiguify(otTcpEndpoint *aEndpoint)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.ReceiveContiguify();
}

otError otTcpCommitReceive(otTcpEndpoint *aEndpoint, size_t aNumBytes, uint32_t aFlags)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.CommitReceive(aNumBytes, aFlags);
}

otError otTcpSendEndOfStream(otTcpEndpoint *aEndpoint)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.SendEndOfStream();
}

otError otTcpAbort(otTcpEndpoint *aEndpoint)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.Abort();
}

otError otTcpEndpointDeinitialize(otTcpEndpoint *aEndpoint)
{
    Ip6::Tcp::Endpoint &endpoint = *static_cast<Ip6::Tcp::Endpoint *>(aEndpoint);

    return endpoint.Deinitialize();
}

otError otTcpListenerInitialize(otInstance *aInstance, otTcpListener *aListener, otTcpListenerInitializeArgs *aArgs)
{
    Ip6::Tcp::Listener &listener = *static_cast<Ip6::Tcp::Listener *>(aListener);

    return listener.Initialize(*static_cast<Instance *>(aInstance), *aArgs);
}

otInstance *otTcpListenerGetInstance(otTcpListener *aListener)
{
    Ip6::Tcp::Listener &listener = *static_cast<Ip6::Tcp::Listener *>(aListener);

    return &listener.GetInstance();
}

void *otTcpListenerGetContext(otTcpListener *aListener)
{
    Ip6::Tcp::Listener &listener = *static_cast<Ip6::Tcp::Listener *>(aListener);

    return listener.GetContext();
}

otError otTcpListen(otTcpListener *aListener, const otSockAddr *aSockName)
{
    Ip6::Tcp::Listener &listener = *static_cast<Ip6::Tcp::Listener *>(aListener);

    return listener.Listen(*static_cast<const Ip6::SockAddr *>(aSockName));
}

otError otTcpStopListening(otTcpListener *aListener)
{
    Ip6::Tcp::Listener &listener = *static_cast<Ip6::Tcp::Listener *>(aListener);

    return listener.StopListening();
}

otError otTcpListenerDeinitialize(otTcpListener *aListener)
{
    Ip6::Tcp::Listener &listener = *static_cast<Ip6::Tcp::Listener *>(aListener);

    return listener.Deinitialize();
}

#endif // OPENTHREAD_CONFIG_TCP_ENABLE
