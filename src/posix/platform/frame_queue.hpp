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
 *   This file includes definitions for frame queue.
 */

#ifndef OT_FRAME_QUEUE_HPP_
#define OT_FRAME_QUEUE_HPP_

#include <stdint.h>

/**
 * @def OPENTHREAD_CONFIG_FRAME_QUEUE_SIZE
 *
 * The size of a frame queue in bytes.
 *
 */
#ifndef OPENTHREAD_CONFIG_FRAME_QUEUE_SIZE
#define OPENTHREAD_CONFIG_FRAME_QUEUE_SIZE 4096
#endif

namespace ot {

class FrameQueue
{
public:
    /**
     * This constructor initializes a frame queue based on ring buffer.
     *
     */
    FrameQueue(void)
        : mHead(0)
        , mTail(0)
    {
    }

    /**
     * This method checks if the cache is empty.
     *
     * @retval true     No frames are queued.
     * @retval false    At least one frame is queued.
     *
     */
    bool IsEmpty(void) const { return mHead == mTail; }

    /**
     * This method pushes one frame into the queue.
     *
     * @param[in]   aFrame      A pointer to a spinel frame to be queued.
     * @param[in]   aLength     Frame length in bytes.
     *
     * @retval OT_ERROR_NONE    Successfully queued this frame.
     * @retval OT_ERROR_NO_BUFS Insufficient memory for this frame.
     *
     */
    otError Push(const uint8_t *aFrame, uint8_t aLength);

    /**
     * This method shifts one frame at head.
     *
     * @note aFrame is only used when necessary, always use the returned pointer to access frame data.
     *
     * @param[out]  aFrame      A pointer to the frame to receive the data.
     * @param[out]  aLength     A reference to receive the frame length.
     *
     * @return A pointer to the frame.
     *
     */
    const uint8_t *Shift(uint8_t *aFrame, uint8_t &aLength);

private:
    enum
    {
        kQueueSize = OPENTHREAD_CONFIG_FRAME_QUEUE_SIZE,
    };

    uint8_t  mBuffer[kQueueSize];
    uint16_t mHead;
    uint16_t mTail;
};

} // namespace ot

#endif // OT_FRAME_CACHE_HPP_
