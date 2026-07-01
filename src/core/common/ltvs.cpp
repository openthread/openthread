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

// Minimum number of bits needed to represent [0, aL-1]: smallest n with 2^n >= aL.
static uint8_t BitsFor(uint8_t aL)
{
    uint8_t n = 0;

    while ((1u << n) < aL)
    {
        n++;
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
        const Ltv &ltv = iter.GetLtv();

        // Advance first: validates the declared length fits in the buffer.
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
    // Packed entries are encoded back-to-front because the header bit-split for each entry
    // depends on L = (own encoded size) + (all subsequent entries' encoded size).  Collecting
    // entries first and then iterating in reverse lets us track `scratchLen` as we go.

    static constexpr uint8_t kMaxEntries = 8;

    struct Entry
    {
        uint8_t        mType;
        uint8_t        mLen;
        const uint8_t *mValue;
    };

    Entry   entries[kMaxEntries];
    uint8_t count = 0;

    for (const uint8_t *p = aPlain, *end = aPlain + aPlainLen; p + 2 <= end && count < kMaxEntries; p += 2 + p[0])
    {
        entries[count].mLen   = p[0];
        entries[count].mType  = p[1];
        entries[count].mValue = p + 2;
        count++;
    }

    uint8_t scratch[128]; // packed output never exceeds plain input; 128 covers max IE payload (127 bytes).
    uint8_t scratchLen = 0;

    for (int i = static_cast<int>(count) - 1; i >= 0; i--)
    {
        const Entry &e = entries[i];
        uint8_t      header[2];
        uint8_t      hdrBytes = 0;

        for (uint8_t hdr = 1; hdr <= 2; hdr++)
        {
            uint8_t L     = static_cast<uint8_t>(hdr + e.mLen + scratchLen);
            uint8_t n     = BitsFor(L);
            uint8_t shift = static_cast<uint8_t>(8u - n);

            if (n == 0)
            {
                // L == 1: the sole byte is a bare type with implicit zero length.
                if (hdr == 1 && e.mLen == 0)
                {
                    header[0] = e.mType;
                    hdrBytes  = 1;
                    break;
                }

                continue;
            }

            if (e.mLen >= (1u << n))
            {
                continue;
            }

            uint8_t allOnes = static_cast<uint8_t>((1u << shift) - 1u);

            if (hdr == 1 && e.mType < allOnes)
            {
                header[0] = static_cast<uint8_t>((e.mLen << shift) | e.mType);
                hdrBytes  = 1;
                break;
            }

            if (hdr == 2)
            {
                // All-ones in the type field is the escape code; type follows in the next byte.
                header[0] = static_cast<uint8_t>((e.mLen << shift) | allOnes);
                header[1] = e.mType;
                hdrBytes  = 2;
                break;
            }
        }

        if (hdrBytes == 0)
        {
            continue;
        }

        uint8_t entryLen = static_cast<uint8_t>(hdrBytes + e.mLen);

        memmove(scratch + entryLen, scratch, scratchLen);
        memcpy(scratch, header, hdrBytes);

        if (e.mLen > 0)
        {
            memcpy(scratch + hdrBytes, e.mValue, e.mLen);
        }

        scratchLen += entryLen;
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
    Error    error  = kErrorNone;
    uint8_t  outLen = 0;
    Iterator iter;

    for (iter.Init(aPacked, aPackedLen); !iter.IsDone();)
    {
        uint8_t len = iter.GetLength();

        VerifyOrExit(outLen + Ltv::kHeaderSize + len <= aPlainMax, error = kErrorNoBufs);
        aPlain[outLen++] = len;
        aPlain[outLen++] = iter.GetType();

        if (len > 0)
        {
            memcpy(aPlain + outLen, iter.GetValue(), len);
            outLen += len;
        }

        VerifyOrExit(iter.Advance() == kErrorNone, error = kErrorParse);
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
    Error   error = kErrorNone;
    uint8_t L;
    uint8_t n;
    uint8_t first;

    if (mOffset >= mTotalLen)
    {
        mDone = true;
        ExitNow();
    }

    L     = static_cast<uint8_t>(mTotalLen - mOffset);
    n     = BitsFor(L);
    first = mBuffer[mOffset];

    if (n == 0)
    {
        mType    = first;
        mLen     = 0;
        mHdrSize = 1;
    }
    else
    {
        uint8_t shift    = static_cast<uint8_t>(8u - n);
        uint8_t lenbits  = static_cast<uint8_t>(first >> shift);
        uint8_t typebits = static_cast<uint8_t>(first & ((1u << shift) - 1u));

        if (typebits == static_cast<uint8_t>((1u << shift) - 1u))
        {
            VerifyOrExit(mOffset + 1u < mTotalLen, error = kErrorParse; mDone = true);
            mType    = mBuffer[mOffset + 1u];
            mLen     = lenbits;
            mHdrSize = 2;
        }
        else
        {
            mType    = typebits;
            mLen     = lenbits;
            mHdrSize = 1;
        }
    }

    VerifyOrExit(mOffset + mHdrSize + mLen <= mTotalLen, error = kErrorParse; mDone = true);
    mValue = mBuffer + mOffset + mHdrSize;

exit:
    return error;
}

} // namespace ot
