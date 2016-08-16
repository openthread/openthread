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
 *   This file contains definitions for a UART based NCP interface to the OpenThread stack.
 */

#include <common/code_utils.hpp>
#include <common/new.hpp>
#include <ncp/ncp.h>
#include <ncp/ncp_uart.hpp>
#include <platform/uart.h>
#include <core/openthread-core-config.h>

namespace Thread {

static otDEFINE_ALIGNED_VAR(sNcpRaw, sizeof(NcpUart), uint64_t);
static NcpUart *sNcpUart;

extern "C" void otNcpInit(void)
{
    sNcpUart = new(&sNcpRaw) NcpUart;
}

NcpUart::UartTxBuffer::UartTxBuffer(void)
    : Hdlc::Encoder::BufferWriteIterator()
{
    Clear();
}

void NcpUart::UartTxBuffer::Clear(void)
{
    mWritePointer = mBuffer;
    mRemainingLength = sizeof(mBuffer);
}

bool NcpUart::UartTxBuffer::IsEmpty(void) const
{
    return mWritePointer == mBuffer;
}

uint16_t NcpUart::UartTxBuffer::GetLength(void) const
{
    return static_cast<uint16_t>(mWritePointer - mBuffer);
}

const uint8_t *NcpUart::UartTxBuffer::GetBuffer(void) const
{
    return mBuffer;
}

NcpUart::NcpUart():
    NcpBase(),
    mFrameDecoder(mRxBuffer, sizeof(mRxBuffer), &HandleFrame, this),
    mUartBuffer(),
    mTxFrameBuffer(mTxBuffer, sizeof(mTxBuffer)),
    mUartSendTask(EncodeAndSendToUart, this)
{
    mState = kStartingFrame;

    mTxFrameBuffer.SetCallbacks(NULL, TxFrameBufferHasData, this);
}

ThreadError NcpUart::OutboundFrameBegin(void)
{
    return mTxFrameBuffer.InFrameBegin();
}

ThreadError NcpUart::OutboundFrameFeedData(const uint8_t *aDataBuffer, uint16_t aDataBufferLength)
{
    return mTxFrameBuffer.InFrameFeedData(aDataBuffer, aDataBufferLength);
}

ThreadError NcpUart::OutboundFrameFeedMessage(Message &aMessage)
{
    return mTxFrameBuffer.InFrameFeedMessage(aMessage);
}

ThreadError NcpUart::OutboundFrameSend(void)
{
    return mTxFrameBuffer.InFrameEnd();
}

void NcpUart::TxFrameBufferHasData(void *aContext, NcpFrameBuffer *aNcpFrameBuffer)
{
    (void)aContext;
    (void)aNcpFrameBuffer;

    sNcpUart->TxFrameBufferHasData();
}

void NcpUart::TxFrameBufferHasData(void)
{
    if (mUartBuffer.IsEmpty())
    {
        mUartSendTask.Post();
    }
}

void NcpUart::EncodeAndSendToUart(void *aContext)
{
    NcpUart *obj = reinterpret_cast<NcpUart *>(aContext);

    obj->EncodeAndSendToUart();
}

// This method encodes a frame from the tx frame buffer (mTxFrameBuffer) into the uart buffer and sends it over uart.
// If the uart buffer gets full, it sends the current encoded portion. This method remembers current state, so on
// sub-sequent calls, it restarts encoding the bytes from where it left of in the frame .
void NcpUart::EncodeAndSendToUart(void)
{
    uint16_t len;

    while (!mTxFrameBuffer.IsEmpty())
    {
        switch (mState)
        {
        case kStartingFrame:

            SuccessOrExit(mFrameEncoder.Init(mUartBuffer));

            mTxFrameBuffer.OutFrameBegin();

            mState = kEncodingFrame;

            while (!mTxFrameBuffer.OutFrameHasEnded())
            {
                mByte = mTxFrameBuffer.OutFrameReadByte();

            case kEncodingFrame:

                SuccessOrExit(mFrameEncoder.Encode(mByte, mUartBuffer));
            }

            mTxFrameBuffer.OutFrameRemove();

            // Notify the super/base class that there is space available in tx frame buffer for a new frame.
            super_t::HandleSpaceAvailableInTxBuffer();

            mState = kFinalizingFrame;

        case kFinalizingFrame:

            SuccessOrExit(mFrameEncoder.Finalize(mUartBuffer));

            mState = kStartingFrame;
        }
    }

exit:
    len = mUartBuffer.GetLength();

    if (len > 0)
    {
        otPlatUartSend(mUartBuffer.GetBuffer(), len);
    }
}

extern "C" void otPlatUartSendDone(void)
{
    sNcpUart->HandleUartSendDone();
}

void NcpUart::HandleUartSendDone(void)
{
    mUartBuffer.Clear();

    mUartSendTask.Post();
}

extern "C" void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    sNcpUart->HandleUartReceiveDone(aBuf, aBufLength);
}

void NcpUart::HandleUartReceiveDone(const uint8_t *aBuf, uint16_t aBufLength)
{
    mFrameDecoder.Decode(aBuf, aBufLength);
}

void NcpUart::HandleFrame(void *context, uint8_t *aBuf, uint16_t aBufLength)
{
    sNcpUart->HandleFrame(aBuf, aBufLength);
    (void)context;
}

void NcpUart::HandleFrame(uint8_t *aBuf, uint16_t aBufLength)
{
    super_t::HandleReceive(aBuf, aBufLength);
}

}  // namespace Thread
