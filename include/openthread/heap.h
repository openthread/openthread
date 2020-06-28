/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *  This file defines the API for setting external heap in OpenThread.
 */

#ifndef OPENTHREAD_HEAP_H_
#define OPENTHREAD_HEAP_H_

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-heap
 *
 * @brief
 *   This module includes functions that set the external OpenThread heap.
 *
 * @{
 *
 */

/**
 * Function pointer used to set external CAlloc function for OpenThread.
 *
 * @param[in]   aCount  Number of allocate units.
 * @param[in]   aSize   Unit size in bytes.
 *
 * @returns A pointer to the allocated memory.
 *
 * @retval  NULL    Indicates not enough memory.
 *
 */
typedef void *(*otHeapCAllocFn)(size_t aCount, size_t aSize);

/**
 * Function pointer used to set external Free function for OpenThread.
 *
 * @param[in]   aPointer    A pointer to the memory to free.
 *
 */
typedef void (*otHeapFreeFn)(void *aPointer);

// This is a temporary API and would be removed after moving heap to platform.
// TODO: Remove after moving heap to platform.
/**
 * @note This API is deprecated and use of it is discouraged.
 *
 */
void *otHeapCAlloc(size_t aCount, size_t aSize);

// This is a temporary API and would be removed after moving heap to platform.
// TODO: Remove after moving heap to platform.
/**
 * @note This API is deprecated and use of it is discouraged.
 *
 */
void otHeapFree(void *aPointer);

/**
 * This function sets the external heap CAlloc and Free
 * functions to be used by the OpenThread stack.
 *
 * This function must be used before invoking instance initialization.
 *
 * @param[in]  aCAlloc  A pointer to external CAlloc function.
 * @param[in]  aFree    A pointer to external Free function.
 *
 */
void otHeapSetCAllocFree(otHeapCAllocFn aCAlloc, otHeapFreeFn aFree);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_HEAP_H_
