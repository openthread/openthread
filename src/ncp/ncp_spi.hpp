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

#include "ncp/ncp_base.hpp"

namespace ot {
namespace Ncp {

/**
 * This class defines a SPI frame.
 *
 */
class SpiFrame
{
public:
    enum
    {
        kHeaderSize = 5, ///< SPI header size (in bytes).
    };

    /**
     * This constructor initializes an `SpiFrame` instance.
     *
     * @param[in] aBuffer     Pointer to buffer containing the frame.
     *
     */
    SpiFrame(uint8_t *aBuffer)
        : mBuffer(aBuffer)
    {
    }

    /**
     * This method gets a pointer to data portion in the SPI frame skipping the header.
     *
     * @returns  A pointer to data in the SPI frame.
     *
     */
    uint8_t *GetData(void) { return mBuffer + kHeaderSize; }

    /**
     * This method indicates whether or not the frame is valid.
     *
     * In a valid frame the flag byte should contain the pattern bits.
     *
     * @returns TRUE if the frame is valid, FALSE otherwise.
     *
     */
    bool IsValid(void) const { return ((mBuffer[kIndexFlagByte] & kFlagPatternMask) == kFlagPattern); }

    /**
     * This method sets the "flag byte" field in the SPI frame header.
     *
     * @param[in] aResetFlag     The status of reset flag (TRUE to set the flag, FALSE to clear flag).
     *
     */
    void SetHeaderFlagByte(bool aResetFlag) { mBuffer[kIndexFlagByte] = kFlagPattern | (aResetFlag ? kFlagReset : 0); }

    /**
     * This method sets the "accept len" field in the SPI frame header.
     *
     * "accept len" specifies number of bytes the sender of the SPI frame can receive.
     *
     * @param[in] aAcceptLen    The accept length in bytes.
     *
     */
    void SetHeaderAcceptLen(uint16_t aAcceptLen)
    {
        Encoding::LittleEndian::WriteUint16(aAcceptLen, mBuffer + kIndexAcceptLen);
    }

    /**
     * This method gets the "accept len" field in the SPI frame header.
     *
     * @returns  The accept length in bytes.
     *
     */
    uint16_t GetHeaderAcceptLen(void) const { return Encoding::LittleEndian::ReadUint16(mBuffer + kIndexAcceptLen); }

    /**
     * This method sets the "data len" field in the SPI frame header.
     *
     * "Data len" specifies number of data bytes in the transmitted SPI frame.
     *
     * @param[in] aDataLen    The data length in bytes.
     *
     */
    void SetHeaderDataLen(uint16_t aDataLen) { Encoding::LittleEndian::WriteUint16(aDataLen, mBuffer + kIndexDataLen); }

    /**
     * This method gets the "data len" field in the SPI frame header.
     *
     * @returns  The data length in bytes.
     *
     */
    uint16_t GetHeaderDataLen(void) const { return Encoding::LittleEndian::ReadUint16(mBuffer + kIndexDataLen); }

private:
    enum
    {
        kIndexFlagByte  = 0, // flag byte  (uint8_t).
        kIndexAcceptLen = 1, // accept len (uint16_t little-endian encoding).
        kIndexDataLen   = 3, // data len   (uint16_t little-endian encoding).

        kFlagReset       = (1 << 7), // Flag byte RESET bit.
        kFlagPattern     = 0x02,     // Flag byte PATTERN bits.
        kFlagPatternMask = 0x03,     // Flag byte PATTERN mask.
    };

    uint8_t *mBuffer;
};

class NcpSpi : public NcpBase
{
public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance  A pointer to the OpenThread instance structure.
     *
     */
    NcpSpi(Instance *aInstance);

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
        kSpiHeaderSize = SpiFrame::kHeaderSize,
    };

    enum TxState
    {
        kTxStateIdle,            // No frame to send.
        kTxStateSending,         // A frame is ready to be sent.
        kTxStateHandlingSendDone // The frame was sent successfully, waiting to prepare the next one (if any).
    };

    typedef uint8_t LargeFrameBuffer[kSpiBufferSize];
    typedef uint8_t EmptyFrameBuffer[kSpiHeaderSize];

    static bool SpiTransactionComplete(void *   aContext,
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

    static void HandleFrameAddedToTxBuffer(void *                   aContext,
                                           NcpFrameBuffer::FrameTag aFrameTag,
                                           NcpFrameBuffer::Priority aPriority,
                                           NcpFrameBuffer *         aNcpFrameBuffer);

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
