/*
 *  Copyright (c) 2022, The OpenThread Authors.
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
 *   This file defines extensions to the OpenThread TCP API.
 */

#ifndef OPENTHREAD_TCP_EXT_H_
#define OPENTHREAD_TCP_EXT_H_

#include <openthread/tcp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-tcp-ext
 *
 * @brief
 *   This module includes easy-to-use abstractions on top of the base TCP API.
 *
 * @{
 */

/**
 * Represents a circular send buffer for use with a TCP endpoint.
 *
 * Using a circular send buffer is optional. Applications can use a TCP
 * endpoint to send data by managing otLinkedBuffers directly. However, some
 * applications may find it more convenient to have a circular send buffer;
 * such applications can call otTcpCircularSendBufferWrite() to "attach" a
 * circular send buffer to a TCP endpoint and send out data on that TCP
 * endpoint, relying on the circular send buffer to manage the underlying
 * otLinkedBuffers.
 *
 * otTcpCircularSendBuffer is implemented on top of the otLinkedBuffer-based
 * API provided by an otTcpEndpoint. Once attached to an otTcpEndpoint, an
 * otTcpCircularSendBuffer performs all the work of managing otLinkedBuffers
 * for the connection. This means that, once an otTcpCircularSendBuffer is
 * attached to an otTcpEndpoint, the application should not call
 * otTcpSendByReference() or otTcpSendByExtension() on that otTcpEndpoint.
 * Instead, the application should use otTcpCircularSendBufferWrite() to add
 * data to the send buffer.
 *
 * The otTcpForwardProgress() callback is the intended way for users to learn
 * when space becomes available in the circular send buffer. On an
 * otTcpEndpoint to which an otTcpCircularSendBuffer is attached, the
 * application MUST install an otTcpForwardProgress() callback and call
 * otTcpCircularSendBufferHandleForwardProgress() on the attached
 * otTcpCircularSendBuffer at the start of the callback function. It is
 * recommended that the user NOT install an otTcpSendDone() callback, as all
 * management of otLinkedBuffers is handled by the circular send buffer.
 *
 * The application should not inspect the fields of this structure directly; it
 * should only interact with it via the TCP Circular Send Buffer API functions
 * whose signature are provided in this file.
 */
typedef struct otTcpCircularSendBuffer
{
    uint8_t *mDataBuffer;   ///< Pointer to data in the circular send buffer
    size_t   mCapacity;     ///< Length of the circular send buffer
    size_t   mStartIndex;   ///< Index of the first valid byte in the send buffer
    size_t   mCapacityUsed; ///< Number of bytes stored in the send buffer

    otLinkedBuffer mSendLinks[2];
    uint8_t        mFirstSendLinkIndex;
} otTcpCircularSendBuffer;

/**
 * Initializes a TCP circular send buffer.
 *
 * @param[in]  aSendBuffer      A pointer to the TCP circular send buffer to initialize.
 * @param[in]  aDataBuffer      A pointer to memory to use to store data in the TCP circular send buffer.
 * @param[in]  aCapacity        The capacity, in bytes, of the TCP circular send buffer, which must equal the size of
 *                              the memory pointed to by @p aDataBuffer .
 */
void otTcpCircularSendBufferInitialize(otTcpCircularSendBuffer *aSendBuffer, void *aDataBuffer, size_t aCapacity);

/**
 * Defines flags passed to @p otTcpCircularSendBufferWrite.
 */
enum
{
    OT_TCP_CIRCULAR_SEND_BUFFER_WRITE_MORE_TO_COME = 1 << 0,
};

/**
 * Sends out data on a TCP endpoint, using the provided TCP circular send
 * buffer to manage buffering.
 *
 * Once this function is called, @p aSendBuffer and @p aEndpoint are considered
 * "attached" to each other. While they are attached, ALL send operations for
 * @p aEndpoint must be made using @p aSendBuffer and ALL operations on
 * @p aSendBuffer must be associated with @p aEndpoint .
 *
 * The only way to "detach" a TCP circular send buffer and a TCP endpoint is to
 * wait for the send buffer to become completely empty. This can happen in two
 * ways: (1) all data in the send buffer is sent and acknowledged in the normal
 * course of TCP protocol operation, or (2) the connection is terminated.
 *
 * The recommended usage pattern is to use a single TCP circular send buffer
 * with a TCP endpoint, and to send data on that TCP endpoint only via its
 * associated TCP circular buffer. This recommended usage pattern sidesteps the
 * issues described above by always using a TCP endpoint and TCP circular send
 * buffer together.
 *
 * If the circular send buffer reaches capacity, only a prefix of the provided
 * data is copied into the circular send buffer.
 *
 * @param[in]   aEndpoint    The TCP endpoint on which to send out data.
 * @param[in]   aSendBuffer  The TCP circular send buffer into which to copy data.
 * @param[in]   aData        A pointer to data to copy into the TCP circular send buffer.
 * @param[in]   aLength      The length of the data pointed to by @p aData to copy into the TCP circular send buffer.
 * @param[out]  aWritten     Populated with the amount of data copied into the send buffer, which might be less than
 *                           @p aLength if the send buffer reaches capacity.
 * @param[in]   aFlags       Flags specifying options for this operation (see enumeration above).
 *
 * @retval OT_ERROR_NONE    Successfully copied data into the send buffer and sent it on the TCP endpoint.
 * @retval OT_ERROR_FAILED  Failed to send out data on the TCP endpoint.
 */
otError otTcpCircularSendBufferWrite(otTcpEndpoint           *aEndpoint,
                                     otTcpCircularSendBuffer *aSendBuffer,
                                     const void              *aData,
                                     size_t                   aLength,
                                     size_t                  *aWritten,
                                     uint32_t                 aFlags);

/**
 * Performs circular-send-buffer-specific handling in the otTcpForwardProgress
 * callback.
 *
 * The application is expected to install an otTcpForwardProgress() callback on
 * the otTcpEndpoint, and call this function at the start of the callback
 * function for circular-send-buffer-specific processing.
 *
 * In the callback function, the application can determine the amount of free
 * space in the circular send buffer by calling
 * otTcpCircularSendBufferFreeSpace(), or by comparing @p aInSendBuffer with
 * the send buffer's capacity, chosen by the user when calling
 * otTcpCircularSendBufferInitialize().
 *
 * @param[in]  aSendBuffer    A pointer to the TCP circular send buffer for the endpoint for which
 *                            otTcpForwardProgress() was invoked.
 * @param[in]  aInSendBuffer  Value of @p aInSendBuffer passed to the otTcpForwardProgress() callback.
 */
void otTcpCircularSendBufferHandleForwardProgress(otTcpCircularSendBuffer *aSendBuffer, size_t aInSendBuffer);

/**
 * Returns the amount of free space in the TCP circular send buffer.
 *
 * This operation will always succeed.
 *
 * @param[in]  aSendBuffer  A pointer to the TCP circular send buffer whose amount of free space to return.
 *
 * @returns The amount of free space in the send buffer.
 */
size_t otTcpCircularSendBufferGetFreeSpace(const otTcpCircularSendBuffer *aSendBuffer);

/**
 * Forcibly discards all data in the circular send buffer.
 *
 * The application is expected to call this function when a TCP connection is
 * terminated unceremoniously (e.g., if the application calls
 * otTcpEndpointAbort() or is informed of a reset connection via the
 * otTcpConnectionLost() callback).
 *
 * Calling this function on a nonempty TCP circular send buffer attached to a
 * TCP endpoint results in undefined behavior.
 *
 * @param[in]  aSendBuffer  The TCP circular send buffer whose data to discard.
 */
void otTcpCircularSendBufferForceDiscardAll(otTcpCircularSendBuffer *aSendBuffer);

/**
 * Deinitializes a TCP circular send buffer, detaching it if attached.
 *
 * If the TCP circular send buffer is not empty, then this operation will fail.
 *
 * @param[in]  aSendBuffer  The TCP circular send buffer to deinitialize.
 *
 * @retval OT_ERROR_NONE    Successfully deinitialize the TCP circular send buffer.
 * @retval OT_ERROR_BUSY    Circular buffer contains data and cannot be deinitialized.
 */
otError otTcpCircularSendBufferDeinitialize(otTcpCircularSendBuffer *aSendBuffer);

/**
 * Context structure to use with mbedtls_ssl_set_bio.
 */
typedef struct otTcpEndpointAndCircularSendBuffer
{
    otTcpEndpoint           *mEndpoint;
    otTcpCircularSendBuffer *mSendBuffer;
} otTcpEndpointAndCircularSendBuffer;

/**
 * Non-blocking send callback to pass to mbedtls_ssl_set_bio.
 *
 * @param[in]  aCtx  A pointer to an otTcpEndpointAndCircularSendBuffer.
 * @param[in]  aBuf  The data to add to the send buffer.
 * @param[in]  aLen  The amount of data to add to the send buffer.
 *
 * @returns The number of bytes sent, or an mbedtls error code.
 */
int otTcpMbedTlsSslSendCallback(void *aCtx, const unsigned char *aBuf, size_t aLen);

/**
 * Non-blocking receive callback to pass to mbedtls_ssl_set_bio.
 *
 * @param[in]   aCtx  A pointer to an otTcpEndpointAndCircularSendBuffer.
 * @param[out]  aBuf  The buffer into which to receive data.
 * @param[in]   aLen  The maximum amount of data that can be received.
 *
 * @returns The number of bytes received, or an mbedtls error code.
 */
int otTcpMbedTlsSslRecvCallback(void *aCtx, unsigned char *aBuf, size_t aLen);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_TCP_EXT_H_
