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
 *   This file implements TCP/IPv6 sockets.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_TCP_ENABLE

#include "tcp6.hpp"

#include "common/code_utils.hpp"
#include "common/error.hpp"

namespace ot {
namespace Ip6 {

Tcp::Tcp(Instance &aInstance)
    : InstanceLocator(aInstance)
{
}

Error Tcp::Endpoint::Initialize(Instance &aInstance, otTcpEndpointInitializeArgs &aArgs)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aArgs);

    return kErrorNotImplemented;
}

Instance &Tcp::Endpoint::GetInstance(void)
{
    return *reinterpret_cast<Instance *>(this->mInstance);
}

void *Tcp::Endpoint::GetContext(void)
{
    return this->mContext;
}

const SockAddr &Tcp::Endpoint::GetLocalAddress(void) const
{
    static otSockAddr temp;
    return *static_cast<SockAddr *>(&temp);
}

const SockAddr &Tcp::Endpoint::GetPeerAddress(void) const
{
    static otSockAddr temp;
    return *static_cast<SockAddr *>(&temp);
}

Error Tcp::Endpoint::Bind(const SockAddr &aSockName)
{
    OT_UNUSED_VARIABLE(aSockName);

    return kErrorNotImplemented;
}

Error Tcp::Endpoint::Connect(const SockAddr &aSockName, uint32_t aFlags)
{
    OT_UNUSED_VARIABLE(aSockName);
    OT_UNUSED_VARIABLE(aFlags);

    return kErrorNotImplemented;
}

Error Tcp::Endpoint::SendByReference(otLinkedBuffer &aBuffer, uint32_t aFlags)
{
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aFlags);

    return kErrorNotImplemented;
}

Error Tcp::Endpoint::SendByExtension(size_t aNumBytes, uint32_t aFlags)
{
    OT_UNUSED_VARIABLE(aNumBytes);
    OT_UNUSED_VARIABLE(aFlags);

    return kErrorNotImplemented;
}

Error Tcp::Endpoint::ReceiveByReference(const otLinkedBuffer *&aBuffer) const
{
    OT_UNUSED_VARIABLE(aBuffer);

    return kErrorNotImplemented;
}

Error Tcp::Endpoint::ReceiveContiguify(void)
{
    return kErrorNotImplemented;
}

Error Tcp::Endpoint::CommitReceive(size_t aNumBytes, uint32_t aFlags)
{
    OT_UNUSED_VARIABLE(aNumBytes);
    OT_UNUSED_VARIABLE(aFlags);

    return kErrorNotImplemented;
}

Error Tcp::Endpoint::SendEndOfStream(void)
{
    return kErrorNotImplemented;
}

Error Tcp::Endpoint::Abort(void)
{
    return kErrorNotImplemented;
}

Error Tcp::Endpoint::Deinitialize(void)
{
    return kErrorNotImplemented;
}

Error Tcp::Listener::Initialize(Instance &aInstance, otTcpListenerInitializeArgs &aArgs)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aArgs);

    return kErrorNotImplemented;
}

Instance &Tcp::Listener::GetInstance(void)
{
    return *reinterpret_cast<Instance *>(this->mInstance);
}

void *Tcp::Listener::GetContext(void)
{
    return this->mContext;
}

Error Tcp::Listener::Listen(const SockAddr &aSockName)
{
    OT_UNUSED_VARIABLE(aSockName);

    return kErrorNotImplemented;
}

Error Tcp::Listener::StopListening(void)
{
    return kErrorNotImplemented;
}

Error Tcp::Listener::Deinitialize(void)
{
    return kErrorNotImplemented;
}

Error Tcp::ProcessReceivedSegment(Message &aMessage, MessageInfo &aMessageInfo)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    return kErrorNotImplemented;
}

} // namespace Ip6
} // namespace ot

#endif // OPENTHREAD_CONFIG_TCP_ENABLE
