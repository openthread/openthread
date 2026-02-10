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
 *   This file includes the platform abstraction for dynamic memory allocation.
 */

#ifndef OPENTHREAD_PLATFORM_MEMORY_H_
#define OPENTHREAD_PLATFORM_MEMORY_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup plat-memory
 *
 * @brief
 *   This module includes the platform abstraction for dynamic memory allocation.
 *
 * @{
 */

/*
 * Dynamic memory allocation is primarily needed for Thread Border Router functionalities and protocols
 * such as SRP (server), mDNS or DHCPv6 PD. It may also be used for OpenThread message buffers.
 */

/**
 * Dynamically allocates new memory. On platforms that support it, they should redirect to `calloc`. For
 * those that don't support `calloc`, they should implement the standard `calloc` behavior.
 *
 * See: https://man7.org/linux/man-pages/man3/calloc.3.html
 *
 * Is required for `OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE`.
 *
 * @param[in] aNum   The number of blocks to allocate
 * @param[in] aSize  The size of each block to allocate
 *
 * @retval void*  The pointer to the front of the memory allocated
 * @retval NULL   Failed to allocate the memory requested.
 */
void *otPlatCAlloc(size_t aNum, size_t aSize);

/**
 * Frees memory that was dynamically allocated by `otPlatCAlloc()`.
 *
 * Is required for `OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE`.
 *
 * @param[in] aPtr  A pointer the memory blocks to free. The pointer may be NULL.
 */
void otPlatFree(void *aPtr);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_PLATFORM_MEMORY_H_
