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
 *  This file defines the OpenThread UDP API.
 */

#ifndef OPENTHREAD_UDP_H_
#define OPENTHREAD_UDP_H_

#include "openthread/types.h"
#include "openthread/message.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup udp  UDP
 *
 * @brief
 *   This module includes functions that control UDP communication.
 *
 * @{
 *
 */

/**
 * This callback allows OpenThread to inform the application of a received UDP message.
 *
 */
typedef void (*otUdpReceive)(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

/**
 * This structure represents a UDP socket.
 *
 */
typedef struct otUdpSocket
{
    otSockAddr           mSockName;  ///< The local IPv6 socket address.
    otSockAddr           mPeerName;  ///< The peer IPv6 socket address.
    otUdpReceive         mHandler;   ///< A function pointer to the application callback.
    void                *mContext;   ///< A pointer to application-specific context.
    void                *mTransport; ///< A pointer to the transport object (internal use only).
    struct otUdpSocket  *mNext;      ///< A pointer to the next UDP socket (internal use only).
} otUdpSocket;

/**
 * Allocate a new message buffer for sending a UDP message.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aLinkSecurityEnabled  TRUE if the message should be secured at Layer 2.
 *
 * @returns A pointer to the message buffer or NULL if no message buffers are available.
 *
 * @sa otFreeMessage
 *
 */
otMessage *otUdpNewMessage(otInstance *aInstance, bool aLinkSecurityEnabled);

/**
 * Open a UDP/IPv6 socket.
 *
 * @param[in]  aInstance  A pointer to an OpenThread instance.
 * @param[in]  aSocket    A pointer to a UDP socket structure.
 * @param[in]  aCallback  A pointer to the application callback function.
 * @param[in]  aContext   A pointer to application-specific context.
 *
 * @retval kThreadErrorNone         Successfully opened the socket.
 * @retval kThreadErrorInvalidArgs  Given socket structure was already opened.
 *
 * @sa otUdpNewMessage
 * @sa otUdpClose
 * @sa otUdpBind
 * @sa otUdpSend
 *
 */
ThreadError otUdpOpen(otInstance *aInstance, otUdpSocket *aSocket, otUdpReceive aCallback, void *aContext);

/**
 * Close a UDP/IPv6 socket.
 *
 * @param[in]  aSocket  A pointer to a UDP socket structure.
 *
 * @retval kThreadErrorNone  Successfully closed the socket.
 *
 * @sa otUdpNewMessage
 * @sa otUdpOpen
 * @sa otUdpBind
 * @sa otUdpSend
 *
 */
ThreadError otUdpClose(otUdpSocket *aSocket);

/**
 * Bind a UDP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a UDP socket structure.
 * @param[in]  aSockName  A pointer to an IPv6 socket address structure.
 *
 * @retval kThreadErrorNone  Bind operation was successful.
 *
 * @sa otUdpNewMessage
 * @sa otUdpOpen
 * @sa otUdpClose
 * @sa otUdpSend
 *
 */
ThreadError otUdpBind(otUdpSocket *aSocket, otSockAddr *aSockName);

/**
 * Send a UDP/IPv6 message.
 *
 * @param[in]  aSocket       A pointer to a UDP socket structure.
 * @param[in]  aMessage      A pointer to a message buffer.
 * @param[in]  aMessageInfo  A pointer to a message info structure.
 *
 * @sa otUdpNewMessage
 * @sa otUdpOpen
 * @sa otUdpClose
 * @sa otUdpBind
 * @sa otUdpSend
 *
 */
ThreadError otUdpSend(otUdpSocket *aSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_UDP_H_
