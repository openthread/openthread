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
 *   This file implements external heap.
 *
 */

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_EXTERNAL_HEAP_ENABLE && !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE

#include "external_heap.hpp"

#include "common/debug.hpp"

namespace ot {
namespace Utils {

static otHeapCAllocFn sCAlloc;
static otHeapFreeFn   sFree;

otError HeapSetCAllocFree(otHeapCAllocFn aCAlloc, otHeapFreeFn aFree)
{
    sCAlloc = aCAlloc;
    sFree   = aFree;

    return OT_ERROR_NONE;
}

Heap::Heap(void)
{
    // Intentionally empty.
}

void *Heap::CAlloc(size_t aCount, size_t aSize)
{
    assert(sCAlloc != NULL);

    return sCAlloc(aCount, aSize);
}

void Heap::Free(void *aPointer)
{
    assert(sFree != NULL);

    sFree(aPointer);
}

bool Heap::IsClean(void) const
{
    // Dummy implementation
    return false;
}

size_t Heap::GetCapacity(void) const
{
    // Dummy implementation
    return 0;
}

size_t Heap::GetFreeSize(void) const
{
    // Dummy implementation
    return 0;
}

} // namespace Utils
} // namespace ot

#endif // OPENTHREAD_CONFIG_EXTERNAL_HEAP_ENABLE && !OPENTHREAD_CONFIG_MULTIPLE_INSTANCE_ENABLE
