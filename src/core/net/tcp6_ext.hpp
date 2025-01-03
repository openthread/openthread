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
 *   This file includes definitions for TCP/IPv6 socket extensions.
 */

#ifndef TCP6_EXT_HPP_
#define TCP6_EXT_HPP_

#include "openthread-core-config.h"

#include <openthread/tcp_ext.h>

#include "net/tcp6.hpp"

namespace ot {
namespace Ip6 {

/**
 * @addtogroup core-tcp-ext
 *
 * @brief
 *   This module includes definitions for TCP/IPv6 socket extensions.
 *
 * @{
 */

/**
 * Represents a TCP circular send buffer.
 */
class TcpCircularSendBuffer : public otTcpCircularSendBuffer
{
public:
    /**
     * Initializes a TCP circular send buffer.
     *
     * @sa otTcpCircularSendBufferInitialize
     *
     * @param[in]  aDataBuffer      A pointer to memory to use to store data in the TCP circular send buffer.
     * @param[in]  aCapacity        The capacity, in bytes, of the TCP circular send buffer, which must equal the size
     *                              of the memory pointed to by @p aDataBuffer .
     */
    void Initialize(void *aDataBuffer, size_t aCapacity);

    /**
     * Sends out data on a TCP endpoint, using this TCP circular send buffer to manage buffering.
     *
     * @sa otTcpCircularSendBufferWrite, particularly for guidance on how @p aEndpoint must be chosen.
     *
     * @param[in]   aEndpoint The TCP endpoint on which to send out data.
     * @param[in]   aData     A pointer to data to copy into the TCP circular send buffer.
     * @param[in]   aLength   The length of the data pointed to by @p aData to copy into the TCP circular send buffer.
     * @param[out]  aWritten  Populated with the amount of data copied into the send buffer, which might be less than
     *                        @p aLength if the send buffer reaches capacity.
     * @param[in]   aFlags    Flags specifying options for this operation.
     *
     * @retval kErrorNone on success, or the error returned by the TCP endpoint on failure.
     */
    Error Write(Tcp::Endpoint &aEndpoint, const void *aData, size_t aLength, size_t &aWritten, uint32_t aFlags);

    /**
     * Performs circular-send-buffer-specific handling in the otTcpForwardProgress callback.
     *
     * @sa otTcpCircularSendBufferHandleForwardProgress
     *
     * @param[in]  aInSendBuffer  Value of @p aInSendBuffer passed to the otTcpForwardProgress() callback.
     */
    void HandleForwardProgress(size_t aInSendBuffer);

    /**
     * Returns the amount of free space in this TCP circular send buffer.
     *
     * @sa otTcpCircularSendBufferFreeSpace
     *
     * @return The amount of free space in the send buffer.
     */
    size_t GetFreeSpace(void) const;

    /**
     * Forcibly discards all data in this TCP circular send buffer.
     *
     * @sa otTcpCircularSendBufferForceDiscardAll
     */
    void ForceDiscardAll(void);

    /**
     * Deinitializes this TCP circular send buffer.
     *
     * @sa otTcpCircularSendBufferDeinitialize
     *
     * @retval kErrorNone    Successfully deinitialized this TCP circular send buffer.
     * @retval kErrorFailed  Failed to deinitialize the TCP circular send buffer.
     */
    Error Deinitialize(void);

private:
    size_t GetIndex(size_t aStart, size_t aOffsetFromStart) const;
};

} // namespace Ip6

DefineCoreType(otTcpCircularSendBuffer, Ip6::TcpCircularSendBuffer);

} // namespace ot

#endif // TCP6_HPP_
