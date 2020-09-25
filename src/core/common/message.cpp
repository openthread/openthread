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

#include "message.hpp"

#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "net/checksum.hpp"
#include "net/ip6.hpp"

namespace ot {

MessagePool::MessagePool(Instance &aInstance)
    : InstanceLocator(aInstance)
#if !OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    , mNumFreeBuffers(kNumBuffers)
    , mBufferPool()
#endif
{
#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    otPlatMessagePoolInit(&GetInstance(), kNumBuffers, sizeof(Buffer));
#endif
}

Message *MessagePool::New(Message::Type aType, uint16_t aReserveHeader, Message::Priority aPriority)
{
    otError  error = OT_ERROR_NONE;
    Message *message;

    VerifyOrExit((message = static_cast<Message *>(NewBuffer(aPriority))) != nullptr, OT_NOOP);

    memset(message, 0, sizeof(*message));
    message->SetMessagePool(this);
    message->SetType(aType);
    message->SetReserved(aReserveHeader);
    message->SetLinkSecurityEnabled(true);

    SuccessOrExit(error = message->SetPriority(aPriority));
    SuccessOrExit(error = message->SetLength(0));

exit:
    if (error != OT_ERROR_NONE)
    {
        Free(message);
        message = nullptr;
    }

    return message;
}

Message *MessagePool::New(Message::Type aType, uint16_t aReserveHeader, const Message::Settings &aSettings)
{
    Message *message = New(aType, aReserveHeader, aSettings.GetPriority());

    if (message)
    {
        message->SetLinkSecurityEnabled(aSettings.IsLinkSecurityEnabled());
    }

    return message;
}

void MessagePool::Free(Message *aMessage)
{
    OT_ASSERT(aMessage->Next() == nullptr && aMessage->Prev() == nullptr);

    FreeBuffers(static_cast<Buffer *>(aMessage));
}

Buffer *MessagePool::NewBuffer(Message::Priority aPriority)
{
    Buffer *buffer = nullptr;

    SuccessOrExit(ReclaimBuffers(1, aPriority));

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT

    buffer = static_cast<Buffer *>(otPlatMessagePoolNew(&GetInstance()));

#else

    buffer = mBufferPool.Allocate();
    VerifyOrExit(buffer != nullptr, OT_NOOP);

    mNumFreeBuffers--;
    buffer->SetNextBuffer(nullptr);

#endif

exit:
    if (buffer == nullptr)
    {
        otLogInfoMem("No available message buffer");
    }

    return buffer;
}

void MessagePool::FreeBuffers(Buffer *aBuffer)
{
    while (aBuffer != nullptr)
    {
        Buffer *next = aBuffer->GetNextBuffer();
#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
        otPlatMessagePoolFree(&GetInstance(), aBuffer);
#else
        mBufferPool.Free(*aBuffer);
        mNumFreeBuffers++;
#endif
        aBuffer = next;
    }
}

otError MessagePool::ReclaimBuffers(int aNumBuffers, Message::Priority aPriority)
{
#if OPENTHREAD_MTD || OPENTHREAD_FTD
    while (aNumBuffers > GetFreeBufferCount())
    {
        SuccessOrExit(Get<MeshForwarder>().EvictMessage(aPriority));
    }

exit:
#else
    OT_UNUSED_VARIABLE(aPriority);
#endif

    // First comparison is to get around issues with comparing
    // signed and unsigned numbers, if aNumBuffers is negative then
    // the second comparison wont be attempted.
    return (aNumBuffers < 0 || aNumBuffers <= GetFreeBufferCount()) ? OT_ERROR_NONE : OT_ERROR_NO_BUFS;
}

uint16_t MessagePool::GetFreeBufferCount(void) const
{
    uint16_t rval;

#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    rval = otPlatMessagePoolNumFreeBuffers(&GetInstance());
#else
    rval = mNumFreeBuffers;
#endif

    return rval;
}

const Message::Settings Message::Settings::kDefault(Message::kWithLinkSecurity, Message::kPriorityNormal);

Message::Settings::Settings(LinkSecurityMode aSecurityMode, Priority aPriority)
    : mLinkSecurityEnabled(aSecurityMode == kWithLinkSecurity)
    , mPriority(aPriority)
{
}

Message::Settings::Settings(const otMessageSettings *aSettings)
    : mLinkSecurityEnabled((aSettings != nullptr) ? aSettings->mLinkSecurityEnabled : true)
    , mPriority((aSettings != nullptr) ? static_cast<Priority>(aSettings->mPriority) : kPriorityNormal)
{
}

otError Message::ResizeMessage(uint16_t aLength)
{
    otError error = OT_ERROR_NONE;

    // add buffers
    Buffer * curBuffer = this;
    Buffer * lastBuffer;
    uint16_t curLength = kHeadBufferDataSize;

    while (curLength < aLength)
    {
        if (curBuffer->GetNextBuffer() == nullptr)
        {
            curBuffer->SetNextBuffer(GetMessagePool()->NewBuffer(GetPriority()));
            VerifyOrExit(curBuffer->GetNextBuffer() != nullptr, error = OT_ERROR_NO_BUFS);
        }

        curBuffer = curBuffer->GetNextBuffer();
        curLength += kBufferDataSize;
    }

    // remove buffers
    lastBuffer = curBuffer;
    curBuffer  = curBuffer->GetNextBuffer();
    lastBuffer->SetNextBuffer(nullptr);

    GetMessagePool()->FreeBuffers(curBuffer);

exit:
    return error;
}

void Message::Free(void)
{
    GetMessagePool()->Free(this);
}

Message *Message::GetNext(void) const
{
    Message *next;
    Message *tail;

    if (GetMetadata().mInPriorityQ)
    {
        PriorityQueue *priorityQueue = GetPriorityQueue();
        VerifyOrExit(priorityQueue != nullptr, next = nullptr);
        tail = priorityQueue->GetTail();
    }
    else
    {
        MessageQueue *messageQueue = GetMessageQueue();
        VerifyOrExit(messageQueue != nullptr, next = nullptr);
        tail = messageQueue->GetTail();
    }

    next = (this == tail) ? nullptr : Next();

exit:
    return next;
}

otError Message::SetLength(uint16_t aLength)
{
    otError  error              = OT_ERROR_NONE;
    uint16_t totalLengthRequest = GetReserved() + aLength;
    uint16_t totalLengthCurrent = GetReserved() + GetLength();
    int      bufs               = 0;

    VerifyOrExit(totalLengthRequest >= GetReserved(), error = OT_ERROR_INVALID_ARGS);

    if (totalLengthRequest > kHeadBufferDataSize)
    {
        bufs = (((totalLengthRequest - kHeadBufferDataSize) - 1) / kBufferDataSize) + 1;
    }

    if (totalLengthCurrent > kHeadBufferDataSize)
    {
        bufs -= (((totalLengthCurrent - kHeadBufferDataSize) - 1) / kBufferDataSize) + 1;
    }

    SuccessOrExit(error = GetMessagePool()->ReclaimBuffers(bufs, GetPriority()));

    SuccessOrExit(error = ResizeMessage(totalLengthRequest));
    GetMetadata().mLength = aLength;

    // Correct offset in case shorter length is set.
    if (GetOffset() > aLength)
    {
        SetOffset(aLength);
    }

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

void Message::MoveOffset(int aDelta)
{
    OT_ASSERT(GetOffset() + aDelta <= GetLength());
    GetMetadata().mOffset += static_cast<int16_t>(aDelta);
    OT_ASSERT(GetMetadata().mOffset <= GetLength());
}

void Message::SetOffset(uint16_t aOffset)
{
    OT_ASSERT(aOffset <= GetLength());
    GetMetadata().mOffset = aOffset;
}

bool Message::IsSubTypeMle(void) const
{
    bool rval;

    switch (GetMetadata().mSubType)
    {
    case kSubTypeMleGeneral:
    case kSubTypeMleAnnounce:
    case kSubTypeMleDiscoverRequest:
    case kSubTypeMleDiscoverResponse:
    case kSubTypeMleChildUpdateRequest:
    case kSubTypeMleDataResponse:
    case kSubTypeMleChildIdRequest:
        rval = true;
        break;

    default:
        rval = false;
        break;
    }

    return rval;
}

otError Message::SetPriority(Priority aPriority)
{
    otError        error         = OT_ERROR_NONE;
    uint8_t        priority      = static_cast<uint8_t>(aPriority);
    PriorityQueue *priorityQueue = nullptr;

    VerifyOrExit(priority < kNumPriorities, error = OT_ERROR_INVALID_ARGS);

    VerifyOrExit(IsInAQueue(), GetMetadata().mPriority = priority);
    VerifyOrExit(GetMetadata().mPriority != priority, OT_NOOP);

    if (GetMetadata().mInPriorityQ)
    {
        priorityQueue = GetMetadata().mQueue.mPriority;
        priorityQueue->Dequeue(*this);
    }

    GetMetadata().mPriority = priority;

    if (priorityQueue != nullptr)
    {
        priorityQueue->Enqueue(*this);
    }

exit:
    return error;
}

otError Message::Append(const void *aBuf, uint16_t aLength)
{
    otError  error     = OT_ERROR_NONE;
    uint16_t oldLength = GetLength();

    SuccessOrExit(error = SetLength(GetLength() + aLength));
    Write(oldLength, aLength, aBuf);

exit:
    return error;
}

otError Message::Prepend(const void *aBuf, uint16_t aLength)
{
    otError error     = OT_ERROR_NONE;
    Buffer *newBuffer = nullptr;

    while (aLength > GetReserved())
    {
        VerifyOrExit((newBuffer = GetMessagePool()->NewBuffer(GetPriority())) != nullptr, error = OT_ERROR_NO_BUFS);

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
    GetMetadata().mLength += aLength;
    SetOffset(GetOffset() + aLength);

    if (aBuf != nullptr)
    {
        Write(0, aLength, aBuf);
    }

exit:
    return error;
}

void Message::RemoveHeader(uint16_t aLength)
{
    OT_ASSERT(aLength <= GetMetadata().mLength);

    GetMetadata().mReserved += aLength;
    GetMetadata().mLength -= aLength;

    if (GetMetadata().mOffset > aLength)
    {
        GetMetadata().mOffset -= aLength;
    }
    else
    {
        GetMetadata().mOffset = 0;
    }
}

void Message::GetFirstChunk(uint16_t aOffset, uint16_t &aLength, Chunk &aChunk) const
{
    // This method gets the first message chunk (contiguous data
    // buffer) corresponding to a given offset and length. On exit
    // `aChunk` is updated such that `aChunk.GetData()` gives the
    // pointer to the start of chunk and `aChunk.GetLength()` gives
    // its length. The `aLength` is also decreased by the chunk
    // length.

    VerifyOrExit(aOffset < GetLength(), aChunk.mLength = 0);

    if (aOffset + aLength >= GetLength())
    {
        aLength = GetLength() - aOffset;
    }

    aOffset += GetReserved();

    aChunk.mBuffer = this;

    // Special case for the first buffer

    if (aOffset < kHeadBufferDataSize)
    {
        aChunk.mData   = GetFirstData() + aOffset;
        aChunk.mLength = kHeadBufferDataSize - aOffset;
        ExitNow();
    }

    aOffset -= kHeadBufferDataSize;

    // Find the `Buffer` matching the offset

    while (true)
    {
        aChunk.mBuffer = aChunk.mBuffer->GetNextBuffer();
        OT_ASSERT(aChunk.mBuffer != nullptr);

        if (aOffset < kBufferDataSize)
        {
            aChunk.mData   = aChunk.mBuffer->GetData() + aOffset;
            aChunk.mLength = kBufferDataSize - aOffset;
            ExitNow();
        }

        aOffset -= kBufferDataSize;
    }

exit:
    if (aChunk.mLength > aLength)
    {
        aChunk.mLength = aLength;
    }

    aLength -= aChunk.mLength;
}

void Message::GetNextChunk(uint16_t &aLength, Chunk &aChunk) const
{
    // This method gets the next message chunk. On input, the
    // `aChunk` should be the previous chunk. On exit, it is
    // updated to provide info about next chunk, and `aLength`
    // is decreased by the chunk length. If there is no more
    // chunk, `aChunk.GetLength()` would be zero.

    VerifyOrExit(aLength > 0, aChunk.mLength = 0);

    aChunk.mBuffer = aChunk.mBuffer->GetNextBuffer();
    OT_ASSERT(aChunk.mBuffer != nullptr);

    aChunk.mData   = aChunk.mBuffer->GetData();
    aChunk.mLength = kBufferDataSize;

    if (aChunk.mLength > aLength)
    {
        aChunk.mLength = aLength;
    }

    aLength -= aChunk.mLength;

exit:
    return;
}

uint16_t Message::Read(uint16_t aOffset, uint16_t aLength, void *aBuf) const
{
    uint8_t *bufPtr = reinterpret_cast<uint8_t *>(aBuf);
    Chunk    chunk;

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        memcpy(bufPtr, chunk.GetData(), chunk.GetLength());
        bufPtr += chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }

    return static_cast<uint16_t>(bufPtr - reinterpret_cast<uint8_t *>(aBuf));
}

void Message::Write(uint16_t aOffset, uint16_t aLength, const void *aBuf)
{
    const uint8_t *bufPtr = reinterpret_cast<const uint8_t *>(aBuf);
    WritableChunk  chunk;

    OT_ASSERT(aOffset + aLength <= GetLength());

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        memmove(chunk.GetData(), bufPtr, chunk.GetLength());
        bufPtr += chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }
}

uint16_t Message::CopyTo(uint16_t aSourceOffset, uint16_t aDestinationOffset, uint16_t aLength, Message &aMessage) const
{
    uint16_t bytesCopied = 0;
    Chunk    chunk;

    // This implementing can potentially overwrite the data when bytes are
    // being copied forward within the same message, i.e., source and
    // destination messages are the same, and source offset is smaller than
    // the destination offset. We assert not allowing such a use.

    OT_ASSERT((&aMessage != this) || (aSourceOffset >= aDestinationOffset));

    GetFirstChunk(aSourceOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        aMessage.Write(aDestinationOffset, chunk.GetLength(), chunk.GetData());
        aDestinationOffset += chunk.GetLength();
        bytesCopied += chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }

    return bytesCopied;
}

Message *Message::Clone(uint16_t aLength) const
{
    otError  error = OT_ERROR_NONE;
    Message *messageCopy;
    uint16_t offset;

    VerifyOrExit((messageCopy = GetMessagePool()->New(GetType(), GetReserved(), GetPriority())) != nullptr,
                 error = OT_ERROR_NO_BUFS);
    SuccessOrExit(error = messageCopy->SetLength(aLength));
    CopyTo(0, 0, aLength, *messageCopy);

    // Copy selected message information.
    offset = GetOffset() < aLength ? GetOffset() : aLength;
    messageCopy->SetOffset(offset);

    messageCopy->SetSubType(GetSubType());
    messageCopy->SetLinkSecurityEnabled(IsLinkSecurityEnabled());
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    messageCopy->SetTimeSync(IsTimeSync());
#endif

exit:
    FreeAndNullMessageOnError(messageCopy, error);
    return messageCopy;
}

bool Message::GetChildMask(uint16_t aChildIndex) const
{
    return GetMetadata().mChildMask.Get(aChildIndex);
}

void Message::ClearChildMask(uint16_t aChildIndex)
{
    GetMetadata().mChildMask.Set(aChildIndex, false);
}

void Message::SetChildMask(uint16_t aChildIndex)
{
    GetMetadata().mChildMask.Set(aChildIndex, true);
}

bool Message::IsChildPending(void) const
{
    return GetMetadata().mChildMask.HasAny();
}

void Message::SetLinkInfo(const ThreadLinkInfo &aLinkInfo)
{
    SetLinkSecurityEnabled(aLinkInfo.mLinkSecurity);
    SetPanId(aLinkInfo.mPanId);
    AddRss(aLinkInfo.mRss);
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_ENABLE
    AddLqi(aLinkInfo.mLqi);
#endif
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SetTimeSyncSeq(aLinkInfo.mTimeSyncSeq);
    SetNetworkTimeOffset(aLinkInfo.mNetworkTimeOffset);
#endif
}

bool Message::IsTimeSync(void) const
{
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    return GetMetadata().mTimeSync;
#else
    return false;
#endif
}

void Message::SetMessageQueue(MessageQueue *aMessageQueue)
{
    GetMetadata().mQueue.mMessage = aMessageQueue;
    GetMetadata().mInPriorityQ    = false;
}

void Message::SetPriorityQueue(PriorityQueue *aPriorityQueue)
{
    GetMetadata().mQueue.mPriority = aPriorityQueue;
    GetMetadata().mInPriorityQ     = true;
}

MessageQueue::MessageQueue(void)
{
    SetTail(nullptr);
}

Message *MessageQueue::GetHead(void) const
{
    return (GetTail() == nullptr) ? nullptr : GetTail()->Next();
}

void MessageQueue::Enqueue(Message &aMessage, QueuePosition aPosition)
{
    OT_ASSERT(!aMessage.IsInAQueue());
    OT_ASSERT((aMessage.Next() == nullptr) && (aMessage.Prev() == nullptr));

    aMessage.SetMessageQueue(this);

    if (GetTail() == nullptr)
    {
        aMessage.Next() = &aMessage;
        aMessage.Prev() = &aMessage;

        SetTail(&aMessage);
    }
    else
    {
        Message *head = GetTail()->Next();

        aMessage.Next() = head;
        aMessage.Prev() = GetTail();

        head->Prev()      = &aMessage;
        GetTail()->Next() = &aMessage;

        if (aPosition == kQueuePositionTail)
        {
            SetTail(&aMessage);
        }
    }
}

void MessageQueue::Dequeue(Message &aMessage)
{
    OT_ASSERT(aMessage.GetMessageQueue() == this);
    OT_ASSERT((aMessage.Next() != nullptr) && (aMessage.Prev() != nullptr));

    if (&aMessage == GetTail())
    {
        SetTail(GetTail()->Prev());

        if (&aMessage == GetTail())
        {
            SetTail(nullptr);
        }
    }

    aMessage.Prev()->Next() = aMessage.Next();
    aMessage.Next()->Prev() = aMessage.Prev();

    aMessage.Prev() = nullptr;
    aMessage.Next() = nullptr;

    aMessage.SetMessageQueue(nullptr);
}

void MessageQueue::GetInfo(uint16_t &aMessageCount, uint16_t &aBufferCount) const
{
    aMessageCount = 0;
    aBufferCount  = 0;

    for (const Message *message = GetHead(); message != nullptr; message = message->GetNext())
    {
        aMessageCount++;
        aBufferCount += message->GetBufferCount();
    }
}

PriorityQueue::PriorityQueue(void)
{
    for (Message *&tail : mTails)
    {
        tail = nullptr;
    }
}

Message *PriorityQueue::FindFirstNonNullTail(Message::Priority aStartPriorityLevel) const
{
    Message *tail = nullptr;
    uint8_t  priority;

    priority = static_cast<uint8_t>(aStartPriorityLevel);

    do
    {
        if (mTails[priority] != nullptr)
        {
            tail = mTails[priority];
            break;
        }

        priority = PrevPriority(priority);
    } while (priority != aStartPriorityLevel);

    return tail;
}

Message *PriorityQueue::GetHead(void) const
{
    Message *tail;

    tail = FindFirstNonNullTail(Message::kPriorityLow);

    return (tail == nullptr) ? nullptr : tail->Next();
}

Message *PriorityQueue::GetHeadForPriority(Message::Priority aPriority) const
{
    Message *head;
    Message *previousTail;

    if (mTails[aPriority] != nullptr)
    {
        previousTail = FindFirstNonNullTail(static_cast<Message::Priority>(PrevPriority(aPriority)));

        OT_ASSERT(previousTail != nullptr);

        head = previousTail->Next();
    }
    else
    {
        head = nullptr;
    }

    return head;
}

Message *PriorityQueue::GetTail(void) const
{
    return FindFirstNonNullTail(Message::kPriorityLow);
}

void PriorityQueue::Enqueue(Message &aMessage)
{
    Message::Priority priority;
    Message *         tail;
    Message *         next;

    OT_ASSERT(!aMessage.IsInAQueue());

    aMessage.SetPriorityQueue(this);

    priority = aMessage.GetPriority();

    tail = FindFirstNonNullTail(priority);

    if (tail != nullptr)
    {
        next = tail->Next();

        aMessage.Next() = next;
        aMessage.Prev() = tail;
        next->Prev()    = &aMessage;
        tail->Next()    = &aMessage;
    }
    else
    {
        aMessage.Next() = &aMessage;
        aMessage.Prev() = &aMessage;
    }

    mTails[priority] = &aMessage;
}

void PriorityQueue::Dequeue(Message &aMessage)
{
    Message::Priority priority;
    Message *         tail;

    OT_ASSERT(aMessage.GetPriorityQueue() == this);

    priority = aMessage.GetPriority();

    tail = mTails[priority];

    if (&aMessage == tail)
    {
        tail = tail->Prev();

        if ((&aMessage == tail) || (tail->GetPriority() != priority))
        {
            tail = nullptr;
        }

        mTails[priority] = tail;
    }

    aMessage.Next()->Prev() = aMessage.Prev();
    aMessage.Prev()->Next() = aMessage.Next();
    aMessage.Next()         = nullptr;
    aMessage.Prev()         = nullptr;

    aMessage.SetMessageQueue(nullptr);
}

void PriorityQueue::GetInfo(uint16_t &aMessageCount, uint16_t &aBufferCount) const
{
    aMessageCount = 0;
    aBufferCount  = 0;

    for (const Message *message = GetHead(); message != nullptr; message = message->GetNext())
    {
        aMessageCount++;
        aBufferCount += message->GetBufferCount();
    }
}

} // namespace ot
