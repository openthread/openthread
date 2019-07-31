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
 *   This file includes definitions heap interface.
 *
 */

#ifndef OT_HEAP_HPP_
#define OT_HEAP_HPP_

#include <openthread/instance.h>

#include <stddef.h>
#include <stdint.h>

namespace ot {
namespace Utils {

/**
 * This class represents heap interface.
 *
 */
class Heap
{
public:
    /**
     * This constructor initializes the heap interface object.
     *
     */
    Heap(void);

    /**
     * This method allocates at least @p aCount * @aSize bytes memory and initialize to zero.
     *
     * @param[in]   aCount  Number of allocate units.
     * @param[in]   aSize   Unit size in bytes.
     *
     * @returns A pointer to the allocated memory.
     *
     * @retval  NULL    Indicates not enough memory.
     *
     */
    void *CAlloc(size_t aCount, size_t aSize);

    /**
     * This method free memory pointed by @p aPointer.
     *
     * @param[in]   aPointer    A pointer to the memory to free.
     *
     */
    void Free(void *aPointer);

    /**
     * This method returns whether the heap is clean.
     *
     */
    bool IsClean(void) const;

    /**
     * This method returns the capacity of this heap.
     *
     */
    size_t GetCapacity(void) const;

    /**
     * This method returns free space of this heap.
     */
    size_t GetFreeSize(void) const;
};

} // namespace Utils
} // namespace ot

#endif // OT_HEAP_HPP_
