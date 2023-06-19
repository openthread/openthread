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

#include "openthread-core-config.h"

#include "lib/spinel/spi_frame.hpp"
#include "ncp/ncp_base.hpp"

namespace ot {
namespace Ncp {

class NcpSpi : public NcpBase
{
public:
    /**
     * Initializes the object.
     *
     * @param[in]  aInstance  A pointer to the OpenThread instance structure.
     *
     */
    explicit NcpSpi(Instance *aInstance);

private:
    enum
    {
        /**
         * SPI tx and rx buffer size (should fit a max length frame + SPI header).
         *
         */
        kSpiBufferSize = OPENTHREAD_CONFIG_NCP_SPI_BUFFER_SIZE,

        /**
         * Size of the SPI header (in bytes).
         *
         */
        kSpiHeaderSize = Spinel::SpiFrame::kHeaderSize,
    };

    enum TxState
    {
        kTxStateIdle,            // No frame to send.
        kTxStateSending,         // A frame is ready to be sent.
        kTxStateHandlingSendDone // The frame was sent successfully, waiting to prepare the next one (if any).
    };

    typedef uint8_t LargeFrameBuffer[kSpiBufferSize];
    typedef uint8_t EmptyFrameBuffer[kSpiHeaderSize];

    static bool SpiTransactionComplete(void    *aContext,
                                       uint8_t *aOutputBuf,
                                       uint16_t aOutputLen,
                                       uint8_t *aInputBuf,
                                       uint16_t aInputLen,
                                       uint16_t aTransLen);
    bool        SpiTransactionComplete(uint8_t *aOutputBuf,
                                       uint16_t aOutputLen,
                                       uint8_t *aInputBuf,
                                       uint16_t aInputLen,
                                       uint16_t aTransLen);

    static void SpiTransactionProcess(void *aContext);
    void        SpiTransactionProcess(void);

    static void HandleFrameAddedToTxBuffer(void                    *aContext,
                                           Spinel::Buffer::FrameTag aFrameTag,
                                           Spinel::Buffer::Priority aPriority,
                                           Spinel::Buffer          *aBuffer);

    static void PrepareTxFrame(Tasklet &aTasklet);
    void        PrepareTxFrame(void);
    void        HandleRxFrame(void);
    void        PrepareNextSpiSendFrame(void);

    volatile TxState mTxState;
    volatile bool    mHandlingRxFrame;
    volatile bool    mResetFlag;

    Tasklet mPrepareTxFrameTask;

    uint16_t         mSendFrameLength;
    LargeFrameBuffer mSendFrame;
    EmptyFrameBuffer mEmptySendFrameFullAccept;
    EmptyFrameBuffer mEmptySendFrameZeroAccept;

    LargeFrameBuffer mReceiveFrame;
    EmptyFrameBuffer mEmptyReceiveFrame;
};

} // namespace Ncp
} // namespace ot

#endif // NCP_SPI_HPP_
