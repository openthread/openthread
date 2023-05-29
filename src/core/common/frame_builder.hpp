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
 *  This file defines OpenThread `FrameBuilder` class.
 */

#ifndef FRAME_BUILDER_HPP_
#define FRAME_BUILDER_HPP_

#include "openthread-core-config.h"

#include "common/error.hpp"
#include "common/type_traits.hpp"
#include "mac/mac_types.hpp"

namespace ot {
class Message;

/**
 * The `FrameBuilder` can be used to construct frame content in a given data buffer.
 *
 */
class FrameBuilder
{
public:
    /**
     * Initializes the `FrameBuilder` to use a given buffer.
     *
     * `FrameBuilder` MUST be initialized before its other methods are used.
     *
     * @param[in] aBuffer   A pointer to a buffer.
     * @param[in] aLength   The data length (number of bytes in @p aBuffer).
     *
     */
    void Init(void *aBuffer, uint16_t aLength);

    /**
     * Returns a pointer to the start of `FrameBuilder` buffer.
     *
     * @returns A pointer to the frame buffer.
     *
     */
    const uint8_t *GetBytes(void) const { return mBuffer; }

    /**
     * Returns the current length of frame (number of bytes appended so far).
     *
     * @returns The current frame length.
     *
     */
    uint16_t GetLength(void) const { return mLength; }

    /**
     * Returns the maximum length of the frame.
     *
     * @returns The maximum frame length (max number of bytes in the frame buffer).
     *
     */
    uint16_t GetMaxLength(void) const { return mMaxLength; }

    /**
     * Sets the maximum length of the frame.
     *
     * Does not perform any checks on the new given length. The caller MUST ensure that the specified max
     * length is valid for the frame buffer.
     *
     * @param[in] aLength  The maximum frame length.
     *
     */
    void SetMaxLength(uint16_t aLength) { mMaxLength = aLength; }

    /**
     * Returns the remaining length (number of bytes that can be appended) in the frame.
     *
     * @returns The remaining length.
     *
     */
    uint16_t GetRemainingLength(void) const { return mMaxLength - mLength; }

    /**
     * Indicates whether or not there are enough bytes remaining in the `FrameBuilder` buffer to append a
     * given number of bytes.
     *
     * @param[in] aLength   The append length.
     *
     * @retval TRUE   There are enough remaining bytes to append @p aLength bytes.
     * @retval FALSE  There are not enough remaining bytes to append @p aLength bytes.
     *
     */
    bool CanAppend(uint16_t aLength) const { return (static_cast<uint32_t>(mLength) + aLength) <= mMaxLength; }

    /**
     * Appends an `uint8_t` value to the `FrameBuilder`.
     *
     * @param[in] aUint8     The `uint8_t` value to append.
     *
     * @retval kErrorNone    Successfully appended the value.
     * @retval kErrorNoBufs  Insufficient available buffers.
     *
     */
    Error AppendUint8(uint8_t aUint8);

    /**
     * Appends an `uint16_t` value assuming big endian encoding to the `FrameBuilder`.
     *
     * @param[in] aUint16    The `uint16_t` value to append.
     *
     * @retval kErrorNone    Successfully appended the value.
     * @retval kErrorNoBufs  Insufficient available buffers.
     *
     */
    Error AppendBigEndianUint16(uint16_t aUint16);

    /**
     * Appends an `uint32_t` value assuming big endian encoding to the `FrameBuilder`.
     *
     * @param[in] aUint32    The `uint32_t` value to append.
     *
     * @retval kErrorNone    Successfully appended the value.
     * @retval kErrorNoBufs  Insufficient available buffers.
     *
     */
    Error AppendBigEndianUint32(uint32_t aUint32);

    /**
     * Appends an `uint16_t` value assuming little endian encoding to the `FrameBuilder`.
     *
     * @param[in] aUint16    The `uint16_t` value to append.
     *
     * @retval kErrorNone    Successfully appended the value.
     * @retval kErrorNoBufs  Insufficient available buffers.
     *
     */
    Error AppendLittleEndianUint16(uint16_t aUint16);

    /**
     * Appends an `uint32_t` value assuming little endian encoding to the `FrameBuilder`.
     *
     * @param[in] aUint32    The `uint32_t` value to append.
     *
     * @retval kErrorNone    Successfully appended the value.
     * @retval kErrorNoBufs  Insufficient available buffers.
     *
     */
    Error AppendLittleEndianUint32(uint32_t aUint32);

    /**
     * Appends bytes from a given buffer to the `FrameBuilder`.
     *
     * @param[in] aBuffer    A pointer to a data bytes to append.
     * @param[in] aLength    Number of bytes in @p aBuffer.
     *
     * @retval kErrorNone    Successfully appended the bytes.
     * @retval kErrorNoBufs  Insufficient available buffers.
     *
     */
    Error AppendBytes(const void *aBuffer, uint16_t aLength);

    /**
     * Appends a given `Mac::Address` to the `FrameBuilder`.
     *
     * @param[in] aMacAddress  A `Mac::Address` to append.
     *
     * @retval kErrorNone    Successfully appended the address.
     * @retval kErrorNoBufs  Insufficient available buffers.
     *
     */
    Error AppendMacAddress(const Mac::Address &aMacAddress);

#if OPENTHREAD_FTD || OPENTHREAD_MTD
    /**
     * Appends bytes read from a given message to the `FrameBuilder`.
     *
     * @param[in] aMessage   The message to read the bytes from.
     * @param[in] aOffset    The offset in @p aMessage to start reading the bytes from.
     * @param[in] aLength    Number of bytes to read from @p aMessage and append.
     *
     * @retval kErrorNone    Successfully appended the bytes.
     * @retval kErrorNoBufs  Insufficient available buffers to append the requested @p aLength bytes.
     * @retval kErrorParse   Not enough bytes in @p aMessage to read @p aLength bytes from @p aOffset.
     *
     */
    Error AppendBytesFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength);
#endif

    /**
     * Appends an object to the `FrameBuilder`.
     *
     * @tparam    ObjectType  The object type to append.
     *
     * @param[in] aObject     A reference to the object to append.
     *
     * @retval kErrorNone    Successfully appended the object.
     * @retval kErrorNoBufs  Insufficient available buffers to append @p aObject.
     *
     */
    template <typename ObjectType> Error Append(const ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        return AppendBytes(&aObject, sizeof(ObjectType));
    }

    /**
     * Writes bytes in `FrameBuilder` at a given offset overwriting the previously appended content.
     *
     * Does not perform any bound checks. The caller MUST ensure that the given data length fits within the
     * previously appended content. Otherwise the behavior of this method is undefined.
     *
     * @param[in] aOffset    The offset to begin writing.
     * @param[in] aBuffer    A pointer to a data buffer to write.
     * @param[in] aLength    Number of bytes in @p aBuffer.
     *
     */
    void WriteBytes(uint16_t aOffset, const void *aBuffer, uint16_t aLength);

    /**
     * Writes an object to the `FrameBuilder` at a given offset overwriting previously appended content.
     *
     * Does not perform any bound checks. The caller MUST ensure the given data length fits within the
     * previously appended content. Otherwise the behavior of this method is undefined.
     *
     * @tparam     ObjectType   The object type to write.
     *
     * @param[in]  aOffset      The offset to begin writing.
     * @param[in]  aObject      A reference to the object to write.
     *
     */
    template <typename ObjectType> void Write(uint16_t aOffset, const ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        WriteBytes(aOffset, &aObject, sizeof(ObjectType));
    }

    /**
     * Inserts bytes in `FrameBuilder` at a given offset, moving previous content forward.
     *
     * The caller MUST ensure that @p aOffset is within the current frame length (from 0 up to and including
     * `GetLength()`). Otherwise the behavior of this method is undefined.
     *
     * @param[in] aOffset   The offset to insert bytes.
     * @param[in] aBuffer   A pointer to a data buffer to insert.
     * @param[in] aLength   Number of bytes in @p aBuffer.
     *
     * @retval kErrorNone    Successfully inserted the bytes.
     * @retval kErrorNoBufs  Insufficient available buffers to insert the bytes.
     *
     */
    Error InsertBytes(uint16_t aOffset, const void *aBuffer, uint16_t aLength);

    /**
     * Inserts an object in `FrameBuilder` at a given offset, moving previous content forward.
     *
     * The caller MUST ensure that @p aOffset is within the current frame length (from 0 up to and including
     * `GetLength()`). Otherwise the behavior of this method is undefined.
     *
     * @tparam     ObjectType   The object type to insert.
     *
     * @param[in]  aOffset      The offset to insert bytes.
     * @param[in]  aObject      A reference to the object to insert.
     *
     * @retval kErrorNone       Successfully inserted the bytes.
     * @retval kErrorNoBufs     Insufficient available buffers to insert the bytes.
     *
     */
    template <typename ObjectType> Error Insert(uint16_t aOffset, const ObjectType &aObject)
    {
        static_assert(!TypeTraits::IsPointer<ObjectType>::kValue, "ObjectType must not be a pointer");

        return InsertBytes(aOffset, &aObject, sizeof(ObjectType));
    }

    /**
     * Removes a given number of bytes in `FrameBuilder` at a given offset, moving existing content
     * after removed bytes backward.
     *
     * Does not perform any bound checks. The caller MUST ensure that the given length and offset fits
     * within the previously appended content. Otherwise the behavior of this method is undefined.
     *
     * @param[in] aOffset   The offset to remove bytes from.
     * @param[in] aLength   The number of bytes to remove.
     *
     */
    void RemoveBytes(uint16_t aOffset, uint16_t aLength);

private:
    uint8_t *mBuffer;
    uint16_t mLength;
    uint16_t mMaxLength;
};

} // namespace ot

#endif // FRAME_BUILDER_HPP_
