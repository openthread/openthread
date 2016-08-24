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
 *   This file contains definitions for a SPI interface to the OpenThread stack.
 */

#ifndef NCP_SPI_HPP_
#define NCP_SPI_HPP_

#include <ncp/ncp_base.hpp>
#include <ncp/ncp_buffer.hpp>

namespace Thread {

class NcpSpi : public NcpBase
{
    typedef NcpBase super_t;

public:
    NcpSpi();

    virtual ThreadError OutboundFrameBegin(void);
    virtual ThreadError OutboundFrameFeedData(const uint8_t *frame, uint16_t frameLength);
    virtual ThreadError OutboundFrameFeedMessage(Message &message);
    virtual ThreadError OutboundFrameEnd(void);

    void ReceiveTask(const uint8_t *aBuf, uint16_t aBufLength);

private:
    enum
    {
        kSpiBufferSize   = 1500, // Spi buffer size (should be large enough to fit a max length frame + spi header).
        kTxBufferSize    = 512,  // Tx Buffer size (used by mTxFrameBuffer).
        kSpiHeaderLength = 5,    // Size of spi header.
    };

    uint16_t OutboundFrameSize(void);

    static void SpiTransactionComplete(
        void *context,
        uint8_t *anOutputBuf,
        uint16_t anOutputBufLen,
        uint8_t *anInputBuf,
        uint16_t anInputBufLen,
        uint16_t aTransactionLength
    );
    void SpiTransactionComplete(
        uint8_t *anOutputBuf,
        uint16_t anOutputBufLen,
        uint8_t *anInputBuf,
        uint16_t anInputBufLen,
        uint16_t aTransactionLength
    );

    static void HandleRxFrame(void *context);
    void HandleRxFrame(void);

    static void PrepareTxFrame(void *context);
    void PrepareTxFrame(void);

    static void TxFrameBufferHasData(void *aContext, NcpFrameBuffer *aNcpFrameBuffer);
    void TxFrameBufferHasData(void);

    ThreadError PrepareNextSpiSendFrame(void);

    bool mSending;
    bool mHandlingRxFrame;
    bool mHandlingSendDone;

    Tasklet mHandleRxFrameTask;
    Tasklet mPrepareTxFrameTask;

    uint8_t mSendFrame[kSpiBufferSize];
    uint16_t mSendFrameLen;
    uint8_t mReceiveFrame[kSpiBufferSize];

    uint8_t mEmptySendFrame[kSpiHeaderLength];
    uint8_t mEmptyReceiveFrame[kSpiHeaderLength];

    uint8_t mTxBuffer[kTxBufferSize];
    NcpFrameBuffer  mTxFrameBuffer;
};

}  // namespace Thread

#endif  // NCP_SPI_HPP_
