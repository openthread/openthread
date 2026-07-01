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
 * Implements LTV (Length-Type-Value) generation and parsing.
 *
 * The wire format is a two-byte header followed by zero or more value bytes:
 *
 *   +---------+---------+-------------------+
 *   | Length  |  Type   |  Value (0+ bytes) |
 *   +---------+---------+-------------------+
 *   | 1 byte  | 1 byte  |   Length bytes    |
 *   +---------+---------+-------------------+
 *
 * Length is the byte count of the Value only; it does not include the Type byte.
 * An LTV with Length=0 carries no Value bytes.
 *
 * The backing store for LTV sequences is raw byte buffers accessed via `FrameBuilder`
 * (write) and `FrameData` (read), not `Message` objects.
 */
OT_TOOL_PACKED_BEGIN
class Ltv
{
public:
    static constexpr uint8_t kHeaderSize = 2; ///< Wire size of the LTV header (Length byte + Type byte).

    /**
     * Returns the Length field value.
     *
     * The Length is the byte count of the Value only; it does not include the Type byte.
     *
     * @returns The Length field value.
     */
    uint8_t GetLength(void) const { return mLength; }

    /**
     * Sets the Length field value.
     *
     * @param[in] aLength  The Length value (byte count of the Value, not including the Type byte).
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

    /**
     * Returns the Type field value.
     *
     * @returns The Type field value.
     */
    uint8_t GetType(void) const { return mType; }

    /**
     * Sets the Type field value.
     *
     * @param[in] aType  The Type value.
     */
    void SetType(uint8_t aType) { mType = aType; }

    /**
     * Returns the total wire size of this LTV in bytes, including the header and the Value.
     *
     * @returns `kHeaderSize` + `GetLength()`.
     */
    uint16_t GetTotalSize(void) const { return kHeaderSize + mLength; }

    /**
     * Returns a pointer to the Value bytes.
     *
     * @returns A pointer to the first Value byte.
     */
    uint8_t *GetValue(void) { return reinterpret_cast<uint8_t *>(this) + kHeaderSize; }

    /**
     * Returns a pointer to the Value bytes.
     *
     * @returns A pointer to the first Value byte.
     */
    const uint8_t *GetValue(void) const { return reinterpret_cast<const uint8_t *>(this) + kHeaderSize; }

    /**
     * Returns a const reference to the Value bytes interpreted as type @p T.
     *
     * The caller MUST verify `sizeof(T) <= GetLength()` before calling.
     *
     * @tparam T  The type to reinterpret the Value bytes as.
     *
     * @returns A const reference to the Value bytes cast to @p T.
     */
    template <typename T> const T &GetValueAs(void) const { return *reinterpret_cast<const T *>(GetValue()); }

    /**
     * Returns a pointer to the next LTV in the sequence.
     *
     * The caller MUST ensure the next LTV lies within a valid buffer.
     *
     * @returns A pointer to the next LTV.
     */
    const Ltv *GetNext(void) const
    {
        return reinterpret_cast<const Ltv *>(reinterpret_cast<const uint8_t *>(this) + GetTotalSize());
    }

    /**
     * Appends this LTV (header and Value) to a `FrameBuilder`.
     *
     * @param[in] aFrameBuilder  The `FrameBuilder` to append to.
     *
     * @retval kErrorNone    Successfully appended.
     * @retval kErrorNoBufs  Insufficient buffer space.
     */
    Error AppendTo(FrameBuilder &aFrameBuilder) const;

    /**
     * Appends an LTV with a given type and value bytes to a `FrameBuilder`.
     *
     * @param[in] aFrameBuilder  The `FrameBuilder` to append to.
     * @param[in] aType          The LTV Type value.
     * @param[in] aValue         A pointer to the Value bytes. May be `nullptr` when @p aLength is zero.
     * @param[in] aLength        The byte count of @p aValue.
     *
     * @retval kErrorNone    Successfully appended.
     * @retval kErrorNoBufs  Insufficient buffer space.
     */
    static Error Append(FrameBuilder &aFrameBuilder, uint8_t aType, const void *aValue, uint8_t aLength);

    /**
     * Appends a fixed-size LTV using a `SimpleLtvInfo` type parameter.
     *
     * @tparam SimpleLtvType  A `SimpleLtvInfo<kType, ValueType>` specialization providing `kType` and `ValueType`.
     *
     * @param[in] aFrameBuilder  The `FrameBuilder` to append to.
     * @param[in] aValue         A const reference to the value to encode.
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
     * Searches a raw byte buffer for the first LTV with a given type.
     *
     * Stops and returns `nullptr` on a malformed (truncated) buffer.
     *
     * @param[in] aBuffer  Pointer to the start of the LTV sequence.
     * @param[in] aLength  Byte count of the buffer.
     * @param[in] aType    The LTV Type to search for.
     *
     * @returns A pointer to the matching `Ltv` within @p aBuffer, or `nullptr` if not found.
     */
    static const Ltv *FindLtv(const void *aBuffer, uint16_t aLength, uint8_t aType);

    /**
     * Iterates forward through an LTV sequence in a raw byte buffer.
     *
     * Usage:
     * @code
     *   Ltv::Iterator iter;
     *   iter.Init(buffer, length);
     *   while (!iter.IsDone())
     *   {
     *       const Ltv &ltv = iter.GetLtv();
     *       // process ltv.GetType() and ltv.GetValue()
     *       IgnoreError(iter.Advance());
     *   }
     * @endcode
     */
    class Iterator
    {
    public:
        /**
         * Initializes the iterator over a raw byte buffer.
         *
         * @param[in] aBuffer  Pointer to the start of the LTV sequence.
         * @param[in] aLength  Byte count of the buffer.
         */
        void Init(const uint8_t *aBuffer, uint16_t aLength);

        /**
         * Indicates whether there are no more LTVs to read.
         *
         * Returns `TRUE` when the remaining buffer is too small to contain an LTV header.
         *
         * @retval TRUE   No more LTVs remain.
         * @retval FALSE  At least one more LTV is available via `GetLtv()`.
         */
        bool IsDone(void) const { return mData.GetLength() < kHeaderSize; }

        /**
         * Returns the current LTV.
         *
         * MUST NOT be called when `IsDone()` returns `TRUE`.
         *
         * @returns A const reference to the current LTV.
         */
        const Ltv &GetLtv(void) const { return *reinterpret_cast<const Ltv *>(mData.GetBytes()); }

        /**
         * Advances the iterator to the next LTV.
         *
         * When `IsDone()` is `TRUE`, this is a no-op that returns `kErrorNone`.
         *
         * @retval kErrorNone   Successfully advanced (or already done).
         * @retval kErrorParse  Buffer is truncated: remaining bytes are fewer than the current LTV declares.
         */
        Error Advance(void);

    private:
        FrameData mData;
    };

private:
    uint8_t mLength; ///< Byte count of the Value (does not include the Type byte).
    uint8_t mType;
} OT_TOOL_PACKED_END;

/**
 * Provides compile-time type information for an LTV type.
 *
 * @tparam kLtvTypeValue  The LTV Type byte value.
 */
template <uint8_t kLtvTypeValue> class LtvInfo
{
public:
    static constexpr uint8_t kType = kLtvTypeValue; ///< The LTV Type value.
};

/**
 * Provides compile-time type information for a fixed-size LTV.
 *
 * Intended for use with `Ltv::Append<SimpleLtvType>()`.
 *
 * @tparam kLtvTypeValue  The LTV Type byte value.
 * @tparam TValueType     The C++ type of the LTV's Value field.
 */
template <uint8_t kLtvTypeValue, typename TValueType> class SimpleLtvInfo : public LtvInfo<kLtvTypeValue>
{
public:
    typedef TValueType ValueType; ///< The C++ type of the Value field.
};

/**
 * Manages a context-sensitive packed LTV encoding where each entry's header width (1 or 2 bytes)
 * is determined by the total remaining byte count from that entry to the end of the stream.
 *
 * In the plain `Ltv` format, every entry has a fixed 2-byte `[L][T]` header regardless of value
 * size.  This encoding compresses the header by allocating `ceil(log2(L))` bits to the length
 * field and the remainder of the first byte to the type field, where L is the remaining byte
 * count (own entry plus all subsequent entries).  When the type value would collide with the
 * all-ones escape code, a second "type byte" follows the first header byte.  The output is
 * always equal in size or smaller than the plain form.
 *
 * Typical workflow:
 *
 *   Write side:
 *     Assemble LTVs into a scratch buffer with `Ltv::Append`.
 *     Encode to the compact wire format with `PackedLtvStream::Encode`.
 *
 *   Read side (full decode):
 *     Call `PackedLtvStream::Decode` to recover the plain `[L][T][V...]` buffer,
 *     then walk it with `Ltv::Iterator`.
 *
 *   Read side (single-type scan, no scratch buffer):
 *     Use `PackedLtvStream::Iterator` to scan the packed stream in-place,
 *     which avoids allocating a plain buffer.
 */
class PackedLtvStream
{
public:
    /**
     * Encodes a plain LTV buffer ([L][T][V...] sequences) into the packed format.
     *
     * @param[in]  aPlain      Pointer to the plain LTV bytes.
     * @param[in]  aPlainLen   Byte count of @p aPlain.
     * @param[out] aPacked     Output buffer; must be at least @p aPlainLen bytes.
     * @param[in]  aPackedMax  Capacity of @p aPacked.
     *
     * @returns  Number of bytes written to @p aPacked.
     */
    static uint8_t Encode(const uint8_t *aPlain, uint8_t aPlainLen, uint8_t *aPacked, uint8_t aPackedMax);

    /**
     * Decodes a packed LTV stream back to plain [L][T][V...] format.
     *
     * @param[in]  aPacked     Pointer to the packed bytes.
     * @param[in]  aPackedLen  Byte count of @p aPacked.
     * @param[out] aPlain      Output buffer for plain LTV bytes.
     * @param[in]  aPlainMax   Capacity of @p aPlain.
     * @param[out] aPlainLen   Number of plain bytes written on success.
     *
     * @retval kErrorNone   Decoded successfully.
     * @retval kErrorParse  Stream is malformed (truncated entry).
     * @retval kErrorNoBufs @p aPlain buffer is too small.
     */
    static Error Decode(const uint8_t *aPacked,
                        uint8_t        aPackedLen,
                        uint8_t       *aPlain,
                        uint8_t        aPlainMax,
                        uint8_t       &aPlainLen);

    /**
     * Forward-scans a packed LTV stream without allocating a plain buffer.
     *
     * Preferred for single-type lookups in ISR context or wherever allocating
     * a scratch buffer for `Decode` is undesirable.
     *
     * Usage:
     * @code
     *   PackedLtvStream::Iterator iter;
     *   iter.Init(payload, len);
     *   while (!iter.IsDone())
     *   {
     *       if (iter.GetType() == kMyType && iter.GetLength() >= sizeof(MyValue)) { ... }
     *       IgnoreError(iter.Advance());
     *   }
     * @endcode
     */
    class Iterator
    {
    public:
        /**
         * Initializes the iterator.
         *
         * @param[in] aPacked     Pointer to the packed LTV stream.
         * @param[in] aPackedLen  Total byte count of the stream.
         */
        void Init(const uint8_t *aPacked, uint8_t aPackedLen);

        /**
         * Indicates whether there are no more LTVs.
         *
         * @retval TRUE   End of stream reached, or stream is malformed.
         * @retval FALSE  At least one LTV is available.
         */
        bool IsDone(void) const { return mDone; }

        /**
         * Returns the type of the current LTV.
         */
        uint8_t GetType(void) const { return mType; }

        /**
         * Returns the value length of the current LTV in bytes.
         */
        uint8_t GetLength(void) const { return mLen; }

        /**
         * Returns a pointer to the first value byte of the current LTV.
         *
         * Valid while `IsDone()` is `false` and the stream buffer is unmodified.
         */
        const uint8_t *GetValue(void) const { return mValue; }

        /**
         * Returns the byte offset of the current LTV's value within the stream buffer.
         *
         * Useful for in-place writes into the original (writable) frame buffer without
         * a separate pointer to the value.
         */
        uint8_t GetValueOffset(void) const { return static_cast<uint8_t>(mOffset + mHdrSize); }

        /**
         * Advances to the next LTV.
         *
         * When `IsDone()` is `true`, this is a no-op.
         *
         * @retval kErrorNone   Advanced, or already at end of stream.
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
