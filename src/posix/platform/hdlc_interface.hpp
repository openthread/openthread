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
 *   This file includes definitions for the HDLC interface to radio (RCP).
 */

#ifndef POSIX_APP_HDLC_INTERFACE_HPP_
#define POSIX_APP_HDLC_INTERFACE_HPP_

#include "openthread-posix-config.h"
#include "platform-posix.h"
#include "spinel_interface.hpp"
#include "ncp/hdlc.hpp"

#if OPENTHREAD_POSIX_RCP_UART_ENABLE

namespace ot {
namespace PosixApp {

/**
 * This class defines an HDLC interface to the Radio Co-processor (RCP)
 *
 */
class HdlcInterface
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in] aCallback     A reference to a `Callback` object.
     * @param[in] aFrameBuffer  A reference to a `RxFrameBuffer` object.
     *
     */
    HdlcInterface(SpinelInterface::Callbacks &aCallback, SpinelInterface::RxFrameBuffer &aFrameBuffer);

    /**
     * This destructor deinitializes the object.
     *
     */
    ~HdlcInterface(void);

    /**
     * This method initializes the interface to the Radio Co-processor (RCP)
     *
     * @note This method should be called before reading and sending spinel frames to the interface.
     *
     * @param[in]  aPlatformConfig  Platform configuration structure.
     *
     * @retval OT_ERROR_NONE          The interface is initialized successfully
     * @retval OT_ERROR_ALREADY       The interface is already initialized.
     * @retval OT_ERROR_INVALID_ARGS  The UART device or executable cannot be found or failed to open/run.
     *
     */
    otError Init(const otPlatformConfig &aPlatformConfig);

    /**
     * This method deinitializes the interface to the RCP.
     *
     */
    void Deinit(void);

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
    otError SendFrame(const uint8_t *aFrame, uint16_t aLength);

    /**
     * This method waits for receiving part or all of spinel frame within specified interval.
     *
     * @param[in]  aTimeout  A reference to the timeout.
     *
     * @retval OT_ERROR_NONE             Part or all of spinel frame is received.
     * @retval OT_ERROR_RESPONSE_TIMEOUT No spinel frame is received within @p aTimeout.
     *
     */
    otError WaitForFrame(const struct timeval &aTimeout);

    /**
     * This method updates the file descriptor sets with file descriptors used by the radio driver.
     *
     * @param[inout]  aReadFdSet   A reference to the read file descriptors.
     * @param[inout]  aWriteFdSet  A reference to the write file descriptors.
     * @param[inout]  aMaxFd       A reference to the max file descriptor.
     * @param[inout]  aTimeout     A reference to the timeout.
     *
     */
    void UpdateFdSet(fd_set &aReadFdSet, fd_set &aWriteFdSet, int &aMaxFd, struct timeval &aTimeout);

    /**
     * This method performs radio driver processing.
     *
     * @param[in]   aReadFdSet      A reference to the read file descriptors.
     * @param[in]   aWriteFdSet     A reference to the write file descriptors.
     *
     */
    void Process(const fd_set &aReadFdSet, const fd_set &aWriteFdSet);

#if OPENTHREAD_POSIX_VIRTUAL_TIME
    /**
     * This method process read data (decode the data).
     *
     * This method is intended only for virtual time simulation. Its behavior is similar to `Read()` but instead of
     * reading the data from the radio socket, it uses the given data in the buffer `aBuffer`.
     *
     * @param[in] aBuffer  A pointer to buffer containing data.
     * @param[in] aLength  The length (number of bytes) in the buffer.
     *
     */
    void ProcessReadData(const uint8_t *aBuffer, uint16_t aLength) { Decode(aBuffer, aLength); }
#endif

private:
    /**
     * This method instructs `HdlcInterface` to read and decode data from radio over the socket.
     *
     * If a full HDLC frame is decoded while reading data, this method invokes the `HandleReceivedFrame()` (on the
     * `aCallback` object from constructor) to pass the received frame to be processed.
     *
     */
    void Read(void);

    /**
     * This method waits for the socket file descriptor associated with the HDLC interface to become writable within
     * `kMaxWaitTime` interval.
     *
     * @retval OT_ERROR_NONE   Socket is writable.
     * @retval OT_ERROR_FAILED Socket did not become writable within `kMaxWaitTime`.
     *
     */
    otError WaitForWritable(void);

    /**
     * This method writes a given frame to the socket.
     *
     * This is blocking call, i.e., if the socket is not writable, this method waits for it to become writable for
     * up to `kMaxWaitTime` interval.
     *
     * @param[in] aFrame  A pointer to buffer containing the frame to write.
     * @param[in] aLength The length (number of bytes) in the frame.
     *
     * @retval OT_ERROR_NONE    Frame was written successfully.
     * @retval OT_ERROR_FAILED  Failed to write due to socket not becoming writable within `kMaxWaitTime`.
     *
     */
    otError Write(const uint8_t *aFrame, uint16_t aLength);

    /**
     * This method performs HDLC decoding on received data.
     *
     * If a full HDLC frame is decoded while reading data, this method invokes the `HandleReceivedFrame()` (on the
     * `aCallback` object from constructor) to pass the received frame to be processed.
     *
     * @param[in] aBuffer  A pointer to buffer containing data.
     * @param[in] aLength  The length (number of bytes) in the buffer.
     *
     */
    void Decode(const uint8_t *aBuffer, uint16_t aLength);

    static void HandleHdlcFrame(void *aContext, otError aError);
    void        HandleHdlcFrame(otError aError);

    static int OpenFile(const char *aFile, const char *aConfig);
#if OPENTHREAD_CONFIG_POSIX_APP_ENABLE_PTY_DEVICE
    static int ForkPty(const char *aCommand, const char *aArguments);
#endif

    enum
    {
        kMaxFrameSize = SpinelInterface::kMaxFrameSize,
        kMaxWaitTime  = 2000, ///< Maximum wait time in Milliseconds for socket to become writable (see `SendFrame`).
    };

    SpinelInterface::Callbacks &    mCallbacks;
    SpinelInterface::RxFrameBuffer &mRxFrameBuffer;

    int           mSockFd;
    Hdlc::Decoder mHdlcDecoder;
};

} // namespace PosixApp
} // namespace ot

#endif // OPENTHREAD_POSIX_RCP_UART_ENABLE
#endif // POSIX_APP_HDLC_INTERFACE_HPP_
