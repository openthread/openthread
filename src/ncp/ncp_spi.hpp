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
 *   This file contains definitions for a SPI interface to the OpenThread stack.
 */

#ifndef NCP_SPI_HPP_
#define NCP_SPI_HPP_

#if !defined(_openthread_config_h_sentinel_)
#error "Please include <openthread-config.h> first"
#endif

#include <ncp/ncp_base.hpp>

namespace ot {

class NcpSpi : public NcpBase
{
    typedef NcpBase super_t;

public:
    /**
     * Constructor
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     *
     */
    NcpSpi(otInstance *aInstance);

    void ReceiveTask(const uint8_t *aBuf, uint16_t aBufLength);

private:
    enum
    {
        kSpiBufferSize   = OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE, // Spi buffer size (should be large enough to fit a
                                                                  // max length frame + spi header).
        kSpiHeaderLength = 5,                                     // Size of spi header.
    };

    enum TxState
    {
        kTxStateIdle,                      // No frame to send
        kTxStateSending,                   // A frame is ready to be sent
        kTxStateHandlingSendDone           // The frame was sent successfully, waiting to prepare the next one (if any)
    };

    uint16_t OutboundFrameSize(void);

    static void SpiTransactionComplete(void *context, uint8_t *aOutputBuf, uint16_t aOutputBufLen, uint8_t *aInputBuf,
                                       uint16_t aInputBufLen, uint16_t aTransactionLength);
    void SpiTransactionComplete(uint8_t *aOutputBuf, uint16_t aOutputBufLen, uint8_t *aInputBuf, uint16_t aInputBufLen,
                                uint16_t aTransactionLength);

    static void HandleRxFrame(void *context);
    void HandleRxFrame(void);

    static void PrepareTxFrame(void *context);
    void PrepareTxFrame(void);

    static void TxFrameBufferHasData(void *aContext, NcpFrameBuffer *aNcpFrameBuffer);
    void TxFrameBufferHasData(void);

    ThreadError PrepareNextSpiSendFrame(void);

    TxState mTxState;
    bool mHandlingRxFrame;

    Tasklet mHandleRxFrameTask;
    Tasklet mPrepareTxFrameTask;

    uint8_t mSendFrame[kSpiBufferSize];
    uint16_t mSendFrameLen;
    uint8_t mReceiveFrame[kSpiBufferSize];

    uint8_t mEmptySendFrame[kSpiHeaderLength];
    uint8_t mEmptyReceiveFrame[kSpiHeaderLength];
};

}  // namespace ot

#endif  // NCP_SPI_HPP_
