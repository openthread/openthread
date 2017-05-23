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
 *  This file defines the top-level icmp6 functions for the OpenThread library.
 */

#ifndef OPENTHREAD_ICMP6_H_
#define OPENTHREAD_ICMP6_H_

#include <openthread/message.h>
#include <openthread/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-icmp6
 *
 * @brief
 *   This module includes functions that control ICMPv6 communication.
 *
 * @{
 *
 */

/**
 * This function indicates whether or not ICMPv6 Echo processing is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   ICMPv6 Echo processing is enabled.
 * @retval FALSE  ICMPv6 Echo processing is disabled.
 *
 */
bool otIcmp6IsEchoEnabled(otInstance *aInstance);

/**
 * This function sets whether or not ICMPv6 Echo processing is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aEnabled  TRUE to enable ICMPv6 Echo processing, FALSE otherwise.
 *
 */
void otIcmp6SetEchoEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function registers a handler to provide received ICMPv6 messages.
 *
 * @note A handler structure @p aHandler has to be stored in persistant (static) memory.
 *       OpenThread does not make a copy of handler structure.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aHandler  A pointer to a handler conitaining callback that is called when
 *                       an ICMPv6 message is received.
 *
 */
otError otIcmp6RegisterHandler(otInstance *aInstance, otIcmp6Handler *aHandler);

/**
 * This function sends an ICMPv6 Echo Request via the Thread interface.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aMessage      A pointer to the message buffer containing the ICMPv6 payload.
 * @param[in]  aMessageInfo  A reference to message information associated with @p aMessage.
 * @param[in]  aIdentifier   An identifier to aid in matching Echo Replies to this Echo Request.
 *                           May be zero.
 *
 */
otError otIcmp6SendEchoRequest(otInstance *aInstance, otMessage *aMessage,
                               const otMessageInfo *aMessageInfo, uint16_t aIdentifier);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_ICMP6_H_
