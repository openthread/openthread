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

#include <openthread/config.h>

#include "ncp_uart.hpp"

#include <stdio.h>

#include <openthread/ncp.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/uart.h>
#include <openthread/platform/misc.h>

#include "openthread-core-config.h"
#include "openthread-instance.h"
#include "common/code_utils.hpp"
#include "common/new.hpp"
#include "common/debug.hpp"
#include "net/ip6.hpp"

#if OPENTHREAD_ENABLE_NCP_UART

namespace ot {
namespace Ncp {

static otDEFINE_ALIGNED_VAR(sNcpRaw, sizeof(NcpUart), uint64_t);

extern "C" void otNcpInit(otInstance *aInstance)
{
    NcpUart *ncpUart = NULL;
    ncpUart = new(&sNcpRaw) NcpUart(aInstance);

    if (ncpUart == NULL || ncpUart != NcpBase::GetNcpInstance())
    {
        assert(false);
    }
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

NcpUart::NcpUart(otInstance *aInstance):
    NcpBase(aInstance),
    mFrameDecoder(mRxBuffer, sizeof(mRxBuffer), &NcpUart::HandleFrame, &NcpUart::HandleError, this),
    mUartBuffer(),
    mState(kStartingFrame),
    mByte(0),
    mUartSendTask(*aInstance, EncodeAndSendToUart, this)
{
    mTxFrameBuffer.SetFrameAddedCallback(HandleFrameAddedToNcpBuffer, this);

    otPlatUartEnable();
}

void NcpUart::HandleFrameAddedToNcpBuffer(void *aContext, NcpFrameBuffer::FrameTag aTag,
                                          NcpFrameBuffer::Priority aPriority, NcpFrameBuffer *aNcpFrameBuffer)
{
    OT_UNUSED_VARIABLE(aNcpFrameBuffer);
    OT_UNUSED_VARIABLE(aTag);
    OT_UNUSED_VARIABLE(aPriority);

    static_cast<NcpUart *>(aContext)->HandleFrameAddedToNcpBuffer();
}

void NcpUart::HandleFrameAddedToNcpBuffer(void)
{
    if (mUartBuffer.IsEmpty())
    {
        mUartSendTask.Post();
    }
}

void NcpUart::EncodeAndSendToUart(Tasklet &aTasklet)
{
    OT_UNUSED_VARIABLE(aTasklet);
    static_cast<NcpUart *>(GetNcpInstance())->EncodeAndSendToUart();
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

            if (super_t::ShouldWakeHost())
            {
                otPlatWakeHost();
            }

            VerifyOrExit(super_t::ShouldDeferHostSend() == false);
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

            mState = kFinalizingFrame;

            // fall through

        case kFinalizingFrame:

            SuccessOrExit(mFrameEncoder.Finalize(mUartBuffer));

            mState = kStartingFrame;
        }
    }

exit:
    len = mUartBuffer.GetLength();

    if (len > 0)
    {
        if (otPlatUartSend(mUartBuffer.GetBuffer(), len) != OT_ERROR_NONE)
        {
            assert(false);
        }
    }
}

extern "C" void otPlatUartSendDone(void)
{
    NcpUart *ncpUart = static_cast<NcpUart *>(NcpBase::GetNcpInstance());

    if (ncpUart != NULL)
    {
        ncpUart->HandleUartSendDone();
    }
}

void NcpUart::HandleUartSendDone(void)
{
    mUartBuffer.Clear();

    mUartSendTask.Post();
}

extern "C" void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    NcpUart *ncpUart = static_cast<NcpUart *>(NcpBase::GetNcpInstance());

    if (ncpUart != NULL)
    {
        ncpUart->HandleUartReceiveDone(aBuf, aBufLength);
    }
}

void NcpUart::HandleUartReceiveDone(const uint8_t *aBuf, uint16_t aBufLength)
{
    mFrameDecoder.Decode(aBuf, aBufLength);
}

void NcpUart::HandleFrame(void *aContext, uint8_t *aBuf, uint16_t aBufLength)
{
    static_cast<NcpUart *>(aContext)->HandleFrame(aBuf, aBufLength);
}

void NcpUart::HandleFrame(uint8_t *aBuf, uint16_t aBufLength)
{
    super_t::HandleReceive(aBuf, aBufLength);
}

void NcpUart::HandleError(void *aContext, otError aError, uint8_t *aBuf, uint16_t aBufLength)
{
    static_cast<NcpUart *>(aContext)->HandleError(aError, aBuf, aBufLength);
}

void NcpUart::HandleError(otError aError, uint8_t *aBuf, uint16_t aBufLength)
{
    char hexbuf[128];
    uint16_t i = 0;

    super_t::IncrementFrameErrorCounter();

    // We can get away with sprintf because we know
    // `hexbuf` is large enough.
    snprintf(hexbuf, sizeof(hexbuf), "Framing error %d: [", aError);

    // Write out the first part of our log message.
    otNcpStreamWrite(0, reinterpret_cast<uint8_t *>(hexbuf), static_cast<int>(strlen(hexbuf)));

    // The first '3' comes from the trailing "]\n\000" at the end o the string.
    // The second '3' comes from the length of two hex digits and a space.
    for (i = 0; (i < aBufLength) && (i < (sizeof(hexbuf) - 3) / 3); i++)
    {
        // We can get away with sprintf because we know
        // `hexbuf` is large enough, based on our calculations
        // above.
        snprintf(&hexbuf[i * 3], sizeof(hexbuf) - i * 3, " %02X", static_cast<uint8_t>(aBuf[i]));
    }

    // Append a final closing bracket and newline character
    // so our log line looks nice.
    snprintf(&hexbuf[i * 3], sizeof(hexbuf) - i * 3, "]\n");

    // Write out the second part of our log message.
    // We skip the first byte since it has a space in it.
    otNcpStreamWrite(0, reinterpret_cast<uint8_t *>(hexbuf + 1), static_cast<int>(strlen(hexbuf) - 1));
}

#if OPENTHREAD_CONFIG_ENABLE_DEFAULT_LOG_OUTPUT
#ifdef __cplusplus
extern "C" {
#endif
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    char logString[128];
    int charsWritten;
    va_list args;

    va_start(args, aFormat);

    if ((charsWritten = vsnprintf(logString, sizeof(logString), aFormat, args)) > 0)
    {
        if (charsWritten > static_cast<int>(sizeof(logString) - 1))
        {
            charsWritten = static_cast<int>(sizeof(logString) - 1);
        }

        otNcpStreamWrite(0, reinterpret_cast<uint8_t *>(logString), charsWritten);
    }

    va_end(args);

    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
}
#ifdef __cplusplus
}  // extern "C"
#endif
#endif // OPENTHREAD_ENABLE_CLI_LOGGING

}  // namespace Ncp
}  // namespace ot

#endif // OPENTHREAD_ENABLE_NCP_UART
