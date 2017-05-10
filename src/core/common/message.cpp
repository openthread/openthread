/*
 *  Copyright (c) 2016, The OpenThread Authors.
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

#define WPP_NAME "message.tmh"

#include  "openthread-enable-defines.h"

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>

namespace ot {

MessagePool::MessagePool(otInstance *aInstance) :
    mInstance(aInstance),
    mAllQueue()
{
#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    // Initialize Platform buffer pool management.
    otPlatMessagePoolInit(mInstance, kNumBuffers, sizeof(Buffer));
#else
    memset(mBuffers, 0, sizeof(mBuffers));

    mFreeBuffers = mBuffers;

    for (uint16_t i = 0; i < kNumBuffers - 1; i++)
    {
        mBuffers[i].SetNextBuffer(&mBuffers[i + 1]);
    }

    mBuffers[kNumBuffers - 1].SetNextBuffer(NULL);
    mNumFreeBuffers = kNumBuffers;
#endif

    // This is required to remove warning of "unused member variable".
    (void)mInstance;
}

Message *MessagePool::New(uint8_t aType, uint16_t aReserved)
{
    Message *message = NULL;

    VerifyOrExit((message = static_cast<Message *>(NewBuffer())) != NULL);

    memset(message, 0, sizeof(*message));
    message->SetMessagePool(this);
    message->SetType(aType);
    message->SetReserved(aReserved);
    message->SetLinkSecurityEnabled(true);
    message->SetPriority(kDefaultMessagePriority);

    if (message->SetLength(0) != kThreadError_None)
    {
        Free(message);
        message = NULL;
    }

exit:
    return message;
}

ThreadError MessagePool::Free(Message *aMessage)
{
    assert(aMessage->Next(MessageInfo::kListAll) == NULL &&
           aMessage->Prev(MessageInfo::kListAll) == NULL);

    assert(aMessage->Next(MessageInfo::kListInterface) == NULL &&
           aMessage->Prev(MessageInfo::kListInterface) == NULL);

    return FreeBuffers(static_cast<Buffer *>(aMessage));
}

Buffer *MessagePool::NewBuffer(void)
{
    Buffer *buffer = NULL;

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT

    buffer = static_cast<Buffer *>(otPlatMessagePoolNew(mInstance));

#else

    if (mFreeBuffers != NULL)
    {
        buffer = mFreeBuffers;
        mFreeBuffers = mFreeBuffers->GetNextBuffer();
        buffer->SetNextBuffer(NULL);
        mNumFreeBuffers--;
    }

#endif

    if (buffer == NULL)
    {
        otLogInfoMem(mInstance, "No available message buffer");
    }

    return buffer;
}

ThreadError MessagePool::FreeBuffers(Buffer *aBuffer)
{
    Buffer *tmpBuffer;

    while (aBuffer != NULL)
    {
        tmpBuffer = aBuffer->GetNextBuffer();
#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
        otPlatMessagePoolFree(mInstance, aBuffer);
#else // OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
        aBuffer->SetNextBuffer(mFreeBuffers);
        mFreeBuffers = aBuffer;
        mNumFreeBuffers++;
#endif // OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
        aBuffer = tmpBuffer;
    }

    return kThreadError_None;
}

ThreadError MessagePool::ReclaimBuffers(int aNumBuffers)
{
    uint16_t numFreeBuffers;

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    numFreeBuffers = otPlatMessagePoolNumFreeBuffers(mInstance);
#else
    numFreeBuffers = mNumFreeBuffers;
#endif

    //First comparison is to get around issues with comparing
    //signed and unsigned numbers, if aNumBuffers is negative then
    //the second comparison wont be attempted.
    if (aNumBuffers < 0 || aNumBuffers <= numFreeBuffers)
    {
        return kThreadError_None;
    }
    else
    {
        return kThreadError_NoBufs;
    }
}

Message *MessagePool::Iterator::Next(void) const
{
    Message *next;

    VerifyOrExit(mMessage != NULL, next = NULL);

    if (mMessage == mMessage->GetMessagePool()->GetAllMessagesTail().GetMessage())
    {
        next = NULL;
    }
    else
    {
        next = mMessage->Next(MessageInfo::kListAll);
    }

exit:
    return next;
}

Message *MessagePool::Iterator::Prev(void) const
{
    Message *prev;

    VerifyOrExit(mMessage != NULL, prev = NULL);

    if (mMessage == mMessage->GetMessagePool()->GetAllMessagesHead().GetMessage())
    {
        prev = NULL;
    }
    else
    {
        prev = mMessage->Prev(MessageInfo::kListAll);
    }

exit:
    return prev;
}

MessagePool::Iterator MessagePool::GetAllMessagesHead(void) const
{
    Message *head;
    Message *tail;

    tail = GetAllMessagesTail().GetMessage();

    if (tail != NULL)
    {
        head = tail->Next(MessageInfo::kListAll);
    }
    else
    {
        head = NULL;
    }

    return Iterator(head);
}

ThreadError Message::ResizeMessage(uint16_t aLength)
{
    ThreadError error = kThreadError_None;

    // add buffers
    Buffer *curBuffer = this;
    Buffer *lastBuffer;
    uint16_t curLength = kHeadBufferDataSize;

    while (curLength < aLength)
    {
        if (curBuffer->GetNextBuffer() == NULL)
        {
            curBuffer->SetNextBuffer(GetMessagePool()->NewBuffer());
            VerifyOrExit(curBuffer->GetNextBuffer() != NULL, error = kThreadError_NoBufs);
        }

        curBuffer = curBuffer->GetNextBuffer();
        curLength += kBufferDataSize;
    }

    // remove buffers
    lastBuffer = curBuffer;
    curBuffer = curBuffer->GetNextBuffer();
    lastBuffer->SetNextBuffer(NULL);

    GetMessagePool()->FreeBuffers(curBuffer);

exit:
    return error;
}

ThreadError Message::Free(void)
{
    return GetMessagePool()->Free(this);
}

Message *Message::GetNext(void) const
{
    Message *next;
    Message *tail;

    if (mBuffer.mHead.mInfo.mInPriorityQ)
    {
        PriorityQueue *priorityQueue = GetPriorityQueue();
        VerifyOrExit(priorityQueue != NULL, next = NULL);
        tail = priorityQueue->GetTail();
    }
    else
    {
        MessageQueue *messageQueue = GetMessageQueue();
        VerifyOrExit(messageQueue != NULL, next = NULL);
        tail = messageQueue->GetTail();
    }

    next = (this == tail) ? NULL : Next(MessageInfo::kListInterface);

exit:
    return next;
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

    SuccessOrExit(error = GetMessagePool()->ReclaimBuffers(bufs));

    SuccessOrExit(error = ResizeMessage(totalLengthRequest));
    mBuffer.mHead.mInfo.mLength = aLength;

exit:
    return error;
}

uint8_t Message::GetBufferCount(void) const
{
    uint8_t rval = 1;

    for (const Buffer *curBuffer = GetNextBuffer(); curBuffer; curBuffer = curBuffer->GetNextBuffer())
    {
        rval++;
    }

    return rval;
}

ThreadError Message::MoveOffset(int aDelta)
{
    ThreadError error = kThreadError_None;

    assert(GetOffset() + aDelta <= GetLength());
    VerifyOrExit(GetOffset() + aDelta <= GetLength(), error = kThreadError_InvalidArgs);

    mBuffer.mHead.mInfo.mOffset += static_cast<int16_t>(aDelta);
    assert(mBuffer.mHead.mInfo.mOffset <= GetLength());

exit:
    return error;
}

ThreadError Message::SetOffset(uint16_t aOffset)
{
    ThreadError error = kThreadError_None;

    assert(aOffset <= GetLength());
    VerifyOrExit(aOffset <= GetLength(), error = kThreadError_InvalidArgs);

    mBuffer.mHead.mInfo.mOffset = aOffset;

exit:
    return error;
}

bool Message::IsSubTypeMle(void) const
{
    bool rval = false;

    if (mBuffer.mHead.mInfo.mSubType == kSubTypeMleAnnounce ||
        mBuffer.mHead.mInfo.mSubType == kSubTypeMleDiscoverRequest ||
        mBuffer.mHead.mInfo.mSubType == kSubTypeMleDiscoverResponse ||
        mBuffer.mHead.mInfo.mSubType == kSubTypeMleChildUpdateRequest ||
        mBuffer.mHead.mInfo.mSubType == kSubTypeMleGeneral)
    {
        rval = true;
    }

    return rval;
}

ThreadError Message::SetPriority(uint8_t aPriority)
{
    ThreadError error = kThreadError_None;
    PriorityQueue *priorityQueue = NULL;

    VerifyOrExit(aPriority < kNumPriorities, error = kThreadError_InvalidArgs);

    VerifyOrExit(IsInAQueue(), mBuffer.mHead.mInfo.mPriority = aPriority);
    VerifyOrExit(mBuffer.mHead.mInfo.mPriority != aPriority);

    if (mBuffer.mHead.mInfo.mInPriorityQ)
    {
        priorityQueue = mBuffer.mHead.mInfo.mPriorityQueue;
        priorityQueue->Dequeue(*this);
    }
    else
    {
        GetMessagePool()->GetAllMessagesQueue()->RemoveFromList(MessageInfo::kListAll, *this);
    }

    mBuffer.mHead.mInfo.mPriority = aPriority;

    if (priorityQueue != NULL)
    {
        priorityQueue->Enqueue(*this);
    }
    else
    {
        GetMessagePool()->GetAllMessagesQueue()->AddToList(MessageInfo::kListAll, *this);
    }

exit:
    return error;
}

ThreadError Message::Append(const void *aBuf, uint16_t aLength)
{
    ThreadError error = kThreadError_None;
    uint16_t oldLength = GetLength();
    int bytesWritten;

    SuccessOrExit(error = SetLength(GetLength() + aLength));
    bytesWritten = Write(oldLength, aLength, aBuf);

    assert(bytesWritten == (int)aLength);
    (void)bytesWritten;

exit:
    return error;
}

ThreadError Message::Prepend(const void *aBuf, uint16_t aLength)
{
    ThreadError error = kThreadError_None;
    Buffer *newBuffer = NULL;

    while (aLength > GetReserved())
    {
        VerifyOrExit((newBuffer = GetMessagePool()->NewBuffer()) != NULL, error = kThreadError_NoBufs);

        newBuffer->SetNextBuffer(GetNextBuffer());
        SetNextBuffer(newBuffer);

        if (GetReserved() < sizeof(mBuffer.mHead.mData))
        {
            // Copy payload from the first buffer.
            memcpy(newBuffer->mBuffer.mHead.mData + GetReserved(), mBuffer.mHead.mData + GetReserved(),
                   sizeof(mBuffer.mHead.mData) - GetReserved());
        }

        SetReserved(GetReserved() + kBufferDataSize);
    }

    SetReserved(GetReserved() - aLength);
    mBuffer.mHead.mInfo.mLength += aLength;
    SetOffset(GetOffset() + aLength);

    if (aBuf != NULL)
    {
        Write(0, aLength, aBuf);
    }

exit:
    return error;
}

ThreadError Message::RemoveHeader(uint16_t aLength)
{
    assert(aLength <= mBuffer.mHead.mInfo.mLength);

    mBuffer.mHead.mInfo.mReserved += aLength;
    mBuffer.mHead.mInfo.mLength -= aLength;

    if (mBuffer.mHead.mInfo.mOffset > aLength)
    {
        mBuffer.mHead.mInfo.mOffset -= aLength;
    }
    else
    {
        mBuffer.mHead.mInfo.mOffset = 0;
    }

    return kThreadError_None;
}

uint16_t Message::Read(uint16_t aOffset, uint16_t aLength, void *aBuf) const
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
        aBuf = static_cast<uint8_t *>(aBuf) + bytesToCopy;

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
        aBuf = static_cast<uint8_t *>(aBuf) + bytesToCopy;

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
        aBuf = static_cast<const uint8_t *>(aBuf) + bytesToCopy;

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
        aBuf = static_cast<const uint8_t *>(aBuf) + bytesToCopy;

        curBuffer = curBuffer->GetNextBuffer();
        aOffset = 0;
    }

    return bytesCopied;
}

int Message::CopyTo(uint16_t aSourceOffset, uint16_t aDestinationOffset, uint16_t aLength, Message &aMessage) const
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

Message *Message::Clone(uint16_t aLength) const
{
    ThreadError error = kThreadError_None;
    Message *messageCopy;

    VerifyOrExit((messageCopy = GetMessagePool()->New(GetType(), GetReserved())) != NULL, error = kThreadError_NoBufs);
    SuccessOrExit(error = messageCopy->SetLength(aLength));
    CopyTo(0, 0, aLength, *messageCopy);

    // Copy selected message information.
    messageCopy->SetOffset(GetOffset());
    messageCopy->SetInterfaceId(GetInterfaceId());
    messageCopy->SetSubType(GetSubType());
    messageCopy->SetPriority(GetPriority());
    messageCopy->SetLinkSecurityEnabled(IsLinkSecurityEnabled());

exit:

    if (error != kThreadError_None && messageCopy != NULL)
    {
        messageCopy->Free();
        messageCopy = NULL;
    }

    return messageCopy;
}

bool Message::GetChildMask(uint8_t aChildIndex) const
{
    assert(aChildIndex < sizeof(mBuffer.mHead.mInfo.mChildMask) * 8);
    return (mBuffer.mHead.mInfo.mChildMask[aChildIndex / 8] & (0x80 >> (aChildIndex % 8))) != 0;
}

void Message::ClearChildMask(uint8_t aChildIndex)
{
    assert(aChildIndex < sizeof(mBuffer.mHead.mInfo.mChildMask) * 8);
    mBuffer.mHead.mInfo.mChildMask[aChildIndex / 8] &= ~(0x80 >> (aChildIndex % 8));
}

void Message::SetChildMask(uint8_t aChildIndex)
{
    assert(aChildIndex < sizeof(mBuffer.mHead.mInfo.mChildMask) * 8);
    mBuffer.mHead.mInfo.mChildMask[aChildIndex / 8] |= 0x80 >> (aChildIndex % 8);
}

bool Message::IsChildPending(void) const
{
    bool rval = false;

    for (size_t i = 0; i < sizeof(mBuffer.mHead.mInfo.mChildMask); i++)
    {
        if (mBuffer.mHead.mInfo.mChildMask[i] != 0)
        {
            ExitNow(rval = true);
        }
    }

exit:
    return rval;
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

void Message::SetMessageQueue(MessageQueue *aMessageQueue)
{
    mBuffer.mHead.mInfo.mMessageQueue = aMessageQueue;
    mBuffer.mHead.mInfo.mInPriorityQ = false;
}

void Message::SetPriorityQueue(PriorityQueue *aPriorityQueue)
{
    mBuffer.mHead.mInfo.mPriorityQueue = aPriorityQueue;
    mBuffer.mHead.mInfo.mInPriorityQ = true;
}

MessageQueue::MessageQueue(void)
{
    SetTail(NULL);
}

void MessageQueue::AddToList(uint8_t aList, Message &aMessage)
{
    Message *head;

    assert((aMessage.Next(aList) == NULL) && (aMessage.Prev(aList) == NULL));

    if (GetTail() == NULL)
    {
        aMessage.Next(aList) = &aMessage;
        aMessage.Prev(aList) = &aMessage;
    }
    else
    {
        head = GetTail()->Next(aList);

        aMessage.Next(aList) = head;
        aMessage.Prev(aList) = GetTail();

        head->Prev(aList) = &aMessage;
        GetTail()->Next(aList) = &aMessage;
    }

    SetTail(&aMessage);
}

void MessageQueue::RemoveFromList(uint8_t aList, Message &aMessage)
{
    assert((aMessage.Next(aList) != NULL) && (aMessage.Prev(aList) != NULL));

    if (&aMessage == GetTail())
    {
        SetTail(GetTail()->Prev(aList));

        if (&aMessage == GetTail())
        {
            SetTail(NULL);
        }
    }

    aMessage.Prev(aList)->Next(aList) = aMessage.Next(aList);
    aMessage.Next(aList)->Prev(aList) = aMessage.Prev(aList);

    aMessage.Prev(aList) = NULL;
    aMessage.Next(aList) = NULL;
}

Message *MessageQueue::GetHead(void) const
{
    return (GetTail() == NULL) ? NULL : GetTail()->Next(MessageInfo::kListInterface);
}

ThreadError MessageQueue::Enqueue(Message &aMessage)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!aMessage.IsInAQueue(), error = kThreadError_Already);

    aMessage.SetMessageQueue(this);

    AddToList(MessageInfo::kListInterface, aMessage);
    aMessage.GetMessagePool()->GetAllMessagesQueue()->AddToList(MessageInfo::kListAll, aMessage);

exit:
    return error;
}

ThreadError MessageQueue::Dequeue(Message &aMessage)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aMessage.GetMessageQueue() == this, error = kThreadError_NotFound);

    RemoveFromList(MessageInfo::kListInterface, aMessage);
    aMessage.GetMessagePool()->GetAllMessagesQueue()->RemoveFromList(MessageInfo::kListAll, aMessage);

    aMessage.SetMessageQueue(NULL);

exit:
    return error;
}

void MessageQueue::GetInfo(uint16_t &aMessageCount, uint16_t &aBufferCount) const
{
    aMessageCount = 0;
    aBufferCount = 0;

    for (const Message *message = GetHead(); message != NULL; message = message->GetNext())
    {
        aMessageCount++;
        aBufferCount += message->GetBufferCount();
    }
}

PriorityQueue::PriorityQueue(void)
{
    for (int priority = 0; priority < Message::kNumPriorities; priority++)
    {
        mTails[priority] = NULL;
    }
}

Message *PriorityQueue::FindFirstNonNullTail(uint8_t aStartPriorityLevel) const
{
    Message *tail = NULL;
    uint8_t priority;

    priority = aStartPriorityLevel;

    do
    {
        if (mTails[priority] != NULL)
        {
            tail = mTails[priority];
            break;
        }

        priority = PrevPriority(priority);
    }
    while (priority != aStartPriorityLevel);

    return tail;
}

Message *PriorityQueue::GetHead(void) const
{
    Message *tail;

    tail = FindFirstNonNullTail(Message::kNumPriorities - 1);

    return (tail == NULL) ? NULL : tail->Next(MessageInfo::kListInterface);
}

Message *PriorityQueue::GetHeadForPriority(uint8_t aPriority) const
{
    Message *head;
    Message *previousTail;

    if (mTails[aPriority] != NULL)
    {
        previousTail = FindFirstNonNullTail(PrevPriority(aPriority));

        assert(previousTail != NULL);

        head = previousTail->Next(MessageInfo::kListInterface);
    }
    else
    {
        head = NULL;
    }

    return head;
}

Message *PriorityQueue::GetTail(void) const
{
    return FindFirstNonNullTail(Message::kNumPriorities - 1);
}

void PriorityQueue::AddToList(uint8_t aList, Message &aMessage)
{
    uint8_t priority;
    Message *tail;
    Message *next;

    priority = aMessage.GetPriority();

    tail = FindFirstNonNullTail(priority);

    if (tail != NULL)
    {
        next = tail->Next(aList);

        aMessage.Next(aList) = next;
        aMessage.Prev(aList) = tail;
        next->Prev(aList) = &aMessage;
        tail->Next(aList) = &aMessage;
    }
    else
    {
        aMessage.Next(aList) = &aMessage;
        aMessage.Prev(aList) = &aMessage;
    }

    mTails[priority] = &aMessage;
}

void PriorityQueue::RemoveFromList(uint8_t aList, Message &aMessage)
{
    uint8_t priority;
    Message *tail;

    priority = aMessage.GetPriority();

    tail = mTails[priority];

    if (&aMessage == tail)
    {
        tail = tail->Prev(aList);

        if ((&aMessage == tail) || (tail->GetPriority() != priority))
        {
            tail = NULL;
        }

        mTails[priority] = tail;
    }

    aMessage.Next(aList)->Prev(aList) = aMessage.Prev(aList);
    aMessage.Prev(aList)->Next(aList) = aMessage.Next(aList);
    aMessage.Next(aList) = NULL;
    aMessage.Prev(aList) = NULL;
}

ThreadError PriorityQueue::Enqueue(Message &aMessage)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(!aMessage.IsInAQueue(), error = kThreadError_Already);

    aMessage.SetPriorityQueue(this);

    AddToList(MessageInfo::kListInterface, aMessage);
    aMessage.GetMessagePool()->GetAllMessagesQueue()->AddToList(MessageInfo::kListAll, aMessage);

exit:
    return error;
}

ThreadError PriorityQueue::Dequeue(Message &aMessage)
{
    ThreadError error = kThreadError_None;

    VerifyOrExit(aMessage.GetPriorityQueue() == this, error = kThreadError_NotFound);

    RemoveFromList(MessageInfo::kListInterface, aMessage);
    aMessage.GetMessagePool()->GetAllMessagesQueue()->RemoveFromList(MessageInfo::kListAll, aMessage);

    aMessage.SetMessageQueue(NULL);

exit:
    return error;
}

void PriorityQueue::GetInfo(uint16_t &aMessageCount, uint16_t &aBufferCount) const
{
    aMessageCount = 0;
    aBufferCount = 0;

    for (const Message *message = GetHead(); message != NULL; message = message->GetNext())
    {
        aMessageCount++;
        aBufferCount += message->GetBufferCount();
    }
}

}  // namespace ot
