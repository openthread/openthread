/*
 *    Copyright (c) 2016, Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 *    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements an FLEN encoder and decoder.
 */

#include <common/code_utils.hpp>
#include <ncp/flen.hpp>

namespace Thread {
namespace Flen {

enum
{
    kFlagSequence   = 0x7e,  ///< FLen Flag value
};

ThreadError Encoder::Init(uint8_t *aOutBuf, uint16_t &aOutLength)
{
    ThreadError error = kThreadError_None;

    mOutBuf = aOutBuf;

    VerifyOrExit(aOutLength >= 3, error = kThreadError_NoBufs);
    aOutBuf[0] = kFlagSequence;
    // Leave the next two bytes empty.
    aOutLength = 3;
    mOutLength = 0;

exit:
    return error;
}

ThreadError Encoder::Encode(uint8_t aInByte, uint8_t *aOutBuf, uint16_t aOutLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mOutOffset + 1 < aOutLength, error = kThreadError_NoBufs);
    aOutBuf[mOutOffset++] = aInByte;

exit:
    return error;
}

ThreadError Encoder::Encode(const uint8_t *aInBuf, uint16_t aInLength, uint8_t *aOutBuf, uint16_t &aOutLength)
{
    ThreadError error = kThreadError_None;

    mOutOffset = 0;

    for (int i = 0; i < aInLength; i++)
    {
        SuccessOrExit(error = Encode(aInBuf[i], aOutBuf, aOutLength));
    }

exit:
    aOutLength = mOutOffset;
    mOutLength += aOutLength;
    return error;
}

ThreadError Encoder::Finalize(uint8_t *aOutBuf, uint16_t &aOutLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(mOutOffset < aOutLength, error = kThreadError_NoBufs);

    mOutBuf[1] = (mOutLength >> 8);
    mOutBuf[2] = (mOutLength & 0xFF);

    aOutLength = 0;

exit:
    return error;
}


Decoder::Decoder(uint8_t *aOutBuf, uint16_t aOutLength, FrameHandler aFrameHandler, void *aContext)
{
    mState = kStateNeedFlag;
    mFrameHandler = aFrameHandler;
    mContext = aContext;
    mOutBuf = aOutBuf;
    mOutOffset = 0;
    mOutLength = aOutLength;
}

void Decoder::Decode(const uint8_t *aInBuf, uint16_t aInLength)
{
    uint8_t byte;

    for (int i = 0; i < aInLength; i++)
    {
        byte = aInBuf[i];

        switch (mState)
        {
        case kStateNeedFlag:
            if (byte == kFlagSequence)
            {
                mState = kStateNeedLenH;
                mOutOffset = 0;
            }

            break;

        case kStateNeedLenH:
            mReadLength = (byte << 8);
            mState = kStateNeedLenL;
            break;

        case kStateNeedLenL:
            mReadLength += byte;

            if (mReadLength > mOutLength)
            {
                // Too big.
                mState = kStateNeedFlag;
            }
            else
            {
                mState = kStateNeedData;
            }

            break;

        case kStateNeedData:
            mOutBuf[mOutOffset++] = byte;

            if (mOutOffset >= mReadLength)
            {
                mState = kStateNeedFlag;
                mFrameHandler(mContext, mOutBuf, mReadLength);
            }

            break;
        }
    }
}

}  // namespace Flen
}  // namespace Thread
