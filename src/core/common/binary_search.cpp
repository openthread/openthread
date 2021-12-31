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
 *  This file implements a generic binary search.
 */

#include "binary_search.hpp"

#include "common/code_utils.hpp"

namespace ot {

const void *BinarySearch::Find(const void *aKey,
                               const void *aTable,
                               uint16_t    aLength,
                               uint16_t    aEntrySize,
                               Comparator  aComparator)
{
    const void *entry;
    uint16_t    left  = 0;
    uint16_t    right = aLength;

    while (left < right)
    {
        uint16_t middle = (left + right) / 2;
        int      compare;

        // Note that `aTable` array entry type is not known here and
        // only its size is given as `aEntrySize`. Based on this, we
        // can calculate the pointer to the table entry at any index
        // (such as `[middle]`) which is then passed to the given
        // `aComparator` function which knows how to cast the void
        // pointer to proper entry type and compare it with `aKey`.
        // This model keeps the implementation generic and reusable
        // allowing it to be used with any entry type.

        entry = reinterpret_cast<const uint8_t *>(aTable) + aEntrySize * middle;

        compare = aComparator(aKey, entry);

        if (compare == 0)
        {
            ExitNow();
        }
        else if (compare > 0)
        {
            left = middle + 1;
        }
        else
        {
            right = middle;
        }
    }

    entry = nullptr;

exit:
    return entry;
}

} // namespace ot
