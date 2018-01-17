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

#include "ncp_uart.hpp"

#include <stdio.h>

#include <openthread/ncp.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/uart.h>
#include <openthread/platform/misc.h>

#include "openthread-core-config.h"
#include "common/code_utils.hpp"
#include "common/new.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "net/ip6.hpp"

#if OPENTHREAD_ENABLE_NCP_UART

namespace ot {
namespace Ncp {

#if OPENTHREAD_ENABLE_NCP_VENDOR_HOOK == 0

static otDEFINE_ALIGNED_VAR(sNcpRaw, sizeof(NcpUart), uint64_t);

extern "C" void otNcpInit(otInstance *aInstance)
{
    NcpUart *ncpUart = NULL;
    Instance *instance = static_cast<Instance *>(aInstance);

    ncpUart = new(&sNcpRaw) NcpUart(instance);

    if (ncpUart == NULL || ncpUart != NcpBase::GetNcpInstance())
    {
        assert(false);
    }
}

#endif // OPENTHREAD_ENABLE_SPINEL_VENDOR_SUPPORT == 0

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

NcpUart::NcpUart(Instance *aInstance):
    NcpBase(aInstance),
    mFrameDecoder(mRxBuffer, sizeof(mRxBuffer), &NcpUart::HandleFrame, &NcpUart::HandleError, this),
    mUartBuffer(),
    mState(kStartingFrame),
    mByte(0),
    mUartSendImmediate(false),
    mUartSendTask(*aInstance, EncodeAndSendToUart, this)
#if OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
    , mTxFrameBufferEncrypterReader(mTxFrameBuffer)
#endif // OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
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
    bool prevHostPowerState;
#if OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
    NcpFrameBufferEncrypterReader &txFrameBuffer = mTxFrameBufferEncrypterReader;
#else
    NcpFrameBuffer &txFrameBuffer = mTxFrameBuffer;
#endif // OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER

    while (!txFrameBuffer.IsEmpty() || (mState == kFinalizingFrame))
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

            txFrameBuffer.OutFrameBegin();

            mState = kEncodingFrame;

            while (!txFrameBuffer.OutFrameHasEnded())
            {
                mByte = txFrameBuffer.OutFrameReadByte();

        case kEncodingFrame:

                SuccessOrExit(mFrameEncoder.Encode(mByte, mUartBuffer));
            }

            // track the change of mHostPowerStateInProgress by the
            // call to OutFrameRemove.
            prevHostPowerState = mHostPowerStateInProgress;

            txFrameBuffer.OutFrameRemove();

            if (prevHostPowerState && !mHostPowerStateInProgress)
            {
                // If mHostPowerStateInProgress transitioned from true -> false
                // in the call to OutFrameRemove, then the frame should be sent
                // out the UART without attempting to push any new frames into
                // the mUartBuffer. This is necessary to avoid prematurely calling
                // otPlatWakeHost.
                mUartSendImmediate = true;
            }

            mState = kFinalizingFrame;

            // fall through

        case kFinalizingFrame:

            SuccessOrExit(mFrameEncoder.Finalize(mUartBuffer));

            mState = kStartingFrame;

            if (mUartSendImmediate)
            {
                // clear state and break;
                mUartSendImmediate = false;
                break;
            }
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
#if OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
    size_t dataLen = aBufLength;
    if (SpinelEncrypter::DecryptInbound(aBuf, sizeof(mRxBuffer), &dataLen))
    {
        super_t::HandleReceive(aBuf, dataLen);
    }
#else
    super_t::HandleReceive(aBuf, aBufLength);
#endif // OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER
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

#if OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER

NcpUart::NcpFrameBufferEncrypterReader::NcpFrameBufferEncrypterReader(NcpFrameBuffer &aTxFrameBuffer) :
    mTxFrameBuffer(aTxFrameBuffer),
    mDataBufferReadIndex(0),
    mOutputDataLength(0) {}

bool NcpUart::NcpFrameBufferEncrypterReader::IsEmpty() const
{
    return mTxFrameBuffer.IsEmpty() && !mOutputDataLength;
}

otError NcpUart::NcpFrameBufferEncrypterReader::OutFrameBegin()
{
    otError status = OT_ERROR_FAILED;

    Reset();

    if ((status = mTxFrameBuffer.OutFrameBegin()) == OT_ERROR_NONE)
    {
        mOutputDataLength = mTxFrameBuffer.OutFrameGetLength();

        if (mOutputDataLength > 0)
        {
            assert(mOutputDataLength <= sizeof(mDataBuffer));
            mTxFrameBuffer.OutFrameRead(mOutputDataLength, mDataBuffer);

            if (!SpinelEncrypter::EncryptOutbound(mDataBuffer, sizeof(mDataBuffer), &mOutputDataLength))
            {
                mOutputDataLength = 0;
                status = OT_ERROR_FAILED;
            }
        }
        else
        {
            status = OT_ERROR_FAILED;
        }
    }

    return status;
}

bool NcpUart::NcpFrameBufferEncrypterReader::OutFrameHasEnded()
{
    return (mDataBufferReadIndex >= mOutputDataLength);
}

uint8_t NcpUart::NcpFrameBufferEncrypterReader::OutFrameReadByte()
{
    return mDataBuffer[mDataBufferReadIndex++];
}

otError NcpUart::NcpFrameBufferEncrypterReader::OutFrameRemove()
{
    return mTxFrameBuffer.OutFrameRemove();
}

void NcpUart::NcpFrameBufferEncrypterReader::Reset()
{
    mOutputDataLength = 0;
    mDataBufferReadIndex = 0;
}

#endif // OPENTHREAD_ENABLE_NCP_SPINEL_ENCRYPTER

}  // namespace Ncp
}  // namespace ot

#endif // OPENTHREAD_ENABLE_NCP_UART
