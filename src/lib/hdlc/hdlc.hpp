/*
 *    Copyright (c) 2016, The OpenThread Authors.
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
 *   This file includes definitions for an HDLC-lite encoder and decoder.
 */

#ifndef HDLC_HPP_
#define HDLC_HPP_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <openthread/error.h>

#include "lib/spinel/multi_frame_buffer.hpp"

namespace ot {

/**
 * @namespace ot::Hdlc
 *
 * @brief
 *   This namespace includes definitions for the HDLC-lite encoder and decoder.
 */
namespace Hdlc {

/**
 * Implements the HDLC-lite encoder.
 */
class Encoder
{
public:
    /**
     * Initializes the object.
     *
     * @param[in] aWritePointer   The `FrameWritePointer` used by `Encoder` to write the encoded frames.
     */
    explicit Encoder(Spinel::FrameWritePointer &aWritePointer);

    /**
     * Begins an HDLC frame.
     *
     * @retval OT_ERROR_NONE     Successfully started the HDLC frame.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to start the HDLC frame.
     */
    otError BeginFrame(void);

    /**
     * Encodes a single byte into current frame.
     *
     * If there is no space to add the byte, the write pointer in frame buffer remains the same.
     *
     * @param[in]    aByte       A byte value to encode and add to frame.
     *
     * @retval OT_ERROR_NONE     Successfully encoded and added the byte to frame buffer.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to encode and add the byte.
     */
    otError Encode(uint8_t aByte);

    /**
     * Encodes a given block of data into current frame.
     *
     * Returns success only if there is space in buffer to encode the entire block of data. If there is no
     * space to encode the entire block of data, the write pointer in frame buffer remains the same.
     *
     * @param[in]    aData       A pointer to a buffer containing the data to encode.
     * @param[in]    aLength     The number of bytes in @p aData.
     *
     * @retval OT_ERROR_NONE     Successfully encoded and added the data to frame.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to add the frame.
     */
    otError Encode(const uint8_t *aData, uint16_t aLength);

    /**
     * Ends/finalizes the HDLC frame.
     *
     * @retval OT_ERROR_NONE     Successfully ended the HDLC frame.
     * @retval OT_ERROR_NO_BUFS  Insufficient buffer space available to end the HDLC frame.
     */
    otError EndFrame(void);

private:
    Spinel::FrameWritePointer &mWritePointer;
    uint16_t                   mFcs;
};

/**
 * Implements the HDLC-lite decoder.
 */
class Decoder
{
public:
    /**
     * Pointer is called when either a complete frame has been decoded or an error occurs during
     * decoding.
     *
     * The decoded frame (or the partially decoded frame in case of an error) is available in `aFrameWritePointer`
     * buffer given in `Decoder` constructor.
     *
     * @param[in] aContext A pointer to arbitrary context information.
     * @param[in] aError   OT_ERROR_NONE    if the frame was decoded successfully,
     *                     OT_ERROR_PARSE   if the Frame Check Sequence (FCS) was incorrect in decoded frame,
     *                     OT_ERROR_NO_BUFS insufficient buffer space available to save the decoded frame.
     */
    typedef void (*FrameHandler)(void *aContext, otError aError);

    /**
     * Initializes the object.
     */
    Decoder(void);

    /**
     * Initializes the decoder.
     *
     * @param[in] aFrameWritePointer   The `FrameWritePointer` used by `Decoder` to write the decoded frames.
     * @param[in] aFrameHandler        The frame handler callback function pointer.
     * @param[in] aContext             A pointer to arbitrary context information.
     */
    void Init(Spinel::FrameWritePointer &aFrameWritePointer, FrameHandler aFrameHandler, void *aContext);

    /**
     * Feeds a block of data into the decoder.
     *
     * If during decoding, a full HDLC frame is successfully decoded or an error occurs, the `FrameHandler` callback
     * is called. The decoded frame (or the partially decoded frame in case of an error) is available in
     * `aFrameWritePointer` buffer from the constructor. The `Decoder` user (if required) must update/reset the write
     * pointer from this callback for the next frame to be decoded.
     *
     * @param[in]  aData    A pointer to a buffer containing data to be fed to decoder.
     * @param[in]  aLength  The number of bytes in @p aData.
     */
    void Decode(const uint8_t *aData, uint16_t aLength);

    /**
     * Resets internal states of the decoder.
     */
    void Reset(void);

private:
    enum State
    {
        kStateNoSync,
        kStateSync,
        kStateEscaped,
    };

    State                      mState;
    Spinel::FrameWritePointer *mWritePointer;
    FrameHandler               mFrameHandler;
    void                      *mContext;
    uint16_t                   mFcs;
    uint16_t                   mDecodedLength;
};

} // namespace Hdlc
} // namespace ot

#endif // HDLC_HPP_
