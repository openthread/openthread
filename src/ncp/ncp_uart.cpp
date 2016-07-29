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
 *   This file implements an HDLC interface to the Thread stack.
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

NcpUart::SendHdlcBuffer::SendHdlcBuffer(void)
    : BufferWriteIterator()
{
    Reset();
}

void
NcpUart::SendHdlcBuffer::Reset(void)
{
    mWritePointer = mBuffer;
    mRemainingLength = sizeof(mBuffer);
}

uint16_t
NcpUart::SendHdlcBuffer::GetLength(void) const
{
    return static_cast<uint16_t>(mWritePointer - mBuffer);
}

const uint8_t *
NcpUart::SendHdlcBuffer::GetBuffer(void) const
{
    return mBuffer;
}

uint16_t
NcpUart::SendHdlcBuffer::GetRemainingLength(void) const
{
    return mRemainingLength;
}

NcpUart::NcpUart():
    NcpBase(),
    mFrameDecoder(mReceiveFrame, sizeof(mReceiveFrame), &HandleFrame, this),
    mSendFrame()
{
}

uint16_t
NcpUart::OutboundFrameGetRemaining(void)
{
    return mSendFrame.GetRemainingLength();
}

ThreadError
NcpUart::OutboundFrameBegin(void)
{
    ThreadError errorCode;

    mSendFrame.Reset();

    errorCode = mFrameEncoder.Init(mSendFrame);

    return errorCode;
}

ThreadError
NcpUart::OutboundFrameFeedData(const uint8_t *frame, uint16_t frameLength)
{
    ThreadError errorCode;

    errorCode = mFrameEncoder.Encode(frame, frameLength, mSendFrame);

    return errorCode;
}

ThreadError
NcpUart::OutboundFrameFeedMessage(Message &message)
{
    ThreadError errorCode;
    uint16_t inLength;
    uint8_t inBuf[16];

    for (int offset = 0; offset < message.GetLength(); offset += sizeof(inBuf))
    {
        (void) OutboundFrameGetRemaining();
        inLength = message.Read(offset, sizeof(inBuf), inBuf);

        errorCode = OutboundFrameFeedData(inBuf, inLength);

        if (errorCode != kThreadError_None)
        {
            break;
        }
    }

    return errorCode;
}

ThreadError
NcpUart::OutboundFrameSend(void)
{
    ThreadError errorCode;

    errorCode = mFrameEncoder.Finalize(mSendFrame);

    if (errorCode == kThreadError_None)
    {
        // We go ahead and set this to `true` here in case
        // `otPlatUartSend()` ends up directly calling
        // `otPlatUartSendDone()`.
        mSending = true;

        errorCode = otPlatUartSend(mSendFrame.GetBuffer(), mSendFrame.GetLength());
    }

    if (errorCode != kThreadError_None)
    {
        mSending = false;
    }

    return errorCode;
}

extern "C" void otPlatUartSendDone(void)
{
    sNcpUart->SendDoneTask();
}

void NcpUart::SendDoneTask(void)
{
    mSending = false;

    super_t::HandleSendDone();
}

extern "C" void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    sNcpUart->ReceiveTask(aBuf, aBufLength);
}

void NcpUart::ReceiveTask(const uint8_t *aBuf, uint16_t aBufLength)
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

