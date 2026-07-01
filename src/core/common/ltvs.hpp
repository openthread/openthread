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
 *   This file includes definitions for generating and parsing LTV (Length-Type-Value) encoded data.
 */

#ifndef OT_CORE_COMMON_LTVS_HPP_
#define OT_CORE_COMMON_LTVS_HPP_

#include "openthread-core-config.h"

#include <stdint.h>

#include <openthread/platform/toolchain.h>

#include "common/error.hpp"
#include "common/frame_builder.hpp"
#include "common/frame_data.hpp"
#include "common/type_traits.hpp"

namespace ot {

/**
 * Implements LTV generation and parsing.
 *
 * The wire format is `[Length][Type][Value...]` where Length is the byte count of the
 * Value field only (not including Type).
 */
OT_TOOL_PACKED_BEGIN
class Ltv
{
public:
    static constexpr uint8_t kHeaderSize = 2; ///< Wire size of the LTV header (Length + Type bytes).

    uint8_t GetLength(void) const { return mLength; }
    void    SetLength(uint8_t aLength) { mLength = aLength; }

    uint8_t GetType(void) const { return mType; }
    void    SetType(uint8_t aType) { mType = aType; }

    uint16_t GetTotalSize(void) const { return kHeaderSize + mLength; }

    uint8_t       *GetValue(void) { return reinterpret_cast<uint8_t *>(this) + kHeaderSize; }
    const uint8_t *GetValue(void) const { return reinterpret_cast<const uint8_t *>(this) + kHeaderSize; }

    /**
     * Returns the Value reinterpreted as @p T.
     *
     * The caller MUST verify `sizeof(T) <= GetLength()` before calling.
     */
    template <typename T> const T &GetValueAs(void) const { return *reinterpret_cast<const T *>(GetValue()); }

    const Ltv *GetNext(void) const
    {
        return reinterpret_cast<const Ltv *>(reinterpret_cast<const uint8_t *>(this) + GetTotalSize());
    }

    /**
     * Appends this LTV to @p aFrameBuilder.
     *
     * @retval kErrorNone    Successfully appended.
     * @retval kErrorNoBufs  Insufficient buffer space.
     */
    Error AppendTo(FrameBuilder &aFrameBuilder) const;

    /**
     * Appends an LTV with @p aType and @p aLength value bytes to @p aFrameBuilder.
     *
     * @param[in] aValue  Pointer to the value bytes; may be `nullptr` when @p aLength is zero.
     *
     * @retval kErrorNone    Successfully appended.
     * @retval kErrorNoBufs  Insufficient buffer space.
     */
    static Error Append(FrameBuilder &aFrameBuilder, uint8_t aType, const void *aValue, uint8_t aLength);

    /**
     * Appends a fixed-size LTV described by @p SimpleLtvType to @p aFrameBuilder.
     *
     * @tparam SimpleLtvType  A `SimpleLtvInfo<kType, ValueType>` specialization.
     *
     * @retval kErrorNone    Successfully appended.
     * @retval kErrorNoBufs  Insufficient buffer space.
     */
    template <typename SimpleLtvType>
    static Error Append(FrameBuilder &aFrameBuilder, const typename SimpleLtvType::ValueType &aValue)
    {
        static_assert(!TypeTraits::IsPointer<typename SimpleLtvType::ValueType>::kValue,
                      "SimpleLtvType::ValueType must not be a pointer");

        return Append(aFrameBuilder, SimpleLtvType::kType, &aValue, sizeof(typename SimpleLtvType::ValueType));
    }

    /**
     * Searches @p aBuffer for the first LTV with @p aType.
     *
     * Returns `nullptr` on a truncated or missing entry.
     */
    static const Ltv *FindLtv(const void *aBuffer, uint16_t aLength, uint8_t aType);

    /**
     * Iterates forward through a plain LTV sequence.
     */
    class Iterator
    {
    public:
        void Init(const uint8_t *aBuffer, uint16_t aLength);

        bool IsDone(void) const { return mData.GetLength() < kHeaderSize; }

        const Ltv &GetLtv(void) const { return *reinterpret_cast<const Ltv *>(mData.GetBytes()); }

        /**
         * Advances to the next LTV.
         *
         * @retval kErrorNone   Advanced (or already done).
         * @retval kErrorParse  Buffer is truncated.
         */
        Error Advance(void);

    private:
        FrameData mData;
    };

private:
    uint8_t mLength;
    uint8_t mType;
} OT_TOOL_PACKED_END;

/**
 * Provides the compile-time type code for an LTV type.
 *
 * @tparam kLtvTypeValue  The LTV Type byte value.
 */
template <uint8_t kLtvTypeValue> class LtvInfo
{
public:
    static constexpr uint8_t kType = kLtvTypeValue;
};

/**
 * Pairs a compile-time type code with a fixed-size value struct for use with `Ltv::Append<>`.
 *
 * @tparam kLtvTypeValue  The LTV Type byte value.
 * @tparam TValueType     The C++ type of the Value field.
 */
template <uint8_t kLtvTypeValue, typename TValueType> class SimpleLtvInfo : public LtvInfo<kLtvTypeValue>
{
public:
    typedef TValueType ValueType;
};

/**
 * Encodes and decodes a context-sensitive packed LTV stream.
 *
 * In the plain `Ltv` format every entry has a fixed 2-byte `[L][T]` header.  The packed format
 * shrinks that overhead by using `BitsFor(L)` bits for the length field and the remaining bits
 * of the first byte for the type, where L is the total remaining byte count from the current
 * entry to the end.  When the type value would collide with the all-ones escape code, a second
 * type byte follows.  The encoded output is never larger than the plain input.
 */
class PackedLtvStream
{
public:
    /**
     * Encodes a plain `[L][T][V...]` buffer into the packed format.
     *
     * @param[in]  aPlain      Plain LTV bytes.
     * @param[in]  aPlainLen   Byte count of @p aPlain.
     * @param[out] aPacked     Output buffer; capacity must be at least @p aPlainLen bytes.
     * @param[in]  aPackedMax  Capacity of @p aPacked.
     *
     * @returns Number of bytes written to @p aPacked.
     */
    static uint8_t Encode(const uint8_t *aPlain, uint8_t aPlainLen, uint8_t *aPacked, uint8_t aPackedMax);

    /**
     * Decodes a packed stream back to plain `[L][T][V...]` format.
     *
     * @param[in]  aPacked     Packed bytes.
     * @param[in]  aPackedLen  Byte count of @p aPacked.
     * @param[out] aPlain      Output buffer for plain LTV bytes.
     * @param[in]  aPlainMax   Capacity of @p aPlain.
     * @param[out] aPlainLen   Number of plain bytes written on success.
     *
     * @retval kErrorNone   Decoded successfully.
     * @retval kErrorParse  Stream is malformed.
     * @retval kErrorNoBufs @p aPlain is too small.
     */
    static Error Decode(const uint8_t *aPacked,
                        uint8_t        aPackedLen,
                        uint8_t       *aPlain,
                        uint8_t        aPlainMax,
                        uint8_t       &aPlainLen);

    /**
     * Iterates forward over a packed LTV stream without allocating a plain buffer.
     */
    class Iterator
    {
    public:
        void Init(const uint8_t *aPacked, uint8_t aPackedLen);

        bool IsDone(void) const { return mDone; }

        uint8_t GetType(void) const { return mType; }
        uint8_t GetLength(void) const { return mLen; }

        const uint8_t *GetValue(void) const { return mValue; }

        /** Returns the byte offset of the current entry's value within the stream buffer. */
        uint8_t GetValueOffset(void) const { return static_cast<uint8_t>(mOffset + mHdrSize); }

        /**
         * Advances to the next LTV.
         *
         * @retval kErrorNone   Advanced (or already at end).
         * @retval kErrorParse  Stream is malformed.
         */
        Error Advance(void);

    private:
        Error DecodeEntry(void);

        const uint8_t *mBuffer;
        uint8_t        mTotalLen;
        uint8_t        mOffset;
        uint8_t        mType;
        uint8_t        mLen;
        const uint8_t *mValue;
        uint8_t        mHdrSize;
        bool           mDone;
    };
};

} // namespace ot

#endif // OT_CORE_COMMON_LTVS_HPP_
