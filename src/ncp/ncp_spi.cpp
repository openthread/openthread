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
 *   This file implements a SPI interface to the OpenThread stack.
 */

#include <openthread/config.h>

#include "ncp_spi.hpp"

#include <openthread/ncp.h>
#include <openthread/platform/spi-slave.h>
#include <openthread/platform/misc.h>

#include "openthread-core-config.h"
#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/new.hpp"
#include "net/ip6.hpp"

#define SPI_RESET_FLAG          0x80
#define SPI_CRC_FLAG            0x40
#define SPI_PATTERN_VALUE       0x02
#define SPI_PATTERN_MASK        0x03

#if OPENTHREAD_ENABLE_NCP_SPI

namespace ot {

static otDEFINE_ALIGNED_VAR(sNcpRaw, sizeof(NcpSpi), uint64_t);

extern "C" void otNcpInit(otInstance *aInstance)
{
    NcpSpi *ncpSpi = NULL;

    ncpSpi = new(&sNcpRaw) NcpSpi(aInstance);

    if (ncpSpi == NULL || ncpSpi != NcpBase::GetNcpInstance())
    {
        assert(false);
    }
}

static void spi_header_set_flag_byte(uint8_t *header, uint8_t value)
{
    header[0] = value;
}

static void spi_header_set_accept_len(uint8_t *header, uint16_t len)
{
    header[1] = ((len >> 0) & 0xff);
    header[2] = ((len >> 8) & 0xff);
}

static void spi_header_set_data_len(uint8_t *header, uint16_t len)
{
    header[3] = ((len >> 0) & 0xff);
    header[4] = ((len >> 8) & 0xff);
}

static uint8_t spi_header_get_flag_byte(const uint8_t *header)
{
    return header[0];
}

static uint16_t spi_header_get_accept_len(const uint8_t *header)
{
    return ( header[1] + static_cast<uint16_t>(header[2] << 8) );
}

static uint16_t spi_header_get_data_len(const uint8_t *header)
{
    return ( header[3] + static_cast<uint16_t>(header[4] << 8) );
}

NcpSpi::NcpSpi(otInstance *aInstance) :
    NcpBase(aInstance),
    mTxState(kTxStateIdle),
    mHandlingRxFrame(false),
    mResetFlag(true),
    mPrepareTxFrameTask(*aInstance, &NcpSpi::PrepareTxFrame, this),
    mSendFrameLen(0)
{
    memset(mSendFrame, 0, kSpiHeaderLength);
    memset(mEmptySendFrameZeroAccept, 0, kSpiHeaderLength);
    memset(mEmptySendFrameFullAccept, 0, kSpiHeaderLength);

    mTxFrameBuffer.SetFrameAddedCallback(HandleFrameAddedToTxBuffer, this);

    spi_header_set_flag_byte(mSendFrame, SPI_RESET_FLAG | SPI_PATTERN_VALUE);
    spi_header_set_flag_byte(mEmptySendFrameZeroAccept, SPI_RESET_FLAG | SPI_PATTERN_VALUE);
    spi_header_set_flag_byte(mEmptySendFrameFullAccept, SPI_RESET_FLAG | SPI_PATTERN_VALUE);

    spi_header_set_accept_len(mSendFrame, sizeof(mReceiveFrame) - kSpiHeaderLength);
    spi_header_set_accept_len(mEmptySendFrameFullAccept, sizeof(mReceiveFrame) - kSpiHeaderLength);
    spi_header_set_accept_len(mEmptySendFrameZeroAccept, 0);

    otPlatSpiSlaveEnable(&NcpSpi::SpiTransactionComplete, &NcpSpi::SpiTransactionProcess, this);

    // We signal an interrupt on this first transaction to
    // make sure that the host processor knows that our
    // reset flag was set.

    otPlatSpiSlavePrepareTransaction(mEmptySendFrameZeroAccept, kSpiHeaderLength, mEmptyReceiveFrame, kSpiHeaderLength,
                                     true);
}

bool NcpSpi::SpiTransactionComplete(void *aContext, uint8_t *aOutputBuf, uint16_t aOutputBufLen, uint8_t *aInputBuf,
                                    uint16_t aInputBufLen, uint16_t aTransactionLength)
{
    return static_cast<NcpSpi *>(aContext)->SpiTransactionComplete(aOutputBuf, aOutputBufLen, aInputBuf, aInputBufLen,
                                                           aTransactionLength);
}

bool NcpSpi::SpiTransactionComplete(uint8_t *aOutputBuf, uint16_t aOutputBufLen, uint8_t *aInputBuf,
                                    uint16_t aInputBufLen, uint16_t aTransactionLength)
{
    // This may be executed from an interrupt context.
    // Must return as quickly as possible.

    uint16_t rx_data_len = 0;
    uint16_t rx_accept_len = 0;
    uint16_t tx_data_len = 0;
    uint16_t tx_accept_len = 0;
    bool shouldProcess = false;

    // TODO: Check `PATTERN` bits of `HDR` and ignore frame if not set.
    //       Holding off on implementing this so as to not cause immediate
    //       compatibility problems, even though it is required by the spec.

    if (aTransactionLength >= kSpiHeaderLength)
    {
        if (aOutputBufLen >= kSpiHeaderLength)
        {
            rx_accept_len = spi_header_get_accept_len(aOutputBuf);
            tx_data_len = spi_header_get_data_len(aOutputBuf);
            (void)spi_header_get_flag_byte(aOutputBuf);
        }

        if (aInputBufLen >= kSpiHeaderLength)
        {
            rx_data_len = spi_header_get_data_len(aInputBuf);
            tx_accept_len = spi_header_get_accept_len(aInputBuf);
        }

        if (!mHandlingRxFrame &&
            (rx_data_len > 0) &&
            (rx_data_len <= aTransactionLength - kSpiHeaderLength) &&
            (rx_data_len <= rx_accept_len))
        {
            mHandlingRxFrame = true;
            shouldProcess = true;
        }

        if ((mTxState == kTxStateSending) &&
            (tx_data_len > 0) &&
            (tx_data_len <= aTransactionLength - kSpiHeaderLength) &&
            (tx_data_len <= tx_accept_len))
        {
            mTxState = kTxStateHandlingSendDone;
            shouldProcess = true;
        }
    }

    if (mResetFlag && (aTransactionLength > 0) && (aOutputBufLen > 0))
    {
        // Clear the reset flag.
        mResetFlag = false;
        spi_header_set_flag_byte(mSendFrame, SPI_PATTERN_VALUE);
        spi_header_set_flag_byte(mEmptySendFrameZeroAccept, SPI_PATTERN_VALUE);
        spi_header_set_flag_byte(mEmptySendFrameFullAccept, SPI_PATTERN_VALUE);
    }

    if (mTxState == kTxStateSending)
    {
        aOutputBuf = mSendFrame;
        aOutputBufLen = mSendFrameLen;
    }
    else
    {
        aOutputBuf = (mHandlingRxFrame) ? mEmptySendFrameZeroAccept : mEmptySendFrameFullAccept;
        aOutputBufLen = kSpiHeaderLength;
    }

    if (mHandlingRxFrame)
    {
        aInputBuf = mEmptyReceiveFrame;
        aInputBufLen = kSpiHeaderLength;
        spi_header_set_accept_len(mSendFrame, 0);
    }
    else
    {
        aInputBuf = mReceiveFrame;
        aInputBufLen = sizeof(mReceiveFrame);
        spi_header_set_accept_len(mSendFrame, sizeof(mReceiveFrame) - kSpiHeaderLength);
    }

    otPlatSpiSlavePrepareTransaction(aOutputBuf, aOutputBufLen, aInputBuf, aInputBufLen,
                                     (mTxState == kTxStateSending));

    return shouldProcess;
}

void NcpSpi::SpiTransactionProcess(void *aContext)
{
    static_cast<NcpSpi *>(aContext)->SpiTransactionProcess();
}

void NcpSpi::SpiTransactionProcess(void)
{
    if (mTxState == kTxStateHandlingSendDone)
    {
        mPrepareTxFrameTask.Post();
    }

    if (mHandlingRxFrame)
    {
        HandleRxFrame();
    }
}

void NcpSpi::HandleFrameAddedToTxBuffer(void *aContext, NcpFrameBuffer::FrameTag aTag,
                                        NcpFrameBuffer::Priority aPriority, NcpFrameBuffer *aNcpFrameBuffer)
{
    OT_UNUSED_VARIABLE(aNcpFrameBuffer);
    OT_UNUSED_VARIABLE(aTag);
    OT_UNUSED_VARIABLE(aPriority);

    static_cast<NcpSpi *>(aContext)->mPrepareTxFrameTask.Post();
}

otError NcpSpi::PrepareNextSpiSendFrame(void)
{
    otError errorCode = OT_ERROR_NONE;
    uint16_t frameLength;
    uint16_t readLength;

    VerifyOrExit(!mTxFrameBuffer.IsEmpty());

    if (ShouldWakeHost())
    {
        otPlatWakeHost();
    }

    SuccessOrExit(errorCode = mTxFrameBuffer.OutFrameBegin());

    frameLength = mTxFrameBuffer.OutFrameGetLength();
    assert(frameLength <= sizeof(mSendFrame) - kSpiHeaderLength);

    spi_header_set_data_len(mSendFrame, frameLength);

    // The "accept length" in `mSendFrame` is already updated based
    // on current state of receive. It is changed either from the
    // `SpiTransactionComplete()` callback or from `HandleRxFrame()`.

    readLength = mTxFrameBuffer.OutFrameRead(frameLength, mSendFrame + kSpiHeaderLength);
    assert(readLength == frameLength);

    mSendFrameLen = frameLength + kSpiHeaderLength;

    mTxState = kTxStateSending;

    // Prepare new transaction by using `mSendFrame` as the output
    // buffer while keeping the input buffer unchanged.

    errorCode = otPlatSpiSlavePrepareTransaction(mSendFrame, mSendFrameLen, NULL, 0, true);

    if (errorCode == OT_ERROR_BUSY)
    {
        // Being busy is OK. We will get the transaction
        // set up properly when the current transaction
        // is completed.
        errorCode = OT_ERROR_NONE;
    }

    if (errorCode != OT_ERROR_NONE)
    {
        mTxState = kTxStateIdle;
        mPrepareTxFrameTask.Post();
        ExitNow();
    }

    mTxFrameBuffer.OutFrameRemove();

exit:
    return errorCode;
}

void NcpSpi::PrepareTxFrame(Tasklet &aTasklet)
{
    OT_UNUSED_VARIABLE(aTasklet);
    static_cast<NcpSpi *>(GetNcpInstance())->PrepareTxFrame();
}

void NcpSpi::PrepareTxFrame(void)
{
    switch (mTxState)
    {
    case kTxStateHandlingSendDone:
        mTxState = kTxStateIdle;

        // Fall-through to next case to prepare the next frame (if any).

    case kTxStateIdle:
        PrepareNextSpiSendFrame();
        break;

    case kTxStateSending:
        // The next frame in queue (if any) will be prepared when the
        // current frame is successfully sent and this task is posted
        // again from the `SpiTransactionComplete()` callback.
        break;
    }
}

void NcpSpi::HandleRxFrame(void)
{
    // Pass the received frame to base class to process.
    HandleReceive(mReceiveFrame + kSpiHeaderLength, spi_header_get_data_len(mReceiveFrame));

    // The order of operations below is important. We should clear
    // the `mHandlingRxFrame` before checking `mTxState` and possibly
    // preparing the next transaction. Note that the callback
    // `SpiTransactionComplete()` can be invoked from ISR at any point.
    //
    // If we switch the order, we have the following race situation:
    // We check `mTxState` and it is in `kTxStateSending`, so we skip
    // preparing the transaction here. But before we set the
    // `mHandlingRxFrame` to `false`, the `SpiTransactionComplete()`
    // happens and prepares the next transaction and sets the accept
    // length to zero on `mSendFrame` (since it assumes we are still
    // handling the previous received frame).

    mHandlingRxFrame = false;

    // If tx state is in `kTxStateSending`, we wait for the callback
    // `SpiTransactionComplete()`  which will then set up everything
    // and prepare the next transaction.

    if (mTxState != kTxStateSending)
    {
        spi_header_set_accept_len(mSendFrame, sizeof(mReceiveFrame) - kSpiHeaderLength);

        otPlatSpiSlavePrepareTransaction(mEmptySendFrameFullAccept, kSpiHeaderLength, mReceiveFrame,
                                         sizeof(mReceiveFrame), false);

        // No need to check the error status. Getting `OT_ERROR_BUSY`
        // is OK as everything will be set up properly from callback when
        // the current transaction is completed.
    }
}

}  // namespace ot

#endif // OPENTHREAD_ENABLE_NCP_SPI
