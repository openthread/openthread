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
 *  This file defines the top-level ip6 functions for the OpenThread library.
 */

#ifndef OPENTHREAD_IP6_H_
#define OPENTHREAD_IP6_H_

#include <openthread-types.h>
#include <platform/radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup api  API
 * @brief
 *   This module includes the application programming interface to the OpenThread stack.
 *
 * @{
 *
 * @defgroup execution Execution
 * @defgroup commands Commands
 * @defgroup config Configuration
 * @defgroup diags Diagnostics
 * @defgroup messages Message Buffers
 * @defgroup ip6 IPv6
 * @defgroup udp UDP
 * @defgroup coap CoAP
 *
 * @}
 *
 */

/**
 * @addtogroup diags  Diagnostics
 *
 * @brief
 *   This module includes functions that expose internal state.
 *
 * @{
 *
 */

/**
 * This function pointer is called when an IEEE 802.15.4 frame is received.
 *
 * @note This callback is called after FCS processing and @p aFrame may not contain the actual FCS that was received.
 *
 * @note This callback is called before IEEE 802.15.4 security processing and mSecurityValid in @p aFrame will
 * always be false.
 *
 * @param[in]  aFrame    A pointer to the received IEEE 802.15.4 frame.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otLinkPcapCallback)(const RadioPacket *aFrame, void *aContext);

/**
 * This function registers a callback to provide received raw IEEE 802.15.4 frames.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aPcapCallback     A pointer to a function that is called when receiving an IEEE 802.15.4 link frame or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 */
void otSetLinkPcapCallback(otInstance *aInstance, otLinkPcapCallback aPcapCallback, void *aCallbackContext);

/**
 * This function indicates whether or not promiscuous mode is enabled at the link layer.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval true   Promiscuous mode is enabled.
 * @retval false  Promiscuous mode is not enabled.
 *
 */
bool otIsLinkPromiscuous(otInstance *aInstance);

/**
 * This function enables or disables the link layer promiscuous mode.
 *
 * @note Promiscuous mode may only be enabled when the Thread interface is disabled.
 *
 * @param[in]  aInstance     A pointer to an OpenThread instance.
 * @param[in]  aPromiscuous  true to enable promiscuous mode, or false otherwise.
 *
 * @retval kThreadError_None          Successfully enabled promiscuous mode.
 * @retval kThreadError_InvalidState  Could not enable promiscuous mode because
 *                                    the Thread interface is enabled.
 *
 */
ThreadError otSetLinkPromiscuous(otInstance *aInstance, bool aPromiscuous);

/**
 * @}
 *
 */

/**
 * @addtogroup messages  Message Buffers
 *
 * @brief
 *   This module includes functions that manipulate OpenThread message buffers
 *
 * @{
 *
 */

/**
 * Free an allocated message buffer.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @retval kThreadErrorNone  Successfully freed the message buffer.
 *
 * @sa otNewUdpMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otFreeMessage(otMessage aMessage);

/**
 * Get the message length in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message length in bytes.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 * @sa otSetMessageLength
 */
uint16_t otGetMessageLength(otMessage aMessage);

/**
 * Set the message length in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aLength   A length in bytes.
 *
 * @retval kThreadErrorNone    Successfully set the message length.
 * @retval kThreadErrorNoBufs  No available buffers to grow the message.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otSetMessageLength(otMessage aMessage, uint16_t aLength);

/**
 * Get the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 *
 * @returns The message offset value.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
uint16_t otGetMessageOffset(otMessage aMessage);

/**
 * Set the message offset in bytes.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 *
 * @retval kThreadErrorNone        Successfully set the message offset.
 * @retval kThreadErrorInvalidArg  The offset is beyond the message length.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otSetMessageOffset(otMessage aMessage, uint16_t aOffset);

/**
 * Append bytes to a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aBuf      A pointer to the data to append.
 * @param[in]  aLength   Number of bytes to append.
 *
 * @retval kThreadErrorNone    Successfully appended to the message
 * @retval kThreadErrorNoBufs  No available buffers to grow the message.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 * @sa otWriteMessage
 */
ThreadError otAppendMessage(otMessage aMessage, const void *aBuf, uint16_t aLength);

/**
 * Read bytes from a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 * @param[in]  aBuf      A pointer to a buffer that message bytes are read to.
 * @param[in]  aLength   Number of bytes to read.
 *
 * @returns The number of bytes read.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otWriteMessage
 */
int otReadMessage(otMessage aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength);

/**
 * Write bytes to a message.
 *
 * @param[in]  aMessage  A pointer to a message buffer.
 * @param[in]  aOffset   An offset in bytes.
 * @param[in]  aBuf      A pointer to a buffer that message bytes are written from.
 * @param[in]  aLength   Number of bytes to write.
 *
 * @returns The number of bytes written.
 *
 * @sa otNewUdpMessage
 * @sa otFreeMessage
 * @sa otAppendMessage
 * @sa otGetMessageLength
 * @sa otSetMessageLength
 * @sa otGetMessageOffset
 * @sa otSetMessageOffset
 * @sa otReadMessage
 */
int otWriteMessage(otMessage aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength);

/**
 * @}
 *
 */

/**
 * @addtogroup ip6  IPv6
 *
 * @brief
 *   This module includes functions that control IPv6 communication.
 *
 * @{
 *
 */

/**
 * Allocate a new message buffer for sending an IPv6 message.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aLinkSecurityEnabled  TRUE if the message should be secured at Layer 2
 *
 * @returns A pointer to the message buffer or NULL if no message buffers are available.
 *
 * @sa otFreeMessage
 */
otMessage otNewIp6Message(otInstance *aInstance, bool aLinkSecurityEnabled);

/**
 * This function pointer is called when an IPv6 datagram is received.
 *
 * @param[in]  aMessage  A pointer to the message buffer containing the received IPv6 datagram.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*otReceiveIp6DatagramCallback)(otMessage aMessage, void *aContext);

/**
 * This function registers a callback to provide received IPv6 datagrams.
 *
 * By default, this callback does not pass Thread control traffic.  See otSetReceiveIp6DatagramFilterEnabled() to
 * change the Thread control traffic filter setting.
 *
 * @param[in]  aInstance         A pointer to an OpenThread instance.
 * @param[in]  aCallback         A pointer to a function that is called when an IPv6 datagram is received or
 *                               NULL to disable the callback.
 * @param[in]  aCallbackContext  A pointer to application-specific context.
 *
 * @sa otIsReceiveIp6DatagramFilterEnabled
 * @sa otSetReceiveIp6DatagramFilterEnabled
 *
 */
void otSetReceiveIp6DatagramCallback(otInstance *aInstance, otReceiveIp6DatagramCallback aCallback,
                                     void *aCallbackContext);

/**
 * This function indicates whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
 * via the callback specified in otSetReceiveIp6DatagramCallback().
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @returns  TRUE if Thread control traffic is filtered out, FALSE otherwise.
 *
 * @sa otSetReceiveIp6DatagramCallback
 * @sa otSetReceiveIp6DatagramFilterEnabled
 *
 */
bool otIsReceiveIp6DatagramFilterEnabled(otInstance *aInstance);

/**
 * This function sets whether or not Thread control traffic is filtered out when delivering IPv6 datagrams
 * via the callback specified in otSetReceiveIp6DatagramCallback().
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aEnabled  TRUE if Thread control traffic is filtered out, FALSE otherwise.
 *
 * @sa otSetReceiveIp6DatagramCallback
 * @sa otIsReceiveIp6DatagramFilterEnabled
 *
 */
void otSetReceiveIp6DatagramFilterEnabled(otInstance *aInstance, bool aEnabled);

/**
 * This function sends an IPv6 datagram via the Thread interface.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aMessage  A pointer to the message buffer containing the IPv6 datagram.
 *
 */
ThreadError otSendIp6Datagram(otInstance *aInstance, otMessage aMessage);

/**
 * This function indicates whether or not ICMPv6 Echo processing is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 *
 * @retval TRUE   ICMPv6 Echo processing is enabled.
 * @retval FALSE  ICMPv6 Echo processing is disabled.
 *
 */
bool otIsIcmpEchoEnabled(otInstance *aInstance);

/**
 * This function sets whether or not ICMPv6 Echo processing is enabled.
 *
 * @param[in]  aInstance A pointer to an OpenThread instance.
 * @param[in]  aEnabled  TRUE to enable ICMPv6 Echo processing, FALSE otherwise.
 *
 */
void otSetIcmpEchoEnabled(otInstance *aInstance, bool aEnabled);

/**
 * @}
 *
 */

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
 * Allocate a new message buffer for sending a UDP message.
 *
 * @param[in]  aInstance             A pointer to an OpenThread instance.
 * @param[in]  aLinkSecurityEnabled  TRUE if the message should be secured at Layer 2.
 *
 * @returns A pointer to the message buffer or NULL if no message buffers are available.
 *
 * @sa otFreeMessage
 */
otMessage otNewUdpMessage(otInstance *aInstance, bool aLinkSecurityEnabled);

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
 * @sa otNewUdpMessage
 * @sa otCloseUdpSocket
 * @sa otBindUdpSocket
 * @sa otSendUdp
 */
ThreadError otOpenUdpSocket(otInstance *aInstance, otUdpSocket *aSocket, otUdpReceive aCallback, void *aContext);

/**
 * Close a UDP/IPv6 socket.
 *
 * @param[in]  aSocket  A pointer to a UDP socket structure.
 *
 * @retval kThreadErrorNone  Successfully closed the socket.
 *
 * @sa otNewUdpMessage
 * @sa otOpenUdpSocket
 * @sa otBindUdpSocket
 * @sa otSendUdp
 */
ThreadError otCloseUdpSocket(otUdpSocket *aSocket);

/**
 * Bind a UDP/IPv6 socket.
 *
 * @param[in]  aSocket    A pointer to a UDP socket structure.
 * @param[in]  aSockName  A pointer to an IPv6 socket address structure.
 *
 * @retval kThreadErrorNone  Bind operation was successful.
 *
 * @sa otNewUdpMessage
 * @sa otOpenUdpSocket
 * @sa otCloseUdpSocket
 * @sa otSendUdp
 */
ThreadError otBindUdpSocket(otUdpSocket *aSocket, otSockAddr *aSockName);

/**
 * Send a UDP/IPv6 message.
 *
 * @param[in]  aSocket       A pointer to a UDP socket structure.
 * @param[in]  aMessage      A pointer to a message buffer.
 * @param[in]  aMessageInfo  A pointer to a message info structure.
 *
 * @sa otNewUdpMessage
 * @sa otOpenUdpSocket
 * @sa otCloseUdpSocket
 * @sa otBindUdpSocket
 * @sa otSendUdp
 */
ThreadError otSendUdp(otUdpSocket *aSocket, otMessage aMessage, const otMessageInfo *aMessageInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_IP6_H_
