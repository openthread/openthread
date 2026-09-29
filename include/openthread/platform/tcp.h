/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes the abstraction for the platform TCP
 */

#ifndef OPENTHREAD_PLATFORM_TCP_H_
#define OPENTHREAD_PLATFORM_TCP_H_

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-tcp
 *
 * @brief
 *   This module includes the platform abstraction for TCP connections and listeners.
 *
 *   All APIs in this module are applicable only when `OPENTHREAD_CONFIG_PLATFORM_TCP_ENABLE` feature is enabled.
 *
 * @{
 */

/**
 * Represents platform-specific data associated with a connection or a listener.
 *
 * This union is provided to add flexibility for the platform. A platform can choose to store a file descriptor
 * (e.g., an `int` for a POSIX socket) or a pointer to an arbitrary context or state structure needed by the
 * platform implementation.
 *
 * The OpenThread stack guarantees that the `otPlatTcpPlatformData` is fully cleared (all bytes set to zero) when a
 * new listener or connection instance is initialized.
 *
 * For an `otPlatTcpListener`, the `otPlatTcpEnableListener()` call provides an opportunity for the platform to allocate
 * or update this information. The OpenThread stack guarantees that `otPlatTcpDisableListener()` will be invoked on any
 * previously enabled listener, providing a deterministic point for the platform implementation to perform cleanup
 * (e.g., deallocating memory or context structures).
 *
 * For an `otPlatTcpConnection`, the `otPlatTcpConnect()` call or the `otPlatTcpAccept()` callback indicate when a new
 * connection instance is provided, allowing the platform data to be initialized. The platform is responsible for
 * cleaning up this data either before invoking `otPlatTcpHandleDisconnected()` (which invalidates the connection) or
 * from an `otPlatTcpAbort()` call. The OpenThread stack guarantees that it will eventually disconnect or abort any
 * active connection, ensuring a reliable cleanup path.
 */
typedef union
{
    int   mDescriptor; ///< A value (like a file descriptor).
    void *mContext;    ///< Pointer to arbitrary platform data.
} otPlatTcpPlatformData;

/**
 * Represents a TCP listener.
 *
 * The OpenThread core owns and manages the `otPlatTcpListener` instances. The platform should track the pointers
 * to these instances and use them when invoking the callbacks. The `otPlatTcpListener *` can be viewed as
 * a "descriptor" or "handle" to the listener.
 */
typedef struct otPlatTcpListener
{
    otPlatTcpPlatformData mData; ///< Platform implementation specific data.
} otPlatTcpListener;

/**
 * Represents a TCP connection.
 *
 * The OpenThread core owns and manages the `otPlatTcpConnection` instances. The platform should track the pointers
 * to these instances and pass them when invoking the `otPlatTcpHandle*` callbacks. The `otPlatTcpConnection *` can
 * be viewed as a "descriptor" or "handle" to the connection.
 *
 * The `otPlatTcpConnection` instance remains valid as long as the connection is active.
 */
typedef struct otPlatTcpConnection
{
    otPlatTcpPlatformData mData; ///< Platform implementation specific data.
} otPlatTcpConnection;

/**
 * Represents a TCP socket address.
 */
typedef struct otPlatTcpSockAddr
{
    otSockAddr mSockAddr; ///< The socket address (IP address and port number). Use IPv4-mapped IPv6 for IPv4.
    uint32_t   mIfIndex;  ///< Interface index. Zero indicates any/unspecified.
} otPlatTcpSockAddr;

/**
 * Defines the reason for a TCP connection disconnection.
 */
typedef enum otPlatTcpDisconnectReason
{
    OT_PLAT_TCP_DISCONNECT_REASON_CLOSED,  ///< Connection was gracefully closed.
    OT_PLAT_TCP_DISCONNECT_REASON_TIMEOUT, ///< Connection timed out (e.g., failed to connect or keepalive failure).
    OT_PLAT_TCP_DISCONNECT_REASON_REFUSED, ///< Connection was refused by the peer (RST received during handshake).
    OT_PLAT_TCP_DISCONNECT_REASON_RESET,   ///< Connection was reset by the peer (RST received on established conn).
    OT_PLAT_TCP_DISCONNECT_REASON_ERROR,   ///< Connection was aborted due to other errors.
} otPlatTcpDisconnectReason;

/**
 * Enables a TCP listener.
 *
 * The platform should start listening for incoming TCP connections on the provided @p aLocalSockAddr. When an
 * incoming connection request is received, the platform must invoke the `otPlatTcpAccept()` callback to accept the
 * request.
 *
 * The @p aLocalSockAddr specifies the local interface, address, and port to bind to. Importantly, the port number
 * within @p aLocalSockAddr must not be zero. The IP address may be unspecified (all zeros) to indicate that the
 * listener should accept connections on any local address.
 *
 * @param[in] aListener      The TCP listener.
 * @param[in] aLocalSockAddr The local socket address to listen on.
 *
 * @retval OT_ERROR_NONE     Successfully enabled or disabled the listener.
 * @retval OT_ERROR_ALREADY  Already listening on the same port/address.
 * @retval OT_ERROR_FAILED   Failed to enable the listener.
 */
otError otPlatTcpEnableListener(otPlatTcpListener *aListener, const otPlatTcpSockAddr *aLocalSockAddr);

/**
 * Disables a TCP listener.
 *
 * The platform should stop listening for incoming connections on the socket associated with the listener.
 * Any incoming connection requests that have not yet been accepted should be discarded.
 *
 * @param[in] aListener  The TCP listener.
 */
void otPlatTcpDisableListener(otPlatTcpListener *aListener);

/**
 * Callback to accept an incoming TCP connection request on an active listener.
 *
 * This function is implemented and provided by the OpenThread stack for the platform to use.
 *
 * The callback returns a pointer to an `otPlatTcpConnection` for the new connection. If the callback returns NULL,
 * the incoming connection is rejected.
 *
 * @param[in] aListener      The TCP listener.
 * @param[in] aPeerSockAddr  The peer's socket address.
 *
 * @returns A pointer for the newly accepted connection, or NULL to reject the connection request.
 */
extern otPlatTcpConnection *otPlatTcpAccept(otPlatTcpListener *aListener, const otPlatTcpSockAddr *aPeerSockAddr);

/**
 * Initiates a TCP connection to a peer.
 *
 * The platform should initiate a TCP connection to the @p aPeerSockAddr.
 *
 * The @p aLocalSockAddr specifies the local address and port to bind to before connecting. It can be NULL if the
 * OpenThread stack does not specify a preference. If provided, fields within @p aLocalSockAddr may still be left
 * unspecified (e.g., the IP address can be all zeros, or the port can be zero). In all such cases, the platform
 * and the underlying TCP stack should automatically select an appropriate local IP address and/or an ephemeral port.
 *
 * If `OT_ERROR_NONE` is returned (indicating successful initialization of the connection process), the platform must
 * subsequently report the status. Upon successful connection establishment, the platform must invoke the
 * `otPlatTcpHandleConnected` callback. If it fails to establish the connection, the `otPlatTcpHandleDisconnected`
 * callback must be called to indicate the failure.
 *
 * @param[in] aConn          The TCP connection.
 * @param[in] aPeerSockAddr  The peer's socket address.
 * @param[in] aLocalSockAddr The local socket address. Can be NULL.
 *
 * @retval OT_ERROR_NONE     Successfully initiated the connection.
 * @retval OT_ERROR_FAILED   Failed to initiate the connection.
 */
otError otPlatTcpConnect(otPlatTcpConnection     *aConn,
                         const otPlatTcpSockAddr *aPeerSockAddr,
                         const otPlatTcpSockAddr *aLocalSockAddr);

/**
 * Indicates whether the TCP connection is currently in the connecting state.
 *
 * This function is provided by the OpenThread stack. The platform can use it to determine if a TCP connection is still
 * waiting for the TCP handshake to complete.
 *
 * @param[in] aConn  The TCP connection.
 *
 * @retval TRUE   The connection is currently in the connecting state.
 * @retval FALSE  The connection is not in the connecting state.
 */
extern bool otPlatTcpIsConnecting(otPlatTcpConnection *aConn);

/**
 * Callback to notify the connection establishment.
 *
 * This callback is implemented and provided by the OpenThread stack. It must be invoked by the platform to indicate
 * that the TCP handshake is complete and that the connection is now established.
 *
 * The platform must call this after a successful call to `otPlatTcpConnect()` when the connection is established.
 * For incoming connection requests (on an `otPlatTcpListener`), the platform must call this after the
 * `otPlatTcpAccept()` callback returns successfully and when the connection is established.
 *
 * @param[in] aConn   The TCP connection.
 */
extern void otPlatTcpHandleConnected(otPlatTcpConnection *aConn);

/**
 * Notifies the platform that there is pending data for transmission.
 *
 * This function is called by the OpenThread stack when it has new data for transmission. After this call, the platform
 * should indicate when it is ready to accept the data by invoking the `otPlatTcpHandleTxReady()` callback.
 *
 * The platform can also use `otPlatTcpIsTxPending()` to check if there is pending data for transmission.
 *
 * It is permissible for the platform implementation to invoke the `otPlatTcpHandleTxReady()` callback directly from
 * within `otPlatTcpNotifyTxPending()` before returning, if the underlying TCP transmit buffer is already available.
 * The OpenThread stack will handle this correctly.
 *
 * @param[in] aConn  The TCP connection.
 */
void otPlatTcpNotifyTxPending(otPlatTcpConnection *aConn);

/**
 * Indicates whether the TCP connection has pending data for transmission.
 *
 * This function is provided by the OpenThread stack. The platform can use it to check if there is any pending data
 * for transmission over the TCP connection.
 *
 * @param[in] aConn  The TCP connection.
 *
 * @retval TRUE   The connection has pending transmit data.
 * @retval FALSE  The connection does not have pending transmit data.
 */
extern bool otPlatTcpIsTxPending(otPlatTcpConnection *aConn);

/**
 * Callback to notify that the platform is ready to accept more transmit data.
 *
 * This function is implemented and provided by the OpenThread stack for the platform to use.
 *
 * The platform should invoke this callback when it is ready to accept more data for transmission over the TCP
 * connection, in response to a prior `otPlatTcpNotifyTxPending()` call. Upon being called, the OpenThread stack will
 * use `otPlatTcpSend()` to provide the pending TX data to the platform. The stack may call `otPlatTcpSend()` multiple
 * times during the execution of this callback.
 *
 * @param[in] aConn  The TCP connection.
 */
extern void otPlatTcpHandleTxReady(otPlatTcpConnection *aConn);

/**
 * Sends data over an active TCP connection.
 *
 * This function is called by the OpenThread stack to provide data for the platform to transmit. The data is provided
 * in a buffer. The platform should copy as much data as it can from the given buffer into its underlying platform
 * transmit buffer.
 *
 * The provided @p aBuffer is temporary. The platform must not store the pointer or assume the content remains valid
 * after this function returns. All required data must be copied during this call.
 *
 * The OpenThread stack typically invokes this function from the `otPlatTcpHandleTxReady()` callback. However, the
 * platform implementation must not assume this and should support being called at any time. If there is no space
 * available to accept any data, the platform can return zero.
 *
 * The OpenThread stack may call this function multiple times back-to-back to provide all queued transmit content
 * in chunks. The platform should be prepared to handle consecutive calls efficiently.
 *
 * @param[in] aConn    The TCP connection.
 * @param[in] aBuffer  A pointer to the buffer containing the data to send.
 * @param[in] aLength  The length (in bytes) of the data in the buffer.
 *
 * @returns The actual number of bytes accepted for transmission.
 */
uint16_t otPlatTcpSend(otPlatTcpConnection *aConn, const uint8_t *aBuffer, uint16_t aLength);

/**
 * Callback to notify the reception of data on a connection.
 *
 * This function is implemented and provided by the OpenThread stack for the platform to use.
 *
 * The platform invokes this callback to provide received data to the OpenThread stack. The provided @p aBuffer
 * only needs to remain valid for the duration of this call. The OpenThread stack will process and copy the
 * bytes as needed, and will not retain the @p aBuffer pointer after the function returns.
 *
 * Since TCP is a stream protocol, data can arrive in arbitrarily sized chunks. The platform does not need to
 * buffer or reassemble these; it can invoke this callback immediately as data is received, even if it is expecting
 * more data. The OpenThread stack handles all stream-level behavior, processing, and retention of the
 * received data. This helps simplify the platform implementation.
 *
 * On certain platforms (such as standard POSIX), a return value of 0 from `read()` or `recv()` indicates an
 * End-of-File (EOF) or graceful closure by the peer. The platform implementation must check for this condition and
 * report it by invoking `otPlatTcpHandleDisconnected()` with the reason set to `OT_PLAT_TCP_DISCONNECT_REASON_CLOSED`.
 * Importantly, calling `otPlatTcpHandleReceive()` with `aLength` set to zero does not signify a graceful closure in
 * the `otPlatTcp` APIs; such a call is treated as a no-op receive event by the OpenThread stack and is ignored.
 *
 * @param[in] aConn    The TCP connection.
 * @param[in] aBuffer  A pointer to the buffer containing the received data. Must not be NULL if @p aLength > 0.
 * @param[in] aLength  The length (in bytes) of the received data.
 */
extern void otPlatTcpHandleReceive(otPlatTcpConnection *aConn, const uint8_t *aBuffer, uint16_t aLength);

/**
 * Gracefully closes the TCP connection.
 *
 * This function initiates a graceful closure of the connection. The platform should transmit any remaining data
 * before performing the standard TCP connection termination.
 *
 * Once the connection is fully disconnected, or if it is already closed, or if an error occurs during the close
 * operation, the platform must indicate this by invoking the `otPlatTcpHandleDisconnected` callback.
 *
 * The platform must always call `otPlatTcpHandleDisconnected()` to report the final outcome of the connection.
 * It is permissible for the platform implementation to invoke this callback directly from within `otPlatTcpClose()`
 * before returning. The OpenThread stack will handle this correctly.
 *
 * @param[in] aConn   The TCP connection.
 */
void otPlatTcpClose(otPlatTcpConnection *aConn);

/**
 * Aborts the TCP connection.
 *
 * This function forcefully terminates the connection. Any unsent data is discarded.
 *
 * After this call, the platform must forget the @p aConn. Importantly, it must not invoke any callbacks using the
 * @p aConn any longer, including `otPlatTcpHandleDisconnected`. This effectively indicates to the platform that the
 * OpenThread core is de-allocating the @p aConn instance and it is no longer valid.
 *
 * @param[in] aConn   The TCP connection.
 */
void otPlatTcpAbort(otPlatTcpConnection *aConn);

/**
 * Callback to notify the connection disconnection.
 *
 * This function is implemented and provided by the OpenThread stack for the platform to use.
 *
 * This callback should be invoked by the platform when it fails to establish a connection, when an established
 * connection is successfully closed (by both endpoints), when it times out, or is reset or aborted.
 *
 * After this callback is invoked, the `otPlatTcpConnection` instance is no longer valid. The platform must not use it
 * in any future callbacks.
 *
 * @param[in] aConn   The TCP connection.
 * @param[in] aReason The reason for the disconnection.
 */
extern void otPlatTcpHandleDisconnected(otPlatTcpConnection *aConn, otPlatTcpDisconnectReason aReason);

/**
 * Gets the OpenThread instance associated with a given TCP connection.
 *
 * This function is provided by OpenThread core. Platform implementations can use it to get the OpenThread instance
 * associated with an active `otPlatTcpConnection`.
 *
 * @param[in] aConn  The TCP connection.
 *
 * @returns The OpenThread instance.
 */
extern otInstance *otPlatTcpGetInstanceForConnection(otPlatTcpConnection *aConn);

/**
 * Gets the OpenThread instance associated with a given TCP listener.
 *
 * This function is provided by OpenThread core. Platform implementations can use it to get the OpenThread instance
 * associated with an active `otPlatTcpListener`.
 *
 * @param[in] aListener  The TCP listener.
 *
 * @returns The OpenThread instance.
 */
extern otInstance *otPlatTcpGetInstanceForListener(otPlatTcpListener *aListener);

/**
 * Iterates through the active TCP listeners.
 *
 * This function can be used to iterate over all currently active TCP listeners associated with the OpenThread
 * instance. It allows platform implementations to process or manage listeners without needing to maintain their own
 * list of active listeners.
 *
 * The iteration is guaranteed to remain consistent even if callbacks (e.g., `otPlatTcpAccept`) are invoked during the
 * process.
 *
 * @param[in] aInstance      The OpenThread instance.
 * @param[in] aPrevListener  A pointer to the previous listener, or `NULL` to start the iteration from the beginning.
 *
 * @returns A pointer to the next listener, or `NULL` if there are no more listeners.
 */
extern otPlatTcpListener *otPlatTcpIterateListeners(otInstance *aInstance, otPlatTcpListener *aPrevListener);

/**
 * Iterates through the active TCP connections.
 *
 * This function can be used to iterate over all currently active TCP connections associated with the OpenThread
 * instance. It allows platform implementations to process or manage connections without needing to maintain their own
 * list of active connections.
 *
 * The iteration is guaranteed to remain consistent and safe even if callbacks are invoked during the process. For
 * example, if a connection is reported as disconnected via `otPlatTcpHandleDisconnected` during iteration, the
 * OpenThread stack ensures that the connection entry remains valid until the iteration is completed.
 *
 * @param[in] aInstance  The OpenThread instance.
 * @param[in] aPrevConn  A pointer to the previous connection, or `NULL` to start the iteration from the beginning.
 *
 * @returns A pointer to the next connection, or `NULL` if there are no more connections.
 */
extern otPlatTcpConnection *otPlatTcpIterateConnections(otInstance *aInstance, otPlatTcpConnection *aPrevConn);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_TCP_H_
