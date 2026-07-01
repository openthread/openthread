/*
 *  Copyright (c) 2025, The OpenThread Authors.
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
 *   This file implements LTV (Length-Type-Value) generation and parsing.
 */

#include "ltvs.hpp"

#include <string.h>

#include "common/code_utils.hpp"

namespace ot {

// Returns ceil(log2(L)), where ceil_log2(0) == 0 and ceil_log2(1) == 0.
static uint8_t CeilLog2(uint8_t aL)
{
    uint8_t n = 0;
    uint8_t p = 1;

    while (p < aL)
    {
        n++;
        p = static_cast<uint8_t>(p << 1);
    }

    return n;
}

Error Ltv::AppendTo(FrameBuilder &aFrameBuilder) const { return aFrameBuilder.AppendBytes(this, GetTotalSize()); }

Error Ltv::Append(FrameBuilder &aFrameBuilder, uint8_t aType, const void *aValue, uint8_t aLength)
{
    Error error;
    Ltv   ltv;

    ltv.SetLength(aLength);
    ltv.SetType(aType);
    SuccessOrExit(error = aFrameBuilder.AppendBytes(&ltv, kHeaderSize));

    if (aLength > 0)
    {
        SuccessOrExit(error = aFrameBuilder.AppendBytes(aValue, aLength));
    }

exit:
    return error;
}

const Ltv *Ltv::FindLtv(const void *aBuffer, uint16_t aLength, uint8_t aType)
{
    Iterator   iter;
    const Ltv *result = nullptr;

    iter.Init(static_cast<const uint8_t *>(aBuffer), aLength);

    while (!iter.IsDone())
    {
        // Capture the LTV reference before advancing. Advance() validates that the
        // declared value fits in the remaining buffer; kErrorParse means truncated.
        const Ltv &ltv = iter.GetLtv();

        VerifyOrExit(iter.Advance() == kErrorNone);

        if (ltv.GetType() == aType)
        {
            result = &ltv;
            ExitNow();
        }
    }

exit:
    return result;
}

void Ltv::Iterator::Init(const uint8_t *aBuffer, uint16_t aLength) { mData.Init(aBuffer, aLength); }

Error Ltv::Iterator::Advance(void)
{
    Error    error = kErrorNone;
    uint16_t size;

    VerifyOrExit(!IsDone());

    size = GetLtv().GetTotalSize();
    VerifyOrExit(mData.CanRead(size), error = kErrorParse);
    mData.SkipOver(size);

exit:
    return error;
}

uint8_t PackedLtvStream::Encode(const uint8_t *aPlain, uint8_t aPlainLen, uint8_t *aPacked, uint8_t aPackedMax)
{
    // Collect (type, valueLen, valuePtr) entries from the plain [L][T][V...] buffer.
    static constexpr uint8_t kMaxEntries = 8;

    struct Entry
    {
        uint8_t        type;
        uint8_t        valueLen;
        const uint8_t *value;
    };

    Entry   entries[kMaxEntries];
    uint8_t count = 0;

    for (const uint8_t *p = aPlain, *end = aPlain + aPlainLen; p + 2 <= end && count < kMaxEntries;)
    {
        entries[count].type     = p[1];
        entries[count].valueLen = p[0];
        entries[count].value    = p + 2;
        count++;
        p += 2 + p[0];
    }

    // Encode back-to-front: each LTV's header bit-split depends on L = total bytes remaining
    // from this LTV to the end (own size + already-encoded suffix size).
    uint8_t scratch[64]; // packed output is always <= plain input length
    uint8_t scratchLen = 0;

    for (int i = static_cast<int>(count) - 1; i >= 0; i--)
    {
        uint8_t        t         = entries[i].type;
        uint8_t        vLen      = entries[i].valueLen;
        const uint8_t *v         = entries[i].value;
        uint8_t        hdrBytes  = 0;
        uint8_t        firstByte = 0;
        uint8_t        typeByte  = 0;
        bool           done      = false;

        // Try compact (1-byte header) then base (2-byte header).
        for (uint8_t hdr = 1; hdr <= 2 && !done; hdr++)
        {
            uint8_t own   = static_cast<uint8_t>(hdr + vLen);
            uint8_t L     = static_cast<uint8_t>(own + scratchLen);
            uint8_t n     = CeilLog2(L);
            uint8_t shift = static_cast<uint8_t>(8u - n);

            if (n == 0)
            {
                // L == 1: encode as a lone type byte (value must be zero-length).
                if (hdr == 1 && vLen == 0)
                {
                    hdrBytes  = 1;
                    firstByte = t;
                    done      = true;
                }

                continue;
            }

            if (vLen > static_cast<uint8_t>((1u << n) - 1u))
            {
                continue;
            }

            uint8_t allOnes = static_cast<uint8_t>((1u << shift) - 1u);

            if (hdr == 1)
            {
                // Compact: type in the lower `shift` bits; all-ones is the escape code.
                if (t < allOnes)
                {
                    hdrBytes  = 1;
                    firstByte = static_cast<uint8_t>((vLen << shift) | t);
                    done      = true;
                }
            }
            else
            {
                // Base: all-ones escape in lower bits; dedicated type byte follows.
                hdrBytes  = 2;
                firstByte = static_cast<uint8_t>((vLen << shift) | allOnes);
                typeByte  = t;
                done      = true;
            }
        }

        if (!done)
        {
            continue;
        }

        uint8_t insertLen = static_cast<uint8_t>(hdrBytes + vLen);

        memmove(scratch + insertLen, scratch, scratchLen);
        scratch[0] = firstByte;

        if (hdrBytes == 2)
        {
            scratch[1] = typeByte;
        }

        if (vLen > 0)
        {
            memcpy(scratch + hdrBytes, v, vLen);
        }

        scratchLen += insertLen;
    }

    uint8_t written = (scratchLen <= aPackedMax) ? scratchLen : aPackedMax;
    memcpy(aPacked, scratch, written);

    return written;
}

Error PackedLtvStream::Decode(const uint8_t *aPacked,
                              uint8_t        aPackedLen,
                              uint8_t       *aPlain,
                              uint8_t        aPlainMax,
                              uint8_t       &aPlainLen)
{
    Error   error  = kErrorNone;
    uint8_t offset = 0;
    uint8_t outLen = 0;

    while (offset < aPackedLen)
    {
        // L is the remaining byte count — the same context value used during encoding.
        uint8_t L     = static_cast<uint8_t>(aPackedLen - offset);
        uint8_t n     = CeilLog2(L);
        uint8_t first = aPacked[offset];
        uint8_t ltvLen;
        uint8_t ltvType;
        uint8_t hdrSize;

        if (n == 0)
        {
            ltvType = first;
            ltvLen  = 0;
            hdrSize = 1;
        }
        else
        {
            uint8_t shift   = static_cast<uint8_t>(8u - n);
            uint8_t lenbits = static_cast<uint8_t>(first >> shift);
            uint8_t rest    = first & static_cast<uint8_t>((1u << shift) - 1u);
            uint8_t allOnes = static_cast<uint8_t>((1u << shift) - 1u);

            if (rest == allOnes)
            {
                VerifyOrExit(offset + 1u < aPackedLen, error = kErrorParse);
                ltvType = aPacked[offset + 1u];
                ltvLen  = lenbits;
                hdrSize = 2;
            }
            else
            {
                ltvType = rest;
                ltvLen  = lenbits;
                hdrSize = 1;
            }
        }

        VerifyOrExit(offset + hdrSize + ltvLen <= aPackedLen, error = kErrorParse);
        VerifyOrExit(outLen + 2u + ltvLen <= aPlainMax, error = kErrorNoBufs);

        aPlain[outLen++] = ltvLen;
        aPlain[outLen++] = ltvType;

        if (ltvLen > 0)
        {
            memcpy(aPlain + outLen, aPacked + offset + hdrSize, ltvLen);
            outLen += ltvLen;
        }

        offset = static_cast<uint8_t>(offset + hdrSize + ltvLen);
    }

    aPlainLen = outLen;

exit:
    return error;
}

void PackedLtvStream::Iterator::Init(const uint8_t *aPacked, uint8_t aPackedLen)
{
    mBuffer   = aPacked;
    mTotalLen = aPackedLen;
    mOffset   = 0;
    mDone     = false;
    mType     = 0;
    mLen      = 0;
    mValue    = nullptr;
    mHdrSize  = 0;
    IgnoreError(DecodeEntry());
}

Error PackedLtvStream::Iterator::Advance(void)
{
    Error error = kErrorNone;

    VerifyOrExit(!mDone);
    mOffset = static_cast<uint8_t>(mOffset + mHdrSize + mLen);
    error   = DecodeEntry();

exit:
    return error;
}

Error PackedLtvStream::Iterator::DecodeEntry(void)
{
    Error error = kErrorNone;

    if (mOffset >= mTotalLen)
    {
        mDone = true;
        ExitNow();
    }

    {
        uint8_t L     = static_cast<uint8_t>(mTotalLen - mOffset);
        uint8_t n     = CeilLog2(L);
        uint8_t first = mBuffer[mOffset];

        if (n == 0)
        {
            mType    = first;
            mLen     = 0;
            mHdrSize = 1;
        }
        else
        {
            uint8_t shift   = static_cast<uint8_t>(8u - n);
            uint8_t lenbits = static_cast<uint8_t>(first >> shift);
            uint8_t rest    = first & static_cast<uint8_t>((1u << shift) - 1u);
            uint8_t allOnes = static_cast<uint8_t>((1u << shift) - 1u);

            if (rest == allOnes)
            {
                if (mOffset + 1u >= mTotalLen)
                {
                    mDone = true;
                    ExitNow(error = kErrorParse);
                }

                mType    = mBuffer[mOffset + 1u];
                mLen     = lenbits;
                mHdrSize = 2;
            }
            else
            {
                mType    = rest;
                mLen     = lenbits;
                mHdrSize = 1;
            }
        }

        if (static_cast<uint8_t>(mOffset + mHdrSize + mLen) > mTotalLen)
        {
            mDone = true;
            ExitNow(error = kErrorParse);
        }

        mValue = mBuffer + mOffset + mHdrSize;
    }

exit:
    return error;
}

} // namespace ot
