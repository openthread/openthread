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

#include <common/code_utils.hpp>
#include <common/debug.hpp>
#include <common/logging.hpp>
#include <common/message.hpp>
#include <net/ip6.hpp>

namespace Thread {

MessagePool::MessagePool(void) :
    mAllQueue()
{
    memset(mBuffers, 0, sizeof(mBuffers));

    mFreeBuffers = mBuffers;

    for (int i = 0; i < kNumBuffers - 1; i++)
    {
        mBuffers[i].SetNextBuffer(&mBuffers[i + 1]);
    }

    mBuffers[kNumBuffers - 1].SetNextBuffer(NULL);
    mNumFreeBuffers = kNumBuffers;

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    // Pass the list of free buffers and a pointer to the count of free buffers
    // to the platform for management.
    otPlatMessagePoolInit(static_cast<BufferHeader *>(mFreeBuffers), &mNumFreeBuffers, sizeof(Buffer));
#endif
}

Message *MessagePool::New(uint8_t aType, uint16_t aReserved)
{
    Message *message = NULL;

    VerifyOrExit((message = static_cast<Message *>(NewBuffer())) != NULL, ;);

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
    buffer = static_cast<Buffer *>(otPlatMessagePoolNew());

    if (buffer == NULL)
    {
        otLogInfoMac("No available message buffer\n");
    }

#else // OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT

    if (mFreeBuffers == NULL)
    {
        otLogInfoMac("No available message buffer");
        ExitNow();
    }

    buffer = mFreeBuffers;
    mFreeBuffers = mFreeBuffers->GetNextBuffer();
    buffer->SetNextBuffer(NULL);
    mNumFreeBuffers--;
#endif // OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT

exit:
    return buffer;
}

ThreadError MessagePool::FreeBuffers(Buffer *aBuffer)
{
    Buffer *tmpBuffer;

    while (aBuffer != NULL)
    {
        tmpBuffer = aBuffer->GetNextBuffer();
#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
        otPlatMessagePoolFree(static_cast<struct BufferHeader *>(aBuffer));
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
    return (aNumBuffers <= mNumFreeBuffers) ? kThreadError_None : kThreadError_NoBufs;
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

    if (mInfo.mInPriorityQ)
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

    SuccessOrExit(error = GetMessagePool()->ReclaimBuffers(bufs));

    SuccessOrExit(error = ResizeMessage(totalLengthRequest));
    mInfo.mLength = aLength;

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

uint16_t Message::GetOffset(void) const
{
    return mInfo.mOffset;
}

ThreadError Message::MoveOffset(int aDelta)
{
    ThreadError error = kThreadError_None;

    assert(GetOffset() + aDelta <= GetLength());
    VerifyOrExit(GetOffset() + aDelta <= GetLength(), error = kThreadError_InvalidArgs);

    mInfo.mOffset += static_cast<int16_t>(aDelta);
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

uint8_t Message::GetSubType(void) const
{
    return mInfo.mSubType;
}

void Message::SetSubType(uint8_t aSubType)
{
    mInfo.mSubType = aSubType;
}

uint8_t Message::GetPriority(void) const
{
    return mInfo.mPriority;
}

ThreadError Message::SetPriority(uint8_t aPriority)
{
    ThreadError error = kThreadError_None;
    PriorityQueue *priorityQueue = NULL;

    VerifyOrExit(aPriority < kNumPriorities, error = kThreadError_InvalidArgs);

    VerifyOrExit(IsInAQueue(), mInfo.mPriority = aPriority);
    VerifyOrExit(mInfo.mPriority != aPriority, ;);

    if (mInfo.mInPriorityQ)
    {
        priorityQueue = mInfo.mPriorityQueue;
        priorityQueue->Dequeue(*this);
    }
    else
    {
        GetMessagePool()->GetAllMessagesQueue()->RemoveFromList(MessageInfo::kListAll, *this);
    }

    mInfo.mPriority = aPriority;

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

        if (GetReserved() < sizeof(mHeadData))
        {
            // Copy payload from the first buffer.
            memcpy(newBuffer->mHeadData + GetReserved(), mHeadData + GetReserved(),
                   sizeof(mHeadData) - GetReserved());
        }

        SetReserved(GetReserved() + kBufferDataSize);
    }

    SetReserved(GetReserved() - aLength);
    mInfo.mLength += aLength;
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
    assert(aLength <= mInfo.mLength);

    mInfo.mReserved += aLength;
    mInfo.mLength -= aLength;

    if (mInfo.mOffset > aLength)
    {
        mInfo.mOffset -= aLength;
    }
    else
    {
        mInfo.mOffset = 0;
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

uint8_t Message::GetChannel(void) const
{
    return mInfo.mChannel;
}

void Message::SetChannel(uint8_t aChannel)
{
    mInfo.mChannel = aChannel;
}

uint8_t Message::GetTimeout(void) const
{
    return mInfo.mTimeout;
}

void Message::SetTimeout(uint8_t aTimeout)
{
    mInfo.mTimeout = aTimeout;
}

int8_t Message::GetInterfaceId(void) const
{
    return mInfo.mInterfaceId;
}

void Message::SetInterfaceId(int8_t aInterfaceId)
{
    mInfo.mInterfaceId = aInterfaceId;
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

void Message::SetMessageQueue(MessageQueue *aMessageQueue)
{
    mInfo.mMessageQueue = aMessageQueue;
    mInfo.mInPriorityQ = false;
}

void Message::SetPriorityQueue(PriorityQueue *aPriorityQueue)
{
    mInfo.mPriorityQueue = aPriorityQueue;
    mInfo.mInPriorityQ = true;
}

MessageQueue::MessageQueue(void) :
    mTail(NULL)
{
}

void MessageQueue::AddToList(uint8_t aList, Message &aMessage)
{
    Message *head;

    assert((aMessage.Next(aList) == NULL) && (aMessage.Prev(aList) == NULL));

    if (mTail == NULL)
    {
        aMessage.Next(aList) = &aMessage;
        aMessage.Prev(aList) = &aMessage;
    }
    else
    {
        head = mTail->Next(aList);

        aMessage.Next(aList) = head;
        aMessage.Prev(aList) = mTail;

        head->Prev(aList) = &aMessage;
        mTail->Next(aList) = &aMessage;
    }

    mTail = &aMessage;
}

void MessageQueue::RemoveFromList(uint8_t aList, Message &aMessage)
{
    assert((aMessage.Next(aList) != NULL) && (aMessage.Prev(aList) != NULL));

    if (&aMessage == mTail)
    {
        mTail = mTail->Prev(aList);

        if (&aMessage == mTail)
        {
            mTail = NULL;
        }
    }

    aMessage.Prev(aList)->Next(aList) = aMessage.Next(aList);
    aMessage.Next(aList)->Prev(aList) = aMessage.Prev(aList);

    aMessage.Prev(aList) = NULL;
    aMessage.Next(aList) = NULL;
}

Message *MessageQueue::GetHead(void) const
{
    return (mTail == NULL) ? NULL : mTail->Next(MessageInfo::kListInterface);
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

}  // namespace Thread
