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
#include <ncp/ncp.hpp>
#include <platform/serial.h>

namespace Thread {

static Ncp *sNcp;

Ncp::Ncp():
    NcpBase(),
    mFrameDecoder(mReceiveFrame, sizeof(mReceiveFrame), &HandleFrame, this)
{
    sNcp = this;
}

ThreadError Ncp::Start()
{
    otPlatSerialEnable();
    return super_t::Start();
}

ThreadError Ncp::Stop()
{
    otPlatSerialDisable();
    return super_t::Stop();
}

uint16_t
Ncp::OutboundFrameGetRemaining(void)
{
    return static_cast<int16_t>(sizeof(mSendFrame) - (mSendFrameIter - mSendFrame));
}

ThreadError
Ncp::OutboundFrameBegin(void)
{
    ThreadError errorCode;
    uint16_t outLength;

    mSendFrameIter = mSendFrame;
    outLength = OutboundFrameGetRemaining();

    errorCode = mFrameEncoder.Init(mSendFrameIter, outLength);

    if (errorCode == kThreadError_None)
    {
        mSendFrameIter += outLength;
    }

    return errorCode;
}

ThreadError
Ncp::OutboundFrameFeedData(const uint8_t *frame, uint16_t frameLength)
{
    ThreadError errorCode;
    uint16_t outLength(OutboundFrameGetRemaining());

    errorCode = mFrameEncoder.Encode(frame, frameLength, mSendFrameIter, outLength);

    if (errorCode == kThreadError_None)
    {
        mSendFrameIter += outLength;
    }

    return errorCode;
}

ThreadError
Ncp::OutboundFrameFeedMessage(Message &message)
{
    ThreadError errorCode;
    uint16_t inLength;
    uint16_t outLength;
    uint8_t inBuf[16];

    for (int offset = 0; offset < message.GetLength(); offset += sizeof(inBuf))
    {
        outLength = OutboundFrameGetRemaining();
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
Ncp::OutboundFrameSend(void)
{
    ThreadError errorCode;
    uint16_t outLength(OutboundFrameGetRemaining());

    errorCode = mFrameEncoder.Finalize(mSendFrameIter, outLength);

    if (errorCode == kThreadError_None)
    {
        mSendFrameIter += outLength;
        errorCode = otPlatSerialSend(mSendFrame, mSendFrameIter - mSendFrame);
    }

    if (errorCode == kThreadError_None)
    {
        mSending = true;
    }

    return errorCode;
}

extern "C" void otPlatSerialSendDone(void)
{
    sNcp->SendDoneTask();
}

void Ncp::SendDoneTask(void)
{
    mSending = false;

    if (mSendMessage)
    {
        Message::Free(*mSendMessage);
        mSendMessage = NULL;
    }

    super_t::HandleSendDone();
}

extern "C" void otPlatSerialReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    sNcp->ReceiveTask(aBuf, aBufLength);
}

void Ncp::ReceiveTask(const uint8_t *aBuf, uint16_t aBufLength)
{
    mFrameDecoder.Decode(aBuf, aBufLength);
}

void Ncp::HandleFrame(void *context, uint8_t *aBuf, uint16_t aBufLength)
{
    sNcp->HandleFrame(aBuf, aBufLength);
}

void Ncp::HandleFrame(uint8_t *aBuf, uint16_t aBufLength)
{
    super_t::HandleReceive(aBuf, aBufLength);
}

}  // namespace Thread
