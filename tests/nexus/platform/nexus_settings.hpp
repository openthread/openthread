/*
 *  Copyright (c) 2024, The OpenThread Authors.
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

#ifndef OT_NEXUS_SETTINGS_HPP_
#define OT_NEXUS_SETTINGS_HPP_

#include "common/const_cast.hpp"
#include "common/owning_list.hpp"
#include "instance/instance.hpp"

namespace ot {
namespace Nexus {

struct Settings
{
    enum SetAddMode : uint8_t
    {
        kSet,
        kAdd,
    };

    Error Get(uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength) const;
    Error SetOrAdd(SetAddMode aMode, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength);
    Error Delete(uint16_t aKey, int aIndex);
    void  Wipe(void);

    struct IndexMatcher
    {
        IndexMatcher(int aIndex) { mIndex = aIndex; }

        int mIndex;
    };

    struct Entry : public Heap::Allocatable<Entry>, public LinkedListEntry<Entry>
    {
        struct Value : public Heap::Allocatable<Value>, public LinkedListEntry<Value>
        {
            bool Matches(const IndexMatcher &aIndexMataher) const { return (AsNonConst(aIndexMataher).mIndex-- == 0); }

            Value     *mNext;
            Heap::Data mData;
        };

        bool Matches(uint16_t aKey) const { return mKey == aKey; }

        Entry            *mNext;
        uint16_t          mKey;
        OwningList<Value> mValues;
    };

    OwningList<Entry> mEntries;
};

} // namespace Nexus
} // namespace ot

#endif // OT_NEXUS_SETTINGS_HPP_
