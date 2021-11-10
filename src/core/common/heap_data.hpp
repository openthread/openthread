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
 *   This file includes definitions for `HeapData` (heap allocated data).
 */

#ifndef HEAP_DATA_HPP_
#define HEAP_DATA_HPP_

#include "openthread-core-config.h"

#include "common/data.hpp"
#include "common/message.hpp"

namespace ot {

/**
 * This class represents a heap allocated data.
 *
 */
class HeapData
{
public:
    /**
     * This constructor initializes the `HeapData` as empty.
     *
     */
    HeapData(void) { mData.Init(nullptr, 0); }

    /**
     * This is the move constructor for `HeapData`.
     *
     * `HeapData` is non-copyable (copy constructor is deleted) but move constructor is provided to allow it to to be
     * used as return type (return by value) from functions/methods (which will then use move semantics).
     *
     * @param[in] aHeapData   An rvalue reference to another `HeapData` to move from.
     *
     */
    HeapData(HeapData &&aHeapData) { TakeFrom(aHeapData); }

    /**
     * This is the destructor for `HeapData` object.
     *
     */
    ~HeapData(void) { Free(); }

    /**
     * This method indicates whether or not the `HeapData` is null (i.e., it was never successfully set or it was
     * freed).
     *
     * @retval TRUE  The `HeapData` is null.
     * @retval FALSE The `HeapData` is not null.
     *
     */
    bool IsNull(void) const { return (mData.GetBytes() == nullptr); }

    /**
     * This method returns a pointer to the `HeapData` bytes buffer.
     *
     * @returns A pointer to data buffer or `nullptr` if the `HeapData` is null (never set or freed).
     *
     */
    const uint8_t *GetBytes(void) const { return mData.GetBytes(); }

    /**
     * This method returns the `HeapData` length.
     *
     * @returns The data length (number of bytes) or zero if the `HeadpData` is null.
     *
     */
    uint16_t GetLength(void) const { return mData.GetLength(); }

    /**
     * This method sets the `HeapData` from the content of a given buffer.
     *
     * @param[in] aBuffer     The buffer to copy bytes from.
     * @param[in] aLength     The buffer length (number of bytes).
     *
     * @retval kErrorNone     Successfully set the `HeapData`.
     * @retval kErrorNoBufs   Failed to allocate buffer.
     *
     */
    Error SetFrom(const uint8_t *aBuffer, uint16_t aLength);

    /**
     * This method sets the `HeapData` from the content of a given message.
     *
     * The bytes are copied from current offset in @p aMessage till the end of the message.
     *
     * @param[in] aMessage    The message to copy bytes from (starting from offset till the end of message).
     * @param[in] aLength     The buffer length (number of bytes).
     *
     * @retval kErrorNone     Successfully set the `HeapData`.
     * @retval kErrorNoBufs   Failed to allocate buffer.
     *
     */
    Error SetFrom(const Message &aMessage);

    /**
     * This method sets the `HeapData` from another one (move semantics).
     *
     * @param[in] aHeapData   The other `HeapData` to set from (rvalue reference).
     *
     */
    void SetFrom(HeapData &&aHeapData);

    /**
     * This method appends the bytes from `HeapData` to a given message.
     *
     * @param[in] aMessage   The message to append the bytes into.
     *
     * @retval kErrorNone     Successfully copied the bytes from `HeapData` into @p aMessage.
     * @retval kErrorNoBufs   Failed to allocate buffer.
     *
     */
    Error CopyBytesTo(Message &aMessage) const { return aMessage.AppendBytes(mData.GetBytes(), mData.GetLength()); }

    /**
     * This method copies the bytes from `HeapData` into a given buffer.
     *
     * It is up to the caller to ensure that @p aBuffer has enough space for the current data length.
     *
     * @param[in] aBuffer     A pointer to buffer to copy the bytes into.
     *
     */
    void CopyBytesTo(uint8_t *aBuffer) const { return mData.CopyBytesTo(aBuffer); }

    /**
     * This method frees any buffer allocated by the `HeapData`.
     *
     * The `HeapData` destructor will automatically call `Free()`. This method allows caller to free the buffer
     * explicitly.
     *
     */
    void Free(void);

    HeapData(const HeapData &) = delete;
    HeapData &operator=(const HeapData &) = delete;

private:
    Error UpdateBuffer(uint16_t aNewLength);
    void  TakeFrom(HeapData &aHeapData);

    MutableData<kWithUint16Length> mData;
};

} // namespace ot

#endif // HEAP_DATA_HPP_
