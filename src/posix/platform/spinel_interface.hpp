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

#include "ncp/hdlc.hpp"

namespace ot {
namespace PosixApp {

class SpinelInterface
{
public:
    enum
    {
        kMaxWaitTime  = 2000, ///< Maximum wait time in Milliseconds for socket to become writable (see `SendFrame`).
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
        virtual void HandleReceivedFrame(void) = 0;
    };

    /**
     * This constructor initializes the object.
     *
     */
    SpinelInterface()
        : mRxFrameBuffer()
    {
    }

    /**
     * This method gets the `RxFrameBuffer`.
     *
     * The receive frame buffer is an `Hdlc::MultiFrameBuffer` and therefore it is capable of storing multiple
     * spinel frames in a FIFO queue manner. The `RxFrameBuffer` contains the received spinel frames.
     *
     * When during `Process()` or `WaitForFrame()` the `Callbacks::HandleReceivedFrame()` is invoked, the newly
     * received spinel frame is available in the receive frame buffer. The callback is expected to either process
     * and then discard the frame (using `RxFrameBuffer::DiscardFrame()` method) or save the frame
     * (using `RxFrameBuffer::SaveFrame()` so that it can be read later.
     *
     * @returns A reference to receive frame buffer containing newly received spinel frame or previously saved spinel
     *          frames.
     *
     */
    RxFrameBuffer &GetRxFrameBuffer(void) { return mRxFrameBuffer; }

    /**
     * This method initializes the interface to the Radio Co-processor (RCP)
     *
     * @note This method should be called before reading and sending spinel frames to the interface.
     *
     * @param[in]   aConfig  Parameters to be given to the device or executable.
     *
     * @retval OT_ERROR_NONE          The interface is initialized successfully
     * @retval OT_ERROR_ALREADY       The interface is already initialized.
     * @retval OT_ERROR_INVALID_ARGS  The device or executable cannot be found or failed to open/run.
     *
     */
    virtual otError Init(const char *aRadioFile, const char *aRadioConfig) = 0;

    /**
     * This method deinitializes the interface to the Radio Co-processor (RCP).
     *
     */
    virtual void Deinit(void) = 0;

    /**
     * This method encodes and sends a spinel frame to Radio Co-processor (RCP) over the socket.
     *
     * This is blocking call, i.e., if the socket is not writable, this method waits for it to become writable for
     * up to `kMaxWaitTime` interval.
     *
     * @param[in] aFrame     A pointer to buffer containing the spinel frame to send.
     * @param[in] aLength    The length (number of bytes) in the frame.
     *
     * @retval OT_ERROR_NONE     Successfully encoded and sent the spinel frame.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to encode the frame.
     * @retval OT_ERROR_FAILED   Failed to send due to socket not becoming writable within `kMaxWaitTime`.
     *
     */
    virtual otError SendFrame(const uint8_t *aFrame, uint16_t aLength) = 0;

    /**
     * This method waits for receiving part or all of spinel frame within specified interval.
     *
     * @param[in]  aTimeout  A reference to the timeout.
     *
     * @retval OT_ERROR_NONE             Part or all of spinel frame is received.
     * @retval OT_ERROR_RESPONSE_TIMEOUT No spinel frame is received within @p aTimeout.
     *
     */
    virtual otError WaitForFrame(struct timeval &aTimeout) = 0;

    /**
     * This method updates the file descriptor sets with file descriptors used by the radio driver.
     *
     * @param[inout]  aReadFdSet   A reference to the read file descriptors.
     * @param[inout]  aWriteFdSet  A reference to the write file descriptors.
     * @param[inout]  aMaxFd       A reference to the max file descriptor.
     * @param[inout]  aTimeout     A reference to the timeout.
     *
     */
    virtual void UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout) = 0;

    /**
     * This method performs radio driver processing.
     *
     * @param[in]   aReadFdSet      A reference to the read file descriptors.
     * @param[in]   aWriteFdSet     A reference to the write file descriptors.
     *
     */
    virtual void Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet) = 0;

private:
    RxFrameBuffer mRxFrameBuffer;
};

} // namespace PosixApp
} // namespace ot

#endif // POSIX_APP_SPINEL_INTERFACE_HPP_
