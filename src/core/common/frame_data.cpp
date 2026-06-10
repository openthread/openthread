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
 *   This file includes implementation of `FrameData`.
 */

#include "frame_data.hpp"

#include "common/code_utils.hpp"
#include "common/encoding.hpp"

namespace ot {

Error FrameData::ReadUint8(uint8_t &aUint8) { return ReadBytes(&aUint8, sizeof(uint8_t)); }

Error FrameData::ReadBigEndianUint16(uint16_t &aUint16)
{
    Error error;

    SuccessOrExit(error = ReadBytes(&aUint16, sizeof(uint16_t)));
    aUint16 = BigEndian::HostSwap16(aUint16);

exit:
    return error;
}

Error FrameData::ReadBigEndianUint32(uint32_t &aUint32)
{
    Error error;

    SuccessOrExit(error = ReadBytes(&aUint32, sizeof(uint32_t)));
    aUint32 = BigEndian::HostSwap32(aUint32);

exit:
    return error;
}

Error FrameData::ReadLittleEndianUint16(uint16_t &aUint16)
{
    Error error;

    SuccessOrExit(error = ReadBytes(&aUint16, sizeof(uint16_t)));
    aUint16 = LittleEndian::HostSwap16(aUint16);

exit:
    return error;
}

Error FrameData::ReadLittleEndianUint32(uint32_t &aUint32)
{
    Error error;

    SuccessOrExit(error = ReadBytes(&aUint32, sizeof(uint32_t)));
    aUint32 = LittleEndian::HostSwap32(aUint32);

exit:
    return error;
}

Error FrameData::ReadBytes(void *aBuffer, uint16_t aLength)
{
    Error error = kErrorNone;

    VerifyOrExit(CanRead(aLength), error = kErrorParse);
    memcpy(aBuffer, GetBytes(), aLength);
    SkipOver(aLength);

exit:
    return error;
}

void FrameData::SkipOver(uint16_t aLength) { Init(GetBytes() + aLength, GetLength() - aLength); }

} // namespace ot
