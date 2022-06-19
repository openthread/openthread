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

#include "common/as_core_type.hpp"
#include "common/locator_getters.hpp"

using namespace ot;

otError otTcpEndpointInitialize(otInstance *                       aInstance,
                                otTcpEndpoint *                    aEndpoint,
                                const otTcpEndpointInitializeArgs *aArgs)
{
    return AsCoreType(aEndpoint).Initialize(AsCoreType(aInstance), *aArgs);
}

otInstance *otTcpEndpointGetInstance(otTcpEndpoint *aEndpoint)
{
    return &AsCoreType(aEndpoint).GetInstance();
}

void *otTcpEndpointGetContext(otTcpEndpoint *aEndpoint)
{
    return AsCoreType(aEndpoint).GetContext();
}

const otSockAddr *otTcpGetLocalAddress(const otTcpEndpoint *aEndpoint)
{
    return &AsCoreType(aEndpoint).GetLocalAddress();
}

const otSockAddr *otTcpGetPeerAddress(const otTcpEndpoint *aEndpoint)
{
    return &AsCoreType(aEndpoint).GetPeerAddress();
}

otError otTcpBind(otTcpEndpoint *aEndpoint, const otSockAddr *aSockName)
{
    return AsCoreType(aEndpoint).Bind(AsCoreType(aSockName));
}

otError otTcpConnect(otTcpEndpoint *aEndpoint, const otSockAddr *aSockName, uint32_t aFlags)
{
    return AsCoreType(aEndpoint).Connect(AsCoreType(aSockName), aFlags);
}

otError otTcpSendByReference(otTcpEndpoint *aEndpoint, otLinkedBuffer *aBuffer, uint32_t aFlags)
{
    return AsCoreType(aEndpoint).SendByReference(*aBuffer, aFlags);
}

otError otTcpSendByExtension(otTcpEndpoint *aEndpoint, size_t aNumBytes, uint32_t aFlags)
{
    return AsCoreType(aEndpoint).SendByExtension(aNumBytes, aFlags);
}

otError otTcpReceiveByReference(otTcpEndpoint *aEndpoint, const otLinkedBuffer **aBuffer)
{
    return AsCoreType(aEndpoint).ReceiveByReference(*aBuffer);
}

otError otTcpReceiveContiguify(otTcpEndpoint *aEndpoint)
{
    return AsCoreType(aEndpoint).ReceiveContiguify();
}

otError otTcpCommitReceive(otTcpEndpoint *aEndpoint, size_t aNumBytes, uint32_t aFlags)
{
    return AsCoreType(aEndpoint).CommitReceive(aNumBytes, aFlags);
}

otError otTcpSendEndOfStream(otTcpEndpoint *aEndpoint)
{
    return AsCoreType(aEndpoint).SendEndOfStream();
}

otError otTcpAbort(otTcpEndpoint *aEndpoint)
{
    return AsCoreType(aEndpoint).Abort();
}

otError otTcpEndpointDeinitialize(otTcpEndpoint *aEndpoint)
{
    return AsCoreType(aEndpoint).Deinitialize();
}

otError otTcpListenerInitialize(otInstance *                       aInstance,
                                otTcpListener *                    aListener,
                                const otTcpListenerInitializeArgs *aArgs)
{
    return AsCoreType(aListener).Initialize(AsCoreType(aInstance), *aArgs);
}

otInstance *otTcpListenerGetInstance(otTcpListener *aListener)
{
    return &AsCoreType(aListener).GetInstance();
}

void *otTcpListenerGetContext(otTcpListener *aListener)
{
    return AsCoreType(aListener).GetContext();
}

otError otTcpListen(otTcpListener *aListener, const otSockAddr *aSockName)
{
    return AsCoreType(aListener).Listen(AsCoreType(aSockName));
}

otError otTcpStopListening(otTcpListener *aListener)
{
    return AsCoreType(aListener).StopListening();
}

otError otTcpListenerDeinitialize(otTcpListener *aListener)
{
    return AsCoreType(aListener).Deinitialize();
}

#endif // OPENTHREAD_CONFIG_TCP_ENABLE
