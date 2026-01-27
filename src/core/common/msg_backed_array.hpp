/*
 *  Copyright (c) 2026, The OpenThread Authors.
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
 *   This file includes definitions for a generic array using `Message` for data storage.
 */

#ifndef OT_CORE_COMMON_MSG_BACKED_ARRAY_HPP_
#define OT_CORE_COMMON_MSG_BACKED_ARRAY_HPP_

#include "openthread-core-config.h"

#include "common/code_utils.hpp"
#include "common/error.hpp"
#include "common/locator.hpp"
#include "common/message.hpp"
#include "common/non_copyable.hpp"

namespace ot {

/**
 * Represents a dynamic array of generic type using `Message` for data storage.
 *
 * @tparam Type        The array element type.
 * @tparam kMaxSize    Specifies the max array size (maximum number of elements in the array).
 */
template <typename Type, uint16_t kMaxSize> class MessageBackedArray : public InstanceLocator, private NonCopyable
{
    static_assert(kMaxSize != 0, "MessageBackedArray `kMaxSize` cannot be zero");

public:
    /**
     * Initializes the array as empty.
     *
     * @param[in] aInstance  The OpenThread instance.
     */
    MessageBackedArray(Instance &aInstance)
        : InstanceLocator(aInstance)
        , mMessage(nullptr)
    {
    }

    /**
     * Destructor.
     */
    ~MessageBackedArray(void) { Clear(); }

    /**
     * Clears the array.
     *
     * This frees the underlying message used for storage.
     */
    void Clear(void)
    {
        if (mMessage != nullptr)
        {
            mMessage->Free();
            mMessage = nullptr;
        }
    }

    /**
     * Returns the current array length (number of elements in the array).
     *
     * @returns The array length.
     */
    uint16_t GetLength(void) const { return (mMessage == nullptr) ? 0 : mMessage->GetLength() / sizeof(Type); }

    /**
     * Reads an element from the array at a given index.
     *
     * @param[in]  aIndex  The index to read from.
     * @param[out] aEntry  A reference to a variable to output the read element.
     *
     * @retval kErrorNone      Successfully read the element.
     * @retval kErrorNotFound  The @p aIndex is out of bounds (beyond current array length).
     */
    Error ReadAt(uint16_t aIndex, Type &aEntry) const
    {
        Error error = kErrorNotFound;

        VerifyOrExit(mMessage != nullptr);
        SuccessOrExit(mMessage->Read(aIndex * sizeof(Type), aEntry));
        error = kErrorNone;

    exit:
        return error;
    }

    /**
     * Writes an element to the array at a given index.
     *
     * @param[in] aIndex  The index to write to.
     * @param[in] aEntry  The value to write.
     *
     * @retval kErrorNone         Successfully wrote the element.
     * @retval kErrorInvalidArgs  The @p aIndex is out of bounds (beyond current array length).
     */
    Error WriteAt(uint16_t aIndex, const Type &aEntry)
    {
        Error error = kErrorInvalidArgs;

        VerifyOrExit(aIndex < GetLength());
        mMessage->Write(aIndex * sizeof(Type), aEntry);
        error = kErrorNone;

    exit:
        return error;
    }

    /**
     * Appends a new entry to the end of the array.
     *
     * @param[in] aEntry     The new entry to push back.
     *
     * @retval kErrorNone    Successfully pushed back @p aEntry to the end of the array.
     * @retval kErrorNoBufs  Could not allocate buffer to grow the array or array reached its max size.
     */
    Error Push(const Type &aEntry)
    {
        Error    error = kErrorNoBufs;
        uint16_t msgLen;

        VerifyOrExit(GetLength() < kMaxSize);

        if (mMessage == nullptr)
        {
            mMessage = Get<MessagePool>().Allocate(Message::kTypeOther);
            VerifyOrExit(mMessage != nullptr);
        }

        msgLen = mMessage->GetLength();

        if (mMessage->Append(aEntry) != kErrorNone)
        {
            IgnoreError(mMessage->SetLength(msgLen));
            ExitNow();
        }

        error = kErrorNone;

    exit:
        return error;
    }

    /**
     * Represents the function pointer check callback used in `Iterate()`.
     *
     * The callback should return `true` to continue iteration, or `false` to stop.
     *
     * @param[in] aIndex    The index of the element.
     * @param[in] aEntry    The element at @p aIndex.
     * @param[in] aContext  A context pointer passed to `Iterate()`.
     *
     * @retval TRUE   To continue iteration.
     * @retval FALSE  To stop iteration.
     */
    typedef bool (*EntryChecker)(uint16_t aIndex, const Type &aEntry, void *aContext);

    /**
     * Iterates over the array elements and invokes an `EntryChecker` callback for each element.
     *
     * @param[in] aChecker   The `EntryChecker` callback to invoke for each element.
     * @param[in] aContext   A context pointer to pass to @p aChecker callback.
     */
    void Iterate(EntryChecker aChecker, void *aContext) const
    {
        bool shouldContinue = true;
        Type entry;

        for (uint16_t index = 0; shouldContinue; index++)
        {
            SuccessOrExit(ReadAt(index, entry));
            shouldContinue = aChecker(index, entry, aContext);
        }

    exit:
        return;
    }

private:
    Message *mMessage;
};

} // namespace ot

#endif // OT_CORE_COMMON_MSG_BACKED_ARRAY_HPP_
