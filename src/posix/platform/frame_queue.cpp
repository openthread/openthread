/*
 *  Copyright (c) 2018, The OpenThread Authors.
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
 *   This file includes implementation of frame queue.
 */

#include "platform-posix.h"

#include "frame_queue.hpp"

#include <assert.h>
#include <string.h>

#include "code_utils.h"

namespace ot {

otError FrameQueue::Push(const uint8_t *aFrame, uint8_t aLength)
{
    otError  error   = OT_ERROR_NONE;
    uint16_t newTail = mTail + aLength + 1;

    assert(aFrame != NULL);
    otEXPECT_ACTION(aFrame != NULL, error = OT_ERROR_INVALID_ARGS);

    if (mHead > mTail)
    {
        otEXPECT_ACTION(newTail < mHead, error = OT_ERROR_NO_BUFS);
    }
    else if (newTail >= sizeof(mBuffer))
    {
        newTail -= sizeof(mBuffer);
        otEXPECT_ACTION(newTail < mHead, error = OT_ERROR_NO_BUFS);
    }

    mBuffer[mTail] = aLength;

    if (newTail > mTail)
    {
        memcpy(mBuffer + mTail + 1, aFrame, aLength);
    }
    else
    {
        uint16_t half = (sizeof(mBuffer) - mTail - 1);
        memcpy(mBuffer + mTail + 1, aFrame, half);
        memcpy(mBuffer, aFrame + half, (aLength - half));
    }

    mTail = newTail;

exit:
    return error;
}

void FrameQueue::Shift(void)
{
    if (mHead != mTail)
    {
        mHead += 1 + mBuffer[mHead];
        mHead %= sizeof(mBuffer);
    }
}

const uint8_t *FrameQueue::Peek(uint8_t *aFrame, uint8_t &aLength)
{
    const uint8_t *frame = NULL;
    uint16_t       next;

    otEXPECT(mHead != mTail);

    aLength = mBuffer[mHead];
    next    = mHead + 1 + aLength;

    if (next >= sizeof(mBuffer))
    {
        uint16_t half = sizeof(mBuffer) - mHead - 1;
        memcpy(aFrame, mBuffer + mHead + 1, half);
        memcpy(aFrame + half, mBuffer, aLength - half);
        frame = aFrame;
    }
    else
    {
        frame = mBuffer + mHead + 1;
    }

exit:
    return frame;
}

} // namespace ot

#if SELF_TEST
#include <stdlib.h>
void TestSingle()
{
    otError        error;
    ot::FrameQueue frameQueue;
    uint8_t        length;
    uint8_t        frame[255];

    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        frame[i] = i;
    }

    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        uint8_t        outFrame[255];
        const uint8_t *retFrame = NULL;
        error                   = frameQueue.Push(frame, i);
        assert(OT_ERROR_NONE == error);
        assert(!frameQueue.IsEmpty());
        retFrame = frameQueue.Peek(outFrame, length);
        assert(retFrame != NULL);
        assert(length == i);

        for (size_t j = 0; j < i; ++j)
        {
            assert(retFrame[j] == frame[j]);
        }

        frameQueue.Shift();
        assert(frameQueue.IsEmpty());
    }
}

void TestMultiple()
{
    otError        error;
    ot::FrameQueue frameQueue;
    uint8_t        length;
    uint8_t        frame[255];

    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        frame[i] = i;
    }

    srand(0);

    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        uint8_t        outFrame[255];
        const uint8_t *retFrame = NULL;

        int action = rand();
        if (action & 0x01) // push when odd
        {
            error = frameQueue.Push(frame, i);
            if (error == OT_ERROR_NO_BUFS)
                continue;

            assert(OT_ERROR_NONE == error);
            assert(!frameQueue.IsEmpty());
            retFrame = frameQueue.Peek(outFrame, length);
            assert(retFrame != NULL);

            for (size_t j = 0; j < length; ++j)
            {
                assert(retFrame[j] == frame[j]);
            }
        }
        else
        {
            frameQueue.Shift();
        }
    }
}

void TestRing()
{
    ot::FrameQueue frameQueue;
    uint8_t        length;
    uint8_t        frame[255];

    for (size_t i = 0; i < sizeof(frame); ++i)
    {
        frame[i] = i;
    }

    for (size_t i = 0; i < OPENTHREAD_CONFIG_FRAME_CACHE_SIZE + 255; i += sizeof(frame))
    {
        uint8_t        outFrame[255];
        const uint8_t *retFrame = NULL;

        frameQueue.Push(frame, sizeof(frame));

        retFrame = frameQueue.Peek(outFrame, length);

        for (size_t j = 0; j < sizeof(frame); ++j)
        {
            assert(retFrame[j] == frame[j]);
        }

        frameQueue.Shift();
    };
}

void RunAllTests()
{
    TestSingle();
    TestMultiple();
    TestRing();
}

int main(void)
{
    RunAllTests();
    printf("All tests passed\n");
    return 0;
}
#endif
