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

#include <openthread-message.h>
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

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // OPENTHREAD_IP6_H_
