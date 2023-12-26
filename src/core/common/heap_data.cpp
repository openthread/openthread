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
 *   This file implements the `Heap::Data` (a heap allocated data).
 */

#include "heap_data.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"

namespace ot {
namespace Heap {

Error Data::SetFrom(const uint8_t *aBuffer, uint16_t aLength)
{
    Error error;

    SuccessOrExit(error = UpdateBuffer(aLength));
    VerifyOrExit(aLength != 0);

    SuccessOrAssert(mData.CopyBytesFrom(aBuffer, aLength));

exit:
    return error;
}

Error Data::SetFrom(const Message &aMessage)
{
    return SetFrom(aMessage, aMessage.GetOffset(), aMessage.GetLength() - aMessage.GetOffset());
}

Error Data::SetFrom(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Error error;

    VerifyOrExit(aOffset + aLength <= aMessage.GetLength(), error = kErrorParse);

    SuccessOrExit(error = UpdateBuffer(aLength));
    VerifyOrExit(aLength != 0);

    SuccessOrAssert(aMessage.Read(aOffset, mData.GetBytes(), aLength));

exit:
    return error;
}

void Data::SetFrom(Data &&aData)
{
    Free();
    TakeFrom(aData);
}

bool Data::Matches(const uint8_t *aBuffer, uint16_t aLength) const
{
    bool matches = false;

    VerifyOrExit(aLength == mData.GetLength());

    if (IsNull())
    {
        matches = (aLength == 0);
    }
    else
    {
        matches = mData.MatchesBytesIn(aBuffer);
    }

exit:
    return matches;
}

void Data::Free(void)
{
    Heap::Free(mData.GetBytes());
    mData.Init(nullptr, 0);
}

Error Data::UpdateBuffer(uint16_t aNewLength)
{
    Error error = kErrorNone;

    VerifyOrExit(aNewLength != mData.GetLength());

    Heap::Free(mData.GetBytes());

    if (aNewLength == 0)
    {
        mData.Init(nullptr, 0);
    }
    else
    {
        uint8_t *newBuffer = static_cast<uint8_t *>(Heap::CAlloc(aNewLength, sizeof(uint8_t)));

        VerifyOrExit(newBuffer != nullptr, error = kErrorNoBufs);
        mData.Init(newBuffer, aNewLength);
    }

exit:
    return error;
}

void Data::TakeFrom(Data &aData)
{
    mData.Init(aData.mData.GetBytes(), aData.GetLength());
    aData.mData.Init(nullptr, 0);
}

} // namespace Heap
} // namespace ot
