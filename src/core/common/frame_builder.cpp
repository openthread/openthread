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
 *   This file implements the `FrameBuilder` class.
 */

#include "frame_builder.hpp"

namespace ot {

void FrameBuilder::Init(void *aBuffer, uint16_t aLength)
{
    mBuffer    = static_cast<uint8_t *>(aBuffer);
    mLength    = 0;
    mMaxLength = aLength;
}

Error FrameBuilder::AppendUint8(uint8_t aUint8) { return Append<uint8_t>(aUint8); }

Error FrameBuilder::AppendBigEndianUint16(uint16_t aUint16)
{
    return Append<uint16_t>(Encoding::BigEndian::HostSwap16(aUint16));
}

Error FrameBuilder::AppendBigEndianUint32(uint32_t aUint32)
{
    return Append<uint32_t>(Encoding::BigEndian::HostSwap32(aUint32));
}

Error FrameBuilder::AppendLittleEndianUint16(uint16_t aUint16)
{
    return Append<uint16_t>(Encoding::LittleEndian::HostSwap16(aUint16));
}

Error FrameBuilder::AppendLittleEndianUint32(uint32_t aUint32)
{
    return Append<uint32_t>(Encoding::LittleEndian::HostSwap32(aUint32));
}

Error FrameBuilder::AppendBytes(const void *aBuffer, uint16_t aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(CanAppend(aLength), error = kErrorNoBufs);
    memcpy(mBuffer + mLength, aBuffer, aLength);
    mLength += aLength;

exit:
    return error;
}

Error FrameBuilder::AppendBytesFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(CanAppend(aLength), error = kErrorNoBufs);
    SuccessOrExit(error = aMessage.Read(aOffset, mBuffer + mLength, aLength));
    mLength += aLength;

exit:
    return error;
}

void FrameBuilder::WriteBytes(uint16_t aOffset, const void *aBuffer, uint16_t aLength)
{
    memcpy(mBuffer + aOffset, aBuffer, aLength);
}

Error FrameBuilder::InsertBytes(uint16_t aOffset, const void *aBuffer, uint16_t aLength)
{
    Error error = kErrorNone;

    OT_ASSERT(aOffset <= mLength);

    VerifyOrExit(CanAppend(aLength), error = kErrorNoBufs);

    memmove(mBuffer + aOffset + aLength, mBuffer + aOffset, mLength - aOffset);
    memcpy(mBuffer + aOffset, aBuffer, aLength);
    mLength += aLength;

exit:
    return error;
}

void FrameBuilder::RemoveBytes(uint16_t aOffset, uint16_t aLength)
{
    memmove(mBuffer + aOffset, mBuffer + aOffset + aLength, mLength - aOffset - aLength);
    mLength -= aLength;
}

} // namespace ot
