/*
 *  Copyright (c) 2023, The OpenThread Authors.
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
 *   This file implements methods for a signed preference value and its 2-bit unsigned representation.
 */

#include "preference.hpp"

namespace ot {

uint8_t Preference::To2BitUint(int8_t aPrf) { return (aPrf == 0) ? k2BitMedium : ((aPrf > 0) ? k2BitHigh : k2BitLow); }

int8_t Preference::From2BitUint(uint8_t a2BitUint)
{
    static const int8_t kPreferences[] = {
        /* 0 (00)  -> */ kMedium,
        /* 1 (01)  -> */ kHigh,
        /* 2 (10)  -> */ kMedium, // Per RFC-4191, the reserved value (10) MUST be treated as (00)
        /* 3 (11)  -> */ kLow,
    };

    return kPreferences[a2BitUint & k2BitMask];
}

bool Preference::IsValid(int8_t aPrf) { return (aPrf == kHigh) || (aPrf == kMedium) || (aPrf == kLow); }

const char *Preference::ToString(int8_t aPrf) { return (aPrf == 0) ? "medium" : ((aPrf > 0) ? "high" : "low"); }

} // namespace ot
