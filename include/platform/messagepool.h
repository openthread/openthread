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
 *   This file includes the platform abstraction for the message pool.
 */

#ifndef MESSAGEPOOL_H_
#define MESSAGEPOOL_H_

#include <stdint.h>
#include <openthread-types.h>

/**
 * @defgroup messagepool MessagePool
 * @ingroup platform
 *
 * @brief
 *   This module includes the platform abstraction for the message pool.
 *
 * @{
 *
 */

/**
 * This structure contains a pointer to the next Message buffer.
 *
 */
struct BufferHeader
{
    struct BufferHeader *mNext;  ///< A pointer to the next Message buffer.
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the platform implemented message pool with the provided buffer list.
 *
 * @param[in] aFreeBufferList   A linked list of free buffers.
 * @param[in] aNumFreeBuffers   A pointer to an int containing the number of free buffers in aFreeBufferList
 *
 */
void otPlatMessagePoolInit(struct BufferHeader *aFreeBufferList, int *aNumFreeBuffers);

/**
 * Allocate a buffer from the platform managed buffer pool
 *
 * @returns A pointer to the Buffer or NULL if no Buffers are available.
 *
 */
struct BufferHeader *otPlatMessagePoolNew(void);

/**
 * This function is used to free a Buffer to the free Buffer pool.
 *
 * @param[in]  aBuffer  The Buffer to free.
 *
 */
void otPlatMessagePoolFree(struct BufferHeader *aBuffer);

#ifdef __cplusplus
}  // extern "C"
#endif

/**
 * @}
 *
 */


#endif  // MESSAGEPOOL_H_
