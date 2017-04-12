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
 * @brief
 *   This file includes the OenThread API for jam detection feature.
 */

#ifndef OPENTHREAD_BORDER_AGENT_PROXY_H_
#define OPENTHREAD_BORDER_AGENT_PROXY_H_

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include "openthread/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPENTHREAD_ENABLE_BORDER_AGENT_PROXY 1
#if OPENTHREAD_ENABLE_BORDER_AGENT_PROXY

/**
 * @addtogroup jam-det  Jamming Detection
 *
 * @brief
 *   This module includes functions for signal jamming detection feature.
 *
 * @{
 *
 */

/**
 * This function pointer is called if signal jam detection is enabled and a jam is detected.
 *
 * @param[in]  aJamState Current jam state (`true` if jam is detected, `false` otherwise).
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otBorderAgentProxyCallback)(otMessage *aMessage, void *aContext);

/**
 * Start the jamming detection.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aCallback            A pointer to a function called to notify of jamming state change.
 * @param[in]  aContext             A pointer to application-specific context.
 *
 * @retval kThreadErrorNone         Successfully started the jamming detection.
 * @retval kThreadErrorAlready      Jam detection has been started before.
 *
 */
ThreadError otBorderAgentProxyStart(otInstance *aInstance, otBorderAgentProxyCallback aBorderAgentProxyCallback, void* aContext);

/**
 * Stop the jamming detection.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 *
 * @retval kThreadErrorNone         Successfully stopped the jamming detection.
 * @retval kThreadErrorAlready      Jam detection is already stopped.
 *
 */
ThreadError otBorderAgentProxyStop(otInstance *aInstance);

ThreadError otBorderAgentProxySend(otInstance *aInstance, otMessage *aMessage);

bool otBorderAgentProxyIsEnabled(otInstance *aInstance);

/**
 * @}
 *
 */

#endif  // OPENTHREAD_ENABLE_BORDER_AGENT_PROXY

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_BORDER_AGENT_PROXY_H_
