/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes the OpenThread API for Border Agent Proxy feature.
 */

#ifndef OPENTHREAD_BORDER_AGENT_PROXY_H_
#define OPENTHREAD_BORDER_AGENT_PROXY_H_

#ifdef OPENTHREAD_CONFIG_FILE
#include OPENTHREAD_CONFIG_FILE
#else
#include <openthread-config.h>
#endif

#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-border-agent
 *
 * @brief
 *   This module includes functions for signal border agent proxy feature.
 *
 * @{
 *
 */

/**
 * This function pointer is called when a CoAP packet for border agent is received.
 *
 * @param[in]  aMessage  A pointer to the CoAP Message.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otBorderAgentProxyStreamHandler)(otMessage *aMessage, uint16_t aLocator, uint16_t aPort, void *aContext);

/**
 * Start the border agent proxy.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aHandler             A pointer to a function called to deliver packet to border agent.
 * @param[in]  aContext             A pointer to application-specific context.
 *
 * @retval kThreadError_None        Successfully started the border agent proxy.
 * @retval kThreadError_Already     Border agent proxy has been started before.
 *
 */
ThreadError otBorderAgentProxyStart(otInstance *aInstance, otBorderAgentProxyStreamHandler aHandler, void *aContext);

/**
 * Stop the border agent proxy.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 *
 * @retval kThreadError_None        Successfully stopped the border agent proxy.
 * @retval kThreadError_Already     Border agent proxy is already stopped.
 *
 */
ThreadError otBorderAgentProxyStop(otInstance *aInstance);

/**
 * Send packet through border agent proxy.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aMessage             A pointer to the CoAP Message.
 * @param[in]  aLocator             Rloc of destination.
 * @param[in]  aPort                Port of destination.
 *
 * @retval kThreadError_None            Successfully send the message.
 * @retval kThreadError_InvalidState    Border agent proxy is not started.
 *
 * @warning No matter the call success or fail, the message is freed.
 *
 */
ThreadError otBorderAgentProxySend(otInstance *aInstance, otMessage *aMessage,
                                   uint16_t aLocator, uint16_t aPort);

/**
 * Get the border agent proxy status (enabled/disabled)
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 *
 * @returns The border agent proxy status (true if enabled, false otherwise).
 */
bool otBorderAgentProxyIsEnabled(otInstance *aInstance);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_BORDER_AGENT_PROXY_H_
