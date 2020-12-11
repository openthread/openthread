/*
 *  Copyright (c) 2020, The OpenThread Authors.
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

#if (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_TCP_ENABLE

#include <openthread/tcp.h>

#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/new.hpp"
#include "net/tcp6.hpp"

using namespace ot;

void otTcpInitialize(otInstance *aInstance, otTcpSocket *aSocket, otTcpEventHandler aEventHandler, void *aContext)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Ip6::Tcp>().Initialize(*reinterpret_cast<Ip6::Tcp::Socket *>(aSocket), aEventHandler, aContext);
}

otError otTcpListen(otTcpSocket *aSocket)
{
    return reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->Listen();
}

void otTcpClose(otTcpSocket *aSocket)
{
    reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->Close();
}

void otTcpAbort(otTcpSocket *aSocket)
{
    reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->Abort();
}

otError otTcpBind(otTcpSocket *aSocket, const otSockAddr *aSockName)
{
    return reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->Bind(*static_cast<const Ip6::SockAddr *>(aSockName));
}

otError otTcpConnect(otTcpSocket *aSocket, const otSockAddr *aSockName)
{
    return reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->Connect(*static_cast<const Ip6::SockAddr *>(aSockName));
}

uint16_t otTcpWrite(otTcpSocket *aSocket, uint8_t *aData, uint16_t aLength)
{
    return reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->Write(aData, aLength);
}

void *otTcpGetContext(otTcpSocket *aSocket)
{
    return reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->GetContext();
}

otTcpState otTcpGetState(otTcpSocket *aSocket)
{
    return reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->GetState();
}

uint16_t otTcpRead(otTcpSocket *aSocket, uint8_t *aBuf, uint16_t aSize)
{
    return reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->Read(aBuf, aSize);
}

const char *otTcpStateToString(otTcpState aState)
{
    return Ip6::Tcp::StateToString(aState);
}

const otSockAddr *otTcpGetPeerName(otTcpSocket *aSocket)
{
    return &(reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->GetPeerName());
}

const otSockAddr *otTcpGetSockName(otTcpSocket *aSocket)
{
    return &(reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->GetSockName());
}

otError otTcpConfigRoundTripTime(otTcpSocket *aSocket, uint32_t aMinRTT, uint32_t aMaxRTT)
{
    return reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->ConfigRoundTripTime(aMinRTT, aMaxRTT);
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void otTcpResetNextSegment(otTcpSocket *aSocket)
{
    reinterpret_cast<Ip6::Tcp::Socket *>(aSocket)->ResetNextSegment();
}

void otTcpSetSegmentRandomDropProb(otInstance *aInstance, uint8_t aProb)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    instance.Get<Ip6::Tcp>().SetSegmentRandomDropProb(aProb);
}

void otTcpGetCounters(otInstance *aInstance, otTcpCounters *aCounters)
{
    Instance &instance = *static_cast<Instance *>(aInstance);

    OT_ASSERT(aCounters != nullptr);

    *aCounters = instance.Get<Ip6::Tcp>().mCounters;
}

#endif

#endif // (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_TCP_ENABLE
