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
 *   This file implements the OpenThread Border Agent Proxy API.
 */

#include "openthread-instance.h"

using namespace Thread;

#ifdef __cplusplus
extern "C" {
#endif

ThreadError otBorderAgentProxyStart(otInstance *aInstance, otBorderAgentProxyStreamHandler aBorderAgentProxyCallback,
                                    void *aContext)
{
    return aInstance->mThreadNetif.GetBorderAgentProxy().Start(aBorderAgentProxyCallback, aContext);
}

ThreadError otBorderAgentProxyStop(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetBorderAgentProxy().Stop();
}

ThreadError otBorderAgentProxySend(otInstance *aInstance, otMessage *aMessage, uint16_t aRloc, uint16_t aPort)
{
    return aInstance->mThreadNetif.GetBorderAgentProxy().Send(*static_cast<Message *>(aMessage), aRloc, aPort);
}

bool otBorderAgentProxyIsEnabled(otInstance *aInstance)
{
    return aInstance->mThreadNetif.GetBorderAgentProxy().IsEnabled();
}

#ifdef __cplusplus
}  // extern "C"
#endif
