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
 *   This file contains definitions for a UART based NCP interface to the OpenThread stack.
 */

#ifndef NCP_UART_HPP_
#define NCP_UART_HPP_

#include <ncp/ncp_base.hpp>
#include <ncp/flen.hpp>
#include <ncp/hdlc.hpp>
#include <ncp/ncp_buffer.hpp>

namespace Thread {

class NcpUart : public NcpBase
{
    typedef NcpBase super_t;

public:
    /**
     * Constructor
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     *
     */
    NcpUart(otInstance *aInstance);

    /**
     * This method is called to start a new outbound frame.
     *
     * @retval kThreadError_None      Successfully started a new frame.
     * @retval kThreadError_NoBufs    Insufficient buffer space available to start a new frame.
     *
     */
    virtual ThreadError OutboundFrameBegin(void);

    /**
     * This method adds data to the current outbound frame being written.
     *
     * If no buffer space is available, this method will discard and clear the frame before returning an error status.
     *
     * @param[in]  aDataBuffer        A pointer to data buffer.
     * @param[in]  aDataBufferLength  The length of the data buffer.
     *
     * @retval kThreadError_None      Successfully added new data to the frame.
     * @retval kThreadError_NoBufs    Insufficient buffer space available to add data.
     *
     */
    virtual ThreadError OutboundFrameFeedData(const uint8_t *aDataBuffer, uint16_t aDataBufferLength);

    /**
     * This method adds a message to the current outbound frame being written.
     *
     * If no buffer space is available, this method will discard and clear the frame before returning an error status.
     * In case of success, the passed-in message @aMessage will be owned by outbound buffer and will be freed
     * when either the the frame is successfully sent and removed or if the frame is discarded.
     *
     * @param[in]  aMessage         A reference to the message to be added to current frame.
     *
     * @retval kThreadError_None    Successfully added the message to the frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to add message.
     *
     */
    virtual ThreadError OutboundFrameFeedMessage(Message &aMessage);

    /**
     * This method finalizes and sends the current outbound frame.
     *
     * If no buffer space is available, this method will discard and clear the frame before returning an error status.
     *
     * @retval kThreadError_None    Successfully added the message to the frame.
     * @retval kThreadError_NoBufs  Insufficient buffer space available to add message.
     *
     */
    virtual ThreadError OutboundFrameEnd(void);

    /**
     * This method is called when uart tx is finished. It prepares and sends the next data chunk (if any) to uart.
     *
     */
    void HandleUartSendDone(void);

    /**
     * This method is called when uart received a data buffer.
     *
     */
    void HandleUartReceiveDone(const uint8_t *aBuf, uint16_t aBufLength);

private:

    enum
    {
        kUartTxBufferSize = 128,  // Uart tx buffer size.
        kTxBufferSize = 512,      // Tx Buffer size (used by mTxFrameBuffer).
        kRxBufferSize = 1500,     // Rx buffer size (should be large enough to fit one whole (decoded) received frame).
    };

    enum UartTxState
    {
        kStartingFrame,          // Starting a new frame.
        kEncodingFrame,          // In middle of encoding a frame.
        kFinalizingFrame,        // Finalizing a frame.
    };

    class UartTxBuffer : public Hdlc::Encoder::BufferWriteIterator
    {
    public:
        UartTxBuffer(void);

        void           Clear(void);
        bool           IsEmpty(void) const;
        uint16_t       GetLength(void) const;
        const uint8_t *GetBuffer(void) const;

    private:
        uint8_t        mBuffer[kUartTxBufferSize];
    };

    void            EncodeAndSendToUart(void);
    void            HandleFrame(uint8_t *aBuf, uint16_t aBufLength);
    void            HandleError(ThreadError aError, uint8_t *aBuf, uint16_t aBufLength);
    void            TxFrameBufferHasData(void);

    static void     EncodeAndSendToUart(void *aContext);
    static void     HandleFrame(void *context, uint8_t *aBuf, uint16_t aBufLength);
    static void     HandleError(void *context, ThreadError aError, uint8_t *aBuf, uint16_t aBufLength);
    static void     TxFrameBufferHasData(void *aContext, NcpFrameBuffer *aNcpFrameBuffer);

    Hdlc::Encoder   mFrameEncoder;
    Hdlc::Decoder   mFrameDecoder;
    UartTxBuffer    mUartBuffer;
    NcpFrameBuffer  mTxFrameBuffer;
    UartTxState     mState;
    uint8_t         mByte;
    uint8_t         mTxBuffer[kTxBufferSize];
    uint8_t         mRxBuffer[kRxBufferSize];
    Tasklet         mUartSendTask;
};

}  // namespace Thread

#endif  // NCP_UART_HPP_
