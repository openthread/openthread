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
 *   This file includes definitions for an HDLC-lite encoder and decoer.
 */

#ifndef HDLC_HPP_
#define HDLC_HPP_

#include <openthread-types.h>
#include <common/message.hpp>

namespace Thread {

/**
 * @namespace Thread::Hdlc
 *
 * @brief
 *   This namespace includes definitions for the HDLC-lite encoder and decoder.
 *
 */
namespace Hdlc {

/**
 * This class implements the HDLC-lite encoder.
 *
 */
class Encoder
{
public:
    /**
     * This method begins an HDLC frame and puts the initial bytes into @p aOutBuf.
     *
     * @param[in]     aOutBuf     A pointer to the output buffer.
     * @param[inout]  aOutLength  On entry, the output buffer size; On exit, the output length.
     *
     * @retval kThreadError_None    Successfully started the HDLC frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to start the HDLC frame.
     *
     */
    ThreadError Init(uint8_t *aOutBuf, uint16_t &aOutLength);

    /**
     * This method encodes the frame.
     *
     * @param[in]   aInBuf      A pointer to the input buffer.
     * @param[in]   aInLength   The number of bytes in @p aInBuf to encode.
     * @param[out]  aOutBuf     A pointer to the output buffer.
     * @param[out]  aOutLength  On exit, the number of bytes placed in @p aOutBuf.
     *
     * @retval kThreadError_None    Successfully encoded the HDLC frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to encode the HDLC frame.
     *
     */
    ThreadError Encode(const uint8_t *aInBuf, uint16_t aInLength, uint8_t *aOutBuf, uint16_t &aOutLength);

    /**
     * This method ends an HDLC frame and puts the initial bytes into @p aOutBuf.
     *
     * @param[in]     aOutBuf     A pointer to the output buffer.
     * @param[inout]  aOutLength  On entry, the output buffer size; On exit, the output length.
     *
     * @retval kThreadError_None    Successfully ended the HDLC frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to end the HDLC frame.
     *
     */
    ThreadError Finalize(uint8_t *aOutBuf, uint16_t &aOutLength);

private:
    ThreadError Encode(uint8_t aInByte, uint8_t *aOutBuf, uint16_t aOutLength);

    uint16_t mOutOffset;
    uint16_t mFcs;
};

/**
 * This class implements the HDLC-lite decoder.
 *
 */
class Decoder
{
public:
    /**
     * This function pointer is called when a complete frame has been formed.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aFrame        A pointer to the frame.
     * @param[in]  aFrameLength  The frame length in bytes.
     *
     */
    typedef void (*FrameHandler)(void *aContext, uint8_t *aFrame, uint16_t aFrameLength);

    /**
     * This constructor initializes the decoder.
     *
     * @param[in]  aOutBuf        A pointer to the output buffer.
     * @param[in]  aOutLength     Size of the output buffer in bytes.
     * @param[in]  aFrameHandler  A pointer to a function that is called when a complete frame is received.
     * @param[in]  aContext       A pointer to arbitrary context information.
     *
     */
    Decoder(uint8_t *aOutBuf, uint16_t aOutLength, FrameHandler aFrameHandler, void *aContext);

    /**
     * This method streams bytes into the decoder.
     *
     * @param[in]  aInBuf     A pointer to the input buffer.
     * @param[in]  aInLength  The number of bytes in @p aInBuf.
     *
     */
    void Decode(const uint8_t *aInBuf, uint16_t aInLength);

private:
    enum State
    {
        kStateNoSync = 0,
        kStateSync,
        kStateEscaped,
    };
    State mState;

    FrameHandler mFrameHandler;
    void *mContext;

    uint8_t *mOutBuf;
    uint16_t mOutOffset;
    uint16_t mOutLength;

    uint16_t mFcs;
};

}  // namespace Hdlc
}  // namespace Thread

#endif  // HDLC_HPP_
