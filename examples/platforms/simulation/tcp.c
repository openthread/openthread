/*
 *  Copyright (c) 2026, The OpenThread Authors.
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

#include "platform-simulation.h"

#include <openthread/platform/tcp.h>

#if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE

otError otPlatTcpEnableListener(otPlatTcpListener *aListener, const otPlatTcpSockAddr *aLocalSockAddr)
{
    OT_UNUSED_VARIABLE(aListener);
    OT_UNUSED_VARIABLE(aLocalSockAddr);

    return OT_ERROR_FAILED;
}

void otPlatTcpDisableListener(otPlatTcpListener *aListener) { OT_UNUSED_VARIABLE(aListener); }

otError otPlatTcpConnect(otPlatTcpConnection     *aConn,
                         const otPlatTcpSockAddr *aPeerSockAddr,
                         const otPlatTcpSockAddr *aLocalSockAddr)
{
    OT_UNUSED_VARIABLE(aConn);
    OT_UNUSED_VARIABLE(aPeerSockAddr);
    OT_UNUSED_VARIABLE(aLocalSockAddr);

    return OT_ERROR_FAILED;
}

void otPlatTcpNotifyTxPending(otPlatTcpConnection *aConn) { OT_UNUSED_VARIABLE(aConn); }

uint16_t otPlatTcpSend(otPlatTcpConnection *aConn, const uint8_t *aBuffer, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aConn);
    OT_UNUSED_VARIABLE(aBuffer);
    OT_UNUSED_VARIABLE(aLength);

    return 0;
}

void otPlatTcpClose(otPlatTcpConnection *aConn) { OT_UNUSED_VARIABLE(aConn); }

void otPlatTcpAbort(otPlatTcpConnection *aConn) { OT_UNUSED_VARIABLE(aConn); }

#endif // #if OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE
