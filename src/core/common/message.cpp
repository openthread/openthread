/*
 *  Copyright (c) 2016, Nest Labs, Inc.
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
 *   This file implements the message buffer pool and message buffers.
 */

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>
#include <openthreadcontext.h>

#ifdef WINDOWS_LOGGING
#include <message.tmh>
#endif

namespace Thread {

static Buffer *NewBuffer(otContext *aContext);
static ThreadError FreeBuffers(otContext *aContext, Buffer *aBuffer);
static ThreadError ReclaimBuffers(otContext *aContext, int aNumBuffers);

Buffer *NewBuffer(otContext *aContext)
{
    Buffer *buffer = NULL;

    if (aContext->mFreeBuffers == NULL)
    {
        otLogWarnMem("Ran out of buffers!");
    }

    VerifyOrExit(aContext->mFreeBuffers != NULL, ;);

    buffer = aContext->mFreeBuffers;
    aContext->mFreeBuffers = aContext->mFreeBuffers->GetNextBuffer();
    buffer->SetNextBuffer(NULL);
    aContext->mNumFreeBuffers--;

exit:
    return buffer;
}

ThreadError FreeBuffers(otContext *aContext, Buffer *aBuffer)
{
    Buffer *tmpBuffer;

    while (aBuffer != NULL)
    {
        tmpBuffer = aBuffer->GetNextBuffer();
        aBuffer->SetNextBuffer(aContext->mFreeBuffers);
        aContext->mFreeBuffers = aBuffer;
        aContext->mNumFreeBuffers++;
        aBuffer = tmpBuffer;
    }

    return kThreadError_None;
}

ThreadError ReclaimBuffers(otContext *aContext, int aNumBuffers)
{
    return (aNumBuffers <= aContext->mNumFreeBuffers) ? kThreadError_None : kThreadError_NoBufs;
}

ThreadError Message::Init(otContext *aContext)
{
    aContext->mFreeBuffers = aContext->mBuffers;

    for (int i = 0; i < kNumBuffers - 1; i++)
    {
        aContext->mBuffers[i].SetNextBuffer(&aContext->mBuffers[i + 1]);
    }

    aContext->mBuffers[kNumBuffers - 1].SetNextBuffer(NULL);
    aContext->mNumFreeBuffers = kNumBuffers;

    return kThreadError_None;
}

Message *Message::New(otContext *aContext, uint8_t aType, uint16_t aReserved)
{
    Message *message = NULL;

    VerifyOrExit((message = reinterpret_cast<Message *>(NewBuffer(aContext))) != NULL, ;);

    memset(message, 0, sizeof(*message));
    message->mHeader.mContext = aContext;
    message->SetType(aType);
    message->SetReserved(aReserved);
    message->SetLinkSecurityEnabled(true);

    VerifyOrExit(message->SetLength(0) == kThreadError_None, Message::Free(*message));

    return message;

exit:
    return NULL;
}

ThreadError Message::Free(Message &aMessage)
{
    assert(aMessage.GetMessageList(MessageInfo::kListAll).mList == NULL &&
           aMessage.GetMessageList(MessageInfo::kListInterface).mList == NULL);
    return FreeBuffers(aMessage.mHeader.mContext, reinterpret_cast<Buffer *>(&aMessage));
}

ThreadError Message::ResizeMessage(uint16_t aLength)
{
    // add buffers
    Buffer *curBuffer = this;
    Buffer *lastBuffer;
    uint16_t curLength = kHeadBufferDataSize;

    while (curLength < aLength)
    {
        if (curBuffer->GetNextBuffer() == NULL)
        {
            curBuffer->SetNextBuffer(NewBuffer(mHeader.mContext));
            VerifyOrExit(curBuffer->GetNextBuffer() != NULL, ;);
        }

        curBuffer = curBuffer->GetNextBuffer();
        curLength += kBufferDataSize;
    }

    // remove buffers
    lastBuffer = curBuffer;
    curBuffer = curBuffer->GetNextBuffer();
    lastBuffer->SetNextBuffer(NULL);

    FreeBuffers(mHeader.mContext, curBuffer);

    return kThreadError_None;

exit:
    return kThreadError_NoBufs;
}

Message *Message::GetNext(void) const
{
    return GetMessageList(MessageInfo::kListInterface).mNext;
}

uint16_t Message::GetLength(void) const
{
    return mInfo.mLength;
}

ThreadError Message::SetLength(uint16_t aLength)
{
    ThreadError error = kThreadError_None;
    uint16_t totalLengthRequest = GetReserved() + aLength;
    uint16_t totalLengthCurrent = GetReserved() + GetLength();
    int bufs = 0;

    if (totalLengthRequest > kHeadBufferDataSize)
    {
        bufs = (((totalLengthRequest - kHeadBufferDataSize) - 1) / kBufferDataSize) + 1;
    }

    if (totalLengthCurrent > kHeadBufferDataSize)
    {
        bufs -= (((totalLengthCurrent - kHeadBufferDataSize) - 1) / kBufferDataSize) + 1;
    }

    SuccessOrExit(error = ReclaimBuffers(mHeader.mContext, bufs));

    SuccessOrExit(error = ResizeMessage(totalLengthRequest));
    mInfo.mLength = aLength;

exit:
    return error;
}

uint16_t Message::GetOffset(void) const
{
    return mInfo.mOffset;
}

ThreadError Message::MoveOffset(int aDelta)
{
    ThreadError error = kThreadError_None;

    assert(GetOffset() + aDelta <= GetLength());
    VerifyOrExit(GetOffset() + aDelta <= GetLength(), error = kThreadError_InvalidArgs);

    mInfo.mOffset += (uint16_t)aDelta;
    assert(mInfo.mOffset <= GetLength());

exit:
    return error;
}

ThreadError Message::SetOffset(uint16_t aOffset)
{
    ThreadError error = kThreadError_None;

    assert(aOffset <= GetLength());
    VerifyOrExit(aOffset <= GetLength(), error = kThreadError_InvalidArgs);

    mInfo.mOffset = aOffset;

exit:
    return error;
}

uint8_t Message::GetType(void) const
{
    return mInfo.mType;
}

void Message::SetType(uint8_t aType)
{
    mInfo.mType = aType;
}

ThreadError Message::Append(const void *aBuf, uint16_t aLength)
{
    ThreadError error = kThreadError_None;
    uint16_t oldLength = GetLength();

    SuccessOrExit(error = SetLength(GetLength() + aLength));
    Write(oldLength, aLength, aBuf);

exit:
    return error;
}

ThreadError Message::Prepend(const void *aBuf, uint16_t aLength)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aLength <= GetReserved(), error = kThreadError_NoBufs);

    SetReserved(GetReserved() - aLength);
    mInfo.mLength += aLength;
    SetOffset(GetOffset() + aLength);

    Write(0, aLength, aBuf);

exit:
    return error;
}

int Message::Read(uint16_t aOffset, uint16_t aLength, void *aBuf) const
{
    Buffer *curBuffer;
    uint16_t bytesCopied = 0;
    uint16_t bytesToCopy;

    if (aOffset >= GetLength())
    {
        ExitNow();
    }

    if (aOffset + aLength >= GetLength())
    {
        aLength = GetLength() - aOffset;
    }

    aOffset += GetReserved();

    // special case first buffer
    if (aOffset < kHeadBufferDataSize)
    {
        bytesToCopy = kHeadBufferDataSize - aOffset;

        if (bytesToCopy > aLength)
        {
            bytesToCopy = aLength;
        }

        memcpy(aBuf, GetFirstData() + aOffset, bytesToCopy);

        aLength -= bytesToCopy;
        bytesCopied += bytesToCopy;
        aBuf = reinterpret_cast<uint8_t *>(aBuf) + bytesToCopy;

        aOffset = 0;
    }
    else
    {
        aOffset -= kHeadBufferDataSize;
    }

    // advance to offset
    curBuffer = GetNextBuffer();

    while (aOffset >= kBufferDataSize)
    {
        assert(curBuffer != NULL);

        curBuffer = curBuffer->GetNextBuffer();
        aOffset -= kBufferDataSize;
    }

    // begin copy
    while (aLength > 0)
    {
        assert(curBuffer != NULL);

        bytesToCopy = kBufferDataSize - aOffset;

        if (bytesToCopy > aLength)
        {
            bytesToCopy = aLength;
        }

        memcpy(aBuf, curBuffer->GetData() + aOffset, bytesToCopy);

        aLength -= bytesToCopy;
        bytesCopied += bytesToCopy;
        aBuf = reinterpret_cast<uint8_t *>(aBuf) + bytesToCopy;

        curBuffer = curBuffer->GetNextBuffer();
        aOffset = 0;
    }

exit:
    return bytesCopied;
}

int Message::Write(uint16_t aOffset, uint16_t aLength, const void *aBuf)
{
    Buffer *curBuffer;
    uint16_t bytesCopied = 0;
    uint16_t bytesToCopy;

    assert(aOffset + aLength <= GetLength());

    if (aOffset + aLength >= GetLength())
    {
        aLength = GetLength() - aOffset;
    }

    aOffset += GetReserved();

    // special case first buffer
    if (aOffset < kHeadBufferDataSize)
    {
        bytesToCopy = kHeadBufferDataSize - aOffset;

        if (bytesToCopy > aLength)
        {
            bytesToCopy = aLength;
        }

        memcpy(GetFirstData() + aOffset, aBuf, bytesToCopy);

        aLength -= bytesToCopy;
        bytesCopied += bytesToCopy;
        aBuf = reinterpret_cast<const uint8_t *>(aBuf) + bytesToCopy;

        aOffset = 0;
    }
    else
    {
        aOffset -= kHeadBufferDataSize;
    }

    // advance to offset
    curBuffer = GetNextBuffer();

    while (aOffset >= kBufferDataSize)
    {
        assert(curBuffer != NULL);

        curBuffer = curBuffer->GetNextBuffer();
        aOffset -= kBufferDataSize;
    }

    // begin copy
    while (aLength > 0)
    {
        assert(curBuffer != NULL);

        bytesToCopy = kBufferDataSize - aOffset;

        if (bytesToCopy > aLength)
        {
            bytesToCopy = aLength;
        }

        memcpy(curBuffer->GetData() + aOffset, aBuf, bytesToCopy);

        aLength -= bytesToCopy;
        bytesCopied += bytesToCopy;
        aBuf = reinterpret_cast<const uint8_t *>(aBuf) + bytesToCopy;

        curBuffer = curBuffer->GetNextBuffer();
        aOffset = 0;
    }

    return bytesCopied;
}

int Message::CopyTo(uint16_t aSourceOffset, uint16_t aDestinationOffset, uint16_t aLength, Message &aMessage)
{
    uint16_t bytesCopied = 0;
    uint16_t bytesToCopy;
    uint8_t buf[16];

    while (aLength > 0)
    {
        bytesToCopy = (aLength < sizeof(buf)) ? aLength : sizeof(buf);

        Read(aSourceOffset, bytesToCopy, buf);
        aMessage.Write(aDestinationOffset, bytesToCopy, buf);

        aSourceOffset += bytesToCopy;
        aDestinationOffset += bytesToCopy;
        aLength -= bytesToCopy;
        bytesCopied += bytesToCopy;
    }

    return bytesCopied;
}

uint16_t Message::GetDatagramTag(void) const
{
    return mInfo.mDatagramTag;
}

void Message::SetDatagramTag(uint16_t aTag)
{
    mInfo.mDatagramTag = aTag;
}

bool Message::GetChildMask(uint8_t aChildIndex) const
{
    assert(aChildIndex < sizeof(mInfo.mChildMask) * 8);
    return (mInfo.mChildMask[aChildIndex / 8] & (0x80 >> (aChildIndex % 8))) != 0;
}

void Message::ClearChildMask(uint8_t aChildIndex)
{
    assert(aChildIndex < sizeof(mInfo.mChildMask) * 8);
    mInfo.mChildMask[aChildIndex / 8] &= ~(0x80 >> (aChildIndex % 8));
}

void Message::SetChildMask(uint8_t aChildIndex)
{
    assert(aChildIndex < sizeof(mInfo.mChildMask) * 8);
    mInfo.mChildMask[aChildIndex / 8] |= 0x80 >> (aChildIndex % 8);
}

bool Message::IsChildPending(void) const
{
    bool rval = false;

    for (size_t i = 0; i < sizeof(mInfo.mChildMask); i++)
    {
        if (mInfo.mChildMask[i] != 0)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
}

uint16_t Message::GetPanId(void) const
{
    return mInfo.mPanId;
}

void Message::SetPanId(uint16_t aPanId)
{
    mInfo.mPanId = aPanId;
}

uint8_t Message::GetTimeout(void) const
{
    return mInfo.mTimeout;
}

void Message::SetTimeout(uint8_t aTimeout)
{
    mInfo.mTimeout = aTimeout;
}

bool Message::GetDirectTransmission(void) const
{
    return mInfo.mDirectTx;
}

void Message::ClearDirectTransmission(void)
{
    mInfo.mDirectTx = false;
}

void Message::SetDirectTransmission(void)
{
    mInfo.mDirectTx = true;
}

bool Message::IsLinkSecurityEnabled(void) const
{
    return mInfo.mLinkSecurity;
}

void Message::SetLinkSecurityEnabled(bool aLinkSecurityEnabled)
{
    mInfo.mLinkSecurity = aLinkSecurityEnabled;
}

bool Message::IsMleDiscoverRequest(void) const
{
    return mInfo.mMleDiscoverRequest;
}

void Message::SetMleDiscoverRequest(bool aMleDiscoverRequest)
{
    mInfo.mMleDiscoverRequest = aMleDiscoverRequest;
}

bool Message::IsMleDiscoverResponse(void) const
{
    return mInfo.mMleDiscoverResponse;
}

void Message::SetMleDiscoverResponse(bool aMleDiscoverResponse)
{
    mInfo.mMleDiscoverResponse = aMleDiscoverResponse;
}

uint16_t Message::UpdateChecksum(uint16_t aChecksum, uint16_t aOffset, uint16_t aLength) const
{
    Buffer *curBuffer;
    uint16_t bytesCovered = 0;
    uint16_t bytesToCover;

    assert(aOffset + aLength <= GetLength());

    aOffset += GetReserved();

    // special case first buffer
    if (aOffset < kHeadBufferDataSize)
    {
        bytesToCover = kHeadBufferDataSize - aOffset;

        if (bytesToCover > aLength)
        {
            bytesToCover = aLength;
        }

        aChecksum = Ip6::Ip6::UpdateChecksum(aChecksum, GetFirstData() + aOffset, bytesToCover);

        aLength -= bytesToCover;
        bytesCovered += bytesToCover;

        aOffset = 0;
    }
    else
    {
        aOffset -= kHeadBufferDataSize;
    }

    // advance to offset
    curBuffer = GetNextBuffer();

    while (aOffset >= kBufferDataSize)
    {
        assert(curBuffer != NULL);

        curBuffer = curBuffer->GetNextBuffer();
        aOffset -= kBufferDataSize;
    }

    // begin copy
    while (aLength > 0)
    {
        assert(curBuffer != NULL);

        bytesToCover = kBufferDataSize - aOffset;

        if (bytesToCover > aLength)
        {
            bytesToCover = aLength;
        }

        aChecksum = Ip6::Ip6::UpdateChecksum(aChecksum, curBuffer->GetData() + aOffset, bytesToCover);

        aLength -= bytesToCover;
        bytesCovered += bytesToCover;

        curBuffer = curBuffer->GetNextBuffer();
        aOffset = 0;
    }

    return aChecksum;
}

uint16_t Message::GetReserved(void) const
{
    return mInfo.mReserved;
}

void Message::SetReserved(uint16_t aReserved)
{
    mInfo.mReserved = aReserved;
}

MessageQueue::MessageQueue(otContext *aContext) :
    mContext(aContext)
{
    mInterface.mHead = NULL;
    mInterface.mTail = NULL;
}

ThreadError MessageQueue::AddToList(int _aList, Message &aMessage)
{
    uint8_t aList = (uint8_t)_aList;
    MessageList *list;

    assert(aMessage.GetMessageList(aList).mNext == NULL &&
           aMessage.GetMessageList(aList).mPrev == NULL &&
           aMessage.GetMessageList(aList).mList != NULL);

    list = aMessage.GetMessageList(aList).mList;

    if (list->mHead == NULL)
    {
        list->mHead = &aMessage;
        list->mTail = &aMessage;
    }
    else
    {
        list->mTail->GetMessageList(aList).mNext = &aMessage;
        aMessage.GetMessageList(aList).mPrev = list->mTail;
        list->mTail = &aMessage;
    }

    return kThreadError_None;
}

ThreadError MessageQueue::RemoveFromList(int _aList, Message &aMessage)
{
    uint8_t aList = (uint8_t)_aList;
    MessageList *list;

    assert(aMessage.GetMessageList(aList).mList != NULL);

    list = aMessage.GetMessageList(aList).mList;

    assert(list->mHead == &aMessage ||
           aMessage.GetMessageList(aList).mNext != NULL ||
           aMessage.GetMessageList(aList).mPrev != NULL);

    if (aMessage.GetMessageList(aList).mPrev)
    {
        aMessage.GetMessageList(aList).mPrev->GetMessageList(aList).mNext = aMessage.GetMessageList(aList).mNext;
    }
    else
    {
        list->mHead = aMessage.GetMessageList(aList).mNext;
    }

    if (aMessage.GetMessageList(aList).mNext)
    {
        aMessage.GetMessageList(aList).mNext->GetMessageList(aList).mPrev = aMessage.GetMessageList(aList).mPrev;
    }
    else
    {
        list->mTail = aMessage.GetMessageList(aList).mPrev;
    }

    aMessage.GetMessageList(aList).mPrev = NULL;
    aMessage.GetMessageList(aList).mNext = NULL;

    return kThreadError_None;
}

Message *MessageQueue::GetHead(void) const
{
    return mInterface.mHead;
}

ThreadError MessageQueue::Enqueue(Message &aMessage)
{
    aMessage.GetMessageList(MessageInfo::kListAll).mList = &mContext->mAll;
    aMessage.GetMessageList(MessageInfo::kListInterface).mList = &mInterface;
    AddToList(MessageInfo::kListAll, aMessage);
    AddToList(MessageInfo::kListInterface, aMessage);
    return kThreadError_None;
}

ThreadError MessageQueue::Dequeue(Message &aMessage)
{
    RemoveFromList(MessageInfo::kListAll, aMessage);
    RemoveFromList(MessageInfo::kListInterface, aMessage);
    aMessage.GetMessageList(MessageInfo::kListAll).mList = NULL;
    aMessage.GetMessageList(MessageInfo::kListInterface).mList = NULL;
    return kThreadError_None;
}

}  // namespace Thread
