/*
 *  Copyright (c) 2022, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements TCP/IPv6 socket extensions.
 */

#include "tcp6_ext.hpp"

#if OPENTHREAD_CONFIG_TCP_ENABLE

#include "instance/instance.hpp"

namespace ot {
namespace Ip6 {

RegisterLogModule("TcpExt");

void TcpCircularSendBuffer::Initialize(void *aDataBuffer, size_t aCapacity)
{
    mDataBuffer = static_cast<uint8_t *>(aDataBuffer);
    mCapacity   = aCapacity;
    ForceDiscardAll();
}

Error TcpCircularSendBuffer::Write(Tcp::Endpoint &aEndpoint,
                                   const void    *aData,
                                   size_t         aLength,
                                   size_t        &aWritten,
                                   uint32_t       aFlags)
{
    Error    error     = kErrorNone;
    size_t   bytesFree = GetFreeSpace();
    size_t   writeIndex;
    uint32_t flags = 0;
    size_t   bytesUntilWrap;

    /*
     * Handle the case where we don't have enough space to accommodate all of the
     * provided data.
     */
    aLength = Min(aLength, bytesFree);
    VerifyOrExit(aLength != 0);

    /*
     * This is a "simplifying" if statement the removes an edge case from the logic
     * below. It guarantees that a write to an empty buffer will never wrap.
     */
    if (mCapacityUsed == 0)
    {
        mStartIndex = 0;
    }

    writeIndex = GetIndex(mStartIndex, mCapacityUsed);

    if ((aFlags & OT_TCP_CIRCULAR_SEND_BUFFER_WRITE_MORE_TO_COME) != 0 && aLength < bytesFree)
    {
        flags |= OT_TCP_SEND_MORE_TO_COME;
    }

    bytesUntilWrap = mCapacity - writeIndex;
    if (aLength <= bytesUntilWrap)
    {
        memcpy(&mDataBuffer[writeIndex], aData, aLength);
        if (writeIndex == 0)
        {
            /*
             * mCapacityUsed == 0 corresponds to the case where we're writing
             * to an empty buffer. mCapacityUsed != 0 && writeIndex == 0
             * corresponds to the case where the buffer is not empty and this is
             * writing the first bytes that wrap.
             */
            uint8_t linkIndex;
            if (mCapacityUsed == 0)
            {
                linkIndex = mFirstSendLinkIndex;
            }
            else
            {
                linkIndex = 1 - mFirstSendLinkIndex;
            }
            {
                otLinkedBuffer &dataSendLink = mSendLinks[linkIndex];

                dataSendLink.mNext   = nullptr;
                dataSendLink.mData   = &mDataBuffer[writeIndex];
                dataSendLink.mLength = aLength;

                LogDebg("Appending link %u (points to index %u, length %u)", static_cast<unsigned>(linkIndex),
                        static_cast<unsigned>(writeIndex), static_cast<unsigned>(aLength));
                error = aEndpoint.SendByReference(dataSendLink, flags);
            }
        }
        else
        {
            LogDebg("Extending tail link by length %u", static_cast<unsigned>(aLength));
            error = aEndpoint.SendByExtension(aLength, flags);
        }
        VerifyOrExit(error == kErrorNone, aLength = 0);
    }
    else
    {
        const uint8_t *dataIndexable = static_cast<const uint8_t *>(aData);
        size_t         bytesWrapped  = aLength - bytesUntilWrap;

        memcpy(&mDataBuffer[writeIndex], &dataIndexable[0], bytesUntilWrap);
        memcpy(&mDataBuffer[0], &dataIndexable[bytesUntilWrap], bytesWrapped);

        /*
         * Because of the "simplifying" if statement at the top, we don't
         * have to worry about starting from an empty buffer in this case.
         */
        LogDebg("Extending tail link by length %u (wrapping)", static_cast<unsigned>(bytesUntilWrap));
        error = aEndpoint.SendByExtension(bytesUntilWrap, flags | OT_TCP_SEND_MORE_TO_COME);
        VerifyOrExit(error == kErrorNone, aLength = 0);

        {
            otLinkedBuffer &wrappedDataSendLink = mSendLinks[1 - mFirstSendLinkIndex];

            wrappedDataSendLink.mNext   = nullptr;
            wrappedDataSendLink.mData   = &mDataBuffer[0];
            wrappedDataSendLink.mLength = bytesWrapped;

            LogDebg("Appending link %u (wrapping)", static_cast<unsigned>(1 - mFirstSendLinkIndex));
            error = aEndpoint.SendByReference(wrappedDataSendLink, flags);
            VerifyOrExit(error == kErrorNone, aLength = bytesUntilWrap);
        }
    }

exit:
    mCapacityUsed += aLength;
    aWritten = aLength;
    return error;
}

void TcpCircularSendBuffer::HandleForwardProgress(size_t aInSendBuffer)
{
    size_t bytesRemoved;
    size_t bytesUntilWrap;

    OT_ASSERT(aInSendBuffer <= mCapacityUsed);
    LogDebg("Forward progress: %u bytes in send buffer\n", static_cast<unsigned>(aInSendBuffer));
    bytesRemoved   = mCapacityUsed - aInSendBuffer;
    bytesUntilWrap = mCapacity - mStartIndex;

    if (bytesRemoved < bytesUntilWrap)
    {
        mStartIndex += bytesRemoved;
    }
    else
    {
        mStartIndex = bytesRemoved - bytesUntilWrap;
        /* The otLinkedBuffer for the pre-wrap data is now empty. */
        LogDebg("Pre-wrap linked buffer now empty: switching first link index from %u to %u\n",
                static_cast<unsigned>(mFirstSendLinkIndex), static_cast<unsigned>(1 - mFirstSendLinkIndex));
        mFirstSendLinkIndex = 1 - mFirstSendLinkIndex;
    }
    mCapacityUsed = aInSendBuffer;
}

size_t TcpCircularSendBuffer::GetFreeSpace(void) const { return mCapacity - mCapacityUsed; }

void TcpCircularSendBuffer::ForceDiscardAll(void)
{
    mStartIndex         = 0;
    mCapacityUsed       = 0;
    mFirstSendLinkIndex = 0;
}

Error TcpCircularSendBuffer::Deinitialize(void) { return (mCapacityUsed != 0) ? kErrorBusy : kErrorNone; }

size_t TcpCircularSendBuffer::GetIndex(size_t aStart, size_t aOffsetFromStart) const
{
    size_t bytesUntilWrap;
    size_t index;

    OT_ASSERT(aStart < mCapacity);
    bytesUntilWrap = mCapacity - aStart;
    if (aOffsetFromStart < bytesUntilWrap)
    {
        index = aStart + aOffsetFromStart;
    }
    else
    {
        index = aOffsetFromStart - bytesUntilWrap;
    }

    return index;
}

} // namespace Ip6
} // namespace ot

#endif // OPENTHREAD_CONFIG_TCP_ENABLE
