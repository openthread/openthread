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

#include "common/as_core_type.hpp"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "common/heap.hpp"
#include "common/locator_getters.hpp"
#include "common/log.hpp"
#include "common/num_utils.hpp"
#include "common/numeric_limits.hpp"
#include "instance/instance.hpp"
#include "net/checksum.hpp"
#include "net/ip6.hpp"

#if OPENTHREAD_MTD || OPENTHREAD_FTD

#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE && OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
#error "OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE conflicts with OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT."
#endif

namespace ot {

RegisterLogModule("Message");

//---------------------------------------------------------------------------------------------------------------------
// MessagePool

MessagePool::MessagePool(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mNumAllocated(0)
    , mMaxAllocated(0)
{
#if OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    otPlatMessagePoolInit(&GetInstance(), kNumBuffers, sizeof(Buffer));
#endif
}

Message *MessagePool::Allocate(Message::Type aType, uint16_t aReserveHeader, const Message::Settings &aSettings)
{
    Error    error = kErrorNone;
    Message *message;

    VerifyOrExit((message = static_cast<Message *>(NewBuffer(aSettings.GetPriority()))) != nullptr);

    memset(message, 0, sizeof(*message));
    message->SetMessagePool(this);
    message->SetType(aType);
    message->SetReserved(aReserveHeader);
    message->SetLinkSecurityEnabled(aSettings.IsLinkSecurityEnabled());
    message->SetLoopbackToHostAllowed(OPENTHREAD_CONFIG_IP6_ALLOW_LOOP_BACK_HOST_DATAGRAMS);
    message->SetOrigin(Message::kOriginHostTrusted);

    SuccessOrExit(error = message->SetPriority(aSettings.GetPriority()));
    SuccessOrExit(error = message->SetLength(0));

exit:
    if (error != kErrorNone)
    {
        Free(message);
        message = nullptr;
    }

    return message;
}

Message *MessagePool::Allocate(Message::Type aType) { return Allocate(aType, 0, Message::Settings::GetDefault()); }

Message *MessagePool::Allocate(Message::Type aType, uint16_t aReserveHeader)
{
    return Allocate(aType, aReserveHeader, Message::Settings::GetDefault());
}

void MessagePool::Free(Message *aMessage)
{
    OT_ASSERT(aMessage->Next() == nullptr && aMessage->Prev() == nullptr);

    FreeBuffers(static_cast<Buffer *>(aMessage));
}

Buffer *MessagePool::NewBuffer(Message::Priority aPriority)
{
    Buffer *buffer = nullptr;

    while ((
#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
               buffer = static_cast<Buffer *>(Heap::CAlloc(1, sizeof(Buffer)))
#elif OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
               buffer = static_cast<Buffer *>(otPlatMessagePoolNew(&GetInstance()))
#else
               buffer = mBufferPool.Allocate()
#endif
                   ) == nullptr)
    {
        SuccessOrExit(ReclaimBuffers(aPriority));
    }

    mNumAllocated++;
    mMaxAllocated = Max(mMaxAllocated, mNumAllocated);

    buffer->SetNextBuffer(nullptr);

exit:
    if (buffer == nullptr)
    {
        LogInfo("No available message buffer");
    }

    return buffer;
}

void MessagePool::FreeBuffers(Buffer *aBuffer)
{
    while (aBuffer != nullptr)
    {
        Buffer *next = aBuffer->GetNextBuffer();
#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
        Heap::Free(aBuffer);
#elif OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
        otPlatMessagePoolFree(&GetInstance(), aBuffer);
#else
        mBufferPool.Free(*aBuffer);
#endif
        mNumAllocated--;

        aBuffer = next;
    }
}

Error MessagePool::ReclaimBuffers(Message::Priority aPriority) { return Get<MeshForwarder>().EvictMessage(aPriority); }

uint16_t MessagePool::GetFreeBufferCount(void) const
{
    uint16_t rval;

#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
#if !OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
    rval = static_cast<uint16_t>(Instance::GetHeap().GetFreeSize() / sizeof(Buffer));
#else
    rval = NumericLimits<uint16_t>::kMax;
#endif
#elif OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
    rval = otPlatMessagePoolNumFreeBuffers(&GetInstance());
#else
    rval = kNumBuffers - mNumAllocated;
#endif

    return rval;
}

uint16_t MessagePool::GetTotalBufferCount(void) const
{
    uint16_t rval;

#if OPENTHREAD_CONFIG_MESSAGE_USE_HEAP_ENABLE
#if !OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
    rval = static_cast<uint16_t>(Instance::GetHeap().GetCapacity() / sizeof(Buffer));
#else
    rval = NumericLimits<uint16_t>::kMax;
#endif
#else
    rval = OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS;
#endif

    return rval;
}

//---------------------------------------------------------------------------------------------------------------------
// Message::Settings

const otMessageSettings Message::Settings::kDefault = {kWithLinkSecurity, kPriorityNormal};

Message::Settings::Settings(LinkSecurityMode aSecurityMode, Priority aPriority)
{
    mLinkSecurityEnabled = aSecurityMode;
    mPriority            = aPriority;
}

const Message::Settings &Message::Settings::From(const otMessageSettings *aSettings)
{
    return (aSettings == nullptr) ? GetDefault() : AsCoreType(aSettings);
}

//---------------------------------------------------------------------------------------------------------------------
// Message::Iterator

void Message::Iterator::Advance(void)
{
    mItem = mNext;
    mNext = NextMessage(mNext);
}

//---------------------------------------------------------------------------------------------------------------------
// Message

Error Message::ResizeMessage(uint16_t aLength)
{
    // This method adds or frees message buffers to meet the
    // requested length.

    Error    error     = kErrorNone;
    Buffer  *curBuffer = this;
    Buffer  *lastBuffer;
    uint16_t curLength = kHeadBufferDataSize;

    while (curLength < aLength)
    {
        if (curBuffer->GetNextBuffer() == nullptr)
        {
            curBuffer->SetNextBuffer(GetMessagePool()->NewBuffer(GetPriority()));
            VerifyOrExit(curBuffer->GetNextBuffer() != nullptr, error = kErrorNoBufs);
        }

        curBuffer = curBuffer->GetNextBuffer();
        curLength += kBufferDataSize;
    }

    lastBuffer = curBuffer;
    curBuffer  = curBuffer->GetNextBuffer();
    lastBuffer->SetNextBuffer(nullptr);

    GetMessagePool()->FreeBuffers(curBuffer);

exit:
    return error;
}

void Message::Free(void) { GetMessagePool()->Free(this); }

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

Error Message::SetLength(uint16_t aLength)
{
    Error    error              = kErrorNone;
    uint16_t totalLengthRequest = GetReserved() + aLength;

    VerifyOrExit(totalLengthRequest >= GetReserved(), error = kErrorInvalidArgs);

    SuccessOrExit(error = ResizeMessage(totalLengthRequest));
    GetMetadata().mLength = aLength;

    // Correct the offset in case shorter length is set.
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

Error Message::SetPriority(Priority aPriority)
{
    Error          error    = kErrorNone;
    uint8_t        priority = static_cast<uint8_t>(aPriority);
    PriorityQueue *priorityQueue;

    static_assert(kNumPriorities <= 4, "`Metadata::mPriority` as a 2-bit field cannot fit all `Priority` values");

    VerifyOrExit(priority < kNumPriorities, error = kErrorInvalidArgs);

    VerifyOrExit(IsInAQueue(), GetMetadata().mPriority = priority);
    VerifyOrExit(GetMetadata().mPriority != priority);

    priorityQueue = GetPriorityQueue();

    if (priorityQueue != nullptr)
    {
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

const char *Message::PriorityToString(Priority aPriority)
{
    static const char *const kPriorityStrings[] = {
        "low",    // (0) kPriorityLow
        "normal", // (1) kPriorityNormal
        "high",   // (2) kPriorityHigh
        "net",    // (3) kPriorityNet
    };

    static_assert(kPriorityLow == 0, "kPriorityLow value is incorrect");
    static_assert(kPriorityNormal == 1, "kPriorityNormal value is incorrect");
    static_assert(kPriorityHigh == 2, "kPriorityHigh value is incorrect");
    static_assert(kPriorityNet == 3, "kPriorityNet value is incorrect");

    return kPriorityStrings[aPriority];
}

Error Message::AppendBytes(const void *aBuf, uint16_t aLength)
{
    Error    error     = kErrorNone;
    uint16_t oldLength = GetLength();

    SuccessOrExit(error = SetLength(GetLength() + aLength));
    WriteBytes(oldLength, aBuf, aLength);

exit:
    return error;
}

Error Message::AppendBytesFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength)
{
    Error    error       = kErrorNone;
    uint16_t writeOffset = GetLength();
    Chunk    chunk;

    VerifyOrExit(aMessage.GetLength() >= aOffset + aLength, error = kErrorParse);
    SuccessOrExit(error = SetLength(GetLength() + aLength));

    aMessage.GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        WriteBytes(writeOffset, chunk.GetBytes(), chunk.GetLength());
        writeOffset += chunk.GetLength();
        aMessage.GetNextChunk(aLength, chunk);
    }

exit:
    return error;
}

Error Message::PrependBytes(const void *aBuf, uint16_t aLength)
{
    Error   error     = kErrorNone;
    Buffer *newBuffer = nullptr;

    while (aLength > GetReserved())
    {
        VerifyOrExit((newBuffer = GetMessagePool()->NewBuffer(GetPriority())) != nullptr, error = kErrorNoBufs);

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
        WriteBytes(0, aBuf, aLength);
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

void Message::RemoveHeader(uint16_t aOffset, uint16_t aLength)
{
    // To shrink the header, we copy the header byte before `aOffset`
    // forward. Starting at offset `aLength`, we write bytes we read
    // from offset `0` onward and copy a total of `aOffset` bytes.
    // Then remove the first `aLength` bytes from message.
    //
    //
    // 0                   aOffset  aOffset + aLength
    // +-----------------------+---------+------------------------+
    // | / / / / / / / / / / / | x x x x |                        |
    // +-----------------------+---------+------------------------+
    //
    // 0       aLength                aOffset + aLength
    // +---------+-----------------------+------------------------+
    // |         | / / / / / / / / / / / |                        |
    // +---------+-----------------------+------------------------+
    //
    //  0                    aOffset
    //  +-----------------------+------------------------+
    //  | / / / / / / / / / / / |                        |
    //  +-----------------------+------------------------+
    //

    WriteBytesFromMessage(/* aWriteOffset */ aLength, *this, /* aReadOffset */ 0, /* aLength */ aOffset);
    RemoveHeader(aLength);
}

Error Message::InsertHeader(uint16_t aOffset, uint16_t aLength)
{
    Error error;

    // To make space in header at `aOffset`, we first prepend
    // `aLength` bytes at front. Then copy the existing bytes
    // backwards. Starting at offset `0`, we write bytes we read
    // from offset `aLength` onward and copy a total of `aOffset`
    // bytes.
    //
    // 0                    aOffset
    // +-----------------------+------------------------+
    // | / / / / / / / / / / / |                        |
    // +-----------------------+------------------------+
    //
    // 0       aLength                aOffset + aLength
    // +---------+-----------------------+------------------------+
    // |         | / / / / / / / / / / / |                        |
    // +---------+-----------------------+------------------------+
    //
    // 0                   aOffset  aOffset + aLength
    // +-----------------------+---------+------------------------+
    // | / / / / / / / / / / / |  N E W  |                        |
    // +-----------------------+---------+------------------------+
    //

    SuccessOrExit(error = PrependBytes(nullptr, aLength));
    WriteBytesFromMessage(/* aWriteOffset */ 0, *this, /* aReadOffset */ aLength, /* aLength */ aOffset);

exit:
    return error;
}

void Message::GetFirstChunk(uint16_t aOffset, uint16_t &aLength, Chunk &aChunk) const
{
    // This method gets the first message chunk (contiguous data
    // buffer) corresponding to a given offset and length. On exit
    // `aChunk` is updated such that `aChunk.GetBytes()` gives the
    // pointer to the start of chunk and `aChunk.GetLength()` gives
    // its length. The `aLength` is also decreased by the chunk
    // length.

    VerifyOrExit(aOffset < GetLength(), aChunk.SetLength(0));

    if (aOffset + aLength >= GetLength())
    {
        aLength = GetLength() - aOffset;
    }

    aOffset += GetReserved();

    aChunk.SetBuffer(this);

    // Special case for the first buffer

    if (aOffset < kHeadBufferDataSize)
    {
        aChunk.Init(GetFirstData() + aOffset, kHeadBufferDataSize - aOffset);
        ExitNow();
    }

    aOffset -= kHeadBufferDataSize;

    // Find the `Buffer` matching the offset

    while (true)
    {
        aChunk.SetBuffer(aChunk.GetBuffer()->GetNextBuffer());

        OT_ASSERT(aChunk.GetBuffer() != nullptr);

        if (aOffset < kBufferDataSize)
        {
            aChunk.Init(aChunk.GetBuffer()->GetData() + aOffset, kBufferDataSize - aOffset);
            ExitNow();
        }

        aOffset -= kBufferDataSize;
    }

exit:
    if (aChunk.GetLength() > aLength)
    {
        aChunk.SetLength(aLength);
    }

    aLength -= aChunk.GetLength();
}

void Message::GetNextChunk(uint16_t &aLength, Chunk &aChunk) const
{
    // This method gets the next message chunk. On input, the
    // `aChunk` should be the previous chunk. On exit, it is
    // updated to provide info about next chunk, and `aLength`
    // is decreased by the chunk length. If there is no more
    // chunk, `aChunk.GetLength()` would be zero.

    VerifyOrExit(aLength > 0, aChunk.SetLength(0));

    aChunk.SetBuffer(aChunk.GetBuffer()->GetNextBuffer());

    OT_ASSERT(aChunk.GetBuffer() != nullptr);

    aChunk.Init(aChunk.GetBuffer()->GetData(), kBufferDataSize);

    if (aChunk.GetLength() > aLength)
    {
        aChunk.SetLength(aLength);
    }

    aLength -= aChunk.GetLength();

exit:
    return;
}

uint16_t Message::ReadBytes(uint16_t aOffset, void *aBuf, uint16_t aLength) const
{
    uint8_t *bufPtr = reinterpret_cast<uint8_t *>(aBuf);
    Chunk    chunk;

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        chunk.CopyBytesTo(bufPtr);
        bufPtr += chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }

    return static_cast<uint16_t>(bufPtr - reinterpret_cast<uint8_t *>(aBuf));
}

Error Message::Read(uint16_t aOffset, void *aBuf, uint16_t aLength) const
{
    return (ReadBytes(aOffset, aBuf, aLength) == aLength) ? kErrorNone : kErrorParse;
}

bool Message::CompareBytes(uint16_t aOffset, const void *aBuf, uint16_t aLength, ByteMatcher aMatcher) const
{
    uint16_t       bytesToCompare = aLength;
    const uint8_t *bufPtr         = reinterpret_cast<const uint8_t *>(aBuf);
    Chunk          chunk;

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        VerifyOrExit(chunk.MatchesBytesIn(bufPtr, aMatcher));
        bufPtr += chunk.GetLength();
        bytesToCompare -= chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }

exit:
    return (bytesToCompare == 0);
}

bool Message::CompareBytes(uint16_t       aOffset,
                           const Message &aOtherMessage,
                           uint16_t       aOtherOffset,
                           uint16_t       aLength,
                           ByteMatcher    aMatcher) const
{
    uint16_t bytesToCompare = aLength;
    Chunk    chunk;

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        VerifyOrExit(aOtherMessage.CompareBytes(aOtherOffset, chunk.GetBytes(), chunk.GetLength(), aMatcher));
        aOtherOffset += chunk.GetLength();
        bytesToCompare -= chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }

exit:
    return (bytesToCompare == 0);
}

void Message::WriteBytes(uint16_t aOffset, const void *aBuf, uint16_t aLength)
{
    const uint8_t *bufPtr = reinterpret_cast<const uint8_t *>(aBuf);
    MutableChunk   chunk;

    OT_ASSERT(aOffset + aLength <= GetLength());

    GetFirstChunk(aOffset, aLength, chunk);

    while (chunk.GetLength() > 0)
    {
        memmove(chunk.GetBytes(), bufPtr, chunk.GetLength());
        bufPtr += chunk.GetLength();
        GetNextChunk(aLength, chunk);
    }
}

void Message::WriteBytesFromMessage(uint16_t       aWriteOffset,
                                    const Message &aMessage,
                                    uint16_t       aReadOffset,
                                    uint16_t       aLength)
{
    if ((&aMessage != this) || (aReadOffset >= aWriteOffset))
    {
        Chunk chunk;

        aMessage.GetFirstChunk(aReadOffset, aLength, chunk);

        while (chunk.GetLength() > 0)
        {
            WriteBytes(aWriteOffset, chunk.GetBytes(), chunk.GetLength());
            aWriteOffset += chunk.GetLength();
            aMessage.GetNextChunk(aLength, chunk);
        }
    }
    else
    {
        // We are copying bytes within the same message forward.
        // To ensure copy forward works, we read and write from
        // end of range and move backwards.

        static constexpr uint16_t kBufSize = 32;

        uint8_t buf[kBufSize];

        aWriteOffset += aLength;
        aReadOffset += aLength;

        while (aLength > 0)
        {
            uint16_t copyLength = Min(kBufSize, aLength);

            aLength -= copyLength;
            aReadOffset -= copyLength;
            aWriteOffset -= copyLength;

            ReadBytes(aReadOffset, buf, copyLength);
            WriteBytes(aWriteOffset, buf, copyLength);
        }
    }
}

Message *Message::Clone(uint16_t aLength) const
{
    Error    error = kErrorNone;
    Message *messageCopy;
    Settings settings(IsLinkSecurityEnabled() ? kWithLinkSecurity : kNoLinkSecurity, GetPriority());
    uint16_t offset;

    aLength     = Min(GetLength(), aLength);
    messageCopy = GetMessagePool()->Allocate(GetType(), GetReserved(), settings);
    VerifyOrExit(messageCopy != nullptr, error = kErrorNoBufs);
    SuccessOrExit(error = messageCopy->AppendBytesFromMessage(*this, 0, aLength));

    // Copy selected message information.
    offset = Min(GetOffset(), aLength);
    messageCopy->SetOffset(offset);

    messageCopy->SetSubType(GetSubType());
    messageCopy->SetLoopbackToHostAllowed(IsLoopbackToHostAllowed());
    messageCopy->SetOrigin(GetOrigin());
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    messageCopy->SetTimeSync(IsTimeSync());
#endif

exit:
    FreeAndNullMessageOnError(messageCopy, error);
    return messageCopy;
}

#if OPENTHREAD_FTD
bool Message::GetChildMask(uint16_t aChildIndex) const { return GetMetadata().mChildMask.Get(aChildIndex); }

void Message::ClearChildMask(uint16_t aChildIndex) { GetMetadata().mChildMask.Set(aChildIndex, false); }

void Message::SetChildMask(uint16_t aChildIndex) { GetMetadata().mChildMask.Set(aChildIndex, true); }

bool Message::IsChildPending(void) const { return GetMetadata().mChildMask.HasAny(); }
#endif

void Message::SetLinkInfo(const ThreadLinkInfo &aLinkInfo)
{
    SetLinkSecurityEnabled(aLinkInfo.mLinkSecurity);
    SetPanId(aLinkInfo.mPanId);
    AddRss(aLinkInfo.mRss);
#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
    AddLqi(aLinkInfo.mLqi);
#endif
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE
    SetTimeSyncSeq(aLinkInfo.mTimeSyncSeq);
    SetNetworkTimeOffset(aLinkInfo.mNetworkTimeOffset);
#endif
#if OPENTHREAD_CONFIG_MULTI_RADIO
    SetRadioType(static_cast<Mac::RadioType>(aLinkInfo.mRadioType));
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
    GetMetadata().mQueue       = aMessageQueue;
    GetMetadata().mInPriorityQ = false;
}

void Message::SetPriorityQueue(PriorityQueue *aPriorityQueue)
{
    GetMetadata().mQueue       = aPriorityQueue;
    GetMetadata().mInPriorityQ = true;
}

//---------------------------------------------------------------------------------------------------------------------
// MessageQueue

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

void MessageQueue::DequeueAndFree(Message &aMessage)
{
    Dequeue(aMessage);
    aMessage.Free();
}

void MessageQueue::DequeueAndFreeAll(void)
{
    Message *message;

    while ((message = GetHead()) != nullptr)
    {
        DequeueAndFree(*message);
    }
}

Message::Iterator MessageQueue::begin(void) { return Message::Iterator(GetHead()); }

Message::ConstIterator MessageQueue::begin(void) const { return Message::ConstIterator(GetHead()); }

void MessageQueue::GetInfo(Info &aInfo) const
{
    for (const Message &message : *this)
    {
        aInfo.mNumMessages++;
        aInfo.mNumBuffers += message.GetBufferCount();
        aInfo.mTotalBytes += message.GetLength();
    }
}

//---------------------------------------------------------------------------------------------------------------------
// PriorityQueue

const Message *PriorityQueue::FindFirstNonNullTail(Message::Priority aStartPriorityLevel) const
{
    // Find the first non-`nullptr` tail starting from the given priority
    // level and moving forward (wrapping from priority value
    // `kNumPriorities` -1 back to 0).

    const Message *tail = nullptr;
    uint8_t        priority;

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

const Message *PriorityQueue::GetHead(void) const
{
    return Message::NextOf(FindFirstNonNullTail(Message::kPriorityLow));
}

const Message *PriorityQueue::GetHeadForPriority(Message::Priority aPriority) const
{
    const Message *head;
    const Message *previousTail;

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

const Message *PriorityQueue::GetTail(void) const { return FindFirstNonNullTail(Message::kPriorityLow); }

void PriorityQueue::Enqueue(Message &aMessage)
{
    Message::Priority priority;
    Message          *tail;
    Message          *next;

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
    Message          *tail;

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

    aMessage.SetPriorityQueue(nullptr);
}

void PriorityQueue::DequeueAndFree(Message &aMessage)
{
    Dequeue(aMessage);
    aMessage.Free();
}

void PriorityQueue::DequeueAndFreeAll(void)
{
    Message *message;

    while ((message = GetHead()) != nullptr)
    {
        DequeueAndFree(*message);
    }
}

Message::Iterator PriorityQueue::begin(void) { return Message::Iterator(GetHead()); }

Message::ConstIterator PriorityQueue::begin(void) const { return Message::ConstIterator(GetHead()); }

void PriorityQueue::GetInfo(Info &aInfo) const
{
    for (const Message &message : *this)
    {
        aInfo.mNumMessages++;
        aInfo.mNumBuffers += message.GetBufferCount();
        aInfo.mTotalBytes += message.GetLength();
    }
}

} // namespace ot
#endif // OPENTHREAD_MTD || OPENTHREAD_FTD
