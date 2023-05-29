/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes the platform abstraction for DNS Stateful Operations (DSO) transport.
 */

#ifndef OPENTHREAD_PLATFORM_DSO_TRANSPORT_H_
#define OPENTHREAD_PLATFORM_DSO_TRANSPORT_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/message.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a DSO connection.
 *
 * It is an opaque struct (the platform implementation only deals with pointers to this struct).
 *
 */
typedef struct otPlatDsoConnection otPlatDsoConnection;

/**
 * Can be used by DSO platform implementation to get the the OpenThread instance associated with a
 * connection instance.
 *
 * @param[in] aConnection   A pointer to the DSO connection.
 *
 * @returns A pointer to the `otInstance`.
 *
 */
extern otInstance *otPlatDsoGetInstance(otPlatDsoConnection *aConnection);

/**
 * Starts or stops listening for incoming connection requests on transport layer.
 *
 * For DNS-over-TLS, the transport layer MUST listen on port 853 and follow RFC 7858.
 *
 * While listening is enabled, if a connection request is received, the `otPlatDsoAccept()` callback MUST be called.
 *
 * @param[in] aInstance    The OpenThread instance.
 * @param[in] aEnable      TRUE to start listening, FALSE to stop listening.
 *
 */
void otPlatDsoEnableListening(otInstance *aInstance, bool aEnable);

/**
 * Is a callback from the DSO platform to indicate an incoming connection request when listening is
 * enabled.
 *
 * Determines whether or not to accept the connection request. It returns a non-null `otPlatDsoConnection`
 * pointer if the request is to be accepted, or `NULL` if the request is to be rejected.
 *
 * If a non-null connection pointer is returned, the platform layer MUST continue establishing the connection with the
 * peer. The platform reports the outcome by invoking `otPlatDsoHandleConnected()` callback on success or
 * `otPlatDsoHandleDisconnected()` callback on failure.
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aPeerSockAddr  The socket address (IPv6 address and port number) of the peer requesting connection.
 *
 * @returns A pointer to the `otPlatDsoConnection` to use if to accept, or `NULL` if to reject.
 *
 */
extern otPlatDsoConnection *otPlatDsoAccept(otInstance *aInstance, const otSockAddr *aPeerSockAddr);

/**
 * Requests the platform layer to initiate establishing a connection with a peer.
 *
 * The platform reports the outcome by invoking `otPlatDsoHandleConnected()` callback on success or
 * `otPlatDsoHandleDisconnected()` callback (on failure).
 *
 * @param[in] aConnection     The connection.
 * @param[in] aPeerSockAddr   The socket address (IPv6 address and port number) of the peer to connect to.
 *
 */
void otPlatDsoConnect(otPlatDsoConnection *aConnection, const otSockAddr *aPeerSockAddr);

/**
 * Is a callback from the platform layer to indicate that a connection is successfully established.
 *
 * It MUST be called either after accepting an incoming connection (`otPlatDsoAccept`) or after a `otPlatDsoConnect()`
 * call.
 *
 * Only after this callback, the connection can be used to send and receive messages.
 *
 * @param[in] aConnection     The connection.
 *
 */
extern void otPlatDsoHandleConnected(otPlatDsoConnection *aConnection);

/**
 * Sends a DSO message to the peer on a connection.
 *
 * Is used only after the connection is successfully established (after `otPlatDsoHandleConnected()`
 * callback).
 *
 * Passes the ownership of the @p aMessage to the DSO platform layer, and the platform implementation is
 * expected to free the message once it is no longer needed.
 *
 * The @p aMessage contains the DNS message (starting with DNS header). Note that it does not contain the the length
 * field that is needed when sending over TLS/TCP transport. The platform layer MUST therefore include the length
 * field when passing the message to TLS/TCP layer.
 *
 * @param[in] aConnection   The connection to send on.
 * @param[in] aMessage      The message to send.
 *
 */
void otPlatDsoSend(otPlatDsoConnection *aConnection, otMessage *aMessage);

/**
 * Is a callback from the platform layer to indicate that a DNS message was received over a connection.
 *
 * The platform MUST call this function only after the connection is successfully established (after callback
 * `otPlatDsoHandleConnected()` is invoked).
 *
 * Passes the ownership of the @p aMessage from the DSO platform layer to OpenThread. OpenThread will
 * free the message when no longer needed.
 *
 * The @p aMessage MUST contain the DNS message (starting with DNS header) and not include the length field that may
 * be included in TCP/TLS exchange.
 *
 * @param[in] aConnection   The connection on which the message was received.
 * @param[in] aMessage      The received message.
 *
 */
extern void otPlatDsoHandleReceive(otPlatDsoConnection *aConnection, otMessage *aMessage);

/**
 * Defines disconnect modes.
 *
 */
typedef enum
{
    OT_PLAT_DSO_DISCONNECT_MODE_GRACEFULLY_CLOSE, ///< Gracefully close the connection.
    OT_PLAT_DSO_DISCONNECT_MODE_FORCIBLY_ABORT,   ///< Forcibly abort the connection.
} otPlatDsoDisconnectMode;

/**
 * Requests a connection to be disconnected.
 *
 * After calling this function, the DSO platform implementation MUST NOT maintain `aConnection` pointer (platform
 * MUST NOT call any callbacks using this `Connection` pointer anymore). In particular, calling `otPlatDsoDisconnect()`
 * MUST NOT trigger the callback `otPlatDsoHandleDisconnected()`.
 *
 * @param[in] aConnection   The connection to disconnect
 * @param[in] aMode         The disconnect mode (close gracefully or forcibly abort).
 *
 */
void otPlatDsoDisconnect(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode);

/**
 * Is a callback from the platform layer to indicate that peer closed/aborted the connection or the
 * connection establishment failed (e.g., peer rejected a connection request).
 *
 * After calling this function, the DSO platform implementation MUST NOT maintain `aConnection` pointer (platform
 * MUST NOT call any callbacks using this `Connection` pointer anymore).
 *
 * @param[in] aConnection   The connection which disconnected.
 * @param[in] aMode         The disconnect mode (closed gracefully or forcibly aborted).
 *
 */
extern void otPlatDsoHandleDisconnected(otPlatDsoConnection *aConnection, otPlatDsoDisconnectMode aMode);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_DSO_TRANSPORT_H_
