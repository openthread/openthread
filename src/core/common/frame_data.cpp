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

template <Encoding kEncoding, typename UintType> Error FrameData::ReadUint(UintType &aUint)
{
    Error error;

    static_assert(TypeTraits::IsUint<UintType>::kValue, "UintType is not valid, it must be an unsigned int");

    SuccessOrExit(error = ReadBytes(&aUint, sizeof(UintType)));
    aUint = HostSwap<kEncoding>(aUint);

exit:
    return error;
}

template Error FrameData::ReadUint<kBigEndian, uint16_t>(uint16_t &aUint);
template Error FrameData::ReadUint<kBigEndian, uint32_t>(uint32_t &aUint);
template Error FrameData::ReadUint<kBigEndian, uint64_t>(uint64_t &aUint);
template Error FrameData::ReadUint<kLittleEndian, uint16_t>(uint16_t &aUint);
template Error FrameData::ReadUint<kLittleEndian, uint32_t>(uint32_t &aUint);
template Error FrameData::ReadUint<kLittleEndian, uint64_t>(uint64_t &aUint);

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
