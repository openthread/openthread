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
#include  "openthread-enable-defines.h"
#include "openthread/types.h"

namespace ot {

/**
 * @namespace ot::Hdlc
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
     * This class defines a write iterator into a buffer used by Encoder.
     *
     * Hdlc users should sub-class this to add the actual buffer space.
     */
    class BufferWriteIterator
    {
    public:

        /**
         * This method writes a byte to the buffer and updates the iterator (if space is available).
         *
         * @retval kThreadError_None    Successfully wrote the byte and updates the iterator.
         * @retval kThreadError_NoBufs  Insufficient buffer space.
         *
         */
        ThreadError WriteByte(uint8_t aByte);

        /**
         * This method checks if there is buffer space available to write @p aWriteLength bytes.
         *
         * param[in] aWriteLength       Number of bytes to write.
         *
         * @retval true                 Enough buffer space available to write the requested number of bytes.
         * @retval false                Insufficient buffer space to write the requested number of bytes.
         *
         */
        bool CanWrite(uint16_t aWriteLength) const;

    protected:

        BufferWriteIterator(void);   ///< Protected constructor to ensure no direct instantiation.

        uint8_t  *mWritePointer;     ///< A pointer to current write position in the buffer.
        uint16_t  mRemainingLength;  ///< Number of remaining bytes available to write.
    };

    /**
     * This constructor initializes the object.
     *
     */
    Encoder(void);

    /**
     * This method begins an HDLC frame and puts the initial bytes into a buffer at the given @p aIterator.
     *
     * @param[inout] aIterator      A reference to a buffer write iterator. On successful exit, the iterator is updated.
     *
     * @retval kThreadError_None    Successfully started the HDLC frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to start the HDLC frame.
     *
     */
    ThreadError Init(BufferWriteIterator &aIterator);

    /**
     * This method encodes a single byte into a buffer at @p aIterator.
     *
     * If there is no space to add the byte, the write iterator remains the same.
     *
     * @param[in]    aInByte        A byte to encode and add.
     * @param[inout] aIterator      A reference to a write buffer iterator. On successful exit, the iterator is updated.
     *
     * @retval kThreadError_None    Successfully encoded and added the byte.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to encode and add the byte.
     *
     */
    ThreadError Encode(uint8_t aInByte, BufferWriteIterator &aIterator);

    /**
     * This method encodes the frame into a buffer at @p aIterator.
     *
     * This method returns success only if there is space in buffer to encode the entire frame. If there is no space
     * to encode the entire frame, the write iterator remains the same.
     *
     * @param[in]    aInBuf         A pointer to the input buffer.
     * @param[in]    aInLength      The number of bytes in @p aInBuf to encode.
     * @param[inout] aIterator      A reference to a write buffer iterator. On successful exit, the iterator is updated.
     *
     * @retval kThreadError_None    Successfully encoded the HDLC frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to encode the HDLC frame.
     *
     */
    ThreadError Encode(const uint8_t *aInBuf, uint16_t aInLength, BufferWriteIterator &aIterator);

    /**
     * This method finalizes an HDLC frame.
     *
     * @param[inout] aIterator      A reference to a write buffer iterator. On successful exit, the iterator is updated.
     *
     * @retval kThreadError_None    Successfully ended the HDLC frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to end the HDLC frame.
     *
     */
    ThreadError Finalize(BufferWriteIterator &aIterator);

private:
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
     * This function pointer is called when an error has occurred.
     *
     * @param[in]  aContext      A pointer to arbitrary context information.
     * @param[in]  aError        An error code describing the error.
     * @param[in]  aFrame        A pointer to the frame.
     * @param[in]  aFrameLength  The frame length in bytes.
     *
     */
    typedef void (*ErrorHandler)(void *aContext, ThreadError aError, uint8_t *aFrame, uint16_t aFrameLength);

    /**
     * This constructor initializes the decoder.
     *
     * @param[in]  aOutBuf        A pointer to the output buffer.
     * @param[in]  aOutLength     Size of the output buffer in bytes.
     * @param[in]  aFrameHandler  A pointer to a function that is called when a complete frame is received.
     * @param[in]  aContext       A pointer to arbitrary context information.
     *
     */
    Decoder(uint8_t *aOutBuf, uint16_t aOutLength, FrameHandler aFrameHandler, ErrorHandler aErrorHandler,
            void *aContext);

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
    ErrorHandler mErrorHandler;
    void *mContext;

    uint8_t *mOutBuf;
    uint16_t mOutOffset;
    uint16_t mOutLength;

    uint16_t mFcs;
};

}  // namespace Hdlc
}  // namespace ot

#endif  // HDLC_HPP_
