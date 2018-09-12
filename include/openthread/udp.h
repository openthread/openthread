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
 *
 */

#ifndef OPENTHREAD_UDP_H_
#define OPENTHREAD_UDP_H_

#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-udp
 *
 * @brief
 *   This module includes functions that control UDP communication.
 *
 * @{
 *
 */

/**
 * This callback allows OpenThread to provide specific handlers for certain UDP messages.
 *
 * @retval  true    The message is handled by this receiver and should not be further processed.
 * @retval  false   The message is not handled by this receiver.
 *
 */
typedef bool (*otUdpHandler)(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo);

/**
 * This structure represents a UDP receiver.
 *
 */
typedef struct otUdpReceiver
{
    struct otUdpReceiver *mNext;    ///< A pointer to the next UDP receiver (internal use only).
    otUdpHandler          mHandler; ///< A function pointer to the receiver callback.
    void *                mContext; ///< A pointer to application-specific context.
} otUdpReceiver;

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
    otSockAddr          mSockName; ///< The local IPv6 socket address.
    otSockAddr          mPeerName; ///< The peer IPv6 socket address.
    otUdpReceive        mHandler;  ///< A function pointer to the application callback.
    void *              mContext;  ///< A pointer to application-specific context.
    struct otUdpSocket *mNext;     ///< A pointer to the next UDP socket (internal use only).
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
 * @retval OT_ERROR_NONE         Successfully opened the socket.
 * @retval OT_ERROR_INVALID_ARGS  Given socket structure was already opened.
 *
 * @sa otUdpNewMessage
 * @sa otUdpClose
 * @sa otUdpBind
 * @sa otUdpConnect
 * @sa otUdpSend
 *
 */
otError otUdpOpen(otInstance *aInstance, otUdpSocket *aSocket, otUdpReceive aCallback, void *aContext);

/**
 * Close a UDP/IPv6 socket.
 *
 * @param[in]  aSocket  A pointer to a UDP socket structure.
 *
 * @retval OT_ERROR_NONE  Successfully closed the socket.
 *
 * @sa otUdpNewMessage
 * @sa otUdpOpen
 * @sa otUdpBind
 * @sa otUdpConnect
 * @sa otUdpSend
 *
 */
otError otUdpClose(otUdpSocket *aSocket);

/**
 * Bind a UDP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a UDP socket structure.
 * @param[in]  aSockName  A pointer to an IPv6 socket address structure.
 *
 * @retval OT_ERROR_NONE  Bind operation was successful.
 *
 * @sa otUdpNewMessage
 * @sa otUdpOpen
 * @sa otUdpConnect
 * @sa otUdpClose
 * @sa otUdpSend
 *
 */
otError otUdpBind(otUdpSocket *aSocket, otSockAddr *aSockName);

/**
 * Connect a UDP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a UDP socket structure.
 * @param[in]  aSockName  A pointer to an IPv6 socket address structure.
 *
 * @retval OT_ERROR_NONE  Connect operation was successful.
 *
 * @sa otUdpNewMessage
 * @sa otUdpOpen
 * @sa otUdpBind
 * @sa otUdpClose
 * @sa otUdpSend
 *
 */
otError otUdpConnect(otUdpSocket *aSocket, otSockAddr *aSockName);

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
 * @sa otUdpConnect
 * @sa otUdpSend
 *
 */
otError otUdpSend(otUdpSocket *aSocket, otMessage *aMessage, const otMessageInfo *aMessageInfo);

/**
 * @}
 *
 */

/**
 * @addtogroup api-udp-proxy
 *
 * @brief
 *   This module includes functions for UDP proxy feature.
 *
 *   The functions in this module are available when udp-proxy feature (`OPENTHREAD_ENABLE_UDP_PROXY`) is enabled.
 *
 * @{
 *
 */

/**
 * This function pointer delivers the UDP packet to host and host should send the packet through its own network stack.
 *
 * @param[in]  aMessage   A pointer to the UDP Message.
 * @param[in]  aPeerPort  The destination UDP port.
 * @param[in]  aPeerAddr  A pointer to the destination IPv6 address.
 * @param[in]  aSockPort  The source UDP port.
 * @param[in]  aContext   A pointer to application-specific context.
 *
 */
typedef void (*otUdpProxySender)(otMessage *   aMessage,
                                 uint16_t      aPeerPort,
                                 otIp6Address *aPeerAddr,
                                 uint16_t      aSockPort,
                                 void *        aContext);

/**
 * Set UDP proxy callback to deliever UDP packets to host.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aSender              A pointer to a function called to deliver UDP packet to host.
 * @param[in]  aContext             A pointer to application-specific context.
 *
 */
void otUdpProxySetSender(otInstance *aInstance, otUdpProxySender aSender, void *aContext);

/**
 * Handle a UDP packet received from host.
 *
 * @param[in]  aInstance            A pointer to an OpenThread instance.
 * @param[in]  aMessage             A pointer to the UDP Message.
 * @param[in]  aPeerPort            The source UDP port.
 * @param[in]  aPeerAddr            A pointer to the source address.
 * @param[in]  aSockPort            The destination UDP port.
 *
 * @warning No matter the call success or fail, the message is freed.
 *
 */
void otUdpProxyReceive(otInstance *        aInstance,
                       otMessage *         aMessage,
                       uint16_t            aPeerPort,
                       const otIp6Address *aPeerAddr,
                       uint16_t            aSockPort);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_UDP_H_
