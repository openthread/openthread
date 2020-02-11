/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file includes definitions for the spinel interface to Radio Co-processor (RCP)
 *
 */

#ifndef POSIX_APP_SPINEL_INTERFACE_HPP_
#define POSIX_APP_SPINEL_INTERFACE_HPP_

#include "openthread-posix-config.h"

#include "ncp/hdlc.hpp"

namespace ot {
namespace PosixApp {

class SpinelInterface
{
public:
    enum
    {
        kMaxFrameSize = 2048, ///< Maximum frame size (number of bytes).
    };

    /**
     * This type defines a receive frame buffer to store received spinel frame(s).
     *
     * @note The receive frame buffer is an `Hdlc::MultiFrameBuffer` and therefore it is capable of storing multiple
     * frames in a FIFO queue manner.
     *
     */
    typedef Hdlc::MultiFrameBuffer<kMaxFrameSize> RxFrameBuffer;

    /**
     * This class defines the callbacks provided by `SpinelInterface` to its owner/user.
     *
     */
    class Callbacks
    {
    public:
        /**
         * This callback is invoked to notify owner/user of `SpinelInterface` of a received spinel frame.
         *
         * The newly received frame is available in `RxFrameBuffer` from `SpinelInterface::GetRxFrameBuffer()`.
         * User can read and process the frame. The callback is expected to either discard the new frame using
         * `RxFrameBuffer::DiscardFrame()` or save the frame using `RxFrameBuffer::SaveFrame()` to be read and
         * processed later.
         *
         */
        void HandleReceivedFrame(void);
    };
};
} // namespace PosixApp
} // namespace ot

#endif // POSIX_APP_SPINEL_INTERFACE_HPP_
