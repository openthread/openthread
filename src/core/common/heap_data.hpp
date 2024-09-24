/*
 *  Copyright (c) 2021, The OpenThread Authors.
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
 *   This file includes definitions for `Heap::Data` (heap allocated data).
 */

#ifndef HEAP_DATA_HPP_
#define HEAP_DATA_HPP_

#include "openthread-core-config.h"

#include "common/data.hpp"
#include "common/heap.hpp"
#include "common/message.hpp"

namespace ot {
namespace Heap {

/**
 * Represents a heap allocated data.
 */
class Data
{
public:
    /**
     * Initializes the `Heap::Data` as empty.
     */
    Data(void) { mData.Init(nullptr, 0); }

    /**
     * This is the move constructor for `Heap::Data`.
     *
     * `Heap::Data` is non-copyable (copy constructor is deleted) but move constructor is provided to allow it to to be
     * used as return type (return by value) from functions/methods (which will then use move semantics).
     *
     * @param[in] aData   An rvalue reference to another `Heap::Data` to move from.
     */
    Data(Data &&aData) { TakeFrom(aData); }

    /**
     * This is the destructor for `Heap::Data` object.
     */
    ~Data(void) { Free(); }

    /**
     * Indicates whether or not the `Heap::Data` is null (i.e., it was never successfully set or it was
     * freed).
     *
     * @retval TRUE  The `Heap::Data` is null.
     * @retval FALSE The `Heap::Data` is not null.
     */
    bool IsNull(void) const { return (mData.GetBytes() == nullptr); }

    /**
     * Returns a pointer to the `Heap::Data` bytes buffer.
     *
     * @returns A pointer to data buffer or `nullptr` if the `Heap::Data` is null (never set or freed).
     */
    const uint8_t *GetBytes(void) const { return mData.GetBytes(); }

    /**
     * Returns the `Heap::Data` length.
     *
     * @returns The data length (number of bytes) or zero if the `HeapData` is null.
     */
    uint16_t GetLength(void) const { return mData.GetLength(); }

    /**
     * Sets the `Heap::Data` from the content of a given buffer.
     *
     * @param[in] aBuffer     The buffer to copy bytes from.
     * @param[in] aLength     The buffer length (number of bytes).
     *
     * @retval kErrorNone     Successfully set the `Heap::Data`.
     * @retval kErrorNoBufs   Failed to allocate buffer.
     */
    Error SetFrom(const uint8_t *aBuffer, uint16_t aLength);

    /**
     * Sets the `Heap::Data` from the content of a given message.
     *
     * The bytes are copied from current offset in @p aMessage till the end of the message.
     *
     * @param[in] aMessage    The message to copy bytes from (starting from offset till the end of message).
     *
     * @retval kErrorNone     Successfully set the `Heap::Data`.
     * @retval kErrorNoBufs   Failed to allocate buffer.
     */
    Error SetFrom(const Message &aMessage);

    /**
     * Sets the `Heap::Data` from the content of a given message read at a given offset up to a given
     * length.
     *
     * @param[in] aMessage    The message to read and copy bytes from.
     * @param[in] aOffset     The offset into the message to start reading the bytes from.
     * @param[in] aLength     The number of bytes to read from message. If @p aMessage contains fewer bytes than
     *                        requested, then `Heap::Data` is cleared and this method returns `kErrorParse`.
     *
     * @retval kErrorNone     Successfully set the `Heap::Data`.
     * @retval kErrorNoBufs   Failed to allocate buffer.
     * @retval kErrorParse    Not enough bytes in @p aMessage to read the requested @p aLength bytes.
     */
    Error SetFrom(const Message &aMessage, uint16_t aOffset, uint16_t aLength);

    /**
     * Sets the `Heap::Data` from another one (move semantics).
     *
     * @param[in] aData   The other `Heap::Data` to set from (rvalue reference).
     */
    void SetFrom(Data &&aData);

    /**
     * Appends the bytes from `Heap::Data` to a given message.
     *
     * @param[in] aMessage   The message to append the bytes into.
     *
     * @retval kErrorNone     Successfully copied the bytes from `Heap::Data` into @p aMessage.
     * @retval kErrorNoBufs   Failed to allocate buffer.
     */
    Error CopyBytesTo(Message &aMessage) const { return aMessage.AppendBytes(mData.GetBytes(), mData.GetLength()); }

    /**
     * Copies the bytes from `Heap::Data` into a given buffer.
     *
     * It is up to the caller to ensure that @p aBuffer has enough space for the current data length.
     *
     * @param[in] aBuffer     A pointer to buffer to copy the bytes into.
     */
    void CopyBytesTo(uint8_t *aBuffer) const { return mData.CopyBytesTo(aBuffer); }

    /**
     * Compares the `Data` content with the bytes from a given buffer.
     *
     * @param[in] aBuffer   A pointer to a buffer to compare with the data.
     * @param[in] aLength   The length of @p aBuffer.
     *
     * @retval TRUE   The `Data` content matches the bytes in @p aBuffer.
     * @retval FALSE  The `Data` content does not match the byes in @p aBuffer.
     */
    bool Matches(const uint8_t *aBuffer, uint16_t aLength) const;

    /**
     * Overloads operator `==` to compare the `Data` content with the content from another one.
     *
     * @param[in] aOtherData   The other `Data` to compare with.
     *
     * @retval TRUE   The two `Data` instances have matching content (same length and same bytes).
     * @retval FALSE  The two `Data` instances do not have matching content.
     */
    bool operator==(const Data &aOtherData) const { return mData == aOtherData.mData; }

    /**
     * Frees any buffer allocated by the `Heap::Data`.
     *
     * The `Heap::Data` destructor will automatically call `Free()`. This method allows caller to free the buffer
     * explicitly.
     */
    void Free(void);

    Data(const Data &)            = delete;
    Data &operator=(const Data &) = delete;

private:
    Error UpdateBuffer(uint16_t aNewLength);
    void  TakeFrom(Data &aData);

    MutableData<kWithUint16Length> mData;
};

} // namespace Heap
} // namespace ot

#endif // HEAP_DATA_HPP_
