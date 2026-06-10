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
 *   This file implements the `Appender` class.
 */

#include "appender.hpp"

namespace ot {

Appender::Appender(Message &aMessage)
    : mType(kMessage)
{
    mShared.mMessage.mMessage     = &aMessage;
    mShared.mMessage.mStartOffset = aMessage.GetLength();
}

Appender::Appender(uint8_t *aBuffer, uint16_t aSize)
    : mType(kBuffer)
{
    mShared.mFrameBuilder.Init(aBuffer, aSize);
}

Error Appender::AppendBytes(const void *aBuffer, uint16_t aLength)
{
    Error error = kErrorNone;

    switch (mType)
    {
    case kMessage:
        error = mShared.mMessage.mMessage->AppendBytes(aBuffer, aLength);
        break;

    case kBuffer:
        error = mShared.mFrameBuilder.AppendBytes(aBuffer, aLength);
        break;
    }

    return error;
}

uint16_t Appender::GetAppendedLength(void) const
{
    uint16_t length = 0;

    switch (mType)
    {
    case kMessage:
        length = mShared.mMessage.mMessage->GetLength() - mShared.mMessage.mStartOffset;
        break;

    case kBuffer:
        length = mShared.mFrameBuilder.GetLength();
        break;
    }

    return length;
}

void Appender::GetAsData(Data<kWithUint16Length> &aData) const
{
    aData.Init(mShared.mFrameBuilder.GetBytes(), mShared.mFrameBuilder.GetLength());
}

} // namespace ot
