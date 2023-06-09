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

#ifndef SPINEL_SPINEL_INTERFACE_HPP_
#define SPINEL_SPINEL_INTERFACE_HPP_

#include "lib/spinel/multi_frame_buffer.hpp"
#include "lib/spinel/spinel.h"
#include "lib/url/url.hpp"

namespace ot {
namespace Spinel {

class SpinelInterface
{
public:
    enum
    {
        kMaxFrameSize = OPENTHREAD_CONFIG_PLATFORM_RADIO_SPINEL_RX_FRAME_BUFFER_SIZE, ///< Maximum buffer size.
    };

    /**
     * Defines a receive frame buffer to store received spinel frame(s).
     *
     * @note The receive frame buffer is an `Spinel::MultiFrameBuffer` and therefore it is capable of storing multiple
     * frames in a FIFO queue manner.
     *
     */
    typedef MultiFrameBuffer<kMaxFrameSize> RxFrameBuffer;

    typedef void (*ReceiveFrameCallback)(void *aContext);

    /**
     * Initializes the interface to the Radio Co-processor (RCP)
     *
     * @note This method should be called before reading and sending spinel frames to the interface.
     *
     * @param[in]  aRadioUrl          RadioUrl parsed from radio url.
     *
     * @retval OT_ERROR_NONE          The interface is initialized successfully
     * @retval OT_ERROR_ALREADY       The interface is already initialized.
     * @retval OT_ERROR_INVALID_ARGS  The UART device or executable cannot be found or failed to open/run.
     *
     */
    virtual otError Init(const Url::Url &aRadioUrl) = 0;

    /**
     * Deinitializes the interface to the RCP.
     *
     */
    virtual void Deinit(void) = 0;

    /**
     * Encodes and sends a spinel frame to Radio Co-processor (RCP) over the socket.
     *
     * @param[in] aFrame     A pointer to buffer containing the spinel frame to send.
     * @param[in] aLength    The length (number of bytes) in the frame.
     *
     * @retval OT_ERROR_NONE     Successfully encoded and sent the spinel frame.
     * @retval OT_ERROR_BUSY     Failed due to another operation is on going.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to encode the frame.
     * @retval OT_ERROR_FAILED   Failed to call the SPI driver to send the frame.
     *
     */
    virtual otError SendFrame(const uint8_t *aFrame, uint16_t aLength) = 0;

    /**
     * Waits for receiving part or all of spinel frame within specified interval.
     *
     * @param[in]  aTimeout  The timeout value in microseconds.
     *
     * @retval OT_ERROR_NONE             Part or all of spinel frame is received.
     * @retval OT_ERROR_RESPONSE_TIMEOUT No spinel frame is received within @p aTimeout.
     *
     */
    virtual otError WaitForFrame(uint64_t aTimeoutUs) = 0;

    /**
     * Updates the file descriptor sets with file descriptors used by the radio driver.
     *
     * @param[in,out]   aMainloopContext  A pointer to the mainloop context.
     *
     */
    virtual void UpdateFdSet(void *aMainloopContext) = 0;

    /**
     * Performs radio driver processing.
     *
     * @param[in]   aMainloopContext  A pointer to the mainloop context.
     *
     */
    virtual void Process(const void *aMainloopContext) = 0;

    /**
     * Returns the bus speed between the host and the radio.
     *
     * @returns   Bus speed in bits/second.
     *
     */
    virtual uint32_t GetBusSpeed(void) const = 0;

    /**
     * Hardware resets the RCP.
     *
     * @retval OT_ERROR_NONE            Successfully reset the RCP.
     * @retval OT_ERROR_NOT_IMPLEMENT   The hardware reset is not implemented.
     *
     */
    virtual otError HardwareReset(void) = 0;

    /**
     * Marks destructor virtual method.
     *
     */
    virtual ~SpinelInterface() = default;

protected:
    /**
     * Indicates whether or not the frame is the Spinel SPINEL_CMD_RESET frame.
     *
     * @param[in] aFrame   A pointer to buffer containing the spinel frame.
     * @param[in] aLength  The length (number of bytes) in the frame.
     *
     * @retval true  If the frame is a Spinel SPINEL_CMD_RESET frame.
     * @retval false If the frame is not a Spinel SPINEL_CMD_RESET frame.
     *
     */
    bool IsSpinelResetCommand(const uint8_t *aFrame, uint16_t aLength)
    {
        static constexpr uint8_t kSpinelResetCommand[] = {SPINEL_HEADER_FLAG | SPINEL_HEADER_IID_0, SPINEL_CMD_RESET};
        return (aLength >= sizeof(kSpinelResetCommand)) &&
               (memcmp(aFrame, kSpinelResetCommand, sizeof(kSpinelResetCommand)) == 0);
    }
};
} // namespace Spinel
} // namespace ot

#endif // SPINEL_SPINEL_INTERFACE_HPP_
