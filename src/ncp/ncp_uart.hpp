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

#include "openthread-core-config.h"

#include "ncp/hdlc.hpp"
#include "ncp/ncp_base.hpp"

#if OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
#include "spinel_encrypter.hpp"
#endif // OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER

namespace ot {
namespace Ncp {

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
    NcpUart(Instance *aInstance);

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
        kUartTxBufferSize = OPENTHREAD_CONFIG_NCP_UART_TX_CHUNK_SIZE,   // Uart tx buffer size.
        kRxBufferSize     = OPENTHREAD_CONFIG_NCP_UART_RX_BUFFER_SIZE + // Rx buffer size (should be large enough to fit
                        OPENTHREAD_CONFIG_NCP_SPINEL_ENCRYPTER_EXTRA_DATA_SIZE, // one whole (decoded) received frame).
    };

    enum UartTxState
    {
        kStartingFrame,   // Starting a new frame.
        kEncodingFrame,   // In middle of encoding a frame.
        kFinalizingFrame, // Finalizing a frame.
    };

#if OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
    /**
     * Wraps NcpFrameBuffer allowing to read data through spinel encrypter.
     * Creates additional buffers to allow transforming of the whole spinel frames.
     */
    class NcpFrameBufferEncrypterReader
    {
    public:
        /**
         * C-tor.
         * Takes a reference to NcpFrameBuffer in order to read spinel frames.
         */
        explicit NcpFrameBufferEncrypterReader(NcpFrameBuffer &aTxFrameBuffer);
        bool    IsEmpty(void) const;
        otError OutFrameBegin(void);
        bool    OutFrameHasEnded(void);
        uint8_t OutFrameReadByte(void);
        otError OutFrameRemove(void);

    private:
        void Reset(void);

        NcpFrameBuffer &mTxFrameBuffer;
        uint8_t         mDataBuffer[kRxBufferSize];
        size_t          mDataBufferReadIndex;
        size_t          mOutputDataLength;
    };
#endif // OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER

    void EncodeAndSendToUart(void);
    void HandleFrame(otError aError);
    void HandleError(otError aError, uint8_t *aBuf, uint16_t aBufLength);
    void TxFrameBufferHasData(void);
    void HandleFrameAddedToNcpBuffer(void);

    static void EncodeAndSendToUart(Tasklet &aTasklet);
    static void HandleFrame(void *aConext, otError aError);
    static void HandleFrameAddedToNcpBuffer(void *                   aContext,
                                            NcpFrameBuffer::FrameTag aTag,
                                            NcpFrameBuffer::Priority aPriority,
                                            NcpFrameBuffer *         aNcpFrameBuffer);

    Hdlc::Encoder                        mFrameEncoder;
    Hdlc::Decoder                        mFrameDecoder;
    Hdlc::FrameBuffer<kUartTxBufferSize> mUartBuffer;
    UartTxState                          mState;
    uint8_t                              mByte;
    Hdlc::FrameBuffer<kRxBufferSize>     mRxBuffer;
    bool                                 mUartSendImmediate;
    Tasklet                              mUartSendTask;

#if OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
    NcpFrameBufferEncrypterReader mTxFrameBufferEncrypterReader;
#endif // OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
};

} // namespace Ncp
} // namespace ot

#endif // NCP_UART_HPP_
