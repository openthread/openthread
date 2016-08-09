/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *  This file defines the top-level functions for the OpenThread NCP module.
 */

#ifndef NCP_H_
#define NCP_H_

#include <openthread-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the NCP.
 *
 * @param[in]  aContext  The OpenThread context structure.
 *
 */
void otNcpInit(otContext *aContext);

/**
 * @brief Send data to the host via a specific stream.
 *
 * This function attempts to send the given data to the host
 * using the given aStreamId. This is useful for reporting
 * error messages, implementing debug/diagnostic consoles,
 * and potentially other types of datastreams.
 *
 * The write either is accepted in its entirety or rejected.
 * Partial writes are not attempted.
 *
 * @param[in]  aStreamId  A numeric identifier for the stream to write to.
 *                        If set to '0', will default to the debug stream.
 * @param[in]  aDataPtr   A pointer to the data to send on the stream.
 *                        If aDataLen is non-zero, this param MUST NOT be NULL.
 * @param[in]  aDataLen   The number of bytes of data from aDataPtr to send.
 *
 * @retval kThreadError_None  The data was queued for delivery to the host.
 * @retval kThreadError_Busy  There are not enough resources to complete this
 *                            request. This is usually a temporary condition.
 * @retval kThreadError_InvalidArgs The given aStreamId was invalid.
*/
ThreadError otNcpStreamWrite(int aStreamId, const uint8_t *aDataPtr, int aDataLen);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
