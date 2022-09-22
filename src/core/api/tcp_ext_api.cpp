/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file implements extensions to the OpenThread TCP API.
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_TCP_ENABLE

#include <openthread/tcp_ext.h>

#include "common/as_core_type.hpp"
#include "common/locator_getters.hpp"
#include "net/tcp6_ext.hpp"

using namespace ot;

void otTcpCircularSendBufferInitialize(otTcpCircularSendBuffer *aSendBuffer, void *aDataBuffer, size_t aCapacity)
{
    AsCoreType(aSendBuffer).Initialize(aDataBuffer, aCapacity);
}

otError otTcpCircularSendBufferWrite(otTcpEndpoint *          aEndpoint,
                                     otTcpCircularSendBuffer *aSendBuffer,
                                     const void *             aData,
                                     size_t                   aLength,
                                     size_t *                 aWritten,
                                     uint32_t                 aFlags)
{
    AssertPointerIsNotNull(aWritten);
    return AsCoreType(aSendBuffer).Write(AsCoreType(aEndpoint), aData, aLength, *aWritten, aFlags);
}

void otTcpCircularSendBufferHandleForwardProgress(otTcpCircularSendBuffer *aSendBuffer, size_t aInSendBuffer)
{
    AsCoreType(aSendBuffer).HandleForwardProgress(aInSendBuffer);
}

size_t otTcpCircularSendBufferGetFreeSpace(const otTcpCircularSendBuffer *aSendBuffer)
{
    return AsCoreType(aSendBuffer).GetFreeSpace();
}

void otTcpCircularSendBufferForceDiscardAll(otTcpCircularSendBuffer *aSendBuffer)
{
    AsCoreType(aSendBuffer).ForceDiscardAll();
}

otError otTcpCircularSendBufferDeinitialize(otTcpCircularSendBuffer *aSendBuffer)
{
    return AsCoreType(aSendBuffer).Deinitialize();
}

#endif // OPENTHREAD_CONFIG_TCP_ENABLE
