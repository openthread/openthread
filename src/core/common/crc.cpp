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
 *   This file implements CRC16 computations.
 */

#include "crc.hpp"

namespace ot {

template <typename UintType> UintType CrcCalculator<UintType>::FeedByte(uint8_t aByte)
{
    static constexpr UintType kMsb      = kIsUint16 ? (1u << 15) : (1u << 31);
    static constexpr uint8_t  kBitShift = kIsUint16 ? 8 : 24;

    mCrc ^= (static_cast<UintType>(aByte) << kBitShift);

    for (uint8_t i = 8; i > 0; i--)
    {
        bool msbIsSet = (mCrc & kMsb);

        mCrc <<= 1;

        if (msbIsSet)
        {
            mCrc ^= mPolynomial;
        }
    }

    return mCrc;
}

template <typename UintType> UintType CrcCalculator<UintType>::FeedBytes(const void *aBytes, uint16_t aLength)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(aBytes);

    while (aLength-- > 0)
    {
        FeedByte(*bytes++);
    }

    return mCrc;
}

template <typename UintType>
UintType CrcCalculator<UintType>::Feed(const Message &aMessage, const OffsetRange &aOffsetRange)
{
    uint16_t       length = aOffsetRange.GetLength();
    Message::Chunk chunk;

    aMessage.GetFirstChunk(aOffsetRange.GetOffset(), length, chunk);

    while (chunk.GetLength() > 0)
    {
        FeedBytes(chunk.GetBytes(), chunk.GetLength());
        aMessage.GetNextChunk(length, chunk);
    }

    return mCrc;
}

template class CrcCalculator<uint16_t>;
template class CrcCalculator<uint32_t>;

} // namespace ot
