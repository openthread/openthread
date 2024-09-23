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
 *  This file provides a generic binary search and related helper functions.
 */

#ifndef BINARY_SEARCH_HPP_
#define BINARY_SEARCH_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

namespace ot {

class BinarySearch
{
public:
    /**
     * Performs binary search in a given sorted table array to find an entry matching a given key.
     *
     * @note This method requires the array to be sorted, otherwise its behavior is undefined.
     *
     * @tparam Key         The type of `Key` to search for.
     * @tparam Entry       The table `Entry` type.
     * @tparam kLength     The array length (number of entries in the array).
     *
     * The `Entry` class MUST provide the following method to compare the entry against a given key.
     *
     *    int Entry::Compare(const Key &aKey) const;
     *
     * The return value indicates the comparison result between @p aKey and the entry (similar to `strcmp()`), i.e.,
     * zero means perfect match, positive (> 0) indicates @p aKey is larger than entry, and negative indicates @p aKey
     * is smaller than entry.
     *
     * @note In the common use of this method as `Find(key, kTable)` where `kTable` is a fixed size array, the
     * template types/parameters do not need to be explicitly specified and can be inferred from the passed-in argument.
     *
     * @param[in] aKey    The key to search for within the table.
     * @param[in] aTable  A reference to an array of `kLength` entries of type `Entry`
     *
     * @returns A pointer to the entry in the table if a match is found, otherwise `nullptr` (no match in table).
     */
    template <typename Key, typename Entry, uint16_t kLength>
    static const Entry *Find(const Key &aKey, const Entry (&aTable)[kLength])
    {
        return static_cast<const Entry *>(
            Find(&aKey, &aTable[0], kLength, sizeof(aTable[0]), BinarySearch::Compare<Key, Entry>));
    }

    /**
     * Indicates whether a given table array is sorted based or not.
     *
     * Is `constexpr` and is intended for use in `static_assert`s to verify that a `constexpr` lookup table
     * array is sorted. It is not recommended for use in other situations.
     *
     * @tparam Entry       The table entry type.
     * @tparam kLength     The array length (number of entries in the array).
     *
     * The `Entry` class MUST provide the following `static` and `constexpr` method to compare two entries.
     *
     *    constexpr static bool Entry::AreInOrder(const Entry &aFirst, const Entry &aSecond);
     *
     * The return value MUST be TRUE if the entries are in order, i.e. `aFirst < aSecond` and FALSE otherwise.
     *
     * @param[in] aTable  A reference to an array of `kLength` entries on type `Entry`
     *
     * @retval TRUE   If the entries in @p aTable are sorted.
     * @retval FALSE  If the entries in @p aTable are not sorted.
     */
    template <typename Entry, uint16_t kLength> static constexpr bool IsSorted(const Entry (&aTable)[kLength])
    {
        return IsSorted(&aTable[0], kLength);
    }

private:
    typedef int (&Comparator)(const void *aKey, const void *aEntry);

    template <typename Entry> static constexpr bool IsSorted(const Entry *aTable, uint16_t aLength)
    {
        return (aLength <= 1) ? true : Entry::AreInOrder(aTable[0], aTable[1]) && IsSorted(aTable + 1, aLength - 1);
    }

    template <typename Key, typename Entry> static int Compare(const void *aKey, const void *aEntry)
    {
        return static_cast<const Entry *>(aEntry)->Compare(*static_cast<const Key *>(aKey));
    }

    static const void *Find(const void *aKey,
                            const void *aTable,
                            uint16_t    aLength,
                            uint16_t    aEntrySize,
                            Comparator  aComparator);
};

} // namespace ot

#endif // BINARY_SEARCH_HPP_
