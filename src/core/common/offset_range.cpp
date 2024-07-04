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

/**
 * @file
 *   This file implements helper method for an offset range.
 */

#include "offset_range.hpp"

#include "common/code_utils.hpp"
#include "common/message.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"

namespace ot {

void OffsetRange::Init(uint16_t aOffset, uint16_t aLength)
{
    uint16_t maxLength = NumericLimits<uint16_t>::kMax - aOffset;

    mOffset = aOffset;
    mLength = Min(aLength, maxLength);
}

void OffsetRange::InitFromRange(uint16_t aStartOffset, uint16_t aEndOffset)
{
    Init(aStartOffset, Max(aStartOffset, aEndOffset) - aStartOffset);
}

void OffsetRange::InitFromMessageOffsetToEnd(const Message &aMessage)
{
    InitFromRange(aMessage.GetOffset(), aMessage.GetLength());
}

void OffsetRange::InitFromMessageFullLength(const Message &aMessage) { Init(0, aMessage.GetLength()); }

void OffsetRange::AdvanceOffset(uint32_t aLength)
{
    uint16_t length = static_cast<uint16_t>(Min<uint32_t>(aLength, mLength));

    mOffset += length;
    mLength -= length;
}

void OffsetRange::ShrinkLength(uint16_t aLength) { mLength = Min(mLength, aLength); }

} // namespace ot
