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
 *   This file implements the `HeapData` (a heap allocated data).
 */

#include "heap_data.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"

namespace ot {

Error HeapData::SetFrom(const uint8_t *aBuffer, uint16_t aLength)
{
    Error error;

    SuccessOrExit(error = UpdateBuffer(aLength));
    VerifyOrExit(aLength != 0);

    SuccessOrAssert(mData.CopyBytesFrom(aBuffer, aLength));

exit:
    return error;
}

Error HeapData::SetFrom(const Message &aMessage)
{
    Error    error;
    uint16_t length = aMessage.GetLength() - aMessage.GetOffset();

    SuccessOrExit(error = UpdateBuffer(length));
    VerifyOrExit(length != 0);

    SuccessOrAssert(aMessage.Read(aMessage.GetOffset(), mData.GetBytes(), mData.GetLength()));

exit:
    return error;
}

void HeapData::SetFrom(HeapData &&aHeapData)
{
    Free();
    TakeFrom(aHeapData);
}

void HeapData::Free(void)
{
    Instance::HeapFree(mData.GetBytes());
    mData.Init(nullptr, 0);
}

Error HeapData::UpdateBuffer(uint16_t aNewLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aNewLength != mData.GetLength());

    Instance::HeapFree(mData.GetBytes());

    if (aNewLength == 0)
    {
        mData.Init(nullptr, 0);
    }
    else
    {
        uint8_t *newBuffer = static_cast<uint8_t *>(Instance::HeapCAlloc(aNewLength, sizeof(uint8_t)));

        VerifyOrExit(newBuffer != nullptr, error = kErrorNoBufs);
        mData.Init(newBuffer, aNewLength);
    }

exit:
    return error;
}

void HeapData::TakeFrom(HeapData &aHeapData)
{
    mData.Init(aHeapData.mData.GetBytes(), aHeapData.GetLength());
    aHeapData.mData.Init(nullptr, 0);
}

} // namespace ot
